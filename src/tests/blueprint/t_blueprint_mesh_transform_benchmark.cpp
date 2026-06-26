// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_transform_benchmark.cpp
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
TEST(conduit_execution, mesh_transform)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    // benchmark::exec(reduce_max_loc, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
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
