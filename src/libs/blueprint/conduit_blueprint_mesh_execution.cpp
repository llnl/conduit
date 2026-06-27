// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_execution.cpp
///
/// Coordset conversion functions that use conduit::execution::forall and are
/// compiled as a device translation unit (CUDA/HIP) when device support is
/// enabled.  Functions that are not yet ported to device live in
/// conduit_blueprint_mesh.cpp instead.
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_execution.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_annotations.hpp"

using namespace conduit;
namespace bputils = conduit::blueprint::mesh::utils;

//-----------------------------------------------------------------------------
// -- begin internal helpers --
//-----------------------------------------------------------------------------

namespace
{

//-----------------------------------------------------------------------------
void
convert_coordset_to_rectilinear(const std::string &/*base_type*/,
                                const conduit::Node &coordset,
                                conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    dest.reset();
    dest["type"].set("rectilinear");

    DataType float_dtype = bputils::find_widest_dtype(coordset, bputils::DEFAULT_FLOAT_DTYPE);

    const std::vector<std::string> csys_axes = bputils::coordset::axes(coordset);
    const std::vector<std::string> &logical_axes = bputils::LOGICAL_AXES;

    // execution setup
    conduit::execution::ExecutionPolicy policy = conduit::execution::get_execution_policy();
    const index_t allocator_id = conduit::execution::get_output_allocator_id();
    const std::string &sync_strategy = conduit::execution::get_sync_strategy();

    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        const std::string& csys_axis = csys_axes[i];
        const std::string& logical_axis = logical_axes[i];

        float64 dim_origin = coordset.has_child("origin") ?
            coordset["origin"][csys_axis].to_float64() : 0.0;
        float64 dim_spacing = coordset.has_child("spacing") ?
            coordset["spacing"]["d"+csys_axis].to_float64() : 1.0;
        index_t dim_len = coordset["dims"][logical_axis].to_int64();

        Node &dst_cvals_node = dest["values"][csys_axis];
        dst_cvals_node.set_allocator(allocator_id);
        dst_cvals_node.set(DataType(float_dtype.id(), dim_len));

        float64_accessor dst_values(dest["values"][csys_axis]);
        dst_values.use_with(policy);
        conduit::execution::forall(policy, 0, dim_len, [=] CONDUIT_EXEC(index_t d)
        {
            const float64 val = dim_origin + d * dim_spacing;
            dst_values.set(d, val);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        dst_values.data_movement(sync_strategy);
    }
}

//-----------------------------------------------------------------------------
void
convert_coordset_to_explicit(const std::string &base_type,
                             const conduit::Node &coordset,
                             conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    bool is_base_rectilinear = base_type == "rectilinear";
    bool is_base_uniform = base_type == "uniform";

    dest.reset();
    dest["type"].set("explicit");

    DataType float_dtype = bputils::find_widest_dtype(coordset, bputils::DEFAULT_FLOAT_DTYPE);

    const std::vector<std::string> csys_axes = bputils::coordset::axes(coordset);
    const std::vector<std::string> &logical_axes = bputils::LOGICAL_AXES;

    index_t dim_lens[3] = {0, 0, 0}, coords_len = 1;
    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        dim_lens[i] = is_base_rectilinear ?
            coordset["values"][csys_axes[i]].dtype().number_of_elements() :
            coordset["dims"][logical_axes[i]].to_int64();
        coords_len *= dim_lens[i];
    }

    Node info;
    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        const std::string& csys_axis = csys_axes[i];

        // NOTE: The following values are specific to the
        // rectilinear transform case.
        const Node &src_cvals_node = coordset.has_child("values") ?
            coordset["values"][csys_axis] : info;
        float64_accessor src_cvals_acc(src_cvals_node);
        // NOTE: The following values are specific to the
        // uniform transform case.
        float64 dim_origin = coordset.has_child("origin") ?
            coordset["origin"][csys_axis].to_float64() : 0.0;
        float64 dim_spacing = coordset.has_child("spacing") ?
            coordset["spacing"]["d"+csys_axis].to_float64() : 1.0;

        index_t dim_block_size = 1, dim_block_count = 1;
        for(index_t j = 0; j < (index_t)csys_axes.size(); j++)
        {
            dim_block_size *= (j < i) ? dim_lens[j] : 1;
            dim_block_count *= (i < j) ? dim_lens[j] : 1;
        }

        // execution setup
        conduit::execution::ExecutionPolicy policy = is_base_rectilinear ?
            conduit::execution::get_execution_policy(src_cvals_node) :
            conduit::execution::get_execution_policy();
        const index_t allocator_id = is_base_rectilinear ?
            conduit::execution::get_output_allocator_id(src_cvals_node) :
            conduit::execution::get_output_allocator_id();
        const std::string &sync_strategy = conduit::execution::get_sync_strategy();

        Node &dst_cvals_node = dest["values"][csys_axis];
        dst_cvals_node.set_allocator(allocator_id);
        dst_cvals_node.set(DataType(float_dtype.id(), coords_len));

        float64_accessor dst_cvals_acc(dst_cvals_node);
        dst_cvals_acc.use_with(policy);
        if (is_base_rectilinear)
        {
            src_cvals_acc.use_with(policy);
        }

        conduit::execution::forall(policy, 0, dim_lens[i], [=] CONDUIT_EXEC(index_t d)
        {
            index_t doffset = d * dim_block_size;
            for(index_t b = 0; b < dim_block_count; b++)
            {
                index_t boffset = b * dim_block_size * dim_lens[i];
                for(index_t bi = 0; bi < dim_block_size; bi++)
                {
                    index_t ioffset = doffset + boffset + bi;
                    if(is_base_rectilinear)
                    {
                        dst_cvals_acc.set(ioffset, src_cvals_acc[d]);
                    }
                    else if(is_base_uniform)
                    {
                        dst_cvals_acc.set(ioffset, dim_origin + d * dim_spacing);
                    }
                }
            }
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        dst_cvals_acc.data_movement(sync_strategy);
    }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// -- end internal helpers --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint public API --
//-----------------------------------------------------------------------------
namespace conduit
{
namespace blueprint
{

//-----------------------------------------------------------------------------
void
mesh::coordset::to_explicit(const conduit::Node& coordset,
                            conduit::Node& coordset_dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    std::string type = coordset.fetch_existing("type").as_string();

    if(type == "uniform")
        mesh::coordset::uniform::to_explicit(coordset, coordset_dest);
    else if(type == "rectilinear")
        mesh::coordset::rectilinear::to_explicit(coordset, coordset_dest);
    else if(type == "explicit")
        coordset_dest.set_external(coordset);
}

//-----------------------------------------------------------------------------
void
mesh::coordset::uniform::to_rectilinear(const conduit::Node &coordset,
                                        conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    convert_coordset_to_rectilinear("uniform", coordset, dest);
}

//-----------------------------------------------------------------------------
void
mesh::coordset::uniform::to_explicit(const conduit::Node &coordset,
                                     conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    convert_coordset_to_explicit("uniform", coordset, dest);
}

//-----------------------------------------------------------------------------
void
mesh::coordset::rectilinear::to_explicit(const conduit::Node &coordset,
                                         conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    convert_coordset_to_explicit("rectilinear", coordset, dest);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
