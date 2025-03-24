// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_mpi_io_silo.cpp
///
//-----------------------------------------------------------------------------

#include "silo_test_utils.hpp"

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mpi.hpp"
#include "conduit_relay_mpi.hpp"
#include "conduit_relay_mpi_io.hpp"
#include "conduit_relay_mpi_io_silo.hpp"
#include "conduit_relay_mpi_io_blueprint.hpp"
#include "conduit_fmt/conduit_fmt.h"

#include <iostream>
#include "gtest/gtest.h"

#include <mpi.h>


using namespace conduit;
using namespace conduit::utils;
using namespace conduit::relay;

//-----------------------------------------------------------------------------
TEST(conduit_relay_mpi_io_silo, round_trip_braid)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    std::cout<<"Rank "<<par_rank<<" of "<<par_size<<"\n";

    index_t npts_x = 10;
    index_t npts_y = 10;
    index_t npts_z = 10;

    std::string mesh_type = "uniform";

    Node save_mesh;
    blueprint::mesh::examples::braid(mesh_type,
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     save_mesh);

    // the example data set has the bounds -10 to 10 in all dims
    // Offset this along x to create mpi 'pencil'

    save_mesh["coordsets/coords/origin/x"] = -10.0 + 20.0 * par_rank;
    save_mesh["state/domain_id"] = par_rank;
    // set cycle to 0, so we can construct the correct root file
    save_mesh["state/cycle"] = 0;

    Node load_mesh, info;

    const std::string basename = "silo_mpi_braid_" + mesh_type + "_3D";
    const std::string filename = basename + ".cycle_000000.root";
    EXPECT_EQ(filename, relay::mpi::io::blueprint::generate_root_filename(save_mesh, basename, "silo", comm));

    remove_path_if_exists(filename);
    relay::mpi::io::silo::write_mesh(save_mesh, basename, comm);
    relay::mpi::io::silo::read_mesh(filename, load_mesh, comm);

    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    silo_uniform_to_rect_conversion("coords", "mesh", save_mesh);

    // make changes to save mesh so the diff will pass
    silo_name_changer("mesh", save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh.child(0).number_of_children(), save_mesh.number_of_children());

    EXPECT_FALSE(load_mesh.child(0).diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_mpi_io_silo, mpi_mesh_examples_braid)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node save_mesh, v_info;
    blueprint::mpi::mesh::examples::braid_uniform_multi_domain(save_mesh,
                                                               comm);
    save_mesh["state/cycle"] = 0;

    // check verify
    EXPECT_TRUE(blueprint::mpi::mesh::verify(save_mesh,v_info, comm));

    // locally, expect 1 domain
    EXPECT_EQ(blueprint::mesh::number_of_domains(save_mesh),1);
    // globally, expect par_size domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(save_mesh,comm),par_size);

    std::string output_base = "silo_mpi_braid_uniform_multi_dom";
    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             output_base,
                                             comm);

    // read this back using read_mesh, should diff clean
    const std::string output_root = output_base + ".cycle_000000.root";
    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", comm));
    Node load_mesh, info;
    conduit::relay::mpi::io::silo::load_mesh(output_root,
                                             load_mesh,
                                             comm);

    silo_uniform_to_rect_conversion("coords", "mesh", save_mesh);

    // make changes to save mesh so the diff will pass
    silo_name_changer("mesh", save_mesh);

    // diff == false, no diff == diff clean
    EXPECT_FALSE(save_mesh.diff(load_mesh.child(0), info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_mpi_io_silo, mpi_mesh_examples_spiral_5doms)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node save_mesh, info;
    blueprint::mpi::mesh::examples::spiral_round_robin(5,
                                                       save_mesh,
                                                       comm);

    // check verify
    EXPECT_TRUE(blueprint::mpi::mesh::verify(save_mesh,info,comm));

    // locally, expect:
    //  rank 0: 3 domain
    //  rank 1: 2 domains
    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(save_mesh),3);
        std::cout << "[rank 0] input domain ids: " << std::endl;
        save_mesh.child(0)["state/domain_id"].print();
        save_mesh.child(1)["state/domain_id"].print();
        save_mesh.child(2)["state/domain_id"].print();
    }
    MPI_Barrier(comm);
    if(par_rank == 1)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(save_mesh),2);
        std::cout << "[rank 1] input domain ids: " << std::endl;
        save_mesh.child(0)["state/domain_id"].print();
        save_mesh.child(1)["state/domain_id"].print();
    }
    MPI_Barrier(comm);

    // globally, expect 5 domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(save_mesh,comm),5);

    std::string output_base = "silo_mpi_spiral_5doms";

    // make sure the files don't exist
    if(par_rank == 0)
    {
        std::string output_dir  = output_base + ".cycle_000000";
        std::string output_root = output_base + ".cycle_000000.root";

        // remove existing output
        remove_path_if_exists(output_dir);
        remove_path_if_exists(output_root);
    }
    MPI_Barrier(comm);

    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             output_base,
                                             comm);

    // read this back using read_mesh.
    // note the domain ids will change, so we don't expect
    // this to diff clean
    
    const std::string output_root = output_base + ".cycle_000000.root";
    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", comm));
    Node load_mesh;
    conduit::relay::mpi::io::silo::load_mesh(output_root,
                                             load_mesh,
                                             comm);

    // globally, expect 5 domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(load_mesh,comm),5);

    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(load_mesh),3);
        std::cout << "[rank 0] read domain ids: " << std::endl;
        load_mesh.child(0)["state/domain_id"].print();
        load_mesh.child(1)["state/domain_id"].print();
        load_mesh.child(2)["state/domain_id"].print();
        // expect we bring back domains 0 - 2
        EXPECT_EQ(load_mesh.child(0)["state/domain_id"].to_index_t(),0);
        EXPECT_EQ(load_mesh.child(1)["state/domain_id"].to_index_t(),1);
        EXPECT_EQ(load_mesh.child(2)["state/domain_id"].to_index_t(),2);
    }
    MPI_Barrier(comm);
    if(par_rank == 1)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(load_mesh),2);
        std::cout << "[rank 1] read domain ids: " << std::endl;
        load_mesh.child(0)["state/domain_id"].print();
        load_mesh.child(1)["state/domain_id"].print();
        // expect we bring back domains 3 - 4
        EXPECT_EQ(load_mesh.child(0)["state/domain_id"].to_index_t(),3);
        EXPECT_EQ(load_mesh.child(1)["state/domain_id"].to_index_t(),4);
    }
    MPI_Barrier(comm);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_mpi_io_silo, mpi_mesh_examples_spiral_1dom)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node save_mesh, info;
    blueprint::mpi::mesh::examples::spiral_round_robin(1,
                                                       save_mesh,
                                                       comm);

    add_matset_to_spiral(save_mesh, 1);

    // check verify
    EXPECT_TRUE(blueprint::mpi::mesh::verify(save_mesh,info,comm));

    // locally, expect:
    //  rank 0: 1 domain
    //  rank 1: 0 domains
    if(par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(save_mesh),1);
    }
    else
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(save_mesh),0);
    }

    // globally, expect par_size domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(save_mesh,comm),1);

    std::string output_base = "silo_mpi_spiral_1dom";
    Node opts;
    opts["file_style"] = "multi_file";

    // make sure the files don't exist
    if(par_rank == 0)
    {
        std::string output_dir  = output_base + ".cycle_000000";
        std::string output_root = output_base + ".cycle_000000.root";

        // remove existing output
        remove_path_if_exists(output_dir);
        remove_path_if_exists(output_root);
    }
    MPI_Barrier(comm);

    const std::string output_root = output_base + ".cycle_000000.root";
    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", opts, comm));

    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             output_base,
                                             opts,
                                             comm);

    // read this back using read_mesh, should diff clean
    Node load_mesh, n_diff_info;
    conduit::relay::mpi::io::silo::load_mesh(output_root,
                                             load_mesh,
                                             comm);

    // make changes to save mesh so the diff will pass
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        // get the matset for this domain
        Node &n_matset = save_mesh[child]["matsets"]["matset"];

        // clean up volume fractions
        Node vf_arr;
        n_matset["volume_fractions"].to_float64_array(vf_arr);
        n_matset["volume_fractions"].reset();
        n_matset["volume_fractions"].set(vf_arr);
        
        // cheat a little bit - we don't have these to start
        n_matset["sizes"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["sizes"]);
        n_matset["offsets"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["offsets"]);
    
        silo_name_changer("mesh", save_mesh[child]);
    }

    // globally, expect 1 domain
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(load_mesh,comm),1);

    if (par_rank == 0)
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(load_mesh),1);
    }
    else
    {
        EXPECT_EQ(blueprint::mesh::number_of_domains(load_mesh),0);
    }

    EXPECT_EQ(save_mesh.number_of_children(),load_mesh.number_of_children());
    for (conduit::index_t i=0;i < save_mesh.number_of_children();i++)
    {
        save_mesh.print();
        load_mesh.print();
        // diff == false, no diff == diff clean
        EXPECT_FALSE(save_mesh.child(i).diff(load_mesh.child(i),
                                             n_diff_info,
                                             CONDUIT_EPSILON,
                                             true));
    }

    // globally, expect par_size domains
    EXPECT_EQ(blueprint::mpi::mesh::number_of_domains(load_mesh, comm), 1);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_mpi_io_silo, spiral_multi_file)
{
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
    Node save_mesh, verify_info;

    // use spiral , with 7 domains
    conduit::blueprint::mesh::examples::spiral(7,save_mesh);
    add_matset_to_spiral(save_mesh, 7);

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        save_mesh.remove(4);
        save_mesh.remove(4);
        save_mesh.remove(4);
    }
    else if(par_rank == 1)
    {
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    EXPECT_TRUE(conduit::blueprint::mesh::verify(save_mesh,verify_info));

    std::ostringstream oss;

    // lets try with -1 to 8 files.

    // nfiles less than 1 should trigger default case
    // (n output files = n domains)
    for(int nfiles=-1; nfiles < 9; nfiles++)
    {
        CONDUIT_INFO("[" << par_rank <<  "] test nfiles = " << nfiles);
        MPI_Barrier(comm);
        oss.str("");
        oss << "silo_mpi_spiral_nfiles_" << nfiles;

        std::string output_base = oss.str();

        std::string output_dir  = output_base + ".cycle_000000";
        std::string output_root = output_base + ".cycle_000000.root";

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

                std::string output_file = conduit_fmt::format("{}{:06d}.silo",
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

        conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                                 output_base,
                                                 opts,
                                                 comm);

        EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", opts, comm));

        MPI_Barrier(comm);

        // count the files
        //  file_%06llu.{protocol}:/domain_%06llu/...


        EXPECT_TRUE(conduit::utils::is_directory(output_dir));
        EXPECT_TRUE(conduit::utils::is_file(output_root));

        for(int i=0;i<nfiles_to_check;i++)
        {

            std::string fprefix = "file_";
            if(nfiles_to_check == 7)
            {
                // in the n domains == n files case, the file prefix is
                // domain_
                fprefix = "domain_";
            }

            std::string fcheck = conduit_fmt::format("{}{:06d}.silo",
                            join_file_path(output_base + ".cycle_000000",
                                           fprefix),
                            i);

            std::cout << " checking: " << fcheck << std::endl;
            EXPECT_TRUE(conduit::utils::is_file(fcheck));
        }

        // read the mesh back in diff to make sure we have the same save_mesh
        Node load_mesh, info;
        relay::mpi::io::silo::load_mesh(output_base + ".cycle_000000.root",
                                        load_mesh,
                                        comm);

        Node diff_mesh;
        diff_mesh.set(save_mesh);

        // make changes to save mesh so the diff will pass
        for (index_t child = 0; child < diff_mesh.number_of_children(); child ++)
        {
            // get the matset for this domain
            Node &n_matset = diff_mesh[child]["matsets"]["matset"];

            // clean up volume fractions
            Node vf_arr;
            n_matset["volume_fractions"].to_float64_array(vf_arr);
            n_matset["volume_fractions"].reset();
            n_matset["volume_fractions"].set(vf_arr);

            // cheat a little bit - we don't have these to start
            n_matset["sizes"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["sizes"]);
            n_matset["offsets"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["offsets"]);

            silo_name_changer("mesh", diff_mesh[child]);
        }

        // rank 0 will have 4, rank 1 will have 3
        int num_local_domains = 4;
        if(par_rank != 0)
        {
            num_local_domains = 3;
        }

        // total doms should be 7
        EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(load_mesh, comm), 7);

        std::cout << "par_rank " << par_rank << "  read # of children " << load_mesh.number_of_children() << std::endl;
        // in all cases we expect 7 domains to match
        for (int dom_idx = 0; dom_idx < num_local_domains; dom_idx ++)
        {
            EXPECT_FALSE(diff_mesh.child(dom_idx).diff(load_mesh.child(dom_idx),
                                                       info,
                                                       CONDUIT_EPSILON,
                                                       true));
        }
    }

    // read this back using read_mesh
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_mpi_io_silo, spiral_root_only)
{
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
    Node save_mesh, verify_info;

    conduit::blueprint::mesh::examples::spiral(7, save_mesh);
    add_matset_to_spiral(save_mesh, 7);

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        save_mesh.remove(4);
        save_mesh.remove(4);
        save_mesh.remove(4);
    }
    else if(par_rank == 1)
    {
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    EXPECT_TRUE(conduit::blueprint::mesh::verify(save_mesh, verify_info));

    // try root only mode
    
    Node opts;
    opts["suffix"] = "none";
    opts["file_style"] = "root_only";
    std::string tout_base = "silo_mpi_spiral_root_only";

    remove_path_if_exists(tout_base + ".root");
    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             tout_base,
                                             opts,
                                             comm);
    EXPECT_TRUE(conduit::utils::is_file(tout_base + ".root"));

    const std::string filename = tout_base + ".root";
    EXPECT_EQ(filename, relay::mpi::io::blueprint::generate_root_filename(save_mesh, tout_base, "silo", opts, comm));

    // read the mesh back in diff to make sure we have the same data
    Node n_read, info;
    relay::mpi::io::silo::read_mesh(tout_base + ".root",
                                    n_read,
                                    comm);

    // rank 0 will have 4, rank 1 will have 3
    int num_local_domains = 4;
    if(par_rank != 0)
    {
        num_local_domains = 3;
    }

    // total doms should be 7
    EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(n_read, comm), 7);

    // make changes to save mesh so the diff will pass
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        // get the matset for this domain
        Node &n_matset = save_mesh[child]["matsets"]["matset"];

        // clean up volume fractions
        Node vf_arr;
        n_matset["volume_fractions"].to_float64_array(vf_arr);
        n_matset["volume_fractions"].reset();
        n_matset["volume_fractions"].set(vf_arr);
        
        // cheat a little bit - we don't have these to start
        n_matset["sizes"].set_external(n_read[child]["matsets"]["mesh_matset"]["sizes"]);
        n_matset["offsets"].set_external(n_read[child]["matsets"]["mesh_matset"]["offsets"]);
    
        silo_name_changer("mesh", save_mesh[child]);
    }

    std::cout << "par_rank " << par_rank << "  read # of children " << n_read.number_of_children();
    // in all cases we expect 7 domains to match
    for (int dom_idx = 0; dom_idx < num_local_domains; dom_idx ++)
    {
        EXPECT_FALSE(save_mesh.child(dom_idx).diff(n_read.child(dom_idx),
                                                   info,
                                                   CONDUIT_EPSILON,
                                                   true));
    }
}

//-----------------------------------------------------------------------------
// note: sparse topo tests are from ascent usecases
TEST(conduit_relay_mpi_io_silo, test_sparse_domains_case_1)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    Node data;
    std::ostringstream oss;

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
    std::string tout_base = "silo_mpi_sparse_case_1";

    remove_path_if_exists(tout_base + ".cycle_000000.root");
    conduit::relay::mpi::io::silo::save_mesh(data,
                                             tout_base,
                                             opts,
                                             comm);
    const std::string filename = tout_base + ".cycle_000000.root";
    EXPECT_TRUE(conduit::utils::is_file(filename));
    EXPECT_EQ(filename, relay::mpi::io::blueprint::generate_root_filename(data, tout_base, "silo", opts, comm));
}

//-----------------------------------------------------------------------------
// note: sparse topo tests are from ascent usecases
TEST(conduit_relay_mpi_io_silo, test_sparse_domains_case_2)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    //
    // Create an example mesh.
    //

    Node data;
    std::ostringstream oss;

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
    std::string tout_base = "silo_mpi_sparse_case_2";

    remove_path_if_exists(tout_base + ".cycle_000000.root");
    conduit::relay::mpi::io::silo::save_mesh(data,
                                             tout_base,
                                             opts,
                                             comm);
    const std::string filename = tout_base + ".cycle_000000.root";
    EXPECT_TRUE(conduit::utils::is_file(filename));
    EXPECT_EQ(filename, relay::mpi::io::blueprint::generate_root_filename(data, tout_base, "silo", opts, comm));
}

//-----------------------------------------------------------------------------
// note: sparse topo tests are from ascent usecases
TEST(conduit_relay_mpi_io_silo, test_sparse_domains_case_3)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank = mpi::rank(comm);
    int par_size = mpi::size(comm);

    //
    // Create an example mesh.
    //

    Node data;
    std::ostringstream oss;

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
    std::string tout_base = "silo_mpi_sparse_case_3";

    remove_path_if_exists(tout_base + ".cycle_000000.root");
    conduit::relay::mpi::io::silo::save_mesh(data,
                                             tout_base,
                                             opts,
                                             comm);
    const std::string filename = tout_base + ".cycle_000000.root";
    EXPECT_TRUE(conduit::utils::is_file(filename));
    EXPECT_EQ(filename, relay::mpi::io::blueprint::generate_root_filename(data, tout_base, "silo", opts, comm));
}

//-----------------------------------------------------------------------------
// 
// special case tests
// 

//-----------------------------------------------------------------------------
// var is not defined on a domain
// 
// tests the silo "EMPTY" capability
TEST(conduit_relay_mpi_io_silo, missing_domain_var)
{
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
    Node save_mesh, verify_info;

    // use spiral , with 7 domains
    conduit::blueprint::mesh::examples::spiral(7,save_mesh);

    // remove information for a particular domain
    save_mesh[2]["fields"].remove_child("dist");

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        save_mesh.remove(4);
        save_mesh.remove(4);
        save_mesh.remove(4);
    }
    else if(par_rank == 1)
    {
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    MPI_Barrier(comm);

    std::string output_base = "silo_mpi_missing_var";

    std::string output_dir  = output_base + ".cycle_000000";
    std::string output_root = output_base + ".cycle_000000.root";

    MPI_Barrier(comm);

    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             output_base,
                                             comm);

    MPI_Barrier(comm);

    EXPECT_TRUE(conduit::utils::is_directory(output_dir));
    EXPECT_TRUE(conduit::utils::is_file(output_root));

    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", comm));

    // read the mesh back in diff to make sure we have the same save_mesh
    Node load_mesh, info;
    relay::mpi::io::silo::load_mesh(output_root,
                                    load_mesh,
                                    comm);

    // make changes to save mesh so the diff will pass
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        silo_name_changer("mesh", save_mesh[child]);

        if (save_mesh[child]["state"]["domain_id"].as_int() == 2)
        {
            save_mesh[child].remove_child("fields");
        }
    }

    // rank 0 will have 4, rank 1 will have 3
    int num_local_domains = 4;
    if(par_rank != 0)
    {
        num_local_domains = 3;
    }

    // total doms should be 7
    EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(load_mesh, comm), 7);

    std::cout << "par_rank " << par_rank << "  read # of children " << load_mesh.number_of_children();
    // in all cases we expect 7 domains to match
    for(int dom_idx =0; dom_idx <num_local_domains; dom_idx++)
    {
        EXPECT_FALSE(save_mesh.child(dom_idx).diff(load_mesh.child(dom_idx),
                                                   info,
                                                   CONDUIT_EPSILON,
                                                   true));
    }
}

//-----------------------------------------------------------------------------
// matset is not defined on a domain
// 
// tests the silo "EMPTY" capability
TEST(conduit_relay_mpi_io_silo, missing_domain_matset)
{
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
    Node save_mesh, verify_info;

    // use spiral , with 7 domains
    conduit::blueprint::mesh::examples::spiral(7, save_mesh);
    add_matset_to_spiral(save_mesh, 7);

    // remove information for a particular domain
    save_mesh[2]["matsets"].remove_child("matset");

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        save_mesh.remove(4);
        save_mesh.remove(4);
        save_mesh.remove(4);
    }
    else if(par_rank == 1)
    {
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    MPI_Barrier(comm);

    std::string output_base = "silo_mpi_missing_matset";

    std::string output_dir  = output_base + ".cycle_000000";
    std::string output_root = output_base + ".cycle_000000.root";

    MPI_Barrier(comm);

    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             output_base,
                                             comm);

    MPI_Barrier(comm);

    EXPECT_TRUE(conduit::utils::is_directory(output_dir));
    EXPECT_TRUE(conduit::utils::is_file(output_root));

    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", comm));

    // read the mesh back in diff to make sure we have the same save_mesh
    Node load_mesh, info;
    relay::mpi::io::silo::load_mesh(output_root,
                                    load_mesh,
                                    comm);

    // make changes to save mesh so the diff will pass
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        if (save_mesh[child].has_path("matsets/matset"))
        {
            // get the matset for this domain
            Node &n_matset = save_mesh[child]["matsets"]["matset"];

            // clean up volume fractions
            Node vf_arr;
            n_matset["volume_fractions"].to_float64_array(vf_arr);
            n_matset["volume_fractions"].reset();
            n_matset["volume_fractions"].set(vf_arr);
            
            // cheat a little bit - we don't have these to start
            n_matset["sizes"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["sizes"]);
            n_matset["offsets"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["offsets"]);
        }

        if (save_mesh[child]["state"]["domain_id"].as_int() == 2)
        {
            save_mesh[child].remove_child("matsets");
        }

        silo_name_changer("mesh", save_mesh[child]);
    }

    // rank 0 will have 4, rank 1 will have 3
    int num_local_domains = 4;
    if(par_rank != 0)
    {
        num_local_domains = 3;
    }

    // total doms should be 7
    EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(load_mesh, comm), 7);

    std::cout << "par_rank " << par_rank << "  read # of children " << load_mesh.number_of_children();
    // in all cases we expect 7 domains to match
    for (int dom_idx =0; dom_idx <num_local_domains; dom_idx++)
    {
        EXPECT_FALSE(save_mesh.child(dom_idx).diff(load_mesh.child(dom_idx),
                                                   info,
                                                   CONDUIT_EPSILON,
                                                   true));
    }
}

//-----------------------------------------------------------------------------
// mesh is not defined on a domain
// 
// This case is much less interesting.
// data passes through the clean mesh filter which
// deletes domains that are missing topos.
// They simply are not part of the mesh and so silo 
// doesn't have to deal with it.
TEST(conduit_relay_mpi_io_silo, missing_domain_mesh_trivial)
{
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
    Node save_mesh;

    // use spiral , with 7 domains
    conduit::blueprint::mesh::examples::spiral(7,save_mesh);

    // remove information for a particular domain
    save_mesh[2]["topologies"].remove_child("topo");

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        save_mesh.remove(4);
        save_mesh.remove(4);
        save_mesh.remove(4);
    }
    else if(par_rank == 1)
    {
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    MPI_Barrier(comm);

    std::string output_base = "silo_mpi_missing_mesh";

    std::string output_dir  = output_base + ".cycle_000000";
    std::string output_root = output_base + ".cycle_000000.root";

    MPI_Barrier(comm);

    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             output_base,
                                             comm);

    MPI_Barrier(comm);

    EXPECT_TRUE(conduit::utils::is_directory(output_dir));
    EXPECT_TRUE(conduit::utils::is_file(output_root));

    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", comm));

    // read the mesh back in diff to make sure we have the same save_mesh
    Node load_mesh, info;
    relay::mpi::io::silo::load_mesh(output_root,
                                    load_mesh,
                                    comm);

    // make changes to save mesh so the diff will pass
    if (par_rank == 0)
    {
        save_mesh.remove(2);
        save_mesh.rename_child("domain_000003", "domain_000002");
        save_mesh[2]["state"]["domain_id"].reset();
        save_mesh[2]["state"]["domain_id"] = 2;
    }
    if (par_rank == 1)
    {
        save_mesh.rename_child("domain_000004", "domain_000003");
        save_mesh[0]["state"]["domain_id"].reset();
        save_mesh[0]["state"]["domain_id"] = 3;

        save_mesh.rename_child("domain_000005", "domain_000004");
        save_mesh[1]["state"]["domain_id"].reset();
        save_mesh[1]["state"]["domain_id"] = 4;

        save_mesh.rename_child("domain_000006", "domain_000005");
        save_mesh[2]["state"]["domain_id"].reset();
        save_mesh[2]["state"]["domain_id"] = 5;
    }
    
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        silo_name_changer("mesh", save_mesh[child]);
    }

    // rank 0 will have 3, rank 1 will have 3
    int num_local_domains = 3;

    // total doms should be 6
    EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(load_mesh, comm), 6);

    std::cout << "par_rank " << par_rank << "  read # of children " << load_mesh.number_of_children();
    // in all cases we expect 7 domains to match
    for (int dom_idx =0; dom_idx <num_local_domains; dom_idx++)
    {
        EXPECT_FALSE(save_mesh.child(dom_idx).diff(load_mesh.child(dom_idx),
                                                   info,
                                                   CONDUIT_EPSILON,
                                                   true));
    }
}

//-----------------------------------------------------------------------------
// mesh is not defined on a domain but there are multiple meshes
TEST(conduit_relay_mpi_io_silo, missing_domain_mesh)
{
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
    Node save_mesh, save_mesh2;

    // use spiral , with 7 domains
    conduit::blueprint::mesh::examples::spiral(7,save_mesh);
    conduit::blueprint::mesh::examples::spiral(7,save_mesh2);

    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        save_mesh[child]["coordsets"].rename_child("coords", "coords2");
        save_mesh[child]["topologies"]["topo"]["coordset"].reset();
        save_mesh[child]["topologies"]["topo"]["coordset"] = "coords2";
        save_mesh[child]["topologies"].rename_child("topo", "topo2");
        save_mesh[child]["fields"]["dist"]["topology"].reset();
        save_mesh[child]["fields"]["dist"]["topology"] = "topo2";
        save_mesh[child]["fields"].rename_child("dist", "dist2");

        save_mesh[child]["coordsets"]["coords"].set_external(save_mesh2[child]["coordsets"]["coords"]);
        save_mesh[child]["topologies"]["topo"].set_external(save_mesh2[child]["topologies"]["topo"]);
        save_mesh[child]["fields"]["dist"].set_external(save_mesh2[child]["fields"]["dist"]);
    }

    // remove information for a particular domain
    save_mesh[2]["topologies"].remove_child("topo");

    // rank 0 gets first 4 domains, rank 1 gets the rest
    if(par_rank == 0)
    {
        save_mesh.remove(4);
        save_mesh.remove(4);
        save_mesh.remove(4);

        save_mesh2.remove(4);
        save_mesh2.remove(4);
        save_mesh2.remove(4);
    }
    else if(par_rank == 1)
    {
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);
        save_mesh.remove(0);

        save_mesh2.remove(0);
        save_mesh2.remove(0);
        save_mesh2.remove(0);
        save_mesh2.remove(0);
    }
    else
    {
        // cyrus was wrong about 2 mpi ranks.
        EXPECT_TRUE(false);
    }

    MPI_Barrier(comm);

    std::string output_base = "silo_mpi_missing_mesh_nontrivial";

    std::string output_dir  = output_base + ".cycle_000000";
    std::string output_root = output_base + ".cycle_000000.root";

    MPI_Barrier(comm);

    conduit::relay::mpi::io::silo::save_mesh(save_mesh,
                                             output_base,
                                             comm);

    MPI_Barrier(comm);

    EXPECT_TRUE(conduit::utils::is_directory(output_dir));
    EXPECT_TRUE(conduit::utils::is_file(output_root));

    EXPECT_EQ(output_root, relay::mpi::io::blueprint::generate_root_filename(save_mesh, output_base, "silo", comm));

    // read the mesh back in diff to make sure we have the same save_mesh
    Node load_mesh, load_mesh2, info, opts;

    opts["mesh_name"] = "mesh_topo2";
    relay::mpi::io::silo::load_mesh(output_root, opts, load_mesh, comm);
    opts["mesh_name"] = "mesh_topo";
    relay::mpi::io::silo::load_mesh(output_root, opts, load_mesh2, comm);

    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh2, info));

    // make changes to save mesh so the diff will pass
    if (par_rank == 0)
    {
        save_mesh[2]["coordsets"].remove_child("coords");
        save_mesh[2]["fields"].remove_child("dist");
    }
    
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        silo_name_changer("mesh", save_mesh[child]);
    }

    // we must merge the two meshes in load mesh
    // this is tricky because one is missing a domain
    if (par_rank == 0)
    {
        for (index_t dom_id = 0; dom_id < 4; dom_id ++)
        {
            index_t load_mesh_dom_id = dom_id;
            index_t load_mesh2_dom_id = dom_id;

            if (dom_id > 2) // load mesh 2 has one less domain
            {
                load_mesh2_dom_id = dom_id - 1;
            }

            std::cout << load_mesh_dom_id << ", " << load_mesh2_dom_id << std::endl;

            if (dom_id != 2) // nothing to do for domain 2
            {
                load_mesh[load_mesh_dom_id]["coordsets"]["mesh_topo"].set_external(load_mesh2[load_mesh2_dom_id]["coordsets"]["mesh_topo"]);
                load_mesh[load_mesh_dom_id]["topologies"]["mesh_topo"].set_external(load_mesh2[load_mesh2_dom_id]["topologies"]["mesh_topo"]);
                load_mesh[load_mesh_dom_id]["fields"]["mesh_dist"].set_external(load_mesh2[load_mesh2_dom_id]["fields"]["mesh_dist"]);
            }
        }
    }
    if (par_rank == 1)
    {
        for (index_t dom_id = 0; dom_id < 3; dom_id ++)
        {
            load_mesh[dom_id]["coordsets"]["mesh_topo"].set_external(load_mesh2[dom_id]["coordsets"]["mesh_topo"]);
            load_mesh[dom_id]["topologies"]["mesh_topo"].set_external(load_mesh2[dom_id]["topologies"]["mesh_topo"]);
            load_mesh[dom_id]["fields"]["mesh_dist"].set_external(load_mesh2[dom_id]["fields"]["mesh_dist"]);
        }
    }

    // rank 0 will have 4, rank 1 will have 3
    int num_local_domains = 4;
    if(par_rank != 0)
    {
        num_local_domains = 3;
    }

    // total doms should be 7
    EXPECT_EQ( conduit::blueprint::mpi::mesh::number_of_domains(load_mesh, comm), 7);

    std::cout << "par_rank " << par_rank << "  read # of children " << load_mesh.number_of_children();
    // in all cases we expect 7 domains to match
    for(int dom_idx =0; dom_idx <num_local_domains; dom_idx++)
    {
        EXPECT_FALSE(save_mesh.child(dom_idx).diff(load_mesh.child(dom_idx),
                                                   info,
                                                   CONDUIT_EPSILON,
                                                   true));
    }
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    conduit::relay::mpi::io::initialize(MPI_COMM_WORLD);
    result = RUN_ALL_TESTS();
    conduit::relay::mpi::io::finalize(MPI_COMM_WORLD);
    MPI_Finalize();
    return result;
}
