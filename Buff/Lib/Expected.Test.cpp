#include "Lib/Expected.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class Expected<int>;
template class Expected<float>;
template class Expected<String>;
template class Expected<String*>;
template class Expected<NoncopyableMovable>;

BUFF_NAMESPACE_END
