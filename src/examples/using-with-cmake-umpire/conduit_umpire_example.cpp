// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_umpire_example.cpp
///
//-----------------------------------------------------------------------------

#include <iostream>

#include "conduit.hpp"
#include "conduit_relay.hpp"
#include "conduit_blueprint.hpp"

#include "umpire/ResourceManager.hpp"
#include "umpire/Allocator.hpp"
#include "umpire/strategy/AlignedAllocator.hpp"
#include "umpire/strategy/DynamicPoolList.hpp"
#include "umpire/strategy/QuickPool.hpp"
#include "umpire/strategy/ThreadSafeAllocator.hpp"


class UmpireConduitAllocatorAdapter
{
public:
    static void memcpy(void* dst, const void* src, size_t size)
    {
        // Evil doings to match the Umpire API:
        void* unconst_src = const_cast<void*>(src);
        umpire::ResourceManager::getInstance().copy(dst, unconst_src, size);
    }

    static void memset(void* dst, int val, size_t size)
    {
        umpire::ResourceManager::getInstance().memset(dst, val, size);
    }

    conduit::index_t registerUmpireConduitAllocator(int umpireId)
    {
        conduit::index_t retval;
        if (m_allocators.count(umpireId) > 0)
        {
            retval = m_allocators[umpireId];
        }
        else
        {
            conduit::utils::handle_alloc_type the_allocator =
                [umpireId](size_t items, size_t size) 
                { 
                    return allocate_pointer(umpireId, items, size); 
                };
            conduit::utils::handle_free_type the_deallocator =
                [](void* ptr) { free_pointer(ptr); };
            retval = conduit::utils::register_allocator(the_allocator, the_deallocator);
            m_allocators[umpireId] = retval;
        }

        return retval;
    }
    
private:
    std::map<int, conduit::index_t> m_allocators;

    static void* allocate_pointer(int umpireId, size_t items, size_t item_size)
    {
        // Assume that the requested allocator exists
        auto allocator = umpire::ResourceManager::getInstance().getAllocator(umpireId);
        size_t bytes = items * item_size;
        return allocator.allocate(bytes);
        // May need to zero out the new allocation before returning the pointer.
    }

    static void free_pointer(void * ptr)
    {
        umpire::ResourceManager::getInstance().deallocate(ptr);
    }
};

int main(int argc, char **argv)
{
    // Hello from Conduit
    conduit::Node about;
    conduit::about(about["conduit"]);
    conduit::relay::about(about["conduit/relay"]);
    conduit::relay::io::about(about["conduit/relay/io"]);
    conduit::blueprint::about(about["conduit/blueprint"]);

    std::cout << about.to_yaml() << std::endl;

    // Hello from Umpire
    auto& rm = umpire::ResourceManager::getInstance();
    umpire::Allocator alloc = rm.getAllocator("HOST");

    std::cout << "Got allocator: " << alloc.getName() << std::endl;

    std::cout << "Available allocators: ";
    for (auto s : rm.getAllocatorNames()){
        std::cout << s << "  ";
    }
    std::cout << std::endl;

    // Now show how to use Conduit allocators with the adapter class.
    // Here are some new Allocators, copied from Umpire test pool_allocator_stress.cpp.
    const bool run_quick{ true };
    const bool run_list{ true };
    const std::size_t one_megabyte{ 1024 * 1024 };
    const std::size_t one_gigabyte{ one_megabyte * 1024 };
    const std::size_t allocation_alignment{ 64 };
    const std::size_t initial_pool_size{ 12ull * one_gigabyte };
    const std::size_t subsequent_pool_increments{ 1024 };
    auto quick_allocation_pool = rm.makeAllocator<umpire::strategy::QuickPool>(
        "HOST_quick_pool", rm.getAllocator("HOST"), initial_pool_size, subsequent_pool_increments);
    auto quick_aligned_allocator = rm.makeAllocator<umpire::strategy::AlignedAllocator>(
        "HOST_quick_aligned", quick_allocation_pool, allocation_alignment);
    auto quick_alloc =
        rm.makeAllocator<umpire::strategy::ThreadSafeAllocator>("HOST_quick_safe_pool", quick_aligned_allocator);

    // Conduit Nodes can use these allocators:
    conduit::utils::set_memcpy_handler(UmpireConduitAllocatorAdapter::memcpy);
    conduit::utils::set_memset_handler(UmpireConduitAllocatorAdapter::memset);
    UmpireConduitAllocatorAdapter ucaa;
    conduit::index_t qap_id = ucaa.registerUmpireConduitAllocator(quick_allocation_pool.getId());
    conduit::index_t qaa_id = ucaa.registerUmpireConduitAllocator(quick_aligned_allocator.getId());
    conduit::index_t qa_id  = ucaa.registerUmpireConduitAllocator(quick_alloc.getId());

    conduit::Node theroot;  // This has the default Conduit allocator.
    conduit::Node& first = theroot["first"];
    first.set_allocator(qap_id);  // Now all Node children of first will use the quick_allocation_pool.
    conduit::Node& second = theroot["second"];
    second.set_allocator(qaa_id);
    conduit::Node& third = theroot["third"];
    third.set_allocator(qa_id);

    return 0;
}


