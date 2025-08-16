
#include "util/pch.h"

#include <Python.h>

#include <imgui/imgui.h>

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
#undef	LOAD_ICON

    }
    
    
    dashboard::~dashboard() {

        if (Py_IsInitialized())
            finalize_python();

        m_generate_icon.reset();
        m_audio_icon.reset();
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

        return initialize_python();
    }

    // shutdown will be called bevor any system is deinitalized
    bool dashboard::shutdown() {

        if (m_worker_running) {                                         // Signal worker to stop

            {                                                           // Clear queue
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                while (!m_generation_queue.empty())
                    m_generation_queue.pop();
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


    void dashboard::draw(f32 delta_time) {
        auto viewport = ImGui::GetMainViewport();
        
        // Create a fullscreen window without decorations
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags flags = 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoSavedSettings;
        
        ImGui::Begin("MainWindow", nullptr, flags);
        {
            // Get available content region
            ImVec2 content_size = ImGui::GetContentRegionAvail();
            float left_width = math::min(200.0f, content_size.x * 0.3f);
            
            // Left panel (fixed width)
            ImGui::BeginChild("LeftPanel", ImVec2(left_width, content_size.y), true);
            {
                // Left panel content
                if (ImGui::Button("Generate Audio")) {
                    generate_audio_async("example text", 
                        (util::get_executable_path() / "output.wav").generic_string());
                }

                ImGui::SliderFloat("Voice Speed", &m_voice_speed, 0.5f, 2.0f);
                if (ImGui::BeginCombo("Voice Type", m_voice)) {
                    if (ImGui::Selectable("am_onyx")) m_voice = "am_onyx";
                    if (ImGui::Selectable("am_echo")) m_voice = "am_echo";
                    ImGui::EndCombo();
                }
                
                if (!m_generation_status.empty()) {
                    ImGui::Text("%s", m_generation_status.c_str());
                }
            }
            ImGui::EndChild();

            UI::seperation_vertical();
            
            ImGui::SameLine();
            
            // Right panel (fills remaining space)
            ImGui::BeginChild("RightPanel", ImVec2(0, content_size.y), true);
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


    void dashboard::on_event(event& event) {}





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
            if (ImGui::ImageButton("##play_audio", m_audio_icon->get(), ImVec2(18, 18), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1))) {
                
                if (has_audio)
                    play_audio(audio_path.string());
                
            }
            if (!has_audio)      ImGui::EndDisabled();

            if (field_generating)
                ImGui::EndDisabled();

            if (i > 0) {

                ImGui::SameLine(0, 10);
                if (ImGui::Button("^"))
                    std::swap(sec.input_fields[i], sec.input_fields[i -1]);
            }

            if (i < sec.input_fields.size() -1)  {

                ImGui::SameLine(0, (i) ? -1 : 29);
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
        PyEval_RestoreThread(m_python_thread_state);
        Py_XDECREF(m_py_generate_tts_function);
        Py_XDECREF(m_py_module);
        Py_Finalize();
    }


    bool dashboard::call_python_generate_tts(const std::string& text, const std::string& output_path) {
        
        PyGILState_STATE gil_state = PyGILState_Ensure();
        PyObject* pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(text.c_str()));
        std::filesystem::path abs_path = std::filesystem::absolute(output_path);
        PyTuple_SetItem(pArgs, 1, PyUnicode_FromString(abs_path.string().c_str()));
        
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


    void dashboard::generate_audio_async(const std::string& text, const std::string& output_path) {
        
        if (m_is_generating || m_shutting_down)
            return;
        
        m_is_generating = true;
        m_generation_status = "Generating audio...";
        m_generation_future = std::async(std::launch::async, [this, text, output_path]() {
            
            LOG(Trace, "generating audio for [" << text << "] to this location [" << output_path << "]")
            bool success = false;
            try {
                success = call_python_generate_tts(text, output_path);
            } catch (const std::exception& e) {
                LOG(Error, "Exception in TTS generation: " << e.what())
                m_generation_status = "Generation error!";
            }
            
            if (!m_shutting_down) {
                m_is_generating = false;
                
                if (success) {
                    play_audio(output_path);
                    m_generation_status = "Audio generated successfully!";
                } else
                    m_generation_status = "Audio generation failed!";
            }
            
            return success;
        });
    }


    void dashboard::play_audio(const std::string& path) {

    #ifdef PLATFORM_LINUX                           // Linux audio playback    
        std::string command = "aplay \"" + path + "\" &";
        std::system(command.c_str());
    #else
        PlaySound(path.c_str(), NULL, SND_FILENAME | SND_ASYNC);
    #endif
    }

}
