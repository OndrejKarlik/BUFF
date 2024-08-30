#include "Lib/AutoPtr.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class AutoPtr<int>;
template class AutoPtr<float>;
template class AutoPtr<String>;
template class AutoPtr<String*>;
template class AutoPtr<NoncopyableMovable>;
template class AutoPtr<Noncopyable>;

BUFF_NAMESPACE_END
