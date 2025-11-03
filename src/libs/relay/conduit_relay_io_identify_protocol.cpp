// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_identify_protocol.cpp
///
//-----------------------------------------------------------------------------
#include <string>
#include <cstring>
#include "conduit_utils.hpp"
#include "conduit_relay_config.h"

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    #include "conduit_relay_mpi_io_identify_protocol.hpp"
#else
    #include "conduit_relay_io_identify_protocol.hpp"
#endif

// includes for optional features
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
#include "conduit_relay_io_hdf5.hpp"
#endif

#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
#include "conduit_relay_io_silo.hpp"
#endif


#include <fstream>

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

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
//-----------------------------------------------------------------------------
namespace mpi
{
#endif

//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::io --
//-----------------------------------------------------------------------------
namespace io
{

//---------------------------------------------------------------------------//
void
identify_protocol(const std::string &path,
                  std::string &io_type)
{
    io_type = "conduit_bin";

    std::string file_path;
    std::string obj_base;

    // check for ":" split
    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    obj_base);

    std::string file_name_base;
    std::string file_name_ext;

    // find file extension to auto match
    conduit::utils::rsplit_string(file_path,
                                  std::string("."),
                                  file_name_ext,
                                  file_name_base);

    
    if(file_name_ext == "hdf5" || 
       file_name_ext == "h5")
    {
        io_type = "hdf5";
    }
    else if(file_name_ext == "silo")
    {
        io_type = "conduit_silo";
    }
    else if(file_name_ext == "json")
    {
        io_type = "json";
    }
    else if(file_name_ext == "conduit_json")
    {
        io_type = "conduit_json";
    }
    else if(file_name_ext == "conduit_base64_json")
    {
        io_type = "conduit_base64_json";
    }
    else if(file_name_ext == "yaml")
    {
        io_type = "yaml";
    }
    else if(file_name_ext == "yml")
    {
        io_type = "yaml";
    }
    else if(file_name_ext == "conduit_yaml")
    {
        io_type = "conduit_yaml";
    }
    else if(file_name_ext == "conduit_base64_yaml")
    {
        io_type = "conduit_base64_yaml";
    }
    else if(file_name_ext == "bp" ||
            file_name_ext == "adios")
    {
        io_type = "adios";
    }
    else if(file_name_ext == "csv")
    {
        io_type = "csv";
    }

    // default to conduit_bin

}

//---------------------------------------------------------------------------//
void
identify_file_type(const std::string &path,
                   std::string &file_type)
{
    file_type = "unknown";
    const std::string hdf5_magic_number = "\211HDF\r\n\032\n";
    const std::string pdb_magic_number  = "<<PDB:";
    // goal: check for: silo, hdf5, json, or yaml
    char buff[257];
    std::memset(buff,0,257);
    std::ifstream ifs;
    // we need to open with std::ios::binary mode
    //  the hdf5 magic number includes a windows style
    //  new line. ifstream::read, even when given a size
    //  will stop at the new line
    ifs.open(path.c_str(),std::ios::binary);
    if(ifs.is_open())
    {
        ifs.read((char *)buff,256);
        int nbytes_read = static_cast<int>(ifs.gcount());
        ifs.close();
        std::string test_str(buff,nbytes_read);
        // check for hdf5 magic number
        if(test_str.find(hdf5_magic_number) != std::string::npos)
        {
            file_type = "hdf5";
#ifdef CONDUIT_RELAY_IO_HDF5_ENABLED
            // if hdf5 it could be a silo file or a normal hdf5 file
            // open with hdf5 and look for presence of silo
            // sentinel _silolibinfo
            hid_t h5_file_id = conduit::relay::io::hdf5_open_file_for_read(path);
            
            if(conduit::relay::io::hdf5_has_path(h5_file_id,"_silolibinfo"))
            {
                file_type="silo";
            }
            // close the hdf5 file
            conduit::relay::io::hdf5_close_file(h5_file_id);
#endif
        }
        
        if(file_type == "unknown")
        {
            // check for pdb magic number first
            if(test_str.find(pdb_magic_number) != std::string::npos)
            {
                // see if pdb file is a silo file
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
                if(conduit::relay::io::is_silo_file(path,"pdb"))
                {
                    file_type = "silo";
                }
#endif
            }
        }

        // else check for yaml or json
        if(file_type == "unknown")
        {
            // for json or yaml, lets make sure a new line exists
            if(test_str.find("\n") != std::string::npos)
            {
                // chars for heuristics could be embedded string leaves
                // so strip out any leaf strings.
                test_str = conduit::utils::strip_quoted_strings(test_str,"\"");
                test_str = conduit::utils::strip_quoted_strings(test_str,"'");
                // for yaml look for ":"
                // for json, look for "{"
                if(test_str.find(":") != std::string::npos)
                {
                   file_type = "yaml";
                }
                // This should not be "else if" because json often includes ":"
                // characters, which is how we recognize yaml.
                if(test_str.find("{") != std::string::npos)
                {
                   file_type = "json";
                }
            }
        }
    }
}



}
//-----------------------------------------------------------------------------
// -- end conduit::relay::io --
//-----------------------------------------------------------------------------


#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi --
//-----------------------------------------------------------------------------
#endif

}
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


