// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_mpi.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay_mpi.hpp"
#include <algorithm>
#include <iostream>
#include <limits>

//-----------------------------------------------------------------------------
/// The CONDUIT_CHECK_MPI_ERROR macro is used to check return values for
/// mpi calls.
//-----------------------------------------------------------------------------
#define CONDUIT_CHECK_MPI_ERROR( check_mpi_err_code )               \
{                                                                   \
    if( static_cast<int>(check_mpi_err_code) != MPI_SUCCESS)        \
    {                                                               \
        char check_mpi_err_str_buff[MPI_MAX_ERROR_STRING];          \
        int  check_mpi_err_str_len=0;                               \
        MPI_Error_string( check_mpi_err_code ,                      \
                          check_mpi_err_str_buff,                   \
                          &check_mpi_err_str_len);                  \
                                                                    \
        CONDUIT_ERROR("MPI call failed: \n"                         \
                      << " error code = "                           \
                      <<  check_mpi_err_code  << "\n"               \
                      << " error message = "                        \
                      <<  check_mpi_err_str_buff << "\n");          \
        return  check_mpi_err_code;                                 \
    }                                                               \
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


//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
//-----------------------------------------------------------------------------
namespace mpi
{


//-----------------------------------------------------------------------------
int
size(MPI_Comm mpi_comm)
{
    int res;
    MPI_Comm_size(mpi_comm,&res);
    return res;
};

//-----------------------------------------------------------------------------
int
rank(MPI_Comm mpi_comm)
{
    int res;
    MPI_Comm_rank(mpi_comm,&res);
    return res;
}

//-----------------------------------------------------------------------------
MPI_Datatype
conduit_dtype_to_mpi_dtype(const DataType &dt)
{
    MPI_Datatype res = MPI_DATATYPE_NULL;

    // can't use switch w/ case statements here b/c NATIVE_IDS may actually
    // be overloaded on some platforms (this happens on windows)

    index_t dt_id = dt.id();

    // signed integer types
    if(dt_id == CONDUIT_INT8_ID)
    {
        res = MPI_INT8_T;
    }
    else if( dt_id == CONDUIT_INT16_ID)
    {
        res = MPI_INT16_T;
    }
    else if( dt_id == CONDUIT_INT32_ID)
    {
        res = MPI_INT32_T;
    }
    else if( dt_id == CONDUIT_INT64_ID)
    {
        res = MPI_INT64_T;
    }
    // unsigned integer types
    else if( dt_id == CONDUIT_UINT8_ID)
    {
        res = MPI_UINT8_T;
    }
    else if( dt_id == CONDUIT_UINT16_ID)
    {
        res = MPI_UINT16_T;
    }
    else if( dt_id == CONDUIT_UINT32_ID)
    {
        res = MPI_UINT32_T;
    }
    else if( dt_id == CONDUIT_UINT64_ID)
    {
        res = MPI_UINT64_T;
    }
    // floating point types
    else if( dt_id == CONDUIT_NATIVE_FLOAT_ID)
    {
        res = MPI_FLOAT;
    }
    else if( dt_id == CONDUIT_NATIVE_DOUBLE_ID)
    {
        res = MPI_DOUBLE;
    }
    #if defined(CONDUIT_USE_LONG_DOUBLE)
    else if( dt_id == CONDUIT_NATIVE_LONG_DOUBLE_ID)
    {
        res = MPI_LONG_DOUBLE;
    }
    #endif
    // string type
    else if( dt_id == DataType::CHAR8_STR_ID)
    {
        res = MPI_CHAR;
    }

    return res;
}


//-----------------------------------------------------------------------------
index_t
mpi_dtype_to_conduit_dtype_id(MPI_Datatype dt)
{
    index_t res = DataType::EMPTY_ID;

    // can't use switch w/ case statements here b/c in some
    // MPI implementations MPI_Datatype is a struct (or something more complex)
    // that won't compile when used in a switch statement.

    // string type
    if(dt == MPI_CHAR)
    {
        res = DataType::CHAR8_STR_ID;
    }
    // mpi c bw-style signed integer types
    if(dt == MPI_INT8_T)
    {
        res = CONDUIT_INT8_ID;
    }
    else if( dt == MPI_INT16_T)
    {
        res = CONDUIT_INT16_ID;
    }
    else if( dt == MPI_INT32_T)
    {
        res = CONDUIT_INT32_ID;
    }
    else if( dt == MPI_INT64_T)
    {
        res = CONDUIT_INT64_ID;
    }
    // mpi c bw-style unsigned integer types
    else if( dt == MPI_UINT8_T)
    {
        res = CONDUIT_UINT8_ID;
    }
    else if( dt == MPI_UINT16_T)
    {
        res = CONDUIT_UINT16_ID;
    }
    else if( dt == MPI_UINT32_T)
    {
        res = CONDUIT_UINT32_ID;
    }
    else if( dt == MPI_UINT64_T)
    {
        res = CONDUIT_UINT64_ID;
    }
    // native c signed integer types
    else if(dt == MPI_SHORT)
    {
        res = CONDUIT_NATIVE_SHORT_ID;
    }
    else if(dt == MPI_INT)
    {
        res = CONDUIT_NATIVE_INT_ID;
    }
    else if(dt == MPI_LONG)
    {
        res = CONDUIT_NATIVE_LONG_ID;
    }
    #if defined(CONDUIT_HAS_LONG_LONG)
    else if(dt == MPI_LONG_LONG)
    {
        res = CONDUIT_NATIVE_LONG_LONG_ID;
    }
    #endif
    // native c unsigned integer types
    else if(dt == MPI_BYTE)
    {
        res = CONDUIT_NATIVE_UNSIGNED_CHAR_ID;
    }
    else if(dt == MPI_UNSIGNED_CHAR)
    {
        res = CONDUIT_NATIVE_UNSIGNED_CHAR_ID;
    }
    else if(dt == MPI_UNSIGNED_SHORT)
    {
        res = CONDUIT_NATIVE_UNSIGNED_SHORT_ID;
    }
    else if(dt == MPI_UNSIGNED)
    {
        res = CONDUIT_NATIVE_UNSIGNED_INT_ID;
    }
    else if(dt == MPI_UNSIGNED_LONG)
    {
        res = CONDUIT_NATIVE_UNSIGNED_LONG_ID;
    }
    #if defined(CONDUIT_HAS_LONG_LONG)
    else if(dt == MPI_UNSIGNED_LONG_LONG)
    {
        res = CONDUIT_NATIVE_UNSIGNED_LONG_LONG_ID;
    }
    #endif
    // floating point types
    else if(dt == MPI_FLOAT)
    {
        res = CONDUIT_NATIVE_FLOAT_ID;
    }
    else if(dt == MPI_DOUBLE)
    {
        res = CONDUIT_NATIVE_DOUBLE_ID;
    }
    #if defined(CONDUIT_USE_LONG_DOUBLE)
    else if(dt == MPI_LONG_DOUBLE)
    {
        res = CONDUIT_NATIVE_LONG_DOUBLE_ID;
    }
    #endif
    return res;
}

//---------------------------------------------------------------------------//
/**
 @brief Some MPI installations install an error handler that causes functions
        to abort if there was an error. That is fine but it makes it difficult
        to find the cause of an error. This class installs a temporary error
        handling function that we can use to trap errors, etc. It follows an
        RAII pattern.
 */
class HandleMPICommError
{
public:
    HandleMPICommError(MPI_Comm comm) : m_comm(comm), m_oldHandlerSet(false),
        m_newHandlerSet(false)
    {
        initialize();
    }

    ~HandleMPICommError()
    {
        finalize();
    }
private:
    /// Create and install the new error handler.
    int initialize()
    {
        int mpi_error = MPI_Comm_get_errhandler(m_comm, &m_oldHandler);
        CONDUIT_CHECK_MPI_ERROR(mpi_error);
        m_oldHandlerSet = true;

        // Create/install a new error handler.
        mpi_error = MPI_Comm_create_errhandler(handler, &m_newHandler);
        CONDUIT_CHECK_MPI_ERROR(mpi_error);
        m_newHandlerSet = true;
        mpi_error = MPI_Comm_set_errhandler(m_comm, m_newHandler);
        CONDUIT_CHECK_MPI_ERROR(mpi_error);

        return MPI_SUCCESS;
    }

    /// Restore the old error handler.
    int finalize()
    {
        if(m_oldHandlerSet && m_newHandlerSet)
        {
            int mpi_error = MPI_Comm_set_errhandler(m_comm, m_oldHandler);
            CONDUIT_CHECK_MPI_ERROR(mpi_error);

            mpi_error = MPI_Errhandler_free(&m_newHandler);
            CONDUIT_CHECK_MPI_ERROR(mpi_error);
        }

        return MPI_SUCCESS;
    }

    /// MPI calls this function to handle errors.
    static void handler(MPI_Comm * /*comm*/,
                        int      * /*errcode*/,
                        ...)
    {
#if 0
        // Variable arguments are MPI implementation-dependent.
        // Be sure to include <cstdarg>
        va_list argp;
        va_start(argp, errcode);
        // Get the next arg as an int pointer.
        int *arg2 = va_arg(argp, int*);
        va_end(argp);
#endif

        //std::cout << "handler: comm=" << *comm << ", errcode=" << *errcode << std::endl;

#if 0
        // We could try emitting a Conduit error.
        (void)checkError(*errcode);
#endif
#if 0
        // We could tell MPI to ignore the error and continue.
        *errcode = MPI_SUCCESS;
#endif
    }

    static int checkError(int errcode)
    {
        CONDUIT_CHECK_MPI_ERROR(errcode);
        return MPI_SUCCESS;
    }

    MPI_Comm       m_comm;
    bool           m_oldHandlerSet;
    bool           m_newHandlerSet;
    MPI_Errhandler m_oldHandler;
    MPI_Errhandler m_newHandler;
};

//---------------------------------------------------------------------------//
/**
 * @brief This class helps to determine MPI tag upper limits.
 */
class TagLimits
{
public:
    /**
     * @brief Return the tag upper limit.
     *
     * @param comm The MPI communicator.
     *
     * @return The tag upper limit.
     *
     * @note We probe to determine the value since query is not as reliable across MPI distributions.
     */
    static int upper_bound(MPI_Comm comm)
    {
      return probe(comm);
    }

private:
    /**
     * @brief Query MPI for the maximum tag value.
     *
     * @param comm The MPI communicator.
     *
     * @return The tag upper bound or -1 on error.
     *
     * @note This is how we are supposed to be able to ask for the max tag value.
     *       However, this method does not seem reliable across MPIs and it is
     *       possible for the query to return values that still do not work in
     *       Isend/Irecv sometimes.
     */
    static int query(MPI_Comm comm)
    {
        bool ok = false;
        int tag_ub = 0, flag = 0;
        int mpi_error = MPI_Comm_get_attr(comm, MPI_TAG_UB, &tag_ub, &flag);
        if(mpi_error == MPI_SUCCESS && flag != 0)
        {
            if(tag_ub > 0)
            {
                ok = true;
            }
        }
        return ok ? tag_ub : -1;
    }

    /**
     * @brief Probe MPI to determine the max tag value.
     *
     * @param comm The MPI communicator.
     *
     * @note MPI error handlers are installed that ignore problems, preventing the
     *       program from dying if the default handler is set to abort on error. The
     *       error handler is restored when leaving this function.
     */
    static int probe(MPI_Comm comm)
    {
        // Temporarily override MPI error handler with a more benign one.
        HandleMPICommError err(comm);
        int tag = probeTagUpperBound(0, std::numeric_limits<int>::max(), comm);
        return tag;
    }

    /**
     * @brief Probe a range of tag values to determine if the range is valid.
     *
     * @param low The low tag value
     * @param high The high tag value
     * @param comm The MPI communicator.
     *
     * @return The max tag value.
     *
     * @note The rank sends a message to itself using a tag value. The result of that
     *       is used to narrow the range of tag values.
     */
    static int probeTagUpperBound(int low, int high, MPI_Comm comm)
    {
        int tag;
        if((high - low) < 2)
            tag = low;
        else
        {
            int rank;
            MPI_Comm_rank(comm, &rank);

            tag = (low + high) / 2;

            // Try sending with the current tag.
            int srcBuff = 0;
            MPI_Request requests[2];
            int mpi_error = MPI_Isend(&srcBuff,
                                      1,
                                      MPI_INT,
                                      rank,
                                      tag,
                                      comm,
                                      &requests[0]);
            if(mpi_error == MPI_SUCCESS)
            {
                // It worked.
                // Issue the recv.
                int destBuff = 0;
                MPI_Irecv(&destBuff,
                          1,
                          MPI_INT,
                          rank,
                          tag,
                          comm,
                          &requests[1]);

                MPI_Status statuses[2];
                MPI_Waitall(2, requests, statuses);

                tag = probeTagUpperBound(tag, high, comm);
            }
            else
            {
                tag = probeTagUpperBound(low, tag, comm);
            }
        }
        return tag;
    }
};

//---------------------------------------------------------------------------//
/**
 @brief Checks whether the input tag is invalid.

 @param tag The MPI tag to check for validity.
 @return True if the tag is wildly incorrect (negative).

 @note Right now, this function only flags tags that are negative. Tags are
       supposed to be less than or equal to MPI_TAG_UB too. This function does
       not check for that - it's done by using safe_tag when tags are used.
 */
bool invalid_tag(int tag)
{
    return tag < 0;
}

//---------------------------------------------------------------------------//
/**
 @brief Return a tag value that is safe to use with MPI functions. The tag is
        determined dynamically the first time the function is called. If the
        input tag is greater than the max tag then the value is clamped.

 @param tag The input tag.
 @param comm The MPI communicator.

 @return A tag value that is safe to use with MPI.
 */
int safe_tag(int tag, MPI_Comm comm)
{
    static constexpr int UPPER_BOUND_NOT_SET = -1;
    static int tag_upper_bound = UPPER_BOUND_NOT_SET;
    if(tag_upper_bound == UPPER_BOUND_NOT_SET)
    {
        // The first time through, determine the upper bound.
        tag_upper_bound = TagLimits::upper_bound(comm);
    }

    int newtag = std::max(0, tag);
    if(newtag > tag_upper_bound)
    {
        newtag = tag_upper_bound;
    }

    return newtag;
}

//---------------------------------------------------------------------------//
int
send_using_schema(const Node &node, int dest, int tag, MPI_Comm comm)
{
    Schema s_data_compact;

    // schema will only be valid if compact and contig
    if( node.is_compact() && node.is_contiguous())
    {
        s_data_compact = node.schema();
    }
    else
    {
        node.schema().compact_to(s_data_compact);
    }

    std::string snd_schema_json = s_data_compact.to_json();

    Schema s_msg;
    s_msg["schema_len"].set(DataType::int64());
    s_msg["schema"].set(DataType::char8_str(snd_schema_json.size()+1));
    s_msg["data"].set(s_data_compact);

    // create a compact schema to use
    Schema s_msg_compact;
    s_msg.compact_to(s_msg_compact);

    Node n_msg(s_msg_compact);
    // these sets won't realloc since schemas are compatible
    n_msg["schema_len"].set((int64)snd_schema_json.length());
    n_msg["schema"].set(snd_schema_json);
    n_msg["data"].update(node);


    index_t msg_data_size = n_msg.total_bytes_compact();

    if(!conduit::utils::value_fits<index_t,int>(msg_data_size))
    {
        CONDUIT_INFO("Warning size value (" << msg_data_size << ")"
                     " exceeds the size of MPI_Send max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }

    int mpi_error = MPI_Send(const_cast<void*>(n_msg.data_ptr()),
                             static_cast<int>(msg_data_size),
                             MPI_BYTE,
                             dest,
                             tag,
                             comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    return mpi_error;
}


//---------------------------------------------------------------------------//
int
recv_using_schema(Node &node, int src, int tag, MPI_Comm comm)
{
    MPI_Status status;

    int mpi_error = MPI_Probe(src, tag, comm, &status);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    int buffer_size = 0;
    MPI_Get_count(&status, MPI_BYTE, &buffer_size);

    Node n_buffer(DataType::uint8(buffer_size));

    mpi_error = MPI_Recv(n_buffer.data_ptr(),
                         buffer_size,
                         MPI_BYTE,
                         status.MPI_SOURCE,
                         status.MPI_TAG,
                         comm,
                         &status);

    uint8 *n_buff_ptr = (uint8*)n_buffer.data_ptr();

    Node n_msg;
    // length of the schema is sent as a 64-bit signed int
    // NOTE: we aren't using this value  ...
    n_msg["schema_len"].set_external((int64*)n_buff_ptr);
    n_buff_ptr +=8;
    // wrap the schema string
    n_msg["schema"].set_external_char8_str((char*)(n_buff_ptr));
    // create the schema
    Schema rcv_schema;
    Generator gen(n_msg["schema"].as_char8_str());
    gen.walk(rcv_schema);

    // advance by the schema length
    n_buff_ptr += n_msg["schema"].total_bytes_compact();

    // apply the schema to the data
    n_msg["data"].set_external(rcv_schema,n_buff_ptr);

    // copy out to our result node
    node.update(n_msg["data"]);

    return mpi_error;
}

//---------------------------------------------------------------------------//
// any source, any tag variant
int
recv_using_schema(Node &node, MPI_Comm comm)
{
    MPI_Status status;

    int mpi_error = MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &status);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    int buffer_size = 0;
    MPI_Get_count(&status, MPI_BYTE, &buffer_size);

    Node n_buffer(DataType::uint8(buffer_size));

    mpi_error = MPI_Recv(n_buffer.data_ptr(),
                         buffer_size,
                         MPI_BYTE,
                         status.MPI_SOURCE,
                         status.MPI_TAG,
                         comm,
                         &status);

    uint8 *n_buff_ptr = (uint8*)n_buffer.data_ptr();

    Node n_msg;
    // length of the schema is sent as a 64-bit signed int
    // NOTE: we aren't using this value  ...
    n_msg["schema_len"].set_external((int64*)n_buff_ptr);
    n_buff_ptr +=8;
    // wrap the schema string
    n_msg["schema"].set_external_char8_str((char*)(n_buff_ptr));
    // create the schema
    Schema rcv_schema;
    Generator gen(n_msg["schema"].as_char8_str());
    gen.walk(rcv_schema);

    // advance by the schema length
    n_buff_ptr += n_msg["schema"].total_bytes_compact();

    // apply the schema to the data
    n_msg["data"].set_external(rcv_schema,n_buff_ptr);

    // copy out to our result node
    node.update(n_msg["data"]);

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
send(const Node &node, int dest, int tag, MPI_Comm comm)
{
    // assumes size and type are known on the other end

    Node snd_compact;

    const void *snd_ptr = node.contiguous_data_ptr();;
    index_t    snd_size = node.total_bytes_compact();;

    if( snd_ptr == NULL ||
        ! node.is_compact())
    {
         node.compact_to(snd_compact);
         snd_ptr = snd_compact.data_ptr();
    }

    if(!conduit::utils::value_fits<index_t,int>(snd_size))
    {
        CONDUIT_INFO("Warning size value (" << snd_size << ")"
                     " exceeds the size of MPI_Send max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }

    int mpi_error = MPI_Send(const_cast<void*>(snd_ptr),
                             static_cast<int>(snd_size),
                             MPI_BYTE,
                             dest,
                             tag,
                             comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
recv(Node &node, int src, int tag, MPI_Comm comm)
{

    MPI_Status status;
    Node rcv_compact;

    bool cpy_out = false;

    const void *rcv_ptr  = node.contiguous_data_ptr();
    index_t     rcv_size = node.total_bytes_compact();

    if( rcv_ptr == NULL  ||
        ! node.is_compact() )
    {
        // we will need to update into rcv node
        cpy_out = true;
        Schema s_rcv_compact;
        node.schema().compact_to(s_rcv_compact);
        rcv_compact.set_schema(s_rcv_compact);
        rcv_ptr  = rcv_compact.data_ptr();
    }

    if(!conduit::utils::value_fits<index_t,int>(rcv_size))
    {
        CONDUIT_INFO("Warning size value (" << rcv_size << ")"
                     " exceeds the size of MPI_Recv max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }


    int mpi_error = MPI_Recv(const_cast<void*>(rcv_ptr),
                             static_cast<int>(rcv_size),
                             MPI_BYTE,
                             src,
                             tag,
                             comm,
                             &status);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    if(cpy_out)
    {
        node.update(rcv_compact);
    }

    return mpi_error;
}

//---------------------------------------------------------------------------//
// any source, any tag variant
int
recv(Node &node, MPI_Comm comm)
{

    MPI_Status status;
    Node rcv_compact;

    bool cpy_out = false;

    const void *rcv_ptr  = node.contiguous_data_ptr();
    index_t     rcv_size = node.total_bytes_compact();

    if( rcv_ptr == NULL  ||
        ! node.is_compact() )
    {
        // we will need to update into rcv node
        cpy_out = true;
        Schema s_rcv_compact;
        node.schema().compact_to(s_rcv_compact);
        rcv_compact.set_schema(s_rcv_compact);
        rcv_ptr  = rcv_compact.data_ptr();
    }

    if(!conduit::utils::value_fits<index_t,int>(rcv_size))
    {
        CONDUIT_INFO("Warning size value (" << rcv_size << ")"
                     " exceeds the size of MPI_Recv max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }


    int mpi_error = MPI_Recv(const_cast<void*>(rcv_ptr),
                             static_cast<int>(rcv_size),
                             MPI_BYTE,
                             MPI_ANY_SOURCE,
                             MPI_ANY_TAG,
                             comm,
                             &status);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    if(cpy_out)
    {
        node.update(rcv_compact);
    }

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
reduce(const Node &snd_node,
       Node &rcv_node,
       MPI_Op mpi_op,
       int root,
       MPI_Comm mpi_comm)
{
    MPI_Datatype mpi_dtype = conduit_dtype_to_mpi_dtype(snd_node.dtype());

    if(mpi_dtype == MPI_DATATYPE_NULL)
    {
        CONDUIT_ERROR("Unsupported send DataType for mpi::reduce"
                      << snd_node.dtype().name());
    }

    void *snd_ptr = NULL;
    void *rcv_ptr = NULL;

    Node snd_compact;
    Node rcv_compact;

    //note: we don't have to ask for contig in this case, since
    // we can only reduce leaf types
    if(snd_node.is_compact())
    {
        snd_ptr = const_cast<void*>(snd_node.data_ptr());
    }
    else
    {
        snd_node.compact_to(snd_compact);
        snd_ptr = snd_compact.data_ptr();
    }

    bool cpy_out = false;

    int rank = mpi::rank(mpi_comm);

    if( rank == root )
    {

        rcv_ptr = rcv_node.contiguous_data_ptr();

        // make sure `rcv_node` can hold data described by `snd_node`
        if( !rcv_node.compatible(snd_node) ||
            rcv_ptr == NULL ||
            !rcv_node.is_compact() )
        {
            // we will need to update into rcv node
            cpy_out = true;

            Schema s_snd_compact;
            snd_node.schema().compact_to(s_snd_compact);

            rcv_compact.set_schema(s_snd_compact);
            rcv_ptr = rcv_compact.data_ptr();
        }
    }

    int num_eles = (int) snd_node.dtype().number_of_elements();

    int mpi_error = MPI_Reduce(snd_ptr,
                               rcv_ptr,
                               num_eles,
                               mpi_dtype,
                               mpi_op,
                               root,
                               mpi_comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    if( rank == root  && cpy_out )
    {
        rcv_node.update(rcv_compact);
    }

    return mpi_error;
}

//--------------------------------------------------------------------------//
int
all_reduce(const Node &snd_node,
           Node &rcv_node,
           MPI_Op mpi_op,
           MPI_Comm mpi_comm)
{
    MPI_Datatype mpi_dtype = conduit_dtype_to_mpi_dtype(snd_node.dtype());

    if(mpi_dtype == MPI_DATATYPE_NULL)
    {
        CONDUIT_ERROR("Unsupported send DataType for mpi::all_reduce"
                      << snd_node.dtype().name());
    }


    void *snd_ptr = NULL;
    void *rcv_ptr = NULL;

    Node snd_compact;
    Node rcv_compact;

    //note: we don't have to ask for contig in this case, since
    // we can only reduce leaf types
    if(snd_node.is_compact())
    {
        snd_ptr = const_cast<void*>(snd_node.data_ptr());
    }
    else
    {
        snd_node.compact_to(snd_compact);
        snd_ptr = snd_compact.data_ptr();
    }

    bool cpy_out = false;


    rcv_ptr = rcv_node.contiguous_data_ptr();

    // make sure `rcv_node` can hold data described by `snd_node`
    if( !rcv_node.compatible(snd_node) ||
        rcv_ptr == NULL ||
        !rcv_node.is_compact() )
    {
        // we will need to update into rcv node
        cpy_out = true;

        Schema s_snd_compact;
        snd_node.schema().compact_to(s_snd_compact);

        rcv_compact.set_schema(s_snd_compact);
        rcv_ptr = rcv_compact.data_ptr();
    }

    int num_eles = (int) snd_node.dtype().number_of_elements();

    int mpi_error = MPI_Allreduce(snd_ptr,
                                  rcv_ptr,
                                  num_eles,
                                  mpi_dtype,
                                  mpi_op,
                                  mpi_comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    if(cpy_out)
    {
        rcv_node.update(rcv_compact);
    }


    return mpi_error;
}

//-- reduce helpers -- //

//---------------------------------------------------------------------------//
int
sum_reduce(const Node &snd_node,
           Node &rcv_node,
           int root,
           MPI_Comm mpi_comm)
{
    return reduce(snd_node,
                  rcv_node,
                  MPI_SUM,
                  root,
                  mpi_comm);
}


//---------------------------------------------------------------------------//
int
min_reduce(const Node &snd_node,
           Node &rcv_node,
           int root,
           MPI_Comm mpi_comm)
{
    return reduce(snd_node,
                  rcv_node,
                  MPI_MIN,
                  root,
                  mpi_comm);
}



//---------------------------------------------------------------------------//
int
max_reduce(const Node &snd_node,
           Node &rcv_node,
           int root,
           MPI_Comm mpi_comm)
{
    return reduce(snd_node,
                  rcv_node,
                  MPI_MAX,
                  root,
                  mpi_comm);
}



//---------------------------------------------------------------------------//
int
prod_reduce(const Node &snd_node,
            Node &rcv_node,
            int root,
            MPI_Comm mpi_comm)
{
    return reduce(snd_node,
                  rcv_node,
                  MPI_PROD,
                  root,
                  mpi_comm);
}



//--- all reduce helpers -- /
//---------------------------------------------------------------------------//
int
sum_all_reduce(const Node &snd_node,
               Node &rcv_node,
               MPI_Comm mpi_comm)
{
    return all_reduce(snd_node,
                      rcv_node,
                      MPI_SUM,
                      mpi_comm);
}


//---------------------------------------------------------------------------//
int
min_all_reduce(const Node &snd_node,
               Node &rcv_node,
               MPI_Comm mpi_comm)
{
    return all_reduce(snd_node,
                      rcv_node,
                      MPI_MIN,
                      mpi_comm);

}



//---------------------------------------------------------------------------//
int
max_all_reduce(const Node &snd_node,
               Node &rcv_node,
               MPI_Comm mpi_comm)
{
    return all_reduce(snd_node,
                      rcv_node,
                      MPI_MAX,
                      mpi_comm);

}


//---------------------------------------------------------------------------//
int
prod_all_reduce(const Node &snd_node,
                Node &rcv_node,
                MPI_Comm mpi_comm)
{
    return all_reduce(snd_node,
                      rcv_node,
                      MPI_PROD,
                      mpi_comm);

}



//---------------------------------------------------------------------------//
int
isend(const Node &node,
      int dest,
      int tag,
      MPI_Comm mpi_comm,
      Request *request)
{
    if(invalid_tag(tag))
    {
       CONDUIT_ERROR("isend given invalid tag (" << tag << ").");
    }

    const void *data_ptr  = node.contiguous_data_ptr();
    index_t     data_size = node.total_bytes_compact();

    // note: this checks for both compact and contig
    if( data_ptr == NULL ||
       !node.is_compact() )
    {
        node.compact_to(request->m_buffer);
        data_ptr  = request->m_buffer.data_ptr();
    }

    // for wait_all,  this must always be NULL except for
    // the irecv cases where copy out is necessary
    // isend case must always be NULL
    request->m_rcv_ptr = NULL;


    if(!conduit::utils::value_fits<index_t,int>(data_size))
    {
        CONDUIT_INFO("Warning size value (" << data_size << ")"
                     " exceeds the size of MPI_Isend max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }

    const int newtag = safe_tag(tag, mpi_comm);
    int mpi_error =  MPI_Isend(const_cast<void*>(data_ptr),
                               static_cast<int>(data_size),
                               MPI_BYTE,
                               dest,
                               newtag,
                               mpi_comm,
                               &(request->m_request));

    CONDUIT_CHECK_MPI_ERROR(mpi_error);
    return mpi_error;
}

//---------------------------------------------------------------------------//
int
irecv(Node &node,
      int src,
      int tag,
      MPI_Comm mpi_comm,
      Request *request)
{
    if(invalid_tag(tag))
    {
       CONDUIT_ERROR("irecv given invalid tag (" << tag << ").");
    }

    // if rcv is compact, we can write directly into recv
    // if it's not compact, we need a recv_buffer

    void    *data_ptr  = node.contiguous_data_ptr();
    index_t  data_size = node.total_bytes_compact();

    // note: this checks for both compact and contig
    if(data_ptr != NULL &&
       node.is_compact() )
    {
        // for wait_all,  this must always be NULL except for
        // the irecv cases where copy out is necessary
        request->m_rcv_ptr = NULL;
    }
    else
    {
        node.compact_to(request->m_buffer);
        data_ptr  = request->m_buffer.data_ptr();
        request->m_rcv_ptr = &node;
    }

    if(!conduit::utils::value_fits<index_t,int>(data_size))
    {
        CONDUIT_INFO("Warning size value (" << data_size << ")"
                     " exceeds the size of MPI_Irecv max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }

    const int newtag = safe_tag(tag, mpi_comm);
    int mpi_error =  MPI_Irecv(data_ptr,
                               static_cast<int>(data_size),
                               MPI_BYTE,
                               src,
                               newtag,
                               mpi_comm,
                               &(request->m_request));

    CONDUIT_CHECK_MPI_ERROR(mpi_error);
    return mpi_error;
}

//---------------------------------------------------------------------------//
// any source any tag variant
int
irecv(Node &node,
      MPI_Comm mpi_comm,
      Request *request)
{
    // if rcv is compact, we can write directly into recv
    // if it's not compact, we need a recv_buffer

    void    *data_ptr  = node.contiguous_data_ptr();
    index_t  data_size = node.total_bytes_compact();

    // note: this checks for both compact and contig
    if(data_ptr != NULL &&
       node.is_compact() )
    {
        // for wait_all,  this must always be NULL except for
        // the irecv cases where copy out is necessary
        request->m_rcv_ptr = NULL;
    }
    else
    {
        node.compact_to(request->m_buffer);
        data_ptr  = request->m_buffer.data_ptr();
        request->m_rcv_ptr = &node;
    }

    if(!conduit::utils::value_fits<index_t,int>(data_size))
    {
        CONDUIT_INFO("Warning size value (" << data_size << ")"
                     " exceeds the size of MPI_Irecv max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }

    int mpi_error =  MPI_Irecv(data_ptr,
                               static_cast<int>(data_size),
                               MPI_BYTE,
                               MPI_ANY_SOURCE,
                               MPI_ANY_TAG,
                               mpi_comm,
                               &(request->m_request));

    CONDUIT_CHECK_MPI_ERROR(mpi_error);
    return mpi_error;
}



//---------------------------------------------------------------------------//
// wait handles both send and recv requests
int
wait(Request *request,
     MPI_Status *status)
{
    int mpi_error = MPI_Wait(&(request->m_request), status);
    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    // we need to update if m_rcv_ptr was used
    // this will only be non NULL in the recv copy out case,
    // sends will always be NULL
    if(request->m_rcv_ptr)
    {
        request->m_rcv_ptr->update(request->m_buffer);
    }

    request->m_buffer.reset();
    request->m_rcv_ptr = NULL;

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
wait_send(Request *request,
          MPI_Status *status)
{
    return wait(request,status);
}

//---------------------------------------------------------------------------//
int
wait_recv(Request *request,
          MPI_Status *status)
{
    return wait(request,status);
}


//---------------------------------------------------------------------------//
// wait all handles mixed batches of sends and receives
int
wait_all(int count,
         Request requests[],
         MPI_Status statuses[])
{
     MPI_Request *justrequests = new MPI_Request[count];

     for (int i = 0; i < count; ++i)
     {
         // mpi requests can be simply copied
         justrequests[i] = requests[i].m_request;
     }

     int mpi_error = MPI_Waitall(count, justrequests, statuses);
     CONDUIT_CHECK_MPI_ERROR(mpi_error);

     for (int i = 0; i < count; ++i)
     {
         // if this request is a recv, we need to check for copy out
         // m_rcv_ptr will always be NULL, unless we have done a
         // irecv where we need to use the pointer.
         if(requests[i].m_rcv_ptr != NULL)
         {
             requests[i].m_rcv_ptr->update(requests[i].m_buffer);
             requests[i].m_rcv_ptr = NULL;
         }

         requests[i].m_request = justrequests[i];
         requests[i].m_buffer.reset();
     }

     delete [] justrequests;

     return mpi_error;
}

//---------------------------------------------------------------------------//
int
wait_all_send(int count,
              Request requests[],
              MPI_Status statuses[])
{
    return wait_all(count,requests,statuses);
}

//---------------------------------------------------------------------------//
int
wait_all_recv(int count,
              Request requests[],
              MPI_Status statuses[])
{
   return  wait_all(count,requests,statuses);
}

//---------------------------------------------------------------------------//
int
gather(Node &send_node,
       Node &recv_node,
       int root,
       MPI_Comm mpi_comm)
{
    Node   n_snd_compact;
    Schema s_snd_compact;

    send_node.schema().compact_to(s_snd_compact);

    const void *snd_ptr = send_node.contiguous_data_ptr();
    index_t    snd_size = 0;


    if(snd_ptr != NULL &&
       send_node.is_compact() )
    {
        snd_ptr  = send_node.data_ptr();
        snd_size = send_node.total_bytes_compact();
    }
    else
    {
        send_node.compact_to(n_snd_compact);
        snd_ptr  = n_snd_compact.data_ptr();
        snd_size = n_snd_compact.total_bytes_compact();
    }

    int mpi_rank = mpi::rank(mpi_comm);
    int mpi_size = mpi::size(mpi_comm);

    if(mpi_rank == root)
    {
        // TODO: copy out support w/o always reallocing?
        recv_node.list_of(s_snd_compact,
                          mpi_size);
    }

    if(!conduit::utils::value_fits<index_t,int>(snd_size))
    {
        CONDUIT_INFO("Warning size value (" << snd_size << ")"
                     " exceeds the size of MPI_Gather max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }

    int mpi_error = MPI_Gather( const_cast<void*>(snd_ptr), // local data
                                static_cast<int>(snd_size), // local data len
                                MPI_BYTE, // send chars
                                recv_node.data_ptr(),  // rcv buffer
                                static_cast<int>(snd_size), // data len
                                MPI_BYTE,  // rcv chars
                                root,
                                mpi_comm); // mpi com

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
all_gather(Node &send_node,
           Node &recv_node,
           MPI_Comm mpi_comm)
{
    Node   n_snd_compact;
    Schema s_snd_compact;
    send_node.schema().compact_to(s_snd_compact);

    const void *snd_ptr  = send_node.contiguous_data_ptr();
    index_t     snd_size = send_node.total_bytes_compact();


    if( snd_ptr == NULL ||
       !send_node.is_compact() )
    {
        send_node.compact_to(n_snd_compact);
        snd_ptr  = n_snd_compact.data_ptr();
    }
    // TODO: copy out support w/o always reallocing?
    // TODO: what about common case of scatter w/ leaf types?
    //       instead of list_of, we would have a leaf of
    //       of a given type w/ # of elements == # of ranks.

    int mpi_size = mpi::size(mpi_comm);

    recv_node.list_of(s_snd_compact,
                      mpi_size);

    if(!conduit::utils::value_fits<index_t,int>(snd_size))
    {
        CONDUIT_INFO("Warning size value (" << snd_size << ")"
                     " exceeds the size of MPI_Gather max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }

    int mpi_error = MPI_Allgather( const_cast<void*>(snd_ptr), // local data
                                   static_cast<int>(snd_size), // local data len
                                   MPI_BYTE, // send chars
                                   recv_node.data_ptr(),  // rcv buffer
                                   static_cast<int>(snd_size), // data len
                                   MPI_BYTE,  // rcv chars
                                   mpi_comm); // mpi com

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    return mpi_error;
}



//---------------------------------------------------------------------------//
int
gather_using_schema(Node &send_node,
                    Node &recv_node,
                    int root,
                    MPI_Comm mpi_comm)
{
    Node n_snd_compact;
    send_node.compact_to(n_snd_compact);

    int m_size = mpi::size(mpi_comm);
    int m_rank = mpi::rank(mpi_comm);

    std::string schema_str = n_snd_compact.schema().to_json();

    int schema_len = static_cast<int>(schema_str.length() + 1);
    int data_len   = static_cast<int>(n_snd_compact.total_bytes_compact());

    // to do the conduit gatherv, first need a gather to get the
    // schema and data buffer sizes

    int snd_sizes[] = {schema_len, data_len};

    Node n_rcv_sizes;

    if( m_rank == root )
    {
        Schema s;
        s["schema_len"].set(DataType::c_int());
        s["data_len"].set(DataType::c_int());
        n_rcv_sizes.list_of(s,m_size);
    }

    int mpi_error = MPI_Gather( snd_sizes, // local data
                                2, // two ints per rank
                                MPI_INT, // send ints
                                n_rcv_sizes.data_ptr(),  // rcv buffer
                                2,  // two ints per rank
                                MPI_INT,  // rcv ints
                                root,  // id of root for gather op
                                mpi_comm); // mpi com

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    Node n_rcv_tmp;

    int  *schema_rcv_counts = NULL;
    int  *schema_rcv_displs = NULL;
    char *schema_rcv_buff   = NULL;

    int  *data_rcv_counts = NULL;
    int  *data_rcv_displs = NULL;
    char *data_rcv_buff   = NULL;

    // we only need rcv params on the gather root
    if( m_rank == root )
    {
        // alloc data for the mpi gather counts and displ arrays
        n_rcv_tmp["schemas/counts"].set(DataType::c_int(m_size));
        n_rcv_tmp["schemas/displs"].set(DataType::c_int(m_size));

        n_rcv_tmp["data/counts"].set(DataType::c_int(m_size));
        n_rcv_tmp["data/displs"].set(DataType::c_int(m_size));

        // get pointers to counts and displs
        schema_rcv_counts = n_rcv_tmp["schemas/counts"].value();
        schema_rcv_displs = n_rcv_tmp["schemas/displs"].value();

        data_rcv_counts = n_rcv_tmp["data/counts"].value();
        data_rcv_displs = n_rcv_tmp["data/displs"].value();

        int schema_curr_displ = 0;
        int data_curr_displ   = 0;
        int i=0;

        NodeIterator itr = n_rcv_sizes.children();
        while(itr.has_next())
        {
            Node &curr = itr.next();

            int schema_curr_count = curr["schema_len"].value();
            int data_curr_count   = curr["data_len"].value();

            schema_rcv_counts[i] = schema_curr_count;
            schema_rcv_displs[i] = schema_curr_displ;
            schema_curr_displ   += schema_curr_count;

            data_rcv_counts[i] = data_curr_count;
            data_rcv_displs[i] = data_curr_displ;
            data_curr_displ   += data_curr_count;

            i++;
        }

        n_rcv_tmp["schemas/data"].set(DataType::c_char(schema_curr_displ));
        schema_rcv_buff = n_rcv_tmp["schemas/data"].value();
    }

    mpi_error = MPI_Gatherv( const_cast <char*>(schema_str.c_str()),
                             schema_len,
                             MPI_BYTE,
                             schema_rcv_buff,
                             schema_rcv_counts,
                             schema_rcv_displs,
                             MPI_BYTE,
                             root,
                             mpi_comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    // build all schemas from JSON, compact them.
    Schema rcv_schema;
    if( m_rank == root )
    {
        //TODO: should we make it easer to create a compact schema?
        // TODO: Revisit, I think we can do this better

        Schema s_tmp;
        for(int i=0;i < m_size; i++)
        {
            Schema &s = s_tmp.append();
            s.set(&schema_rcv_buff[schema_rcv_displs[i]]);
        }

        s_tmp.compact_to(rcv_schema);
    }


    if( m_rank == root )
    {
        // allocate data to hold the gather result
        // TODO can we support copy out w/out realloc
        recv_node.set(rcv_schema);
        data_rcv_buff = (char*)recv_node.data_ptr();
    }

    mpi_error = MPI_Gatherv( n_snd_compact.data_ptr(),
                             data_len,
                             MPI_BYTE,
                             data_rcv_buff,
                             data_rcv_counts,
                             data_rcv_displs,
                             MPI_BYTE,
                             root,
                             mpi_comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
all_gather_using_schema(Node &send_node,
                        Node &recv_node,
                        MPI_Comm mpi_comm)
{
    Node n_snd_compact;
    send_node.compact_to(n_snd_compact);

    int m_size = mpi::size(mpi_comm);

    std::string schema_str = n_snd_compact.schema().to_json();

    int schema_len = static_cast<int>(schema_str.length() + 1);
    int data_len   = static_cast<int>(n_snd_compact.total_bytes_compact());

    // to do the conduit gatherv, first need a gather to get the
    // schema and data buffer sizes

    int snd_sizes[] = {schema_len, data_len};

    Node n_rcv_sizes;

    Schema s;
    s["schema_len"].set(DataType::c_int());
    s["data_len"].set(DataType::c_int());
    n_rcv_sizes.list_of(s,m_size);

    int mpi_error = MPI_Allgather( snd_sizes, // local data
                                   2, // two ints per rank
                                   MPI_INT, // send ints
                                   n_rcv_sizes.data_ptr(),  // rcv buffer
                                   2,  // two ints per rank
                                   MPI_INT,  // rcv ints
                                   mpi_comm); // mpi com

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    Node n_rcv_tmp;

    int  *schema_rcv_counts = NULL;
    int  *schema_rcv_displs = NULL;
    char *schema_rcv_buff   = NULL;

    int  *data_rcv_counts = NULL;
    int  *data_rcv_displs = NULL;
    char *data_rcv_buff   = NULL;


    // alloc data for the mpi gather counts and displ arrays
    n_rcv_tmp["schemas/counts"].set(DataType::c_int(m_size));
    n_rcv_tmp["schemas/displs"].set(DataType::c_int(m_size));

    n_rcv_tmp["data/counts"].set(DataType::c_int(m_size));
    n_rcv_tmp["data/displs"].set(DataType::c_int(m_size));

    // get pointers to counts and displs
    schema_rcv_counts = n_rcv_tmp["schemas/counts"].value();
    schema_rcv_displs = n_rcv_tmp["schemas/displs"].value();

    data_rcv_counts = n_rcv_tmp["data/counts"].value();
    data_rcv_displs = n_rcv_tmp["data/displs"].value();

    int schema_curr_displ = 0;
    int data_curr_displ   = 0;

    NodeIterator itr = n_rcv_sizes.children();

    index_t child_idx = 0;

    while(itr.has_next())
    {
        Node &curr = itr.next();

        int schema_curr_count = curr["schema_len"].value();
        int data_curr_count   = curr["data_len"].value();

        schema_rcv_counts[child_idx] = schema_curr_count;
        schema_rcv_displs[child_idx] = schema_curr_displ;
        schema_curr_displ   += schema_curr_count;

        data_rcv_counts[child_idx] = data_curr_count;
        data_rcv_displs[child_idx] = data_curr_displ;
        data_curr_displ   += data_curr_count;

        child_idx+=1;
    }

    n_rcv_tmp["schemas/data"].set(DataType::c_char(schema_curr_displ));
    schema_rcv_buff = n_rcv_tmp["schemas/data"].value();

    mpi_error = MPI_Allgatherv( const_cast <char*>(schema_str.c_str()),
                                schema_len,
                                MPI_BYTE,
                                schema_rcv_buff,
                                schema_rcv_counts,
                                schema_rcv_displs,
                                MPI_BYTE,
                                mpi_comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    // build all schemas from JSON, compact them.
    Schema rcv_schema;
    //TODO: should we make it easer to create a compact schema?
    // TODO: Revisit, I think we can do this better
    Schema s_tmp;
    for(int s_idx=0; s_idx < m_size; s_idx++)
    {
        Schema &s_new = s_tmp.append();
        s_new.set(&schema_rcv_buff[schema_rcv_displs[s_idx]]);
    }

    // TODO can we support copy out w/out realloc
    s_tmp.compact_to(rcv_schema);

    // allocate data to hold the gather result
    recv_node.set(rcv_schema);
    data_rcv_buff = (char*)recv_node.data_ptr();

    mpi_error = MPI_Allgatherv( n_snd_compact.data_ptr(),
                                data_len,
                                MPI_BYTE,
                                data_rcv_buff,
                                data_rcv_counts,
                                data_rcv_displs,
                                MPI_BYTE,
                                mpi_comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    return mpi_error;
}


//---------------------------------------------------------------------------//
int
broadcast(Node &node,
          int root,
          MPI_Comm comm)
{
    int rank = mpi::rank(comm);

    Node bcast_buffer;

    bool cpy_out = false;

    void    *bcast_data_ptr  = node.contiguous_data_ptr();
    index_t  bcast_data_size = node.total_bytes_compact();

    // setup buffers on root for send
    if(rank == root)
    {
        if( bcast_data_ptr == NULL ||
            ! node.is_compact() )
        {
            node.compact_to(bcast_buffer);
            bcast_data_ptr  = bcast_buffer.data_ptr();
        }

    }
    else // rank != root,  setup buffers on non root for rcv
    {
        if( bcast_data_ptr == NULL ||
            ! node.is_compact() )
        {
            Schema s_compact;
            node.schema().compact_to(s_compact);
            bcast_buffer.set_schema(s_compact);

            bcast_data_ptr  = bcast_buffer.data_ptr();
            cpy_out = true;
        }
    }


    if(!conduit::utils::value_fits<index_t,int>(bcast_data_size))
    {
        CONDUIT_INFO("Warning size value (" << bcast_data_size << ")"
                     " exceeds the size of MPI_Bcast max value "
                     "(" << std::numeric_limits<int>::max() << ")")
    }


    int mpi_error = MPI_Bcast(bcast_data_ptr,
                              static_cast<int>(bcast_data_size),
                              MPI_BYTE,
                              root,
                              comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    // note: cpy_out will always be false when rank == root
    if( cpy_out )
    {
        node.update(bcast_buffer);
    }

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
broadcast_using_schema(Node &node,
                       int root,
                       MPI_Comm comm)
{
    int rank = mpi::rank(comm);

    Node bcast_buffers;

    void *bcast_data_ptr = NULL;
    int   bcast_data_size = 0;

    int bcast_schema_size = 0;
    int rcv_bcast_schema_size = 0;

    // setup buffers for send
    if(rank == root)
    {

        bcast_data_ptr  = node.contiguous_data_ptr();
        bcast_data_size = static_cast<int>(node.total_bytes_compact());

        if(bcast_data_ptr != NULL &&
           node.is_compact() &&
           node.is_contiguous())
        {
            bcast_buffers["schema"] = node.schema().to_json();
        }
        else
        {
            Node &bcast_data_compact = bcast_buffers["data"];
            node.compact_to(bcast_data_compact);

            bcast_data_ptr  = bcast_data_compact.data_ptr();
            bcast_buffers["schema"] =  bcast_data_compact.schema().to_json();
        }



        bcast_schema_size = static_cast<int>(bcast_buffers["schema"].dtype().number_of_elements());
    }

    int mpi_error = MPI_Allreduce(&bcast_schema_size,
                                  &rcv_bcast_schema_size,
                                  1,
                                  MPI_INT,
                                  MPI_MAX,
                                  comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    bcast_schema_size = rcv_bcast_schema_size;

    // alloc for rcv for schema
    if(rank != root)
    {
        bcast_buffers["schema"].set(DataType::char8_str(bcast_schema_size));
    }

    // broadcast the schema
    mpi_error = MPI_Bcast(bcast_buffers["schema"].data_ptr(),
                          bcast_schema_size,
                          MPI_CHAR,
                          root,
                          comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    bool cpy_out = false;

    // setup buffers for receive
    if(rank != root)
    {
        Schema bcast_schema;
        Generator gen(bcast_buffers["schema"].as_char8_str());
        gen.walk(bcast_schema);

        // only check compat for leaves
        // there are more zero copy cases possible here, but
        // we need a better way to identify them
        // compatible won't work for object cases that
        // have different named leaves
        if( !(node.dtype().is_empty() ||
              node.dtype().is_object() ||
              node.dtype().is_list() ) &&
            !(bcast_schema.dtype().is_empty() ||
              bcast_schema.dtype().is_object() ||
              bcast_schema.dtype().is_list() )
            // make sure `node` can hold data described by `bcast_schema`
            && node.schema().compatible(bcast_schema))
        {

            bcast_data_ptr  = node.contiguous_data_ptr();
            bcast_data_size = static_cast<int>(node.total_bytes_compact());

            if( bcast_data_ptr == NULL ||
                ! node.is_compact() )
            {
                Node &bcast_data_buffer = bcast_buffers["data"];
                bcast_data_buffer.set_schema(bcast_schema);

                bcast_data_ptr  = bcast_data_buffer.data_ptr();
                cpy_out = true;
            }
        }
        else
        {
            node.set_schema(bcast_schema);

            bcast_data_ptr  = node.data_ptr();
            bcast_data_size = static_cast<int>(node.total_bytes_compact());
        }
    }

    mpi_error = MPI_Bcast(bcast_data_ptr,
                          bcast_data_size,
                          MPI_BYTE,
                          root,
                          comm);

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    // note: cpy_out will always be false when rank == root
    if( cpy_out )
    {
        node.update(bcast_buffers["data"]);
    }

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
memory_usage(Node &info,
             MPI_Comm comm)
{
    info.reset();
    int com_size = mpi::size(comm);

    uint64 musage =conduit::utils::memory_usage();

    void *musage_ptr = static_cast<void*>(&musage);

    info.set(DataType::uint64(com_size));

    // we want an array answer, not a list
    // so we use mpi directly vs conduit all gather helper
    MPI_Datatype mpi_dtype = conduit_dtype_to_mpi_dtype(info.dtype());

    int mpi_error = MPI_Allgather( musage_ptr, // local data
                                   1, // local data len
                                   mpi_dtype, // send uint64
                                   info.data_ptr(),  // rcv buffer
                                   1, // data len
                                   mpi_dtype,  // rcv uint64
                                   comm); // mpi com

    CONDUIT_CHECK_MPI_ERROR(mpi_error);

    return mpi_error;
}

//---------------------------------------------------------------------------//
int
memory_stats(Node &info,
             MPI_Comm comm)
{
    info.reset();

    int com_size = mpi::size(comm);

    Node snd,rcv;
    snd = conduit::utils::memory_usage();

    int mpi_error = mpi::all_reduce(snd, rcv, MPI_MIN, comm);
    CONDUIT_CHECK_MPI_ERROR(mpi_error);
    info["min"] = rcv;

    mpi_error = mpi::all_reduce(snd, rcv, MPI_MAX, comm);
    CONDUIT_CHECK_MPI_ERROR(mpi_error);
    info["max"] = rcv;

    mpi_error = mpi::all_reduce(snd, rcv, MPI_SUM, comm);
    CONDUIT_CHECK_MPI_ERROR(mpi_error);
    info["sum"] = rcv.to_uint64();
    info["mean"] = (rcv.to_uint64() / static_cast<uint64>(com_size));

    return mpi_error;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const int communicate_using_schema::OP_SEND = 1;
const int communicate_using_schema::OP_RECV = 2;

//-----------------------------------------------------------------------------
communicate_using_schema::communicate_using_schema(MPI_Comm c) :
    comm(c), operations(), loggingRoot("communicate_using_schema"), logging(false)
{
}

//-----------------------------------------------------------------------------
communicate_using_schema::~communicate_using_schema()
{
    clear();
}

//-----------------------------------------------------------------------------
void
communicate_using_schema::clear()
{
    for(size_t i = 0; i < operations.size(); i++)
    {
        if(operations[i].free[0])
            delete operations[i].node[0];
        if(operations[i].free[1])
            delete operations[i].node[1];
    }
    operations.clear();
}

//-----------------------------------------------------------------------------
void
communicate_using_schema::set_logging(bool val)
{
    logging = val;
}

//-----------------------------------------------------------------------------
void
communicate_using_schema::set_logging_root(const std::string &filename)
{
    loggingRoot = filename;
}

//-----------------------------------------------------------------------------
void
communicate_using_schema::add_isend(const Node &node, int dest, int tag)
{
    if(invalid_tag(tag))
    {
       CONDUIT_ERROR("add_isend given invalid tag (" << tag << ").");
    }
    // Note: any tags larger than MPI_TAG_UB will be handled in execute.

    // Append the work to the operations.
    operation work;
    work.op = OP_SEND;
    work.rank = dest;
    work.tag = tag;
    work.node[0] = const_cast<Node *>(&node); // The node we're sending.
    work.free[0] = false;
    work.node[1] = nullptr;
    work.free[1] = false;
    operations.push_back(work);
}

//-----------------------------------------------------------------------------
void
communicate_using_schema::add_irecv(Node &node, int src, int tag)
{
    if(invalid_tag(tag))
    {
       CONDUIT_ERROR("add_irecv given invalid tag (" << tag << ").");
    }
    // Note: any tags larger than MPI_TAG_UB will be handled in execute.

    // Append the work to the operations.
    operation work;
    work.op = OP_RECV;
    work.rank = src;
    work.tag = tag;
    work.node[0] = &node; // Node that will contain final data.
    work.free[0] = false; // Don't need to free it.
    work.node[1] = nullptr;
    work.free[1] = false;
    operations.push_back(work);
}

//-----------------------------------------------------------------------------
int
communicate_using_schema::execute()
{
    // Uncomment this to install an MPI_Comm error handler.
    // HandleMPICommError errHandler(comm);

    // Get the currently installed warning and error handlers.
    conduit::utils::conduit_warning_handler onWarning = conduit::utils::warning_handler();
    conduit::utils::conduit_error_handler onError = conduit::utils::error_handler();

    // Install the default exception-throwing handlers.
    conduit::utils::set_warning_handler(conduit::utils::default_warning_handler);
    conduit::utils::set_error_handler(conduit::utils::default_error_handler);

    int retval = 0;
    try
    {
        retval = execute_internal();

        // Restore warning/error handlers.
        conduit::utils::set_warning_handler(onWarning);
        conduit::utils::set_error_handler(onError);
    }
    catch(conduit::Error &e)
    {
        // Restore warning/error handlers.
        conduit::utils::set_warning_handler(onWarning);
        conduit::utils::set_error_handler(onError);

        // Rethrow the exception.
        throw e;
    }
    return retval;
}

//-----------------------------------------------------------------------------
int
communicate_using_schema::execute_internal()
{
    int mpi_error = 0;
    std::vector<MPI_Request> requests(operations.size());
    std::vector<MPI_Status>  statuses(operations.size());

    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    std::ofstream log;
    double t0 = MPI_Wtime();
    if(logging)
    {
        char fn[128];
        snprintf(fn, sizeof(fn), ".%04d.log", rank);
        std::string filename(loggingRoot + fn);
        log.open(filename.c_str(), std::ofstream::out);
        log << "* Log started on rank " << rank << " at " << t0 << std::endl;
    }

    // Issue all the sends (so they are in flight by the time we probe them)
    for(size_t i = 0; i < operations.size(); i++)
    {
        if(operations[i].op == OP_SEND)
        {
            Schema s_data_compact;
            const Node &node = *operations[i].node[0];
            // schema will only be valid if compact and contig
            if( node.is_compact() && node.is_contiguous())
            {
                s_data_compact = node.schema();
            }
            else
            {
                node.schema().compact_to(s_data_compact);
            }

            std::string snd_schema_json = s_data_compact.to_json();

            Schema s_msg;
            s_msg["schema_len"].set(DataType::int64());
            s_msg["schema"].set(DataType::char8_str(snd_schema_json.size()+1));
            s_msg["data"].set(s_data_compact);

            // create a compact schema to use
            Schema s_msg_compact;
            s_msg.compact_to(s_msg_compact);

            operations[i].node[1] = new Node(s_msg_compact);
            operations[i].free[1] = true;
            Node &n_msg = *operations[i].node[1];
            // these sets won't realloc since schemas are compatible
            n_msg["schema_len"].set((int64)snd_schema_json.length());
            n_msg["schema"].set(snd_schema_json);
            n_msg["data"].update(node);

            // Send the serialized node data.
            index_t msg_data_size = operations[i].node[1]->total_bytes_compact();
            const int newtag = safe_tag(operations[i].tag, comm);
            if(logging)
            {
                log << "    MPI_Isend("
                    << const_cast<void*>(operations[i].node[1]->data_ptr()) << ", "
                    << msg_data_size << ", "
                    << "MPI_BYTE, "
                    << operations[i].rank << ", "
                    << newtag << ", "
                    << "comm, &requests[" << i << "]);" << std::endl;
            }

            if(!conduit::utils::value_fits<index_t,int>(msg_data_size))
            {
                CONDUIT_INFO("Warning size value (" << msg_data_size << ")"
                             " exceeds the size of MPI_Isend max value "
                             "(" << std::numeric_limits<int>::max() << ")")
            }

            mpi_error = MPI_Isend(const_cast<void*>(operations[i].node[1]->data_ptr()),
                                  static_cast<int>(msg_data_size),
                                  MPI_BYTE,
                                  operations[i].rank,
                                  newtag,
                                  comm,
                                  &requests[i]);
            CONDUIT_CHECK_MPI_ERROR(mpi_error);
        }
    }
    double t1 = MPI_Wtime();
    if(logging)
    {
        log << "* Time issuing MPI_Isend calls: " << (t1-t0) << std::endl;
        log.flush();
    }

    // Issue all the recvs.
    for(size_t i = 0; i < operations.size(); i++)
    {
        if(operations[i].op == OP_RECV)
        {
            // Probe the message for its buffer size.
            const int newtag = safe_tag(operations[i].tag, comm);
            if(logging)
            {
                log << "    MPI_Probe("
                    << operations[i].rank << ", "
                    << newtag << ", "
                    << "comm, &statuses[" << i << "]);" << std::endl;
            }
            mpi_error = MPI_Probe(operations[i].rank, newtag, comm, &statuses[i]);
            CONDUIT_CHECK_MPI_ERROR(mpi_error);

            int buffer_size = 0;
            MPI_Get_count(&statuses[i], MPI_BYTE, &buffer_size);
            if(logging)
            {
                log << "    MPI_Get_count(&statuses[" << i << "], MPI_BYTE, &buffer_size); -> "
                    << buffer_size << std::endl;
            }

            // Allocate a node into which we'll receive the raw data.
            operations[i].node[1] = new Node(DataType::uint8(buffer_size));
            operations[i].free[1] = true;

            if(logging)
            {
                log << "    MPI_Irecv("
                    << operations[i].node[1]->data_ptr() << ", "
                    << buffer_size << ", "
                    << "MPI_BYTE, "
                    << operations[i].rank << ", "
                    << newtag << ", "
                    << "comm, &requests[" << i << "]);" << std::endl;
            }

            // Post the actual receive.
            mpi_error = MPI_Irecv(operations[i].node[1]->data_ptr(),
                                  buffer_size,
                                  MPI_BYTE,
                                  operations[i].rank,
                                  newtag,
                                  comm,
                                  &requests[i]);
            CONDUIT_CHECK_MPI_ERROR(mpi_error);
        }
    }
    double t2 = MPI_Wtime();
    if(logging)
    {
        log << "* Time issuing MPI_Irecv calls: " << (t2-t1) << std::endl;
        log.flush();
    }

    // Wait for the requests to complete.
    int n = static_cast<int>(operations.size());
    if(logging)
    {
        log << "    MPI_Waitall(" << n << ", &requests[0], &statuses[0]);" << std::endl;
        log.flush();
    }
    mpi_error = MPI_Waitall(n, &requests[0], &statuses[0]);
    CONDUIT_CHECK_MPI_ERROR(mpi_error);
    double t3 = MPI_Wtime();
    if(logging)
    {
        log << "* Time in MPI_Waitall: " << (t3-t2) << std::endl;
        log.flush();
    }

    // Finish building the nodes for which we received data.
    for(size_t i = 0; i < operations.size(); i++)
    {
        if(operations[i].op == OP_RECV)
        {
            // Get the buffer of the data we received.
            uint8 *n_buff_ptr = (uint8*)operations[i].node[1]->data_ptr();

            Node n_msg;
            // length of the schema is sent as a 64-bit signed int
            // NOTE: we aren't using this value  ...
            n_msg["schema_len"].set_external((int64*)n_buff_ptr);
            n_buff_ptr +=8;
            // wrap the schema string
            n_msg["schema"].set_external_char8_str((char*)(n_buff_ptr));
            // create the schema
            Schema rcv_schema;
            Generator gen(n_msg["schema"].as_char8_str());
            gen.walk(rcv_schema);

            // advance by the schema length
            n_buff_ptr += n_msg["schema"].total_bytes_compact();

            // apply the schema to the data
            n_msg["data"].set_external(rcv_schema,n_buff_ptr);

            // copy out to our result node
            operations[i].node[0]->update(n_msg["data"]);

            if(logging)
            {
                log << "* Built output node " << i << std::endl;
                log.flush();
            }
        }
    }
    double t4 = MPI_Wtime();
    if(logging)
    {
        log << "* Time building output nodes " << (t4-t3) << std::endl;
        log.close();
    }

    // Cleanup
    clear();

    return 0;
}

//---------------------------------------------------------------------------//
std::string
about()
{
    Node n;
    mpi::about(n);
    return n.to_yaml();
}

//---------------------------------------------------------------------------//
void
about(Node &n)
{
    n.reset();
    n["mpi"] = "enabled";
}

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


