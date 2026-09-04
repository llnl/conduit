// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_memory_manager.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_MEMORY_MANAGER_HPP
#define CONDUIT_MEMORY_MANAGER_HPP

#include <cstddef>

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit_config.h"
#include "conduit_exports.h"

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::execution --
//-----------------------------------------------------------------------------
namespace execution
{

///
/// Interfaces for host and device memory allocation / deallocation.
///

//-----------------------------------------------------------------------------
/// Host Memory allocation / deallocation interface (singleton)
///  Uses a Conduit-owned Umpire host allocator when Umpire is enabled,
///  Uses malloc/free when Umpire is disabled. 
//-----------------------------------------------------------------------------
struct CONDUIT_API HostMemory
{
    static void *allocate(size_t bytes);
    static void *allocate(size_t items, size_t item_size);
    static void  deallocate(void *data_ptr);

private:
    static size_t m_total_bytes_alloced;
    static size_t m_alloc_count;
    static size_t m_free_count;

};
//-----------------------------------------------------------------------------
/// Device Memory allocation / deallocation interface (singleton)
///  Uses a Conduit-owned Umpire device allocator when Umpire is enabled.
///  allocate() and deallocate() throw errors when Umpire is disabled.
//-----------------------------------------------------------------------------
struct CONDUIT_API DeviceMemory
{
    static void *allocate(size_t bytes);
    static void *allocate(size_t items, size_t item_size);
    static void  deallocate(void *data_ptr);
    // whether the device can access ptr
    static bool  is_device_ptr(const void *ptr);
    static void  is_device_ptr(const void *ptr, bool &is_gpu, bool &is_unified);
    // whether ptr was allocated by the device runtime
    static bool  is_device_allocation(const void *ptr);
    // whether the host and device share a memory space
    static bool  unified();

private:
    static size_t m_total_bytes_alloced;
    static size_t m_alloc_count;
    static size_t m_free_count;

};

//-----------------------------------------------------------------------------
struct CONDUIT_API MagicMemory
{
    static void set(void *ptr, int value, size_t num);
    static void copy(void *destination, const void *source, size_t num);
};

}
//-----------------------------------------------------------------------------
// -- end conduit::execution --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
