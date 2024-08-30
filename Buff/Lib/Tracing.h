#pragma once
#include "Lib/containers/Map.h"
#include "Lib/String.h"
#include "Lib/StringView.h"
#include "Lib/Time.h"
#include <iostream>

BUFF_NAMESPACE_BEGIN

namespace Detail {
class TraceWithTimer {
    Timer mTimer;

public:
    explicit TraceWithTimer(const StringView message) {
        std::cout << message << "... ";
    }
    ~TraceWithTimer() {
        std::cout << "done in " << mTimer.getElapsed().getUserReadable() << "." << std::endl;
    }
};

class HitCountTracer {
    struct Record {
        const char*          function;
        const char*          file;
        int                  line;
        std::strong_ordering operator<=>(const Record& other) const {
            const std::strong_ordering lineCmp = line <=> other.line;
            // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr - false positive
            if (lineCmp != 0) {
                return lineCmp;
            } else if (const int functionCmp = strcmp(function, other.function)) {
                return functionCmp <=> 0;
            } else {
                return strcmp(file, other.file) <=> 0;
            }
        }
    };

    inline static Map<Record, int64> sHits;

public:
    static void record(const char* function, const char* filename, const int line) {
        const Record record {function, filename, line};
        if (int64* found = sHits.find(record)) {
            ++*found;
        } else {
            sHits[record] = 1;
        }
    }
    static void dumpTo(std::ostream& stream);
};

} // namespace Detail

#define BUFF_TRACE_DURATION(msg) Detail::TraceWithTimer traceDurationTimer_(msg)
#define BUFF_TRACE_NUM_HITS()    Detail::HitCountTracer::record(__FUNCTION__, __FILE__, __LINE__)

BUFF_NAMESPACE_END
