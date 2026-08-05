// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_transform_benchmark.cpp
///
//-----------------------------------------------------------------------------

/*
Usage:
    t_blueprint_mesh_transform_benchmark [num_warmup_iterations]
                                         [num_iterations]
                                         [dim_size ...]

    All arguments are optional and positional:
      num_warmup_iterations  untimed iterations per benchmark (default: 2)
      num_iterations         timed iterations per benchmark (default: 2)
      dim_size ...           one or more mesh sizes, given as the number of
                             vertices per axis. Each must be > 1 and every
                             benchmark is run at every size (default: [2])

Examples:
    # Defaults: 2 warmups, 2 timed iterations, dim size 2
    ./t_blueprint_mesh_transform_benchmark

    # 5 warmups, 20 timed iterations, dim size 2
    ./t_blueprint_mesh_transform_benchmark 5 20

    # 5 warmups, 20 timed iterations, at dim sizes 10, 20, and 40
    ./t_blueprint_mesh_transform_benchmark 5 20 10 20 40

If conduit was built with Caliper support, timing results are written to
<YYYYmmdd_HHMMSS>.cali in the current directory, which can be plotted
with src/tests/blueprint/plot_benchmark_output.py (cmake automatically
copies it to the tests/blueprint folder for convenience).
*/

#include "conduit.hpp"
#include "conduit_annotations.hpp"
#include "conduit_benchmark.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_core.hpp"

#include "gtest/gtest.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace conduit;

// Number of vertices per axis, braid requires at least two
std::vector<index_t> BENCHMARK_DIM_SIZES = {2};

// Small by default, to minimize CI time spent benchmarking
index_t BENCHMARK_NUM_WARMUP_ITERATIONS = 2;
index_t BENCHMARK_NUM_ITERATIONS = 2;

//-----------------------------------------------------------------------------
// Reports vertex/element counts for the input and output meshes. Some
// operations (e.g. generate_corners) produce far more elements than they
// started with, so it's worth recording both.
std::string
mesh_size_info(const Node &input, const Node &output)
{
    const index_t inverts  = blueprint::mesh::coordset::length(input["coordsets"].child(0));
    const index_t inelems  = blueprint::mesh::topology::length(input["topologies"].child(0));
    const index_t outverts = blueprint::mesh::coordset::length(output["coordsets"].child(0));
    const index_t outelems = blueprint::mesh::topology::length(output["topologies"].child(0));

    return "inverts-"   + std::to_string(inverts)
         + "_inelems-"  + std::to_string(inelems)
         + "_outverts-" + std::to_string(outverts)
         + "_outelems-" + std::to_string(outelems);
}

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
                   const index_t npts,
                   Node &src)
{
    const bool is_2d = src_type == "tris" ||
                       src_type == "quads" ||
                       src_type == "mixed_2d";

    const index_t npts_z = is_2d ? 0 : npts;

    // Braid will reset `src` for us internally 
    blueprint::mesh::examples::braid(src_type,
                                     npts,
                                     npts,
                                     npts_z,
                                     src);
}

//-----------------------------------------------------------------------------
// One mesh::convert() benchmark: convert a braid `src_type` mesh to `target`.
struct ConvertConfig
{
    std::string name;
    std::string src_type;
    std::string target;
};

//-----------------------------------------------------------------------------
void
run_benchmarks(const std::vector<ConvertConfig> &convert_configs)
{
    // The available execution configurations (host/device) based
    // on what Conduit was compiled with.
    const auto exec_configs = benchmark::get_exec_configs();

    // We create src and dst nodes once and reuse them across all benchmarks
    Node src;
    Node dst;

    // Iterating over the dimension sizes first allows us to reuse `src`
    // across benchmarks that use the same `src_type` and `npts`, reducing
    // overall runtime and memory usage.
    for (const auto npts : BENCHMARK_DIM_SIZES)
    {
        // Iterating over `exec_configs` second will allow us to reuse `src`
        // across all configurations of a particular `src_location`. This
        // allows us to create `src` once for host and once for device,
        // per `src_type` and `npts` combination.
        std::string built_src_type;
        for (const auto &exec_config : exec_configs)
        {
            // This iterates over all of the benchmark configurations
            // themselves (i.e., which source and target types to use).
            for (const auto &convert_config : convert_configs)
            {
                // Since multiple benchmarks will use the same
                // src_type and npts, we can improve performance and
                // limit memory utilization by only rebuilding `src`
                // when the next entry's (src_type, npts) differs.
                if (convert_config.src_type != built_src_type)
                {
                    make_braid_dataset(convert_config.src_type, npts, src);
                    built_src_type = convert_config.src_type;
                }

                // TODO: There may be other interesting options
                Node options;
                options["target"] = convert_config.target;
                options["copy"]   = 0;

                // This lambda defines the code to be benchmarked
                auto run = [&](const Node &input, Node &output) {
                    blueprint::mesh::convert(input, options, output);
                };

                // This executes a benchmark of the current configuration
                benchmark::exec(convert_config.name,
                                src,
                                dst,
                                run,
                                mesh_size_info,
                                exec_config,
                                npts,
                                BENCHMARK_NUM_WARMUP_ITERATIONS,
                                BENCHMARK_NUM_ITERATIONS);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Benchmarks that measure the performance of full mesh conversions
TEST(blueprint_mesh_transform_benchmark, mesh_transforms)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    run_benchmarks({
        {"mesh_uniform_to_rectilinear",      "uniform",     "rectilinear"},
        {"mesh_uniform_to_structured",       "uniform",     "structured"},
        {"mesh_uniform_to_unstructured",     "uniform",     "unstructured"},
        {"mesh_rectilinear_to_structured",   "rectilinear", "structured"},
        {"mesh_rectilinear_to_unstructured", "rectilinear", "unstructured"},
        {"mesh_structured_to_unstructured",  "structured",  "unstructured"},
    });
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, generate_transforms)
// Benchmarks that measure the performance of mesh generation transforms
{
    // This is not an exhaustive benchmark of the topology transforms;
    // it only includes 'to_unstructured' since that is the only transform
    // that made sense to port to the device execution model.

    CONDUIT_ANNOTATE_MARK_FUNCTION;
    
    const std::vector<std::string> shapes = {
        "quads",
        "hexs",
        "pyramids",    // particularly slow pre-device execution
        // "mixed_2d", // TODO: investigate why this segfaults
        // "mixed"     // TODO: investigate why this segfaults
    };

    // Building this list programmatically makes it easy to benchmark with
    // different shape types.
    std::vector<ConvertConfig> configs;
    for (const auto &shape : shapes)
    {
        configs.push_back({"to_polytopal_" + shape,       shape, "polytopal"});
        configs.push_back({"generate_points_" + shape,    shape, "generate_points"});
        configs.push_back({"generate_lines_" + shape,     shape, "generate_lines"});
        configs.push_back({"generate_faces_" + shape,     shape, "generate_faces"});
        configs.push_back({"generate_centroids_" + shape, shape, "generate_centroids"});
        configs.push_back({"generate_sides_" + shape,     shape, "generate_sides"});
        configs.push_back({"generate_corners_" + shape,   shape, "generate_corners"});
    }

    run_benchmarks(configs);
}

//-----------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    if (!annotations::supported())
    {
        std::cout << "WARNING: conduit was built without Caliper support, "
                     "so this benchmark will run but will not produce any "
                     "timing output (.cali file)."
                  << std::endl;
    }

    if (argc >= 2)
    {
        BENCHMARK_NUM_WARMUP_ITERATIONS = static_cast<index_t>(std::atoll(argv[1]));
    }

    if (argc >= 3)
    {
        BENCHMARK_NUM_ITERATIONS = static_cast<index_t>(std::atoll(argv[2]));
    }

    if (argc >= 4)
    {
        BENCHMARK_DIM_SIZES.clear();

        for (int i = 3; i < argc; i++)
        {
            BENCHMARK_DIM_SIZES.push_back(static_cast<index_t>(std::atoll(argv[i])));
        }
    }

    for (const index_t dim_size : BENCHMARK_DIM_SIZES)
    {
        if (dim_size <= 1)
        {
            // Braid will error if this is the case, so we may
            // as well not go any further.
            CONDUIT_ERROR("Mesh transform benchmark dimensions must be "
                          "greater than 1; received " << dim_size << ".");
        }
    }

    // TODO: Look at Caliper options related to OpenMP/GPU profiling
    const std::string timestamp = benchmark::get_timestamp();

    // Caliper options can be configured here
    Node cali_opts;
    // TODO: Investigate spot vs hatchet-region-profile
    cali_opts["config"] = "runtime-report,hatchet-region-profile(output=" + timestamp + ".cali)";

    // Begin timing
    annotations::initialize(cali_opts);

    // Run all benchmarks
    const int result = RUN_ALL_TESTS();

    // Finalize timing
    annotations::finalize();

    return result;
}
