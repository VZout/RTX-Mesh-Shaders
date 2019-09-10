#pragma once

#include "vulkan/vulkan.h"
#include "../graphics/context.hpp"
#include "../graphics/gpu_buffers.hpp"
#include "../graphics/command_queue.hpp"
#include "../graphics/shader.hpp"
#include "../graphics/render_window.hpp"
#include <vec2.hpp>

struct PushConstBlock {
	glm::vec2 scale;
	glm::vec2 translate;
};

struct ImGuiImpl
{
	VkSampler sampler;
	gfx::GPUBuffer* vertexBuffer;
	gfx::GPUBuffer* indexBuffer;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;
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
	void UpdateBuffers();
	void Draw(gfx::CommandList* cmd_list, std::uint32_t frame_idx);
};