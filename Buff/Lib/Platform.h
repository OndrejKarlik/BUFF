#pragma once
#include "Lib/Bootstrap.h"

BUFF_NAMESPACE_BEGIN

/// Used to retrieve and pass window handles in a type safe and platform-independent way, even though
/// ultimately this is converted to the platform-specific type
struct NativeWindowHandle {
    void* handle = nullptr;
};

/// \param alignment Must be power of 2
void* alignedMalloc(int64 size, int alignment);

void alignedFree(const void* ptr);

BUFF_NAMESPACE_END
