// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_mpi_io.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay_mpi_io.hpp"

//-----------------------------------------------------------------------------
// standard lib includes
//-----------------------------------------------------------------------------
#include <iostream>

// Include a helper function for figuring out protocols.
#include "conduit_relay_mpi_io_identify_protocol.hpp"

// includes for optional features
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
#include "conduit_relay_mpi_io_hdf5.hpp"
#endif

#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
#include "conduit_relay_mpi_io_silo.hpp"
#endif

#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
#include "conduit_relay_mpi_io_adios.hpp"
#endif


#include "conduit_relay_io_handle.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay --
//-----------------------------------------------------------------------------
namespace relay
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
//-----------------------------------------------------------------------------
namespace mpi
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi::io --
//-----------------------------------------------------------------------------
namespace io
{

//---------------------------------------------------------------------------//
std::string
about(MPI_Comm comm)
{
    Node n;
    io::about(n, comm);
    return n.to_yaml();
}

//---------------------------------------------------------------------------//
void
about(Node &n, MPI_Comm comm)
{
    n.reset();
    Node &io_protos = n["io/protocols"];

    // json io
    io_protos["json"] = "enabled";
    io_protos["conduit_json"] = "enabled";
    io_protos["conduit_base64_json"] = "enabled";
    
    // yaml io
    io_protos["yaml"] = "enabled";
    

    // standard binary io
    io_protos["conduit_bin"] = "enabled";

#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
    // straight hdf5 
    io_protos["hdf5"] = "enabled";
    
    hdf5_options(n["io/options/hdf5"]);
#else
    // straight hdf5 
    io_protos["hdf5"] = "disabled";
#endif
    
    // silo
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
    // node is packed into two silo objects
    io_protos["conduit_silo"] = "enabled";
#else
    // node is packed into two silo objects
    io_protos["conduit_silo"] = "disabled";
#endif
    
    // silo mesh aware
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
    io_protos["conduit_silo_mesh"] = "enabled";
#else
    io_protos["conduit_silo_mesh"] = "disabled";
#endif

    // ADIOS aware
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
    io_protos["adios"] = "enabled";
    adios_options(n["io/options/adios"], comm);
#else
    CONDUIT_UNUSED(comm);
    io_protos["adios"] = "disabled";
#endif
}


//---------------------------------------------------------------------------//
void
initialize(MPI_Comm comm)
{
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
    adios_initialize_library(comm);
#else 
    CONDUIT_UNUSED(comm);
#endif
}

//---------------------------------------------------------------------------//
void
finalize(MPI_Comm comm)
{
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
    adios_finalize_library(comm);
#else 
    CONDUIT_UNUSED(comm);
#endif
}

//---------------------------------------------------------------------------//
void 
save(const Node &node,
     const std::string &path,
     MPI_Comm comm)
{
    std::string protocol;
    identify_protocol(path,protocol);
    save(node,path,protocol,comm);
}

//---------------------------------------------------------------------------//
void 
save_merged(const Node &node,
            const std::string &path,
            MPI_Comm comm)
{
    std::string protocol;
    identify_protocol(path,protocol);
    save_merged(node,path,protocol,comm);
}

//---------------------------------------------------------------------------//
void 
load(const std::string &path,
     Node &node,
     MPI_Comm comm)
{
    std::string protocol;
    identify_protocol(path,protocol);
    load(path,protocol,node,comm);
}

//---------------------------------------------------------------------------//
void 
load_merged(const std::string &path,
            Node &node,
            MPI_Comm comm)
{
    std::string protocol;
    identify_protocol(path,protocol);
    load_merged(path,protocol,node,comm);
}

//---------------------------------------------------------------------------//
void 
save(const Node &node,
     const std::string &path,
     const std::string &protocol,
     MPI_Comm comm)
{
    Node options;
    save(node, path, protocol, options, comm);
}

//---------------------------------------------------------------------------//
void 
save(const Node &node,
     const std::string &path,
     const std::string &protocol_,
     const Node &options,
     MPI_Comm comm)
{
    std::string protocol = protocol_;
    // allow empty protocol to be used for auto detect
    if(protocol.empty())
    {
        identify_protocol(path,protocol);
    }
    
    // support conduit::Node's basic save cases
    if(protocol == "conduit_bin" ||
       protocol == "json" ||
       protocol == "conduit_json" ||
       protocol == "conduit_base64_json" ||
       protocol == "yaml" )
    {
        node.save(path,protocol);
    }
    else if( protocol == "hdf5")
    {
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
        Node prev_options;
        if(options.has_child("hdf5"))
        {
            hdf5_options(prev_options);
            hdf5_set_options(options["hdf5"]);
        }

        hdf5_save(node,path);
        
        if(!prev_options.dtype().is_empty())
        {
            hdf5_set_options(prev_options);
        }
#else
        CONDUIT_ERROR("conduit_relay_mpi_io lacks HDF5 support: " << 
                      "Failed to save conduit node to path " << path);
#endif
    }
    else if( protocol == "conduit_silo")
    {
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
        silo_write(node,path);
#else
        CONDUIT_ERROR("conduit_relay_mpi_io lacks Silo support: " << 
                      "Failed to save conduit node to path " << path);
#endif
    }
    else if( protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
        Node prev_options;
        if(options.has_child("adios"))
        {
            adios_options(prev_options, comm);
            adios_set_options(options["adios"], comm);
        }
        
        adios_save(node,path,comm);

        if(!prev_options.dtype().is_empty())
        {
            adios_set_options(prev_options, comm);
        }
#else
        CONDUIT_UNUSED(comm);
        CONDUIT_ERROR("conduit_relay_mpi_io lacks ADIOS support: " << 
                      "Failed to save conduit node to path " << path);
#endif
    }
    else
    {
        CONDUIT_ERROR("unknown conduit_relay protocol: " << protocol);
    }
}

//---------------------------------------------------------------------------//
void 
save_merged(const Node &node,
            const std::string &path,
            const std::string &protocol,
            MPI_Comm comm)
{
    Node options;
    save_merged(node, path, protocol, options, comm);
}

//---------------------------------------------------------------------------//
void 
save_merged(const Node &node,
            const std::string &path,
            const std::string &protocol_,
            const Node &options,
            MPI_Comm comm)
{
    std::string protocol = protocol_;
    // allow empty protocol to be used for auto detect
    if(protocol.empty())
    {
        identify_protocol(path,protocol);
    }

    if (protocol != "adios")
    {
        CONDUIT_ERROR("conduit::relay::mpi::io::save_merged() was called with "
                      "a protocol that is not adios. The non-MPI version of "
                      "save_merged should be used for non-adios cases.");
    }
    
    // support conduit::Node's basic save cases
    if(protocol == "conduit_bin" ||
       protocol == "json" ||
       protocol == "conduit_json" ||
       protocol == "conduit_base64_json" ||
       protocol == "yaml" )
    {
        Node n;
        n.load(path,protocol);
        n.update(node);
        n.save(path,protocol);
    }
    else if( protocol == "hdf5")
    {
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
        
        Node prev_options;
        if(options.has_child("hdf5"))
        {
            hdf5_options(prev_options);
            hdf5_set_options(options["hdf5"]);
        }
        
        hdf5_write(node,path);
        
        if(!prev_options.dtype().is_empty())
        {
            hdf5_set_options(prev_options);
        }
        
        
#else
        CONDUIT_ERROR("conduit_relay lacks HDF5 support: " << 
                      "Failed to save conduit node to path " << path);
#endif
    }
    else if( protocol == "conduit_silo")
    {
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
        Node n;
        silo_read(path,n);
        n.update(node);
        silo_write(n,path);
#else
        CONDUIT_ERROR("conduit_relay_mpi_io lacks Silo support: " << 
                      "Failed to save conduit node to path " << path);
#endif
    }
    else if( protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
        Node prev_options;
        if(options.has_child("adios"))
        {
            adios_options(prev_options, comm);
            adios_set_options(options["adios"], comm);
        }
        
        adios_save_merged(node,path,comm);
        
        if(!prev_options.dtype().is_empty())
        {
            adios_set_options(prev_options, comm);
        }
#else
        CONDUIT_UNUSED(comm);
        CONDUIT_ERROR("conduit_relay_mpi_io lacks ADIOS support: " << 
                      "Failed to save conduit node to path " << path);
#endif
    }
    else
    {
        CONDUIT_ERROR("unknown conduit_relay protocol: " << protocol);
    }
}

//---------------------------------------------------------------------------//
void
add_step(const Node &node,
         const std::string &path,
         MPI_Comm comm)
{
    std::string protocol;
    identify_protocol(path,protocol);
    add_step(node, path, protocol, comm);
}

//---------------------------------------------------------------------------//
void
add_step(const Node &node,
         const std::string &path,
         const std::string &protocol,
         MPI_Comm comm)
{
    Node options;
    add_step(node, path, protocol, options, comm);
}


//---------------------------------------------------------------------------//
void
add_step(const Node &node,
         const std::string &path,
         const std::string &protocol_,
         const Node &options,
         MPI_Comm comm)
{
    std::string protocol = protocol_;
    // allow empty protocol to be used for auto detect
    if(protocol.empty())
    {
        identify_protocol(path,protocol);
    }
    
    if(protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED

        Node prev_options;
        if(options.has_child("adios"))
        {
            adios_options(prev_options, comm);
            adios_set_options(options["adios"], comm);
        }
                
        adios_add_step(node, path, comm);
        
        if(!prev_options.dtype().is_empty())
        {
            adios_set_options(prev_options, comm);
        }
#else
    CONDUIT_UNUSED(node);
    CONDUIT_UNUSED(options);
    CONDUIT_UNUSED(comm);
#endif
    }
    else
    {
        CONDUIT_ERROR("add_step is not currently supported for protocol "
                      << protocol);

        // Future idea: make path be some type of filename generator object
        //              that can make the next filename in a time series 
        //              and call save(node,generatedpath)
    }
}

//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
void
load(const std::string &path,
     const std::string &protocol_,
     const Node &options,
     Node &node,
     MPI_Comm comm)
{

    std::string protocol = protocol_;
    // allow empty protocol to be used for auto detect
    if(protocol.empty())
    {
        identify_protocol(path,protocol);
    }
    
    // support conduit::Node's basic load cases
    if(protocol == "conduit_bin" ||
       protocol == "json" ||
       protocol == "conduit_json" ||
       protocol == "conduit_base64_json" ||
       protocol == "yaml" )
    {
        node.load(path,protocol);
    }
    else if( protocol == "hdf5")
    {
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
        node.reset();
        Node prev_options;
        if(options.has_child("hdf5"))
        {
            hdf5_options(prev_options);
            hdf5_set_options(options["hdf5"]);
        }
        
        hdf5_read(path,node);
        
        if(!prev_options.dtype().is_empty())
        {
            hdf5_set_options(prev_options);
        }
#else
        CONDUIT_ERROR("conduit_relay_mpi_io lacks HDF5 support: " << 
                      "Failed to load conduit node from path " << path);
#endif
    }
    else if( protocol == "sidre_hdf5")
    {
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
        relay::io::IOHandle hnd;
        // split path to get file and sub path part
        // check for ":" split    
        std::string file_path;
        std::string sub_base;
        conduit::utils::split_file_path(path,
                                        std::string(":"),
                                        file_path,
                                        sub_base);

        hnd.open(file_path);
        hnd.read(sub_base,node);
        hnd.close();
#else
        CONDUIT_ERROR("conduit_relay lacks Sidre HDF5 support: " << 
                      "Failed to save conduit node to path " << path);
#endif
    }
    else if( protocol == "conduit_silo")
    {
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
        silo_read(path,node);
#else
        CONDUIT_ERROR("conduit_relay lacks Silo support: " << 
                    "Failed to load conduit node from path " << path);
#endif
    }
    else if(protocol == "conduit_silo_mesh")
    {
        CONDUIT_ERROR("the conduit_relay conduit_silo_mesh protocol does not "
                      "support \"load\"");
    }
    else if( protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
        Node prev_options;
        if(options.has_child("adios"))
        {
            adios_options(prev_options, comm);
            adios_set_options(options["adios"], comm);
        }

        node.reset();
        adios_load(path,node,comm);
        
        if(!prev_options.dtype().is_empty())
        {
            adios_set_options(prev_options, comm);
        }
#else
        CONDUIT_UNUSED(comm);
        CONDUIT_ERROR("conduit_relay_mpi_io lacks ADIOS support: " << 
                    "Failed to load conduit node from path " << path);
#endif
    }
    else
    {
        CONDUIT_ERROR("unknown conduit_relay protocol: " << protocol);
        
    }
}

//---------------------------------------------------------------------------//
void
load(const std::string &path,
     const std::string &protocol,
     Node &node,
     MPI_Comm comm)
{
    Node options;
    load(path, protocol, options, node, comm);
}

//---------------------------------------------------------------------------//
void
load(const std::string &path,
     const std::string &protocol_,
     int step,
     int domain,
     const Node &options,
     Node &node,
     MPI_Comm comm)
{
    std::string protocol = protocol_;
    // allow empty protocol to be used for auto detect
    if(protocol.empty())
    {
        identify_protocol(path,protocol);
    }
    
    // support conduit::Node's basic load cases
    if(protocol == "conduit_bin" ||
       protocol == "json" ||
       protocol == "conduit_json" ||
       protocol == "conduit_base64_json" ||
       protocol == "yaml" )
    {
        node.load(path,protocol);
    }
    else if( protocol == "hdf5")
    {
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
        node.reset();
        hdf5_read(path,node);
#else
        CONDUIT_ERROR("conduit_relay_mpi_io lacks HDF5 support: " << 
                      "Failed to load conduit node from path " << path);
#endif
    }
    else if( protocol == "conduit_silo")
    {
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
        silo_read(path,node);
#else
        CONDUIT_ERROR("conduit_relay lacks Silo support: " << 
                    "Failed to load conduit node from path " << path);
#endif
    }
    else if(protocol == "conduit_silo_mesh")
    {
        CONDUIT_ERROR("the conduit_relay conduit_silo_mesh protocol does not "
                      "support \"load\"");
    }
    else if( protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
        
        Node prev_options;
        if(options.has_child("adios"))
        {
            adios_options(prev_options, comm);
            adios_set_options(options["adios"], comm);
        }
        
        node.reset();
        adios_load(path,step,domain,node,comm);
        
        if(!prev_options.dtype().is_empty())
        {
            adios_set_options(prev_options, comm);
        }
#else
        CONDUIT_UNUSED(step);
        CONDUIT_UNUSED(domain);
        CONDUIT_UNUSED(options);
        CONDUIT_UNUSED(comm);
        CONDUIT_ERROR("conduit_relay_mpi_io lacks ADIOS support: " << 
                    "Failed to load conduit node from path " << path);
#endif
    }
    else
    {
        CONDUIT_ERROR("unknown conduit_relay protocol: " << protocol);
        
    }
}

//---------------------------------------------------------------------------//
void
load(const std::string &path,
     const std::string &protocol,
     int step,
     int domain,
     Node &node,
     MPI_Comm comm)
{
    Node options;
    load(path, protocol, step, domain, options, node, comm);
}

//---------------------------------------------------------------------------//
void
load_merged(const std::string &path,
            const std::string &protocol_,
            Node &node,
            MPI_Comm comm)
{
    std::string protocol = protocol_;
    // allow empty protocol to be used for auto detect
    if(protocol.empty())
    {
        identify_protocol(path,protocol);
    }
    // support conduit::Node's basic load cases
    if(protocol == "conduit_bin" ||
       protocol == "json" ||
       protocol == "conduit_json" ||
       protocol == "conduit_base64_json" ||
       protocol == "yaml" )
    {
        Node n;
        n.load(path,protocol);
        // update into dest
        node.update(n);

    }
    else if( protocol == "hdf5")
    {
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
        Node n;
        hdf5_read(path,n);
        node.update(n);
#else
        CONDUIT_ERROR("conduit_relay_mpi_io lacks HDF5 support: " << 
                      "Failed to read conduit node from path " << path);
#endif
    }
    else if( protocol == "conduit_silo")
    {
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
        Node n;
        silo_read(path,n);
        node.update(n);
#else
        CONDUIT_ERROR("conduit_relay_mpi_io lacks Silo support: " << 
                    "Failed to load conduit node from path " << path);
#endif
    }
    else if(protocol == "conduit_silo_mesh")
    {
        CONDUIT_ERROR("the relay conduit_silo_mesh protocol does not "
                      "support \"load\"");
    }
    else if( protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
        Node n;
        adios_load(path,n,comm);
        node.update(n);
#else
        CONDUIT_UNUSED(comm);
        CONDUIT_ERROR("conduit_relay_mpi_io lacks ADIOS support: " << 
                      "Failed to read conduit node from path " << path);
#endif
    }
    else
    {
        CONDUIT_ERROR("relay unknown protocol: " << protocol);
        
    }

}

//-----------------------------------------------------------------------------
int query_number_of_steps(const std::string &path, MPI_Comm comm)
{
    int nsteps = 1;
    std::string protocol;
    identify_protocol(path,protocol);

    if(protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
        // TODO: see if we can do this on comm's rank 0 and bcast.
        nsteps = adios_query_number_of_steps(path, comm);
#else
        CONDUIT_UNUSED(comm);
#endif
    }

    return nsteps;
}

//-----------------------------------------------------------------------------
int query_number_of_domains(const std::string &path, MPI_Comm comm)
{
    int ndoms = 1;
    std::string protocol;
    identify_protocol(path,protocol);

    if(protocol == "adios")
    {
#ifdef CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED
        // TODO: see if we can do this on comm's rank 0 and bcast.
        ndoms = adios_query_number_of_domains(path, comm);
#else
        CONDUIT_UNUSED(comm);
#endif
    }

    return ndoms;
}


}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi::io --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


