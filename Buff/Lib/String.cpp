#include "Lib/String.h"
#include "Lib/containers/Array.h"
#include "Lib/Exception.h"
#include "Lib/Utils.h"
#include <utf8cpp/utf8/checked.h>
#ifdef __EMSCRIPTEN__
#    include <iomanip>
#endif

BUFF_NAMESPACE_BEGIN

String::String(const std::wstring& str) {
    if constexpr (BUFF_DEBUG) {
        for (auto& i : str) {
            BUFF_ASSERT(i != L'\0');
        }
    }

    if constexpr (BUFF_DEBUG) {
        // TODO: std::ranges version not supported by emscripten yet
        if (std::ranges::all_of(str, isascii)) { // Faster in debug...
            mImpl.reserve(str.size() + 1);
            for (const wchar_t i : str) {
                BUFF_ASSERT(i <= 127); // iswascii does not work with emscripten
                mImpl.pushBack(char(i));
            }
            afterModification();
            return;
        }
    }
    struct BackInserter {
        Array<char>* array;
        void         operator=(const char value) {
            array->pushBack(value);
        }
        BackInserter& operator++(int) {
            return *this;
        }
        BackInserter& operator*() {
            return *this;
        }
    };
    utf8::utf16to8(str.begin(), str.end(), BackInserter {&mImpl});
    afterModification();
}

std::wstring String::asWString() const {
    if (isEmpty()) {
        return {};
    }
    std::wstring result;
    try {
        utf8::utf8to16(mImpl.begin(), mImpl.end() - 1, std::back_inserter(result));
    } catch (...) {
        throw Exception("Invalid UTF-8 string");
    }
    return result;
}

String String::fromCp1252(const char* array, const int numChars) {
    //"€ � ‚ ƒ „ … † ‡ ˆ ‰ Š ‹ Œ � Ž � � ‘ ’ “ ” • – — ˜ ™ š › œ � ž Ÿ   ¡ ¢ £ ¤ ¥ ¦ § ¨ © ª « ¬ ­ ® ¯
    // ° ± ² ³ ´ µ ¶ · ¸ ¹ º » ¼ ½ ¾ ¿ À Á Â Ã Ä Å Æ Ç È É Ê Ë Ì Í Î Ï Ð Ñ Ò Ó Ô Õ Ö × Ø Ù Ú Û Ü Ý Þ ß
    // à á â ã ä å æ ç è é ê ë ì í î ï ð ñ ò ó ô õ ö ÷ ø ù ú û ü ý þ ÿ ";
    static constexpr StringView UTF8_CONVERSIONS_JOINED =
        "\u20AC \uFFFD \u201A \u0192 \u201E \u2026 \u2020 \u2021 \u02C6 \u2030 \u0160 \u2039 \u0152 "
        "\uFFFD \u017D \uFFFD \uFFFD \u2018 \u2019 \u201C \u201D \u2022 \u2013 \u2014 \u02DC \u2122 "
        "\u0161 \u203A \u0153 \uFFFD \u017E \u0178 \u00A0 \u00A1 \u00A2 \u00A3 \u00A4 \u00A5 \u00A6 "
        "\u00A7 \u00A8 \u00A9 \u00AA \u00AB \u00AC \u00AD \u00AE \u00AF \u00B0 \u00B1 \u00B2 \u00B3 "
        "\u00B4 \u00B5 \u00B6 \u00B7 \u00B8 \u00B9 \u00BA \u00BB \u00BC \u00BD \u00BE \u00BF \u00C0 "
        "\u00C1 \u00C2 \u00C3 \u00C4 \u00C5 \u00C6 \u00C7 \u00C8 \u00C9 \u00CA \u00CB \u00CC \u00CD "
        "\u00CE \u00CF \u00D0 \u00D1 \u00D2 \u00D3 \u00D4 \u00D5 \u00D6 \u00D7 \u00D8 \u00D9 \u00DA "
        "\u00DB \u00DC \u00DD \u00DE \u00DF \u00E0 \u00E1 \u00E2 \u00E3 \u00E4 \u00E5 \u00E6 \u00E7 "
        "\u00E8 \u00E9 \u00EA \u00EB \u00EC \u00ED \u00EE \u00EF \u00F0 \u00F1 \u00F2 \u00F3 \u00F4 "
        "\u00F5 \u00F6 \u00F7 \u00F8 \u00F9 \u00FA \u00FB \u00FC \u00FD \u00FE \u00FF";
    static const Array<StringView> UTF8_CONVERSIONS = UTF8_CONVERSIONS_JOINED.explode(" ");
    BUFF_ASSERT(UTF8_CONVERSIONS.size() == 128);

    Array<char> result;
    result.reserve(numChars);
    for (const int i : range(numChars)) {
        const char in = array[i];
        if (in >= 0) {
            result.pushBack(in);
        } else {
            result.pushBackRange(ArrayView(UTF8_CONVERSIONS[static_cast<unsigned char>(in) - 128]));
        }
    }
    return fromArray(std::move(result));
}

String& String::operator=(const StringView stringView) {
    if (stringView.size() > 0) { // stringView could overlap with String!
        mImpl.resize(stringView.size() + 1);
        std::memmove(mImpl.data(), stringView.data(), stringView.size());
        mImpl.back() = '\0';
    } else {
        mImpl.resize(0);
    }
    // assertValidity();
    return *this;
}

bool String::isAscii() const {
    return std::ranges::all_of(mImpl, Buff::isAscii);
}

bool String::isValidUtf8() const {
    return utf8::is_valid(mImpl.begin(), mImpl.end());
}

String String::getQuoted() const {
    return "\"" + *this + "\"";
}

String String::getWithReplaceAll(const StringView& pattern, const StringView& replacement) const {
    String result = *this;
    result.replaceAll(pattern, replacement);
    return result;
}

String operator+(const String& a, const String& b) {
    String retVal = a;
    retVal << b;
    return retVal;
}

String formatBigNumber(const int64 number) {
    struct SeparateThousands : std::numpunct<char> {
        virtual char_type do_thousands_sep() const override {
            return ' ';
        } // separate with commas
        virtual string_type do_grouping() const override {
            return "\3";
        } // groups of 3 digit
    };

    std::stringstream tmp;
    tmp.imbue(std::locale(tmp.getloc(), new SeparateThousands()));

    tmp << number;
    return tmp.str();
}

String toStrHexadecimal(const int64 number) {
    std::stringstream tmp;
    tmp << std::hex << number;
    return tmp.str();
}

Optional<int64> fromStrHexadecimal(const StringView number) {
    int64             result;
    std::stringstream tmp;
    tmp << number;
    tmp >> std::hex >> result;
    if (tmp.fail()) {
        return {};
    }
    return result;
}

String formatFloat(const double number, const FormatFloatParams& params) {
    BUFF_ASSERT(params.maxDecimals >= 0);
    std::stringstream tmp;
    tmp << std::fixed << std::setprecision(params.maxDecimals) << number;
    String res = tmp.str();
    if (!params.forceDecimals) {
        while (res.endsWith("0")) {
            res.erase(res.size() - 1);
        }
        if (res.endsWith(".")) {
            res.erase(res.size() - 1);
        }
    }
    return res;
}

String String::getToLower() const {
    String result = *this;
    result.beforeModification();
    for (auto& c : result.mImpl) {
        c = safeIntegerCast<char>(::tolower(c));
    }
    result.afterModification();
    return result;
}

String String::getToUpper() const {
    String result = *this;
    result.beforeModification();
    for (auto& c : result.mImpl) {
        c = safeIntegerCast<char>(::toupper(c));
    }
    result.afterModification();
    return result;
}

LevenshteinDistance getLevenshteinDistance(const StringView& from, const StringView& to) {
    if (from == to) {
        return LevenshteinDistance {.distance = 0};
    }

    // Algorithm from https://en.wikipedia.org/wiki/Levenshtein_distance#Iterative_with_two_matrix_rows

    // In addition to the result we will eventually return, we also need to track if there is an active
    // "streak" of changes in this chain. Streak is broken when a no-modification step happens.
    struct WorkingDistance : LevenshteinDistance {
        bool hasStreak = false;
    };
    // create two work vectors of integer distances
    Array<WorkingDistance> v0(to.size() + 1);
    Array<WorkingDistance> v1(to.size() + 1);

    // initialize v0 (the previous row of distances)
    // this row is A[0][i]: edit distance from an empty s to t;
    // that distance is the number of characters to append to  s to make t.
    for (auto& it : iterate(v0)) {
        it->distance = int(it.index());
        if (it.notFirst()) {
            it->hasStreak = true;
            it->changes.pushBack({
                .where = 0,
                .from  = 0,
                .count = int(it.index()),
                .type  = LevenshteinDistance::INSERTION,
            });
        }
    }
    for (int fromIdx = 0; fromIdx < from.size(); ++fromIdx) {
        // calculate v1 (current row distances) from the previous row v0

        // first element of v1 is A[i + 1][0]
        //   edit distance is delete (i + 1) chars from s to match empty t
        v1[0].distance  = fromIdx + 1;
        v1[0].hasStreak = true;
        v1[0].changes.resize(1);
        v1[0].changes[0] = LevenshteinDistance::Change {
            .where = 0,
            .from  = 0,
            .count = fromIdx + 1,
            .type  = LevenshteinDistance::DELETION,
        };

        // use formula to fill in the rest of the row
        for (int toIdx = 0; toIdx < to.size(); ++toIdx) {
            // calculating costs for A[i + 1][j + 1]
            const bool                       subIsFree = from[fromIdx] == to[toIdx];
            StaticArray<WorkingDistance*, 3> options   = {
                &v1[toIdx],     // Insertion
                &v0[toIdx + 1], // Deletion
                &v0[toIdx],     // Substitution
            };

            // Now find the preferred option what to do, based on what the last operation was. Double the
            // costs and subtract 1 when the operation would be a continuation. This will not change the
            // choice on different scores, but it will break ties in a way to minimize number of operations
            StaticArray<int, 3> costs;
            for (int k = 0; k < 3; ++k) {
                auto& option = *options[k];
                costs[k]     = option.distance * 2;
                if (LevenshteinDistance::Type(k) != LevenshteinDistance::SUBSTITUTION || !subIsFree) {
                    costs[k] += 2;
                }
                if (option.hasStreak && int(option.changes.back().type) == k) {
                    // Continuation
                    --costs[k];
                }
            }
            const int choice = argMin(costs[0], costs[1], costs[2]);
            v1[toIdx + 1]    = *options[choice];
            if (LevenshteinDistance::Type(choice) == LevenshteinDistance::SUBSTITUTION && subIsFree) {
                // No modification to string necessary
                v1[toIdx + 1].hasStreak = false;
            } else if (costs[choice] & 1) { // Continuation of previous modification is best
                v1[toIdx + 1].changes.back().count += 1;
                v1[toIdx + 1].hasStreak = true;
                v1[toIdx + 1].distance += 1;
            } else { // We need to start a different operation
                v1[toIdx + 1].hasStreak = true;
                v1[toIdx + 1].distance += 1;
                const StaticArray<int, 3> whereOptions = {
                    // "Magic" found by trial and error
                    toIdx,     // Insertion
                    toIdx + 1, // Deletion
                    toIdx,     // Substitution
                };
                v1[toIdx + 1].changes.pushBack(LevenshteinDistance::Change {
                    .where = whereOptions[choice],
                    .from  = toIdx,
                    .count = 1,
                    .type  = LevenshteinDistance::Type(choice),
                });
            }
        }
        // copy v1 (current row) to v0 (previous row) for next iteration
        // since data in v1 is always invalidated, a swap without copy could be more efficient
        std::swap(v0, v1);
    }
    // after the last swap, the results of v1 are now in v0
    return LevenshteinDistance(v0.back());
}

String leftPad(const StringView input, const int desiredLength, const char paddingChar) {
    BUFF_ASSERT(desiredLength >= 0);
    const int numPad = desiredLength - input.size();
    if (numPad <= 0) {
        return input;
    }
    String result;
    result.reserve(desiredLength);
    for (int i = 0; i < numPad; ++i) {
        result << paddingChar;
    }
    result << input;
    return result;
}

BUFF_NAMESPACE_END
