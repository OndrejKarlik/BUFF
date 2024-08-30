#include "Lib/Platform.h"
#include "Lib/Math.h"
#include "Lib/Path.h"

BUFF_NAMESPACE_BEGIN

void* alignedMalloc(const int64 size, const int alignment) {
    BUFF_ASSERT(size > 0 && alignment > 0);
    BUFF_ASSERT(isPowerOf2(alignment));
    // void* result = ::operator new(uint64(size + alignment), std::align_val_t(alignment));
    void* result =
#ifdef _MSC_VER
        _aligned_malloc(size, alignment);
#else
        std::aligned_alloc(alignment, size);
#endif
    BUFF_ASSERT(result);
    BUFF_ASSERT(uint64(result) % alignment == 0);
    return result;
}

void alignedFree(const void* ptr) {
#ifdef _MSC_VER
    _aligned_free
#else
    free
#endif
        (const_cast<void*>(ptr));
}

BUFF_NAMESPACE_END
