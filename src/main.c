#include "tangram.h"
#include "shader.h"

bool tangram_test_pointer(void *ptr)
{
    if (ptr != NULL)
        return true;
    fprintf(stderr, "Pointer \"%s\" test FAILED! %s\n", ptr, SDL_GetError());
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pointer \"%s\" test FAILED! %s\n", ptr, SDL_GetError());

    return false;
}

void init_screen()
{
    size_t size = width * height * sizeof(unsigned int);
    tangram.screen = malloc(size);
    memset(tangram.screen, 0, size);
}

static void reset_viewport()
{
    int ww, wh;
    SDL_GetWindowSize(tangram.window, &ww, &wh);
    glViewport(0, 0, ww, wh);
}

static void enable_2d()
{
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    reset_viewport();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static void disable_2d()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

void init_clock(TangramClock *clock)
{
    // clock->now = SDL_GetPerformanceCounter();
    clock->now = SDL_GetTicks64();
    clock->last = 0;
    clock->dt = 0;
    clock->desired_delta = 1000 / fps;
    clock->tick = 0;
}

void tick_start(TangramClock *clock)
{
    clock->last = SDL_GetTicks64();
}

void tick_end(TangramClock *clock)
{
    // clock->dt = (double)((clock->now - clock->last) / (double)SDL_GetPerformanceFrequency());
    clock->now = SDL_GetTicks64();
    clock->dt = clock->now - clock->last;
    clock->last = clock->now;
    clock->tick++;
}

bool key_is_down(SDL_KeyCode key)
{
    return tangram.keystate[SDL_GetScancodeFromKey(key)];
}

bool key_is_pressed(SDL_KeyCode key)
{
    if (key_is_down(key) && !tangram.pressed.keys[key])
    {
        tangram.pressed.keys[key] = true;
        return true;
    }
    return false;
}

bool key_is_up(SDL_KeyCode key)
{
    return !tangram.keystate[SDL_GetScancodeFromKey(key)];
}

TangramTexture *new_texture(const char *filename)
{
    TangramTexture *t = malloc(sizeof(TangramTexture));
    t->data = stbi_load(filename, &t->w, &t->h, &t->channels, STBI_rgb_alpha);
    if (t->data)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Created texture from %s!", filename);
        return t;
    }
    return NULL;
}

void free_textures()
{
    stbi_image_free(tangram.textures.logo);
}

void draw_clear(unsigned int color)
{
    unsigned int *p = tangram.screen;
    unsigned int *end = tangram.screen + width * height;
    unsigned int c = RGB_TO_BGR(color);
    while (p < end)
        *p++ = c;
}

void draw_pixel(int x, int y, unsigned int color)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        unsigned int *pixel = &tangram.screen[((height - 1 - y) * width) + x];
        unsigned int rgb = RGB_TO_BGR(color);
        if (*pixel != rgb)
        {
            *pixel = rgb;
            free(pixel);
        }
        /*
        glViewport(0, 0, width, height);
        glUseProgram(tangram.gl.program);
        glUniform4f(
            tangram.gl.color_uniform,
            ((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f,
            (color & 0xFF) / 255.0f, 1.0f);

        glBegin(GL_POINTS);
        glVertex2f((float)x, (float)y);
        glEnd();

        reset_viewport();
        */
    }
}

void draw_line(Point from, Point to, unsigned int color)
{
    // Bresenham's line algorithm
    float dx = fabs(to.x - from.x);
    float dy = fabs(to.y - from.y);
    float sx = from.x < to.x ? 1.0f : -1.0f;
    float sy = from.y < to.y ? 1.0f : -1.0f;
    float err = dx - dy;

    for (int i = 0; i <= fmaxf(dx, dy); i++)
    {
        draw_pixel((int)(from.x + 0.5f), (int)(from.y + 0.5f), color);
        if (from.x == to.x && from.y == to.y)
            break;
        int e2 = 2.0f * err;
        if (e2 > -dy)
        {
            err -= dy;
            from.x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            from.y += sy;
        }
    }
}

void draw_rectangle(Point from, Point to, unsigned int color, bool outline)
{
    const Point A = (Point){from.x, from.y};
    const Point B = (Point){to.x, from.y};
    const Point C = (Point){to.x, to.y};
    const Point D = (Point){from.x, to.y};
    if (outline)
    {
        draw_line(A, B, color);
        draw_line(B, C, color);
        draw_line(C, D, color);
        draw_line(D, A, color);
    }
    else
    {
        for (int rx = from.x; rx < to.x; rx++)
            for (int ry = from.y; ry < to.y; ry++)
                draw_pixel(rx, ry, color);
    }
}

void draw_texture(TangramTexture *texture, IPoint pos, IPoint uv, IPoint size, float scale, unsigned int blend)
{
    // the image will pretend to have 3 channels but all textures are loaded with 4
    int channels = 4;

    // Pixel where loop will start based on UV coords
    int offset = uv.y * texture->w * channels + uv.x;
    unsigned char *cursor = texture->data + offset;

    for (int ty = 0; ty < size.y; ty++)
    {
        for (int tx = 0; tx < size.x; tx++)
        {
            unsigned char r = cursor[0];
            unsigned char g = cursor[1];
            unsigned char b = cursor[2];
            // unsigned char a = cursor[3];

            unsigned int color = 0;
            color |= (r << 16);
            color |= (g << 8);
            color |= b;

            draw_pixel(pos.x + tx, pos.y + ty, color);
            cursor += channels;
        }
        cursor += (texture->w - size.x) * channels; // move to next row
    }
}

HSTREAM new_sound(const char *filename)
{
    if (filename != NULL)
    {
        return BASS_StreamCreateFile(0, filename, 0, 0, 0);
    }
}

void init_sounds()
{
    // sounds are initialized here
}

void play_sound(int id, float volume)
{
    HSTREAM sfx = tangram.sfx[id];
    if (BASS_ChannelIsActive(sfx) == BASS_ACTIVE_PLAYING)
        if (!BASS_ChannelStop(sfx))
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not stop sound #%d: %d", id, BASS_ErrorGetCode());
    BASS_ChannelSetAttribute(sfx, BASS_ATTRIB_VOL, volume);
    if (!BASS_ChannelPlay(sfx, 0))
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not play sound #%d: %d", id, BASS_ErrorGetCode());
}

void free_sounds()
{
    for (int i = 0; i < SOUND_AMOUNT; i++)
        BASS_StreamFree(tangram.sfx[i]);
}

bool every_n_frames(unsigned int frames)
{
    return tangram.clock.tick % frames == 0;
}

static int tangram_event_setup()
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    tangram.window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (!tangram.window)
    {
        fprintf(stderr, "Failed to create Tangram window: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
    SDL_SetWindowMinimumSize(tangram.window, 640, 480);

    init_screen();
    tangram.textures.logo = new_texture("data/img/logo.png");

    // GL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_EnableScreenSaver();
    atexit(SDL_Quit);

    tangram.gl.context = SDL_GL_CreateContext(tangram.window);
    SDL_GL_MakeCurrent(tangram.window, tangram.gl.context);
    if (gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) <= 0)
    {
        fprintf(stderr, "Failed to create GL context: %s\n", SDL_GetError());
        return false;
    }

    tangram.gl.program = glCreateProgram();

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
    glCompileShader(fragment_shader);

    glAttachShader(tangram.gl.program, vertex_shader);
    glAttachShader(tangram.gl.program, fragment_shader);

    glLinkProgram(tangram.gl.program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glGenVertexArrays(1, &tangram.gl.vertex_array);
    glBindVertexArray(tangram.gl.vertex_array);

    const GLVertex vertices[] = {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    };

    glGenBuffers(1, &tangram.gl.vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, tangram.gl.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, pos));
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, color));
    glEnableVertexAttribArray(1);
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void *)offsetof(GLVertex, tex_coord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    /*
    glGenTextures(1, &tangram.gl.framebuffer_texture);
    glBindTexture(GL_TEXTURE_2D, tangram.gl.framebuffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    glGenFramebuffers(1, &tangram.gl.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, tangram.gl.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tangram.gl.framebuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        SDL_Log("Framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    */
    // Main texture, uses software rendedrer
    glGenTextures(1, &tangram.gl.texture);
    glBindTexture(GL_TEXTURE_2D, tangram.gl.texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tangram.screen);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUseProgram(tangram.gl.program);
    glUniform1i(glGetUniformLocation(tangram.gl.program, "iTexture"), 0);

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(tangram.window, &wmi);
    HWND handle = wmi.info.win.window;

    if (!BASS_Init(-1, 48000, BASS_DEVICE_DEFAULT, handle, NULL))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize BASS: %d\n", BASS_ErrorGetCode());
        return false;
    }

    init_sounds();

    tangram.music = BASS_MusicLoad(0, "data/music/jazzberry juice edit.mod", 0, 0, BASS_SAMPLE_LOOP | BASS_MUSIC_NONINTER, 0);
    if (!tangram.music)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load music: %d\n", BASS_ErrorGetCode());
        return false;
    }

    init_genrand(SDL_GetTicks() + rand());
    init_clock(&tangram.clock);
    tangram.clock.dt = 1;

    BASS_ChannelSetAttribute(tangram.music, BASS_ATTRIB_VOL, 1.f);
    BASS_ChannelPlay(tangram.music, 1);

    return true;
}

static void tangram_event_update()
{
    tangram.keystate = SDL_GetKeyboardState(NULL);
    tick_start(&tangram.clock);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Key pressed: %u\n", event.key.keysym.sym);
            if (key_is_down(SDLK_ESCAPE))
                tangram.running = false;
            break;
        case SDL_KEYUP:
            tangram.pressed.keys[event.key.keysym.sym] = false;
            break;
        case SDL_QUIT:
            tangram.running = false;
            break;
        default:
            break;
        }
    }

    tick_end(&tangram.clock);

    float t = SDL_GetTicks() * 0.001f;
    glUniform1f(glGetUniformLocation(tangram.gl.program, "iTime"), t);

    int ww, wh;
    SDL_GetWindowSizeInPixels(tangram.window, &ww, &wh);
    glUniform3f(glGetUniformLocation(tangram.gl.program, "iResolution"), (float)ww, (float)wh, 0.f);
}

static void tangram_event_render()
{
    enable_2d();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    draw_clear(0x040305);

    IPoint logo_position = (IPoint){
        width * 0.5 - tangram.textures.logo->w * 0.5,
        height * 0.5 - tangram.textures.logo->h * 0.5,
    };
    IPoint logo_size = (IPoint){tangram.textures.logo->w, tangram.textures.logo->h};
    draw_texture(tangram.textures.logo, logo_position, (IPoint){0}, logo_size, 1.f, 0xFFFFFF);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tangram.gl.texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tangram.screen);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(tangram.gl.vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, tangram.gl.vertex_buffer);

    disable_2d();

    SDL_GL_SwapWindow(tangram.window);
}

static void tangram_event_exit()
{
    free_sounds();
    BASS_MusicFree(tangram.music);
    BASS_Free();
    glDeleteVertexArrays(1, &tangram.gl.vertex_array);
    glDeleteBuffers(1, &tangram.gl.vertex_buffer);
    glDeleteProgram(tangram.gl.program);
    glDeleteTextures(1, &tangram.gl.texture);
    free_textures();
    SDL_GL_DeleteContext(tangram.gl.context);
    SDL_DestroyWindow(tangram.window);
    SDL_Quit();
}


int main(int argc, char *argv[])
{
    tangram.running = tangram_event_setup();

    while (tangram.running)
    {
        tangram.debug.fps = tangram.clock.dt;

        tangram_event_update();
        tangram_event_render();

        if (tangram.clock.dt < tangram.clock.desired_delta)
            SDL_Delay(tangram.clock.desired_delta - tangram.clock.dt);
    }

    tangram_event_exit();

    return 0;
}
