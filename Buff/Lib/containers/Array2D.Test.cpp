#include "Lib/containers/Array2D.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class Array2D<int>;
template class Array2D<float>;
template class Array2D<String>;
template class Array2D<String*>;
template class Array2D<NoncopyableMovable>;

BUFF_NAMESPACE_END
