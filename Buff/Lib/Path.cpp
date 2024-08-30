#include "Lib/Path.h"

BUFF_NAMESPACE_BEGIN

// ===========================================================================================================
// FilePath
// ===========================================================================================================

FilePath::FilePath(const std::filesystem::path& path)
    : mImpl(path.native()) {
    // std::filesystem::path::native() does not replace backslashes...
    normalizeInternal();
}

FilePath::operator std::filesystem::path() const {
    if constexpr (BUFF_DEBUG) {
        if (mImpl.isAscii()) { // Faster in debug build...
            return std::filesystem::path(mImpl.asCString());
        }
    }
    return std::filesystem::path(mImpl.asWString());
}

Optional<DirectoryPath> FilePath::getDirectory() const {
    if (const auto lastSlash = mImpl.findLastOf("/")) {
        return DirectoryPath(mImpl.getSubstring(0, *lastSlash));
    } else {
        return NULL_OPTIONAL;
    }
}

// ===========================================================================================================
// DirectoryPath
// ===========================================================================================================

DirectoryPath::DirectoryPath(const std::filesystem::path& path)
    : mImpl(path.native()) {
    // std::filesystem::path::native() does not replace backslashes...
    normalizeInternal();
}

DirectoryPath::operator std::filesystem::path() const {
    return std::filesystem::path(mImpl.asWString());
}

BUFF_NAMESPACE_END
