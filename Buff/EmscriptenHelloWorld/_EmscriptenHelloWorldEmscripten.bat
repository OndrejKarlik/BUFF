REM call .tools/emscripten/emsdk_env

@echo on

REM rmdir ..\.build-emscripten /S /Q

mkdir ..\.build-emscripten

cd ..\.build-emscripten

call ..\.tools\emscripten\upstream\emscripten\em++ ../EmscriptenHelloWorld/EmscriptenHelloWorld.cpp -Xclang -std=c++2b --embed-file ../EmscriptenHelloWorld/assets -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s WASM=1 -s SDL2_IMAGE_FORMATS=["png"] -o EmscriptenHelloWorld.html

if %errorLevel% GEQ 1 (
    exit /b 1
)

cd ..\EmscriptenHelloWorld\

REM -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.18362.0\um" -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.18362.0\shared" -I"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\Llvm\x64\lib\clang\10.0.0\include" -I"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.27.29110\include"