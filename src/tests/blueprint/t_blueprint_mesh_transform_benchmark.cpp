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

std::vector<index_t> BENCHMARK_DIM_SIZES = {2, 4};
index_t BENCHMARK_NUM_WARMUP_ITERATIONS  = 10;
index_t BENCHMARK_NUM_ITERATIONS         = 100;

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
// 2D braid element types error out if given a non-zero z dimension
index_t
braid_bound_npts_z(const std::string &mesh_type, index_t npts_z)
{
    return (mesh_type == "tris" || mesh_type == "quads") ? 0 : npts_z;
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
                                     braid_bound_npts_z(src_type, config.dim_size),
                                     src);
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
void
benchmark_topo_generate_single(const char *name,
                               const std::string &src_type,
                               void (*transform)(const Node &, Node &))
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_unstructured_braid_dataset(src_type, config, src);
    };

    // Perform a transform on the source Node
    auto run = [=](const Node &src) {
        Node dst;
        transform(src["topologies"].child(0), dst);
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
benchmark_topo_generate_maps(const char *name,
                             const std::string &src_type,
                             void (*transform)(const Node &, Node &, Node &, Node &))
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_unstructured_braid_dataset(src_type, config, src);
    };

    // Perform a transform on the source Node
    auto run = [=](const Node &src) {
        Node dst, s2dmap, d2smap;
        transform(src["topologies"].child(0), dst, s2dmap, d2smap);
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
benchmark_topo_generate_topo_coords_maps(const char *name,
                                         const std::string &src_type,
                                         void (*transform)(const Node &, Node &, Node &, Node &, Node &))
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_unstructured_braid_dataset(src_type, config, src);
    };

    // Perform a transform on the source Node
    auto run = [=](const Node &src) {
        Node topo_dst, coords_dst, s2dmap, d2smap;
        transform(src["topologies"].child(0), topo_dst, coords_dst, s2dmap, d2smap);
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
benchmark_topo_generate_offsets_inline(const char *name,
                                       const std::string &src_type)
{
    // Create the source Node once and reuse it for each iteration
    auto setup = [&](const benchmark::ExecConfig &config, Node &src) {
        make_unstructured_braid_dataset(src_type, config, src);
    };

    // generate_offsets_inline mutates its argument in place and is a no-op if
    // offsets are already present, so each iteration needs its own copy of
    // the source topology to do real work.
    auto run = [=](const Node &src) {
        Node topo_copy;
        topo_copy.set(src["topologies"].child(0));
        blueprint::mesh::topology::unstructured::generate_offsets_inline(topo_copy);
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
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    benchmark_topology_transform("topology_uniform_to_unstructured",
                                 "uniform",
                                 blueprint::mesh::topology::uniform::to_unstructured);
    benchmark_topology_transform("topology_rectilinear_to_unstructured",
                                 "rectilinear",
                                 blueprint::mesh::topology::rectilinear::to_unstructured);
    benchmark_topology_transform("topology_structured_to_unstructured",
                                 "structured",
                                 blueprint::mesh::topology::structured::to_unstructured);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_transform_benchmark, unstructured_generate_transforms)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    for (const std::string &src_type : {"quads", "hexs"})
    {
        benchmark_topo_generate_single(("to_polytopal_" + src_type).c_str(),
            src_type,
            blueprint::mesh::topology::unstructured::to_polytopal);
        benchmark_topo_generate_single(("generate_offsets_" + src_type).c_str(),
            src_type,
            static_cast<void (*)(const Node &, Node &)>(
                blueprint::mesh::topology::unstructured::generate_offsets));
        benchmark_topo_generate_maps(("generate_points_" + src_type).c_str(),
            src_type,
            blueprint::mesh::topology::unstructured::generate_points);
        benchmark_topo_generate_maps(("generate_lines_" + src_type).c_str(),
            src_type,
            blueprint::mesh::topology::unstructured::generate_lines);
        benchmark_topo_generate_maps(("generate_faces_" + src_type).c_str(),
            src_type,
            blueprint::mesh::topology::unstructured::generate_faces);
        benchmark_topo_generate_topo_coords_maps(("generate_centroids_" + src_type).c_str(),
            src_type,
            blueprint::mesh::topology::unstructured::generate_centroids);
        benchmark_topo_generate_topo_coords_maps(("generate_sides_" + src_type).c_str(),
            src_type,
            static_cast<void (*)(const Node &, Node &, Node &, Node &, Node &)>(
                blueprint::mesh::topology::unstructured::generate_sides));
        benchmark_topo_generate_topo_coords_maps(("generate_corners_" + src_type).c_str(),
            src_type,
            blueprint::mesh::topology::unstructured::generate_corners);
        benchmark_topo_generate_offsets_inline(("generate_offsets_inline_" + src_type).c_str(),
            src_type);
    }
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
