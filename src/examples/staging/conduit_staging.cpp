// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_staging.cpp
///
//-----------------------------------------------------------------------------
#include <conduit.hpp>
#include <conduit_relay_mpi_io.hpp>
#include <conduit_utils.hpp>
#include <iostream>
#include <cstdlib>
#include <cstring>

#ifdef CONDUIT_RELAY_IO_ADIOS_ENABLED
#include <conduit_relay_mpi_io_adios.hpp>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif
#include <mpi.h>

using conduit::Node;
using std::cout;
using std::endl;

//-----------------------------------------------------------------------------
bool streaming_transport(const std::string &transport)
{
    return (transport == "DATASPACES" ||
            transport == "DIMES" ||
            transport == "FLEXPATH" ||
            transport == "ICEE");
}

//-----------------------------------------------------------------------------
std::string write_verb(const std::string &transport)
{
    return streaming_transport(transport) ? "staging" : "writing";
}

//-----------------------------------------------------------------------------
int
producer(const Node &config, int nts, MPI_Comm comm)
{
    const char *prefix = "producer: ";
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    std::string path(config["path"].as_string());
    std::string selected_options(config["selected_options"].as_string());
    const Node &options = config[selected_options];
    bool streaming = streaming_transport(options["write/transport"].as_string());

    if(rank == 0)
    {
        cout << prefix << "Options: " << options.to_json() << endl;
        cout << prefix << "Starting." << endl;
    }
    // Remove the file if it exists.
    conduit::utils::remove_file(path);

    // Write multiple time steps to the same "file".
    for(int ts = 0; ts < nts; ++ts)
    {
        Node out;
        int idx = ts*100 + rank*10;
        out["a"] = idx + 1;
        out["b"] = idx + 2;
        out["c/d"] = idx + 3;
        out["c/e"] = idx + 4;
        out["f"] = 3.14159f * float(ts);

        if(rank == 0)
            cout << prefix << "Before add_step" << endl;
        // Add a new time step to the output file.
        conduit::relay::mpi::io::add_step(out, path, "adios", options, comm);

        // Show some progress
        if(rank == 0)
        {
            cout << prefix
                 << write_verb(options["write/transport"].as_string())
                 << " time step " << ts << endl;
        }

        if(!streaming)
        {
             MPI_Barrier(MPI_COMM_WORLD);
        }

        //conduit::utils::sleep(1);
    }

    return 0;
}

//-----------------------------------------------------------------------------
int
consumer(const Node &config, int nts, MPI_Comm comm)
{
    const char *prefix = "consumer: ";
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if(rank == 0)
        cout << prefix << "Starting." << endl;

    char outpath[1024];
    std::string path(config["path"].as_string());
    std::string protocol(config["protocol"].as_string());
    std::string format(config["outpath"].as_string());
    std::string selected_options(config["selected_options"].as_string());
    const Node &options = config[selected_options];
    bool streaming = streaming_transport(options["write/transport"].as_string());

    // Read from the producer and save out.
    for(int ts = 0; ts < nts; ++ts)
    {
        if(!streaming)
        {
             if(rank == 0)
                  cout << prefix << "Wait for timestep..." << endl;
             MPI_Barrier(MPI_COMM_WORLD);
        }

        // TODO: query the number of domains, distribute among ranks,
        // read each domain. For now, assume producer and consumer have
        // same number of ranks.
        int domain = rank;

        if(rank == 0)
            cout << prefix << "reading time step " << ts << endl;

        // Read in a domain of the current time step.
        Node in;

        if(streaming)
        {
            // Read "current" time step from the stream.
            conduit::relay::mpi::io::load(path, protocol, options, in, comm);
        }
        else
        {
            // Read a specific time step from the file.
            std::ostringstream tspath;
            tspath << path << ":" << ts << ":" << domain;
            conduit::relay::mpi::io::load(tspath.str(), protocol, options, in, comm);
        }

        // Make an output filename and save.
        snprintf(outpath, 1024, format.c_str(), ts, domain);
        conduit::relay::mpi::io::save(in, outpath, MPI_COMM_WORLD);

        if(rank == 0)
            cout << prefix << "writing time step " << ts << endl;
    }

    return 0;
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int retval = 0;
    std::string configfile("conduit_staging.json");
    int mode = 0;
    int nts = 10;
    int p = 1;
    int c = 1;
    bool isProducer = true;
    bool split = false;
    for(int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "--config") == 0)
        {
            configfile = std::string(argv[i+1]);
            ++i;
        }
        else if(strcmp(argv[i], "--nts") == 0)
        {
            int ivalue = atoi(argv[i+1]);
            if(ivalue > 0)
                nts = ivalue;
            ++i;
        }
        else if(strcmp(argv[i], "--split") == 0)
        {
            int p0 = 1, c0 = 1;
            bool bs = true;
            if(sscanf(argv[i+1], "%d:%d", &p0, &c0) == 2)
            {
                if((p0 + c0) == size &&
                   p0 > 0 &&
                   c0 > 0
                   && p0 == c0 // The consumer is not coded for M:N right now.
                   )
                {
                    p = p0;
                    c = c0;
                    split = true;
                    bs = false;
                }
            }
            if(bs && rank == 0)
            {
                cout << "Bad --split " << argv[i+1] << endl;
                mode = 3;
                break;
            }
            ++i;
        }
        else if(strcmp(argv[i], "--producer") == 0)
            isProducer = true;
        else if(strcmp(argv[i], "--consumer") == 0)
            isProducer = false;
        else if(strcmp(argv[i], "--help") == 0 ||
                strcmp(argv[i], "-h") == 0)
        {
            mode = 2;
        }
        else
        {
            if(rank == 0)
            {
                for(int j = 0; j < argc; j++)
                    cout << argv[j] << endl;
            }
            mode = 3;
            retval = -1;
            break;
        }
    }

    if(mode >= 2)
    {
        if(rank == 0)
        {
            if(mode > 2)
            {
                cout << "ARGS:" << endl;
                for(int j = 0; j < argc; j++)
                    cout << "\t" << argv[j] << endl;
                cout << endl;
            }

            cout << "Usage: " << argv[0] << " [--config file] [--nts number] "
                "[--split p:c] [--help]" << endl;
            cout << endl;
            cout << "--config file  The name of the config file." << endl;
            cout << "--nts number   The number of time steps." << endl;
            cout << "--split p:c    The ratio of producers to consumers." << endl;
            cout << "--help         Print help and exit." << endl;
        }
    }
    else
    {
        // Split (or dup) the communicator
        MPI_Comm comm;
        if(split)
        {
            isProducer = rank < p;
            MPI_Comm_split(MPI_COMM_WORLD, rank < p, 0, &comm);
        }
        else
        {
            p = size / 2;
            MPI_Comm_dup(MPI_COMM_WORLD, &comm);
        }

        try
        {
            conduit::relay::mpi::io::initialize(comm);

#ifdef CONDUIT_RELAY_IO_ADIOS_ENABLED
            Node opts;
            conduit::relay::mpi::io::adios_options(opts, comm);
            if(rank == 0)
                cout << "Default ADIOS options = " << opts.to_json() << endl;
#endif
            // Read the config file.
            Node config;
            conduit::relay::mpi::io::load(configfile, config, comm);
            if(rank == 0)
            {
                cout << "config = " << config.to_json() << endl;
            }

            if(isProducer)
            {
                retval = producer(config, nts, comm);
            }
            else
            {
                retval = consumer(config, nts, comm);
            }
        }
        catch(...)
        {
            cout << "Failed to read config file " << configfile << endl;
            retval = -2;
        }

        // Wait for *all* ranks to complete
        MPI_Barrier(MPI_COMM_WORLD);

        conduit::relay::mpi::io::finalize(comm);
        MPI_Comm_free(&comm);
    }

    MPI_Finalize();

    return retval;
}
