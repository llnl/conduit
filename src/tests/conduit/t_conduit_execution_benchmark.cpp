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

#include "gtest/gtest.h"

using namespace conduit;
using EP = execution::ExecutionPolicy;

index_t BENCHMARK_ARRAY_SIZE = 4;
index_t BENCHMARK_NUM_WARMUP_ITERATIONS = 10;
index_t BENCHMARK_NUM_ITERATIONS = 100;

//-----------------------------------------------------------------------------
// Atomic benchmarks

//-----------------------------------------------------------------------------
template <typename AtomicOp>
void
atomic_benchmark(const std::string& name, EP policy, AtomicOp&& atomic_op)
{
    CONDUIT_ANNOTATE_MARK_BEGIN(name.c_str());

    // TODO: Generate arrays of random numbers and of arbitrary size,
    // controllable from CLI
    const index_t size = 4;
    index_t host_vals[size] = {0, -1, -2, -3};
    index_t* vals_ptr = nullptr;

    CONDUIT_ANNOTATE_MARK_BEGIN("allocate");
    if (policy.is_device_policy())
    {
        vals_ptr = static_cast<index_t*>(
            execution::DeviceMemory::allocate(sizeof(index_t) * size));
    }
    else // if (!policy.is_device_policy())
    {
        vals_ptr = static_cast<index_t*>(execution::HostMemory::allocate(sizeof(index_t) * size));
    }
    CONDUIT_ANNOTATE_MARK_END("allocate");

    CONDUIT_ANNOTATE_MARK_BEGIN("copy");
    execution::MagicMemory::copy(vals_ptr, &host_vals[0], sizeof(index_t) * size);
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

    CONDUIT_ANNOTATE_MARK_END(name.c_str());
}

//-----------------------------------------------------------------------------
void
atomic_add(EP policy)
{
    atomic_benchmark("atomic_add", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        execution::atomic_add(vals_ptr + i, i);
    });
}

//-----------------------------------------------------------------------------
void
atomic_min(EP policy)
{
    atomic_benchmark("atomic_min", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        execution::atomic_min(vals_ptr + i, i);
    });
}

//-----------------------------------------------------------------------------
void
atomic_max(EP policy)
{
    atomic_benchmark("atomic_max", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        execution::atomic_max(vals_ptr + i, i);
    });
}

//-----------------------------------------------------------------------------
// Reducer benchmarks

//-----------------------------------------------------------------------------
template <typename ReduceOp>
void
reduce_benchmark(const std::string& name, EP policy, ReduceOp&& reduce_op)
{
    CONDUIT_ANNOTATE_MARK_BEGIN(name.c_str());

    // TODO: Generate arrays of random numbers and of arbitrary size,
    // controllable from CLI
    const index_t size = 4;
    index_t host_vals[size] = {0, -10, 10, 5};
    index_t *vals_ptr = nullptr;

    CONDUIT_ANNOTATE_MARK_BEGIN("allocate");
    if (policy.is_device_policy())
    {
        vals_ptr = static_cast<index_t*>(execution::DeviceMemory::allocate(sizeof(index_t) * size));
    }
    else // if (!policy.is_device_policy())
    {
        vals_ptr = static_cast<index_t*>(execution::HostMemory::allocate(sizeof(index_t) * size));
    }
    CONDUIT_ANNOTATE_MARK_END("allocate");

    CONDUIT_ANNOTATE_MARK_BEGIN("copy");
    execution::MagicMemory::copy(vals_ptr, &host_vals[0], sizeof(index_t) * size);
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

    CONDUIT_ANNOTATE_MARK_END(name.c_str());
}

//-----------------------------------------------------------------------------
void
reduce_sum(EP policy)
{
    execution::ReduceSum<index_t> reducer(0);
    reduce_benchmark("reduce_sum", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        reducer += vals_ptr[i];
    });
}

//-----------------------------------------------------------------------------
void
reduce_min(EP policy)
{
    execution::ReduceMin<index_t> reducer(std::numeric_limits<index_t>::max());
    reduce_benchmark("reduce_min", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        reducer.min(vals_ptr[i]);
    });
}

//-----------------------------------------------------------------------------
void
reduce_max(EP policy)
{
    execution::ReduceMax<index_t> reducer(std::numeric_limits<index_t>::lowest());
    reduce_benchmark("reduce_max", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        reducer.max(vals_ptr[i]);
    });
}

//-----------------------------------------------------------------------------
void
reduce_min_loc(EP policy)
{
    execution::ReduceMinLoc<index_t> reducer(std::numeric_limits<index_t>::max(), -1);
    reduce_benchmark("reduce_min_loc", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        reducer.minloc(vals_ptr[i], i);
    });
}

//-----------------------------------------------------------------------------
void
reduce_max_loc(EP policy)
{
    execution::ReduceMaxLoc<index_t> reducer(std::numeric_limits<index_t>::lowest(), -1);
    reduce_benchmark("reduce_max_loc", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        reducer.maxloc(vals_ptr[i], i);
    });
}

//-----------------------------------------------------------------------------
// Atomic benchmarks
// 
// TODO: OpenMP w/ no RAJA seems incredibly slow, we should look at what RAJA
// does differently to improve performance. Maybe limiting the number of threads
// to 2-4 for atomics would help?

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_add)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(atomic_add, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_min)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(atomic_min, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_max)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(atomic_max, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
// Reducer benchmarks
// 
// TODO: We are curious if we gain anything by using the bonus reducer policies:
// https://raja.readthedocs.io/en/main/sphinx/user_guide/cook_book/reduction.html

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_sum)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_sum, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_min)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_min, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_max)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_max, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_min_loc)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_min_loc, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, reduce_max_loc)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(reduce_max_loc, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    // TODO: Do CLI input properly
    if(argc == 2)
    {
        BENCHMARK_NUM_WARMUP_ITERATIONS = atoi(argv[1]);
    }

    if(argc == 3)
    {
        BENCHMARK_NUM_WARMUP_ITERATIONS = atoi(argv[1]);
        BENCHMARK_NUM_ITERATIONS = atoi(argv[2]);
    }

    return RUN_ALL_TESTS();
}
