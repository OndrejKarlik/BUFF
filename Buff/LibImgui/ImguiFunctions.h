#pragma once
#include "LibImgui/LibImgui.h"
#include "Lib/BoundingBox.h"
#include <imgui_impl_opengl3.h>

BUFF_NAMESPACE_BEGIN

namespace ImGui {
using namespace ::ImGui;

inline Pixel getWindowResolution() {
    const auto [x, y] = ImGui::GetWindowSize();
    BUFF_ASSERT(float(int(x)) == x && float(int(y)) == y);
    BUFF_ASSERT(ImGui::GetWindowContentRegionMin().x == 0 && ImGui::GetWindowContentRegionMin().y == 0);
    BUFF_ASSERT(ImGui::GetWindowContentRegionMax().x == x && ImGui::GetWindowContentRegionMax().y == y);
    return {int(x), int(y)};
}

inline void buildFonts() {
    BUFF_CHECKED_CALL(true, ImGui_ImplOpenGL3_CreateDeviceObjects());
}

inline void addTextCentered(const BoundingBox2& pos, Rgba8Bit col, const StringView text) {
    const float drawWidth = pos.getSize().x;
    float       y         = pos.getTopLeft().y;
    for (auto& segment : text.explode("\n")) {
        const char* remaining = segment.data();
        if (segment.isEmpty()) { // Empty lines should be visible
            y += ImGui::CalcTextSize(" ", nullptr, false, pos.getSize().x).y;
            continue;
        }
        while (remaining - segment.data() < segment.size()) {
            const char* nextWrap = ImGui::GetFont()->CalcWordWrapPositionA(1.f,
                                                                           remaining,
                                                                           segment.data() + segment.size(),
                                                                           drawWidth);

            // Trim the string - CalcWordWrapPositionA returns position with leading spaces/newlines
            const StringView str = StringView(remaining, int(nextWrap - remaining)).getTrimmed();

            const ImVec2 size =
                ImGui::CalcTextSize(str.data(), str.data() + str.size(), false, pos.getSize().x);
            ImGui::GetWindowDrawList()->AddText({pos.getTopLeft().x + (drawWidth - size.x) / 2, y},
                                                toImgui(col),
                                                str.data(),
                                                str.data() + str.size());
            remaining = nextWrap;
            y += size.y;
        }
    }
}
} // namespace ImGui

BUFF_NAMESPACE_END
