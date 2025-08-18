
#pragma once

#include "render/data_structures_for_renderer.h"
#include "platform/window.h"
#include "render/buffer.h"

namespace AT {
    class window;
    struct material_file_header;
}
namespace AT::geometry { class static_mesh; }
namespace AT::serializer { enum class option; }


namespace AT::render {

    class renderer {
    public:
    
        DELETE_COPY_CONSTRUCTOR(renderer);
        renderer(ref<window> window)
            : m_window(window) {}
        virtual ~renderer() = default;
    
		DEFAULT_GETTERS(general_performance_metrik,	                    general_performance_metrik)
        DEFAULT_GETTER_SETTER(bool,                                     render_world_viewport)
        DEFAULT_GETTER_SETTER(system_state,                             state)
		DEFAULT_GETTER(glm::ivec2,	                                    window_size)
		DEFAULT_GETTER(glm::ivec2,	                                    viewport_size)

        virtual void draw_startup_UI(float delta_time) = 0;
        virtual void draw_frame(float delta_time) = 0;
        virtual void set_window_size(const u32 width, const u32 height) = 0;                    // resize application window
	    virtual void resource_free() = 0;

        // -------- ImGui --------
        virtual void imgui_init() = 0;
        virtual void imgui_shutdown() = 0;
        virtual void imgui_create_fonts() = 0;
        virtual void imgui_destroy_fonts() = 0;

        // -------- fixed for all sub-classes --------
        template<typename T>
        void execute_command(T&& command) {
            m_shader_command_queue.push_back([command = std::forward<T>(command)]() {
                command();
            });
        }
        
    protected:

        virtual void execute_pending_commands() = 0;
        virtual void serialize(serializer::option option) = 0;

		// bool								m_render_to_texture = true;	    // TODO: implement later:  TRUE: render to image, then display it in an imgui window    FALSE: render directly into GLFW_window
		system_state						m_state = system_state::inactive;
        glm::ivec2                          m_window_size{0, 0};
        glm::ivec2                          m_viewport_size{0, 0};
        ref<AT::window>                     m_window;
        system_state                        m_system_state = system_state::inactive;
        general_performance_metrik          m_general_performance_metrik{};
        bool                                m_render_world_viewport = true;
        std::vector<std::function<void()>>  m_shader_command_queue;
		bool							    m_imgui_initalized = false;
    
        mutable std::shared_mutex           m_shared_resources_mutex;
    };

}
