#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/SharedPtr.h"

BUFF_NAMESPACE_BEGIN

namespace Detail {
// Taken from llvm-project/libcxx/include/__type_traits/strip_signature.h
template <typename T>
struct CreateTemplateSignature;
template <typename TReturn, typename TClass, typename... TArgs>
struct CreateTemplateSignature<TReturn (TClass::*)(TArgs...)> {
    using Type = TReturn(TArgs...);
};
template <typename TReturn, typename TClass, typename... TArgs>
struct CreateTemplateSignature<TReturn (TClass::*)(TArgs...) const> {
    using Type = TReturn(TArgs...);
};
template <typename TReturn, typename TClass, typename... TArgs>
struct CreateTemplateSignature<TReturn (TClass::*)(TArgs...) noexcept> {
    using Type = TReturn(TArgs...);
};
template <typename TReturn, typename TClass, typename... TArgs>
struct CreateTemplateSignature<TReturn (TClass::*)(TArgs...) const noexcept> {
    using Type = TReturn(TArgs...);
};
}

template <typename TSignature>
class Function;

/// Note: our Function has SharedPtr semantics when wrapping a stateful lambda, not "value" (deep copy)
/// semantics!
///
/// \internal
/// The second template here after templated fwdecl is pretty WTF, but it is by far the best (maybe only) way
/// to get separate return type and args type from a single function signature template parameter.
template <typename TReturn, typename... TArgs>
class Function<TReturn(TArgs...)> {
    struct StatefulFunctor : Polymorphic {
        virtual TReturn call(TArgs... args) const = 0;
    };
    using Signature = TReturn(TArgs...);

    /// Holds functions that do not have any state and can be decayed to a function pointer. This is faster
    /// both to allocate and to call, and causes less overhead stack frames
    Signature* mStatelessFunction = nullptr;

    /// Functions that hold some state (e.g. captured variables) need to be stored in an allocated state that
    /// resembles std::any.
    SharedPtr<StatefulFunctor> mStatefulFunction;

public:
    Function() = default;

    Function(std::nullptr_t) {}

    template <typename T>
    Function(T&& functor) requires(!std::is_same_v<std::decay_t<T>, Function> &&
                                   std::is_same_v<TReturn, decltype(functor(std::declval<TArgs>()...))>) {
        if constexpr (std::is_convertible_v<T, Signature*>) {
            mStatelessFunction = functor;
        } else {
            struct SpecificFunctor final : StatefulFunctor {
                /// Mutable necessary for mutable lambdas - they have non-const operator()
                mutable std::decay_t<T> fn;
                explicit SpecificFunctor(T&& fn)
                    : fn(std::forward<T>(fn)) {}
                virtual TReturn call(TArgs... args) const override {
                    return fn(std::forward<TArgs>(args)...);
                }
            };
            mStatefulFunction = makeSharedPtr<SpecificFunctor>(std::forward<T>(functor));
        }
    }

    Function(const Function& other)            = default;
    Function(Function&& other)                 = default;
    Function& operator=(const Function& other) = default;
    Function& operator=(Function&& other)      = default;

    constexpr TReturn operator()(TArgs... args) const {
        BUFF_ASSERT(bool(mStatelessFunction) != bool(mStatefulFunction)); // Exactly one active
        if (mStatelessFunction) {
            return (*mStatelessFunction)(std::forward<TArgs>(args)...);
        } else {
            BUFF_ASSERT(mStatefulFunction);
            return mStatefulFunction->call(std::forward<TArgs>(args)...);
        }
    }

    operator bool() const {
        BUFF_ASSERT(!mStatelessFunction || !mStatefulFunction); // At most one active
        return mStatelessFunction != nullptr || mStatefulFunction != nullptr;
    }
};

// Resharper naming lint still does nto understand template deduction guides
// ReSharper disable CppInconsistentNaming
template <typename TReturn, typename... TArgs>
Function(TReturn (*)(TArgs...)) -> Function<TReturn(TArgs...)>;
template <class TLambda>
Function(TLambda) -> Function<typename Detail::CreateTemplateSignature<decltype(&TLambda::operator())>::Type>;
// ReSharper restore CppInconsistentNaming

BUFF_NAMESPACE_END
