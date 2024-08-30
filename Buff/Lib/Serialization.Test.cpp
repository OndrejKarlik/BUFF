#include "Lib/Serialization.h"
#include "Lib/AutoPtr.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("BinarySerializer::roundtrip") {
    BinarySerializer serializer;
    serializer.serialize(25, "1");
    serializer.serialize(false, "2");
    serializer.serialize(true, "3");
    serializer.serialize(1.5f, "4");
    BinaryDeserializer deserializer(serializer.getState());
    int                i = 0;
    deserializer.deserialize(i, "1");
    CHECK(i == 25);
    bool b = true;
    deserializer.deserialize(b, "2");
    CHECK(!b);
    deserializer.deserialize(b, "3");
    CHECK(b);
    float f = 0;
    deserializer.deserialize(f, "4");
    CHECK(f == 1.5f);
}

struct SType {
    int         x                     = 0;
    mutable int numSerializationCalls = 0;
    void        serializeCustom(ISerializer& serializer) const {
        serializer.serialize(42, "x");
        ++numSerializationCalls;
    }
    void deserializeCustom(IDeserializer& deserializer) {
        int read = 0;
        deserializer.deserialize(read, "x");
        CHECK(read == 42);
        ++numSerializationCalls;
    }
};

TEST_CASE("BinarySerializer::custom type") {
    BinarySerializer serializer;
    SType            dummy;
    serializer.serialize(dummy, "dummy");
    CHECK(dummy.numSerializationCalls == 1);
    dummy.x = 333;
    BinaryDeserializer deserializer(serializer.getState());
    deserializer.deserialize(dummy, "dummy");
    CHECK(dummy.numSerializationCalls == 2);
    CHECK(dummy.x == 333);
}

TEST_CASE("BinarySerializer::String") {
    const String empty;
    const String a = "a";
    const String b = "alphabet123";

    BinarySerializer serializer;
    serializer.serialize(empty, "empty");
    serializer.serialize(a, "a");
    serializer.serialize(b, "b");
    BinaryDeserializer deserializer(serializer.getState());
    String             res;
    deserializer.deserialize(res, "empty");
    CHECK(res.isEmpty());
    deserializer.deserialize(res, "a");
    CHECK(res == "a");
    deserializer.deserialize(res, "b");
    CHECK(res == "alphabet123");
}

struct SerializablePolymorphicType : public Polymorphic {
    BUFF_REGISTER_FOR_POLYMORPHIC_SERIALIZATION(SerializablePolymorphicType);
    int  x = 0;
    void serializeCustom(ISerializer& serializer) const {
        serializer.serialize(x, "x");
    }
    void deserializeCustom(IDeserializer& deserializer) {
        deserializer.deserialize(x, "x");
    }
    SerializablePolymorphicType(const int x = 0)
        : x(x) {}
};
struct SerializablePolymorphicType2 : public Polymorphic {
    BUFF_REGISTER_FOR_POLYMORPHIC_SERIALIZATION(SerializablePolymorphicType2);
    double x = 0;
    void   serializeCustom(ISerializer& serializer) const {
        serializer.serialize(x, "x");
    }
    void deserializeCustom(IDeserializer& deserializer) {
        deserializer.deserialize(x, "x");
    }
    SerializablePolymorphicType2(const double x = 0)
        : x(x) {}
};

TEST_CASE("BinarySerializer::polymorphic") {
    BinarySerializer serializer;

    {
        const AutoPtr<Polymorphic> empty;
        const AutoPtr<Polymorphic> ptr1 = makeAutoPtr<SerializablePolymorphicType>(32);
        const AutoPtr<Polymorphic> ptr2 = makeAutoPtr<SerializablePolymorphicType2>(-1.5);
        serializer.serialize(empty, "empty");
        serializer.serialize(ptr1, "ptr1");
        serializer.serialize(ptr2, "ptr2");
    }

    BinaryDeserializer   deserializer(serializer.getState());
    AutoPtr<Polymorphic> res;
    deserializer.deserialize(res, "empty");
    CHECK(!res);
    deserializer.deserialize(res, "ptr1");
    Polymorphic* resPtr = res.get();
    CHECK(typeid(*resPtr) == typeid(SerializablePolymorphicType&));
    CHECK(dynamic_cast<SerializablePolymorphicType&>(*resPtr).x == 32);
    deserializer.deserialize(res, "ptr2");
    resPtr = res.get();
    CHECK(typeid(*resPtr) == typeid(SerializablePolymorphicType2&));
    CHECK(dynamic_cast<SerializablePolymorphicType2&>(*resPtr).x == -1.5);
}

BUFF_NAMESPACE_END
