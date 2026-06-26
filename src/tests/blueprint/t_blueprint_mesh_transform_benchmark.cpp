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
#include "conduit_blueprint.hpp"

#include "gtest/gtest.h"

using namespace conduit;
using EP = execution::ExecutionPolicy;

index_t BENCHMARK_ARRAY_SIZE            = 64;
index_t BENCHMARK_NUM_WARMUP_ITERATIONS = 10;
index_t BENCHMARK_NUM_ITERATIONS        = 100;

//-----------------------------------------------------------------------------
void
coordset_uniform_to_explicit(EP /*policy*/,
                             benchmark::FillMode /*mode*/)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    Node mesh;
    blueprint::mesh::examples::braid("uniform",
                                     BENCHMARK_ARRAY_SIZE,
                                     BENCHMARK_ARRAY_SIZE,
                                     BENCHMARK_ARRAY_SIZE,
                                     mesh);

    const Node &coordset = mesh["coordsets"].child(0);
    Node dst;
    blueprint::mesh::coordset::uniform::to_explicit(coordset, dst);
}

//-----------------------------------------------------------------------------
void
coordset_rectilinear_to_explicit(EP /*policy*/,
                                 benchmark::FillMode /*mode*/)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    Node mesh;
    blueprint::mesh::examples::braid("rectilinear",
                                     BENCHMARK_ARRAY_SIZE,
                                     BENCHMARK_ARRAY_SIZE,
                                     BENCHMARK_ARRAY_SIZE,
                                     mesh);

    const Node &coordset = mesh["coordsets"].child(0);
    Node dst;
    blueprint::mesh::coordset::rectilinear::to_explicit(coordset, dst);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform, coordset_to_explicit)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(coordset_uniform_to_explicit,
                    BENCHMARK_NUM_WARMUP_ITERATIONS,
                    BENCHMARK_NUM_ITERATIONS,
                    {benchmark::FillMode::None});
    benchmark::exec(coordset_rectilinear_to_explicit,
                    BENCHMARK_NUM_WARMUP_ITERATIONS,
                    BENCHMARK_NUM_ITERATIONS,
                    {benchmark::FillMode::None});
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

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
