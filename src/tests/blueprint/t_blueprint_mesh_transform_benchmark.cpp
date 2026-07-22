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

#include <iostream>
#include <utility>

using namespace conduit;

// Intentionally small by default, to minimize CI time spent benchmarking
std::vector<index_t> BENCHMARK_DIM_SIZES = {2, 4};
index_t BENCHMARK_NUM_WARMUP_ITERATIONS  = 10;
index_t BENCHMARK_NUM_ITERATIONS         = 100;

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
                   Node &src)
{
    blueprint::mesh::examples::braid(src_type,
                                     config.dim_size,
                                     config.dim_size,
                                     config.dim_size,
                                     src);
}

//-----------------------------------------------------------------------------
// 2D braid element types error out if given a non-zero z dimension,
// so clamp npts_z to zero for them.
index_t
braid_npts_z(const std::string &mesh_type,
             index_t npts_z)
{
    if (mesh_type == "tris" || mesh_type == "quads")
    {
        return 0;
    }
    return npts_z;
}

//-----------------------------------------------------------------------------
void
make_unstructured_braid_dataset(const std::string &src_type,
                                const benchmark::ExecConfig &config,
                                Node &src)
{
    blueprint::mesh::examples::braid(src_type,
                                     config.dim_size,
                                     config.dim_size,
                                     braid_npts_z(src_type, config.dim_size),
                                     src);
}

//-----------------------------------------------------------------------------
void
benchmark_coordset_transform(const std::string &name,
                             const std::string &src_type,
                             void (*transform)(const Node &, Node &))
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_braid_dataset(src_type, config, src);
    };

    // Perform a transform on the source Node
    auto run = [&](const Node &src, Node &dst) {
        transform(src["coordsets"].child(0), dst);
    };

    run_benchmark(name, setup, run);
}

//-----------------------------------------------------------------------------
// Benchmarks blueprint::mesh::convert(), which covers every topology and
// unstructured-generate transform below.
void
benchmark_mesh_convert(const std::string &name,
                       void (*make_dataset)(const std::string &, const benchmark::ExecConfig &, Node &),
                       const std::string &src_type,
                       const std::string &target)
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_dataset(src_type, config, src);
    };

    Node options;
    options["target"] = target;

    // Perform a transform on the source Node
    auto run = [&](const Node &src, Node &dst) {
        blueprint::mesh::convert(src, options, dst);
    };

    run_benchmark(name, setup, run);
}

//-----------------------------------------------------------------------------
// generate_offsets has no blueprint::mesh::convert() target.
void
benchmark_topology_generate_offsets(const std::string &name,
                                    const std::string &src_type)
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_unstructured_braid_dataset(src_type, config, src);
    };

    // Perform a transform on the source Node
    auto run = [&](const Node &src, Node &dst) {
        blueprint::mesh::topology::unstructured::generate_offsets(src["topologies"].child(0),
                                                                  dst);
    };

    run_benchmark(name, setup, run);
}

//-----------------------------------------------------------------------------
// generate_offsets_inline also has no blueprint::mesh::convert() target.
void
benchmark_topology_generate_offsets_inline(const std::string &name,
                                           const std::string &src_type)
{
    // generate_offsets_inline mutates its argument in place and is a no-op
    // if offsets are already present, so keep a working copy of the source
    // topology alive across iterations and strip its offsets before each
    // run.
    Node topo;

    // Create the working topology once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_unstructured_braid_dataset(src_type, config, src);
        topo.set(src["topologies"].child(0));
    };

    // Perform the transform on the working topology
    auto run = [&](const Node &, Node &) {
        if (topo.has_path("elements/offsets"))
        {
            topo["elements"].remove_child("offsets");
        }
        blueprint::mesh::topology::unstructured::generate_offsets_inline(topo);
    };

    run_benchmark(name, setup, run);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, coordset_transforms)
{
    // This is not an exhaustive benchmark of the coordset transforms;
    // it only includes the ones that made sense to port to the device
    // execution model.

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
    benchmark_mesh_convert("topology_uniform_to_unstructured",
                           make_braid_dataset,
                           "uniform",
                           "unstructured");
    benchmark_mesh_convert("topology_rectilinear_to_unstructured",
                           make_braid_dataset,
                           "rectilinear",
                           "unstructured");
    benchmark_mesh_convert("topology_structured_to_unstructured",
                           make_braid_dataset,
                           "structured",
                           "unstructured");
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, unstructured_generate_transforms)
{
    // This is not an exhaustive benchmark of the unstructured generate
    // transforms; it only includes the ones that made sense to port to the
    // device execution model.

    CONDUIT_ANNOTATE_MARK_FUNCTION;
    for (const std::string &src_type : {"quads", "hexs"})
    {
        benchmark_mesh_convert("unstructured_to_polytopal_" + src_type,
                               make_unstructured_braid_dataset,
                               src_type,
                               "polytopal");
        benchmark_mesh_convert("unstructured_generate_points_" + src_type,
                               make_unstructured_braid_dataset,
                               src_type,
                               "generate_points");
        benchmark_mesh_convert("unstructured_generate_lines_" + src_type,
                               make_unstructured_braid_dataset,
                               src_type,
                               "generate_lines");
        benchmark_mesh_convert("unstructured_generate_faces_" + src_type,
                               make_unstructured_braid_dataset,
                               src_type,
                               "generate_faces");
        benchmark_mesh_convert("unstructured_generate_centroids_" + src_type,
                               make_unstructured_braid_dataset,
                               src_type,
                               "generate_centroids");
        benchmark_mesh_convert("unstructured_generate_sides_" + src_type,
                               make_unstructured_braid_dataset,
                               src_type,
                               "generate_sides");
        benchmark_mesh_convert("unstructured_generate_corners_" + src_type,
                               make_unstructured_braid_dataset,
                               src_type,
                               "generate_corners");
        benchmark_topology_generate_offsets("unstructured_generate_offsets_" + src_type,
                                            src_type);
        benchmark_topology_generate_offsets_inline("unstructured_generate_offsets_inline_" + src_type,
                                                   src_type);
    }
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    if (!annotations::supported())
    {
        std::cout << "WARNING: conduit was built without Caliper support, "
                     "so this benchmark will run but will not produce any "
                     "timing output (.cali file)." << std::endl;
    }

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
