
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
#include "application.h"
#include "config/imgui_config.h"
#include "util/ui/pannel_collection.h"

#include "dashboard.h"


namespace AT {

#ifdef _WIN32
    #include <Windows.h>
    #pragma comment(lib, "winmm.lib")
#endif


    dashboard::dashboard()
        : m_is_generating(false) {

        // Section 1
        section sec1;
        sec1.title = "test section 000";
        sec1.input_fields = {
            {"Once upon a time in a land far, far away..."},
            {"The quick brown fox jumps over the lazy dog"},
            {"Hello world! This is a test of the text-to-speech system. "},
            {"My implementation of Peter Shirley's Ray Tracing in One Weekend books using Vulkan and NVIDIA's RTX extension"}
        };
        
        // Section 2
        section sec2;
        sec1.title = "test section 001";
        sec2.input_fields = {
            {"The rain in Spain falls mainly on the plain"},
            {"To be or not to be, that is the question"},
            {"All work and no play makes Jack a dull boy"}
        };
        
        // Section 3
        section sec3;
        sec1.title = "test section 002";
        sec3.input_fields = {
            {"Four score and seven years ago our fathers brought forth..."},
            {"Ask not what your country can do for you..."},
            {"I have a dream that one day this nation will rise up..."},
            {"The only thing we have to fear is fear itself"},
            {"Tear down this wall!"},
            {"Yes we can!"}
        };
        
        m_sections = {sec1, sec2, sec3};

		const std::filesystem::path icon_path = CONTENT_PATH / "images";
#define LOAD_ICON(name)			m_##name##_icon = create_ref<image>(icon_path / #name ".png", image_format::RGBA)
		LOAD_ICON(generate);
		LOAD_ICON(audio);
		LOAD_ICON(stop);
		LOAD_ICON(settings);
		LOAD_ICON(library);
#undef	LOAD_ICON

        initialize_python();

    }
    
    
    dashboard::~dashboard() {

        if (Py_IsInitialized())
            finalize_python();

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
        return true;
    }

    // shutdown will be called bevor any system is deinitalized
    bool dashboard::shutdown() {

        // Acquire GIL if Python is initialized
        PyGILState_STATE gstate = PyGILState_UNLOCKED;
        if (Py_IsInitialized())
            gstate = PyGILState_Ensure();

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


    void dashboard::update(f32 delta_time) {
            
    }


    void dashboard::on_event(event& event) {}

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
        ImGui::Begin("MainWindow", nullptr, flags);
        {
            sidebar();
                        
            // const ImVec2 content_size = ImGui::GetContentRegionAvail();
            // Right panel (fills remaining space)
            ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
            {
                
                // Draw sections
                for (size_t i = 0; i < m_sections.size(); i++) {
                    draw_section(i);
                }
                
                // Add section button
                if (ImGui::Button("Add section")) {
                    m_sections.push_back(section{});
                    m_sections.back().input_fields.push_back(input_field{});
                }
            }
            ImGui::EndChild();
        }
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


    void dashboard::sidebar() {

        // Get available content region
        const ImVec2 content_size = ImGui::GetContentRegionAvail();
        const float icon_size = 30.0f;

    #define SECTION_HEADER(button, section_title)                UI::shift_cursor_pos(0.f, 5.f); ImGui::Image(button->get(), ImVec2(icon_size, icon_size), ImVec2(0, 0), ImVec2(1, 1));          \
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
                draw_sidebar_button("##kokoro_settings", sidebar_status::kokoro_settings, m_settings_icon);
		        
                UI::shift_cursor_pos(0, 10);
                draw_sidebar_button("##project_manager", sidebar_status::project_manager, m_library_icon);
                
            } break;

            case sidebar_status::kokoro_settings: {

                const f32 sidebar_width = math::min(200.0f, content_size.x * 0.3f);
                ImGui::BeginChild("LeftPanel", ImVec2(sidebar_width, content_size.y), true);
               
                SECTION_HEADER(m_settings_icon, "Kokoro settings");

                UI::begin_table("kokoro_settings", false);
                UI::table_row_slider("Voice Speed", m_voice_speed, .5f, 2.f, .05f);
                UI::table_row([]() { ImGui::Text("Voice Type"); },
                    [&]() {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        if (ImGui::BeginCombo("##Voice Type", m_voice, ImGuiComboFlags_HeightLarge)) {
                        #define SELECTABLE(voice)       if (ImGui::Selectable(voice)) m_voice = voice;
                            SELECTABLE("af_alloy")      SELECTABLE("af_aoede")      SELECTABLE("af_bella")      SELECTABLE("af_heart")
                            SELECTABLE("af_jessica")    SELECTABLE("af_kore")       SELECTABLE("af_nicole")     SELECTABLE("af_nova")
                            SELECTABLE("af_river")      SELECTABLE("af_sarah")      SELECTABLE("af_sky")        SELECTABLE("am_adam")
                            SELECTABLE("am_echo")       SELECTABLE("am_eric")       SELECTABLE("am_fenrir")     SELECTABLE("am_liam")
                            SELECTABLE("am_michael")    SELECTABLE("am_onyx")       SELECTABLE("am_puck")       SELECTABLE("am_santa")
                            SELECTABLE("bf_alice")      SELECTABLE("bf_emma")       SELECTABLE("bf_isabella")   SELECTABLE("bf_lily")
                            SELECTABLE("bm_daniel")     SELECTABLE("bm_fable")      SELECTABLE("bm_george")     SELECTABLE("bm_lewis")
                            SELECTABLE("ef_dora")       SELECTABLE("em_alex")       SELECTABLE("em_santa")      SELECTABLE("ff_siwis")
                            SELECTABLE("hf_alpha")      SELECTABLE("hf_beta")       SELECTABLE("hm_omega")      SELECTABLE("hm_psi")
                            SELECTABLE("if_sara")       SELECTABLE("im_nicola")     SELECTABLE("jf_alpha")      SELECTABLE("jf_gongitsune")
                            SELECTABLE("jf_nezumi")     SELECTABLE("jf_tebukuro")   SELECTABLE("jm_kumo")       SELECTABLE("pf_dora")
                            SELECTABLE("pm_alex")       SELECTABLE("pm_santa")      SELECTABLE("zf_xiaobei")    SELECTABLE("zf_xiaoni")
                            SELECTABLE("zf_xiaoxiao")   SELECTABLE("zf_xiaoyi")     SELECTABLE("zm_yunjian")    SELECTABLE("zm_yunxia")
                            SELECTABLE("zm_yunxi")      SELECTABLE("zm_yunyang")
                        #undef SELECTABLE()
                            ImGui::EndCombo();
                        }
                    }
                );
                UI::end_table();

                ImGui::Separator();
                if (ImGui::Button("Back"))
            	    m_sidebar_status = sidebar_status::menu;
                
            } break;
            
            case sidebar_status::project_manager: {

                const f32 sidebar_width = math::min(200.0f, content_size.x * 0.3f);
                ImGui::BeginChild("LeftPanel", ImVec2(sidebar_width, content_size.y), true);
               
                SECTION_HEADER(m_library_icon, "Project Management");

                
                // TODO: add project manager controls (create/save/load/delete/...)


                ImGui::Separator();
                if (ImGui::Button("Back"))
            	    m_sidebar_status = sidebar_status::menu;
                
            } break;

            default: break;
        }

    #undef SECTION_HEADER()

        ImGui::EndChild();
        UI::seperation_vertical();
        ImGui::SameLine();
    }


    void dashboard::draw_section(int section_index) {
        auto& sec = m_sections[section_index];
        
        // Section styling
        ImGui::PushID(section_index);
        ImVec4 bg_color = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
        bg_color.w *= 1.05f;                                                                                             // Darker background
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_color);
        
        const float width = ImGui::GetContentRegionAvail().x;
        const f32 padding_x = ImGui::GetStyle().FramePadding.y * 2.0f;
        const f32 button_size = 105.f;

        ImGui::Separator();
        UI::big_text((sec.title).c_str());

        for (size_t i = 0; i < sec.input_fields.size(); i++) {                                                             // Input fields

            auto& field = sec.input_fields[i];
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

            if (ImGui::InputTextMultiline("##InputField", buffer, BUFFER_SIZE, ImVec2(width - button_size, height), ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AllowTabInput))
                field.content = buffer;

                
            ImGui::SameLine();
            if (ImGui::ImageButton("##generate_button", m_generate_icon->get(), ImVec2(18, 18), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1))) {
                
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
            if (ImGui::ImageButton("##play_audio", (field.playing_audio) ? m_stop_icon->get() : m_audio_icon->get(), ImVec2(18, 18), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                if (field.playing_audio)
                    stop_audio();
                else
                    play_audio(field);            // can only be pressed if audio found
            if (!has_audio)      ImGui::EndDisabled();

            if (field_generating)
                ImGui::EndDisabled();

            if (i > 0) {

                ImGui::SameLine(0, 10);
                if (ImGui::Button("^"))
                    std::swap(sec.input_fields[i], sec.input_fields[i -1]);
            }

            if (i < sec.input_fields.size() -1)  {

                ImGui::SameLine(0, (i) ? -1 : 29);                              // move "down" button for first row
                if (ImGui::Button("v"))
                    std::swap(sec.input_fields[i], sec.input_fields[i +1]);
            }
            
            ImGui::PopID();
        }
        
        // Add field button
        if (ImGui::Button("+ Add Field")) {
            sec.input_fields.push_back(input_field{});
        }
        
        // Generate all button for this section
        ImGui::SameLine(0, 20);
        if (ImGui::Button("Generate All")) {

            for (size_t i = 0; i < sec.input_fields.size(); i++)       // set all fields to generate
                sec.input_fields[i].generating = true;
            
            // TODO: Implement batch audio generation

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
                size_t section_index = 0;
                size_t field_index = 0;
                for (section_index = 0; section_index < m_sections.size(); section_index++) {
                    for (field_index = 0; field_index < m_sections[section_index].input_fields.size(); field_index++) {
                        if (generation_task_ID == m_sections[section_index].input_fields[field_index].ID) {
                            text_to_generate = m_sections[section_index].input_fields[field_index].content;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }

                VALIDATE(found, continue, "Found text corresponding to ID [" << generation_task_ID << "]", "Could not find text corresponding to ID [" << generation_task_ID << "]")
                
                // Generate audio
                std::filesystem::path output_path = util::get_executable_path() / "audio" / (util::to_string(generation_task_ID) + ".wav");
                std::filesystem::create_directories(output_path.parent_path());
                bool success = call_python_generate_tts(text_to_generate, output_path.string());
                VALIDATE(success, , "Successfully generated audio as [" << output_path.string() << "]", "Could not generate audio for [" << output_path.string() << "]")
                
                m_sections[section_index].input_fields[field_index].generating = false;                     // update status
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
                                for (size_t section_index = 0; section_index < m_sections.size(); section_index++) {
                                    for (size_t field_index = 0; field_index < m_sections[section_index].input_fields.size(); field_index++) {
                                        if (m_current_audio_field == m_sections[section_index].input_fields[field_index].ID) {
                                            m_sections[section_index].input_fields[field_index].playing_audio = false;
                                            found = true;
                                            break;
                                        }
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
        
        if (m_current_audio_field) {        // make sure we need to reset at all
        
            bool found = false;
            for (size_t section_index = 0; section_index < m_sections.size(); section_index++) {
                for (size_t field_index = 0; field_index < m_sections[section_index].input_fields.size(); field_index++) {
                    if (m_current_audio_field == m_sections[section_index].input_fields[field_index].ID) {
                        m_sections[section_index].input_fields[field_index].playing_audio = false;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            VALIDATE(found, , "Found and reset bool for [" << m_current_audio_field << "]", "Could not find bool for corresponding to ID [" << m_current_audio_field << "]")
            m_current_audio_field = 0;
        }

    #ifdef PLATFORM_LINUX
        if (m_audio_pid > 0) {
            kill(m_audio_pid, SIGTERM);
            waitpid(m_audio_pid, nullptr, 0); // Clean up zombie process
            m_audio_pid = 0;
            m_audio_playing = false;
        }
    #else
        PlaySound(NULL, NULL, 0); // Stop Windows audio
    #endif
    }

}
