/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#include "constant_buffer_pool.hpp"

ConstantBufferPool::ConstantBufferPool()
	: m_next_id(0)
{

}

ConstantBufferHandle ConstantBufferPool::Allocate(std::uint64_t size)
{
	auto new_id = m_next_id;

	ConstantBufferHandle handle;
	handle.m_cb_id = new_id;

	Allocate_Impl(handle, size);
	
	m_next_id++;
	return handle;
}
