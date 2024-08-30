#include "Lib/meta/MetaStruct.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
// template class GenericPropertyValue<true>; // These are explicitly instantiated in the MetaStruct.cpp
// template class GenericPropertyValue<false>; // Doing it here causes a warning
template class PropertyValueWrapper<const int>;
template class PropertyValueWrapper<int>;
template class PropertyValueWrapper<const String>;
template class PropertyValueWrapper<String>;
template class ListValueWrapper<const int>;
template class ListValueWrapper<int>;
template class ListValueWrapper<const String>;
template class ListValueWrapper<String>;

class Simple : public MetaStruct<Simple> {
public:
    static constexpr PropertyDefinition PROPERTIES[] = {
        {"string", PropertyType::STRING},
        {"number", PropertyType::INT   },
        {"float",  PropertyType::FLOAT },
    };
};

TEST_CASE("MetaStruct.primitive") {
    Simple metaStruct;
    static_assert(std::is_same_v<decltype(metaStruct.get<"number">()), PropertyValueWrapper<int>>);
    metaStruct.get<"number">() = 42;
    CHECK((metaStruct.get<"number">() == 42));
    metaStruct.get<"number">() = 1337;
    CHECK((metaStruct.get<"number">() == 1337));

    metaStruct.get<"float">() = 42.5f;
    CHECK((metaStruct.get<"float">() == 42.5f));
    metaStruct.get<"float">() = 1337.5f;
    CHECK((metaStruct.get<"float">() == 1337.5f));
}

TEST_CASE("MetaStruct.string") {
    Simple metaStruct;
    metaStruct.get<"string">() = String("mystr");
    CHECK((metaStruct.get<"string">() == String("mystr")));
}

// ===========================================================================================================
// Generic access
// ===========================================================================================================

TEST_CASE("MetaStruct.generic") {
    Simple metaStruct;
    metaStruct.get<"number">() = 42;
    MetaStructBase& base       = metaStruct;
    CHECK(base.getUnchecked("number").as<int>() == 42);
    base.getUnchecked("number").assign(154);
    CHECK(metaStruct.get<"number">() == 154);
}

// ===========================================================================================================
// Observers
// ===========================================================================================================

TEST_CASE("MetaStruct.universalObservers") {
    Array<String> called;
    bool          observerRetVal = true;
    auto          observer       = [&](const String& key) {
        called.pushBack(key);
        return observerRetVal;
    };
    Simple metaStruct;
    metaStruct.addObserver(observer);

    metaStruct.get<"number">() = 99;
    CHECK(called.size() == 1);
    CHECK(called[0] == "number");

    metaStruct.get<"float">()  = 1.f;
    metaStruct.get<"float">()  = 2.f;
    metaStruct.get<"string">() = "aaa"_S;
    CHECK(called.size() == 4);
    CHECK(called[1] == "float");
    CHECK(called[2] == "float");
    CHECK(called[3] == "string");

    called                     = {};
    observerRetVal             = false;
    metaStruct.get<"float">()  = 1.f;
    metaStruct.get<"float">()  = 2.f;
    metaStruct.get<"string">() = " aaa "_S;
    CHECK(called.size() == 1);
    CHECK(called[0] == "float");
}

TEST_CASE("MetaStruct.propertyObservers") {
    int    calledNumber      = 0;
    int    calledString      = 0;
    bool   continueObserving = true;
    Simple metaStruct;
    metaStruct.get<"number">().addObserver([&]() {
        ++calledNumber;
        return continueObserving;
    });
    metaStruct.get<"string">().addObserver([&]() {
        ++calledString;
        return continueObserving;
    });
    CHECK(calledNumber == 0);
    CHECK(calledString == 0);
    metaStruct.get<"number">() = 99;
    CHECK(calledNumber == 1);
    CHECK(calledString == 0);
    metaStruct.get<"number">() = 10;
    CHECK(calledNumber == 2);
    CHECK(calledString == 0);

    metaStruct.get<"string">() = "aaa"_S;
    CHECK(calledNumber == 2);
    CHECK(calledString == 1);

    continueObserving          = false;
    metaStruct.get<"number">() = 1;
    CHECK(calledNumber == 3);
    metaStruct.get<"number">() = 10;
    CHECK(calledNumber == 3);
}

// ===========================================================================================================
// Lists
// ===========================================================================================================

class WithLists : public MetaStruct<WithLists> {
public:
    static constexpr PropertyDefinition PROPERTIES[] = {
        {"int",    PropertyType::INT,    {.isList = true}},
        {"string", PropertyType::STRING, {.isList = true}},
    };
};

TEST_CASE("MetaStruct.ListsAccessing") {
    WithLists metaStruct;
    CHECK(metaStruct.get<"int">().size() == 0);
    CHECK(metaStruct.get<"string">().size() == 0);

    metaStruct.get<"int">().pushBack(42);
    CHECK(metaStruct.get<"int">().size() == 1);
    CHECK(metaStruct.get<"int">().getArrayView() == Array<int> {std::initializer_list<int> {42}});

    metaStruct.get<"int">().pushBack(1337);
    CHECK(metaStruct.get<"int">().size() == 2);
    CHECK(metaStruct.get<"int">().getArrayView() == Array {
                                                        {42, 1337}
    });

    metaStruct.get<"string">().pushBack("abc"_S);
    CHECK(metaStruct.get<"string">().size() == 1);
    CHECK(metaStruct.get<"string">().getArrayView() == Array {{"abc"_S}});

    metaStruct.get<"string">().pushBack("xyz"_S);
    CHECK(metaStruct.get<"string">().size() == 2);
    CHECK(metaStruct.get<"string">().getArrayView() == Array {
                                                           {"abc"_S, "xyz"_S}
    });
}

TEST_CASE("MetaStruct.ListsObservers") {
    WithLists         metaStruct;
    Array<ListChange> changesInt, changesString;
    metaStruct.get<"int">().addListObserver([&](const ListChange& change) {
        changesInt.pushBack(change);
        return true;
    });
    metaStruct.get<"int">().pushBack(4);
    CHECK(changesInt.size() == 1);
    CHECK(changesInt[0] == ListChange {.type = ListChange::ADD, .index = 0});
    metaStruct.get<"int">().pushBack(99);
    CHECK(changesInt.size() == 2);
    CHECK(changesInt[1] == ListChange {.type = ListChange::ADD, .index = 1});

    metaStruct.get<"string">().addListObserver([&](const ListChange& change) {
        changesString.pushBack(change);
        return true;
    });
    CHECK(changesString.size() == 0);
    metaStruct.get<"string">().pushBack("abc"_S);
    CHECK(changesString.size() == 1);
    CHECK(changesString[0] == ListChange {.type = ListChange::ADD, .index = 0});
    metaStruct.get<"string">().pushBack("xyz"_S);
    CHECK(changesString.size() == 2);
    CHECK(changesString[1] == ListChange {.type = ListChange::ADD, .index = 1});

    metaStruct.get<"string">().erase(0);
    CHECK(changesString.size() == 3);
    CHECK(changesString[2] == ListChange {.type = ListChange::ERASE, .index = 0});
}

TEST_CASE("MetaStruct.IterateLists") {
    WithLists metaStruct;
    metaStruct.get<"int">().pushBack(42);
    metaStruct.get<"int">().pushBack(0);
    metaStruct.get<"int">().pushBack(666);
    metaStruct.get<"int">().pushBack(1023);
    Array<int> res;
    for (const int& item : metaStruct.get<"int">()) {
        res.pushBack(item);
    }
    CHECK(res == Array({42, 0, 666, 1023}));
}

// TEST_CASE("MetaStruct.ListsAccessing") {
//    WithLists metaStruct;
//    metaStruct["int"].pushBack(42);
//    CHECK(metaStruct["int"].size() == 1);
//    CHECK(metaStruct["int"][0] == 42);
//
//    metaStruct["int"].pushBack(1337);
//    CHECK(metaStruct["int"].size() == 2);
//    CHECK(metaStruct["int"][0] == 42);
//    CHECK(metaStruct["int"][1] == 1337);
//
//    metaStruct["string"].pushBack("abc"_S);
//    CHECK(metaStruct["string"].size() == 1);
//    CHECK(metaStruct["string"][0] == "abc"_S);
//    metaStruct["string"].pushBack("xyz"_S);
//    CHECK(metaStruct["string"].size() == 2);
//    CHECK(metaStruct["string"][0] == "abc"_S);
//    CHECK(metaStruct["string"][2] == "xyz"_S);
//}
// ===========================================================================================================
// Serialization
// ===========================================================================================================

TEST_CASE("MetaStruct.serialization") {
    Simple metaStruct;
    metaStruct.get<"number">() = 42;
    metaStruct.get<"float">()  = 12.5f;
    metaStruct.get<"string">() = "abc"_S;
    JsonSerializer serializer;
    serializer.serialize(metaStruct, "data");
    String res = serializer.getJson(true);
    CHECK_STREQ(res,
                R"({
    "data": {
        "float": {
            "float": 12.5
        },
        "number": {
            "number": 42
        },
        "string": {
            "string": "abc"
        }
    }
})");

    Simple           toLoad;
    JsonDeserializer deserializer(res);
    deserializer.deserialize(toLoad, "data");
    CHECK((toLoad.get<"number">() == 42));
    CHECK((toLoad.get<"float">() == 12.5f));
    CHECK((toLoad.get<"string">() == "abc"_S));
}

class SingleList : public MetaStruct<SingleList> {
public:
    static constexpr PropertyDefinition PROPERTIES[] = {
        {"int", PropertyType::INT, {.isList = true}},
    };
};

TEST_CASE("MetaStruct.EmptyListSerialization") {
    SingleList     metaStruct;
    JsonSerializer serializer;
    serializer.serialize(metaStruct, "data");
    String res = serializer.getJson(true);
    CHECK_STREQ(res,
                R"({
    "data": {
        "int": {
            "list": {
                "items": [],
                "size": 0
            }
        }
    }
})");

    SingleList       toLoad;
    JsonDeserializer deserializer(res);
    deserializer.deserialize(toLoad, "data");
    CHECK(toLoad.get<"int">().size() == 0);
}

TEST_CASE("MetaStruct.SingleListSerialization") {
    SingleList metaStruct;
    metaStruct.get<"int">().assignFromArray(Array {1, 2, 3, 4, 5, 6});

    JsonSerializer serializer;
    serializer.serialize(metaStruct, "data");
    String res = serializer.getJson(true);
    CHECK_STREQ(res,
                R"({
    "data": {
        "int": {
            "list": {
                "items": [1, 2, 3, 4, 5, 6],
                "size": 6
            }
        }
    }
})");

    SingleList       toLoad;
    JsonDeserializer deserializer(res);
    deserializer.deserialize(toLoad, "data");
    CHECK(toLoad.get<"int">().getArrayView() == Array({1, 2, 3, 4, 5, 6}));
}

TEST_CASE("MetaStruct.ListsSerialization") {
    WithLists metaStruct;
    metaStruct.get<"int">().assignFromArray(Array {1, 2, 3, 4, 5, 6});
    metaStruct.get<"string">().pushBack("abc"_S);

    JsonSerializer serializer;
    serializer.serialize(metaStruct, "data");
    String res = serializer.getJson(true);
    CHECK_STREQ(res,
                R"({
    "data": {
        "int": {
            "list": {
                "items": [1, 2, 3, 4, 5, 6],
                "size": 6
            }
        },
        "string": {
            "list": {
                "items": ["abc"],
                "size": 1
            }
        }
    }
})");

    WithLists        toLoad;
    JsonDeserializer deserializer(res);
    deserializer.deserialize(toLoad, "data");
    CHECK(toLoad.get<"int">().getArrayView() == Array({1, 2, 3, 4, 5, 6}));
    CHECK(toLoad.get<"string">().size() == 1);
    CHECK(toLoad.get<"string">()[0] == "abc");
}

// ===========================================================================================================
// Custom Types
// ===========================================================================================================

struct Custom1 {
    int    x;
    String y;
    bool   operator==(const Custom1& other) const = default;
    void   enumerateStructMembers(auto&& functor) {
        functor(x, "x");
        functor(y, "y");
    }
    // TODO: remove with deducing this
    constexpr void enumerateStructMembers(auto&& functor) const {
        functor(x, "x");
        functor(y, "y");
    }
};
static_assert(Serializable<Custom1>);
static_assert(Serializable<ArrayView<Custom1>>);
BUFF_META_REGISTER_CUSTOM_TYPE(Custom1, 1);

struct Custom2 {
    Pixel  a;
    String b;
    bool   operator==(const Custom2& other) const = default;
    void   enumerateStructMembers(auto&& functor) {
        functor(a, "a");
        functor(b, "b");
    }
    // TODO: remove with deducing this
    constexpr void enumerateStructMembers(auto&& functor) const {
        functor(a, "a");
        functor(b, "b");
    }
};
BUFF_META_REGISTER_CUSTOM_TYPE(Custom2, 2);

class CustomTypes : public MetaStruct<CustomTypes> {
public:
    static constexpr PropertyDefinition PROPERTIES[] = {
        {"custom1", getPropertyType<Custom1>()},
        {"custom2", getPropertyType<Custom2>()},
    };
};

TEST_CASE("MetaStruct.CustomTypes") {
    CustomTypes metaStruct;
    metaStruct.get<"custom1">() = Custom1 {42, "abc"_S};
    CHECK((metaStruct.get<"custom1">() == Custom1 {42, "abc"_S}));
    metaStruct.get<"custom2">() = Custom2 {
        Pixel {1, 2},
        "xyz"_S
    };
    CHECK((metaStruct.get<"custom2">() == Custom2 {
                                              Pixel {1, 2},
                                              "xyz"_S
    }));
}

BUFF_NAMESPACE_END
