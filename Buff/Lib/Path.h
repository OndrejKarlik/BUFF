#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/String.h"
#include <filesystem>

BUFF_NAMESPACE_BEGIN

class DirectoryPath;

class FilePath {
    String mImpl;

public:
    FilePath() = default;
    explicit FilePath(String path)
        : mImpl(std::move(path)) {
        normalizeInternal();
    }
    // ReSharper disable once CppNonExplicitConvertingConstructor
    FilePath(const std::filesystem::path& path);

    explicit FilePath(const char* utf8)
        : FilePath(String(utf8)) {}

    // ReSharper disable once CppNonExplicitConversionOperator
    operator std::filesystem::path() const;

    bool isAbsolute() const {
        return mImpl.tryGet(1) == ':';
    }

    bool isRelative() const {
        return !isAbsolute();
    }

    String getNative() const {
        return mImpl.getWithReplaceAll("/", "\\");
    }

    const String& getGeneric() const {
        return mImpl;
    }

    std::strong_ordering operator<=>(const FilePath& other) const {
        if (mImpl < other.mImpl) {
            return std::strong_ordering::less;
        } else if (other.mImpl < mImpl) {
            return std::strong_ordering::greater;
        } else {
            return std::strong_ordering::equal;
        }
    }

    friend std::ostream& operator<<(std::ostream& stream, const FilePath& value) {
        stream << value.getGeneric();
        return stream;
    }

    bool operator==(const FilePath& other) const {
        return mImpl.getToLower() == other.mImpl.getToLower();
    }

    bool operator<(const FilePath& other) const {
        return mImpl.getToLower() < other.mImpl.getToLower();
    }

    void serializeCustom(ISerializer& serializer) const {
        serializer.serialize(getGeneric(), "generic");
    }
    void deserializeCustom(IDeserializer& deserializer) {
        String generic;
        deserializer.deserialize(generic, "generic");
        *this = FilePath(generic);
    }

    // =======================================================================================================
    // Parsing
    // =======================================================================================================

    Optional<DirectoryPath> getDirectory() const;

    StringView getFilenameWithExtension() const {
        if (const auto pos = mImpl.findLastOf("/")) {
            return mImpl.getSubstring(*pos + 1);
        } else {
            return mImpl;
        }
    }
    StringView getFilenameWithoutExtension() const {
        const StringView withExtension = getFilenameWithExtension();
        if (const auto pos = withExtension.findLastOf(".")) {
            return withExtension.getSubstring(0, *pos);
        } else {
            return withExtension;
        }
    }

    /// Returns the extension WITHOUT the dot
    Optional<StringView> getExtension() const {
        const StringView filenameWithExtension = getFilenameWithExtension();
        if (const auto fileName = filenameWithExtension.findLastOf(".")) {
            return filenameWithExtension.getSubstring(*fileName + 1);
        } else {
            return NULL_OPTIONAL;
        }
    }

private:
    void normalizeInternal() {
        mImpl.replaceAll("\\", "/");
        BUFF_ASSERT(!mImpl.startsWith("/") && !mImpl.endsWith("/"));
    }
};

inline FilePath operator""_File(const char* utf8, const size_t size) {
    return FilePath(String(utf8, int(size)));
}

class DirectoryPath {
    String mImpl;

public:
    DirectoryPath() = default;
    explicit DirectoryPath(String path)
        : mImpl(std::move(path)) {
        normalizeInternal();
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    DirectoryPath(const std::filesystem::path& path);

    explicit DirectoryPath(const char* utf8)
        : DirectoryPath(String(utf8)) {}

    // ReSharper disable once CppNonExplicitConversionOperator
    operator std::filesystem::path() const;

    bool isAbsolute() const {
        return mImpl.tryGet(1) == ':';
    }

    bool isRelative() const {
        return !isAbsolute();
    }

    DirectoryPath operator/(const DirectoryPath& path) const {
        BUFF_ASSERT(path.isRelative());
        return DirectoryPath(mImpl + path);
    }

    FilePath operator/(const FilePath& path) const {
        BUFF_ASSERT(path.isRelative());
        return FilePath(mImpl + path);
    }

    String getNative() const {
        return mImpl.getWithReplaceAll("/", "\\");
    }

    const String& getGeneric() const {
        return mImpl;
    }

    bool operator==(const DirectoryPath& other) const {
        return mImpl.getToLower() == other.mImpl.getToLower();
    }

    bool operator<(const DirectoryPath& other) const {
        return mImpl.getToLower() < other.mImpl.getToLower();
    }

    friend std::ostream& operator<<(std::ostream& stream, const DirectoryPath& value) {
        stream << value.getGeneric();
        return stream;
    }

    void serializeCustom(ISerializer& serializer) const {
        serializer.serialize(getGeneric(), "generic");
    }
    void deserializeCustom(IDeserializer& deserializer) {
        String generic;
        deserializer.deserialize(generic, "generic");
        *this = DirectoryPath(generic);
    }

    // =======================================================================================================
    // Parsing
    // =======================================================================================================

    Optional<DirectoryPath> getParentFolder() const {
        if (const auto split = splitLastSegment()) {
            return DirectoryPath(split->first);
        } else {
            return NULL_OPTIONAL;
        }
    }

    StringView getLastSegment() const {
        StringView result;
        if (const auto split = splitLastSegment()) {
            result = split->second;
        } else {
            result = mImpl;
        }
        if (result.isEmpty()) {
            return result;
        } else {
            BUFF_ASSERT(result.endsWith("/"));
            return result.getSubstring(0, result.size() - 1);
        }
    }

private:
    Optional<std::pair<StringView, StringView>> splitLastSegment() const {
        if (mImpl.notEmpty()) {
            BUFF_ASSERT(mImpl.endsWith("/"));
            if (const auto lastSlash = mImpl.getSubstring(0, mImpl.size() - 1).findLastOf("/")) {
                return std::pair(mImpl.getSubstring(0, *lastSlash), mImpl.getSubstring(*lastSlash + 1));
            }
        }
        return NULL_OPTIONAL;
    }

    void normalizeInternal() {
        mImpl.replaceAll("\\", "/");
        if (!mImpl.endsWith("/")) {
            mImpl << "/";
        }
        BUFF_ASSERT(!mImpl.startsWith("/"));
    }
};

inline DirectoryPath operator""_Dir(const char* utf8, const size_t size) {
    return DirectoryPath(String(utf8, int(size)));
}

BUFF_NAMESPACE_END
