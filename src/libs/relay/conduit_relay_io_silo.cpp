// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_silo.cpp
///
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    #include "conduit_relay_mpi_io_silo.hpp"
#else
    #include "conduit_relay_io_silo.hpp"
#endif

//-----------------------------------------------------------------------------
// standard lib includes
//-----------------------------------------------------------------------------
#include <iostream>
#include <string.h>
#include <memory>
#include <map>

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_fmt/conduit_fmt.h"

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    #include "conduit_blueprint_mpi.hpp"
    #include "conduit_relay_mpi.hpp"
    #include "conduit_relay_mpi_io_blueprint.hpp"
#else
    #include "conduit_relay_io_blueprint.hpp"
#endif

#include "conduit_relay_io_hdf5.hpp"

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include <silo.h>

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
// Define an argument macro that adds the communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG) ,ARG
#else
// Define an argument macro that does not add the communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG)
#endif

//-----------------------------------------------------------------------------
//
/// The CONDUIT_CHECK_SILO_ERROR macro is used to check error codes from silo.
//-----------------------------------------------------------------------------
#define CONDUIT_CHECK_SILO_ERROR( silo_err, msg )                   \
{                                                                   \
    if( (silo_err) != 0)                                            \
    {                                                               \
        std::ostringstream silo_err_oss;                            \
        silo_err_oss << "Silo Error code "                          \
            << (silo_err) << " " << DBErrString()                   \
            << " " << msg;                                          \
        CONDUIT_ERROR( silo_err_oss.str());                         \
    }                                                               \
}                                                                   \

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


//-----------------------------------------------------------------------------
// Private class used to suppress Silo error messages.
//
// Creating an instance of this class will disable the current Silo error
// callbacks. The default Silo callbacks print error messages during various
// API calls. When the instance is destroyed, the previous error state is
// restored.
//
//-----------------------------------------------------------------------------
class SiloErrorSuppressor
{
public:
        SiloErrorSuppressor()
        :  silo_error_lvl(0),
           silo_error_func(NULL)
        {
            disable_silo_error_func();
        }

       ~SiloErrorSuppressor()
        {
            restore_silo_error_func();
        }

private:
    // saves current error level and func
    // then turns off errors
    void disable_silo_error_func()
    {
        silo_error_lvl = DBErrlvl();
        silo_error_func = DBErrfunc();
        DBShowErrors(DB_NONE,NULL);
    }

    // restores saved error func
    void restore_silo_error_func()
    {
        DBShowErrors(silo_error_lvl,silo_error_func);
    }

    // silo error level
    int                  silo_error_lvl;
    // callback used for silo error interface
    void (*silo_error_func)(char*);
};



//---------------------------------------------------------------------------//
void
silo_write(const Node &node,
           const std::string &path)
{
    // check for ":" split
    std::string file_path;
    std::string silo_obj_base;
    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    silo_obj_base);

    /// If silo_obj_base is empty, we have a problem ...
    CONDUIT_ASSERT(silo_obj_base.size() != 0, "Invalid path for save: " << path);

    silo_write(node,file_path,silo_obj_base);
}

//---------------------------------------------------------------------------//
void
silo_read(const std::string &path,
          Node &node)
{
    // check for ":" split
    std::string file_path;
    std::string silo_obj_base;
    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    silo_obj_base);

    /// If silo_obj_base is empty, we have a problem ...
    CONDUIT_ASSERT(silo_obj_base.size() != 0, "Invalid path for load: " << path);

    silo_read(file_path,silo_obj_base,node);
}


//---------------------------------------------------------------------------//
void silo_write(const Node &node,
                const std::string &file_path,
                const std::string &silo_obj_path)
{
    DBfile *dbfile = DBCreate(file_path.c_str(),
                              DB_CLOBBER,
                              DB_LOCAL,
                              NULL,
                              DB_HDF5);

    CONDUIT_ASSERT(dbfile, "Error opening Silo file for writing: " << file_path);
    silo_write(node,dbfile,silo_obj_path);
    CONDUIT_ASSERT(DBClose(dbfile) == 0, "Error closing Silo file: " << file_path);
}

//---------------------------------------------------------------------------//
void silo_read(const std::string &file_path,
               const std::string &silo_obj_path,
               Node &n)
{
    DBfile *dbfile = silo_open_file_for_read(file_path);

    CONDUIT_ASSERT(dbfile, "Error opening Silo file for reading: " << file_path);
    silo_read(dbfile,silo_obj_path,n);
    CONDUIT_ASSERT(DBClose(dbfile) == 0, "Error closing Silo file: " << file_path);
}


//---------------------------------------------------------------------------//
void silo_write(const  Node &node,
                DBfile *dbfile,
                const std::string &silo_obj_path)
{
    Schema schema_c;
    node.schema().compact_to(schema_c);
    std::string schema = schema_c.to_json();
    int schema_len = schema.length() + 1;

    std::vector<uint8> data;
    node.serialize(data);
    int data_len = data.size();

    // use path to construct dest silo obj paths

    std::string dest_json = silo_obj_path +  "_conduit_json";
    std::string dest_data = silo_obj_path +  "_conduit_bin";

    int silo_error = 0;
    silo_error += DBWrite(dbfile,
                          dest_json.c_str(),
                          schema.c_str(),
                          &schema_len,
                          1,
                          DB_CHAR);
    silo_error += DBWrite(dbfile,
                          dest_data.c_str(),
                          &data[0],
                          &data_len,
                          1,
                          DB_CHAR);

    CONDUIT_CHECK_SILO_ERROR(silo_error,
                             "Error writing conduit Node to Silo file");
}


//---------------------------------------------------------------------------//
void silo_read(DBfile *dbfile,
               const std::string &silo_obj_path,
               Node &node)
{
    std::string src_json = silo_obj_path +  "_conduit_json";
    std::string src_data = silo_obj_path +  "_conduit_bin";

    int schema_len = DBGetVarLength(dbfile, src_json.c_str());
    int data_len   = DBGetVarLength(dbfile, src_data.c_str());

    char *schema = new char[schema_len];
    char *data   = new char[data_len];


    DBReadVar(dbfile, src_json.c_str(), schema);
    DBReadVar(dbfile, src_data.c_str(), data);

    CONDUIT_ASSERT(!(schema == NULL || data == NULL),
        "Error extracting data conduit Node from Silo file");

    Generator node_gen(schema, "conduit_json", data);
    /// gen copy
    node_gen.walk(node);

    delete [] schema;
    delete [] data;
}

//---------------------------------------------------------------------------//
bool
is_silo_file(const std::string &file_path)
{
    return is_silo_file(file_path,"unknown");
}

//---------------------------------------------------------------------------//
bool
is_silo_file(const std::string &file_path, const std::string &silo_driver)
{
    SiloErrorSuppressor ses;

    bool res = false;
    if(silo_driver == "hdf5")
    {
        const std::string hdf5_magic_number = "\211HDF\r\n\032\n";
        char buff[257];
        std::memset(buff,0,257);
        std::ifstream ifs;
        ifs.open(file_path.c_str());
        if(ifs.is_open())
        {
            ifs.read((char *)buff,256);
            int nbytes_read = static_cast<int>(ifs.gcount());
            ifs.close();
            std::string test_str(buff,nbytes_read);
            // check for hdf5 magic number
            if(test_str.find(hdf5_magic_number) != std::string::npos)
            {
                // if hdf5 it could be a silo file or a normal hdf5 file
                // open with hdf5 and look for presence of silo
                // sentinel _silolibinfo
                hid_t h5_file_id = conduit::relay::io::hdf5_open_file_for_read(file_path);

                if(conduit::relay::io::hdf5_has_path(h5_file_id,"_silolibinfo"))
                {
                    res = true;
                }
                // close the hdf5 file
                conduit::relay::io::hdf5_close_file(h5_file_id);
            }
        }
    }
    else if(silo_driver == "pdb")
    {
        DBfile *silo_dbfile = DBOpen(file_path.c_str(), DB_PDB, DB_READ);

        if(silo_dbfile != NULL)
        {
            // we are able to open with silo, if we want to be extra careful
            // we can also ask:
            // if(DBInqVarExists(silo_dbfile, "_silolibinfo"))
            // {
            //     res = true;
            // }
            res = true;
            silo_close_file(silo_dbfile);
        }
    }
    else // try unknown
    {
        DBfile *silo_dbfile = DBOpen(file_path.c_str(), DB_UNKNOWN, DB_READ);

        if(silo_dbfile != NULL)
        {
            // we are able to open with silo, if we want to be extra careful
            // we can also ask:
            // if(DBInqVarExists(silo_dbfile, "_silolibinfo"))
            // {
            //     res = true;
            // }
            res = true;
            silo_close_file(silo_dbfile);
        }
    }

    return res;
}


//---------------------------------------------------------------------------//
DBfile *
silo_open_file_for_read(const std::string &file_path)
{
    // this open cascade is an optimization -- we expect most open cases
    // will need hdf5 driver, DB_UNKNOWN has more logic that is slightly
    // more expensive.

    DBfile *res = DBOpen(file_path.c_str(), DB_HDF5, DB_READ);
    if(res != NULL)
    {
        return res;
    }

    res = DBOpen(file_path.c_str(), DB_PDB, DB_READ);
    if(res != NULL)
    {
        return res;
    }

    res = DBOpen(file_path.c_str(), DB_UNKNOWN, DB_READ);
    if(res != NULL)
    {
        return res;
    }

    return res;
}

//---------------------------------------------------------------------------//
void
silo_close_file(DBfile *silo_handle)
{
    if(silo_handle !=NULL)
    {
        DBClose(silo_handle);
    }
}


//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::io::silo --
//-----------------------------------------------------------------------------
namespace silo
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::io::silo::detail --
//-----------------------------------------------------------------------------
namespace detail
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::io::silo::detail::name_generator_tools --
//-----------------------------------------------------------------------------
namespace name_generator_tools // tools for making `SiloTreePathGenerator`s
{

//-----------------------------------------------------------------------------

class SiloTreePathGenerator
{
private:
    int                      nblocks;
    std::vector<std::string> names_list;
    DBnamescheme            *file_namescheme;
    DBnamescheme            *block_namescheme;
    int                      empty_count;
    const index_t           *empty_list;

public:
    SiloTreePathGenerator(DBfile *dbfile,
                          const int num_domains,
                          const Node &n_item,
                          const std::string &what_kind_of_paths) :
        nblocks(num_domains), file_namescheme(0), block_namescheme(0)
    {
        // fetch the paths from part of the root silo index
        if (n_item.has_child(what_kind_of_paths))
        {
            // Preallocate memory to avoid repeated reallocations
            names_list.reserve(num_domains);
            for (int block_id = 0; block_id < num_domains; block_id ++)
            {
                //                mesh_index   ["mesh_paths"][domain#]
                names_list.push_back(n_item[what_kind_of_paths][block_id].as_string());
            }
        }

        if (n_item.has_path("namescheme/empty_list"))
        {
            empty_count = n_item["namescheme"]["empty_list"].dtype().number_of_elements();
            empty_list = n_item["namescheme"]["empty_list"].as_index_t_ptr();
        }
        else
        {
            empty_count = 0;
            empty_list = nullptr;
        }

        // if we have an empty object
        if (0 == nblocks)
        {
            return;
        }

        // if we have a list of names we are done
        if (0 != names_list.size())
        {
            return;
        }

        // create nameschemes
        if (n_item.has_path("namescheme/file"))
        {
            file_namescheme = 
                DBMakeNamescheme(
                    n_item["namescheme"]["file"].as_char8_str(), 
                    0, 
                    dbfile,
                    ".");
        }
        if (n_item.has_path("namescheme/block"))
        {
            block_namescheme = 
                DBMakeNamescheme(
                    n_item["namescheme"]["block"].as_char8_str(), 
                    0, 
                    dbfile,
                    ".");
        }
    }

    virtual ~SiloTreePathGenerator()
    {
        // clean up namescheme data if we generated it.
        if (nullptr != file_namescheme)
        {
            DBFreeNamescheme(file_namescheme);
        }
        if (nullptr != block_namescheme)
        {
            DBFreeNamescheme(block_namescheme);
        }
    }

    std::string
    Name(int idx) const
    {
        std::string res = "";

        // bounds check
        if (idx < 0 || idx >= nblocks)
        {
            return res;
        }

        // check for simple case
        if (0 < names_list.size())
        {
            return names_list[idx];
        }

        if (nullptr != empty_list)
        {
            int bot = 0;
            int top = empty_count - 1;
            int mid;
            while (bot <= top)
            {
                mid = (bot + top) >> 1;

                if (idx > empty_list[mid])
                {
                    bot = mid + 1;
                }
                else if (idx < empty_list[mid])
                {
                    top = mid - 1;
                }
                else
                {
                    return "EMPTY";
                }
            }
        }

        // namescheme case
        if (nullptr != file_namescheme)
        {
            const char *file_res = DBGetName(file_namescheme, idx);
            if (nullptr != file_res)
            {
                res += (std::string(file_res) + ":");
            }
        }

        if (nullptr != block_namescheme)
        {
            const char *block_res = DBGetName(block_namescheme, idx);
            if (nullptr != block_res)
            {
                res += std::string(block_res);
            }
        }

        return res;
    }
};

//-----------------------------------------------------------------------------
// populates a map of field/matset/specset names to path name generators for them
void
populate_path_gen_map(DBfile *root_file,
                      const int num_domains,
                      const Node &mesh_index,
                      const std::string item, // "vars", "matsets", "specsets"
                      const std::string what_kind_of_paths,
                      std::map<std::string, SiloTreePathGenerator*> &path_gen_map)
{
    if (mesh_index.has_child(item))
    {
        auto item_itr = mesh_index[item].children();
        while (item_itr.has_next())
        {
            const Node &n_item = item_itr.next();
            const std::string item_name = item_itr.name();
            path_gen_map.emplace(item_name,
                                 new SiloTreePathGenerator(root_file,
                                                           num_domains,
                                                           n_item,
                                                           what_kind_of_paths));
        }
    }
}

//-----------------------------------------------------------------------------
void
generate_paths(const std::string &path,
               const std::string &relative_dir,
               std::string &file_path,
               std::string &silo_name)
{
    conduit::utils::rsplit_file_path(path, ":", silo_name, file_path);
    if (silo_name.length() > 1 && silo_name[0] == '/')
    {
        silo_name = silo_name.substr(1);
    }
    if (! file_path.empty())
    {
        file_path = conduit::utils::join_file_path(relative_dir, file_path);
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io::silo::detail::name_generator_tools --
//-----------------------------------------------------------------------------

template <class T, class Deleter>
class SiloObjectWrapper
{
private:
    T *obj;
    Deleter del;

public:
    SiloObjectWrapper(T *o, Deleter d) :
        obj(o), del{d} {}
    T* getSiloObject() { return obj; }
    void setSiloObject(T *o) { obj = o; }
    ~SiloObjectWrapper()
    {
        if (obj)
        {
            del(obj);
        }
    }
};

//-----------------------------------------------------------------------------

template <class T, class Deleter>
class SiloObjectWrapperCheckError
{
private:
    T *obj;
    Deleter del;
    std::string errmsg = "";

public:
    SiloObjectWrapperCheckError(T *o, Deleter d, std::string err) :
        obj(o), del{d}, errmsg{err} {}
    SiloObjectWrapperCheckError(T *o, Deleter d) :
        obj(o), del{d} {}
    T* getSiloObject() { return obj; }
    void setSiloObject(T *o) { obj = o; }
    void setErrMsg(std::string newmsg) { errmsg = newmsg; }
    ~SiloObjectWrapperCheckError()
    {
        CONDUIT_ASSERT(!(obj && del(obj) != 0 && !errmsg.empty()), errmsg);
    }
};

//-----------------------------------------------------------------------------
// silo likes alphanumeric names
bool check_alphanumeric(const std::string &str)
{
    for (const char &ch : str)
    {
        if (!std::isalnum(ch) && ch != '_')
        {
            return false;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// ATTR_INTEGER      0  /* Integer variable */
// ATTR_FLOAT        1  /* Double precision floating point variable */
int silo_type_to_ovl_attr_type(int silo_type)
{
    const int ATTR_INTEGER = 0;
    const int ATTR_FLOAT = 1;

    if (silo_type == DB_FLOAT ||
        silo_type == DB_DOUBLE)
    {
        return ATTR_FLOAT;
    }
    else if (silo_type == DB_NOTYPE)
    {
        return -1;
    }
    else
    {
        return ATTR_INTEGER;
    }
}

//-----------------------------------------------------------------------------
// we don't support string variables (DB_CHAR) or anything else not in this list
bool
check_var_type(int datatype)
{
    return datatype == DB_INT    ||
           datatype == DB_SHORT  ||
           datatype == DB_LONG   ||
           datatype == DB_FLOAT  ||
           datatype == DB_DOUBLE ||
           datatype == DB_LONG_LONG;
}

//-----------------------------------------------------------------------------
int dtype_to_silo_type(DataType dtype)
{
    if (dtype.is_float())
    {
        return DB_FLOAT;
    }
    else if (dtype.is_double())
    {
        return DB_DOUBLE;
    }
    else if (dtype.is_int())
    {
        return DB_INT;
    }
    else if (dtype.is_long())
    {
        return DB_LONG;
    }
    else if (dtype.is_long_long())
    {
        return DB_LONG_LONG;
    }
    else if (dtype.is_char())
    {
        return DB_CHAR;
    }
    else if (dtype.is_short())
    {
        return DB_SHORT;
    }
    return DB_NOTYPE;
}

//-----------------------------------------------------------------------------
std::string
shapetype_to_string(int shapetype)
{
    if (shapetype == DB_ZONETYPE_BEAM)
    {
        return "line";
    }
    else if (shapetype == DB_ZONETYPE_TRIANGLE)
    {
        return "tri";
    }
    else if (shapetype == DB_ZONETYPE_QUAD)
    {
        return "quad";
    }
    else if (shapetype == DB_ZONETYPE_TET)
    {
        return "tet";
    }
    else if (shapetype == DB_ZONETYPE_HEX)
    {
        return "hex";
    }
    else if (shapetype == DB_ZONETYPE_PRISM)
    {
        return "wedge";
    }
    else if (shapetype == DB_ZONETYPE_PYRAMID)
    {
        return "pyramid";
    }
    else if (shapetype == DB_ZONETYPE_POLYHEDRON)
    {
        return "polyhedral";
    }
    else if (shapetype == DB_ZONETYPE_POLYGON)
    {
        return "polygonal";
    }

    CONDUIT_ERROR("Unsupported zone type " << shapetype);
    return "";
}

//---------------------------------------------------------------------------//
template<typename T>
void
silo_wedge_connectivity_to_conduit(Node &n_mesh_conn)
{
    const int conn_size = n_mesh_conn.dtype().number_of_elements();
    T *conn_ptr = n_mesh_conn.value();
    for (int i = 0; i < conn_size; i += 6)
    {
        auto conn0 = conn_ptr[i + 0];
        auto conn2 = conn_ptr[i + 2];
        auto conn4 = conn_ptr[i + 4];
        auto conn5 = conn_ptr[i + 5];
        conn_ptr[i + 0] = conn2;
        conn_ptr[i + 2] = conn5;
        conn_ptr[i + 4] = conn0;
        conn_ptr[i + 5] = conn4;
    }
}

//---------------------------------------------------------------------------//
template<typename T>
void
conduit_wedge_connectivity_to_silo(Node &n_mesh_conn)
{
    const int conn_size = n_mesh_conn.dtype().number_of_elements();
    T *conn_ptr = n_mesh_conn.value();
    for (int i = 0; i < conn_size; i += 6)
    {
        auto conn0 = conn_ptr[i + 0];
        auto conn2 = conn_ptr[i + 2];
        auto conn4 = conn_ptr[i + 4];
        auto conn5 = conn_ptr[i + 5];
        conn_ptr[i + 2] = conn0;
        conn_ptr[i + 5] = conn2;
        conn_ptr[i + 0] = conn4;
        conn_ptr[i + 4] = conn5;
    }
}


//---------------------------------------------------------------------------//
int get_coordset_silo_type(const std::string &sys)
{
    if (sys == "cartesian")
    {
        return DB_CARTESIAN;
    }
    else if (sys == "cylindrical")
    {
        return DB_CYLINDRICAL;
    }
    else if (sys == "spherical")
    {
        return DB_SPHERICAL;
    }
    CONDUIT_ERROR("Unrecognized coordinate system " << sys);
    return -1;
}

//---------------------------------------------------------------------------//
std::vector<const char *>
get_coordset_axis_labels(const int sys)
{
    std::vector<const char *> coordnames;
    if (sys == DB_CARTESIAN)
    {
        coordnames.push_back(conduit::blueprint::mesh::utils::CARTESIAN_AXES[0].c_str());
        coordnames.push_back(conduit::blueprint::mesh::utils::CARTESIAN_AXES[1].c_str());
        coordnames.push_back(conduit::blueprint::mesh::utils::CARTESIAN_AXES[2].c_str());
    }
    else if (sys == DB_CYLINDRICAL)
    {
        coordnames.push_back(conduit::blueprint::mesh::utils::CYLINDRICAL_AXES[0].c_str());
        coordnames.push_back(conduit::blueprint::mesh::utils::CYLINDRICAL_AXES[1].c_str());
        coordnames.push_back(nullptr);
    }
    else if (sys == DB_SPHERICAL)
    {
        coordnames.push_back(conduit::blueprint::mesh::utils::SPHERICAL_AXES[0].c_str());
        coordnames.push_back(conduit::blueprint::mesh::utils::SPHERICAL_AXES[1].c_str());
        coordnames.push_back(conduit::blueprint::mesh::utils::SPHERICAL_AXES[2].c_str());
    }
    else if (sys == DB_NUMERICAL)
    {
        CONDUIT_ERROR("Conduit Blueprint does not support DB_NUMERICAL coordinate systems.");
    }
    else if (sys == DB_OTHER)
    {
        CONDUIT_INFO("Encountered DB_OTHER; defaulting to a cartesian coordinate system.");
        coordnames.push_back(conduit::blueprint::mesh::utils::CARTESIAN_AXES[0].c_str());
        coordnames.push_back(conduit::blueprint::mesh::utils::CARTESIAN_AXES[1].c_str());
        coordnames.push_back(conduit::blueprint::mesh::utils::CARTESIAN_AXES[2].c_str());
    }
    else
    {
        CONDUIT_ERROR("Invalid coordinate system " << sys);
    }
    return coordnames;
}

//-----------------------------------------------------------------------------
// recursively compacts nodes if they are not already compact
void conditional_compact(const Node &n_src,
                         Node &n_dest)
{
    // are we already compact?
    if (n_src.dtype().is_compact())
    {
        n_dest.set_external(n_src);
    }
    else
    {
        if (n_src.dtype().is_object())
        {
            auto val_itr = n_src.children();
            while (val_itr.has_next())
            {
                val_itr.next();
                const std::string label = val_itr.name();
                conditional_compact(n_src[label], n_dest[label]);
            }
        }
        else
        {
            n_src.compact_to(n_dest);
        }
    }
}

//-----------------------------------------------------------------------------
// recursively converts nodes to double arrays if they are not already double
// arrays
// will give you a result that is compact
void convert_to_double_array(const Node &n_src,
                             Node &n_dest)
{
    if (n_src.dtype().is_object())
    {
        auto val_itr = n_src.children();
        while (val_itr.has_next())
        {
            val_itr.next();
            const std::string label = val_itr.name();
            convert_to_double_array(n_src[label], n_dest[label]);
        }
    }
    else
    {
        if (n_src.dtype().is_number())
        {
            // if it's already a double array, we just need to compact it
            if (n_src.dtype().is_double())
            {
                conditional_compact(n_src, n_dest);
            }
            else
            {
                n_src.to_double_array(n_dest);
            }
        }
        else
        {
            n_dest.set_external(n_src);
        }
    }
}

//-----------------------------------------------------------------------------
void convert_to_c_int_array(const Node &n_src,
                            Node &n_dest)
{
    if (n_src.dtype().is_int())
    {
        n_dest.set_external(n_src);
    }
    else
    {
        n_src.to_int_array(n_dest);
    }
}

//-----------------------------------------------------------------------------
bool
check_using_whole_coordset(const int *dims,
                           const int *min_index,
                           const int *max_index,
                           const int ndims)
{
    const bool dim0_ok = (min_index[0] == 0 && max_index[0] == dims[0] - 1);
    const bool dim1_ok = (ndims > 1 ? (min_index[1] == 0 && max_index[1] == dims[1] - 1) : true);
    const bool dim2_ok = (ndims > 2 ? (min_index[2] == 0 && max_index[2] == dims[2] - 1) : true);
    return dim0_ok && dim1_ok && dim2_ok;
};

//-----------------------------------------------------------------------------
void
colmajor_regular_striding(int *strides_out,
                          const int ndims,
                          const std::string &error_msg,
                          const int *silo_strides,
                          const int *silo_dims)
{
    // we can only succeed here if the data is regularly strided
    if (1 == ndims)
    {
        CONDUIT_ASSERT(silo_strides[0] == 1,
                       error_msg);
        strides_out[0] = 1;
    }
    else if (2 == ndims)
    {
        CONDUIT_ASSERT(silo_strides[0] == 1 &&
                       silo_strides[1] == silo_dims[0],
                       error_msg);
        strides_out[0] = silo_dims[1];
        strides_out[1] = 1;
    }
    else // (3 == ndims)
    {
        CONDUIT_ASSERT(silo_strides[0] == 1 &&
                       silo_strides[1] == silo_dims[0] &&
                       silo_strides[2] == silo_dims[0] * silo_dims[1],
                       error_msg);
        strides_out[0] = silo_dims[1] * silo_dims[2];
        strides_out[1] = silo_dims[2];
        strides_out[2] = 1;
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void
copy_point_coords(void *coords[3],
                  const int ndims,
                  const int *dims,
                  const int coord_sys,
                  std::vector<const char *> &labels,
                  Node &coordset_values)
{
    labels = get_coordset_axis_labels(coord_sys);
    // TODO audit all conduit asserts, we want skips not errors
    CONDUIT_ASSERT(!(coord_sys == DB_CYLINDRICAL && ndims >= 3),
        "Blueprint only supports 2D cylindrical coordinates");
    for (int dim_id = 0; dim_id < ndims; dim_id ++)
    {
        if (nullptr != coords[dim_id])
        {
            coordset_values[labels[dim_id]].set(static_cast<T *>(coords[dim_id]), dims[dim_id]);
        }
        else
        {
            return;
        }
    }
}

//-----------------------------------------------------------------------------
void
set_units_or_labels(char *units_or_labels[3],
                    const int ndims,
                    const std::vector<const char *> &labels,
                    Node &coordset,
                    const std::string &units_or_labels_string)
{
    for (int dim_id = 0; dim_id < ndims; dim_id ++)
    {
        if (units_or_labels[dim_id])
        {
            coordset[units_or_labels_string][labels[dim_id]] = units_or_labels[dim_id];
        }
        else
        {
            break;
        }
    }
}

//-----------------------------------------------------------------------------
void
add_sizes_and_offsets(DBzonelist *zones,
                      Node &n_elements)
{
    std::vector<int> sizes;
    std::vector<int> offsets;
    int offset = 0;
    for (int i = 0; i < zones->nshapes; ++i)
    {
        const int shapecnt = zones->shapecnt[i];
        // there could be more than one shape
        for (int j = 0; j < shapecnt; j ++)
        {
            const int size = zones->shapesize[i];
            sizes.push_back(size);
            offsets.push_back(offset);
            offset += size;
        }
    }
    n_elements["sizes"].set(sizes);
    n_elements["offsets"].set(offsets);
}

//-----------------------------------------------------------------------------
void
add_shape_info(DBzonelist *zonelist_ptr,
               Node &n_elements)
{
    // TODO handle min and max index case (check_using_whole_coordset case)

    for (int i = 0; i < zonelist_ptr->nshapes; ++i)
    {
        CONDUIT_ASSERT(zonelist_ptr->shapetype[0] == zonelist_ptr->shapetype[i],
                       "Expected a single shape type, got "
                           << zonelist_ptr->shapetype[0] << " and "
                           << zonelist_ptr->shapetype[i]);
    }

    // TODO you can have a list of different shapetypes, so querying
    // zonelist_ptr->shapetype[0] and guessing that the rest are the same
    // is wrong.

    n_elements["shape"] = shapetype_to_string(zonelist_ptr->shapetype[0]);
    n_elements["connectivity"].set(zonelist_ptr->nodelist, zonelist_ptr->lnodelist);
    if (zonelist_ptr->shapetype[0] == DB_ZONETYPE_PRISM)
    {
        // we must reorder the wedge connectivity b/c conduit uses the
        // vtk ordering, NOT the silo ordering
        DataType dtype = n_elements["connectivity"].dtype();

        // swizzle the connectivity
        if (dtype.is_uint64())
        {
            silo_wedge_connectivity_to_conduit<uint64>(n_elements["connectivity"]);
        }
        else if (dtype.is_uint32())
        {
            silo_wedge_connectivity_to_conduit<uint32>(n_elements["connectivity"]);
        }
        else if (dtype.is_int64())
        {
            silo_wedge_connectivity_to_conduit<int64>(n_elements["connectivity"]);
        }
        else if (dtype.is_int32())
        {
            silo_wedge_connectivity_to_conduit<int32>(n_elements["connectivity"]);
        }
        else
        {
            CONDUIT_ERROR("Unsupported connectivity type in " << dtype.to_yaml());
        }
    }

    // TODO polytopal support
    if (zonelist_ptr->shapetype[0] == DB_ZONETYPE_POLYHEDRON)
    {
        CONDUIT_ERROR("Polyhedra not yet supported");
        // n_elements["sizes"].set(zonelist_ptr->shapesize, zonelist_ptr->nzones);
        // TODO double check this approach
        add_sizes_and_offsets(zonelist_ptr, n_elements["subelements"]);
    }
    if (zonelist_ptr->shapetype[0] == DB_ZONETYPE_POLYGON)
    {
        add_sizes_and_offsets(zonelist_ptr, n_elements);
    }
}

//-----------------------------------------------------------------------------
template <class T>
void
assign_values_helper(int nvals,
                     int nels,
                     void **vals,
                     Node &field_values)
{
    if (nvals == 1)
    {
        field_values.set(static_cast<T *>(vals[0]), nels);
    }
    else
    {
        for (int i = 0; i < nvals; i ++)
        {
            // need to put the values under a vector component
            field_values[std::to_string(i)].set(static_cast<T *>(vals[i]), nels);
        }
    }
}

//-----------------------------------------------------------------------------
void
assign_values(int datatype,
              int nvals,
              int nels,
              void **vals,
              Node &field_out)
{
    if (datatype == DB_INT)
    {
        assign_values_helper<int>(nvals, nels, vals, field_out);
    }
    else if (datatype == DB_SHORT)
    {
        assign_values_helper<short>(nvals, nels, vals, field_out);
    }
    else if (datatype == DB_LONG)
    {
        assign_values_helper<long>(nvals, nels, vals, field_out);
    }
    else if (datatype == DB_FLOAT)
    {
        assign_values_helper<float>(nvals, nels, vals, field_out);
    }
    else if (datatype == DB_DOUBLE)
    {
        assign_values_helper<double>(nvals, nels, vals, field_out);
    }
    else if (datatype == DB_LONG_LONG)
    {
        assign_values_helper<long long>(nvals, nels, vals, field_out);
    }
    else if (datatype == DB_CHAR)
    {
        CONDUIT_ERROR("Variable values cannot be strings.");
    }
    else
    {
        CONDUIT_ERROR("Unsupported type in " << datatype);
    }
}

//-----------------------------------------------------------------------------
// check if mesh types or var types are the same
// we ignore the missing domains
// We are using this function to decide if we can save a single mesh/var type
// or need to save mesh/var types for every domain. If some domains are missing
// it doesn't matter if the rest of them have the same type. When we read, we
// don't use missing mesh/var types to decide if we skip a domain or not, we use 
// the mesh/var name.
bool
all_types_the_same(const int32_accessor &types_acc,
                   const index_t_accessor &status_acc,
                   int32 &first_non_trivial_type)
{
    first_non_trivial_type = -1;
    index_t first_non_trivial_type_index = -1;
    for (index_t global_domain_id = 0;
         global_domain_id < types_acc.number_of_elements();
         global_domain_id ++)
    {
        if (status_acc[global_domain_id] == 1)
        {
            first_non_trivial_type = types_acc[global_domain_id];
            first_non_trivial_type_index = global_domain_id;
            break;
        }
    }

    if (first_non_trivial_type_index == -1)
    {
        return true;
    }

    for (index_t global_domain_id = first_non_trivial_type_index;
         global_domain_id < types_acc.number_of_elements();
         global_domain_id ++)
    {
        if (status_acc[global_domain_id] == 1)
        {
            if (first_non_trivial_type != types_acc[global_domain_id])
            {
                return false;
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
void
generate_silo_mb_data(const Node &n_mesh_state,
                      const std::string &silo_path,
                      const std::string &silo_name,
                      const int num_files,
                      const int global_num_domains,
                      const bool root_only,
                      const Node &dom_flags,
                      const bool do_nameschemes,
                      std::vector<std::string> &name_strings,
                      std::vector<int> *empty_domains)
{
    // this function does the following: 
    //  - generates names for multi-block data paths,
    //  - and tracks the empty domains.
    // we only generate names if we ARE NOT doing nameschemes, and we only
    // track the empty domains if we ARE doing nameschemes.

    // a little helper to determine the domain or file
    auto determine_domain_or_file = [&](const std::string domain_or_file,
                                        const index_t global_domain_id) -> index_t
    {
        if (n_mesh_state.has_path("partition_map/" + domain_or_file))
        {
            index_t_accessor part_map_domain_or_file_vals = n_mesh_state["partition_map"][domain_or_file].value();
            return part_map_domain_or_file_vals[global_domain_id];
        }
        else
        {
            return global_domain_id;
        }
    };

    // if we're doing nameschemes, we don't have to generate names but we do need
    // to track empty domains
    if (do_nameschemes)
    {
        int_accessor domain_flags = dom_flags.value();
        for (index_t global_domain_id = 0; global_domain_id < global_num_domains; global_domain_id ++)
        {
            // determine which domain
            const index_t domain_index = determine_domain_or_file("domain", global_domain_id);

            // we are missing a domain
            if (domain_flags[domain_index] == -1)
            {
                empty_domains->push_back(domain_index);
            }
            // else
        }
    }
    // we need to record per-block object names and do not need to track empty domains
    else
    {
        int_accessor domain_flags = dom_flags.value();
        for (index_t global_domain_id = 0; global_domain_id < global_num_domains; global_domain_id ++)
        {
            // determine which domain
            const index_t domain_index = determine_domain_or_file("domain", global_domain_id);

            // we are missing a domain
            if (domain_flags[domain_index] == -1)
            {
                // we create the silo names
                name_strings.push_back("EMPTY");
            }
            else
            {
                // we create the silo names
                
                std::string generated_silo_name;
                // we have three cases, just as we had in write_mesh
                // we don't want to be making any choices here, just using
                // what was already decided in write_mesh

                // single file case
                if (root_only)
                {
                    if (global_num_domains == 1)
                    {
                        generated_silo_name = conduit_fmt::format(silo_path, silo_name);
                    }
                    else
                    {
                        generated_silo_name = conduit_fmt::format(silo_path, domain_index, silo_name);
                    }
                }
                // num domains == num files case
                else if (global_num_domains == num_files)
                {
                    generated_silo_name = conduit_fmt::format(silo_path, domain_index, silo_name);
                }
                // m to n case
                else
                {
                    // determine which file
                    const index_t f = determine_domain_or_file("file", global_domain_id);
                    generated_silo_name = conduit_fmt::format(silo_path, f, domain_index, silo_name);
                }

                name_strings.push_back(generated_silo_name);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// options Node:
//
// comp_info:
//   comp:                "meshes", "vars", "matsets" or "specsets"
//   comp_name:           meshname, varname, matsetname, or specsetname
// domain_info:
//   local_num_domains:
//   local_domain_index:
//   global_domain_id:
// write_overlink:      "yes" or "no"
// (only one version of the following is included, depending on what the comp is)
// specific_info: // for meshes
//   comp_type:           only used for meshes and vars (not matsets nor species)
// specific_info: // for vars
//   comp_type:           only used for meshes and vars (not matsets nor species)
//   var_data_type:       only used for vars
//   var_parent:          optionally used for vars
// specific_info: // omitted for matsets and specsets
//
void
track_local_type_domain_info(const Node &options,
                             Node &local_type_domain_info)
{
    // fetch the passed in options
    const std::string &comp = options["comp_info"]["comp"].as_string();
    const std::string &comp_name = options["comp_info"]["comp_name"].as_string();
    const index_t local_num_domains = options["domain_info"]["local_num_domains"].to_index_t();
    const index_t local_domain_index = options["domain_info"]["local_domain_index"].to_index_t();
    const index_t global_domain_id = options["domain_info"]["global_domain_id"].to_index_t();
    const bool write_overlink = options["write_overlink"].as_string() == "yes";

    Node &local_type_domain_info_comp = local_type_domain_info[comp];

    if (! local_type_domain_info_comp.has_child(comp_name))
    {
        local_type_domain_info_comp[comp_name]["domain_ids"].set(DataType::index_t(local_num_domains));
        index_t_array domain_ids = local_type_domain_info_comp[comp_name]["domain_ids"].value();
        domain_ids.fill(-1); // we want missing domains to have -1 and not 0 to avoid confusion

        // meshes and vars have type information that must be tracked
        if (comp == "meshes" || comp == "vars")
        {
            local_type_domain_info_comp[comp_name]["types"].set(DataType::index_t(local_num_domains));
        }

        // for overlink, we must save the var data type for each var (int or float)
        // this is used later when writing out the var attributes
        if (write_overlink && comp == "vars")
        {
            const index_t var_data_type = options["specific_info"]["var_data_type"].to_index_t();

            // we only need to do this once since overlink assumes all domains have
            // the same data type.
            local_type_domain_info_comp[comp_name]["ovl_datatype"] = var_data_type;

            if (options["specific_info"].has_child("var_parent"))
            {
                local_type_domain_info_comp[comp_name]["var_parent"] =
                    options["specific_info"]["var_parent"].as_string();
            }
        }
    }
    index_t_array domain_ids = local_type_domain_info_comp[comp_name]["domain_ids"].value();
    domain_ids[local_domain_index] = global_domain_id;
    // for vars and meshes we want to store the var and mesh type, respectively
    if (comp == "meshes" || comp == "vars")
    {
        index_t comp_type = options["specific_info"]["comp_type"].to_index_t();

        index_t_array comp_types = local_type_domain_info_comp[comp_name]["types"].value();
        comp_types[local_domain_index] = comp_type;
    }
}

//-----------------------------------------------------------------------------
void
read_material_map(const Node &material_map,
                  int &nmat,
                  std::vector<std::string> &matnames,
                  std::vector<const char *> &matname_ptrs,
                  std::vector<int> &matnos)
{
    // get the number of materials in this matset to write out
    nmat = material_map.number_of_children();

    // get material names and material numbers and package up char ptrs for silo
    matnames = material_map.child_names();
    for (size_t i = 0; i < matnames.size(); i ++)
    {
        matnos.push_back(material_map[matnames[i]].to_int());
        matname_ptrs.push_back(matnames[i].c_str());
    }
}

//-----------------------------------------------------------------------------
int
read_dims_from_mesh_info(const Node &mesh_info_for_topo, int *dims)
{
    const std::string mesh_type = mesh_info_for_topo["type"].as_string();
    if (mesh_type == "structured" || mesh_type == "rectilinear" || mesh_type == "uniform")
    {
        const int ndims = mesh_info_for_topo["ndims"].as_int();
        dims[0] = mesh_info_for_topo["elements"]["i"].as_int();
        dims[1] = mesh_info_for_topo["elements"]["j"].as_int();
        if (ndims == 3)
        {
            dims[2] = mesh_info_for_topo["elements"]["k"].as_int();
        }
        return ndims;
    }
    else
    {
        dims[0] = mesh_info_for_topo["num_elems"].to_value();
        return 1; // ndims == 1
    }
}

//-----------------------------------------------------------------------------
// pull out logic that is common to all multi-block object writers
// we pass everything in the kitchen sink
// to get either an optlist packed full of namescheme stuff
// or a set of pointers to names that we can pass directly to silo
const char **
handle_nameschemes_or_pathnames(const bool do_nameschemes,
                                const std::vector<std::string> &name_strings,
                                const std::string &global_file_namescheme,
                                const std::string &global_block_namescheme,
                                const std::string &silo_name,
                                const std::string &mbobj_name,
                                const std::string &mbobj_writer_type,
                                const int &num_empty_doms,
                                std::vector<int> &empty_domains,
                                DBoptlist *optlist,
                                std::string &block_namescheme,
                                std::vector<const char *> &name_ptrs)
{
    if (do_nameschemes)
    {
        block_namescheme = conduit_fmt::format(global_block_namescheme, silo_name);

        if (! global_file_namescheme.empty())
        {
            CONDUIT_CHECK_SILO_ERROR(
                DBAddOption(optlist,
                            DBOPT_MB_FILE_NS,
                            const_cast<char *>(global_file_namescheme.c_str())),
                "Error adding file namescheme option for " << mbobj_writer_type << 
                " for " << mbobj_name << ".");
        }

        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(optlist,
                        DBOPT_MB_BLOCK_NS,
                        const_cast<char *>(block_namescheme.c_str())),
            "Error adding block namescheme option for " << mbobj_writer_type << 
            " for " << mbobj_name << ".");
    
        // if we have some domains that are "empty"
        if (! empty_domains.empty())
        {
            // we need it sorted because readers expect it this way
            // e.g. VisIt, Conduit as well but we could change it
            // if we wished.
            std::sort(empty_domains.begin(), empty_domains.end());

            CONDUIT_CHECK_SILO_ERROR(
                DBAddOption(optlist,
                            DBOPT_MB_EMPTY_LIST,
                            empty_domains.data()),
                "Error adding empty list option for " << mbobj_writer_type << 
                " for " << mbobj_name << ".");

            CONDUIT_CHECK_SILO_ERROR(
                DBAddOption(optlist,
                            DBOPT_MB_EMPTY_COUNT,
                            const_cast<void *>(
                                static_cast<const void *>(
                                    &num_empty_doms))),
                "Error adding empty size option for " << mbobj_writer_type << 
                " for " << mbobj_name << ".");
        }
        return nullptr;
    }
    else
    {
        // package up char ptrs for silo
        for (size_t i = 0; i < name_strings.size(); i ++)
        {
            name_ptrs.push_back(name_strings[i].c_str());
        }
        return name_ptrs.data();
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io::silo::detail --
//-----------------------------------------------------------------------------

using SiloNameGenerator = detail::name_generator_tools::SiloTreePathGenerator;

//-----------------------------------------------------------------------------
// add complete topology and coordset entries to a mesh domain
bool
read_ucdmesh_domain(DBucdmesh *ucdmesh_ptr,
                    const std::string &mesh_name,
                    const std::string &multimesh_name,
                    Node &mesh_domain)
{
    Node intermediate_coordset, intermediate_topo;

    if (ucdmesh_ptr->zones)
    {
        if (ucdmesh_ptr->phzones)
        {
            CONDUIT_INFO("Both phzones and zones are defined in mesh " << mesh_name);
            return false;
        }
        detail::add_shape_info(ucdmesh_ptr->zones,
                               intermediate_topo["elements"]);
    }
    else if (ucdmesh_ptr->phzones)
    {
        // TODO implement support for phzones
        CONDUIT_INFO("Silo ucdmesh phzones not yet supported");
        return false;
        intermediate_topo["elements"]["shape"] =
            detail::shapetype_to_string(DB_ZONETYPE_POLYHEDRON);
    }
    else
    {
        CONDUIT_INFO("Neither phzones nor zones is defined in mesh " << mesh_name);
        return false;
    }

    intermediate_topo["coordset"] = multimesh_name;
    intermediate_topo["type"] = "unstructured";
    intermediate_coordset["type"] = "explicit";

    // explicit coords
    const int dims[] = {ucdmesh_ptr->nnodes,
                        ucdmesh_ptr->nnodes,
                        ucdmesh_ptr->nnodes};

    const int ndims = ucdmesh_ptr->ndims;

    if (ucdmesh_ptr->datatype != DB_DOUBLE && ucdmesh_ptr->datatype != DB_FLOAT)
    {
        CONDUIT_INFO("Unsupported mesh data type " << ucdmesh_ptr->datatype);
        return false;
    }

    std::vector<const char *> labels;
    if (ucdmesh_ptr->datatype == DB_DOUBLE)
    {
        detail::copy_point_coords<double>(ucdmesh_ptr->coords,
                                          ndims,
                                          dims,
                                          ucdmesh_ptr->coord_sys,
                                          labels,
                                          intermediate_coordset["values"]);
    }
    else
    {
        // we have guaranteed that this must be float
        detail::copy_point_coords<float>(ucdmesh_ptr->coords,
                                         ndims,
                                         dims,
                                         ucdmesh_ptr->coord_sys,
                                         labels,
                                         intermediate_coordset["values"]);
    }

    detail::set_units_or_labels(ucdmesh_ptr->units, ndims, labels, intermediate_coordset, "units");
    detail::set_units_or_labels(ucdmesh_ptr->labels, ndims, labels, intermediate_coordset, "labels");

    // TODO the following statement should be the gold standard for everything that is
    // read from silo. Make it happen.
    // I don't want to create these entries in the output unless no errors have
    // been encountered. Errors will trigger an early return, not a crash.
    mesh_domain["topologies"][multimesh_name].move(intermediate_topo);
    mesh_domain["coordsets"][multimesh_name].move(intermediate_coordset);

    return true;
}

//-----------------------------------------------------------------------------
// add complete topology and coordset entries to a mesh domain
bool
read_quadmesh_domain(DBquadmesh *quadmesh_ptr,
                     const std::string &multimesh_name,
                     Node &mesh_domain)
{
    Node intermediate_coordset, intermediate_topo;

    const int coordtype{quadmesh_ptr->coordtype};
    int ndims{quadmesh_ptr->ndims};
    int dims[] = {quadmesh_ptr->nnodes,
                  quadmesh_ptr->nnodes,
                  quadmesh_ptr->nnodes};
    int *real_dims = dims;

    if (coordtype == DB_COLLINEAR)
    {
        intermediate_coordset["type"] = "rectilinear";
        intermediate_topo["type"] = "rectilinear";
        real_dims = quadmesh_ptr->dims;

        // TODO these should trigger skips not errors
        CONDUIT_ASSERT(detail::check_using_whole_coordset(quadmesh_ptr->dims,
                                                          quadmesh_ptr->min_index,
                                                          quadmesh_ptr->max_index,
                                                          ndims),
                       "Rectilinear grid (collinear quadmesh) " << multimesh_name <<
                       " is using a subset of the provided coordinates. We do not "
                       "support this case.");

        CONDUIT_ASSERT(quadmesh_ptr->major_order == DB_ROWMAJOR,
                       "Rectilinear grid (collinear quadmesh) " << multimesh_name <<
                       " is column major in silo. We do not support this case.");
    }
    else if (coordtype == DB_NONCOLLINEAR)
    {
        intermediate_coordset["type"] = "explicit";
        intermediate_topo["type"] = "structured";

        const std::string irregular_striding_err_msg = "Structured (noncollinear)"
            " column major quadmesh " + multimesh_name + " has irregular striding,"
            " which makes it impossible to correctly convert to Blueprint.";

        if (detail::check_using_whole_coordset(quadmesh_ptr->dims,
                                               quadmesh_ptr->min_index,
                                               quadmesh_ptr->max_index,
                                               ndims))
        {
            // We subtract 1 from each of these because in silo these dims are node dims, not element dims
            intermediate_topo["elements/dims/i"] = quadmesh_ptr->dims[0] - 1;
            if (ndims > 1)
            {
                intermediate_topo["elements/dims/j"] = quadmesh_ptr->dims[1] - 1;
            }
            if (ndims > 2)
            {
                intermediate_topo["elements/dims/k"] = quadmesh_ptr->dims[2] - 1;
            }

            // row major case requires nothing else
            if (quadmesh_ptr->major_order == DB_COLMAJOR)
            {
                // resort to strided structured
                int strides[] = {0,0,0};
                detail::colmajor_regular_striding(strides,
                                                  ndims,
                                                  irregular_striding_err_msg,
                                                  quadmesh_ptr->stride,
                                                  quadmesh_ptr->dims);
                intermediate_topo["elements/dims/strides"].set(strides, ndims);
            }
        }
        else
        {
            // strided structured case

            intermediate_topo["elements/dims/i"] = quadmesh_ptr->max_index[0] - quadmesh_ptr->min_index[0];
            if (ndims > 1)
            {
                intermediate_topo["elements/dims/j"] = quadmesh_ptr->max_index[1] - quadmesh_ptr->min_index[1];
            }
            if (ndims > 2)
            {
                intermediate_topo["elements/dims/k"] = quadmesh_ptr->max_index[2] - quadmesh_ptr->min_index[2];
            }

            intermediate_topo["elements/dims/offsets"].set(quadmesh_ptr->min_index, ndims);

            if (quadmesh_ptr->major_order == DB_ROWMAJOR)
            {
                intermediate_topo["elements/dims/strides"].set(quadmesh_ptr->stride, ndims);
            }
            else // colmajor
            {
                int actual_strides[] = {0,0,0};
                detail::colmajor_regular_striding(actual_strides,
                                                  ndims,
                                                  irregular_striding_err_msg,
                                                  quadmesh_ptr->stride,
                                                  quadmesh_ptr->dims);
                intermediate_topo["elements/dims/strides"].set(actual_strides, ndims);
            }
        }
    }
    else
    {
        CONDUIT_ERROR("Undefined coordtype in " << coordtype);
    }

    intermediate_topo["coordset"] = multimesh_name;

    // If the origin is not the default value, then we need to specify it
    if (quadmesh_ptr->base_index[0] != 0 &&
        quadmesh_ptr->base_index[1] != 0 &&
        quadmesh_ptr->base_index[2] != 0)
    {
        Node &origin = intermediate_topo["elements"]["origin"];
        origin["i"] = quadmesh_ptr->base_index[0];
        if (ndims > 1)
        {
            origin["i"] = quadmesh_ptr->base_index[1];
        }
        if (ndims > 2)
        {
            origin["i"] = quadmesh_ptr->base_index[2];
        }
    }

    if (quadmesh_ptr->datatype != DB_DOUBLE && quadmesh_ptr->datatype != DB_FLOAT)
    {
        CONDUIT_INFO("Unsupported mesh data type " << quadmesh_ptr->datatype);
        return false;
    }

    std::vector<const char *> labels;
    if (quadmesh_ptr->datatype == DB_DOUBLE)
    {
        detail::copy_point_coords<double>(quadmesh_ptr->coords,
                                          ndims,
                                          real_dims,
                                          quadmesh_ptr->coord_sys,
                                          labels,
                                          intermediate_coordset["values"]);
    }
    else
    {
        // we have guaranteed that this must be float
        detail::copy_point_coords<float>(quadmesh_ptr->coords,
                                         ndims,
                                         real_dims,
                                         quadmesh_ptr->coord_sys,
                                         labels,
                                         intermediate_coordset["values"]);
    }

    detail::set_units_or_labels(quadmesh_ptr->units, ndims, labels, intermediate_coordset, "units");
    detail::set_units_or_labels(quadmesh_ptr->labels, ndims, labels, intermediate_coordset, "labels");

    // I don't want to create these entries in the output unless no errors have
    // been encountered. Errors will trigger an early return, not a crash.
    mesh_domain["topologies"][multimesh_name].move(intermediate_topo);
    mesh_domain["coordsets"][multimesh_name].move(intermediate_coordset);

    return true;
}

//-----------------------------------------------------------------------------
// add complete topology and coordset entries to a mesh domain
bool
read_pointmesh_domain(DBpointmesh *pointmesh_ptr,
                      const std::string &multimesh_name,
                      Node &mesh_domain)
{
    Node intermediate_coordset, intermediate_topo;

    intermediate_topo["type"] = "points";
    intermediate_topo["coordset"] = multimesh_name;
    intermediate_coordset["type"] = "explicit";
    const int dims[] = {pointmesh_ptr->nels,
                        pointmesh_ptr->nels,
                        pointmesh_ptr->nels};

    const int ndims = pointmesh_ptr->ndims;

    if (pointmesh_ptr->datatype != DB_DOUBLE && pointmesh_ptr->datatype != DB_FLOAT)
    {
        CONDUIT_INFO("Unsupported mesh data type " << pointmesh_ptr->datatype);
        return false;
    }

    std::vector<const char *> labels;
    if (pointmesh_ptr->datatype == DB_DOUBLE)
    {
        detail::copy_point_coords<double>(pointmesh_ptr->coords,
                                          ndims,
                                          dims,
                                          DB_CARTESIAN,
                                          labels,
                                          intermediate_coordset["values"]);
    }
    else
    {
        // we have guaranteed that this must be float
        detail::copy_point_coords<float>(pointmesh_ptr->coords,
                                         ndims,
                                         dims,
                                         DB_CARTESIAN,
                                         labels,
                                         intermediate_coordset["values"]);
    }

    detail::set_units_or_labels(pointmesh_ptr->units, ndims, labels, intermediate_coordset, "units");
    detail::set_units_or_labels(pointmesh_ptr->labels, ndims, labels, intermediate_coordset, "labels");

    // I don't want to create these entries in the output unless no errors have
    // been encountered. Errors will trigger an early return, not a crash.
    mesh_domain["topologies"][multimesh_name].move(intermediate_topo);
    mesh_domain["coordsets"][multimesh_name].move(intermediate_coordset);

    return true;
}

//-----------------------------------------------------------------------------
bool
read_mesh_domain(const int meshtype,
                 DBfile *mesh_domain_file_to_use,
                 const std::string &mesh_name,
                 const std::string &multimesh_name,
                 const std::string &domain_path,
                 Node &mesh)
{
    // quadmeshes are finnicky with their types so we use this helpful lambda
    auto meshtype_is_quad = [](const int meshtype)
    {
        return meshtype == DB_QUADMESH || meshtype == DB_QUADCURV || meshtype == DB_QUADRECT;
    };

    if (! DBInqVarExists(mesh_domain_file_to_use, mesh_name.c_str()))
    {
        // This mesh is missing
        return false;
    }

    if (meshtype == DB_UCDMESH)
    {
        if (DBInqVarType(mesh_domain_file_to_use, mesh_name.c_str()) != DB_UCDMESH)
        {
            // This mesh is the wrong type
            return false;
        }
        detail::SiloObjectWrapper<DBucdmesh, decltype(&DBFreeUcdmesh)> ucdmesh{
            DBGetUcdmesh(mesh_domain_file_to_use, mesh_name.c_str()),
            &DBFreeUcdmesh};
        if (! ucdmesh.getSiloObject())
        {
            // If we cannot fetch this mesh we will skip
            return false;
        }
        if (! read_ucdmesh_domain(ucdmesh.getSiloObject(),
                                  mesh_name,
                                  multimesh_name,
                                  mesh[domain_path]))
        {
            return false;
        }
    }
    else if (meshtype_is_quad(meshtype))
    {
        if (! meshtype_is_quad(DBInqVarType(mesh_domain_file_to_use, mesh_name.c_str())))
        {
            // This mesh is the wrong type
            return false;
        }
        detail::SiloObjectWrapper<DBquadmesh, decltype(&DBFreeQuadmesh)> quadmesh{
            DBGetQuadmesh(mesh_domain_file_to_use, mesh_name.c_str()),
            &DBFreeQuadmesh};
        if (! quadmesh.getSiloObject())
        {
            // If we cannot fetch this mesh we will skip
            return false;
        }
        if (! read_quadmesh_domain(quadmesh.getSiloObject(),
                                   multimesh_name,
                                   mesh[domain_path]))
        {
            return false;
        }
    }
    else if (meshtype == DB_POINTMESH)
    {
        if (DBInqVarType(mesh_domain_file_to_use, mesh_name.c_str()) != DB_POINTMESH)
        {
            // This mesh is the wrong type
            return false;
        }
        detail::SiloObjectWrapper<DBpointmesh, decltype(&DBFreePointmesh)> pointmesh{
            DBGetPointmesh(mesh_domain_file_to_use, mesh_name.c_str()),
            &DBFreePointmesh};
        if (! pointmesh.getSiloObject())
        {
            // If we cannot fetch this mesh we will skip
            return false;
        }
        if (! read_pointmesh_domain(pointmesh.getSiloObject(),
                                    multimesh_name,
                                    mesh[domain_path]))
        {
            return false;
        }
    }
    else
    {
        CONDUIT_INFO("Unsupported mesh type " << meshtype);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
template <typename T>
void
read_matset_values(const Node &silo_mixvals,
                   const Node &matset_field_reconstruction,
                   Node &field_out)
{
    std::vector<T> matset_values;

    const T *silo_mixvals_ptr = silo_mixvals.value();
    const T *bp_field_vals    = field_out["values"].value();

    int_accessor recipe = matset_field_reconstruction["recipe"].value();
    int_accessor sizes  = matset_field_reconstruction["sizes"].value();

    int num_elems = matset_field_reconstruction["sizes"].dtype().number_of_elements();
    int bp_vals_index = 0;
    int recipe_index = 0;

    // iterate thru the zones
    for (int i = 0; i < num_elems; i ++)
    {
        // this is not a mixed zone
        if (sizes[i] == 1) // and recipe[i] == -1
        {
            // we can simply copy from the field values
            matset_values.push_back(bp_field_vals[bp_vals_index]);
            // we have advanced thru one bp field value
            bp_vals_index ++;
            // we have advanced thru one recipe value
            recipe_index ++;
        }
        // this zone is mixed
        else
        {
            // fetch how many materials are in the zone
            int size = sizes[i];
            // we want to copy one value for every material
            while (size > 0)
            {
                // the recipe contains the index of the silo mixval we want
                int silo_mixval_index = recipe[recipe_index];
                // we grab that mixval and save it
                matset_values.push_back(silo_mixvals_ptr[silo_mixval_index]);
                // we have advanced thru one recipe value
                recipe_index ++;
                // advanced thru one material
                size --;
            }
            // we have only advanced thru one bp field value b/c the zone was mixed
            bp_vals_index ++;
        }
    }

    field_out["matset_values"].set(matset_values);
}

//-----------------------------------------------------------------------------
// only to be used for ucdvars and quadvars
template <class T>
void
read_variable_domain_mixvals(const T *var_ptr,
                             const std::string &var_name,
                             Node &mesh_out,
                             Node &field_out,
                             const Node &matset_field_reconstruction)
{
    if (var_ptr->mixlen <= 0)
    {
        return;
    }

    if (! detail::check_var_type(var_ptr->datatype))
    {
        CONDUIT_INFO("Unsupported datatype " << var_ptr->datatype << " for variable " <<
                     var_name << ". Skipping.");
        return;
    }

    CONDUIT_ASSERT(var_ptr->mixvals,
        "mixlen is > 0 but no mixvals are provided for var " << var_name);
    CONDUIT_ASSERT(var_ptr->mixvals[0], "mixvals are NULL for var " << var_name);
    CONDUIT_ASSERT(mesh_out.has_child("matsets"),
        "Missing matset despite field " << var_name << " requiring one.");
    // should be enforced earlier, but doesn't hurt to check again
    CONDUIT_ASSERT(mesh_out["matsets"].number_of_children() == 1,
        "This mesh has multiple matsets, which is ambiguous.");

    const std::string matset_name = mesh_out["matsets"].children().next().name();
    field_out["matset"].set(matset_name);

    Node silo_mixvals;
    detail::assign_values(var_ptr->datatype,
                          var_ptr->nvals,
                          var_ptr->mixlen,
                          var_ptr->mixvals,
                          silo_mixvals);

    if (var_ptr->datatype == DB_INT)
    {
        read_matset_values<int>(silo_mixvals,
                                matset_field_reconstruction,
                                field_out);
    }
    else if (var_ptr->datatype == DB_SHORT)
    {
        read_matset_values<short>(silo_mixvals,
                                  matset_field_reconstruction,
                                  field_out);
    }
    else if (var_ptr->datatype == DB_LONG)
    {
        read_matset_values<long>(silo_mixvals,
                                 matset_field_reconstruction,
                                 field_out);
    }
    else if (var_ptr->datatype == DB_FLOAT)
    {
        read_matset_values<float>(silo_mixvals,
                                  matset_field_reconstruction,
                                  field_out);
    }
    else if (var_ptr->datatype == DB_DOUBLE)
    {
        read_matset_values<double>(silo_mixvals,
                                   matset_field_reconstruction,
                                   field_out);
    }
    else if (var_ptr->datatype == DB_LONG_LONG)
    {
        read_matset_values<long long>(silo_mixvals,
                                      matset_field_reconstruction,
                                      field_out);
    }
    else if (var_ptr->datatype == DB_CHAR)
    {
        CONDUIT_ERROR("Mixvar values cannot be strings.");
    }
    else
    {
        CONDUIT_ERROR("Unsupported type in " << var_ptr->datatype);
    }
}

//-----------------------------------------------------------------------------
template <class T>
bool
read_variable_domain_helper(const T *var_ptr,
                            const std::string &var_name,
                            const std::string &multimesh_name,
                            const int vartype,
                            const std::string &bottom_level_mesh_name,
                            const std::string &volume_dependent,
                            const Node &mesh_out,
                            Node &intermediate_field)
{
    // If we cannot fetch this var we will skip
    if (! var_ptr)
    {
        CONDUIT_INFO("Variable " << var_name << " was unable to be fetched. Skipping.");
        return false;
    }

    // check that this var is associated with the mesh
    std::string var_meshname = var_ptr->meshname;
    if (var_meshname.length() > 1 && var_meshname[0] == '/')
    {
        var_meshname = var_meshname.substr(1);
    }
    if (var_meshname != bottom_level_mesh_name)
    {
        std::string vartype_str;
        if (vartype == DB_UCDVAR)
        {
            vartype_str = "DB_UCDVAR";
        }
        else if (vartype == DB_QUADVAR)
        {
            vartype_str = "DB_QUADVAR";
        }
        else // if (vartype == DB_POINTVAR)
        {
            vartype_str = "DB_POINTVAR";
        }
        CONDUIT_INFO(vartype_str + " " + var_name + " is not "
                     "associated with mesh " + bottom_level_mesh_name +
                     ". Skipping.");
        return false;
    }

    if (! detail::check_var_type(var_ptr->datatype))
    {
        CONDUIT_INFO("Unsupported datatype " << var_ptr->datatype << " for variable " <<
                     var_name << ". Skipping.");
        return false;
    }

    intermediate_field["topology"] = multimesh_name;

    const std::string &topo_type = mesh_out["topologies"][multimesh_name]["type"].as_string();

    // handle association and strided structured
    if (vartype == DB_UCDVAR)
    {
        intermediate_field["association"] = var_ptr->centering == DB_ZONECENT ? "element" : "vertex";
    }
    else if (vartype == DB_QUADVAR)
    {
        intermediate_field["association"] = var_ptr->centering == DB_NODECENT ? "vertex" : "element";

        // put me in jail
        const DBquadvar* quadvar_ptr = reinterpret_cast<const DBquadvar*>(var_ptr);

        if (topo_type == "structured")
        {
            const std::string irregular_striding_err_msg = "Structured (noncollinear)"
                " column major quadmesh " + multimesh_name + " field " + var_name +
                " has irregular striding, which makes it impossible to correctly"
                " convert to Blueprint.";

            if (detail::check_using_whole_coordset(quadvar_ptr->dims,
                                                   quadvar_ptr->min_index,
                                                   quadvar_ptr->max_index,
                                                   quadvar_ptr->ndims))
            {
                // row major case requires nothing else
                if (quadvar_ptr->major_order == DB_COLMAJOR)
                {
                    // resort to strided structured
                    int strides[] = {0,0,0};
                    detail::colmajor_regular_striding(strides,
                                                      quadvar_ptr->ndims,
                                                      irregular_striding_err_msg,
                                                      quadvar_ptr->stride,
                                                      quadvar_ptr->dims);
                    intermediate_field["strides"].set(strides, quadvar_ptr->ndims);
                }
            }
            else
            {
                // strided structured case

                intermediate_field["offsets"].set(quadvar_ptr->min_index, quadvar_ptr->ndims);

                if (quadvar_ptr->major_order == DB_ROWMAJOR)
                {
                    intermediate_field["strides"].set(quadvar_ptr->stride, quadvar_ptr->ndims);
                }
                else // colmajor
                {
                    int actual_strides[] = {0,0,0};
                    detail::colmajor_regular_striding(actual_strides,
                                                      quadvar_ptr->ndims,
                                                      irregular_striding_err_msg,
                                                      quadvar_ptr->stride,
                                                      quadvar_ptr->dims);
                    intermediate_field["strides"].set(actual_strides, quadvar_ptr->ndims);
                }
            }
        }
        else // rectilinear
        {
            CONDUIT_ASSERT(detail::check_using_whole_coordset(quadvar_ptr->dims,
                                                              quadvar_ptr->min_index,
                                                              quadvar_ptr->max_index,
                                                              quadvar_ptr->ndims),
                           "Field " << var_name << " for Rectilinear grid (collinear quadmesh) " <<
                           multimesh_name << " is using a subset of the provided field values. We "
                           "do not support this case.");

            CONDUIT_ASSERT(quadvar_ptr->major_order == DB_ROWMAJOR,
                           "Field " << var_name << " for Rectilinear grid (collinear quadmesh) " <<
                           multimesh_name << " is column major in silo. We do not support this case.");
        }
    }
    else // if (vartype == DB_POINTVAR)
    {
        intermediate_field["association"] = "vertex";

        // we don't need to worry about the min_index and max_index case
        // nor do we need to worry about strides
        // nor do we need to worry about colmajor versus row major
        // I have it on good authority that these cases are meaningless for point var data
        // each of the above attributes are dependent on ndims > 0, which seems to never be
        // the case for point variable data.
    }

    // if we have volume dependence we can track it
    if (! volume_dependent.empty())
    {
        intermediate_field["volume_dependent"] = volume_dependent;
    }

    if (var_ptr->units)
    {
        intermediate_field["units"] = var_ptr->units;
    }
    if (var_ptr->label)
    {
        intermediate_field["label"] = var_ptr->label;
    }

    // TODO investigate the dims, major_order, and stride for vars. Should match the mesh;
    // what to do if it is different? Will I need to walk these arrays differently?

    detail::assign_values(var_ptr->datatype,
                          var_ptr->nvals,
                          var_ptr->nels,
                          var_ptr->vals,
                          intermediate_field["values"]);

    return true;
}

//-----------------------------------------------------------------------------
bool
read_variable_domain(const int vartype,
                     DBfile* var_domain_file_to_use,
                     const std::string &var_name,
                     const std::string &multimesh_name,
                     const std::string &multivar_name,
                     const std::string &bottom_level_mesh_name,
                     const std::string &volume_dependent,
                     const std::string &opts_matset_style,
                     const Node &matset_field_reconstruction,
                     Node &mesh_out)
{
    // TODO audit the failure cases. Can we skip instead?
    if (! DBInqVarExists(var_domain_file_to_use, var_name.c_str()))
    {
        // This var is missing
        CONDUIT_INFO("Variable " << var_name << " is missing. Skipping.");
        return false;
    }
    if (DBInqVarType(var_domain_file_to_use, var_name.c_str()) != vartype)
    {
        // This var is the wrong type
        CONDUIT_INFO("Variable " << var_name << " is not the right type. Skipping.");
        return false;
    }

    // create an intermediate field, in case we need to transform it later
    Node intermediate_field;

    if (vartype == DB_UCDVAR)
    {
        // create ucd var
        detail::SiloObjectWrapper<DBucdvar, decltype(&DBFreeUcdvar)> ucdvar{
            DBGetUcdvar(var_domain_file_to_use, var_name.c_str()),
            &DBFreeUcdvar};

        if (!read_variable_domain_helper<DBucdvar>(
            ucdvar.getSiloObject(), var_name, multimesh_name, vartype,
            bottom_level_mesh_name, volume_dependent, mesh_out, intermediate_field))
        {
            return false; // we hit a case where we want to skip this var
        }

        read_variable_domain_mixvals<DBucdvar>(ucdvar.getSiloObject(),
            var_name, mesh_out, intermediate_field,
            matset_field_reconstruction);
    }
    else if (vartype == DB_QUADVAR)
    {
        // create quad var
        detail::SiloObjectWrapper<DBquadvar, decltype(&DBFreeQuadvar)> quadvar{
            DBGetQuadvar(var_domain_file_to_use, var_name.c_str()),
            &DBFreeQuadvar};

        if (!read_variable_domain_helper<DBquadvar>(
            quadvar.getSiloObject(), var_name, multimesh_name, vartype,
            bottom_level_mesh_name, volume_dependent, mesh_out, intermediate_field))
        {
            return false; // we hit a case where we want to skip this var
        }

        read_variable_domain_mixvals<DBquadvar>(quadvar.getSiloObject(),
            var_name, mesh_out, intermediate_field,
            matset_field_reconstruction);
    }
    else if (vartype == DB_POINTVAR)
    {
        // create point var
        detail::SiloObjectWrapper<DBmeshvar, decltype(&DBFreeMeshvar)> meshvar{
            DBGetPointvar(var_domain_file_to_use, var_name.c_str()),
            &DBFreeMeshvar};

        if (!read_variable_domain_helper<DBmeshvar>(
            meshvar.getSiloObject(), var_name, multimesh_name, vartype,
            bottom_level_mesh_name, volume_dependent, mesh_out, intermediate_field))
        {
            return false; // we hit a case where we want to skip this var
        }
    }
    else
    {
        CONDUIT_ERROR("Unsupported variable type " << vartype);
    }

    // create an entry for this field in the output
    Node &field_out = mesh_out["fields"][multivar_name];

    if (intermediate_field.has_child("matset"))
    {
        const std::string matset_name = intermediate_field["matset"].as_string();

        // collect the sparse by element matset we generated
        const Node &original_matset = matset_field_reconstruction["original_matset"];

        if (opts_matset_style == "default" || opts_matset_style == "sparse_by_element")
        {
            field_out.move(intermediate_field);
        }
        else if (opts_matset_style == "multi_buffer_full")
        {
            conduit::blueprint::mesh::field::to_multi_buffer_full(original_matset,
                                                                  intermediate_field,
                                                                  matset_name,
                                                                  field_out);
        }
        else // "multi_buffer_by_material"
        {
            conduit::blueprint::mesh::field::to_multi_buffer_by_material(original_matset,
                                                                         intermediate_field,
                                                                         matset_name,
                                                                         field_out);
        }
    }
    else
    {
        field_out.move(intermediate_field);
    }

    return true;
}

//-----------------------------------------------------------------------------
template<typename T>
void
read_matlist_entry(const DBmaterial* matset_ptr,
                   const int matlist_index,
                   std::vector<double> &volume_fractions,
                   std::vector<int> &material_ids,
                   std::vector<int> &sizes,
                   std::vector<int> &offsets,
                   int &curr_offset,
                   std::vector<int> &field_reconstruction_recipe)
{
    const int matlist_entry = matset_ptr->matlist[matlist_index];
    if (matlist_entry >= 0) // this relies on matset_ptr->allowmat0 == 0
    {
        // clean zone
        // this zone has the material with mat id == matlist_entry
        const int &mat_id = matlist_entry;
        field_reconstruction_recipe.push_back(-1);
        volume_fractions.push_back(1.0);
        material_ids.push_back(mat_id);
        sizes.push_back(1);
        offsets.push_back(curr_offset);
        curr_offset ++;
    }
    else
    {
        // for mixed zones, the numbers in the matlist are negated 1-indices into
        // the silo mixed data arrays. To turn them into zero-indices, we must add
        // 1 and negate the result. Example:
        // indices: -1 -2 -3 -4 ...
        // become:   0  1  2  3 ...

        int mix_id = -1 * (matlist_entry + 1);
        int curr_size = 0;

        // when matset_ptr->mix_next[mix_id] is 0, we are on the last one
        while (mix_id >= 0)
        {
            material_ids.push_back(matset_ptr->mix_mat[mix_id]);

            // mix_vf is a void ptr so we must cast
            volume_fractions.push_back(static_cast<T *>(matset_ptr->mix_vf)[mix_id]);

            field_reconstruction_recipe.push_back(mix_id);

            curr_size ++;
            // since mix_id is a 1-index, we must subtract one
            // this makes sure that mix_id = 0 is the last case,
            // since it will make our mix_id == -1, which ends
            // the while loop.
            mix_id = matset_ptr->mix_next[mix_id] - 1;
        }

        sizes.push_back(curr_size);
        offsets.push_back(curr_offset);
        curr_offset += curr_size;
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void
read_matlist(const DBmaterial* matset_ptr,
             const int nx,
             const int ny,
             const int nz,
             std::vector<double> &volume_fractions,
             std::vector<int> &material_ids,
             std::vector<int> &sizes,
             std::vector<int> &offsets,
             std::vector<int> &field_reconstruction_recipe)
{
    int curr_offset = 0;

    if (matset_ptr->major_order == DB_ROWMAJOR)
    {
        for (int z = 0; z < nz; z ++)
        {
            for (int y = 0; y < ny; y ++)
            {
                for (int x = 0; x < nx; x ++)
                {
                    const int matlist_index = x + y * nx + z * nx * ny;
                    read_matlist_entry<T>(matset_ptr,
                                          matlist_index,
                                          volume_fractions,
                                          material_ids,
                                          sizes,
                                          offsets,
                                          curr_offset,
                                          field_reconstruction_recipe);
                }
            }
        }
    }
    else // COLMAJOR
    {
        // I'm not convinced it is ever possible to hit this case.
        // If you have column major mesh data, you hit the strided structured
        // case, which (for now) forces an early return at the beginning of
        // this function. We may reenable that case later, which requires
        // filtering the matset down and is potentially quite challenging.
        // The only way you can get here I think is if you have column major
        // material data and row major mesh data. I've never seen an example
        // file like that so far.
        for (int x = 0; x < nx; x ++)
        {
            for (int y = 0; y < ny; y ++)
            {
                for (int z = 0; z < nz; z ++)
                {
                    const int matlist_index = z + y * nz + x * nz * ny;
                    read_matlist_entry<T>(matset_ptr,
                                          matlist_index,
                                          volume_fractions,
                                          material_ids,
                                          sizes,
                                          offsets,
                                          curr_offset,
                                          field_reconstruction_recipe);
                }
            }
        }
    }

    // TODO still need to find colmajor data to test this
}

//-----------------------------------------------------------------------------
bool
read_matset_domain(DBfile* matset_domain_file_to_use,
                   const Node &n_matset,
                   const std::string &matset_name,
                   const std::string &multimesh_name,
                   const std::string &multimat_name,
                   const std::string &bottom_level_mesh_name,
                   const std::string &opts_matset_style,
                   Node &matset_field_reconstruction,
                   Node &silo_material,
                   Node &mesh_out)
{
    if (! DBInqVarExists(matset_domain_file_to_use, matset_name.c_str()))
    {
        // This matset is missing
        return false;
    }
    if (DBInqVarType(matset_domain_file_to_use, matset_name.c_str()) != DB_MATERIAL)
    {
        // This matset is the wrong type
        return false;
    }

    // create silo matset
    detail::SiloObjectWrapper<DBmaterial, decltype(&DBFreeMaterial)> material{
        DBGetMaterial(matset_domain_file_to_use, matset_name.c_str()),
        &DBFreeMaterial};

    const DBmaterial* matset_ptr = material.getSiloObject();

    // If we cannot fetch this matset we will skip
    if (! matset_ptr)
    {
        return false;
    }

    // check that this matset is associated with the mesh
    std::string matset_meshname = matset_ptr->meshname;
    if (matset_meshname.length() > 1 && matset_meshname[0] == '/')
    {
        matset_meshname = matset_meshname.substr(1);
    }
    if (matset_meshname != bottom_level_mesh_name)
    {
        CONDUIT_INFO("DBmaterial " + matset_name + " is not "
                     "associated with mesh " + bottom_level_mesh_name +
                     ". Skipping.");
        return false;
    }

    // Check for structured strided case:
    const Node &topo_out = mesh_out["topologies"][multimesh_name];
    if (topo_out["type"].as_string() == "structured" &&
        (topo_out.has_path("elements/dims/strides") ||
         topo_out.has_path("elements/dims/strides")))
    {
        CONDUIT_INFO("DBmaterial " + matset_name + " is associated with mesh " +
                     bottom_level_mesh_name + ", which is a Structured (noncollinear) "
                     "quadmesh. It uses a subset of the coordinates to define the mesh. "
                     "We map meshes like this to Structured Strided meshes in Blueprint, "
                     "which expect associated matsets to only provide data for mesh zones "
                     "that are actually being used by the topology. Silo materials provide "
                     "data for all mesh zones, whether or not they are being used by the "
                     "quadmesh. We have opted to not support this case, as to do so would "
                     "require us to manually remove material set data that corresponds to "
                     "unused zones. If there is a need for this feature, please contact a "
                     "Conduit developer. In the meantime, we will skip reading this DBmaterial.");
        return false;
    }

    // we can only succeed here if the data is regularly strided
    const std::string irregular_striding_warn_msg = "DBmaterial " + matset_name +
        " has irregular striding, which makes it impossible to correctly convert"
        " to Blueprint. Skipping.";
    if (1 == matset_ptr->ndims)
    {
        if (matset_ptr->stride[0] != 1)
        {
            CONDUIT_INFO(irregular_striding_warn_msg);
            return false;
        }
    }
    else if (2 == matset_ptr->ndims)
    {
        if (matset_ptr->stride[0] != 1 ||
            matset_ptr->stride[1] != matset_ptr->dims[0])
        {
            CONDUIT_INFO(irregular_striding_warn_msg);
            return false;
        }
    }
    else // (3 == matset_ptr->ndims)
    {
        if (matset_ptr->stride[0] != 1 ||
            matset_ptr->stride[1] != matset_ptr->dims[0] ||
            matset_ptr->stride[2] != matset_ptr->dims[0] * matset_ptr->dims[1])
        {
            CONDUIT_INFO(irregular_striding_warn_msg);
            return false;
        }
    }

    if (matset_ptr->allowmat0 != 0)
    {
        CONDUIT_INFO(
            "Material " << matset_name << " for multimesh " << multimesh_name <<
            " may contain zones with no materials defined on them." <<
            " We currently do not support this case. Either contact a Conduit developer" <<
            " or disable DBOPT_ALLOWMAT0 in calls to DBPutMaterial().");
        return false;
    }

    // create an intermediate matset, in case we need to transform it later
    Node intermediate_matset;

    intermediate_matset["topology"] = multimesh_name;

    // we read into sparse by element, then use transforms to modify later if requested

    Node &material_map = intermediate_matset["material_map"];
    // if we have material map information from the multimat, we want to use that instead
    if (n_matset.has_child("material_map"))
    {
        // ignore what is here and use what was found in the multimat
        material_map.set(n_matset["material_map"]);
    }
    else
    {
        if (matset_ptr->nmat <= 0)
        {
            CONDUIT_INFO("Number of materials is non-positive for material "
                         << matset_name << " for multimesh " << multimesh_name);
            return false;
        }
        for (int i = 0; i < matset_ptr->nmat; i ++)
        {
            int matno;
            if (matset_ptr->matnos)
            {
                // we have mat nos to work with
                matno = matset_ptr->matnos[i];
            }
            else
            {
                // we infer that matnos run from 1 to nmat, inclusive
                matno = i + 1;
            }

            if (matset_ptr->matnames) // may be null
            {
                material_map[matset_ptr->matnames[i]] = matno;
            }
            else
            {
                material_map[std::to_string(matno)] = matno;
            }
        }
    }

    // We only need to check the data type is one of the allowed types
    // if mixlen is greater than 0 (meaning that we have volume fractions
    // and mixed zones). One user reported hitting this error with data
    // that had no mixed zones, although we have not been able to reproduce,
    // even using data they provided. Regardless, it doesn't hurt to relax
    // the restriction here.
    if (matset_ptr->mixlen > 0)
    {
        if (matset_ptr->datatype != DB_DOUBLE && matset_ptr->datatype != DB_FLOAT)
        {
            CONDUIT_INFO("Volume fractions must be doubles or floats." <<
                         " Unknown type for volume fractions for " << matset_name);
            return false;
        }
    }

    std::vector<double> volume_fractions;
    std::vector<int> material_ids;
    std::vector<int> sizes;
    std::vector<int> offsets;

    // The field reconstruction recipe is an array that will help us to
    // reconstruct the blueprint matset_values for any fields that use
    // this matset. I put -1 into it whenever I am supposed to read from
    // the regular values, and a positive index into it whenever I am
    // supposed to read from the mixvals from silo.
    std::vector<int> field_reconstruction_recipe;

    const int nx = matset_ptr->dims[0];
    const int ny = (matset_ptr->ndims > 1) ? matset_ptr->dims[1] : 1;
    const int nz = (matset_ptr->ndims > 2) ? matset_ptr->dims[2] : 1;

    if (matset_ptr->datatype == DB_DOUBLE)
    {
        read_matlist<double>(matset_ptr,
                             nx, ny, nz,
                             volume_fractions,
                             material_ids,
                             sizes,
                             offsets,
                             field_reconstruction_recipe);
    }
    else
    {
        // We have verified up above that this is a float, or that there
        // are no mixed zones, in which case we won't need to use the
        // `float` type.
        read_matlist<float>(matset_ptr,
                            nx, ny, nz,
                            volume_fractions,
                            material_ids,
                            sizes,
                            offsets,
                            field_reconstruction_recipe);
    }

    // we need to save silo material information for use when reading specsets
    silo_material["matlist"].set(matset_ptr->matlist, static_cast<int>(sizes.size()));
    silo_material["mix_next"].set(matset_ptr->mix_next, matset_ptr->mixlen);
    silo_material["mix_mat"].set(matset_ptr->mix_mat, matset_ptr->mixlen);
    silo_material["matnos"].set(matset_ptr->matnos, matset_ptr->nmat);
    silo_material["material_map"].set(material_map);
    silo_material["num_zones"].set(static_cast<int>(sizes.size()));
    silo_material["matset_path"].set(matset_name);
    silo_material["matset_name"].set(multimat_name);
    silo_material["dims/nx"].set(nx);
    silo_material["dims/ny"].set(ny);
    silo_material["dims/nz"].set(nz);
    silo_material["major_order"].set(matset_ptr->major_order);

    intermediate_matset["material_ids"].set(material_ids);
    intermediate_matset["volume_fractions"].set(volume_fractions);
    intermediate_matset["sizes"].set(sizes);
    intermediate_matset["offsets"].set(offsets);

    matset_field_reconstruction["recipe"].set(field_reconstruction_recipe);
    matset_field_reconstruction["sizes"].set(sizes);

    // create an entry for this matset in the output
    Node &matset_out = mesh_out["matsets"][multimat_name];

    if (opts_matset_style == "default" || opts_matset_style == "sparse_by_element")
    {
        matset_out.move(intermediate_matset);

        // matset out will live forever, so we can point to it
        matset_field_reconstruction["original_matset"].set_external(matset_out);
    }
    else if (opts_matset_style == "multi_buffer_full")
    {
        conduit::blueprint::mesh::matset::to_multi_buffer_full(intermediate_matset, matset_out);

        // we only need to stash the matset for use in converters later if we need
        // a different flavor of matset
        matset_field_reconstruction["original_matset"].move(intermediate_matset);
    }
    else // "multi_buffer_by_material"
    {
        conduit::blueprint::mesh::matset::to_multi_buffer_by_material(intermediate_matset, matset_out);

        // we only need to stash the matset for use in converters later if we need
        // a different flavor of matset
        matset_field_reconstruction["original_matset"].move(intermediate_matset);
    }

    return true;
}

//-----------------------------------------------------------------------------
template<typename T>
void
read_speclist_entry(const DBmatspecies* specset_ptr,
                    const int zone_id,
                    const int_accessor &silo_matlist,
                    const int_accessor &silo_mix_mat,
                    const int_accessor &silo_mix_next,
                    const std::map<int, std::string> &reverse_matmap,
                    Node &matset_values)
{
    const int matlist_entry = silo_matlist[zone_id];
    const int speclist_entry = specset_ptr->speclist[zone_id];

    if (matlist_entry >= 0) // this relies on matset_ptr->allowmat0 == 0
    {
        // clean zone
        // this zone has the material with mat id == matlist_entry
        const int &mat_id = matlist_entry;
        const std::string &matname = reverse_matmap.at(mat_id);

        // if this material has species
        if (matset_values.has_child(matname))
        {
            // read mass fractions for this material in this zone
            const std::vector<std::string> &specnames_for_mat = matset_values[matname].child_names();
            const int num_spec_for_mat = matset_values[matname].number_of_children();
            // speclist_entry is a 1-index into species_mf
            const int species_mf_index = speclist_entry - 1;
            for (int spec_id = 0; spec_id < num_spec_for_mat; spec_id ++)
            {
                const std::string &specname = specnames_for_mat[spec_id];
                float64_array mass_fractions = matset_values[matname][specname].value();
                // species_mf is a void ptr so we must cast
                mass_fractions[zone_id] = static_cast<T *>(specset_ptr->species_mf)[species_mf_index + spec_id];
            }

            // we don't have to do anything for the other materials because
            // we used data_array fill up above to set everything to all zeros
            // mass fractions should be zero for all species that belong to
            // materials that are not in the zone.
        }
        // else... if this material has no species, then there is nothing to do here
    }
    else
    {
        // for mixed zones, the numbers in the matlist are negated 1-indices into
        // the silo mixed data arrays. To turn them into zero-indices, we must add
        // 1 and negate the result. Example:
        // indices: -1 -2 -3 -4 ...
        // become:   0  1  2  3 ...

        int mix_id = -1 * (matlist_entry + 1);

        // when silo_mix_next[mix_id] is 0, we are on the last one
        while (mix_id >= 0)
        {
            const int mat_id = silo_mix_mat[mix_id];
            const std::string &matname = reverse_matmap.at(mat_id);

            // if this material has species
            if (matset_values.has_child(matname))
            {
                // read mass fractions for this material in this zone
                const std::vector<std::string> &specnames_for_mat = matset_values[matname].child_names();
                const int num_spec_for_mat = matset_values[matname].number_of_children();
                // mix_speclist entry is a 1-index into species_mf
                const int species_mf_index = specset_ptr->mix_speclist[mix_id] - 1;
                for (int spec_id = 0; spec_id < num_spec_for_mat; spec_id ++)
                {
                    const std::string &specname = specnames_for_mat[spec_id];
                    float64_array mass_fractions = matset_values[matname][specname].value();
                    // species_mf is a void ptr so we must cast
                    mass_fractions[zone_id] = static_cast<T *>(specset_ptr->species_mf)[species_mf_index + spec_id];
                }
            }
            // else... if this material has no species, then there is nothing to do here

            // since mix_id is a 1-index, we must subtract one
            // this makes sure that mix_id = 0 is the last case,
            // since it will make our mix_id == -1, which ends
            // the while loop.
            mix_id = silo_mix_next[mix_id] - 1;
        }
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void
read_speclist(const DBmatspecies* specset_ptr,
              const int nx,
              const int ny,
              const int nz,
              const int_accessor &silo_matlist,
              const int_accessor &silo_mix_mat,
              const int_accessor &silo_mix_next,
              const std::map<int, std::string> &reverse_matmap,
              Node &matset_values)
{
    if (specset_ptr->major_order == DB_ROWMAJOR)
    {
        for (int z = 0; z < nz; z ++)
        {
            for (int y = 0; y < ny; y ++)
            {
                for (int x = 0; x < nx; x ++)
                {
                    const int zone_id = x + y * nx + z * nx * ny;
                    read_speclist_entry<T>(specset_ptr,
                                           zone_id,
                                           silo_matlist,
                                           silo_mix_mat,
                                           silo_mix_next,
                                           reverse_matmap,
                                           matset_values);
                }
            }
        }
    }
    else // COLMAJOR
    {
        // I'm not convinced it is ever possible to hit this case.
        // If you have column major mesh data, you hit the strided structured
        // case, which (for now) forces an early return at the beginning of
        // this function. We may reenable that case later, which requires
        // filtering the specset down and is potentially quite challenging.
        // The only way you can get here I think is if you have column major
        // species set data and row major mesh data. I've never seen an example
        // file like that so far.
        for (int x = 0; x < nx; x ++)
        {
            for (int y = 0; y < ny; y ++)
            {
                for (int z = 0; z < nz; z ++)
                {
                    const int zone_id = z + y * nz + x * nz * ny;
                    read_speclist_entry<T>(specset_ptr,
                                           zone_id,
                                           silo_matlist,
                                           silo_mix_mat,
                                           silo_mix_next,
                                           reverse_matmap,
                                           matset_values);
                }
            }
        }
    }

    // TODO still need to find colmajor data to test this
}

//-----------------------------------------------------------------------------
bool
read_specset_domain(DBfile* specset_domain_file_to_use,
                    const Node &n_specset,
                    const std::string &specset_name,
                    const std::string &multimatspecies_name,
                    const std::string &opts_matset_style,
                    const Node &silo_material,
                    Node &mesh_out)
{
    // TODO remove this once we support specset converters
    if (opts_matset_style != "multi_buffer_full")
    {
        CONDUIT_INFO("TODO if the matset flavor is not multi_buffer + "
                     "element_dominant, we currently cannot read species "
                     "from silo. Please contact a Conduit developer.");
        return false;
    }

    if (! DBInqVarExists(specset_domain_file_to_use, specset_name.c_str()))
    {
        // This specset is missing
        return false;
    }
    if (DBInqVarType(specset_domain_file_to_use, specset_name.c_str()) != DB_MATSPECIES)
    {
        // This specset is the wrong type
        return false;
    }

    // create silo specset
    detail::SiloObjectWrapper<DBmatspecies, decltype(&DBFreeMatspecies)> species_set{
        DBGetMatspecies(specset_domain_file_to_use, specset_name.c_str()),
        &DBFreeMatspecies};

    const DBmatspecies* specset_ptr = species_set.getSiloObject();

    // If we cannot fetch this specset we will skip
    if (! specset_ptr)
    {
        return false;
    }

    if (! silo_material.has_child("matlist") ||
        ! silo_material.has_child("mix_next") ||
        ! silo_material.has_child("mix_mat") ||
        ! silo_material.has_child("material_map") ||
        ! silo_material.has_child("matnos") ||
        ! silo_material.has_child("num_zones") ||
        ! silo_material.has_child("matset_path") ||
        ! silo_material.has_path("dims/nx") ||
        ! silo_material.has_path("dims/ny") ||
        ! silo_material.has_path("dims/nz") ||
        ! silo_material.has_child("major_order"))
    {
        CONDUIT_INFO("Attempting to read DBmatspecies " + specset_name +
                     " but required DBmaterial information is missing. Skipping.");
        return false;
    }

    // check that this specset is associated with a matset we have read
    std::string assoc_matname = specset_ptr->matname;
    if (assoc_matname.length() > 1 && assoc_matname[0] == '/')
    {
        assoc_matname = assoc_matname.substr(1);
    }
    std::string specset_path, bottom_level_specset_name;
    conduit::utils::rsplit_file_path(specset_name, "/", bottom_level_specset_name, specset_path);
    const std::string matset_path = (specset_path.empty() ?
                                     assoc_matname :
                                     specset_path + "/" + assoc_matname);
    if (matset_path != silo_material["matset_path"].as_string())
    {
        CONDUIT_INFO("DBmatspecies " + specset_name + " is associated "
                     "with a matset called " + assoc_matname + " which "
                     "has not been read from Silo. Skipping.");
        return false;
    }

    // we can only succeed here if the data is regularly strided
    const std::string irregular_striding_warn_msg = "DBmatspecies " + specset_name +
        " has irregular striding, which makes it impossible to correctly convert"
        " to Blueprint. Skipping.";
    if (1 == specset_ptr->ndims)
    {
        if (specset_ptr->stride[0] != 1)
        {
            CONDUIT_INFO(irregular_striding_warn_msg);
            return false;
        }
    }
    else if (2 == specset_ptr->ndims)
    {
        if (specset_ptr->stride[0] != 1 ||
            specset_ptr->stride[1] != specset_ptr->dims[0])
        {
            CONDUIT_INFO(irregular_striding_warn_msg);
            return false;
        }
    }
    else // (3 == specset_ptr->ndims)
    {
        if (specset_ptr->stride[0] != 1 ||
            specset_ptr->stride[1] != specset_ptr->dims[0] ||
            specset_ptr->stride[2] != specset_ptr->dims[0] * specset_ptr->dims[1])
        {
            CONDUIT_INFO(irregular_striding_warn_msg);
            return false;
        }
    }

    const int_accessor silo_matlist = silo_material["matlist"].value();
    const int_accessor silo_mix_next = silo_material["mix_next"].value();
    const int_accessor silo_mix_mat = silo_material["mix_mat"].value();
    const int_accessor silo_matnos = silo_material["matnos"].value();
    const int num_zones = silo_material["num_zones"].as_int();

    const int *nmatspec = nullptr;
    // does the multimatspecies object have nmatspec?
    if (n_specset.has_child("nmatspec"))
    {
        nmatspec = n_specset["nmatspec"].as_int_ptr();
    }
    else // if not we can use the local version
    {
        nmatspec = specset_ptr->nmatspec;
    }

    std::map<int, std::string> reverse_matmap =
        conduit::blueprint::mesh::matset::create_reverse_material_map(silo_material["material_map"]);

    if (silo_matnos.number_of_elements() != specset_ptr->nmat)
    {
        CONDUIT_INFO("Attempting to read DBmatspecies " + specset_name +
                     " but there is a mismatch with nmat in the associated DBmaterial. Skipping.");
        return false;
    }

    if (specset_ptr->datatype != DB_DOUBLE && specset_ptr->datatype != DB_FLOAT)
    {
        CONDUIT_INFO("Species mass fractions must be doubles or floats." <<
                     " Unknown type for mass fractions for " << specset_name);
        return false;
    }

    const int nx = specset_ptr->dims[0];
    const int ny = (specset_ptr->ndims > 1) ? specset_ptr->dims[1] : 1;
    const int nz = (specset_ptr->ndims > 2) ? specset_ptr->dims[2] : 1;

    if (nx != silo_material["dims/nx"].as_int() ||
        ny != silo_material["dims/ny"].as_int() ||
        nz != silo_material["dims/nz"].as_int())
    {
        CONDUIT_INFO("Species object dimensions must match the dimensions "
                     "of the associated material object.");
        return false;
    }

    if (specset_ptr->major_order != silo_material["major_order"].as_int())
    {
        CONDUIT_INFO("Species object major order must match the major order "
                     "of the associated material object.");
        return false;
    }

    // create an entry for this matset in the output
    Node &specset_out = mesh_out["specsets"][multimatspecies_name];

    // add the matset association
    specset_out["matset"] = silo_material["matset_name"].as_string();

    // create the matset_values output
    Node &matset_values = specset_out["matset_values"];

    auto init_matset_values = [&](auto get_spec_name)
    {
        int running_index = 0;
        for (int mat_idx = 0; mat_idx < specset_ptr->nmat; mat_idx ++)
        {
            const int mat_id = silo_matnos[mat_idx];
            const std::string &matname = reverse_matmap[mat_id];
            const int num_spec_for_mat = nmatspec[mat_idx];
            // if species are not present for a material, no entry for the material
            // will be added to the specset
            for (int specname_id = 0; specname_id < num_spec_for_mat; specname_id ++)
            {
                const std::string spec_name = get_spec_name(running_index + specname_id);
                matset_values[matname][spec_name].set(DataType::float64(num_zones));
                float64_array mass_fractions = matset_values[matname][spec_name].value();
                mass_fractions.fill(0.0);

            }
            running_index += num_spec_for_mat;
        }
    };

    // does the multimatspecies object have species_names?
    if (n_specset.has_child("species_names"))
    {
        const int sum_of_nmatspec = [&]()
        {
            int sum = 0;
            for (int i = 0; i < specset_ptr->nmat; i ++)
            {
                sum += nmatspec[i];
            }
            return sum;
        }();
        CONDUIT_ASSERT(n_specset["species_names"].number_of_children() == sum_of_nmatspec,
                       "Attempting to read DBmatspecies " + specset_name +
                       " but there is a mismatch with nmatspec in the DBmultimatspecies.");
        init_matset_values([&](const int index) -> std::string
                           { return n_specset["species_names"][index].as_string(); });
    }
    else if (nullptr != specset_ptr->specnames)
    {
        // if not we can use the local version, provided it exists
        init_matset_values([&](const int index) -> std::string
                           { return specset_ptr->specnames[index]; });
    }
    else
    {
        // we make up species names if they do not exist
        init_matset_values([&](const int index) -> std::string
                           { return "species" + std::to_string(index); });
    }

    // we read into full (element_dominant and multi_buffer)

    // TODO:
    // 1. write specset flavor converters
    // 2. read species data into uni_buffer element_dominant
    // 3. convert as necessary, following what we do for matsets.

    if (specset_ptr->datatype == DB_DOUBLE)
    {
        read_speclist<double>(specset_ptr,
                              nx, ny, nz,
                              silo_matlist,
                              silo_mix_mat,
                              silo_mix_next,
                              reverse_matmap,
                              matset_values);
    }
    else
    {
        // we have verified up above that this is a float
        read_speclist<float>(specset_ptr,
                             nx, ny, nz,
                             silo_matlist,
                             silo_mix_mat,
                             silo_mix_next,
                             reverse_matmap,
                             matset_values);
    }

    return true;
}

//-----------------------------------------------------------------------------
// overlink only
void
read_adjset(DBfile *dbfile,
            const std::string &multimesh_name,
            const int domain_id,
            Node &mesh_out)
{
    const std::string dom_neighbor_nums_name = "DOMAIN_NEIGHBOR_NUMS";
    if (! DBInqVarExists(dbfile, dom_neighbor_nums_name.c_str()))
    {
        // The domain neighbor nums are not present. They are optional, so we can return early
        return;
    }
    if (DBInqVarType(dbfile, dom_neighbor_nums_name.c_str()) != DB_ARRAY)
    {
        // The domain neighbor nums are the wrong type. They are optional, so we can return early
        return;
    }
    detail::SiloObjectWrapper<DBcompoundarray, decltype(&DBFreeCompoundarray)> dom_neighbor_nums_obj{
        DBGetCompoundarray(dbfile, dom_neighbor_nums_name.c_str()),
        &DBFreeCompoundarray};
    DBcompoundarray *dom_neighbor_nums = dom_neighbor_nums_obj.getSiloObject();
    if (! dom_neighbor_nums)
    {
        // we failed to read the domain neighbor nums. We can skip them.
        return;
    }

    // fetch pointers to elements inside the compound array
    const int *dom_neighbor_nums_values  = static_cast<int *>(dom_neighbor_nums->values);
    const int dom_neighbor_nums_datatype = dom_neighbor_nums->datatype;

    if (dom_neighbor_nums_datatype != DB_INT)
    {
        // for overlink, the domain neighbor nums must contain integer data
        return;
    }

    const int num_neighboring_doms = dom_neighbor_nums_values[0];
    if (num_neighboring_doms <= 0)
    {
        // there are no neighboring domains, so we cannot create an adjset
        return;
    }

    // create the adjset
    Node &adjset_out = mesh_out["adjsets"]["adjset"];
    adjset_out["topology"] = multimesh_name;
    adjset_out["association"] = "vertex";

    for (int i = 1; i <= num_neighboring_doms; i ++)
    {
        const int neighbor_domain_id = dom_neighbor_nums_values[i];
        std::string group_name;


        if (domain_id < neighbor_domain_id)
        {
            group_name = "group_" +
                         std::to_string(domain_id) + "_" +
                         std::to_string(neighbor_domain_id);
        }
        else
        {
            group_name = "group_" +
                         std::to_string(neighbor_domain_id) + "_" +
                         std::to_string(domain_id);
        }

        // create the adjset group
        Node &group_out = adjset_out["groups"][group_name];

        // set the only neighbor
        group_out["neighbors"].set(static_cast<index_t>(neighbor_domain_id));

        const int m = i - 1; // overlink index
        const std::string arr_name = "DOMAIN_NEIGHBOR" + std::to_string(m);

        if (! DBInqVarExists(dbfile, arr_name.c_str()))
        {
            // DomainNeighborX is not present. It is optional, so we can skip
            continue;
        }
        if (DBInqVarType(dbfile, arr_name.c_str()) != DB_ARRAY)
        {
            // DomainNeighborX is the wrong type. It is optional, so we can skip
            continue;
        }
        detail::SiloObjectWrapper<DBcompoundarray, decltype(&DBFreeCompoundarray)> dom_neighbor_x_obj{
            DBGetCompoundarray(dbfile, arr_name.c_str()),
            &DBFreeCompoundarray};
        DBcompoundarray *dom_neighbor_x = dom_neighbor_x_obj.getSiloObject();
        if (! dom_neighbor_x)
        {
            // we failed to read DomainNeighborX. We can skip it.
            continue;
        }

        // fetch pointers to elements inside the compound array
        const int *neighbor_values      = static_cast<int *>(dom_neighbor_x->values);
        const int neighbor_datatype     = dom_neighbor_x->datatype;
        const int neighbor_nvalues      = dom_neighbor_x->nvalues;

        if (neighbor_datatype != DB_INT)
        {
            // for overlink, DomainNeighborX must contain integer data
            continue;
        }

        const int num_shared_nodes = neighbor_nvalues;
        group_out["values"].set(DataType::index_t(num_shared_nodes));
        index_t_array shared_nodes = group_out["values"].value();
        for (int i = 0; i < num_shared_nodes; i ++)
        {
            shared_nodes[i] = neighbor_values[i];
        }
    }
}

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API
read_mesh(const std::string &root_file_path,
          Node &mesh
          CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    read_mesh(root_file_path,
              opts,
              mesh,
              mpi_comm);
#else
    read_mesh(root_file_path,
              opts,
              mesh);
#endif
}

//-----------------------------------------------------------------------------
DBfile*
open_or_reuse_file(const bool ovltop_case,
                   std::string &domain_filename,
                   std::map<std::string, DBfile*> &file_map)
{
    // handle ovltop.silo case
    if (ovltop_case)
    {
        // first, we will assume valid overlink, so
        // we need to move the mesh/var/mat path to ../
        const std::string old_domain_filename = domain_filename;
        std::string actual_filename, directory;
        conduit::utils::rsplit_file_path(domain_filename, actual_filename, directory);
        if (!directory.empty())
        {
            std::string dir_lvl_up, bottom_lvl_dir;
            conduit::utils::rsplit_file_path(directory, bottom_lvl_dir, dir_lvl_up);

            domain_filename = conduit::utils::join_file_path(dir_lvl_up, actual_filename);
        }

        // if we have already opened this file
        if (file_map.count(domain_filename) > 0)
        {
            return file_map.at(domain_filename);
        }
        // otherwise we need to open our own file
        else
        {
            auto not_valid_overlink = [&]() -> DBfile*
            {
                CONDUIT_INFO("Provided file is not valid Overlink; defaulting "
                             "to absolute path rather than assumed path.")
                // this is not valid overlink so we default to what is in the path
                domain_filename = old_domain_filename;

                // if we have already opened this file
                if (file_map.count(domain_filename) > 0)
                {
                    return file_map.at(domain_filename);
                }
                // otherwise we need to open our own file
                else
                {
                    DBfile *domain_file_to_use = silo_open_file_for_read(domain_filename);
                    CONDUIT_ASSERT(domain_file_to_use,
                                   "Error opening Silo file for reading: " << domain_filename);
                    file_map[domain_filename] = domain_file_to_use;
                    return domain_file_to_use;
                }
            };

            if (DBInqFile(domain_filename.c_str()) > 0) // the file exists
            {
                DBfile *domain_file_to_use = silo_open_file_for_read(domain_filename);
                if (domain_file_to_use)
                {
                    file_map[domain_filename] = domain_file_to_use;
                    return domain_file_to_use;
                }
                else
                {
                    return not_valid_overlink();
                }
            }
            else
            {
                return not_valid_overlink();
            }
        }
    }
    // standard case
    else
    {
        // if we have already opened this file
        if (file_map.count(domain_filename) > 0)
        {
            return file_map.at(domain_filename);
        }
        // otherwise we need to open our own file
        else
        {
            DBfile *domain_file_to_use = silo_open_file_for_read(domain_filename);
            CONDUIT_ASSERT(domain_file_to_use,
                           "Error opening Silo file for reading: " << domain_filename);
            file_map[domain_filename] = domain_file_to_use;
            return domain_file_to_use;
        }
    }
}

//-----------------------------------------------------------------------------
bool
read_multimesh(DBfile *dbfile,
               const std::string &multimesh_name,
               int &nblocks,
               Node &root_node,
               std::ostringstream &error_oss)
{
    // extract the multimesh
    detail::SiloObjectWrapper<DBmultimesh, decltype(&DBFreeMultimesh)> multimesh{
        DBGetMultimesh(dbfile, multimesh_name.c_str()),
        &DBFreeMultimesh};
    DBmultimesh *mmesh_ptr = multimesh.getSiloObject();

    if (nullptr == mmesh_ptr)
    {
        error_oss << "Error opening multimesh " << multimesh_name;
        return false;
    }

    nblocks = mmesh_ptr->nblocks;
    root_node[multimesh_name]["nblocks"] = nblocks;

    // does this mesh use nameschemes?
    bool nameschemes = false;
    if (nullptr == mmesh_ptr->meshnames)
    {
        // if we do not have mesnnames, then we are either using nameschemes
        // or our mmesh is invalid
        if (nullptr == mmesh_ptr->block_ns)
        {
            error_oss << "Multimesh " << multimesh_name << " is missing mesh names and namescheme specifiers.";
            return false;
        }
        else
        {
            nameschemes = true;
        }
    }

    // you can have mesh names or a namescheme to describe mesh names
    // you can have mesh types or a single mesh type
    // these two options are independent of each other, so we must support
    // all four cases.

    if (nameschemes)
    {
        root_node[multimesh_name]["namescheme"]["block"].set(mmesh_ptr->block_ns);
        // file nameschemes are optional
        if (nullptr != mmesh_ptr->file_ns)
        {
            root_node[multimesh_name]["namescheme"]["file"].set(mmesh_ptr->file_ns);
        }
        // list of empty domains is optional
        if (nullptr != mmesh_ptr->empty_list && 0 < mmesh_ptr->empty_cnt)
        {
            root_node[multimesh_name]["namescheme"]["empty_list"].set(DataType::index_t(mmesh_ptr->empty_cnt));
            index_t_array empty_list = root_node[multimesh_name]["namescheme"]["empty_list"].value();
            for (int empty_id = 0; empty_id < mmesh_ptr->empty_cnt; empty_id ++)
            {
                // save the empty_list elements
                empty_list[empty_id] = mmesh_ptr->empty_list[empty_id];
            }
        }
    }
    else
    {
        for (int block_id = 0; block_id < nblocks; block_id ++)
        {
            // save the mesh name
            Node &mesh_path = root_node[multimesh_name]["mesh_paths"].append();
            mesh_path.set(mmesh_ptr->meshnames[block_id]);
        }
    }
    
    if (nullptr == mmesh_ptr->meshtypes)
    {
        // if we do not have meshtypes, we can assume we either have a single
        // mesh type or we have invalid data. We have no way of figuring out
        // if the provided single mesh type is a valid mesh type yet, so we
        // assume the best.
        root_node[multimesh_name]["single_mesh_type"].set(mmesh_ptr->block_type);
    }
    else
    {
        root_node[multimesh_name]["mesh_types"].set(DataType::index_t(nblocks));
        index_t_array mesh_types = root_node[multimesh_name]["mesh_types"].value();
        for (int block_id = 0; block_id < nblocks; block_id ++)
        {
            // save the mesh type
            mesh_types[block_id] = mmesh_ptr->meshtypes[block_id];
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
void
read_multivars(DBtoc *toc,
               DBfile *dbfile,
               const std::string &multimesh_name,
               const int &nblocks,
               Node &root_node)
{
    // iterate thru the multivars and find the ones that are associated with
    // the chosen multimesh
    for (int multivar_id = 0; multivar_id < toc->nmultivar; multivar_id ++)
    {
        const std::string multivar_name = toc->multivar_names[multivar_id];
        detail::SiloObjectWrapper<DBmultivar, decltype(&DBFreeMultivar)> multivar{
            DBGetMultivar(dbfile, multivar_name.c_str()),
            &DBFreeMultivar};
        DBmultivar *mvar_ptr = multivar.getSiloObject();
        if (nullptr == mvar_ptr)
        {
            CONDUIT_INFO("Error opening multivar " << multivar_name << ". Skipping.");
            continue;
        }

        // does this variable use nameschemes?
        bool nameschemes = false;
        if (nullptr == mvar_ptr->varnames)
        {
            // if we do not have varnames, then we are either using nameschemes
            // or our mvar is invalid
            if (nullptr == mvar_ptr->block_ns)
            {
                CONDUIT_INFO("Multivar " << multivar_name << 
                             " is missing var names and namescheme specifiers. Skipping.");
                continue;
            }
            else
            {
                nameschemes = true;
            }
        }

        // is this multivar associated with a multimesh?
        bool multimesh_assoc = false;

        // there are two cases:
        // 1. the multivar is directly associated with a multimesh
        // 2. the components of the multivar are associated with components of a multimesh

        // we begin with the second case:
        if (nullptr == mvar_ptr->mmesh_name)
        {
            // This multivar has no associated multimesh.
            // We will assume it is associated with the multimesh
            // And then check later when we are actually reading vars
            multimesh_assoc = true;
        }
        // and then the first case
        else if (mvar_ptr->mmesh_name == multimesh_name)
        {
            multimesh_assoc = true;
        }

        if (! multimesh_assoc)
        {
            CONDUIT_INFO("Multivar " << multivar_name << " is not associated " <<
                         "with multimesh " << multimesh_name << ". Skipping.");
            continue;
        }

        if (mvar_ptr->nvars != nblocks)
        {
            CONDUIT_INFO("Domain count mismatch between multivar " +
                         multivar_name + " and multimesh " +
                         multimesh_name + ". Skipping.");
            continue;
        }

        // create the variable now that we have passed all skip cases
        Node &var = root_node[multimesh_name]["vars"][multivar_name];

        // you can have var names or a namescheme to describe var names
        // you can have var types or a single var type
        // these two options are independent of each other, so we must support
        // all four cases.

        if (nameschemes)
        {
            var["namescheme"]["block"].set(mvar_ptr->block_ns);
            // file nameschemes are optional
            if (nullptr != mvar_ptr->file_ns)
            {
                var["namescheme"]["file"].set(mvar_ptr->file_ns);
            }
            // list of empty domains is optional
            if (nullptr != mvar_ptr->empty_list && 0 < mvar_ptr->empty_cnt)
            {
                var["namescheme"]["empty_list"].set(DataType::index_t(mvar_ptr->empty_cnt));
                index_t_array empty_list = var["namescheme"]["empty_list"].value();
                for (int empty_id = 0; empty_id < mvar_ptr->empty_cnt; empty_id ++)
                {
                    // save the empty_list elements
                    empty_list[empty_id] = mvar_ptr->empty_list[empty_id];
                }
            }
        }
        else
        {
            for (int block_id = 0; block_id < nblocks; block_id ++)
            {
                // save the var name
                Node &var_path = var["var_paths"].append();
                var_path.set(mvar_ptr->varnames[block_id]);
            }
        }
        
        if (nullptr == mvar_ptr->vartypes)
        {
            // if we do not have vartypes, we can assume we either have a single
            // var type or we have invalid data. We have no way of figuring out
            // if the provided single var type is a valid var type yet, so we
            // assume the best.
            var["single_var_type"].set(mvar_ptr->block_type);
        }
        else
        {
            var["var_types"].set(DataType::index_t(nblocks));
            index_t_array var_types = var["var_types"].value();
            for (int block_id = 0; block_id < nblocks; block_id ++)
            {
                // save the var type
                var_types[block_id] = mvar_ptr->vartypes[block_id];
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
read_multimats(DBtoc *toc,
               DBfile *dbfile,
               const std::string &multimesh_name,
               const int nblocks,
               Node &root_node)
{
    // iterate thru the multimats and find the ones that are associated with
    // the chosen multimesh
    for (int multimat_id = 0; multimat_id < toc->nmultimat; multimat_id ++)
    {
        const std::string multimat_name = toc->multimat_names[multimat_id];
        detail::SiloObjectWrapper<DBmultimat, decltype(&DBFreeMultimat)> multimat_obj{
            DBGetMultimat(dbfile, multimat_name.c_str()),
            &DBFreeMultimat};
        DBmultimat *multimat_ptr = multimat_obj.getSiloObject();
        if (nullptr == multimat_ptr)
        {
            CONDUIT_INFO("Error opening Multimat " << multimat_name << ". Skipping.");
            continue;
        }

        // does this material use nameschemes?
        bool nameschemes = false;
        if (nullptr == multimat_ptr->matnames)
        {
            // if we do not have matnames, then we are either using nameschemes
            // or our mmat is invalid
            if (nullptr == multimat_ptr->block_ns)
            {
                CONDUIT_INFO("Multimat " << multimat_name << 
                             " is missing material names and namescheme specifiers. Skipping.");
                continue;
            }
            else
            {
                nameschemes = true;
            }
        }

        // is this multimat associated with a multimesh?
        bool multimesh_assoc = false;

        // there are two cases:
        // 1. the multimat is directly associated with a multimesh
        // 2. the components of the multimat are associated with components of a multimesh

        // we begin with the second case:
        if (!multimat_ptr->mmesh_name)
        {
            // This multimat has no associated multimesh.
            // We will assume it is associated with the multimesh
            // And then check later when we are actually reading materials
            multimesh_assoc = true;
        }
        // and then the first case
        else if (multimat_ptr->mmesh_name == multimesh_name)
        {
            multimesh_assoc = true;
        }

        if (! multimesh_assoc)
        {
            CONDUIT_INFO("MultiMaterial " << multimat_name << " is not associated " <<
                         "with multimesh " << multimesh_name << ". Skipping.");
            continue;
        }

        if (multimat_ptr->allowmat0 != 0)
        {
            CONDUIT_INFO("MultiMaterial " << multimat_name <<
                         " for multimesh " << multimesh_name <<
                         " may contain zones with no materials defined on them." <<
                         " We currently do not support this case. Either contact a Conduit developer" <<
                         " or disable DBOPT_ALLOWMAT0 in calls to DBPutMaterial()." <<
                         " Skipping this MultiMaterial.");
            continue;
        }

        if (multimat_ptr->nmats != nblocks)
        {
            CONDUIT_INFO("Domain count mismatch between multimat " +
                         multimat_name + " and multimesh " +
                         multimesh_name + ". Skipping.");
            continue;
        }

        // create the material now that we have passed all skip cases
        Node &material = root_node[multimesh_name]["matsets"][multimat_name];

        // you can have mat names or a namescheme to describe mat names
        if (nameschemes)
        {
            material["namescheme"]["block"].set(multimat_ptr->block_ns);
            // file nameschemes are optional
            if (nullptr != multimat_ptr->file_ns)
            {
                material["namescheme"]["file"].set(multimat_ptr->file_ns);
            }
            // list of empty domains is optional
            if (nullptr != multimat_ptr->empty_list && 0 < multimat_ptr->empty_cnt)
            {
                material["namescheme"]["empty_list"].set(DataType::index_t(multimat_ptr->empty_cnt));
                index_t_array empty_list = material["namescheme"]["empty_list"].value();
                for (int empty_id = 0; empty_id < multimat_ptr->empty_cnt; empty_id ++)
                {
                    // save the empty_list elements
                    empty_list[empty_id] = multimat_ptr->empty_list[empty_id];
                }
            }
        }
        else
        {
            for (int block_id = 0; block_id < nblocks; block_id ++)
            {
                // save the mat name
                Node &mat_path = material["matset_paths"].append();
                mat_path.set(multimat_ptr->matnames[block_id]);
            }
        }

        // reconstruct material map, if possible
        int nmatnos = multimat_ptr->nmatnos;
        int *matnos = multimat_ptr->matnos;
        char **matnames = multimat_ptr->material_names;
        if (nmatnos > 0 && matnos)
        {
            Node &material_map = material["material_map"];
            if (nullptr != matnames)
            {
                for (int i = 0; i < nmatnos; i ++)
                {
                    material_map[matnames[i]] = matnos[i];
                }
            }
            else
            {
                for (int i = 0; i < nmatnos; i ++)
                {
                    material_map[std::to_string(matnos[i])] = matnos[i];
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
read_multimatspecs(DBtoc *toc,
                   DBfile *dbfile,
                   const std::string &multimesh_name,
                   const int nblocks,
                   Node &root_node)
{
    // iterate thru the multimatspecs and find the ones that are associated with
    // the chosen multimesh
    for (int mmatspec_id = 0; mmatspec_id < toc->nmultimatspecies; mmatspec_id ++)
    {
        const std::string multimatspec_name = toc->multimatspecies_names[mmatspec_id];
        detail::SiloObjectWrapper<DBmultimatspecies, decltype(&DBFreeMultimatspecies)> multimatspec_obj{
            DBGetMultimatspecies(dbfile, multimatspec_name.c_str()),
            &DBFreeMultimatspecies};
        DBmultimatspecies *multimatspec_ptr = multimatspec_obj.getSiloObject();
        if (nullptr == multimatspec_ptr)
        {
            CONDUIT_INFO("Error opening Multimatspecies " << multimatspec_name << ". Skipping.");
            continue;
        }

        // does this specset use nameschemes?
        bool nameschemes = false;
        if (nullptr == multimatspec_ptr->specnames)
        {
            // if we do not have specnames, then we are either using nameschemes
            // or our mmatspec is invalid
            if (nullptr == multimatspec_ptr->block_ns)
            {
                CONDUIT_INFO("Multimatspecies " << multimatspec_name << 
                             " is missing material species names and namescheme specifiers. Skipping.");
                continue;
            }
            else
            {
                nameschemes = true;
            }
        }

        // we can't check multimesh association at this point so we hope
        // for the best

        if (multimatspec_ptr->nspec != nblocks)
        {
            CONDUIT_INFO("Domain count mismatch between multimatspec " +
                         multimatspec_name + " and multimesh " +
                         multimesh_name + ". Skipping.");
            continue;
        }

        // create the specset now that we have passed all skip cases
        Node &species_set = root_node[multimesh_name]["specsets"][multimatspec_name];

        // you can have spec names or a namescheme to describe spec names
        if (nameschemes)
        {
            species_set["namescheme"]["block"].set(multimatspec_ptr->block_ns);
            // file nameschemes are optional
            if (nullptr != multimatspec_ptr->file_ns)
            {
                species_set["namescheme"]["file"].set(multimatspec_ptr->file_ns);
            }
            // list of empty domains is optional
            if (nullptr != multimatspec_ptr->empty_list && 0 < multimatspec_ptr->empty_cnt)
            {
                species_set["namescheme"]["empty_list"].set(DataType::index_t(multimatspec_ptr->empty_cnt));
                index_t_array empty_list = species_set["namescheme"]["empty_list"].value();
                for (int empty_id = 0; empty_id < multimatspec_ptr->empty_cnt; empty_id ++)
                {
                    // save the empty_list elements
                    empty_list[empty_id] = multimatspec_ptr->empty_list[empty_id];
                }
            }
        }
        else
        {
            for (int block_id = 0; block_id < nblocks; block_id ++)
            {
                // save the spec name
                Node &spec_path = species_set["specset_paths"].append();
                spec_path.set(multimatspec_ptr->specnames[block_id]);
            }
        }

        // gather species set information, if possible
        const int nmat = multimatspec_ptr->nmat;
        const int *nmatspec = multimatspec_ptr->nmatspec;
        char **species_names = multimatspec_ptr->species_names;
        if (0 != nmat)
        {
            // there's not much we can do without material names
            // so we save the info that is here as it is
            if (NULL != nmatspec)
            {
                species_set["nmatspec"].set(nmatspec, nmat);
                if (NULL != species_names)
                {
                    const int num_specs = [&]()
                    {
                        int sum = 0;
                        for (int mat_id = 0; mat_id < nmat; mat_id ++)
                        {
                            sum += nmatspec[mat_id];
                        }
                        return sum;
                    }();
                    for (int specname_id = 0; specname_id < num_specs; specname_id ++)
                    {
                        Node &spec_name = species_set["species_names"].append();
                        spec_name.set(species_names[specname_id]);
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
read_state(DBfile *dbfile,
           Node &root_node,
           const std::string &multimesh_name)
{
    // TODO every silo call should have a CONDUIT_CHECK_SILO_ERROR around it

    // look first for dtime, then time if dtime is not found like in VisIt
    if (DBInqVarExists(dbfile, "dtime"))
    {
        double dtime;
        DBReadVar(dbfile, "dtime", &dtime);
        root_node[multimesh_name]["state"]["time"] = dtime;
    }
    else if (DBInqVarExists(dbfile, "time"))
    {
        float ftime;
        DBReadVar(dbfile, "time", &ftime);
        root_node[multimesh_name]["state"]["time"] = (double) ftime;
    }

    if (DBInqVarExists(dbfile, "cycle"))
    {
        int cycle;
        DBReadVar(dbfile, "cycle", &cycle);
        root_node[multimesh_name]["state"]["cycle"] = cycle;
    }
}

//-----------------------------------------------------------------------------
void
read_var_attributes(DBfile *dbfile,
                    const std::string &multimesh_name,
                    Node &root_node)
{
    if (! root_node[multimesh_name].has_child("vars"))
    {
        // nothing to do if there are no vars
        return;
    }

    const std::string var_attr_name = "VAR_ATTRIBUTES";
    if (! DBInqVarExists(dbfile, var_attr_name.c_str()))
    {
        // The var attributes are not present. They are optional, so we can return early
        return;
    }
    if (DBInqVarType(dbfile, var_attr_name.c_str()) != DB_ARRAY)
    {
        // The var attributes are the wrong type. They are optional, so we can return early
        return;
    }
    detail::SiloObjectWrapper<DBcompoundarray, decltype(&DBFreeCompoundarray)> var_attr_obj{
        DBGetCompoundarray(dbfile, var_attr_name.c_str()),
        &DBFreeCompoundarray};
    DBcompoundarray *var_attr = var_attr_obj.getSiloObject();
    if (! var_attr)
    {
        // we failed to read the variable attributes. We can skip them.
        return;
    }

    // fetch pointers to elements inside the compound array
    char **elemnames = var_attr->elemnames;
    int *elemlengths = var_attr->elemlengths;
    int nelems       = var_attr->nelems;
    int *values      = static_cast<int *>(var_attr->values);
    int datatype     = var_attr->datatype;

    if (datatype != DB_INT)
    {
        // for overlink, the var attributes must contain integer data
        // we don't want to complain however, since we don't know if we are doing overlink
        // or not at this point.
        return;
    }

    // a map from field names (strings) to whether or not it is volume dependent (bools)
    std::map<std::string, bool> field_vol_dep;

    // next we fill our map
    int currpos = 0;
    for (int i = 0; i < nelems; i ++)
    {
        const std::string varname = elemnames[i];
        const int elemlength = elemlengths[i];
        const int scaling_property = values[currpos + 1]; // + 1 b/c scaling property is stored
        // in the 2nd position

        // scaling property: ATTR_INTENSIVE 0, ATTR_EXTENSIVE 1
        // intensive (0) IS NOT volume dependent
        // extensive (1) IS volume dependent
        const bool vol_dep = scaling_property != 0;

        field_vol_dep[varname] = vol_dep;

        currpos += elemlength;
    }

    // finally we use our map to put information into our fields
    Node &root_fields = root_node[multimesh_name]["vars"];
    for (auto const &mapitem : field_vol_dep)
    {
        std::string fieldname = mapitem.first;
        std::string volume_dependent = mapitem.second ? "true" : "false";

        // we only want this information for variables that we have read
        if (root_fields.has_child(fieldname))
        {
            root_fields[fieldname]["volume_dependent"] = volume_dependent;
        }
    }
}

//-----------------------------------------------------------------------------
bool
read_root_silo_index(const std::string &root_file_path,
                     const Node &opts,
                     Node &root_node, // output
                     std::string &mesh_name_to_read, // output
                     std::ostringstream &error_oss, // output
                     std::map<std::string, DBfile*> &file_map) // output
{
    // clear output vars
    root_node.reset();
    mesh_name_to_read = "";
    error_oss.str("");

    // first, make sure we can open the root file
    if (! is_silo_file(root_file_path))
    {
        error_oss << "failed to open root file: " << root_file_path;
        return false;
    }

    // open silo file
    file_map[root_file_path] = silo_open_file_for_read(root_file_path);
    if (! file_map.at(root_file_path))
    {
        error_oss << "Error opening Silo file for reading: " << root_file_path;
        return false;
    }

    // get table of contents
    DBtoc *toc = DBGetToc(file_map.at(root_file_path)); // shouldn't be free'd
    if (!toc)
    {
        error_oss << "Table of contents could not be extracted from file: " << root_file_path;
        return false;
    }

    // decide what multimesh to extract
    if (opts.has_child("mesh_name") && opts["mesh_name"].dtype().is_string())
    {
        mesh_name_to_read = opts["mesh_name"].as_string();
    }

    // we want to know the mesh type so it is easy to read later
    int mesh_type = -1;

    // check mesh name
    if (mesh_name_to_read.empty())
    {
        // if not specified, we will default to first multimesh we find
        if (toc->nmultimesh > 0)
        {
            mesh_name_to_read = toc->multimesh_names[0];
            mesh_type = DB_MULTIMESH;
        }
        // otherwise we will try quadmeshes
        else if (toc->nqmesh > 0)
        {
            mesh_name_to_read = toc->qmesh_names[0];
            mesh_type = DB_QUADMESH;
        }
        // otherwise we will try ucdmeshes
        else if (toc->nucdmesh > 0)
        {
            mesh_name_to_read = toc->ucdmesh_names[0];
            mesh_type = DB_UCDMESH;
        }
        // otherwise we will try ptmeshes
        else if (toc->nptmesh > 0)
        {
            mesh_name_to_read = toc->ptmesh_names[0];
            mesh_type = DB_POINTMESH;
        }
        else
        {
            error_oss << "No meshes found in file: " << root_file_path;
            return false;
        }
    }
    else
    {
        bool found = false;
        auto check_mesh_name_in_file = [&](const int nmeshes,
                                           char **mesh_names,
                                           const int curr_mesh_type)
        {
            for (int i = 0; i < nmeshes; i ++)
            {
                if (mesh_names[i] == mesh_name_to_read)
                {
                    found = true;
                    mesh_type = curr_mesh_type;
                    break;
                }
            }
        };
        check_mesh_name_in_file(toc->nmultimesh, toc->multimesh_names, DB_MULTIMESH);
        check_mesh_name_in_file(toc->nqmesh, toc->qmesh_names, DB_QUADMESH);
        check_mesh_name_in_file(toc->nucdmesh, toc->ucdmesh_names, DB_UCDMESH);
        check_mesh_name_in_file(toc->nptmesh, toc->ptmesh_names, DB_POINTMESH);

        if (!found)
        {
            error_oss << "No mesh found matching " << mesh_name_to_read;
            return false;
        }
    }

    // Get the selected matset flavor
    std::string opts_matset_style = "";
    if (opts.has_child("matset_style") && opts["matset_style"].dtype().is_string())
    {
        opts_matset_style = opts["matset_style"].as_string();
        if (opts_matset_style != "default" &&
            opts_matset_style != "multi_buffer_full" &&
            opts_matset_style != "sparse_by_element" &&
            opts_matset_style != "multi_buffer_by_material")
        {
            error_oss << "read_mesh invalid matset_style option: \""
                         << opts_matset_style << "\"\n"
                         " expected: \"default\", \"multi_buffer_full\", "
                         "\"sparse_by_element\", or \"multi_buffer_by_material\"";
            return false;
        }
    }

    auto prep_simple_silo_obj_metadata = [&](const int num_vars,
                                             char **var_names,
                                             const int var_type,
                                             const int num_mats,
                                             char **mat_names,
                                             const int num_specsets,
                                             char **spec_names)
    {
        root_node[mesh_name_to_read]["nblocks"] = 1;
        root_node[mesh_name_to_read]["single_mesh_type"].set(mesh_type);
        root_node[mesh_name_to_read]["mesh_paths"].append().set(mesh_name_to_read);

        // TODO should we check here if vars are associated with this mesh?
        // we have logic to get the right ones later, but it could be quick to check now
        for (int i = 0; i < num_vars; i ++)
        {
            const std::string var_name = var_names[i];
            Node &var = root_node[mesh_name_to_read]["vars"][var_name];
            var["single_var_type"].set(var_type);
            var["var_paths"].append().set(var_name);
        }
        // TODO should we check here if materials are associated with this mesh?
        // we have logic to get the right one later, but it could be quick to check now
        for (int i = 0; i < num_mats; i ++)
        {
            const std::string mat_name = mat_names[i];
            Node &material = root_node[mesh_name_to_read]["matsets"][mat_name];
            material["matset_paths"].append().set(mat_name);
        }

        // TODO should we check here if specsets are associated with this mesh?
        // we have logic to get the right one later, but it could be quick to check now
        for (int i = 0; i < num_specsets; i ++)
        {
            const std::string spec_name = spec_names[i];
            Node &specset = root_node[mesh_name_to_read]["specsets"][spec_name];
            specset["specset_paths"].append().set(spec_name);
        }
    };

    if (DB_MULTIMESH == mesh_type)
    {
        int nblocks;
        if (! read_multimesh(file_map.at(root_file_path),
                             mesh_name_to_read,
                             nblocks,
                             root_node,
                             error_oss))
        {
            return false;
        }
        read_multivars(toc,
                       file_map.at(root_file_path),
                       mesh_name_to_read,
                       nblocks,
                       root_node);
        read_multimats(toc,
                       file_map.at(root_file_path),
                       mesh_name_to_read,
                       nblocks,
                       root_node);
        read_multimatspecs(toc,
                           file_map.at(root_file_path),
                           mesh_name_to_read,
                           nblocks,
                           root_node);
        // overlink-specific
        read_var_attributes(file_map.at(root_file_path),
                            mesh_name_to_read,
                            root_node);
    }
    else if (DB_QUADMESH == mesh_type)
    {
        prep_simple_silo_obj_metadata(toc->nqvar, toc->qvar_names, DB_QUADVAR,
                                      toc->nmat, toc->mat_names,
                                      toc->nmatspecies, toc->matspecies_names);
    }
    else if (DB_UCDMESH == mesh_type)
    {
        prep_simple_silo_obj_metadata(toc->nucdvar, toc->ucdvar_names, DB_UCDVAR,
                                      toc->nmat, toc->mat_names,
                                      toc->nmatspecies, toc->matspecies_names);
    }
    else if (DB_POINTMESH == mesh_type)
    {
        prep_simple_silo_obj_metadata(toc->nptvar, toc->ptvar_names, DB_POINTVAR,
                                      toc->nmat, toc->mat_names,
                                      toc->nmatspecies, toc->matspecies_names);
    }
    else
    {
        error_oss << "Unknown mesh type for mesh " << mesh_name_to_read;
        return false;
    }

    read_state(file_map.at(root_file_path), root_node, mesh_name_to_read);

    if (! opts_matset_style.empty())
    {
        root_node[mesh_name_to_read]["matset_style"] = opts_matset_style;
    }

    return true;

    // our silo index should look like this:

    // mesh:
    //    state:
    //       cycle: 100
    //       time: 10
    //       dtime: 10
    //    nblocks: 5
    //    // we will either have 'mesh_paths' or 'namescheme', never both
    //    mesh_paths:
    //       - "domain_000000.silo:mesh"
    //       - "domain_000001.silo:mesh"
    //         ...
    //    namescheme:
    //       block: "|/domain%d/mesh|#domfiles[n]"
    //       file: "|domain_%06d.silo|#procs[n]" // (optional)
    //       empty_list: [5, 7, 9] // (optional)
    //    // we will either have 'mesh_types' or 'single_mesh_type', never both
    //    mesh_types: [DB_UCDMESH, DB_UCDMESH, ...]
    //    single_mesh_type: [DB_UCDMESH]
    //    vars:
    //       field:
    //          // we will either have 'var_paths' or 'namescheme', never both
    //          var_paths:
    //             - "domain_000000.silo:field"
    //             - "domain_000001.silo:field"
    //               ...
    //          namescheme:
    //             block: "|/domain%d/field|#domfiles[n]"
    //             file: "|domain_%06d.silo|#procs[n]" // (optional)
    //             empty_list: [5, 7, 9] // (optional)
    //          // we will either have 'var_types' or 'single_var_type', never both
    //          var_types: [DB_UCDVAR, DB_UCDVAR, ...]
    //          single_var_type: [DB_UCDVAR]
    //          volume_dependent: "false" // (optional) this can be provided with overlink var attributes
    //       ...
    //    matsets:
    //       material:
    //          // we will either have 'matset_paths' or 'namescheme', never both
    //          matset_paths:
    //             - "domain_000000.silo:material"
    //             - "domain_000001.silo:material"
    //               ...
    //          namescheme:
    //             block: "|/domain%d/material|#domfiles[n]"
    //             file: "|domain_%06d.silo|#procs[n]" // (optional)
    //             empty_list: [5, 7, 9] // (optional)
    //          material_map: // (optional) this can be reconstructed if dboptions are present
    //             a: 1
    //             b: 2
    //             c: 0
    //             ...
    //       ...
    //    specsets:
    //       specset:
    //          // we will either have 'specset_paths' or 'namescheme', never both
    //          specset_paths:
    //             - "domain_000000.silo:species"
    //             - "domain_000001.silo:species"
    //               ...
    //          namescheme:
    //             block: "|/domain%d/species|#domfiles[n]"
    //             file: "|domain_%06d.silo|#procs[n]" // (optional)
    //             empty_list: [5, 7, 9] // (optional)
    //          species_names: // (optional together with nmatspec)
    //             - "energon"
    //             - "unobtanium"
    //             - "tibanna gas"
    //             - "wood"
    //             - "coin"
    //             - "food"
    //             - "stone"
    //             - "favor"
    //               ...
    //          nmatspec: [3, 5, ...] // (optional together with species_names)
    //       ...
    //    matset_style: "default", OR "multi_buffer_full", OR "sparse_by_element", OR "multi_buffer_by_material"
}

//-----------------------------------------------------------------------------
///
/// opts:
///      mesh_name: "{name}"
///          provide explicit mesh name, for cases where silo data includes
///          more than one mesh.
///          We only allow reading of a single mesh to keep these options on
///          par with the relay io blueprint options.
///
///      matset_style: "default", "multi_buffer_full", "sparse_by_element",
///            "multi_buffer_by_material"
///            "default"   ==> "sparse_by_element"
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API
read_mesh(const std::string &root_file_path,
          const Node &opts,
          Node &mesh
          CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    int par_rank = 0;
#if CONDUIT_RELAY_IO_MPI_ENABLED
    par_rank = relay::mpi::rank(mpi_comm);
    int par_size = relay::mpi::size(mpi_comm);
#endif

    int error = 0;
    std::ostringstream error_oss;
    std::string mesh_name_to_read;
    Node root_node;

    // a map from filenames to dbfiles (file handles)
    std::map<std::string, DBfile*> file_map;
    file_map.emplace(root_file_path, nullptr);

    // only read bp index on rank 0
    if (0 == par_rank)
    {
        if (! read_root_silo_index(root_file_path,
                                   opts,
                                   root_node,
                                   mesh_name_to_read,
                                   error_oss,
                                   file_map))
        {
            error = 1;
        }
    }

#if CONDUIT_RELAY_IO_MPI_ENABLED
    Node n_local, n_global;
    n_local.set((int)error);
    relay::mpi::sum_all_reduce(n_local,
                               n_global,
                               mpi_comm);

    error = n_global.as_int();

    if (1 == error)
    {
        // we have a problem, broadcast string message
        // from rank 0 all ranks can throw an error
        n_global.set(error_oss.str());
        conduit::relay::mpi::broadcast_using_schema(n_global,
                                                    0,
                                                    mpi_comm);

        CONDUIT_ERROR(n_global.as_string());
    }
    else
    {
        // broadcast the mesh name and the root node
        // from rank 0 to all ranks
        n_global.set(mesh_name_to_read);
        conduit::relay::mpi::broadcast_using_schema(n_global,
                                                    0,
                                                    mpi_comm);
        mesh_name_to_read = n_global.as_string();
        conduit::relay::mpi::broadcast_using_schema(root_node,
                                                    0,
                                                    mpi_comm);
    }
#else
    // non MPI case, throw error
    if (1 == error)
    {
        CONDUIT_ERROR(error_oss.str());
    }
#endif

    // we need the root file so we can create nameschemes on every rank
    if (0 != par_rank)
    {
        file_map[root_file_path] = silo_open_file_for_read(root_file_path);
        if (! file_map.at(root_file_path))
        {
            // this is really bad.
            // TODO parallel error checking
            CONDUIT_ERROR("Failed to open root file on rank " << par_rank);
        }
    }

    const Node &mesh_index = root_node[mesh_name_to_read];

    // read all domains for given mesh
    const int num_domains = mesh_index["nblocks"].to_int();

    std::ostringstream oss;
    int domain_start = 0;
    int domain_end = num_domains;

#if CONDUIT_RELAY_IO_MPI_ENABLED

    int read_size = num_domains / par_size;
    int rem = num_domains % par_size;
    if(par_rank < rem)
    {
        read_size ++;
    }

    Node n_read_size;
    Node n_doms_per_rank;

    n_read_size.set_int32(read_size);

    relay::mpi::all_gather_using_schema(n_read_size,
                                        n_doms_per_rank,
                                        mpi_comm);
    int *counts = static_cast<int *>(n_doms_per_rank.data_ptr());

    int rank_offset = 0;
    for(int i = 0; i < par_rank; ++i)
    {
        rank_offset += counts[i];
    }

    domain_start = rank_offset;
    domain_end = rank_offset + read_size;
#endif

    const std::string opts_matset_style = (mesh_index.has_child("matset_style") ?
        mesh_index["matset_style"].as_string() : "default");

    //
    // Create name generators for the mesh and each variable, matset, specset
    //
    SiloNameGenerator *mesh_path_gen = new SiloNameGenerator(file_map.at(root_file_path),
                                                             num_domains,
                                                             mesh_index,
                                                             "mesh_paths");
    std::map<std::string, SiloNameGenerator*> mat_path_gen;
    std::map<std::string, SiloNameGenerator*> spec_path_gen;
    std::map<std::string, SiloNameGenerator*> var_path_gen;
    detail::name_generator_tools::populate_path_gen_map(file_map.at(root_file_path),
                                                        num_domains,
                                                        mesh_index,
                                                        "matsets",
                                                        "matset_paths",
                                                        mat_path_gen);
    detail::name_generator_tools::populate_path_gen_map(file_map.at(root_file_path),
                                                        num_domains,
                                                        mesh_index,
                                                        "specsets",
                                                        "specset_paths",
                                                        spec_path_gen);
    detail::name_generator_tools::populate_path_gen_map(file_map.at(root_file_path),
                                                        num_domains,
                                                        mesh_index,
                                                        "vars",
                                                        "var_paths",
                                                        var_path_gen);

    std::string root_file_name, relative_dir;
    utils::rsplit_file_path(root_file_path, root_file_name, relative_dir);
    // example:
    //    root_file_path = "/path/to/silo/myrootfile.root"
    //    relative_dir   = "/path/to/silo/"
    //    root_file_name = "myrootfile.root"

    // If the root file is named OvlTop.silo, then there is a very good chance that
    // this file is valid overlink. Therefore, we must modify the paths we get from
    // the root node to reflect this.
    bool ovltop_case = (root_file_name == "OvlTop.silo");

    //
    // Main Loop
    //
    for (int domain_id = domain_start; domain_id < domain_end; domain_id ++)
    {
        //
        // Read Mesh
        //

        const std::string silo_mesh_path = mesh_path_gen->Name(domain_id);
        const int meshtype = [&]() -> int
        {
            // it is either one or the other
            if (mesh_index.has_child("mesh_types"))
            {
                const int_accessor meshtypes = mesh_index["mesh_types"].value();
                return meshtypes[domain_id];
            }
            else // (mesh_index.has_child("single_mesh_type"))
            {
                return mesh_index["single_mesh_type"].as_int();
            }
        }();

        std::string mesh_name, mesh_domain_filename;
        detail::name_generator_tools::generate_paths(silo_mesh_path,
                                                     relative_dir,
                                                     mesh_domain_filename,
                                                     mesh_name);
        // example:
        //    silo_mesh_path:       "domain_000000.silo:path/to/mesh"
        //    relative_dir:         "/path/to/silo/"
        //    mesh_domain_filename: "/path/to/silo/domain_000000.silo"
        //    mesh_name:            "path/to/mesh"
        // example2:
        //    silo_mesh_path:       "path/to/mesh"
        //    relative_dir:         "/path/to/silo/"
        //    mesh_domain_filename: ""
        //    mesh_name:            "path/to/mesh"

        if ("EMPTY" == mesh_name)
        {
            continue; // skip this domain
        }

        std::string bottom_level_mesh_name, tmp;
        conduit::utils::rsplit_file_path(mesh_name, "/", bottom_level_mesh_name, tmp);
        // example:
        //    mesh_name:              "path/to/mesh"
        //    bottom_level_mesh_name: "mesh"
        //    tmp:                    "path/to/"

        // root only case
        if (mesh_domain_filename.empty())
        {
            mesh_domain_filename = root_file_path;
            // we are in the root file only case so overlink is not possible
            ovltop_case = false;
        }

        DBfile *mesh_domain_file_to_use =
            open_or_reuse_file(ovltop_case,
                               mesh_domain_filename,
                               file_map);

        // this is for the blueprint mesh output
        const std::string domain_path = conduit_fmt::format("domain_{:06d}", domain_id);

        if (! read_mesh_domain(meshtype,
                               mesh_domain_file_to_use,
                               mesh_name,
                               mesh_name_to_read,
                               domain_path,
                               mesh))
        {
            continue; // we hit a case where we want to skip this mesh domain
        }

        // we know we were for sure successful (we didn't skip ahead to the next domain)
        // so we create the mesh_out now for good
        Node &mesh_out = mesh[domain_path];

        //
        // Read State
        //

        mesh_out["state"]["domain_id"] = static_cast<index_t>(domain_id);
        if (mesh_index.has_path("state/time"))
        {
            mesh_out["state"]["time"] = mesh_index["state"]["time"].as_double();
        }
        if (mesh_index.has_path("state/cycle"))
        {
            mesh_out["state"]["cycle"] = (index_t) mesh_index["state"]["cycle"].as_int();
        }

        //
        // Read Adjset (overlink only)
        //

        read_adjset(mesh_domain_file_to_use, mesh_name_to_read, domain_id, mesh_out);

        //
        // Read Materials
        //

        // We only read a single material. Reasoning is explained below.

        // This node will house the recipe for reconstructing matset_values
        // from silo mixvals.
        Node matset_field_reconstruction;

        // This node will house the silo representation of the matset, for use
        // in reading species sets.
        Node silo_material;

        // for each mesh domain, we would like to iterate through all the materials
        // and extract the same domain from them.
        if (mesh_index.has_child("matsets"))
        {
            auto matset_itr = mesh_index["matsets"].children();
            while (matset_itr.has_next())
            {
                const Node &n_matset = matset_itr.next();
                const std::string multimat_name = matset_itr.name();
                const std::string silo_matset_path = 
                    mat_path_gen.at(multimat_name)->Name(domain_id);

                std::string matset_name, matset_domain_filename;
                detail::name_generator_tools::generate_paths(silo_matset_path,
                                                             relative_dir,
                                                             matset_domain_filename,
                                                             matset_name);
                // example:
                //    silo_matset_path:       "domain_000000.silo:path/to/matset"
                //    relative_dir:           "/path/to/silo/"
                //    matset_domain_filename: "/path/to/silo/domain_000000.silo"
                //    matset_name:            "path/to/matset"
                // example2:
                //    silo_matset_path:       "path/to/matset"
                //    relative_dir:           "/path/to/silo/"
                //    matset_domain_filename: ""
                //    matset_name:            "path/to/matset"

                if ("EMPTY" == matset_name)
                {
                    // we choose not to write anything to blueprint
                    continue;
                }

                // root only case
                if (matset_domain_filename.empty())
                {
                    matset_domain_filename = root_file_path;
                    // we are in the root file only case so overlink is not possible
                    ovltop_case = false;
                }

                DBfile *matset_domain_file_to_use =
                    open_or_reuse_file(ovltop_case,
                                       matset_domain_filename,
                                       file_map);

                // If this completes successfully, it means we have found a matset
                // associated with this mesh. Thus we can break iteration here,
                // since there can only be one matset. This is the earliest we can
                // break iteration because the silo index may have multiple matsets,
                // and we have no way of knowing until now which one is associated
                // with our mesh.
                // In silo, for each mesh, there can only be one matset, because otherwise
                // it would be ambiguous. In Blueprint, we can allow multiple matsets per
                // topo, because the fields explicitly link to the matset they use.
                if (read_matset_domain(matset_domain_file_to_use,
                                       n_matset,
                                       matset_name,
                                       mesh_name_to_read,
                                       multimat_name,
                                       bottom_level_mesh_name,
                                       opts_matset_style,
                                       matset_field_reconstruction,
                                       silo_material,
                                       mesh_out))
                {
                    break;
                }
            }
        }

        //
        // Read Species Sets
        //

        // for each mesh domain, we would like to iterate through all the species sets
        // and extract the same domain from them.
        if (mesh_index.has_child("specsets"))
        {
            auto specset_itr = mesh_index["specsets"].children();
            while (specset_itr.has_next())
            {
                const Node &n_specset = specset_itr.next();
                const std::string multimatspec_name = specset_itr.name();
                const std::string silo_specset_path = 
                    spec_path_gen.at(multimatspec_name)->Name(domain_id);

                std::string specset_name, specset_domain_filename;
                detail::name_generator_tools::generate_paths(silo_specset_path,
                                                             relative_dir,
                                                             specset_domain_filename,
                                                             specset_name);
                // example:
                //    silo_specset_path:       "domain_000000.silo:path/to/specset"
                //    relative_dir:            "/path/to/silo/"
                //    specset_domain_filename: "/path/to/silo/domain_000000.silo"
                //    specset_name:            "path/to/specset"
                // example2:
                //    silo_specset_path:       "path/to/specset"
                //    relative_dir:            "/path/to/silo/"
                //    specset_domain_filename: ""
                //    specset_name:            "path/to/specset"

                if ("EMPTY" == specset_name)
                {
                    // we choose not to write anything to blueprint
                    continue;
                }

                // root only case
                if (specset_domain_filename.empty())
                {
                    specset_domain_filename = root_file_path;
                    // we are in the root file only case so overlink is not possible
                    ovltop_case = false;
                }

                DBfile *specset_domain_file_to_use =
                    open_or_reuse_file(ovltop_case,
                                       specset_domain_filename,
                                       file_map);

                read_specset_domain(specset_domain_file_to_use,
                                    n_specset,
                                    specset_name,
                                    multimatspec_name,
                                    opts_matset_style,
                                    silo_material,
                                    mesh_out);
            }
        }

        //
        // Read Fields
        //

        // for each mesh domain, we would like to iterate through all the variables
        // and extract the same domain from them.
        if (mesh_index.has_child("vars"))
        {
            auto var_itr = mesh_index["vars"].children();
            while (var_itr.has_next())
            {
                const Node &n_var = var_itr.next();
                const std::string multivar_name = var_itr.name();
                const std::string silo_var_path = 
                    var_path_gen.at(multivar_name)->Name(domain_id);
                const int vartype = [&]() -> int
                {
                    // it is either one or the other
                    if (n_var.has_child("var_types"))
                    {
                        const int_accessor vartypes = n_var["var_types"].value();
                        return vartypes[domain_id];
                    }
                    else // (n_var.has_child("single_var_type"))
                    {
                        return n_var["single_var_type"].as_int();
                    }
                }();

                std::string var_name, var_domain_filename;
                detail::name_generator_tools::generate_paths(silo_var_path,
                                                             relative_dir,
                                                             var_domain_filename,
                                                             var_name);
                // example:
                //    silo_var_path:       "domain_000000.silo:path/to/var"
                //    relative_dir:        "/path/to/silo/"
                //    var_domain_filename: "/path/to/silo/domain_000000.silo"
                //    var_name:            "path/to/var"
                // example2:
                //    silo_var_path:       "path/to/var"
                //    relative_dir:        "/path/to/silo/"
                //    var_domain_filename: ""
                //    var_name:            "path/to/var"

                if ("EMPTY" == var_name)
                {
                    // we choose not to write anything to blueprint
                    continue;
                }

                // this info can be tracked with overlink VAR_ATTRIBUTES
                std::string volume_dependent = "";
                if (n_var.has_child("volume_dependent"))
                {
                    volume_dependent = n_var["volume_dependent"].as_string();
                }

                // root only case
                if (var_domain_filename.empty())
                {
                    var_domain_filename = root_file_path;
                    // we are in the root file only case so overlink is not possible
                    ovltop_case = false;
                }

                DBfile *var_domain_file_to_use =
                    open_or_reuse_file(ovltop_case,
                                       var_domain_filename,
                                       file_map);

                // we don't care if this skips the var or not since this is the
                // last thing in the loop iteration
                read_variable_domain(vartype,
                                     var_domain_file_to_use,
                                     var_name,
                                     mesh_name_to_read,
                                     multivar_name,
                                     bottom_level_mesh_name,
                                     volume_dependent,
                                     opts_matset_style,
                                     matset_field_reconstruction,
                                     mesh_out);
            }
        }

        // close open files (except the root file!)
        for (auto file_map_itr = file_map.begin(); file_map_itr != file_map.end(); )
        {
            // if this file is open and is not the root file
            if (file_map_itr->first != root_file_path && 
                nullptr != file_map_itr->second)
            {
                // close the file
                CONDUIT_ASSERT(0 == DBClose(file_map_itr->second),
                               "Error closing Silo file " << file_map_itr->first);
                file_map_itr->second = nullptr;
            }

            if (file_map_itr->second == nullptr)
            {
                file_map_itr = file_map.erase(file_map_itr);
            }
            else
            {
                file_map_itr ++;
            }
        }
    }

    delete mesh_path_gen;
    mesh_path_gen = nullptr;

    for (auto& pair : mat_path_gen)
    {
        delete pair.second;
    }
    mat_path_gen.clear();
    for (auto& pair : spec_path_gen)
    {
        delete pair.second;
    }
    spec_path_gen.clear();
    for (auto& pair : var_path_gen)
    {
        delete pair.second;
    }
    var_path_gen.clear();

    // clean up open files
    for (auto file_map_itr = file_map.begin(); file_map_itr != file_map.end(); )
    {
        // if this file is open
        if (nullptr != file_map_itr->second)
        {
            // close the file
            CONDUIT_ASSERT(0 == DBClose(file_map_itr->second),
                           "Error closing Silo file " << file_map_itr->first);
            file_map_itr->second = nullptr;
        }
        if (file_map_itr->second == nullptr)
        {
            file_map_itr = file_map.erase(file_map_itr);
        }
        else
        {
            file_map_itr ++;
        }
    }
}

//-----------------------------------------------------------------------------
// The load semantics, the mesh node is reset before reading.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void load_mesh(const std::string &root_file_path,
               Node &mesh
               CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    mesh.reset();

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    read_mesh(root_file_path,
              mesh,
              mpi_comm);
#else
    read_mesh(root_file_path,
              mesh);
#endif
}

//-----------------------------------------------------------------------------
/// opts:
///      mesh_name: "{name}"
///          provide explicit mesh name, for cases where silo data includes
///          more than one mesh.
///          We only allow reading of a single mesh to keep these options on
///          par with the relay io blueprint options.
///
///      matset_style: "default", "multi_buffer_full", "sparse_by_element",
///            "multi_buffer_by_material"
///            "default"   ==> "sparse_by_element"
//-----------------------------------------------------------------------------
void load_mesh(const std::string &root_file_path,
               const Node &opts,
               Node &mesh
               CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    mesh.reset();

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    read_mesh(root_file_path,
              opts,
              mesh,
              mpi_comm);
#else
    read_mesh(root_file_path,
              opts,
              mesh);
#endif
}

//---------------------------------------------------------------------------//
void** prepare_mixed_field_for_write(const Node &n_var,
                                     const Node &mesh_domain,
                                     const std::string &var_name,
                                     const DataType &vals_dtype,
                                     const int &nvars,
                                     bool &convert_to_double_array,
                                     Node &silo_matset,
                                     Node &silo_mixvar_vals_compact,
                                     std::vector<void *> &mixvars_ptrs,
                                     int &mixlen)
{
    if (n_var.has_child("matset"))
    {
        const std::string matset_name = n_var["matset"].as_string();

        CONDUIT_ASSERT(mesh_domain.has_path("matsets/" + matset_name),
            "Missing matset " << matset_name << " for field " << var_name);

        const Node &n_matset = mesh_domain["matsets"][matset_name];
        conduit::blueprint::mesh::field::to_silo(n_var, n_matset, silo_matset);

        DataType mixvar_vals_dtype = silo_matset["field_mixvar_values"].dtype();

        // if we are not already converting to double array, we can do an
        // additional check to see if we need to
        if (! convert_to_double_array)
        {
            // if the field is mixed (has both vals and matset vals) and the types
            // of vals and matset vals DO NOT match, we must convert both to
            // double arrays.
            convert_to_double_array = vals_dtype.id() != mixvar_vals_dtype.id();
        }

        if (convert_to_double_array)
        {
            detail::convert_to_double_array(silo_matset["field_mixvar_values"],
                                            silo_mixvar_vals_compact);
        }
        else
        {
            detail::conditional_compact(silo_matset["field_mixvar_values"],
                                        silo_mixvar_vals_compact);
        }

        if (nvars == 1)
        {
            CONDUIT_ASSERT(! silo_mixvar_vals_compact.dtype().is_object(),
                "Number of variable components is 1 but to_silo did not return a leaf node.");

            mixvars_ptrs[0] = silo_mixvar_vals_compact.data_ptr();
            mixlen = silo_mixvar_vals_compact.dtype().number_of_elements();
        }
        else
        {
            CONDUIT_ASSERT(silo_mixvar_vals_compact.dtype().is_object(),
                "Number of variable components is > 1 but to_silo returned a leaf node.");
            CONDUIT_ASSERT(silo_mixvar_vals_compact.number_of_children() == nvars,
                "Number of variable components does not match what was returned from to_silo.");

            mixlen = silo_mixvar_vals_compact[0].dtype().number_of_elements();
            for (int i = 0; i < nvars; i ++)
            {
                mixvars_ptrs[i] = silo_mixvar_vals_compact[i].data_ptr();
            }
        }
        return mixvars_ptrs.data();
    }
    return nullptr;
}

//---------------------------------------------------------------------------//
bool prepare_field_for_write(const bool &convert_to_double_array,
                             const Node &n_var,
                             const int &nvars,
                             const int &silo_vals_type,
                             const std::string &var_name,
                             Node &n_values_compact,
                             std::vector<std::string> &comp_name_strings,
                             std::vector<const void *> &comp_vals_ptrs,
                             std::vector<const char *> &comp_name_ptrs)
{
    // we compact to support a strided array case
    if (convert_to_double_array)
    {
        detail::convert_to_double_array(n_var["values"], n_values_compact);
    }
    else
    {
        detail::conditional_compact(n_var["values"], n_values_compact);
    }

    // package up field data for silo
    if (nvars == 1)
    {
        comp_name_strings.push_back("unused");
        comp_vals_ptrs.push_back(n_values_compact.element_ptr(0));
    }
    else // we have vector/tensor values
    {
        auto val_itr = n_values_compact.children();
        while (val_itr.has_next())
        {
            const Node &n_comp = val_itr.next();
            const std::string comp_name = val_itr.name();

            if (! detail::check_alphanumeric(comp_name))
            {
                CONDUIT_INFO("Field component name " << var_name << "/" << comp_name <<
                             " contains non-alphanumeric characters. Skipping " <<
                             "Silo write of field " << var_name << ".");
                return false;
            }

            CONDUIT_ASSERT(silo_vals_type == detail::dtype_to_silo_type(n_comp.dtype()),
                "Inconsistent values types across vector components in field " << var_name);

            comp_name_strings.push_back(comp_name);
            comp_vals_ptrs.push_back(n_comp.element_ptr(0));
        }
    }

    // package up char ptrs for silo
    for (size_t i = 0; i < comp_name_strings.size(); i ++)
    {
        comp_name_ptrs.push_back(comp_name_strings[i].c_str());
    }

    // success
    return true;
}

//---------------------------------------------------------------------------//
void silo_write_field(DBfile *dbfile,
                      const std::string &var_name,
                      const Node &n_var,
                      const std::string &topo_name,
                      const Node &mesh_domain,
                      const bool write_overlink,
                      const int local_num_domains,
                      const int local_domain_index,
                      const uint64 global_domain_id,
                      const Node &n_mesh_info,
                      std::set<std::string> &used_names,
                      Node &local_type_domain_info)
{
    if (! detail::check_alphanumeric(var_name))
    {
        CONDUIT_INFO("Field name " << var_name << " contains " <<
                     "non-alphanumeric characters. Skipping.");
        return;
    }

    if (!n_mesh_info.has_path(topo_name))
    {
        CONDUIT_INFO("Skipping this variable because the linked "
                     "topology is invalid: "
                      << "fields/" << var_name
                      << "/topology: " << topo_name);
        return;
    }

    const std::string mesh_type = n_mesh_info[topo_name]["type"].as_string();
    const int num_elems = n_mesh_info[topo_name]["num_elems"].to_value();
    const int num_pts = n_mesh_info[topo_name]["num_pts"].to_value();

    int centering = 0;
    int num_values = 0;

    if (!n_var.has_path("association"))
    {
        CONDUIT_INFO("Skipping this variable because we are "
                     "missing association: "
                      << "fields/" << var_name << "/association");
        return;
    }

    const std::string association = n_var["association"].as_string();
    if (association == "element")
    {
        centering = DB_ZONECENT;
        num_values = num_elems;
    }
    else if (association == "vertex")
    {
        centering = DB_NODECENT;
        num_values = num_pts;
    }
    else
    {
        CONDUIT_ERROR("Unknown association in " << association);
    }

    // perhaps in the future we will support the material-dependent case. It
    // requires doing extra math to reconstruct the per-element values so we
    // can send them to silo.
    CONDUIT_ASSERT(n_var.has_path("values"),
        "Missing values for field " << var_name <<
        ". Material dependent fields are not supported.");

    const DataType vals_dtype = n_var["values"].dtype();
    int silo_vals_type = DB_NOTYPE;
    int nvars = 0;

    // determine the number of variable components
    if (vals_dtype.is_object()) // we have vector/tensor values
    {
        nvars = n_var["values"].number_of_children();
        CONDUIT_ASSERT(nvars > 0, "Expected object to have children.");
        silo_vals_type = detail::dtype_to_silo_type(n_var["values"][0].dtype());
    }
    else
    {
        nvars = 1;
        silo_vals_type = detail::dtype_to_silo_type(vals_dtype);
    }

    if (silo_vals_type == DB_NOTYPE)
    {
        // skip the field if we don't support its type
        CONDUIT_INFO("skipping field "
                     << var_name
                     << ", since its type is not implemented, found "
                     << vals_dtype.name());
        return;
    }

    // if we are a mixed field and we do not share a type with our
    // matset_values then we want to convert both to double arrays
    bool convert_to_double_array = false;

    // overlink requires doubles
    if (write_overlink)
    {
        convert_to_double_array = true;
    }

    //
    // Prepare Matset Values (if they exist)
    //

    // the following logic provides values to mixlen and mixvars_ptr_ptr
    // so they can be provided in the silo write calls down below.

    // these have to live out here so that pointers to them are valid later
    Node silo_matset;
    Node silo_mixvar_vals_compact;
    std::vector<void *> mixvars_ptrs(nvars);
    int mixlen = 0;
    void **mixvars_ptr_ptr =
        prepare_mixed_field_for_write(n_var,
                                      mesh_domain,
                                      var_name,
                                      vals_dtype,
                                      nvars,
                                      convert_to_double_array,
                                      silo_matset,
                                      silo_mixvar_vals_compact,
                                      mixvars_ptrs,
                                      mixlen);

    if (convert_to_double_array)
    {
        silo_vals_type = DB_DOUBLE;
    }

    //
    // Prepare Field Values
    //

    // these have to live out here so that pointers to them are valid later
    Node n_values_compact;
    std::vector<std::string> comp_name_strings;
    std::vector<const void *> comp_vals_ptrs;
    std::vector<const char *> comp_name_ptrs;
    if (! prepare_field_for_write(convert_to_double_array,
                                  n_var,
                                  nvars,
                                  silo_vals_type,
                                  var_name,
                                  n_values_compact,
                                  comp_name_strings,
                                  comp_vals_ptrs,
                                  comp_name_ptrs))
    {
        // We already printed a warning about the bad variable component names.
        // We should skip this variable entirely.
        return;
    }


    const std::string units = (n_var.has_child("units") ? n_var["units"].as_string() : "");
    const std::string label = (n_var.has_child("label") ? n_var["label"].as_string() : "");

    // create optlist
    detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
        DBMakeOptlist(1),
        &DBFreeOptlist,
        "Error freeing optlist."};
    CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating optlist");

    if (!units.empty())
    {
        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(optlist.getSiloObject(),
                        DBOPT_UNITS,
                        const_cast<char *>(units.c_str())),
            "error adding units option");
    }
    if (!label.empty())
    {
        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(optlist.getSiloObject(),
                        DBOPT_LABEL,
                        const_cast<char *>(label.c_str())),
            "error adding label option");
    }

    const std::string silo_meshname = write_overlink ? "MESH" : topo_name;
    int var_type = DB_INVALID_OBJECT;
    int silo_error = 0;
    if (mesh_type == "unstructured")
    {
        // save the var type
        var_type = DB_UCDVAR;

        // If I am writing a scalar variable, I can just use the scalar version
        // of this function.
        if (nvars == 1)
        {
            void *vals_ptr = nullptr;
            if (comp_vals_ptrs.size() > 0)
            {
                vals_ptr = const_cast<void *>(comp_vals_ptrs[0]);
            }

            void *mixvar_ptr = nullptr;
            if (mixvars_ptrs.size() > 0)
            {
                mixvar_ptr = mixvars_ptrs[0];
            }

            if (used_names.find(var_name) != used_names.end())
            {
                CONDUIT_INFO("The name " << var_name << " has already been saved to "
                             "Silo as the name for a different object. Saving this field "
                             "will overwrite that previous object, so we will skip field "
                             << var_name << ".");
                return;
            }

            silo_error = DBPutUcdvar1(dbfile, // Database file pointer
                                      var_name.c_str(), // variable name
                                      silo_meshname.c_str(), // mesh name
                                      vals_ptr, // the data values
                                      num_values, // number of elements
                                      mixvar_ptr, // mixed data arrays
                                      mixlen, // length of mixed data arrays
                                      silo_vals_type, // Datatype of the variable
                                      centering, // centering (nodal or zonal)
                                      optlist.getSiloObject()); // optlist
        }
        else
        {
            // overlink requires that non-scalar data be split into
            // multiple scalar variables
            if (write_overlink)
            {
                for (int comp_id = 0; comp_id < nvars; comp_id ++)
                {
                    void *vals_ptr = const_cast<void *>(comp_vals_ptrs[comp_id]);
                    void *mixvar_ptr = mixvars_ptrs[comp_id];
                    const std::string comp_var_name = var_name + "_" + comp_name_strings[comp_id];

                    if (used_names.find(comp_var_name) != used_names.end())
                    {
                        CONDUIT_INFO("The name " << comp_var_name << " has already been saved to "
                                     "Silo as the name for a different object. Saving this field component "
                                     "will overwrite that previous object, so we will skip field component "
                                     << comp_var_name << ".");
                        return;
                    }

                    silo_error = DBPutUcdvar1(dbfile, // Database file pointer
                                              comp_var_name.c_str(), // variable name
                                              silo_meshname.c_str(), // mesh name
                                              vals_ptr, // the data values
                                              num_values, // number of elements
                                              mixvar_ptr, // mixed data arrays
                                              mixlen, // length of mixed data arrays
                                              silo_vals_type, // Datatype of the variable
                                              centering, // centering (nodal or zonal)
                                              optlist.getSiloObject()); // optlist
                }
            }
            else
            {
                if (used_names.find(var_name) != used_names.end())
                {
                    CONDUIT_INFO("The name " << var_name << " has already been saved to "
                                 "Silo as the name for a different object. Saving this field "
                                 "will overwrite that previous object, so we will skip field "
                                 << var_name << ".");
                    return;
                }

                silo_error = DBPutUcdvar(dbfile, // Database file pointer
                                         var_name.c_str(), // variable name
                                         silo_meshname.c_str(), // mesh name
                                         nvars, // number of variable components
                                         comp_name_ptrs.data(), // variable component names
                                         comp_vals_ptrs.data(), // the data values
                                         num_values, // number of elements
                                         mixvars_ptr_ptr, // mixed data arrays
                                         mixlen, // length of mixed data arrays
                                         silo_vals_type, // Datatype of the variable
                                         centering, // centering (nodal or zonal)
                                         optlist.getSiloObject()); // optlist
            }
        }
    }
    else if (mesh_type == "rectilinear" ||
             mesh_type == "uniform" ||
             mesh_type == "structured")
    {
        int dims[3] = {0, 0, 0};
        int num_dims = 2;

        dims[0] = n_mesh_info[topo_name]["elements/i"].value();
        dims[1] = n_mesh_info[topo_name]["elements/j"].value();

        if (n_mesh_info[topo_name]["elements"].has_path("k"))
        {
            num_dims = 3;
            dims[2] = n_mesh_info[topo_name]["elements/k"].value();
        }

        if (centering == DB_NODECENT)
        {
            dims[0] += 1;
            dims[1] += 1;
            dims[2] += 1;
        }

        // save the var type
        var_type = DB_QUADVAR;

        // If I am writing a scalar variable, I can just use the scalar version
        // of this function.
        if (nvars == 1)
        {
            void *vals_ptr = nullptr;
            if (comp_vals_ptrs.size() > 0)
            {
                vals_ptr = const_cast<void *>(comp_vals_ptrs[0]);
            }

            void *mixvar_ptr = nullptr;
            if (mixvars_ptrs.size() > 0)
            {
                mixvar_ptr = mixvars_ptrs[0];
            }

            if (used_names.find(var_name) != used_names.end())
            {
                CONDUIT_INFO("The name " << var_name << " has already been saved to "
                             "Silo as the name for a different object. Saving this field "
                             "will overwrite that previous object, so we will skip field "
                             << var_name << ".");
                return;
            }

            silo_error = DBPutQuadvar1(dbfile, // Database file pointer
                                       var_name.c_str(), // variable name
                                       silo_meshname.c_str(), // mesh name
                                       vals_ptr, // the data values
                                       dims, // the dimensions of the data
                                       num_dims, // number of dimensions
                                       mixvar_ptr, // mixed data arrays
                                       mixlen, // length of mixed data arrays
                                       silo_vals_type, // Datatype of the variable
                                       centering, // centering (nodal or zonal)
                                       optlist.getSiloObject()); // optlist
        }
        else
        {
            // overlink requires that non-scalar data be split into
            // multiple scalar variables
            if (write_overlink)
            {
                for (int comp_id = 0; comp_id < nvars; comp_id ++)
                {
                    void *vals_ptr = const_cast<void *>(comp_vals_ptrs[comp_id]);
                    void *mixvar_ptr = mixvars_ptrs[comp_id];
                    const std::string comp_var_name = var_name + "_" + comp_name_strings[comp_id];

                    if (used_names.find(comp_var_name) != used_names.end())
                    {
                        CONDUIT_INFO("The name " << comp_var_name << " has already been saved to "
                                     "Silo as the name for a different object. Saving this field component "
                                     "will overwrite that previous object, so we will skip field component "
                                     << comp_var_name << ".");
                        return;
                    }

                    silo_error = DBPutQuadvar1(dbfile, // Database file pointer
                                               comp_var_name.c_str(), // variable name
                                               silo_meshname.c_str(), // mesh name
                                               vals_ptr, // the data values
                                               dims, // the dimensions of the data
                                               num_dims, // number of dimensions
                                               mixvar_ptr, // mixed data arrays
                                               mixlen, // length of mixed data arrays
                                               silo_vals_type, // Datatype of the variable
                                               centering, // centering (nodal or zonal)
                                               optlist.getSiloObject()); // optlist
                }
            }
            else
            {
                if (used_names.find(var_name) != used_names.end())
                {
                    CONDUIT_INFO("The name " << var_name << " has already been saved to "
                                 "Silo as the name for a different object. Saving this field "
                                 "will overwrite that previous object, so we will skip field "
                                 << var_name << ".");
                    return;
                }

                silo_error = DBPutQuadvar(dbfile, // Database file pointer
                                          var_name.c_str(), // variable name
                                          silo_meshname.c_str(), // mesh name
                                          nvars, // number of variable components
                                          comp_name_ptrs.data(), // variable component names
                                          comp_vals_ptrs.data(), // the data values
                                          dims, // the dimensions of the data
                                          num_dims, // number of dimensions
                                          mixvars_ptr_ptr, // mixed data arrays
                                          mixlen, // length of mixed data arrays
                                          silo_vals_type, // Datatype of the variable
                                          centering, // centering (nodal or zonal)
                                          optlist.getSiloObject()); // optlist
            }
        }
    }
    else if (mesh_type == "points")
    {
        if (write_overlink)
        {
            CONDUIT_ERROR("Cannot write point var " << topo_name << " to overlink."
                          << " Only DB_UCDVAR and DB_QUADVAR are supported.");
        }
        // save the var type
        var_type = DB_POINTVAR;

        if (used_names.find(var_name) != used_names.end())
        {
            CONDUIT_INFO("The name " << var_name << " has already been saved to "
                         "Silo as the name for a different object. Saving this field "
                         "will overwrite that previous object, so we will skip field "
                         << var_name << ".");
            return;
        }

        silo_error = DBPutPointvar(dbfile, // Database file pointer
                                   var_name.c_str(), // variable name
                                   silo_meshname.c_str(), // mesh name
                                   nvars, // number of variable components
                                   comp_vals_ptrs.data(), // data values
                                   num_pts, // Number of elements (points)
                                   silo_vals_type, // Datatype of the variable
                                   optlist.getSiloObject()); // optlist
    }
    else
    {
        CONDUIT_ERROR("only DBPutQuadvar + DBPutUcdvar + DBPutPointvar var are supported");
    }

    CONDUIT_CHECK_SILO_ERROR(silo_error, "after creating field " << var_name);

    Node bookkeeping_info;
    bookkeeping_info["comp_info"]["comp"] = "vars";
    bookkeeping_info["specific_info"]["comp_type"] = var_type;
    bookkeeping_info["specific_info"]["var_data_type"] = detail::silo_type_to_ovl_attr_type(silo_vals_type);
    bookkeeping_info["domain_info"]["local_num_domains"] = local_num_domains;
    bookkeeping_info["domain_info"]["local_domain_index"] = local_domain_index;
    bookkeeping_info["domain_info"]["global_domain_id"] = global_domain_id;
    bookkeeping_info["write_overlink"] = (write_overlink ? "yes" : "no");

    // if we are writing overlink and we have separated variable components into new vars
    if (write_overlink && nvars != 1)
    {
        bookkeeping_info["specific_info"]["var_parent"] = var_name;
        for (int comp_id = 0; comp_id < nvars; comp_id ++)
        {
            if (bookkeeping_info.has_path("comp_info/comp_name"))
            {
                bookkeeping_info["comp_info"]["comp_name"].reset();
            }

            const std::string comp_var_name = var_name + "_" + comp_name_strings[comp_id];

            bookkeeping_info["comp_info"]["comp_name"] = comp_var_name;

            // bookkeeping
            detail::track_local_type_domain_info(bookkeeping_info, local_type_domain_info);

            used_names.insert(comp_var_name);
        }
    }
    else
    {
        bookkeeping_info["comp_info"]["comp_name"] = var_name;

        // bookkeeping
        detail::track_local_type_domain_info(bookkeeping_info, local_type_domain_info);

        used_names.insert(var_name);
    }
}

//---------------------------------------------------------------------------//
// overlink only
void silo_write_adjset(DBfile *dbfile,
                       const Node *n_adjset)
{
    auto write_dom_neighbor_nums = [&](const int num_neighboring_doms,
                                       const std::vector<int> &dom_neighbor_nums)
    {
        std::vector<std::string> elem_name_strings;
        elem_name_strings.push_back("num_neighbors");
        elem_name_strings.push_back("neighbor_nums");
        // package up char ptrs for silo
        std::vector<const char *> elem_name_ptrs;
        for (size_t i = 0; i < elem_name_strings.size(); i ++)
        {
            elem_name_ptrs.push_back(elem_name_strings[i].c_str());
        }

        std::vector<int> elemlengths;
        elemlengths.push_back(1);
        elemlengths.push_back(num_neighboring_doms);

        const int nelems = 2;
        const int nvalues = num_neighboring_doms + 1;

        void *dom_neighbor_nums_ptr = static_cast<void *>(
                                          const_cast<int *>(
                                              dom_neighbor_nums.data()));

        CONDUIT_CHECK_SILO_ERROR(
            DBPutCompoundarray(dbfile, // dbfile
                               "DOMAIN_NEIGHBOR_NUMS", // name
                               elem_name_ptrs.data(), // elemnames
                               elemlengths.data(), // elemlengths
                               nelems, // nelems
                               dom_neighbor_nums_ptr, // values
                               nvalues, // nvalues
                               DB_INT, // datatype
                               NULL), // optlist
            "Error writing domain neighbor nums.");
    };

    //
    // DOMAIN NEIGHBOR NUMS
    //

    if (n_adjset == nullptr)
    {
        const int num_neighboring_doms = 0;

        // our compound array data that we are saving
        std::vector<int> dom_neighbor_nums;

        // the first entry is the number of neighboring domains
        dom_neighbor_nums.push_back(num_neighboring_doms);

        write_dom_neighbor_nums(num_neighboring_doms, dom_neighbor_nums);

        // we are done there is nothing else to write
        return;
    }

    CONDUIT_ASSERT((*n_adjset)["association"].as_string() != "element",
        "We do not support the element-associated adjset case. "
        "Please contact a Conduit developer.");

    Node pairwise_adjset;
    conduit::blueprint::mesh::adjset::to_pairwise(*n_adjset, pairwise_adjset);

    const int num_neighboring_doms = pairwise_adjset["groups"].number_of_children();

    // our compound array data that we are saving
    std::vector<int> dom_neighbor_nums;

    // the first entry is the number of neighboring domains
    dom_neighbor_nums.push_back(num_neighboring_doms);

    // the following entries are the domain ids of the neighboring domains
    auto group_itr = pairwise_adjset["groups"].children();
    while (group_itr.has_next())
    {
        const Node &group = group_itr.next();
        // since we have forced pairwise, we can assume that there is only one
        const int neighbor = group["neighbors"].to_int();
        dom_neighbor_nums.push_back(neighbor);
    }

    write_dom_neighbor_nums(num_neighboring_doms, dom_neighbor_nums);

    //
    // COMMUNICATIONS LISTS FOR NEIGHBORING DOMAINS
    //

    // if there are no domain neighbors then we can return early
    if (num_neighboring_doms <= 0)
    {
        return;
    }

    int neighbor_index = 0;
    group_itr = pairwise_adjset["groups"].children();
    while (group_itr.has_next())
    {
        const std::string arr_name = "DOMAIN_NEIGHBOR" + std::to_string(neighbor_index);

        char const *elemname = "shared_nodes";
        const int nelems_comm = 1;

        std::vector<int> shared_nodes;

        const Node &group = group_itr.next();
        // since we have forced pairwise, we can assume that there is only one
        int_accessor group_values = group["values"].value();
        const int num_shared_nodes = group["values"].dtype().number_of_elements();

        for (int i = 0; i < num_shared_nodes; i ++)
        {
            shared_nodes.push_back(group_values[i]);
        }

        CONDUIT_CHECK_SILO_ERROR(
            DBPutCompoundarray(dbfile, // dbfile
                               arr_name.c_str(), // name
                               &elemname, // elemnames
                               &num_shared_nodes, // elemlengths
                               nelems_comm, // nelems
                               static_cast<void *>(shared_nodes.data()), // values
                               num_shared_nodes, // nvalues
                               DB_INT, // datatype
                               NULL), // optlist
            "Error writing " + arr_name);

        neighbor_index ++;
    }
}

//---------------------------------------------------------------------------//
int
assign_coords_ptrs(void *coords_ptrs[3],
                   const int ndims,
                   Node &n_coords_compact,
                   char const * const coordnames[])
{
    DataType dtype = n_coords_compact[coordnames[0]].dtype();
    CONDUIT_ASSERT(dtype.id() == n_coords_compact[coordnames[1]].dtype().id(),
                   "all coordinate arrays must have same type, got " << dtype.to_string()
                    << " and " << n_coords_compact[coordnames[1]].dtype().to_string());
    if (ndims == 3)
    {
        CONDUIT_ASSERT(dtype.id() == n_coords_compact[coordnames[2]].dtype().id(),
                       "all coordinate arrays must have same type, got " << dtype.to_string()
                        << " and " << n_coords_compact[coordnames[2]].dtype().to_string());
        coords_ptrs[2] = n_coords_compact[coordnames[2]].element_ptr(0);
    }
    coords_ptrs[0] = n_coords_compact[coordnames[0]].element_ptr(0);
    coords_ptrs[1] = n_coords_compact[coordnames[1]].element_ptr(0);

    if (dtype.is_float())
    {
        return DB_FLOAT;
    }
    else if (dtype.is_double())
    {
        return DB_DOUBLE;
    }
    else
    {
        CONDUIT_ERROR("coords data type not implemented, found "
                      << dtype.name());
        return -1;
    }
}

//---------------------------------------------------------------------------//
// calculates and checks the number of points for an explicit coordset
int get_explicit_num_pts(const Node &n_vals)
{
    auto val_itr = n_vals.children();
    CONDUIT_ASSERT(val_itr.has_next(),
        "Cannot count the number of points because no points given.");
    const Node &n_first_val = val_itr.next();
    int num_pts = n_first_val.dtype().number_of_elements();
    while (val_itr.has_next())
    {
        const Node &n_val = val_itr.next();
        CONDUIT_ASSERT(num_pts == n_val.dtype().number_of_elements(),
            "Number of points in explicit coordset does not match between dimensions.");
    }
    return num_pts;
}

//---------------------------------------------------------------------------//
void silo_write_ucd_zonelist(DBfile *dbfile,
                             const std::string &topo_name,
                             const Node &n_topo,
                             const bool &write_overlink,
                             Node &n_mesh_info)
{
    const Node &n_elements = n_topo["elements"];

    CONDUIT_ASSERT(n_elements.dtype().is_object(),
        "Invalid elements for 'unstructured' case");

    const std::string topo_shape = n_elements["shape"].as_string();

    Node n_conn;

    // We are using the vtk ordering for our wedges; silo wedges (prisms)
    // expect a different ordering. Thus before we output to silo, we must
    // change the ordering of each of our wedges.
    if (topo_shape == "wedge")
    {
        n_conn.set(n_elements["connectivity"]);
        DataType dtype = n_conn.dtype();
        // swizzle the connectivity
        if (dtype.is_uint64())
        {
            detail::conduit_wedge_connectivity_to_silo<uint64>(n_conn);
        }
        else if (dtype.is_uint32())
        {
            detail::conduit_wedge_connectivity_to_silo<uint32>(n_conn);
        }
        else if (dtype.is_int64())
        {
            detail::conduit_wedge_connectivity_to_silo<int64>(n_conn);
        }
        else if (dtype.is_int32())
        {
            detail::conduit_wedge_connectivity_to_silo<int32>(n_conn);
        }
        else
        {
            CONDUIT_ERROR("Unsupported connectivity type in " << dtype.to_yaml());
        }
    }
    else
    {
        n_conn.set_external(n_elements["connectivity"]);
    }

    int total_num_elems = 0;

    std::vector<int> shapetype;
    std::vector<int> shapesize;
    std::vector<int> shapecnt;

    index_t num_shapes = 0;

    auto set_up_single_shape_type = [&](index_t num_pts, int db_zonetype)
    {
        num_shapes = 1;

        int num_elems = n_conn.dtype().number_of_elements() / num_pts;

        shapetype.push_back(db_zonetype);
        shapesize.push_back(num_pts);
        shapecnt.push_back(num_elems);

        total_num_elems += num_elems;
    };

    if (topo_shape == "quad")
    {
        set_up_single_shape_type(4, DB_ZONETYPE_QUAD);
    }
    else if (topo_shape == "tri")
    {
        CONDUIT_ASSERT(! write_overlink, "Triangle topologies are not supported in Overlink, "
                       "use general Silo instead.");
        set_up_single_shape_type(3, DB_ZONETYPE_TRIANGLE);
    }
    else if (topo_shape == "hex")
    {
        set_up_single_shape_type(8, DB_ZONETYPE_HEX);
    }
    else if (topo_shape == "tet")
    {
        CONDUIT_ASSERT(! write_overlink, "Tetrahedral topologies are not supported in Overlink, "
                       "use general Silo instead.");
        set_up_single_shape_type(4, DB_ZONETYPE_TET);
    }
    else if( topo_shape == "wedge")
    {
        CONDUIT_ASSERT(! write_overlink, "Wedge topologies are not supported in Overlink, "
                       "use general Silo instead.");
        set_up_single_shape_type(6, DB_ZONETYPE_PRISM);
    }
    else if( topo_shape == "pyramid")
    {
        CONDUIT_ASSERT(! write_overlink, "Pyramid topologies are not supported in Overlink, "
                       "use general Silo instead.");
        set_up_single_shape_type(5, DB_ZONETYPE_PYRAMID);
    }
    else if (topo_shape == "line")
    {
        CONDUIT_ASSERT(! write_overlink, "Line topologies are not supported in Overlink, "
                       "use general Silo instead.");
        set_up_single_shape_type(2, DB_ZONETYPE_BEAM);
    }
    else if (topo_shape == "polygonal")
    {
        int num_elems = n_elements["sizes"].dtype().number_of_elements();
        int_accessor sizes = n_elements["sizes"].value();

        // worst case is that all elems are a different shape
        num_shapes = num_elems;

        for (int i = 0; i < num_elems; i ++)
        {
            shapetype.push_back(DB_ZONETYPE_POLYGON);
            shapesize.push_back(sizes[i]);
            shapecnt.push_back(1);
        }
        total_num_elems += num_elems;
    }
    else
    {
        // TODO add polyhedra and mixed
        CONDUIT_ERROR("Unsupported topo shape " << topo_shape);
    }

    Node n_conn_compact;
    detail::conditional_compact(n_conn, n_conn_compact);

    const int conn_len = n_conn_compact.dtype().number_of_elements();
    int *conn_ptr = n_conn_compact.value();

    n_mesh_info[topo_name]["num_elems"] = total_num_elems;

    const std::string zlist_name = write_overlink ? "zonelist" : topo_name + "_connectivity";
    n_mesh_info[topo_name]["zonelist_name"] = zlist_name;

    const int ndims = n_mesh_info[topo_name]["ndims"].as_int();

    CONDUIT_CHECK_SILO_ERROR(
        DBPutZonelist2(dbfile,             // silo file
                       zlist_name.c_str(), // silo obj name
                       total_num_elems,    // number of elements
                       ndims,              // spatial dims
                       conn_ptr,           // connectivity array
                       conn_len,           // len of connectivity array
                       0,                  // base offset
                       0,                  // # ghosts low
                       0,                  // # ghosts high
                       shapetype.data(),   // list of shapes ids
                       shapesize.data(),   // number of points per shape id
                       shapecnt.data(),    // number of elements each shape id is used for
                       num_shapes,         // number of shapes ids
                       NULL),              // optlist
        "after saving ucd " + topo_shape + " topology");
}

//---------------------------------------------------------------------------//
void silo_write_quad_rect_mesh(DBfile *dbfile,
                               const std::string &topo_name,
                               const Node &n_topo,
                               const Node &n_coords,
                               DBoptlist *state_optlist,
                               const int ndims,
                               char const * const coordnames[],
                               const bool write_overlink,
                               Node &n_mesh_info)
{
    Node n_coords_compact;
    detail::conditional_compact(n_coords["values"], n_coords_compact);

    int pts_dims[3];
    pts_dims[0] = n_coords_compact[coordnames[0]].dtype().number_of_elements();
    pts_dims[1] = n_coords_compact[coordnames[1]].dtype().number_of_elements();
    pts_dims[2] = 1;

    int num_pts = pts_dims[0] * pts_dims[1];
    int num_elems = (pts_dims[0] - 1) * (pts_dims[1] - 1);
    if (ndims == 3)
    {
        pts_dims[2] = n_coords_compact[coordnames[2]].dtype().number_of_elements();
        num_pts *= pts_dims[2];
        num_elems *= (pts_dims[2] - 1);
        n_mesh_info[topo_name]["elements/k"] = pts_dims[2] - 1;
    }

    n_mesh_info[topo_name]["num_pts"].set(num_pts);
    n_mesh_info[topo_name]["num_elems"].set(num_elems);
    n_mesh_info[topo_name]["elements/i"] = pts_dims[0] - 1;
    n_mesh_info[topo_name]["elements/j"] = pts_dims[1] - 1;

    void *coords_ptrs[3] = {NULL, NULL, NULL};
    int coords_dtype = assign_coords_ptrs(coords_ptrs,
                                          ndims,
                                          n_coords_compact,
                                          coordnames);

    int base_index[] = {0,0,0};
    if (n_topo.has_path("elements/origin"))
    {
        base_index[0] = n_topo["elements/origin/i"].as_int();
        base_index[1] = n_topo["elements/origin/j"].as_int();
        base_index[2] = n_topo["elements/origin/k"].as_int();

        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(state_optlist,
                        DBOPT_BASEINDEX,
                        base_index),
            "Error adding option");
    }

    const std::string silo_meshname = write_overlink ? "MESH" : topo_name;

    CONDUIT_CHECK_SILO_ERROR(
        DBPutQuadmesh(dbfile,           // silo file ptr
                      silo_meshname.c_str(), // mesh name
                      coordnames,       // coord names
                      coords_ptrs,      // coords values
                      pts_dims,         // dims vals
                      ndims,            // number of dims
                      coords_dtype,     // type of data array
                      DB_COLLINEAR,     // DB_COLLINEAR or DB_NONCOLLINEAR
                      state_optlist),   // opt list
        "DBPutQuadmesh");
}

//---------------------------------------------------------------------------//
void silo_write_ucd_mesh(DBfile *dbfile,
                         const std::string &topo_name,
                         DBoptlist *optlist,
                         const int ndims,
                         const int num_pts,
                         char const * const coordnames[],
                         const void *coords_ptrs,
                         const int coords_dtype,
                         const bool write_overlink,
                         Node &n_mesh_info)
{
    const int num_elems = n_mesh_info[topo_name]["num_elems"].value();

    // TODO polyhedral zone lists are named differently
    const std::string zlist_name = n_mesh_info[topo_name]["zonelist_name"].as_string();
    const std::string silo_meshname = (write_overlink ? "MESH" : topo_name);

    CONDUIT_CHECK_SILO_ERROR(
        DBPutUcdmesh(dbfile,                // silo file ptr
                     silo_meshname.c_str(), // mesh name
                     ndims,                 // number of dims
                     coordnames,            // coord names
                     coords_ptrs,           // coords values
                     num_pts,               // number of points
                     num_elems,             // number of elements
                     zlist_name.c_str(),    // zone list name
                     NULL,                  // face list names
                     coords_dtype,          // type of data array
                     optlist),              // opt list
        "DBPutUcdmesh");
}

//---------------------------------------------------------------------------//
void silo_write_structured_mesh(DBfile *dbfile,
                                const std::string &topo_name,
                                const Node &n_topo,
                                DBoptlist *optlist,
                                const int ndims,
                                char const * const coordnames[],
                                const void *coords_ptrs,
                                const int coords_dtype,
                                const bool write_overlink,
                                Node &n_mesh_info)
{
    // check for strided structured case
    if (n_topo.has_path("elements/dims/offsets") ||
        n_topo.has_path("elements/dims/strides"))
    {
        // I could potentially support either case where only offsets or only strides
        // is present. Would need to think more about that. But certainly if both
        // are present we have to give up, or completely take apart the mesh.
        CONDUIT_ERROR("Strided Structured Blueprint case does not have a general "
                      "analog in Silo.");
    }
    int ele_dims[3];
    ele_dims[0] = n_topo["elements/dims/i"].to_value();
    ele_dims[1] = n_topo["elements/dims/j"].to_value();
    ele_dims[2] = 0;

    index_t num_elems = ele_dims[0] * ele_dims[1];

    if (ndims == 3)
    {
        ele_dims[2] = n_topo["elements/dims/k"].to_value();
        num_elems *= ele_dims[2];
    }

    // silo needs the node dims to define a structured grid
    int pts_dims[3];
    pts_dims[0] = ele_dims[0] + 1;
    pts_dims[1] = ele_dims[1] + 1;
    pts_dims[2] = 1;

    n_mesh_info[topo_name]["num_elems"].set(num_elems);
    n_mesh_info[topo_name]["elements/i"] = ele_dims[0];
    n_mesh_info[topo_name]["elements/j"] = ele_dims[1];

    if (ndims == 3)
    {
        n_mesh_info[topo_name]["elements/k"] = ele_dims[2];
        pts_dims[2] = ele_dims[2] + 1;
    }

    int base_index[] = {0,0,0};
    if (n_topo.has_path("elements/origin"))
    {
        base_index[0] = n_topo["elements/origin/i"].as_int();
        base_index[1] = n_topo["elements/origin/j"].as_int();
        if (ndims == 3)
        {
            base_index[2] = n_topo["elements/origin/k"].as_int();
        }


        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(optlist,
                        DBOPT_BASEINDEX,
                        base_index),
            "Error adding option");
    }

    const std::string silo_meshname = write_overlink ? "MESH" : topo_name;

    CONDUIT_CHECK_SILO_ERROR(
        DBPutQuadmesh(dbfile,                // silo file ptr
                      silo_meshname.c_str(), // mesh name
                      coordnames,            // coord names
                      coords_ptrs,           // coords values
                      pts_dims,              // dims vals
                      ndims,                 // number of dims
                      coords_dtype,          // type of data array
                      DB_NONCOLLINEAR,       // DB_COLLINEAR (rectilinear grid) or DB_NONCOLLINEAR (structured grid)
                      optlist),              // opt list
        "DBPutQuadmesh");
}

//---------------------------------------------------------------------------//
void silo_write_pointmesh(DBfile *dbfile,
                          const std::string &topo_name,
                          DBoptlist *optlist,
                          const int ndims,
                          const int num_pts,
                          const void *coords_ptrs,
                          const int coords_dtype,
                          Node &n_mesh_info)
{
    n_mesh_info[topo_name]["num_elems"].set(num_pts);
    CONDUIT_CHECK_SILO_ERROR(
        DBPutPointmesh(dbfile,            // silo file ptr
                       topo_name.c_str(), // mesh name
                       ndims,             // num_dims
                       coords_ptrs,       // coords values
                       num_pts,           // num eles = num pts
                       coords_dtype,      // type of data array
                       optlist),          // opt list
        "after saving DBPutPointmesh");
}

//---------------------------------------------------------------------------//
bool silo_write_topo(DBfile *dbfile,
                     const Node &mesh_domain,
                     const std::string &topo_name,
                     const bool write_overlink,
                     const int local_num_domains,
                     const int local_domain_index,
                     const uint64 global_domain_id,
                     Node &n_mesh_info,
                     Node &local_type_domain_info)
{
    if (! detail::check_alphanumeric(topo_name))
    {
        CONDUIT_INFO("Topology name " << topo_name << " contains " <<
                     "non-alphanumeric characters. Skipping.");
        return false;
    }

    const Node &n_topo = mesh_domain["topologies"][topo_name];
    std::string topo_type = n_topo["type"].as_string();
    n_mesh_info[topo_name]["type"].set(topo_type);

    // make sure we have coordsets
    CONDUIT_ASSERT(mesh_domain.has_path("coordsets"), "mesh missing: coordsets");

    // get this topo's coordset name
    const std::string coordset_name = n_topo["coordset"].as_string();

    n_mesh_info[topo_name]["coordset"].set(coordset_name);

    // obtain the coordset with the name
    CONDUIT_ASSERT(mesh_domain["coordsets"].has_path(coordset_name),
        "mesh is missing coordset named "
        << coordset_name << " for topology named "
        << topo_name);

    const Node &n_coords = mesh_domain["coordsets"][coordset_name];

    // check dims
    int ndims = conduit::blueprint::mesh::utils::coordset::dims(n_coords);
    CONDUIT_ASSERT(2 <= ndims && ndims <= 3, "Dimension count not accepted: " << ndims);
    // I need ndims before we write the zonelist
    n_mesh_info[topo_name]["ndims"].set(ndims);

    bool unstructured_points = false;
    if (topo_type == "unstructured")
    {
        std::string ele_shape = n_topo["elements/shape"].as_string();
        if (ele_shape != "point")
        {
            // we need a zone list for a ucd mesh
            silo_write_ucd_zonelist(dbfile,
                                    topo_name,
                                    n_topo,
                                    write_overlink,
                                    n_mesh_info);
        }
        else
        {
            unstructured_points = true;
            topo_type = "points";
            n_mesh_info[topo_name]["type"].set(topo_type);
        }
    }

    // get coordsys info
    const std::string coordsys = conduit::blueprint::mesh::utils::coordset::coordsys(n_coords);
    int silo_coordsys_type = detail::get_coordset_silo_type(coordsys);
    std::vector<const char *> silo_coordset_axis_labels = detail::get_coordset_axis_labels(silo_coordsys_type);
    // create optlist
    detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
        DBMakeOptlist(1),
        &DBFreeOptlist,
        "Error freeing state optlist."};
    CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating optlist");
    CONDUIT_CHECK_SILO_ERROR(
        DBAddOption(optlist.getSiloObject(),
                    DBOPT_COORDSYS,
                    &silo_coordsys_type),
        "error adding coordsys option");

    //
    // handle units and labels, if they exist
    //

    std::vector<std::string> units;
    std::vector<std::string> labels;
    std::vector<const char *> units_str;
    std::vector<const char *> labels_str;

    if (n_coords.has_child("units"))
    {
        auto units_itr = n_coords["units"].children();
        while (units_itr.has_next())
        {
            const Node &n_unit = units_itr.next();
            units.emplace_back(n_unit.as_string());
        }
    }
    if (n_coords.has_child("labels"))
    {
        auto labels_itr = n_coords["labels"].children();
        while (labels_itr.has_next())
        {
            const Node &n_label = labels_itr.next();
            labels.emplace_back(n_label.as_string());
        }
    }
    for (const std::string &unit_name : units)
    {
        units_str.emplace_back(unit_name.c_str());
    }
    for (const std::string &label_name : labels)
    {
        labels_str.emplace_back(label_name.c_str());
    }
    for (int i = 0; i < static_cast<int>(units_str.size()); i ++)
    {
        const int dbopt = (i == 0 ? DBOPT_XUNITS : (i == 1 ? DBOPT_YUNITS : DBOPT_ZUNITS));
        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(optlist.getSiloObject(),
                        dbopt,
                        static_cast<void *>(const_cast<char *>(units_str[i]))),
            "error adding units option");
    }
    for (int i = 0; i < static_cast<int>(labels_str.size()); i ++)
    {
        const int dbopt = (i == 0 ? DBOPT_XLABEL : (i == 1 ? DBOPT_YLABEL : DBOPT_ZLABEL));
        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(optlist.getSiloObject(),
                        dbopt,
                        static_cast<void *>(const_cast<char *>(labels_str[i]))),
            "error adding labels option");
    }

    int mesh_type = DB_INVALID_OBJECT;

    if (topo_type == "unstructured" ||
        topo_type == "structured" ||
        topo_type == "points")
    {
        // check for explicit coords
        CONDUIT_ASSERT(n_coords["type"].as_string() == "explicit",
            "Expected an explicit coordset when writing " << topo_type
            << " mesh " << topo_name)

        // compact arrays
        Node n_coords_compact, n_coords_compact_final, new_coords;

        // here we handle the unstructured points case:
        if (unstructured_points)
        {
            // we need to change the coords to only have the ones that are used
            int_accessor conn = n_topo["elements"]["connectivity"].value();
            const int num_elem = conn.number_of_elements();

            new_coords["values"][silo_coordset_axis_labels[0]].set(DataType::float64(num_elem));
            new_coords["values"][silo_coordset_axis_labels[1]].set(DataType::float64(num_elem));
            if (ndims == 3)
            {
                new_coords["values"][silo_coordset_axis_labels[2]].set(DataType::float64(num_elem));
            }

            for (int conn_index = 0; conn_index < num_elem; conn_index ++)
            {
                int old_coord_index = conn[conn_index];

                double_array new_x_coords = new_coords["values"][silo_coordset_axis_labels[0]].value();
                double_accessor old_x_coords = n_coords["values"][silo_coordset_axis_labels[0]].value();
                new_x_coords[conn_index] = old_x_coords[old_coord_index];

                double_array new_y_coords = new_coords["values"][silo_coordset_axis_labels[1]].value();
                double_accessor old_y_coords = n_coords["values"][silo_coordset_axis_labels[1]].value();
                new_y_coords[conn_index] = old_y_coords[old_coord_index];

                if (ndims == 3)
                {
                    double_array new_z_coords = new_coords["values"][silo_coordset_axis_labels[2]].value();
                    double_accessor old_z_coords = n_coords["values"][silo_coordset_axis_labels[2]].value();
                    new_z_coords[conn_index] = old_z_coords[old_coord_index];
                }
            }

            detail::conditional_compact(new_coords["values"], n_coords_compact);
        }
        else
        {
            detail::conditional_compact(n_coords["values"], n_coords_compact);
        }

        // overlink requires doubles
        if (write_overlink)
        {
            detail::convert_to_double_array(n_coords_compact, n_coords_compact_final);
        }
        else
        {
            n_coords_compact_final.set_external(n_coords_compact);
        }

        // get num pts
        const int num_pts = get_explicit_num_pts(n_coords_compact_final);
        n_mesh_info[topo_name]["num_pts"].set(num_pts);

        // get coords ptrs
        void *coords_ptrs[3] = {NULL, NULL, NULL};
        int coords_dtype = assign_coords_ptrs(coords_ptrs,
                                              ndims,
                                              n_coords_compact_final,
                                              silo_coordset_axis_labels.data());

        if (topo_type == "unstructured")
        {
            mesh_type = DB_UCDMESH;
            silo_write_ucd_mesh(dbfile, topo_name,
                                optlist.getSiloObject(),
                                ndims, num_pts, silo_coordset_axis_labels.data(),
                                coords_ptrs, coords_dtype,
                                write_overlink, n_mesh_info);
        }
        else if (topo_type == "structured")
        {
            mesh_type = DB_QUADMESH;
            silo_write_structured_mesh(dbfile, topo_name, n_topo,
                                       optlist.getSiloObject(),
                                       ndims, silo_coordset_axis_labels.data(),
                                       coords_ptrs, coords_dtype,
                                       write_overlink, n_mesh_info);
        }
        else if (topo_type == "points")
        {
            if (write_overlink)
            {
                CONDUIT_ERROR("Cannot write point mesh " << topo_name << " to overlink."
                              << " Only DB_UCDMESH and DB_QUADMESH are supported.");
            }
            mesh_type = DB_POINTMESH;
            silo_write_pointmesh(dbfile, topo_name,
                                 optlist.getSiloObject(),
                                 ndims, num_pts,
                                 coords_ptrs, coords_dtype,
                                 n_mesh_info);
        }
    }
    else if (topo_type == "rectilinear")
    {
        mesh_type = DB_QUADMESH;
        silo_write_quad_rect_mesh(dbfile, topo_name,
                                  n_topo, n_coords,
                                  optlist.getSiloObject(),
                                  ndims, silo_coordset_axis_labels.data(),
                                  write_overlink, n_mesh_info);
    }
    else if (topo_type == "uniform")
    {
        mesh_type = DB_QUADMESH;
        // silo doesn't have a direct path for a uniform mesh
        // we need to convert its implicit uniform coords to
        // implicit rectilinear coords

        Node n_rect;
        Node &n_rect_coords = n_rect["coordsets"][coordset_name];
        Node &n_rect_topo = n_rect["topologies"][topo_name];
        conduit::blueprint::mesh::topology::uniform::to_rectilinear(
            n_topo, n_rect_topo, n_rect_coords);

        silo_write_quad_rect_mesh(dbfile, topo_name,
                                  n_rect_topo, n_rect_coords,
                                  optlist.getSiloObject(),
                                  ndims, silo_coordset_axis_labels.data(),
                                  write_overlink, n_mesh_info);
    }
    else
    {
        CONDUIT_ERROR("Unknown topo type in " << topo_type);
    }

    Node bookkeeping_info;
    bookkeeping_info["comp_info"]["comp"] = "meshes";
    bookkeeping_info["comp_info"]["comp_name"] = topo_name;
    bookkeeping_info["specific_info"]["comp_type"] = mesh_type;
    bookkeeping_info["domain_info"]["local_num_domains"] = local_num_domains;
    bookkeeping_info["domain_info"]["local_domain_index"] = local_domain_index;
    bookkeeping_info["domain_info"]["global_domain_id"] = global_domain_id;
    bookkeeping_info["write_overlink"] = (write_overlink ? "yes" : "no");

    // bookkeeping
    detail::track_local_type_domain_info(bookkeeping_info, local_type_domain_info);

    return true;
}

//---------------------------------------------------------------------------//
bool silo_write_matset(DBfile *dbfile,
                       const std::string &matset_name,
                       const Node &n_matset,
                       const std::string &topo_name,
                       const bool write_overlink,
                       const int local_num_domains,
                       const int local_domain_index,
                       const uint64 global_domain_id,
                       Node &n_mesh_info,
                       std::set<std::string> &used_names,
                       Node &local_type_domain_info)
{
    if (! detail::check_alphanumeric(matset_name))
    {
        CONDUIT_INFO("Matset name " << matset_name << " contains " <<
                     "non-alphanumeric characters. Skipping.");
        return false;
    }

    const std::string silo_matset_name = write_overlink ? "MATERIAL" : matset_name;

    if (used_names.find(silo_matset_name) != used_names.end())
    {
        CONDUIT_INFO("The name " << silo_matset_name << " has already been saved to "
                     "Silo as the name for a different object. Saving this matset "
                     "will overwrite that previous object, so we will skip matset "
                     << matset_name << ".");
        return false;
    }

    if (!n_mesh_info.has_path(topo_name))
    {
        CONDUIT_INFO("Skipping this matset because the linked "
                     "topology is invalid: "
                      << "matsets/" << matset_name
                      << "/topology: " << topo_name);
        return false;
    }
    const std::string silo_meshname = write_overlink ? "MESH" : topo_name;

    // use to_silo utility to create the needed silo arrays
    // cache all of these for later (in case we are writing specsets. If not, it doesn't hurt)
    Node &silo_matset = n_mesh_info["matsets"][matset_name]["silo_matset"];
    Node &silo_matset_compact = n_mesh_info["matsets"][matset_name]["silo_matset_compact"];
    Node &silo_mix_vfs_final = n_mesh_info["matsets"][matset_name]["silo_mix_vfs_final"];
    conduit::blueprint::mesh::matset::to_silo(n_matset, silo_matset);

    // compact the arrays if necessary
    detail::conditional_compact(silo_matset, silo_matset_compact);

    // mix vfs must be doubles for overlink
    if (write_overlink)
    {
        detail::convert_to_double_array(silo_matset_compact["mix_vf"], silo_mix_vfs_final);
    }
    else
    {
        silo_mix_vfs_final.set_external(silo_matset_compact["mix_vf"]);
    }

    // extract data from material map
    int nmat;
    std::vector<std::string> matnames;
    std::vector<const char *> matname_ptrs;
    std::vector<int> matnos;
    detail::read_material_map(silo_matset_compact["material_map"],
                              nmat,
                              matnames,
                              matname_ptrs,
                              matnos);

    // save topo name in case we are saving species as well
    n_mesh_info["matsets"][matset_name]["topo_name"] = topo_name;

    const int num_elems = n_mesh_info[topo_name]["num_elems"].to_value();
    CONDUIT_ASSERT(num_elems == silo_matset_compact["matlist"].dtype().number_of_elements(),
        "matset " << matset_name << " must have the same number of elements as its associated topology.");

    int dims[] = {0,0,0};
    const int ndims = detail::read_dims_from_mesh_info(n_mesh_info[topo_name], dims);

    // get the length of the mixed data arrays
    const int mixlen = silo_matset_compact["mix_mat"].dtype().number_of_elements();

    // get the datatype of the volume fractions
    const int mat_type = detail::dtype_to_silo_type(silo_mix_vfs_final.dtype());
    CONDUIT_ASSERT(mat_type == DB_FLOAT || mat_type == DB_DOUBLE,
        "Invalid matset volume fraction type: " << silo_mix_vfs_final.dtype().to_string());

    // create optlist and add to it
    detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
        DBMakeOptlist(1),
        &DBFreeOptlist,
        "Error freeing optlist."};
    CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating optlist");
    CONDUIT_CHECK_SILO_ERROR(
        DBAddOption(optlist.getSiloObject(),
                    DBOPT_MATNAMES,
                    matname_ptrs.data()),
        "error adding matnames option");

    Node int_arrays;
    detail::convert_to_c_int_array(silo_matset_compact["mix_mat"], int_arrays["mix_mat"]);
    detail::convert_to_c_int_array(silo_matset_compact["mix_next"], int_arrays["mix_next"]);
    detail::convert_to_c_int_array(silo_matset_compact["matlist"], int_arrays["matlist"]);

    CONDUIT_CHECK_SILO_ERROR(
        DBPutMaterial(dbfile, // Database file pointer
                      silo_matset_name.c_str(), // matset name
                      silo_meshname.c_str(), // mesh name
                      nmat, // number of materials
                      matnos.data(), // material numbers
                      int_arrays["matlist"].value(),
                      dims,                           // number of elements in each dimension in matlist
                      ndims,                          // number of dimensions in dims
                      int_arrays["mix_next"].value(),
                      int_arrays["mix_mat"].value(),
                      NULL,                           // mix zone is optional
                      silo_mix_vfs_final.data_ptr(),  // volume fractions
                      mixlen,                         // length of mixed data arrays
                      mat_type,                       // data type of volume fractions
                      optlist.getSiloObject()),       // optlist
        "DBPutMaterial");

    Node bookkeeping_info;
    bookkeeping_info["comp_info"]["comp"] = "matsets";
    bookkeeping_info["comp_info"]["comp_name"] = matset_name;
    bookkeeping_info["domain_info"]["local_num_domains"] = local_num_domains;
    bookkeeping_info["domain_info"]["local_domain_index"] = local_domain_index;
    bookkeeping_info["domain_info"]["global_domain_id"] = global_domain_id;
    bookkeeping_info["write_overlink"] = (write_overlink ? "yes" : "no");

    // bookkeeping
    detail::track_local_type_domain_info(bookkeeping_info, local_type_domain_info);

    used_names.insert(silo_matset_name);

    return true;
}

//---------------------------------------------------------------------------//
void silo_write_specset(DBfile *dbfile,
                        const std::string &specset_name,
                        const Node &n_specset,
                        const std::string &matset_name,
                        const bool write_overlink,
                        const int local_num_domains,
                        const int local_domain_index,
                        const uint64 global_domain_id,
                        const std::map<std::string, std::pair<std::string, std::string>> &ovl_specset_names,
                        const Node &n_mesh_info,
                        std::set<std::string> &used_names,
                        Node &local_type_domain_info)
{
    if (! detail::check_alphanumeric(specset_name))
    {
        CONDUIT_INFO("Specset name " << specset_name << " contains " <<
                     "non-alphanumeric characters. Skipping.");
        return;
    }

    std::string silo_specset_name;
    if (write_overlink)
    {
        if (ovl_specset_names.find(specset_name) != ovl_specset_names.end())
        {
            silo_specset_name = ovl_specset_names.at(specset_name).first;
            if ("ERROR" == silo_specset_name)
            {
                CONDUIT_INFO(ovl_specset_names.at(specset_name).second);
                return;
            }
        }
        else
        {
            CONDUIT_INFO("Specset with name " << specset_name << " is missing from "
                         "Blueprint index. Skipping.");
            return;
        }
    }
    else
    {
        silo_specset_name = specset_name;
    }

    if (used_names.find(silo_specset_name) != used_names.end())
    {
        CONDUIT_INFO("The name " << silo_specset_name << " has already been saved to "
                     "Silo as the name for a different object. Saving this specset "
                     "will overwrite that previous object, so we will skip specset "
                     << specset_name << ".");
        return;
    }

    const Node &silo_matset = n_mesh_info["matsets"][matset_name]["silo_matset_compact"];

    // TODO remove this once we add support for all specset flavors to to_silo
    if (silo_matset["buffer_style"].as_string() != "multi" ||
        silo_matset["dominance"].as_string() != "element")
    {
        CONDUIT_INFO("TODO Currently specsets can only be saved to silo if "
                     "they are multi_buffer + element_dominant.");
        return;
    }

    Node silo_specset;
    conduit::blueprint::mesh::specset::to_silo(n_specset, silo_matset, silo_specset);

    // get the datatype of the species_mf
    const int datatype = DB_DOUBLE; // to_silo produces species_mf data using float64s

    // get the length of the mixed data arrays
    const int mixlen = silo_specset["mixlen"].to_value();

    const std::vector<std::string> specnames = silo_specset["specnames"].child_names();

    // package up char ptrs for silo
    std::vector<const char *> specname_ptrs;
    for (size_t i = 0; i < specnames.size(); i ++)
    {
        specname_ptrs.push_back(specnames[i].c_str());
    }

    // create optlist and add to it
    detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
        DBMakeOptlist(1),
        &DBFreeOptlist,
        "Error freeing optlist."};
    CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating optlist");
    CONDUIT_CHECK_SILO_ERROR(
        DBAddOption(optlist.getSiloObject(),
                    DBOPT_SPECNAMES,
                    specname_ptrs.data()),
        "error adding specnames option");

    // TODO adding the specnames appears to be bugged - see output files

    const std::string silo_matset_name = write_overlink ? "MATERIAL" : matset_name;

    const int nmat = silo_specset["nmat"].to_value();

    Node int_arrays;
    detail::convert_to_c_int_array(silo_specset["nmatspec"], int_arrays["nmatspec"]);
    detail::convert_to_c_int_array(silo_specset["speclist"], int_arrays["speclist"]);
    detail::convert_to_c_int_array(silo_specset["mix_spec"], int_arrays["mix_spec"]);

    const std::string topo_name = n_mesh_info["matsets"][matset_name]["topo_name"].as_string();
    int dims[] = {0,0,0};
    const int ndims = detail::read_dims_from_mesh_info(n_mesh_info[topo_name], dims);

    const int nspecies_mf = silo_specset["nspecies_mf"].to_value();

    CONDUIT_CHECK_SILO_ERROR(
        DBPutMatspecies(dbfile,                                // Database file pointer
                        silo_specset_name.c_str(),             // specset name
                        silo_matset_name.c_str(),              // matset name
                        nmat,                                  // number of materials
                        int_arrays["nmatspec"].value(),        // number of species associated with each material
                        int_arrays["speclist"].value(),        // indices into species_mf and mix_spec
                        dims,                                  // array of length ndims that defines the shape of the speclist array
                        ndims,                                 // number of dimensions in the speclist array
                        nspecies_mf,                           // length of the species_mf array
                        silo_specset["species_mf"].data_ptr(), // mass fractions of the matspecies in an array of length nspecies_mf
                        int_arrays["mix_spec"].value(),        // array of length mixlen containing indices into the species_mf array
                        mixlen,                                // length of mix_spec array
                        datatype,                              // datatype of mass fraction data in species_mf
                        optlist.getSiloObject()),              // optlist
        "DBPutMatspecies");

    Node bookkeeping_info;
    bookkeeping_info["comp_info"]["comp"] = "specsets";
    bookkeeping_info["comp_info"]["comp_name"] = specset_name;
    bookkeeping_info["domain_info"]["local_num_domains"] = local_num_domains;
    bookkeeping_info["domain_info"]["local_domain_index"] = local_domain_index;
    bookkeeping_info["domain_info"]["global_domain_id"] = global_domain_id;
    bookkeeping_info["write_overlink"] = (write_overlink ? "yes" : "no");

    // bookkeeping
    detail::track_local_type_domain_info(bookkeeping_info, local_type_domain_info);

    used_names.insert(silo_specset_name);
}

//---------------------------------------------------------------------------//
void silo_mesh_write(DBfile *dbfile,
                     const Node &mesh_domain,
                     const std::string &silo_obj_path,
                     const std::string &ovl_topo_name,
                     const int local_num_domains,
                     const int local_domain_index,
                     const uint64 global_domain_id,
                     const bool write_overlink,
                     const std::map<std::string, std::pair<std::string, std::string>> &ovl_specset_names,
                     Node &local_type_domain_info)
{
    // TODO audit errors and find places we can skip instead of erroring

    int silo_error = 0;
    char silo_prev_dir[256];
    if (!silo_obj_path.empty())
    {
        silo_error += DBGetDir(dbfile, silo_prev_dir);

        std::string dir;
        std::stringstream ss(silo_obj_path);
        while (getline(ss, dir, '/'))
        {
            // create the directory if it doesn't already exist
            DBMkDir(dbfile, dir.c_str()); // if this fails we want to keep going
            silo_error += DBSetDir(dbfile, dir.c_str());
        }
        CONDUIT_CHECK_SILO_ERROR(silo_error,
                                 "failed to make silo directory: "
                                 << silo_obj_path);
    }

    // In blueprint, you can have a topo, field, matset, and specset that all have the
    // same name. In silo, that will break things. They will overwrite each other.
    // So we need to keep track of the names we have used.
    // We only care about tracking this in the non-Overlink case as in Overlink,
    // naming conventions are rigid and do not allow for this case.
    std::set<std::string> used_names;
    Node n_mesh_info;

    if (write_overlink)
    {
        if (mesh_domain["topologies"].has_child(ovl_topo_name))
        {
            // we choose one topo to write out: ovl_topo_name
            silo_write_topo(dbfile,
                            mesh_domain,
                            ovl_topo_name,
                            write_overlink,
                            local_num_domains,
                            local_domain_index,
                            global_domain_id,
                            n_mesh_info,
                            local_type_domain_info);
        }
    }
    else
    {
        // we write out all topos
        auto topo_itr = mesh_domain["topologies"].children();
        while (topo_itr.has_next())
        {
            topo_itr.next();
            const std::string topo_name = topo_itr.name();
            if (silo_write_topo(dbfile,
                                mesh_domain,
                                topo_name,
                                write_overlink,
                                local_num_domains,
                                local_domain_index,
                                global_domain_id,
                                n_mesh_info,
                                local_type_domain_info))
            {
                used_names.insert(topo_name);
            }
        }
    }

    // either we are not writing overlink, or there is a matset present
    // overlink requires a matset
    CONDUIT_ASSERT(! write_overlink || mesh_domain.has_path("matsets"),
        "Writing to Overlink requires a matset.");

    if (mesh_domain.has_path("matsets"))
    {
        // We want to enforce that there is only one matset per topo
        // that we save out to silo. Multiple matsets for a topo is
        // supported in blueprint, but in silo it is ambiguous, as
        // silo provides no link from fields back to matsets. Therefore
        // we enforce one matset per topo.

        // the names of the topos the matsets are associated with
        std::set<std::string> topo_names;
        auto matset_itr = mesh_domain["matsets"].children();
        while (matset_itr.has_next())
        {
            const Node &n_matset = matset_itr.next();
            const std::string matset_name = matset_itr.name();

            if (!n_matset.has_path("topology"))
            {
                CONDUIT_INFO("Skipping this matset because we are "
                             "missing a linked topology: "
                              << "matsets/" << matset_name << "/topology");
                continue;
            }

            const std::string topo_name = n_matset["topology"].as_string();

            // if we've already written a matset for this topo successfully
            if (topo_names.find(topo_name) != topo_names.end())
            {
                CONDUIT_INFO("There are multiple matsets that belong to the same topology "
                             "for topo " << topo_name << ". This is ambiguous in Silo. "
                             "Skipping matset " << matset_name << ".");
                continue;
            }

            if (! write_overlink || topo_name == ovl_topo_name)
            {
                if (silo_write_matset(dbfile,
                                      matset_name,
                                      n_matset,
                                      topo_name,
                                      write_overlink,
                                      local_num_domains,
                                      local_domain_index,
                                      global_domain_id,
                                      n_mesh_info,
                                      used_names,
                                      local_type_domain_info))
                {
                    topo_names.insert(topo_name);
                }
            }
        }
    }

    if (mesh_domain.has_path("specsets"))
    {
        auto specset_itr = mesh_domain["specsets"].children();
        while (specset_itr.has_next())
        {
            const Node &n_specset = specset_itr.next();
            const std::string specset_name = specset_itr.name();
            if (! n_specset.has_child("matset"))
            {
                CONDUIT_INFO("Skipping this specset because we are "
                             "missing a linked matset: "
                              << "specsets/" << specset_name << "/matset");
                continue;
            }
            const std::string matset_name = n_specset["matset"].as_string();
            if (! n_mesh_info.has_path("matsets/" + matset_name + "/topo_name"))
            {
                CONDUIT_INFO("Skipping this specset because the linked "
                             "matset is invalid or was not written: "
                              << "specsets/" << specset_name
                              << "/matset: " << matset_name);
                continue;
            }
            const std::string topo_name = n_mesh_info["matsets"][matset_name]["topo_name"].as_string();
            if (! write_overlink || topo_name == ovl_topo_name)
            {
                silo_write_specset(dbfile,
                                   specset_name,
                                   n_specset,
                                   matset_name,
                                   write_overlink,
                                   local_num_domains,
                                   local_domain_index,
                                   global_domain_id,
                                   ovl_specset_names,
                                   n_mesh_info,
                                   used_names,
                                   local_type_domain_info);
            }
        }
    }

    if (mesh_domain.has_path("fields"))
    {
        auto field_itr = mesh_domain["fields"].children();
        while (field_itr.has_next())
        {
            const Node &n_var = field_itr.next();
            const std::string var_name = field_itr.name();
            if (! n_var.has_path("topology"))
            {
                CONDUIT_INFO("Skipping this variable because we are "
                             "missing a linked topology: "
                              << "fields/" << var_name << "/topology");
                continue;
            }
            const std::string topo_name = n_var["topology"].as_string();
            if (! write_overlink || topo_name == ovl_topo_name)
            {
                silo_write_field(dbfile,
                                 var_name,
                                 n_var,
                                 topo_name,
                                 mesh_domain,
                                 write_overlink,
                                 local_num_domains,
                                 local_domain_index,
                                 global_domain_id,
                                 n_mesh_info,
                                 used_names,
                                 local_type_domain_info);
            }
        }
    }

    // we only write adjacency set information if we are writing overlink
    if (write_overlink)
    {
        if (mesh_domain.has_path("adjsets"))
        {
            if (mesh_domain["adjsets"].number_of_children() > 1)
            {
                CONDUIT_INFO("Only one adjset belonging to topology "
                             << ovl_topo_name << " will be saved as "
                             "per the Overlink spec.");
            }
            auto adjset_itr = mesh_domain["adjsets"].children();
            while (adjset_itr.has_next())
            {
                const Node &n_adjset = adjset_itr.next();
                if (n_adjset["topology"].as_string() == ovl_topo_name)
                {
                    silo_write_adjset(dbfile, &n_adjset);
                }
                // we will give up after writing 1 because we can only have
                // 1 adjset per topo
                break;
            }
        }
        else
        {
            // we still need to write things even if there is no adjset
            silo_write_adjset(dbfile, nullptr);
        }
    }

    if (!silo_obj_path.empty())
    {
        silo_error = DBSetDir(dbfile, silo_prev_dir);
        CONDUIT_CHECK_SILO_ERROR(silo_error,
                                 "changing silo directory to previous path");
    }
}

//-----------------------------------------------------------------------------
void write_multimesh(DBfile *dbfile,
                     const Node &n_mesh,
                     const std::string &topo_name,
                     const Node &root,
                     const int global_num_domains,
                     const std::string &multimesh_name,
                     const bool overlink,
                     const bool prefer_unified_types,
                     const bool do_nameschemes)
{
    const int num_files = root["number_of_files"].as_int();
    const bool root_only = root["file_style"].as_string() == "root_only";
    const std::string silo_meshname = overlink ? "MESH": topo_name;
    const std::string &global_file_namescheme = root["global_file_namescheme"].as_string();
    const std::string &global_block_namescheme = root["global_block_namescheme"].as_string();
    std::vector<std::string> domain_name_strings;
    std::vector<int> empty_domains;

    detail::generate_silo_mb_data(n_mesh["state"],
                                  root["silo_path"].as_string(),
                                  silo_meshname,
                                  num_files,
                                  global_num_domains,
                                  root_only,
                                  root["type_domain_info"]["meshes"][topo_name + "_status"],
                                  do_nameschemes,
                                  domain_name_strings,
                                  &empty_domains);

    // create optlist
    detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
        DBMakeOptlist(4),
        &DBFreeOptlist,
        "Error freeing optlist."};
    CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating optlist");

    int cycle;
    float ftime;
    double dtime;
    if (n_mesh.has_child("state"))
    {
        int silo_error = 0;
        const Node &n_state = n_mesh["state"];
        if (n_state.has_child("cycle"))
        {
            cycle = n_state["cycle"].to_int();
            silo_error += DBAddOption(optlist.getSiloObject(),
                                      DBOPT_CYCLE,
                                      &cycle);
        }
        if (n_state.has_child("time"))
        {
            ftime = n_state["time"].to_float();
            silo_error += DBAddOption(optlist.getSiloObject(),
                                      DBOPT_TIME,
                                      &ftime);
            dtime = n_state["time"].to_double();
            silo_error += DBAddOption(optlist.getSiloObject(),
                                      DBOPT_DTIME,
                                      &dtime);
        }
        CONDUIT_CHECK_SILO_ERROR(silo_error,
                                 "creating optlist (time, cycle) ");
    }

    // need to create vars out here so that they have scope for the function lifetime
    int *mesh_types_ptr = nullptr;
    int32 mesh_type;
    const bool all_types_the_same = detail::all_types_the_same(
                                        root["type_domain_info"]["meshes"][topo_name + "_types"].value(),
                                        root["type_domain_info"]["meshes"][topo_name + "_status"].value(),
                                        mesh_type);
    if (prefer_unified_types && all_types_the_same && mesh_type != -1)
    {
        CONDUIT_CHECK_SILO_ERROR(
            DBAddOption(optlist.getSiloObject(),
                        DBOPT_MB_BLOCK_TYPE,
                        &mesh_type),
            "Error adding block type option for DBPutMultimesh for " << multimesh_name << ".");
    }
    else
    {
        mesh_types_ptr = const_cast<int *>(root["type_domain_info"]["meshes"][topo_name + "_types"].as_int32_ptr());
    }

    // need to create vars out here so they have lifetime thru the end of the function
    std::vector<const char *> domain_name_ptrs;
    std::string block_namescheme;
    const int num_empty_doms = static_cast<int>(empty_domains.size());
    const char **dom_names_ptr = 
        detail::handle_nameschemes_or_pathnames(do_nameschemes,
                                                domain_name_strings,
                                                global_file_namescheme,
                                                global_block_namescheme,
                                                silo_meshname,
                                                multimesh_name,
                                                "DBPutMultimesh",
                                                num_empty_doms,
                                                empty_domains,
                                                optlist.getSiloObject(),
                                                block_namescheme,
                                                domain_name_ptrs);

    CONDUIT_CHECK_SILO_ERROR(
        DBPutMultimesh(
            dbfile,
            multimesh_name.c_str(),
            global_num_domains,
            dom_names_ptr,
            mesh_types_ptr,
            optlist.getSiloObject()),
        "Error putting multimesh corresponding to topo: " << topo_name);
}

//-----------------------------------------------------------------------------
void write_multimeshes(DBfile *dbfile,
                       const std::string &opts_out_mesh_name,
                       const std::string &ovl_topo_name,
                       const Node &root,
                       const bool write_overlink,
                       const bool prefer_unified_types,
                       const bool do_nameschemes)
{
    const int global_num_domains = root["number_of_domains"].to_index_t();
    const Node &n_mesh = root["blueprint_index"][opts_out_mesh_name];
    const Node &n_type_dom_info = root["type_domain_info"];

    // these should be the same b/c the num domains the bp index was given
    // was global_num_domains
    CONDUIT_ASSERT(((index_t) global_num_domains) == n_mesh["state/number_of_domains"].to_index_t(),
        "Domain count mismatch");

    // write only the chosen mesh for overlink case
    if (write_overlink)
    {
        write_multimesh(dbfile,
                        n_mesh,
                        ovl_topo_name,
                        root,
                        global_num_domains,
                        opts_out_mesh_name, // "MMESH"
                        write_overlink,
                        prefer_unified_types,
                        do_nameschemes);
    }
    // write all meshes for nonoverlink case
    else
    {
        auto topo_itr = n_mesh["topologies"].children();
        while (topo_itr.has_next())
        {
            topo_itr.next();
            const std::string topo_name = topo_itr.name();
            const std::string multimesh_name = opts_out_mesh_name + "_" + topo_name;

            // did we actually write this mesh to silo?
            if (! n_type_dom_info.has_path("meshes/" + topo_name + "_status"))
            {
                // we skipped this mesh before so we can skip it now
                continue;
            }

            write_multimesh(dbfile,
                            n_mesh,
                            topo_name,
                            root,
                            global_num_domains,
                            multimesh_name,
                            write_overlink,
                            prefer_unified_types,
                            do_nameschemes);
        }
    }
}

//-----------------------------------------------------------------------------
void
write_multivars(DBfile *dbfile,
                const std::string &opts_mesh_name,
                const std::string &ovl_topo_name,
                const Node &root,
                const bool write_overlink,
                const bool prefer_unified_types,
                const bool do_nameschemes)
{
    const int num_files = root["number_of_files"].to_index_t();
    const int global_num_domains = root["number_of_domains"].to_index_t();
    const Node &n_mesh = root["blueprint_index"][opts_mesh_name];
    const Node &n_type_dom_info = root["type_domain_info"];
    const bool root_only = root["file_style"].as_string() == "root_only";
    const std::string &global_file_namescheme = root["global_file_namescheme"].as_string();
    const std::string &global_block_namescheme = root["global_block_namescheme"].as_string();

    // these should be the same b/c the num domains the bp index was given
    // was global_num_domains
    CONDUIT_ASSERT(((index_t) global_num_domains) == n_mesh["state/number_of_domains"].to_index_t(),
        "Domain count mismatch");

    if (n_mesh.has_child("fields"))
    {
        auto field_itr = n_mesh["fields"].children();
        while (field_itr.has_next())
        {
            const Node &n_var = field_itr.next();
            const std::string var_name = field_itr.name();

            // did we actually write this field to silo?
            if (write_overlink)
            {
                if (n_var["number_of_components"].to_int64() != 1)
                {
                    if (! n_type_dom_info.has_path("ovl_var_parents/" + var_name))
                    {
                        // we skipped this field before so we can skip it now
                        continue;
                    }
                }
                else
                {
                    if (! n_type_dom_info.has_path("vars/" + var_name + "_status"))
                    {
                        // we skipped this field before so we can skip it now
                        continue;
                    }
                }
            }
            else
            {
                if (! n_type_dom_info.has_path("vars/" + var_name + "_status"))
                {
                    // we skipped this field before so we can skip it now
                    continue;
                }
            }

            auto write_multivar = [&](const std::string var_name)
            {
                const std::string linked_topo_name = n_var["topology"].as_string();

                // TODO do we need this check? Did we already check this before when writing fields?
                // if we are not writing overlink, we can go ahead
                // if we are writing overlink, we must ensure we are dealing with
                // the correct topology.
                if (! write_overlink || linked_topo_name == ovl_topo_name)
                {
                    std::vector<std::string> var_name_strings;
                    std::vector<int> empty_domains;
                    detail::generate_silo_mb_data(n_mesh["state"],
                                                  root["silo_path"].as_string(),
                                                  var_name,
                                                  num_files,
                                                  global_num_domains,
                                                  root_only,
                                                  root["type_domain_info"]["vars"][var_name + "_status"],
                                                  do_nameschemes,
                                                  var_name_strings,
                                                  &empty_domains);

                    // create optlist
                    detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
                        DBMakeOptlist(1),
                        &DBFreeOptlist,
                        "Error freeing optlist."};
                    CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating options");

                    std::string multimesh_name, multivar_name;
                    if (write_overlink)
                    {
                        multimesh_name = opts_mesh_name;
                        multivar_name = var_name;
                    }
                    else
                    {
                        multimesh_name = opts_mesh_name + "_" + linked_topo_name;
                        multivar_name = opts_mesh_name + "_" + var_name;
                    }

                    // have to const_cast because converting to void *
                    CONDUIT_CHECK_SILO_ERROR(
                        DBAddOption(optlist.getSiloObject(),
                                    DBOPT_MMESH_NAME,
                                    const_cast<char *>(multimesh_name.c_str())),
                        "Error creating options for putting multivar");

                    int *var_types_ptr = nullptr;
                    int var_type;
                    const bool all_types_the_same = detail::all_types_the_same(
                                                        root["type_domain_info"]["vars"][var_name + "_types"].value(),
                                                        root["type_domain_info"]["vars"][var_name + "_status"].value(),
                                                        var_type);
                    if (prefer_unified_types && all_types_the_same && var_type != -1)
                    {
                        CONDUIT_CHECK_SILO_ERROR(
                            DBAddOption(optlist.getSiloObject(),
                                        DBOPT_MB_BLOCK_TYPE,
                                        &var_type),
                            "Error adding block type option for DBPutMultivar for " << multivar_name << ".");
                    }
                    else
                    {
                        var_types_ptr = const_cast<int *>(root["type_domain_info"]["vars"][var_name + "_types"].as_int32_ptr());
                    }

                    // need to create vars out here so they have lifetime thru the end of the function
                    std::vector<const char *> var_name_ptrs;
                    std::string block_namescheme;
                    const int num_empty_doms = static_cast<int>(empty_domains.size());
                    const char **var_names_ptr = 
                        detail::handle_nameschemes_or_pathnames(do_nameschemes,
                                                                var_name_strings,
                                                                global_file_namescheme,
                                                                global_block_namescheme,
                                                                var_name,
                                                                multivar_name,
                                                                "DBPutMultivar",
                                                                num_empty_doms,
                                                                empty_domains,
                                                                optlist.getSiloObject(),
                                                                block_namescheme,
                                                                var_name_ptrs);

                    CONDUIT_CHECK_SILO_ERROR(
                        DBPutMultivar(
                            dbfile,
                            multivar_name.c_str(),
                            global_num_domains,
                            var_names_ptr,
                            var_types_ptr,
                            optlist.getSiloObject()),
                        "Error putting multivar corresponding to field: " << var_name);
                }
            };

            // in this case we have to write multiple multivars, one for each comp
            if (write_overlink && n_var["number_of_components"].to_int64() != 1)
            {
                std::vector<std::string> comp_var_names =
                    n_type_dom_info["ovl_var_parents"][var_name].child_names();
                for (size_t comp_id = 0; comp_id < comp_var_names.size(); comp_id ++)
                {
                    write_multivar(comp_var_names[comp_id]);
                }
            }
            else
            {
                write_multivar(var_name);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
write_multimats(DBfile *dbfile,
                const std::string &opts_mesh_name,
                const std::string &ovl_topo_name,
                const Node &root,
                const bool write_overlink,
                const bool do_nameschemes)
{
    const int num_files = root["number_of_files"].to_index_t();
    const int global_num_domains = root["number_of_domains"].to_index_t();
    const Node &n_mesh = root["blueprint_index"][opts_mesh_name];
    const Node &n_type_dom_info = root["type_domain_info"];
    const bool root_only = root["file_style"].as_string() == "root_only";
    const std::string &global_file_namescheme = root["global_file_namescheme"].as_string();
    const std::string &global_block_namescheme = root["global_block_namescheme"].as_string();

    // these should be the same b/c the num domains the bp index was given
    // was global_num_domains
    CONDUIT_ASSERT(((index_t) global_num_domains) == n_mesh["state/number_of_domains"].to_index_t(),
        "Domain count mismatch");

    if (n_mesh.has_child("matsets"))
    {
        auto matset_itr = n_mesh["matsets"].children();
        while (matset_itr.has_next())
        {
            const Node &n_matset = matset_itr.next();
            const std::string matset_name = matset_itr.name();

            // did we actually write this matset to silo?
            if (! n_type_dom_info.has_path("matsets/" + matset_name + "_status"))
            {
                // we skipped this matset before so we can skip it now
                continue;
            }

            // TODO do we need to check if this topo was written?
            const std::string linked_topo_name = n_matset["topology"].as_string();

            // if we are not writing overlink, we can go ahead
            // if we are writing overlink, we must ensure we are dealing with
            // the correct topology.
            if (! write_overlink || linked_topo_name == ovl_topo_name)
            {
                const std::string silo_matset_name = (write_overlink ? "MATERIAL" : matset_name);

                std::vector<std::string> matset_name_strings;
                std::vector<int> empty_domains;
                detail::generate_silo_mb_data(n_mesh["state"],
                                              root["silo_path"].as_string(),
                                              silo_matset_name,
                                              num_files,
                                              global_num_domains,
                                              root_only,
                                              root["type_domain_info"]["matsets"][matset_name + "_status"],
                                              do_nameschemes,
                                              matset_name_strings,
                                              &empty_domains);

                // create optlist
                detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
                    DBMakeOptlist(4),
                    &DBFreeOptlist,
                    "Error freeing optlist."};
                CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating options");

                const std::string multimesh_name = (write_overlink ?
                                                    opts_mesh_name :
                                                    opts_mesh_name + "_" + linked_topo_name);
                const std::string multimat_name = (write_overlink ?
                                                   "MMATERIAL" :
                                                   opts_mesh_name + "_" + silo_matset_name);

                // extract info from the material map to save to dbopts
                int nmat;
                std::vector<std::string> matnames;
                std::vector<const char *> matname_ptrs;
                std::vector<int> matnos;
                detail::read_material_map(n_matset["material_map"],
                                          nmat,
                                          matnames,
                                          matname_ptrs,
                                          matnos);

                // have to const_cast because converting to void *
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_MMESH_NAME,
                                const_cast<char *>(multimesh_name.c_str())),
                    "Error adding mmesh name db option.");
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_NMATNOS,
                                &nmat),
                    "Error adding nmatnos db option.");
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_MATNOS,
                                matnos.data()),
                    "Error adding matnos db option.");
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_MATNAMES,
                                matname_ptrs.data()),
                    "Error adding matnames db option.");

                // need to create vars out here so they have lifetime thru the end of the function
                std::vector<const char *> matset_name_ptrs;
                std::string block_namescheme;
                const int num_empty_doms = static_cast<int>(empty_domains.size());
                const char **matset_names_ptr = 
                    detail::handle_nameschemes_or_pathnames(do_nameschemes,
                                                            matset_name_strings,
                                                            global_file_namescheme,
                                                            global_block_namescheme,
                                                            silo_matset_name,
                                                            multimat_name,
                                                            "DBPutMultimat",
                                                            num_empty_doms,
                                                            empty_domains,
                                                            optlist.getSiloObject(),
                                                            block_namescheme,
                                                            matset_name_ptrs);

                CONDUIT_CHECK_SILO_ERROR(
                    DBPutMultimat(
                        dbfile,
                        multimat_name.c_str(),
                        global_num_domains,
                        matset_names_ptr,
                        optlist.getSiloObject()),
                    "Error putting multimaterial corresponding to matset: " << matset_name);
            }
        }
    }
}

//-----------------------------------------------------------------------------
int
write_multimatspecs(DBfile *dbfile,
                    const std::string &opts_mesh_name,
                    const std::string &ovl_topo_name,
                    const Node &root,
                    const bool write_overlink,
                    const std::map<std::string, std::pair<std::string, std::string>> &ovl_specset_names,
                    const bool do_nameschemes)
{
    const int num_files = root["number_of_files"].to_index_t();
    const int global_num_domains = root["number_of_domains"].to_index_t();
    const Node &n_mesh = root["blueprint_index"][opts_mesh_name];
    const Node &n_type_dom_info = root["type_domain_info"];
    const bool root_only = root["file_style"].as_string() == "root_only";
    const std::string &global_file_namescheme = root["global_file_namescheme"].as_string();
    const std::string &global_block_namescheme = root["global_block_namescheme"].as_string();

    // we have to keep track for overlink
    int num_specsets_written = 0;

    // these should be the same b/c the num domains the bp index was given
    // was global_num_domains
    CONDUIT_ASSERT(((index_t) global_num_domains) == n_mesh["state/number_of_domains"].to_index_t(),
        "Domain count mismatch");

    if (n_mesh.has_child("specsets"))
    {
        // Overlink asks that the first multimatspecies object is names MSPECIES
        // and ensuing multimatspecies objects are named MSPECIES1, MSPECIES2, etc.
        // so we track how many we have written to name them appropriately
        auto specset_itr = n_mesh["specsets"].children();
        while (specset_itr.has_next())
        {
            const Node &n_specset = specset_itr.next();
            const std::string specset_name = specset_itr.name();

            // did we actually write this specset to silo?
            if (! n_type_dom_info.has_path("specsets/" + specset_name + "_status"))
            {
                // we skipped this specset before so we can skip it now
                continue;
            }

            const std::string linked_matset_name = n_specset["matset"].as_string();
            if (! n_mesh.has_path("matsets/" + linked_matset_name + "/topology"))
            {
                // either matset doesn't exist or it has no linked topo
                continue;
            }

            const Node &n_matset = n_mesh["matsets"][linked_matset_name];

            const std::string linked_topo_name = n_mesh["matsets"][linked_matset_name]["topology"].as_string();

            // if we are not writing overlink, we can go ahead
            // if we are writing overlink, we must ensure we are dealing with
            // the correct topology.
            if (! write_overlink || linked_topo_name == ovl_topo_name)
            {
                std::string silo_specset_name;
                if (write_overlink)
                {
                    if (ovl_specset_names.find(specset_name) != ovl_specset_names.end())
                    {
                        silo_specset_name = ovl_specset_names.at(specset_name).first;
                        if ("ERROR" == silo_specset_name)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    silo_specset_name = specset_name;
                }

                std::vector<std::string> specset_name_strings;
                std::vector<int> empty_domains;
                detail::generate_silo_mb_data(n_mesh["state"],
                                              root["silo_path"].as_string(),
                                              silo_specset_name,
                                              num_files,
                                              global_num_domains,
                                              root_only,
                                              root["type_domain_info"]["specsets"][specset_name + "_status"],
                                              do_nameschemes,
                                              specset_name_strings,
                                              &empty_domains);

                const std::string silo_matset_name = (write_overlink ? "MATERIAL" : linked_matset_name);
                const std::string multimesh_name = (write_overlink ?
                                                    opts_mesh_name :
                                                    opts_mesh_name + "_" + linked_topo_name);
                const std::string multimat_name = (write_overlink ?
                                                   "MMATERIAL" :
                                                   opts_mesh_name + "_" + silo_matset_name);
                const std::string multimatspec_name = (write_overlink ?
                                                       "M" + silo_specset_name :
                                                       opts_mesh_name + "_" + silo_specset_name);

                // extract info from the material map to save to dbopts
                int nmat;
                std::vector<std::string> matnames;
                std::vector<const char *> matname_ptrs;
                std::vector<int> matnos;
                detail::read_material_map(n_matset["material_map"],
                                          nmat,
                                          matnames,
                                          matname_ptrs,
                                          matnos);

                std::vector<std::pair<int, std::string>> matno_to_matname;
                for (size_t mat_idx = 0; mat_idx < matnames.size(); mat_idx ++)
                {
                    matno_to_matname.emplace_back(matnos[mat_idx], matnames[mat_idx]);
                }
                // this will sort by matno
                std::sort(matno_to_matname.begin(), matno_to_matname.end());

                std::vector<int> nmatspec;
                std::vector<std::string> specnames;

                for (const auto &matno_matname : matno_to_matname)
                {
                    const Node &curr_mat = n_specset["species"][matno_matname.second];
                    nmatspec.push_back(curr_mat.number_of_children());

                    // get the specie names for this material
                    auto spec_name_itr = curr_mat.children();
                    while (spec_name_itr.has_next())
                    {
                        spec_name_itr.next();
                        const std::string specname = spec_name_itr.name();
                        specnames.push_back(specname);
                    }
                }

                // package up char ptrs for silo
                std::vector<const char *> specname_ptrs;
                for (size_t i = 0; i < specnames.size(); i ++)
                {
                    specname_ptrs.push_back(specnames[i].c_str());
                }

                // create optlist
                detail::SiloObjectWrapperCheckError<DBoptlist, decltype(&DBFreeOptlist)> optlist{
                    DBMakeOptlist(4),
                    &DBFreeOptlist,
                    "Error freeing optlist."};
                CONDUIT_ASSERT(optlist.getSiloObject(), "Error creating options");

                // have to const_cast because converting to void *
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_MATNAME,
                                const_cast<char *>(multimat_name.c_str())),
                    "Error adding mmat name db option.");
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_NMAT,
                                &nmat),
                    "Error adding nmat db option.");
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_NMATSPEC,
                                nmatspec.data()),
                    "Error adding nmatspec db option.");
                CONDUIT_CHECK_SILO_ERROR(
                    DBAddOption(optlist.getSiloObject(),
                                DBOPT_SPECNAMES,
                                specname_ptrs.data()),
                    "error adding matnames option");

                // need to create vars out here so they have lifetime thru the end of the function
                std::vector<const char *> specset_name_ptrs;
                std::string block_namescheme;
                const int num_empty_doms = static_cast<int>(empty_domains.size());
                const char **specset_names_ptr = 
                    detail::handle_nameschemes_or_pathnames(do_nameschemes,
                                                            specset_name_strings,
                                                            global_file_namescheme,
                                                            global_block_namescheme,
                                                            silo_specset_name,
                                                            multimatspec_name,
                                                            "DBPutMultimatspecies",
                                                            num_empty_doms,
                                                            empty_domains,
                                                            optlist.getSiloObject(),
                                                            block_namescheme,
                                                            specset_name_ptrs);

                CONDUIT_CHECK_SILO_ERROR(
                    DBPutMultimatspecies(
                        dbfile,
                        multimatspec_name.c_str(),
                        global_num_domains,
                        specset_names_ptr,
                        optlist.getSiloObject()),
                    "Error putting multimaterial corresponding to specset: " << specset_name);

                num_specsets_written ++;
            }
        }
    }

    return num_specsets_written;
}

//-----------------------------------------------------------------------------
// only for overlink
void
write_pad_dims(DBfile *dbfile,
               const std::string &opts_mesh_name,
               const Node &root)
{
    const Node &n_mesh = root["blueprint_index"][opts_mesh_name];
    // this only applies to structured topos
    // (quadmeshes, so rectilinear, uniform, and structured)
    // we can grab the "first" topo because we know there is only one.
    const std::string topo_type = n_mesh["topologies"][0]["type"].as_string();
    if (topo_type == "structured" ||
        topo_type == "rectilinear" ||
        topo_type == "uniform")
    {
        char const *elemname = "paddims";
        const int elemlength = 6;
        const int nelems = 1;
        const int nvalues = 6;

        // we do not have a way to record ghost nodes in blueprint
        // so we just write out six zeroes to make overlink happy
        std::vector<int> paddim_vals(6, 0);

        CONDUIT_CHECK_SILO_ERROR(
            DBPutCompoundarray(dbfile, // dbfile
                               "PAD_DIMS", // name
                               &elemname, // elemnames
                               &elemlength, // elemlengths
                               nelems, // nelems
                               static_cast<void *>(paddim_vals.data()), // values
                               nvalues, // nvalues
                               DB_INT, // datatype
                               NULL), // optlist
            "Error writing pad dims.");
    }
}

//-----------------------------------------------------------------------------
// only for overlink
void
write_var_attributes(DBfile *dbfile,
                     const std::string &opts_mesh_name,
                     const Node &root)
{
    const Node &n_mesh = root["blueprint_index"][opts_mesh_name];
    const Node &n_type_dom_info = root["type_domain_info"];
    if (n_mesh.has_child("fields"))
    {
        std::vector<std::string> multivar_name_strings;
        std::vector<int> elemlengths;
        int nvalues = 0;
        std::vector<int> var_attr_values;

        auto field_itr = n_mesh["fields"].children();
        while (field_itr.has_next())
        {
            const Node &n_var = field_itr.next();
            const std::string var_name = field_itr.name();

            // did we actually write this field to silo?
            if (n_var["number_of_components"].to_int64() != 1)
            {
                if (! n_type_dom_info.has_path("ovl_var_parents/" + var_name))
                {
                    // we skipped this field before so we can skip it now
                    continue;
                }
            }
            else
            {
                if (! n_type_dom_info.has_path("vars/" + var_name + "_status"))
                {
                    // we skipped this field before so we can skip it now
                    continue;
                }
            }

            auto write_var_attr_for_field = [&](const std::string var_name)
            {
                multivar_name_strings.push_back(var_name);

                const int num_attr = 5; // we are writing 5 var attributes for now

                elemlengths.push_back(num_attr);
                nvalues += num_attr;

                //
                // centering: ATTR_NODAL 0, ATTR_ZONAL 1, ATTR_FACE, ATTR_EDGE
                //
                if (n_var["association"].as_string() == "vertex")
                {
                    var_attr_values.push_back(0); // nodal == vertex
                }
                else
                {
                    var_attr_values.push_back(1); // zonal == element
                }

                //
                // scaling property: ATTR_INTENSIVE 0, ATTR_EXTENSIVE 1
                //
                // intensive (0) IS NOT volume dependent
                // extensive (1) IS volume dependent
                if (n_var.has_child("volume_dependent") &&
                    n_var["volume_dependent"].as_string() == "true")
                {
                    var_attr_values.push_back(1); // extensive == volume dependent
                }
                else
                {
                    var_attr_values.push_back(0); // intensive == NOT volume dependent
                }

                //
                // linking: ATTR_FIRST ORDER 0, ATTR_SECOND ORDER 1
                //
                // Use ATTR_SECOND_ORDER which means it computes the gradient of the
                // field value in each zone for a second order remap of the values.
                // The first order remap is less accurate since it treats the value
                // as constant within the zone.
                var_attr_values.push_back(1);

                //
                // unused: 0
                //
                var_attr_values.push_back(0);

                //
                // data type: ATTR_INTEGER, ATTR_FLOAT
                //
                // we cached this info earlier, just need to retrieve it
                var_attr_values.push_back(n_type_dom_info["ovl_var_datatypes"][var_name].to_index_t());
            };

            if (n_var["number_of_components"].to_int64() != 1)
            {
                std::vector<std::string> comp_var_names =
                    n_type_dom_info["ovl_var_parents"][var_name].child_names();
                for (size_t comp_id = 0; comp_id < comp_var_names.size(); comp_id ++)
                {
                    write_var_attr_for_field(comp_var_names[comp_id]);
                }
            }
            else
            {
                write_var_attr_for_field(var_name);
            }
        }
        // package up char ptrs for silo
        std::vector<const char *> multivar_name_ptrs;
        for (size_t i = 0; i < multivar_name_strings.size(); i ++)
        {
            multivar_name_ptrs.push_back(multivar_name_strings[i].c_str());
        }

        CONDUIT_CHECK_SILO_ERROR(
            DBPutCompoundarray(dbfile, // dbfile
                               "VAR_ATTRIBUTES", // name
                               multivar_name_ptrs.data(), // elemnames
                               elemlengths.data(), // elemlengths
                               multivar_name_ptrs.size(), // nelems
                               static_cast<void *>(var_attr_values.data()), // values
                               nvalues, // nvalues
                               DB_INT, // datatype
                               NULL), // optlist
            "Error writing variable attributes.");
    }
}

//-----------------------------------------------------------------------------
// only for overlink
void
write_num_species_sets(DBfile *dbfile,
                       const int &num_specsets_written)
{
    const int data_length = 1;

    CONDUIT_CHECK_SILO_ERROR(
        DBWrite(dbfile, // dbfile
                "num_species_sets", // name
                const_cast<void *>(
                    static_cast<const void *>(
                        &num_specsets_written)), // address of single integer
                &data_length, // data length of one
                1, // dimension of the data
                DB_INT), // data is an integer
        "Error writing num_species_sets to Overlink.");
}

//-----------------------------------------------------------------------------
// only needed for nameschemes in the m domains to n files case
void
write_dom2filemap(DBfile *dbfile,
                  const Node &partition_map)
{
    index_t_accessor file   = partition_map["file"].value();
    index_t_accessor domain = partition_map["domain"].value();

    // load this data into a vector of pairs for sorting
    // we want to get the file for a domain i given by the index
    std::vector<std::pair<index_t, index_t>> file_dom_pairs;
    for (index_t i = 0; i < file.number_of_elements(); i ++)
    {
        // create pair objects in place
        file_dom_pairs.emplace_back(domain[i], file[i]);
    }

    // sort based on the domain ids
    std::sort(file_dom_pairs.begin(), 
              file_dom_pairs.end(), 
              [](std::pair<index_t, index_t> pair1,
                 std::pair<index_t, index_t> pair2)
              {
                  return pair1.first < pair2.first;
              });

    std::vector<int> sorted_file_ids;
    for (size_t i = 0; i < file_dom_pairs.size(); i ++)
    {
        sorted_file_ids.push_back(static_cast<int>(file_dom_pairs[i].second));
    }

    const int data_length = static_cast<int>(sorted_file_ids.size());

    CONDUIT_CHECK_SILO_ERROR(
        DBWrite(dbfile, // dbfile
                "dom2filemap", // name
                sorted_file_ids.data(), // address of single integer
                &data_length, // data length
                1, // dimension of the data
                DB_INT), // data is an integer
        "Error writing dom2filemap to Silo.");
}

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///
///      file_style: "default", "root_only", "multi_file", "overlink"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      silo_type: "default", "pdb", "hdf5", "unknown"
///            when the file we are writing to exists, "default" ==> "unknown"
///            else,                                   "default" ==> "hdf5"
///         note: these are additional silo_type options that we could add
///         support for in the future:
///           "hdf5_sec2", "hdf5_stdio", "hdf5_mpio", "hdf5_mpiposix", "taurus"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      root_file_ext: "default", "root", "silo"
///            "default"   ==> "root"
///            if overlink, this parameter is unused.
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      ovl_topo_name: (used if present, default ==> "")
///
///      nameschemes: "default", "yes", "no"
///            "default" ==> "no"
///
///      unified_types: "default", "yes", "no"
///            "default" ==> "yes"
///            prefer single mesh/var types versus writing an entire array
///            of types. "yes" will prefer this if possible, "no" will 
///            always write the entire array.
///
///      number_of_files:  {# of files}
///            when "multi_file" or "overlink":
///                 <= 0, use # of files == # of domains
///                  > 0, # of files == number_of_files
///
/// Note:
///  In the non-overlink case...
///   1) We have made the choice to output ALL topologies as multimeshes.
///   2) We prepend the provided mesh_name to each of these topo names. We do
///      this to avoid a name collision in the root only + single domain case.
///      We do this across all cases for the sake of consistency. We also use
///      the mesh_name as the name of the silo directory within each silo file
///      where data is stored.
///   3) ovl_topo_name is ignored if provided.
///  In the overlink case...
///   1) We have made the choice to output only ONE topology as a multimesh.
///   2) mesh_name is ignored if provided and changed to "MMESH"
///   3) ovl_topo_name is the name of the topo we are outputting. If it is not
///      provided, we choose the first topology in the blueprint.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const Node &mesh,
                                  const std::string &path,
                                  const Node &opts
                                  CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // The assumption here is that everything is multi domain

    std::string opts_file_style           = "default";
    std::string opts_silo_type            = "default";
    std::string opts_suffix               = "default";
    std::string opts_root_file_ext        = "default";
    std::string opts_out_mesh_name        = "mesh"; // used only for the non-overlink case
    std::string opts_ovl_topo_name        = ""; // used only for the overlink case
    bool        opts_nameschemes          = false;
    bool        opts_prefer_unified_types = true;
    int         opts_num_files            = -1;
    bool        opts_truncate             = false;
    int         silo_type                 = DB_HDF5;

    // check for + validate file_style option
    if(opts.has_child("file_style") && opts["file_style"].dtype().is_string())
    {
        opts_file_style = opts["file_style"].as_string();

        if(opts_file_style != "default" &&
           opts_file_style != "root_only" &&
           opts_file_style != "multi_file" &&
           opts_file_style != "overlink")
        {
            CONDUIT_ERROR("write_mesh invalid file_style option: \""
                          << opts_file_style << "\"\n"
                          " expected: \"default\", \"root_only\", "
                          "\"multi_file\", or \"overlink\"");
        }
    }

    // this is the earliest place we know for sure if we are writing overlink or not
    // this is set in stone.
    const bool write_overlink = opts_file_style == "overlink";

    // check for + validate suffix option
    if(opts.has_child("suffix") && opts["suffix"].dtype().is_string())
    {
        opts_suffix = opts["suffix"].as_string();

        if(opts_suffix != "default" &&
           opts_suffix != "cycle" &&
           opts_suffix != "none" )
        {
            CONDUIT_ERROR("write_mesh invalid suffix option: \""
                          << opts_suffix << "\"\n"
                          " expected: \"default\", \"cycle\", or \"none\"");
        }
    }

    // check for + validate root_file_ext option
    if(opts.has_child("root_file_ext") && opts["root_file_ext"].dtype().is_string())
    {
        opts_root_file_ext = opts["root_file_ext"].as_string();

        if(opts_root_file_ext != "default" &&
           opts_root_file_ext != "root" &&
           opts_root_file_ext != "silo" )
        {
            CONDUIT_ERROR("write_mesh invalid root_file_ext option: \""
                          << opts_root_file_ext << "\"\n"
                          " expected: \"default\", \"root\", or \"silo\"");
        }
    }

    // check for + validate mesh_name option
    if(opts.has_child("mesh_name") && opts["mesh_name"].dtype().is_string())
    {
        opts_out_mesh_name = opts["mesh_name"].as_string();
        CONDUIT_ASSERT(detail::check_alphanumeric(opts_out_mesh_name),
                       "Silo write_mesh mesh_name " << opts_out_mesh_name <<
                       " contains non-alphanumeric characters. Exiting.");
    }

    // we only care about this argument if we are using overlink
    if (write_overlink)
    {
        // check for + validate ovl_topo_name option
        if(opts.has_child("ovl_topo_name") && opts["ovl_topo_name"].dtype().is_string())
        {
            opts_ovl_topo_name = opts["ovl_topo_name"].as_string();
        }
    }


    // check for number_of_files, 0 or -1 implies #files => # domains
    if(opts.has_child("number_of_files") && opts["number_of_files"].dtype().is_integer())
    {
        opts_num_files = (int) opts["number_of_files"].to_int();
    }

    // check for truncate (overwrite)
    if(opts.has_child("truncate") && opts["truncate"].dtype().is_string())
    {
        const std::string truncate_string = opts["truncate"].as_string();
        if (truncate_string == "true")
        {
            opts_truncate = true;
        }
    }

    // check for + validate nameschemes option
    if (opts.has_child("nameschemes") && opts["nameschemes"].dtype().is_string())
    {
        const std::string nameschemes_string = opts["nameschemes"].as_string();

        if (nameschemes_string != "default" &&
            nameschemes_string != "yes" &&
            nameschemes_string != "no" )
        {
            CONDUIT_ERROR("write_mesh invalid nameschemes option: \""
                          << nameschemes_string << "\"\n"
                          " expected: \"default\", \"yes\", or \"no\"");
        }
        // We only do nameschemes if asked for. Default is "no"
        opts_nameschemes = nameschemes_string == "yes";
    }

    // check for + validate unified_types option
    if (opts.has_child("unified_types") && opts["unified_types"].dtype().is_string())
    {
        const std::string unified_types_string = opts["unified_types"].as_string();

        if (unified_types_string != "default" &&
            unified_types_string != "yes" &&
            unified_types_string != "no" )
        {
            CONDUIT_ERROR("write_mesh invalid unified_types option: \""
                          << unified_types_string << "\"\n"
                          " expected: \"default\", \"yes\", or \"no\"");
        }
        // We try to do unified_types unless explicitly turned off. Default is "yes"
        opts_prefer_unified_types = unified_types_string != "no";
    }

    // check for + validate silo_type option
    if (opts.has_child("silo_type") && opts["silo_type"].dtype().is_string())
    {
        opts_silo_type = opts["silo_type"].as_string();

        if(opts_silo_type != "default" &&
           opts_silo_type != "pdb" &&
           opts_silo_type != "hdf5" &&
           // opts_silo_type != "hdf5_sec2" &&
           // opts_silo_type != "hdf5_stdio" &&
           // opts_silo_type != "hdf5_mpio" &&
           // opts_silo_type != "hdf5_mpiposix" &&
           // opts_silo_type != "taurus" &&
           opts_silo_type != "unknown" )
        {
            CONDUIT_ERROR("write_mesh invalid suffix option: \""
                          << opts_silo_type << "\"\n"
                          " expected: \"default\", \"pdb\", \"hdf5\", or \"unknown\"");
        }
    }

    if (opts_silo_type == "default")
    {
        silo_type = DB_HDF5;
        // "default" logic will be handled later, once we know
        // what the `root_filename` is.
    }
    else if (opts_silo_type == "pdb")
    {
        silo_type = DB_PDB;
    }
    else if (opts_silo_type == "hdf5")
    {
        silo_type = DB_HDF5;
    }
    else if (opts_silo_type == "unknown")
    {
        silo_type = DB_UNKNOWN;
    }
    // TODO these are the additional silo_type options we could add support
    // for in the future.
    // else if (opts_silo_type == "hdf5_sec2")
    // {
    //     silo_type = DB_HDF5_SEC2;
    // }
    // else if (opts_silo_type == "hdf5_stdio")
    // {
    //     silo_type = DB_HDF5_STDIO;
    // }
    // else if (opts_silo_type == "hdf5_mpio")
    // {
    //     silo_type = DB_HDF5_MPIO;
    // }
    // else if (opts_silo_type == "hdf5_mpiposix")
    // {
    //     silo_type = DB_HDF5_MPIPOSIX;
    // }
    // else if (opts_silo_type == "taurus")
    // {
    //     silo_type = DB_TAURUS;
    // }

    if (opts_root_file_ext == "default")
    {
        opts_root_file_ext = "root";
    }

    // more will happen for this case later
    if (write_overlink)
    {
        opts_suffix = "none"; // force no suffix for overlink case
        opts_root_file_ext = "silo"; // force .silo file extension for root file
    }

    int num_files = opts_num_files;

    //-----------------------------------------------------------------------------
    // make sure some MPI task has data
    //-----------------------------------------------------------------------------
    Node multi_dom;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    if (! conduit::relay::mpi::io::blueprint::clean_mesh(mesh, multi_dom, mpi_comm))
#else
    if (! conduit::relay::io::blueprint::clean_mesh(mesh, multi_dom))
#endif
    {
        CONDUIT_INFO("Silo save: no valid data exists. Skipping save");
        return;
    }

    // -----------------------------------------------------------
    // get the cycle info
    // -----------------------------------------------------------
    index_t cycle;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    conduit::relay::mpi::io::blueprint::determine_cycle_and_resolve_suffix(
        multi_dom,
        path,
        cycle,
        opts_suffix,
        false, // we are not just generating the root file name
        mpi_comm);
#else
    conduit::relay::io::blueprint::determine_cycle_and_resolve_suffix(
        multi_dom,
        path,
        cycle,
        opts_suffix,
        false); // we are not just generating the root file name
#endif

    // -----------------------------------------------------------
    // par_rank and par_size
    // -----------------------------------------------------------
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    const int par_rank = relay::mpi::rank(mpi_comm);
    const int par_size = relay::mpi::size(mpi_comm);
#else
    const int par_rank = 0;
    const int par_size = 1;
#endif

    // -----------------------------------------------------------
    // find the local and global # of domains
    // -----------------------------------------------------------
    const int local_num_domains = conduit::blueprint::mesh::number_of_domains(multi_dom);
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    const int global_num_domains = conduit::blueprint::mpi::mesh::number_of_domains(multi_dom, mpi_comm);
#else
    const int global_num_domains = local_num_domains;
#endif

    if (global_num_domains == 0)
    {
        if (par_rank == 0)
        {
            CONDUIT_WARN("There is no data to save. Doing nothing.");
        }
        return;
    }

    std::string output_dir = "";

    // resolve file_style == default
    //
    // default implies multi_file if more than one domain
    if(opts_file_style == "default")
    {
        if( global_num_domains > 1)
        {
            opts_file_style = "multi_file";
        }
        else // other wise, use root only
        {
            opts_file_style = "root_only";
        }
    }

    // -----------------------------------------------------------
    // handle overlink-specific rules
    // -----------------------------------------------------------
    if (write_overlink)
    {
        // for overlink, things are different.
        // we are only going to write out one multimesh, with the name "MMESH"
        // the topo we will choose is the one named opts_ovl_topo_name, or, if that does
        // not exist, it is the first topo we can find.
        const Node &dom = multi_dom.child(0);

        if (dom.has_child("topologies"))
        {
            const Node &dom_topos = dom["topologies"];
            if (!dom_topos.has_child(opts_ovl_topo_name))
            {
                if (par_rank == 0)
                {
                    CONDUIT_INFO("Silo save: Overlink: topo name not provided or not found.");
                }

                if (dom_topos.number_of_children() > 0)
                {
                    opts_ovl_topo_name = dom_topos.children().next().name();
                    if (par_rank == 0)
                    {
                        CONDUIT_INFO("Silo save: Overlink: topo name defaulting to " + opts_ovl_topo_name);
                    }
                }
                else
                {
                    if (par_rank == 0)
                    {
                        CONDUIT_WARN("Silo save: Overlink: No topologies to save. Doing nothing.");
                    }
                    return;
                }
            }
            // else we are good, the provided mesh name is valid
        }
        else
        {
            if (par_rank == 0)
            {
                CONDUIT_WARN("Silo save: Overlink: No topologies to save. Doing nothing.");
            }
            return;
        }

        // hardcode this so that we use the correct name going forward
        opts_out_mesh_name = "MMESH";
    }

    // ----------------------------------------------------
    // if using multi_file or overlink, create output dir
    // ----------------------------------------------------
    if ("multi_file" == opts_file_style || write_overlink)
    {
        // setup the directory
        output_dir = path;

        // at this point for suffix, we should only see
        // cycle or none -- default has been resolved
        if (opts_suffix == "cycle")
        {
            output_dir += conduit_fmt::format(".cycle_{:06d}",cycle);
        }

        bool dir_ok = false;

        // let rank zero handle dir creation
        if (par_rank == 0)
        {
            // check if the dir exists
            dir_ok = utils::is_directory(output_dir);
            if (!dir_ok)
            {
                // if not try to let rank zero create it
                dir_ok = utils::create_directory(output_dir);
            }
        }

        // make sure everyone knows if dir creation was successful

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        Node n_local, n_reduced;
        // use an mpi sum to check if the dir exists
        n_local = dir_ok ? 1 : 0;

        relay::mpi::sum_all_reduce(n_local,
                                   n_reduced,
                                   mpi_comm);

        dir_ok = (n_reduced.as_int() == 1);
#endif

        CONDUIT_ASSERT(dir_ok, "Error: failed to create directory " << output_dir);
    }

    // ----------------------------------------------------
    // setup root file name
    // ----------------------------------------------------
    std::string root_filename;
    if (write_overlink)
    {
        root_filename = utils::join_file_path(output_dir, "OvlTop." + opts_root_file_ext);
    }
    else
    {
        root_filename = path;

        // at this point for suffix, we should only see
        // cycle or none -- default has been resolved
        if(opts_suffix == "cycle")
        {
            root_filename += conduit_fmt::format(".cycle_{:06d}",cycle);
        }

        root_filename += "." + opts_root_file_ext;
    }

    // ----------------------------------------------------
    // check silo type now that root file name is known
    // ----------------------------------------------------
    // if the file exists and we are not truncating
    if (utils::is_file(root_filename) && !opts_truncate)
    {
        // then silo type must be unknown
        if (silo_type != DB_UNKNOWN)
        {
            silo_type = DB_UNKNOWN;
            CONDUIT_INFO("Overriding silo type to DB_UNKNOWN because the "
                         "file already exists and truncation is disabled.");
        }
    }
    else // the file does not exist or we are truncating
    {
        // then silo type can be anything except unknown
        if (silo_type == DB_UNKNOWN)
        {
            // silo type can be anything except unknown
            silo_type = DB_HDF5;
            CONDUIT_INFO("Overriding chosen silo type (DB_UNKNOWN) to DB_HDF5 "
                         "because either the file does not exist or "
                         "truncation is enabled.");
        }
    }

    // zero or negative (default cases), use one file per domain
    if(num_files <= 0)
    {
        num_files = global_num_domains;
    }

    // if global domains > num_files, warn and use one file per domain
    if(global_num_domains < num_files)
    {
        CONDUIT_INFO("Requested more files than actual domains, "
                     "writing one file per domain");
        num_files = global_num_domains;
    }

    // generate the bp index - we need it before writing anything to silo
    Node local_bp_idx, bp_idx;
    if(local_num_domains > 0)
    {
        ::conduit::blueprint::mesh::generate_index(multi_dom,
                                                   opts_out_mesh_name,
                                                   global_num_domains,
                                                   local_bp_idx);
    }
    // handle mpi case.
    // this logic is from the mpi ver of mesh index gen
    // it is duplicated here b/c we dont want a circular dep
    // between conduit_blueprint_mpi and conduit_relay_io_mpi
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    // NOTE: due to save vs write cases, these updates should be
    // single mesh only
    Node gather_bp_idx;
    relay::mpi::all_gather_using_schema(local_bp_idx,
                                        gather_bp_idx,
                                        mpi_comm);

    // union all entries into final index that reps
    // all domains
    NodeConstIterator itr = gather_bp_idx.children();
    while(itr.has_next())
    {
        const Node &curr = itr.next();
        bp_idx[opts_out_mesh_name].update(curr);
    }
#else
    // NOTE: due to save vs write cases, these updates should be
    // single mesh only
    bp_idx[opts_out_mesh_name] = local_bp_idx;
#endif

    // I want the names of specsets that are associated with the first
    // matset associated with the chosen topology
    std::map<std::string, std::pair<std::string, std::string>> ovl_specset_names;
    // we need this to ensure that all species get assigned a unique name
    // for overlink, independent of the order they appear for a particular domain.
    // TODO test me, specset1 and specset2, specset1 appears first on one dom
    // and second on the other dom.
    if (write_overlink)
    {
        int ovl_mspecies_object_index = 0;
        auto specset_itr = bp_idx[opts_out_mesh_name]["specsets"].children();
        while (specset_itr.has_next())
        {
            // these continue cases mirror the error cases in silo_write_specset

            const Node &specset_idx = specset_itr.next();
            const std::string specset_name = specset_itr.name();

            if (! detail::check_alphanumeric(specset_name))
            {
                // non-alphanumeric name
                ovl_specset_names[specset_name] = std::make_pair(
                    "ERROR", "Specset name " + specset_name + " contains "
                    "non-alphanumeric characters. Skipping.");
                continue;
            }

            if (! specset_idx.has_child("matset"))
            {
                // no linked matset
                ovl_specset_names[specset_name] = std::make_pair(
                    "ERROR", "Specset name " + specset_name + " is "
                    "missing a linked matset. Skipping.");
                continue;
            }
            const std::string linked_matset_name = specset_idx["matset"].as_string();

            if (! bp_idx[opts_out_mesh_name].has_path("matsets/" + linked_matset_name + "/topology"))
            {
                // linked matset is invalid or missing topo
                ovl_specset_names[specset_name] = std::make_pair(
                    "ERROR", "Specset name " + specset_name + " has "
                    "an invalid linked matset. Skipping.");
                continue;
            }
            const std::string linked_topo_name = bp_idx[opts_out_mesh_name]["matsets"][linked_matset_name]["topology"].as_string();

            if (linked_topo_name != opts_ovl_topo_name)
            {
                // specset is for a different topo
                ovl_specset_names[specset_name] = std::make_pair(
                    "ERROR", "Specset name " + specset_name + " is "
                    "for a topology other than the selected topology to write. Skipping.");
                continue;
            }

            if (ovl_mspecies_object_index == 0)
            {
                ovl_specset_names[specset_name] = std::make_pair("SPECIES", "");
            }
            else
            {
                ovl_specset_names[specset_name] = std::make_pair(
                    "SPECIES" + std::to_string(ovl_mspecies_object_index), "");
            }
        }
    }

    // TODO for overlink: Number of species per material within a set must agree
    // across domains, and agree with that specified in the corresponding
    // DBPutMultimatspecies call.

    // new style bp index partition_map
    // NOTE: the part_map is inited during write process for N domains
    // to M files case.
    // Other cases are simpler and are created when root file is written
    Node output_partition_map;

    Node local_type_domain_info;
    // our local type info is going to look like this:
    // meshes:
    //   mesh1:
    //     domain_ids: [5, 53, 74, ...]
    //     types: [quadmesh, ucdmesh, quadmesh, ...]
    //   mesh2:
    //     domain_ids: [5, 53, 74, ...]
    //     types: [pointmesh, pointmesh, pointmesh, ...]
    //   ...
    // vars:
    //   var1:
    //     domain_ids: [5, 53, 74, ...]
    //     types: [quadvar, ucdvar, quadvar, ...]
    //     ovl_datatype: [1, 1, 0, -1, ...]
    //   var2:
    //     domain_ids: [5, 53, 74, ...]
    //     types: [pointvar, pointvar, pointvar, ...]
    //     ovl_datatype: [1, 0, -1, 1, ...]
    //     var_parent: var1
    //   ...
    // matsets:
    //   matset1:
    //     domain_ids: [5, 53, 74, ...]
    //   matset2:
    //     domain_ids: [5, 53, 74, ...]
    //   ...
    // specsets:
    //   specset1:
    //     domain_ids: [5, 53, 74, ...]
    //   specset2:
    //     domain_ids: [5, 53, 74, ...]
    //   ...
    // each array is local_num_domains long

    // at this point for file_style,
    // default has been resolved, we need to just handle:
    //   root_only, multi_file
    if (opts_file_style == "root_only")
    {
        // write out local domains, since all tasks will
        // write to single file in this case, we need baton.
        // the outer loop + par_rank == current_writer implements
        // the baton.

        Node local_root_file_created;
        Node global_root_file_created;
        local_root_file_created.set((int)0);
        global_root_file_created.set((int)0);

        for (int current_writer = 0; current_writer < par_size; current_writer ++)
        {
            if (par_rank == current_writer)
            {
                detail::SiloObjectWrapperCheckError<DBfile, decltype(&DBClose)> dbfile{
                    nullptr,
                    &DBClose,
                    "Error closing Silo file: " + root_filename};

                for (int i = 0; i < local_num_domains; ++i)
                {
                    // if truncate, first rank to touch the file needs
                    // to open at
                    if (!dbfile.getSiloObject()
                        && (global_root_file_created.as_int() == 0)
                        && opts_truncate)
                    {
                        dbfile.setSiloObject(DBCreate(root_filename.c_str(), DB_CLOBBER, DB_LOCAL, NULL, silo_type));
                        CONDUIT_ASSERT(dbfile.getSiloObject(),
                            "Error opening Silo file for writing: " << root_filename);
                        local_root_file_created.set((int)1);
                    }

                    if (!dbfile.getSiloObject())
                    {
                        if (utils::is_file(root_filename))
                        {
                            dbfile.setSiloObject(DBOpen(root_filename.c_str(), silo_type, DB_APPEND));
                        }
                        else
                        {
                            dbfile.setSiloObject(DBCreate(root_filename.c_str(), DB_CLOBBER, DB_LOCAL, NULL, silo_type));
                        }

                        CONDUIT_ASSERT(dbfile.getSiloObject(),
                            "Error opening Silo file for writing: " << root_filename);
                    }

                    const Node &dom = multi_dom.child(i);
                    // figure out the proper mesh path the file
                    std::string mesh_path;

                    uint64 domain = dom["state/domain_id"].to_uint64();
                    if (global_num_domains == 1)
                    {
                        // no domain prefix, write to mesh name
                        mesh_path = opts_out_mesh_name;
                    }
                    else
                    {
                        // multiple domains, we need to use a domain prefix
                        mesh_path = conduit_fmt::format("domain_{:06d}/{}",
                                                        domain,
                                                        opts_out_mesh_name);
                        // we cannot have overlink in the root_only case so no need to handle it
                    }
                    silo_mesh_write(dbfile.getSiloObject(),
                                    dom,
                                    mesh_path,
                                    opts_ovl_topo_name,
                                    local_num_domains,
                                    i, // local domain index
                                    domain, // global domain id
                                    write_overlink,
                                    ovl_specset_names,
                                    local_type_domain_info);
                }
            }

        // Reduce to sync up (like a barrier) and solve first writer need
        #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
            mpi::max_all_reduce(local_root_file_created,
                                global_root_file_created,
                                mpi_comm);
        #else
            global_root_file_created.set(local_root_file_created);
        #endif
        }
    }
    else if (global_num_domains == num_files)
    {
        // write out each domain
        // writes are independent, so no baton here
        for (int i = 0; i < local_num_domains; ++i)
        {
            const Node &dom = multi_dom.child(i);
            uint64 domain = dom["state/domain_id"].to_uint64();

            std::string output_file;
            if (write_overlink)
            {
                output_file = conduit::utils::join_file_path(output_dir,
                                                conduit_fmt::format("domain{:d}.silo",
                                                                    domain));
            }
            else
            {
                output_file = conduit::utils::join_file_path(output_dir,
                                                conduit_fmt::format("domain_{:06d}.silo",
                                                                    domain));
            }

            // properly support truncate vs non truncate

            detail::SiloObjectWrapperCheckError<DBfile, decltype(&DBClose)> dbfile{
                nullptr,
                &DBClose,
                "Error closing Silo file: " + output_file};

            if (opts_truncate || !utils::is_file(output_file))
            {
                dbfile.setSiloObject(DBCreate(output_file.c_str(), DB_CLOBBER, DB_LOCAL, NULL, silo_type));
            }
            else
            {
                dbfile.setSiloObject(DBOpen(output_file.c_str(), silo_type, DB_APPEND));
            }
            CONDUIT_ASSERT(dbfile.getSiloObject(),
                "Error opening Silo file for writing: " << output_file);

            std::string mesh_path = write_overlink ? "" : opts_out_mesh_name;

            // write to mesh name subpath
            silo_mesh_write(dbfile.getSiloObject(),
                            dom,
                            mesh_path,
                            opts_ovl_topo_name,
                            local_num_domains,
                            i, // local domain index
                            domain, // global domain id
                            write_overlink,
                            ovl_specset_names,
                            local_type_domain_info);
        }
    }
    else // more complex case, N domains to M files
    {
        //
        // recall: we have re-labeled domain ids from 0 - > N-1, however
        // some mpi tasks may have no data.
        //

        // books we keep:
        Node books;
        books["local_domain_to_file"].set(DataType::index_t(local_num_domains));
        books["local_domain_status"].set(DataType::index_t(local_num_domains));

        // batons
        books["local_file_batons"].set(DataType::index_t(num_files));
        books["global_file_batons"].set(DataType::index_t(num_files));

        // used to track first touch
        books["local_file_created"].set(DataType::index_t(num_files));
        books["global_file_created"].set(DataType::index_t(num_files));

        // size local # of domains
        index_t_array local_domain_to_file = books["local_domain_to_file"].value();
        index_t_array local_domain_status  = books["local_domain_status"].value();

        // size num total files
        /// batons
        index_t_array local_file_batons    = books["local_file_batons"].value();
        index_t_array global_file_batons   = books["global_file_batons"].value();
        /// file created flags
        index_t_array local_file_created    = books["local_file_created"].value();
        index_t_array global_file_created   = books["global_file_created"].value();


        Node d2f_map;
        blueprint::gen_domain_to_file_map(global_num_domains,
                                          num_files,
                                          books);

        // generate part map
        // use global_d2f is what we need for "file" part of part_map
        output_partition_map["file"] = books["global_domain_to_file"];
        output_partition_map["domain"].set(DataType::index_t(global_num_domains));
        index_t_array part_map_domain_vals = output_partition_map["domain"].value();
        for(index_t i=0; i < global_num_domains; i++)
        {
            part_map_domain_vals[i] = i;
        }

        index_t_accessor global_d2f = books["global_domain_to_file"].value();

        // init our local map and status array
        for(int d = 0; d < local_num_domains; ++d)
        {
            const Node &dom = multi_dom.child(d);
            uint64 domain = dom["state/domain_id"].to_uint64();
            // local domain index to file map
            local_domain_to_file[d] = global_d2f[domain];
            local_domain_status[d] = 1; // pending (1), vs done (0)
        }

        //
        // Round and round we go, will we deadlock I believe no :-)
        //
        // Here is how this works:
        //  At each round, if a rank has domains pending to write to a file,
        //  we put the rank id in the local file_batons vec.
        //  This vec is then mpi max'ed, and the highest rank
        //  that needs access to each file will write this round.
        //
        //  When a rank does not need to write to a file, we
        //  put -1 for this rank.
        //
        //  During each round, max of # files writers are participating
        //
        //  We are done when the mpi max of the batons is -1 for all files.
        //

        bool another_twirl = true;
        int twirls = 0;

        int local_all_is_good  = 1;
        int global_all_is_good = 1;

        books["local_all_is_good"].set_external(&local_all_is_good,1);
        books["global_all_is_good"].set_external(&global_all_is_good,1);

        std::string local_io_exception_msg = "";

        while(another_twirl)
        {
            // update baton requests
            for(int f = 0; f < num_files; ++f)
            {
                // reset file baton logic, look
                // to see if any local domains are
                // destined for this file
                local_file_batons[f] = -1;
                for(int d = 0; d < local_num_domains; ++d)
                {
                    // do we need to write this domain,
                    // and if so is it going to the file
                    // f
                    if(local_domain_status[d] == 1 &&
                       local_domain_to_file[d] == f)
                    {
                        local_file_batons[f] = par_rank;
                    }
                }
            }

            // mpi max file batons array
            #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                mpi::max_all_reduce(books["local_file_batons"],
                                    books["global_file_batons"],
                                    mpi_comm);
            #else
                global_file_batons.set(local_file_batons);
            #endif

            // mpi max file created array
            #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                mpi::max_all_reduce(books["local_file_created"],
                                    books["global_file_created"],
                                    mpi_comm);
            #else
                global_file_created.set(local_file_created);
            #endif


            // we now have valid batons (global_file_batons)
            for(int f = 0; f < num_files && local_all_is_good == 1 ; ++f)
            {
                // check if this rank has the global baton for this file
                if( global_file_batons[f] == par_rank )
                {
                    // check the domains this rank has pending
                    for(int d = 0; d < local_num_domains && local_all_is_good == 1; ++d)
                    {
                        if(local_domain_status[d] == 1 &&  // pending
                           local_domain_to_file[d] == f) // destined for this file
                        {
                            // now is the time to write!
                            // pattern is:
                            //  file_%06llu.{protocol}:/domain_%06llu/...
                            const Node &dom = multi_dom.child(d);
                            uint64 domain_id = dom["state/domain_id"].to_uint64();

                            // construct file name and path name
                            std::string file_name, curr_path;
                            if (write_overlink)
                            {
                                file_name = conduit_fmt::format("domfile{:d}.silo", f);
                                curr_path = conduit_fmt::format("domain{:d}", domain_id);
                            }
                            else
                            {
                                file_name = conduit_fmt::format("file_{:06d}.silo", f);
                                curr_path = conduit_fmt::format("domain_{:06d}/{}",
                                                                domain_id,
                                                                opts_out_mesh_name);
                            }

                            std::string output_file = conduit::utils::join_file_path(output_dir,
                                                                                     file_name);

                            try
                            {
                                detail::SiloObjectWrapperCheckError<DBfile, decltype(&DBClose)> dbfile{
                                    nullptr,
                                    &DBClose,
                                    "Error closing Silo file: " + output_file};
                                // if truncate == true check if this is the first time we are
                                // touching file, and use DBCREATE w/ DB_CLOBBER
                                Node open_opts;
                                if(opts_truncate && global_file_created[f] == 0)
                                {
                                    if(!dbfile.getSiloObject())
                                    {
                                        dbfile.setSiloObject(DBCreate(output_file.c_str(), DB_CLOBBER, DB_LOCAL, NULL, silo_type));
                                        CONDUIT_ASSERT(dbfile.getSiloObject(),
                                            "Error opening Silo file for writing: " << output_file);
                                    }
                                    local_file_created[f]  = 1;
                                    global_file_created[f] = 1;
                                }

                                if(!dbfile.getSiloObject())
                                {
                                    if (utils::is_file(output_file))
                                    {
                                        dbfile.setSiloObject(DBOpen(output_file.c_str(), silo_type, DB_APPEND));
                                    }
                                    else
                                    {
                                        dbfile.setSiloObject(DBCreate(output_file.c_str(), DB_CLOBBER, DB_LOCAL, NULL, silo_type));
                                    }
                                    CONDUIT_ASSERT(dbfile.getSiloObject(),
                                        "Error opening Silo file for writing: " << output_file);
                                }

                                // CONDUIT_INFO("rank " << par_rank << " output_file"
                                //              << output_file << " path " << path);

                                silo_mesh_write(dbfile.getSiloObject(),
                                                dom,
                                                curr_path,
                                                opts_ovl_topo_name,
                                                local_num_domains,
                                                d, // local domain index
                                                domain_id, // global domain id
                                                write_overlink,
                                                ovl_specset_names,
                                                local_type_domain_info);

                                // update status, we are done with this doman
                                local_domain_status[d] = 0;
                            }
                            catch(conduit::Error &e)
                            {
                                local_all_is_good = 0;
                                local_io_exception_msg = e.message();
                            }
                        }
                    }
                }
            }

            // if any I/O errors happened stop and have all
            // tasks bail out with an exception (to avoid hangs)
            #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                mpi::min_all_reduce(books["local_all_is_good"],
                                    books["global_all_is_good"],
                                    mpi_comm);
            #else
                global_all_is_good = local_all_is_good;
            #endif

            if(global_all_is_good == 0)
            {
                std::string emsg = "Failed to write mesh data on one more more ranks.";

                if(!local_io_exception_msg.empty())
                {
                     emsg += conduit_fmt::format("Exception details from rank {}: {}.",
                                                 par_rank, local_io_exception_msg);
                }
                CONDUIT_ERROR(emsg);
            }

            // // If you need to debug the baton algorithm,
            // // uncomment to examine the books:
            // if(par_rank == 0)
            // {
            //    std::cout << "[" << par_rank << "] "
            //              << " twirls: " << twirls
            //              << " details\n"
            //              << books.to_yaml();
            // }
            // void twirls warning for general case
            (void)twirls;

            // check if we have another round
            // stop when all batons are -1
            another_twirl = false;

            for(int f = 0; f < num_files && !another_twirl; ++f)
            {
                // if any entry is not -1, we still have more work to do
                if(global_file_batons[f] != -1)
                {
                    another_twirl = true;
                    twirls++;
                }
            }
        }
    }

    int root_file_writer = 0;
    if(local_num_domains == 0)
    {
        root_file_writer = -1;
    }
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    // Rank 0 could have an empty domain, so we have to check
    // to find someone with a data set to write out the root file.
    Node out;
    out = local_num_domains;
    Node rcv;

    mpi::all_gather_using_schema(out, rcv, mpi_comm);
    root_file_writer = -1;
    int* res_ptr = (int*)rcv.data_ptr();
    for(int i = 0; i < par_size; ++i)
    {
        if(res_ptr[i] != 0)
        {
            root_file_writer = i;
            break;
        }
    }

    MPI_Barrier(mpi_comm);
#endif

    if(root_file_writer == -1)
    {
        // this should not happen. global doms is already 0
        CONDUIT_WARN("Relay: there are no domains to write out");
    }

    Node global_type_domain_info;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    relay::mpi::gather_using_schema(local_type_domain_info,
                                    global_type_domain_info,
                                    root_file_writer,
                                    mpi_comm);
#else
    global_type_domain_info.append().set_external(local_type_domain_info);
#endif

    // root_file_writer will now write out the root file
    if (par_rank == root_file_writer)
    {
        // we will gather type info into one place and organize it
        // by the end we should have a root_type_domain_info that looks like this:
        // (one entry in each list for each domain)
        //
        // meshes:
        //   mesh1_types: [ucdmesh, ucdmesh, ...]
        //   mesh1_status: [1, 1, 1, ...]
        //   mesh2_types: [ucdmesh, pointmesh, ...]
        //   mesh2_status: [1, 1, 1, ...]
        //   mesh3_types: [quadmesh, quadmesh, ...]
        //   mesh3_status: [1, -1, 1, ...]
        //   ...
        // vars:
        //   var1_types: [ucdvar, ucdvar, ...]
        //   var1_status: [1, 1, -1, ...]
        //   var2_types: [ucdvar, pointvar, ...]
        //   var2_status: [1, 1, 1, ...]
        //   var3_types: [quadvar, quadvar, ...]
        //   var3_status: [1, 1, 1, ...]
        //   ...
        // matsets:
        //   matset1_status: [1, 1, -1, ...]
        //   matset2_status: [1, -1, 1, ...]
        //   ...
        // specsets:
        //   specset1_status: [1, 1, -1, ...]
        //   specset2_status: [1, -1, 1, ...]
        //   ...
        // ovl_var_datatypes:              (used for overlink only)
        //   var1: [1, 1, 0, -1, ...]
        //   var2: [1, 0, -1, 1, ...]
        // ovl_var_parents:                (used for overlink only)
        //   var1:
        //     var2:
        //   ...
        Node root_type_domain_info;

        auto type_domain_info_itr = global_type_domain_info.children();
        while (type_domain_info_itr.has_next())
        {
            // type info from a particular MPI rank
            const Node &type_domain_info_from_rank = type_domain_info_itr.next();

            //
            // workhorse lambda to put together the root_type_domain_info object
            //
            auto assemble_root_type_dom_info = [&](const std::string comp, const int32 default_value = -1)
            {
                // comp is "meshes", "vars", "matsets", or "specsets"
                Node &root_type_domain_info_comp = root_type_domain_info[comp];

                // do we have any information for this kind of object?
                if (type_domain_info_from_rank.has_child(comp))
                {
                    // if we do have this kind of object, we will read information
                    // from all of the objects of this kind
                    auto read_comp_itr = type_domain_info_from_rank[comp].children();
                    while (read_comp_itr.has_next())
                    {
                        // We are one level from the bottom of the tree now.
                        // This is information for a single mesh/var/matset/specset
                        // on some domain.
                        const Node &read_comp_type_domain_info = read_comp_itr.next();
                        // mesh/var/matset/specset name
                        const std::string read_comp_name = read_comp_itr.name();

                        // if this object has never been seen before on any domain
                        if (!root_type_domain_info_comp.has_child(read_comp_name + "_status"))
                        {
                            // For this object, we allocate an array that is num domains long.
                            // Into this array we will record the status of thee domain, i.e.
                            // whether this object exists for a particular domain.
                            root_type_domain_info_comp[read_comp_name + "_status"].set(DataType::index_t(global_num_domains));
                            // get a pointer to our new array
                            index_t_array root_comp_status = root_type_domain_info_comp[read_comp_name + "_status"].value();
                            // -1 is our special flag that indicates that a domain does not
                            // contain our object. We initialize our array to contain all
                            // -1 to start and then fill in domains that we have information for.
                            root_comp_status.fill(-1);

                            if (comp == "meshes" || comp == "vars")
                            {
                                // for meshes and vars we additionally save an array containing the types
                                root_type_domain_info_comp[read_comp_name + "_types"].set(DataType::int32(global_num_domains));
                                int32_array root_comp_types = root_type_domain_info_comp[read_comp_name + "_types"].value();
                                // we fill this with our default value, which is DB_QUADMESH for meshes and 
                                // DB_QUADVAR for vars.
                                root_comp_types.fill(default_value);
                            }

                            // for overlink, we need the overlink data type for each var
                            // to save out in the var attributes
                            if (comp == "vars" && write_overlink)
                            {
                                root_type_domain_info["ovl_var_datatypes"][read_comp_name].set(
                                    read_comp_type_domain_info["ovl_datatype"]);

                                // we also want to save the relationship between the var component
                                // and the original variable.
                                if (read_comp_type_domain_info.has_child("var_parent"))
                                {
                                    const std::string var_parent = read_comp_type_domain_info["var_parent"].as_string();
                                    root_type_domain_info["ovl_var_parents"][var_parent][read_comp_name];
                                }
                            }
                        }
                        // this is where we are writing the data to
                        index_t_array root_comp_status = root_type_domain_info_comp[read_comp_name + "_status"].value();

                        // the global domain ids array is of length local domain ids
                        // local domain ids index into it to read global domain ids out
                        index_t_accessor global_domain_ids = read_comp_type_domain_info["domain_ids"].value();

                        if (comp == "meshes" || comp == "vars")
                        {
                            int32_array root_comp_types = root_type_domain_info_comp[read_comp_name + "_types"].value();
                            index_t_accessor read_comp_types = read_comp_type_domain_info["types"].value();
                            for (index_t local_domain_id = 0;
                                 local_domain_id < global_domain_ids.number_of_elements();
                                 local_domain_id ++)
                            {
                                index_t global_domain_index = global_domain_ids[local_domain_id];
                                if (global_domain_index != -1)
                                {
                                    root_comp_types[global_domain_index] = read_comp_types[local_domain_id];
                                    root_comp_status[global_domain_index] = 1;
                                }
                            }
                        }
                        else
                        {
                            for (index_t local_domain_id = 0;
                                 local_domain_id < global_domain_ids.number_of_elements();
                                 local_domain_id ++)
                            {
                                index_t global_domain_index = global_domain_ids[local_domain_id];
                                if (global_domain_index != -1)
                                {
                                    root_comp_status[global_domain_index] = 1;
                                }
                            }
                        }
                    }
                }
            };

            assemble_root_type_dom_info("meshes", DB_QUADMESH);
            assemble_root_type_dom_info("vars", DB_QUADVAR);
            assemble_root_type_dom_info("matsets");
            assemble_root_type_dom_info("specsets");
        }

        std::string output_silo_path, global_block_namescheme, global_file_namescheme;
        const std::string namescheme_delimiter = "|";

        // single file case
        if (opts_file_style == "root_only")
        {
            // there is no case for overlink here because you can't do
            // the root only case when you are doing overlink.

            global_file_namescheme = "";
            if (1 == global_num_domains)
            {
                output_silo_path = opts_out_mesh_name + "/{}";

                // "|mesh/topo"
                global_block_namescheme = namescheme_delimiter
                                        + opts_out_mesh_name + "/{}";
            }
            else
            {
                output_silo_path = "domain_{:06d}/" + opts_out_mesh_name + "/{}";

                // "|domain_%06d/mesh/topo|n"
                global_block_namescheme = namescheme_delimiter
                                        + "domain_%06d/"
                                        + opts_out_mesh_name + "/{}"
                                        + namescheme_delimiter + "n";
            }

            // generate part map (we only need domain for this case)
            output_partition_map["domain"].set(DataType::index_t(global_num_domains));
            index_t_array part_map_domain_vals = output_partition_map["domain"].value();
            for (index_t i = 0; i < global_num_domains; i ++)
            {
                part_map_domain_vals[i] = i;
            }
        }
        else
        {
            std::string output_dir_base, output_dir_path;
            utils::rsplit_file_path(output_dir,
                                    output_dir_base,
                                    output_dir_path);

            // num domains == num files case
            if (global_num_domains == num_files)
            {
                // generate partition map
                output_partition_map["file"].set(DataType::index_t(global_num_domains));
                output_partition_map["domain"].set(DataType::index_t(global_num_domains));
                index_t_array part_map_file_vals   = output_partition_map["file"].value();
                index_t_array part_map_domain_vals = output_partition_map["domain"].value();

                for (index_t i = 0; i < global_num_domains; i ++)
                {
                    // file id == domain id
                    part_map_file_vals[i]   = i;
                    part_map_domain_vals[i] = i;
                }

                if (write_overlink)
                {
                    output_silo_path = utils::join_file_path(output_dir_base, "domain{:d}.silo:{}");

                    // "|my_overlink_dir/domain%d.silo|n"
                    global_file_namescheme = namescheme_delimiter 
                                           + utils::join_file_path(output_dir_base, 
                                                                   "domain%d.silo")
                                           + namescheme_delimiter + "n";
                    // "|topo"
                    global_block_namescheme = namescheme_delimiter + "{}";
                }
                else
                {
                    output_silo_path = utils::join_file_path(output_dir_base, "domain_{:06d}.silo") + ":"
                                     + opts_out_mesh_name + "/{}";

                    // "|my_silo_dir.cycle_000000/domain%06d.silo|n"
                    global_file_namescheme = namescheme_delimiter
                                           + utils::join_file_path(output_dir_base, 
                                                                   "domain_%06d.silo")
                                           + namescheme_delimiter + "n";
                    // "|mesh/topo"
                    global_block_namescheme = namescheme_delimiter
                                            + opts_out_mesh_name + "/{}";
                }
            }
            // m to n case
            else
            {
                // we generated the partition map earlier

                if (write_overlink)
                {
                    output_silo_path = utils::join_file_path(output_dir_base, "domfile{:d}.silo:domain{:d}/{}");

                    // "|my_overlink_dir/domfile%d.silo|#dom2filemap[n]"
                    global_file_namescheme = namescheme_delimiter 
                                           + utils::join_file_path(output_dir_base, 
                                                                   "domfile%d.silo")
                                           + namescheme_delimiter + "#dom2filemap[n]";
                    // "|domain%d/topo"
                    global_block_namescheme = namescheme_delimiter + "domain%d/{}"
                                            + namescheme_delimiter + "n";
                }
                else
                {
                    output_silo_path = utils::join_file_path(output_dir_base, "file_{:06d}.silo") + ":"
                                     + "domain_{:06d}/"
                                     + opts_out_mesh_name + "/{}";

                    // "|my_silo_dir.cycle_000000/file_%06d.silo|#dom2filemap[n]"
                    global_file_namescheme = namescheme_delimiter
                                           + utils::join_file_path(output_dir_base, 
                                                                   "file_%06d.silo")
                                           + namescheme_delimiter + "#dom2filemap[n]";
                    // "|domain_%06d/mesh/topo|n"
                    global_block_namescheme = namescheme_delimiter
                                            + "domain_%06d/"
                                            + opts_out_mesh_name + "/{}"
                                            + namescheme_delimiter + "n";
                }
            }
        }

        /////////////////////////////
        // mesh partition map
        /////////////////////////////
        // example of cases (for opts_out_mesh_name == "mesh"):
        // root only, single domain
        // silo_path: "mesh/{}"
        //
        // root only, multi domain
        // silo_path: "domain_{:06d}/mesh/{}"
        // partition_map:
        //   domain: [0, 1, 2, 3, 4 ]
        //
        // # domains == # files:
        // silo_path: "out/domain_{:06d}.silo:mesh/{}"
        // partition_map:
        //   file:  [ 0, 1, 2, 3, 4 ]
        //   domain: [ 0, 1, 2, 3, 4 ]
        //
        // N domains to M files:
        // silo_path: "out/file_{:06d}.silo:domain_{:06d}/mesh/{}"
        // partition_map:
        //   file:  [ 0, 0, 1, 2, 2 ]
        //   domain: [ 0, 1, 2, 3, 4 ]
        //
        // N domains to M files (non trivial domain order):
        // silo_path: "out/file_{:06d}.silo:domain_{:06d}/mesh/{}"
        // partition_map:
        //    file:  [ 0, 0, 1, 2, 2 ]
        //    domain: [ 4, 0, 3, 2, 1 ]

        // TODO I would argue that the final case where the domains are scrambled
        // is not possible no matter what you do. If that is the case, then a lot
        // of the `std::sort`ing that we do can be eliminated. Look back at how
        // the partition map is created to see why it is not possible to get that
        // case. I checked with Cyrus and we don't think it is possible for the
        // domain list to be scrambled nor do we think it is possible for the file
        // list to not be monotonic increasing. But he thinks we should think about
        // this further before making any changes.

        if (output_partition_map.number_of_children() > 0 )
        {
            bp_idx[opts_out_mesh_name]["state/partition_map"] = output_partition_map;
        }

        Node root;
        root["blueprint_index"].set(bp_idx);

        root["protocol/name"]    = "silo";
        root["protocol/version"] = CONDUIT_VERSION;

        root["number_of_files"]  = num_files;
        root["number_of_domains"]  = global_num_domains;

        root["silo_path"] = output_silo_path;
        root["file_style"] = opts_file_style;

        root["type_domain_info"].set_external(root_type_domain_info);

        root["global_file_namescheme"] = global_file_namescheme;
        root["global_block_namescheme"] = global_block_namescheme;

        detail::SiloObjectWrapperCheckError<DBfile, decltype(&DBClose)> dbfile{
            nullptr,
            &DBClose,
            "Error closing Silo file: " + root_filename};

        // if not root only, this is the first time we are writing
        // to the root file -- make sure to properly support truncate
        if(opts_file_style != "root_only" && opts_truncate)
        {
            if(!dbfile.getSiloObject())
            {
                dbfile.setSiloObject(DBCreate(root_filename.c_str(), DB_CLOBBER, DB_LOCAL, NULL, silo_type));
                CONDUIT_ASSERT(dbfile.getSiloObject(),
                    "Error opening Silo file for writing: " << root_filename);
            }
        }

        if(!dbfile.getSiloObject())
        {
            if (utils::is_file(root_filename))
            {
                dbfile.setSiloObject(DBOpen(root_filename.c_str(), silo_type, DB_APPEND));
            }
            else
            {
                dbfile.setSiloObject(DBCreate(root_filename.c_str(), DB_CLOBBER, DB_LOCAL, NULL, silo_type));
            }

            CONDUIT_ASSERT(dbfile.getSiloObject(),
                "Error opening Silo file for writing: " << root_filename);
        }

        write_multimeshes(dbfile.getSiloObject(),
                          opts_out_mesh_name,
                          opts_ovl_topo_name,
                          root,
                          write_overlink,
                          opts_prefer_unified_types,
                          opts_nameschemes);
        write_multivars(dbfile.getSiloObject(),
                        opts_out_mesh_name,
                        opts_ovl_topo_name,
                        root,
                        write_overlink,
                        opts_prefer_unified_types,
                        opts_nameschemes);
        write_multimats(dbfile.getSiloObject(),
                        opts_out_mesh_name,
                        opts_ovl_topo_name,
                        root,
                        write_overlink,
                        opts_nameschemes);

        // TODO for overlink: Specie sets: A domain may contain multiple specie
        // sets. All domains must contain the same number of specie sets. The
        // numbers of species per material in each set may be different for the
        // same material in different sets. The number of species per material
        // in each set must be the same for all domains.

        const int num_specsets_written =
            write_multimatspecs(dbfile.getSiloObject(),
                                opts_out_mesh_name,
                                opts_ovl_topo_name,
                                root,
                                write_overlink,
                                ovl_specset_names,
                                opts_nameschemes);

        if (write_overlink)
        {
            write_num_species_sets(dbfile.getSiloObject(),
                                   num_specsets_written);

            write_var_attributes(dbfile.getSiloObject(),
                                 opts_out_mesh_name,
                                 root);

            write_pad_dims(dbfile.getSiloObject(),
                           opts_out_mesh_name,
                           root);
        }

        // only if we are doing nameschemes and we are in the 
        // m domains to n files case
        if (opts_nameschemes && 
            opts_file_style != "root_only" &&
            global_num_domains != num_files)
        {
            write_dom2filemap(dbfile.getSiloObject(),
                              root["blueprint_index"][opts_out_mesh_name]["state"]["partition_map"]);
        }
    }

    // barrier at end of work to avoid file system race
    // (non root task could write the root file in write_mesh,
    // but root task is always the one to read the root file
    // in read_mesh.

    #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        MPI_Barrier(mpi_comm);
    #endif
}

//-----------------------------------------------------------------------------
// Write a blueprint mesh to silo
//-----------------------------------------------------------------------------
/// These methods assume `mesh` is a valid blueprint mesh.
///
/// Note: These methods use "write" semantics, they will append to existing
///       files.
///
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const Node &mesh,
                                  const std::string &path
                                  CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    write_mesh(mesh, path, opts, mpi_comm);
#else
    write_mesh(mesh, path, opts);
#endif
}

//-----------------------------------------------------------------------------
// Save a blueprint mesh to silo
//-----------------------------------------------------------------------------
/// These methods assume `mesh` is a valid blueprint mesh.
///
/// Note: These methods use "save" semantics, they will overwrite existing
///       files.
///
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const Node &mesh,
                                 const std::string &path
                                 CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    save_mesh(mesh, path, opts, mpi_comm);
#else
    save_mesh(mesh, path, opts);
#endif
}

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///
///      file_style: "default", "root_only", "multi_file", "overlink"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      silo_type: "default", "pdb", "hdf5", "unknown"
///            when the file we are writing to exists, "default" ==> "unknown"
///            else,                                   "default" ==> "hdf5"
///         note: these are additional silo_type options that we could add
///         support for in the future:
///           "hdf5_sec2", "hdf5_stdio", "hdf5_mpio", "hdf5_mpiposix", "taurus"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      root_file_ext: "default", "root", "silo"
///            "default"   ==> "root"
///            if overlink, this parameter is unused.
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      ovl_topo_name: (used if present, default ==> "")
///
///      nameschemes: "default", "yes", "no"
///            "default" ==> "no"
///
///      unified_types: "default", "yes", "no"
///            "default" ==> "yes"
///            prefer single mesh/var types versus writing an entire array
///            of types. "yes" will prefer this if possible, "no" will 
///            always write the entire array.
///
///      number_of_files:  {# of files}
///            when "multi_file" or "overlink":
///                 <= 0, use # of files == # of domains
///                  > 0, # of files == number_of_files
///
/// Note:
///  In the non-overlink case...
///   1) We have made the choice to output ALL topologies as multimeshes.
///   2) We prepend the provided mesh_name to each of these topo names. We do
///      this to avoid a name collision in the root only + single domain case.
///      We do this across all cases for the sake of consistency. We also use
///      the mesh_name as the name of the silo directory within each silo file
///      where data is stored.
///   3) ovl_topo_name is ignored if provided.
///  In the overlink case...
///   1) We have made the choice to output only ONE topology as a multimesh.
///   2) mesh_name is ignored if provided and changed to "MMESH"
///   3) ovl_topo_name is the name of the topo we are outputting. If it is not
///      provided, we choose the first topology in the blueprint.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const Node &mesh,
                                 const std::string &path,
                                 const Node &opts
                                 CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // we force overwrite to true, so we need a copy of the const opts passed.
    Node save_opts;
    save_opts.set(opts);
    save_opts["truncate"] = "true";

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    write_mesh(mesh, path, save_opts, mpi_comm);
#else
    write_mesh(mesh, path, save_opts);
#endif
}

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io::silo --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io --
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
