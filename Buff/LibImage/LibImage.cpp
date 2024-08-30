#include "LibImage.h"
#include "LibSdl/LibSdl.h"
#include "LibSdl/SdlTexture.h"
#include "LibSdl/SdlUtils.h"
#include "Lib/Exception.h"
#include "Lib/Path.h"
#include "Lib/String.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

BUFF_NAMESPACE_BEGIN

static int gStartedCalls = 0;

[[maybe_unused]] static bool isInitialized() {
    return gStartedCalls > 0;
}

void libImageStart() {
    BUFF_ASSERT(!isInitialized());
    constexpr int TO_INITIALIZE = IMG_INIT_PNG | IMG_INIT_JPG;
    BUFF_CHECKED_CALL(TO_INITIALIZE, IMG_Init(TO_INITIALIZE));
    ++gStartedCalls;
}
void libImageShutdown() {
    BUFF_ASSERT(gStartedCalls == 1);
    IMG_Quit();
    --gStartedCalls;
}

struct Image::Impl {
    SDL_Surface* surface = nullptr;
};

Image::Image()
    : mImpl(ALLOCATE_DEFAULT_CONSTRUCTED) {}

Image::Image(const FilePath& path)
    : Image() {
    mImpl->surface = IMG_Load(path.getNative().asCString());
    if (!mImpl->surface) {
        throw Exception("IMG_Load"_S + IMG_GetError());
    }
}

Image::~Image() {
    if (mImpl->surface) {
        SDL_FreeSurface(mImpl->surface);
    }
}

SDL_Surface* Image::getSurface() const {
    return mImpl->surface;
}

Array2D<Rgba8Bit> Image::getBitmap() const {
    return surfaceToBitmap(mImpl->surface);
}

SdlTexture Image::toSdlTexture() && {
    SDL_Surface* surface = mImpl->surface;
    mImpl->surface       = nullptr;
    return SdlTexture::fromSurface(surface);
}

// ===========================================================================================================
// To/From file formats
// ===========================================================================================================

Image Image::fromFile(const ArrayView<const std::byte>& data) {
    BUFF_ASSERT(isInitialized());
    Image result;

    SDL_RWops* ops        = SDL_RWFromConstMem(data.data(), safeIntegerCast<int>(data.size()));
    result.mImpl->surface = IMG_Load_RW(ops, SDL_TRUE);
    if (!result.mImpl->surface) {
        throw Exception("IMG_Load_RW"_S + IMG_GetError());
    }
    return result;
}

void Image::saveToFile(const FilePath& path) const {
    BUFF_ASSERT(isInitialized());
    const String extension = path.getExtension()->getToLower();

    [[maybe_unused]] int result = -1;
    if (extension == "png") {
        result = IMG_SavePNG(mImpl->surface, path.getGeneric().asCString());
    } else if (extension == anyOf("jpg", "jpeg")) {
        result = IMG_SaveJPG(mImpl->surface, path.getGeneric().asCString(), 95);
    } else {
        BUFF_STOP;
    }
    BUFF_ASSERT(result == 0);
}

BUFF_NAMESPACE_END
