// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mpi_mesh_flatten.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MPI_MESH_FLATTEN_HPP
#define CONDUIT_BLUEPRINT_MPI_MESH_FLATTEN_HPP

// Internal utility header

//-----------------------------------------------------------------------------
// std lib includes
//-----------------------------------------------------------------------------
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_mesh_flatten.hpp"
#include "conduit_blueprint_exports.h"

#include <mpi.h>

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint --
//-----------------------------------------------------------------------------
namespace blueprint
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mpi --
//-----------------------------------------------------------------------------
namespace mpi
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mpi::mesh --
//-----------------------------------------------------------------------------
namespace mesh
{

class CONDUIT_BLUEPRINT_API ParallelMeshFlattener
    : public blueprint::mesh::MeshFlattener
{
public:
    ParallelMeshFlattener(MPI_Comm _comm);
    virtual ~ParallelMeshFlattener();

    virtual bool set_options(const Node &opts) override;
protected:
    struct FieldInfo {
        std::vector<std::string> field_names;
        std::vector<index_t> field_ncomps;
        // 0 for vertex, 1 for element
        std::vector<index_t> field_assocs;
        std::vector<index_t> field_dtypes;
        std::vector<std::string> comp_names;

        FieldInfo();
        FieldInfo(const Node &n);
        void from_node(const Node &n_field_info);
        void to_node(Node &n) const;
    };

    struct MeshMetaData {
        index_t coord_type;
        index_t dimension;
        std::vector<index_t> counts;
        std::vector<std::string> axes;
        index_t nverts;
        index_t nelems;

        MeshMetaData();
    };

    bool rank_has_data(const MeshInfo &mesh) const;

    void add_mpi_rank(const MeshInfo &my_mesh, index_t vert_offset,
        index_t elem_offset, Node &output) const;

    void build_local_field_info(const std::vector<std::string> &fields_to_flatten,
        const Node &mesh, const std::string &topo_name, Node &field_info) const;
    void calculate_unique_fields(const Node &all_rank_fields, Node &output) const;
    FieldInfo determine_global_fields(const Node &mesh) const;

    /**
    @brief On all ranks, populates out.coord_type and out.dimension.
        On root, populates the entire MeshMetaData struct
    */
    void gather_global_mesh_metadata(const MeshInfo &my_info, MeshMetaData &out) const;

    void gather_values(int nrows,
        int *rank_counts,
        int *rank_offsets,
        Node &values) const;
    void gather_results(const MeshInfo &my_info,
        const MeshMetaData &global_meta_data,
        Node &output) const;

    void make_local_allocations(const MeshInfo &my_info,
        const FieldInfo &global_field_info,
        Node &output) const;

    void make_root_allocations(const MeshMetaData &global_meta_data,
        const FieldInfo &global_field_info,
        const MeshInfo &my_info,
        Node &output) const;

    virtual void cleanup_output(Node &output) const override;

    virtual void flatten_many_domains(const Node &mesh,
                                      Node &output) const override;

    MPI_Comm comm;
    int root;
    int rank;
    bool add_rank;
};

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi::mesh --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit --
//-----------------------------------------------------------------------------

#endif
