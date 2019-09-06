#include "root_signature.hpp"

#include "context.hpp"

gfx::RootSignature::RootSignature(Context* context)
	: m_context(context), m_pipeline_layout(VK_NULL_HANDLE), m_pipeline_layout_info()
{

}

gfx::RootSignature::~RootSignature()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyPipelineLayout(logical_device, m_pipeline_layout, nullptr);
}

void gfx::RootSignature::Compile()
{
	auto logical_device = m_context->m_logical_device;

	m_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_pipeline_layout_info.setLayoutCount = 0;
	m_pipeline_layout_info.pSetLayouts = nullptr;
	m_pipeline_layout_info.pushConstantRangeCount = 0;
	m_pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(logical_device, &m_pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
}