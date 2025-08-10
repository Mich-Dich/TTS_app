#pragma once

#include <imgui.h>
#include <implot.h>

#include "util/system.h"
#include "util/io/serializer_data.h"
#include "util/io/config.h"


namespace AT::UI {

	#define LERP_GRAY(value)					{value, value, value, 1.f }
	#define IMCOLOR_GRAY(value)					ImColor{value, value, value, 255 }

	enum class window_pos;

	FORCEINLINE static u32 convert_color_to_int(const ImVec4& color) { return IM_COL32(255 * color.x, 255 * color.y, 255 * color.z, 255 * color.w); }

	enum class theme_selection : u8 {
		dark,
		light
	};

	[[maybe_unused]] static std::filesystem::path ini_file_location = AT::config::get_filepath_from_configtype_ini(util::get_executable_path(), config::file::imgui).c_str();
	[[maybe_unused]] static f32  m_font_size = 15.f, m_font_size_header_0 = 19.f, m_font_size_header_1 = 23.f, m_font_size_header_2 = 27.f, m_big_font_size = 18.f;
	[[maybe_unused]] static theme_selection UI_theme = theme_selection::dark;
	[[maybe_unused]] static bool window_border{};
	[[maybe_unused]] static ImVec4 highlited_window_bg = LERP_GRAY(0.57f);
	[[maybe_unused]] static f32 default_item_width{};

	GETTER_REF_FUNC(ImVec4, main_color);
	GETTER_REF_FUNC(ImVec4, main_titlebar_color);

	GETTER_REF_FUNC(ImVec4, action_color_00_faded);
	GETTER_REF_FUNC(ImVec4, action_color_00_weak);
	GETTER_REF_FUNC(ImVec4, action_color_00_default);
	GETTER_REF_FUNC(ImVec4, action_color_00_hover);
	GETTER_REF_FUNC(ImVec4, action_color_00_active);

	GETTER_REF_FUNC(ImVec4, default_gray);
	GETTER_REF_FUNC(ImVec4, default_gray_1);

	GETTER_REF_FUNC(ImVec4, action_color_gray_default);
	GETTER_REF_FUNC(ImVec4, action_color_gray_hover);
	GETTER_REF_FUNC(ImVec4, action_color_gray_active);

	void set_UI_theme_selection(theme_selection theme_selection);
	void enable_window_border(bool enable);
	void update_UI_theme();
	void update_UI_colors(ImVec4 new_color);

	class imgui_config {
	public:

		imgui_config();
		~imgui_config();

		DEFAULT_GETTER(ImGuiContext*, 		context_imgui)
		DEFAULT_GETTER(ImPlotContext*, 		context_implot)
		DEFAULT_GETTER_SETTER_ALL(bool, 	show_FPS_window);

		void serialize(AT::serializer::option option);
		ImFont* get_font(const std::string& name = "default");
		ImGuiID m_viewport_ID{};

	private:

		// ------------------------- general -------------------------
		ImGuiContext* 								m_context_imgui{};
		ImPlotContext* 								m_context_implot{};
		std::unordered_map<std::string, ImFont*> 	m_fonts{};

		// ------------------------- performance display -------------------------
		bool										m_show_FPS_window = true;
		f32											m_work_time = 0.f, m_sleep_time = 0.f;
		u32											m_target_fps = 0, m_current_fps = 0;
		bool										m_limit_fps = false;
		bool										fonts_need_recreation = false;
		UI::window_pos								FPS_window_location = static_cast<window_pos>(2);
	};
}
