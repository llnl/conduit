// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_mpi_partition.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_relay.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_mpi_mesh.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"

#include "conduit_fmt/conduit_fmt.h"

#include <math.h>
#include <iostream>
#include "gtest/gtest.h"

#include <mpi.h>

#include "blueprint_test_helpers.hpp"

using std::cout;
using std::endl;

// Enable this macro to generate baselines.
//#define GENERATE_BASELINES

//-----------------------------------------------------------------------------
#ifdef _WIN32
const std::string sep("\\");
#else
const std::string sep("/");
#endif

//-----------------------------------------------------------------------------
std::string
baseline_dir()
{
    std::string path(__FILE__);
    auto idx = path.rfind(sep);
    if(idx != std::string::npos)
        path = path.substr(0, idx);
    path = path + sep + std::string("baselines");
    return path;
}

//-----------------------------------------------------------------------------
std::string test_name() { return std::string("t_blueprint_mpi_mesh_partition"); }

//-----------------------------------------------------------------------------
int
get_rank()
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
}

//-----------------------------------------------------------------------------
void
barrier()
{
    MPI_Barrier(MPI_COMM_WORLD);
}

//-----------------------------------------------------------------------------
// Include some helper function definitions
#include "blueprint_baseline_helpers.hpp"

//-----------------------------------------------------------------------------
void
make_offsets(const int ndomains[4], int offsets[4])
{
    offsets[0] = 0;
    for(int i = 1; i < 4; i++)
        offsets[i] = offsets[i-1] + ndomains[i-1];
}

//-----------------------------------------------------------------------------
void
distribute_domains(int rank, const int ndomains[4],
    const conduit::Node &src_domains, conduit::Node &domains)
{
    int offsets[4];
    make_offsets(ndomains, offsets);

    // Limit to just the domains for this rank.
    domains.reset();
    for(int i = 0; i < ndomains[rank]; i++)
    {
        conduit::Node &dom = domains.append();
        dom.set_external(src_domains[offsets[rank] + i]);
    }
}

//-----------------------------------------------------------------------------
void make_spiral(int ndoms, conduit::Node &n)
{
    conduit::blueprint::mesh::examples::spiral(ndoms, n);

    // Renumber the domains so we can check things more easily.
    for(int i = 0; i < ndoms; i++)
    {
        if(n[i].has_path("state/domain_id"))
        {
            n[i]["state/domain_id"].set(static_cast<conduit::int64>(i * 10));
        }
    }
}

//-----------------------------------------------------------------------------
std::string
rank_str(int rank)
{
    return conduit_fmt::format("{:02}",rank);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, all_ranks)
{
    const std::string base("all_ranks");

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make some domains that we'll distribute in different ways.
    conduit::Node spiral;
    make_spiral(7, spiral);
#if 0
    if(rank == 0)
        save_visit("spiral", spiral);
#endif
    conduit::Node input, output, options;
    int ndomains[] = {3,2,1,1};
    distribute_domains(rank, ndomains, spiral, input);

    // Go from 7 to 10 domains
    const char *opt0 =
"target: 10";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b00 = baseline_file((base + "_00_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b00, output);
#ifdef GENERATE_BASELINES
        make_baseline(b00, output);
#else
        EXPECT_EQ(compare_baseline(b00, output), true);
#endif
    }

    // Go from 7 to 4 domains
    const char *opt1 =
"target: 4";
    options.reset(); options.parse(opt1, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b01 = baseline_file((base + "_01_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b01, output);
#ifdef GENERATE_BASELINES
        make_baseline(b01, output);
#else
        EXPECT_EQ(compare_baseline(b01, output), true);
#endif
    }

    // Go from 7 to 2 domains (some ranks donate data but end up with none)
    const char *opt2 =
"target: 2";
    options.reset(); options.parse(opt2, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b02 = baseline_file((base + "_02_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b02, output);
#ifdef GENERATE_BASELINES
        make_baseline(b02, output);
#else
        EXPECT_EQ(compare_baseline(b02, output), true);
#endif
    }

    // Go from 7 to 1 domains
    const char *opt3 =
"target: 1";
    options.reset(); options.parse(opt3, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b03 = baseline_file((base + "_03_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b03, output);
#ifdef GENERATE_BASELINES
        make_baseline(b03, output);
#else
        EXPECT_EQ(compare_baseline(b03, output), true);
#endif
    }

    // Select an IJK subset of the meshes. Not all ranks will have data for each selection.
    const char *opt4 =
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 10\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 20\n"
"     start: [0,0,0]\n"
"     end:   [1,1,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 30\n"
"     start: [1,0,0]\n"
"     end:   [2,2,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 40\n"
"     start: [1,1,0]\n"
"     end:   [5,5,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 50\n"
"     start: [0,1,0]\n"
"     end:   [3,7,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 60\n"
"     start: [1,0,0]\n"
"     end:   [8,8,0]\n";
    options.reset(); options.parse(opt4, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b04 = baseline_file((base + "_04_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b04, output);
#ifdef GENERATE_BASELINES
        make_baseline(b04, output);
#else
        EXPECT_EQ(compare_baseline(b04, output), true);
#endif
    }

    // Select an IJK subset of the meshes. The last 2 ranks will not make any
    // selections since their domains are not selected.
    const char *opt5 =
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 10\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 20\n"
"     start: [0,0,0]\n"
"     end:   [1,1,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 30\n"
"     start: [1,0,0]\n"
"     end:   [2,2,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 40\n"
"     start: [1,1,0]\n"
"     end:   [5,5,0]\n";
    options.reset(); options.parse(opt5, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b05 = baseline_file((base + "_05_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b05, output);
#ifdef GENERATE_BASELINES
        make_baseline(b05, output);
#else
        EXPECT_EQ(compare_baseline(b05, output), true);
#endif
    }

    // Combine the subselected mesh
    std::string opt6(opt5);
    opt6 += "target: 2";
    options.reset(); options.parse(opt6, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b06 = baseline_file((base + "_06_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b06, output);
#ifdef GENERATE_BASELINES
        make_baseline(b06, output);
#else
        EXPECT_EQ(compare_baseline(b06, output), true);
#endif
    }
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, split_single)
{
    const std::string base("split_single");

    // Make single domain on rank 0 and then split across 4 ranks into N targets.

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make 20x20x1 cell mesh.
    conduit::Node input, output, options, msg;
    conduit::index_t vdims[] = {21,21,1};
    if(rank == 0)
    {
        conduit::blueprint::mesh::examples::braid("uniform",
            vdims[0], vdims[1], vdims[2], input);
        // Override with int64 because YAML loses int/uint information.
        conduit::int64 i100 = 100;
        input["state/cycle"].set(i100);
        input["state/domain_id"] = 0;
    }

    // Go from 1 to 2 domains
    const char *opt0 =
"target: 2";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b00 = baseline_file((base + "_00_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b00, output);
#ifdef GENERATE_BASELINES
        make_baseline(b00, output);
#else
        EXPECT_EQ(compare_baseline(b00, output), true);
#endif
    }

    // Go from 1 to 5 domains
    const char *opt1 =
"target: 5";
    options.reset(); options.parse(opt1, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b01 = baseline_file((base + "_01_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b01, output);
#ifdef GENERATE_BASELINES
        make_baseline(b01, output);
#else
        EXPECT_EQ(compare_baseline(b01, output), true);
#endif
    }
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, split_combine)
{
    const std::string base("split_combine");

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make 20x20x1 cell mesh.
    conduit::Node input, output, output2, options, msg;
    conduit::index_t vdims[] = {21,21,1};
    if(rank == 0)
    {
        conduit::blueprint::mesh::examples::braid("uniform",
            vdims[0], vdims[1], vdims[2], input);
        // Override with int64 because YAML loses int/uint information.
        conduit::int64 i100 = 100;
        input["state/cycle"].set(i100);
        input["state/domain_id"] = 0;
    }

    // Go from 1 to 5 domains then back to 1. Make sure it is still uniform.
    const char *opt0 =
"target: 5";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
const char *opt01 =
"target: 1";
    options.reset(); options.parse(opt01, "yaml");
    conduit::blueprint::mpi::mesh::partition(output, options, output2, MPI_COMM_WORLD);
    std::string b00 = baseline_file((base + "_00_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output2) > 0)
    {
        EXPECT_EQ(output2["topologies/mesh/type"].as_string(), "uniform");
        save_visit(b00, output2);
#ifdef GENERATE_BASELINES
        make_baseline(b00, output2);
#else
        EXPECT_EQ(compare_baseline(b00, output2), true);
#endif
    }
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, no_selections_apply)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make 20x20x1 cell mesh on rank 0
    conduit::Node input, output, output2, options, msg;
    conduit::index_t vdims[] = {21,21,1};
    if(rank == 0)
    {
        conduit::blueprint::mesh::examples::braid("uniform", vdims[0], vdims[1], vdims[2], input);
        // Override with int64 because YAML loses int/uint information.
        conduit::int64 i100 = 100;
        input["state/cycle"].set(i100);
        input["state/domain_id"] = 0;
    }

    // Make a selection that does not apply
    const char *opt0 =
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 99\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 0);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, permutations)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make some domains that we'll distribute in different ways.
    conduit::Node spiral;
    make_spiral(7, spiral);
    int nelem = 0;
    for(conduit::index_t i = 0; i < spiral.number_of_children(); i++)
    {
        const conduit::Node &topo = spiral[i]["topologies"][0];
        int n = conduit::blueprint::mesh::topology::length(topo);
        nelem += n;
    }

    // Distribute the 7 domains across 4 ranks in various ways and combine to
    // 1 domain.
    conduit::Node input, output, options;
    const char *opt0 = "target: 1";
    options.reset(); options.parse(opt0, "yaml");
    int ndomains[4] = {0,0,0,0};
    for(int r0 = 0; r0 < size; r0++)
    for(int r1 = 0; r1 < size; r1++)
    for(int r2 = 0; r2 < size; r2++)
    for(int r3 = 0; r3 < size; r3++)
    {
        if(r0+r1+r2+r3 == 7)
        {
            ndomains[0] = r0;
            ndomains[1] = r1;
            ndomains[2] = r2;
            ndomains[3] = r3;

            input.reset();
            distribute_domains(rank, ndomains, spiral, input);

            output.reset();
            conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
            if(rank == 0)
            {
                int ndoms = conduit::blueprint::mesh::number_of_domains(output);
                EXPECT_EQ(ndoms, 1);
                const conduit::Node &topo = output["topologies"][0];
                int n = conduit::blueprint::mesh::topology::length(topo);
                EXPECT_EQ(nelem, n);
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, different_selections_each_rank)
{
    const std::string base("different_selections_each_rank");

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make some domains that we'll distribute in different ways.
    conduit::Node spiral;
    make_spiral(7, spiral);
    conduit::Node input, output, options;
    int ndomains[] = {3,2,1,1};
    distribute_domains(rank, ndomains, spiral, input);

    // Select an IJK subset of the meshes. These selections apply to the
    // rank's local data.
    const char *opt0[4] = {
// rank 0
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 10\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 20\n"
"     start: [0,0,0]\n"
"     end:   [1,1,0]\n"
,
// rank 1
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 30\n"
"     start: [1,0,0]\n"
"     end:   [2,2,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 40\n"
"     start: [1,1,0]\n"
"     end:   [5,5,0]\n"
,
// rank 2
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 50\n"
"     start: [0,1,0]\n"
"     end:   [3,7,0]\n"
,
// rank 3
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 60\n"
"     start: [1,0,0]\n"
"     end:   [8,8,0]\n"
};

    // The ranks will initially disagree on the target number of domains and will
    // have to add their numbers of selections together.
    options.reset(); options.parse(opt0[rank], "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b00 = baseline_file((base + "_00_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b00, output);
#ifdef GENERATE_BASELINES
        make_baseline(b00, output);
#else
        EXPECT_EQ(compare_baseline(b00, output), true);
#endif
    }

    // Now, how about a version of selections where we only hit certain ranks.
    // They ought to agree that there are 4 selections even though none knows
    // them all.
    const char *opt1[4] = {
// rank 0
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 10\n"
"     start: [0,0,0]\n"
"     end:   [0,0,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 20\n"
"     start: [0,0,0]\n"
"     end:   [1,1,0]\n"
,
// rank 1
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 30\n"
"     start: [1,0,0]\n"
"     end:   [2,2,0]\n"
,
// rank 2
"selections:\n"
,
// rank 3
"selections:\n"
};
    options.reset(); options.parse(opt1[rank], "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    std::string b01 = baseline_file((base + "_01_") + rank_str(rank));
    if(conduit::blueprint::mesh::number_of_domains(output) > 0)
    {
        save_visit(b01, output);
#ifdef GENERATE_BASELINES
        make_baseline(b01, output);
#else
        EXPECT_EQ(compare_baseline(b01, output), true);
#endif
    }
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, invalid_selections)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make some domains that we'll distribute in different ways.
    conduit::Node spiral;
    make_spiral(7, spiral);
    conduit::Node input, output, options;
    int ndomains[] = {3,2,1,1};
    distribute_domains(rank, ndomains, spiral, input);

const char *opt0 =
"selections:\n"
"   -\n"
"     type: lksdhskdlf\n"
"   -\n"
"     type: sdfgsdf\n";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 0);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, different_targets)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Make some domains that we'll distribute in different ways.
    conduit::Node spiral;
    make_spiral(7, spiral);
    conduit::Node input, output, options;
    int ndomains[] = {3,2,1,1};
    distribute_domains(rank, ndomains, spiral, input);

    // Each rank had an invalid target. It will ignore them.
const char *opt0[4] = {
"target: invalid\n",
"target: invalid\n",
"target: invalid\n",
"target: invalid\n"
};
    options.reset(); options.parse(opt0[rank], "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    int ndom0[] = {1,1,1,4};
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), ndom0[rank]);

    // Each rank wants a different target. Partition will take the max. target=4.
const char *opt1[4] = {
"target: 1\n",
"target: 2\n",
"target: 3\n",
"target: 4\n"
};
    options.reset(); options.parse(opt1[rank], "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    int ndom1[] = {1,1,1,1};
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), ndom1[rank]);

    // Each rank wants a different target. They are all invalid. Ignore them.
const char *opt2[4] = {
"target: -1\n",
"target: -2\n",
"target: -3\n",
"target: -4\n"
};
    options.reset(); options.parse(opt2[rank], "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    int ndom2[] = {1,1,1,4};
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), ndom2[rank]);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_mpi_partition, field_selection)
{
    std::string base("field_selection");

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conduit::Node input, output, options;
    int masks[] = {1, 2, 4, 8};
    partition::make_field_selection_example(input, masks[rank]);

    const char *opt0 =
"selections:\n"
"   -\n"
"     type: field\n"
"     domain_id: 0\n"
"     field: selection_field\n"
"   -\n"
"     type: field\n"
"     domain_id: 1\n"
"     field: selection_field\n"
"   -\n"
"     type: field\n"
"     domain_id: 2\n"
"     field: selection_field\n"
"   -\n"
"     type: field\n"
"     domain_id: 3\n"
"     field: selection_field\n";

    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    int ndoms = conduit::blueprint::mesh::number_of_domains(output);
    int gndoms = 0;
    MPI_Allreduce(&ndoms, &gndoms, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    EXPECT_EQ(gndoms, 6);
    std::string b00 = baseline_file((base + "_00_") + rank_str(rank));
    save_visit(b00, output);
#ifdef GENERATE_BASELINES
    make_baseline(b00, output);
#else
    EXPECT_EQ(compare_baseline(b00, output), true);
#endif

    // Test domain_id: any
    const char *opt1 =
"selections:\n"
"   -\n"
"     type: field\n"
"     domain_id: any\n"
"     field: selection_field\n";
    options.reset(); options.parse(opt1, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    ndoms = conduit::blueprint::mesh::number_of_domains(output);
    gndoms = 0;
    MPI_Allreduce(&ndoms, &gndoms, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    EXPECT_EQ(gndoms, 6);
    std::string b01 = baseline_file((base + "_01_") + rank_str(rank));
    save_visit(b01, output);
#ifdef GENERATE_BASELINES
    make_baseline(b01, output);
#else
    EXPECT_EQ(compare_baseline(b01, output), true);
#endif

    // Test "target: 10". We can split field selections further as
    // explicit selections.
    const char *opt2 =
"selections:\n"
"   -\n"
"     type: field\n"
"     domain_id: any\n"
"     field: selection_field\n"
"target: 10\n";
    options.reset(); options.parse(opt2, "yaml");
    conduit::blueprint::mpi::mesh::partition(input, options, output, MPI_COMM_WORLD);
    ndoms = conduit::blueprint::mesh::number_of_domains(output);
    gndoms = 0;
    MPI_Allreduce(&ndoms, &gndoms, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    EXPECT_EQ(gndoms, 10);
    std::string b02 = baseline_file((base + "_02_") + rank_str(rank));
    save_visit(b02, output);
#ifdef GENERATE_BASELINES
    make_baseline(b02, output);
#else
    EXPECT_EQ(compare_baseline(b02, output), true);
#endif
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if(size == 4)
        result = RUN_ALL_TESTS();
    else
    {
        cout << "This program requires 4 ranks." << endl;
        result = -1;
    }
    MPI_Finalize();

    return result;
}
