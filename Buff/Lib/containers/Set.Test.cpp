#include "Lib/containers/Set.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

struct NoncopyableMovableComparable : NoncopyableMovable {
    int  value;
    bool operator<(const NoncopyableMovableComparable& other) const {
        return value < other.value;
    }
};

// Test compilation:
template class Set<int>;
template class Set<float>;
template class Set<String>;
template class Set<String*>;
template class Set<NoncopyableMovableComparable>;

BUFF_NAMESPACE_END
