#include "root_signature.hpp"

#include "context.hpp"
#include "gfx_settings.hpp"
#include "../util/log.hpp"

gfx::RootSignature::RootSignature(Context* context, Desc desc)
	: m_context(context), m_pipeline_layout(VK_NULL_HANDLE), m_pipeline_layout_info(), m_desc(desc)
{

}

gfx::RootSignature::~RootSignature()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyPipelineLayout(logical_device, m_pipeline_layout, nullptr);
	for (auto& layout : m_descriptor_set_layouts)
	{
		vkDestroyDescriptorSetLayout(logical_device, layout, nullptr);
	}
}

void gfx::RootSignature::Compile()
{
	auto logical_device = m_context->m_logical_device;

	m_descriptor_set_layouts.resize(m_desc.m_parameters.size());

	for (auto& layout : m_descriptor_set_layouts)
	{
		VkDescriptorSetLayoutCreateInfo descriptor_set_create_info = {};
		descriptor_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_set_create_info.bindingCount = m_desc.m_parameters.size();
		descriptor_set_create_info.pBindings = m_desc.m_parameters.data();

		if (vkCreateDescriptorSetLayout(logical_device, &descriptor_set_create_info, nullptr, &layout) != VK_SUCCESS)
		{
			LOGC("failed to create descriptor set layout!");
		}
	}

	m_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_pipeline_layout_info.setLayoutCount = m_descriptor_set_layouts.size();
	m_pipeline_layout_info.pSetLayouts = m_descriptor_set_layouts.data();
	m_pipeline_layout_info.pushConstantRangeCount = 0;
	m_pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(logical_device, &m_pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
	{
		LOGC("failed to create pipeline layout!");
	}
}