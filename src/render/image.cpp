#include "util/pch.h"
#include <imgui.h>

// Include Windows headers first for Windows platform
#if defined(PLATFORM_WINDOWS)
    #include <Windows.h>
#endif

#if defined(RENDER_API_VULKAN)
	#include "application.h"
    #include <vulkan/vulkan.h>
    #include "vulkan/vk_image.h"
    #include "vulkan/vk_initializers.h"
    #include "imgui_impl_vulkan.h"
    #include "engine/render/vulkan/vk_types.h"
    #include "engine/render/vulkan/vk_renderer.h"
#elif defined(RENDER_API_OPENGL)
    #if defined(PLATFORM_LINUX)
        #include <GL/glew.h>
    #elif defined(PLATFORM_WINDOWS)
        #include <GL/glew.h>
    #endif
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "image.h"

namespace AT {

    namespace util {
        u32 bytes_per_pixel(image_format format) {
            switch (format) {
                case image_format::RGBA:    return 4;
                case image_format::RGBA32F: return 16;
                default: return 0;
            }
        }

#if defined(RENDER_API_VULKAN)

		// static uint32_t get_vulkan_memory_type(VkMemoryPropertyFlags properties, u32 type_bits) {
			
		// 	VkPhysicalDeviceMemoryProperties prop;
		// 	vkGetPhysicalDeviceMemoryProperties(GET_RENDERER_CAST_TO_API->get_chosenGPU(), &prop);
		// 	for (uint32_t i = 0; i < prop.memoryTypeCount; i++) {
		// 		if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
		// 			return i;
		// 	}

		// 	return 0xffffffff;
		// }

		static VkFormat image_format_to_vulkan_vormat(image_format format) {
		
			switch (format) {
				case image_format::RGBA:    return VK_FORMAT_R8G8B8A8_UNORM;
				case image_format::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
				default: return (VkFormat)0;
			}
		}

#elif defined(RENDER_API_OPENGL)
        GLenum to_gl_format(image_format format) {
            switch (format) {
                case image_format::RGBA:    return GL_RGBA;
                case image_format::RGBA32F: return GL_RGBA32F;
                default: return 0;
            }
        }
		
		GLenum to_gl_internal_format(image_format format) {
			switch (format) {
				case image_format::RGBA:    return GL_RGBA8;
				case image_format::RGBA32F: return GL_RGBA32F;
				default: return GL_RGBA8;
			}
		}

		GLenum to_gl_base_format(image_format format) {
			switch (format) {
				case image_format::RGBA:    return GL_RGBA;
				case image_format::RGBA32F: return GL_RGBA;
				default: return GL_RGBA;
			}
		}

		GLenum to_gl_data_format(image_format format) {
			switch (format) {
				case image_format::RGBA:    return GL_UNSIGNED_BYTE;
				case image_format::RGBA32F: return GL_FLOAT;
				default: return GL_UNSIGNED_BYTE;
			}
		}
#endif
    }

    image::~image() {
		
		// LOG_SHUTDOWN
		release();
    }

    // Common methods
    u32 image::get_width() const { return m_image_extent.width; }

    u32 image::get_height() const { return m_image_extent.height; }

    ImTextureID image::get() {
#if defined(RENDER_API_VULKAN)
		if (m_descriptor_set == nullptr)
			m_descriptor_set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(GET_RENDERER_CAST_TO_API->get_default_sampler_linear(), m_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return reinterpret_cast<void*>(m_descriptor_set);

#elif defined(RENDER_API_OPENGL)
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_textureID));
#endif
    }

#if defined(RENDER_API_VULKAN)
    
	image::image(void* data, extent_3D size, image_format format, bool mipmapped) {

		allocate_memory(data, size, format, mipmapped);
	}

	image::image(void* data, u32 width, u32 height, image_format format, bool mipmapped) {

		allocate_memory(data, extent_3D{width, height, 1}, format, mipmapped);
	}

	image::image(std::filesystem::path image_path, image_format format, bool mipmapped) {

		int channels;
		int width = 0, height = 0;
		void* data = stbi_load(image_path.string().c_str(), &width, &height, &channels, 4);
		VALIDATE(data != nullptr, return, "", "Could not load image from path [" << image_path.generic_string() << "]")
		allocate_memory(data, extent_3D{ (u32)width, (u32)height, 1 }, format, mipmapped);
		stbi_image_free(data);
	}

	void image::allocate_memory(void* data, extent_3D size, image_format format, bool mipmapped, VkImageUsageFlags usage) {

		size_t data_size = size.depth * size.width * size.height * util::bytes_per_pixel(format);
		render::buffer upload_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, data_size, data);		// stack buffer, dies with stack-frame

		allocate_image(size, util::image_format_to_vulkan_vormat(format), usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);
		GET_RENDERER_CAST_TO_API->immediate_submit([&](VkCommandBuffer cmd) {

			render::vulkan::util::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent.width = size.width;
			copyRegion.imageExtent.height = size.height;
			copyRegion.imageExtent.depth = size.depth;

			vkCmdCopyBufferToImage(cmd, upload_buffer.get_buffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
			render::vulkan::util::transition_image(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

		m_is_initalized = true;
	}

	void image::allocate_image(extent_3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {

		set_image_format(format);
		set_image_extent(size);

		VkImageCreateInfo img_info = render::vulkan::init::image_create_info(format, usage, size);
		if (mipmapped)
			img_info.mipLevels = static_cast<u32>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;

		// always allocate images on dedicated GPU memory
		VmaAllocationCreateInfo allocinfo = {};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_S(vmaCreateImage(GET_RENDERER_CAST_TO_API->get_allocator(), &img_info, &allocinfo, &m_image, &m_allocation, nullptr));

		// if the format is a depth format, we will need to have it use the correct aspect flag
		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (format == VK_FORMAT_D32_SFLOAT)
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;

		// build a image-view for the image
		VkImageViewCreateInfo view_info = render::vulkan::init::imageview_create_info(format, m_image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;
		VK_CHECK_S(vkCreateImageView(GET_RENDERER_CAST_TO_API->get_device(), &view_info, nullptr, &m_image_view));

		m_is_initalized = true;
	}

	void image::release() {

		if (m_is_initalized) {

			GET_RENDERER_CAST_TO_API->submit_resource_free([image = m_image, image_view = m_image_view, allocation = m_allocation, descriptor_set = m_descriptor_set] {

				vkDestroyImageView(GET_RENDERER_CAST_TO_API->get_device(), image_view, nullptr);
				vmaDestroyImage(GET_RENDERER_CAST_TO_API->get_allocator(), image, allocation);

				if (descriptor_set != nullptr)
					ImGui_ImplVulkan_RemoveTexture(descriptor_set);
			});

			m_image = VK_NULL_HANDLE;
			m_image_view = VK_NULL_HANDLE;
			m_descriptor_set = VK_NULL_HANDLE;
			m_allocation = nullptr;
			m_is_initalized = false;
		}
	}

	VkDescriptorSet image::generate_descriptor_set(VkSampler sampler, VkImageLayout layout) {

		return m_descriptor_set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(sampler, m_image_view, layout);
	}

#elif defined(RENDER_API_OPENGL)

    image::image(void* data, extent_3D size, image_format format, bool mipmapped) {
        allocate_memory(data, size, format, mipmapped);
    }

    image::image(void* data, u32 width, u32 height, image_format format, bool mipmapped) {
        allocate_memory(data, extent_3D{width, height, 1}, format, mipmapped);
    }

    image::image(std::filesystem::path image_path, image_format format, bool mipmapped) {
        int width, height, channels;
        stbi_uc* data = stbi_load(image_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (data) {
            allocate_memory(data, extent_3D{(u32)width, (u32)height, 1}, format, mipmapped);
            stbi_image_free(data);
        }
    }

	void image::allocate_memory(void* data, extent_3D size, image_format format, bool mipmapped) {

		m_image_extent.width = size.width;
		m_image_extent.height = size.height;
		m_format = format;
		m_mipmapped = mipmapped;

		glGenTextures(1, &m_textureID);
		glBindTexture(GL_TEXTURE_2D, m_textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLint alignment = 1;
		if (m_image_extent.width % 8 == 0) 				alignment = 8;
		else if (m_image_extent.width % 4 == 0) 			alignment = 4;
		else if (m_image_extent.width % 2 == 0) 			alignment = 2;

		glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
		glTexImage2D(GL_TEXTURE_2D, 0, util::to_gl_internal_format(format), m_image_extent.width, m_image_extent.height, 0, util::to_gl_base_format(format), util::to_gl_data_format(format), data );			// Upload texture data

		if (mipmapped) {																			// Generate mipmaps only if requested
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		m_is_initialized = true;

		GLenum err = glGetError();
		VALIDATE(err == GL_NO_ERROR, , "", "OpenGL error [" << err << "]")
    }
	
    void image::release() {
     
		if (m_is_initialized) {

			if (m_bindless_handle) {

				// gldestoyBindlesHandle
			}

            glDeleteTextures(1, &m_textureID);
            m_textureID = 0;
            m_is_initialized = false;
        }
    }

	void image::create_bindless_handel() {

		if (!m_bindless_handle) {

			m_bindless_handle = glGetTextureHandleARB(m_textureID);
			glMakeTextureHandleResidentARB(m_bindless_handle);
		}
	}


#endif

    // Static utilities (common for both APIs)
    void* image::decode(const void* data, u64 length, u32& outWidth, u32& outHeight) {

        int width, height, channels;
        stbi_uc* buffer = stbi_load_from_memory(
            static_cast<const stbi_uc*>(data), 
            static_cast<int>(length), 
            &width, &height, &channels, STBI_rgb_alpha
        );
        if (buffer) {
            outWidth = static_cast<u32>(width);
            outHeight = static_cast<u32>(height);
        }
        return buffer;
    }

}
