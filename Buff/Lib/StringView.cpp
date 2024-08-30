#include "Lib/StringView.h"
#include "Lib/Exception.h"
#include "Lib/String.h"
#include <utf8cpp/utf8/checked.h>

BUFF_NAMESPACE_BEGIN

String StringView::getToLower() const {
    return String(*this).getToLower();
}

String StringView::getToUpper() const {
    return String(*this).getToUpper();
}

std::wstring StringView::asWString() const {
    std::wstring result;
    try {
        utf8::utf8to16(mImpl.begin(), mImpl.end(), std::back_inserter(result));
    } catch (...) {
        throw Exception("Invalid UTF-8 string");
    }
    return result;
}

Array<StringView> StringView::explode(const StringView delimiter) const {
    BUFF_ASSERT(delimiter.notEmpty());
    Array<StringView> result;
    int               current = 0;
    while (Optional<int> next = find(delimiter, current)) {
        result.pushBack(getSubstring(current, *next - current));
        current = *next + delimiter.size();
    }
    result.pushBack(getSubstring(current));
    return result;
}

BUFF_NAMESPACE_END
