#include "Lib/Filesystem.h"
#include "Lib/containers/StaticArray.h"
#include "Lib/Exception.h"
#include "Lib/Flags.h"
#include "Lib/Function.h"
#include "Lib/Optional.h"
#include "Lib/Path.h"
#include "Lib/String.h"
#include <filesystem>
#include <fstream>
#include <functional>

BUFF_NAMESPACE_BEGIN

// ===========================================================================================================
// Files
// ===========================================================================================================

bool fileExists(const FilePath& filename) {
    return std::filesystem::is_regular_file(filename);
}

bool removeFile(const FilePath& filename) {
    return std::filesystem::remove(filename);
}

bool copyFile(const FilePath& from, const FilePath& to) {
    return std::filesystem::copy_file(from, to);
}

void renameFile(const FilePath& from, const FilePath& to) {
    return std::filesystem::rename(from, to);
}

Optional<int64> fileSize(const FilePath& filename) {
    try {
        return int64(std::filesystem::file_size(filename));
    } catch (const std::filesystem::filesystem_error& BUFF_UNUSED(error)) {
        return NULL_OPTIONAL;
    }
}

Optional<String> readTextFile(const FilePath& filename) {
    // TODO: this is inefficient, but standard std::ifstream stops at character \x19 (END OF MEDIUM).
    if (const Optional<Array<std::byte>> data = readBinaryFile(filename)) {
        String result;
        result.reserve(int(data->size()));
        for (int i = 0; i < data->size(); ++i) {
            if ((*data)[i] == std::byte('\r')) {
                result << '\n';
                if (i + 1 < data->size() && (*data)[i + 1] == std::byte('\n')) {
                    ++i; // for windows newlines remove second character of the sequence
                }
            } else { // Batch append of regular chars for performance
                const int begin = i;
                while (i < data->size() - 1 && (*data)[i + 1] != std::byte('\r')) {
                    ++i;
                }
                result << StringView(reinterpret_cast<const char*>(data->data()) + begin, i - begin + 1);
            }
        }
        return result;
    } else {
        return NULL_OPTIONAL;
    }
}

Optional<Array<std::byte>> readBinaryFile(const FilePath& filename) {
    if (auto stream = std::ifstream(filename.getNative().asWString(), std::ios::ate | std::ios::binary)) {
        // ate == seek to the end of the stream
        const auto pos = stream.tellg();

        Array<std::byte> result(pos);
        stream.seekg(0);
        stream.read(reinterpret_cast<char*>(result.data()), pos);
        return result;
    } else {
        return NULL_OPTIONAL;
    }
}

bool writeTextFile(const FilePath& filename, const StringView text) {
    std::ofstream stream(filename.getNative().asWString(), std::ios::out);
    stream << text;
    return stream.good();
}

bool writeBinaryFile(const FilePath& filename, const ArrayView<const std::byte> data) {
    std::ofstream stream(filename.getNative().asWString(), std::ios::out | std::ios::binary);
    stream.write(reinterpret_cast<const char*>(data.data()), data.size());
    return stream.good();
}

bool areFilesIdentical(const FilePath& filename1, const FilePath& filename2) {
    if (fileSize(filename1) != fileSize(filename2)) {
        return false;
    }

    std::ifstream file1(filename1.getNative().asWString(), std::ios::in | std::ios::binary);
    std::ifstream file2(filename2.getNative().asWString(), std::ios::in | std::ios::binary);
    BUFF_ASSERT(file1.good() && file2.good());

    constexpr int                  BUFFER_SIZE = 64 * 1024;
    StaticArray<char, BUFFER_SIZE> buffer1;
    StaticArray<char, BUFFER_SIZE> buffer2;

    while (true) {
        file1.read(buffer1.data(), BUFFER_SIZE);
        file2.read(buffer2.data(), BUFFER_SIZE);
        const int read1 = int(file1.gcount());
        const int read2 = int(file2.gcount());

        BUFF_ASSERT(read1 == read2);

        if (read1 == 0) { // Both files are fully read
            BUFF_ASSERT(read2 == 0);
            return true;
        }

        if (!std::equal(buffer1.begin(), buffer1.begin() + read1, buffer2.begin())) {
            return false;
        }
    }
}

// ===========================================================================================================
// Directories
// ===========================================================================================================

bool directoryExists(const DirectoryPath& filename) {
    return std::filesystem::is_directory(filename);
}

Optional<int64> directorySize(const DirectoryPath& directory) {
    int64 sum = 0;
    try {
        iterateAllFiles(directory,
                        [&sum](const FilePath& current) {
                            if (auto res = fileSize(current)) {
                                sum += *res;
                            } else {
                                throw std::filesystem::filesystem_error("fileSize failed", {});
                            }
                        },
                        IterateFilesystemFlag::RECURSIVELY,
                        {});
    } catch (const std::filesystem::filesystem_error& BUFF_UNUSED(error)) {
        return NULL_OPTIONAL;
    }
    return sum;
}

bool createDirectory(const DirectoryPath& directory) {
    std::error_code errorCode;
    const bool      res = std::filesystem::create_directories(directory, errorCode);
    // The call returns false if the directory already exists, so we need to double-check when res is false:
    return res || directoryExists(directory);
}

bool removeDirectory(const DirectoryPath& directory) {
    if (directoryExists(directory)) {
        std::error_code errorCode;
        return std::filesystem::remove(directory, errorCode);
    } else {
        return true;
    }
}

DirectoryPath getWorkingDirectory() {
    return std::filesystem::current_path();
}

void setWorkingDirectory(const DirectoryPath& directory) {
    std::filesystem::current_path(directory);
}

// ===========================================================================================================
// Enumerating
// ===========================================================================================================

template <typename TPath>
static void iterateFilesystemImpl(const DirectoryPath&                         directory,
                                  const Function<IterateStatus(const TPath&)>& functor,
                                  const Flags<IterateFilesystemFlag>           flags,
                                  const ArrayView<const String>                extensionFilter) {
    BUFF_ASSERT(directory.isAbsolute());
#ifndef __EMSCRIPTEN__
    static constexpr StringView UNC_PREFIX = R"(\\?\)";
    // Note that using / operator with \\?\ lhs does nothing...
    const std::filesystem::path directoryPathFixed = (UNC_PREFIX + directory.getNative()).asWString();
    auto                        fixPath            = [](const std::filesystem::path& in) -> TPath {
        String str = in.native();
        BUFF_ASSERT(str.startsWith(UNC_PREFIX));
        str = str.getSubstring(UNC_PREFIX.size());
        return TPath(str);
    };
    Array<std::wstring> extensionFilterNative;
    for (auto& i : extensionFilter) {
        extensionFilterNative.pushBack(("." + i).asWString()); // STL version contains .
    }
#else
    const std::filesystem::path directoryPathFixed = directory;
    Array<std::string>          extensionFilterNative;
    for (auto& i : extensionFilter) {
        extensionFilterNative.pushBack(("." + i).asCString()); // STL version contains .
    }
    auto fixPath = [](const std::filesystem::path& in) -> TPath { return TPath(in); };
#endif
    constexpr auto FLAGS = std::filesystem::directory_options::skip_permission_denied;
    auto           doIt  = [&]<typename T>(T&& iterator) {
        auto currentIt = std::filesystem::begin(iterator);
        while (currentIt != std::filesystem::end(iterator)) {
            const auto& path = currentIt->path();

            bool process = true;
            //  The status functions can fail on WINAPI Error 1920 (ERROR_CANT_ACCESS_FILE) for some specific
            //  files, e.g. in the recycle bin, even with skip_permission_denied.
            try {
                if constexpr (std::is_same_v<std::decay_t<TPath>, DirectoryPath>) {
                    process = currentIt->is_directory();
                } else {
                    static_assert(std::is_same_v<std::decay_t<TPath>, FilePath>);
                    process = currentIt->is_regular_file();
                    if (process && extensionFilterNative.notEmpty()) {
                        process = extensionFilterNative.contains(path.extension());
                    }
                }
                // exists() returns false for example for directories pointing to non-existing/non-accessible
                // locations. is_symlink is needed because even though "symlinks are skipped", it only applies
                // for directories, not individual files.
                process &= currentIt->exists();
                process &= !currentIt->is_symlink();
            } catch (std::filesystem::filesystem_error& BUFF_UNUSED(err)) {
                process = false;
            }

            if (process) {
                const IterateStatus res = functor(fixPath(path));
                if (res == IterateStatus::ABORT) {
                    return;
                } else if (res == IterateStatus::IGNORE_SUBTREE) {
                    if constexpr (std::is_same_v<std::decay_t<T>,
                                                            std::filesystem::recursive_directory_iterator>) {
                        iterator.pop();
                    } else {
                        return; // No subtrees on non-recursive iterators.
                    }
                }
            }
            try {
                ++currentIt;
            } catch (const std::exception& ex) {
                const char* what = ex.what();
                BUFF_ASSERT(false, what, path);
                throw Exception("Filesystem iterator exception: "_S + what +
                                "\nLast processed path: " + path);
            }
        }
    };
    if (flags.hasFlag(IterateFilesystemFlag::RECURSIVELY)) {
        doIt(std::filesystem::recursive_directory_iterator(directoryPathFixed, FLAGS));
    } else {
        doIt(std::filesystem::directory_iterator(directoryPathFixed, FLAGS));
    }
}

void iterateAllFiles(const DirectoryPath&                            directory,
                     const Function<IterateStatus(const FilePath&)>& functor,
                     const Flags<IterateFilesystemFlag>              flags,
                     const ArrayView<const String>                   extensionFilter) {
    iterateFilesystemImpl<FilePath>(directory, functor, flags, extensionFilter);
}

void iterateAllFiles(const DirectoryPath&                   directory,
                     const Function<void(const FilePath&)>& functor,
                     const Flags<IterateFilesystemFlag>     flags,
                     const ArrayView<const String>          extensionFilter) {
    return iterateAllFiles(directory,
                           Function([&](const FilePath& path) {
                               functor(path);
                               return IterateStatus::CONTINUE;
                           }),
                           flags,
                           extensionFilter);
}

void iterateAllDirectories(const DirectoryPath&                                 directory,
                           const Function<IterateStatus(const DirectoryPath&)>& functor,
                           const Flags<IterateFilesystemFlag>                   flags) {
    iterateFilesystemImpl<DirectoryPath>(directory, functor, flags, {});
}

void iterateAllDirectories(const DirectoryPath&                        directory,
                           const Function<void(const DirectoryPath&)>& functor,
                           const Flags<IterateFilesystemFlag>          flags) {
    return iterateAllDirectories(
        directory,
        [&](const DirectoryPath& path) {
            functor(path);
            return IterateStatus::CONTINUE;
        },
        flags);
}

BUFF_NAMESPACE_END
