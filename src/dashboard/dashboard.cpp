
#include "util/pch.h"

#include <imgui/imgui.h>

#include "events/event.h"
#include "events/application_event.h"
#include "events/mouse_event.h"
#include "events/key_event.h"
#include "application.h"
#include "config/imgui_config.h"

#include "dashboard.h"


namespace AT {

    dashboard::dashboard() {}
    
    
    dashboard::~dashboard() {}


    // init will be called when every system is initalized
    bool dashboard::init() {

        return true;
    }

    // shutdown will be called bevor any system is deinitalized
    bool dashboard::shutdown() {

        return true;
    }


    void dashboard::update(f32 delta_time) {}


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

        // main content
        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();

    }


    void dashboard::on_event(event& event) {}

}
