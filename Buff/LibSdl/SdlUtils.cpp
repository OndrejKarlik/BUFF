#include "LibSdl/SdlUtils.h"
#include "Lib/Utils.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
SDL_Surface* bitmapToSurfaceImpl(const Array2D<T>& bitmap) {
    static_assert(sizeof(T) == 3 || sizeof(T) == 4);
    // C API... -> const cast
    SDL_Surface* surface = SDL_CreateRGBSurface(0,
                                                bitmap.size().x,
                                                bitmap.size().y,
                                                sizeof(T) * 8,
                                                0xFF,
                                                0xFFu << 8,
                                                0xFFu << 16,
                                                0xFFu << 24);
    BUFF_CHECKED_CALL(0, SDL_LockSurface(surface));
    T* dst = static_cast<T*>(surface->pixels);
    for (int y = 0; y < bitmap.size().y; ++y) {
        for (int x = 0; x < bitmap.size().x; ++x) {
            dst[x] = bitmap(x, y);
        }
        // The surface can have custom row length, not equal to sizeof(pixel)*numPixels
        dst = addBytesToPointer(dst, surface->pitch);
    }
    SDL_UnlockSurface(surface);
    return surface;
}

SDL_Surface* bitmapToSurface(const Array2D<Rgb8Bit>& bitmap) {
    static_assert(sizeof(Rgb8Bit) == 3);
    return bitmapToSurfaceImpl(bitmap);
}

SDL_Surface* bitmapToSurface(const Array2D<Rgba8Bit>& bitmap) {
    static_assert(sizeof(Rgba8Bit) == 4);
    return bitmapToSurfaceImpl(bitmap);
}

Array2D<Rgba8Bit> surfaceToBitmap(SDL_Surface* surface) {
    BUFF_ASSERT(surface);
    BUFF_CHECKED_CALL(0, SDL_LockSurface(surface));
    Array2D<Rgba8Bit> result(surface->w, surface->h);

    auto copy = [&result, surface](auto* src) {
        for (int y = 0; y < result.size().y; ++y) {
            for (int x = 0; x < result.size().x; ++x) {
                result(x, y) = Rgba8Bit(src[x]);
            }
            // The surface can have custom row length, not equal to sizeof(pixel)*numPixels
            src = addBytesToPointer(src, surface->pitch);
        }
    };

    if (surface->format->BytesPerPixel == 4) {
        copy(static_cast<Rgba8Bit*>(surface->pixels));
    } else {
        BUFF_ASSERT(surface->format->BytesPerPixel == 3);
        copy(static_cast<Rgb8Bit*>(surface->pixels));
    }
    SDL_UnlockSurface(surface);

    BUFF_ASSERT(result.getPixelCount() > 0);
    return result;
}

BUFF_NAMESPACE_END
