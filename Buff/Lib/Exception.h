#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

/// std::exception cannot be constructed from const char* when using pure clang, so we need this
class Exception : public std::exception {
    String mWhat;

public:
    Exception() = default;

    explicit Exception(String string)
        : mWhat(std::move(string)) {}

    virtual const char* what() const noexcept override {
        return mWhat.asCString();
    }
};

/// Exception thrown by BUFF_STOP, it has an extra assert in the constructor for debugging
class StopException final : public Exception {

public:
    StopException(const String& function, const String& file, const int line)
        : Exception(String("STOP @ ") + function + " (" + file + "(" + toStr(line) + "))") {
        BUFF_ASSERT(false, function, file, line);
    }
};

BUFF_NAMESPACE_END
