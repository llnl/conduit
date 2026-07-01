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

std::vector<index_t> BENCHMARK_DIM_SIZES = {2, 4, 8, 16, 32, 64, 128};
index_t BENCHMARK_NUM_WARMUP_ITERATIONS  = 10;
index_t BENCHMARK_NUM_ITERATIONS         = 100;

//-----------------------------------------------------------------------------
// It's much faster to create the source coordset once and reuse it vs
// recreating it before every iteration
const Node
make_coordset(const std::string &src_type,
              const benchmark::ExecConfig &config)
{
    Node mesh;
    blueprint::mesh::examples::braid(src_type,
                                     config.dim_size,
                                     config.dim_size,
                                     config.dim_size,
                                     mesh);

    const Node &host_coordset = mesh["coordsets"].child(0);
    Node src;
    if ("device" == config.src_location && host_coordset.has_child("values"))
    {
        // Move the coordinate arrays to device memory
        const index_t alloc_id = execution::get_device_allocator_id();
        src["type"].set(host_coordset["type"]);
        for (const auto &axis : host_coordset["values"].child_names())
        {
            src["values"][axis].set_allocator(alloc_id);
            src["values"][axis].set(host_coordset["values"][axis]);
        }
    }
    else // if ("host" == config.src_location)
    {
        src.set(host_coordset);
    }
    return src;
}

//-----------------------------------------------------------------------------
void
benchmark_transform(const char *name,
                    const std::string &src_type,
                    void (*transform)(const Node &, Node &))
{
    // Create the source coordset once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config) {
        return make_coordset(src_type, config);
    };

    // Perform a transform on the source coordset
    auto run = [=](const Node &src) {
        Node dst;
        transform(src, dst);
    };

    // Benchmark the transform function
    benchmark::exec(name,
                    setup,
                    run,
                    BENCHMARK_NUM_WARMUP_ITERATIONS,
                    BENCHMARK_NUM_ITERATIONS,
                    BENCHMARK_DIM_SIZES);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_execution, coordset_transforms)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark_transform("uniform_to_rectilinear",
                        "uniform",
                        blueprint::mesh::coordset::uniform::to_rectilinear);
    benchmark_transform("uniform_to_explicit",
                        "uniform",
                        blueprint::mesh::coordset::uniform::to_explicit);
    benchmark_transform("rectilinear_to_explicit",
                        "rectilinear",
                        blueprint::mesh::coordset::rectilinear::to_explicit);
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
        BENCHMARK_DIM_SIZES.clear();
        for (int i = 3; i < argc; i++)
        {
            BENCHMARK_DIM_SIZES.push_back(static_cast<index_t>(atoll(argv[i])));
        }
    }

    // TODO: Look at Caliper options related to OpenMP/GPU profiling
    const std::string timestamp = benchmark::get_timestamp();
    Node cali_opts;
    cali_opts["config"] = "hatchet-region-profile(output=" + timestamp + ".cali)";

    // Begin profiling
    annotations::initialize(cali_opts);

    // Begin benchmarking
    const int result = RUN_ALL_TESTS();

    // End profiling
    annotations::finalize();
    
    return result;
}
