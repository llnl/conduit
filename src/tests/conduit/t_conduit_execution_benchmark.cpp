// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_execution_benchmark.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_annotations.hpp"
#include "conduit_benchmark.hpp"
#include "conduit_execution.hpp"
#include "conduit_execution_policy.hpp"
#include "conduit_memory_manager.hpp"

#include <vector>

#include "gtest/gtest.h"

using namespace conduit;
using EP = execution::ExecutionPolicy;

index_t BENCHMARK_ARRAY_SIZE            = 1024;
index_t BENCHMARK_NUM_WARMUP_ITERATIONS = 10;
index_t BENCHMARK_NUM_ITERATIONS        = 100;

//-----------------------------------------------------------------------------
// Atomic benchmarks

//-----------------------------------------------------------------------------
template <typename T, typename AtomicOp>
void
atomic_benchmark(const std::string& name,
                 EP policy,
                 benchmark::FillMode mode,
                 AtomicOp&& atomic_op)
{
    const std::string annotated_name = benchmark::get_annotated_name<T>(name, mode);
    CONDUIT_ANNOTATE_MARK_BEGIN(annotated_name.c_str());

    const index_t size = BENCHMARK_ARRAY_SIZE;
    const index_t size_bytes = static_cast<index_t>(sizeof(T)) * size;

    std::vector<uint8_t> host_data;
    benchmark::make_data(benchmark::dtype_of<T>(), size, host_data, mode);
    T* vals_ptr = nullptr;

    CONDUIT_ANNOTATE_MARK_BEGIN("allocate");
    if (policy.is_device_policy())
    {
        vals_ptr = static_cast<T*>(execution::DeviceMemory::allocate(size_bytes));
    }
    else // if (!policy.is_device_policy())
    {
        vals_ptr = static_cast<T*>(execution::HostMemory::allocate(size_bytes));
    }
    CONDUIT_ANNOTATE_MARK_END("allocate");

    CONDUIT_ANNOTATE_MARK_BEGIN("copy");
    execution::MagicMemory::copy(vals_ptr, host_data.data(), size_bytes);
    CONDUIT_ANNOTATE_MARK_END("copy");

    CONDUIT_ANNOTATE_MARK_BEGIN("exec");
    execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
    {
        atomic_op(vals_ptr, i);
    });
    CONDUIT_ANNOTATE_MARK_END("exec");

    CONDUIT_ANNOTATE_MARK_BEGIN("deallocate");
    if (policy.is_device_policy())
    {
        execution::DeviceMemory::deallocate(vals_ptr);
    }
    else // if (!policy.is_device_policy())
    {
        execution::HostMemory::deallocate(vals_ptr);
    }
    CONDUIT_ANNOTATE_MARK_END("deallocate");

    CONDUIT_ANNOTATE_MARK_END(annotated_name.c_str());
}

//-----------------------------------------------------------------------------
template <typename T>
void
atomic_add(EP policy,
           benchmark::FillMode mode)
{
    atomic_benchmark<T>("atomic_add", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        execution::atomic_add(vals_ptr + i, static_cast<T>(i));
    });
}

//-----------------------------------------------------------------------------
template <typename T>
void
atomic_min(EP policy,
           benchmark::FillMode mode)
{
    atomic_benchmark<T>("atomic_min", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        execution::atomic_min(vals_ptr + i, static_cast<T>(i));
    });
}

//-----------------------------------------------------------------------------
template <typename T>
void
atomic_max(EP policy,
           benchmark::FillMode mode)
{
    atomic_benchmark<T>("atomic_max", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        execution::atomic_max(vals_ptr + i, static_cast<T>(i));
    });
}

//-----------------------------------------------------------------------------
// Reducer benchmarks

//-----------------------------------------------------------------------------
template <typename T, typename ReduceOp>
void
reduce_benchmark(const std::string& name,
                 EP policy,
                 benchmark::FillMode mode,
                 ReduceOp&& reduce_op)
{
    const std::string annotated_name = benchmark::get_annotated_name<T>(name, mode);
    CONDUIT_ANNOTATE_MARK_BEGIN(annotated_name.c_str());

    const index_t size = BENCHMARK_ARRAY_SIZE;
    const index_t size_bytes = static_cast<index_t>(sizeof(T)) * size;

    std::vector<uint8_t> host_data;
    benchmark::make_data(benchmark::dtype_of<T>(), size, host_data, mode);
    T* vals_ptr = nullptr;

    CONDUIT_ANNOTATE_MARK_BEGIN("allocate");
    if (policy.is_device_policy())
    {
        vals_ptr = static_cast<T*>(execution::DeviceMemory::allocate(size_bytes));
    }
    else // if (!policy.is_device_policy())
    {
        vals_ptr = static_cast<T*>(execution::HostMemory::allocate(size_bytes));
    }
    CONDUIT_ANNOTATE_MARK_END("allocate");

    CONDUIT_ANNOTATE_MARK_BEGIN("copy");
    execution::MagicMemory::copy(vals_ptr, host_data.data(), size_bytes);
    CONDUIT_ANNOTATE_MARK_END("copy");

    CONDUIT_ANNOTATE_MARK_BEGIN("exec");
    execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
    {
        reduce_op(vals_ptr, i);
    });
    CONDUIT_ANNOTATE_MARK_END("exec");

    CONDUIT_ANNOTATE_MARK_BEGIN("deallocate");
    if (policy.is_device_policy())
    {
        execution::DeviceMemory::deallocate(vals_ptr);
    }
    else // if (!policy.is_device_policy())
    {
        execution::HostMemory::deallocate(vals_ptr);
    }
    CONDUIT_ANNOTATE_MARK_END("deallocate");

    CONDUIT_ANNOTATE_MARK_END(annotated_name.c_str());
}

//-----------------------------------------------------------------------------
template <typename T>
void
reduce_sum(EP policy,
           benchmark::FillMode mode)
{
    execution::ReduceSum<T> reducer(T(0));
    reduce_benchmark<T>("reduce_sum", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        reducer += vals_ptr[i];
    });
}

//-----------------------------------------------------------------------------
template <typename T>
void
reduce_min(EP policy,
           benchmark::FillMode mode)
{
    execution::ReduceMin<T> reducer(std::numeric_limits<T>::max());
    reduce_benchmark<T>("reduce_min", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        reducer.min(vals_ptr[i]);
    });
}

//-----------------------------------------------------------------------------
template <typename T>
void
reduce_max(EP policy,
           benchmark::FillMode mode)
{
    execution::ReduceMax<T> reducer(std::numeric_limits<T>::lowest());
    reduce_benchmark<T>("reduce_max", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        reducer.max(vals_ptr[i]);
    });
}

//-----------------------------------------------------------------------------
template <typename T>
void
reduce_min_loc(EP policy,
               benchmark::FillMode mode)
{
    execution::ReduceMinLoc<T> reducer(std::numeric_limits<T>::max(), -1);
    reduce_benchmark<T>("reduce_min_loc", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        reducer.minloc(vals_ptr[i], i);
    });
}

//-----------------------------------------------------------------------------
template <typename T>
void
reduce_max_loc(EP policy,
               benchmark::FillMode mode)
{
    execution::ReduceMaxLoc<T> reducer(std::numeric_limits<T>::lowest(), -1);
    reduce_benchmark<T>("reduce_max_loc", policy, mode, [=] CONDUIT_EXEC(T* vals_ptr, index_t i)
    {
        reducer.maxloc(vals_ptr[i], i);
    });
}

//-----------------------------------------------------------------------------
// Atomic benchmark tests
//
// TODO: OpenMP w/ no RAJA seems incredibly slow (most obvious when doing many
// iterations). We should look at what RAJA does differently to improve
// performance. Maybe limiting the number of omp threads to 1-4 before atomics
// would help?

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_add)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(atomic_add<int32>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(atomic_add<int64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_min)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(atomic_min<int32>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(atomic_min<int64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_max)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(atomic_max<int32>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(atomic_max<int64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
// Reducer benchmark tests
//
// TODO: We are curious if we gain anything by using the bonus reducer policies:
// https://raja.readthedocs.io/en/main/sphinx/user_guide/cook_book/reduction.html

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_sum)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_sum<int32>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_sum<int64>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_sum<float64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_min)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_min<int32>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_min<int64>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_min<float64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_max)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_max<int32>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_max<int64>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_max<float64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_min_loc)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_min_loc<int32>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_min_loc<int64>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_min_loc<float64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_max_loc)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_max_loc<int32>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_max_loc<int64>,   BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(reduce_max_loc<float64>, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    // TODO: Use a real argument parser
    if (argc >= 2)
    {
        BENCHMARK_NUM_WARMUP_ITERATIONS = static_cast<index_t>(atoll(argv[1]));
    }
    if (argc >= 3)
    {
        BENCHMARK_NUM_ITERATIONS = static_cast<index_t>(atoll(argv[2]));
    }
    if (argc >= 4)
    {
        BENCHMARK_ARRAY_SIZE = static_cast<index_t>(atoll(argv[3]));
    }

    return RUN_ALL_TESTS();
}
