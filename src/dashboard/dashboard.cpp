
#include "util/pch.h"

#include <Python.h>
#ifdef PLATFORM_LINUX
    #include <sys/wait.h>   // For waitpid()
    #include <unistd.h>     // For fork(), execvp()
#endif

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "events/event.h"
#include "events/application_event.h"
#include "events/mouse_event.h"
#include "events/key_event.h"
#include "util/ui/pannel_collection.h"
#include "util/io/serializer_data.h"
#include "util/io/serializer_yaml.h"
#include "util/system.h"
#include "config/imgui_config.h"
#include "application.h"

#include "dashboard.h"


namespace AT {

#ifdef _WIN32
    #include <Windows.h>
    #pragma comment(lib, "winmm.lib")
#endif

    dashboard::dashboard()
        : m_is_generating(false) {

    #if defined(PLATFORM_LINUX)
        util::init_qt();
    #endif

		const std::filesystem::path icon_path = util::get_executable_path() / ASSET_DIR / "images";
#define LOAD_ICON(name)			m_##name##_icon = create_ref<image>(icon_path / #name ".png", image_format::RGBA)
		LOAD_ICON(generate);
		LOAD_ICON(audio);
		LOAD_ICON(stop);
		LOAD_ICON(settings);
		LOAD_ICON(library);
#undef	LOAD_ICON
    }
    
    
    dashboard::~dashboard() {

    #if defined(PLATFORM_LINUX)
        util::shutdown_qt();
    #endif

        m_generate_icon.reset();
        m_audio_icon.reset();
        m_stop_icon.reset();
        m_settings_icon.reset();
        m_library_icon.reset();
    }



    // init will be called when every system is initalized
    bool dashboard::init() {

        // Get the correct path to setup script
        const auto script_dir = util::get_executable_path() / "kokoro";
        const auto setup_script = script_dir / "setup_venv.sh";
        VALIDATE(std::filesystem::exists(setup_script), return false, "", "setup_venv.sh not found in [" << script_dir << "]")
        
        // Make script executable
        std::filesystem::permissions(setup_script, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);
        
        // Run setup script using the virtual environment's Python
        const auto venv_python = (script_dir / "venv" / "bin" / "python").string();
        const auto command = venv_python + " -m pip --version > /dev/null 2>&1";
        int result = std::system(command.c_str());
        if (result != 0) {
            // If venv pip not available, run the setup script normally
            const auto bash_command = "bash " + setup_script.string();
            result = std::system(bash_command.c_str());
            VALIDATE(result == 0, return false, "", "Failed to setup virtual environment (exit code: " << result << ")")
        }
        
        std::filesystem::create_directories(util::get_executable_path() / "audio");
        serialize(serializer::option::load_from_file);
        m_last_save_time = util::get_system_time();
        m_font_size = AT::UI::g_font_size;

        if (m_auto_open_last && !m_current_project.empty() && m_project_paths.contains(m_current_project)) {

            const auto project_to_load = m_project_paths.at(m_current_project);
            load_project(m_current_project, project_to_load);
            m_sidebar_status = sidebar_status::menu;
        } else
            m_sidebar_status = sidebar_status::project_manager;

        return true;
    }


    void dashboard::finalize_init() {

        initialize_python();
    }

    // shutdown will be called before any system is deinitialize
    bool dashboard::shutdown() {

        // save all relevant data
        serialize(serializer::option::save_to_file);
        AT::UI::g_font_size = m_font_size;
        application::get().get_imgui_config_ref()->serialize(serializer::option::save_to_file);

        // Acquire GIL if Python is initialized
        if (Py_IsInitialized())
            PyGILState_Ensure();

        if (m_worker_running) {                                         // Signal worker to stop

            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                m_generation_queue = {};
            }
            
            if (m_generation_future.valid())                            // Wait for worker to finish
                m_generation_future.wait_for(std::chrono::seconds(1));
        }
        
        m_shutting_down = true;
        if (m_is_generating) {

            // Wait for a reasonable time, then detach if still running
            if (m_generation_future.wait_for(std::chrono::seconds(1)) != std::future_status::ready) {
                LOG(Warn, "TTS generation still running, detaching thread")
                m_generation_future = {}; // Release the future (will detach the thread)
            }
        }
        
        finalize_python();
        return true;
    }


    void dashboard::on_crash() {
        
        LOG(Error, "Crash occurred, saving")
        serialize(serializer::option::save_to_file);

        // Save projects that have a path registered
        for (auto& proj : m_open_projects) {

            LOG(Trace, "project: " << proj.name)
            std::filesystem::path project_path{};
            if (!m_project_paths.contains(proj.name))
                continue;

            project_path = m_project_paths.at(proj.name);
            LOG(Trace, "saving project [" << proj.name << "] to [" << project_path.generic_string() << "]")
            serialize_project(proj, project_path, serializer::option::save_to_file);
        }
        LOG(Trace, "Done saving")
    }


    void dashboard::update(f32 delta_time)  {

        if (m_last_save_time.is_older_than(util::get_system_time(), m_save_interval_sec)) {

            LOG(Trace, "Auto saving")

            save_open_projects();
            m_last_save_time = util::get_system_time();
        }
    }


    void dashboard::on_event(event& event)  {
        
        //  ignore if no proj open      is keyboard event
        if (!m_open_projects.empty() && event.get_category_flag() & EC_Keyboard) {

            auto& key_event = static_cast<AT::key_event&>(event);
            
            // Check for Control key press/release
            if (key_event.get_keycode() == key_code::key_left_control || 
                key_event.get_keycode() == key_code::key_right_control) {
                
                if (key_event.m_key_state == key_state::press)
                    m_control_key_pressed = true;
                else if (key_event.m_key_state == key_state::release)
                    m_control_key_pressed = false;
            }

            // Check for S key with Control pressed
            if (key_event.get_keycode() == key_code::key_S && key_event.m_key_state == key_state::press && m_control_key_pressed) {
                
                LOG(Info, "Ctrl+S pressed - saving project")
                save_open_projects();
                event.handled = true;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------
    // UI
    // --------------------------------------------------------------------------------------------------------------

    void dashboard::draw(f32 delta_time) {

        auto viewport = ImGui::GetMainViewport();
        
        // Create a fullscreen window without decorations
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("main_window", nullptr, flags);
        
        draw_sidebar();
        
        ImGui::BeginChild("right_panel", ImVec2(0, 0), true);
        if (ImGui::BeginTabBar("projects_tab_bar", ImGuiTabBarFlags_None)) {                        // Create tab bar for projects
            for (auto& proj : m_open_projects) {
                if (ImGui::BeginTabItem(proj.name.c_str(), nullptr, proj.saved ? ImGuiTabItemFlags_None : ImGuiTabItemFlags_UnsavedDocument)) {                       // Create a tab for each project

                    ImGui::BeginChild("current_project", ImVec2(0, 0), true);
                    draw_project(proj);
                    ImGui::EndChild();
                    ImGui::EndTabItem();

                    if (m_current_project != proj.name)
                        m_current_project = proj.name;
                }
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        ImGui::End();
    }


    void dashboard::draw_init_UI(f32 delta_time) {

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        
        ImGui::Begin("Initialization", nullptr, window_flags);
        {

            ImGui::PushFont(application::get().get_imgui_config_ref()->get_font("giant"));
            const char* text = "Initializing...";
            const float target_font_size = 50.0f;
            ImVec2 base_text_size = ImGui::CalcTextSize(text);                                      // Calculate base text size at default font scale
            float scale = (base_text_size.y > 0) ? target_font_size / base_text_size.y : 1.0f;      // Calculate required scale to reach target font size
            ImVec2 available = ImGui::GetContentRegionAvail() * 0.9f;                               // Get available space with 10% margin
            ImVec2 scaled_size = base_text_size * scale;                                            // Calculate scaled text size
            
            if (scaled_size.x > available.x || scaled_size.y > available.y) {                       // Adjust scale if needed to fit available space
                float width_ratio = available.x / scaled_size.x;
                float height_ratio = available.y / scaled_size.y;
                scale *= (width_ratio < height_ratio) ? width_ratio : height_ratio;
            }
            
            // Set font scale and calculate final position
            ImGui::SetWindowFontScale(scale);
            ImVec2 text_size = ImGui::CalcTextSize(text);
            ImVec2 position = (ImGui::GetContentRegionAvail() - text_size) * 0.5f;
            
            ImGui::SetCursorPos(position);
            ImGui::TextUnformatted(text);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopFont();

            UI::shift_cursor_pos((ImGui::GetContentRegionAvail().x / 2) - 30, 30);
            UI::loading_indicator_circle("##loading_indicator", 30, 13, 5);

        }
        ImGui::End();
    }


    void dashboard::draw_sidebar() {

        // Get available content region
        const ImVec2 content_size = ImGui::GetContentRegionAvail();
        const float icon_size = 30.0f;

    #define SECTION_HEADER(button, section_title)                UI::shift_cursor_pos(0.f, 5.f);                                                \
                ImGui::Image(button->get(), ImVec2(icon_size, icon_size), ImVec2(0, 0), ImVec2(1, 1));                                          \
                ImGui::SameLine(); UI::shift_cursor_pos(10.f, 5.f); UI::big_text(section_title); UI::shift_cursor_pos(0.f, 20.f);
        
        switch (m_sidebar_status) {

            case sidebar_status::menu: {

                const f32 sidebar_width = 40.f;
                const f32 padding_x = (sidebar_width - icon_size - 10) / 2;
                const f32 content_width = sidebar_width - (2 * padding_x);
                const ImVec2 button_dims(content_width, content_width);
                auto draw_sidebar_button = [&](const char* label, sidebar_status section, ref<image> icon) {

            	    ImGui::PushStyleColor(ImGuiCol_Button, UI::get_default_gray_ref());
            	    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::get_action_color_gray_hover_ref());
            	    ImGui::PushStyleColor(ImGuiCol_ButtonActive, UI::get_action_color_gray_active_ref());
            
            	    if (ImGui::Button(label, button_dims))
            	    	m_sidebar_status = section;
            
            	    ImVec2 button_min = ImGui::GetItemRectMin();
            	    ImVec2 button_max = ImGui::GetItemRectMax();
            	    ImVec2 button_center = ImVec2((button_min.x + button_max.x) * 0.5f, (button_min.y + button_max.y) * 0.5f);
            
            	    if (icon) {				// Draw icon centered horizontally, near top
            	    	ImVec2 icon_pos(button_center.x - icon_size * 0.5f, button_min.y + button_dims.y * 0.15f);
            	    	ImGui::SetCursorScreenPos(icon_pos);
            	    	ImGui::Image(icon->get(), ImVec2(icon_size, icon_size), ImVec2(0, 0), ImVec2(1, 1));
            	    }
            
            	    ImGui::PopStyleColor(3);
                };

                ImGui::BeginChild("LeftPanel", ImVec2(sidebar_width, content_size.y), true);
                
		        UI::shift_cursor_pos(0, 10);
                draw_sidebar_button("##settings", sidebar_status::settings, m_settings_icon);
		        
                UI::shift_cursor_pos(0, 10);
                draw_sidebar_button("##project_manager", sidebar_status::project_manager, m_library_icon);
                
            } break;

            case sidebar_status::settings: {

                const f32 sidebar_width = math::min(230.f + (AT::UI::g_font_size-10) * 10, content_size.x * 0.3f);
                ImGui::BeginChild("LeftPanel", ImVec2(sidebar_width, content_size.y), true);
                
                // Header section
                SECTION_HEADER(m_settings_icon, "Kokoro Settings");
                
                auto draw_title = [](const char* text) {

                    UI::shift_cursor_pos(0.f, 20.f);
                    ImGui::PushFont(application::get().get_imgui_config_ref()->get_font("bold"));
                    ImGui::TextColored( AT::UI::get_main_color_ref(), text);
                    ImGui::PopFont();
                    ImGui::Separator();
                };

                // Voice Settings section
                draw_title("VOICE SETTINGS");
                
                UI::begin_table("settings", false);
                UI::table_row_slider("Voice Speed", m_voice_speed, .5f, 2.f, .05f);
                UI::table_row([]() { 
                    ImGui::Text("Voice Type"); 
                    UI::help_marker("Select the voice model for text-to-speech generation");
                }, [&]() {
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (ImGui::BeginCombo("##Voice Type", m_voice, ImGuiComboFlags_HeightLarge)) {
                        // Group voices by type for better organization
                        ImGui::TextDisabled("Female Voices");
                        ImGui::Separator();
                        #define SELECTABLE(voice) if (ImGui::Selectable(voice)) m_voice = voice;
                        SELECTABLE("af_alloy") SELECTABLE("af_aoede") SELECTABLE("af_bella")
                        SELECTABLE("af_heart") SELECTABLE("af_jessica") SELECTABLE("af_kore")
                        SELECTABLE("af_nicole") SELECTABLE("af_nova") SELECTABLE("af_river")
                        SELECTABLE("af_sarah") SELECTABLE("af_sky")
                        
                        ImGui::Spacing();
                        ImGui::TextDisabled("Male Voices");
                        ImGui::Separator();
                        SELECTABLE("am_adam") SELECTABLE("am_echo") SELECTABLE("am_eric")
                        SELECTABLE("am_fenrir") SELECTABLE("am_liam") SELECTABLE("am_michael")
                        SELECTABLE("am_onyx") SELECTABLE("am_puck") SELECTABLE("am_santa")
                        
                        ImGui::Spacing();
                        ImGui::TextDisabled("Other Voices");
                        ImGui::Separator();
                        SELECTABLE("bf_alice") SELECTABLE("bf_emma") SELECTABLE("bf_isabella")
                        SELECTABLE("bf_lily") SELECTABLE("bm_daniel") SELECTABLE("bm_fable")
                        SELECTABLE("bm_george") SELECTABLE("bm_lewis") SELECTABLE("ef_dora")
                        SELECTABLE("em_alex") SELECTABLE("em_santa") SELECTABLE("ff_siwis")
                        SELECTABLE("hf_alpha") SELECTABLE("hf_beta") SELECTABLE("hm_omega")
                        SELECTABLE("hm_psi") SELECTABLE("if_sara") SELECTABLE("im_nicola")
                        SELECTABLE("jf_alpha") SELECTABLE("jf_gongitsune") SELECTABLE("jf_nezumi")
                        SELECTABLE("jf_tebukuro") SELECTABLE("jm_kumo") SELECTABLE("pf_dora")
                        SELECTABLE("pm_alex") SELECTABLE("pm_santa") SELECTABLE("zf_xiaobei")
                        SELECTABLE("zf_xiaoni") SELECTABLE("zf_xiaoxiao") SELECTABLE("zf_xiaoyi")
                        SELECTABLE("zm_yunjian") SELECTABLE("zm_yunxia") SELECTABLE("zm_yunxi")
                        SELECTABLE("zm_yunyang")
                        #undef SELECTABLE
                        ImGui::EndCombo();
                    }
                });
                UI::end_table();

                // UI::shift_cursor_pos(0.f, 20.f);
                // ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "SAVE/LOAD");
                // ImGui::Separator();
                
                draw_title("SAVE/LOAD");
                UI::begin_table("settings", false);
                UI::table_row("Auto Save", m_auto_save);
                if (!m_auto_save) ImGui::BeginDisabled();
                UI::table_row_slider<u32>("Interval (seconds)", m_save_interval_sec, 10, 600, 10);
                if (!m_auto_save) ImGui::EndDisabled();
                UI::table_row("Auto Open Last", m_auto_open_last);
                UI::end_table();

                draw_title("DISPLAY");                
                UI::begin_table("settings", false);
                UI::table_row_slider<u16>("Font Size", m_font_size, 10, 50, 1);
                // if (m_font_size == AT::UI::g_font_size)
                //     ImGui::BeginDisabled();
                // UI::table_row([]() { ImGui::Text("Apply new font size"); }, [this]() {
                //         if (ImGui::Button("Apply"))
                //             application::get().get_imgui_config_ref()->resize_fonts(m_font_size);
                //     });
                // if (m_font_size == AT::UI::g_font_size)
                //     ImGui::EndDisabled();
                UI::end_table();

                UI::shift_cursor_pos(0.f, 20.f);
                ImGui::Separator();
                UI::shift_cursor_pos(0.f, 10.f);
                
                // Back button
                if (ImGui::Button("Back", ImVec2(-1, 0)))
                    m_sidebar_status = sidebar_status::menu;
                
            } break;
            
            case sidebar_status::project_manager: {
    
                const f32 sidebar_width = math::min(300.0f, content_size.x * 0.3f);
                ImGui::BeginChild("LeftPanel", ImVec2(sidebar_width, content_size.y), true);
                SECTION_HEADER(m_library_icon, "Project Management");
                
                // Project list section
                UI::shift_cursor_pos(0.f, 10.f);
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "RECENT PROJECTS");
                ImGui::Separator();

                static f32 project_description_height = 0.f;
                const f32 project_list_height = content_size.y - project_description_height - 
                    ImGui::GetCursorPosY() -            // Account for space used so far
                    ImGui::GetStyle().WindowPadding.y;  // Account for bottom padding

                ImGui::BeginChild("project_list", ImVec2(0, project_list_height), true);               // Project list with scrollable area
                {
                    // Create a vector to store keys to remove (since we can't modify map while iterating)
                    static std::vector<std::string> keys_to_remove;
                    keys_to_remove.clear();

                    // Iterate through the map
                    size_t i = 0;
                    for (const auto& [project_name, project_path] : m_project_paths) {
                        ImGui::PushID(i++);

                        const bool is_selected = (!m_open_projects.empty() && m_current_project == project_name);
                        if (ImGui::Selectable(project_name.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                load_project(project_name, project_path);
                            }
                        }

                        // Context menu for project options
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("Open"))
                                load_project(project_name, project_path);

                            if (ImGui::MenuItem("Remove from list"))
                                keys_to_remove.push_back(project_name);

                            ImGui::EndPopup();
                        }
                        ImGui::PopID();
                    }

                    // Remove projects marked for removal
                    for (const auto& key : keys_to_remove) {
                        m_project_paths.erase(key);
                    }

                    if (m_project_paths.empty())
                        ImGui::TextDisabled("No recent projects");
                }
                ImGui::EndChild();
                
                const f32 bu_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.f;

                const f32 project_description_start = ImGui::GetCursorPosY();
                // Action buttons
                UI::shift_cursor_pos(0.f, 20.f);
                if (ImGui::Button("New Project", ImVec2(bu_width, 0))) {

                    m_open_projects.emplace_back();

                    // Find a unique project name
                    std::string base_name = "New Project";
                    std::string candidate_name = base_name;
                    int counter = 0;
                    
                    auto name_exists = [&](const std::string& name) {                       // Check if name already exists in open-projects or project-paths
                        for (const auto& proj : m_open_projects)
                            if (proj.name == name) return true;

                        for (const auto& proj_path : m_project_paths)
                            if (proj_path.first == name) return true;
                        return false;
                    };
                    
                    while (name_exists(candidate_name))                                    // Find available name
                        candidate_name = base_name + " " + (++counter < 10 ? "0" + std::to_string(counter) : std::to_string(counter));

                    m_open_projects.back().name = candidate_name;
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Open Project...", ImVec2(bu_width, 0))) {
                    
                    // <TUTORIAL> ignore this for the tutorial and try to m
                    // TODO: Implement file dialog for opening projects
                }
                
                if (!m_open_projects.empty() && ImGui::Button("Save Project", ImVec2(bu_width, 0))) {

                    LOG(Trace, "Saving Project [" << m_current_project << "]")

                    std::filesystem::path project_path{};
                    if (m_project_paths.contains(m_current_project))
                        project_path = m_project_paths.at(m_current_project);

                    if (!project_path.empty()) {
                        LOG(Trace, "project path found [" << project_path.generic_string() << "]")
                        for (auto& proj : m_open_projects)                  // get currently open project-tap
                            if (m_current_project == proj.name) {

                                LOG(Trace, "Serializing project")
                                serialize_project(proj, project_path, serializer::option::save_to_file);
                                proj.saved = true;
                            }

                    } else {
                        
                        // TODO: should ask the user for a location (file dialog)
                        std::filesystem::path proj_path = util::file_dialog("Select location for [" + m_current_project + "]", {}, true) / m_current_project / m_current_project;       // use [m_current_project] twice to make a directory and a file
                        proj_path.replace_extension(PROJECT_EXTENTION);
                        LOG(Trace, "selected directory [" << proj_path << "]")
                        for (auto& proj : m_open_projects)                  // get currently open project-tap
                            if (m_current_project == proj.name) {

                                serialize_project(proj, proj_path, serializer::option::save_to_file);
                                proj.saved = true;
                            }

                        m_project_paths[m_current_project] = proj_path;
                        serialize(serializer::option::save_to_file);
                    }
                }
                
                ImGui::SameLine();
                if (!m_open_projects.empty() && ImGui::Button("Save as", ImVec2(bu_width, 0))) {


                    // TODO: should ask the user for a location (file dialog)
                    std::filesystem::path proj_path = util::file_dialog("Select location for [" + m_current_project + "]", {}, true) / m_current_project / m_current_project;       // use [m_current_project] twice to make a directory and a file
                    VALIDATE(!proj_path.empty(), break, "", "Failed to select a location")

                    proj_path.replace_extension(PROJECT_EXTENTION);
                    LOG(Trace, "selected directory [" << proj_path << "]")
                    for (auto& proj : m_open_projects)                  // get currently open project-tap
                        if (m_current_project == proj.name) {

                            serialize_project(proj, proj_path, serializer::option::save_to_file);
                            proj.saved = true;
                        }

                    m_project_paths[m_current_project] = proj_path;
                    serialize(serializer::option::save_to_file);

                    LOG(Trace, "Saving Project [" << m_current_project << "] to [" << proj_path << "]")
                }
                
                UI::shift_cursor_pos(0.f, 20.f);
                ImGui::Separator();
                UI::shift_cursor_pos(0.f, 10.f);
                
                // Current project info (if any project is open)
                if (!m_open_projects.empty()) {
                    for (auto& proj : m_open_projects) {
                        if (m_current_project == proj.name) {
                            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "CURRENT PROJECT");
                            
                            // Project name editing
                            static bool is_editing = false;
                            static char edit_buffer[256] = ""; // Use char array instead of std::string
                            
                            if (is_editing) {
                                ImGui::SetNextItemWidth(-1);
                                if (ImGui::InputText("##proj_name", edit_buffer, IM_ARRAYSIZE(edit_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                                    // Apply changes when Enter is pressed
                                    proj.name = edit_buffer;
                                    m_current_project = edit_buffer;
                                    is_editing = false;
                                    proj.saved = false; // Mark as unsaved
                                }
                                if (!ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                                    // Cancel editing on Escape
                                    is_editing = false;
                                }
                            } else {
                                ImGui::TextWrapped("Name: %s", proj.name.c_str());
                                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                    // Start editing on double-click
                                    strncpy(edit_buffer, proj.name.c_str(), IM_ARRAYSIZE(edit_buffer) - 1);
                                    edit_buffer[IM_ARRAYSIZE(edit_buffer) - 1] = '\0'; // Ensure null termination
                                    is_editing = true;
                                }
                            }

                            // Rest of the information
                            if (!proj.description.empty())
                                ImGui::TextWrapped("Description: %s", proj.description.c_str());
                            
                            // Project statistics
                            const size_t total_sections = proj.sections.size();
                            size_t total_fields = 0;
                            for (const auto& section : proj.sections)
                                total_fields += section.input_fields.size();
                            
                            ImGui::TextDisabled("Sections: %zu | Fields: %zu", total_sections, total_fields);
                            ImGui::TextDisabled("Status: %s", proj.saved ? "Saved" : "Unsaved");
                            const std::string path = m_project_paths.contains(m_current_project) ? 
                                                    m_project_paths.at(m_current_project) : "<not set>";
                            ImGui::TextDisabled("Location: %s", path.c_str());
                        }
                    }
                }
                
                UI::shift_cursor_pos(0.f, 20.f);

                if (!m_open_projects.empty())
                    ImGui::Separator();

                if (ImGui::Button("Back", ImVec2(-1, 0)))
                    m_sidebar_status = sidebar_status::menu;

                project_description_height = ImGui::GetCursorPosY() - project_description_start;

            } break;

            default: break;
        }

    #undef SECTION_HEADER

        ImGui::EndChild();
        UI::seperation_vertical();
        ImGui::SameLine();
    }


    void dashboard::draw_project(project& project_data) {

            for( auto& sec : project_data.sections) {
                if (ImGui::CollapsingHeader(sec.title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    
                    draw_section(project_data, sec);
                }
            }
            
            if (ImGui::Button("Add section")) {                                                     // Add section button
                
                project_data.sections.push_back(section{});
                project_data.sections.back().input_fields.push_back(input_field{});
                project_data.saved = false;
            }
    }

    
    void dashboard::draw_section(project& project_data, section& section_data) {
        
        // Section styling
        ImGui::PushID(&section_data);
        const auto imgui_style = ImGui::GetStyle();
        ImVec4 bg_color = imgui_style.Colors[ImGuiCol_FrameBg];
        bg_color.w *= 1.05f;                                                                        // Darker background
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_color);
        
        const float width = ImGui::GetContentRegionAvail().x;
        const f32 padding_x = imgui_style.FramePadding.y * 2.0f;
        const ImVec2 icon_button_size = ImVec2(15 + (AT::UI::g_font_size / 10));
        f32 button_size = (icon_button_size.x * 2) + (imgui_style.ItemSpacing.x * 4) + 20 + icon_button_size.x + 29; 
        // Generate + Audio buttons

        constexpr size_t TITLE_SIZE = 512;
        static char title_buffer[TITLE_SIZE];
        strncpy(title_buffer, section_data.title.c_str(), TITLE_SIZE - 1);
        title_buffer[TITLE_SIZE - 1] = '\0';
        ImGui::PushStyleColor(ImGuiCol_FrameBg, UI::get_default_gray_ref());
        ImGui::PushFont(application::get().get_imgui_config_ref()->get_font("header_0"));
        if (ImGui::InputText("##input_field_title", title_buffer, TITLE_SIZE, ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AllowTabInput))
            section_data.title = title_buffer;
        ImGui::PopFont();
        ImGui::PopStyleColor();

        for (size_t i = 0; i < section_data.input_fields.size(); i++) {                                                             // Input fields

            auto& field = section_data.input_fields[i];
            ImGui::PushID(&field); // Ensure unique ID if multiple fields exist

            const ImVec2 text_size = ImGui::CalcTextSize(field.content.c_str(), nullptr, false, width);                 // Calculate required height based on wrapped text
            const float height = math::max(text_size.y + padding_x, ImGui::GetTextLineHeight() * 1.5f);                 // Set height (text height + frame padding)

            constexpr size_t BUFFER_SIZE = 4096;
            static char buffer[BUFFER_SIZE];
            strncpy(buffer, field.content.c_str(), BUFFER_SIZE - 1);
            buffer[BUFFER_SIZE - 1] = '\0';

            const bool field_generating = field.generating;
            if (field_generating)
                ImGui::BeginDisabled();

            if (ImGui::InputTextMultiline("##InputField", buffer, BUFFER_SIZE, ImVec2(width - button_size, height), ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AllowTabInput)) {

                field.content = buffer;
                project_data.saved = false;
            }

                
            ImGui::SameLine();
            if (ImGui::ImageButton("##generate_button", m_generate_icon->get(), icon_button_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1))) {
                
                field.generating = true;

                {                                                       // Add to generation queue
                    std::lock_guard<std::mutex> lock(m_queue_mutex);
                    m_generation_queue.push(field.ID);
                }
                generation_worker();
            }
            
            ImGui::SameLine();
            const std::filesystem::path audio_path = util::get_executable_path() / "audio" / (util::to_string(field.ID) + ".wav");
            const bool has_audio = std::filesystem::exists(audio_path);

            if (!has_audio)      ImGui::BeginDisabled();
            if (ImGui::ImageButton("##play_audio", (field.playing_audio) ? m_stop_icon->get() : m_audio_icon->get(), icon_button_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                if (field.playing_audio)
                    stop_audio();
                else
                    play_audio(field);            // can only be pressed if audio found
            if (!has_audio)      ImGui::EndDisabled();

            if (field_generating)
                ImGui::EndDisabled();

            if (i > 0) {

                ImGui::SameLine(0, 10);
                if (ImGui::Button("^")) {

                    std::swap(section_data.input_fields[i], section_data.input_fields[i -1]);
                    project_data.saved = false;
                }
            }

            if (i < section_data.input_fields.size() -1)  {

                ImGui::SameLine(0, (i) ? -1 : 29);                              // move "down" button for first row
                if (ImGui::Button("v")) {

                    std::swap(section_data.input_fields[i], section_data.input_fields[i +1]);
                    project_data.saved = false;
                }
            }
            
            ImGui::PopID();
        }
        
        if (ImGui::Button("+ Add Field")) {                                     // Add field button
            
            section_data.input_fields.push_back(input_field{});
            project_data.saved = false;
        }
        
        
        ImGui::SameLine(0, 20);
        if (ImGui::Button("Generate All")) {                                    // Generate all button for this section

            std::lock_guard<std::mutex> lock(m_queue_mutex);
            for (size_t i = 0; i < section_data.input_fields.size(); i++) {

                m_generation_queue.push(section_data.input_fields[i].ID);       // Add to generation queue
                section_data.input_fields[i].generating = true;                 // set all fields to generate
            }
            generation_worker();
        }

        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    // --------------------------------------------------------------------------------------------------------------
    // PYTHON
    // --------------------------------------------------------------------------------------------------------------

    void dashboard::generation_worker() {

        m_worker_running = true;
        m_worker_future = std::async(std::launch::async, [this]() {

            while (true) {
                
                LOG(Trace, "Next iteration")

                UUID generation_task_ID;
                {                                                           // Get next task
                    LOG(Trace, "Trying to find next task")
                    std::lock_guard<std::mutex> lock(m_queue_mutex);
                    if (m_generation_queue.empty()) break;
                    generation_task_ID = m_generation_queue.front();
                    m_generation_queue.pop();
                }
                
                LOG(Trace, "Trying to find Corresponding string for [" << generation_task_ID << "]")

                // Find Corresponding string
                std::string text_to_generate;
                bool found = false;
                
                for (size_t project_index = 0; project_index < m_open_projects.size(); project_index++) {
                    for (size_t section_index = 0; section_index < m_open_projects[project_index].sections.size(); section_index++) {
                        for (size_t field_index = 0; field_index < m_open_projects[project_index].sections[section_index].input_fields.size(); field_index++) {
                            if (generation_task_ID == m_open_projects[project_index].sections[section_index].input_fields[field_index].ID) {
                                text_to_generate = m_open_projects[project_index].sections[section_index].input_fields[field_index].content;
                                found = true;
                                break;
                            }
                        }
                        if (found) break;
                    }
                    if (found) break;
                }

                VALIDATE(found, continue, "Found text corresponding to ID [" << generation_task_ID << "]", "Could not find text corresponding to ID [" << generation_task_ID << "]")
                
                // Generate audio
                std::filesystem::path output_path = util::get_executable_path() / "audio" / (util::to_string(generation_task_ID) + ".wav");
                std::filesystem::create_directories(output_path.parent_path());
                bool success = call_python_generate_tts(text_to_generate, output_path.string());
                VALIDATE(success, , "Successfully generated audio as [" << output_path.string() << "]", "Could not generate audio for [" << output_path.string() << "]")
                
                // need new search because user could re-arange the fields while generating
                found = false;
                for (size_t project_index = 0; project_index < m_open_projects.size(); project_index++) {
                    for (size_t section_index = 0; section_index < m_open_projects[project_index].sections.size(); section_index++) {
                        for (size_t field_index = 0; field_index < m_open_projects[project_index].sections[section_index].input_fields.size(); field_index++) {
                            if (generation_task_ID == m_open_projects[project_index].sections[section_index].input_fields[field_index].ID) {
                                m_open_projects[project_index].sections[section_index].input_fields[field_index].generating = false;                     // update status
                                found = true;
                                break;
                            }
                        }
                        if (found) break;
                    }
                    if (found) break;
                }
                
            }
            
            LOG(Trace, "Worker finished")
            m_worker_running = false;
        });
    }


    bool dashboard::initialize_python() {

        if (Py_IsInitialized())
            return true;

        Py_Initialize();
        PyEval_InitThreads();

        // Get paths
        const auto script_dir = util::get_executable_path() / "kokoro";
        const auto venv_site_packages = script_dir / "venv" / "lib" / ("python" + std::to_string(PY_MAJOR_VERSION) 
            + "." + std::to_string(PY_MINOR_VERSION)) / "site-packages";

        // Configure Python paths
        PyRun_SimpleString(("import sys\n"
                            "sys.path.append('" + script_dir.string() + "')\n"
                            "sys.path.append('" + venv_site_packages.string() + "')\n").c_str());

        // Import module
        PyObject* pModule = PyImport_ImportModule("kokoro_tts");
        VALIDATE(pModule, PyErr_Print(); return false, "", "Failed to import module")

        // Get generate_tts function
        PyObject* pFunc = PyObject_GetAttrString(pModule, "generate_tts");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            PyErr_Print();
            return false;
        }

        // Store references
        m_py_module = pModule;
        m_py_generate_tts_function = pFunc;
        m_python_thread_state = PyEval_SaveThread();
        return true;
    }


    void dashboard::finalize_python() {
        
        // PyEval_RestoreThread(m_python_thread_state);
        Py_XDECREF(m_py_generate_tts_function);
        Py_XDECREF(m_py_module);
        Py_Finalize();
    }


    bool dashboard::call_python_generate_tts(const std::string& text, const std::string& output_path) {
        
        PyGILState_STATE gil_state = PyGILState_Ensure();
        PyObject* pArgs = PyTuple_New(4);
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(text.c_str()));
        PyTuple_SetItem(pArgs, 1, PyUnicode_FromString(std::filesystem::absolute(output_path).string().c_str()));
        PyTuple_SetItem(pArgs, 2, PyUnicode_FromString(m_voice));
        PyTuple_SetItem(pArgs, 3, PyFloat_FromDouble(m_voice_speed));
        
        // Call function
        PyObject* pResult = PyObject_CallObject(m_py_generate_tts_function, pArgs);
        Py_DECREF(pArgs);
        if (!pResult) {
            PyErr_Print();
            PyGILState_Release(gil_state);
            return false;
        }
        
        // Get result as boolean
        bool success = (PyObject_IsTrue(pResult) == 1);
        Py_DECREF(pResult);
        PyGILState_Release(gil_state);
        return success;
    }

    // --------------------------------------------------------------------------------------------------------------
    // AUDIO
    // --------------------------------------------------------------------------------------------------------------

    void dashboard::play_audio(input_field& field) {
        const std::filesystem::path audio_path = util::get_executable_path() / "audio" / (util::to_string(field.ID) + ".wav");
        
    #ifdef PLATFORM_LINUX
        stop_audio(); // Stop any existing playback
        m_current_audio_field = static_cast<u64>(field.ID);
        field.playing_audio = true;

        const std::vector<std::vector<std::string>> commands = {
            {"paplay", audio_path.string()},
            {"aplay", "-D", "default", audio_path.string()},
            {"mpg123", audio_path.string()}
        };

        for (const auto& cmd : commands) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child process: Redirect output and execute player
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                
                // Prepare arguments for execvp
                std::vector<char*> args;
                for (const auto& arg : cmd) {
                    args.push_back(const_cast<char*>(arg.c_str()));
                }
                args.push_back(nullptr);
                
                execvp(args[0], args.data());
                _exit(EXIT_FAILURE); // Exit if exec fails
            }
            else if (pid > 0) {
                // Parent process: Check if player started successfully
                int status;
                usleep(10000); // Brief delay to catch quick failures
                if (waitpid(pid, &status, WNOHANG) == 0) {
                    m_audio_pid = pid;
                    m_audio_playing = true;
                    
                    // Start monitor thread to detect completion
                    m_audio_monitor = std::thread([this, pid, audio_path, id = field.ID]() {
                        // Wait for the audio process to finish
                        int status;
                        waitpid(pid, &status, 0);
                        
                        // Only log if we're tracking this specific process
                        if (m_audio_playing && m_audio_pid == pid) {

                            if (m_current_audio_field) {        // make sure we need to reset at all
                                bool found = false;

                                for (size_t project_index = 0; project_index < m_open_projects.size(); project_index++) {
                                    for (size_t section_index = 0; section_index < m_open_projects[project_index].sections.size(); section_index++) {
                                        for (size_t field_index = 0; field_index < m_open_projects[project_index].sections[section_index].input_fields.size(); field_index++) {
                                            if (m_current_audio_field == m_open_projects[project_index].sections[section_index].input_fields[field_index].ID) {
                                                m_open_projects[project_index].sections[section_index].input_fields[field_index].playing_audio = false;
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (found) break;
                                    }
                                    if (found) break;
                                }
                                VALIDATE(found, , "", "Could not reset [playing_audio] for corresponding to ID [" << m_current_audio_field << "]")
                                m_current_audio_field = 0;
                            }
                            
                            m_audio_playing = false;
                        }
                    });
                    m_audio_monitor.detach();
                    
                    return;
                }
            }
        }
        LOG(Error, "No working audio player found for: " << audio_path.string());
    #else
        PlaySound(audio_path.string().c_str(), NULL, SND_FILENAME | SND_ASYNC);
    #endif
    }


    void dashboard::stop_audio() {
        
        if (m_current_audio_field) {                // make sure we need to reset at all
        
            bool found = false;


            for (size_t project_index = 0; project_index < m_open_projects.size(); project_index++) {
                for (size_t section_index = 0; section_index < m_open_projects[project_index].sections.size(); section_index++) {
                    for (size_t field_index = 0; field_index < m_open_projects[project_index].sections[section_index].input_fields.size(); field_index++) {
                        if (m_current_audio_field == m_open_projects[project_index].sections[section_index].input_fields[field_index].ID) {
                            m_open_projects[project_index].sections[section_index].input_fields[field_index].playing_audio = false;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                if (found) break;
            }
            
            VALIDATE(found, , "Found and reset bool for [" << m_current_audio_field << "]", "Could not find bool for corresponding to ID [" << m_current_audio_field << "]")
            m_current_audio_field = 0;
        }

    #ifdef PLATFORM_LINUX
        if (m_audio_pid > 0) {
            kill(m_audio_pid, SIGTERM);
            waitpid(m_audio_pid, nullptr, 0);       // Clean up zombie process
            m_audio_pid = 0;
            m_audio_playing = false;
        }
    #else
        PlaySound(NULL, NULL, 0);                   // Stop Windows audio
    #endif
    }

    // --------------------------------------------------------------------------------------------------------------
    // UTIL
    // --------------------------------------------------------------------------------------------------------------

    void dashboard::serialize_project(project& project_data, const std::filesystem::path path, const serializer::option option) {

        serializer::yaml(path, "project_data", option)
            .entry(KEY_VALUE(project_data.name))
            .entry(KEY_VALUE(project_data.description))
            .vector(KEY_VALUE(project_data.sections), [&](serializer::yaml& yaml, u64 x) {

				yaml.entry(KEY_VALUE(project_data.sections[x].title))
                .vector(KEY_VALUE(project_data.sections[x].input_fields), [&](serializer::yaml& yaml, u64 y) {

                    yaml.entry(KEY_VALUE(project_data.sections[x].input_fields[y].content))
                    .entry(KEY_VALUE(project_data.sections[x].input_fields[y].ID));
                });
			});
        
        project_data.saved = true;          // set to saved, doesn't matter if loading/saving
    }


    void dashboard::serialize(const serializer::option option) {

        serializer::yaml(util::get_executable_path() / "config" / "project_data.yml", "project_data", option)
            .entry(KEY_VALUE(m_current_project))
            .entry(KEY_VALUE(m_auto_save))
            .entry(KEY_VALUE(m_save_interval_sec))
            .entry(KEY_VALUE(m_auto_open_last))
            .unordered_map(KEY_VALUE(m_project_paths));
    }

    
    void dashboard::save_open_projects() {
        
        serialize(serializer::option::save_to_file);
        u32 save_counter = 0;
        for (auto& proj : m_open_projects) {

            std::filesystem::path project_path{};
            if (proj.saved || !m_project_paths.contains(proj.name))        // Save projects that need it and have a path registered
                continue;

            project_path = m_project_paths.at(proj.name);
            LOG(Trace, "saving project [" << proj.name << "] to [" << project_path.generic_string() << "]")
            serialize_project(proj, project_path, serializer::option::save_to_file);
            save_counter++;
        }

        LOG(Trace, "saved [" << save_counter << "] projects")
    }


    void dashboard::load_project(const std::string& project_name, const std::filesystem::path& project_path) {

        LOG(Trace, "open [" << project_name << "] from [" << project_path << "]")
        project loaded_project{};
        serialize_project(loaded_project, project_path, serializer::option::load_from_file);
        m_open_projects.push_back(loaded_project);
    }

}
