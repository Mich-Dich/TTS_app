
#pragma once

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

    private:

        void generate_audio_async(const std::string& text, const std::string& output_path);
        void play_audio(const std::string& path);
        
        std::atomic<bool> m_is_generating = false;
        std::future<bool> m_generation_future;

        // Python integration
        bool initialize_python();
        void finalize_python();
        bool call_python_generate_tts(const std::string& text, const std::string& output_path);

        PyThreadState* m_python_thread_state = nullptr;
        PyObject* m_py_module = nullptr;
        PyObject* m_py_generate_tts_function = nullptr;

        std::string m_generation_status{};                          // Status messages
        std::atomic<bool> m_shutting_down = false;

    };
}
