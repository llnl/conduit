// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_mpi.hpp
///
//-----------------------------------------------------------------------------


#ifndef CONDUIT_RELAY_MPI_HPP
#define CONDUIT_RELAY_MPI_HPP

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include <mpi.h>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_relay_exports.h"

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

    struct Request
    {
        MPI_Request  m_request;
        Node         m_buffer;
        Node        *m_rcv_ptr;
    };


//-----------------------------------------------------------------------------
/// Helpers for MPI Params
//-----------------------------------------------------------------------------
    int CONDUIT_RELAY_API size(MPI_Comm mpi_comm);

    int CONDUIT_RELAY_API rank(MPI_Comm mpi_comm);

//-----------------------------------------------------------------------------
/// Helpers for converting between MPI data types  and conduit data types
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
/// Helpers for converting between mpi dtypes and conduit dtypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
    MPI_Datatype CONDUIT_RELAY_API conduit_dtype_to_mpi_dtype(
                                                        const DataType &dtype);

//-----------------------------------------------------------------------------
    index_t CONDUIT_RELAY_API mpi_dtype_to_conduit_dtype_id(MPI_Datatype dt);


//-----------------------------------------------------------------------------
/// Standard MPI Send Recv
//-----------------------------------------------------------------------------

    /**
      @brief MPI tags can be in the range [0,MPI_TAG_UB]. The values are
             implementation-dependent. If the tag is not in that range, return
             MPI_TAG_UB so it is safe to use with MPI functions.

      @param tag The input tag.
      @param comm The MPI communicator.

      @return A tag value that is safe to use with MPI.
    */
    int CONDUIT_RELAY_API safe_tag(int tag, MPI_Comm comm);

    int CONDUIT_RELAY_API send(const Node &node,
                                int dest,
                                int tag,
                                MPI_Comm comm);

    int CONDUIT_RELAY_API recv(Node &node,
                               int source,
                               int tag,
                               MPI_Comm comm);

    // any source, any tag variant
    int CONDUIT_RELAY_API recv(Node &node,
                               MPI_Comm comm);


    int CONDUIT_RELAY_API send_using_schema(const Node &node,
                                            int dest,
                                            int tag,
                                            MPI_Comm comm);

    int CONDUIT_RELAY_API recv_using_schema(Node &node,
                                            int source,
                                            int tag,
                                            MPI_Comm comm);

    // any source, any tag variant
    int CONDUIT_RELAY_API recv_using_schema(Node &node,
                                            MPI_Comm comm);


//-----------------------------------------------------------------------------
/// MPI Reduce
//-----------------------------------------------------------------------------

    /// MPI reduce and all reduce methods.

    /// While the input does not need to be compact,
    /// reductions require all MPI ranks have identical compact representations.

    /// These methods do not check across ranks for identical compact
    /// representation.

    /// Conduit empty, object, and list dtypes can not be reduced.

    /// If the send_node is not compact, it will be compacted prior to sending.

    /// for reduce on the root rank and all_reduce for all ranks:
    ///   if the recv_node is compatible but not compact, data will be placed
    ///   into a compact buffer, then read back out into the recv_node node.
    ///
    ///   if the recv_node is not compatible, it will be reset to
    ///   a compact compatible type.

    int CONDUIT_RELAY_API reduce(const Node &send_node,
                                 Node &recv_node,
                                 MPI_Op mpi_op,
                                 int root,
                                 MPI_Comm comm);

    int CONDUIT_RELAY_API all_reduce(const Node &send_node,
                                     Node &recv_node,
                                     MPI_Op mpi_op,
                                     MPI_Comm comm);


//-----------------------------------------------------------------------------
/// MPI Reduce Helpers
//-----------------------------------------------------------------------------

    int CONDUIT_RELAY_API sum_reduce(const Node &send_node,
                                     Node &recv_node,
                                     int root,
                                     MPI_Comm comm);


    int CONDUIT_RELAY_API min_reduce(const Node &send_node,
                                     Node &recv_node,
                                     int root,
                                     MPI_Comm comm);

    int CONDUIT_RELAY_API max_reduce(const Node &send_node,
                                     Node &recv_node,
                                     int root,
                                     MPI_Comm comm);

    int CONDUIT_RELAY_API prod_reduce(const Node &send_node,
                                      Node &recv_node,
                                      int root,
                                      MPI_Comm comm);



    int CONDUIT_RELAY_API sum_all_reduce(const Node &send_node,
                                         Node &recv_node,
                                         MPI_Comm comm);


    int CONDUIT_RELAY_API min_all_reduce(const Node &send_node,
                                         Node &recv_node,
                                         MPI_Comm comm);

    int CONDUIT_RELAY_API max_all_reduce(const Node &send_node,
                                         Node &recv_node,
                                         MPI_Comm comm);

    int CONDUIT_RELAY_API prod_all_reduce(const Node &send_node,
                                          Node &recv_node,
                                          MPI_Comm comm);


//-----------------------------------------------------------------------------
/// Async MPI Send Recv
//-----------------------------------------------------------------------------

    int CONDUIT_RELAY_API isend(const Node &node,
                                int dest,
                                int tag,
                                MPI_Comm mpi_comm,
                                Request *request);

    int CONDUIT_RELAY_API irecv(Node &node,
                                int src,
                                int tag,
                                MPI_Comm comm,
                                Request *request);

    // any source, any tag variant
    int CONDUIT_RELAY_API irecv(Node &node,
                                MPI_Comm comm,
                                Request *request);

    // wait for either an isend or irecv request
    int CONDUIT_RELAY_API wait(Request *request,
                               MPI_Status *status);

    int CONDUIT_RELAY_API wait_send(Request *request,
                                    MPI_Status *status);

    int CONDUIT_RELAY_API wait_recv(Request *request,
                                    MPI_Status *status);

    // wait for batch of isend and/or irecv requests
    int CONDUIT_RELAY_API wait_all(int count,
                                   Request requests[],
                                   MPI_Status statuses[]);

    int CONDUIT_RELAY_API wait_all_send(int count,
                                        Request requests[],
                                        MPI_Status statuses[]);

    int CONDUIT_RELAY_API wait_all_recv(int count,
                                        Request requests[],
                                        MPI_Status statuses[]);


//-----------------------------------------------------------------------------
/// MPI gather
//-----------------------------------------------------------------------------

    // these expect identical schemas
    int CONDUIT_RELAY_API gather(Node &send_node,
                                 Node &recv_node,
                                 int root,
                                 MPI_Comm mpi_comm);

    int CONDUIT_RELAY_API all_gather(Node &send_node,
                                     Node &recv_node,
                                     MPI_Comm mpi_comm);


    int CONDUIT_RELAY_API gather_using_schema(Node &send_node,
                                              Node &recv_node,
                                              int root,
                                              MPI_Comm mpi_comm);

    int CONDUIT_RELAY_API all_gather_using_schema(Node &send_node,
                                                  Node &recv_node,
                                                  MPI_Comm mpi_comm);

//-----------------------------------------------------------------------------
/// MPI broadcast
//-----------------------------------------------------------------------------

    int CONDUIT_RELAY_API broadcast(Node &node,
                                    int root,
                                    MPI_Comm comm );

    int CONDUIT_RELAY_API broadcast_using_schema(Node &node,
                                                 int root,
                                                 MPI_Comm comm );

//-----------------------------------------------------------------------------
/// Memory usage helpers (results in kB)
//-----------------------------------------------------------------------------
    /// memory usage per mpi task in kB
    int CONDUIT_RELAY_API memory_usage(Node &info, MPI_Comm comm);
    ///  min, max, mean memory usage across mpi tasks
    int CONDUIT_RELAY_API memory_stats(Node &info, MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Communicate multiple nodes at once using schema
//-----------------------------------------------------------------------------
/**
 @brief This class sends or receives nodes using non-blocking MPI communication
        and it handles the schema serialization for multiple requests, etc.
        This helps us issue all the calls at once and then wait for them to
        complete. This helps avoid extra bookkeeping in the caller and
        potential for deadlock.
 */
class CONDUIT_RELAY_API communicate_using_schema
{
public:
    communicate_using_schema(MPI_Comm c);
    ~communicate_using_schema();

    /**
     @brief Set whether log files of the MPI calls are created.
     @param val If true, the execute() method will create log files of the MPI
                calls for each rank.
     */
    void set_logging(bool val);

    /**
     @brief Set the name used for creating the log files.
     @param filename The filename that is used to create the actual log files.
     */
    void set_logging_root(const std::string &filename);

    /**
     @brief Schedule the node for movement to another rank.
     @param node The node to move to another rank.
     @param dest The rank to which the node will be moved.
     @param tag The message tag to use for the node. This must match a tag
                used in a corresponding add_irecv on another rank.
     @note The node needs to remain valid until after execute() is called.
     */
    void add_isend(const Node &node, int dest, int tag);

    /**
     @brief Receive a node from another rank into the provided node.
     @param node The node to receive the data.
     @param src The rank that sends data to this rank.
     @param tag The message tag to use for the node. This must match a tag
                used in a corresponding add_isend on another rank.
     @note The node needs to remain valid until after execute() is called.
     */
    void add_irecv(Node &node, int src, int tag);

    /**
     @brief Execute all the outstanding isend/irecv calls and reconstruct any
            nodes after doing the data movement.
     @return The return value from MPI_Waitall.
     */
    int  execute();
private:
    void clear();

    /**
     @brief Execute all the outstanding isend/irecv calls and reconstruct any
            nodes after doing the data movement.
     @return The return value from MPI_Waitall.
     */
    int  execute_internal();

    static const int OP_SEND;
    static const int OP_RECV;
    struct operation
    {
        int   op;
        int   rank;
        int   tag;
        Node *node[2];
        bool  free[2];
    };

    MPI_Comm comm;
    std::vector<operation> operations;
    std::string loggingRoot;
    bool logging;
};

//-----------------------------------------------------------------------------
/// The about methods construct human readable info about how conduit_mpi was
/// configured.
//-----------------------------------------------------------------------------
 std::string CONDUIT_RELAY_API about();
 void        CONDUIT_RELAY_API about(Node &);

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


#endif

