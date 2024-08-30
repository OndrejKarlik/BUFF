#pragma once
#include "Lib/AutoPtr.h"
#include "Lib/containers/Array2D.h"
#include "Lib/Rgb8Bit.h"

// ReSharper disable once CppInconsistentNaming
struct SDL_Surface;

BUFF_NAMESPACE_BEGIN

class FilePath;
class SdlTexture;

void libImageStart();
void libImageShutdown();

class Image : NoncopyableMovable {
    struct Impl;
    AutoPtr<Impl> mImpl;

public:
    explicit Image(const FilePath& path);
    Image(Image&& BUFF_UNUSED(from))            = default;
    Image& operator=(Image&& BUFF_UNUSED(from)) = default;
    ~Image();

    SDL_Surface* getSurface() const;

    Array2D<Rgba8Bit> getBitmap() const;

    SdlTexture toSdlTexture() &&;

    // =======================================================================================================
    // To/From file formats
    // =======================================================================================================

    static Image fromFile(const ArrayView<const std::byte>& data);

    void saveToFile(const FilePath& path) const;

private:
    Image();
};

BUFF_NAMESPACE_END
