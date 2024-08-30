#include "Lib/SharedPtr.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class SharedPtr<int>;
template class SharedPtr<float>;
template class SharedPtr<String>;
template class SharedPtr<String*>;
template class SharedPtr<NoncopyableMovable>;
template class SharedPtr<Noncopyable>;

BUFF_NAMESPACE_END
