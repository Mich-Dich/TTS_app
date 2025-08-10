
#include "util/pch.h"

#include <imgui.h>
#include <implot.h>

#include "util/system.h"
#include "util/io/serializer_yaml.h"
#include "util/ui/pannel_collection.h"
#include "util/ui/window_images.embed"

#include "application.h"
#include "render/renderer.h"
//#include "render/image.h"

#include "imgui_config.h"


#define UI_ACTIVE_THEME						AT::UI::THEME::current_theme

#define LERP_GRAY_A(value, alpha)			{value, value, value, alpha }
//#define COLOR_INT_CONVERTION(color)		IM_COL32(255 * color.x, 255 * color.y, 255 * color.z, 255 * color.w)
#define LERP_MAIN_COLOR_DARK(value)			{main_color.x * value, main_color.y * value, main_color.z * value, 1.f }		// Set [w] to be [1.f] to disable accidental transparency
#define LERP_MAIN_COLOR_LIGHT(value)		{	(1.f - value) * 1.f + value * main_color.x, \
												(1.f - value) * 1.f + value * main_color.y, \
												(1.f - value) * 1.f + value * main_color.z, \
												1.f }																		// Set [w] to be [1.f] to disable accidental transparency

namespace AT::UI {

	GETTER_REF_FUNC_IMPL(ImVec4, main_color);
	GETTER_REF_FUNC_IMPL(ImVec4, main_titlebar_color);

	GETTER_REF_FUNC_IMPL(ImVec4, action_color_00_faded);
	GETTER_REF_FUNC_IMPL(ImVec4, action_color_00_weak);
	GETTER_REF_FUNC_IMPL(ImVec4, action_color_00_default);
	GETTER_REF_FUNC_IMPL(ImVec4, action_color_00_hover);
	GETTER_REF_FUNC_IMPL(ImVec4, action_color_00_active);
			  
	GETTER_REF_FUNC_IMPL2(ImVec4, default_gray, IMCOLOR_GRAY(30));
	GETTER_REF_FUNC_IMPL2(ImVec4, default_gray_1, IMCOLOR_GRAY(35));
			  
	GETTER_REF_FUNC_IMPL2(ImVec4, action_color_gray_default, LERP_GRAY(0.2f));
	GETTER_REF_FUNC_IMPL2(ImVec4, action_color_gray_hover, LERP_GRAY(0.27f));
	GETTER_REF_FUNC_IMPL2(ImVec4, action_color_gray_active, LERP_GRAY(0.35f));

	static ImVec4 vector_multi(const ImVec4& vec_0, const ImVec4& vec_1) {
		return ImVec4{ vec_0.x * vec_1.x, vec_0.y * vec_1.y, vec_0.z * vec_1.z, vec_0.w * vec_1.w };
	}

	void set_UI_theme_selection(theme_selection theme_selection) { UI_theme = theme_selection; }

	//void save_UI_theme_data() {

	//	serialize(serializer::option::save_to_file);
	//	update_UI_theme();
	//}

	//void load_UI_data() {

	//	serialize(serializer::option::load_from_file);
	//	update_UI_theme();
	//}


	imgui_config::imgui_config() { 
		
		LOG_INIT 
		
		IMGUI_CHECKVERSION();
		m_context_imgui = ImGui::CreateContext();
		m_context_implot = ImPlot::CreateContext();
		application::get().get_renderer()->imgui_init();
		

		main_color = { .0f,	.4088f,	1.0f,	1.f };
		window_border = false;
		highlited_window_bg = LERP_GRAY(0.57f);
		default_item_width = 200.f;

		serialize(serializer::option::load_from_file);

		main_titlebar_color = LERP_MAIN_COLOR_DARK(.5f);			// lerp after loading main color
		action_color_00_faded = LERP_MAIN_COLOR_DARK(0.5f);
		action_color_00_weak = LERP_MAIN_COLOR_DARK(0.6f);
		action_color_00_default = LERP_MAIN_COLOR_DARK(0.7f);
		action_color_00_hover = LERP_MAIN_COLOR_DARK(0.85f);
		action_color_00_active = LERP_MAIN_COLOR_DARK(1.f);
	
		{	// Load fonts
			std::filesystem::path base_path = AT::util::get_executable_path() / "assets" / "fonts";
			std::filesystem::path OpenSans_path = base_path / "Open_Sans" / "static";
			std::filesystem::path Inconsolata_path = base_path / "Inconsolata" / "static";

			auto io = ImGui::GetIO();
						
			io.FontAllowUserScaling = true;
			m_fonts["regular"] =		io.Fonts->AddFontFromFileTTF((OpenSans_path/ "OpenSans-Regular.ttf").string().c_str(), m_font_size);
			m_fonts["bold"] =			io.Fonts->AddFontFromFileTTF((OpenSans_path/ "OpenSans-Bold.ttf").string().c_str(), m_font_size);
			m_fonts["italic"] =			io.Fonts->AddFontFromFileTTF((OpenSans_path/ "OpenSans-Italic.ttf").string().c_str(), m_font_size);

			m_fonts["regular_big"] =	io.Fonts->AddFontFromFileTTF((OpenSans_path / "OpenSans-Regular.ttf").string().c_str(), m_big_font_size);
			m_fonts["bold_big"] =		io.Fonts->AddFontFromFileTTF((OpenSans_path / "OpenSans-Bold.ttf").string().c_str(), m_big_font_size);
			m_fonts["italic_big"] =		io.Fonts->AddFontFromFileTTF((OpenSans_path / "OpenSans-Italic.ttf").string().c_str(), m_big_font_size);

			m_fonts["header_0"] =		io.Fonts->AddFontFromFileTTF((OpenSans_path / "OpenSans-Regular.ttf").string().c_str(), m_font_size_header_2);
			m_fonts["header_1"] =		io.Fonts->AddFontFromFileTTF((OpenSans_path / "OpenSans-Regular.ttf").string().c_str(), m_font_size_header_1);
			m_fonts["header_2"] =		io.Fonts->AddFontFromFileTTF((OpenSans_path / "OpenSans-Regular.ttf").string().c_str(), m_font_size_header_0);

			m_fonts["giant"] =			io.Fonts->AddFontFromFileTTF((OpenSans_path / "OpenSans-Bold.ttf").string().c_str(), 30.f);

			//Inconsolata-Regular
			m_fonts["monospace_regular"] = io.Fonts->AddFontFromFileTTF((Inconsolata_path / "Inconsolata-Regular.ttf").string().c_str(), m_font_size * 0.92f);
			m_fonts["monospace_regular_big"] = io.Fonts->AddFontFromFileTTF((Inconsolata_path / "Inconsolata-Regular.ttf").string().c_str(), m_big_font_size * 1.92f);

			io.FontDefault = m_fonts["regular"];

			application::get().get_renderer()->imgui_create_fonts();
		}

		update_UI_theme();
	}


	imgui_config::~imgui_config() { 

		LOG_SHUTDOWN
				
		application::get().get_renderer()->imgui_shutdown();
		ImGui::DestroyContext(m_context_imgui);
		ImPlot::DestroyContext(m_context_implot);

		serialize(serializer::option::save_to_file);
	}


	ImFont* imgui_config::get_font(const std::string& name) {

		for (auto loc_font : m_fonts)
			if (loc_font.first == name)
				return loc_font.second;
	
		return nullptr;
	}


	void imgui_config::serialize(serializer::option option) {

		AT::serializer::yaml(config::get_filepath_from_configtype(util::get_executable_path(), config::file::ui), "theme", option)			// load general aperance settings
			.entry(KEY_VALUE(m_font_size))
			.entry(KEY_VALUE(m_font_size_header_0))
			.entry(KEY_VALUE(m_font_size_header_1))
			.entry(KEY_VALUE(m_font_size_header_2))
			.entry(KEY_VALUE(m_big_font_size))
			.entry(KEY_VALUE(UI_theme))
			.entry(KEY_VALUE(window_border))
			.entry(KEY_VALUE(highlited_window_bg))
			.entry(KEY_VALUE(default_item_width))

			// color
			.entry(KEY_VALUE(main_color))
			.entry(KEY_VALUE(main_titlebar_color))

			// gray
			.entry(KEY_VALUE(action_color_gray_default))
			.entry(KEY_VALUE(action_color_gray_hover))
			.entry(KEY_VALUE(action_color_gray_active));
	}

	// --------------------------------------------------------------------------------------------------------------------------------------------

	void update_UI_colors(ImVec4 new_color) {

		main_color = new_color;
		application::get().get_imgui_config_ref()->serialize(serializer::option::save_to_file);
		update_UI_theme();
	}


	void enable_window_border(bool enable) {

		window_border = enable;
		//serialize(serializer::option::save_to_file);

		ImGuiStyle* style = &ImGui::GetStyle();
		style->WindowBorderSize = enable ? 1.f : 0.f;
	}
	

	void update_UI_theme() {
		
		//LOG(Debug, "updating UI theme");

		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		// main sizes
		style->WindowPadding = ImVec2(10.f, 4.f);
		style->FramePadding = ImVec2(4.f, 4.f);
		style->CellPadding = ImVec2(4.f, 4.f);
		style->ItemSpacing = ImVec2(4.f, 4.f);
		style->ItemInnerSpacing = ImVec2(4.f, 4.f);
		style->TouchExtraPadding = ImVec2(4.f, 4.f);
		style->IndentSpacing = 10.f;
		style->ScrollbarSize = 14.f;
		style->GrabMinSize = 14.f;
		style->WindowMenuButtonPosition = ImGuiDir_Right;

		// border
		style->WindowBorderSize = 1.0f;
		style->ChildBorderSize = 0.0f;
		style->PopupBorderSize = 0.0f;
		style->FrameBorderSize = 0.0f;
		style->TabBorderSize = 0.0f;
		style->TabBarBorderSize = 0.0f;

		// padding
		style->WindowRounding = 2.f;
		style->ChildRounding = 2.f;
		style->FrameRounding = 2.f;
		style->PopupRounding = 2.f;
		style->ScrollbarRounding = 2.f;
		style->GrabRounding = 2.f;
		style->TabRounding = 2.f;

		/*
		Alpha;                      // Global alpha applies to everything in Dear ImGui.
		float       DisabledAlpha;              // Additional alpha multiplier applied by BeginDisabled(). Multiply over current value of Alpha.
		ImVec2      WindowPadding;              // Padding within a window.
		float       WindowRounding;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows. Large values tend to lead to variety of artifacts and are not recommended.
		float       WindowBorderSize;           // Thickness of border around windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).
		ImVec2      WindowMinSize;              // Minimum window size. This is a global setting. If you want to constrain individual windows, use SetNextWindowSizeConstraints().
		ImVec2      WindowTitleAlign;           // Alignment for title bar text. Defaults to (0.0f,0.5f) for left-aligned,vertically centered.
		ImGuiDir    WindowMenuButtonPosition;   // Side of the collapsing/docking button in the title bar (None/Left/Right). Defaults to ImGuiDir_Left.
		float       ChildRounding;              // Radius of child window corners rounding. Set to 0.0f to have rectangular windows.
		float       ChildBorderSize;            // Thickness of border around child windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).
		float       PopupRounding;              // Radius of popup window corners rounding. (Note that tooltip windows use WindowRounding)
		float       PopupBorderSize;            // Thickness of border around popup/tooltip windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).
		ImVec2      FramePadding;               // Padding within a framed rectangle (used by most widgets).
		float       FrameRounding;              // Radius of frame corners rounding. Set to 0.0f to have rectangular frame (used by most widgets).
		float       FrameBorderSize;            // Thickness of border around frames. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly).
		ImVec2      ItemSpacing;                // Horizontal and vertical spacing between widgets/lines.
		ImVec2      ItemInnerSpacing;           // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label).
		ImVec2      CellPadding;                // Padding within a table cell. Cellpadding.x is locked for entire table. CellPadding.y may be altered between different rows.
		ImVec2      TouchExtraPadding;          // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
		float       IndentSpacing;              // Horizontal indentation when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
		float       ColumnsMinSpacing;          // Minimum horizontal spacing between two columns. Preferably > (FramePadding.x + 1).
		float       ScrollbarSize;              // Width of the vertical scrollbar, Height of the horizontal scrollbar.
		float       ScrollbarRounding;          // Radius of grab corners for scrollbar.
		float       GrabMinSize;                // Minimum width/height of a grab box for slider/scrollbar.
		float       GrabRounding;               // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
		float       LogSliderDeadzone;          // The size in pixels of the dead-zone around zero on logarithmic sliders that cross zero.
		float       TabRounding;                // Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs.
		float       TabBorderSize;              // Thickness of border around tabs.
		float       TabMinWidthForCloseButton;  // Minimum width for close button to appear on an unselected tab when hovered. Set to 0.0f to always show when hovering, set to FLT_MAX to never show close button unless selected.
		float       TabBarBorderSize;           // Thickness of tab-bar separator, which takes on the tab active color to denote focus.
		float       TableAngledHeadersAngle;    // Angle of angled headers (supported values range from -50.0f degrees to +50.0f degrees).
		ImGuiDir    ColorButtonPosition;        // Side of the color button in the ColorEdit4 widget (left/right). Defaults to ImGuiDir_Right.
		ImVec2      ButtonTextAlign;            // Alignment of button text when button is larger than text. Defaults to (0.5f, 0.5f) (centered).
		ImVec2      SelectableTextAlign;        // Alignment of selectable text. Defaults to (0.0f, 0.0f) (top-left aligned). It's generally important to keep this left-aligned if you want to lay multiple items on a same line.
		float       SeparatorTextBorderSize;    // Thickkness of border in SeparatorText()
		ImVec2      SeparatorTextAlign;         // Alignment of text within the separator. Defaults to (0.0f, 0.5f) (left aligned, center).
		ImVec2      SeparatorTextPadding;       // Horizontal offset of text from each edge of the separator + spacing on other axis. Generally small values. .y is recommended to be == FramePadding.y.
		ImVec2      DisplayWindowPadding;       // Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows.
		ImVec2      DisplaySafeAreaPadding;     // If you cannot see the edges of your screen (e.g. on a TV) increase the safe area padding. Apply to popups/tooltips as well regular windows. NB: Prefer configuring your TV sets correctly!
		float       DockingSeparatorSize;       // Thickness of resizing border between docked windows
		float       MouseCursorScale;           // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). We apply per-monitor DPI scaling over this scale. May be removed later.
		bool        AntiAliasedLines;           // Enable anti-aliased lines/borders. Disable if you are really tight on CPU/GPU. Latched at the beginning of the frame (copied to ImDrawList).
		bool        AntiAliasedLinesUseTex;     // Enable anti-aliased lines/borders using textures where possible. Require backend to render with bilinear filtering (NOT point/nearest filtering). Latched at the beginning of the frame (copied to ImDrawList).
		bool        AntiAliasedFill;            // Enable anti-aliased edges around filled shapes (rounded rectangles, circles, etc.). Disable if you are really tight on CPU/GPU. Latched at the beginning of the frame (copied to ImDrawList).
		float       CurveTessellationTol;       // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
		float       CircleTessellationMaxError; // Maximum error (in pixels) allowed when using AddCircle()/AddCircleFilled() or drawing rounded corner rectangles with no explicit segment count specified. Decrease for higher quality but more geometry.
		*/
		
		switch (UI_theme) {

			case AT::UI::theme_selection::dark: {

				action_color_00_faded = LERP_MAIN_COLOR_DARK(0.5f);
				action_color_00_weak = LERP_MAIN_COLOR_DARK(0.6f);
				action_color_00_default = LERP_MAIN_COLOR_DARK(0.7f);
				action_color_00_hover = LERP_MAIN_COLOR_DARK(0.85f);
				action_color_00_active = LERP_MAIN_COLOR_DARK(0.92f);

				action_color_gray_default = LERP_GRAY(0.15f);
				action_color_gray_hover = LERP_GRAY(0.2f);
				action_color_gray_active = LERP_GRAY(0.25f);

				highlited_window_bg = LERP_GRAY(0.57f);
				main_titlebar_color = LERP_MAIN_COLOR_DARK(.5f);

				colors[ImGuiCol_Text]					= IMCOLOR_GRAY(255);
				colors[ImGuiCol_TextDisabled]			= IMCOLOR_GRAY(180);
				colors[ImGuiCol_WindowBg]				= default_gray;
				colors[ImGuiCol_ChildBg]				= default_gray;
				colors[ImGuiCol_PopupBg]				= IMCOLOR_GRAY(20);
				colors[ImGuiCol_Border]					= LERP_GRAY_A(.43f, .5f);
				colors[ImGuiCol_BorderShadow]			= LERP_GRAY_A(.12f, .5f);
				colors[ImGuiCol_FrameBg]				= LERP_GRAY_A(.06f, .54f);
				colors[ImGuiCol_FrameBgHovered]			= LERP_GRAY_A(.19f, .4f);
				colors[ImGuiCol_FrameBgActive]			= LERP_GRAY_A(.3f, .67f);
				colors[ImGuiCol_TitleBg]				= IMCOLOR_GRAY(22);
				colors[ImGuiCol_TitleBgActive]			= IMCOLOR_GRAY(22);
				colors[ImGuiCol_TitleBgCollapsed]		= IMCOLOR_GRAY(22);
				colors[ImGuiCol_MenuBarBg]				= LERP_GRAY(.1f);
				colors[ImGuiCol_ScrollbarBg]			= LERP_GRAY(0.23f);
				colors[ImGuiCol_ScrollbarGrab]			= action_color_00_default;
				colors[ImGuiCol_ScrollbarGrabHovered]	= action_color_00_hover;
				colors[ImGuiCol_ScrollbarGrabActive]	= action_color_00_active;
				colors[ImGuiCol_CheckMark]				= action_color_00_active;
				colors[ImGuiCol_SliderGrab]				= action_color_00_default;
				colors[ImGuiCol_SliderGrabActive]		= action_color_00_active;
				colors[ImGuiCol_Button]					= action_color_00_default;
				colors[ImGuiCol_ButtonHovered]			= action_color_00_hover;
				colors[ImGuiCol_ButtonActive]			= action_color_00_active;
				colors[ImGuiCol_Header]					= LERP_GRAY(.3f);
				colors[ImGuiCol_HeaderHovered]			= LERP_GRAY(.4f);
				colors[ImGuiCol_HeaderActive]			= LERP_GRAY(.5f);
				colors[ImGuiCol_Separator]				= LERP_GRAY(.45f);
				colors[ImGuiCol_SeparatorHovered]		= LERP_GRAY(.45f);
				colors[ImGuiCol_SeparatorActive]		= LERP_GRAY(.45f);
				colors[ImGuiCol_ResizeGrip]				= action_color_00_default;
				colors[ImGuiCol_ResizeGripHovered]		= action_color_00_hover;
				colors[ImGuiCol_ResizeGripActive]		= action_color_00_active;
				colors[ImGuiCol_Tab]					= LERP_MAIN_COLOR_DARK(0.4f);
				colors[ImGuiCol_TabHovered]				= LERP_MAIN_COLOR_DARK(0.5f);
				colors[ImGuiCol_TabActive]				= LERP_MAIN_COLOR_DARK(0.6f);
				colors[ImGuiCol_TabUnfocused]			= LERP_MAIN_COLOR_DARK(0.5f);
				colors[ImGuiCol_TabUnfocusedActive]		= LERP_MAIN_COLOR_DARK(0.6f);
				colors[ImGuiCol_DockingPreview]			= action_color_00_active;
				colors[ImGuiCol_DockingEmptyBg]			= LERP_GRAY(0.2f);
				colors[ImGuiCol_PlotLines]				= ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
				colors[ImGuiCol_PlotLinesHovered]		= ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
				colors[ImGuiCol_PlotHistogram]			= LERP_MAIN_COLOR_DARK(.75f);
				colors[ImGuiCol_PlotHistogramHovered]	= action_color_00_active;
				colors[ImGuiCol_TableHeaderBg]			= ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
				colors[ImGuiCol_TableBorderStrong]		= ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
				colors[ImGuiCol_TableBorderLight]		= ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
				colors[ImGuiCol_TableRowBg]				= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_TableRowBgAlt]			= ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
				colors[ImGuiCol_TextSelectedBg]			= LERP_MAIN_COLOR_DARK(.4f);
				colors[ImGuiCol_DragDropTarget]			= LERP_MAIN_COLOR_DARK(.6f);
				colors[ImGuiCol_NavHighlight]			= ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
				colors[ImGuiCol_NavWindowingHighlight]	= ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg]		= ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
				colors[ImGuiCol_ModalWindowDimBg]		= ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

			} break;

			case AT::UI::theme_selection::light: {

				action_color_00_default = LERP_MAIN_COLOR_LIGHT(0.7f);
				action_color_00_hover = LERP_MAIN_COLOR_LIGHT(0.85f);
				action_color_00_active = LERP_MAIN_COLOR_LIGHT(1.f);

				action_color_gray_default = LERP_GRAY(0.2f);
				action_color_gray_hover = LERP_GRAY(0.27f);
				action_color_gray_active = LERP_GRAY(0.35f);

				highlited_window_bg = LERP_GRAY(0.8f);
				main_titlebar_color = LERP_MAIN_COLOR_LIGHT(.5f);

				colors[ImGuiCol_Text]					= LERP_GRAY(1.0f);
				colors[ImGuiCol_TextDisabled]			= LERP_GRAY(.7f);
				colors[ImGuiCol_WindowBg]				= LERP_GRAY(.25f);
				colors[ImGuiCol_ChildBg]				= LERP_GRAY(.25f);
				colors[ImGuiCol_PopupBg]				= LERP_GRAY(.25f);
				colors[ImGuiCol_Border]					= LERP_GRAY_A(0.2f, .50f);
				colors[ImGuiCol_BorderShadow]			= LERP_GRAY(.12f);
				colors[ImGuiCol_FrameBg]				= LERP_GRAY_A(.75f, .75f);
				colors[ImGuiCol_FrameBgHovered]			= LERP_GRAY_A(.70f, .75f);
				colors[ImGuiCol_FrameBgActive]			= LERP_GRAY_A(.65f, .75f);
				colors[ImGuiCol_TitleBg]				= action_color_00_default;
				colors[ImGuiCol_TitleBgActive]			= action_color_00_active;
				colors[ImGuiCol_TitleBgCollapsed]		= action_color_00_default;
				colors[ImGuiCol_MenuBarBg]				= LERP_GRAY(.58f);
				colors[ImGuiCol_ScrollbarBg]			= LERP_GRAY(.75f);
				colors[ImGuiCol_ScrollbarGrab]			= action_color_00_default;
				colors[ImGuiCol_ScrollbarGrabHovered]	= action_color_00_hover;
				colors[ImGuiCol_ScrollbarGrabActive]	= action_color_00_active;
				colors[ImGuiCol_CheckMark]				= action_color_00_active;
				colors[ImGuiCol_SliderGrab]				= action_color_00_default;
				colors[ImGuiCol_SliderGrabActive]		= action_color_00_active;
				colors[ImGuiCol_Button]					= action_color_00_default;
				colors[ImGuiCol_ButtonHovered]			= action_color_00_hover;
				colors[ImGuiCol_ButtonActive]			= action_color_00_active;
				colors[ImGuiCol_Header]					= LERP_GRAY(.75f);
				colors[ImGuiCol_HeaderHovered]			= LERP_GRAY(.7f);
				colors[ImGuiCol_HeaderActive]			= LERP_GRAY(.65f);
				colors[ImGuiCol_Separator]				= LERP_GRAY(.45f);
				colors[ImGuiCol_SeparatorHovered]		= LERP_GRAY(.45f);
				colors[ImGuiCol_SeparatorActive]		= LERP_GRAY(.45f);
				colors[ImGuiCol_ResizeGrip]				= action_color_00_default;
				colors[ImGuiCol_ResizeGripHovered]		= action_color_00_hover;
				colors[ImGuiCol_ResizeGripActive]		= action_color_00_active;
				colors[ImGuiCol_Tab]					= action_color_00_default;
				colors[ImGuiCol_TabHovered]				= action_color_00_hover;
				colors[ImGuiCol_TabActive]				= action_color_00_active;
				colors[ImGuiCol_TabUnfocused]			= LERP_MAIN_COLOR_LIGHT(0.5f);
				colors[ImGuiCol_TabUnfocusedActive]		= LERP_MAIN_COLOR_LIGHT(0.6f);
				colors[ImGuiCol_DockingPreview]			= action_color_00_active;
				colors[ImGuiCol_DockingEmptyBg]			= LERP_GRAY(.2f);
				colors[ImGuiCol_PlotLines]				= ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
				colors[ImGuiCol_PlotLinesHovered]		= ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
				colors[ImGuiCol_PlotHistogram]			= LERP_MAIN_COLOR_LIGHT(.75f);
				colors[ImGuiCol_PlotHistogramHovered]	= action_color_00_active;
				colors[ImGuiCol_TableHeaderBg]			= ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
				colors[ImGuiCol_TableBorderStrong]		= ImVec4(0.31f, 0.31f, 0.35f, 1.00f);	// Prefer using Alpha=1.0 here
				colors[ImGuiCol_TableBorderLight]		= ImVec4(0.23f, 0.23f, 0.25f, 1.00f);		// Prefer using Alpha=1.0 here
				colors[ImGuiCol_TableRowBg]				= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_TableRowBgAlt]			= ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
				colors[ImGuiCol_TextSelectedBg]			= LERP_MAIN_COLOR_LIGHT(.4f);
				colors[ImGuiCol_DragDropTarget]			= ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
				colors[ImGuiCol_NavHighlight]			= ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
				colors[ImGuiCol_NavWindowingHighlight]	= ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg]		= ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
				colors[ImGuiCol_ModalWindowDimBg]		= ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

			} break;

			default:
				break;
		}

	}

}
