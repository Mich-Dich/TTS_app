
#include "util/pch.h"

#include <GLFW/glfw3.h>

#include "util/crash_handler.h"
#include "util/util.h"
#include "platform/window.h"
#include "events/event.h"
#include "events/application_event.h"
#include "events/mouse_event.h"
#include "events/key_event.h"
#include "render/renderer.h"

#if defined(RENDER_API_OPENGL)
    #include "render/open_GL/GL_renderer.h"
#endif

#include "config/imgui_config.h"
#include "dashboard/dashboard.h"

#include "application.h"

namespace AT {

    
    application* application::s_instance = nullptr;
    ref<window> application::s_window;
    bool application::s_running;

    application::application(int argc, char* argv[]) {
        
        ASSERT(!s_instance, "", "Application already exists");
        LOG_INIT
        s_instance = this;
    
        // ----------- general subsystems -----------
    #if defined(PLATFORM_LINUX)
        util::init_qt();
    #endif
        set_fps_settings(m_target_fps);
        s_window = std::make_shared<window>();
        s_window->set_event_callback(BIND_FUNCTION(application::on_event));

        // ----------- general subsystems -----------
    #if defined(RENDER_API_OPENGL)
        m_renderer = create_ref<AT::render::open_GL::GL_renderer>(s_window);
    #elif defined(RENDER_API_VULKAN)
        m_renderer = create_ref<AT::render::vulkan::VK_renderer>(s_window);
    #endif

        // ----------- user defined system -----------
        m_imgui_config = create_ref<UI::imgui_config>();
        m_dashboard = create_ref<dashboard>();
        m_crash_subscription = AT::crash_handler::subscribe([this]() { m_dashboard->on_crash(); });
    }

    application::~application() {

        m_dashboard.reset();
        AT::crash_handler::unsubscribe(m_crash_subscription);
		m_imgui_config.reset();
        
        m_renderer->resource_free();         // need to call free manually because some destructors need the applications access to the renderer (eg: image)
		m_renderer.reset();
        s_window.reset();
    #if defined(PLATFORM_LINUX)
        util::shutdown_qt();
    #endif
        LOG_SHUTDOWN
        logger::shutdown();
    }

    
    void application::run() {
    
        // ---------------------------------------- finished setup ----------------------------------------
        bool long_startup_process = false;
		AT::serializer::yaml(config::get_filepath_from_configtype(util::get_executable_path(), config::file::app_settings), "general_settings", AT::serializer::option::load_from_file)
			.entry(KEY_VALUE(long_startup_process));

        if (long_startup_process) {

            m_renderer->set_state(system_state::active);
            s_running = true;
            s_window->show_window(true);
            start_fps_measurement();

            std::atomic<bool> running_init = true;
            std::future<bool> init_future = std::async(std::launch::async, [this, &running_init]() {
                ASSERT(m_dashboard->init(), "", "Failed to init user process")
                running_init = false;
                return true;                            // return true if not crashing
            });

            while (running_init) {
    
                s_window->poll_events();				// update internal state
                m_renderer->draw_startup_UI(m_delta_time);
                limit_fps();
            }

            if (!s_running) {                           // Handle early termination
                init_future.wait();                     // Ensure thread finishes before shutdown
                return;                                 // Skip main loop entirely
            }

        } else {

            ASSERT(m_dashboard->init(), "", "Failed to init user process")
            m_renderer->set_state(system_state::active);
            s_running = true;
            s_window->show_window(true);
            s_window->poll_events();
            start_fps_measurement();
        }
    
        // ---------------------------------------- main loop ----------------------------------------
        while (s_running) {
    
            // PROFILE_SCOPE("run")
            s_window->poll_events();				// update internal state
            m_dashboard->update(m_delta_time);
            m_renderer->draw_frame(m_delta_time);
            limit_fps();
        }
    
        LOG(Trace, "Exiting main run loop")
        ASSERT(m_dashboard->shutdown(), "", "Failed user shutdown process");
    }
    
    // ==================================================================== PUBLIC ====================================================================
    
    void application::set_fps_settings(const bool set_for_engine_focused, const u32 new_limit) {
    
        if (set_for_engine_focused)
            m_target_fps = new_limit;
        else
            m_nonefocus_fps = new_limit;
    
        set_fps_settings(m_focus ? m_target_fps : m_nonefocus_fps);
    }


    void application::set_fps_settings(u32 target_fps)              { target_duration = static_cast<f32>(1.0 / target_fps); }

    // ==================================================================== PRIVATE ====================================================================

    void application::start_fps_measurement()                       { m_last_frame_time = static_cast<f32>(glfwGetTime()); }
    

    void application::end_fps_measurement(f32& work_time)           { work_time = static_cast<f32>(glfwGetTime()) - m_last_frame_time; }
    

    void application::limit_fps() {
    
        m_work_time = static_cast<f32>(glfwGetTime()) - m_last_frame_time;
        if (m_work_time < target_duration) {
    
            // PROFILE_SCOPE("sleep");
            m_sleep_time = (target_duration - m_work_time) * 1000;
            util::high_precision_sleep(m_sleep_time / 1000);
        } else
            m_sleep_time = 0;
    
        f32 time = static_cast<f32>(glfwGetTime());
        m_delta_time = std::min<f32>(time - m_last_frame_time, 100000);
        m_absolute_time += m_delta_time;
        m_last_frame_time = time;
        m_fps = static_cast<u32>(1.0 / (m_work_time + (m_sleep_time * 0.001)) + 0.5); // Round to nearest integer
    }
    
    // ==================================================================== event handling ====================================================================
    
    void application::on_event(event& event) {
    
        // application events
        event_dispatcher dispatcher(event);
        dispatcher.dispatch<window_close_event>(BIND_FUNCTION(application::on_window_close));
        dispatcher.dispatch<window_resize_event>(BIND_FUNCTION(application::on_window_resize));
        dispatcher.dispatch<window_refresh_event>(BIND_FUNCTION(application::on_window_refresh));
        dispatcher.dispatch<window_focus_event>(BIND_FUNCTION(application::on_window_focus));
    
        // none application events
        m_dashboard->on_event(event);
    }
    
    bool application::on_window_close(window_close_event& event) {
    
        s_running = false;
        return true;
    }
    
    bool application::on_window_resize(window_resize_event& event) {
    
        m_renderer->set_window_size(event.get_width(), event.get_height());
        return true;
    }
    
    bool application::on_window_refresh(window_refresh_event& event) {
    
        limit_fps();
        return true;
    }
    
    bool application::on_window_focus(window_focus_event& event) {
    
        m_focus = event.get_focus();
        if (event.get_focus())
            set_fps_settings(m_target_fps);
        else
            set_fps_settings(m_nonefocus_fps);
    
        return true;
    }

}
