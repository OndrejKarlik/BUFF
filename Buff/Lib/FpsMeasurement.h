#pragma once
#include "Lib/containers/Array.h"
#include "Lib/Time.h"
#include <deque>

BUFF_NAMESPACE_BEGIN

class FpsMeasurement {
private:
    struct Sample {};
    std::deque<TimeStamp> mLastSamples;
    mutable int           mMaxSamples = 1024;

public:
    /// Call once per tick.
    void registerTick() {
        const TimeStamp now = TimeStamp::now();
        if (int(mLastSamples.size()) == mMaxSamples) {
            mLastSamples.pop_front();
        }
        mLastSamples.push_back(now);
    }

    /// Return FPS over given time period. Returns null if not enough data is stored
    Optional<float> getFps(const int timePeriodMs) const {
        const TimeStamp now    = TimeStamp::now();
        const TimeStamp cutoff = now - Duration::milliseconds(timePeriodMs);
        const auto      it     = std::ranges::lower_bound(mLastSamples, cutoff);
        if (it == mLastSamples.begin()) {
            if (int(mLastSamples.size()) == mMaxSamples) {
                mMaxSamples += mMaxSamples / 2;
            }
            return NULL_OPTIONAL;
        }
        const int frames = int(mLastSamples.end() - it);
        return float(frames) * 1000.f / float(timePeriodMs);
    }
    Optional<float> getLastFrameJitterPercent(const int timePeriodMs) const {
        if (Optional<float> fps = getFps(timePeriodMs)) {
            const Duration duration =
                mLastSamples[mLastSamples.size() - 1] - mLastSamples[mLastSamples.size() - 2];
            const float jitterMs      = duration.toMilliseconds() - 1'000 / *fps;
            const float jitterPercent = 100 * jitterMs / (1'000 / *fps);
            return jitterPercent;
        } else {
            return NULL_OPTIONAL;
        }
    }
};

BUFF_NAMESPACE_END
