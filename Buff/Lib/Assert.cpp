#include "Lib/Assert.h"
#include "Lib/Platform.h"
#include "Lib/String.h"
#include <iostream>

BUFF_NAMESPACE_BEGIN

[[noreturn]] static void defaultAssertHandler(const AssertArguments& assert) {
    const String error = String("ASSERT FAILED: ") + assert.getMessage();
    std::cerr << error << std::endl;
    throw AssertException(error.asCString());
}

AssertHandler getDefaultAssertHandler() {
    return defaultAssertHandler;
}

static AssertHandler gAssertHandler = defaultAssertHandler;

void setAssertHandler(const AssertHandler& handler) {
    gAssertHandler = handler;
}
AssertHandler getCurrentAssertHandler() {
    return gAssertHandler;
}

#if BUFF_DEBUG
void Detail::handleAssert(const char*              condition,
                          const char*              filename,
                          const int                lineNumber,
                          const std::stringstream& args) {
    gAssertHandler({condition, filename, lineNumber, args.str().c_str()});
}
#endif

BUFF_NAMESPACE_END
