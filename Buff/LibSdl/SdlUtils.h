#pragma once
#include "Lib/BoundingBox.h"
#include "Lib/containers/Array2D.h"
#include "Lib/Filesystem.h"
#include "Lib/Path.h"
#include "Lib/Pixel.h"
#include "Lib/Rgb8Bit.h"
#include "Lib/String.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

BUFF_NAMESPACE_BEGIN

class SdlTexture;

SDL_Surface*      bitmapToSurface(const Array2D<Rgb8Bit>& bitmap);
SDL_Surface*      bitmapToSurface(const Array2D<Rgba8Bit>& bitmap);
Array2D<Rgba8Bit> surfaceToBitmap(SDL_Surface* surface);

inline SDL_Color toSdl(const Rgb8Bit& input) {
    return {input[0], input[1], input[2], 255};
}
inline SDL_Color toSdl(const Rgba8Bit& input) {
    return {input[0], input[1], input[2], input.alpha()};
}

inline SDL_Rect toSdl(const BoundingBox2I& rectangle) {
    SDL_Rect result;
    result.x = rectangle.getTopLeft().x;
    result.y = rectangle.getTopLeft().y;
    result.w = rectangle.getSize().x;
    result.h = rectangle.getSize().y;
    return result;
}

inline BoundingBox2I fromSdl(const SDL_Rect& rectangle) {
    return BoundingBox2I(Pixel(rectangle.x, rectangle.y), Pixel(rectangle.w, rectangle.h));
}

class SdlFont : public Noncopyable {
    TTF_Font* mFont = nullptr;

public:
    void init(const FilePath& path, const int size) {
        BUFF_ASSERT(fileExists(path));
        mFont = TTF_OpenFont(path.getNative().asCString(), size);
        BUFF_ASSERT(mFont);
    }

    TTF_Font* getResource() const {
        return mFont;
    }

    ~SdlFont() {
        TTF_CloseFont(mFont);
    }
};

BUFF_NAMESPACE_END
