#pragma once

#include "vulkan/vulkan.h"
#include "../graphics/context.hpp"
#include "../graphics/gpu_buffers.hpp"
#include "../graphics/command_queue.hpp"
#include "../graphics/shader.hpp"
#include "../graphics/render_window.hpp"
#include <vec2.hpp>
#include <array>

struct PushConstBlock {
	glm::vec2 scale;
	glm::vec2 translate;
};

struct ImGuiImpl
{
	// 1 MB in total for imguis vertex data.
	static inline std::uint64_t m_max_vertices = 500000;
	static inline std::uint64_t m_max_indices = 500000;

	VkSampler sampler;
	std::array<gfx::GPUBuffer*, 3> vertexBuffer;
	std::array <gfx::GPUBuffer*, 3> indexBuffer;
	VkDeviceMemory fontMemory = VK_NULL_HANDLE;
	VkImage fontImage = VK_NULL_HANDLE;
	VkImageView fontView = VK_NULL_HANDLE;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	gfx::Context* m_context;
	gfx::Shader* m_vertex_shader;
	PushConstBlock pushConstBlock;
	gfx::Shader* m_fragment_shader;

	~ImGuiImpl();

	void InitImGuiResources(gfx::Context* m_context, gfx::RenderWindow* render_window, gfx::CommandQueue* direct_queue);
	void UpdateBuffers(std::uint32_t frame_idx);
	void Draw(gfx::CommandList* cmd_list, std::uint32_t frame_idx);
};