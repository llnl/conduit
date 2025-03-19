// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mpi_mesh_relay.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mpi.hpp"
#include "conduit_relay.hpp"
#include "conduit_relay_mpi.hpp"
#include "conduit_relay_mpi_io.hpp"
#include "conduit_relay_mpi_io_blueprint.hpp"
#include "conduit_utils.hpp"
#include "conduit_fmt/conduit_fmt.h"

#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
#include "silo_test_utils.hpp"
#endif

#include <mpi.h>
#include <iostream>
#include <sstream>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::relay;
using namespace conduit::relay::mpi;
using namespace conduit::utils;

using namespace std;

//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, basic_use)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // only run this test if hdf5 is enabled
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("hdf5 is disabled, skipping hdf5 dependent test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    std::cout<<"Rank "<<par_rank<<" of "<<par_size<<"\n";
    index_t npts_x = 10;
    index_t npts_y = 10;
    index_t npts_z = 10;

    Node dset;
    blueprint::mesh::examples::braid("uniform",
                                      npts_x,
                                      npts_y,
                                      npts_z,
                                      dset);

    // the example data set has the bounds -10 to 10 in all dims
    // Offset this along x to create mpi 'pencil'

    dset["coordsets/coords/origin/x"] = -10.0 + 20.0 * par_rank;
    dset["state/domain_id"] = par_rank;
    // set cycle to 0, so we can construct the correct root file
    dset["state/cycle"] = 0;

    string protocol = "hdf5";
    string output_base = "test_blueprint_mpi_relay";
    string output_dir  = output_base + ".cycle_000000";
    string output_root = output_base + ".cycle_000000.root";
    
    if(par_rank == 0)
    {

        remove_path_if_exists(output_root);
        std::vector<std::string> domain_files = {"domain_000000.hdf5",
                                                 "domain_000001.hdf5"};
        std::string output_file;
        for(auto dfile :domain_files)
        {
            output_file = join_file_path(output_dir,
                                         dfile);
            remove_path_if_exists(output_file);
        }
        remove_path_if_exists(output_dir);
    }
    MPI_Barrier(comm);

    // what we want:
    // relay::mpi::io::blueprint::write_mesh(dset, output_path,"hdf5");
    conduit::relay::mpi::io::blueprint::write_mesh(dset,
                                                  output_base,
                                                  "hdf5",
                                                  comm);

    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(dset, output_base, "hdf5", comm));

    // read this back using read_mesh, should diff clean
    Node n_read, n_diff_info;
    conduit::relay::mpi::io::blueprint::read_mesh(output_root,
                                                  n_read,
                                                  comm);
    // diff == false, no diff == diff clean
    EXPECT_FALSE(dset.diff(n_read.child(0),n_diff_info));
}


//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, mpi_mesh_examples_braid)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // only run this test if hdf5 is enabled
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("hdf5 is disabled, skipping hdf5 dependent test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node dset, v_info;
    blueprint::mpi::mesh::examples::braid_uniform_multi_domain(dset,
                                                               comm);

    // check verify
    EXPECT_TRUE(blueprint::mpi::mesh::verify(dset,v_info,comm));

    // locally, expect 1 domain
    EXPECT_EQ(blueprint::mesh::number_of_domains(dset),1);
    // globally, expect par_size domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(dset,comm),par_size);

    string protocol = "hdf5";
    string output_base = "tout_blueprint_mpi_relay_braid_uniform_multi_dom";
    string output_dir  = output_base + ".cycle_000000";
    string output_root = output_base + ".cycle_000000.root";
    
    if(par_rank == 0)
    {

        remove_path_if_exists(output_root);
        std::vector<std::string> domain_files = {"domain_000000.hdf5",
                                                 "domain_000001.hdf5"};
        std::string output_file;
        for(auto dfile :domain_files)
        {
            output_file = join_file_path(output_dir,
                                         dfile);
            remove_path_if_exists(output_file);
        }
        remove_path_if_exists(output_dir);
    }
    MPI_Barrier(comm);
    // what we want:
    // relay::mpi::io::blueprint::write_mesh(dset, output_path,"hdf5");
    conduit::relay::mpi::io::blueprint::write_mesh(dset,
                                                  output_base,
                                                  "hdf5",
                                                  comm);

    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(dset, output_base, "hdf5", comm));

    // read this back using read_mesh, should diff clean
    Node n_read, n_diff_info;
    conduit::relay::mpi::io::blueprint::read_mesh(output_root,
                                                  n_read,
                                                  comm);
    // diff == false, no diff == diff clean
    EXPECT_FALSE(dset.diff(n_read.child(0),n_diff_info));
}

//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, mpi_mesh_examples_spiral_5doms)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // only run this test if hdf5 is enabled
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("hdf5 is disabled, skipping hdf5 dependent test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node dset, v_info;
    blueprint::mpi::mesh::examples::spiral_round_robin(5,
                                                       dset,
                                                       comm);

    // check verify
    EXPECT_TRUE(blueprint::mpi::mesh::verify(dset,v_info,comm));

    // locally, expect:
    //  rank 0: 3 domain
    //  rank 1: 2 domains
    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(dset),3);
        std::cout << "[rank 0] input domain ids: " << std::endl;
        dset.child(0)["state/domain_id"].print();
        dset.child(1)["state/domain_id"].print();
        dset.child(2)["state/domain_id"].print();
    }
    MPI_Barrier(comm);
    if(par_rank == 1)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(dset),2);
        std::cout << "[rank 1] input domain ids: " << std::endl;
        dset.child(0)["state/domain_id"].print();
        dset.child(1)["state/domain_id"].print();
    }
    MPI_Barrier(comm);

    // globally, expect 5 domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(dset,comm),5);

    string protocol = "hdf5";
    string output_base = "tout_blueprint_mpi_relay_spiral_mpi_dist_5doms";

    // make sure the files don't exist
    if(par_rank == 0)
    {
        const string output_dir  = output_base + ".cycle_000000";
        const string output_root = output_base + ".cycle_000000.root";

        // remove existing output
        remove_path_if_exists(output_dir);
        remove_path_if_exists(output_root);
    }
    MPI_Barrier(comm);

    conduit::relay::mpi::io::blueprint::write_mesh(dset,
                                                  output_base,
                                                  "hdf5",
                                                  comm);

    // read this back using read_mesh.
    // note the domain ids will change, so we don't expect
    // this to diff clean

    const string output_root = output_base + ".cycle_000000.root";
    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(dset, output_base, "hdf5", comm));
    Node n_read, n_diff_info;
    conduit::relay::mpi::io::blueprint::read_mesh(output_root,
                                                  n_read,
                                                  comm);

    // globally, expect 5 domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(n_read,comm),5);

    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(n_read),3);
        std::cout << "[rank 0] read domain ids: " << std::endl;
        n_read.child(0)["state/domain_id"].print();
        n_read.child(1)["state/domain_id"].print();
        n_read.child(2)["state/domain_id"].print();
        // expect we bring back domains 0 - 2
        EXPECT_EQ(n_read.child(0)["state/domain_id"].to_index_t(),0);
        EXPECT_EQ(n_read.child(1)["state/domain_id"].to_index_t(),1);
        EXPECT_EQ(n_read.child(2)["state/domain_id"].to_index_t(),2);
    }
    MPI_Barrier(comm);
    if(par_rank == 1)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(n_read),2);
        std::cout << "[rank 1] read domain ids: " << std::endl;
        n_read.child(0)["state/domain_id"].print();
        n_read.child(1)["state/domain_id"].print();
        // expect we bring back domains 3 - 4
        EXPECT_EQ(n_read.child(0)["state/domain_id"].to_index_t(),3);
        EXPECT_EQ(n_read.child(1)["state/domain_id"].to_index_t(),4);
    }
    MPI_Barrier(comm);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, mpi_mesh_examples_spiral_1dom)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // only run this test if hdf5 is enabled
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("hdf5 is disabled, skipping hdf5 dependent test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node dset, v_info;
    blueprint::mpi::mesh::examples::spiral_round_robin(1,
                                                       dset,
                                                       comm);

    // check verify
    EXPECT_TRUE(blueprint::mpi::mesh::verify(dset,v_info,comm));

    // locally, expect:
    //  rank 0: 1 domain
    //  rank 1: 0 domains
    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(dset),1);
    }
    else
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(dset),0);
    }

    // globally, expect par_size domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(dset,comm),1);

    string protocol = "hdf5";
    string output_base = "tout_blueprint_mpi_relay_spiral_mpi_dist_1dom";
    Node opts;
    opts["file_style"] = "multi_file";

    // make sure the files don't exist
    if(par_rank == 0)
    {
        string output_dir  = output_base + ".cycle_000000";
        string output_root = output_base + ".cycle_000000.root";

        // remove existing output
        remove_path_if_exists(output_dir);
        remove_path_if_exists(output_root);
    }
    MPI_Barrier(comm);

    conduit::relay::mpi::io::blueprint::write_mesh(dset,
                                                  output_base,
                                                  "hdf5",
                                                  opts,
                                                  comm);

    // read this back using read_mesh, should diff clean
    const string output_root = output_base + ".cycle_000000.root";
    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(dset, output_base, "hdf5", opts, comm));
    Node n_read, n_diff_info;
    conduit::relay::mpi::io::blueprint::read_mesh(output_root,
                                                  n_read,
                                                  comm);

    // globally, expect 1 domain
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(n_read,comm),1);

    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(n_read),1);
    }
    else
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(n_read),0);
    }

    EXPECT_EQ(dset.number_of_children(),n_read.number_of_children());
    for(conduit::index_t i=0;i < dset.number_of_children();i++)
    {
        dset.print();
        n_read.print();
        // diff == false, no diff == diff clean
        EXPECT_FALSE(dset.child(i).diff(n_read.child(i),n_diff_info));
    }

    // globally, expect par_size domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(n_read,comm),1);


}


//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, spiral_multi_file)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // only run this test if hdf5 is enabled
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("hdf5 is disabled, skipping hdf5 dependent test");
        return;
    }

    //
    // Set Up MPI
    //
    int par_rank;
    int par_size;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &par_rank);
    MPI_Comm_size(comm, &par_size);

    CONDUIT_INFO("Rank "
                  << par_rank
                  << " of "
                  << par_size
                  << " reporting");

    //
    // Create an example mesh.
    //
    Node data, verify_info;

    // use spiral , with 7 domains
    conduit::blueprint::mesh::examples::spiral(7,data);

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        data.remove(4);
        data.remove(4);
        data.remove(4);
    }
    else if(par_rank == 1)
    {
        data.remove(0);
        data.remove(0);
        data.remove(0);
        data.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    EXPECT_TRUE(conduit::blueprint::mesh::verify(data,verify_info));

    std::ostringstream oss;

    // lets try with -1 to 8 files.

    // nfiles less than 1 should trigger default case
    // (n output files = n domains)
    for(int nfiles=-1; nfiles < 9; nfiles++)
    {
        CONDUIT_INFO("[" << par_rank <<  "] test nfiles = " << nfiles);
        MPI_Barrier(comm);
        oss.str("");
        oss << "tout_relay_mpi_spiral_mesh_nfiles_" << nfiles;

        string output_base = oss.str();

        string output_dir  = output_base + ".cycle_000000";
        string output_root = output_base + ".cycle_000000.root";

        int nfiles_to_check = nfiles;
        if(nfiles <=0 || nfiles == 8) // expect 7 files (one per domain)
        {
            nfiles_to_check = 7;
        }

        if(par_rank == 0)
        {
            // remove existing root, output files and directory
            remove_path_if_exists(output_root);
            for(int i=0;i<nfiles_to_check;i++)
            {

                std::string fprefix = "file_";
                if(nfiles_to_check == 7)
                {
                    // in the n domains == n files case, the file prefix is
                    // domain_
                    fprefix = "domain_";
                }

                std::string output_file = conduit_fmt::format("{}{:06d}.hdf5",
                                join_file_path(output_base + ".cycle_000000",
                                               fprefix),
                                i);
                remove_path_if_exists(output_file);
            }

            remove_path_if_exists(output_dir);
        }

        MPI_Barrier(comm);

        Node opts;
        opts["number_of_files"] = nfiles;
        conduit::relay::mpi::io::blueprint::write_mesh(data,
                                                      output_base,
                                                      "hdf5",
                                                      opts,
                                                      comm);

        MPI_Barrier(comm);

        // count the files
        //  file_%06llu.{protocol}:/domain_%06llu/...


        EXPECT_TRUE(conduit::utils::is_directory(output_dir));
        EXPECT_TRUE(conduit::utils::is_file(output_root));
        EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(data, output_base, "hdf5", opts, comm));

        for(int i=0;i<nfiles_to_check;i++)
        {

            std::string fprefix = "file_";
            if(nfiles_to_check == 7)
            {
                // in the n domains == n files case, the file prefix is
                // domain_
                fprefix = "domain_";
            }

            std::string fcheck = conduit_fmt::format("{}{:06d}.hdf5",
                            join_file_path(output_base + ".cycle_000000",
                                           fprefix),
                            i);

            std::cout << " checking: " << fcheck << std::endl;
            EXPECT_TRUE(conduit::utils::is_file(fcheck));
        }

        // read the mesh back in diff to make sure we have the same data
        Node n_read, info;
        relay::mpi::io::blueprint::read_mesh(output_base + ".cycle_000000.root",
                                             n_read,
                                             comm);

        // rank 0 will have 4, rank 1 wil have 3
        int num_local_domains = 4;
        if(par_rank != 0)
        {
            num_local_domains = 3;
        }

        // total doms should be 7
        EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(n_read, comm), 7);

        std::cout << "par_rank " << par_rank << "  read # of children " << n_read.number_of_children();
        // in all cases we expect 7 domains to match
        for(int dom_idx =0; dom_idx <num_local_domains; dom_idx++)
        {
            EXPECT_FALSE(data.child(dom_idx).diff(n_read.child(dom_idx),info));
        }

    }

    // read this back using read_mesh
}


//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, spiral_root_only)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // only run this test if hdf5 is enabled
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("hdf5 is disabled, skipping hdf5 dependent test");
        return;
    }

    //
    // Set Up MPI
    //
    int par_rank;
    int par_size;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &par_rank);
    MPI_Comm_size(comm, &par_size);

    CONDUIT_INFO("Rank "
                  << par_rank
                  << " of "
                  << par_size
                  << " reporting");

    //
    // Create an example mesh, spiral , with 7 domains
    //
    Node data, verify_info;

    conduit::blueprint::mesh::examples::spiral(7,data);

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        data.remove(4);
        data.remove(4);
        data.remove(4);
    }
    else if(par_rank == 1)
    {
        data.remove(0);
        data.remove(0);
        data.remove(0);
        data.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    EXPECT_TRUE(conduit::blueprint::mesh::verify(data,verify_info));

    // try root only mode

    Node opts;
    opts["suffix"] = "none";
    opts["file_style"] = "root_only";
    const std::string tout_base = "tout_relay_mpi_spiral_root_only_hdf5";
    const std::string tout_root = tout_base + ".root";

    remove_path_if_exists(tout_root);
    conduit::relay::mpi::io::blueprint::save_mesh(data,
                                                  tout_base,
                                                  "hdf5",
                                                  opts,
                                                  comm);
    EXPECT_TRUE(conduit::utils::is_file(tout_root));
    EXPECT_EQ(tout_root, relay::mpi::io::blueprint::generate_root_filename(data, tout_base, "hdf5", opts, comm));

    // read the mesh back in diff to make sure we have the same data
    Node n_read, info;
    relay::mpi::io::blueprint::read_mesh(tout_root,
                                         n_read,
                                         comm);

    // rank 0 will have 4, rank 1 wil have 3
    int num_local_domains = 4;
    if(par_rank != 0)
    {
        num_local_domains = 3;
    }

    // total doms should be 7
    EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(n_read, comm), 7);

    std::cout << "par_rank " << par_rank << "  read # of children " << n_read.number_of_children();
    // in all cases we expect 7 domains to match
    for(int dom_idx =0; dom_idx <num_local_domains; dom_idx++)
    {
        EXPECT_FALSE(data.child(dom_idx).diff(n_read.child(dom_idx),info));
    }

}


//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, test_write_error_hang)
{

    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping save_read_mesh_truncate test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);


    std::string output_base = "tout_relay_mpi_mesh_save_load_truncate";

    Node data;

    // 6 doms (2 per mpi task)
    blueprint::mesh::examples::braid("uniform",
                                     2,
                                     2,
                                     2,
                                     data.append());

    blueprint::mesh::examples::braid("uniform",
                                     2,
                                     2,
                                     2,
                                     data.append());

    blueprint::mesh::examples::braid("uniform",
                                     2,
                                     2,
                                     2,
                                     data.append());

    // set the domain ids
    data.child(0)["state/domain_id"] = 0 + (1 * par_rank);
    data.child(1)["state/domain_id"] = 1 + (1 * par_rank);
    data.child(2)["state/domain_id"] = 2 + (1 * par_rank);

    // use 2 files
    Node opts;
    opts["number_of_files"] = 2;

    const std::string output_root = output_base + ".cycle_000100.root";
    remove_path_if_exists(output_root);
    remove_path_if_exists(output_base + ".cycle_000100/file_000000.hdf5");
    remove_path_if_exists(output_base + ".cycle_000100/file_000001.hdf5");

    conduit::relay::mpi::io::blueprint::write_mesh(data,
                                                   output_base,
                                                   "hdf5",
                                                   opts,
                                                   comm);

    // now save diff mesh to same file set, will fail with write_mesh
    // b/c hdf5 paths won't be compat

    data.reset();

    // 6 doms (2 per mpi task)
    blueprint::mesh::examples::braid("uniform",
                                     5,
                                     5,
                                     0,
                                     data.append());

    blueprint::mesh::examples::braid("uniform",
                                     5,
                                     5,
                                     0,
                                     data.append());

    blueprint::mesh::examples::braid("uniform",
                                     5,
                                     5,
                                     0,
                                     data.append());

    // set the domain ids
    data.child(0)["state/domain_id"] = 0 + (3 * par_rank);
    data.child(1)["state/domain_id"] = 1 + (3 * par_rank);
    data.child(2)["state/domain_id"] = 2 + (3 * par_rank);

    if(par_rank == 0)
    {
        for(int i=0;i<3;i++)
        {
            std::cout << "[rank 0] child " << i << " domain id = ";
            data.child(i)["state/domain_id"].print();
        }
    }
    MPI_Barrier(comm);

    if(par_rank == 1)
    {
        for(int i=0;i<3;i++)
        {
            std::cout << "[rank 1] child " << i << " domain id = ";
            data.child(i)["state/domain_id"].print();
        }
    }
    MPI_Barrier(comm);

    // non-trunk will error b/c leaf sizes aren't compat
    EXPECT_THROW( conduit::relay::mpi::io::blueprint::write_mesh(data,
                                                                 output_base,
                                                                 "hdf5",
                                                                 opts,
                                                                 comm),Error);

    // trunc will work
    conduit::relay::mpi::io::blueprint::save_mesh(data,
                                                  output_base,
                                                  "hdf5",
                                                  opts,
                                                  comm);

    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(data, output_base, "hdf5", opts, comm));

    Node n_read, info;
    // load mesh back back in and diff to check values
    relay::mpi::io::blueprint::load_mesh(output_root,
                                         n_read,
                                         comm);

    // total doms should be 6,
    EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(n_read,comm), 6);

    // each task has 3 local doms, check them
    for(int dom_idx=0; dom_idx < 3; dom_idx++)
    {
        EXPECT_FALSE(data.child(dom_idx).diff(n_read.child(dom_idx),info));
        if(par_rank == 0)
        {
            info.print();
        }
        MPI_Barrier(comm);
        if(par_rank == 1)
        {
            info.print();
        }
        MPI_Barrier(comm);
    }
}

//-----------------------------------------------------------------------------
// note: sparse topo tests are from ascent usecases
TEST(blueprint_mpi_relay, test_sparse_domains_case_1)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping test_sparse_domains_case_1 test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node data;
    ostringstream oss;

    // create mesh where each rank has three domains with different topos
    for(index_t d =0; d<3; d++)
    {
        Node &mesh = data.append();

        mesh["state/cycle"] = 0;

        oss.str("");
        oss << "my_coords_rank_" <<  par_rank << "_" << d;
        std::string c_name = oss.str();

        oss.str("");
        oss << "my_topo_rank_" <<  par_rank << "_" << d;
        std::string t_name = oss.str();

        oss.str("");
        oss << "my_field_rank_" <<  par_rank << "_" << d;
        std::string f_name = oss.str();

        // create the coordinate set
        mesh["coordsets"][c_name]["type"] = "uniform";
        mesh["coordsets"][c_name]["dims/i"] = 3;
        mesh["coordsets"][c_name]["dims/j"] = 3;
        mesh["coordsets"][c_name]["origin/x"] = -10.0;
        mesh["coordsets"][c_name]["origin/y"] = -10.0;
        mesh["coordsets"][c_name]["spacing/dx"] = 10.0;
        mesh["coordsets"][c_name]["spacing/dy"] = 10.0;

        mesh["topologies"][t_name]["type"] = "uniform";
        mesh["topologies"][t_name]["coordset"] = c_name;

        mesh["fields"][f_name]["association"] =  "element";
        mesh["fields"][f_name]["topology"] =  t_name;
        mesh["fields"][f_name]["values"].set(DataType::float64(4));

        float64 *ele_vals_ptr = mesh["fields"][f_name]["values"].value();

        for(int i=0;i<4;i++)
        {
            ele_vals_ptr[i] = float64(d);
        }
    }

    Node verify_info;
    EXPECT_TRUE(conduit::blueprint::mesh::verify(data,verify_info));

    Node opts; // empty for now
    const std::string tout_base = "tout_relay_mpi_sparse_case_1_hdf5";
    const std::string tout_root = tout_base + ".cycle_000000.root";

    remove_path_if_exists(tout_root);
    conduit::relay::mpi::io::blueprint::save_mesh(data,
                                                  tout_base,
                                                  "hdf5",
                                                  opts,
                                                  comm);
    EXPECT_TRUE(conduit::utils::is_file(tout_root));
    EXPECT_EQ(tout_root, relay::mpi::io::blueprint::generate_root_filename(data, tout_base, "hdf5", opts, comm));

}

//-----------------------------------------------------------------------------
// note: sparse topo tests are from ascent usecases
TEST(blueprint_mpi_relay, test_sparse_domains_case_2)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping test_sparse_domains_case_2 test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    //
    // Create an example mesh.
    //

    Node data;
    ostringstream oss;

    // rank 1 have 3 domains, rank zero none
    if(par_rank > 0)
    {
        // three domains with different topos
        for(index_t d =0; d<3; d++)
        {
            Node &mesh = data.append();

            mesh["state/cycle"] = 0;

            oss.str("");
            oss << "my_coords_rank_" <<  par_rank << "_" << d;
            std::string c_name = oss.str();

            oss.str("");
            oss << "my_topo_rank_" <<  par_rank << "_" << d;
            std::string t_name = oss.str();

            oss.str("");
            oss << "my_field_rank_" <<  par_rank << "_" << d;
            std::string f_name = oss.str();

            // create the coordinate set
            mesh["coordsets"][c_name]["type"] = "uniform";
            mesh["coordsets"][c_name]["dims/i"] = 3;
            mesh["coordsets"][c_name]["dims/j"] = 3;
            // add origin and spacing to the coordset (optional)
            mesh["coordsets"][c_name]["origin/x"] = -10.0;
            mesh["coordsets"][c_name]["origin/y"] = -10.0;
            mesh["coordsets"][c_name]["spacing/dx"] = 10.0;
            mesh["coordsets"][c_name]["spacing/dy"] = 10.0;

            // add the topology
            // this case is simple b/c it's implicitly derived from the coordinate set
            mesh["topologies"][t_name]["type"] = "uniform";
            // reference the coordinate set by name
            mesh["topologies"][t_name]["coordset"] = c_name;

            // add a simple element-associated field
            mesh["fields"][f_name]["association"] =  "element";
            // reference the topology this field is defined on by name
            mesh["fields"][f_name]["topology"] =  t_name;
            // set the field values, for this case we have 4 elements
            mesh["fields"][f_name]["values"].set(DataType::float64(4));

            float64 *ele_vals_ptr = mesh["fields"][f_name]["values"].value();

            for(int i=0;i<4;i++)
            {
                ele_vals_ptr[i] = float64(d);
            }
        }

        Node verify_info;
        EXPECT_TRUE(conduit::blueprint::mesh::verify(data,verify_info));
    }

    Node opts; // empty for now
    const std::string tout_base = "tout_relay_mpi_sparse_case_2_hdf5";
    const std::string tout_root = tout_base + ".cycle_000000.root";

    remove_path_if_exists(tout_root);
    conduit::relay::mpi::io::blueprint::save_mesh(data,
                                                  tout_base,
                                                  "hdf5",
                                                  opts,
                                                  comm);
    EXPECT_TRUE(conduit::utils::is_file(tout_root));
    EXPECT_EQ(tout_root, relay::mpi::io::blueprint::generate_root_filename(data, tout_base, "hdf5", opts, comm));



}

//-----------------------------------------------------------------------------
// note: sparse topo tests are from ascent usecases
TEST(blueprint_mpi_relay, test_sparse_domains_case_3)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping test_sparse_domains_case_3 test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    //
    // Create an example mesh.
    //

    Node data;
    ostringstream oss;

    // rank 1 have 3 domains, rank zero none
    if(par_rank > 0)
    {
        // three domains with different topos
        for(index_t d =0; d<3; d++)
        {
            Node &mesh = data.append();

            mesh["state/cycle"] = 0;

            oss.str("");
            oss << "my_coords_rank_" <<  par_rank << "_" << d;
            std::string c_name = oss.str();

            oss.str("");
            oss << "my_topo_rank_" <<  par_rank << "_" << d;
            std::string t_name = oss.str();

            oss.str("");
            oss << "my_field_rank_" <<  par_rank << "_" << d;
            std::string f_name = oss.str();

            // create the coordinate set
            mesh["coordsets"][c_name]["type"] = "uniform";
            mesh["coordsets"][c_name]["dims/i"] = 3;
            mesh["coordsets"][c_name]["dims/j"] = 3;
            // add origin and spacing to the coordset (optional)
            mesh["coordsets"][c_name]["origin/x"] = -10.0;
            mesh["coordsets"][c_name]["origin/y"] = -10.0;
            mesh["coordsets"][c_name]["spacing/dx"] = 10.0;
            mesh["coordsets"][c_name]["spacing/dy"] = 10.0;

            // add the topology
            // this case is simple b/c it's implicitly derived from the coordinate set
            mesh["topologies"][t_name]["type"] = "uniform";
            // reference the coordinate set by name
            mesh["topologies"][t_name]["coordset"] = c_name;

            // add a simple element-associated field
            mesh["fields"][f_name]["association"] =  "element";
            // reference the topology this field is defined on by name
            mesh["fields"][f_name]["topology"] =  t_name;
            // set the field values, for this case we have 4 elements
            mesh["fields"][f_name]["values"].set(DataType::float64(4));

            float64 *ele_vals_ptr = mesh["fields"][f_name]["values"].value();

            for(int i=0;i<4;i++)
            {
                ele_vals_ptr[i] = float64(d);
            }
        }

        Node verify_info;
        EXPECT_TRUE(conduit::blueprint::mesh::verify(data,verify_info));
    }

    Node opts;
    opts["suffix"] = "cycle";
    const std::string tout_base = "tout_relay_mpi_sparse_case_3_hdf5";
    const std::string tout_root = tout_base + ".cycle_000000.root";

    remove_path_if_exists(tout_root);
    conduit::relay::mpi::io::blueprint::save_mesh(data,
                                                  tout_base,
                                                  "hdf5",
                                                  opts,
                                                  comm);
    EXPECT_TRUE(conduit::utils::is_file(tout_root));
    EXPECT_EQ(tout_root, relay::mpi::io::blueprint::generate_root_filename(data, tout_base, "hdf5", opts, comm));


}


//-----------------------------------------------------------------------------
TEST(blueprint_mpi_relay, spiral_multi_file_yaml_json_hdf5_silo)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // only run this test if hdf5 is enabled
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("hdf5 is disabled, skipping hdf5 dependent test");
        return;
    }

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node dset, v_info;
    blueprint::mpi::mesh::examples::spiral_round_robin(5,
                                                       dset,
                                                       comm);

    // check verify
    EXPECT_TRUE(blueprint::mpi::mesh::verify(dset,v_info,comm));

    // locally, expect:
    //  rank 0: 3 domain
    //  rank 1: 2 domains
    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(dset),3);
        std::cout << "[rank 0] input domain ids: " << std::endl;
        dset.child(0)["state/domain_id"].print();
        dset.child(1)["state/domain_id"].print();
        dset.child(2)["state/domain_id"].print();
    }
    MPI_Barrier(comm);
    if(par_rank == 1)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(dset),2);
        std::cout << "[rank 1] input domain ids: " << std::endl;
        dset.child(0)["state/domain_id"].print();
        dset.child(1)["state/domain_id"].print();
    }
    MPI_Barrier(comm);

    // globally, expect 5 domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(dset,comm),5);

    string protocol = "hdf5";
    string output_base = "tout_blueprint_mpi_relay_spiral_mpi_dist_5doms";

    // make sure the files don't exist
    if(par_rank == 0)
    {
        string output_dir  = output_base + ".cycle_000000";
        string output_root = output_base + ".cycle_000000.root";

        // remove existing output
        remove_path_if_exists(output_dir);
        remove_path_if_exists(output_root);
    }
    MPI_Barrier(comm);

    conduit::relay::mpi::io::blueprint::write_mesh(dset,
                                                  output_base,
                                                  "hdf5",
                                                  comm);

    // read this back using read_mesh.
    // note the domain ids will change, so we don't expect
    // this to diff clean

    const string output_root = output_base + ".cycle_000000.root";
    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(dset, output_base, "hdf5", comm));
    Node n_read, n_diff_info;
    conduit::relay::mpi::io::blueprint::read_mesh(output_root,
                                                  n_read,
                                                  comm);

    // globally, expect 5 domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(n_read,comm),5);

    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(n_read),3);
        std::cout << "[rank 0] read domain ids: " << std::endl;
        n_read.child(0)["state/domain_id"].print();
        n_read.child(1)["state/domain_id"].print();
        n_read.child(2)["state/domain_id"].print();
        // expect we bring back domains 0 - 2
        EXPECT_EQ(n_read.child(0)["state/domain_id"].to_index_t(),0);
        EXPECT_EQ(n_read.child(1)["state/domain_id"].to_index_t(),1);
        EXPECT_EQ(n_read.child(2)["state/domain_id"].to_index_t(),2);
    }
    MPI_Barrier(comm);
    if(par_rank == 1)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(n_read),2);
        std::cout << "[rank 1] read domain ids: " << std::endl;
        n_read.child(0)["state/domain_id"].print();
        n_read.child(1)["state/domain_id"].print();
        // expect we bring back domains 3 - 4
        EXPECT_EQ(n_read.child(0)["state/domain_id"].to_index_t(),3);
        EXPECT_EQ(n_read.child(1)["state/domain_id"].to_index_t(),4);
    }
    MPI_Barrier(comm);
}
//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_relay, spiral_multi_file_yaml_json_hdf5_silo)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    bool silo_enabled = io_protos["io/protocols/conduit_silo_mesh"].as_string() == "enabled";

    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    std::vector<std::string> protocols = {"yaml","json"};
    if(hdf5_enabled)
    {
        protocols.push_back("hdf5");
    }

    if(silo_enabled)
    {
        protocols.push_back("silo");
    }

    for(auto protocol : protocols)
    {
        // use spiral rr, with  domains
        index_t n_total_domains = 6;
        Node data;
        blueprint::mpi::mesh::examples::spiral_round_robin(n_total_domains,
                                                           data,
                                                           comm);
        index_t n_local_domains = data.number_of_children();

        if(par_rank == 0)
        {
            std::cout << "testing protocol: " << protocol << std::endl;
        }

        std::string output_base = "tout_relay_mpi_spiral_mesh_save_proto_" + protocol;
        std::string output_dir  = output_base + ".cycle_000000";
        std::string output_root = output_base + ".cycle_000000.root";
        // clean up files from prior runs
        // remove existing root file, directory and any output files
        if(par_rank == 0)
        {
            remove_path_if_exists(output_root);
            for(int i=0;i<n_total_domains;i++)
            {
                std::string fprefix = "domain_";
                std::string output_file = conduit_fmt::format("{}{:06d}.{}",
                                join_file_path(output_base + ".cycle_000000",
                                               fprefix),
                            i,
                            protocol);
                remove_path_if_exists(output_file);
            }
            remove_path_if_exists(output_dir);
        }
        MPI_Barrier(comm);

        conduit::relay::mpi::io::blueprint::write_mesh(data,
                                                       output_base,
                                                       protocol,
                                                       comm);

        EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(data, output_base, protocol, comm));

        // make sure we can load back, this tests the auto detection
        // of the file type
        // read the mesh back in diff to make sure we have the same data
        Node n_read, info;
        conduit::relay::mpi::io::blueprint::read_mesh(output_root,
                                                      n_read,
                                                      comm);

        // note the domain ids will change, so we don't expect
        // this to diff clean

        for(int dom_idx =0; dom_idx <n_local_domains; dom_idx++)
        {
            data.child(dom_idx)["state/domain_id"]   = -42;
            n_read.child(dom_idx)["state/domain_id"] = -42;
        }

        #ifdef CONDUIT_RELAY_IO_SILO_ENABLED
        if(protocol == "silo")
        {
            // silo does not always preserve names or mesh types so we
            // use a test helper on the input to make things comparable
            for(int dom_idx =0; dom_idx <n_local_domains; dom_idx++)
            {
                silo_name_changer("mesh", data.child(dom_idx));
            }
        }
        #endif

        // in all cases we expect domains to match
        for(int dom_idx =0; dom_idx <n_local_domains; dom_idx++)
        {
            EXPECT_FALSE(data.child(dom_idx).diff(n_read.child(dom_idx),info,CONDUIT_EPSILON, true));
        }
    }
}


//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}

