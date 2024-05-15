# Tangram

This is a very simple graphics engine written in C.

## Building

### Python

> [!IMPORTANT]
> This project requires at least Python **3.6**.

If you want to change the look of the game via the shaders (in the `shaders` folder), you must make sure that the Python script works since the shaders must be compiled into the game before compiling the game itself.

> [!TIP]
> Create a virtual environment then activate it:
> ```
> $ python3 -m venv venv
> ```

Activate on **Unix**
```
$ source venv/bin/activate
```

Activate on **Windows**
```
> .\venv\Scripts\activate
```

Install the script's dependencies:
```
$ pip install -r requirements.txt
```

Make sure the script is working:
```
python3 shader.py
```

If the script runs and compiles the shaders correctly, you're all set!

### C

I compiled this game using TCC using the [build.bat](build.bat) script, but if you wish to use another compiler, you must link the following libraries:

- SDL2
- SDL2Main
- bass

If you are on Windows, be sure to include `bass.dll` and `SDL2.dll` alongside the game's executable.

TODO: For now the script will compile an EXE for Windows. Multitarget Makefile is still pending.

## Contributing

Feel free to report bugs, request features, or even implement them. You can do this last one by:

- Forking the repository
- Making the desired changes or add the desired features
- Open a pull request in this repository
- I'll review it and if it doesn't cause any conflicts with this codebase I'll merge it!
