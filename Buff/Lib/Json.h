#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/ArrayView.h"
#include "Lib/containers/HashMap.h"
#include "Lib/containers/Map.h"
#include "Lib/Exception.h"
#include "Lib/String.h"
#include "Lib/Variant.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
class Function;
class JsObject;

/// Does not add enclosing " "!
String escapeJson(StringView str);
String unEscapeJson(StringView str);

Optional<JsObject> parseJson(StringView json);

class JsValue;

class JsObject {
    Map<String, JsValue> mValues;

public:
    JsObject() = default;
    JsObject(const JsObject& other);
    JsObject(JsObject&& other);
    JsObject& operator=(const JsObject& other);
    JsObject& operator=(JsObject&& other);

    JsObject(ArrayView<const std::pair<String, JsValue>> values);

    JsValue&       operator[](StringView name);
    const JsValue* find(StringView name) const;

    bool contains(const StringView name) const {
        return mValues.contains(name);
    }

    int64 numItems() const {
        return mValues.size();
    }

    String toJson(Optional<int> indentation = NULL_OPTIONAL) const;

    void visitAllValues(const Function<void(const String&, const JsValue&)>& functor) const;
};

class JsArray {
    Map<int64, JsValue> mValues;
    int64               mHighestIndex = -1;

public:
    JsArray() = default;
    JsArray(const JsArray& other);
    JsArray(JsArray&& other);
    JsArray& operator=(const JsArray& other);
    JsArray& operator=(JsArray&& other);

    template <IterableContainer TArray, typename TConverter>
    JsArray(TArray&& container, TConverter&& converter)
        requires requires(TArray& b, TConverter& c) { JsValue(c(*b.begin())); } {
        for (auto& it : container) {
            push(converter(it));
        }
    }
    JsValue&       operator[](int64 index);
    const JsValue& operator[](int64 index) const;

    void push(JsValue value);

    const JsValue& peek() const;
    JsValue&       peek();

    String toJson(Optional<int> indentation = NULL_OPTIONAL) const;

    int64 numItems() const {
        return mValues.size();
    }

    void visitAllValues(const Function<void(int64, const JsValue&)>& functor) const;
};

class JsValue {
public:
    struct Undefined {};
    struct Null {};

private:
    Variant<Undefined, Null, bool, double, String, JsObject, JsArray> mImpl;

public:
    enum class Type {
        UNDEFINED,
        NULLPTR, // Cannot use NULL as it is a macro...
        BOOLEAN,
        NUMBER,
        STRING,
        OBJECT,
        ARRAY,
    };
    using enum Type;

    JsValue() {
        mImpl.emplace<Undefined>();
    }

    // =======================================================================================================
    // Constructors
    // =======================================================================================================

    JsValue(const Undefined& BUFF_UNUSED(undefined)) {
        mImpl.emplace<Undefined>();
    }
    JsValue(const Null& BUFF_UNUSED(null)) {
        mImpl.emplace<Null>();
    }

    JsValue(const double val) {
        mImpl.emplace<double>(val);
    }
    JsValue(const int val) {
        mImpl.emplace<double>(double(val));
    }
    JsValue(const int64 val) {
        mImpl.emplace<double>(double(val));
    }
    JsValue(const uint64 val) {
        mImpl.emplace<double>(double(val));
    }

    // trick needed to prevent "this" binding to char*
    template <typename T>
    JsValue(const T val) requires std::is_same_v<std::decay_t<T>, bool> {
        mImpl.emplace<bool>(val);
    }
    JsValue(String val) {
        mImpl.emplace<String>(std::move(val));
    }
    JsValue(const StringView val) {
        mImpl.emplace<String>(val);
    }
    JsValue(JsObject val) {
        mImpl.emplace<JsObject>(std::move(val));
    }
    JsValue(JsArray val) {
        mImpl.emplace<JsArray>(std::move(val));
    }
    // =======================================================================================================
    // Accessing types
    // =======================================================================================================

    // We cannot have operator bool because it leads to wrong type deductions
    operator bool() const = delete;

    operator double() const {
        return mImpl.get<double>();
    }
    operator int() const {
        return safeIntegerCast<int>(int64(mImpl.get<double>()));
    }
    operator int64() const {
        return int64(mImpl.get<double>());
    }
    operator StringView() const {
        return mImpl.get<String>();
    }
    bool isArray() const {
        return mImpl.holdsType<JsArray>();
    }

    Type getType() const {
        return mImpl.visit([](const Undefined&) { return UNDEFINED; },
                           [](const Null&) { return NULLPTR; },
                           [](const bool&) { return BOOLEAN; },
                           [](const double&) { return NUMBER; },
                           [](const String&) { return STRING; },
                           [](const JsObject&) { return OBJECT; },
                           [](const JsArray&) { return ARRAY; });
    }

    // =======================================================================================================
    // Rest
    // =======================================================================================================

    // JsObject, JsArray only
    int64 size() const {
        if (const JsObject* obj = mImpl.tryGet<JsObject>()) {
            return obj->numItems();
        } else if (const JsArray* arr = mImpl.tryGet<JsArray>()) {
            return arr->numItems();
        } else {
            BUFF_STOP;
        }
    }

    /// Only for JsObject type
    JsValue& operator[](StringView name);
    JsValue& operator[](const char* name) {
        return (*this)[StringView(name)];
    }
    const JsValue* find(StringView name) const;
    bool           contains(const StringView name) const {
        return mImpl.get<JsObject>().contains(name);
    }

    /// Only for JsArray type
    JsValue&       operator[](int index);
    const JsValue& operator[](int index) const;
    void           push(JsValue value);
    const JsValue& peek() const;
    JsValue&       peek();

    template <typename... T>
    auto visitValue(T&&... functor) const {
        return mImpl.visit(std::forward<T>(functor)...);
    }

    template <typename T>
    T& get() {
        return mImpl.get<T>();
    }
    template <typename T>
    const T& get() const {
        return mImpl.get<T>();
    }

    String toJson(Optional<int> indentation = NULL_OPTIONAL) const;
};

BUFF_NAMESPACE_END
