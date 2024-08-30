#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

using AssertHandler = void (*)(const AssertArguments& arguments);

void setAssertHandler(const AssertHandler& handler);

AssertHandler getCurrentAssertHandler();

AssertHandler getDefaultAssertHandler();

class AssertException final : public std::exception {
    String mWhat;

public:
    AssertException() = default;

    explicit AssertException(String string)
        : mWhat(std::move(string)) {}

    virtual const char* what() const noexcept override {
        return mWhat.asCString();
    }
};

BUFF_NAMESPACE_END
