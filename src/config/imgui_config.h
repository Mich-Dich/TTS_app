#pragma once

#include <imgui.h>
#include <implot.h>

#include "util/system.h"
#include "util/io/serializer_data.h"
#include "util/io/config.h"


namespace AT::UI {

	// Macros for common color manipulations.
	#define LERP_GRAY(value)					{value, value, value, 1.f }
	#define IMCOLOR_GRAY(value)					ImColor{value, value, value, 255 }

	enum class window_pos; // Forward declaration of window position enum.


	// Converts an ImVec4 color to a packed 32-bit integer representation.
	// @param color Input ImVec4 color.
	// @return Packed 32-bit RGBA color.
	FORCEINLINE static u32 convert_color_to_int(const ImVec4& color) {
		return IM_COL32(255 * color.x, 255 * color.y, 255 * color.z, 255 * color.w);
	}


	// Enum representing available UI theme options.
	enum class theme_selection : u8 {
		dark, // Dark theme.
		light // Light theme.
	};


	// -----------------------------------------------------------------------------------------------------------------------
	// Global ImGui configuration variables
	// -----------------------------------------------------------------------------------------------------------------------

	inline std::filesystem::path g_ini_file_location = AT::config::get_filepath_from_configtype_ini(util::get_executable_path(), config::file::imgui); // Path to the ImGui .ini file.
	inline f32 g_font_size = 15.f; 								// Default UI font size.
	inline f32 g_font_size_header_0 = 19.f; 					// Font size for small headers.
	inline f32 g_font_size_header_1 = 23.f; 					// Font size for medium headers.
	inline f32 g_font_size_header_2 = 27.f; 					// Font size for large headers.
	inline f32 g_big_font_size = 18.f; 							// Font size for emphasized text.
	inline theme_selection g_UI_theme = theme_selection::dark; 	// Currently selected UI theme.
	inline bool g_window_border = false; 						// Whether window borders are enabled.
	inline ImVec4 g_highlighted_window_bg; 						// Highlighted background color for selected windows.

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


	// Sets the current UI theme.
	// @param theme_selection New theme to apply.
	void set_UI_theme_selection(theme_selection theme_selection);


	// Enables or disables window borders globally.
	// @param enable Whether to enable borders.
	void enable_window_border(bool enable);


	// Applies the currently selected UI theme.
	void update_UI_theme();


	// Updates the main UI color and saves the change to config.
	// @param new_color New main color to apply.
	void update_UI_colors(ImVec4 new_color);




	// ImGui configuration class
	// Handles font management, context initialization, serialization,
	// and overall UI appearance settings.
	class imgui_config {
	public:

		imgui_config(); 	// Constructor: initializes ImGui and loads fonts.
		~imgui_config(); 	// Destructor: shuts down ImGui and saves settings.


		DEFAULT_GETTER(ImGuiContext*, 		context_imgui) 		// Returns pointer to the ImGui context.
		DEFAULT_GETTER(ImPlotContext*, 		context_implot) 	// Returns pointer to the ImPlot context.
		DEFAULT_GETTER_SETTER_ALL(bool, 	show_FPS_window); 	// Getter/setter for FPS window visibility.


		// Serializes or deserializes UI-related configuration.
		// @param option Choose between loading or saving settings.
		void serialize(AT::serializer::option option);


		// Resizes fonts based on a single base size.
		// @note !! IMPORTANT !! - Do not call during rendering. Call it during update
		// @param font_size New base font size.
		void resize_fonts(const f32 font_size);


		// Resizes fonts based on multiple custom sizes.
		// @note !! IMPORTANT !! - Do not call during rendering. Call it during update
		// @param font_size Base font size.
		// @param big_font_size Larger font size for emphasis.
		// @param font_size_header_0 Font size for header level 0.
		// @param font_size_header_1 Font size for header level 1.
		// @param font_size_header_2 Font size for header level 2.
		void resize_fonts(const f32 font_size, const f32 big_font_size, const f32 font_size_header_0, const f32 font_size_header_1, const f32 font_size_header_2);


		// Retrieves a font by name.
		// @param name Font identifier (e.g., "regular", "bold").
		// @return Pointer to the requested ImFont, or nullptr if not found.
		ImFont* get_font(const std::string& name = "default");


		ImGuiID m_viewport_ID{}; // ID of the current viewport.

	private:

		// Loads all fonts into ImGui based on configured sizes.
		void load_fonts();


		// ------------------------- General -------------------------
		ImGuiContext* 								m_context_imgui{}; 		// Pointer to the ImGui context.
		ImPlotContext* 								m_context_implot{}; 	// Pointer to the ImPlot context.
		std::unordered_map<std::string, ImFont*> 	m_fonts{}; 				// Loaded fonts mapped by name.


		// ------------------------- Performance display -------------------------
		bool 										m_show_FPS_window = true;							// Whether to display FPS overlay.
		f32 										m_work_time = 0.f, m_sleep_time = 0.f; 				// Timing metrics for performance analysis.
		u32 										m_target_fps = 0, m_current_fps = 0; 				// Target and current frames per second.
		bool 										m_limit_fps = false; 								// Whether FPS is capped.
		bool 										fonts_need_recreation = false; 						// Flag indicating if fonts should be recreated.
		UI::window_pos 								FPS_window_location = static_cast<window_pos>(2); 	// Default location of the FPS window.
	};
}
