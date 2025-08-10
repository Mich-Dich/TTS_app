#pragma once

#include "util/core_config.h"
#include "imgui.h"

#if defined(RENDER_API_VULKAN)
    #include "engine/render/vulkan/vk_types.h"
#elif defined(RENDER_API_OPENGL)
    #include <GL/glew.h>
#endif

struct extent_3D { u32 width, height, depth; };

namespace AT {
    
    enum class image_format {
        None = 0,
        RGBA,
        RGBA32F
    };

    class image {
    public:
        // --------- Common interface ---------
        image() = default;
        image(void* data, extent_3D size, image_format format, bool mipmapped = false);
        image(void* data, u32 width, u32 height, image_format format, bool mipmapped = false);
        image(std::filesystem::path image_path, image_format format, bool mipmapped = false);
        ~image();

        u32 get_width() const;
        u32 get_height() const;
        void release();

        ImTextureID get();                                          // Unified texture access for ImGui

#if defined(RENDER_API_VULKAN)

        DEFAULT_GETTER_SETTER_ALL(VkImage,          image);
        DEFAULT_GETTER_SETTER_ALL(VkImageView,      image_view);
        DEFAULT_GETTER_SETTER_ALL(VmaAllocation,    allocation);
        DEFAULT_GETTER_SETTER_ALL(extent_3D,        image_extent);
        DEFAULT_GETTER_SETTER_ALL(VkFormat,         image_format);
        VkDescriptorSet generate_descriptor_set(VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

#elif defined(RENDER_API_OPENGL)

        DEFAULT_GETTER_C(GLuint,                    textureID)
        DEFAULT_GETTER_C(GLuint64,                  bindless_handle)
        void create_bindless_handel();
        
#endif

        // --------- Common utilities ---------
        static void* decode(const void* data, u64 length, u32& outWidth, u32& outHeight);

    private:
    
        extent_3D                                   m_image_extent{};
        bool                                        m_is_initialized = false;
        bool                                        m_mipmapped = false;

#if defined(RENDER_API_VULKAN)
    
        void allocate_memory(void* data, extent_3D size, image_format format, bool mipmapped, VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);
        void allocate_image(extent_3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);

        bool                                        m_is_initalized = false;
        VkImage                                     m_image = VK_NULL_HANDLE;
        VkImageView                                 m_image_view = VK_NULL_HANDLE;
        VmaAllocation                               m_allocation = nullptr;
        VkFormat                                    m_image_format{};
        VkDescriptorSet                             m_descriptor_set = VK_NULL_HANDLE;

#elif defined(RENDER_API_OPENGL)

        void allocate_memory(void* data, extent_3D size, image_format format, bool mipmapped);
        
        GLuint                                      m_textureID = 0;
        GLuint64                                    m_bindless_handle = 0;
        image_format                                m_format = image_format::None;
#endif
    };
}
