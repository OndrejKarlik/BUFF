#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include "Lib/Expected.h"
#include "Lib/String.h"
#include "Lib/Time.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
class Function;

/// Implements https://en.wikipedia.org/wiki/Percent-encoding
String urlEncode(StringView input, bool forPostParams = false);

struct HttpPostOptions {
    Duration timeout = Duration::seconds(30);
};

class HttpPostPayload {
    String mPayload;

public:
    explicit HttpPostPayload(String payload)
        : mPayload(std::move(payload)) {}
    explicit HttpPostPayload(ArrayView<const std::pair<String, String>> parameters);

    StringView getPayload() const {
        return mPayload;
    }
};

/// \param url
/// Do not include http/https prefix!. Examples:
/// example.com/a/b
/// subdomain.example.com:1234/a/b
Expected<String> httpPostRequest(StringView             url,
                                 const HttpPostPayload& payload,
                                 const HttpPostOptions& options = {});

/// Returns immediately
/// \param onResult
/// Gets called when the request finished (successfully or unsuccessfully) from a different thread.
void httpPostRequestAsync(Function<void(Expected<String>)> onResult,
                          String                           url,
                          HttpPostPayload                  payload,
                          HttpPostOptions                  options = {});

BUFF_NAMESPACE_END
