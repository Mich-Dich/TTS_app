#include "util/pch.h"

#if defined(RENDER_API_VULKAN)
    #include "application.h"
    #include "engine/render/vulkan/vk_types.h"
    #include "engine/render/vulkan/vk_renderer.h"
#elif defined(RENDER_API_OPENGL)
    #if defined(PLATFORM_LINUX)
        #include <GL/glew.h>
    #elif defined(PLATFORM_WINDOWS)
        #include <GL/glew.h>
    #endif
#endif

#include <glm/glm.hpp>

#include "buffer.h"



namespace AT::render {

#if defined(RENDER_API_OPENGL)

    GLenum gl_usage(const buffer::usage usage) {
        switch(usage) {
            case buffer::usage::STATIC: return GL_STATIC_DRAW;
            case buffer::usage::DYNAMIC: return GL_DYNAMIC_DRAW;
            case buffer::usage::STREAM: return GL_STREAM_DRAW;
            default: return GL_STATIC_DRAW;
        }
    }


    buffer::buffer(type type, usage usage, const void* data, size_t size)
        : m_type(type), m_usage(usage) {

        if (m_ID)
            glDeleteBuffers(1, &m_ID);
        
        glGenBuffers(1, &m_ID);

        {   // bind
            const GLenum target = m_type == buffer::type::VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
            glBindBuffer(target, m_ID);
        }

        switch(m_type) {
            case type::VERTEX:      glBufferData(GL_ARRAY_BUFFER, size, data, gl_usage(m_usage)); break;
            case type::INDEX:       glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, gl_usage(m_usage)); break;
            default: break;
        }
        
        {   // unbind
            const GLenum target = m_type == buffer::type::VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
            glBindBuffer(target, 0);
        }

        m_size = size;
    }
    
    buffer::~buffer() {
        if (m_ID)
            glDeleteBuffers(1, &m_ID);
    }

#elif defined(RENDER_API_VULKAN)

    buffer::buffer(VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, size_t alloc_size, void* data) {
        
        VkBufferCreateInfo buffer_CI = {};
        buffer_CI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_CI.pNext = nullptr;
        buffer_CI.size = alloc_size;
        buffer_CI.usage = usage;
        
        VmaAllocationCreateInfo vmaalloc_CI = {};
        vmaalloc_CI.usage = memoryUsage;
        vmaalloc_CI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        VK_CHECK_S(vmaCreateBuffer(GET_RENDERER_CAST_TO_API->get_allocator(), &buffer_CI, &vmaalloc_CI, &m_buffer, &m_allocation, &m_info));
        memcpy(m_info.pMappedData, data, alloc_size);
        
    }

    buffer::~buffer() {

        vmaDestroyBuffer(GET_RENDERER_CAST_TO_API->get_allocator(), m_buffer, m_allocation);
    }

#endif

}