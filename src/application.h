
#pragma once

#include "config/imgui_config.h"
#include "dashboard/dashboard.h"

namespace AT {

    class window;
    class event;
    class window_resize_event;
    class window_close_event;
    class window_refresh_event;
    class window_focus_event;
    class dashboard;
    namespace UI        { class imgui_config; }
    namespace render    { class renderer; }

    class application {
    public:

        application(int argc, char* argv[]);
        ~application();

        DEFAULT_GETTER_C(f64,											    delta_time);
        DEFAULT_GETTER(ref<AT::render::renderer>,					        renderer);
        DEFAULT_GETTER_S(ref<window>,							            window);
        DEFAULT_GETTER_REF(ref<UI::imgui_config>,                           imgui_config);
        DEFAULT_GETTER(ref<dashboard>,                                      dashboard);

        FORCEINLINE static application& get()								{ return *s_instance; }
        FORCEINLINE static void close_application()							{ s_running = false; }

        bool update(f32 delta_time);
        bool draw(f32 delta_time);
        void run();

        // ---------------------- fps control ---------------------- 
        void set_fps_settings(u32 target_fps);
        void set_fps_settings(const bool set_for_engine_focused, const u32 new_limit);

    protected:

        ref<UI::imgui_config>               m_imgui_config;
        std::vector<event>			        m_event_queue;		// TODO: change to queue
        ref<AT::render::renderer>           m_renderer{};

    private:

        // ---------------------- event ---------------------- 
        void on_event(event& event);
        bool on_window_close(window_close_event& event);
        bool on_window_resize(window_resize_event& event);
        bool on_window_refresh(window_refresh_event& event);
        bool on_window_focus(window_focus_event& event);

        void start_fps_measurement();
        void end_fps_measurement(f32& work_time);
        void limit_fps();

        static application*			        s_instance;
        static ref<window>		            s_window;
        static bool					        s_running;

        ref<dashboard>                      m_dashboard;
        u64                                 m_crash_subscription = 0;
        bool						        m_focus = true;
        bool                                m_is_titlebar_hovered = false;
        u32							        m_target_fps = 60;
        u32							        m_nonefocus_fps = 30;
        u32							        m_fps{};
        f32							        m_delta_time = 0.f;
        f32							        m_absolute_time = 0.f;
        f32							        m_work_time{}, m_sleep_time{};
        f32							        target_duration{};
        f32							        m_last_frame_time = 0.f;
    };

}
