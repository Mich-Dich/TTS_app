
#include "util/pch.h"

#include <Python.h>

#include <imgui/imgui.h>

#include "events/event.h"
#include "events/application_event.h"
#include "events/mouse_event.h"
#include "events/key_event.h"
#include "application.h"
#include "config/imgui_config.h"

#include "dashboard.h"


namespace AT {

#ifdef _WIN32
    #include <Windows.h>
    #pragma comment(lib, "winmm.lib")
#endif

    dashboard::dashboard()
        : m_is_generating(false) {}
    
    
    dashboard::~dashboard() {

        if (Py_IsInitialized())
            finalize_python();
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
        
        return initialize_python();
    }

    // shutdown will be called bevor any system is deinitalized
    bool dashboard::shutdown() {
        
        // Signal that we're shutting down
        m_shutting_down = true;
        
        // Cancel any ongoing generation
        if (m_is_generating) {
            // Wait for a reasonable time, then detach if still running
            if (m_generation_future.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
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
        
        // create a full-window dockspace
        {
            auto viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y));
            ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y));
            ImGui::SetNextWindowViewport(viewport->ID);

            ImGuiWindowFlags host_window_flags = 0;
            host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
            host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("main_content_area", NULL, host_window_flags);
                ImGui::PopStyleVar(3);

                static ImGuiDockNodeFlags dockspace_flags = 0;
                ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            ImGui::End();
        }


        // Main content window
        ImGui::SetNextWindowSize({400, 300});
        ImGui::Begin("Kokoro TTS");
        {
            if (ImGui::Button("Generate Audio"))
                generate_audio_async("example text", "./output.wav");        // Force simple dummy text for now
            
            if (!m_generation_status.empty())
                ImGui::Text("%s", m_generation_status.c_str());
        }
        ImGui::End();

    }


    void dashboard::on_event(event& event) {}




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
