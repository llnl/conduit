// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_hdf5_api.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_HDF5_API_HPP
#define CONDUIT_RELAY_HDF5_API_HPP

//-----------------------------------------------------------------------------
/// The CONDUIT_CHECK_HDF5_ERROR macro is used to check error codes from HDF5.
//-----------------------------------------------------------------------------
#define CONDUIT_CHECK_HDF5_ERROR( hdf5_err, msg    )                \
{                                                                   \
    if( hdf5_err < 0 )                                              \
    {                                                               \
        std::ostringstream hdf5_err_oss;                            \
        hdf5_err_oss << "HDF5 Error code"                           \
            <<  hdf5_err                                            \
            << " " << msg;                                          \
        CONDUIT_ERROR( hdf5_err_oss.str());                         \
    }                                                               \
}

//-----------------------------------------------------------------------------
//
/// When writing a node to a HDF5 hierarchy there are two steps:
/// (1) Given a destination path, find (or create) the proper HDF5 parent group
/// (2) Write the node's data into the parent group
///
///
/// 1) To find (or create) the parent group there are two cases to finalize
///    the desired parent's HDF5 path:
///
///  A) If the input node is a leaf type, the last part of the path
///     (the portion after the last slash '/', or the entire path if
///     the path doesn't contain any slashes '/') is reserved for the
///     name of the HDF5 dataset that will be used to store the node's data.
///     The rest of the path will be used to find or create the proper parent
///     group.
///
///  B) If the input node is an Object, the full input path will be used to
///     find or create the proper parent group.
///
///  Given the desired parent path:
///
///  If the desired parent path exists in the HDF5 tree and resolves to
///  a group, that group will be used as the parent.
///
///  If the desired parent path does not exist, or partially exists
///  relay will attempt to create a hierarchy of HDF5 groups to represent it.
///
///  During this process, if any part of the path exists and is not a HDF5
///  group, an error is thrown and nothing will be modified.
///  (This error check happens before anything is modified in HDF5)
///
///
/// 2) For writing the data, there are two cases:
///   A) If the input node is an Object, the children of the node will be
///      written to the parent group. If children correspond to existing
///      HDF5 entires and they are incompatible (e.g. a group vs a dataset)
//       an error is thrown and nothing will be written.
///      (This error check happens before anything is modified in HDF5)
///
///   B) If the input node is a leaf type, the last part of the path will be
///      used as the name for a HDF5 dataset that will hold the node's data.
///      If a child with this name already exists in in the parent group, and
///      it is not compatible (e.g. a group vs a dataset) an error is thrown
///     and nothing will be written.
///
//-----------------------------------------------------------------------------
///  Note: HDF5 I/O is not implemented for Conduit Nodes in the List role.
///        We believe this will require a non-standard HDF5 convention, and
///        we want to focus on the Object and Leaf cases since they are
///        compatible with HDF5's data model.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Create a hdf5 file for read and write using conduit's selected hdf5 plists.
//-----------------------------------------------------------------------------
hid_t CONDUIT_RELAY_API hdf5_create_file(const std::string &file_path);

//-----------------------------------------------------------------------------
/// Close hdf5 file handle
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_close_file(hid_t hdf5_id);

//-----------------------------------------------------------------------------
/// Save node data to a given path.
///
/// Save Semantics: Existing file will be overwritten
/// /// Calls hdf5_write(node,path,opts,false);
///
///
/// This methods supports a file system and hdf5 path, joined using a ":"
///  ex: "/path/on/file/system.hdf5:/path/inside/hdf5/file"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_save(const Node &node,
                                 const std::string &path);

void CONDUIT_RELAY_API hdf5_save(const Node &node,
                                 const std::string &path,
                                 const Node &opts);

//-----------------------------------------------------------------------------
/// Save node data to given file system path and internal hdf5 path
///
/// Save Semantics: Existing file will be overwritten
/// Calls hdf5_write(node,file_path,hdf5_path,opts,false);
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_save(const Node &node,
                                 const std::string &file_path,
                                 const std::string &hdf5_path);

void CONDUIT_RELAY_API hdf5_save(const Node &node,
                                 const std::string &file_path,
                                 const std::string &hdf5_path,
                                 const Node &opts);

//-----------------------------------------------------------------------------
/// Write node data to a given path.
///
/// Append Semantics: Append to existing file
/// Calls hdf5_write(node,path,opts,true);
///
///
/// This methods supports a file system and hdf5 path, joined using a ":"
///  ex: "/path/on/file/system.hdf5:/path/inside/hdf5/file"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_append(const Node &node,
                                   const std::string &path);

void CONDUIT_RELAY_API hdf5_append(const Node &node,
                                   const std::string &path,
                                   const Node &opts);

//-----------------------------------------------------------------------------
/// Write node data to given file system path and internal hdf5 path
///
/// Append Semantics: Append to existing file
/// Calls hdf5_write(node,file_path,hdf5_path,opts,true);
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_append(const Node &node,
                                   const std::string &file_path,
                                   const std::string &hdf5_path);

void CONDUIT_RELAY_API hdf5_append(const Node &node,
                                   const std::string &file_path,
                                   const std::string &hdf5_path,
                                   const Node &opts);


//-----------------------------------------------------------------------------
/// Write node data to a given path in an existing file.
///
/// This methods supports a file system and hdf5 path, joined using a ":"
///  ex: "/path/on/file/system.hdf5:/path/inside/hdf5/file"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  const std::string &path,
                                  bool append=false);

void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  const std::string &path,
                                  const Node &opts,
                                  bool append=false);

//-----------------------------------------------------------------------------
/// Write node data to given file system path and internal hdf5 path
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  const std::string &file_path,
                                  const std::string &hdf5_path,
                                  bool append=false);

void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  const std::string &file_path,
                                  const std::string &hdf5_path,
                                  const Node &opts,
                                  bool append=false);

//-----------------------------------------------------------------------------
/// Write node data to the hdf5_path relative to group represented  by
/// hdf5_id
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  hid_t hdf5_id,
                                  const std::string &hdf5_path);

//
// hdf5_write support with options may require us to convert datasets
// from fixed to extendible. This removes and closes the existing dataset.
// To support this, hdf5_id is passed as a reference and is updated
// as needed.
//
void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  hid_t &hdf5_id,
                                  const std::string &hdf5_path,
                                  const Node &opts);

//-----------------------------------------------------------------------------
/// Write node data to group represented by hdf5_id
///
/// Note: this only works for Conduit Nodes in the Object role.
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  hid_t hdf5_id);

//
// hdf5_write support with options may require us to convert datasets
// from fixed to extendible. This removes and closes the existing dataset.
// To support this, hdf5_id is passed as a reference and is updated
// as needed.
//
void CONDUIT_RELAY_API hdf5_write(const Node &node,
                                  hid_t &hdf5_id,
                                  const Node &opts);


//-----------------------------------------------------------------------------
/// Open a hdf5 file for reading, using conduit's selected hdf5 plists.
//-----------------------------------------------------------------------------
hid_t CONDUIT_RELAY_API hdf5_open_file_for_read(const std::string &file_path);
hid_t CONDUIT_RELAY_API hdf5_open_file_for_read_write(const std::string &file_path);


//-----------------------------------------------------------------------------
//
/// A user can specify options to read a subset or "hyperslab" from an HDF5
/// multidimensional numeric array into a Conduit node.  See
/// https://portal.hdfgroup.org/display/HDF5/Reading+From+or+Writing+To+a+Subset+of+a+Dataset
/// for reference.
/// 
/// Hyperslab Options:
/// - "sizes" (or "size"): number of elements to read
/// - "offsets" (or "offset"): where in the array to start reading
/// - "strides" (or "stride"): distance between elements to read
/// 
/// Conduit doesn't support an option for data blocks.
/// 
/// No hyperslab option is mandatory.  If "offset" is not specified, Conduit
/// starts reading at the first value in the array.  If "stride" is not
/// specified, Conduit reads each value, skipping none.  If "size" is not
/// specified, Conduit reads from the offset to the end of the array.
/// 
/// If the dataset is a one-dimensional array, assign scalar values to
/// opts["size"], opts["offset"], and opts["stride"].  To read a hyperslab
/// from an N-dimensional array, set each desired option to a length-N array.
/// 
/// Please be aware that HDF5 requires its parameter arrays to be ordered like
/// indices to C++ multidimensional arrays.  The first element indexes the
/// slowest-varying dimension, while the last element indexes the fastest-
/// varying dimension.  Thus, to specify a 3D offset of level 2, row 4,
/// column 3, a user could write the following:
/// 
/// \code{c++}
/// int constexpr rank = 3;
/// int p_offset[rank] {2, 4, 3};
/// Node opts;
/// opts["offset"].set_external(p_offset, rank);
/// \endcode
/// 
/// When working with paper and pencil, and in other areas of Conduit, we
/// often use the opposite formulation.  We write (3, 4, 2) to indicate a
/// point at x=3, y=4, z=2.  Exercise care to distinguish HDF5's convention
/// (C++ multidimensional arrays, "fastest varying rightmost") from other
/// conventions of notation.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Read hdf5 data from given path into the output node
///
/// This methods supports a file system and hdf5 path, joined using a ":"
///  ex: "/path/on/file/system.hdf5:/path/inside/hdf5/file"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read(const std::string &path,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read(const std::string &path,
                                 const Node &opts,
                                 Node &node);

//-----------------------------------------------------------------------------
/// Read hdf5 data from given file system path and internal hdf5 path into
/// the output node
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read(const std::string &file_path,
                                 const std::string &hdf5_path,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read(const std::string &file_path,
                                 const std::string &hdf5_path,
                                 const Node &opts,
                                 Node &node);

//-----------------------------------------------------------------------------
/// Read from hdf5 path relative to the hdf5 id into the output node
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read(hid_t hdf5_id,
                                 const std::string &hdf5_path,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read(hid_t hdf5_id,
                                 const std::string &hdf5_path,
                                 const Node &opts,
                                 Node &node);

//-----------------------------------------------------------------------------
/// Read from hdf5 id into the output node
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read(hid_t hdf5_id,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read(hid_t hdf5_id,
                                 const Node &opts,
                                 Node &node);

// here change

//-----------------------------------------------------------------------------
/// Read hdf5 info from given path into the output node
///
/// This methods supports a file system and hdf5 path, joined using a ":"
///  ex: "/path/on/file/system.hdf5:/path/inside/hdf5/file"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read_info(const std::string &path,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read_info(const std::string &path,
                                 const Node &opts,
                                 Node &node);

//-----------------------------------------------------------------------------
/// Read hdf5 info from given file system path and internal hdf5 path into
/// the output node
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read_info(const std::string &file_path,
                                 const std::string &hdf5_path,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read_info(const std::string &file_path,
                                 const std::string &hdf5_path,
                                 const Node &opts,
                                 Node &node);

//-----------------------------------------------------------------------------
/// Read info from hdf5 path relative to the hdf5 id into the output node
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read_info(hid_t hdf5_id,
                                 const std::string &hdf5_path,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read_info(hid_t hdf5_id,
                                 const std::string &hdf5_path,
                                 const Node &opts,
                                 Node &node);

//-----------------------------------------------------------------------------
/// Read info from hdf5 id into the output node
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_read_info(hid_t hdf5_id,
                                 Node &node);

void CONDUIT_RELAY_API hdf5_read_info(hid_t hdf5_id,
                                 const Node &opts,
                                 Node &node);

//-----------------------------------------------------------------------------
/// Helpers for converting between hdf5 dtypes and conduit dtypes
///
///  Throughout the relay hdf5 implementation, we use DataType::Empty when
///  the hdf5 data space is H5S_NULL, regardless of what the hdf5 data type is.
///  That isn't reflected in these helper functions, they handle
///  mapping of endianness and leaf types other than empty.
///
///  conduit_dtype_to_hdf5_dtype uses default HDF5 datatypes except for
///  the string case. String case result needs to be cleaned up with
///  H5Tclose(). You can use conduit_dtype_to_hdf5_dtype_cleanup() to
///  properly cleanup in all cases.
///
///  You also can detect the custom string type case with:
///
///    if(  ! H5Tequal(hdf5_dtype_id, H5T_C_S1) &&
///        ( H5Tget_class(hdf5_dtype_id) == H5T_STRING ) )
///    {
///      // custom string type case
///    }
///
///  Note: In these functions, ref_path is used to provide context about the
///  hdf5 tree when an error occurs. Using it is recommend but not required.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
hid_t CONDUIT_RELAY_API    conduit_dtype_to_hdf5_dtype(const DataType &dt,
                                                const std::string &ref_path="");

//-----------------------------------------------------------------------------
void  CONDUIT_RELAY_API    conduit_dtype_to_hdf5_dtype_cleanup(
                                                hid_t hdf5_dtype_id,
                                                const std::string &ref_path="");

//-----------------------------------------------------------------------------
DataType CONDUIT_RELAY_API hdf5_dtype_to_conduit_dtype(hid_t hdf5_dtype_id,
                                                       index_t num_elems,
                                                const std::string &ref_path="");

//-----------------------------------------------------------------------------
DataType CONDUIT_RELAY_API hdf5_dtype_to_conduit_dtype(hid_t hdf5_dtype_id,
                                                       hsize_t * num_elems_array,
                                                       index_t rank,
                                                const std::string &ref_path="");

//-----------------------------------------------------------------------------
/// Checks if the given path is a valid hdf5 file by opening it.
//-----------------------------------------------------------------------------
bool CONDUIT_RELAY_API is_hdf5_file(const std::string &path);

//-----------------------------------------------------------------------------
/// Returns the names of the children of the path relative to hdf5 id.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_group_list_child_names(hid_t hdf5_id,
                                                 const std::string &path,
                                                 std::vector<std::string> &res);

//-----------------------------------------------------------------------------
/// Check if path exists relative to hdf5 id
//-----------------------------------------------------------------------------
bool CONDUIT_RELAY_API hdf5_has_path(hid_t hdf5_id, const std::string &path);

//-----------------------------------------------------------------------------
/// Remove a hdf5 path, if it exists
///
/// Note: This does not necessarily reclaim the space used, however
/// it does allow you to write new data to this path, avoiding errors
/// related to incompatible groups or data sets.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_remove_path(hid_t hdf5_id, const std::string &path);


//-----------------------------------------------------------------------------
/// Pass a Node to set hdf5 i/o options.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_set_options(const Node &opts);

//-----------------------------------------------------------------------------
/// Get a Node that contains hdf5 i/o options.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_options(Node &opts);

//-----------------------------------------------------------------------------
/// Get a node that describes open hdf5 handles
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API hdf5_identifier_report(Node &out);

void CONDUIT_RELAY_API hdf5_identifier_report(hid_t hdf5_id, Node &out);


#endif
