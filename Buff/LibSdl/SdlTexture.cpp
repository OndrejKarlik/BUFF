#include "LibSdl/SdlTexture.h"
#include "LibSdl/SdlUtils.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

SdlTexture::SdlTexture(const Array2D<Rgb8Bit>& pixels)
    : mSurface(bitmapToSurface(pixels))
    , mSize(pixels.size()) {
    BUFF_ASSERT(pixels.getPixelCount() > 0);
}

SdlTexture::SdlTexture(const Array2D<Rgba8Bit>& pixels)
    : mSurface(bitmapToSurface(pixels))
    , mSize(pixels.size()) {
    BUFF_ASSERT(pixels.getPixelCount() > 0);
}

SdlTexture::SdlTexture(const Rgb8Bit solidColor)
    : mSize(Pixel::ONE()) {
    Array2D<Rgb8Bit> dummyArray(Pixel::ONE());
    dummyArray[Pixel::ZERO()] = solidColor;
    mSurface                  = bitmapToSurface(dummyArray);
}

SdlTexture::SdlTexture(SDL_Surface* surface)
    : mSurface(surface) {
    BUFF_ASSERT(mSurface);
    mSize = {mSurface->w, mSurface->h};
    BUFF_ASSERT(mSize.x > 0 && mSize.y > 0);
}

SdlTexture::SdlTexture(const SdlFont& font, const String& text, const Rgba8Bit textColor)
    : mSurface(TTF_RenderUTF8_Blended(font.getResource(), text.asCString(), toSdl(textColor))) {
    BUFF_ASSERT(mSurface);
    mSize = {mSurface->w, mSurface->h};
    BUFF_ASSERT(mSize.x > 0 && mSize.y > 0);
}

SdlTexture SdlTexture::fromSurface(SDL_Surface* surface) {
    return SdlTexture(surface);
}

void SdlTexture::draw(const BoundingBox2I where, SDL_Renderer* renderer) const {
    if (!mTexture) {
        finishInitialization(renderer);
    }
    const SDL_Rect rect = toSdl(where);
    BUFF_CHECKED_CALL(0, SDL_RenderCopy(renderer, mTexture, nullptr, &rect));
}

void SdlTexture::draw(const Pixel where, SDL_Renderer* renderer) const {
    return draw(BoundingBox2I(where, getResolution()), renderer);
}

Array2D<Rgba8Bit> SdlTexture::toBitmap() const {
    return surfaceToBitmap(mSurface);
}

void SdlTexture::finishInitialization(SDL_Renderer* renderer) const {
    BUFF_ASSERT(mSurface);
    mTexture = SDL_CreateTextureFromSurface(renderer, mSurface);
    BUFF_ASSERT(mTexture);
}

AutoPtr<SdlTexture> SdlTexture::layeredDraw(const SdlTexture&   base,
                                            const SdlTexture&   top,
                                            const BoundingBox2I topRectangle) {
    AutoPtr<SdlTexture> result = makeAutoPtr<SdlTexture>(base.toBitmap());
    SDL_Rect            topSdl = toSdl(topRectangle);
    BUFF_CHECKED_CALL(0, SDL_BlitScaled(top.mSurface, nullptr, result->mSurface, &topSdl));
    return result;
}

BUFF_NAMESPACE_END
