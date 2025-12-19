// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_hdf5.cpp
///
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    #include "conduit_relay_mpi_io_hdf5.hpp"
#else
    #include "conduit_relay_io_hdf5.hpp"
#endif

#include "conduit_fmt/conduit_fmt.h"

//-----------------------------------------------------------------------------
// zfp
//-----------------------------------------------------------------------------
#if defined(CONDUIT_RELAY_IO_H5ZZFP_ENABLED)
    #include "H5Zzfp_lib.h"
    #include "H5Zzfp_props.h"
    #include "zfp.h"
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// standard lib includes
//-----------------------------------------------------------------------------
#include <iostream>

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include <hdf5.h>

//-----------------------------------------------------------------------------
/// macro used to check if an HDF5 object id is valid
//-----------------------------------------------------------------------------
#define CONDUIT_HDF5_VALID_ID( hdf5_id )   hdf5_id  >= 0
//-----------------------------------------------------------------------------
/// macro used to check if an HDF5 return status is ok
//-----------------------------------------------------------------------------
#define CONDUIT_HDF5_STATUS_OK(hdf5_id )   hdf5_id  >= 0

//-----------------------------------------------------------------------------
/// The CONDUIT_HDF5_ERROR macro is used for errors with ref paths.
//-----------------------------------------------------------------------------
#define CONDUIT_HDF5_ERROR( ref_path, msg )                               \
{                                                                         \
    CONDUIT_ERROR( "HDF5 Error (reference path: \"" << ref_path << "\") " \
                    <<  msg);                                             \
}

//-----------------------------------------------------------------------------
/// The CONDUIT_HDF5_WARN macro is used for warnings with ref paths.
//-----------------------------------------------------------------------------
#define CONDUIT_HDF5_WARN( ref_path, msg )                                 \
{                                                                          \
    CONDUIT_WARN( "HDF5 Warning (reference path: \"" << ref_path << "\") " \
                    <<  msg);                                              \
}

//-----------------------------------------------------------------------------
/// The CONDUIT_CHECK_HDF5_ERROR macro is used to check error codes from HDF5.
//-----------------------------------------------------------------------------
#define CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH( hdf5_err, ref_path, msg ) \
{                                                                         \
    if( hdf5_err < 0 )                                                    \
    {                                                                     \
        std::ostringstream hdf5_err_oss;                                  \
        hdf5_err_oss << "HDF5 Error (error code: "                        \
            <<  hdf5_err                                                  \
            <<  ", reference path: \""                                    \
            <<  ref_path << "\""                                          \
            <<  ") " << msg;                                              \
        CONDUIT_ERROR( hdf5_err_oss.str());                               \
    }                                                                     \
}

//-----------------------------------------------------------------------------
/// The CONDUIT_CHECK_HDF5_ERROR macro is used to check error codes from HDF5.
//-----------------------------------------------------------------------------
#define CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH( hdf5_err, hdf5_obj_id, ref_path, msg ) \
{                                                                         \
    if( hdf5_err < 0 )                                                    \
    {                                                                     \
        /* Try to find the file system path from hdf5_obj */              \
        ssize_t hdf5_f_sz = H5Fget_name(hdf5_obj_id, NULL, 0 );           \
        std::ostringstream hdf5_err_oss;                                  \
        hdf5_err_oss << "HDF5 Error (error code: "                        \
            <<  hdf5_err                                                  \
            <<  ", reference path: \"";                                   \
        if(hdf5_f_sz > 0)                                                 \
        {                                                                 \
            std::vector<char>hdf5_f_buff(hdf5_f_sz+1, 0);                 \
            H5Fget_name(hdf5_obj_id, &hdf5_f_buff[0], hdf5_f_sz+1);       \
            hdf5_err_oss << std::string(&hdf5_f_buff[0]) << ":";          \
        }                                                                 \
            hdf5_err_oss <<  ref_path << "\""                             \
            <<  ") " << msg;                                              \
        CONDUIT_ERROR( hdf5_err_oss.str());                               \
    }                                                                     \
}



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


static std::string conduit_hdf5_list_attr_name = "__conduit_list";


//-----------------------------------------------------------------------------
// Private class used to hold options that control hdf5 i/o params.
//
// These values are read by about(), and are set by io::hdf5_set_options()
//
//
//-----------------------------------------------------------------------------

class HDF5Options
{
public:
    static std::string libver;
    static std::string messages;

    static bool chunking_enabled;
    static int  chunk_threshold;
    static int  chunk_size;

    static bool compact_storage_enabled;
    static int  compact_storage_threshold;

    static std::string compression_method;
    // gzip options (gzip level)
    static int         compression_level;

//-----------------------------------------------------------------------------
// zfp options
//-----------------------------------------------------------------------------
#if defined(CONDUIT_RELAY_IO_H5ZZFP_ENABLED)
    static int          zfp_mode;
    static double       zfp_rate;
    static double       zfp_acc;
    static unsigned int zfp_prec;
    static unsigned int zfp_minbits;
    static unsigned int zfp_maxbits;
    static unsigned int zfp_maxprec;
    static int          zfp_minexp;
//-----------------------------------------------------------------------------
#endif // zfp options
//-----------------------------------------------------------------------------

public:

    //------------------------------------------------------------------------
    static void set(const Node &opts)
    {

        if(opts.has_child("libver"))
        {
            libver = opts["libver"].as_string();
        }

        if(opts.has_child("messages"))
        {
            messages = opts["messages"].as_string();
        }

        if(opts.has_child("compact_storage"))
        {
            const Node &compact = opts["compact_storage"];

            if(compact.has_child("enabled"))
            {
                std::string enabled = compact["enabled"].as_string();
                if(enabled == "false")
                {
                    compact_storage_enabled = false;
                }
                else
                {
                    compact_storage_enabled = true;
                }
            }

            if(compact.has_child("threshold"))
            {
                compact_storage_threshold = compact["threshold"].to_value();
            }
        }

        if(opts.has_child("chunking"))
        {
            const Node &chunking = opts["chunking"];

            if(chunking.has_child("enabled"))
            {
                std::string enabled = chunking["enabled"].as_string();
                if(enabled == "false")
                {
                    chunking_enabled = false;
                }
                else
                {
                    chunking_enabled = true;
                }
            }

            if(chunking.has_child("threshold"))
            {
                chunk_threshold = chunking["threshold"].to_value();
            }

            if(chunking.has_child("chunk_size"))
            {
                chunk_size = chunking["chunk_size"].to_value();
            }

            if(chunking.has_child("compression"))
            {
                const Node &comp = chunking["compression"];

                if(comp.has_child("method"))
                {
                    compression_method = comp["method"].as_string();
                }

                if(comp.has_path("level"))
                {
                    compression_level = comp["level"].to_value();
                }

//-----------------------------------------------------------------------------
// zfp options
//-----------------------------------------------------------------------------
#if defined(CONDUIT_RELAY_IO_H5ZZFP_ENABLED)
                if(comp.has_path("zfp"))
                {
                    const Node &zfp_opts = comp["zfp"];
                    if(zfp_opts.has_path("mode"))
                    {
                        std::string zfp_mode = zfp_opts["mode"].as_string();
                        if(zfp_mode == "rate")
                        {
                            zfp_mode = H5Z_ZFP_MODE_RATE;
                        }
                        else if(zfp_mode == "precision")
                        {
                            zfp_mode = H5Z_ZFP_MODE_PRECISION;
                        }
                        else if(zfp_mode == "accuracy")
                        {
                            zfp_mode = H5Z_ZFP_MODE_ACCURACY;
                        }
                        else if(zfp_mode == "expert")
                        {
                            zfp_mode = H5Z_ZFP_MODE_EXPERT;
                        }
                        else if(zfp_mode == "reversible")
                        {
                            zfp_mode = H5Z_ZFP_MODE_REVERSIBLE;
                        }
                        else // unknown
                        {
                            // ERROR?
                        }
                    }

                    if(zfp_opts.has_path("rate"))
                    {
                        zfp_rate = zfp_opts["rate"].to_value();
                    }

                    if(zfp_opts.has_path("acc"))
                    {
                        zfp_acc = zfp_opts["acc"].to_value();
                    }

                    if(zfp_opts.has_path("prec"))
                    {
                        zfp_prec = zfp_opts["prec"].to_value();
                    }

                    if(zfp_opts.has_path("minbits"))
                    {
                        zfp_minbits = zfp_opts["minbits"].to_value();
                    }

                    if(zfp_opts.has_path("maxbits"))
                    {
                        zfp_maxbits = zfp_opts["maxbits"].to_value();
                    }

                    if(zfp_opts.has_path("maxprec"))
                    {
                        zfp_maxprec = zfp_opts["maxprec"].to_value();
                    }

                    if(zfp_opts.has_path("minexp"))
                    {
                        zfp_minexp = zfp_opts["minexp"].to_value();
                    }
                }
//-----------------------------------------------------------------------------
#endif // zfp options
//-----------------------------------------------------------------------------
            }
        }
    }

    //------------------------------------------------------------------------
    static void about(Node &opts)
    {
        opts.reset();

        // report hdf5_library_version
        unsigned int major_num=0;
        unsigned int minor_num=0;
        unsigned int release_num=0;

        herr_t h5_status = H5get_libversion(&major_num, &minor_num,&release_num);

        CONDUIT_CHECK_HDF5_ERROR(h5_status,
                                 "Failed to fetch HDF5 library version info ");

        opts["hdf5_library_version"] = conduit_fmt::format("v{0}.{1}.{2}",
                                                           major_num,
                                                           minor_num,
                                                           release_num);
        opts["libver"] = libver;
        opts["messages"] = messages;

        if(compact_storage_enabled)
        {
            opts["compact_storage/enabled"] = "true";
        }
        else
        {
            opts["compact_storage/enabled"] = "false";
        }

        opts["compact_storage/threshold"] = compact_storage_threshold;

        if(chunking_enabled)
        {
            opts["chunking/enabled"] = "true";
        }
        else
        {
            opts["chunking/enabled"] = "false";
        }

        opts["chunking/threshold"] = chunk_threshold;
        opts["chunking/chunk_size"] = chunk_size;

        opts["chunking/compression/method"] = compression_method;
        if(compression_method == "gzip")
        {
            opts["chunking/compression/level"] = compression_level;
        }
//-----------------------------------------------------------------------------
// zfp options
//-----------------------------------------------------------------------------
#if defined(CONDUIT_RELAY_IO_H5ZZFP_ENABLED)

        Node &zfp_opts = opts["chunking/compression/zfp"];

        if(zfp_mode == H5Z_ZFP_MODE_RATE)
        {
            zfp_opts["mode"] = "rate";
        }
        else if(zfp_mode == H5Z_ZFP_MODE_PRECISION)
        {
            zfp_opts["mode"] = "precision";
        }
        else if(zfp_mode == H5Z_ZFP_MODE_ACCURACY)
        {
            zfp_opts["mode"] = "accuracy";
        }
        else if(zfp_mode == H5Z_ZFP_MODE_EXPERT)
        {
            zfp_opts["mode"] = "expert";
        }
        else if(zfp_mode == H5Z_ZFP_MODE_REVERSIBLE)
        {
            zfp_opts["mode"] = "reversible";
        }

        zfp_opts["rate"]    = zfp_rate;
        zfp_opts["acc"]     = zfp_acc;
        zfp_opts["prec"]    = zfp_prec;
        zfp_opts["minbits"] = zfp_minbits;
        zfp_opts["maxbits"] = zfp_maxbits;
        zfp_opts["maxprec"] = zfp_maxprec;
        zfp_opts["minexp"]  = zfp_minexp;
//-----------------------------------------------------------------------------
#endif // zfp options
//-----------------------------------------------------------------------------
    }
};

// default hdf5 i/o settings

std::string HDF5Options::libver             = "default";
// quiet (default) suppresses hdf5 diag warnings in outer relay API layers
std::string HDF5Options::messages           = "quiet";

bool HDF5Options::compact_storage_enabled   = true;
int  HDF5Options::compact_storage_threshold = 1024;

bool        HDF5Options::chunking_enabled   = true;
int         HDF5Options::chunk_size         = 1000000; // 1 mb
int         HDF5Options::chunk_threshold    = 2000000; // 2 mb

// TODO: ndarray chunk heuristics


std::string HDF5Options::compression_method = "gzip";
int         HDF5Options::compression_level  = 5;

//-----------------------------------------------------------------------------
// zfp options
//-----------------------------------------------------------------------------
#if defined(CONDUIT_RELAY_IO_H5ZZFP_ENABLED)
int          HDF5Options::zfp_mode    = H5Z_ZFP_MODE_RATE;
double       HDF5Options::zfp_rate    = 4;  // default from h5z-zfp example
double       HDF5Options::zfp_acc     = 0;  // default from h5z-zfp example
unsigned int HDF5Options::zfp_prec    = 11; // default from h5z-zfp example
unsigned int HDF5Options::zfp_minbits = ZFP_MIN_BITS; // default from zfp
unsigned int HDF5Options::zfp_maxbits = ZFP_MAX_BITS; // default from zfp
unsigned int HDF5Options::zfp_maxprec = ZFP_MAX_PREC; // default from zfp
int          HDF5Options::zfp_minexp  = ZFP_MIN_EXP;  // default from zfp
//-----------------------------------------------------------------------------
#endif // zfp options
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void
hdf5_set_options(const Node &opts)
{
    HDF5Options::set(opts);
}

//-----------------------------------------------------------------------------
void
hdf5_options(Node &opts)
{
    HDF5Options::about(opts);
}

//-----------------------------------------------------------------------------
// Private class used to suppress HDF5 error messages.
//
// Creating an instance of this class will disable the current HDF5 error
// callbacks. The default HDF5 callbacks print error messages when probing
// properties of the HDF5 tree. When the instance is destroyed, the previous
// error state is restored.
//
// Suppression is only enabled when HDF5Options::messages == "quiet"
//-----------------------------------------------------------------------------
class HDF5ErrorStackSuppressor
{
public:
        HDF5ErrorStackSuppressor()
        :  herr_func(NULL),
           herr_func_client_data(NULL),
           active(true)
        {
            active = (HDF5Options::messages == "quiet");
            if(active)
            {
                disable_hdf5_error_func();
            }
        }

       ~HDF5ErrorStackSuppressor()
        {
            if(active)
            {
                restore_hdf5_error_func();
            }
            active = false;
        }

private:
    // saves current error func.
    // for hdf5's default setup this disable printed error messages
    // that occur when we are probing properties of the hdf5 tree
    void disable_hdf5_error_func()
    {
            H5Eget_auto(H5E_DEFAULT,
                        &herr_func,
                        &herr_func_client_data);

            H5Eset_auto(H5E_DEFAULT,
                        NULL,
                        NULL);
    }

    // restores saved error func
    void restore_hdf5_error_func()
    {
        H5Eset_auto(H5E_DEFAULT,
                    herr_func,
                    herr_func_client_data);
    }

    // callback used for hdf5 error interface
    H5E_auto2_t  herr_func;
    // data container for hdf5 error interface callback
    void         *herr_func_client_data;
    // reflects if suppression is on
    bool          active;
};


//-----------------------------------------------------------------------------
// Private class used manage and automatically cleanup HDF5 handles.
//
// Base template is templated on structs that implement close using HDF5 API
// to cover HDF5 O/F/G/D/S/T/P/A types.
//
//-----------------------------------------------------------------------------
template <class Handler>
class RelayH5Handle
{
public:
    // construct with ref parent id and path
    RelayH5Handle(hid_t handle_id,
             hid_t parent_id,
             const std::string &ref_path)
    : m_id(handle_id),
      m_parent_id(parent_id),
      m_ref_path(ref_path)
    {
       ////  for debugging
       // std::cout << "Construct Handle[" << Handler::name << "] "
       //           << m_ref_path << " id: " << m_id << std::endl;
    }

    // construct with ref path
    RelayH5Handle(hid_t handle_id,
             const std::string &ref_path)
    : m_id(handle_id),
      m_parent_id(-1),
      m_ref_path(ref_path)
    {
        ////  for debugging
        // std::cout << "Construct Handle[" << Handler::name << "] "
        //           << m_ref_path << " id: " << m_id << std::endl;
    }

    // destruct, close handle if needed
    ~RelayH5Handle()
    {
        close();
    }

    // fetch hdf5 handle id
    hid_t id() const
    {
        return m_id; 
    }

    // check if handle id is valid
    bool is_valid() const
    {
        return m_id > 0;
    }

    // set new handle id, does not close
    void set_id(hid_t id)
    {
        m_id = id;
        ////  for debugging
        // std::cout << "Set Handle[" << Handler::name << "] "
        //           << m_ref_path << " id: " << m_id << std::endl;
    }

    // helper to check if id was created correctly
    void check_created()
    {
        if(!is_valid())
        {
            if(m_parent_id != -1)
            {
                CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(m_id,
                                                                m_parent_id,
                                                                m_ref_path,
                    "Failed to create HDF5 " << Handler::name);
            }
            else
            {
                CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(m_id,
                                                       m_ref_path,
                    "Failed to create HDF5 " << Handler::name);
            }

            m_id = -1;
        }
    }

    // helper close handle
    void close()
    {
        if(is_valid())
        {
            ////  for debugging
            // std::cout << "Close[" << Handler::name << "] "
            //           << m_ref_path  << " id: " << m_id << std::endl;
            herr_t res = Handler::close_handle(m_id);
            if(m_parent_id != -1)
            {
                CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(res,
                                                                m_parent_id,
                                                                m_ref_path,
                    "Failed to close HDF5 " << Handler::name << " : " << m_id);
            }
            else
            {
                CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(res,
                                                       m_ref_path,
                    "Failed to close HDF5 " << Handler::name << " : " << m_id);
            }
            m_id = -1;
        }
    }

private:
    hid_t              m_id;
    hid_t              m_parent_id;
    const std::string &m_ref_path; // TODO: c++17 string_view
};

//-----------------------------------------------------------------//
// helper structs
//-----------------------------------------------------------------//

//-----------------------------------------------------------------//
// H5O
struct RelayH5OHandler
{
    static constexpr const char* name = "Object";
    static herr_t close_handle(hid_t id){return H5Oclose(id);}
};

//-----------------------------------------------------------------//
// H5F
//-----------------------------------------------------------------//
struct RelayH5FHandler
{
    static constexpr const char* name = "File";
    static herr_t close_handle(hid_t id){return H5Fclose(id);}
};

//-----------------------------------------------------------------//
// H5G
//-----------------------------------------------------------------//
struct RelayH5GHandler
{
    static constexpr const char* name = "Group";
    static herr_t close_handle(hid_t id){return H5Gclose(id);}
};

//-----------------------------------------------------------------//
// H5D
//-----------------------------------------------------------------//
struct RelayH5DHandler
{
    static constexpr const char* name = "Dataset";
    static herr_t close_handle(hid_t id){return H5Dclose(id);}
};

//-----------------------------------------------------------------//
// H5S
//-----------------------------------------------------------------//
struct RelayH5SHandler
{
    static constexpr const char* name = "Dataspace";
    static herr_t close_handle(hid_t id){return H5Sclose(id);}
};

//-----------------------------------------------------------------//
// H5T
//-----------------------------------------------------------------//
struct RelayH5THandler
{
    static constexpr const char* name = "Datatype";
    static herr_t close_handle(hid_t id){return H5Tclose(id);}
};

//-----------------------------------------------------------------//
// H5P
//-----------------------------------------------------------------//
struct RelayH5PHandler
{
    static constexpr const char* name = "Property List";
    static herr_t close_handle(hid_t id){return H5Pclose(id);}
};

//-----------------------------------------------------------------//
// H5A
//-----------------------------------------------------------------//
struct RelayH5AHandler
{
    static constexpr const char* name = "Attribute";
    static herr_t close_handle(hid_t id){return H5Aclose(id);}
};

//-----------------------------------------------------------------//
// Concrete Handle Classes
//-----------------------------------------------------------------//
typedef RelayH5Handle<RelayH5OHandler> RelayH5OHandle;
typedef RelayH5Handle<RelayH5FHandler> RelayH5FHandle;
typedef RelayH5Handle<RelayH5GHandler> RelayH5GHandle;
typedef RelayH5Handle<RelayH5DHandler> RelayH5DHandle;
typedef RelayH5Handle<RelayH5SHandler> RelayH5SHandle;
typedef RelayH5Handle<RelayH5THandler> RelayH5THandle;
typedef RelayH5Handle<RelayH5PHandler> RelayH5PHandle;
typedef RelayH5Handle<RelayH5AHandler> RelayH5AHandle;


//-----------------------------------------------------------------------------
// helper method decls
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// helpers for finding hdf5 object filename and constructing ref paths for
// errors
//-----------------------------------------------------------------------------
void  hdf5_filename_from_hdf5_obj_id(hid_t hdf5_id,
                                     std::string &result);

void  hdf5_ref_path_with_filename(hid_t hdf5_id,
                                  const std::string &ref_path,
                                  std::string &result);

//-----------------------------------------------------------------------------
// helpers for data type conversions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// helpers for checking if compatible
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// if incompatible, incompat_details contains human readable details
// about why
//-----------------------------------------------------------------------------
bool check_if_conduit_leaf_is_compatible_with_hdf5_obj(const DataType &dtype,
                                                  const std::string &ref_path,
                                                                hid_t hdf5_id,
                                                             const Node &opts,
                                                std::string &incompat_details);

//-----------------------------------------------------------------------------
bool check_if_conduit_object_is_compatible_with_hdf5_tree(const Node &node,
                                                const std::string &ref_path,
                                                             hid_t hdf5_id,
                                                             const Node &opts,
                                                std::string &incompat_details);

//-----------------------------------------------------------------------------
bool check_if_conduit_node_is_compatible_with_hdf5_tree(const Node &node,
                                              const std::string &ref_path,
                                                            hid_t hdf5_id,
                                                            const Node &opts,
                                                std::string &incompat_details);

//-----------------------------------------------------------------------------
bool check_if_conduit_list_is_compatible_with_hdf5_tree(const Node &node,
                                                   const std::string &ref_path,
                                                        hid_t hdf5_id,
                                                        const Node &opts,
                                                std::string &incompat_details);

//-----------------------------------------------------------------------------
bool check_if_hdf5_group_has_conduit_list_attribute(hid_t hdf5_group_id);

//-----------------------------------------------------------------------------
// helpers for writing
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
hid_t create_hdf5_dataset_for_conduit_leaf(const DataType &dt,
                                           const std::string &ref_path,
                                           hid_t hdf5_group_id,
                                           const std::string &hdf5_dset_name,
                                           bool extendible);

//-----------------------------------------------------------------------------
hid_t create_hdf5_group_for_conduit_node(const Node &node,
                                         const std::string &ref_path,
                                         hid_t hdf5_parent_group_id,
                                         const std::string &hdf5_child_group_name);
//-----------------------------------------------------------------------------
// Note: Options may cause the dataset to be recreated,
// this is why the hdf5_dset_id is passed as a reference.
//-----------------------------------------------------------------------------
void  write_conduit_leaf_to_hdf5_dataset(const Node &node,
                                         const std::string &ref_path,
                                         hid_t &hdf5_dset_id,
                                         const Node &opts);

//-----------------------------------------------------------------------------
void  write_conduit_leaf_to_hdf5_group(const Node &node,
                                       const std::string &ref_path,
                                       hid_t hdf5_group_id,
                                       const std::string &hdf5_dset_name,
                                       const Node &opts);

//-----------------------------------------------------------------------------
void  write_conduit_node_children_to_hdf5_group(const Node &node,
                                                const std::string &ref_path,
                                                hid_t hdf5_group_id,
                                                const Node &opts);

//-----------------------------------------------------------------------------
void write_conduit_hdf5_list_attribute(hid_t hdf5_group_id,
                                       const std::string &ref_path);

//-----------------------------------------------------------------------------
void remove_conduit_hdf5_list_attribute(hid_t hdf5_group_id,
                                        const std::string &ref_path);

//-----------------------------------------------------------------------------
// helpers for reading
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void read_hdf5_dataset_into_conduit_node(hid_t hdf5_dset_id,
                                         const std::string &ref_path,
                                         bool only_get_metadata,
                                         const Node &opts,
                                         Node &dest);

//-----------------------------------------------------------------------------
void read_hdf5_group_into_conduit_node(hid_t hdf5_group_id,
                                       const std::string &ref_path,
                                       bool only_get_metadata,
                                       const Node &opts,
                                       Node &dest);

//-----------------------------------------------------------------------------
void read_hdf5_tree_into_conduit_node(hid_t hdf5_id,
                                      const std::string &ref_path,
                                      bool only_get_metadata,
                                      const Node &opts,
                                      Node &dest);



//-----------------------------------------------------------------------------
// helper for HDF5 manipulation of ND arrays
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Fill options relating to reading or writing an HDF5 dataset
///
/// @param inopts Input options
/// @param dataspace_id ID of the dataspace of the dataset in question
/// @param filled_opts[out] The options, filled out with defaults
///
/// This routine uses the following children of \a inopts:
/// - sizes (or size)
/// - offsets (or offset)
/// - strides (or stride)
/// .
/// All are optional.  All should be numeric and of the same length
/// (the dimensionality of the dataset \a dataspace_id).
///
/// This routine does the following:
/// 1. makes a deep copy of \a inopts into \a filled_opts
/// 2. retrieves metadata from \a dataspace_id.  If it's not a dataspace,
///    throw an error.
/// 3. sets filled_opts["slabparams/rank"] as a scalar, the rank (number
///    of dimensions) of the dataset
/// 4. sets filled_opts["slabparams/dataset_sizes"] as the size of the dataset
/// 5. sets children sizes, offsets, strides of filled_opts["slabparams"]
///    based on metadata retrieved from the dataset
///    - default sizes is the size of the data set, also stored as
///      "dataset_sizes"
///    - default strides is all 1s (read or write every element)
///    - default offsets is all 0s (start at the first element)
/// 6. modifies slabparams/sizes, offsets, strides based on what
///    the user provided in inopts
/// 7. sets filled_opts["slabparams/readcount"] as a scalar, the number
///    of values to read, as specified by offset, stride, and size
void
fill_dataset_opts(const std::string & ref_path, const Node& inopts,
    hid_t dataspace_id, Node& filled_opts);

hsize_t*
make_dataset_opt_copy(const Node& opts, const std::string opt_name);

//-----------------------------------------------------------------------------
// helper used to properly create a new ref_path for a child
std::string
join_ref_paths(const std::string &parent, const std::string &child)
{
    if(parent.size() > 0)
    {
        return parent + "/" + child;
    }
    else
    {
        return child;
    }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Data Type Helper methods that are a part of public conduit::relay::io
//
//  conduit_dtype_to_hdf5_dtype
//  conduit_dtype_to_hdf5_dtype_cleanup
//  hdf5_dtype_to_conduit_dtype
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
hid_t
conduit_dtype_to_hdf5_dtype(const DataType &dt,
                            const std::string &ref_path)
{
    hid_t res = -1;

    // // This code path enables writing strings in a way that is friendlier
    // // to hdf5 command line tools like hd5dump and h5ls. However
    // // using this path we *cannot* compress that string data, so
    // // is currently disabled
    //
    // if(dt.is_string())
    // {
    //
    //     // modify the default hdf5 type to include string length info,
    //     // so hdf5 tools display the string contents in a human friendly way
    //
    //     // create a copy of the default type
    //     res = H5Tcopy(H5T_C_S1);
    //     CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(res,
    //                                            ref_path,
    //                                     "Failed to copy HDF5 type for string");
    //
    //     // set the size
    //     CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(
    //                                 H5Tset_size(res,
    //                                             // string size + null
    //                                             dt.number_of_elements()),
    //                                 ref_path,
    //                                 "Failed to set size in HDF5 string type");
    //
    //     // set term
    //     CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(
    //                                 H5Tset_strpad(res, H5T_STR_NULLTERM),
    //                                 ref_path,
    //                                 "Failed to set strpad in HDF5 string type");
    // }

    // strings are special, check for them first
    if( dt.is_string() )
    {
        res = H5T_C_S1;
    }
    // next check endianness
    else if(dt.is_little_endian()) // we know we are little endian
    {
        switch(dt.id())
        {
            case DataType::INT8_ID:  res = H5T_STD_I8LE;  break;
            case DataType::INT16_ID: res = H5T_STD_I16LE; break;
            case DataType::INT32_ID: res = H5T_STD_I32LE; break;
            case DataType::INT64_ID: res = H5T_STD_I64LE; break;

            case DataType::UINT8_ID:  res = H5T_STD_U8LE;  break;
            case DataType::UINT16_ID: res = H5T_STD_U16LE; break;
            case DataType::UINT32_ID: res = H5T_STD_U32LE; break;
            case DataType::UINT64_ID: res = H5T_STD_U64LE; break;

            case DataType::FLOAT32_ID: res = H5T_IEEE_F32LE; break;
            case DataType::FLOAT64_ID: res = H5T_IEEE_F64LE; break;

            case DataType::CHAR8_STR_ID:
                CONDUIT_HDF5_ERROR(ref_path,
                              "conduit::DataType to HDF5 Leaf DataType "
                              << "Conversion:"
                              << dt.to_json()
                              << " needs to be handled with string logic");
                        break;
            default:
                CONDUIT_HDF5_ERROR(ref_path,
                                  "conduit::DataType to HDF5 Leaf DataType "
                                  << "Conversion:"
                                  << dt.to_json()
                                  << " is not a leaf data type");
        };
    }
    else // we know we are big endian
    {
        switch(dt.id())
        {
            case DataType::INT8_ID:  res = H5T_STD_I8BE;  break;
            case DataType::INT16_ID: res = H5T_STD_I16BE; break;
            case DataType::INT32_ID: res = H5T_STD_I32BE; break;
            case DataType::INT64_ID: res = H5T_STD_I64BE; break;

            case DataType::UINT8_ID:  res = H5T_STD_U8BE;  break;
            case DataType::UINT16_ID: res = H5T_STD_U16BE; break;
            case DataType::UINT32_ID: res = H5T_STD_U32BE; break;
            case DataType::UINT64_ID: res = H5T_STD_U64BE; break;

            case DataType::FLOAT32_ID: res = H5T_IEEE_F32BE; break;
            case DataType::FLOAT64_ID: res = H5T_IEEE_F64BE; break;

            case DataType::CHAR8_STR_ID:
                CONDUIT_HDF5_ERROR(ref_path,
                              "conduit::DataType to HDF5 Leaf DataType "
                              << "Conversion:"
                              << dt.to_json()
                              << " needs to be handled with string logic");
                        break;
            default:
                CONDUIT_HDF5_ERROR(ref_path,
                                  "conduit::DataType to HDF5 Leaf DataType "
                                  << "Conversion:"
                                  << dt.to_json()
                                  << " is not a leaf data type");
        };
    }

    // return a copy of built in type so that we have he same ref count
    // semanitcs as other objects
    return H5Tcopy(res);
}

//-----------------------------------------------------------------------------
DataType
hdf5_dtype_to_conduit_dtype(hid_t hdf5_dtype_id,
                            index_t num_elems,
                            const std::string& ref_path)
{
    hsize_t num_elems_array[1] = { static_cast<hsize_t>(num_elems) };
    return hdf5_dtype_to_conduit_dtype(hdf5_dtype_id, num_elems_array, 1, ref_path);
}


//-----------------------------------------------------------------------------
DataType
hdf5_dtype_to_conduit_dtype(hid_t hdf5_dtype_id,
                            hsize_t * num_elems_array,
                            index_t rank,
                            const std::string &ref_path)
{
    // TODO: there may be a more straight forward way to do this using
    // hdf5's data type introspection methods
    index_t num_elems = 1;
    for (int d = 0; d < rank; ++d)
    {
        num_elems = num_elems * (index_t)num_elems_array[d];
    }

    DataType res;
    //-----------------------------------------------
    // signed ints
    //-----------------------------------------------
    // little endian
    if(H5Tequal(hdf5_dtype_id,H5T_STD_I8LE))
    {
        res = DataType::int8(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_I16LE))
    {
        res = DataType::int16(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_I32LE))
    {
        res = DataType::int32(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_I64LE))
    {
        res = DataType::int64(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
     // big endian
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_I8BE))
    {
        res = DataType::int8(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_I16BE))
    {
        res = DataType::int16(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_I32BE))
    {
        res = DataType::int32(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_I64BE))
    {
        res = DataType::int64(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    //-----------------------------------------------
    // unsigned ints
    //-----------------------------------------------
    // little endian
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U8LE))
    {
        res = DataType::uint8(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U16LE))
    {
        res = DataType::uint16(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U32LE))
    {
        res = DataType::uint32(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U64LE))
    {
        res = DataType::uint64(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    // big endian
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U8BE))
    {
        res = DataType::uint8(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U16BE))
    {
        res = DataType::uint16(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U32BE))
    {
        res = DataType::uint32(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_STD_U64BE))
    {
        res = DataType::uint64(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    //-----------------------------------------------
    // floating point types
    //-----------------------------------------------
    // little endian
    else if(H5Tequal(hdf5_dtype_id,H5T_IEEE_F32LE))
    {
        res = DataType::float32(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_IEEE_F64LE))
    {
        res = DataType::float64(num_elems);
        res.set_endianness(Endianness::LITTLE_ID);
    }
    // big endian
    else if(H5Tequal(hdf5_dtype_id,H5T_IEEE_F32BE))
    {
        res = DataType::float32(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    else if(H5Tequal(hdf5_dtype_id,H5T_IEEE_F64BE))
    {
        res = DataType::float64(num_elems);
        res.set_endianness(Endianness::BIG_ID);
    }
    //-----------------------------------------------
    // String Types
    //-----------------------------------------------
    else if(H5Tequal(hdf5_dtype_id,H5T_C_S1))
    {
        // string as array case (old way of writing)
        res = DataType::char8_str(num_elems);
    }
    // extended string reps
    else if( H5Tget_class(hdf5_dtype_id) == H5T_STRING )
    {
        // for strings of this type, the length
        // is encoded in the hdf5 type not the hdf5 data space
        index_t hdf5_strlen = H5Tget_size(hdf5_dtype_id);
        // check for variable type first
        if( H5Tis_variable_str(hdf5_dtype_id) )
        {
            res = DataType::char8_str(-1);
        }
        else
        {
            res = DataType::char8_str(hdf5_strlen);
        }
    }
    //-----------------------------------------------
    // Unsupported
    //-----------------------------------------------
    else
    {
        CONDUIT_HDF5_ERROR(ref_path,
                           "Error with HDF5 DataType to conduit::DataType "
                           << "Leaf Conversion");
    }

    return res;
}

//---------------------------------------------------------------------------//
// General Helper
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
void
hdf5_filename_from_hdf5_obj_id(hid_t hdf5_id,
                               std::string &hdf5_filename)
{
    hdf5_filename.clear();
    ssize_t hdf5_f_sz = H5Fget_name(hdf5_id, NULL, 0 );
    if(hdf5_f_sz > 0)
    {
        std::vector<char>hdf5_f_buff(hdf5_f_sz+1, 0);
        H5Fget_name(hdf5_id, &hdf5_f_buff[0], hdf5_f_sz+1);
        hdf5_filename = std::string(&hdf5_f_buff[0]);
    }
}

//---------------------------------------------------------------------------//
void
hdf5_ref_path_with_filename(hid_t hdf5_id,
                            const std::string &ref_path,
                            std::string &result)
{
    hdf5_filename_from_hdf5_obj_id(hdf5_id, result);
    if(result.empty())
    {
        result = ref_path;
    }
    else
    {
        if(!ref_path.empty())
        {
            result +=  ":" + ref_path;
        }
    }
}

//---------------------------------------------------------------------------//
// Write Helpers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
bool
check_if_conduit_leaf_is_compatible_with_hdf5_obj(const DataType &dtype,
                                                  const std::string &ref_path,
                                                  hid_t hdf5_id,
                                                  const Node& opts,
                                                  std::string &incompat_details)
{
    bool res = true;
    H5O_info_t h5_obj_info;

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_status = H5Oget_info(hdf5_id, &h5_obj_info, H5O_INFO_ALL);
#else
    herr_t h5_status = H5Oget_info(hdf5_id, &h5_obj_info);
#endif

    // make sure it is a dataset ...
    if( CONDUIT_HDF5_STATUS_OK(h5_status) &&
        ( h5_obj_info.type == H5O_TYPE_DATASET ) )
    {
        // get the hdf5 dataspace for the passed hdf5 obj
        RelayH5SHandle h5_test_dspace_hnd(H5Dget_space(hdf5_id),
                                          hdf5_id,
                                          ref_path);

        if( H5Sget_simple_extent_type(h5_test_dspace_hnd.id()) == H5S_NULL )
        {
            // a dataset with H5S_NULL data space is only compatible with
            // conduit empty
            if(!dtype.is_empty())
            {
                std::ostringstream oss;
                oss << "Conduit Node (leaf) at path '" << ref_path << "'"
                    << " is not compatible with given HDF5 Dataset at path"
                    << " '" << ref_path << "'"
                    << "\nHDF5 dataset has a H5S_NULL Dataspace which"
                    << " only compatible with an empty Conduit Node";

                incompat_details = oss.str();

                res = false;
            }
        }
        else
        {
            // get the hdf5 datatype that matchs the conduit dtype
            RelayH5THandle h5_dtype_hnd(conduit_dtype_to_hdf5_dtype(dtype,
                                                                   ref_path),
                                        hdf5_id,
                                        ref_path);
            h5_dtype_hnd.check_created();

            // get the hdf5 datatype for the passed hdf5 obj
            RelayH5THandle h5_test_dtype_hnd(H5Dget_type(hdf5_id),
                                             hdf5_id,
                                             ref_path);
            h5_test_dtype_hnd.check_created();

            // we will check the 1d-properties of the hdf5 dataspace
            hssize_t h5_test_num_ele = H5Sget_simple_extent_npoints(h5_test_dspace_hnd.id());

            hsize_t dataset_max_dims[1];
            H5Sget_simple_extent_dims(h5_test_dspace_hnd.id(), NULL, dataset_max_dims);

            // string case is special, check it first

            // if the dataset in the file is a custom string type
            // check the type's size vs the # of elements
            if(   ( ! H5Tequal(h5_test_dtype_hnd.id(), H5T_C_S1) &&
                  ( H5Tget_class(h5_test_dtype_hnd.id()) == H5T_STRING ) &&
                  ( H5Tget_class(h5_dtype_hnd.id()) == H5T_STRING ) ) &&
                 // if not shorted out, we have a string w/ custom type
                 // check length to see if compat
                 // note: both hdf5 and conduit dtypes include null term in string size
                 (dtype.number_of_elements() !=  (index_t)H5Tget_size(h5_test_dtype_hnd.id()) ) )
            {
                std::ostringstream oss;
                oss << "Conduit Node (string leaf) at path '" << ref_path << "'"
                    << " is not compatible with given HDF5 Dataset at path"
                    << " '" << ref_path << "'"
                    << "\nConduit leaf String Node length ("
                    << dtype.number_of_elements() << ")"
                    << " != HDF5 Dataset size (" << H5Tget_size(h5_test_dtype_hnd.id()) << ")";

                incompat_details = oss.str();

                res = false;
            }
            else if( ! (H5Tequal(h5_dtype_hnd.id(), h5_test_dtype_hnd.id()) > 0) )
            {

                std::ostringstream oss;
                oss << "Conduit Node (leaf) at path '" << ref_path << "'"
                    << " is not compatible with given HDF5 Dataset at path"
                    << " '" << ref_path << "'";

                incompat_details = oss.str();

                res = false;
            }
            else if( dataset_max_dims[0] != H5S_UNLIMITED && !opts.has_child("offset")
                && !opts.has_child("stride") && dtype.number_of_elements()
                !=  h5_test_num_ele )
            {

                std::ostringstream oss;
                oss << "Conduit Node (leaf) at path '" << ref_path << "'"
                    << " is not compatible with given HDF5 Dataset at path"
                    << " '" << ref_path << "'"
                    << "\nConduit leaf Node number of elements ("
                    << dtype.number_of_elements() << " " << h5_test_num_ele << ")"
                    << " != HDF5 Dataset size (" << H5Tget_size(h5_test_dtype_hnd.id()) << ")";

                incompat_details = oss.str();

                res = false;
            }


            // auto cleanup of h5_test_dtype_hnd and h5_dtype_hnd
        }

        // auto cleanup of h5_test_dspace_hnd
        //
    }
    else
    {
        // bad id, or not a dataset
        std::ostringstream oss;
        oss << "Conduit Node (leaf) at path '" << ref_path << "'"
            << " is not compatible with given HDF5 Dataset at path"
            << " '" << ref_path << "'"
            << "\nConduit leaf vs HDF5 Dataset: Bad HDF5 Leaf ID"
            << " or HDF5 ID is not a HDF5 Dataset";

        // check if we have the root group of a file, if so provide a
        // more info
        if( h5_obj_info.type == H5O_TYPE_GROUP )
        {
            // we have a group, check if the id is actually a file as well
            H5F_info_t h5_file_info;
            h5_status = H5Fget_info(hdf5_id, &h5_file_info);
            // dest is root group of a hdf5 file
            if( CONDUIT_HDF5_STATUS_OK(h5_status) )
            {
                oss << "\nAttempt to write Conduit leaf dataset to HDF5 file root."
                    << "\nThe root of a HDF5 file is always a HDF5 Group and only"
                    << " supports Conduit `Object` or `List` Nodes.";
            }
        }

        incompat_details = oss.str();
        res = false;
    }

    return res;
}

//---------------------------------------------------------------------------//
bool
check_if_conduit_object_is_compatible_with_hdf5_tree(const Node &node,
                                                     const std::string &ref_path,
                                                     hid_t hdf5_id,
                                                     const Node &opts,
                                                     std::string &incompat_details)
{
    bool res = true;

    // make sure we have a group ...

    H5O_info_t h5_obj_info;

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_status = H5Oget_info(hdf5_id, &h5_obj_info, H5O_INFO_ALL);
#else
    herr_t h5_status = H5Oget_info(hdf5_id, &h5_obj_info);
#endif

    // make sure it is a group ...
    if( CONDUIT_HDF5_STATUS_OK(h5_status) &&
        (h5_obj_info.type == H5O_TYPE_GROUP) )
    {
        NodeConstIterator itr = node.children();

        // call on each child with expanded path
        while(itr.has_next() && res)
        {

            const Node &child = itr.next();
            // check if the HDF5 group has child with same name
            // as the node's child

            RelayH5OHandle h5_child_obj_hnd(H5Oopen(hdf5_id,
                                                    itr.name().c_str(),
                                                    H5P_DEFAULT),
                                            ref_path);

            std::string chld_ref_path = join_ref_paths(ref_path,itr.name());
            if( CONDUIT_HDF5_VALID_ID(h5_child_obj_hnd.id()) )
            {
                // if a child does exist, we need to make sure the child is
                // compatible with the conduit node
                res = check_if_conduit_node_is_compatible_with_hdf5_tree(child,
                                                                  chld_ref_path,
                                                                  h5_child_obj_hnd.id(),
                                                                  opts,
                                                                  incompat_details);
                h5_child_obj_hnd.close();
            }
            // no child exists with this name,  we are ok (it can be created
            // to match) check the next child
        }
    }
    else // bad id or not a group
    {
        std::ostringstream oss;
        oss << "Conduit Node (object) at path '" << ref_path << "'"
            << " is not compatible with given HDF5 tree at path"
            << "'" << ref_path << "'"
            << "\nConduit Object vs HDF5 Group: Bad HDF5 Group ID "
            << "or HDF5 ID is not a HDF5 Group";

        incompat_details = oss.str();

        res = false;
    }

    return res;
}

//---------------------------------------------------------------------------//
bool
check_if_conduit_list_is_compatible_with_hdf5_tree(const Node &node,
                                                   const std::string &ref_path,
                                                   hid_t hdf5_id,
                                                   const Node &opts,
                                                   std::string &incompat_details)
{
    bool res = true;

    // make sure we have a group ...

    H5O_info_t h5_obj_info;

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_status = H5Oget_info(hdf5_id, &h5_obj_info, H5O_INFO_ALL);
#else
    herr_t h5_status = H5Oget_info(hdf5_id, &h5_obj_info);
#endif

    // make sure it is a group ...
    if( CONDUIT_HDF5_STATUS_OK(h5_status) &&
        (h5_obj_info.type == H5O_TYPE_GROUP) )
    {
        // TODO: should we force the group should have our att that signals a
        //       list ?

        // if(!check_if_hdf5_group_has_conduit_list_attribute(hdf5_id,ref_path))
        // {
        //     // we don't have a list
        //         std::ostringstream oss;
        //         oss << "Conduit Node (list) at path '" << ref_path << "'"
        //             << " is not compatible with given HDF5 tree at path"
        //             << " '" << ref_path << "'"
        //             << "\nHDF5 Group missing '" << conduit_hdf5_list_attr_name
        //             << "' attribute";
        //         incompat_details = oss.str();
        //         res = false;
        //         return res;
        // }

        NodeConstIterator itr = node.children();

        // call on each child with expanded path
        while(itr.has_next() && res)
        {

            const Node &child = itr.next();
            // check if the HDF5 group has ordered children
            // that are compat with our node.

            // for a list, we check the group's children by index (not name)

            RelayH5OHandle h5_child_obj_hnd(H5Oopen_by_idx(hdf5_id, ".",
                                                H5_INDEX_CRT_ORDER,
                                                H5_ITER_INC,
                                                itr.index(),
                                                H5P_DEFAULT),
                                            hdf5_id,
                                            ref_path);

            std::string chld_ref_path = join_ref_paths(ref_path,itr.name());
            if( CONDUIT_HDF5_VALID_ID(h5_child_obj_hnd.id()) )
            {
                // if a child does exist, we need to make sure the child is
                // compatible with the conduit node
                res = check_if_conduit_node_is_compatible_with_hdf5_tree(child,
                                                                  chld_ref_path,
                                                                  h5_child_obj_hnd.id(),
                                                                  opts,
                                                                  incompat_details);
            }
            // no child exists with this index,  we are ok
            // (it can be created to match)
        }
    }
    else // bad id or not a group
    {
        std::ostringstream oss;
        oss << "Conduit Node (list) at path '" << ref_path << "'"
            << " is not compatible with given HDF5 tree at path"
            << "'" << ref_path << "'"
            << "\nConduit List vs HDF5 Group: Bad HDF5 Group ID "
            << "or HDF5 ID is not a HDF5 Group";

        incompat_details = oss.str();

        res = false;
    }

    return res;
}



//---------------------------------------------------------------------------//
bool
check_if_conduit_node_is_compatible_with_hdf5_tree(const Node &node,
                                                   const std::string &ref_path,
                                                   hid_t hdf5_id,
                                                   const Node &opts,
                                                   std::string &incompat_details)
{
    bool res = true;

    DataType dt = node.dtype();
    // check for leaf or group
    if(dt.is_number() || dt.is_string() || dt.is_empty())
    {
        res = check_if_conduit_leaf_is_compatible_with_hdf5_obj(dt,
                                                                ref_path,
                                                                hdf5_id,
                                                                opts,
                                                                incompat_details);
    }
    else if(dt.is_object())
    {
        res = check_if_conduit_object_is_compatible_with_hdf5_tree(node,
                                                                   ref_path,
                                                                   hdf5_id,
                                                                   opts,
                                                                   incompat_details);
    }
    else if(dt.is_list())
    {
        res = check_if_conduit_list_is_compatible_with_hdf5_tree(node,
                                                                 ref_path,
                                                                 hdf5_id,
                                                                 opts,
                                                                 incompat_details);
    }
    else // not supported
    {
        std::ostringstream oss;
        oss << "Conduit Node at path '" << ref_path << "'"
            << " has an unsupported dtype (" << dt.name() << ")"
            << " for HDF5 I/O and cannot be written to HDF5 path"
            << " '" << ref_path  << "'";

        incompat_details = oss.str();
        res = false;
    }

    return res;
}

//---------------------------------------------------------------------------//
bool
check_if_hdf5_group_has_conduit_list_attribute(hid_t hdf5_group_id,
                                               const std::string &ref_path)
{
    htri_t h5_att_status = H5Aexists_by_name(hdf5_group_id, ".",
                                          conduit_hdf5_list_attr_name.c_str(),
                                             H5P_DEFAULT);
    //
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5A.html#Annot-ExistsByName
    // > 0 exists, 0 doesn't exist, < 0 error
    //

    CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(h5_att_status,
                                           ref_path,
                                           "Failed call to H5Aexists_by_name"
                                           << " to check for '"
                                           << conduit_hdf5_list_attr_name
                                           << "' attribute of HDF5 Group ID "
                                           << " " << hdf5_group_id);
    return h5_att_status > 0;
}

//---------------------------------------------------------------------------//
hid_t
create_hdf5_compact_plist_for_conduit_leaf()
{
    hid_t h5_cprops_id = H5Pcreate(H5P_DATASET_CREATE);

    H5Pset_layout(h5_cprops_id,H5D_COMPACT);

    return h5_cprops_id;
}


//---------------------------------------------------------------------------//
hid_t
create_hdf5_chunked_plist_for_conduit_leaf(const DataType &dtype)
{
    hid_t h5_cprops_id = H5Pcreate(H5P_DATASET_CREATE);

    // TODO: add ndarray heuristics

    // Turn on chunking

    // hdf5 sets chunking in elements, not bytes,
    // our options are in bytes, so convert to # of elems
    hsize_t h5_chunk_size =  (hsize_t) (HDF5Options::chunk_size / dtype.element_bytes());

    H5Pset_chunk(h5_cprops_id, 1, &h5_chunk_size);

    if(HDF5Options::compression_method == "gzip" )
    {
        // Turn on compression
        H5Pset_shuffle(h5_cprops_id);
        H5Pset_deflate(h5_cprops_id, HDF5Options::compression_level);
    }
//-----------------------------------------------------------------------------
// zfp options
//-----------------------------------------------------------------------------
#if defined(CONDUIT_RELAY_IO_H5ZZFP_ENABLED)
    // else if(false)//HDF5Options::compression_method == "zfp" )
    else if(HDF5Options::compression_method == "zfp" )
    {
        // TODO: call once?
        H5Z_zfp_initialize();
        // Setup the filter using properties interface.
        if (HDF5Options::zfp_mode == H5Z_ZFP_MODE_RATE)
        {
            H5Pset_zfp_rate(h5_cprops_id, HDF5Options::zfp_rate);
        }
        else if (HDF5Options::zfp_mode == H5Z_ZFP_MODE_PRECISION)
        {
            H5Pset_zfp_precision(h5_cprops_id, HDF5Options::zfp_prec);
        }
        else if (HDF5Options::zfp_mode == H5Z_ZFP_MODE_ACCURACY)
        {
            H5Pset_zfp_accuracy(h5_cprops_id, HDF5Options::zfp_acc);
        }
        else if (HDF5Options::zfp_mode == H5Z_ZFP_MODE_EXPERT)
        {
            H5Pset_zfp_expert(h5_cprops_id,
                              HDF5Options::zfp_minbits,
                              HDF5Options::zfp_maxbits,
                              HDF5Options::zfp_maxprec,
                              HDF5Options::zfp_minexp);
        }
        else if (HDF5Options::zfp_mode == H5Z_ZFP_MODE_REVERSIBLE)
        {
            H5Pset_zfp_reversible(h5_cprops_id);
        }
    }
//-----------------------------------------------------------------------------
#endif // zfp options
//-----------------------------------------------------------------------------

    return h5_cprops_id;
}


//---------------------------------------------------------------------------//
hid_t
create_hdf5_dataset_for_conduit_leaf(const DataType &dtype,
                                     const std::string &ref_path,
                                     hid_t hdf5_group_id,
                                     const std::string &hdf5_dset_name,
                                     bool extendible)
{
    hid_t res = -1;

    RelayH5THandle h5_dtype_hnd(conduit_dtype_to_hdf5_dtype(dtype,ref_path),
                                hdf5_group_id,
                                ref_path);
    h5_dtype_hnd.check_created();

    hsize_t num_eles = (hsize_t) dtype.number_of_elements();

    hid_t h5_cprops_id = H5P_DEFAULT;

    RelayH5PHandle h5_cprops_hnd(-1, hdf5_group_id, ref_path);


    bool unlimited_dim = false;

    if (extendible && !HDF5Options::chunking_enabled)
    {
        CONDUIT_ERROR("Chunking must be enabled to create an extendible array.");
    }

    // TODO: add ndarray heuristics

    // if an offset is supplied, we will default to creating an extendible array
    if( !extendible && HDF5Options::compact_storage_enabled &&
        dtype.bytes_compact() <= HDF5Options::compact_storage_threshold)
    {
        h5_cprops_id = create_hdf5_compact_plist_for_conduit_leaf();
        // if we create custom plist, connect to handle for auto cleanup
        h5_cprops_hnd.set_id(h5_cprops_id);
    }
    else if( extendible || (HDF5Options::chunking_enabled &&
             dtype.bytes_compact() > HDF5Options::chunk_threshold))
    {
        h5_cprops_id = create_hdf5_chunked_plist_for_conduit_leaf(dtype);
        // if we create custom plist, connect to handle for auto cleanup
        h5_cprops_hnd.set_id(h5_cprops_id);
        unlimited_dim = true;
    }

    if(h5_cprops_id != H5P_DEFAULT)
    {
        h5_cprops_hnd.check_created();
    }

    RelayH5SHandle h5_dspace_hnd(-1, hdf5_group_id, ref_path);

    // string a scalar with size embedded in type is disabled
    // b/c this path undermines compression
    // if(dtype.is_string())
    // {
    //     h5_dspace_id = H5Screate(H5S_SCALAR);
    // }
    // else
    // {
    //     h5_dspace_id = H5Screate_simple(1,
    //                                     &num_eles,
    //                                     NULL);
    // }

    if (unlimited_dim)
    {
        hsize_t unlimited_dims[1] = {H5S_UNLIMITED};
        h5_dspace_hnd.set_id(H5Screate_simple(1,
                                              &num_eles,
                                              unlimited_dims));
    }
    else
    {
        // TODO: add ndarray heuristics
        h5_dspace_hnd.set_id(H5Screate_simple(1,
                                              &num_eles,
                                              NULL));
    }

    h5_dspace_hnd.check_created();

    // create new dataset
    res = H5Dcreate(hdf5_group_id,
                    hdf5_dset_name.c_str(),
                    h5_dtype_hnd.id(),
                    h5_dspace_hnd.id(),
                    H5P_DEFAULT,
                    h5_cprops_id,
                    H5P_DEFAULT);

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(res,
                                                    hdf5_group_id,
                                                    ref_path,
                                           "Failed to create HDF5 Dataset "
                                           << hdf5_group_id << " "
                                           << hdf5_dset_name);


    // auto cleanup of h5_dtype_hnd, h5_dspace_hnd, and h5_cprops_hnd (if used)
    return res;
}



//---------------------------------------------------------------------------//
hid_t
create_hdf5_dataset_for_conduit_empty(hid_t hdf5_group_id,
                                      const std::string &ref_path,
                                      const std::string &hdf5_dset_name)
{
    hid_t res = -1;

    // for conduit empty, use an opaque data type with zero size;
    RelayH5THandle h5_dtype_hnd(H5Tcreate(H5T_OPAQUE, 1), 
                                hdf5_group_id,
                                ref_path);

    h5_dtype_hnd.check_created();

    RelayH5SHandle h5_dspace_hnd(H5Screate(H5S_NULL),
                                 hdf5_group_id,
                                 ref_path);
    h5_dspace_hnd.check_created();

    // create new dataset
    res = H5Dcreate(hdf5_group_id,
                    hdf5_dset_name.c_str(),
                    h5_dtype_hnd.id(),
                    h5_dspace_hnd.id(),
                    H5P_DEFAULT,
                    H5P_DEFAULT,
                    H5P_DEFAULT);

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(res,
                                                    hdf5_group_id,
                                                    ref_path,
                                           "Failed to create HDF5 Dataset "
                                           << hdf5_group_id
                                           << " " << hdf5_dset_name);

    return res;
    // auto clean up of h5_dtype_id_hnd and h5_dspace_id
}

//-----------------------------------------------------------------------------
hid_t
create_hdf5_group_for_conduit_node(const Node &node,
                                   const std::string &ref_path,
                                   hid_t hdf5_parent_group_id,
                                   const std::string &hdf5_new_group_name)
{
    RelayH5PHandle h5_gc_plist_hnd(H5Pcreate(H5P_GROUP_CREATE),
                                   hdf5_parent_group_id,
                                   ref_path);
    h5_gc_plist_hnd.check_created();

    // track creation order
    herr_t h5_status = H5Pset_link_creation_order(h5_gc_plist_hnd.id(),
            ( H5P_CRT_ORDER_TRACKED |  H5P_CRT_ORDER_INDEXED) );

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_parent_group_id,
                                                    ref_path,
                     "Failed to set group link creation property");

    // prefer compact group storage
    // https://support.hdfgroup.org/HDF5/doc/RM/RM_H5G.html#Group-GroupStyles
    h5_status = H5Pset_link_phase_change(h5_gc_plist_hnd.id(),
                                         32,  // max for compact storage
                                         32); // min for dense storage

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_parent_group_id,
                                                    ref_path,
                "Failed to set group link phase change property ");

    // calc hints for meta data about link names
    NodeConstIterator chld_itr = node.children();

    index_t chld_names_avg_size = 0;
    index_t num_children = node.number_of_children();

    while(chld_itr.has_next())
    {
        chld_itr.next();
        chld_names_avg_size +=chld_itr.name().size();
    }

    if(chld_names_avg_size > 0 && num_children > 0 )
    {
        chld_names_avg_size = chld_names_avg_size / num_children;
    }

    // set hints for meta data about link names
    h5_status = H5Pset_est_link_info(h5_gc_plist_hnd.id(),
                                     // number of children
                                     (unsigned int)num_children,
                                     // est name size
                                     (unsigned int)chld_names_avg_size);

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_parent_group_id,
                                                    ref_path,
                "Failed to set group est link info property ");

    hid_t h5_child_id = H5Gcreate(hdf5_parent_group_id,
                                  hdf5_new_group_name.c_str(),
                                  H5P_DEFAULT,
                                  h5_gc_plist_hnd.id(),
                                  H5P_DEFAULT);

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_child_id,
                                                    hdf5_parent_group_id,
                                                    ref_path,
                          "Failed to create HDF5 Group "
                          << " parent: " << hdf5_parent_group_id
                          << " name: "   << hdf5_new_group_name);

    return h5_child_id;
    // auto cleanup of h5_gc_plist_hnd
}


//---------------------------------------------------------------------------//
void
write_conduit_leaf_to_hdf5_dataset(const Node &node,
                                   const std::string &ref_path,
                                   hid_t &hdf5_dset_id,
                                   const Node &opts)
{
    DataType dt = node.dtype();

    // NOTE: The `hdf5_dset_id` may change, not captured in current design
    RelayH5THandle h5_dtype_hnd(conduit_dtype_to_hdf5_dtype(dt,ref_path),
                                hdf5_dset_id,
                                ref_path);
    h5_dtype_hnd.check_created();

    herr_t h5_status = -1;

    hsize_t offset = 0;
    if(opts.has_child("offset"))
    {
        offset = opts["offset"].to_value();
    }

    hsize_t stride = 1;
    if(opts.has_child("stride"))
    {
        stride = opts["stride"].to_value();
    }
    if (stride == 0)
    {
        CONDUIT_ERROR("Stride must be greater than zero.");
    }

    // get dimensions of dset
    RelayH5SHandle h5_dspace_hnd(H5Dget_space(hdf5_dset_id),
                                 hdf5_dset_id,
                                 ref_path);

    hsize_t dataset_dim = H5Sget_simple_extent_npoints(h5_dspace_hnd.id());
    hsize_t dataset_max_dims[1];
    H5Sget_simple_extent_dims(h5_dspace_hnd.id(), NULL, dataset_max_dims);

    // if the layout is fixed and no offset/stride is supplied,
    // the entire array is overwriten
    if (dataset_max_dims[0] != H5S_UNLIMITED && offset == 0 && stride == 1)
    {

        // if the node is compact, we can write directly from its data ptr
        if(dt.is_compact())
        {
            // write data
            h5_status = H5Dwrite(hdf5_dset_id,
                                 h5_dtype_hnd.id(),
                                 H5S_ALL,
                                 H5S_ALL,
                                 H5P_DEFAULT,
                                 node.data_ptr());
        }
        else
        {
            // otherwise, we need to compact our data first
            Node n;
            node.compact_to(n);
            h5_status = H5Dwrite(hdf5_dset_id,
                                 h5_dtype_hnd.id(),
                                 H5S_ALL,
                                 H5S_ALL,
                                 H5P_DEFAULT,
                                 n.data_ptr());
        }
    }

    // otherwise, any fixed datasets are converted into extendible datasets
    // and the first n_elements of the entire array are overwritten.
    else
    {

        // get the node dset size
        hsize_t node_size[1] = {(hsize_t) dt.number_of_elements()};

        hsize_t offsets[1] = {offset};
        hsize_t strides[1] = {stride};

        // convert the fixed dataset to an extendible dataset if necessary
        if (dataset_max_dims[0] != H5S_UNLIMITED)
        {

            if (!HDF5Options::chunking_enabled)
            {
                CONDUIT_ERROR("Chunking must be enabled to create an "
                    << "extendible array.");
            }

            // read the hdf5 dataset into memory since node may only contain
            // part of the hdf5 dataset
            Node dset_to_node, opts_read;
            read_hdf5_dataset_into_conduit_node(hdf5_dset_id,
                                                ref_path,
                                                false,
                                                opts_read,
                                                dset_to_node);

            // get dset's name
            ssize_t hdf5_i_sz = H5Iget_name(hdf5_dset_id, NULL, 0 );
            std::vector<char>hdf5_i_buff(hdf5_i_sz+1, 0);
            H5Iget_name(hdf5_dset_id, &hdf5_i_buff[0], hdf5_i_sz+1);
            std::string hdf5_dset_path = std::string(&hdf5_i_buff[0]);

            // get the hdf5 file ID containing dset

            RelayH5FHandle h5_file_hnd(H5Iget_file_id(hdf5_dset_id),
                                       ref_path);

            // get dset's name and parent group name
            std::string hdf5_dset_parent_name;
            std::string hdf5_dset_name;
            conduit::utils::rsplit_file_path(hdf5_dset_path,
                                             hdf5_dset_name,
                                             hdf5_dset_parent_name);

            if(hdf5_dset_parent_name.size() == 0)
            {
                hdf5_dset_parent_name = "/";
            }

            // get dset's parent group ID
            RelayH5OHandle hdf5_dset_parent_hnd(H5Oopen(h5_file_hnd.id(),
                                                        hdf5_dset_parent_name.c_str(),
                                                        H5P_DEFAULT),
                                                h5_file_hnd.id(),
                                                ref_path);
            hdf5_dset_parent_hnd.check_created();

            // close the old dataset to prevent the old identifier from
            // interfering

            CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(H5Oclose(hdf5_dset_id),
                                                            ref_path,
                                 "Failed to close HDF5 Object: " <<  hdf5_dset_id);

            // delete old dset (space is made inaccessible, lost,
            // and not reclaimed)
            hdf5_remove_path(h5_file_hnd.id(), hdf5_dset_path);

            // create new extendible dset
            Node opts_create;
            opts_create["offset"] = 0;
            write_conduit_leaf_to_hdf5_group(dset_to_node,
                                             ref_path,
                                             hdf5_dset_parent_hnd.id(),
                                             hdf5_dset_name,
                                             opts_create);

            hdf5_dset_id = H5Oopen(h5_file_hnd.id(),
                                   hdf5_dset_path.c_str(),
                                   H5P_DEFAULT);
            CONDUIT_CHECK_HDF5_ERROR_WITH_REF_PATH(hdf5_dset_id,
                                                   ref_path,
                                 "Failed to get handle to new HDF5 Dataset");

            // auto clean up of hdf5_dset_parent_hnd and h5_file_hnd
            h5_dspace_hnd.close();
            h5_dspace_hnd.set_id(H5Dget_space(hdf5_dset_id));

        }

        // get the dimensions required to fit the node in the dset
        hsize_t required_dim[1] = {(hsize_t) (offset +
            dt.number_of_elements() + (dt.number_of_elements() - 1) * (stride - 1))};

        // extend the dataset if necessary
        if (dataset_dim < required_dim[0])
        {
            h5_status = H5Dset_extent(hdf5_dset_id, required_dim);

            // check extend result
            CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                            hdf5_dset_id,
                                                            ref_path,
                                                "Failed to extend HDF5 Dataset "
                                                << hdf5_dset_id);

            //get new dataspace after extending
            h5_dspace_hnd.close();
            h5_dspace_hnd.set_id(H5Dget_space(hdf5_dset_id));
        }

        // select indices to write to
        H5Sselect_hyperslab(h5_dspace_hnd.id(), H5S_SELECT_SET, offsets,
                            strides, node_size, NULL);

        RelayH5SHandle h5_node_dspace_hnd(H5Screate_simple(1, node_size, NULL),
                                          hdf5_dset_id,
                                          ref_path);
        // if the node is compact, we can write directly from its data ptr
        if(dt.is_compact())
        {
            // write data
            h5_status = H5Dwrite(hdf5_dset_id,
                                 h5_dtype_hnd.id(),
                                 h5_node_dspace_hnd.id(),
                                 h5_dspace_hnd.id(),
                                 H5P_DEFAULT,
                                 node.data_ptr());
        }
        else
        {
            // otherwise, we need to compact our data first
            Node n;
            node.compact_to(n);
            h5_status = H5Dwrite(hdf5_dset_id,
                                 h5_dtype_hnd.id(),
                                 h5_node_dspace_hnd.id(),
                                 h5_dspace_hnd.id(),
                                 H5P_DEFAULT,
                                 n.data_ptr());
        }
        // auto cleanup of h5_node_dspace_hnd
    }
    // auto cleanup of h5_dspace_hnd and h5_dtype_hnd

    // check write result
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_dset_id,
                                                    ref_path,
                                           "Failed to write to HDF5 Dataset "
                                           << hdf5_dset_id);
}



//---------------------------------------------------------------------------//
void
write_conduit_leaf_to_hdf5_group(const Node &node,
                                 const std::string &ref_path,
                                 hid_t hdf5_group_id,
                                 const std::string &hdf5_dset_name,
                                 const Node &opts)
{
    // data set case ...
    // check if the dataset exists
    H5O_info_t h5_obj_info;

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_info_status =  H5Oget_info_by_name(hdf5_group_id,
                                                 hdf5_dset_name.c_str(),
                                                 &h5_obj_info,
                                                 H5O_INFO_ALL,
                                                 H5P_DEFAULT);
#else
    herr_t h5_info_status =  H5Oget_info_by_name(hdf5_group_id,
                                                 hdf5_dset_name.c_str(),
                                                 &h5_obj_info,
                                                 H5P_DEFAULT);
#endif



    // NOTE: legacy H5Gget_objinfo might be better to use
    // https://github.com/PyTables/PyTables/issues/402#issuecomment-75051913
    // herr_t h5_info_status =  H5Gget_objinfo(hdf5_group_id,
    //                                         hdf5_dset_name.c_str(),
    //                                         0,
    //                                         NULL);
    hid_t h5_child_id = -1;
    RelayH5DHandle h5_child_hnd(h5_child_id, hdf5_group_id, ref_path);

    if( CONDUIT_HDF5_STATUS_OK(h5_info_status) )
    {
        // if it does exist, we assume it is compatible
        // (this private method will only be called after a
        //  compatibility check)
        h5_child_id = H5Dopen(hdf5_group_id,
                              hdf5_dset_name.c_str(),
                              H5P_DEFAULT);
        h5_child_hnd.set_id(h5_child_id);
        // custom check for better context
        CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_child_id,
                                                        hdf5_group_id,
                                                        ref_path,
                                               "Failed to open HDF5 Dataset "
                                               << " parent: "
                                               << hdf5_group_id
                                               << " name: "
                                               << hdf5_dset_name);
    }
    else
    {
        // if the hdf5 dataset does not exist, we need to create it
        bool extendible = false;
        if (opts.has_child("offset"))
        {
            extendible = true;
        }

        h5_child_id = create_hdf5_dataset_for_conduit_leaf(node.dtype(),
                                                                ref_path,
                                                                hdf5_group_id,
                                                                hdf5_dset_name,
                                                                extendible);
        h5_child_hnd.set_id(h5_child_id);
        CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_child_id,
                                                        hdf5_group_id,
                                                        ref_path,
                                               "Failed to create HDF5 Dataset "
                                               << " parent: "
                                               << hdf5_group_id
                                               << " name: "
                                               << hdf5_dset_name);
    }

    std::string chld_ref_path = join_ref_paths(ref_path,hdf5_dset_name);
    // write the data
    write_conduit_leaf_to_hdf5_dataset(node,
                                       chld_ref_path,
                                       h5_child_id,
                                       opts);
    // note: h5_child_id may have changed
    h5_child_hnd.set_id(h5_child_id);

    // auto cleanup of h5_child_hnd
}

//---------------------------------------------------------------------------//
void
write_conduit_empty_to_hdf5_group(hid_t hdf5_group_id,
                                  const std::string &ref_path,
                                  const std::string &hdf5_dset_name)
{

    // check if the dataset exists
    H5O_info_t h5_obj_info;

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_info_status =  H5Oget_info_by_name(hdf5_group_id,
                                                 hdf5_dset_name.c_str(),
                                                 &h5_obj_info,
                                                 H5O_INFO_ALL,
                                                 H5P_DEFAULT);
#else
    herr_t h5_info_status =  H5Oget_info_by_name(hdf5_group_id,
                                                 hdf5_dset_name.c_str(),
                                                 &h5_obj_info,
                                                 H5P_DEFAULT);
#endif

    if( CONDUIT_HDF5_STATUS_OK(h5_info_status) )
    {
        // if it does exist, we assume it is compatible
        // (this private method will only be called after a
        //  compatibility check)
    }
    else
    {
        // if the hdf5 dataset does not exist, we need to create it
        RelayH5DHandle h5_child_hnd(create_hdf5_dataset_for_conduit_empty(hdf5_group_id,
                                                                          ref_path,
                                                                          hdf5_dset_name),
                                    hdf5_group_id,
                                    ref_path);
        h5_child_hnd.check_created();
        // auto cleanup of h5_child_hnd
    }


}

//---------------------------------------------------------------------------//
void
setup_hdf5_group_atts_for_conduit_node(const Node &node,
                                       const std::string &ref_path,
                                       hid_t hdf5_group_id)
{
    bool has_list_attr = check_if_hdf5_group_has_conduit_list_attribute(hdf5_group_id,
                                                                        ref_path);

    if( !has_list_attr && node.dtype().is_list() )
    {
        write_conduit_hdf5_list_attribute(hdf5_group_id,
                                          ref_path);
    }

    if( has_list_attr && node.dtype().is_object() )
    {
        // std::cout << " remove_conduit_hdf5_list_attribute " << std::endl;
        remove_conduit_hdf5_list_attribute(hdf5_group_id,
                                           ref_path);
    }
}


//---------------------------------------------------------------------------//
// assume this is called only if we know the hdf5 state is compatible
//---------------------------------------------------------------------------//
void
write_conduit_node_children_to_hdf5_group(const Node &node,
                                          const std::string &ref_path,
                                          hid_t hdf5_group_id,
                                          const Node &opts)
{
    // make sure our special atts are setup correctly
    setup_hdf5_group_atts_for_conduit_node(node,
                                           ref_path,
                                           hdf5_group_id);

    NodeConstIterator itr = node.children();

    // call on each child with expanded path
    while(itr.has_next())
    {
        const Node &child = itr.next();
        DataType dt = child.dtype();
        std::string child_name = itr.name();

        if(dt.is_number() || dt. is_string())
        {
            write_conduit_leaf_to_hdf5_group(child,
                                             ref_path,
                                             hdf5_group_id,
                                             child_name.c_str(),
                                             opts);
        }
        else if(dt.is_empty())
        {
            // if we have an empty node, it will become
            // a dataset with an null shape
            write_conduit_empty_to_hdf5_group(hdf5_group_id,
                                              ref_path,
                                              child_name.c_str());
        }
        else if( dt.is_object() || dt.is_list() )
        {
            // check if the HDF5 group has child with same name
            // as the node's child
            H5O_info_t h5_obj_info;

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
            herr_t h5_info_status =  H5Oget_info_by_name(hdf5_group_id,
                                                         child_name.c_str(),
                                                         &h5_obj_info,
                                                         H5O_INFO_ALL,
                                                         H5P_DEFAULT);
#else
            herr_t h5_info_status =  H5Oget_info_by_name(hdf5_group_id,
                                                         child_name.c_str(),
                                                         &h5_obj_info,
                                                         H5P_DEFAULT);
#endif

            RelayH5GHandle h5_child_hnd(-1, hdf5_group_id, ref_path);

            if( CONDUIT_HDF5_STATUS_OK(h5_info_status) )
            {
                // if the hdf5 group exists, open it
                h5_child_hnd.set_id(H5Gopen(hdf5_group_id,
                                            child_name.c_str(),
                                            H5P_DEFAULT));
                // custom check for better context
                CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_child_hnd.id(),
                                                                hdf5_group_id,
                                                                ref_path,
                                             "Failed to open HDF5 Group "
                                             << " parent: " << hdf5_group_id
                                             << " name: "   << itr.name());
            }
            else
            {
                // if the hdf5 group doesn't exist, we need to create it
                h5_child_hnd.set_id(create_hdf5_group_for_conduit_node(child,
                                                                       ref_path,
                                                                       hdf5_group_id,
                                                                       child_name));
                h5_child_hnd.check_created();
            }

            // traverse
            write_conduit_node_children_to_hdf5_group(child,
                                                      ref_path,
                                                      h5_child_hnd.id(),
                                                      opts);
            // auto cleanup of h5_child_hnd
        }
        else
        {
            CONDUIT_HDF5_WARN(ref_path,
                               "DataType \'"
                               << DataType::id_to_name(dt.id())
                               <<"\' not supported for relay HDF5 I/O");
        }
    }
}


//---------------------------------------------------------------------------//
// assumes compatible, dispatches to proper specific write
//---------------------------------------------------------------------------//
void
write_conduit_node_to_hdf5_tree(const Node &node,
                                const std::string &ref_path,
                                hid_t &hdf5_id,
                                const Node &opts)
{

    DataType dt = node.dtype();
    // we support a leaf or a group
    if( dt.is_number() || dt.is_string() )
    {
        write_conduit_leaf_to_hdf5_dataset(node,
                                           ref_path,
                                           hdf5_id,
                                           opts);
    }
    else if( dt.is_object() || dt.is_list() )
    {
        write_conduit_node_children_to_hdf5_group(node,
                                                  ref_path,
                                                  hdf5_id,
                                                  opts);
    }
    else // not supported
    {
        std::string hdf5_err_ref_path;
        hdf5_ref_path_with_filename(hdf5_id, ref_path, hdf5_err_ref_path);
        CONDUIT_HDF5_ERROR(hdf5_err_ref_path,
                   "HDF5 write doesn't support EMPTY_ID nodes.");
    }
}


//---------------------------------------------------------------------------//
void
write_conduit_hdf5_list_attribute(hid_t hdf5_group_id,
                                  const std::string &ref_path)
{
    //  We really just use the presence of the attribute, we don't need
    //  data associated with it.
    //
    //  I tried to write a null att (null hdf5 dt, etc) but that didn't work.
    //  H5Awrite fails with message about null data. I could't find any
    //  examples that demoed this either -- it may not be supported.
    //
    //  So, we write a single meaningless int as the attribute data.
    //  Perhaps someone could breath meaning into this int in the future
    //  or find a way to eliminate it.

    int att_value = 1;

    RelayH5SHandle h5_dspace_hnd(H5Screate(H5S_SCALAR),
                                 hdf5_group_id,
                                 ref_path);
    h5_dspace_hnd.check_created();

    RelayH5AHandle h5_attr_hnd(H5Acreate(hdf5_group_id,
                                         conduit_hdf5_list_attr_name.c_str(),
                                         H5T_NATIVE_INT,
                                         h5_dspace_hnd.id(),
                                         H5P_DEFAULT,
                                         H5P_DEFAULT),
                               hdf5_group_id,
                               ref_path);

    // custom check for better context
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_attr_hnd.id(),
                                                    hdf5_group_id,
                                                    ref_path,
                                           "Failed to create HDF5 Attribute "
                                           << hdf5_group_id
                                           << " "
                                           << conduit_hdf5_list_attr_name.c_str());


    hid_t h5_status = H5Awrite(h5_attr_hnd.id(),
                               H5T_NATIVE_INT,
                               &att_value);
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_group_id,
                                                    ref_path,
                                           "Failed to write HDF5 Attribute "
                                           << hdf5_group_id
                                           << " "
                                           << conduit_hdf5_list_attr_name.c_str());

    // auto cleanup of h5_dspace_hnd and h5_attr_hnd
}

//---------------------------------------------------------------------------//
void
remove_conduit_hdf5_list_attribute(hid_t hdf5_group_id,
                                   const std::string &ref_path)
{
    // cleanup group attached att, just in case a group changes roles
    // and is still compatible otherwise
    herr_t h5_status = H5Adelete(hdf5_group_id,
                                 conduit_hdf5_list_attr_name.c_str());

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_group_id,
                                                    ref_path,
                                           "Failed to remove HDF5 Attribute "
                                           << hdf5_group_id
                                           << " "
                                           << conduit_hdf5_list_attr_name.c_str());
}



//---------------------------------------------------------------------------//
// Read Helpers
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
/// Data structures and callbacks that allow us to read an HDF5 hierarchy
/// via H5Literate
/// (adapted from: h5ex_g_traverse)
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
///
/// I also reviewed / partially tried the seemingly more straight forward
/// approach in:
///  https://www.hdfgroup.org/ftp/HDF5/examples/misc-examples/h5_info.c
///
/// But since H5Gget_objtype_by_idx and H5Gget_objname_by_idx are deprecated
/// there in favor of H5Lget_name_by_idx & H5Oget_info_by_idx, there isn't a
/// direct way to check for links which could create cycles.
///
/// It appears that the H5Literate method (as demonstrated in the
/// h5ex_g_traverse example) is the preferred way to read an hdf5 location
/// hierarchically, even if it seems overly complex.
///
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
/// Operator struct type for our H5Literate callback.
/// (adapted from: h5ex_g_traverse)
//---------------------------------------------------------------------------//
//
// Define operator data structure type for H5Literate callback.
// During recursive iteration, these structures will form a
// linked list that can be searched for duplicate groups,
// preventing infinite recursion.
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// For conduit+relay: Added a pointer to hold a conduit node for construction.
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
//---------------------------------------------------------------------------//
struct h5_read_opdata
{
    unsigned                recurs;      /* Recursion level.  0=root */
    struct h5_read_opdata   *prev;       /* Pointer to previous opdata */

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    H5O_token_t            *token;       /* Group token */
#else
    haddr_t                 addr;        /* Group address */
#endif

    // pointer to conduit node, anchors traversal to
    Node            *node;
    const Node      *opts;
    std::string      ref_path;

    // whether to only get metadata
    bool             metadata_only;

    // hold error state
    //
    // don't let exceptions unwind or hdf5 will leak handles used for traversal
    // https://support.hdfgroup.org/documentation/hdf5/latest/_r_m.html#cpp_c_api_note
    //
    // stash error info in our traversal callback, so we can throw
    // post traverse
    bool            error_thrown;
    conduit::Error  traversal_error;
};

//
// Helper to init our h5_read_opdata struct
//
void init_h5_read_opdata(h5_read_opdata &h5_od,
                         H5O_info_t &h5_info_buf,
                         const std::string &ref_path,
                         bool only_get_metadata,
                         const Node &opts,
                         Node &dest)
{

    // setup linked list tracking that allows us to detect cycles
    h5_od.recurs = 0;
    h5_od.prev = NULL;
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    h5_od.token = &h5_info_buf.token;
#else
    h5_od.addr = h5_info_buf.addr;
#endif
    // attach the pointer to our node
    h5_od.node = &dest;
    h5_od.opts = &opts;
    // keep ref path
    h5_od.ref_path = ref_path;

    // whether to only get metadata
    if (only_get_metadata)
    {
        h5_od.metadata_only = true;
    }
    else
    {
        h5_od.metadata_only = false;
    }

    h5_od.error_thrown = false;
    h5_od.traversal_error = conduit::Error();
}

//---------------------------------------------------------------------------//
/// Recursive check for cycles.
/// (adapted from: h5ex_g_traverse)
//---------------------------------------------------------------------------//
//
//  This function recursively searches the linked list of
//  h5_read_opdata structures for one whose address matches
//  target_addr.  Returns 1 if a match is found, and 0
//  otherwise.
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// -- hdf5 1.12 implementation
//---------------------------------------------------------------------------//
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
int
h5_group_check(h5_read_opdata *od,
               hid_t h5_id,
               H5O_token_t *target_token)
{

    int cmp = -1;
    H5Otoken_cmp(h5_id, od->token, target_token, &cmp);

    if (cmp == 0)
    {
        /* Addresses match */
        return 1;
    }
    else if (!od->recurs)
    {
        /* Root group reached with no matches */
        return 0;
    }
    else
    {
        /* Recursively examine the next node */
        return h5_group_check(od->prev, h5_id, target_token);
    }
}
#else
//---------------------------------------------------------------------------//
// -- pre hdf5 1.12 implementation
//---------------------------------------------------------------------------//
int
h5_group_check(h5_read_opdata *od,
               haddr_t target_addr)
{

    if (od->addr == target_addr)
    {
        /* Addresses match */
        return 1;
    }
    else if (!od->recurs)
    {
        /* Root group reached with no matches */
        return 0;
    }
    else
    {
        /* Recursively examine the next node */
        return h5_group_check(od->prev, target_addr);
    }
}
#endif

//---------------------------------------------------------------------------//
Node *
h5l_iterate_traverse_op_func_get_child(Node &node,
                                       const std::string &hdf5_path)
{
    Node *chld_node_ptr = NULL;
    // if( h5_od->node->dtype().is_object() )
    if( node.dtype().is_object() )
    {
        // execute traversal for this group
        chld_node_ptr = &node.fetch(hdf5_path);
    }
    else if( node.dtype().is_list() )
    {
        // we need the child index, use name to index for now
        // not sure if it is possible to get iteration index
        // from h5literate

        // Either the child already exists in conduit
        // (compat case), or we need to append to add
        // a new child

        // parse index for this list entry
        std::istringstream iss(hdf5_path);
        int child_idx = -1;
        iss >> child_idx;

        if(node.number_of_children() <= child_idx )
        {
            node.append();
        }

        chld_node_ptr = &node.child(child_idx);
    }
    else
    {
        // TODO Error: We should not land here, H5Literate should
        // only be called on groups, which will correspond
        // to either objects or lists
    }

    return chld_node_ptr;
}

//---------------------------------------------------------------------------//
/// Our main callback for H5Literate.
/// (adapted from: h5ex_g_traverse)
//---------------------------------------------------------------------------//
//  Operator function.  This function prints the name and type
//  of the object passed to it.  If the object is a group, it
//  is first checked against other groups in its path using
//  the group_check function, then if it is not a duplicate,
//  H5Literate is called for that group.  This guarantees that
//  the program will not enter infinite recursion due to a
//  circular path in the file.
//---------------------------------------------------------------------------//
herr_t
h5l_iterate_traverse_op_func(hid_t hdf5_id,
                             const char *hdf5_path,
                             const H5L_info_t *,// hdf5_info -- unused
                             void *hdf5_operator_data)
{
    herr_t h5_status = 0;
    herr_t h5_return_val = 0;
    H5O_info_t h5_info_buf;

    /* Type conversion */
    struct h5_read_opdata *h5_od = (struct h5_read_opdata*)hdf5_operator_data;

    // catch any exceptions to avoid stack unwind during traversal
    try
    {
        /*
        * Get type of the object and display its name and type.
        * The name of the object is passed to this function by
        * the Library.
        */

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
        h5_status = H5Oget_info_by_name(hdf5_id,
                                        hdf5_path,
                                        &h5_info_buf,
                                        H5O_INFO_ALL,
                                        H5P_DEFAULT);
#else
        h5_status = H5Oget_info_by_name(hdf5_id,
                                        hdf5_path,
                                        &h5_info_buf,
                                        H5P_DEFAULT);
#endif

        CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                        hdf5_id,
                                                        h5_od->ref_path,
                                            "Error fetching HDF5 Object info: "
                                            << " parent: " << hdf5_id
                                            << " path:"    << hdf5_path) ;

        std::string chld_ref_path = h5_od->ref_path;
        if(chld_ref_path != std::string("/"))
        {
            chld_ref_path += std::string("/");
        }
        chld_ref_path += std::string(hdf5_path);

        switch (h5_info_buf.type)
        {
            case H5O_TYPE_GROUP:
            {
    #if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
                /*
                * With 1.12, we compare tokens, with the hope this provides
                * the same cycle avoidance.
                */
                if ( h5_group_check (h5_od, hdf5_id, &h5_info_buf.token) )
    #else
                /*
                * Check group address against linked list of operator
                * data structures.  We will always run the check, as the
                * reference count cannot be relied upon if there are
                * symbolic links, and H5Oget_info_by_name always follows
                * symbolic links.  Alternatively we could use H5Lget_info
                * and never recurse on groups discovered by symbolic
                * links, however it could still fail if an object's
                * reference count was manually manipulated with
                * H5Odecr_refcount.
                */
                if ( h5_group_check (h5_od, h5_info_buf.addr) )
    #endif
                {
                    // skip cycles in the graph ...
                }
                else
                {
                    RelayH5GHandle h5_group_hnd(H5Gopen(hdf5_id,
                                                        hdf5_path,
                                                        H5P_DEFAULT),
                                                hdf5_id,
                                                h5_od->ref_path);
                    // custom check for better context
                    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_group_hnd.id(),
                                                                    hdf5_id,
                                                                    h5_od->ref_path,
                                                        "Error opening HDF5 "
                                                        << "Group: "
                                                        << " parent: "
                                                        << hdf5_id
                                                        << " path:"
                                                        << hdf5_path);

                    Node *chld_node_ptr = h5l_iterate_traverse_op_func_get_child(
                                                    *h5_od->node,
                                                    std::string(hdf5_path));

                    read_hdf5_group_into_conduit_node(h5_group_hnd.id(),
                                                    chld_ref_path,
                                                    h5_od->metadata_only,
                                                    *h5_od->opts,
                                                    *chld_node_ptr);

                    // autoclean up of h5_group_hnd
                }
                break;
            }
            case H5O_TYPE_DATASET:
            {
                Node *chld_node_ptr = h5l_iterate_traverse_op_func_get_child(
                                                    *h5_od->node,
                                                    std::string(hdf5_path));

                // open hdf5 dataset at path
                RelayH5DHandle h5_dset_hnd(H5Dopen(hdf5_id,
                                                hdf5_path,
                                                H5P_DEFAULT),
                                            hdf5_id,
                                            h5_od->ref_path);

                // custom check for better context
                CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_dset_hnd.id(),
                                                                hdf5_id,
                                                                h5_od->ref_path,
                                                    "Error opening HDF5 "
                                                    << " Dataset: "
                                                    << " parent: "
                                                    << hdf5_id
                                                    << " path:"
                                                    << hdf5_path);
                read_hdf5_dataset_into_conduit_node(h5_dset_hnd.id(),
                                                    chld_ref_path,
                                                    h5_od->metadata_only,
                                                    *h5_od->opts,
                                                    *chld_node_ptr);

                // auto clean up of h5_dset_hnd
                break;
            }
            default:
            {
                // unsupported
            }
        }
    }
    catch(const conduit::Error &e)
    {
        // record that an error was thrown and
        // a copy of the details so we can rethrow
        // after hdf5 traversal process is complete
        h5_od->error_thrown = true;
        h5_od->traversal_error = e;
        h5_return_val = H5_ITER_ERROR;
    }

    return h5_return_val;
}


//---------------------------------------------------------------------------//
void
read_hdf5_group_into_conduit_node(hid_t hdf5_group_id,
                                  const std::string &ref_path,
                                  bool only_get_metadata,
                                  const Node &opts,
                                  Node &dest)
{
    // get info, we need to get the obj addr for cycle tracking
    H5O_info_t h5_info_buf;
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_status = H5Oget_info(hdf5_group_id,
                                   &h5_info_buf,
                                   H5O_INFO_ALL);
#else
    herr_t h5_status = H5Oget_info(hdf5_group_id,
                                   &h5_info_buf);
#endif

    // Check if this is a list or an object case
    if(check_if_hdf5_group_has_conduit_list_attribute(hdf5_group_id,
                                                      ref_path))
    {
        // special att: we have a list
        dest.set(DataType::list());
    }
    else
    {
        // no special att: we have an object
        dest.set(DataType::object());
    }

    // setup the callback struct we will use for  H5Literate
    struct h5_read_opdata  h5_od;

    init_h5_read_opdata(h5_od,
                        h5_info_buf,
                        ref_path,
                        only_get_metadata,
                        opts,
                        dest);

    H5_index_t h5_grp_index_type = H5_INDEX_NAME;

    // check for creation order index using propertylist

    RelayH5PHandle h5_gc_plist_hnd(H5Gget_create_plist(hdf5_group_id),
                                   hdf5_group_id,
                                   ref_path);

    if( CONDUIT_HDF5_VALID_ID(h5_gc_plist_hnd.id()) )
    {
        unsigned int h5_gc_flags = 0;
        h5_status = H5Pget_link_creation_order(h5_gc_plist_hnd.id(),
                                           &h5_gc_flags);

        // first make sure we have the link creation order plist
        if( CONDUIT_HDF5_STATUS_OK(h5_status) )
        {
            // check that we have both order_tracked and order_indexed
            if( h5_gc_flags & (H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED) )
            {
                // if so, we can use creation order in h5literate
                h5_grp_index_type = H5_INDEX_CRT_ORDER;
            }
        }
    }

    // auto cleanup of h5_gc_plist_hnd

    // use H5_ITER_ERROR
    // don't let exceptions unwind or hdf5 will leak handles used for traversal
    // https://support.hdfgroup.org/documentation/hdf5/latest/_r_m.html#cpp_c_api_note

    // use H5Literate to traverse
    h5_status = H5Literate(hdf5_group_id,
                           h5_grp_index_type,
                           H5_ITER_INC,
                           NULL,
                           h5l_iterate_traverse_op_func,
                           (void *) &h5_od);

    if(h5_status == H5_ITER_ERROR && h5_od.error_thrown)
    {
        throw h5_od.traversal_error;
    }

    // check for an error that was not an exception
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_group_id,
                                                    ref_path,
                                           "Error calling H5Literate to "
                                           << "traverse and read HDF5 "
                                           << "hierarchy: "
                                           << hdf5_group_id);
}

//---------------------------------------------------------------------------//
index_t_array
conduit_node_to_argarray(Node& n,
                         const char* argname,
                         const char* altname,
                         int rank,
                         index_t dft)
{
    Node& hdf5array = n["slabparams"][argname];
    if (n.has_child(argname))
    {
        n[argname].to_index_t_array(hdf5array);
    }
    else if (n.has_child(altname))
    {
        n[altname].to_index_t_array(hdf5array);
    }
    else
    {
        hdf5array.set(DataType::index_t(rank));
        index_t_array p_ary = hdf5array.value();
        for (int d = 0; d < rank; ++d)
        {
            p_ary[d] = dft;
        }
    }

    return hdf5array.as_index_t_array();
}


//---------------------------------------------------------------------------//
index_t
calculate_readsize(index_t_array readsize, index_t rank,
    const index_t_array dataset_sizes, const index_t_array offsets, const index_t_array strides)
{
    index_t readtotal = 1;
    for (int d = 0; d < rank; ++d)
    {
        if (readsize[d] == 0)
        {
            readsize[d] = (dataset_sizes[d] - offsets[d]) / strides[d];
            if ((dataset_sizes[d] - offsets[d]) % strides[d] != 0)
            {
                readsize[d]++;
            }
        }
        readtotal = readtotal * readsize[d];
    }
    return readtotal;
}


//---------------------------------------------------------------------------//
void
fill_dataset_opts(const std::string &ref_path,
                  const Node & inopts,
                  hid_t dataspace_id,
                  Node &filled_opts)
{

    // Intent here is to do a deep copy, since opts is a const ref
    // and I want to modify it
    filled_opts = inopts;

    // If we've already filled in the options, don't re-fill them.
    if (inopts.has_child("slabparams"))
    {
        return;
    }

    index_t rank       = H5Sget_simple_extent_ndims(dataspace_id);
    filled_opts["slabparams/rank"] = rank;

    // Here we should do some error checking.  At least:
    // - Each element of stride >= 1
    // - Each element of offset >= 0

    // If dataspace_id is a scalar, then H5Sget_simple_extent_ndims will
    // return zero.  Setting rank to 0 makes the following code create
    // zero-length arrays for offset, stride, and size, which is unhealthy.
    bool is_scalar = false;
    if (rank < 1)
    {
        is_scalar = true;
        rank = 1;
    }

    Node& nsizes = filled_opts["slabparams/dataset_sizes"];
    nsizes.set(DataType::index_t(rank));
    index_t_array nsizes_array = nsizes.value();
    std::vector<hsize_t> psizes(rank);
    H5Sget_simple_extent_dims(dataspace_id, psizes.data(), nullptr);
    for (int d = 0; d < rank; ++d)
    {
        nsizes_array[d] = psizes[d];
    }

    index_t_array offset = conduit_node_to_argarray(filled_opts, "offsets", "offset", rank, 0);
    index_t_array stride = conduit_node_to_argarray(filled_opts, "strides", "stride", rank, 1);
    index_t_array readsz = conduit_node_to_argarray(filled_opts, "sizes", "size", rank, (int)is_scalar);

    for (int d = 0; d < rank; ++d)
    {
        if (stride[d] == 0)
        {
            CONDUIT_HDF5_ERROR(ref_path,
                "Error reading HDF5 Dataset with options:"
                << inopts.to_yaml() <<
                "`stride` must be greater than zero.");
        }
    }

    index_t readcount = calculate_readsize(readsz, rank, nsizes_array, offset, stride);
    filled_opts["slabparams/readcount"] = readcount;
}

//---------------------------------------------------------------------------//
void
make_dataset_opt_copy(const Node& opts,
                      const std::string opt_name,
                      std::vector<hsize_t> &values)
{
    const index_t_accessor hdf5array = opts["slabparams"].fetch_existing(opt_name).as_index_t_accessor();
    int rank = hdf5array.number_of_elements();
    values.resize(rank);
    for (int d = 0; d < rank; ++d)
    {
        values[d] = (hsize_t)hdf5array[d];
    }
}

//---------------------------------------------------------------------------//
void
read_hdf5_dataset_into_conduit_node(hid_t hdf5_dset_id,
                                    const std::string &ref_path,
                                    bool only_get_metadata,
                                    const Node &opts,
                                    Node &dest)
{

    RelayH5SHandle h5_dspace_hnd(H5Dget_space(hdf5_dset_id),
                                 hdf5_dset_id,
                                 ref_path);
    // custom check for better context
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_dspace_hnd.id(),
                                                    hdf5_dset_id,
                                                    ref_path,
                                           "Error reading HDF5 Dataspace: "
                                           << h5_dspace_hnd.id());

    // check for empty case
    if(H5Sget_simple_extent_type(h5_dspace_hnd.id()) == H5S_NULL)
    {
        // change to empty
        dest.reset();
    }
    else
    {
        RelayH5THandle h5_dtype_hnd(H5Dget_type(hdf5_dset_id), 
                                    hdf5_dset_id,
                                    ref_path);
        // custom check for better context
        CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_dtype_hnd.id(),
                                                        hdf5_dset_id,
                                                        ref_path,
                                               "Error reading HDF5 Datatype: "
                                               << hdf5_dset_id);

        hid_t h5_status = 0;

        Node filled_opts;
        fill_dataset_opts(ref_path, opts, h5_dspace_hnd.id(), filled_opts);

        Node& slab_params = filled_opts["slabparams"];
        index_t rank = slab_params["rank"].to_long_long();
        hsize_t readtotal = slab_params["readcount"].to_unsigned_long_long();
        index_t nelems = (index_t)readtotal;
        // use std::vector vs raw allocd pointer here b/c
        // the vector will be cleaned up in exception cases
        std::vector<hsize_t> readsize_vec;
        std::vector<hsize_t> offset_vec;
        std::vector<hsize_t> stride_vec;

        make_dataset_opt_copy(filled_opts, "sizes",readsize_vec);
        make_dataset_opt_copy(filled_opts, "offsets", offset_vec);
        make_dataset_opt_copy(filled_opts, "strides", stride_vec);
        hsize_t* readsize = readsize_vec.data();
        hsize_t* offset = offset_vec.data();
        hsize_t* stride = stride_vec.data();

        // copy metadata to the node under hard-coded keys
        if (only_get_metadata)
        {
            dest["num_elements"].set(readsize, rank);
        }
        else
        {
            // Note: string case is handed properly in hdf5_dtype_to_conduit_dtype
            DataType dt        = hdf5_dtype_to_conduit_dtype(h5_dtype_hnd.id(),
                                                             readsize,
                                                             rank,
                                                             ref_path);

            // if the endianness of the dset in the file doesn't
            // match the current machine we always want to convert it
            // on read.

            // check endianness
            // Note: string cases never land here b/c they are
            // created with default endianness
            if(!dt.endianness_matches_machine())
            {
                // if they don't match, modify the dt
                // and get the proper hdf5 data type handle
                dt.set_endianness(Endianness::machine_default());

                // clean up our old handle
                h5_dtype_hnd.close();

                // get ref to standard variant of this dtype
                h5_dtype_hnd.set_id(conduit_dtype_to_hdf5_dtype(dt,
                                                           ref_path));
                h5_dtype_hnd.check_created();
            }

            hsize_t node_size[1] = {readtotal};
            RelayH5SHandle h5_node_dspace_hnd(H5Screate_simple(1,node_size,NULL),
                                              hdf5_dset_id,
                                              ref_path);
            h5_node_dspace_hnd.check_created();

            // select hyperslab
            H5Sselect_hyperslab(h5_dspace_hnd.id(), H5S_SELECT_SET, offset, stride,
                                readsize, NULL);
            // Don't check for errors here, because H5Sselect_hyperslab
            // returns -1 (error) if dataspace refers to a scalar.

            // check for string special case, H5T_VARIABLE string
            if( H5Tis_variable_str(h5_dtype_hnd.id()) )
            {
                //special case for reading variable string data
                // hdf5 reads the data onto its heap, and
                // gives us a pointer to that location

                char *read_ptr[1] = {NULL};
                h5_status = H5Dread(hdf5_dset_id,
                                    h5_dtype_hnd.id(),
                                    h5_node_dspace_hnd.id(),
                                    h5_dspace_hnd.id(),
                                    H5P_DEFAULT,
                                    read_ptr);

                if(read_ptr[0] != NULL)
                {
                    // copy the data out to the conduit node
                    dest.set_string(read_ptr[0]);
                    // hdf5 allocates this for us, so we need
                    // to clean up
                    free(read_ptr[0]);
                }
            }
            // check for bad # of elements
            else if( dt.number_of_elements() < 0 )
            {
                CONDUIT_HDF5_ERROR(ref_path,
                                   "Error reading HDF5 Dataset with options:"
                                   << opts.to_yaml()
                                   << "Cannot read using offset (" << offset << ")"
                                   << " greater than the number of entries in"
                                   << " the HDF5 dataset (" << nelems << ")");
            }
            else if(dest.dtype().is_compact() &&
               dest.dtype().compatible(dt) )
            {
                // we can read directly from hdf5 dataset if compact
                // & compatible
                h5_status = H5Dread(hdf5_dset_id,
                                    h5_dtype_hnd.id(),
                                    h5_node_dspace_hnd.id(),
                                    h5_dspace_hnd.id(),
                                    H5P_DEFAULT,
                                    dest.data_ptr());
            }
            else
            {
                // we create a temp Node b/c we want read to work for
                // strided data
                //
                // the hdf5 data will always be compact, source node we are
                // reading will not unless it's already compatible and compact.
                Node n_tmp(dt);
                h5_status = H5Dread(hdf5_dset_id,
                                    h5_dtype_hnd.id(),
                                    h5_node_dspace_hnd.id(),
                                    h5_dspace_hnd.id(),
                                    H5P_DEFAULT,
                                    n_tmp.data_ptr());

                // copy out to our dest
                dest.set(n_tmp);
            }
            // auto cleanup of h5_node_dspace_hnd
        }

        if(opts.dtype().is_empty())
        {
            CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                            hdf5_dset_id,
                                                            ref_path,
                                                            "Error reading HDF5 Dataset: "
                                                                << hdf5_dset_id);
        }
        else
        {
            CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                            hdf5_dset_id,
                                                            ref_path,
                                                            "Error reading HDF5 Dataset: "
                                                            << hdf5_dset_id
                                                            << " with options: "
                                                            << opts.to_yaml()
                                                            << "HDF5 dataset size: "
                                                            << nelems);
        }
    }

    // auto cleanup of h5_dspace_hnd
}

//---------------------------------------------------------------------------//
void
read_hdf5_tree_into_conduit_node(hid_t hdf5_id,
                                 const std::string &ref_path,
                                 bool only_get_metadata,
                                 const Node &opts,
                                 Node &dest)
{
    H5O_info_t h5_info_buf;

#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_status = H5Oget_info(hdf5_id,&h5_info_buf,H5O_INFO_ALL);
#else
    herr_t h5_status = H5Oget_info(hdf5_id,&h5_info_buf);
#endif


    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_id,
                                                    ref_path,
                                           "Error fetching HDF5 object "
                                           << "info from: "
                                           << hdf5_id);

    switch (h5_info_buf.type)
    {
        // if hdf5_id + hdf5_path points to a group,
        // use a H5Literate traversal
        case H5O_TYPE_GROUP:
        {
            read_hdf5_group_into_conduit_node(hdf5_id,
                                              ref_path,
                                              only_get_metadata,
                                              opts,
                                              dest);
            break;
        }
        // if hdf5_id + hdf5_path points directly to a dataset
        // skip the H5Literate traversal
        case H5O_TYPE_DATASET:
        {
            read_hdf5_dataset_into_conduit_node(hdf5_id,
                                                ref_path,
                                                only_get_metadata,
                                                opts,
                                                dest);
            break;
        }
        // unsupported types
        case H5O_TYPE_UNKNOWN:
        {
            // we only construct these strings
            // when an error occurs, to avoid overhead
            // for healthy fetches
            std::string hdf5_err_ref_path;
            hdf5_ref_path_with_filename(hdf5_id,
                                        ref_path,
                                        hdf5_err_ref_path);
            CONDUIT_HDF5_ERROR(hdf5_err_ref_path,
                               "Cannot read HDF5 Object : "
                                << "(type == H5O_TYPE_UNKNOWN )");
            break;
        }
        case H5O_TYPE_NAMED_DATATYPE:
        {
            std::string hdf5_err_ref_path;
            hdf5_ref_path_with_filename(hdf5_id,
                                        ref_path,
                                        hdf5_err_ref_path);
            CONDUIT_HDF5_ERROR(hdf5_err_ref_path,
                               "Cannot read HDF5 Object "
                               << "(type == H5O_TYPE_NAMED_DATATYPE )");
            break;
        }
        case H5O_TYPE_NTYPES:
        {
            std::string hdf5_err_ref_path;
            hdf5_ref_path_with_filename(hdf5_id,
                                        ref_path,
                                        hdf5_err_ref_path);
            CONDUIT_HDF5_ERROR(hdf5_err_ref_path,
              "Cannot read HDF5 Object "
               << "(type == H5O_TYPE_NTYPES [This is an invalid HDF5 type!]");
            break;
        }
        default:
        {
            std::string hdf5_err_ref_path;
            hdf5_ref_path_with_filename(hdf5_id,
                                        ref_path,
                                        hdf5_err_ref_path);
            CONDUIT_HDF5_ERROR(hdf5_err_ref_path,
                               "Cannot read HDF5 Object (type == Unknown )");
        }
    }
}



//---------------------------------------------------------------------------//
hid_t
create_hdf5_file_access_plist()
{
    // create property list and set use latest lib ver settings
    hid_t h5_fa_props = H5Pcreate(H5P_FILE_ACCESS);

    CONDUIT_CHECK_HDF5_ERROR(h5_fa_props,
                             "Failed to create H5P_FILE_ACCESS "
                             << " property list");

    unsigned int major_num=0;
    unsigned int minor_num=0;
    unsigned int release_num=0;

    herr_t h5_status = H5get_libversion(&major_num, &minor_num,&release_num);

    CONDUIT_CHECK_HDF5_ERROR(h5_status,
                             "Failed to fetch HDF5 library version info ");

    // most of our use cases are still using 1.8.
    // to allow hdf5 1.8 readers to read from hdf5 1.10 writers,
    // we want to pin to hdf5 1.8 features for now.
    // There isn't a way to select 1.8,
    // https://forum.hdfgroup.org/t/seconding-the-request-for-h5pset-libver-bounds-1-8-x-file-compat-option/4056
    // so only enable H5F_LIBVER_LATEST if we are using hdf5 1.8

    if( (major_num == 1 && minor_num >= 8) ||
         major_num > 1 )
    {
#if H5_VERSION_GE(1, 10, 2)
        if(HDF5Options::libver == "default" ||
           HDF5Options::libver == "v108")
        {
            h5_status = H5Pset_libver_bounds(h5_fa_props, H5F_LIBVER_V18, H5F_LIBVER_V18);
        }
        else if(HDF5Options::libver == "v1110")
        {
            h5_status = H5Pset_libver_bounds(h5_fa_props, H5F_LIBVER_V18, H5F_LIBVER_V110);
        }
// nested case for hdf5 >= 1.12
#if H5_VERSION_GE(1, 12, 0)
        else if(HDF5Options::libver == "v1120")
        {
            h5_status = H5Pset_libver_bounds(h5_fa_props, H5F_LIBVER_V18, H5F_LIBVER_V112);
        }
#endif
// nested case for hdf5 >= 1.14
#if H5_VERSION_GE(1, 14, 0)
        else if(HDF5Options::libver == "v1140")
        {
            h5_status = H5Pset_libver_bounds(h5_fa_props, H5F_LIBVER_V18, H5F_LIBVER_V114);
        }
#endif
// nested case for hdf5 >= 2.0
#if H5_VERSION_GE(2, 0, 0)
        else if(HDF5Options::libver == "v200")
        {
            h5_status = H5Pset_libver_bounds(h5_fa_props, H5F_LIBVER_V200, H5F_LIBVER_V200);
        }
#endif
        else if(HDF5Options::libver == "latest")
        {
            h5_status = H5Pset_libver_bounds(h5_fa_props, H5F_LIBVER_V18, H5F_LIBVER_LATEST);
        }
        else if( HDF5Options::libver == "none" )
        {
            // no op
        }
        else
        {
            // unknown or unsupported libver
            CONDUIT_ERROR("HDF5 libver option: '"
                          << HDF5Options::libver
                          << "' is unknown or unsupported with HDF5 v"
                          << major_num << "." << major_num << "." << release_num
                          );
        }
#else
        // for 1.8, we map def, v108, and latest to H5F_LIBVER_LATEST
        if(HDF5Options::libver == "default" ||
           HDF5Options::libver == "v108" ||
           HDF5Options::libver == "latest")
        {
            h5_status = H5Pset_libver_bounds(h5_fa_props, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
        }
        else if( HDF5Options::libver == "none" )
        {
            // no op
        }
        else
        {
            // unknown or unsupported libver
            CONDUIT_ERROR("HDF5 libver option: '"
                          << HDF5Options::libver
                          << "' is unknown or unsupported with HDF5 v"
                          << major_num << "." << major_num << "." << release_num
                          );
        }
#endif

        CONDUIT_CHECK_HDF5_ERROR(h5_status,
                                 "Failed to set libver options for "
                                 << "property list " << h5_fa_props);

    }
    return h5_fa_props;
}

//---------------------------------------------------------------------------//
hid_t
create_hdf5_file_create_plist()
{
    // create property list and set it to preserve creation order
    hid_t h5_fc_props = H5Pcreate(H5P_FILE_CREATE);

    CONDUIT_CHECK_HDF5_ERROR(h5_fc_props,
                             "Failed to create H5P_FILE_CREATE "
                             << " property list");

    herr_t h5_status = H5Pset_link_creation_order(h5_fc_props,
            ( H5P_CRT_ORDER_TRACKED |  H5P_CRT_ORDER_INDEXED) );

    CONDUIT_CHECK_HDF5_ERROR(h5_status,
                             "Failed to set creation order options for "
                             << "property list " << h5_fc_props);
    return h5_fc_props;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// Public interface: Write Methods
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
hid_t
hdf5_create_file(const std::string &file_path)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    RelayH5PHandle h5_fc_plist_hnd(create_hdf5_file_create_plist(),file_path);
    h5_fc_plist_hnd.check_created();
    RelayH5PHandle h5_fa_plist_hnd(create_hdf5_file_access_plist(),file_path);
    h5_fa_plist_hnd.check_created();

    // open the hdf5 file for writing
    hid_t h5_file_id = H5Fcreate(file_path.c_str(),
                                 H5F_ACC_TRUNC,
                                 h5_fc_plist_hnd.id(),
                                 h5_fa_plist_hnd.id());

    CONDUIT_CHECK_HDF5_ERROR(h5_file_id,
                             "Error opening HDF5 file for writing: "
                             << file_path);

    return h5_file_id;

    // auto cleanup of h5_fc_plist_hnd and h5_fa_plist_hnd
    // enable hdf5 error stack
}

//-----------------------------------------------------------------------------
void
hdf5_close_file(hid_t hdf5_id)
{
    // close the hdf5 file
    CONDUIT_CHECK_HDF5_ERROR(H5Fclose(hdf5_id),
                             "Error closing HDF5 file handle: " << hdf5_id);
}


//-----------------------------------------------------------------------------
void
hdf5_flush_file(hid_t hdf5_id)
{
    // flush the hdf5 file
    CONDUIT_CHECK_HDF5_ERROR(H5Fflush(hdf5_id,H5F_SCOPE_LOCAL),
                             "Error flushing HDF5 file handle: " << hdf5_id);
}


//---------------------------------------------------------------------------//
void
hdf5_write(const Node &node,
           hid_t hdf5_id,
           const std::string &hdf5_path)
{
    Node opts;
    hdf5_write(node,hdf5_id,hdf5_path,opts);
}



//---------------------------------------------------------------------------//
void
hdf5_write(const Node &node,
           hid_t &hdf5_id,
           const std::string &hdf5_path,
           const Node &opts)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    // TODO: we only want to support abs paths if hdf5_id is a file
    // if ( (not hdf5 file) &&
    //      (hdf5_path.size() > 0) &&
    //      (hdf5_path[0] == "/") )
    //{
    //    CONDUIT_ERROR("HDF5 id must represent a file to use a HDF5 "
    ///                 "absolute path (a path starts with '/'");
    //}

    // after this check strip leading forward and trailing slashes
    // if the exist

    size_t pos = 0;
    size_t len = hdf5_path.size();

    if( (hdf5_path.size() > 0) &&
        (hdf5_path[0] == '/' ) )
    {
        pos = 1;
        len--;
    }

    // only trim right side if we are sure there is more than one char
    // (avoid "/" case, which would already have been trimmed )
    if( (hdf5_path.size() > 1 ) &&
        (hdf5_path[hdf5_path.size()-1] == '/') )
    {
        len--;
    }

    std::string path = hdf5_path.substr(pos,len);

    // TODO: Creating the external tree is inefficient but the compatibility
    // checks and write methods handle node paths easily handle this case.
    // revisit if this is too slow

    Node n;
    if(path.size() > 0)
    {
        // strong dose of evil casting, but it's ok b/c we are grownups here?
        // time we will tell ...
        n.fetch(path).set_external(const_cast<Node&>(node));
    }
    else
    {
        // strong dose of evil casting, but it's ok b/c we are grownups here?
        // time we will tell ...
        n.set_external(const_cast<Node&>(node));
    }

    std::string incompat_details;
    // check compat
    if(check_if_conduit_node_is_compatible_with_hdf5_tree(n,
                                                          "",
                                                          hdf5_id,
                                                          opts,
                                                          incompat_details))
    {
        // write if we are compat
        write_conduit_node_to_hdf5_tree(n,"",hdf5_id,opts);
    }
    else
    {
        std::string hdf5_error_ref_path;
        hdf5_ref_path_with_filename(hdf5_id,
                                    hdf5_path,
                                    hdf5_error_ref_path);

        CONDUIT_ERROR("Failed to write node to "
                      << "\"" << hdf5_error_ref_path << "\", "
                      << "existing HDF5 tree is "
                      << "incompatible with the Conduit Node."
                      << "\nDetails:\n" << incompat_details);
    }

    // restore hdf5 error stack
}



//---------------------------------------------------------------------------//
void
hdf5_write(const Node &node,
           hid_t hdf5_id)
{
    Node opts;
    hdf5_write(node,hdf5_id,opts);
}



//---------------------------------------------------------------------------//
void
hdf5_write(const Node &node,
           hid_t &hdf5_id,
           const Node &opts)
{
    // disable hdf5 error stack
    // TODO: we may only need to use this in an outer level variant
    // of check_if_conduit_node_is_compatible_with_hdf5_tree
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    std::string incompat_details;

    // check compat
    if(check_if_conduit_node_is_compatible_with_hdf5_tree(node,
                                                          "",
                                                          hdf5_id,
                                                          opts,
                                                          incompat_details))
    {
        // write if we are compat
        write_conduit_node_to_hdf5_tree(node,
                                        "",
                                        hdf5_id,
                                        opts);
    }
    else
    {
        std::string hdf5_fname;
        hdf5_filename_from_hdf5_obj_id(hdf5_id, hdf5_fname);

        CONDUIT_ERROR("Failed to write node to "
                      << "\"" << hdf5_fname << "\", "
                      << "existing HDF5 tree is "
                      << "incompatible with the Conduit Node."
                      << " Details: " << incompat_details);
    }
    // restore hdf5 error stack
}


//---------------------------------------------------------------------------//
void
hdf5_save(const Node &node,
          const std::string &path)
{
    Node opts;
    hdf5_write(node,path,opts,false);
}


//---------------------------------------------------------------------------//
void
hdf5_save(const Node &node,
          const std::string &path,
          const Node &opts)
{
    hdf5_write(node,path,opts,false);
}

//---------------------------------------------------------------------------//
void
hdf5_save(const Node &node,
          const std::string &file_path,
          const std::string &hdf5_path)
{
    Node opts;
    hdf5_write(node,file_path,hdf5_path,opts,false);
}


//---------------------------------------------------------------------------//
void
hdf5_save(const Node &node,
          const std::string &file_path,
          const std::string &hdf5_path,
          const Node &opts)
{
    hdf5_write(node,file_path,hdf5_path,opts,false);
}


//---------------------------------------------------------------------------//
void
hdf5_append(const Node &node,
            const std::string &path)
{
    Node opts;
    hdf5_write(node,path,opts,true);
}


//---------------------------------------------------------------------------//
void
hdf5_append(const Node &node,
            const std::string &path,
            const Node &opts)
{
    hdf5_write(node,path,opts,true);
}

//---------------------------------------------------------------------------//
void
hdf5_append(const Node &node,
            const std::string &file_path,
            const std::string &hdf5_path)
{
    Node opts;
    hdf5_write(node,file_path,hdf5_path,opts,true);
}

//---------------------------------------------------------------------------//
void
hdf5_append(const Node &node,
            const std::string &file_path,
            const std::string &hdf5_path,
            const Node &opts)
{
    hdf5_write(node,file_path,hdf5_path,opts,true);
}


//---------------------------------------------------------------------------//
void
hdf5_write(const  Node &node,
           const std::string &path,
           bool append)
{
    Node opts;
    hdf5_write(node,path,opts,append);
}


//---------------------------------------------------------------------------//
void
hdf5_write(const  Node &node,
           const std::string &path,
           const Node &opts,
           bool append)
{
    // check for ":" split
    std::string file_path;
    std::string hdf5_path;

    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    hdf5_path);

    // we will write to the root if no hdf5_path is given.
    // this should be fine for OBJECT_T, not sure about others ...
    if(hdf5_path.size() == 0)
    {
        hdf5_path = "/";
    }

    hdf5_write(node,
               file_path,
               hdf5_path,
               opts,
               append);
}


//---------------------------------------------------------------------------//
void
hdf5_write(const Node &node,
           const std::string &file_path,
           const std::string &hdf5_path,
           bool append)
{
    Node opts;
    hdf5_write(node,file_path,hdf5_path,opts,append);
}


//---------------------------------------------------------------------------//
void
hdf5_write(const Node &node,
           const std::string &file_path,
           const std::string &hdf5_path,
           const Node &opts,
           bool append)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    // file handle for auto cleanup
    RelayH5FHandle h5_file_hnd(-1,file_path);

    if(append && utils::is_file(file_path))
    {
        // open existing hdf5 file for read + write
        h5_file_hnd.set_id(hdf5_open_file_for_read_write(file_path));
    }
    else // trunc
    {
        // open the hdf5 file for writing
        h5_file_hnd.set_id(hdf5_create_file(file_path));
    }

    h5_file_hnd.check_created();

    hid_t h5_file_id = h5_file_hnd.id();

    hdf5_write(node,
               h5_file_id,
               hdf5_path,
               opts);

    // note: h5_file_id wont change in this case,
    //       but general write api supports the id changing
    //       still execute the motions
    h5_file_hnd.set_id(h5_file_id);

    // auto cleanup of h5_file_hnd
    // restore hdf5 error stack
}


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// Public interface: Read Methods
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
hid_t
hdf5_open_file_for_read(const std::string &file_path)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    RelayH5PHandle h5_fa_plist_hnd(create_hdf5_file_access_plist(),file_path);

    // open the hdf5 file for reading
    hid_t h5_file_id = H5Fopen(file_path.c_str(),
                               H5F_ACC_RDONLY,
                               h5_fa_plist_hnd.id());

    CONDUIT_CHECK_HDF5_ERROR(h5_file_id,
                             "Error opening HDF5 file for read only access: "
                              << file_path);

    return h5_file_id;
    // auto cleanup of h5_fa_plist_hnd
    // restore hdf5 error stack
}

//---------------------------------------------------------------------------//
hid_t
hdf5_open_file_for_read_write(const std::string &file_path)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    RelayH5PHandle h5_fa_plist_hnd(create_hdf5_file_access_plist(),file_path);

    // open the hdf5 file for read + write
    hid_t h5_file_id = H5Fopen(file_path.c_str(),
                               H5F_ACC_RDWR,
                               h5_fa_plist_hnd.id());

    CONDUIT_CHECK_HDF5_ERROR(h5_file_id,
                             "Error opening HDF5 file for read + write access: "
                              << file_path);

    return h5_file_id;
    // auto cleanup of h5_fa_plist_hnd
    // restore hdf5 error stack
}


//---------------------------------------------------------------------------//
void
hdf5_read(hid_t hdf5_id,
          const std::string &hdf5_path,
          Node &dest)
{
    Node opts;
    hdf5_read(hdf5_id,hdf5_path,opts,dest);
}

//---------------------------------------------------------------------------//
void
hdf5_read(hid_t hdf5_id,
          const std::string &hdf5_path,
          const Node &opts,
          Node &dest)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;


    // get hdf5 object at path, then call read_hdf5_tree_into_conduit_node
    RelayH5OHandle h5_child_obj_hnd(H5Oopen(hdf5_id,
                                            hdf5_path.c_str(),
                                            H5P_DEFAULT),
                                    hdf5_id,
                                    hdf5_path);
    // custom check for better context
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_child_obj_hnd.id(),
                                                    hdf5_id,
                                                    hdf5_path,
                            "Failed to fetch HDF5 object from: "
                             << hdf5_id << ":" << hdf5_path);

    read_hdf5_tree_into_conduit_node(h5_child_obj_hnd.id(),
                                     hdf5_path,
                                     false,
                                     opts,
                                     dest);

    // auto cleanup of h5_child_obj_hnd
    // restore hdf5 error stack
}

//---------------------------------------------------------------------------//
void
hdf5_read(const std::string &file_path,
          const std::string &hdf5_path,
          Node &node)
{
    Node opts;
    hdf5_read(file_path,hdf5_path,opts,node);
}

//---------------------------------------------------------------------------//
void
hdf5_read(const std::string &file_path,
          const std::string &hdf5_path,
          const Node &opts,
          Node &node)
{
    // note: hdf5 error stack is suppressed in these calls

    // open the hdf5 file for reading
    RelayH5FHandle h5_file_hnd(hdf5_open_file_for_read(file_path),
                               file_path);

    hdf5_read(h5_file_hnd.id(),
              hdf5_path,
              opts,
              node);

    // auto cleanup of h5_file_hnd
}

//---------------------------------------------------------------------------//
void
hdf5_read(const std::string &path,
          Node &node)
{
    Node opts;
    hdf5_read(path,opts,node);
}

//---------------------------------------------------------------------------//
void
hdf5_read(const std::string &path,
          const Node &opts,
          Node &node)
{
    // check for ":" split
    std::string file_path;
    std::string hdf5_path;

    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    hdf5_path);

    // We will read the root if no hdf5_path is given.
    if(hdf5_path.size() == 0)
    {
        hdf5_path = "/";
    }

    // note: hdf5 error stack is suppressed in this call
    hdf5_read(file_path,
              hdf5_path,
              opts,
              node);
}


//---------------------------------------------------------------------------//
void
hdf5_read(hid_t hdf5_id,
          Node &dest)
{
    Node opts;
    hdf5_read(hdf5_id,opts,dest);
}


//---------------------------------------------------------------------------//
void
hdf5_read(hid_t hdf5_id,
          const Node &opts,
          Node &dest)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    read_hdf5_tree_into_conduit_node(hdf5_id,
                                     "",
                                     false,
                                     opts,
                                     dest);

    // restore hdf5 error stack
}


//---------------------------------------------------------------------------//
void
hdf5_read_info(hid_t hdf5_id,
          const std::string &hdf5_path,
          Node &dest)
{
    Node opts;
    hdf5_read_info(hdf5_id,hdf5_path,opts,dest);
}

//---------------------------------------------------------------------------//
void
hdf5_read_info(hid_t hdf5_id,
          const std::string &hdf5_path,
          const Node &opts,
          Node &dest)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    // get hdf5 object at path, then call read_hdf5_tree_into_conduit_node
    RelayH5OHandle h5_child_obj_hnd(H5Oopen(hdf5_id,
                                            hdf5_path.c_str(),
                                            H5P_DEFAULT),
                                    hdf5_id,
                                    hdf5_path);
    // custom check for better context
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_child_obj_hnd.id(),
                                                    hdf5_id,
                                                    hdf5_path,
                            "Failed to fetch HDF5 object from: "
                             << hdf5_id << ":" << hdf5_path);

    read_hdf5_tree_into_conduit_node(h5_child_obj_hnd.id(),
                                     hdf5_path,
                                     true,
                                     opts,
                                     dest);

    // auto cleanup of h5_child_obj_hnd
    // restore hdf5 error stack
}

//---------------------------------------------------------------------------//
void
hdf5_read_info(const std::string &file_path,
          const std::string &hdf5_path,
          Node &node)
{
    Node opts;
    hdf5_read_info(file_path,hdf5_path,opts,node);
}

//---------------------------------------------------------------------------//
void
hdf5_read_info(const std::string &file_path,
          const std::string &hdf5_path,
          const Node &opts,
          Node &node)
{
    // note: hdf5 error stack is suppressed in these calls

    // open the hdf5 file for reading
    RelayH5FHandle h5_file_hnd(hdf5_open_file_for_read(file_path),
                               file_path);
    h5_file_hnd.check_created();

    hdf5_read_info(h5_file_hnd.id(),
              hdf5_path,
              opts,
              node);

    // auto cleanup of h5_file_hnd
    // close the hdf5 file
}

//---------------------------------------------------------------------------//
void
hdf5_read_info(const std::string &path,
          Node &node)
{
    Node opts;
    hdf5_read_info(path,opts,node);
}

//---------------------------------------------------------------------------//
void
hdf5_read_info(const std::string &path,
          const Node &opts,
          Node &node)
{
    // check for ":" split
    std::string file_path;
    std::string hdf5_path;

    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    hdf5_path);

    // We will read the root if no hdf5_path is given.
    if(hdf5_path.size() == 0)
    {
        hdf5_path = "/";
    }

    // note: hdf5 error stack is suppressed in this call
    hdf5_read_info(file_path,
              hdf5_path,
              opts,
              node);
}


//---------------------------------------------------------------------------//
void
hdf5_read_info(hid_t hdf5_id,
          Node &dest)
{
    Node opts;
    hdf5_read_info(hdf5_id,opts,dest);
}


//---------------------------------------------------------------------------//
void
hdf5_read_info(hid_t hdf5_id,
          const Node &opts,
          Node &dest)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    read_hdf5_tree_into_conduit_node(hdf5_id,
                                     "",
                                     true,
                                     opts,
                                     dest);

    // restore hdf5 error stack
}


//---------------------------------------------------------------------------//
bool
hdf5_has_path(hid_t hdf5_id,
              const std::string &hdf5_path)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    int res = H5Lexists(hdf5_id,hdf5_path.c_str(),H5P_DEFAULT);


    // H5Lexists returns:
    //  a positive value if the link exists.
    //
    //  0 if it doesn't exist
    //
    //  a negative # in some cases when it doesn't exist, and in some cases
    //    where there is an error.
    // For our cases, we treat 0 and negative as does not exist.

    return (res > 0);
    // restore hdf5 error stack
}

//---------------------------------------------------------------------------//
void
hdf5_remove_path(hid_t hdf5_id,
                 const std::string &hdf5_path)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(H5Ldelete(hdf5_id,
                                                              hdf5_path.c_str(),
                                                              H5P_DEFAULT),
                                                    hdf5_id,
                                                    hdf5_path,
                             "Error deleting HDF5 path: "
                              << hdf5_id << ":" << hdf5_path);

    // restore hdf5 error stack
}




//---------------------------------------------------------------------------//
bool
is_hdf5_file(const std::string &file_path)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    bool res = false;
    // open the file for read to check if it is valid hdf5
    //
    // don't use H5F_ACC_RDWR, b/c if we already have a file handle open
    // that is RDONLY, the open will fail
    //
    // use H5F_ACC_RDONLY b/c it will work with open file handles
    hid_t h5_file_id = H5Fopen(file_path.c_str(),
                               H5F_ACC_RDONLY,
                               H5P_DEFAULT);

    if( h5_file_id >= 0)
    {
        res = true;
        H5Fclose(h5_file_id);
    }

    return res;
    // restore hdf5 error stack
}

//---------------------------------------------------------------------------//
void hdf5_group_list_child_names(hid_t hdf5_id,
                                 const std::string &hdf5_path,
                                 std::vector<std::string> &res)
{
    // disable hdf5 error stack
    HDF5ErrorStackSuppressor supress_hdf5_errors;

    res.clear();
    // first, hdf5_id + path must be a group in order to have children

    H5O_info_t h5_info_buf;


#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
    herr_t h5_status = H5Oget_info_by_name(hdf5_id,
                                           hdf5_path.c_str(),
                                           &h5_info_buf,
                                           H5O_INFO_ALL,
                                           H5P_DEFAULT);
#else
    herr_t h5_status = H5Oget_info_by_name(hdf5_id,
                                           hdf5_path.c_str(),
                                           &h5_info_buf,
                                           H5P_DEFAULT);
#endif

    // Get type of the object

    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_status,
                                                    hdf5_id,
                                                    "",
                                           "Error fetching HDF5 Object info: "
                                           << " parent: " << hdf5_id
                                           << " path:"    << hdf5_path) ;

    if( h5_info_buf.type != H5O_TYPE_GROUP )
    {
        //  not a group, child names will be empty
        // we could also choose to throw an error in the future
        return;
    }

    // we have a group
    // we don't care about links in this case, we want
    // the child names regardless, so we don't have to use H5Literate
    //
    // we can use H5Lget_name_by_idx, as demoed in
    // https://support.hdfgroup.org/ftp/HDF5/examples/examples-by-api/hdf5-examples/1_10/C/H5G/h5ex_g_corder.c
    //

    RelayH5GHandle h5_group_hnd(H5Gopen(hdf5_id,
                                        hdf5_path.c_str(),
                                        H5P_DEFAULT),
                                hdf5_id,
                                hdf5_path);

    // custom check for better context
    CONDUIT_CHECK_HDF5_ERROR_WITH_FILE_AND_REF_PATH(h5_group_hnd.id(),
                                                    hdf5_id,
                                           "",
                                           "Error opening HDF5 "
                                           << "Group: "
                                           << " parent: "
                                           << hdf5_id
                                           << " path:"
                                           << hdf5_path);

    // get group info so we can find the # of children
    H5G_info_t  h5_group_info;
    h5_status = H5Gget_info(h5_group_hnd.id(), &h5_group_info);

    // buffer for child names, if the names are bigger than this
    // buffer can hold, we will fall back to a malloc
    char name_buff[512];

    for (hsize_t i=0; i < h5_group_info.nlinks; i++)
    {
        char *name_buff_ptr = name_buff;
        char *name_buff_tmp = NULL;
        // Get size of name,
        int name_size = H5Lget_name_by_idx(h5_group_hnd.id(), ".",
                                           H5_INDEX_CRT_ORDER,
                                           H5_ITER_INC,
                                           i,
                                           NULL,
                                           0,
                                           H5P_DEFAULT);

        if(name_size <= 0)
        {
            // error, not valid
            std::string hdf5_err_ref_path;
            hdf5_ref_path_with_filename(h5_group_hnd.id(),
                                        hdf5_path,
                                        hdf5_err_ref_path);

            CONDUIT_HDF5_ERROR(hdf5_err_ref_path,
                               "Failed to fetch group child name at index " << i);
        }
        // now that we know the name exists, add 1 for null terminator.
        name_size++;

        if(name_size > 512)
        {
            // we dont have room with our built in buffer
            // fall back to malloc
            name_buff_tmp = (char*)malloc(sizeof(char)*name_size);
            name_buff_ptr = name_buff_tmp;
        }

        name_size = H5Lget_name_by_idx(h5_group_hnd.id(), ".",
                                       H5_INDEX_CRT_ORDER,
                                       H5_ITER_INC,
                                       i,
                                       name_buff_ptr,
                                       name_size,
                                       H5P_DEFAULT);

        res.push_back(std::string(name_buff_ptr));

        if(name_buff_tmp)
        {
            free(name_buff_tmp);
            name_buff_tmp = NULL;
        }
   }

   // auto cleanup of h5_group_hnd
   // restore hdf5 error stack
}
//-----------------------------------------------------------------------------
void
hdf5_identifier_report(Node &out)
{
    hdf5_identifier_report(H5F_OBJ_ALL,out);
}

//-----------------------------------------------------------------------------
void
hdf5_identifier_report(hid_t hdf5_id, Node &out)
{
    out.reset();
    int h5_num_objs = H5Fget_obj_count(hdf5_id, H5F_OBJ_ALL);

    if(h5_num_objs > 0)
    {
        // get ids
        std::vector<hid_t> h5_obj_ids(h5_num_objs);
        H5Fget_obj_ids(H5F_OBJ_ALL,
                       H5F_OBJ_ALL,
                       h5_num_objs,
                       h5_obj_ids.data());

        // everything should be named
        int unnamed_counter = 0;
        // store name to id map in conduit node
        for (int i = 0; i < h5_num_objs; i++)
        {
            hid_t h5_obj_id = h5_obj_ids[i];
            // get size of the name first
            std::string name = "";
            ssize_t obj_name_size = H5Iget_name(h5_obj_id, NULL,4096);
            if(obj_name_size > 0)
            {
                std::vector<char> name_buff(obj_name_size + 1);
                H5Iget_name(h5_obj_id, name_buff.data(), obj_name_size+1);
                name = std::string(name_buff.data());
            }

            if(name == "")
            {
                name = conduit_fmt::format("_unnamed_{:d}",unnamed_counter);
                unnamed_counter++;
            }

            Node &ent =  out.add_child(name);

            // is valid
            htri_t h5_is_valid = H5Iis_valid(h5_obj_id);
            if(h5_is_valid > 0)
            {
                ent["valid"] = "true";
            }
            else if(h5_is_valid == 0)
            {
                ent["valid"] = "false";
            }
            else
            {
                ent["valid"] = "unknown";
            }

            // provide ref count
            ent["ref_count"] = H5Iget_ref(h5_obj_id);

            // provide type info
            H5I_type_t h5_obj_type = H5Iget_type(h5_obj_id);

            ent["id"] = h5_obj_ids[i];
            switch(h5_obj_type)
            {
                case H5I_UNINIT:
                    ent["type"] = "uninitialized";
                    break;
                case H5I_BADID:
                    ent["type"] = "invalid";
                    break;
                case H5I_FILE:
                    ent["type"] ="file";
                    break;
                case H5I_GROUP:
                    ent["type"] ="group";
                    break;
                case H5I_DATATYPE:
                    ent["type"] ="datatype";
                    break;
                case H5I_DATASPACE:
                    ent["type"] ="dataspace";
                    break;
                case H5I_DATASET:
                    ent["type"] ="dataset";
                    break;
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
                // 1.12 +
                case H5I_MAP:
                    ent["type"] ="map";
                    break;
#endif
                case H5I_ATTR:
                    ent["type"] ="attribute";
                    break;
#if H5_VERSION_LE(1, 10, 99)
                // deprecated -- removed in 1.12
                case H5I_REFERENCE:
                    ent["type"] ="reference";
                    break;
#endif
                case H5I_VFL:
                    ent["type"] ="vfl";
                    break;
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
                // 1.12 +
                case H5I_VOL:
                    ent["type"] ="vol";
                    break;
#endif
                case H5I_GENPROP_CLS:
                    ent["type"] ="genprop_class";
                    break;
                case H5I_GENPROP_LST:
                    ent["type"] ="genprop_list";
                    break;
                case H5I_ERROR_CLASS:
                    ent["type"] ="error_class";
                    break;
                case H5I_ERROR_MSG:
                    ent["type"] ="error_message";
                    break;
                case H5I_ERROR_STACK:
                    ent["type"] ="error_stack";
                    break;
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_18_API)
                // 1.12 +
                case H5I_SPACE_SEL_ITER:
                    ent["type"] ="dataspace_selection_iterator";
                    break;
#endif
                default:
                    ent["type"] ="unknown";
                    break;
            }

        }
    }
}


}
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io --
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
}
//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
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
