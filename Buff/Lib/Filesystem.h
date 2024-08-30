#pragma once
#include "Lib/containers/Array.h"
#include "Lib/Flags.h"
#include "Lib/Optional.h"
#include "Lib/String.h"
#include <functional>

BUFF_NAMESPACE_BEGIN

template <typename T>
class Function;
class FilePath;
class DirectoryPath;

// TODO: unify handling of errors between removeFile and removeDir

// ===========================================================================================================
// Files
// ===========================================================================================================

[[nodiscard]] bool fileExists(const FilePath& filename);

/// Returns true if successful, false if file does not exist. Throws on errors.
[[nodiscard]] bool removeFile(const FilePath& filename);

/// Returns true if copy took place
[[nodiscard]] bool copyFile(const FilePath& from, const FilePath& to);

void renameFile(const FilePath& from, const FilePath& to);

/// Returns NULL_OPTIONAL on errors (e.g. file does not exist, or is inaccessible).
[[nodiscard]] Optional<int64> fileSize(const FilePath& filename);

/// Returns NULL_OPTIONAL on errors (e.g. file does not exist, or is inaccessible).
[[nodiscard]] Optional<String> readTextFile(const FilePath& filename);

/// Returns NULL_OPTIONAL on errors (e.g. file does not exist, or is inaccessible).
[[nodiscard]] Optional<Array<std::byte>> readBinaryFile(const FilePath& filename);

[[nodiscard]] bool areFilesIdentical(const FilePath& filename1, const FilePath& filename2);

[[nodiscard]] bool writeTextFile(const FilePath& filename, StringView text);

[[nodiscard]] bool writeBinaryFile(const FilePath& filename, ArrayView<const std::byte> data);

// ===========================================================================================================
// Directories
// ===========================================================================================================

[[nodiscard]] bool directoryExists(const DirectoryPath& filename);

[[nodiscard]] DirectoryPath getWorkingDirectory();

void setWorkingDirectory(const DirectoryPath& directory);

/// Returns NULL_OPTIONAL on errors (e.g. path does not exist, any folder/file inside the folder is
/// inaccessible).
[[nodiscard]] Optional<int64> directorySize(const DirectoryPath& directory);

// TODO: change the bool retvals to enum with ERROR, NO_ACTION_PERFORMED, ACTION_PERFORMED?

/// Returns true if the folder exists after this call (even when it already existed before and nothing
/// happened).
[[nodiscard]] bool createDirectory(const DirectoryPath& directory);

/// Removes single non-empty directory. Returns true if the directory does not exist after the call (even if
/// it did not exist before).
[[nodiscard]] bool removeDirectory(const DirectoryPath& directory);

// ===========================================================================================================
// Enumerating
// ===========================================================================================================

enum class IterateFilesystemFlag {
    RECURSIVELY = 1 << 0,
};

enum class IterateStatus {
    /// Continue iterating
    CONTINUE,

    /// Immediately abort iterating, no further file/folder is visited
    ABORT,

    /// Skip the current folder (immediate containing folder for file iteration) and all its subfolders. It is
    /// possible to skip folders for file iteration by returning this for any of the files in the folder - no
    /// further files from the same folder will be visited
    IGNORE_SUBTREE,
};

/// Does not follow symlinks
void iterateAllFiles(const DirectoryPath&                            directory,
                     const Function<IterateStatus(const FilePath&)>& functor,
                     Flags<IterateFilesystemFlag>                    flags,
                     ArrayView<const String>                         extensionFilter);

/// Does not follow symlinks
void iterateAllFiles(const DirectoryPath&                   directory,
                     const Function<void(const FilePath&)>& functor,
                     Flags<IterateFilesystemFlag>           flags,
                     ArrayView<const String>                extensionFilter);

/// Does not follow symlinks
void iterateAllDirectories(const DirectoryPath&                                 directory,
                           const Function<IterateStatus(const DirectoryPath&)>& functor,
                           Flags<IterateFilesystemFlag>                         flags);

/// Does not follow symlinks
void iterateAllDirectories(const DirectoryPath&                        directory,
                           const Function<void(const DirectoryPath&)>& functor,
                           Flags<IterateFilesystemFlag>                flags);

BUFF_NAMESPACE_END
