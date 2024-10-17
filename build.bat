@echo off
start /wait ./venv/Scripts/python.exe shader.py
if %errorlevel% == 2 (
    echo Error compiling shaders!
    exit
)
tcc -c ./src/include/gl.c -o ./build/gl.o
tcc -c ./src/include/mt19937ar.c -o ./build/rng.o
tcc -c ./src/main.c -o ./build/main.o
tcc ./build/main.o ./build/gl.o ./build/rng.o -o "tangram.exe" -Wall -lSDL2 -lbass -lSDL2main -Wl,-subsystem=windows
if %errorlevel% == 0 (
    .\tangram.exe
) else (
    echo Error compiling game!
)
