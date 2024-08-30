#include "Lib/Tracing.h"
#include "Lib/AutoPtr.h"
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include "Lib/Math.h"
#ifdef __EMSCRIPTEN__
#    include <iomanip>
#endif

BUFF_NAMESPACE_BEGIN

void Detail::HitCountTracer::dumpTo(std::ostream& stream) {
    if (sHits.isEmpty()) {
        return;
    }
    auto result = Array<std::pair<Record, int64>>(sHits);
    std::ranges::sort(result, [](const auto& x, const auto& y) { return x.second > y.second; });
    const int64 maximum = result[0].second;
    // get number of digits in maximum:
    const int maxDigits = getNumDigits(uint64(maximum));
    stream << "Hit count tracer:" << std::endl;
    for (const auto& [record, count] : result) {
        stream << std::setw(maxDigits) << count << " hit: " << record.function << "    (" << record.file
               << "(" << record.line << "))\n";
    }
}

BUFF_NAMESPACE_END
