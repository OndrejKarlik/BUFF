#pragma once
#include "LibSdl/SdlWindow.h"
#include "Lib/containers/Array2D.h"
#include "Lib/Path.h"
#include "Lib/Rgb8Bit.h"
#include "Lib/Vector.h"
#include <GLES3/gl32.h>
#include <imgui.h>

BUFF_NAMESPACE_BEGIN

struct ImguiSettings {
    DirectoryPath assetsDir = "assets"_Dir;
};

void imguiStart(const ImguiSettings& settings);
void imguiEnd();

class ImguiWindow final : public SdlWindow {
    static inline bool sCreated = false;
    SDL_GLContext      mOpenglContext;
    ImGuiWindowFlags   mFlags;

public:
    ImguiWindow(Pixel size, StringView title, Flags<SDL_WindowFlags> sdlFlags, ImGuiWindowFlags imguiFlags);
    virtual ~ImguiWindow() override;

    virtual void dispatchProcessEvent(const SDL_Event& event) override;

    virtual void dispatchDraw() const override;
};

class ImguiTexture final : public Noncopyable {
    Array2D<Rgba8Bit> mTexture;
    Optional<GLuint>  mTextureId;
    Pixel             mSize;

public:
    explicit ImguiTexture(const FilePath& filename);

    explicit ImguiTexture(const Array2D<Rgba8Bit>& bitmap);
    explicit ImguiTexture(const Array2D<Rgb8Bit>& bitmap);

    ~ImguiTexture();

    void draw(const Vector2& position, Optional<Vector2> size = NULL_OPTIONAL) const;
    void draw(const Pixel& position, Optional<Pixel> size = NULL_OPTIONAL) const;

    Array2D<Rgba8Bit> toBitmap() const;

    Pixel getResolution() const;

private:
    void init();
};

inline Vector2 fromImgui(const ImVec2& value) {
    return {value.x, value.y};
}

inline ImVec2 toImgui(const Vector2& value) {
    return {value.x, value.y};
}

inline ImVec2 toImgui(const Pixel& value) {
    return {float(value.x), float(value.y)};
}

inline ImU32 toImgui(const Rgb8Bit& value, const uint8 alpha = 255) {
    return IM_COL32(value[0], value[1], value[2], alpha);
}
inline ImU32 toImgui(const Rgba8Bit& value) {
    return IM_COL32(value[0], value[1], value[2], value.alpha());
}

BUFF_NAMESPACE_END
