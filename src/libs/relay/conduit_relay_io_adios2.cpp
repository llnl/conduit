// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_adios2.cpp
///
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    #include "conduit_relay_mpi_io_adios2.hpp"

// Define argument macros that add a communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG0(ARG) ARG
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG) ,ARG

#else
    #include "conduit_relay_io_adios2.hpp"

// Define an argument macro that does not add the communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG0(ARG) 
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG) 

// for non-mpi adios2 we need to define _NOMPI
#define _NOMPI

#endif

//-----------------------------------------------------------------------------
// standard lib includes
//-----------------------------------------------------------------------------
#include <iostream>
#include <algorithm>
#include <map>
#include <cstdio>
#include <cstring>
#ifdef MAKE_SEPARATE_GROUPS
#include <sys/time.h>
#endif

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include <adios2.h>
#include <adios2_read.h>
#include <adios2_transform_methods.h>

//-----------------------------------------------------------------------------
// -- conduit includes -- 
//-----------------------------------------------------------------------------
#include "conduit_error.hpp"
#include "conduit_utils.hpp"

using std::cout;
using std::endl;

#define CURRENT_TIME_STEP -1

#warning "TODO: do we need this?"
// The DISPARATE_TREE_SUPPORT macro turns on disparate tree support where we
// prepend a hash value of the Conduit tree to the start of variables. If the
// node hashes are not all the same across MPI ranks then we switch into this
// behavior. We write a domain map variable ".dm." that stores the node hash
// for each domain and this lets us determine which variable to read for a 
// given domain in case trees are different across MPI processors. This lets
// us read back data onto the appropriate domain since ADIOS2 destroys 
// that information.
#define DISPARATE_TREE_SUPPORT

#define DOMAIN_MAP_PREFIX    ".dm."
#define DOMAIN_MAP_PREFIXLEN 4
#define NODEHASH_PREFIX_LEN 11
// "1222333444/"
#define NWRITERS_VAR ".nw."

#warning "TODO: do we need this?"
// The ENCODE_TYPE_IN_PATH macro causes the code to prepend a type code to
// the start of the variable name. We do this since variables declared on 
// non-rank 0 processes cannot successfully define attributes in ADIOS2
#define ENCODE_TYPE_IN_PATH

// This is the variable we store in the file if we're encoding types so we
// can check for this name and assume that we're encoding type names.
#define ENCODE_TYPE_VAR ".et."

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
// -- begin conduit::relay::<mpi>::io::internals --
//-----------------------------------------------------------------------------
namespace internals
{

static int rank = 0;
static int size = 1;

//#define DEBUG_PRINT_RANK(OSSEXPR) if (internals::rank == 0) { cout << OSSEXPR << endl; }
#define DEBUG_PRINT_RANK(OSSEXPR) 

//-----------------------------------------------------------------------------
void print_varinfo(std::ostream &os, ADIOS2_FILE *afile, ADIOS2_VARINFO *v)
{
    os << "vname=" << afile->var_namelist[v->varid] << endl << "{" << endl;
    os << "   type = " << v->type << endl;
    os << "   ndim = " << v->ndim << endl;
    os << "   dims = {";
    for (int q = 0; q < v->ndim; ++q)
        os << "," << v->dims[q];
    os << "}" << endl;
    os << "   nsteps = " << v->nsteps << endl;
    os << "   global = " << v->global << endl;
    os << "   nblocks = {";
    for (int q = 0; q < v->nsteps; ++q)
        os << "," << v->nblocks[q];
    os << "}" << endl;
    os << "   sum_nblocks = " << v->sum_nblocks << endl;
    os << "   nattrs = " << v->nattrs << endl;
    os << "   attr_ids = {";
    for (int q = 0; q < v->nattrs; ++q)
    {
        os << "," << v->attr_ids[q] << " (" << afile->attr_namelist[v->attr_ids[q]];
        if (strstr(afile->attr_namelist[v->attr_ids[q]], "conduit_type") != NULL)
        {
            std::string avalue;
            ADIOS2_DATATYPES atype;
            int asize = 0;
            void *aptr = NULL;
            adios2_get_attr_byid(afile, v->attr_ids[q], &atype, &asize, &aptr);
            if (atype == adios2_string)
            {
                avalue = std::string((const char *)aptr);
            }
            free(aptr);

            os << " = " << avalue;
        }
        os << ")";
    }
    os << "}" << endl;

    os << "   blockinfo = {" << endl;
    for (int bi = 0; bi < v->sum_nblocks; ++bi)
    {
        os << "       {" << endl;

        os << "           start={";
        for (int q = 0; q < v->ndim; ++q)
            os << "," << v->blockinfo[bi].start[q];
        os << "}" << endl;

        os << "           count={";
        for (int q = 0; q < v->ndim; ++q)
            os << "," << v->blockinfo[bi].count[q];
        os << "}" << endl;

        os << "           process_id=" << v->blockinfo[bi].process_id << endl;
        os << "           time_index=" << v->blockinfo[bi].time_index << endl;
        os << "       }," << endl;
    }
    os << "   }" << endl;

    os << "}" << endl;
}

//-----------------------------------------------------------------------------
std::string
read_method_to_string(ADIOS2_READ_METHOD m)
{
    std::string s("ADIOS2_READ_METHOD_BP");
    if (m == ADIOS2_READ_METHOD_BP)
        s = "ADIOS2_READ_METHOD_BP";
    else if (m == ADIOS2_READ_METHOD_BP_AGGREGATE)
        s = "ADIOS2_READ_METHOD_BP_AGGREGATE";
    else if (m == ADIOS2_READ_METHOD_DATASPACES)
        s = "ADIOS2_READ_METHOD_DATASPACES";
    else if (m == ADIOS2_READ_METHOD_DIMES)
        s = "ADIOS2_READ_METHOD_DIMES";
    else if (m == ADIOS2_READ_METHOD_FLEXPATH)
        s = "ADIOS2_READ_METHOD_FLEXPATH";
    else if (m == ADIOS2_READ_METHOD_ICEE)
        s = "ADIOS2_READ_METHOD_ICEE";
    return s;
}

//-----------------------------------------------------------------------------
ADIOS2_READ_METHOD
string_to_read_method(const std::string &s)
{
    ADIOS2_READ_METHOD m = ADIOS2_READ_METHOD_BP;
    if (s == "ADIOS2_READ_METHOD_BP")
        m = ADIOS2_READ_METHOD_BP;
    else if (s == "ADIOS2_READ_METHOD_BP_AGGREGATE")
        m = ADIOS2_READ_METHOD_BP_AGGREGATE;
    else if (s == "ADIOS2_READ_METHOD_DATASPACES")
        m = ADIOS2_READ_METHOD_DATASPACES;
    else if (s == "ADIOS2_READ_METHOD_DIMES")
        m = ADIOS2_READ_METHOD_DIMES;
    else if (s == "ADIOS2_READ_METHOD_FLEXPATH")
        m = ADIOS2_READ_METHOD_FLEXPATH;
    else if (s == "ADIOS2_READ_METHOD_ICEE")
        m = ADIOS2_READ_METHOD_ICEE;
    return m;
}

//-----------------------------------------------------------------------------
std::string
lock_mode_to_string(ADIOS2_LOCKMODE m)
{
    std::string s("ADIOS2_LOCKMODE_NONE");
    if (m == ADIOS2_LOCKMODE_NONE)
        s = "ADIOS2_LOCKMODE_NONE";
    else if (m == ADIOS2_LOCKMODE_CURRENT)
        s = "ADIOS2_LOCKMODE_CURRENT";
    else if (m == ADIOS2_LOCKMODE_ALL)
        s = "ADIOS2_LOCKMODE_ALL";
    return s;
}

//-----------------------------------------------------------------------------
ADIOS2_LOCKMODE
string_to_lock_mode(const std::string &s)
{
    ADIOS2_LOCKMODE m = ADIOS2_LOCKMODE_NONE;
    if (s == "ADIOS2_LOCKMODE_NONE")
        m = ADIOS2_LOCKMODE_NONE;
    else if (s == "ADIOS2_LOCKMODE_CURRENT")
        m = ADIOS2_LOCKMODE_CURRENT;
    else if (s == "ADIOS2_LOCKMODE_ALL")
        m = ADIOS2_LOCKMODE_ALL;
    return m;
}

//-----------------------------------------------------------------------------
std::string
statistics_flag_to_string(ADIOS2_STATISTICS_FLAG f)
{
    std::string s("adios2_stat_no");
    if (f == adios2_stat_default)
        s = "adios2_stat_default";
    else if (f == adios2_stat_no)
        s = "adios2_stat_no";
    else if (f == adios2_stat_minmax)
        s = "adios2_stat_minmax";
    else if (f == adios2_stat_full)
        s = "adios2_stat_full";
    return s;
}

//-----------------------------------------------------------------------------
ADIOS2_STATISTICS_FLAG
string_to_statistics_flag(const std::string &s)
{
    ADIOS2_STATISTICS_FLAG f = adios2_stat_no;
    if (s == "adios2_stat_default")
        f = adios2_stat_default;
    else if (s == "adios2_stat_no")
        f = adios2_stat_no;
    else if (s == "adios2_stat_minmax")
        f = adios2_stat_minmax;
    else if (s == "adios2_stat_full")
        f = adios2_stat_full;
    return f;
}

//-----------------------------------------------------------------------------
bool
streamIsFileBased(ADIOS2_READ_METHOD method)
{
    switch(method)
    {
    case ADIOS2_READ_METHOD_BP:
    case ADIOS2_READ_METHOD_BP_AGGREGATE:
        return true;
    case ADIOS2_READ_METHOD_DATASPACES:
    case ADIOS2_READ_METHOD_DIMES:
    case ADIOS2_READ_METHOD_FLEXPATH:
    case ADIOS2_READ_METHOD_ICEE:
        return false;
    }
    return false;
}

//-----------------------------------------------------------------------------
#ifdef MAKE_SEPARATE_GROUPS
static unsigned long
current_time_stamp()
{
    // Get the current time of day.
    timeval t;
    gettimeofday(&t, 0);
    // Convert to microseconds since epoch.
    return t.tv_sec * 1000000 + t.tv_usec;
}
#endif

//-----------------------------------------------------------------------------
static void
create_group_name(char name[32])
{
#ifdef MAKE_SEPARATE_GROUPS
    unsigned long t = current_time_stamp();
    sprintf(name, "conduit%020ld", t);
#else
    strcpy(name, "conduit");
#endif
}

//-----------------------------------------------------------------------------

static void
create_domain_map_name(char name[32])
{
#if 1
    strcpy(name, DOMAIN_MAP_PREFIX);
#else
    unsigned long t = current_time_stamp();
    sprintf(name, DOMAIN_MAP_PREFIX "%020ld", t);
#endif
}

//-----------------------------------------------------------------------------
static std::vector<std::string> available_transports()
{
    std::vector<std::string> v;
    ADIOS2_AVAILABLE_WRITE_METHODS *wm = adios_available_write_methods();
    if (wm)
    {
        for (int i=0; i<wm->nmethods; i++)
            v.push_back(wm->name[i]);
        adios_available_write_methods_free(wm); 
    }
    return v;
}

//-----------------------------------------------------------------------------
static std::vector<std::string> available_transforms()
{
    std::vector<std::string> v;
#if 1
    // Workaround
    v.push_back("identity");
    v.push_back("zfp");
#else
    // Crashing
    ADIOS2_AVAILABLE_TRANSFORM_METHODS *t = adios_available_transform_methods();
    if (t)
    {
        for (int i=0; i<t->ntransforms; i++)
            v.push_back(t->name[i]);
        adios_available_transform_methods_free(t); 
    }
#endif
    return v;
}

//-----------------------------------------------------------------------------
static bool
string_vector_contains(const std::vector<std::string> &sv, const std::string &s)
{
    return std::find(sv.begin(), sv.end(), s) != sv.end();
}

//-----------------------------------------------------------------------------
bool is_integer(const std::string &s, int &ivalue)
{
    return sscanf(s.c_str(), "%d", &ivalue) == 1;
}

//-----------------------------------------------------------------------------
bool is_positive_integer(const std::string &s, int &ivalue)
{
    bool ret = is_integer(s, ivalue);
    return ret && ivalue >= 0;
}

//-----------------------------------------------------------------------------
// NOTE: Move to conduit::utils?
void
splitpath(const std::string &path, std::string &filename,
    int &time_step, int &domain, std::vector<std::string> &subpaths, 
    bool prefer_time = false)
{
    std::vector<std::string> tok;
    conduit::utils::split_string(path, ':', tok);

    if (tok.empty())
        filename = path; // Would have had to be an empty string.
    else if (tok.size() == 1)
        filename = path;
    else if (tok.size() == 2)
    {
        filename = tok[0];
        int ivalue = 0;
        if (is_integer(tok[1], ivalue))
        {
            if (prefer_time)
            {
                // filename:timestep
                time_step = ivalue;
            }
            else
            {
                // filename:domain
                domain = ivalue;
            }
        }
        else
        {
            // filename:subpaths
            subpaths.push_back(tok[1]);
        }
    }
    else if (tok.size() >= 3)
    {
        filename = tok[0];
        int ivalue1 = 0, ivalue2 = 0;
        bool arg1 = is_integer(tok[1], ivalue1);
        bool arg2 = is_positive_integer(tok[2], ivalue2);
        if (arg1 && arg2)
        {
            // filename:timestep:domain
            time_step = ivalue1;
            domain    = ivalue2;
        }
        else if (arg1 && !arg2)
        {
            // filename:domain:subpaths
            domain = ivalue1;
            subpaths.push_back(tok[2]);
        }
        else if (!arg1 && arg2)
        {
            // filename:<non-int>:int
            // Assume these are just numeric subpaths for now.
            // We could test for tok[1] == "current" to denote time...
            subpaths.push_back(tok[1]);
            subpaths.push_back(tok[2]);
        }
        else // !arg1 && !arg2
        {
            // filename:subpath:subpath
            subpaths.push_back(tok[1]);
            subpaths.push_back(tok[2]);
        }

        // Save the remaining tokens as subpaths
        for (size_t i = 3; i < tok.size(); ++i)
            subpaths.push_back(tok[i]);
    }
}

//-----------------------------------------------------------------------------
// Private class used to hold options that control adios i/o params.
// 
// These values are read by about(), and are set by io::adios_set_options()
// 
// NOTE: the options are split into separate nodes in the Conduit about/set
//       so the options will be more obvious to the querying application. The
//       alternative is to make them all just a string and let the user have
//       to guess about what the options are.
//
//-----------------------------------------------------------------------------

class ADIOS2Options
{
public:
    long                  buffer_size;
    std::string           transport;
    ADIOS2_STATISTICS_FLAG statistics_flag;
    std::string           transform;
    std::string transport_options;
    std::string transform_options;
    int                   enable_nodehash;

    ADIOS2_READ_METHOD read_method;
    std::string       read_parameters;
    ADIOS2_LOCKMODE    read_lock_mode;
    int               read_verbose;
    float             read_timeout;
public:
    ADIOS2Options() :
        // Write options
        buffer_size(1024*1024), transport("POSIX"), 
        statistics_flag(adios_stat_no), transform(),
        transport_options(),
        transform_options(),
        enable_nodehash(1),

        // Read options
        read_method(ADIOS2_READ_METHOD_BP),
        read_parameters(),
        read_lock_mode(ADIOS2_LOCKMODE_ALL), //ADIOS2_LOCKMODE_CURRENT would be for streams.
        read_verbose(0),
        read_timeout(-1.0f) // block by default
    {
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        transport = "MPI";
#endif
    }

    //------------------------------------------------------------------------
    void set(const Node &opts)
    {
        // Write options
        if (opts.has_child("write"))
        {
            const Node &n = opts["write"];

            if (n.has_child("buffer_size"))
                buffer_size = n["buffer_size"].as_long();

            if (n.has_child("transport"))
            {
                std::string s(n["transport"].as_string());
                if (string_vector_contains(available_transports(), s))
                    transport = s;
            }

            if (n.has_child("statistics_flag"))
            {
                std::string s(n["statistics_flag"].as_string());
                statistics_flag = internals::string_to_statistics_flag(s);
            }

            if (n.has_child("transform"))
            {
                std::string s(n["transform"].as_string());
                if (s == "" || 
                   string_vector_contains(available_transforms(), s))
                {
                    transform = s;
                }
            }

            if (n.has_child("transport_options"))
                transport_options = n["transport_options"].as_string();

            if (n.has_child("transform_options"))
                transform_options = n["transform_options"].as_string();

            if (n.has_child("enable_nodehash"))
                read_verbose = n["enable_nodehash"].to_value();
        }

        // Read options
        if (opts.has_child("read"))
        {
            const Node &n = opts["read"];
            if (n.has_child("read_method"))
            {
                std::string s(n["read_method"].as_string());
                read_method = internals::string_to_read_method(s);
            }

            if (n.has_child("parameters"))
                read_parameters = n["parameters"].as_string();

            if (n.has_child("lock_mode"))
            {
                std::string s(n["lock_mode"].as_string());
                read_lock_mode = internals::string_to_lock_mode(s);
            }

            if (n.has_child("timeout"))
                read_timeout = n["timeout"].to_value();

            if (n.has_child("verbose"))
                read_verbose = n["verbose"].to_value();
        }
    }

    //------------------------------------------------------------------------
    void about(Node &opts)
    {
        opts.reset();

        // Write options.
        opts["write/buffer_size"] = buffer_size;
        opts["write/transport"] = transport;
        opts["write/transport_options"] = transport_options;
        opts["write/transform"] = transform;
        opts["write/transform_options"] = transform_options;
        opts["write/statistics_flag"] = 
            internals::statistics_flag_to_string(statistics_flag);
        opts["write/enable_nodehash"] = enable_nodehash;

        //
        // Add some read options.
        //
        if (read_method == ADIOS2_READ_METHOD_BP)
            opts["read/method"] = "ADIOS2_READ_METHOD_BP";
        else if (read_method == ADIOS2_READ_METHOD_BP_AGGREGATE)
            opts["read/method"] = "ADIOS2_READ_METHOD_BP_AGGREGATE";
        else if (read_method == ADIOS2_READ_METHOD_DATASPACES)
            opts["read/method"] = "ADIOS2_READ_METHOD_DATASPACES";
        else if (read_method == ADIOS2_READ_METHOD_DIMES)
            opts["read/method"] = "ADIOS2_READ_METHOD_DIMES";
        else if (read_method == ADIOS2_READ_METHOD_FLEXPATH)
            opts["read/method"] = "ADIOS2_READ_METHOD_FLEXPATH";

        opts["read/parameters"] = read_parameters;

        if (read_lock_mode == ADIOS2_LOCKMODE_NONE)
            opts["read/lock_mode"] = "";
        else if (read_lock_mode == ADIOS2_LOCKMODE_CURRENT)
            opts["read/lock_mode"] = "ADIOS2_LOCKMODE_CURRENT";
        else if (read_lock_mode == ADIOS2_LOCKMODE_ALL)
            opts["read/lock_mode"] = "ADIOS2_LOCKMODE_ALL";

        opts["read/verbose"] = read_verbose;
        opts["read/timeout"] = read_timeout;

#if 0
// NOTE: This is meant to provide an informative list of the available
//       transports. Something about it is not playing nicely with MACSio.
//       It's either the use of a list or maybe strings point to these
//       local string vectors...?

        // Add in the available transports and transforms.
        std::string sep(", ");
        std::vector<std::string> transports(available_transports()),
                                 transforms(available_transforms());
        std::string key_transport("information/available_transports");
        opts[key_transport].set(DataType::list());
        for (size_t i = 0; i < transports.size(); ++i)
            opts[key_transport].append().set(transports[i]);
        std::string key_transform("information/available_transforms");
        opts[key_transform].set(DataType::list());
        for (size_t i = 0; i < transforms.size(); ++i)
            opts[key_transform].append().set(transforms[i]);
#endif
    }
};

// default adios i/o settings
static ADIOS2Options *adios2State_options = NULL;

//-----------------------------------------------------------------------------
// @brief Clean up the options at exit.
static void cleanup_options(void)
{
    if (adios2State_options != NULL)
    {
        delete adios2State_options;
        adios2State_options = NULL;
    }
}

//-----------------------------------------------------------------------------
// @brief Access the ADIOS save options, creating them first if needed. 
//        We create them on the heap to make sure that the object does
//        not fail to initialize statically.
static ADIOS2Options *options()
{
    if (adios2State_options == NULL)
    {
        adios2State_options = new ADIOS2Options;
        atexit(cleanup_options);
    }
    return adios2State_options;
}

//-----------------------------------------------------------------------------
static bool adios_initialized = false;
#define MAX_READ_METHODS 15
static int read_methods_initialized[MAX_READ_METHODS];

static void initialize(
    CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)
    )
{
    //cout << "initialize: init'd=" << adios_initialized << endl;
    if (!adios_initialized)
    {
        // Mark that we have not initialized any read methods. Read methods
        // are not initialized until we use them.
        memset(read_methods_initialized, 0, sizeof(int)*MAX_READ_METHODS);

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        // Initialize ADIOS.
        adios_init_noxml(comm);
#else
        // Initialize ADIOS.
        adios_init_noxml(0);
#endif
        adios_initialized = true;
    }
}

//-----------------------------------------------------------------------------
static void mark_read_method_initialized(ADIOS2_READ_METHOD m)
{
    read_methods_initialized[static_cast<int>(m)] = 1;
}

//-----------------------------------------------------------------------------
static void finalize_read_methods()
{
    for (int i = 0; i < MAX_READ_METHODS; ++i)
    {
        if (read_methods_initialized[i])
        {
            // cout << "adios_read_finalize_method("
            //      << static_cast<ADIOS2_READ_METHOD>(i)<< ")" << endl;
            adios_read_finalize_method(static_cast<ADIOS2_READ_METHOD>(i));
            read_methods_initialized[i] = 0;
        }
    }
}

//-----------------------------------------------------------------------------
static void finalize(
    CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)
    )
{
    cleanup_options();
    finalize_read_methods();
    // cout << "adios_finalize()" << endl;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    int rank = 0;
    MPI_Comm_rank(comm, &rank);
    adios_finalize(rank);
#else
    adios_finalize(0);
#endif
}

//-----------------------------------------------------------------------------
static void iterate_conduit_node_internal(
    const conduit::Node &node,
    std::string::size_type offset,
    void (*func)(const Node &,
                 const std::string &,
                 void *,
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                 MPI_Comm
#else
                 int
#endif
                ),
    void *funcData,
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm comm
#else
    int comm
#endif
    )
{
    // NOTE: we have to build up the path of the node ourselves in case we
    //       were passed a node that has parents that we're not saving.
    if (node.number_of_children() == 0)
    {
        if (offset != std::string::npos)
            func(node, std::string(node.path().c_str() + offset), funcData, comm);
        else
            func(node, node.path(), funcData, comm);
    }
    else
    {
        for (conduit::index_t i = 0; i < node.number_of_children(); ++i)
        {
            iterate_conduit_node_internal(node.child(i), offset, func, funcData, comm);
        }
    }  
}

//-----------------------------------------------------------------------------
static void iterate_conduit_node(
    const conduit::Node &node,
    void (*func)(const Node &,
                 const std::string &,
                 void *,
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                 MPI_Comm
#else
                 int
#endif
                ),
    void *funcData,
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm comm
#else
    int comm
#endif
    )
{
    // We're saving everthing under the current node so we want to exclude
    // everything that has the current node's path + "/"
    std::string::size_type offset = 0;
    if (!node.path().empty())
        offset = node.path().size() + 1;
    iterate_conduit_node_internal(node, offset, func, funcData, comm);
}

//-----------------------------------------------------------------------------
struct adios_save_state
{
    adios_save_state() : fid(0), gid(0), gSize(0), adios_path()
    {
        memset(groupName, 0, 32 * sizeof(char));
    }

    int64_t  fid;
    int64_t  gid;
    uint64_t gSize;
    char        groupName[32];
    std::string adios_path;
};

//-----------------------------------------------------------------------------
static ADIOS2_DATATYPES conduit_dtype_to_adios_dtype(const Node &node,
    bool &isString)
{
    isString = false;
    ADIOS2_DATATYPES dtype = adios_unknown;
    if (!node.dtype().is_empty() &&
       !node.dtype().is_object() &&
       !node.dtype().is_list())
    {
        if (node.dtype().is_number())
        {
            if (node.dtype().is_int8())
                dtype = adios_byte;
            else if (node.dtype().is_int16())
                dtype = adios_short;
            else if (node.dtype().is_int32())
                dtype = adios_integer;
            else if (node.dtype().is_int64())
                dtype = adios_long;
            else if (node.dtype().is_uint8())
                dtype = adios_unsigned_byte;
            else if (node.dtype().is_uint16())
                dtype = adios_unsigned_short;
            else if (node.dtype().is_uint32())
                dtype = adios_unsigned_integer;
            else if (node.dtype().is_uint64())
                dtype = adios_unsigned_long;
            else if (node.dtype().is_float32())
                dtype = adios_real;
            else if (node.dtype().is_float64())
                dtype = adios_double;
//            else if (node.dtype().is_index_t()) // Conduit does not implement
//                dtype = adios_unsigned_long; // ???
            else if (node.dtype().is_char())
                dtype = adios_byte;
            else if (node.dtype().is_short())
                dtype = adios_short;
            else if (node.dtype().is_int())
                dtype = adios_integer;
            else if (node.dtype().is_long())
                dtype = adios_long;
            else if (node.dtype().is_unsigned_char())
                dtype = adios_unsigned_byte;
            else if (node.dtype().is_unsigned_short())
                dtype = adios_unsigned_short;
            else if (node.dtype().is_unsigned_int())
                dtype = adios_unsigned_integer;
            else if (node.dtype().is_unsigned_long())
                dtype = adios_unsigned_long;
            else if (node.dtype().is_float())
                dtype = adios_real;
            else if (node.dtype().is_double())
                dtype = adios_double;
        }
        else if (node.dtype().is_string() || 
                node.dtype().is_char8_str())
        {
            // NOTE: store as byte arrays since adios_string is really
            //       just for attributes and some scalars.
            dtype = adios_byte;
            isString = true;
        }
    }

    return dtype;
}

#ifdef ENCODE_TYPE_IN_PATH
//-----------------------------------------------------------------------------
static const char *
adios_dtype_to_short_type_name(ADIOS2_DATATYPES dtype)
{
    const char *stn;
    switch(dtype)
    {
    case adios_byte:             stn = "_b"; break;
    case adios_short:            stn = "_s"; break;
    case adios_integer:          stn = "_i"; break;
    case adios_long:             stn = "_l"; break;
    case adios_unsigned_byte:    stn = "ub"; break;
    case adios_unsigned_short:   stn = "us"; break;
    case adios_unsigned_integer: stn = "ui"; break;
    case adios_unsigned_long:    stn = "ul"; break;
    case adios_real:             stn = "_r"; break;
    case adios_double:           stn = "_d"; break;
    case adios_string:           stn = "st"; break;
    default:                     stn = "??"; break;
    }
    return stn;
}

//-----------------------------------------------------------------------------
static ADIOS2_DATATYPES
short_type_name_to_adios_dtype(const char *stn)
{
    ADIOS2_DATATYPES dtype = adios_unknown;
    if (strncmp(stn, "_b", 2) == 0)
        dtype = adios_byte;
    else if (strncmp(stn, "_s", 2) == 0)
        dtype = adios_short;
    else if (strncmp(stn, "_i", 2) == 0)
        dtype = adios_integer;
    else if (strncmp(stn, "_l", 2) == 0)
        dtype = adios_long;
    else if (strncmp(stn, "ub", 2) == 0)
        dtype = adios_unsigned_byte;
    else if (strncmp(stn, "us", 2) == 0)
        dtype = adios_unsigned_short;
    else if (strncmp(stn, "ui", 2) == 0)
        dtype = adios_unsigned_integer;
    else if (strncmp(stn, "ul", 2) == 0)
        dtype = adios_unsigned_long;
    else if (strncmp(stn, "_r", 2) == 0)
        dtype = adios_real;
    else if (strncmp(stn, "_d", 2) == 0)
        dtype = adios_double;
    else if (strncmp(stn, "st", 2) == 0)
        dtype = adios_string;
    return dtype;
}
#endif

//-----------------------------------------------------------------------------
static void define_variables(const Node &node, 
    const std::string &node_path,
    void *funcData,
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm 
#else
    int 
#endif
    )
{
    adios_save_state *state = (adios_save_state *)funcData;

    // Map Conduit types to ADIOS types.
    bool isString = false;
    ADIOS2_DATATYPES dtype = conduit_dtype_to_adios_dtype(node, isString);
    if (dtype == adios_unknown)
    {
        CONDUIT_ERROR("Unsupported Conduit to ADIOS type conversion.");
        return;
    }

    // Prepend a path
#ifdef ENCODE_TYPE_IN_PATH
    std::string atc(adios_dtype_to_short_type_name(
                    isString ? adios_string : dtype));
    std::string path(conduit::utils::join_path(atc, state->adios_path));
    std::string adios_var(conduit::utils::join_path(path, node_path));
#else
    std::string adios_var(conduit::utils::join_path(state->adios_path, node_path));
#endif

    // Dimensions
    char offset[20], global[20], local[20];
    memset(offset, 0, sizeof(char) * 20);
    memset(global, 0, sizeof(char) * 20);
    memset(local,  0, sizeof(char) * 20);
    if (node.dtype().number_of_elements() > 1)
    {
        sprintf(local, "%ld", static_cast<long>(node.dtype().number_of_elements()));
    }
    else
    {
        sprintf(offset, "%d", internals::rank);
        sprintf(global, "%d", internals::size);
        sprintf(local,  "%d", 1);   
    }
    DEBUG_PRINT_RANK("adios_define_var(gid, \"" << adios_var
                     << "\", \"\", " << adios_type_to_string(dtype)
                     << ", \"" << local << "\""
                     << ", \"" << global << "\""
                     << ", \"" << offset << "\")")

    int64_t vid = adios_define_var(state->gid, adios_var.c_str(), 
                      "", dtype,
                      local, global, offset);
    if (vid < 0)
    {
        CONDUIT_ERROR("ADIOS Error:" << adios_get_last_errmsg()); 
        return;
    }

    // If we wanted a data transform in the options, add that now.
    const int transform_threshold = 1;
    if (!options()->transform.empty() &&
       node.dtype().number_of_elements() > transform_threshold)
    {
        std::vector<std::string> transform, tmp;
        if (options()->transform == "zfp")
        {
            // Only allow zfp on floating point variables.
            if (dtype == adios_real || dtype == adios_double)
            {
                // The zfp transform complained about passing multiple options at once.
                conduit::utils::split_string(options()->transform_options, ',', tmp);
                for (size_t i = 0; i < tmp.size(); i++)
                    transform.push_back(options()->transform + ":" + tmp[i]);
            }
        }
        else
        {
            transform.push_back(options()->transform + ":" + 
                                options()->transform_options);
        }

        for (size_t i = 0; i < transform.size(); ++i)
        {
            DEBUG_PRINT_RANK("adios_set_transform(varid, "
                             << "\"" << transform[i] << "\")")
            if (adios_set_transform(vid, transform[i].c_str()) != 0)
            {
                CONDUIT_ERROR("ADIOS Error:" << adios_get_last_errmsg()); 
            }
        }
    }
#ifndef ENCODE_TYPE_IN_PATH
    // Store the name of the actual Conduit type as an attribute.
    DEBUG_PRINT_RANK("adios_define_attribute(gid, \"conduit_type\", \""
                     << adios_var << "\", adios_string, "
                     << "\"" << node.dtype().name() << "\", \""
                     << adios_var << "\")")
    if (adios_define_attribute(state->gid, "conduit_type", adios_var.c_str(),
        adios_string, node.dtype().name().c_str(), adios_var.c_str()) < 0)
    {
        CONDUIT_ERROR("ADIOS Error:" << adios_get_last_errmsg()); 
        return;
    }
#endif

    // Add the variable's var size to the total for the group.
    state->gSize += adios_expected_var_size(vid);
}

//-----------------------------------------------------------------------------
static void write_variables(const Node &node, 
    const std::string &node_path,
    void *funcData,
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm
#else
    int
#endif
    )
{
    adios_save_state *state = (adios_save_state *)funcData;

    // Map Conduit types to ADIOS types.
    bool isString = false;
    ADIOS2_DATATYPES dtype = conduit_dtype_to_adios_dtype(node, isString);
    if (dtype == adios_unknown)
    {
        CONDUIT_ERROR("Unsupported Conduit to ADIOS type conversion.");
        return;
    }

    // Prepend a path.
#ifdef ENCODE_TYPE_IN_PATH
    std::string atc(adios_dtype_to_short_type_name(
                    isString ? adios_string : dtype));
    std::string path(conduit::utils::join_path(atc, state->adios_path));
    std::string adios_var(conduit::utils::join_path(path, node_path));
#else
    std::string adios_var(conduit::utils::join_path(state->adios_path, node_path));
#endif

    // if the node is compact, we can write directly from its data ptr
    DEBUG_PRINT_RANK("adios_write(file, \"" << adios_var
                     << "\", node.data_ptr())")
    int s;
    if (node.dtype().is_compact()) 
    {
        s = adios_write(state->fid, adios_var.c_str(), node.data_ptr());
    }
    else
    {
        // otherwise, we need to compact our data first
        Node n;
        node.compact_to(n);
        s = adios_write(state->fid, adios_var.c_str(), n.data_ptr());
    }

    if (s != 0)
    {
        CONDUIT_ERROR("ADIOS Error: " << adios_get_last_errmsg());
    }
}

//-----------------------------------------------------------------------------
static bool declare_group(int64_t *gid, 
    const std::string &groupName, const std::string &timeIndex)
{
    DEBUG_PRINT_RANK("adios_declare_group(&gid, \""
        << groupName << "\", \"" << timeIndex << "\", "
        << internals::statistics_flag_to_string(options()->statistics_flag)
        << ")")

    int retval = adios_declare_group(gid, groupName.c_str(), timeIndex.c_str(), 
        options()->statistics_flag);
    if (retval != 0)
    {
        CONDUIT_ERROR("ADIOS error: " << adios_get_last_errmsg());
        return false;
    }

    const char *base_path = ""; // blank for current directory.
    DEBUG_PRINT_RANK("adios_select_method(gid, \"" 
        << options()->transport << "\", "
        << "\"" << options()->transport_options << "\", "
        << "\"" << base_path << "\")")
    retval = adios_select_method(*gid,
                 options()->transport.c_str(),
                 options()->transport_options.c_str(),
                 base_path);
    if (retval != 0)
    {
        CONDUIT_ERROR("ADIOS error: " << adios_get_last_errmsg());
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
static void hash_tree(const Node &, 
    const std::string &node_path,
    void *funcData,
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm
#else
    int
#endif
    )
{
    unsigned int *h = (unsigned int *)funcData;
    *h = conduit::utils::hash(node_path, *h);
}

//-----------------------------------------------------------------------------
static void compute_nodehash(const Node &node, unsigned int &nodehash, 
    int &rank_for_nodehash, int &size_for_nodehash
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
    )
{
    // Iterate over all of the nodes in the tree and make a hash of
    // all of the node paths in there.
    nodehash = 0;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm_rank(comm, &internals::rank);
    MPI_Comm_size(comm, &internals::size);

    if (options()->enable_nodehash)
    {
        iterate_conduit_node(node, hash_tree, &nodehash, comm);
        unsigned int reduced_nodehash = 0;
        MPI_Allreduce(&nodehash, &reduced_nodehash, 1, 
                      MPI_UNSIGNED, MPI_BAND, comm);
        if (nodehash != reduced_nodehash)
        {
            // We have different node hashes across ranks. So, we need to determine
            // the rank of this process in the group of ranks that has this 
            // nodehash value.
            unsigned int *allnodehashes = new unsigned int[internals::size];
            MPI_Allgather(&nodehash, 1, MPI_UNSIGNED,
                          allnodehashes, 1, MPI_UNSIGNED,
                          comm);
            // Count the ranks that have this nodehash
            size_for_nodehash = 0;
            for (int i = 0; i < internals::size; ++i)
            {
                if (allnodehashes[i] == nodehash)
                    ++size_for_nodehash;
            }
            // Count the number of occurances of nodehash before this rank to
            // get a rank.
            rank_for_nodehash = 0;
            for (int i = 0; i < internals::rank; ++i)
            {
                if (allnodehashes[i] == nodehash)
                    ++rank_for_nodehash;
            }
            delete [] allnodehashes;
        }
        else
        {
            size_for_nodehash = internals::size;
            rank_for_nodehash = internals::rank;
        }
    }
    else
    {
        size_for_nodehash = internals::size;
        rank_for_nodehash = internals::rank;
    }
#else
    if (options()->enable_nodehash)
        iterate_conduit_node(node, hash_tree, &nodehash, 0);
    rank_for_nodehash = 0;
    size_for_nodehash = 1;
#endif
}

//-----------------------------------------------------------------------------
static void
save(const Node &node, const std::string &path, const char *flag,
    unsigned int nodehash, int nodehash_rank, int nodehash_size
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
    )
{
    adios_save_state state;
    std::string file_path(path);

    // check for ":" split  
    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    state.adios_path);

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm_rank(comm, &internals::rank);
    MPI_Comm_size(comm, &internals::size);

    // Bcast the filename so we know it will be the same on all ranks.
    int len = path.size()+1;
    MPI_Bcast(&len, 1, MPI_INT, 0, comm);
    char *sbuf = new char[len];
    if (internals::rank == 0)
        strcpy(sbuf, path.c_str());
    MPI_Bcast(sbuf, len, MPI_CHAR, 0, comm);
    if (rank > 0)
        file_path = std::string(sbuf);
    delete [] sbuf;

    // Initialize ADIOS.
    initialize(comm);
#else
    int comm = 0;

    // Initialize ADIOS.
    initialize();
#endif

    // Set ADIOS's max buffer sized based on the options.
    DEBUG_PRINT_RANK("adios_set_max_buffer_size("
                     << options()->buffer_size << ")")
    adios_set_max_buffer_size(static_cast<uint64_t>(options()->buffer_size));

    //
    // Group
    //
    create_group_name(state.groupName);
#if defined(CONDUIT_RELAY_IO_MPI_ENABLED) && defined(MAKE_SEPARATE_GROUPS)
    MPI_Bcast(state.groupName, 32, MPI_CHAR, 0, comm);
#endif
    std::string timeIndex;
    if (declare_group(&state.gid, state.groupName, timeIndex))
    {
#ifdef DISPARATE_TREE_SUPPORT
        //
        // Define nodehash variable (if enabled and relevant)
        //
        char domain_map_name[32];
        unsigned int dm[2] = {0,0};
        bool write_domain_map = options()->enable_nodehash &&
                                nodehash_size != internals::size; // >1 hashes
        if (write_domain_map)
        {
            create_domain_map_name(domain_map_name);
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
            MPI_Bcast(domain_map_name, 32, MPI_CHAR, 0, comm);
#endif
            // Define a domainmap variable.
            dm[0] = nodehash;
            dm[1] = nodehash_rank;
            DEBUG_PRINT_RANK("adios_define_var(gid, \"" << domain_map_name 
                << "\", \"\", adios_unsigned_integer, \"2\", \"\", \"\")")
            adios_define_var(state.gid, domain_map_name,
                             "", adios_unsigned_integer,
                             "2", ""/*global*/, ""/*offset*/);
            state.gSize += 2 * sizeof(unsigned int);

            // Save the number of writers.
            adios_define_var(state.gid, NWRITERS_VAR,
                         "", adios_integer,
                         "", "", "");
            state.gSize += sizeof(int);

            // We'll prepend the nodehash onto the rest of the variables
            // that we declare.
            char nhprefix[12];
            sprintf(nhprefix, "%010u", nodehash);
            state.adios_path = conduit::utils::join_path(std::string(nhprefix), state.adios_path);
        }
#endif
#ifdef ENCODE_TYPE_IN_PATH
        // Define a scalar that indicates we're encoding types in the path.
        adios_define_var(state.gid, ENCODE_TYPE_VAR,
                         "", adios_integer,
                         "", "", "");
        state.gSize += sizeof(int);
#endif

        //
        // Define variables.
        //
        iterate_conduit_node(node, define_variables, &state, comm);

        //
        // Open the file
        //
        DEBUG_PRINT_RANK("adios_open(&fid, \"" << state.groupName 
            << "\", \"" << path << "\", "
            << "\"" << flag << "\", comm)")
        if (adios_open(&state.fid, state.groupName, file_path.c_str(), 
                      flag, comm) == 0)
        {
            // This is an optional call that lets ADIOS size its output buffers.
            uint64_t total_size = 0;
            DEBUG_PRINT_RANK("adios_group_size(fid, " << state.gSize << ", &total)")
            if (adios_group_size(state.fid, state.gSize, &total_size) == 0)
            {
#ifdef DISPARATE_TREE_SUPPORT
                if (write_domain_map)
                {
                    // Write the domainmap variable.
                    DEBUG_PRINT_RANK("adios_write(fid, \"" << domain_map_name << "\", dm)")
                    adios_write(state.fid, domain_map_name, dm);

                    // Write the number of writers.
                    DEBUG_PRINT_RANK("adios_write(fid, \"" << NWRITERS_VAR << "\", size)")
                    adios_write(state.fid, NWRITERS_VAR, &internals::size);
                }
#endif
#ifdef ENCODE_TYPE_IN_PATH
                int etflag = 1;
                DEBUG_PRINT_RANK("adios_write(fid, \"" << ENCODE_TYPE_VAR << "\", etflag)")
                adios_write(state.fid, ENCODE_TYPE_VAR, &etflag);
#endif
                //
                // Write Variables
                //
                iterate_conduit_node(node, write_variables, &state, comm);
            }
            else
            {
                CONDUIT_ERROR("ADIOS error: " << adios_get_last_errmsg());
            }

            //
            // Close the file.
            //
            DEBUG_PRINT_RANK("adios_close(fid)")
            if (adios_close(state.fid) != 0)
            {
//                CONDUIT_ERROR("ADIOS error: " << adios_get_last_errmsg());
            }
        }
        else
        {
            CONDUIT_ERROR("ADIOS error: " << adios_get_last_errmsg());
        }
    }
    else
    {
        CONDUIT_ERROR("ADIOS Error: failed to create group.");
    }

    // Delete the variable definitions from the group so we can define them
    // again the next time around. Free the group too.
    DEBUG_PRINT_RANK("adios_delete_vardefs(gid)")
    adios_delete_vardefs(state.gid);
    DEBUG_PRINT_RANK("adios_free_group(gid)")
    adios_free_group(state.gid);
}

//-----------------------------------------------------------------------------
bool name_matches_subpaths(const std::string &name, 
    const std::vector<std::string> &subpaths)
{
    bool match = false;
    if (subpaths.empty())
        match = true;
    else
    {
        for (size_t i = 0; i < subpaths.size(); ++i)
        {
            size_t lsubpath = subpaths[i].size();
            if (strncmp(name.c_str(), subpaths[i].c_str(), lsubpath) == 0)
            {
                // We have a match so far.
                size_t lname = name.size();
                if (lname == lsubpath)
                {
                    // name:    a/b/c
                    // subpath: a/b/c
                    match = true;
                    break;
                }
                else if (lname < lsubpath)
                {
                    // name:    a/b/c
                    // subpath: a/b/cat
                    match = false;
                }
                else if (lname > lsubpath)
                {
                    // name:   a/b/cat, a/b/c/d
                    // subpath a/b/c
                    if (name[lsubpath] == '/')
                    {
                        match = true;
                        break;
                    }
                }
            }
        }
    }
    return match;
}

//-----------------------------------------------------------------------------
void read_conduit_type_attribute(ADIOS2_FILE *afile,
    ADIOS2_VARINFO *v, std::string &conduit_type)
{
    for (int q = 0; q < v->nattrs; ++q)
    {
        if (strstr(afile->attr_namelist[v->attr_ids[q]], "conduit_type") != NULL)
        {
            ADIOS2_DATATYPES atype;
            int asize = 0;
            void *aptr = NULL;
            adios_get_attr_byid(afile, v->attr_ids[q], &atype, &asize, &aptr);
            if (atype == adios_string)
                conduit_type = std::string((const char *)aptr);
            free(aptr);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
struct adios_load_state
{
   adios_load_state() : filename(), time_step(CURRENT_TIME_STEP), domain(0),
       subpaths()
   {
   }

   std::string              filename;
   int                      time_step;
   int                      domain; 
   std::vector<std::string> subpaths;
};

//-----------------------------------------------------------------------------
void
open_file_and_process(adios_load_state *state,
    void (*cb)(adios_load_state *, ADIOS2_FILE *, void *),
    void *cbdata
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
    )
{
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    MPI_Comm_rank(comm, &internals::rank);
    MPI_Comm_size(comm, &internals::size);
#else
    int comm = 0;
#endif

    // Initialize the read method.
    DEBUG_PRINT_RANK("adios_read_init_method(" 
       << internals::read_method_to_string(internals::options()->read_method)
       << ", comm, \""
       << internals::options()->read_parameters
       << "\")");
    adios_read_init_method(options()->read_method, 
                           comm,
                           options()->read_parameters.c_str()
                           );
    // Mark that we've initialized the read method.
    mark_read_method_initialized(options()->read_method);

    // Open the file for read.
    ADIOS2_FILE *afile = NULL;

    if (streamIsFileBased(options()->read_method))
    {
        // NOTE: This read function seems to have much better behavior in
        //       providing access to time steps.
        afile = adios_read_open_file(state->filename.c_str(), 
                    options()->read_method,
                    comm);
    }
    else
    {
        // NOTE: use adios_read_open to open the stream.
        DEBUG_PRINT_RANK("adios_read_open(\"" << state->filename << "\","
            << internals::read_method_to_string(options()->read_method) << ", "
            << "comm, "
            << internals::lock_mode_to_string(options()->read_lock_mode) << ", "
            << options()->read_timeout << ")")

        afile = adios_read_open(state->filename.c_str(), 
                    options()->read_method,
                    comm,
                    options()->read_lock_mode,
                    options()->read_timeout);
    }

    if (afile != NULL)
    {
        // Now that the file is open, do something with it.
        (*cb)(state, afile, cbdata);

        // Close the file.
        DEBUG_PRINT_RANK("adios_read_close(afile)")
        adios_read_close(afile);
    }
    else
    {
        CONDUIT_ERROR("ADIOS Error: " << adios_get_last_errmsg());
    }
}

//-----------------------------------------------------------------------------
#ifdef ENCODE_TYPE_IN_PATH
bool encode_type_found(ADIOS2_FILE *afile)
{
    for (int i = 0; i < afile->nvars; ++i)
    {
        if (strcmp(afile->var_namelist[i], ENCODE_TYPE_VAR) == 0)
        {
            return true;
        }
    }
    return false;
}
#endif

#ifdef DISPARATE_TREE_SUPPORT
//-----------------------------------------------------------------------------
bool domain_map_found(ADIOS2_FILE *afile)
{
    for (int i = 0; i < afile->nvars; ++i)
    {
        if (strncmp(afile->var_namelist[i], 
                   DOMAIN_MAP_PREFIX, DOMAIN_MAP_PREFIXLEN) == 0)
        {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool nwriters_found(ADIOS2_FILE *afile)
{
    for (int i = 0; i < afile->nvars; ++i)
    {
        if (strcmp(afile->var_namelist[i], NWRITERS_VAR) == 0)
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
static void
load_domain_map(ADIOS2_FILE *afile, int ts, int domain,
    std::vector<std::string>  &dmprefixes,
    std::vector<unsigned int> &dmblocks
    )
{
    std::map<unsigned int, unsigned int> dmap;

    // Determine the number of writers in the file.
    int nwriters_in_file = 1; // initial guess
    if (nwriters_found(afile))
    {
        ADIOS2_VARINFO *v = adios_inq_var(afile, NWRITERS_VAR);
        if (v)
        {
            nwriters_in_file = *((int *)v->value);
            adios_free_varinfo(v);
        }
    }

    if (domain_map_found(afile))
    {
        // Get the size of .dm. We will assume that if there are more than size
        // pieces that we have multiple parts that we want to combine if they
        // have different node hashes.
        ADIOS2_VARINFO *v = adios_inq_var(afile, DOMAIN_MAP_PREFIX);
        if (v)
        {
            adios_inq_var_blockinfo(afile,v);

            // Check time step validity.
            if (ts == CURRENT_TIME_STEP)
                ts = afile->current_step;

            // ASSUMPTION: We assume that each save_merged() was the same nprocs.
            int nblocks = v->nblocks[ts];
            int varid = v->varid;
            for (int d = domain; d < nblocks; d += nwriters_in_file)
            {
                ADIOS2_SELECTION *sel = adios_selection_writeblock(d);

                unsigned int dm[2] = {0,0};
                adios_schedule_read_byid(afile, sel, varid, ts, 1, dm);
                int blocking = 1;
                adios_perform_reads(afile, blocking);

                // Ensure that we're only putting different node hashes/blocks into dmap.
                if (dmap.find(dm[0]) == dmap.end())
                {
                    dmap[dm[0]] = dm[1];
                }
            }

            adios_free_varinfo(v);
        }

        // Check if there are any duplicate node hashes.
        for (std::map<unsigned int, unsigned int>::const_iterator it = dmap.begin();
            it != dmap.end(); ++it)
        {
            char prefix[32];
            sprintf(prefix, "%010u", it->first);
            dmprefixes.push_back(prefix);
            dmblocks.push_back(it->second);
        }
    }
}
#endif

//-----------------------------------------------------------------------------
// Callback for load()
static void
load_node(adios_load_state *state, ADIOS2_FILE *afile, void *cbdata)
{
    // Convert cbdata back to Node * and get a reference.
    Node *node_ptr = (Node *)cbdata;
    Node &node = *node_ptr;

    int scheduled_reads = 0;
    std::vector<ADIOS2_SELECTION *> sels;

#ifdef DISPARATE_TREE_SUPPORT
    std::vector<std::string>  dmprefixes;
    std::vector<unsigned int> dmblocks;
    load_domain_map(afile, state->time_step, state->domain, dmprefixes, dmblocks);
#endif
    int skip = 0;
#ifdef ENCODE_TYPE_IN_PATH
    if (encode_type_found(afile))
        skip += 3;
#endif

    for (int i = 0; i < afile->nvars; ++i)
    {
        const char *var_original  = afile->var_namelist[i];
        // The file may encode type in the first few chars e.g.: "_r/"
        const char *var_no_prefix = afile->var_namelist[i] + skip;

        // Skip ADIOS statistics variables.
        if (strncmp(var_original, "/__adios__", 10) == 0)
            continue;

#ifdef ENCODE_TYPE_IN_PATH
        // Skip the encode types var.
        if (strcmp(var_original, ENCODE_TYPE_VAR) == 0)
            continue;
#endif
#ifdef DISPARATE_TREE_SUPPORT
        // Skip the domain map variables.
        if (strncmp(var_original, DOMAIN_MAP_PREFIX, DOMAIN_MAP_PREFIXLEN) == 0)
            continue;
        // Skip the num writer variable.
        if (strcmp(var_original, NWRITERS_VAR) == 0)
            continue;

        int search_domain = 0;
        std::string vname;
        if (dmprefixes.empty())
        {
            // The file did not contain domain map information so the trees
            // must have been the same.
            search_domain = state->domain;
            vname = std::string(var_no_prefix);
        }
        else
        {
            // The file has domain maps. See if the variable has a prefix
            // for the domain we want.
            bool has_prefix = false;
            for (size_t j = 0; j < dmprefixes.size(); ++j)
            {
                if (strncmp(var_no_prefix, 
                    dmprefixes[j].c_str(), dmprefixes[j].size()) == 0)
                {
                    vname = std::string(var_no_prefix + dmprefixes[j].size() + 1);
                    search_domain = dmblocks[j];
                    has_prefix = true;
                    break;
                }
            }
            if (!has_prefix)
                continue;
        }
#else
        int search_domain = state->domain;
        vname = std::string(var_original);
#endif

        // Test that the variable is something we want to read.
        if (!internals::name_matches_subpaths(vname, state->subpaths))
            continue;

        DEBUG_PRINT_RANK("adios_inq_var(afile, \""
            << var_original << "\")")
        ADIOS2_VARINFO *v = adios_inq_var(afile, var_original);
        if (v)
        {
            // We have array data. Let's allocate a buffer for 
            // it and schedule a read.
            DEBUG_PRINT_RANK("adios_inq_var_blockinfo(afile, v)")
            adios_inq_var_blockinfo(afile,v);

            // Check time step validity.
            int ts = state->time_step;
            if (state->time_step == CURRENT_TIME_STEP)
                ts = afile->current_step;

#if 0
            // Print variable information.
            if (internals::rank == 0)
            {
                cout << "Reading domain=" << search_domain << ", ts=" << ts << endl;
                internals::print_varinfo(cout, afile, v);
            }
#endif

            // ADIOS time steps start at 1.
            uint32_t ts1 = static_cast<uint32_t>(ts) + 1;
            bool streaming = !streamIsFileBased(options()->read_method);

            int biOffset = 0;
            if (streaming)
            {
                // For streaming, force read of time step 0, the current time step.
                ts = 0;
            }
            else
            {
                // The blocks for the current time step start at biOffset,
                // which is the sum of the preceding block counts.
                for (int ti = 0; ti < ts; ++ti)
                    biOffset += v->nblocks[ti];
            }

            // There will be v->nblocks[ts] blocks for the current
            // time step starting at biOffset. We can make sure that
            // it's for the right time step. The blocks we ask for
            // in the read call take the time step and the local block
            // number in that time step. So, read_dom will be
            // 0..nblocks[ts]-1.
            bool block_found = false;
            uint64_t nelem = 1;
            int read_dom = search_domain;
            if (read_dom >= v->nblocks[ts])
            {
                DEBUG_PRINT_RANK("CAPPING read_com at 0");
                read_dom = 0;
            }

// NOTE: Norbert suggests not using the var block info as transports may reorganize
//       data into global arrays. It would be safer to read an element out of the
//       data using a bounding box selection (read a 1 element array slice). The
//       DATASPACES transport reassembles data from multiple rank "local" arrays
//       into bigger chunks.
//
//       For variable-sized array data, store an array of offsets into the whole data
//       array that is being saved. Burlen takes an approach like this for SENSEI.
//       Saving offsets could mean all-gathering the sizes to each rank, which would
//       be challenging for write since we don't know which ranks have what variables.
//       We could define/write a size var for array variables though. Then save the 
//       size in ADIOS. For reading, the size array would have to be read [0,rank-1]
//       and then offsets computed from it to make the bounding box selection.


            DEBUG_PRINT_RANK("state->time_step = " << state->time_step
                << ", v->nsteps=" << v->nsteps
                << ", ts=" << ts << ", ts1 = " << ts1 << endl
                << "biOffset=" << biOffset << ", read_dom=" <<read_dom);

            if (streaming ||
               v->blockinfo[biOffset + read_dom].time_index == ts1)
            {
                nelem = v->blockinfo[biOffset + read_dom].count[0];
                if (nelem == 0)
                {
                    // The count is 0 so we probably have a scalar.
                    //  Use the v->dims instead.
                    nelem = 1;
                    for (int d = 0; d < v->ndim; ++d)
                        nelem *= std::max(uint64_t(1), v->dims[d]);
                }
                block_found = true;                    
            }

            if (block_found)
            {
                // Select the block we want. This number is local to the 
                // current time step.
                DEBUG_PRINT_RANK("adios_selection_writeblock(" << read_dom << ")")
                ADIOS2_SELECTION *sel = adios_selection_writeblock(read_dom);
                sels.push_back(sel);

                // Allocate memory for the variable.
                void *vbuf = NULL;
                if (v->type == adios_byte)
                {
                    // Determine if the adios_byte was really an adios_string.
                    bool isString = false;
#ifdef ENCODE_TYPE_IN_PATH
                    isString = short_type_name_to_adios_dtype(var_original) ==
                               adios_string;
#else
                    std::string conduit_type;
                    internals::read_conduit_type_attribute(afile, 
                        v, conduit_type);
                    isString = conduit_type == "char8_str";
#endif

                    if (isString)
                    {
                        // Treat the byte array as a string.
                        // NOTE: when the data were saved, a '\0' terminator
                        //       was saved with the string. Decrement nelem
                        //       by 1 since the string will add space for
                        //       the terminator. This way the string sizes
                        //       are right.
                        size_t n_minus_nullterm = nelem - 1;
                        node[vname].set(std::string(n_minus_nullterm, ' '));
                        vbuf = (void *)node[vname].data_ptr();
                    }
                    else
                    {
                        node[vname].set(DataType::int8(nelem));
                        int8 *buf = node[vname].value();
                        vbuf = (void *)buf;
                    }
                }
                else if (v->type == adios_short)
                {
                    node[vname].set(DataType::int16(nelem));
                    int16 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_integer)
                {
                    node[vname].set(DataType::int32(nelem));
                    int32 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_long)
                {
                    node[vname].set(DataType::int64(nelem));
                    int64 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_unsigned_byte)
                {
                    node[vname].set(DataType::uint8(nelem));
                    uint8 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_unsigned_short)
                {
                    node[vname].set(DataType::uint16(nelem));
                    uint16 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_unsigned_integer)
                {
                    node[vname].set(DataType::uint32(nelem));
                    uint32 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_unsigned_long)
                {
                    node[vname].set(DataType::uint64(nelem));
                    uint64 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_real)
                {
                    node[vname].set(DataType::float32(nelem));
                    float32 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else if (v->type == adios_double)
                {
                    node[vname].set(DataType::float64(nelem));
                    float64 *buf = node[vname].value();
                    vbuf = (void *)buf;
                }
                else
                {
                    // Other cases should not happen.
                    CONDUIT_ERROR("Unsupported type " << v->type);
                }

                // Schedule the read. NOTE that the time step passed here will
                // cooperate with the block selection to get the right block
                // for the right time step.
                if (vbuf != NULL)
                {
                    DEBUG_PRINT_RANK("adios_schedule_read_byid(afile, sel, "
                        << v->varid << ", " << ts << ", 1, vbuf)")
                    adios_schedule_read_byid(afile, sel, v->varid, ts, 1, vbuf);
                    scheduled_reads++;
                }
            } // block_found
#if 0
            else if (internals::rank == 0)
            {
                // We could not find the desired block.
                cout << "No block for " << vname << " process_id=" << state->domain
                     << " time_index=" << (ts+1) << endl;
                internals::print_varinfo(cout, afile, v);
            }
#endif
            adios_free_varinfo(v);
        }           
    }

    // Perform any outstanding reads, blocking until reads are done.
    if (scheduled_reads > 0)
    {
        int blocking = 1;
        DEBUG_PRINT_RANK("adios_perform_reads(afile, " << blocking << ")")
        adios_perform_reads(afile, blocking);
    }
#if 0
    // Free the selections. (TODO: see if this still crashes ADIOS)
    for (size_t s = 0; s < sels.size(); ++s)
        adios_selection_delete(sels[s]);
#endif
}

//-----------------------------------------------------------------------------
static void
load(adios_load_state *state, Node *node
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
    )
{
    open_file_and_process(state, load_node, node
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        ,comm
#endif
        );
}

//-----------------------------------------------------------------------------
// Callback for query_number_of_domains()
static void
find_max_domains(adios_load_state *state, ADIOS2_FILE *afile, void *cbdata)
{
    int *ndoms = (int *)cbdata;
    ADIOS2_VARINFO *v;

    // Determine if there are prefixes to skip on the variables.
    int skip = 0;
#ifdef ENCODE_TYPE_IN_PATH
    if (encode_type_found(afile))
        skip += 3;
#endif
#ifdef DISPARATE_TREE_SUPPORT
    bool dm_found = domain_map_found(afile);
    skip += (dm_found ? NODEHASH_PREFIX_LEN : 0);
#endif

    // Attempt to get the number of writers.
    int nwriters_in_file = 1; // initial guess.
    if (nwriters_found(afile))
    {
        if ((v = adios_inq_var(afile, NWRITERS_VAR)) != NULL)
        {
            nwriters_in_file = *((int*)v->value);
            adios_free_varinfo(v);
        }
    }

    for (int i = 0; i < afile->nvars; ++i)
    {
        // Skip ADIOS statistics variables.
        if (strncmp(afile->var_namelist[i], "/__adios__", 10) == 0)
            continue;
#ifdef ENCODE_TYPE_IN_PATH
        // Skip the encode type in path flag.
        if (strcmp(afile->var_namelist[i], ENCODE_TYPE_VAR) == 0)
            continue;
#endif
#ifdef DISPARATE_TREE_SUPPORT
        // If the domain map is present, only consider domain map variables
        // since their length indicates a number of domains.
        if (dm_found)
        {
           bool notdm = strncmp(afile->var_namelist[i], 
                                DOMAIN_MAP_PREFIX, DOMAIN_MAP_PREFIXLEN) != 0;
           if (notdm)
               continue;
        }
#else
        std::string vname(afile->var_namelist[i] + skip);
        // Test that the variable is something we want to read.
        if (!internals::name_matches_subpaths(vname, state->subpaths))
            continue;
#endif
        DEBUG_PRINT_RANK("adios_inq_var(afile, \""
            << afile->var_namelist[i] << "\")")
        if ((v = adios_inq_var(afile, afile->var_namelist[i])) != NULL)
        {
            DEBUG_PRINT_RANK("adios_inq_var_blockinfo(afile, v)")
            adios_inq_var_blockinfo(afile,v);

             // This determines which nblocks we'll look at.
            int ts = state->time_step;
            if (state->time_step == CURRENT_TIME_STEP)
                ts = afile->current_step;
            else if (state->time_step >= v->nsteps)
                ts = v->nsteps-1;

            // Let's look at the nblocks for the current time step.
            *ndoms = std::max(*ndoms, v->nblocks[ts] / nwriters_in_file);
     
            adios_free_varinfo(v);
        }
    }
}

//-----------------------------------------------------------------------------
static int
query_number_of_domains(adios_load_state *state
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
   )
{ 
    int ndoms = 0;
    open_file_and_process(state, find_max_domains, &ndoms
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        ,comm
#endif
        );

    return ndoms;
}

//-----------------------------------------------------------------------------
// Callback for query_number_of_time_steps()
static void
find_num_timesteps(adios_load_state *, ADIOS2_FILE *afile, void *cbdata)
{
    int *ntimesteps = (int *)cbdata;
    // last_step is the index of the time. To get the number of time steps, add 1.
    *ntimesteps = afile->last_step + 1;
}


//-----------------------------------------------------------------------------
static int
query_number_of_steps(adios_load_state *state
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
   )
{ 
    int ntimesteps = 0;
    open_file_and_process(state, find_num_timesteps, &ntimesteps
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        ,comm
#endif
        );

    return ntimesteps;
}


} // namespace
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io::internals --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void
adios_set_options(const Node &opts
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
    )
{
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::initialize(comm);
#else
    internals::initialize();
#endif
    internals::options()->set(opts);
}

//-----------------------------------------------------------------------------
void
adios_options(Node &opts
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
    )
{
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::initialize(comm);
#else
    internals::initialize();
#endif
    internals::options()->about(opts);
}

//-----------------------------------------------------------------------------
void
adios_initialize_library(
    CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)
    )
{
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::initialize(comm);
#else
    internals::initialize();
#endif
}

//-----------------------------------------------------------------------------
void
adios_finalize_library(
    CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)
    )
{
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::finalize(comm);
#else
    internals::finalize();
#endif
}

//-----------------------------------------------------------------------------
void
adios_save(const Node &node, const std::string &path
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
    )
{
    unsigned int nodehash = 0;
    int nodehash_rank = 0, nodehash_size = 1;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::compute_nodehash(node, nodehash, nodehash_rank, nodehash_size, comm);
    internals::save(node, path, "w", nodehash, nodehash_rank, nodehash_size, comm);
#else
    internals::compute_nodehash(node, nodehash, nodehash_rank, nodehash_size);
    internals::save(node, path, "w", nodehash, nodehash_rank, nodehash_size);
#endif
}

//-----------------------------------------------------------------------------
void adios_save_merged(const Node &node, const std::string &path
   CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
   )
{
    // save_merged() is not allowed for streaming.
    if (!internals::streamIsFileBased(internals::options()->read_method))
    {
        CONDUIT_ERROR("save_merged() is not allowed for streaming.");
        return;
    }

    unsigned int nodehash = 0;
    int nodehash_rank = 0, nodehash_size = 1;

    // NOTE: we use "u" to update the file so the time step is not incremented.
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::compute_nodehash(node, nodehash, nodehash_rank, nodehash_size, comm);
    // TODO: read the number of domains in the file for this node hash (if hashing
    //       is present in the file) and adjust the nodehash_rank by that number.
    internals::save(node, path, "u", nodehash, nodehash_rank, nodehash_size, comm);
#else
    internals::compute_nodehash(node, nodehash, nodehash_rank, nodehash_size);
    // TODO: read the number of domains in the file for this node hash (if hashing
    //       is present in the file) and adjust the nodehash_rank by that number.
    internals::save(node, path, "u", nodehash, nodehash_rank, nodehash_size);
#endif
}

void adios_add_step(const Node &node, const std::string &path
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm))
{
    // check for ":" split  
    std::string file_path, adios_path;
    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    adios_path);

    unsigned int nodehash = 0;
    int nodehash_rank = 0, nodehash_size = 1;

    // NOTE: we use "a" to update the file to the next time step.
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::compute_nodehash(node, nodehash, nodehash_rank, nodehash_size, comm);
    internals::save(node, path, "a", nodehash, nodehash_rank, nodehash_size, comm);
#else
    internals::compute_nodehash(node, nodehash, nodehash_rank, nodehash_size);
    internals::save(node, path, "a", nodehash, nodehash_rank, nodehash_size);
#endif
}

//-----------------------------------------------------------------------------
void
adios_load(const std::string &path,
   int time_step,
   int domain, 
   Node &node
   CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
   )
{
    internals::adios_load_state state;
    state.time_step = time_step; // Force specific timestep/domain.
    state.domain = domain;

    // Split the incoming path in case it includes other information.
    // This may override the timestep and domain.
    internals::splitpath(path, state.filename, state.time_step, state.domain, state.subpaths);

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::load(&state, &node, comm);
#else
    internals::load(&state, &node);
#endif
}

//-----------------------------------------------------------------------------
void
adios_load(const std::string &path, Node &node
   CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
   )
{
    internals::adios_load_state state;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    // Read the rank'th domain if there is one.
    MPI_Comm_rank(comm, &state.domain);
#endif

    // Split the incoming path in case it includes other information.
    // This may override the timestep and domain.
    internals::splitpath(path, state.filename, state.time_step, state.domain, state.subpaths);

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    internals::load(&state, &node, comm);
#else
    internals::load(&state, &node);
#endif
}

//-----------------------------------------------------------------------------
int
adios_query_number_of_steps(const std::string &path
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
   )
{
    internals::adios_load_state state;
    std::string tmp;

    // check for ":" split  
    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    state.filename,
                                    tmp);

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    return internals::query_number_of_steps(&state, comm);
#else
    return internals::query_number_of_steps(&state);
#endif
}

//-----------------------------------------------------------------------------
int
adios_query_number_of_domains(const std::string &path
    CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)
   )
{
    internals::adios_load_state state;

    // Split the incoming path in case it includes other information.
    // This may override the timestep and domain.
    internals::splitpath(path, state.filename, state.time_step, state.domain, state.subpaths);

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    return internals::query_number_of_domains(&state, comm);
#else
    return internals::query_number_of_domains(&state);
#endif
}

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
