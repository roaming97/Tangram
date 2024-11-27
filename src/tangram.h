#ifndef TANGRAM_HEADER
#define TANGRAM_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_ttf.h>
#ifdef __TINYC__
#undef main
#endif
#include <glad/gl.h>
#include <bass/bass.h>
#define STBI_NO_SIMD
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "include/mt19937ar.h"

// Engine macros

#define RGB_TO_BGR(c) ((c & 0xFF) << 16) | (c & 0xFF00) | ((c & 0xFF0000) >> 16)
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define sign(x) ((x < 0) ? -1 : 1)

// Game constants

static const unsigned int width = 960;
static const unsigned int height = 720;
static const unsigned int fps = 60;
static const unsigned char *title = "Tangram";

// Game loop events

// This event only runs once, it configures and initializes Tangram. Returns `true` if setup is successful, `false` otherwise.
static int tangram_event_setup();
// This event runs every frame of the application's lifetime, it processes input, updates engine variables and components.
static void tangram_event_update();
// This event runs every frame of the application's lifetime, it draws to the screen and processes graphics.
static void tangram_event_render();
// This event only runs once at the end of the application's lifetime, it makes sure to deinitialize and free everything gracefully.
static void tangram_event_exit();

void init_screen();

// Timing

typedef struct TangramClock
{
    Uint64 now;
    Uint64 last;
    Uint64 tick;
    Uint64 dt;
    Uint64 desired_delta;
} TangramClock;

void init_clock(TangramClock *clock);
void tick_start(TangramClock *clock);
void tick_end(TangramClock *clock);

// Keyboard input

typedef struct KeyboardMap
{
    int keys[322];
} KeyboardMap;

bool key_is_down(SDL_KeyCode key);
bool key_is_pressed(SDL_KeyCode key);
bool key_is_up(SDL_KeyCode key);

// Graphics

typedef struct GLVertex
{
  float pos[3];
  float color[3];
  float tex_coord[2];
} GLVertex;

typedef struct Point
{
	float x, y;
} Point;

typedef struct IPoint
{
    int x, y;
} IPoint;

typedef struct TangramTexture
{
    unsigned char *data;
    int w;
    int h;
    int channels;
} TangramTexture;

TangramTexture *new_texture(const char *filename);
void free_textures();

typedef struct TextureMap
{
    TangramTexture *logo;
} TextureMap;

void draw_clear(unsigned int color);
void draw_pixel(int x, int y, unsigned int color);
void draw_line(Point from, Point to, unsigned int color);
void draw_rectangle(Point from, Point to, unsigned int color, bool outline);
void draw_texture(TangramTexture *texture, IPoint pos, IPoint uv, IPoint size, float scale, unsigned int blend);

typedef struct TangramGL
{
    SDL_GLContext context;
    GLuint program;
    GLuint vertex_array, vertex_buffer;
    GLuint texture;
} TangramGL;

// This sets the GL viewport, this can be useful when performing graphics commands.
static void reset_viewport();
static void enable_2d();
static void disable_2d();

// Audio

#define SOUND_AMOUNT 0

HSTREAM new_sound(const char *filename);
void init_sounds();
void play_sound(int id, float volume);
void free_sounds();

// Debug

// Structure for debugging stats.
typedef struct TangramDebug
{
    double fps;
} TangramDebug;


/// The main Tangram instance.
///
/// This manager holds the window, GL context, application states, etc.
struct TangramManager
{
    SDL_Window *window;
    TangramGL gl;
    TangramClock clock;
    TangramDebug debug;
    const unsigned char *keystate;
    KeyboardMap pressed;
    unsigned int *screen;
    TextureMap textures;
    HMUSIC music;
    HSTREAM sfx[1]; // Change this to match SOUND_AMOUNT
    bool running;
} tangram;

// Utility

bool tangram_test_pointer(void *ptr);
bool every_n_frames(unsigned int frames);

#endif
