#include "LibUltralight/LibUltralight.h"
#include "LibUltralight/UlFilesystem.h"
#include "LibUltralight/UlLogger.h"
#include "LibWindows/Platform.h"
#include "Lib/containers/Set.h"
#include "Lib/Cryptography.h"
#include "Lib/Filesystem.h"
#include "Lib/FpsMeasurement.h"
#include "Lib/Function.h"
#include "Lib/Json.h"
#include "Lib/Thread.h"
#include "Lib/Time.h"
#include <AppCore/App.h>
#include <AppCore/JSHelpers.h>
#include <AppCore/Monitor.h>
#include <AppCore/Overlay.h>
#include <AppCore/Window.h>
#include <thread>
#include <Ultralight/platform/FileSystem.h>
#include <Ultralight/platform/Logger.h>
#include <Ultralight/platform/Platform.h>
#ifdef __EMSCRIPTEN__
#    include "LibEmscripten/LibEmscripten.h"
#endif
// order matters here
#include "Lib/Expected.h"
#include <bcrypt.h>
#include <d3dkmthk.h>
#include <ntstatus.h>
#include "Lib/UndefIntrusiveMacros.h"

BUFF_NAMESPACE_BEGIN

constexpr bool LOG_TIMING_ISSUES = false;
constexpr int  WM_NEW_FRAME      = WM_USER + 1;
constexpr int  WM_RUN_CALLBACK   = WM_USER + 2;

static String jsFolderPrefix() {
    return "LibUltralight/";
}

class InspectorView final : public ul::WindowListener {
    ul::RefPtr<ul::Window>  mWindow;
    ul::RefPtr<ul::Overlay> mOverlay;

public:
    virtual void OnResize(ul::Window*    BUFF_UNUSED(window),
                          const uint32_t width,
                          const uint32_t height) override {
        mOverlay->Resize(width, height);
    }
    virtual void OnClose(ul::Window* BUFF_UNUSED(window)) override {
        mWindow  = nullptr;
        mOverlay = nullptr;
    }

    void handleNeedsRepaint() {
        if (mOverlay && mOverlay->NeedsRepaint()) {
            BUFF_ASSERT(!mOverlay->is_hidden());
            BUFF_CHECKED_CALL(TRUE, InvalidateRect(HWND(mWindow->native_handle()), nullptr, false));
        }
    }

    ul::RefPtr<ul::View> onCreateInspectorView(const StringView appName, ul::Monitor* monitor) {
        if (mWindow) {
            return nullptr;
        }
        mWindow = ul::Window::Create(monitor,
                                     600,
                                     600,
                                     false,
                                     ul::kWindowFlags_Titled | ul::kWindowFlags_Resizable |
                                         ul::kWindowFlags_Maximizable);
        mWindow->SetTitle((appName + " - DEBUG inspector").asCString());
        mOverlay = ul::Overlay::Create(mWindow, mWindow->width(), mWindow->height(), 0, 0);
        mWindow->set_listener(this);
        mOverlay->Show();
        return mOverlay->view();
    }
};

class UltralightApp::Impl final
    : public ul::WindowListener
    , public ul::ViewListener
    , public ul::LoadListener
    , public Noncopyable {
    ul::RefPtr<ul::App>     mApp;
    ul::RefPtr<ul::Window>  mWindow;
    ul::RefPtr<ul::Overlay> mOverlay;
    InspectorView           mInspector;
    UlLogger                mLogger;
    AutoPtr<UlFilesystem>   mFilesystem;
    String                  mAppName;
    FpsMeasurement          mFps;
    bool                    mRunning = true;
    String                  mTitle;
    TimeStamp               mPreviousIdleTick;
    Duration                mExpectedFrameDuration;
    bool                    mFinishedLoading = false;
    Optional<String>        mPendingException;

public:
    Set<AutoPtr<Panel>>         panels;
    Array<Function<void()>>     onCloseCallbacks;
    int                         nextPanelId = 0;
    Optional<Expected<JsValue>> lastInvokeJsResult;

    UltralightApp& parent;

    // Only for debugging purposes
    // std::atomic_bool insideWebkitMessageHandling = false;
    std::atomic_bool insideJsFunction = false;

    struct ConnectedMetaParam {
        MetaStructBase* metaStruct;
        String          paramKey;
        ConnectedMetaParam(MetaStructBase* metaStruct, String paramKey)
            : metaStruct(metaStruct)
            , paramKey(std::move(paramKey)) {}
    };
    Map<String, ConnectedMetaParam> connectedParams;

    /// Filenames are normalized (lowercase, forward slashes only)
    Map<String, WeakPtr<MemoryFile>> virtualFiles;

    explicit Impl(const UltralightAppSettings& settings, UltralightApp& parent)
        : mTitle(settings.appName)
        , mExpectedFrameDuration(Duration::microseconds(int(1'000'000.f / float(*getRefreshRate()))))
        , parent(parent) {
        mFilesystem = makeAutoPtr<UlFilesystem>(settings.assetsDir, mLogger, virtualFiles);
        BUFF_ASSERT(isMainThread());
        printInVisualStudioDebugWindow(String("Webkit version: ") + WebKitVersionString());
        auto& platform = ul::Platform::instance();
        platform.set_logger(&mLogger);
        platform.set_file_system(mFilesystem.get());

        ul::Settings ulSettings;
        ulSettings.app_name           = toUl(settings.appName);
        ulSettings.developer_name     = BUFF_LIBRARY_NAME;
        ulSettings.force_cpu_renderer = settings.cpuOnly;
        ul::Config config;
        // We instead limit the refresh rate in the event loop:
        config.scroll_timer_delay = config.animation_timer_delay = 1.0 / 999;
        // config.force_repaint = true; // Causes massive slowdowns, probably useful to test GPU accel
        // config.max_update_time                                   = 1.f;

        mApp = ul::App::Create(ulSettings, config);

        ul::Monitor* monitor = mApp->main_monitor();
        const double scale   = monitor->scale();

        mWindow = ul::Window::Create(monitor,
                                     uint(settings.windowSize.x / scale),
                                     uint(settings.windowSize.y / scale),
                                     false,
                                     ul::kWindowFlags_Titled | ul::kWindowFlags_Resizable |
                                         ul::kWindowFlags_Maximizable);
        BUFF_ASSERT(mWindow->is_accelerated() == !settings.cpuOnly);

        if (settings.isMaximized) {
            setWindowMaximized(getNativeWindowHandle(), true);
        } else {
            mWindow->MoveTo(mWindow->PixelsToScreen(settings.windowPosition.x),
                            mWindow->PixelsToScreen(settings.windowPosition.y));
        }

        if (settings.iconRcName.notEmpty()) {
            HWND  hwnd = HWND(getNativeWindowHandle().handle);
            HICON icon = LoadIconA(GetModuleHandle(nullptr), settings.iconRcName.asCString());
            BUFF_ASSERT(icon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG, LPARAM(icon));
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, LPARAM(icon));
        }

        mOverlay = ul::Overlay::Create(mWindow, mWindow->width(), mWindow->height(), 0, 0);

        mWindow->set_listener(this);
        mOverlay->view()->set_view_listener(this);
        mOverlay->view()->set_load_listener(this);

        if (!mFilesystem->FileExists(toUl(settings.mainWindowHtml.getNative()))) {
            throw Exception("Required resource file '" + settings.mainWindowHtml.getNative() +
                            "' not found!");
        }
    }

    /// Needs to be split because it is calling back into the parent in OnWindowObjectReady
    void load(const FilePath& mainUrl) {
        mOverlay->view()->LoadURL(toUl("file:///" + mainUrl.getNative()));

        // Wait for the page to fully load before continuing. Note that this might execute some javascript
        while (!mFinishedLoading) {
            // insideWebkitMessageHandling = true;
            mApp->renderer()->Update();
            // insideWebkitMessageHandling = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    virtual void OnWindowObjectReady(ul::View*         BUFF_UNUSED(caller),
                                     uint64_t          BUFF_UNUSED(frameId),
                                     bool              isMainFrame,
                                     const ul::String& BUFF_UNUSED(url)) override {
        BUFF_ASSERT(isMainFrame);
        // ===================================================================================================
        // Setting up JS
        // ===================================================================================================

        // TODO: namespace the global functions inside some object? It would make the code cleaner, but we
        // would lose lint telling us what is undefined
        const auto context = mOverlay->view()->LockJSContext();
        ul::SetJSContext(context->ctx());

        parent.bindGlobalJsConstant("ULTRALIGHT_VERSION_IMPL",
                                    ULTRALIGHT_VERSION_MAJOR + (ULTRALIGHT_VERSION_MINOR / 10.f));
        parent.bindGlobalJsConstant("IS_DEBUG_IMPL", true);

        // Default alert does not do anything
        getJsObject().DeleteProperty("alert");
        parent.bindGlobalJsFunction<1>("alert", [](const ArrayView<const JsValue> args) {
            displayMessageWindow("JS Alert", args[0]);
            return true;
        });

        parent.bindGlobalJsFunction<0>("toggleInspector", [this]() {
            mOverlay->view()->CreateLocalInspectorView();
            return true;
        });

        parent.bindGlobalJsFunction<1>("getMetaStructParam", [this](const ArrayView<const JsValue> args) {
            const String              id    = String(args[0]);
            const ConnectedMetaParam* param = connectedParams.find(id);
            BUFF_ASSERT(param);
            JsonSerializer serializer;
            serializer.serialize(param->metaStruct->getUnchecked(param->paramKey), "value");
            const String json = serializer.getJson(false);
            return json;
        });

        parent.bindGlobalJsFunction<2>("setMetaStructParam", [this](const ArrayView<const JsValue> args) {
            const String              id    = String(args[0]);
            const ConnectedMetaParam* param = connectedParams.find(id);
            const String              json  = String(args[1]);
            JsonDeserializer          deserializer(json);
            auto                      tmp = param->metaStruct->getUnchecked(param->paramKey);
            deserializer.deserialize(tmp, "value");
            return true;
        });

        parent.bindGlobalJsFunction<2>("returnInvokeJsResult", [this](const ArrayView<const JsValue> args) {
            BUFF_ASSERT(isMainThread());
            BUFF_ASSERT(!lastInvokeJsResult);
            const bool isException = args[1].get<bool>();
            lastInvokeJsResult     = isException ? makeUnexpected(args[0].toJson()) : Expected(args[0]);
        });
    }

    virtual void OnDOMReady(ul::View*         BUFF_UNUSED(caller),
                            uint64_t          BUFF_UNUSED(frameId),
                            bool              isMainFrame,
                            const ul::String& BUFF_UNUSED(url)) override {
        BUFF_ASSERT(isMainFrame);
        mFinishedLoading = true;
    }

    virtual void OnClose(ul::Window* BUFF_UNUSED(window)) override {
        for (const auto& it : onCloseCallbacks) {
            it();
        }
        mRunning = false;
    }
    virtual void OnResize(ul::Window*    BUFF_UNUSED(window),
                          const uint32_t width,
                          const uint32_t height) override {
        mOverlay->Resize(width, height);
    }
    virtual void OnChangeCursor(ul::View* BUFF_UNUSED(caller), const ul::Cursor cursor) override {
        // printInVisualStudioDebugWindow("OnChangeCursor: " + toStr(cursor));
        mWindow->SetCursor(cursor);
    }
    virtual ul::RefPtr<ul::View> OnCreateInspectorView(ul::View*         BUFF_UNUSED(caller),
                                                       bool              BUFF_UNUSED(isLocal),
                                                       const ul::String& BUFF_UNUSED(inspectedUrl)) override {
        return mInspector.onCreateInspectorView(mAppName, mApp->main_monitor());
    }

#if ULTRALIGHT_VERSION_MINOR >= 4
    virtual void OnAddConsoleMessage(ul::View* BUFF_UNUSED(caller), const ul::ConsoleMessage& msg) override {
        const auto level        = msg.level();
        const auto source       = msg.source();
        const auto lineNumber   = msg.line_number();
        const auto columnNumber = msg.column_number();
        const auto sourceId     = msg.source_id();
        const auto message      = msg.message();
#else
    virtual void OnAddConsoleMessage(ul::View*               BUFF_UNUSED(caller),
                                     const ul::MessageSource source,
                                     const ul::MessageLevel  level,
                                     const ul::String&       message,
                                     const uint32_t          lineNumber,
                                     const uint32_t          columnNumber,
                                     const ul::String&       sourceId) override {
#endif
        mLogger.logConsoleMessage(source, level, fromUl(message), lineNumber, columnNumber, fromUl(sourceId));
    }

    void run() {
        std::atomic_int pendingFrames = 0;

        // We might be able to remove this thread and use
        // https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-getframelatencywaitableobject?redirectedfrom=MSDN
        // instead
        Thread thread([&]() {
            setThreadName(GetCurrentThread(), "!"_S + BUFF_LIBRARY_NAME + "_UltralightPaintScheduler");
            while (mRunning) {
                static const D3DKMT_WAITFORVERTICALBLANKEVENT sData = [&]() {
                    D3DKMT_OPENADAPTERFROMHDC getAdapter = {};
                    getAdapter.hDc                       = GetDC(nullptr);
                    [[maybe_unused]] const NTSTATUS res  = D3DKMTOpenAdapterFromHdc(&getAdapter);
                    BUFF_ASSERT(res >= 0);
                    D3DKMT_WAITFORVERTICALBLANKEVENT waitForEvent = {};
                    waitForEvent.hAdapter                         = getAdapter.hAdapter;
                    waitForEvent.VidPnSourceId                    = getAdapter.VidPnSourceId;
                    return waitForEvent;
                }();
                const NTSTATUS res = D3DKMTWaitForVerticalBlankEvent(&sData);
                // There are various error messages that happen when the application is left minimized and/or
                // the system goes to sleep, is remote-connected-to, etc.
                BUFF_ASSERT(res == anyOf(STATUS_WAIT_0,
                                         STATUS_GRAPHICS_PRESENT_OCCLUDED,
                                         WAIT_TIMEOUT,
                                         STATUS_DEVICE_REMOVED),
                            res);
                // if (pendingFrames < 10000) {
                ++pendingFrames;
                const BOOL postResult = PostMessage(HWND(getNativeWindowHandle().handle), WM_NEW_FRAME, 0, 0);
                BUFF_ASSERT(postResult || !mRunning);
                // } else {
                //     printInVisualStudioDebugWindow("Paint scheduler: " + toStr(pendingFrames) +
                //                                    " frames behind, dropping updates...");
                // }
            }
        });

        MSG msg;
        while (mRunning && GetMessage(&msg, nullptr, 0, 0) > 0) {
            const Timer timer;
            if (msg.message == WM_NEW_FRAME) {
                onIdleTick();
                --pendingFrames;
                BUFF_ASSERT(pendingFrames >= 0); // who sent the message without incrementing pendingFrames?!
            } else if (msg.message == WM_RUN_CALLBACK) {
                Function<void()>* functor = reinterpret_cast<Function<void()>*>(msg.wParam);
                (*functor)();
                delete functor;
            } else {
                TranslateMessage(&msg);
                // insideWebkitMessageHandling = true;
                BUFF_ASSERT(!insideJsFunction);
                DispatchMessage(&msg);
                BUFF_ASSERT(!insideJsFunction);
                // insideWebkitMessageHandling = false;
            }
            if (mPendingException) {
                displayMessageWindow("Exception", *mPendingException);
                mPendingException = {};
            }
            if constexpr (LOG_TIMING_ISSUES) {
                if (timer.getElapsed().toMilliseconds() > 20) {
                    printInVisualStudioDebugWindow(
                        "MSG processing took " + timer.getElapsed().getUserReadable() +
                        ". IDLE: " + toStr(msg.message == WM_NEW_FRAME) + ", MSG: " + toStr(msg.message));
                }
            }
        }
        mRunning = false;
    }

    void onIdleTick() {
        const TimeStamp now            = TimeStamp::now();
        const Duration  sinceLastFrame = now - mPreviousIdleTick;
        if constexpr (LOG_TIMING_ISSUES) {
            if (sinceLastFrame > mExpectedFrameDuration * 1.5) {
                printInVisualStudioDebugWindow(
                    "MISSED FRAME! Expected interval between v-blank events: " +
                    mExpectedFrameDuration.getUserReadable() + ", got: " + sinceLastFrame.getUserReadable() +
                    " (" +
                    toStr(sinceLastFrame.toMicroseconds() / mExpectedFrameDuration.toMicroseconds() * 100.f) +
                    "%).");
            }
        }
        mPreviousIdleTick = now;

        const Timer timer;

        mFps.registerTick();

        constexpr FormatFloatParams FPS_FORMAT = {.maxDecimals = 1, .forceDecimals = true};
        mWindow->SetTitle((mTitle + " [" + formatFloat(double(mFps.getFps(1'000).valueOr(-1)), FPS_FORMAT) +
                           " fps last second, " +
                           formatFloat(double(mFps.getFps(10'000).valueOr(-1)), FPS_FORMAT) +
                           " fps last 10s]")
                              .asCString());

        BUFF_ASSERT(!insideJsFunction);
        mApp->renderer()->Update();
        BUFF_ASSERT(!insideJsFunction);

#if ULTRALIGHT_VERSION_MINOR >= 4
        mApp->renderer()->RefreshDisplay(0); // TODO: causing double update frequency
#endif

        static Optional<Array<WindowScreenshot>> sBitmaps; // = Array<WindowScreenshot>();
        if (sBitmaps) {
            if (sBitmaps->size() < 200) {
                sBitmaps->pushBack(screenshotWindow(getNativeWindowHandle()));
            } else {
                for (auto& it : iterate(*sBitmaps)) {
                    it->saveToFile(FilePath("C:/!tmp/screenshot" + toStr(it.index() + 10000) + ".bmp"));
                }
                sBitmaps = {};
            }
        }

        // BUFF_ASSERT(mOverlay->NeedsRepaint());
        if (mOverlay->NeedsRepaint()) {
            BUFF_ASSERT(!mOverlay->is_hidden());
            BUFF_CHECKED_CALL(TRUE, InvalidateRect(HWND(getNativeWindowHandle().handle), nullptr, false));
        }
        mInspector.handleNeedsRepaint();
        if constexpr (LOG_TIMING_ISSUES) {
            if (timer.getElapsed().toMilliseconds() > 3) {
                printInVisualStudioDebugWindow("onIdle() took " + timer.getElapsed().getUserReadable());
            }
        }
    }

    ul::View& getView() {
        return *mOverlay->view();
    }
    ul::Renderer& getRenderer() {
        return *mApp->renderer();
    }

    ul::Window& getWindow() {
        return *mWindow;
    }

    ul::JSObject getJsObject() {
        return ul::JSGlobalObject();
    }

    NativeWindowHandle getNativeWindowHandle() const {
        return NativeWindowHandle(mWindow->native_handle());
    }

    Panel* addPanel(const String& title, const FilePath& htmlFile) {
        const ul::RefPtr<ul::Buffer> fileContent = mFilesystem->OpenFile(toUl(htmlFile.getNative()));
        const String                 srcBase64 =
            toBase64(ArrayView(static_cast<const std::byte*>(fileContent->data()), fileContent->size()));
        const String titleBase64 = toBase64(title);
        const String id          = "panel_" + toStr(nextPanelId++);
        invokeJsFunction(jsFolderPrefix() + "Panel.js",
                         "addFloatingPanelBase64",
                         {id, titleBase64, srcBase64});
        evaluateJs("console.log('Adding panel: " + title + "' );");
        printInVisualStudioDebugWindow("Adding panel: " + title + " with id: " + id + ".");
        AutoPtr<Panel> panel = makeAutoPtr<Panel>(id);
        Panel*         res   = panel.get();
        panels.insert(std::move(panel));
        return res;
    }
    JsValue invokeJsFunction(const StringView               module,
                             const StringView               functionName,
                             const ArrayView<const JsValue> args) {
        BUFF_ASSERT(isMainThread());
        BUFF_ASSERT(!lastInvokeJsResult);
        const String call =
            functionName + "(" + listToStr(args, ", ", [](const JsValue& str) { return str.toJson(); }) + ")";
        const String code = "import('" + module + "').then(m => m." + call +
                            ").then(res => returnInvokeJsResult(res, false)).catch(error => "
                            "returnInvokeJsResult('Promise rejected: ' + error, true));";
        evaluateJs(code);

        while (!lastInvokeJsResult) {
            getRenderer().Update();
        }
        const Finally finally = [&] { lastInvokeJsResult = {}; };
        if (*lastInvokeJsResult) {
            return **lastInvokeJsResult;
        } else {
            const String msg = "Error in invokeJsFunction(" + module + " :: " + functionName +
                               "): " + lastInvokeJsResult->getError();
            BUFF_ASSERT(false, msg);
            throw Exception(msg);
        }
    }
    String evaluateJs(const String& js) {
        ul::String       exception;
        const ul::String result = getView().EvaluateScript(toUl(js), &exception);
        if (!exception.empty()) {
            throw Exception("Error in evaluateJs: " + fromUl(exception) + " for code: " + js);
        }
        return fromUl(result);
    }

    void postException(String exception) {
        mPendingException = std::move(exception);
    }
};

UltralightApp::UltralightApp(const UltralightAppSettings& settings) {
    mImpl = makeAutoPtr<Impl>(settings, *this);
    mImpl->load(settings.mainWindowHtml);
}

UltralightApp::~UltralightApp() = default;

void UltralightApp::run() {
    mImpl->run();
}

void UltralightApp::enqueueOnMainThread(Function<void()> functor) {
    Function<void()>* heapFunctor = new Function(std::move(functor));
    BUFF_CHECKED_CALL(TRUE,
                      PostMessage(HWND(getNativeWindowHandle().handle),
                                  WM_RUN_CALLBACK,
                                  reinterpret_cast<WPARAM>(heapFunctor),
                                  0));
}

ul::View& UltralightApp::getView() {
    return mImpl->getView();
}

ul::JSObject UltralightApp::getJsObject() {
    return mImpl->getJsObject();
}

NativeWindowHandle UltralightApp::getNativeWindowHandle() const {
    return {mImpl->getNativeWindowHandle()};
}

Panel* UltralightApp::addPanel(const String& title, const FilePath& htmlFile) {
    return mImpl->addPanel(title, htmlFile);
}

String UltralightApp::evaluateJs(const String& js) {
    return mImpl->evaluateJs(js);
}

JsValue UltralightApp::invokeJsFunction(const StringView               module,
                                        const StringView               functionName,
                                        const ArrayView<const JsValue> args) {
    return mImpl->invokeJsFunction(module, functionName, args);
}

void UltralightApp::addVirtualFile(const FilePath& virtualPath, WeakPtr<MemoryFile> content) {
    BUFF_ASSERT(!virtualPath.getGeneric().startsWith(VIRTUAL_FILE_PREFIX));
    mImpl->virtualFiles.insert(normalizeVirtualFilePath(virtualPath), std::move(content));
}

ul::JSValue jsValueToUl(const JsValue& value) {
    return value.visitValue(
        [](const JsValue::Undefined BUFF_UNUSED(x)) { return ul::JSValue(ul::JSValueUndefinedTag {}); },
        [](const JsValue::Null BUFF_UNUSED(x)) { return ul::JSValue(ul::JSValueNullTag {}); },
        [](const bool x) { return ul::JSValue(x); },
        [](const double x) { return ul::JSValue(x); },
        [](const String& x) { return ul::JSValue(toUl(x)); },
        [](const JsArray& x) {
            ul::JSArray retVal;
            x.visitAllValues(
                [&](const int64 index, const JsValue& val) { retVal[unsigned(index)] = jsValueToUl(val); });
            return ul::JSValue(retVal);
        },
        [](const JsObject& x) {
            ul::JSObject retVal;
            x.visitAllValues([&](const String& k, const JsValue& v) { retVal[toUl(k)] = jsValueToUl(v); });
            return ul::JSValue(retVal);
        });
}

JsValue jsValueFromUl(const ul::JSValue& value) {
    if (value.IsUndefined()) {
        return JsValue::Undefined();
    } else if (value.IsNull()) {
        return JsValue::Null();
    } else if (value.IsBoolean()) {
        return value.ToBoolean();
    } else if (value.IsNumber()) {
        return value.ToNumber();
    } else if (value.IsString()) {
        return fromUl(value.ToString());
    } else if (value.IsArray()) {
        ul::JSArray arr = value.ToArray();
        JsArray     result;
        for (const int i : range(arr.length())) {
            result.push(jsValueFromUl(arr[i]));
        }
        return std::move(result);
    } else if (value.IsObject()) {
        // We need to use the JSCore API, there is no API to discover keys of an object in ultralight...
        JsObject                     result;
        const ul::JSObject           object        = value.ToObject();
        const JSObjectRef            objRef        = object;
        const JSContextRef           context       = object.context();
        const JSPropertyNameArrayRef propertyNames = JSObjectCopyPropertyNames(context, objRef);
        const Finally                finally       = [&] { JSPropertyNameArrayRelease(propertyNames); };
        const size_t                 count         = JSPropertyNameArrayGetCount(propertyNames);
        for (const size_t i : range(count)) {
            ul::JSString name                = JSPropertyNameArrayGetNameAtIndex(propertyNames, i);
            result[fromUl(ul::String(name))] = jsValueFromUl(object[name]);
        }
        return std::move(result);
    } else {
        BUFF_STOP;
    }
}

void UltralightApp::connectParamImpl(MetaStructBase& metaStruct, const String& key) {
    if (Impl::ConnectedMetaParam* found = mImpl->connectedParams.find(key)) {
        *found = Impl::ConnectedMetaParam {&metaStruct, key};
    } else {
        mImpl->connectedParams.insert(key, {&metaStruct, key});
    }

    // Connect update C++ -> JS:
    metaStruct.getUnchecked(key).addObserver([this, key]() -> bool {
        if (!mImpl->connectedParams.contains(key)) {
            // This can happen when the UI element was destroyed but the callback didn't run yet. It is
            // simpler to just unregister the callback at its next run.
            return false;
        }
        return invokeJsFunction(jsFolderPrefix() + "Lib.js", "updateMetaStructParam", {key}).get<bool>();
    });

    // Connect update JS -> C++:
    invokeJsFunction(jsFolderPrefix() + "Lib.js", "connectMetaStructParam", {key});
}

void UltralightApp::bindGlobalJsFunctionImpl(const StringView                            name,
                                             const int                                   numArgs,
                                             Function<JsValue(ArrayView<const JsValue>)> function) {
    ul::JSObject globalJsObject = getJsObject();
    BUFF_ASSERT(!globalJsObject.HasProperty(toUl(name)));
    globalJsObject[toUl(name)] = std::function(
        [this, function = std::move(function), numArgs](const ul::JSObject& BUFF_UNUSED(thisObj),
                                                        const ul::JSArgs&   args) -> ul::JSValue {
            // BUFF_ASSERT(!isMainThread()); Can be both main and worker thread
            try {
                mImpl->insideJsFunction = true;
                const Finally reset     = [&]() { mImpl->insideJsFunction = false; };
                // BUFF_ASSERT(mImpl->insideWebkitMessageHandling);
                BUFF_ASSERT(args.size() == uint(numArgs));
                Array<JsValue> convertedArgs;
                convertedArgs.reserve(numArgs);
                for (const int i : range(numArgs)) {
                    convertedArgs.pushBack(jsValueFromUl(args[i]));
                }
                const JsValue result = function(convertedArgs);
                // BUFF_ASSERT(mImpl->insideWebkitMessageHandling);
                return jsValueToUl(result);
            } catch (const std::exception& e) {
                mImpl->postException(e.what());
                return {};
            }
        });
}

void UltralightApp::bindGlobalJsConstant(StringView name, JsValue value) {
    const ul::JSObject globalJsObject = getJsObject();
    BUFF_ASSERT(!globalJsObject.HasProperty(toUl(name)));
    globalJsObject[toUl(name)] = jsValueToUl(value);
}

Pixel UltralightApp::getWindowSize() const {
    auto& window = mImpl->getWindow();
    Pixel result(window.width(), window.height());
    return result;
}

Pixel UltralightApp::getWindowPosition() const {
    const auto& window = mImpl->getWindow();
    const Pixel result(window.ScreenToPixels(window.x()), window.ScreenToPixels(window.y()));
    return result;
}

bool UltralightApp::isMaximized() const {
    return isWindowMaximized(mImpl->getNativeWindowHandle());
}

void UltralightApp::registerOnClose(Function<void()> callback) {
    mImpl->onCloseCallbacks.pushBack(std::move(callback));
}

void UltralightApp::exit() {
    // using ultralight::Window::Close from javascript callback crashes the application
    BUFF_CHECKED_CALL(TRUE, PostMessage(HWND(mImpl->getNativeWindowHandle().handle), WM_CLOSE, 0, 0));
}

BUFF_NAMESPACE_END
