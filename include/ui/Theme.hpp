#pragma once

#include <imgui.h>
#include <implot.h>
#include <string>

namespace crypto {

enum class ThemePreset {
    CHAINBLOCK_DARK = 0,    // 🖤 Exact Reference: Black / Dark Gray / Emerald #49B89A
    CLASSIC_SLATE,
    SAKURA_PASTEL,
    COZY_MOCHA,
    SWEET_CANDY,
    MATRIX_EMERALD,
    BLOOMBERG_GOLD,
    TOKYO_NEON,
    RED_TACTICAL_HACKER,
    CYBERPUNK_OBSIDIAN
};

class Theme {
public:
    static inline ThemePreset s_activePreset = ThemePreset::CHAINBLOCK_DARK;

    // 1. Primary Background (#000000)
    static inline ImVec4 PureBlack()       { return ImVec4(0.000f, 0.000f, 0.000f, 1.00f); } // #000000
    static inline ImVec4 DeepObsidian()    { return ImVec4(0.000f, 0.000f, 0.000f, 1.00f); } // #000000
    
    // 2. Sidebar (#101010)
    static inline ImVec4 SidebarBg()       { return ImVec4(0.063f, 0.063f, 0.063f, 1.00f); } // #101010
    
    // 3. Main Dashboard (#141414)
    static inline ImVec4 MainDashboardBg() { return ImVec4(0.078f, 0.078f, 0.078f, 1.00f); } // #141414
    static inline ImVec4 DarkSurface()     { return ImVec4(0.078f, 0.078f, 0.078f, 1.00f); } // #141414
    
    // 4. Cards (#161616 / #181818)
    static inline ImVec4 CardSurface()     { return ImVec4(0.086f, 0.086f, 0.086f, 1.00f); } // #161616
    static inline ImVec4 CardSurfaceAlt()  { return ImVec4(0.094f, 0.094f, 0.094f, 1.00f); } // #181818
    static inline ImVec4 CardHover()       { return ImVec4(0.110f, 0.110f, 0.110f, 1.00f); } // #1C1C1C
    
    // 5. Borders (#2A2A2A / #303030)
    static inline ImVec4 BorderGlow()      { return ImVec4(0.165f, 0.165f, 0.165f, 1.00f); } // #2A2A2A
    static inline ImVec4 BorderLight()     { return ImVec4(0.188f, 0.188f, 0.188f, 1.00f); } // #303030
    
    // 6. Typography
    static inline ImVec4 CrystalWhite()    { return ImVec4(0.961f, 0.961f, 0.961f, 1.00f); } // #F5F5F5 Primary Text
    static inline ImVec4 TextMuted()       { return ImVec4(0.627f, 0.627f, 0.627f, 1.00f); } // #A0A0A0 Secondary Text
    static inline ImVec4 TextDim()         { return ImVec4(0.400f, 0.400f, 0.400f, 1.00f); } // #666666 Muted Text
    
    // 7. Accents & Trading Colors
    static inline ImVec4 EmeraldGreen()    { return ImVec4(0.286f, 0.722f, 0.604f, 1.00f); } // #49B89A Main Accent
    static inline ImVec4 ButtonGreen()     { return ImVec4(0.333f, 0.780f, 0.651f, 1.00f); } // #55C7A6 Button Green
    static inline ImVec4 ChartGreen()      { return ImVec4(0.302f, 0.725f, 0.608f, 1.00f); } // #4DB99B Chart Green
    static inline ImVec4 CrimsonCoral()    { return ImVec4(0.753f, 0.314f, 0.302f, 1.00f); } // #C0504D Muted Negative Red
    
    // Compatibility Aliases (all mapped to exact reference palette)
    static inline ImVec4 BodyPink()        { return EmeraldGreen(); }
    static inline ImVec4 DeepFuchsia()     { return ButtonGreen(); }
    static inline ImVec4 SkyBlue()         { return ChartGreen(); }
    static inline ImVec4 ElectricCyan()    { return ChartGreen(); }
    static inline ImVec4 NeonGreen()       { return EmeraldGreen(); }
    static inline ImVec4 LaserRed()        { return CrimsonCoral(); }
    static inline ImVec4 AmberGold()       { return TextMuted(); }
    static inline ImVec4 GlitchOrange()    { return EmeraldGreen(); }
    static inline ImVec4 CyberPurple()     { return CardSurfaceAlt(); }
    static inline ImVec4 MatrixDim()       { return TextDim(); }

    static void setPreset(ThemePreset preset) {
        s_activePreset = preset;
        applyTheme();
    }

    static void applyTheme() {
        if (!ImGui::GetCurrentContext()) return;

        ImGuiStyle& style = ImGui::GetStyle();

        // Exact Chainblock Geometry: 12-14px Window Radius, 10-12px Card Radius, 6-8px Controls
        style.WindowRounding    = 14.0f;
        style.ChildRounding     = 12.0f;
        style.FrameRounding     = 8.0f;
        style.PopupRounding     = 10.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding      = 6.0f;
        style.TabRounding       = 8.0f;

        style.WindowPadding     = ImVec2(20.0f, 20.0f);
        style.FramePadding      = ImVec2(12.0f, 7.0f);
        style.ItemSpacing       = ImVec2(12.0f, 10.0f);
        style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
        style.ScrollbarSize     = 8.0f;
        style.WindowBorderSize  = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.FrameBorderSize   = 1.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                  = CrystalWhite();
        colors[ImGuiCol_TextDisabled]          = TextDim();
        colors[ImGuiCol_WindowBg]              = MainDashboardBg();
        colors[ImGuiCol_ChildBg]               = CardSurface();
        colors[ImGuiCol_PopupBg]               = CardSurface();
        colors[ImGuiCol_Border]                = BorderGlow();
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.90f);
        colors[ImGuiCol_FrameBgHovered]        = CardHover();
        colors[ImGuiCol_FrameBgActive]         = CardSurfaceAlt();
        colors[ImGuiCol_TitleBg]               = MainDashboardBg();
        colors[ImGuiCol_TitleBgActive]         = MainDashboardBg();
        colors[ImGuiCol_TitleBgCollapsed]      = MainDashboardBg();
        colors[ImGuiCol_MenuBarBg]             = MainDashboardBg();
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.04f, 0.04f, 0.04f, 0.50f);
        colors[ImGuiCol_ScrollbarGrab]         = BorderGlow();
        colors[ImGuiCol_ScrollbarGrabHovered]  = BorderLight();
        colors[ImGuiCol_ScrollbarGrabActive]   = EmeraldGreen();
        colors[ImGuiCol_CheckMark]             = EmeraldGreen();
        colors[ImGuiCol_SliderGrab]            = EmeraldGreen();
        colors[ImGuiCol_SliderGrabActive]      = ButtonGreen();
        colors[ImGuiCol_Button]                = CardSurfaceAlt();
        colors[ImGuiCol_ButtonHovered]         = CardHover();
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_Header]                = CardHover();
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_Separator]             = BorderGlow();
        colors[ImGuiCol_SeparatorHovered]      = BorderLight();
        colors[ImGuiCol_SeparatorActive]       = EmeraldGreen();
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_ResizeGripHovered]     = BorderLight();
        colors[ImGuiCol_ResizeGripActive]      = EmeraldGreen();
        colors[ImGuiCol_Tab]                   = CardSurface();
        colors[ImGuiCol_TabHovered]            = CardHover();
        colors[ImGuiCol_TabActive]             = CardSurfaceAlt();
        colors[ImGuiCol_TabUnfocused]          = CardSurface();
        colors[ImGuiCol_TabUnfocusedActive]    = CardSurface();
        colors[ImGuiCol_TableHeaderBg]         = CardSurfaceAlt();
        colors[ImGuiCol_TableBorderStrong]     = BorderGlow();
        colors[ImGuiCol_TableBorderLight]      = BorderGlow();
        colors[ImGuiCol_TableRowBg]            = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.05f, 0.05f, 0.05f, 0.40f);

        // ImPlot Styling
        if (ImPlot::GetCurrentContext()) {
            ImPlotStyle& plotStyle = ImPlot::GetStyle();
            plotStyle.PlotBorderSize = 0.0f;
            plotStyle.PlotPadding = ImVec2(8.0f, 8.0f);
            plotStyle.Colors[ImPlotCol_PlotBg] = CardSurface();
            plotStyle.Colors[ImPlotCol_PlotBorder] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            plotStyle.Colors[ImPlotCol_LegendBg] = CardSurfaceAlt();
            plotStyle.Colors[ImPlotCol_LegendBorder] = BorderGlow();
            plotStyle.Colors[ImPlotCol_LegendText] = CrystalWhite();
            plotStyle.Colors[ImPlotCol_TitleText] = CrystalWhite();
            plotStyle.Colors[ImPlotCol_InlayText] = TextDim();
            plotStyle.Colors[ImPlotCol_FrameBg] = CardSurface();
            plotStyle.Colors[ImPlotCol_Crosshairs] = EmeraldGreen();
        }
    }

    static void RenderCutePill(const char* label, const ImVec4& color, const char* icon = nullptr) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x, color.y, color.z, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(color.x, color.y, color.z, 0.50f));
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 3.0f));

        std::string badgeText;
        if (icon) badgeText += std::string(icon) + " ";
        badgeText += label;

        ImGui::Button(badgeText.c_str());

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
    }

    static void RenderHackerBadge(const char* label, const ImVec4& color, const char* status = nullptr) {
        RenderCutePill(label, color, status);
    }
};

} // namespace crypto
