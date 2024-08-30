#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/ArrayView.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

String                     toBase64(ArrayView<const std::byte> data);
String                     toBase64(StringView input);
Optional<Array<std::byte>> fromBase64(StringView input);

String getMd5Ascii(StringView string);

BUFF_NAMESPACE_END
