#include "Lib/Json.h"
#include "Lib/containers/SmallArray.h"
#include "Lib/Exception.h"
#include "Lib/Function.h"

BUFF_NAMESPACE_BEGIN

static String indent(const Optional<int> indentation) {
    String result;
    for (int i = 0; i < indentation.valueOr(0); ++i) {
        result << "    ";
    }
    return result;
}

// ===========================================================================================================
// JsObject
// ===========================================================================================================

// Must be in cpp file because of reference to undefined object JsValue
JsObject::JsObject(const JsObject& other)            = default;
JsObject::JsObject(JsObject&& other)                 = default;
JsObject& JsObject::operator=(const JsObject& other) = default;
JsObject& JsObject::operator=(JsObject&& other)      = default;

JsObject::JsObject(const ArrayView<const std::pair<String, JsValue>> values) {
    mValues.insertRange(values);
}

JsValue& JsObject::operator[](const StringView name) {
    return mValues[name];
}

const JsValue* JsObject::find(const StringView name) const {
    return mValues.find(name);
}

void JsObject::visitAllValues(const Function<void(const String&, const JsValue&)>& functor) const {
    for (auto& i : mValues) {
        functor(i.first, i.second);
    }
}

String JsObject::toJson(const Optional<int> indentation) const {
    // TODO: Seems unquoted keys are only allowed in JSON5: https://en.wikipedia.org/wiki/JSON#JSON5
    // auto isValidKey = [](const StringView identifier) {
    //    // TODO: Improve - allow numbers for example
    //    return std::ranges::all_of(identifier, [](const char c) { return isLetter(c) || c == '_'; });
    //};
    const String        newline         = indentation ? "\n" : "";
    const Optional<int> indentationNext = indentation ? Optional(*indentation + 1) : NULL_OPTIONAL;
    return "{" + newline +
           listToStr(mValues,
                     ","_S + (indentation ? "\n" : " "),
                     [&](const auto& pair) {
                         String result;
                         if (indentation) {
                             result << indent(indentationNext);
                         }
                         result << /*(isValidKey(pair.first) ? pair.first :*/ pair.first.getQuoted() /*)*/ +
                                       ": " + pair.second.toJson(indentationNext);
                         return result;
                     }) +
           newline + indent(indentation) + "}";
}

// ===========================================================================================================
// JsArray
// ===========================================================================================================

// Must be in cpp file because of reference to undefined object JsValue
JsArray::JsArray(const JsArray& other)            = default;
JsArray::JsArray(JsArray&& other)                 = default;
JsArray& JsArray::operator=(const JsArray& other) = default;
JsArray& JsArray::operator=(JsArray&& other)      = default;

JsValue& JsArray::operator[](const int64 index) {
    BUFF_ASSERT(index >= 0);
    mHighestIndex = max(mHighestIndex, index);
    return mValues[index];
}
const JsValue& JsArray::operator[](const int64 index) const {
    BUFF_ASSERT(mValues.contains(index));
    return *mValues.find(index);
}

void JsArray::push(JsValue value) {
    mValues[++mHighestIndex] = std::move(value);
}
const JsValue& JsArray::peek() const {
    BUFF_ASSERT(mValues.notEmpty());
    return *mValues.find(mHighestIndex);
}
JsValue& JsArray::peek() {
    BUFF_ASSERT(mValues.notEmpty());
    return *mValues.find(mHighestIndex);
}

String JsArray::toJson(const Optional<int> indentation) const {
    String result       = "[";
    int    currentIndex = 0;
    for (auto& [index, value] : mValues) {
        while (currentIndex < index) {
            result << "undefined, ";
            ++currentIndex;
        }
        result << value.toJson(indentation) << ", ";
        ++currentIndex;
    }
    if (mValues.notEmpty()) {
        BUFF_ASSERT(result[result.size() - 2] == ',');
        result.erase(result.size() - 2);
    }
    return result + "]";
}

void JsArray::visitAllValues(const Function<void(int64, const JsValue&)>& functor) const {
    for (auto& i : mValues) {
        functor(i.first, i.second);
    }
}

// ===========================================================================================================
// JsValue
// ===========================================================================================================

JsValue& JsValue::operator[](const StringView name) {
    return mImpl.get<JsObject>()[name];
}
const JsValue* JsValue::find(StringView name) const {
    return mImpl.get<JsObject>().find(name);
}
JsValue& JsValue::operator[](const int index) {
    return mImpl.get<JsArray>()[index];
}
const JsValue& JsValue::operator[](const int index) const {
    return mImpl.get<JsArray>()[index];
}
void JsValue::push(JsValue value) {
    return mImpl.get<JsArray>().push(std::move(value));
}
const JsValue& JsValue::peek() const {
    return mImpl.get<JsArray>().peek();
}
JsValue& JsValue::peek() {
    return mImpl.get<JsArray>().peek();
}

String JsValue::toJson(const Optional<int> indentation) const {
    return mImpl.visit([](const Undefined BUFF_UNUSED(x)) { return String("undefined"); },
                       [](const Null BUFF_UNUSED(x)) { return String("null"); },
                       [](const bool x) { return String(x ? "true" : "false"); },
                       [](const double x) { return toStr(x); },
                       [](const String& x) { return escapeJson(x).getQuoted(); },
                       [&](const JsArray& x) { return x.toJson(indentation); },
                       [&](const JsObject& x) { return x.toJson(indentation); });
}

const Array<std::pair<char, String>> JSON_ESCAPE_PAIRS = {
    {'\"', "\\\""},
    {'\\', "\\\\"},
    {'\b', "\\b" },
    {'\f', "\\f" },
    {'\n', "\\n" },
    {'\r', "\\r" },
    {'\t', "\\t" },
};

String escapeJson(const StringView str) {
    String result;
    for (const unsigned char ch : str) {
        if (ch < ' ' || ch == anyOf('\\', '\"')) {
            bool found = false;
            for (const auto& [from, to] : JSON_ESCAPE_PAIRS) {
                if (ch == from) {
                    result << to;
                    found = true;
                    break;
                }
            }
            if (!found) {
                result << "\\u" << leftPad(toStrHexadecimal(int(ch)), 4, '0');
            }
        } else {
            result << ch;
        }
    }
    return result;
}

String unEscapeJson(const StringView str) {
    Array<char> result;
    for (int i = 0; i < str.size(); ++i) {
        if (str[i] == '\\') {
            ++i;
            if (i == str.size()) {
                throw Exception("Unexpected end of input");
            }
            if (str[i] == 'u') {
                if (i >= str.size() - 4) {
                    throw Exception("Unexpected end of input");
                }
                const Optional<int64> from = fromStrHexadecimal(str.getSubstring(i + 1, 4));
                i += 4;
                if (!from || *from > 127) {
                    throw Exception("Invalid unicode character");
                }
                result.pushBack(char(*from));
            } else {
                bool found = false;
                for (const auto& [from, to] : JSON_ESCAPE_PAIRS) {
                    BUFF_ASSERT(to.size() == 2);
                    if (str[i] == to[1]) {
                        result.pushBack(from);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    throw Exception("Invalid escape sequence");
                }
            }
        } else {
            result.pushBack(str[i]);
        }
    }
    return String::fromArray(result);
}

// ===========================================================================================================
// parseJson
// ===========================================================================================================

class Reader {
    StringView mStr;
    int        mPos = 0;

public:
    explicit Reader(const StringView& str)
        : mStr(str) {
        skipWhitespace();
    }
    bool isDone() const {
        return mPos == mStr.size();
    }

    StringView getNextToken() {
        if (isDone()) {
            throw Exception("Unexpected EOF");
        }
        BUFF_ASSERT(!isWhitespace(mStr[mPos]));

        int                                   count         = 1;
        const char                            first         = get();
        constexpr static StaticArray<char, 6> SINGLE_TOKENS = {'{', '}', '[', ']', ',', ':'};
        if (SINGLE_TOKENS.contains(first)) {
            // Done
        } else if (first == '\"') { // String
            while (true) {
                const char next = get();
                ++count;
                if (next == '\\') {
                    ++count;
                    get(); // Read next char as escaped
                } else if (next == '\"') {
                    break;
                }
            }
        } else if (std::isalpha(first)) { // keywords (null, true, false)
            while (peekChar() && std::isalpha(*peekChar())) {
                ++count;
                get();
            }
        } else if (first == '-' || std::isdigit(first)) {
            while (peekChar() && *peekChar() != anyOf(',', ']', '}', '\t', ' ', '\n')) {
                ++count;
                get();
            }
        }
        const StringView result = mStr.getSubstring(mPos - count, count);
        skipWhitespace();
        return result;
    }

    void getNextTokenChecked(const StringView value) {
        const StringView token = getNextToken();
        if (token != value) {
            throw Exception("Expected " + value + ", got " + token);
        }
    }

    Optional<char> peekChar() const {
        return isDone() ? Optional<char>() : Optional(mStr[mPos]);
    }

private:
    void skipWhitespace() {
        while (!isDone() && isWhitespace(mStr[mPos])) {
            ++mPos;
        }
    }
    char get() {
        if (isDone()) {
            throw Exception("Unexpected EOF");
        }
        return mStr[mPos++];
    }
};

JsObject parseObject(Reader& reader);
JsArray  parseArray(Reader& reader);

static String unescapeString(const StringView read) {
    if (!read.startsWith("\"") || !read.endsWith("\"") || read.size() < 2) {
        throw Exception("Wrong key format");
    }
    return unEscapeJson(read.getSubstring(1, read.size() - 2));
}

static JsValue readValue(Reader& reader) {
    const StringView token = reader.getNextToken();
    if (token[0] == '"') {
        return unescapeString(token);
    } else if (token[0] == '{') {
        return parseObject(reader);
    } else if (token[0] == '[') {
        return parseArray(reader);
    } else if (token == "undefined") {
        return JsValue::Undefined {};
    } else if (token == "null") {
        return JsValue::Null {};
    } else if (token == "true") {
        return true;
    } else if (token == "false") {
        return false;
    } else {
#ifdef __EMSCRIPTEN__
        // 2024-06 Still missing std::from_chars...
        return std::atof(String(token).asCString());
#else
        double                       result;
        const std::from_chars_result fromCharsResult =
            std::from_chars(token.data(), token.data() + token.size(), result);
        BUFF_ASSERT(fromCharsResult.ptr == token.data() + token.size());
        return result;
#endif
    }
}

JsObject parseObject(Reader& reader) {
    JsObject result;
    bool     first = true;
    while (reader.peekChar() != '}') {
        if (!first) {
            reader.getNextTokenChecked(",");
        } else {
            first = false;
        }
        const StringView key = reader.getNextToken();
        reader.getNextTokenChecked(":");
        result[unescapeString(key)] = readValue(reader);
    }
    reader.getNextTokenChecked("}");
    return result;
}

JsArray parseArray(Reader& reader) {
    JsArray result;
    bool    first = true;
    while (reader.peekChar() != ']') {
        if (!first) {
            reader.getNextTokenChecked(",");
        } else {
            first = false;
        }
        if (reader.peekChar() == ',') {
            result.push({});
        } else {
            result.push(readValue(reader));
        }
    }
    reader.getNextTokenChecked("]");
    return result;
}

Optional<JsObject> parseJson(const StringView json) {
    Reader reader(json);

    try {
        reader.getNextTokenChecked("{");
        JsObject result = parseObject(reader);
        if (!reader.isDone()) {
            throw Exception("More than 1 object in string");
        }
        return Optional(std::move(result));
    } catch (Exception& ex) {
        BUFF_ASSERT(false, ex.what());
        return {};
    }
}

BUFF_NAMESPACE_END
