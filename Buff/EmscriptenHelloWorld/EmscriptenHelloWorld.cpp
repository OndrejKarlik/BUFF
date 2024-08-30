#include "LibImage/LibImage.h"
#include "LibSdl/LibSdl.h"
#include "Lib/Path.h"
#include <SDL2/SDL.h>
#include <thread>
#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

/**
 * Inverse square root of two, for normalizing velocity
 */
constexpr double REC_SQRT2 = 0.7071067811865475;

/**
 * Set of input states
 */
enum InputState {
    NOTHING_PRESSED = 0,
    UP_PRESSED      = 1,
    DOWN_PRESSED    = 1 << 1,
    LEFT_PRESSED    = 1 << 2,
    RIGHT_PRESSED   = 1 << 3
};

/**
 * Context structure that will be passed to the loop handler
 */
struct Context {
    SDL_Renderer* renderer;

    /**
     * Rectangle that the owl texture will be rendered into
     */
    SDL_Rect     dest;
    SDL_Texture* owlTex;

    int activeState;

    /**
     * x and y components of owl's velocity
     */
    int owlVx;
    int owlVy;
};

/**
 * Loads the owl texture into the context
 */
static int getOwlTexture(Context* ctx) {
    Buff::Image  loadedImage = Buff::Image(Buff::FilePath("assets/owl.png"));
    SDL_Surface* image       = loadedImage.getSurface();

    ctx->owlTex = SDL_CreateTextureFromSurface(ctx->renderer, image);
    ctx->dest.w = image->w;
    ctx->dest.h = image->h;

    SDL_FreeSurface(image);

    return 1;
}

#ifndef __EMSCRIPTEN__
static bool gIsQuitting = false;
#endif

/**
 * Processes the input events and sets the velocity
 * of the owl accordingly
 */
static void processInput(Context* ctx) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#else
            gIsQuitting = true;

#endif
        }
        switch (event.key.keysym.sym) {
        case SDLK_UP:
            if (event.key.type == SDL_KEYDOWN) {
                ctx->activeState |= int(UP_PRESSED);
            } else if (event.key.type == SDL_KEYUP) {
                ctx->activeState ^= int(UP_PRESSED);
            }
            break;
        case SDLK_DOWN:
            if (event.key.type == SDL_KEYDOWN) {
                ctx->activeState |= int(DOWN_PRESSED);
            } else if (event.key.type == SDL_KEYUP) {
                ctx->activeState ^= int(DOWN_PRESSED);
            }
            break;
        case SDLK_LEFT:
            if (event.key.type == SDL_KEYDOWN) {
                ctx->activeState |= int(LEFT_PRESSED);
            } else if (event.key.type == SDL_KEYUP) {
                ctx->activeState ^= int(LEFT_PRESSED);
            }
            break;
        case SDLK_RIGHT:
            if (event.key.type == SDL_KEYDOWN) {
                ctx->activeState |= int(RIGHT_PRESSED);
            } else if (event.key.type == SDL_KEYUP) {
                ctx->activeState ^= int(RIGHT_PRESSED);
            }
            break;
        default:
            break;
        }
    }

    ctx->owlVy = 0;
    ctx->owlVx = 0;
    if (ctx->activeState & UP_PRESSED) {
        ctx->owlVy = -5;
    }
    if (ctx->activeState & DOWN_PRESSED) {
        ctx->owlVy = 5;
    }
    if (ctx->activeState & LEFT_PRESSED) {
        ctx->owlVx = -5;
    }
    if (ctx->activeState & RIGHT_PRESSED) {
        ctx->owlVx = 5;
    }

    if (ctx->owlVx != 0 && ctx->owlVy != 0) {
        ctx->owlVx = int(ctx->owlVx * REC_SQRT2);
        ctx->owlVy = int(ctx->owlVy * REC_SQRT2);
    }
}

/**
 * Loop handler that gets called each animation frame,
 * process the input, update the position of the owl and
 * then render the texture
 */
static void loopHandler(void* arg) {
    Context* ctx = static_cast<Context*>(arg);

    processInput(ctx);

    ctx->dest.x += ctx->owlVx;
    ctx->dest.y += ctx->owlVy;

    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->owlTex, nullptr, &ctx->dest);
    SDL_RenderPresent(ctx->renderer);
}

#ifdef main
#    undef main
#endif
int main() {
    SDL_Window* window = nullptr;
    Context     ctx    = {};

    Buff::sdlStart();
    Buff::libImageStart();

    SDL_CreateWindowAndRenderer(600, 400, 0, &window, &ctx.renderer);
    SDL_SetRenderDrawColor(ctx.renderer, 255, 255, 255, 255);

    getOwlTexture(&ctx);
    ctx.activeState = NOTHING_PRESSED;
    ctx.dest.x      = 200;
    ctx.dest.y      = 100;
    ctx.owlVx       = 0;
    ctx.owlVy       = 0;

    /**
     * Schedule the main loop handler to get
     * called on each animation frame
     */
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(loopHandler, &ctx, -1, 1);
#else
    while (!gIsQuitting) {
        loopHandler(&ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
#endif

    Buff::libImageShutdown();
    Buff::sdlShutdown();

    return 0;
}
