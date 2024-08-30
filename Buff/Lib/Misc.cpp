#include "Lib/Pixel.h"
#include "Lib/String.h"
#include "Lib/Time.h"
#include "Lib/Vector.h"
#ifdef __EMSCRIPTEN__
#    include <iomanip>
#endif

BUFF_NAMESPACE_BEGIN

String AssertArguments::getMessage() const {
    return String(condition) + "\nFile: " + filename + "\nLine: " + lineNumber + "\nArguments:\n" + arguments
           << "\n";
}

Pixel toPixel(const Vector2 input) {
    return {int(input.x), int(input.y)};
}

Vector2 toVector2(const Pixel input) {
    return {float(input.x), float(input.y)};
}

String getFormattedTimeStamp() {
    time_t  t = time(nullptr);
    std::tm tm;
#ifdef __EMSCRIPTEN__
    localtime_r(&t, &tm);
#else
    localtime_s(&tm, &t);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm, "%F %T");
    return ss.str();
}

BUFF_NAMESPACE_END
