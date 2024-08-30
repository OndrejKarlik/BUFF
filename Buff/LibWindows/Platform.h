#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include "Lib/Expected.h"
#include "Lib/Path.h"
#include "Lib/Pixel.h"
#include "Lib/Platform.h"
#include <Windows.h>
#include "Lib/UndefIntrusiveMacros.h"

BUFF_NAMESPACE_BEGIN

class String;

#define BUFF_CHECKED_CALL_HRESULT(functionCall)                                                              \
    {                                                                                                        \
        [[maybe_unused]] const HRESULT hr_ = functionCall;                                                   \
        BUFF_ASSERT(SUCCEEDED(hr_), unsigned(hr_), HRESULT_CODE(hr_), GetLastError());                       \
    }                                                                                                        \
    static_assert(true) /* force semicolon */

bool isDebuggerPresent();

void breakInDebugger();

int executeShell(StringView command);

struct ExecuteProcessParams {
    FilePath      executable;
    Array<String> args;

    /// If not set, inherits the working dir from the current process
    Optional<DirectoryPath> workingDir;

    /// If true, standard outputs are read after process finishes and returned in the return value. If false,
    /// standard outputs are sent immediately to this process standard outputs
    ///
    /// TODO: STDIN is also redirected but not implemented yet. We should have an option to actually provide
    /// STDIN to the process.
    bool captureStandardOutputs = true;

    /// If true, the execution will return immediately and will not wait for the process to finish. Process
    /// return code and outputs will not be available. This is incompatible with captureStandardOutputs.
    bool runDetached = false;

    String getCommandLine() const;
};

struct ExecuteProcessResult {
    /// NULL_OPTIONAL if the process runs detached
    Optional<int> returnValue;
    String        stdOut;
    String        stdErr;
};
Expected<ExecuteProcessResult> executeProcess(const ExecuteProcessParams& params);

void executeAsAdministrator(const FilePath& executable, StringView args);

/// Returns list of drives (their letters) available on the machine
Array<char> getAllDrives();

String getDriveName(char drive);

/// Prints text in MSVS "Output" window, on a new line, with [$BUFF_LIBRARY_NAME] prefix
void printInVisualStudioDebugWindow(String message);

void setConsoleTitle(StringView title);

FilePath getThisModuleFilename();

/// Returns characters width/height of the console window
Pixel getConsoleSize();

/// Gets monitor display rate, in Hertz
Optional<int> getRefreshRate();

int64 getDriveFreeSpace(char driveLetter);

/// \param bytesPerPixel
/// Must be 3 or 4 (4 meaning there is padding or discarded alpha channel as the last byte of each pixel)
/// \param saveAlpha
/// If true, the alpha channel is saved as well into a separate file
bool saveBmp(const FilePath&            filename,
             ArrayView<const std::byte> rgbPixels,
             Pixel                      size,
             int                        bytesPerPixel,
             bool                       saveAlpha = false);

/// Sets name of a thread
void setThreadName(HANDLE thread, StringView name);

enum class ConsoleColor { BLUE, RED, GREEN, YELLOW, CYAN, MAGENTA };

void printStdoutColored(StringView message, ConsoleColor color);

Array<String> parseCommandLine(StringView commandLine);

void setEnvironmentVariable(StringView name, StringView value);

Optional<String> expandEnvironmentVariables(StringView input);

String getRandomUuid();

// ===========================================================================================================
// System directories/Files
// ===========================================================================================================

/// Returns the (global) Windows temp folder
DirectoryPath getGlobalTempDirectory();

/// Returns a filename that can be used for a temporary file (located in OS temp folder). Does not actually
/// create the file.
FilePath getTemporaryFilename();

/// Returns a filename that can be used for a temporary directory (located in OS temp folder). Does not
/// actually create the folder.
DirectoryPath getTemporaryDirectory();

DirectoryPath getRoamingDirectory();

// ===========================================================================================================
// Gui/Windows
// ===========================================================================================================

class WindowScreenshot : public NoncopyableMovable {
    HBITMAP mBitmap;
    HDC     mHdc;

public:
    WindowScreenshot(const HBITMAP bitmap, const HDC hdc)
        : mBitmap(bitmap)
        , mHdc(hdc) {}
    WindowScreenshot(WindowScreenshot&& other) noexcept
        : mBitmap(other.mBitmap)
        , mHdc(other.mHdc) {
        other.mBitmap = nullptr;
        other.mHdc    = nullptr;
    }
    void operator=(WindowScreenshot&& other) noexcept {
        std::swap(mBitmap, other.mBitmap);
        std::swap(mHdc, other.mHdc);
    }
    bool saveToFile(const FilePath& filename) const;
    ~WindowScreenshot();
};
WindowScreenshot screenshotWindow(NativeWindowHandle window);

void displayMessageWindow(StringView title, StringView text);

bool isWindowMaximized(NativeWindowHandle hwnd);

void setWindowMaximized(NativeWindowHandle window, bool maximized = true);

BUFF_NAMESPACE_END
