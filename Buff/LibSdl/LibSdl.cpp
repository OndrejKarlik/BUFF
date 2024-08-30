#include "LibSdl/LibSdl.h"
#include "Lib/String.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

BUFF_NAMESPACE_BEGIN

void sdlStart() {
    BUFF_CHECKED_CALL(0, SDL_Init(SDL_INIT_VIDEO));
    BUFF_CHECKED_CALL(0, TTF_Init());

    // Does not seem to do anything:
    BUFF_CHECKED_CALL(SDL_TRUE, SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2"));
    BUFF_CHECKED_CALL(SDL_TRUE, SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1"));

    BUFF_CHECKED_CALL(0, SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1));
    BUFF_CHECKED_CALL(0, SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2));

    BUFF_CHECKED_CALL(0, SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1));
}
void sdlShutdown() {
    TTF_Quit();
    SDL_Quit();
}

BUFF_NAMESPACE_END
