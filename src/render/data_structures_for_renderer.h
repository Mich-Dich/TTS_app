#pragma once

#include "util/pch.h"


namespace AT {

    struct mesh_data {
        u32     vertex_offset;         // Offset in global vertex buffer
        u32     index_offset;          // Offset in global index buffer
        u32     bvh_offset;            // Offset in global BVH buffer
        u32     triidx_offset;         // Offset in global triangle index buffer
    };

    struct BVH_debug_visualization_settings {
        
        u32 max_depth = 3;
        u32 debug_method = 0;         // 0: Show no debug info, 1: Show bounding boxes, 2: Show box tests, 3: Show triangle
        glm::vec4 color{0, 0, 1, 1};
    };

    #pragma pack(push, 1)
    struct mesh_instance {
        glm::mat4   transform;                      // Model-to-world matrix
        glm::mat4   inv_transform;                  // World-to-model matrix (inverse)      // mesh_index is stored in inv_transform[3][3]
        u64         albedo_texture_pointer;         // [21: texture size weight  21: texture size height  11: array to use   11: index in array]
        u64         normal_texture_pointer;         // [21: texture size weight  21: texture size height  11: array to use   11: index in array]
    };
    #pragma pack(pop)

    namespace render {

        struct general_performance_metrik {

            u32 meshes = 0, mesh_instances = 0, draw_calls = 0, material_binding_count = 0, pipline_binding_count = 0;
            u64 vertices = 0;
            f32 sleep_time = 0.f, work_time = 0.f;
            #define GENERAL_PERFORMANCE_METRIK_ARRAY_SIZE 200

            FORCEINLINE u32 get_array_size() { return (u32)GENERAL_PERFORMANCE_METRIK_ARRAY_SIZE; }

            f32 renderer_draw_time[GENERAL_PERFORMANCE_METRIK_ARRAY_SIZE] = {};
            f32 draw_geometry_time[GENERAL_PERFORMANCE_METRIK_ARRAY_SIZE] = {};
            f32 waiting_idle_time[GENERAL_PERFORMANCE_METRIK_ARRAY_SIZE] = {};
            u16 current_index = 0;

            void next_iteration() {

                current_index = (current_index + 1) % GENERAL_PERFORMANCE_METRIK_ARRAY_SIZE;
                // renderer_draw_time[current_index] = draw_geometry_time[current_index] = waiting_idle_time[current_index] = 0.f;
                material_binding_count = pipline_binding_count = draw_calls = 0;
                vertices = 0;
                sleep_time = work_time = 0.f;
            }
        };
    };

}
