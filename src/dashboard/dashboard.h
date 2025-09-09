#pragma once

#include "util/data_structures/UUID.h"
#include "render/image.h"
// #include "util/io/serializer_data.h"

// Forward declarations for Python
struct _ts;
typedef struct _ts PyThreadState;
struct _object;
typedef struct _object PyObject;


namespace AT {

    class event;
    namespace serializer {
        enum class option;
    }

    struct input_field {
        bool                        generating = false;
        bool                        playing_audio = false;
        UUID                        ID{};
        std::string                 content{};
    };

    struct section {
        std::string                 title{};
        std::vector<input_field>    input_fields{};
    };

    struct project {
        bool                        saved = true;       // visual flag
        std::string                 name{};
        std::string                 description;
        std::vector<section>        sections{};
    };

    enum class sidebar_status {
        menu = 0,
        settings,
        project_manager,
    };


    class dashboard {
    public:

        dashboard();
        ~dashboard();

        bool init();
        void finalize_init();
        void update(f32 delta_time);
        void draw(f32 delta_time);
        bool shutdown();
        void on_crash();

        void on_event(event& event);
        void draw_init_UI(f32 delta_time);
        
    private:

        // UI
        void draw_project(project& project_data);
        void draw_section(project& project_data, section& section_data);
	    void draw_sidebar();

        // Python integration
        bool initialize_python();
        void finalize_python();
        bool call_python_generate_tts(const std::string& text, const std::string& output_path);
        void generation_worker();

        // audio
        void play_audio(input_field& field);
        void stop_audio();

        void serialize_project(project& project_data, const std::filesystem::path path, const serializer::option option);
        void serialize(const serializer::option option);
        void save_open_projects();
        void load_project(const std::string& project_name, const std::filesystem::path& project_path);

    #ifdef PLATFORM_LINUX
        pid_t                                                           m_audio_pid = 0;
        std::atomic<bool>                                               m_audio_playing{false};
        std::thread                                                     m_audio_monitor;
    #endif                              
        u64                                                             m_current_audio_field = 0;

        std::atomic<bool>                                               m_is_generating = false;
        std::future<void>                                               m_worker_future;
        std::future<bool>                                               m_generation_future;

        std::string                                                     m_current_project{};
        std::vector<project>                                            m_open_projects{};               // projects currently opened
        std::unordered_map<std::string, std::filesystem::path>          m_project_paths{};

        sidebar_status                                                  m_sidebar_status = sidebar_status::project_manager;     // start at PM because that is always the first step

        std::queue<UUID>                                                m_generation_queue{};
        std::mutex                                                      m_queue_mutex;
        std::atomic<bool>                                               m_worker_running = false;

        PyThreadState*                                                  m_python_thread_state = nullptr;
        PyObject*                                                       m_py_module = nullptr;
        PyObject*                                                       m_py_generate_tts_function = nullptr;

        bool                                                            m_auto_save = true;
        system_time                                                     m_last_save_time;
        u32                                                             m_save_interval_sec = 300;
        bool                                                            m_control_key_pressed = false;
        const char*                                                     m_voice = "am_onyx";
        f32                                                             m_voice_speed = 1.2;
        bool                                                            m_auto_open_last = true;
        u16                                                             m_font_size = 15;

        std::string                                                     m_generation_status{};                          // Status messages
        std::atomic<bool>                                               m_shutting_down = false;
		ref<image>						                                m_generate_icon;
		ref<image>						                                m_audio_icon;
		ref<image>						                                m_stop_icon;
		ref<image>						                                m_settings_icon;
		ref<image>						                                m_library_icon;

    };
}
