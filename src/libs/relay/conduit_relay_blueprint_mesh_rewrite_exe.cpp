// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_blueprint_mesh_convert_exe.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_relay.hpp"

#include "conduit_relay_io_blueprint.hpp"
#include <iostream>
#include <stdlib.h>

#ifdef CONDUIT_REPLAY_BLUEPRINT_MESH_REWRITE_MPI
#include <mpi.h>
#include <conduit_relay_mpi.hpp>
#include <conduit_blueprint_mpi_mesh.hpp>
#include <conduit_relay_mpi_io_blueprint.hpp>
#endif


using namespace conduit;
using namespace conduit::relay;


//-----------------------------------------------------------------------------
void
usage()
{
    std::cout << "usage: conduit_relay_blueprint_mesh_convert{_mpi} {input root file} {output file base} [--write-protocol protocol] [--opts filename] [--help]"
              << std::endl << std::endl 
              << " optional arguments:"
              << std::endl
              << "  --write-protocol {relay protocol} Protocol used to write data file"
              << std::endl
              << "  --opts {options file}             Set options file used to initialize program."
              << std::endl
              << "  --help/-h                         Print usage and exit."
              << std::endl
              << std::endl;
}

//-----------------------------------------------------------------------------
int
parse_args(int argc,
           char *argv[],
           std::string &input_root,
           std::string &output_file,
           std::string &write_proto,
           std::string &opts_file)
{
    int retval = 0;
    for(int i=1; i < argc ; i++)
    {
        std::string arg_str(argv[i]);
        if(arg_str == "-h" || arg_str == "--help")
        {
            retval = -1;
            break;
        }
        else if(arg_str == "--write-protocol")
        {
            if(i+1 >= argc )
            {
                CONDUIT_ERROR("expected value following --write-protocol option");
            }

            write_proto = std::string(argv[i+1]);
            i++;
        }
        else if(arg_str == "--opts")
        {
            if(i+1 >= argc )
            {
                CONDUIT_ERROR("expected value following --opts option");
            }

            opts_file = std::string(argv[i+1]);
            i++;
        }
        else if(input_root == "")
        {
            input_root = arg_str;
        }
        else if(output_file == "")
        {
            output_file = arg_str;
        }
    }
    return retval;
}



//-----------------------------------------------------------------------------
int
main(int argc, char* argv[])
{
    if(argc < 2)
    {
        usage();
        return -1;
    }

    std::string input_root("");
    std::string output_base("");
    std::string write_proto("hdf5");
    std::string opts_file("");

    if(parse_args(argc,
                  argv,
                  input_root,
                  output_base,
                  write_proto,
                  opts_file) < 0)
    {
        usage();
        return -2;
    }

    int comm_size = 1;
    int comm_rank = 0;
#if defined(CONDUIT_REPLAY_BLUEPRINT_MESH_REWRITE_MPI)
    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);
#endif

    std::cout << comm_size << " " << comm_rank << std::endl;

    Node opts;
    if(opts_file != "")
    {
        // TODO MPI LOAD AND BROADCAST
        CONDUIT_INFO("Using opts file:" << opts_file);
        io::load(opts_file,opts);
    }

    if(input_root.empty())
    {
        CONDUIT_ERROR("no input file passed");
    }

    if(output_base.empty())
    {
        CONDUIT_ERROR("no output base passed");
    }

    Node data;
    Node timer_results;

    // load data

    utils::Timer t;
#if defined(CONDUIT_REPLAY_BLUEPRINT_MESH_REWRITE_MPI)
    relay::mpi::io::blueprint::load_mesh(input_root, data,comm);
    MPI_Barrier(comm);
#else
    relay::io::blueprint::load_mesh(input_root, data);
#endif
    timer_results["load_time"] = t.elapsed();

    // save data

    t.reset();
#if defined(CONDUIT_REPLAY_BLUEPRINT_MESH_REWRITE_MPI)
    if(opts.number_of_children() == 0)
    {
        relay::mpi::io::blueprint::save_mesh(data, output_base, write_proto, comm);
    }
    else
    {
        relay::mpi::io::blueprint::save_mesh(data, output_base, write_proto, opts, comm);
    }
    MPI_Barrier(comm);
#else
    if(opts.number_of_children() == 0)
    {
        relay::io::blueprint::save_mesh(data, output_base, write_proto);
    }
    else
    {
        relay::io::blueprint::save_mesh(data, output_base, write_proto, opts);
    }
#endif

    timer_results["save_time"] = t.elapsed();

    std::cout << timer_results.to_string("json",0,0," ", " ") << std::endl;

    return 0;
}
