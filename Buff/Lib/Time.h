#pragma once
#include <chrono>

BUFF_NAMESPACE_BEGIN

class Duration {
    friend class TimeStamp;

    std::chrono::nanoseconds mImpl;

public:
    Duration()                                 = default;
    Duration(const Duration& other)            = default;
    Duration(Duration&& other)                 = default;
    Duration& operator=(const Duration& other) = default;
    Duration& operator=(Duration&& other)      = default;

    static Duration nanoseconds(const int64 value) {
        return Duration(std::chrono::nanoseconds(value));
    }
    static Duration microseconds(const double value) {
        return Duration(std::chrono::nanoseconds(int64(value * 1'000)));
    }
    static Duration milliseconds(const double value) {
        return Duration(std::chrono::nanoseconds(int64(value * 1'000'000)));
    }
    static Duration seconds(const double value) {
        return Duration(std::chrono::nanoseconds(int64(value * 1'000'000'000)));
    }
    static Duration zero() {
        return Duration(std::chrono::nanoseconds(0));
    }

    int64 toNanoseconds() const {
        return mImpl.count();
    }
    float toMicroseconds() const {
        return float(double(toNanoseconds()) / 1'000.0);
    }
    float toMilliseconds() const {
        return float(double(toNanoseconds()) / 1'000'000.0);
    }
    float toSeconds() const {
        return float(double(toNanoseconds()) / 1'000'000'000.0);
    }

    auto operator<=>(const Duration& rhs) const = default;

    Duration operator*(const double rhs) const {
        return Duration::nanoseconds(int64(double(toNanoseconds()) * rhs));
    }
    Duration& operator-=(const Duration& rhs) {
        mImpl -= rhs.mImpl;
        return *this;
    }
    Duration& operator+=(const Duration& rhs) {
        mImpl -= rhs.mImpl;
        return *this;
    }

    String getUserReadable() const {
        const int64                 nanoseconds = toNanoseconds();
        constexpr FormatFloatParams PARAMS      = {.maxDecimals = 3, .forceDecimals = false};
        if (nanoseconds < 500) {
            return formatFloat(double(nanoseconds), PARAMS) + " ns";
        } else if (nanoseconds < 500'000) { // microseconds
            return formatFloat(double(nanoseconds) / 1'000.0, PARAMS) + " ns";
        } else if (nanoseconds < 500'000'000) { // milliseconds
            return formatFloat(double(nanoseconds) / 1'000'000.0, PARAMS) + " ms";
        } else if (nanoseconds < 1'800'000'000'000) { // seconds
            return formatFloat(double(nanoseconds) / 1'000'000'000.0, PARAMS) + " s";
        } else { // hours
            return formatFloat(double(nanoseconds) / 3'600'000'000'000.0, PARAMS) + " h";
        }
    }

    void serializeCustom(ISerializer& serializer) const {
        serializer.serialize(toNanoseconds(), "nanoseconds");
    }
    void deserializeCustom(IDeserializer& deserializer) {
        int64 nanoseconds;
        deserializer.deserialize(nanoseconds, "nanoseconds");
        mImpl = std::chrono::nanoseconds(nanoseconds);
    }

private:
    explicit Duration(const std::chrono::nanoseconds& impl)
        : mImpl(impl) {}
};

class TimeStamp {
    std::chrono::time_point<std::chrono::steady_clock> mImpl;

public:
    TimeStamp() = default;

    static TimeStamp now() {
        return TimeStamp(std::chrono::steady_clock::now());
    }

    Duration operator-(const TimeStamp& rhs) const {
        return Duration(mImpl - rhs.mImpl);
    }
    TimeStamp operator-(const Duration& rhs) const {
        return TimeStamp(mImpl - std::chrono::nanoseconds(rhs.toNanoseconds()));
    }
    TimeStamp operator+(const Duration& rhs) const {
        return TimeStamp(mImpl + std::chrono::nanoseconds(rhs.toNanoseconds()));
    }
    auto operator<=>(const TimeStamp& rhs) const = default;

private:
    explicit TimeStamp(const std::chrono::time_point<std::chrono::steady_clock>& impl)
        : mImpl(impl) {}
};

class Timer {
    TimeStamp mBegin;

public:
    Timer() {
        reset();
    }
    void reset() {
        mBegin = TimeStamp::now();
    }

    Duration getElapsed() const {
        return TimeStamp::now() - mBegin;
    }
};

String getFormattedTimeStamp();

BUFF_NAMESPACE_END
