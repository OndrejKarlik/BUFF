#pragma once
#include "LibWindows/Platform.h"
#include "Lib/Assert.h"
#include "Lib/Bootstrap.h"
#include <iostream>

BUFF_NAMESPACE_BEGIN

inline void debuggingAssertHandler(const AssertArguments& assert) {
    const String error = String("ASSERT FAILED: ") << assert.getMessage();
    std::cerr << error;
    printInVisualStudioDebugWindow(error);
    if (isDebuggerPresent()) {
        breakInDebugger();
    } else {
        std::cerr << error << std::endl;
        displayMessageWindow("ASSERT FAILED!", error);
    }
}

BUFF_NAMESPACE_END
