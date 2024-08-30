#include "Lib/Cryptography.h"
#include "Lib/containers/Array.h"
#include "Lib/containers/StaticArray.h"
#include "Lib/Optional.h"
#include "Lib/String.h"
#ifdef __EMSCRIPTEN__
#    include <iomanip>
#endif

BUFF_NAMESPACE_BEGIN

// ===========================================================================================================
// Base64 - based on https://gist.github.com/tomykaira/f0fd86b6c73063283afe550bc5d77594 (MIT license)
// ===========================================================================================================

String toBase64(const StringView input) {
    return toBase64(ArrayView(reinterpret_cast<const std::byte*>(input.data()), input.size()));
}

String toBase64(const ArrayView<const std::byte> data) {
    static constexpr StaticArray<char, 64> ENCODING_TABLE = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

    const int inLen = int(data.size());
    String    ret;
    int       i = 0;

    for (; i < inLen - 2; i += 3) {
        ret << ENCODING_TABLE[(char(data[i]) >> 2) & 0x3F];
        ret << ENCODING_TABLE[((char(data[i]) & 0x3) << 4) | ((char(data[i + 1]) & 0xF0) >> 4)];
        ret << ENCODING_TABLE[((char(data[i + 1]) & 0xF) << 2) | ((char(data[i + 2]) & 0xC0) >> 6)];
        ret << ENCODING_TABLE[char(data[i + 2]) & 0x3F];
    }
    if (i < inLen) {
        ret << ENCODING_TABLE[(char(data[i]) >> 2) & 0x3F];
        if (i == (inLen - 1)) {
            ret << ENCODING_TABLE[((char(data[i]) & 0x3) << 4)];
            ret << '=';
        } else {
            ret << ENCODING_TABLE[((char(data[i]) & 0x3) << 4) | ((char(data[i + 1]) & 0xF0) >> 4)];
            ret << ENCODING_TABLE[((char(data[i + 1]) & 0xF) << 2)];
        }
        ret << '=';
    }

    return ret;
}

Optional<Array<std::byte>> fromBase64(const StringView input) {
    static constexpr StaticArray<uint8, 256> DECODING_TABLE = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,
        7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};

    const int inLen = input.size();
    if (inLen % 4 != 0) {
        return {}; //"Input data size is not a multiple of 4";}
    }
    if (inLen == 0) {
        return Array<std::byte>(); // Not error but empty result
    }

    int outLen = inLen / 4 * 3;
    if (input[inLen - 1] == '=') {
        outLen--;
    }
    if (input[inLen - 2] == '=') {
        outLen--;
    }

    Array<std::byte> out;
    out.resize(outLen);

    for (int i = 0, j = 0; i < inLen;) {
        const uint a = input[i] == '=' ? 0 & i++ : DECODING_TABLE[static_cast<int>(input[i++])];
        const uint b = input[i] == '=' ? 0 & i++ : DECODING_TABLE[static_cast<int>(input[i++])];
        const uint c = input[i] == '=' ? 0 & i++ : DECODING_TABLE[static_cast<int>(input[i++])];
        const uint d = input[i] == '=' ? 0 & i++ : DECODING_TABLE[static_cast<int>(input[i++])];

        const uint triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

        if (j < outLen) {
            out[j++] = std::byte((triple >> 2 * 8) & 0xFF);
        }
        if (j < outLen) {
            out[j++] = std::byte((triple >> 1 * 8) & 0xFF);
        }
        if (j < outLen) {
            out[j++] = std::byte((triple >> 0 * 8) & 0xFF);
        }
    }

    return out;
}

// ===========================================================================================================
// MD5 - from https://github.com/kerukuro/digestpp (Unlicense)
// ===========================================================================================================

class Md5 {
    StaticArray<uint, 4>  mH     = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    StaticArray<char, 64> mM     = {};
    size_t                mPos   = 0;
    uint64_t              mTotal = 0;

    constexpr static StaticArray<uint, 64> K = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

    constexpr static StaticArray<uint8, 64> S = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                                 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                                                 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                                 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

public:
    void update(const char* data, size_t len) {
        absorbBytes(data, len, 64, 64, mM.data(), mPos, mTotal, [this](const char* data, size_t len) {
            transform(data, len);
        });
    }

    StaticArray<std::byte, 16> final() {
        mTotal += mPos * 8;
        mM[mPos++] = static_cast<char>(0x80);
        if (mPos > 56) {
            if (mPos != 64) {
                memset(&mM[mPos], 0, 64 - mPos);
            }
            transform(mM.data(), 1);
            mPos = 0;
        }
        memset(mM.data() + mPos, 0, 56 - mPos);
        memcpy(mM.data() + (64 - 8), &mTotal, 64 / 8);
        transform(mM.data(), 1);
        StaticArray<std::byte, 16> result;
        memcpy(result.data(), mH.data(), 16);
        return result;
    }

private:
    void transform(const char* data, const size_t numBlks) {
        const uint* m = reinterpret_cast<const uint*>(data);
        for (uint64_t blk = 0; blk < numBlks; blk++, m += 16) {
            uint a = mH[0];
            uint b = mH[1];
            uint c = mH[2];
            uint d = mH[3];

            roundf(0, a, b, c, d, m);
            roundf(1, d, a, b, c, m);
            roundf(2, c, d, a, b, m);
            roundf(3, b, c, d, a, m);
            roundf(4, a, b, c, d, m);
            roundf(5, d, a, b, c, m);
            roundf(6, c, d, a, b, m);
            roundf(7, b, c, d, a, m);
            roundf(8, a, b, c, d, m);
            roundf(9, d, a, b, c, m);
            roundf(10, c, d, a, b, m);
            roundf(11, b, c, d, a, m);
            roundf(12, a, b, c, d, m);
            roundf(13, d, a, b, c, m);
            roundf(14, c, d, a, b, m);
            roundf(15, b, c, d, a, m);

            roundg(16, a, b, c, d, m);
            roundg(17, d, a, b, c, m);
            roundg(18, c, d, a, b, m);
            roundg(19, b, c, d, a, m);
            roundg(20, a, b, c, d, m);
            roundg(21, d, a, b, c, m);
            roundg(22, c, d, a, b, m);
            roundg(23, b, c, d, a, m);
            roundg(24, a, b, c, d, m);
            roundg(25, d, a, b, c, m);
            roundg(26, c, d, a, b, m);
            roundg(27, b, c, d, a, m);
            roundg(28, a, b, c, d, m);
            roundg(29, d, a, b, c, m);
            roundg(30, c, d, a, b, m);
            roundg(31, b, c, d, a, m);

            roundh(32, a, b, c, d, m);
            roundh(33, d, a, b, c, m);
            roundh(34, c, d, a, b, m);
            roundh(35, b, c, d, a, m);
            roundh(36, a, b, c, d, m);
            roundh(37, d, a, b, c, m);
            roundh(38, c, d, a, b, m);
            roundh(39, b, c, d, a, m);
            roundh(40, a, b, c, d, m);
            roundh(41, d, a, b, c, m);
            roundh(42, c, d, a, b, m);
            roundh(43, b, c, d, a, m);
            roundh(44, a, b, c, d, m);
            roundh(45, d, a, b, c, m);
            roundh(46, c, d, a, b, m);
            roundh(47, b, c, d, a, m);

            roundi(48, a, b, c, d, m);
            roundi(49, d, a, b, c, m);
            roundi(50, c, d, a, b, m);
            roundi(51, b, c, d, a, m);
            roundi(52, a, b, c, d, m);
            roundi(53, d, a, b, c, m);
            roundi(54, c, d, a, b, m);
            roundi(55, b, c, d, a, m);
            roundi(56, a, b, c, d, m);
            roundi(57, d, a, b, c, m);
            roundi(58, c, d, a, b, m);
            roundi(59, b, c, d, a, m);
            roundi(60, a, b, c, d, m);
            roundi(61, d, a, b, c, m);
            roundi(62, c, d, a, b, m);
            roundi(63, b, c, d, a, m);

            mH[0] += a;
            mH[1] += b;
            mH[2] += c;
            mH[3] += d;
        }
    }

    // Rotate 32-bit unsigned integer to the left.
    static uint rotateLeft(const uint& x, unsigned n) {
        return (x << n) | (x >> (32 - n));
    }

    // Accumulate data and call the transformation function for full blocks.
    template <typename T, typename TF>
    static void absorbBytes(const char*  data,
                            size_t       len,
                            const size_t bs,
                            const size_t bschk,
                            char*        m,
                            size_t&      pos,
                            T&           total,
                            TF           transform) {
        if (pos && pos + len >= bschk) {
            memcpy(m + pos, data, bs - pos);
            transform(m, 1);
            len -= bs - pos;
            data += bs - pos;
            total += bs * 8;
            pos = 0;
        }
        if (len >= bschk) {
            const size_t blocks = (len + bs - bschk) / bs;
            const size_t bytes  = blocks * bs;
            transform(data, blocks);
            len -= bytes;
            data += bytes;
            total += (bytes) * 8;
        }
        memcpy(m + pos, data, len);
        pos += len;
    }

    static void roundf(const int round, uint& a, const uint b, const uint c, const uint d, const uint* m) {
        a = b + rotateLeft(a + (d ^ (b & (c ^ d))) + K[round] + m[round], S[round]);
    }

    static void roundg(const int round, uint& a, const uint b, const uint c, const uint d, const uint* m) {
        a = b + rotateLeft(a + (c ^ (d & (b ^ c))) + K[round] + m[(5 * round + 1) % 16], S[round]);
    }

    static void roundh(const int round, uint& a, const uint b, const uint c, const uint d, const uint* m) {
        a = b + rotateLeft(a + (b ^ c ^ d) + K[round] + m[(3 * round + 5) % 16], S[round]);
    }

    static void roundi(const int round, uint& a, const uint b, const uint c, const uint d, const uint* m) {
        a = b + rotateLeft(a + (c ^ (b | ~d)) + K[round] + m[(7 * round) % 16], S[round]);
    }
};

String getMd5Ascii(const StringView string) {
    Md5 md5;
    md5.update(string.data(), string.size());
    const StaticArray<std::byte, 16> binary = md5.final();
    String                           result;
    for (auto& i : binary) {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2) << std::hex << int(i);
        result << stream.str();
    }
    return result;
}

BUFF_NAMESPACE_END
