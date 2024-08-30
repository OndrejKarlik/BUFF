#pragma once
#include "Lib/AutoPtr.h"
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include "Lib/containers/Map.h"
#include <typeindex>
#include <vector>

BUFF_NAMESPACE_BEGIN

class IGameItem
    : public Polymorphic
    , public Noncopyable {
public:
    virtual bool isActive() const {
        return true;
    }
};

class Game : public Noncopyable {
    Array<AutoPtr<IGameItem>> mItems;

    // Only for end-level items
    mutable Map<std::type_index, Array<IGameItem*>> mSorted;

public:
    Game() = default;

    void addItem(AutoPtr<IGameItem> item) {
        BUFF_ASSERT(item);
        mItems.pushBack(std::move(item));
        mSorted.clear();
    }

    void removeItem(const IGameItem& item) {
        if (mItems.size() > 1) {
            const Optional<int64> index = mItems.find(&item);
            std::swap(mItems[*index], mItems.back());
            mItems.popBack();
        } else {
            mItems.eraseByValue(&item);
        }
        mSorted.clear();
    }

    template <typename T>
    Array<const T*> iterateItemsFiltered() const {
        return iterateItemsFilteredImpl<const T>();
    }

    template <typename T>
    Array<T*> iterateItemsFiltered() {
        return iterateItemsFilteredImpl<T>();
    }

    void serializeCustom(ISerializer& serializer) const {
        serializer.serialize(mItems.size(), "size");
        serializer.serializeList(mItems, "items");
    }
    void deserializeCustom(IDeserializer& deserializer) {
        mItems.clear();
        mSorted.clear();
        int64 count = {};
        deserializer.deserialize(count, "size");
        mItems.resize(count);
        deserializer.deserializeList(mItems, "items");
    }

private:
    template <typename T>
    Array<T*> iterateItemsFilteredImpl() const {
        const std::type_index key(typeid(std::decay_t<T>));
        Array<T*>             result;

        if (const Array<IGameItem*>* cached = mSorted.find(key)) {
            for (auto& item : *cached) {
                result.pushBack(static_cast<T*>(item));
            }
        } else {
            Array<IGameItem*>& storeToCache = mSorted[key];
            for (auto& item : mItems) {
                if (T* castItem = dynamic_cast<T*>(item.get())) {
                    result.pushBack(castItem);
                    storeToCache.pushBack(item.get());
                }
            }
        }
        return result;
    }
};

BUFF_NAMESPACE_END
