#include <Windows.h>
#include <wininet.h>
// Order of includes matters here
#include "LibWindows/Net.h"
#include "LibWindows/Platform.h"
#include "Lib/Function.h"
#include "Lib/ThreadPool.h"
#include "Lib/Time.h"

BUFF_NAMESPACE_BEGIN

// <wininet.h> clashes with other windows-specific includes in Platform.cpp, so we have a separate file

String urlEncode(const StringView input, const bool forPostParams) {
    // https://en.wikipedia.org/wiki/Percent-encoding
    String result;
    for (const char i : input) {
        if (i == ' ' && forPostParams) {
            result << '+';
        } else if (isalnum(i) || i == anyOf('-', '_', '.', '~')) {
            result << i;
        } else {
            result << "%" << leftPad(toStrHexadecimal(uint8(i)), 2, '0');
        }
    }
    return result;
}

HttpPostPayload::HttpPostPayload(ArrayView<const std::pair<String, String>> parameters) {
    mPayload = listToStr(parameters, "&", [](const std::pair<String, String>& pair) {
        return urlEncode(pair.first) + "=" + urlEncode(pair.second);
    });
}

static Expected<String> httpPostRequestImpl(const StringView       url,
                                            const HttpPostPayload& payload,
                                            const HttpPostOptions& options = {}) {
    const Timer   timer;
    HINTERNET     hInternet = nullptr;
    HINTERNET     hConnect  = nullptr;
    HINTERNET     hRequest  = nullptr;
    const Finally finally   = [&]() {
        for (auto& handle : {hRequest, hConnect, hInternet}) {
            if (handle) {
                BUFF_CHECKED_CALL(TRUE, InternetCloseHandle(handle));
            }
        }
    };
    auto makeError = [&](const String& function) {
        const auto errCode = GetLastError();
        String     errorText;
        switch (errCode) {
        case ERROR_INTERNET_CANNOT_CONNECT:
            errorText = "ERROR_INTERNET_CANNOT_CONNECT";
            break;
        case ERROR_INTERNET_TIMEOUT:
            errorText = "ERROR_INTERNET_TIMEOUT";
            break;
        case ERROR_INVALID_NAME:
            errorText = "ERROR_INVALID_NAME";
            break;
        default:
            break;
        }
        String result = "Function " + function + " failed after " + timer.getElapsed().getUserReadable() +
                        " + : error code: " + toStr(errCode);
        if (errorText.notEmpty()) {
            result << " (" << errorText << ")";
        }
        return makeUnexpected(result);
    };

    hInternet = InternetOpenA(BUFF_LIBRARY_NAME, INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!hInternet) {
        return makeError("InternetOpen");
    }

    // For timeout to actually work, we need to set all of these
    constexpr StaticArray<DWORD, 5> TIMEOUT_OPTIONS({
        INTERNET_OPTION_CONNECT_TIMEOUT,
        INTERNET_OPTION_DATA_RECEIVE_TIMEOUT,
        INTERNET_OPTION_DATA_SEND_TIMEOUT,
        INTERNET_OPTION_RECEIVE_TIMEOUT,
        INTERNET_OPTION_SEND_TIMEOUT,
    });

    const int    firstSlash = url.findFirstOf("/").valueOr(url.size());
    String       server     = url.getSubstring(0, firstSlash);
    const String endpoint   = url.getSubstring(firstSlash);
    int          port       = 80;
    if (const Optional<int> portSplit = url.findFirstOf(":")) {
        server = url.getSubstring(0, *portSplit);
        port   = *fromStr<int>(url.getSubstring(*portSplit + 1));
    }

    hConnect = InternetConnect(hInternet,
                               server.asWString().c_str(),
                               INTERNET_PORT(port),
                               nullptr,
                               nullptr,
                               INTERNET_SERVICE_HTTP,
                               0,
                               0);
    if (!hConnect) {
        return makeError("InternetConnect");
    }

    // TODO: lpszAcceptTypes - should we fill it?
    hRequest =
        HttpOpenRequest(hConnect, L"POST", endpoint.asWString().c_str(), nullptr, nullptr, nullptr, 0, 0);
    if (!hRequest) {
        return makeError("HttpOpenRequest");
    }
    DWORD timeoutMs = DWORD(options.timeout.toMilliseconds());
    for (const auto& option : TIMEOUT_OPTIONS) {
        BUFF_CHECKED_CALL(TRUE, InternetSetOption(hRequest, option, &timeoutMs, sizeof(timeoutMs)));
        BUFF_ASSERT(timeoutMs == DWORD(options.timeout.toMilliseconds()));
    }

    if (!HttpSendRequest(hRequest,
                         nullptr,
                         0,
                         const_cast<char*>(payload.getPayload().data()),
                         payload.getPayload().size())) {
        return makeError("HttpSendRequest");
    }

    String                  result;
    StaticArray<char, 1024> buffer;
    DWORD                   bytesRead = 0;
    while (true) {
        if (InternetReadFile(hRequest, buffer.data(), DWORD(buffer.size() - 1), &bytesRead)) {
            if (bytesRead == 0) {
                BUFF_ASSERT(result.notEmpty());
                return result;
            } else {
                result << StringView(buffer.data(), bytesRead);
            }
        } else {
            return makeError("InternetReadFile");
        }
    }
}

Expected<String> httpPostRequest(const StringView       url,
                                 const HttpPostPayload& payload,
                                 const HttpPostOptions& options) {
    return httpPostRequestImpl(url, payload, options);
}

void httpPostRequestAsync(Function<void(Expected<String>)> onResult,
                          String                           url,
                          HttpPostPayload                  payload,
                          HttpPostOptions                  options) {

    static ThreadTaskPool sTaskPool(
        [](const int id) { setThreadName(GetCurrentThread(), "_httpPostRequestAsync_" + toStr(id)); });
    sTaskPool.runThreadTask(
        [url      = std::move(url),
         payload  = std::move(payload),
         options  = std::move(options),
         onResult = std::move(onResult)]() { onResult(httpPostRequestImpl(url, payload, options)); });
}

BUFF_NAMESPACE_END
