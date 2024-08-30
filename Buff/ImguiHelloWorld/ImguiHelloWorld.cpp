// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics
// context creation, etc.) (GL3W is a helper library to access OpenGL functions since there is no standard
// header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.) If you are new to Dear
// ImGui, read documentation from the docs/ folder + read the top of imgui.cpp. Read online:
// https://github.com/ocornut/imgui/tree/master/docs

// ReSharper disable All
#include <GLES3/gl32.h>
#include <imgui.h>
#include <imgui_freetype.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_internal.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

struct FreeTypeTest {

    bool         WantRebuild;
    float        FontsMultiply;
    int          FontsPadding;
    unsigned int FontsFlags;

    FreeTypeTest() {
        WantRebuild   = true;
        FontsMultiply = 1.0f;
        FontsPadding  = 1;
        FontsFlags    = 0;
    }

    // Call _BEFORE_ NewFrame()
    bool UpdateRebuild() {
        if (!WantRebuild)
            return false;
        ImGuiIO& io               = ImGui::GetIO();
        io.Fonts->TexGlyphPadding = FontsPadding;
        for (int n = 0; n < io.Fonts->ConfigData.Size; n++) {
            ImFontConfig* font_config       = static_cast<ImFontConfig*>(&io.Fonts->ConfigData[n]);
            font_config->RasterizerMultiply = FontsMultiply;
            font_config->FontBuilderFlags   = FontsFlags;
        }

        io.Fonts->ClearTexData();
        io.Fonts->FontBuilderIO    = ImGuiFreeType::GetBuilderForFreeType();
        io.Fonts->FontBuilderFlags = FontsFlags;
        WantRebuild                = false;
        return true;
    }

    // Call to draw interface
    void ShowFreetypeOptionsWindow() {
        ImGui::Begin("FreeType Options");
        ImGui::ShowFontSelector("Fonts");

        WantRebuild |= ImGui::DragFloat("Multiply", &FontsMultiply, 0.001f, 0.0f, 2.0f);
        WantRebuild |= ImGui::DragInt("Padding", &FontsPadding, 0.1f, 0, 16);
        WantRebuild |= ImGui::CheckboxFlags("NoHinting", &FontsFlags, ImGuiFreeTypeBuilderFlags_NoHinting);
        WantRebuild |= ImGui::CheckboxFlags("NoAutoHint", &FontsFlags, ImGuiFreeTypeBuilderFlags_NoAutoHint);
        WantRebuild |=
            ImGui::CheckboxFlags("ForceAutoHint", &FontsFlags, ImGuiFreeTypeBuilderFlags_ForceAutoHint);
        WantRebuild |=
            ImGui::CheckboxFlags("LightHinting", &FontsFlags, ImGuiFreeTypeBuilderFlags_LightHinting);
        WantRebuild |=
            ImGui::CheckboxFlags("MonoHinting", &FontsFlags, ImGuiFreeTypeBuilderFlags_MonoHinting);
        WantRebuild |= ImGui::CheckboxFlags("Bold", &FontsFlags, ImGuiFreeTypeBuilderFlags_Bold);
        WantRebuild |= ImGui::CheckboxFlags("Oblique", &FontsFlags, ImGuiFreeTypeBuilderFlags_Oblique);
        WantRebuild |= ImGui::CheckboxFlags("Monochrome", &FontsFlags, ImGuiFreeTypeBuilderFlags_Monochrome);
        ImGui::End();
    }
};

// Main code
#ifdef main
#    undef main
#endif
int main(int, char**) {
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    static SDL_Window* window =
        SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example 2",
                         SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED,
                         1280,
                         720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    const SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    static ImGuiIO& io = ImGui::GetIO();
    io.IniFilename     = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(nullptr);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and
    // use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among
    // multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your
    // application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a
    // double backslash \\ !
    io.Fonts->AddFontDefault();
    ImFontConfig fontConfig;
    // fontConfig.RasterizerMultiply = 1.8f;
    static ImFont* font =
        io.Fonts->AddFontFromFileTTF("assets/ImguiHelloWorld/arial.ttf", 16.0f, &fontConfig);
    io.Fonts->AddFontFromFileTTF("assets/ImguiHelloWorld/segoeui.ttf", 17.0f, &fontConfig);
    io.Fonts->AddFontFromFileTTF("assets/ImguiHelloWorld/consola.ttf", 15.0f, &fontConfig);
    io.Fonts->AddFontFromFileTTF("assets/LibImgui/OpenSans-Regular.ttf", 20.0f, &fontConfig);
    if (!font) {
        fprintf(stderr, "Failed to initialize font!\n");
        return 1;
    }
    io.Fonts->FontBuilderIO    = ImGuiFreeType::GetBuilderForFreeType();
    io.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    io.FontDefault             = font;
    // ImGui::PushFont(font);

    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL,
    // io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

    static FreeTypeTest freetype_test;

    // Our state
    static bool   showDemoWindow    = true;
    static bool   showAnotherWindow = false;
    static ImVec4 clearColor        = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    static bool done = false;

    auto func = [](void*) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to
        // use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
        // application. Generally you may always pass all inputs to dear imgui, and hide them from your
        // application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (freetype_test.UpdateRebuild()) {
            // REUPLOAD FONT TEXTURE TO GPU
            ImGui_ImplOpenGL3_DestroyDeviceObjects();
            ImGui_ImplOpenGL3_CreateDeviceObjects();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        freetype_test.ShowFreetypeOptionsWindow();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse
        // its code to learn more about Dear ImGui!).
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named
        // window.
        {
            static float f       = 0.0f;
            static int   counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window",
                            &showDemoWindow); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &showAnotherWindow);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color",
                              reinterpret_cast<float*>(&clearColor)); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when
                                         // edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0 / double(ImGui::GetIO().Framerate),
                        double(ImGui::GetIO().Framerate));
            ImGui::End();
        }

        // 3. Show another simple window.
        if (showAnotherWindow) {
            ImGui::Begin("Another Window",
                         &showAnotherWindow); // Pass a pointer to our bool variable (the window will have a
                                              // closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                showAnotherWindow = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, int(io.DisplaySize.x), int(io.DisplaySize.y));
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    };

#ifdef __EMSCRIPTEN__
    BUFF_ASSERT(!done); // Just to silence warnings
    emscripten_set_main_loop_arg(func, NULL, 0, true);
#else
    while (!done) {
        func(nullptr);
    }
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
