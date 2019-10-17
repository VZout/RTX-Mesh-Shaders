/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "vk_material_pool.hpp"

#include "../texture_pool.hpp"
#include "gpu_buffers.hpp"
#include "descriptor_heap.hpp"
#include "../util/log.hpp"
#include "context.hpp"
#include "../buffer_definitions.hpp"

gfx::VkMaterialPool::VkMaterialPool(gfx::Context* context)
	: m_context(context),
	m_material_set_layout(VK_NULL_HANDLE),
	m_desc_heap(nullptr)
{
	auto logical_device = context->m_logical_device;

	gfx::DescriptorHeap::Desc desc;
	desc.m_versions = 1;
	desc.m_num_descriptors = 500;
	m_desc_heap = new gfx::DescriptorHeap(m_context, desc);

	// TODO: make this entire layout static and use it when creating root signatures.
	std::vector<VkDescriptorSetLayoutBinding> parameters(1);
	parameters[0].binding = 2; // root parameter 1
	parameters[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	parameters[0].descriptorCount = 3;
	parameters[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	parameters[0].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptor_set_create_info = {};
	descriptor_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_create_info.bindingCount = parameters.size();
	descriptor_set_create_info.pBindings = parameters.data();

	if (vkCreateDescriptorSetLayout(logical_device, &descriptor_set_create_info, nullptr, &m_material_set_layout) != VK_SUCCESS)
	{
		LOGC("failed to create descriptor set layout!");
	}

	parameters[0].binding = 3; // root parameter 1
	parameters[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	parameters[0].descriptorCount = 1;
	parameters[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptor_set_create_info.bindingCount = parameters.size();
	descriptor_set_create_info.pBindings = parameters.data();


	if (vkCreateDescriptorSetLayout(logical_device, &descriptor_set_create_info, nullptr, &m_material_cb_set_layout) != VK_SUCCESS)
	{
		LOGC("failed to create descriptor set layout!");
	}
}

gfx::VkMaterialPool::~VkMaterialPool()
{
	auto logical_device = m_context->m_logical_device;

	vkDestroyDescriptorSetLayout(logical_device, m_material_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(logical_device, m_material_cb_set_layout, nullptr);

	for (auto mat : m_constant_buffers)
	{
		delete mat.second;
	}

	delete m_desc_heap;
}

std::uint32_t gfx::VkMaterialPool::GetDescriptorSetID(MaterialHandle handle)
{
	if (auto it = m_descriptor_sets.find(handle.m_material_id); it != m_descriptor_sets.end())
	{
		return it->second;
	}

	LOGE("Failed to get descriptor set id from material handle");
	return 0;
}

std::uint32_t gfx::VkMaterialPool::GetCBDescriptorSetID(MaterialHandle handle)
{
	if (auto it = m_descriptor_cb_sets.find(handle.m_material_id); it != m_descriptor_cb_sets.end())
	{
		return it->second;
	}

	LOGE("Failed to get constant buffer descriptor set id from material handle");
	return 0;
}

gfx::GPUBuffer* gfx::VkMaterialPool::GetCBBuffer(MaterialHandle handle)
{
	if (auto it = m_constant_buffers.find(handle.m_material_id); it != m_constant_buffers.end())
	{
		return it->second;
	}

	LOGE("Failed to get constant buffer descriptor set id from material handle");
	return nullptr;
}

gfx::DescriptorHeap* gfx::VkMaterialPool::GetDescriptorHeap()
{
	return m_desc_heap;
}

void gfx::VkMaterialPool::Update(MaterialHandle handle, MaterialData const & data)
{
	cb::BasicMaterial material_cb_data;
	material_cb_data.color = glm::vec3(data.m_base_color[0], data.m_base_color[1], data.m_base_color[2]);
	material_cb_data.roughness = data.m_base_roughness;
	material_cb_data.metallic = data.m_base_metallic;
	material_cb_data.reflectivity = data.m_base_reflectivity;
	material_cb_data.anisotropy = data.m_base_anisotropy;
	material_cb_data.anisotropy_dir = data.m_base_anisotropy_dir;
	material_cb_data.normal_strength = data.m_base_normal_strength;
	material_cb_data.clear_coat = data.m_base_clear_coat;
	material_cb_data.clear_coat_roughness = data.m_base_clear_coat_roughness;

	auto buffer = GetCBBuffer(handle);

	buffer->Map();
	buffer->Update(&material_cb_data, sizeof(cb::BasicMaterial));
	buffer->Unmap();
}

void gfx::VkMaterialPool::Load_Impl(MaterialHandle& handle, MaterialData const & data, TexturePool* texture_pool)
{
	gfx::SamplerDesc sampler_desc
	{
		.m_filter = gfx::enums::TextureFilter::FILTER_LINEAR,
		.m_address_mode = gfx::enums::TextureAddressMode::TAM_WRAP,
		.m_border_color = gfx::enums::BorderColor::BORDER_WHITE,
	};

	auto buffer = new gfx::GPUBuffer(m_context, sizeof(cb::BasicMaterial), gfx::enums::BufferUsageFlag::CONSTANT_BUFFER);
	auto descriptor_cb_set_id = m_desc_heap->CreateSRVFromCB(buffer, m_material_cb_set_layout, 3, 0);

	cb::BasicMaterial material_cb_data;
	material_cb_data.color = glm::vec3(data.m_base_color[0], data.m_base_color[1], data.m_base_color[2]);
	material_cb_data.roughness = data.m_base_roughness;
	material_cb_data.metallic = data.m_base_metallic;
	material_cb_data.reflectivity = data.m_base_reflectivity;
	material_cb_data.anisotropy = data.m_base_anisotropy;
	material_cb_data.anisotropy_dir = data.m_base_anisotropy_dir;
	material_cb_data.normal_strength = data.m_base_normal_strength;

	buffer->Map();
	buffer->Update(&material_cb_data, sizeof(cb::BasicMaterial));
	buffer->Unmap();
	m_constant_buffers.insert({ handle.m_material_id, buffer });

	// Textures
	auto textures = texture_pool->GetTextures({ handle.m_albedo_texture_handle, handle.m_normal_texture_handle, handle.m_roughness_texture_handle });
	auto descriptor_set_id = m_desc_heap->CreateSRVSetFromTexture(textures, m_material_set_layout, 2, 0, sampler_desc);
	handle.m_material_set_id = descriptor_set_id;
	handle.m_material_cb_set_id = descriptor_cb_set_id;

	m_descriptor_sets.insert({ handle.m_material_id, descriptor_set_id }); //TODO: Unhardcode this handle (1). We want this to be a global static. see line 25
	m_descriptor_cb_sets.insert({ handle.m_material_id, descriptor_cb_set_id }); //TODO: Unhardcode this handle (1). We want this to be a global static. see line 25
}