#include <ShlObj.h>
// Do not reorder includes - ShlObj.h needs CONST defined as a macro
#include "LibWindows/Platform.h"
#include "Lib/Exception.h"
#include "Lib/Flags.h"
#include "Lib/Path.h"
#include "Lib/String.h"
#include <fstream>
#include <iostream>
#include <shellapi.h>

BUFF_NAMESPACE_BEGIN

Array<char> getAllDrives() {
    Array<char> result;
    auto        winResult = GetLogicalDrives();
    for (const int64 i : range(sizeof(winResult) * 8)) {
        if (winResult & 1) {
            result.pushBack('a' + char(i));
        }
        winResult >>= 1;
    }
    return result;
}

String getDriveName(const char drive) {
    StaticArray<WCHAR, MAX_PATH> result;
    BUFF_CHECKED_CALL(TRUE,
                      GetVolumeInformationW((drive + ":/"_S).asWString().c_str(),
                                            result.data(),
                                            MAX_PATH - 1,
                                            nullptr,
                                            nullptr,
                                            nullptr,
                                            nullptr,
                                            0));
    return std::wstring(result.data());
}

bool isDebuggerPresent() {
    return IsDebuggerPresent();
}

void breakInDebugger() {
    DebugBreak();
}

int executeShell(const StringView command) {
    return _wsystem(command.asWString().c_str());
}

String ExecuteProcessParams::getCommandLine() const {
    auto quoteArg = [](const StringView str) -> String {
        // Inspired by
        // https://learn.microsoft.com/en-us/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way
        if (str.isEmpty()) {
            return "\"\"";
        } else if (!str.findFirstOf(" \t\n\v\"")) {
            return str;
        } else {
            String result         = "\"";
            uint   numBackslashes = 0;
            for (const char ch : str) {
                if (ch == '\\') {
                    ++numBackslashes;
                } else {
                    if (ch == '"') {
                        // Escape all backslashes and the following double quotation mark.
                        result << std::string(numBackslashes * 2 + 1, '\\');
                    } else if (numBackslashes > 0) {
                        // Backslashes aren't special here.
                        result << std::string(numBackslashes, '\\');
                    }
                    numBackslashes = 0;
                    result << ch;
                }
            }
            if (numBackslashes > 0) {
                // Escape all backslashes, but let the terminating  double quotation mark we add below be
                // interpreted as a metacharacter.
                result << std::string(numBackslashes * 2, '\\');
            }
            result << "\"";
            return result;
        }
    };
    return expandEnvironmentVariables(executable.getNative())->getQuoted() + " " +
           listToStr(args, " ", quoteArg);
}

struct Pipe {
    HANDLE read  = nullptr;
    HANDLE write = nullptr;

    Pipe() {
        SECURITY_ATTRIBUTES securityAttributes  = {};
        securityAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
        securityAttributes.bInheritHandle       = TRUE;
        securityAttributes.lpSecurityDescriptor = nullptr;

        BUFF_CHECKED_CALL(TRUE, CreatePipe(&read, &write, &securityAttributes, 0));
        BUFF_CHECKED_CALL(TRUE, SetHandleInformation(read, HANDLE_FLAG_INHERIT, 0));
    }

    void closeWrite() {
        BUFF_CHECKED_CALL(TRUE, CloseHandle(write));
        write = nullptr;
    }

    String readCurrentContents() {
        StaticArray<char, 64 * 1024> buffer;
        DWORD                        bytesRead = 0;
        String                       result;
        while (ReadFile(read, buffer.data(), int(buffer.size()), &bytesRead, nullptr) && bytesRead > 0) {
            for (const int64 i : range(bytesRead)) {
                result << buffer[i];
            }
            bytesRead = 0;
        }
        return result;
    }
    ~Pipe() {
        if (read) {
            BUFF_CHECKED_CALL(TRUE, CloseHandle(read));
        }
        if (write) {
            BUFF_CHECKED_CALL(TRUE, CloseHandle(write));
        }
    }
};

Expected<ExecuteProcessResult> executeProcess(const ExecuteProcessParams& params) {
    BUFF_ASSERT(!params.runDetached || !params.captureStandardOutputs, "Cannot have both at the same time!");
    // According to
    // https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po

    Pipe stdOut;
    Pipe stdErr;
    Pipe stdIn;

    STARTUPINFO startupInfo = {};
    startupInfo.cb          = sizeof(startupInfo);
    if (params.captureStandardOutputs) {
        startupInfo.hStdError  = stdErr.write;
        startupInfo.hStdOutput = stdOut.write;
        startupInfo.hStdInput  = stdIn.write;
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    }

    PROCESS_INFORMATION processInfo = {};
    std::wstring        cmdline     = params.getCommandLine().asWString();
    cmdline.push_back('\0');
    const std::wstring workingDirectory = params.workingDir.valueOr({}).getNative().asWString();
    // Start the child process.
    if (!CreateProcessW(nullptr,        // No module name (use command line)
                        cmdline.data(), // Command line
                        nullptr,        // Process handle not inheritable
                        nullptr,        // Thread handle not inheritable
                        true,           // Set handle inheritance
                        0,              // No creation flags
                        nullptr,        // Use parent's environment block
                        params.workingDir ? workingDirectory.c_str() : nullptr,
                        &startupInfo,    // Pointer to STARTUPINFO structure
                        &processInfo)) { // Pointer to PROCESS_INFORMATION structure
        switch (GetLastError()) {
        case ERROR_BAD_EXE_FORMAT:
            return makeUnexpected(params.executable.getNative() + " is not a valid Windows application");
        default:
            return makeUnexpected(
                "CreateProcess failed with unknown error. TODO: add a case for this error: " +
                toStr(GetLastError()));
        }
    }
    const Finally finally = [&] {
        BUFF_CHECKED_CALL(TRUE, CloseHandle(processInfo.hThread));
        BUFF_CHECKED_CALL(TRUE, CloseHandle(processInfo.hProcess));
    };

    if (params.runDetached) { // We do not wait for the process if we run detached
        return ExecuteProcessResult {};
    }

    ExecuteProcessResult result;
    if (params.captureStandardOutputs) {
        bool processDone = false;
        stdOut.closeWrite(); // Needs to be done, otherwise ReadFile will block
        stdErr.closeWrite();
        while (!processDone) {
            StaticArray<HANDLE, 3> handles = {processInfo.hProcess, stdOut.read, stdErr.read};
            const auto res = WaitForMultipleObjects(int(handles.size()), handles.data(), false, INFINITE);
            switch (res) {
            case WAIT_OBJECT_0:
                processDone = true;
                break;
            case WAIT_OBJECT_0 + 1:
                result.stdOut << stdOut.readCurrentContents();
                break;
            case WAIT_OBJECT_0 + 2:
                result.stdErr << stdErr.readCurrentContents();
                break;
            default:
                BUFF_STOP;
            }
            // Make sure we got everything:
            result.stdOut << stdOut.readCurrentContents();
            result.stdErr << stdErr.readCurrentContents();
        }
    } else {
        BUFF_CHECKED_CALL(WAIT_OBJECT_0, WaitForSingleObject(processInfo.hProcess, INFINITE));
    }

    DWORD returnValue = {};
    BUFF_CHECKED_CALL(TRUE, GetExitCodeProcess(processInfo.hProcess, &returnValue));

    result.returnValue = int(returnValue);
    return result;
}

void executeAsAdministrator(const FilePath& executable, const StringView args) {
    SHELLEXECUTEINFO info          = {};
    info.cbSize                    = sizeof(SHELLEXECUTEINFO);
    info.fMask                     = SEE_MASK_NOCLOSEPROCESS;
    info.hwnd                      = nullptr;
    info.lpVerb                    = L"runas";
    const std::wstring executableW = executable.getNative().asWString();
    info.lpFile                    = executableW.c_str();
    const std::wstring argsW       = args.asWString();
    info.lpParameters              = argsW.c_str();
    info.lpDirectory               = nullptr;
    info.nShow                     = SW_NORMAL;

    BUFF_CHECKED_CALL(TRUE, ShellExecuteEx(&info));
    BUFF_CHECKED_CALL(WAIT_OBJECT_0, WaitForSingleObject(info.hProcess, INFINITE));
    BUFF_CHECKED_CALL(TRUE, CloseHandle(info.hProcess));
}

void printInVisualStudioDebugWindow(String message) {
    if (isDebuggerPresent()) {
        message            = "["_S + BUFF_LIBRARY_NAME + "] " + message + "\n";
        const auto wString = message.asWString();
        OutputDebugStringW(wString.c_str());
    }
}

void setConsoleTitle(const StringView title) {
    BUFF_CHECKED_CALL(TRUE, SetConsoleTitleW(title.asWString().c_str()));
}

FilePath getThisModuleFilename() {
    StaticArray<wchar_t, MAX_PATH + 1> array;
    GetModuleFileNameW(nullptr, array.data(), MAX_PATH);
    return FilePath(array.data());
}

Pixel getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    BUFF_CHECKED_CALL(TRUE, GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi));
    return {
        csbi.srWindow.Right - csbi.srWindow.Left + 1,
        csbi.srWindow.Bottom - csbi.srWindow.Top + 1,
    };
}

Optional<int> getRefreshRate() {
    DEVMODE devMode       = {};
    devMode.dmSize        = sizeof(DEVMODE);
    devMode.dmDriverExtra = 0;

    if (!EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode) || devMode.dmDisplayFrequency <= 1) {
        return {};
    } else {
        return devMode.dmDisplayFrequency;
    }
}

int64 getDriveFreeSpace(const char driveLetter) {
    const StaticArray<wchar_t, 4> drive = {wchar_t(driveLetter), ':', '/', '\0'};
    ULARGE_INTEGER                res;
    BUFF_CHECKED_CALL(TRUE, GetDiskFreeSpaceEx(drive.data(), nullptr, nullptr, &res));
    return res.QuadPart;
}

// https://stackoverflow.com/questions/24644709/c-hbitmap-bitmap-into-bmp-file
static BITMAPINFO createBitmapInfo(const HBITMAP bitmap) {
    BITMAP bmp;
    BUFF_CHECKED_CALL(sizeof(BITMAP), GetObject(bitmap, sizeof(BITMAP), &bmp));

    BITMAPINFO result              = {};
    result.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    result.bmiHeader.biWidth       = bmp.bmWidth;
    result.bmiHeader.biHeight      = bmp.bmHeight;
    result.bmiHeader.biPlanes      = bmp.bmPlanes; // we are assuming that there is only one plane
    result.bmiHeader.biBitCount    = bmp.bmBitsPixel;
    result.bmiHeader.biCompression = BI_RGB; // no compression this is an RGB bitmap
    // calculate size and align to a DWORD (32 bit), we are assuming there is only one plane.
    result.bmiHeader.biSizeImage =
        alignUp(unsigned(result.bmiHeader.biWidth * bmp.bmBitsPixel), 32) / 8 * result.bmiHeader.biHeight;
    // all device colours are important
    result.bmiHeader.biClrImportant = 0;
    return result;
}

enum class SaveBmpFlag {
    FLIP_Y     = 1 << 0,
    SAVE_ALPHA = 1 << 1,
};

static bool saveBmpImpl(FilePath                   filename,
                        ArrayView<const std::byte> rgbPixels,
                        const Pixel                size,
                        const int                  bytesPerPixel,
                        Flags<SaveBmpFlag>         flags) {
    BUFF_ASSERT(bytesPerPixel == anyOf(3, 4));
    BUFF_ASSERT(filename.getExtension() == "bmp");
    BITMAPFILEHEADER fileHeader = {};
    fileHeader.bfType           = 0x4D42; // 'BM'
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + int(size.getPixelCount()) * 3;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bi = {};
    bi.biSize           = sizeof(BITMAPINFOHEADER);
    bi.biWidth          = size.x;
    bi.biHeight   = flags.hasFlag(SaveBmpFlag::FLIP_Y) ? -size.y : size.y; // negative height for top-down DIB
    bi.biPlanes   = 1;
    bi.biBitCount = WORD(bytesPerPixel * 8);
    bi.biCompression = BI_RGB;

    std::ofstream file(filename.getNative().asWString(), std::ios::out | std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(BITMAPFILEHEADER));
    file.write(reinterpret_cast<const char*>(&bi), sizeof(BITMAPINFOHEADER));
    BUFF_ASSERT(rgbPixels.size() == size.getPixelCount() * bytesPerPixel,
                bytesPerPixel,
                rgbPixels.size(),
                size.getPixelCount() * bytesPerPixel);
    file.write(reinterpret_cast<const char*>(rgbPixels.data()), rgbPixels.size());
    file.close();
    if (!file.good()) {
        return false;
    }
    if (flags.hasFlag(SaveBmpFlag::SAVE_ALPHA)) {
        flags.clearFlag(SaveBmpFlag::SAVE_ALPHA);
        filename = filename.getDirectory().valueOr({}) /
                   FilePath(filename.getFilenameWithoutExtension() + "_alpha.bmp");
        // BMP does not support alpha channel, so we save it as a separate file
        Array<std::byte> alphaPixels(3 * size.getPixelCount());
        for (const int64 i : range(alphaPixels.size())) {
            alphaPixels[i] = rgbPixels[(i / 3) * bytesPerPixel + 3];
        }
        return saveBmpImpl(filename, alphaPixels, size, 3, flags);
    }
    return true;
}

bool saveBmp(const FilePath&                  filename,
             const ArrayView<const std::byte> rgbPixels,
             const Pixel                      size,
             const int                        bytesPerPixel,
             const bool                       saveAlpha) {
    Flags<SaveBmpFlag> flags = SaveBmpFlag::FLIP_Y;
    if (saveAlpha) {
        flags |= SaveBmpFlag::SAVE_ALPHA;
        BUFF_ASSERT(bytesPerPixel == 4);
    }
    return saveBmpImpl(filename, rgbPixels, size, bytesPerPixel, flags);
}

void setThreadName(const HANDLE thread, const StringView name) {
    BUFF_CHECKED_CALL_HRESULT(SetThreadDescription(thread, name.asWString().c_str()));
}

void printStdoutColored(const StringView message, const ConsoleColor color) {
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD         attribute;
    switch (color) {
    case ConsoleColor::BLUE:
        attribute = FOREGROUND_BLUE;
        break;
    case ConsoleColor::RED:
        attribute = FOREGROUND_RED;
        break;
    case ConsoleColor::GREEN:
        attribute = FOREGROUND_GREEN;
        break;
    case ConsoleColor::YELLOW:
        attribute = FOREGROUND_RED | FOREGROUND_GREEN;
        break;
    case ConsoleColor::CYAN:
        attribute = FOREGROUND_BLUE | FOREGROUND_GREEN;
        break;
    case ConsoleColor::MAGENTA:
        attribute = FOREGROUND_BLUE | FOREGROUND_RED;
        break;
    default:
        BUFF_STOP;
    }
    BUFF_CHECKED_CALL(TRUE, SetConsoleTextAttribute(handle, attribute));
    std::cout << message;
    BUFF_CHECKED_CALL(TRUE,
                      SetConsoleTextAttribute(handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE));
}

Array<String> parseCommandLine(const StringView commandLine) {
    int       argc = 0;
    wchar_t** argv = CommandLineToArgvW(String(commandLine).asWString().c_str(), &argc);
    BUFF_ASSERT(argv);
    Array<String> result(argc);
    for (const int i : range(argc)) {
        result[i] = String(std::wstring(argv[i]));
    }
    LocalFree(argv);
    return result;
}

void setEnvironmentVariable(const StringView name, const StringView value) {
    BUFF_CHECKED_CALL(TRUE, SetEnvironmentVariableW(name.asWString().c_str(), value.asWString().c_str()));
}

Optional<String> expandEnvironmentVariables(const StringView input) {
    const std::wstring wide       = input.asWString();
    const DWORD        bufferSize = ExpandEnvironmentStringsW(wide.c_str(), nullptr, 0);
    if (bufferSize == 0) {
        return NULL_OPTIONAL;
    }
    Array<wchar_t> buffer(bufferSize);
    const DWORD    result = ExpandEnvironmentStringsW(wide.c_str(), buffer.data(), bufferSize);
    if (result == 0) {
        return NULL_OPTIONAL;
    }
    return String(std::wstring(buffer.data()));
}

String getRandomUuid() {
    UUID uuid;
    BUFF_CHECKED_CALL(RPC_S_OK, UuidCreate(&uuid));
    RPC_CSTR uuidString = {};
    BUFF_CHECKED_CALL(RPC_S_OK, UuidToStringA(&uuid, &uuidString));
    const String result = reinterpret_cast<const char*>(uuidString);
    BUFF_CHECKED_CALL(RPC_S_OK, RpcStringFreeA(&uuidString));
    return result;
}

// ===========================================================================================================
// System directories/Files
// ===========================================================================================================

DirectoryPath getGlobalTempDirectory() {
    StaticArray<wchar_t, MAX_PATH + 2> tempFolder;
    // TODO: Use GetTempPath2 with fallback to GetTempPath on older windows versions
    const DWORD retVal = GetTempPathW(DWORD(tempFolder.size()), tempFolder.data());
    if (retVal == 0) {
        throw Exception("getGlobalTempDirectory failed");
    }
    BUFF_ASSERT(tempFolder[retVal] == L'\0');
    return DirectoryPath(tempFolder.data());
}

FilePath getTemporaryFilename() {
    const DirectoryPath temp = getGlobalTempDirectory();
    FilePath result = temp / FilePath(String("__"_S + BUFF_LIBRARY_NAME + "_tmp_") + getRandomUuid());
    return result;
}

DirectoryPath getTemporaryDirectory() {
    const FilePath result = getTemporaryFilename();
    return DirectoryPath(result.getGeneric());
}

DirectoryPath getRoamingDirectory() {
    PWSTR         path;
    const auto    res     = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);
    const Finally finally = [&]() { CoTaskMemFree(path); };
    BUFF_ASSERT(SUCCEEDED(res));
    return DirectoryPath(String(path));
}

// ===========================================================================================================
// Gui/Windows
// ===========================================================================================================

WindowScreenshot screenshotWindow(const NativeWindowHandle window) {
    const HWND    hwnd         = HWND(window.handle);
    const HDC     hdcScreen    = GetDC(hwnd);
    const Finally deleteScreen = [&]() { BUFF_CHECKED_CALL(TRUE, ReleaseDC(hwnd, hdcScreen)); };
    const HDC     hdcMem       = CreateCompatibleDC(hdcScreen);

    RECT rect;
    BUFF_CHECKED_CALL(TRUE, GetClientRect(hwnd, &rect));
    const int     width   = rect.right - rect.left;
    const int     height  = rect.bottom - rect.top;
    const HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(hdcMem, hBitmap);
    BUFF_CHECKED_CALL(TRUE, BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY));
    return WindowScreenshot(hBitmap, hdcMem);
}

// https://stackoverflow.com/questions/24644709/c-hbitmap-bitmap-into-bmp-file
bool WindowScreenshot::saveToFile(const FilePath& filename) const {
    BITMAPINFO bi = createBitmapInfo(mBitmap);
    BUFF_ASSERT(bi.bmiHeader.biBitCount == anyOf(24, 32));
    Array<BYTE> bits(bi.bmiHeader.biSizeImage);
    BUFF_CHECKED_CALL(bi.bmiHeader.biHeight,
                      GetDIBits(mHdc, mBitmap, 0, bi.bmiHeader.biHeight, bits.data(), &bi, DIB_RGB_COLORS));

    return saveBmpImpl(filename,
                       ArrayView(bits).asBytes(),
                       Pixel(bi.bmiHeader.biWidth, bi.bmiHeader.biHeight),
                       bi.bmiHeader.biBitCount / 8,
                       {});
}

WindowScreenshot::~WindowScreenshot() {
    if (mBitmap) {
        SelectObject(mHdc, nullptr);
        BUFF_CHECKED_CALL(TRUE, DeleteObject(mBitmap));
        BUFF_CHECKED_CALL(TRUE, DeleteDC(mHdc));
    }
}

void displayMessageWindow(const StringView title, const StringView text) {
    MessageBoxW(nullptr, text.asWString().c_str(), title.asWString().c_str(), MB_OK);
}

bool isWindowMaximized(const NativeWindowHandle hwnd) {
    return IsZoomed(HWND(hwnd.handle));
}

void setWindowMaximized(const NativeWindowHandle window, const bool maximized) {
    const HWND hwnd = HWND(window.handle);
    BOOL       res;
    if (maximized) {
        res = ShowWindow(hwnd, SW_MAXIMIZE);
        BUFF_ASSERT(res);
    } else if (isWindowMaximized(window)) {
        res = ShowWindow(hwnd, SW_RESTORE);
        BUFF_ASSERT(res);
    }
}

BUFF_NAMESPACE_END
