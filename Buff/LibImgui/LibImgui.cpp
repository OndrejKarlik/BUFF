#include "LibImgui/LibImgui.h"
#include "LibImage/LibImage.h"
#include "LibSdl/LibSdl.h"
#include "LibSdl/SdlTexture.h"
#include "Lib/Filesystem.h"
#include "Lib/Time.h"
#include <GLES3/gl32.h>
#include <imgui_freetype.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include <SDL2/SDL_video.h>

BUFF_NAMESPACE_BEGIN

#if 0 // TODO make this work
static SDL_GLContext gOpenglContext;

static void openGlErrorCallback(const GLenum  source,
                                const GLenum  type,
                                const GLuint  id,
                                const GLenum  severity,
                                const GLsizei BUFF_UNUSED(length),
                                const GLchar* message,
                                const void*   BUFF_UNUSED(userParam)) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        // Just informative messages
        return;
    }
    BUFF_ASSERT(false,
              "OpenGL error: source",
              source,
              "type",
              type,
              "id",
              id,
              "severity",
              severity,
              "message",
              message);
}
#endif

void imguiStart(const ImguiSettings& settings) {
    sdlStart();
    libImageStart();

    // Create window with graphics context
    BUFF_CHECKED_CALL(0, SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
    BUFF_CHECKED_CALL(0, SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24));
    BUFF_CHECKED_CALL(0, SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8));

    // Setup Dear ImGui context
    ImGui::CreateContext();

    ImGuiIO& io    = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImFontConfig fontConfig;
    fontConfig.RasterizerMultiply = 1.2f;
    ImFont* font                  = io.Fonts->AddFontFromFileTTF(
        (settings.assetsDir / "LibImgui/OpenSans-Regular.ttf"_File).getNative().asCString(),
        19.0f,
        &fontConfig);
    BUFF_ASSERT(font);
    io.FontDefault             = font;
    io.Fonts->FontBuilderIO    = ImGuiFreeType::GetBuilderForFreeType();
    io.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
}

void imguiEnd() {
    ImGui::DestroyContext();

    libImageShutdown();
    sdlShutdown();
}

// ===========================================================================================================

// Based on https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9#file-imguiutils-h-L9-L93
static void setupImGuiStyle() {
#if 0
    ImGuiStyle& style = ImGui::GetStyle();
    // light style from Pacôme Danhiez (user itamago)
    // https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
    style.Alpha                         = 1.0f;
    style.FrameRounding                 = 3.0f;
    style.Colors[ImGuiCol_Text]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]     = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    // style.Colors[ImGuiCol_ChildWindowBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow]         = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    //  style.Colors[ImGuiCol_ComboBg]              = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
    style.Colors[ImGuiCol_CheckMark]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]       = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button]           = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]     = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header]           = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    //    style.Colors[ImGuiCol_Column]               = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    //   style.Colors[ImGuiCol_ColumnHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    //   style.Colors[ImGuiCol_ColumnActive]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]        = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    //  style.Colors[ImGuiCol_CloseButton]          = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
    //  style.Colors[ImGuiCol_CloseButtonHovered]   = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
    //  style.Colors[ImGuiCol_CloseButtonActive]    = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    //  style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    constexpr bool DARK = true;
    if (DARK) {
        for (int i = 0; i <= ImGuiCol_COUNT; i++) {
            ImVec4& col = style.Colors[i];
            float   H, S, V;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

            if (S < 0.1f) {
                V = 1.0f - V;
            }
            ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
            // if (col.w < 1.00f) {
            //     col.w *= alpha_;
            // }
        }
    } /* else {
         for (int i = 0; i <= ImGuiCol_COUNT; i++) {
             ImVec4& col = style.Colors[i];
             if (col.w < 1.00f) {
                 col.x *= alpha_;
                 col.y *= alpha_;
                 col.z *= alpha_;
                 col.w *= alpha_;
             }
         }
     }*/
#else

    auto& colors               = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg]  = ImVec4 {0.1f, 0.1f, 0.13f, 1.0f};
    colors[ImGuiCol_MenuBarBg] = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};

    // Border
    colors[ImGuiCol_Border]       = ImVec4 {0.44f, 0.37f, 0.61f, 0.29f};
    colors[ImGuiCol_BorderShadow] = ImVec4 {0.0f, 0.0f, 0.0f, 0.24f};

    // Text
    colors[ImGuiCol_Text]         = ImVec4 {1.0f, 1.0f, 1.0f, 1.0f};
    colors[ImGuiCol_TextDisabled] = ImVec4 {0.5f, 0.5f, 0.5f, 1.0f};

    // Headers
    colors[ImGuiCol_Header]        = ImVec4 {0.13f, 0.13f, 0.17f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4 {0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_HeaderActive]  = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button]        = ImVec4 {0.13f, 0.13f, 0.17f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4 {0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_ButtonActive]  = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_CheckMark]     = ImVec4 {0.74f, 0.58f, 0.98f, 1.0f};

    // Popups
    colors[ImGuiCol_PopupBg] = ImVec4 {0.1f, 0.1f, 0.13f, 0.92f};

    // Slider
    colors[ImGuiCol_SliderGrab]       = ImVec4 {0.44f, 0.37f, 0.61f, 0.54f};
    colors[ImGuiCol_SliderGrabActive] = ImVec4 {0.74f, 0.58f, 0.98f, 0.54f};

    // Frame BG
    colors[ImGuiCol_FrameBg]        = ImVec4 {0.13f, 0.13f, 0.17f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4 {0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_FrameBgActive]  = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};

    // Tabs
    colors[ImGuiCol_Tab]                = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TabHovered]         = ImVec4 {0.24f, 0.24f, 0.32f, 1.0f};
    colors[ImGuiCol_TabActive]          = ImVec4 {0.2f, 0.22f, 0.27f, 1.0f};
    colors[ImGuiCol_TabUnfocused]       = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};

    // Title
    colors[ImGuiCol_TitleBg]          = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TitleBgActive]    = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]          = ImVec4 {0.1f, 0.1f, 0.13f, 1.0f};
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4 {0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4 {0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4 {0.24f, 0.24f, 0.32f, 1.0f};

    // Separator
    colors[ImGuiCol_Separator]        = ImVec4 {0.44f, 0.37f, 0.61f, 1.0f};
    colors[ImGuiCol_SeparatorHovered] = ImVec4 {0.74f, 0.58f, 0.98f, 1.0f};
    colors[ImGuiCol_SeparatorActive]  = ImVec4 {0.84f, 0.58f, 1.0f, 1.0f};

    // Resize Grip
    colors[ImGuiCol_ResizeGrip]        = ImVec4 {0.44f, 0.37f, 0.61f, 0.29f};
    colors[ImGuiCol_ResizeGripHovered] = ImVec4 {0.74f, 0.58f, 0.98f, 0.29f};
    colors[ImGuiCol_ResizeGripActive]  = ImVec4 {0.84f, 0.58f, 1.0f, 0.29f};

    // Docking
    colors[ImGuiCol_DockingPreview] = ImVec4 {0.44f, 0.37f, 0.61f, 1.0f};

    auto& style             = ImGui::GetStyle();
    style.TabRounding       = 4;
    style.ScrollbarRounding = 9;
    style.WindowRounding    = 7;
    style.GrabRounding      = 3;
    style.FrameRounding     = 3;
    style.PopupRounding     = 4;
    style.ChildRounding     = 4;

#endif
}

ImguiWindow::ImguiWindow(const Pixel                  size,
                         const StringView             title,
                         const Flags<SDL_WindowFlags> flags,
                         const ImGuiWindowFlags       imguiFlags)
    : SdlWindow(size, title, SDL_WINDOW_OPENGL | flags)
    , mFlags(imguiFlags) {
    BUFF_ASSERT(!sCreated);

    mOpenglContext = SDL_GL_CreateContext(mWindow);
    BUFF_ASSERT(mOpenglContext);
    BUFF_CHECKED_CALL(0, SDL_GL_SetSwapInterval(1)); // Enable vsync

#if 0 // TODO: Does not link on windows, is not present at all on emscripten
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openGlErrorCallback, 0);
#endif

    // Setup Platform/Renderer backends
    BUFF_CHECKED_CALL(true, ImGui_ImplSDL2_InitForOpenGL(mWindow, mOpenglContext));
    BUFF_CHECKED_CALL(true, ImGui_ImplOpenGL3_Init(nullptr));
    setupImGuiStyle();
    sCreated = true;
}

ImguiWindow::~ImguiWindow() {
    BUFF_ASSERT(sCreated);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    SDL_GL_DeleteContext(mOpenglContext);
    sCreated = false;
}

void ImguiWindow::dispatchProcessEvent(const SDL_Event& event) {
    if constexpr ((0)) {
        std::cout << clock() << " | " << event.type << std::endl;
    }

    ImGui_ImplSDL2_ProcessEvent(&event);
    if constexpr ((0)) {
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                std::cout << "Window Resized: " << event.window.data1 << " " << event.window.data2
                          << std::endl;
            }
        }
    }
    mContent->processEvent(event);
}

void ImguiWindow::dispatchDraw() const {
    const Timer    timer;
    const ImGuiIO& io = ImGui::GetIO();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(float(io.DisplaySize.x), float(io.DisplaySize.y)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    const float originalWindowRounding = ImGui::GetStyle().WindowRounding;
    ImGui::GetStyle().WindowRounding   = 0;
    BUFF_ASSERT((mFlags & (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) == 0);
    ImGui::Begin("root", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | mFlags);
    ImGui::GetStyle().WindowRounding = originalWindowRounding;
    mContent->draw();
    ImGui::End();
    ImGui::PopStyleVar(2);

    // Rendering
    ImGui::Render();
    glViewport(0, 0, int(io.DisplaySize.x), int(io.DisplaySize.y));
    glClearColor(0, 1.f, 0, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Do this before swap which does the FPS limiting
    setLastRenderingTime(int(timer.getElapsed().toMilliseconds()));

    SDL_GL_SwapWindow(mWindow);
}

ImguiTexture::ImguiTexture(const FilePath& filename) {
    BUFF_ASSERT(fileExists(filename));
    mTexture = Image(filename).getBitmap();
    init();
}

ImguiTexture::ImguiTexture(const Array2D<Rgba8Bit>& bitmap)
    : mTexture(bitmap) {
    init();
}

ImguiTexture::ImguiTexture(const Array2D<Rgb8Bit>& bitmap) {
    mTexture.resize(bitmap.size());
    for (int64 i = 0; i < mTexture.getPixelCount(); ++i) {
        mTexture.getNthPixel(i) = Rgba8Bit(mTexture.getNthPixel(i));
    }
    init();
}

ImguiTexture::~ImguiTexture() {
    if (mTextureId) {
        glDeleteTextures(1, &*mTextureId);
    }
}

void ImguiTexture::draw(const Vector2& position, const Optional<Vector2> size) const {
    const ImVec2      corner0 = toImgui(position);
    const ImVec2      corner1 = toImgui(position + size.valueOr(Vector2(mSize)));
    const ImTextureID id      = reinterpret_cast<ImTextureID>(intptr_t(*mTextureId));
    ImGui::GetWindowDrawList()->AddImage(id, corner0, corner1);
}

void ImguiTexture::draw(const Pixel& position, Optional<Pixel> size) const {
    return draw(Vector2(position), size ? Optional(Vector2(*size)) : Optional<Vector2> {});
}

void ImguiTexture::init() {
    mSize = mTexture.size();
    BUFF_ASSERT(mSize.x > 0 && mSize.y > 0);

    mTextureId.emplace();
    glGenTextures(1, &*mTextureId);
    glBindTexture(GL_TEXTURE_2D, *mTextureId);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, mTexture.data());
}

Array2D<Rgba8Bit> ImguiTexture::toBitmap() const {
    BUFF_ASSERT(mTextureId);
    return mTexture;
}

Pixel ImguiTexture::getResolution() const {
    return mTexture.size();
}

BUFF_NAMESPACE_END
