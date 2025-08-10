#pragma once

#if defined(RENDER_API_VULKAN)
    #include "engine/render/vulkan/vk_types.h"
#endif

namespace AT::render {
    
    // Base buffer interface
    class buffer {
    public:
    
    #if defined(RENDER_API_OPENGL)
        enum class type { VERTEX, INDEX };
        enum class usage { STATIC, DYNAMIC, STREAM };
        buffer(type type, usage usage, const void* data, size_t size);

        DEFAULT_GETTER_SETTER(u32,  ID)
        DEFAULT_GETTER(type,        type)

    #elif defined(RENDER_API_VULKAN)
        buffer() = default;
        buffer(VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, size_t alloc_size, void* data);        // constructor for empty buffer
        
        DEFAULT_GETTER(VkBuffer,            buffer)
        DEFAULT_GETTER_REF(VkBuffer,        buffer)

    #endif
        ~buffer();
            
        DEFAULT_GETTER(size_t,              size)

    protected:
        size_t                              m_size = 0;
    
    #if defined(RENDER_API_OPENGL)
        type                                m_type;
        usage                               m_usage;
        u32                                 m_ID = 0;

    #elif defined(RENDER_API_VULKAN)
        VkBuffer                            m_buffer;
        VmaAllocation                       m_allocation;
        VmaAllocationInfo                   m_info;

    #endif
    
    };

}
