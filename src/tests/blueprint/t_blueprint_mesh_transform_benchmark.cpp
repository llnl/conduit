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
#include "conduit_core.hpp"

#include "gtest/gtest.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace conduit;

// Number of points per active axis. Braid requires at least two.
std::vector<index_t> BENCHMARK_DIM_SIZES = {2, 4};

// Intentionally small by default, to minimize CI time spent benchmarking.
index_t BENCHMARK_NUM_WARMUP_ITERATIONS = 10;
index_t BENCHMARK_NUM_ITERATIONS = 100;

//-----------------------------------------------------------------------------
template <typename SetupFn, typename RunFn>
void
run_benchmark(const std::string &name,
              SetupFn &&setup,
              RunFn &&run)
{
    benchmark::exec(name,
                    std::forward<SetupFn>(setup),
                    std::forward<RunFn>(run),
                    BENCHMARK_NUM_WARMUP_ITERATIONS,
                    BENCHMARK_NUM_ITERATIONS,
                    BENCHMARK_DIM_SIZES);
}

//-----------------------------------------------------------------------------
void
make_braid_dataset(const std::string &src_type,
                   const benchmark::ExecConfig &config,
                   const index_t npts,
                   Node &src)
{
    const bool is_2d = src_type == "tris" ||
                       src_type == "quads" ||
                       src_type == "mixed_2d";

    const index_t npts_z = is_2d ? 0 : npts;

    blueprint::mesh::examples::braid(src_type,
                                     npts,
                                     npts,
                                     npts_z,
                                     src);
}

//-----------------------------------------------------------------------------
void
benchmark_mesh_convert(const std::string &name,
                       const std::string &src_type,
                       const std::string &target)
{
    // Create the source Node once and reuse it for every iteration
    auto setup = [&](const benchmark::ExecConfig &config,
                               const index_t npts,
                               Node &src) {
        make_braid_dataset(src_type, config, npts, src);
    };

    // The type being converted to
    Node options;
    options["target"] = target;

    // Perform the conversion
    auto run = [&](const Node &src, Node &dst) {
        blueprint::mesh::convert(src, options, dst);
    };

    // Launch the benchmark
    run_benchmark(name, setup, run);
}

//-----------------------------------------------------------------------------
// generate_offsets has no blueprint::mesh::convert() target.
void
benchmark_topology_generate_offsets(const std::string &name,
                                    const std::string &src_type)
{
    // Create the source Node once and reuse it for every iteration
    auto setup = [&](const benchmark::ExecConfig &config,
                               const index_t npts,
                               Node &src) {
        make_braid_dataset(src_type, config, npts, src);
    };

    // Perform the conversion
    auto run = [&](const Node &src, Node &dst) {
        blueprint::mesh::topology::unstructured::generate_offsets(
            src["topologies"].child(0),
            dst);
    };

    // Launch the benchmark
    run_benchmark(name, setup, run);
}

//-----------------------------------------------------------------------------
// generate_offsets_inline also has no blueprint::mesh::convert() target.
void
benchmark_topology_generate_offsets_inline(const std::string &name,
                                           const std::string &src_type)
{
    // This conversion is unique in that it mutates its argument in place,
    // and is a no-op if the offsets already exist.
    Node topo;

    // Create the source Node once, but in this case we reuse topo
    // instead of src.
    auto setup = [&](const benchmark::ExecConfig &config,
                               const index_t npts,
                               Node &src) {
        make_braid_dataset(src_type, config, npts, src);
        topo.set(src["topologies"].child(0));
    };

    // Perform the conversion
    auto run = [&](const Node &, Node &) {
        if (topo.has_path("elements/offsets"))
        {
            // Remove the offsets before each iteration to
            // avoid a no-op.
            topo["elements"].remove_child("offsets");
        }

        blueprint::mesh::topology::unstructured::
            generate_offsets_inline(topo);
    };

    // Launch the benchmark
    run_benchmark(name, setup, run);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, mesh_transforms)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // mesh::convert converts an entire mesh, including both its coordset and
    // topology.
    benchmark_mesh_convert("mesh_uniform_to_rectilinear",
                           "uniform",
                           "rectilinear");

    benchmark_mesh_convert("mesh_uniform_to_structured",
                           "uniform",
                           "structured");

    benchmark_mesh_convert("mesh_uniform_to_unstructured",
                           "uniform",
                           "unstructured");

    benchmark_mesh_convert("mesh_rectilinear_to_structured",
                           "rectilinear",
                           "structured");

    benchmark_mesh_convert("mesh_rectilinear_to_unstructured",
                           "rectilinear",
                           "unstructured");

    benchmark_mesh_convert("mesh_structured_to_unstructured",
                           "structured",
                           "unstructured");
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, unstructured_generate_transforms)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // Exercise fixed-shape and mixed-shape 2D and 3D unstructured cell
    // meshes.
    const std::vector<std::string> source_types = {
        "tris",
        "quads",
        // "mixed_2d", // segfaults
        "tets",
        "hexs",
        "wedges",
        "pyramids",
        // "mixed" // segfaults
    };

    for (const std::string &src_type : source_types)
    {
        benchmark_mesh_convert("unstructured_to_polytopal_" + src_type,
                               src_type,
                               "polytopal");
        benchmark_mesh_convert("unstructured_generate_points_" + src_type,
                               src_type,
                               "generate_points");
        benchmark_mesh_convert("unstructured_generate_lines_" + src_type,
                               src_type,
                               "generate_lines");
        benchmark_mesh_convert("unstructured_generate_faces_" + src_type,
                               src_type,
                               "generate_faces");
        benchmark_mesh_convert("unstructured_generate_centroids_" + src_type,
                               src_type,
                               "generate_centroids");
        benchmark_mesh_convert("unstructured_generate_sides_" + src_type,
                               src_type,
                               "generate_sides");
        benchmark_mesh_convert("unstructured_generate_corners_" + src_type,
                               src_type,
                               "generate_corners");
        benchmark_topology_generate_offsets(
            "unstructured_generate_offsets_" + src_type,
            src_type);
        benchmark_topology_generate_offsets_inline(
            "unstructured_generate_offsets_inline_" + src_type,
            src_type);
    }
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
        BENCHMARK_NUM_WARMUP_ITERATIONS =
            static_cast<index_t>(std::atoll(argv[1]));
    }

    if (argc >= 3)
    {
        BENCHMARK_NUM_ITERATIONS =
            static_cast<index_t>(std::atoll(argv[2]));
    }

    if (argc >= 4)
    {
        BENCHMARK_DIM_SIZES.clear();

        for (int i = 3; i < argc; i++)
        {
            BENCHMARK_DIM_SIZES.push_back(
                static_cast<index_t>(std::atoll(argv[i])));
        }
    }

    for (const index_t dim_size : BENCHMARK_DIM_SIZES)
    {
        if (dim_size <= 1)
        {
            std::cerr
                << "ERROR: braid benchmark dimensions must be greater than "
                   "1; received "
                << dim_size << "."
                << std::endl;

            return EXIT_FAILURE;
        }
    }

    // TODO: Look at Caliper options related to OpenMP/GPU profiling.
    const std::string timestamp = benchmark::get_timestamp();

    Node cali_opts;
    cali_opts["config"] = "hatchet-region-profile(output=" + timestamp + ".cali)";

    annotations::initialize(cali_opts);

    const int result = RUN_ALL_TESTS();

    annotations::finalize();

    return result;
}