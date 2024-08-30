#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Pixel.h"

BUFF_NAMESPACE_BEGIN

inline auto iterate2D(const Pixel& size) {
    struct Iterator {
        const Pixel size;
        Pixel       current;

        void operator++() {
            ++current.x;
            if (current.x == size.x) {
                current.x = 0;
                ++current.y;
            }
        }
        Pixel operator*() const {
            BUFF_ASSERT(current.y < size.y);
            return current;
        }
        bool operator==(const Iterator& other) const {
            BUFF_ASSERT(size == other.size);
            return current == other.current;
        }
    };
    struct IteratorWrapper {
        Pixel    size;
        Iterator begin() const {
            return {size, Pixel::ZERO()};
        }
        Iterator end() const {
            return {size, Pixel(0, size.y)};
        }
    };
    return IteratorWrapper {size};
}

inline uint hash(uint x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

BUFF_NAMESPACE_END
