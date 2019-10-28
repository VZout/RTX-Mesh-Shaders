/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

#include <vector>

#include "resource_loader.hpp"
#include "resource_structs.hpp"

namespace gfx
{
	class CommandList;
	class StagingTexture;
}

struct ConstantBufferHandle
{
	std::uint32_t m_cb_id;
	std::uint32_t m_cb_set_id;
};

class ConstantBufferPool
{
public:
	ConstantBufferPool();
	virtual ~ConstantBufferPool() = default;

	ConstantBufferHandle Allocate(std::uint64_t size);
	virtual std::vector<std::uint32_t> CreateConstantBufferSet(std::vector<ConstantBufferHandle> handles) = 0;
	virtual void Update(ConstantBufferHandle handle, std::uint64_t size, void* data, std::uint32_t frame_idx, std::uint64_t offset = 0) = 0;

private:
	virtual void Allocate_Impl(ConstantBufferHandle& handle, std::uint64_t size) = 0;

	std::uint32_t m_next_id;
};
