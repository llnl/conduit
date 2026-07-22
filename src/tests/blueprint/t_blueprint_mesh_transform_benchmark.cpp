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

// Intentionally small by default, to minimize CI time spent benchmarking
std::vector<index_t> BENCHMARK_DIM_SIZES = {2, 4};
index_t BENCHMARK_NUM_WARMUP_ITERATIONS  = 10;
index_t BENCHMARK_NUM_ITERATIONS         = 100;

//-----------------------------------------------------------------------------
#include "conduit_execution.hpp"
#include "conduit_blueprint_mesh_examples.hpp"

void
copy_numeric_arrays_to_device(const Node &src,
                              Node &dst,
                              index_t device_alloc)
{
    // Scalars have to stay on the host, attempting to copy them to device
    // will result in a segfault when the transform code later dereferences
    // them directly.
    if(src.dtype().is_object())
    {
        NodeConstIterator itr = src.children();
        while(itr.has_next())
        {
            const Node &src_child = itr.next();
            copy_numeric_arrays_to_device(src_child, dst[itr.name()], device_alloc);
        }
    }
    else if(src.dtype().is_list())
    {
        NodeConstIterator itr = src.children();
        while(itr.has_next())
        {
            const Node &src_child = itr.next();
            copy_numeric_arrays_to_device(src_child, dst.append(), device_alloc);
        }
    }
    else if(src.dtype().is_number() && src.dtype().number_of_elements() > 1)
    {
        dst.set_allocator(device_alloc);
        dst.set(src);
    }
    else // Not a numeric array, leave it in host memory
    {
        dst.set(src);
    }
}

//-----------------------------------------------------------------------------
void
make_braid_dataset(const std::string &src_type,
                   const benchmark::ExecConfig &config,
                   Node &src)
{
    Node host_src;
    blueprint::mesh::examples::braid(src_type,
                                     config.dim_size,
                                     config.dim_size,
                                     config.dim_size,
                                     host_src);

    if (config.src_location == "device")
    {
        copy_numeric_arrays_to_device(host_src,
                                      src,
                                      execution::get_device_allocator_id());
    }
    else // if (config.src_location == "host")
    {
        src.set(host_src);
    }
}

//-----------------------------------------------------------------------------
void
benchmark_coordset_transform(const char *name,
                             const std::string &src_type,
                             void (*transform)(const Node &, Node &))
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_braid_dataset(src_type, config, src);
    };

    // Perform a transform on the source Node
    auto run = [=](const Node &src) {
        Node dst;
        transform(src["coordsets"].child(0), dst);
    };

    // Execute the benchmark
    benchmark::exec(name,
                    setup,
                    run,
                    BENCHMARK_NUM_WARMUP_ITERATIONS,
                    BENCHMARK_NUM_ITERATIONS,
                    BENCHMARK_DIM_SIZES);
}

//-----------------------------------------------------------------------------
void
benchmark_topology_transform(const char *name,
                             const std::string &src_type,
                             void (*transform)(const Node &, Node &, Node &))
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_braid_dataset(src_type, config, src);
    };

    // Perform a transform on the source Node
    auto run = [=](const Node &src) {
        Node topo_dst, coords_dst;
        transform(src["topologies"].child(0), topo_dst, coords_dst);
    };

    // Execute the benchmark
    benchmark::exec(name,
                    setup,
                    run,
                    BENCHMARK_NUM_WARMUP_ITERATIONS,
                    BENCHMARK_NUM_ITERATIONS,
                    BENCHMARK_DIM_SIZES);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, coordset_transforms)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark_coordset_transform("coordset_uniform_to_rectilinear",
                                 "uniform",
                                 blueprint::mesh::coordset::uniform::to_rectilinear);
    benchmark_coordset_transform("coordset_uniform_to_explicit",
                                 "uniform",
                                 blueprint::mesh::coordset::uniform::to_explicit);
    benchmark_coordset_transform("coordset_rectilinear_to_explicit",
                                 "rectilinear",
                                 blueprint::mesh::coordset::rectilinear::to_explicit);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, topology_transforms)
{
    // This is not an exhaustive benchmark of the topology transforms;
    // it only includes 'to_unstructured' since that is the only transform
    // that made sense to port to the device execution model.

    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark_topology_transform("topology_uniform_to_unstructured",
                                 "uniform",
                                 blueprint::mesh::topology::uniform::to_unstructured);
    benchmark_topology_transform("topology_rectilinear_to_structured",
                                 "rectilinear",
                                 blueprint::mesh::topology::rectilinear::to_structured);
    benchmark_topology_transform("topology_rectilinear_to_unstructured",
                                 "rectilinear",
                                 blueprint::mesh::topology::rectilinear::to_unstructured);
    benchmark_topology_transform("topology_structured_to_unstructured",
                                 "structured",
                                 blueprint::mesh::topology::structured::to_unstructured);
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
