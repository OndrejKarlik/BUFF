#pragma once
#include "Lib/AutoPtr.h"
#include "Lib/Function.h"
#include "Lib/meta/MetaStruct.h"
#include "Lib/Path.h"
#include "Lib/Pixel.h"
#include "Lib/Platform.h"
#include "Lib/SharedPtr.h"
#include <Ultralight/String.h>

// ReSharper disable CppInconsistentNaming
namespace ultralight {
class JSObject;
class JSValue;
class View;
}
// ReSharper restore CppInconsistentNaming

BUFF_NAMESPACE_BEGIN

class JsValue;
class JsObject;
template <typename T>
class Function;

// ReSharper disable once CppInconsistentNaming
namespace ul = ultralight;

inline const String VIRTUAL_FILE_PREFIX = "_VIRTUAL_/";

inline ul::String toUl(const StringView in) {
    return {in.data(), size_t(in.size())};
}
inline ul::String toUl(const String& in) {
    return {in.asCString(), size_t(in.size())};
}

inline String fromUl(const ul::String& in) {
    String result = in.utf8().data();
    if (!result.isValidUtf8()) {
        // TODO: There is a bug in webkit (?) that causes CP-1252 encoded strings to be returned sometimes
        // posing as UTF-8
        result = String::fromCp1252(in.utf8().data(), int(in.utf8().size()));
        BUFF_ASSERT(result.isValidUtf8());
    }
    return result;
}

ul::JSValue jsValueToUl(const JsValue& value);

JsValue jsValueFromUl(const ul::JSValue& value);

class Panel : public Noncopyable {
    String mId;

public:
    explicit Panel(const String& id)
        : mId(id) {}
    auto operator<=>(const Panel& other) const {
        return mId <=> other.mId;
    }
};

struct UltralightAppSettings {
    String appName = "[DEFAULT]";

    DirectoryPath assetsDir {"assets"};

    FilePath mainWindowHtml;

    /// Starting window position in pixels
    Pixel windowSize = Pixel(1280, 800);

    /// Starting window active region size in pixels. The window will be bigger than this because of borders
    /// and title bar
    Pixel windowPosition = Pixel(400, 400);

    bool isMaximized = false;

    bool cpuOnly = false;

    String iconRcName;
};

class UltralightApp : public Noncopyable {
    class Impl;
    AutoPtr<Impl> mImpl;

public:
    /// Creates a window with HTML specified in the settings.
    explicit UltralightApp(const UltralightAppSettings& settings);
    ~UltralightApp();

    void         run();
    ul::View&    getView();
    ul::JSObject getJsObject();

    void enqueueOnMainThread(Function<void()> functor);

    String evaluateJs(const String& js);

    JsValue invokeJsFunction(StringView module, StringView functionName, ArrayView<const JsValue> args);

    Panel* addPanel(const String& title, const FilePath& htmlFile);

    /// virtual files are accessible with file:///_VIRTUAL_/... URLs (see VIRTUAL_FILE_PREFIX)
    /// \param virtualPath does NOT contain the _VIRTUAL_ prefix
    void addVirtualFile(const FilePath& virtualPath, WeakPtr<Array<std::byte>> content);

    template <StringLiteral TParamName, typename TMetaStruct>
    void connectParam(TMetaStruct& metaStruct) {
        constexpr typename TMetaStruct::CheckedKey KEY(TParamName.value);
        return connectParamImpl(metaStruct, KEY.getName());
    }

    NativeWindowHandle getNativeWindowHandle() const;

    /// Returns the size of the window's client area in pixels
    Pixel getWindowSize() const;

    /// Returns the position of the window in pixels
    Pixel getWindowPosition() const;

    bool isMaximized() const;

    void registerOnClose(Function<void()> callback);

    void exit();

    template <int TArgumentCount, typename TFunction>
    void bindGlobalJsFunction(StringView name, TFunction&& function) {
        bindGlobalJsFunctionImpl(
            name,
            TArgumentCount,
            [function = std::forward<TFunction>(function)](const ArrayView<const JsValue> args) -> JsValue {
                if constexpr (TArgumentCount == 0) {
                    BUFF_ASSERT(args.size() == 0);
                    if constexpr (std::is_same_v<void, std::invoke_result_t<TFunction>>) {
                        function();
                        return {};
                    } else {
                        return function();
                    }
                } else {
                    if constexpr (std::is_same_v<void, std::invoke_result_t<TFunction, ArrayView<JsValue>>>) {
                        function(args);
                        return {};
                    } else {
                        return function(args);
                    }
                }
            });
    }

    void bindGlobalJsConstant(StringView name, JsValue value);

private:
    void connectParamImpl(MetaStructBase& metaStruct, const String& key);
    void bindGlobalJsFunctionImpl(StringView                                  name,
                                  int                                         numArgs,
                                  Function<JsValue(ArrayView<const JsValue>)> function);
};

BUFF_NAMESPACE_END
