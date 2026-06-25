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
#include "conduit_execution.hpp"
#include "conduit_execution_policy.hpp"
#include "conduit_memory_manager.hpp"

#include "gtest/gtest.h"

using namespace conduit;
using EP = conduit::execution::ExecutionPolicy;

index_t BENCHMARK_ARRAY_SIZE = 4;
index_t BENCHMARK_NUM_WARMUP_ITERATIONS = 10;
index_t BENCHMARK_NUM_ITERATIONS = 1000;

//-----------------------------------------------------------------------------
// TODO: This might be nice as a public helper in the execution:: API
template <typename PolicyFn>
void add_if_enabled(std::vector<EP>& policies, bool enabled, PolicyFn&& make_policy)
{
    // Don't execute policies that are compile-time disabled
    if (!enabled)
    {
        return;
    }

    EP policy = make_policy();
    const auto id = policy.policy_id();

    // Don't insert repeat policies into the vector (so that we only
    // do each one exactly once)
    auto it = std::find_if(
        policies.begin(),
        policies.end(),
        [id](const EP& p)
        {
            return p.policy_id() == id;
        });

    if (it == policies.end())
    {
        policies.push_back(policy);
    }
}

//-----------------------------------------------------------------------------
std::vector<EP> get_enabled_policies()
{
    std::vector<EP> policies;
    policies.reserve(7);

    add_if_enabled(policies, EP::is_serial_enabled(),   [] { return EP::serial(); });
    add_if_enabled(policies, EP::is_parallel_enabled(), [] { return EP::parallel(); });
    add_if_enabled(policies, EP::is_host_enabled(),     [] { return EP::host(); });
    add_if_enabled(policies, EP::is_device_enabled(),   [] { return EP::device(); });
    add_if_enabled(policies, EP::is_openmp_enabled(),   [] { return EP::openmp(); });
    add_if_enabled(policies, EP::is_cuda_enabled(),     [] { return EP::cuda(); });
    add_if_enabled(policies, EP::is_hip_enabled(),      [] { return EP::hip(); });

    return policies;
}

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
    conduit::execution::MagicMemory::copy(vals_ptr, &host_vals[0], sizeof(index_t) * size);
    CONDUIT_ANNOTATE_MARK_END("copy");

    CONDUIT_ANNOTATE_MARK_BEGIN("exec");
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
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
        conduit::execution::atomic_add(vals_ptr + i, i);
    });
}

//-----------------------------------------------------------------------------
void
atomic_min(EP policy)
{
    atomic_benchmark("atomic_min", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        conduit::execution::atomic_min(vals_ptr + i, i);
    });
}

//-----------------------------------------------------------------------------
void
atomic_max(EP policy)
{
    atomic_benchmark("atomic_max", policy, [=] CONDUIT_EXEC(index_t* vals_ptr, index_t i)
    {
        conduit::execution::atomic_max(vals_ptr + i, i);
    });
}

template <typename BenchmarkFn>
void
benchmark(BenchmarkFn&& fn)
{
    // Setup
    execution::init_device_memory_handlers();
    auto policies = get_enabled_policies();

    for (const auto& policy : policies)
    {
        // Warm-up
        for (int i = 0; i < BENCHMARK_NUM_WARMUP_ITERATIONS; i++)
        {
            fn(policy);
        }

        std::cout << "\n" << policy.policy_name() << "," << BENCHMARK_NUM_WARMUP_ITERATIONS << "," << BENCHMARK_NUM_ITERATIONS << std::endl;

        // TODO: Do we care to have bespoke timing in the case that conduit was built
        // without caliper?

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        // Benchmark
        for (int i = 0; i < BENCHMARK_NUM_ITERATIONS; i++)
        {
            fn(policy);
        }

        annotations::finalize();
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_add)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark(atomic_add);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_min)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark(atomic_min);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, atomic_max)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark(atomic_max);
}

// TODO: We are curious if we gain anything by using the bonus reducer policies:
// https://raja.readthedocs.io/en/main/sphinx/user_guide/cook_book/reduction.html

// TODO: Reducer benchmarks

// TODO: Blueprint mesh benchmarks

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
