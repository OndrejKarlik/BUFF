#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Math.h"
#include "Lib/Pixel.h"
#include "Lib/Serialization.h"

BUFF_NAMESPACE_BEGIN

template <typename TActualClass, int TDimension>
class Vector {

    template <typename TVector>
    struct Iterator {
        TVector* parent;
        int      index;

        decltype(auto) operator*() const {
            return (*parent)[index];
        }
        void operator++() {
            ++index;
        }
        bool operator==(const Iterator& other) const {
            BUFF_ASSERT(parent == other.parent);
            return index == other.index;
        }
    };

    float& operator[](const int i) {
        return static_cast<TActualClass&>(*this)[i];
    }
    float operator[](const int i) const {
        return static_cast<const TActualClass&>(*this)[i];
    }

public:
    constexpr static TActualClass ZERO() {
        return TActualClass(0.f);
    }
    constexpr static TActualClass ONE() {
        return TActualClass(1.f);
    }

    friend TActualClass operator-(const TActualClass& vector) {
        TActualClass result;
        for (int i = 0; i < TDimension; ++i) {
            result[i] = -vector[i];
        }
        return result;
    }

    friend TActualClass operator+(const TActualClass& first, const TActualClass& second) {
        TActualClass result;
        for (int i = 0; i < TDimension; ++i) {
            result[i] = first[i] + second[i];
        }
        return result;
    }

    friend TActualClass operator-(const TActualClass& first, const TActualClass& second) {
        TActualClass result;
        for (int i = 0; i < TDimension; ++i) {
            result[i] = first[i] - second[i];
        }
        return result;
    }

    friend TActualClass operator*(const TActualClass& first, const TActualClass& second) {
        TActualClass result;
        for (int i = 0; i < TDimension; ++i) {
            result[i] = first[i] * second[i];
        }
        return result;
    }

    friend TActualClass operator/(const TActualClass& first, const TActualClass& second) {
        TActualClass result;
        for (int i = 0; i < TDimension; ++i) {
            result[i] = first[i] / second[i];
        }
        return result;
    }

    friend TActualClass operator*(const TActualClass& vector, const float factor) {
        TActualClass result;
        for (int i = 0; i < TDimension; ++i) {
            result[i] = vector[i] * factor;
        }
        return result;
    }

    friend TActualClass operator/(const TActualClass& vector, const float factor) {
        TActualClass result;
        for (int i = 0; i < TDimension; ++i) {
            result[i] = vector[i] / factor;
        }
        return result;
    }

    friend TActualClass& operator+=(TActualClass& first, const TActualClass& second) {
        for (int i = 0; i < TDimension; ++i) {
            first[i] += second[i];
        }
        return first;
    }

    friend TActualClass& operator-=(TActualClass& first, const TActualClass& second) {
        for (int i = 0; i < TDimension; ++i) {
            first[i] -= second[i];
        }
        return first;
    }

    friend TActualClass& operator*=(TActualClass& vector, const float factor) {
        for (int i = 0; i < TDimension; ++i) {
            vector[i] *= factor;
        }
        return vector;
    }

    friend TActualClass& operator/=(TActualClass& vector, const float factor) {
        for (int i = 0; i < TDimension; ++i) {
            vector[i] /= factor;
        }
        return vector;
    }

    friend float sumElements(const TActualClass& vector) {
        float result = 0;
        for (int i = 0; i < TDimension; ++i) {
            result += vector[i];
        }
        return result;
    }

    friend float distance(const TActualClass& first, const TActualClass& second) {
        const TActualClass square = sqr(first - second);
        return sqrt(sumElements(square));
    }

    friend TActualClass sqr(const TActualClass& vector) {
        return vector * vector;
    }

    auto begin() const {
        return Iterator<const Vector> {this, 0};
    }
    auto begin() {
        return Iterator<Vector> {this, 0};
    }
    auto end() const {
        return Iterator<const Vector> {this, TDimension};
    }
    auto end() {
        return Iterator<Vector> {this, TDimension};
    }

    void serializeCustom(ISerializer& serializer) const {
        serializer.serializeList(*this, "items");
    }
    void deserializeCustom(IDeserializer& deserializer) {
        deserializer.deserializeList(*this, "items");
    }
};

struct Vector2 : Vector<Vector2, 2> {
    float x = NAN;
    float y = NAN;

    constexpr Vector2() = default;
    explicit constexpr Vector2(const Pixel value)
        : x(float(value.x))
        , y(float(value.y)) {}
    constexpr Vector2(const float x, const float y)
        : x(x)
        , y(y) {}
    explicit constexpr Vector2(const float onlyValue)
        : x(onlyValue)
        , y(onlyValue) {}

    float& operator[](const int i) {
        BUFF_ASSERT(unsigned(i) < 2);
        return i == 0 ? x : y;
    }
    float operator[](const int i) const {
        BUFF_ASSERT(unsigned(i) < 2);
        return i == 0 ? x : y;
    }
};

inline bool isReal(const Vector2 in) {
    return isReal(in.x) && isReal(in.y);
}

BUFF_NAMESPACE_END
