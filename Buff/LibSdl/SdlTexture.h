#pragma once
#include "Lib/AutoPtr.h"
#include "Lib/BoundingBox.h"
#include "Lib/containers/Array2D.h"
#include "Lib/Pixel.h"
#include "Lib/Rgb8Bit.h"
#include <SDL2/SDL.h>

BUFF_NAMESPACE_BEGIN

class SdlFont;

class SdlTexture : public Noncopyable {
    SDL_Surface*         mSurface = nullptr;
    mutable SDL_Texture* mTexture = nullptr; // lazy init

    Pixel mSize;

public:
    explicit SdlTexture(const Array2D<Rgb8Bit>& pixels);

    explicit SdlTexture(const Array2D<Rgba8Bit>& pixels);

    explicit SdlTexture(Rgb8Bit solidColor);

    SdlTexture(const SdlFont& font, const String& text, Rgba8Bit textColor);

    /// Takes ownership
    static SdlTexture fromSurface(SDL_Surface* surface);

    static AutoPtr<SdlTexture> layeredDraw(const SdlTexture& base,
                                           const SdlTexture& top,
                                           BoundingBox2I     topRectangle);

    static AutoPtr<SdlTexture> layeredDraw(const SdlTexture& base,
                                           const SdlTexture& top,
                                           const Pixel       topWhere) {
        return layeredDraw(base, top, BoundingBox2I(topWhere, top.getResolution()));
    }

    ~SdlTexture() {
        BUFF_ASSERT(mSurface);
        if (mTexture) {
            SDL_DestroyTexture(mTexture);
        }
        SDL_FreeSurface(mSurface);
    }

    void draw(BoundingBox2I where, SDL_Renderer* renderer) const;

    // Draws with texture's native size
    void draw(Pixel where, SDL_Renderer* renderer) const;

    Array2D<Rgba8Bit> toBitmap() const;

    Pixel getResolution() const {
        return mSize;
    }

private:
    /// Takes ownership
    explicit SdlTexture(SDL_Surface* surface);

    /// Copies stuff from CPU to GPU, initializes both mSurface and mTexture. Done lazily, when Renderer
    /// becomes available
    void finishInitialization(SDL_Renderer* renderer) const;
};

BUFF_NAMESPACE_END
