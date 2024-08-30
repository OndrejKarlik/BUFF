REM call .tools/emscripten/emsdk_env

@echo on

REM rmdir ..\.build-emscripten /S /Q

mkdir ..\.build-emscripten

cd ..\.build-emscripten

set LIB_FILES=
REM ../Lib/Assert.cpp ../Lib/Filesystem.cpp ../Lib/Misc.cpp ../Lib/Path.cpp ../Lib/Platform.cpp ../Lib/String.cpp
set LIB_SDL_FILES=
REM ../LibSdl/LibSdl.cpp ../LibSdl/SdlUtils.cpp ../LibSdl/SdlWindow.cpp ../LibSdl/SdlTexture.cpp
set FILES= ../ImguiHelloWorld/ImguiHelloWorld.cpp ../LibImgui/3rdparty/imgui_impl_sdl2.cpp ../LibImgui/3rdparty/imgui_freetype.cpp ../.dependencies/installed/wasm32-emscripten/lib/libimgui.a
REM ../ChainExploder/ChainExploderBoard.cpp ../ChainExploder/ChainExploderWindow.cpp ../ChainExploder/ChainExploderMain.cpp ../ChainExploder/Grid.cpp


set EMSCRIPTEN_FLAGS=-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s WASM=1 -s SDL2_IMAGE_FORMATS=["png"] -s ALLOW_MEMORY_GROWTH=1 -s MAX_WEBGL_VERSION=3
REM -s MAX_WEBGL_VERSION=2
set CLANG_FLAGS=-O2 -std=c++2b
set DEFINES=-DBUFF_DEBUG=0 -DBUFF_RELEASE=1 -DIMGUI_USER_CONFIG=\"LibImgui/ImguiConfigInjection.h\"
set INCLUDES=-I.. -I..\.dependencies\installed\wasm32-emscripten\include\
set EMBED=--embed-file ../ImguiHelloWorld/assets

call ..\.tools\emscripten\upstream\emscripten\em++ %FILES% %LIB_FILES% %LIB_SDL_FILES% %INCLUDES% %DEFINES% %CLANG_FLAGS% %EMSCRIPTEN_FLAGS% %EMBED% -o imgui.html

if %errorLevel% GEQ 1 (
    exit /b 1
)

cd ..\ImguiHelloWorld