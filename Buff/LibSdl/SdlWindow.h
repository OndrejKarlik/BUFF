#pragma once
#include "Lib/AutoPtr.h"
#include "Lib/Flags.h"
#include "Lib/FpsMeasurement.h"
#include "Lib/Pixel.h"
#include "Lib/String.h"
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

BUFF_NAMESPACE_BEGIN

inline constexpr auto DEFAULT_SDL_WINDOW_FLAGS = SDL_WINDOW_ALLOW_HIGHDPI;

// To be implemented by the user
class ISdlWindowContent : public Polymorphic {
public:
    /// Called when there is some pending event to handle
    ///
    /// Does NOT fire for SDL_QUIT event - that one is already pre-filtered out and handled separately
    virtual void processEvent(const SDL_Event& event) = 0;

    /// Called on "idle" iterations, when there are no more events to handle. Used for animation and
    /// timer-based stuff.
    virtual void tick(Duration timeSinceLast) = 0;

    /// Called after processEvent(s) and tick, meant for updating rendered content
    virtual void draw() = 0;

    virtual void resize(const Pixel& newSize) = 0;

    virtual BinarySerializationState saveForReload() const                                 = 0;
    virtual void                     reloadFromSave(const BinarySerializationState& state) = 0;
};

class SdlWindow
    : public Polymorphic
    , public Noncopyable {
protected:
    SDL_Window*                mWindow = nullptr;
    AutoPtr<ISdlWindowContent> mContent;

private:
    SDL_Renderer* mRenderer = nullptr;
    Pixel         mSize;
    TimeStamp     mLastTick = TimeStamp::now();
    String        mWindowTitle;
    mutable int   mLastRenderingTimeMs = 0;

    FpsMeasurement mFpsMeasurement;

    bool mQuitting = false;
#ifndef __EMSCRIPTEN__
    std::atomic_bool mInterrupted;
#endif

public:
    SdlWindow(const Pixel                  size,
              const String&                title,
              const Flags<SDL_WindowFlags> flags = DEFAULT_SDL_WINDOW_FLAGS)
        : mSize(size)
        , mWindowTitle(title) {
        BUFF_CHECKED_CALL(
            0,
            SDL_CreateWindowAndRenderer(mSize.x, mSize.y, flags.toUnderlying(), &mWindow, &mRenderer));
        SDL_SetWindowTitle(mWindow, title.asCString());
        if constexpr (BUFF_DEBUG) {
            Pixel check;
            BUFF_CHECKED_CALL(0, SDL_GetRendererOutputSize(mRenderer, &check.x, &check.y));
            BUFF_ASSERT(check.x == size.x && check.y == size.y);
        }
    }

    void setContent(AutoPtr<ISdlWindowContent> content) {
        mContent = std::move(content);
        if (mContent) {
            mContent->resize(getWindowSize());
        }
    }
    const ISdlWindowContent& getContent() const {
        return *mContent;
    }

    virtual ~SdlWindow() override {
        mContent.deleteResource();
        SDL_DestroyRenderer(mRenderer);
        SDL_DestroyWindow(mWindow);
    }

    Pixel getWindowSize() const {
        return mSize;
    }

    void run() {
#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop_arg(loopIteration, this, -1, 1);
#else
        mInterrupted = false;
        while (!mQuitting && !mInterrupted) {
            // ImGui does automatic rate limiting, SDL implementation would need to do it itself
            loopIteration(this);
        }
#endif
    }

    /// Causes run() to be aborted. Should be called from a secondary thread.
    void interruptRun() {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#else
        mInterrupted = true;
#endif
    }

    /// Returns true if the user closed the window
    bool wasClosed() const {
        return mQuitting;
    }

    // =======================================================================================================
    // Private methods
    // =======================================================================================================

    /// Called repeatedly either by us (windows), or by emscripten
    static void loopIteration(void* arg) {
        auto*     window = static_cast<SdlWindow*>(arg);
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                window->mQuitting = true;
#ifdef __EMSCRIPTEN__
                emscripten_cancel_main_loop();
#endif
                return;
            }
            window->dispatchProcessEvent(e);
        }
        const auto lastTick = window->mLastTick;
        window->mLastTick   = TimeStamp::now();
        window->mContent->tick(window->mLastTick - lastTick);

        window->dispatchDraw();

        window->mFpsMeasurement.registerTick();
        const String currentTitle = window->mWindowTitle + " (" +
                                    toStr(window->mFpsMeasurement.getFps(1000).valueOr(-1)) + " fps, " +
                                    toStr(window->mLastRenderingTimeMs) + "ms draw)";
        SDL_SetWindowTitle(window->mWindow, currentTitle.asCString());
    }

protected:
    // =======================================================================================================
    // Stuff potentially overriden in derived classes. TODO: ugly, remove when SdlWindow is not used (after
    // scrabble is ported to ImGui)
    // =======================================================================================================

    virtual void dispatchProcessEvent(const SDL_Event& event) {
        mContent->processEvent(event);
    }
    virtual void dispatchDraw() const {
        mContent->draw();
    }
    void setLastRenderingTime(const int timeMs) const {
        mLastRenderingTimeMs = timeMs;
    }
};

BUFF_NAMESPACE_END
