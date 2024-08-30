#pragma once
#include "LibUltralight/LibUltralight.h"
#include "LibWindows/Platform.h"
#include <Ultralight/Listener.h>
#include <Ultralight/platform/Logger.h>

BUFF_NAMESPACE_BEGIN

class UlLogger final : public ul::Logger {
public:
    virtual void LogMessage(ul::LogLevel logLevel, const ul::String& message) override {
        static const StaticArray<String, 3> LEVELS = {"Error", "Warning", "Info"};
        printInVisualStudioDebugWindow("UL[" + LEVELS[int(logLevel)] + "]: " + fromUl(message));
    }

    void logConsoleMessage(const ul::MessageSource source,
                           const ul::MessageLevel  level,
                           const String&           message,
                           const int               lineNumber,
                           const int               columnNumber,
                           const String&           sourceId) {
        auto stringifySource = [](const ul::MessageSource src) {
            switch (src) {
            case ul::kMessageSource_XML:
                return "XML";
            case ul::kMessageSource_JS:
                return "JS";
            case ul::kMessageSource_Network:
                return "Network";
            case ul::kMessageSource_ConsoleAPI:
                return "ConsoleAPI";
            case ul::kMessageSource_Storage:
                return "Storage";
            case ul::kMessageSource_AppCache:
                return "AppCache";
            case ul::kMessageSource_Rendering:
                return "Rendering";
            case ul::kMessageSource_CSS:
                return "CSS";
            case ul::kMessageSource_Security:
                return "Security";
            case ul::kMessageSource_ContentBlocker:
                return "ContentBlocker";
            case ul::kMessageSource_Other:
                return "Other";
            default:
                return "";
            }
        };

        auto stringifyLevel = [](const ul::MessageLevel lvl) {
            switch (lvl) {
            case ul::kMessageLevel_Log:
                return "Log";
            case ul::kMessageLevel_Warning:
                return "Warning";
            case ul::kMessageLevel_Error:
                return "Error";
            case ul::kMessageLevel_Debug:
                return "Debug";
            case ul::kMessageLevel_Info:
                return "Info";
            default:
                return "";
            }
        };
        String text = String("[Console]: [")
                      << stringifySource(source) << "] [" << stringifyLevel(level) << "] " << message;

        if (source == ul::kMessageSource_JS) {
            text << " (" << sourceId << " @ line " << lineNumber << ", col " << columnNumber << ")";
        }

        if (level == ul::kMessageLevel_Error || level == ul::kMessageLevel_Warning) {
            BUFF_ASSERT(false, text);
        }
        this->LogMessage(ul::LogLevel::Info, toUl(text));
    }
};

BUFF_NAMESPACE_END
