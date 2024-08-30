#include "Lib/Function.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

template class Function<void()>;
template class Function<void(int)>;
template class Function<void(int*, String*)>;
template class Function<int()>;
template class Function<int(int)>;
template class Function<int(int*, String*)>;

static int gGet4FnCalled = 0;
static int get4Fn() {
    ++gGet4FnCalled;
    return 4;
}
static int plus1Fn(int x) {
    return x + 1;
}

static void increment(int& x) {
    ++x;
}

TEST_CASE("Function simple function") {
    int      originalCalled = gGet4FnCalled;
    Function f              = get4Fn;
    CHECK(f() == 4);
    CHECK(gGet4FnCalled == originalCalled + 1);
    CHECK(f() == 4);
    CHECK(gGet4FnCalled == originalCalled + 2);

    Function g = plus1Fn;
    CHECK(g(42) == 43);
    CHECK(g(0) == 1);
}

TEST_CASE("Function simple lambda") {
    static int sNumCalled = 0;
    auto       get4       = []() {
        ++sNumCalled;
        return 4;
    };
    int      originalCalled = 0;
    Function f              = get4;
    CHECK(f() == 4);
    CHECK(sNumCalled == originalCalled + 1);
    CHECK(f() == 4);
    CHECK(sNumCalled == originalCalled + 2);

    auto     plus1   = [](int x) { return x + 1; };
    Function plus1Fn = plus1;
    CHECK(plus1Fn(42) == 43);
    CHECK(plus1Fn(0) == 1);
}

TEST_CASE("Function stateful") {
    int  numCalled = 0;
    auto get4      = [&numCalled]() {
        ++numCalled;
        return 4;
    };
    Function<int()> f = get4;
    CHECK(f() == 4);
    CHECK(numCalled == 1);
    CHECK(f() == 4);
    CHECK(numCalled == 2);
}

TEST_CASE("Function stateful mutable") {
    Function f = [counter = 1]() mutable { return ++counter; };
    CHECK(f() == 2);
    CHECK(f() == 3);
    CHECK(f() == 4);
    CHECK(f() == 5);
}

TEST_CASE("Function pass by reference") {
    int      x = 100;
    Function f = increment;
    f(x);
    CHECK(x == 101);
    f(x);
    CHECK(x == 102);
}

static void foo(const Function<bool(const int&)>& functor) {
    int        x   = 42;
    const bool res = functor(x);
    CHECK(res);
}
static void foo(const Function<void(const int&)>& functor) {
    int x = 142;
    functor(x);
}
TEST_CASE("Function overloads") {
    Array<int> returned;
    foo([&](const int& x) { returned.pushBack(x); });
    CHECK(returned == Array {142});
    foo([&](const int& x) {
        returned.pushBack(x);
        return true;
    });
    CHECK(returned == Array {
                          {142, 42}
    });
}

TEST_CASE("Function Noncopyable") {
    struct Foo : Noncopyable {
        int numCalled = 0;
    };
    Foo      foo;
    Function f = [](Foo& foo) { ++foo.numCalled; };
    f(foo);
    CHECK(foo.numCalled == 1);
}

BUFF_NAMESPACE_END
