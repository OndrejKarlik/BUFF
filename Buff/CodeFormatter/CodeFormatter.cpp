#include "LibWindows/AssertHandler.h"
#include "LibWindows/Platform.h"
#include "Lib/Assert.h"
#include "Lib/containers/Map.h"
#include "Lib/Cryptography.h"
#include "Lib/Exception.h"
#include "Lib/Expected.h"
#include "Lib/Filesystem.h"
#include "Lib/Function.h"
#include "Lib/Path.h"
#include "Lib/String.h"
#include "Lib/Time.h"
#include <algorithm>
#include <iostream>

BUFF_NAMESPACE_BEGIN

constexpr bool BINARY_SERIALIZATION = true;

struct Cache {
    Map<FilePath, String> files; // absolutePath -> contentHash
    String                clangFormatVersion;
    String                clangFormatConfigHash;
    String                prettierVersion;

    void enumerateStructMembers(auto&& functor) {
        functor(clangFormatVersion, "clangFormatVersion");
        functor(clangFormatConfigHash, "clangFormatConfigHash");
        functor(prettierVersion, "prettierVersion");
        functor(files, "files");
        BUFF_ASSERT_SIZEOF(sizeof(files) + sizeof(clangFormatVersion) + sizeof(clangFormatConfigHash) +
                           sizeof(prettierVersion));
    }
};

static String getFileHash(const FilePath& path) {
    Optional<String> content = readTextFile(path);
    return getMd5Ascii(*content);
}

struct Environment {
    FilePath clangFormatExecutable;
};

const StaticArray<String, 2> CPP_EXTENSIONS {"h", "cpp"};
// Do not format .js - those are produced by build
const StaticArray<String, 2> JAVASCRIPT_EXTENSIONS {"ts", "mts"};
// Rest of web extensions formattable by prettier
const StaticArray<String, 6> WEB_EXTENSIONS {"html", "htm", "css", "less", "json", "code-workspace"};

static Expected<TrueType> formatFile(const Environment& environment, const FilePath& path) {
    // TODO: Deduplicate?
    if (std::ranges::contains(CPP_EXTENSIONS, String(path.getExtension().valueOr({})).getToLower())) {
        const Expected<ExecuteProcessResult> res = executeProcess({
            .executable = environment.clangFormatExecutable,
            .args       = {"-i", path.getNative()}
        });
        BUFF_ASSERT(res);
        if (res->returnValue == 0) {
            return TrueType {};
        } else {
            return makeUnexpected("clang-format failed with exit code " + toStr(res->returnValue) + ":\n" +
                                  res->stdOut + "\n" + res->stdErr);
        }
    } else {
        const Expected<ExecuteProcessResult> res = executeProcess({
            .executable = "prettier.cmd"_File,
            .args       = Array {"--write"_S, path.getNative()}
        });
        BUFF_ASSERT(res);
        if (res->returnValue == 0) {
            return TrueType {};
        } else {
            return makeUnexpected("js-beautify failed with exit code " + toStr(res->returnValue) + ":\n" +
                                  res->stdOut + "\n" + res->stdErr);
        }
    }
}

static void runClangFormat(const Array<String>& args) {
    const Timer timer;
    if (args.size() != 3) {
        throw Exception("Incorrect arguments! Usage:\nCodeFormatter path/to/clang-format.exe "
                        "path/to/cache.tmp folder/to/format.");
    }
    const FilePath      clangFormatExecutable = FilePath(args[0]);
    const FilePath      cacheFile             = FilePath(args[1]);
    const DirectoryPath runDir                = DirectoryPath(args[2]);

    Cache cache;
    if (fileExists(cacheFile)) {
        try {
            if constexpr (BINARY_SERIALIZATION) {
                Optional<Array<std::byte>> data = readBinaryFile(cacheFile);
                BinaryDeserializer         deserializer(std::vector(data->begin(), data->end()));
                deserializer.deserialize(cache, "cache");
            } else {
                Optional<String> data = readTextFile(cacheFile);
                JsonDeserializer deserializer(*data);
                deserializer.deserialize(cache, "cache");
            }
            std::cout << "Loaded cache with " << cache.files.size() << " entries." << std::endl;
        } catch (Exception& BUFF_UNUSED(ex)) {
            std::cout << "Tossing corrupted cache." << std::endl;
            cache = {};
        }
    }
    const String clangFormatVersion =
        executeProcess({.executable = clangFormatExecutable, .args = {"--version"}})->stdOut;
    String prettierVersion;
    if (const Expected prettierVerRes =
            executeProcess({.executable = "prettier.cmd"_File, .args = {"--version"}})) {
        prettierVersion = prettierVerRes->stdOut;
    } else {
        throw Exception("ERROR: Prettier not installed! Run setup to fix.");
    }

    if (cache.files.notEmpty() &&
        (clangFormatVersion != cache.clangFormatVersion || prettierVersion != cache.prettierVersion)) {
        std::cout << "Formatting tools version mismatch:\n";
        std::cout << "clang-format: saved: " << cache.clangFormatVersion
                  << ", current: " << clangFormatVersion << "\n;";
        std::cout << "prettier:     saved: " << cache.prettierVersion << ", current: " << prettierVersion
                  << "\n;";
        std::cout << ". Tossing cache." << std::endl;
        cache.files.clear();
    }
    cache.clangFormatVersion = clangFormatVersion;
    cache.prettierVersion    = prettierVersion;

    const String clangFormatConfigHash = getMd5Ascii(*readTextFile(runDir / ".clang-format"_File));
    if (cache.files.notEmpty() && clangFormatConfigHash != cache.clangFormatConfigHash) {
        std::cout << ".clang-format contents mismatch. Tossing cache." << std::endl;
        cache.files.clear();
    }
    cache.clangFormatConfigHash = clangFormatConfigHash;

    int             filesConsidered = 0;
    Array<FilePath> files;
    iterateAllFiles(runDir,
                    [&](const FilePath& i) {
                        ++filesConsidered;
                        const String ext = String(i.getExtension().valueOr({})).getToLower();
                        if ((std::ranges::contains(CPP_EXTENSIONS, ext) ||
                             std::ranges::contains(JAVASCRIPT_EXTENSIONS, ext) ||
                             std::ranges::contains(WEB_EXTENSIONS, ext)) &&
                            !i.getGeneric().contains("/node_modules/")) {
                            files.pushBack(i);
                        }
                        return IterateStatus::CONTINUE;
                    },
                    IterateFilesystemFlag::RECURSIVELY,
                    {}); // TODO: Is it faster to run with filter (but losing ability to ignore subtrees)?
    // std::cout << "Considered " << filesConsidered << " files\n";

    float     currentlyDrawn  = 0;
    int       currentProgress = 0;
    int       modified        = 0;
    int       attempted       = 0;
    const int consoleWidth    = getConsoleSize().x;

    Environment environment = {.clangFormatExecutable = clangFormatExecutable};
    for (auto& i : files) {
        ++currentProgress;
        while (currentlyDrawn / float(consoleWidth) < float(currentProgress) / float(files.size())) {
            ++currentlyDrawn;
            std::cout << ".";
        }
        BUFF_ASSERT(i.isAbsolute());
        const String md5        = getFileHash(i);
        String&      cacheEntry = cache.files[i];
        if (cacheEntry != md5) { // Can be default empty string if not previously present
            ++attempted;
            if (const Expected<TrueType> res = formatFile(environment, i)) {
                cacheEntry = getFileHash(i); // Hash might have changed
                if (cacheEntry != md5) {
                    std::cout << "\nFormatted " << i << std::endl;
                    ++modified;
                }
            } else {
                const String error = "Failed formatting file " + i.getNative() + "\n" + res.getError();
                std::cerr << error;
                cacheEntry.clear();
            }
        }
    }

    if constexpr (BINARY_SERIALIZATION) {
        BinarySerializer serializer;
        serializer.serialize(cache, "cache");
        auto state = serializer.getState();
        BUFF_CHECKED_CALL(true, writeBinaryFile(cacheFile, ArrayView(state.data(), state.size())));
    } else {
        JsonSerializer serializer;
        serializer.serialize(cache, "cache");
        String state = serializer.getJson(true);
        BUFF_CHECKED_CALL(true, writeTextFile(cacheFile, state));
    }
    std::cout << "\n"
              << modified << " files changed. " << attempted << " files were actually processed. Formatting "
              << files.size() << " files took " << timer.getElapsed().getUserReadable() << "." << std::endl;
}

BUFF_NAMESPACE_END

int main(const int argv, const char* argc[]) {
    try {
        Buff::setAssertHandler(Buff::debuggingAssertHandler);
        Buff::Array<Buff::String> args;
        for (const int i : Buff::range(1, argv)) {
            args.pushBack(argc[i]);
        }
        Buff::runClangFormat(args);
        return 0;
    } catch (std::exception& ex) {
        std::cerr << ex.what();
        return 1;
    }
}
