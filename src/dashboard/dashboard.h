#pragma once

#include "util/data_structures/UUID.h"
#include "render/image.h"

// Forward declarations for Python
struct _ts;
typedef struct _ts PyThreadState;
struct _object;
typedef struct _object PyObject;


namespace AT {

    class event;

    class dashboard {
    public:

        dashboard();
        ~dashboard();

        bool init();
        void update(f32 delta_time);
        void draw(f32 delta_time);
        bool shutdown();

        void on_event(event& event);

		DEFAULT_GETTER(ref<image>,				generate_icon)
		DEFAULT_GETTER(ref<image>,				audio_icon)

    private:

        void generate_audio_async(const std::string& text, const std::string& output_path);
        void play_audio(const std::string& path);
        
        std::atomic<bool>               m_is_generating = false;
        std::future<void>               m_worker_future;
        std::future<bool>               m_generation_future;

        // Python integration
        bool initialize_python();
        void finalize_python();
        bool call_python_generate_tts(const std::string& text, const std::string& output_path);
        void draw_section(int section_index);
        void generation_worker();
        void play_audio(std::filesystem::path audio_path);

        struct input_field {
            std::string                 content{};
            bool                        generating = false;
            UUID                        ID{};
        };
        struct section {

            std::string                 title{};
            std::vector<input_field>    input_fields{};
        };
        
        std::vector<section>            m_sections{};
        std::queue<UUID>                m_generation_queue{};
        std::mutex                      m_queue_mutex;
        std::atomic<bool>               m_worker_running = false;

        PyThreadState*                  m_python_thread_state = nullptr;
        PyObject*                       m_py_module = nullptr;
        PyObject*                       m_py_generate_tts_function = nullptr;
        const char*                     m_voice = "am_onyx";
        f32                             m_voice_speed = 1.2;
        std::string                     m_generation_status{};                          // Status messages
        std::atomic<bool>               m_shutting_down = false;
		ref<image>						m_generate_icon;
		ref<image>						m_audio_icon;

    };
}
