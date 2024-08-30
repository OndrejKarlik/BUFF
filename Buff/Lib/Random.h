#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/StaticArray.h"
#include "Lib/Pixel.h"
#include "Lib/Vector.h"
#include <random>

BUFF_NAMESPACE_BEGIN

namespace Detail {

/// According to https://prng.di.unimi.it/xoshiro256starstar.c
class Xoshiro256StarStar {
    StaticArray<uint64, 4> mState;

public:
    explicit Xoshiro256StarStar(uint64 seed) {
        // The paper recommends using a different PRNG to seed the internal state, such as Split Mix 64
        const auto splitMix64 = [](uint64& state) {
            state += 0x9e3779b97f4a7c15;
            uint64 z = state;
            z        = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
            z        = (z ^ (z >> 27)) * 0x94d049bb133111eb;
            return z ^ (z >> 31);
        };
        for (auto& i : mState) {
            i = splitMix64(seed);
        }
    }

    uint64 next() {
        const uint64_t result = rotateLeft(mState[1] * 5, 7) * 9;
        const uint64_t t      = mState[1] << 17;
        mState[2] ^= mState[0];
        mState[3] ^= mState[1];
        mState[1] ^= mState[2];
        mState[0] ^= mState[3];
        mState[2] ^= t;
        mState[3] = rotateLeft(mState[3], 45);
        return result;
    }

    static constexpr uint64 min() {
        return 0;
    }
    static constexpr uint64 max() {
        return std::numeric_limits<uint64>::max();
    }

    void enumerateStructMembers(auto&& functor) {
        functor(mState, "state");
        BUFF_ASSERT_SIZEOF(sizeof(mState));
    }

private:
    static uint64 rotateLeft(const uint64_t x, const int k) {
        return (x << k) | (x >> (64 - k));
    }
};

} // namespace Detail

class RandomNumberGenerator {
    Detail::Xoshiro256StarStar mImpl;

public:
    explicit RandomNumberGenerator(const uint64 seed = 0x1337'BEEF'1029'3847)
        : mImpl(seed) {}

    uint64 operator()() {
        return mImpl.next();
    }

    // ReSharper disable once CppInconsistentNaming
    using result_type = decltype(mImpl.next());

    static constexpr auto min() {
        return decltype(mImpl)::min();
    }
    static constexpr auto max() {
        return decltype(mImpl)::max();
    }

    // max is inclusive!
    int getRandomInt(const int max) {
        return getRandomInt(0, max);
    }
    int getRandomInt(const int min, const int max) {
        BUFF_ASSERT(max >= min);
        std::uniform_int_distribution distr(min, max); // emscripten STL does not allow const here
        return distr(*this);
    }
    float getRandomFloat(const float max) {
        return getRandomFloat(0.f, max);
    }
    float getRandomFloat(const float min, const float max) {
        BUFF_ASSERT(max > min);
        // emscripten STL does not allow const here
        std::uniform_real_distribution distr(min, max);
        return distr(*this);
    }
    Vector2 getRandomVector2(const Vector2 max) {
        return getRandomVector2(Vector2::ZERO(), max);
    }
    Vector2 getRandomVector2(const Vector2 min, const Vector2 max) {
        BUFF_ASSERT(max.x > min.x && max.y > min.y);
        // emscripten STL does not allow const here
        std::uniform_real_distribution distrX(min.x, max.x);
        std::uniform_real_distribution distrY(min.y, max.y);
        return {distrX(*this), distrY(*this)};
    }

    Pixel getRandomPixel(const Pixel max) {
        return getRandomPixel(Pixel::ZERO(), max);
    }
    Pixel getRandomPixel(const Pixel min, const Pixel max) {
        BUFF_ASSERT(max.x > min.x && max.y > min.y);
        // emscripten STL does not allow const here
        std::uniform_int_distribution distrX(min.x, max.x);
        std::uniform_int_distribution distrY(min.y, max.y);
        return {distrX(*this), distrY(*this)};
    }

    void enumerateStructMembers(auto&& functor) {
        functor(mImpl, "impl");
        BUFF_ASSERT_SIZEOF(sizeof(mImpl));
    }
};

template <Integer T>
T generateRandomNumber() {
    static std::random_device        sRandom;
    std::uniform_int_distribution<T> distribution(std::numeric_limits<T>::min(),
                                                  std::numeric_limits<T>::max());
    return distribution(sRandom);
}

BUFF_NAMESPACE_END
