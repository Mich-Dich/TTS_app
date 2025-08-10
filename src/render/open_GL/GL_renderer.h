#pragma once

#include "render/data_structures_for_renderer.h"
#include "render/renderer.h"

namespace AT {

    class layer_stack;
    class file_watcher_system;
}

namespace AT::render::open_GL {

    typedef unsigned int	GLuint;
    typedef unsigned int    GLenum;

    class GL_renderer : public AT::render::renderer{
    public:
        GL_renderer(ref<window> window);
        ~GL_renderer();
    
        void draw_frame(float delta_time) override;
		void set_window_size(const u32 width, const u32 height) override;

        // -------- ImGui --------
        void imgui_init() override;
        void imgui_shutdown() override;
        void imgui_create_fonts() override;
        void imgui_destroy_fonts() override;

        void execute_pending_commands();
        void resource_free() override;

    private:
    
        GLuint                      m_shader_program;
        GLuint                      m_total_render_time{};

        void serialize(serializer::option option) override;
    };

}
