// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_unified_memory.cpp
///
//-----------------------------------------------------------------------------

// Prints debug info about how this machine handles host and device memory
// (device attributes, XNACK, how Conduit classifies pointers), then checks
// that a kernel can round trip data allocated with the host allocator.

#include "conduit.hpp"
#include "conduit_execution.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>
#include "gtest/gtest.h"

#if defined(CONDUIT_USE_CUDA)
#include <cuda_runtime.h>
#elif defined(CONDUIT_USE_HIP)
#include <hip/hip_runtime.h>
#endif

using namespace conduit;
using conduit::execution::ExecutionPolicy;

//-----------------------------------------------------------------------------
void
print_device_info()
{
    const char *hsa_xnack = std::getenv("HSA_XNACK");
    std::cout << "HSA_XNACK: " << (hsa_xnack ? hsa_xnack : "(unset)") << std::endl;
    // GPU aware MPI is required before MPI can touch device memory
    const char *gpu_mpi = std::getenv("MPICH_GPU_SUPPORT_ENABLED");
    std::cout << "MPICH_GPU_SUPPORT_ENABLED: " << (gpu_mpi ? gpu_mpi : "(unset)")
              << std::endl;

#if defined(CONDUIT_USE_CUDA) || defined(CONDUIT_USE_HIP)
    int device = 0;
    int value = -1;
#if defined(CONDUIT_USE_CUDA)
    cudaGetDevice(&device);
    cudaDeviceProp props;
    cudaGetDeviceProperties(&props, device);
    std::cout << "device: " << props.name << std::endl;
    #define PRINT_ATTR(attr) \
        value = -1; \
        cudaDeviceGetAttribute(&value, cudaDevAttr##attr, device); \
        std::cout << #attr << ": " << value << std::endl;
    PRINT_ATTR(Integrated)
    PRINT_ATTR(ManagedMemory)
    PRINT_ATTR(ConcurrentManagedAccess)
    PRINT_ATTR(PageableMemoryAccess)
    PRINT_ATTR(PageableMemoryAccessUsesHostPageTables)
    #undef PRINT_ATTR
#else
    hipGetDevice(&device);
    hipDeviceProp_t props;
    hipGetDeviceProperties(&props, device);
    std::cout << "device: " << props.name << " (" << props.gcnArchName << ")"
              << std::endl;
    #define PRINT_ATTR(attr) \
        value = -1; \
        hipDeviceGetAttribute(&value, hipDeviceAttribute##attr, device); \
        std::cout << #attr << ": " << value << std::endl;
    PRINT_ATTR(Integrated)
    PRINT_ATTR(ManagedMemory)
    PRINT_ATTR(ConcurrentManagedAccess)
    PRINT_ATTR(PageableMemoryAccess)
    PRINT_ATTR(PageableMemoryAccessUsesHostPageTables)
    #undef PRINT_ATTR
#endif
#else
    std::cout << "device: none (conduit built without CUDA/HIP)" << std::endl;
#endif

    std::cout << "DeviceMemory::unified(): "
              << execution::DeviceMemory::unified() << std::endl;
}

//-----------------------------------------------------------------------------
void
print_pointer_classification()
{
    std::vector<float64> vec(16);
    std::cout << "is_device_ptr(std::vector): "
              << execution::DeviceMemory::is_device_ptr(vec.data()) << std::endl;

    Node host_node;
    host_node.set_allocator(execution::get_host_allocator_id());
    host_node.set(vec);
    std::cout << "is_device_ptr(host allocator node): "
              << execution::DeviceMemory::is_device_ptr(host_node.data_ptr())
              << std::endl;

#if defined(CONDUIT_USE_DEVICE)
    Node device_node;
    device_node.set_allocator(execution::get_device_allocator_id());
    device_node.set(vec);
    std::cout << "is_device_ptr(device allocator node): "
              << execution::DeviceMemory::is_device_ptr(device_node.data_ptr())
              << std::endl;

    void *managed = nullptr;
#if defined(CONDUIT_USE_CUDA)
    if (cudaMallocManaged(&managed, 64) == cudaSuccess)
    {
        std::cout << "is_device_ptr(cudaMallocManaged): "
                  << execution::DeviceMemory::is_device_ptr(managed) << std::endl;
        cudaFree(managed);
    }
#else
    if (hipMallocManaged(&managed, 64) == hipSuccess)
    {
        std::cout << "is_device_ptr(hipMallocManaged): "
                  << execution::DeviceMemory::is_device_ptr(managed) << std::endl;
        hipFree(managed);
    }
#endif
#endif // defined(CONDUIT_USE_DEVICE)
}

//-----------------------------------------------------------------------------
TEST(conduit_unified_memory, report)
{
    execution::init_device_memory_handlers();

    print_device_info();
    print_pointer_classification();

    Node opts;
    execution::execution_options(opts);
    std::cout << "execution options:\n" << opts.to_yaml() << std::endl;
}

//-----------------------------------------------------------------------------
// Data allocated with the host allocator, executed with the device policy,
// read back on the host.
void
run_host_allocator_round_trip()
{
    const index_t n = 1024;
    std::vector<float64> src_vals(n);
    for (index_t i = 0; i < n; i++)
    {
        src_vals[i] = static_cast<float64>(i + 1);
    }

    Node node;
    node["src"].set_allocator(execution::get_host_allocator_id());
    node["des"].set_allocator(execution::get_host_allocator_id());
    node["src"].set(src_vals);
    node["des"].set(DataType::float64(n));

    ExecutionPolicy policy = ExecutionPolicy::device();
    float64_accessor src(node["src"]);
    float64_accessor des(node["des"]);
    src.use_with(policy);
    des.use_with(policy);

    // use_with() points the accessor at a working buffer when it had to
    // stage a copy, and at the node's own buffer otherwise
    const bool staged = des.element_ptr(0) != node["des"].data_ptr();
    std::cout << "des staged through a working buffer: " << staged << std::endl;
    if (execution::DeviceMemory::unified())
    {
        EXPECT_FALSE(staged);
    }

    execution::forall(policy, 0, n, [src, des] CONDUIT_EXEC(index_t i)
    {
        des.set(i, 2.0 * src[i]);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    des.sync();

    float64_accessor result(node["des"]);
    result.use_with(ExecutionPolicy::host());
    for (index_t i = 0; i < n; i++)
    {
        EXPECT_EQ(result[i], 2.0 * src_vals[i]);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_unified_memory, host_allocator_round_trip)
{
    execution::init_device_memory_handlers();

    if (!ExecutionPolicy::is_device_enabled())
    {
        std::cout << "no device support, skipping" << std::endl;
        return;
    }

    run_host_allocator_round_trip();
}
