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

index_t BENCHMARK_DIM_SIZE              = 64;
index_t BENCHMARK_NUM_WARMUP_ITERATIONS = 10;
index_t BENCHMARK_NUM_ITERATIONS        = 100;

//-----------------------------------------------------------------------------
void
coordset_transform(const char *name,
                   const std::string &src_type,
                   void (*transform)(const Node &, Node &),
                   const benchmark::ExecConfig &config)
{
    CONDUIT_ANNOTATE_MARK_SCOPE(name);

    Node mesh;
    blueprint::mesh::examples::braid(src_type,
                                     BENCHMARK_DIM_SIZE,
                                     BENCHMARK_DIM_SIZE,
                                     BENCHMARK_DIM_SIZE,
                                     mesh);

    const Node &host_coordset = mesh["coordsets"].child(0);
    Node dst;
    if ("device" == config.src_location && host_coordset.has_child("values"))
    {
        // Move the coordinate arrays to device memory
        const index_t alloc_id = execution::get_device_allocator_id();
        Node device_coordset;
        device_coordset["type"].set(host_coordset["type"]);
        for (const std::string &axis : host_coordset["values"].child_names())
        {
            device_coordset["values"][axis].set_allocator(alloc_id);
            device_coordset["values"][axis].set(host_coordset["values"][axis]);
        }
        transform(device_coordset, dst);
    }
    else
    {
        transform(host_coordset, dst);
    }
}

//-----------------------------------------------------------------------------
void uniform_to_rectilinear(const benchmark::ExecConfig &config)
{
    coordset_transform(__FUNCTION__,
                       "uniform",
                       blueprint::mesh::coordset::uniform::to_rectilinear,
                       config);
}

void uniform_to_explicit(const benchmark::ExecConfig &config)
{
    coordset_transform(__FUNCTION__,
                       "uniform",
                       blueprint::mesh::coordset::uniform::to_explicit,
                       config);
}

void rectilinear_to_explicit(const benchmark::ExecConfig &config)
{
    coordset_transform(__FUNCTION__,
                       "rectilinear",
                       blueprint::mesh::coordset::rectilinear::to_explicit,
                       config);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_execution, coordset_to_explicit)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark::exec(uniform_to_rectilinear,  BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(uniform_to_explicit,     BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
    benchmark::exec(rectilinear_to_explicit, BENCHMARK_NUM_WARMUP_ITERATIONS, BENCHMARK_NUM_ITERATIONS);
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
        BENCHMARK_DIM_SIZE = static_cast<index_t>(atoll(argv[3]));
    }

    const std::string timestamp = benchmark::get_timestamp();
    Node cali_opts;
    // cali_opts["config"] = "hatchet-region-profile(output=" + timestamp + ".cali)";
    cali_opts["config"] = "runtime-report(max_column_width=999)";
    annotations::initialize(cali_opts);

    const int result = RUN_ALL_TESTS();

    annotations::finalize();
    
    return result;
}
