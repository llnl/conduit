// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_utils.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// std lib includes
#include <algorithm>
#include <cmath>
#include <deque>
#include <string>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <vector>
#include <unordered_map>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_fmt/conduit_fmt.h"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_o2mrelation.hpp"
#include "conduit_blueprint_o2mrelation_iterator.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_mesh_utils_iterate_elements.hpp"
#include "conduit_execution.hpp"
#include "conduit_annotations.hpp"
#include "conduit_utils.hpp"
#include "conduit_blueprint_mesh_kdtree.hpp"

// access one-to-many index types
namespace o2mrelation = conduit::blueprint::o2mrelation;

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
// -- begin conduit::blueprint::mesh --
//-----------------------------------------------------------------------------
namespace mesh
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils --
//-----------------------------------------------------------------------------
namespace utils
{

//-----------------------------------------------------------------------------
/// blueprint mesh utility structures
//-----------------------------------------------------------------------------

std::string ShapeType::empty_type;

//---------------------------------------------------------------------------//
ShapeType::ShapeType()
{
    init(InvalidTypeId);
}

//---------------------------------------------------------------------------//
ShapeType::ShapeType(const index_t type_id)
{
    init(type_id);
}

//---------------------------------------------------------------------------//
ShapeType::ShapeType(const std::string &type_name)
{
    init(type_name);
}

//---------------------------------------------------------------------------//
ShapeType::ShapeType(const conduit::Node &topology)
{
    init(InvalidTypeId);

    const std::string topoType = topology["type"].as_string();
    if(topoType == "unstructured" && topology["elements"].has_child("shape"))
    {
        const auto shape = topology["elements/shape"].as_string();
        init(shape);
        // Do more work to initialize the dimensions for mixed.
        if(shape == "mixed" && topology.has_path("elements/shape_map"))
        {
            const conduit::Node &n_shape_map = topology["elements/shape_map"];
            index_t maxShapeDim = InvalidDimension;
            for(conduit::index_t i = 0; i < n_shape_map.number_of_children(); i++)
            {
                auto id = type_name_to_id(n_shape_map[i].name());
                if(id != InvalidTypeId)
                {
                    maxShapeDim = std::max(maxShapeDim, TOPO_SHAPE_DIMS[id]);
                }
            }
            if(maxShapeDim != InvalidDimension)
            {
               dim = maxShapeDim;
            }
        }
    }
    else if(topoType == "points")
    {
        // handle points separately.
        init("point");
    }
    else
    {
        // Handle other topology types. Using the spatial dimension as a proxy
        // for the topological dimension.
        const Node *coordset = find_reference_node(topology, "coordset");
        conduit::index_t d = coordset::dims(*coordset);
        switch(d)
        {
        case 0:
            init("point");
            break;
        case 1:
            init("line");
            break;
        case 2:
            init("quad");
            break;
        case 3:
            init("hex");
            break;
        default:
            CONDUIT_ERROR("Unable to determine shape for topology.");
            break;
        }
    }
}


//---------------------------------------------------------------------------//
void
ShapeType::init(const std::string &type_name)
{
    init(type_name_to_id(type_name));
}


//---------------------------------------------------------------------------//
void
ShapeType::init(const index_t type_id)
{
    if(type_id < 0 || type_id >= (index_t)TOPO_SHAPES.size())
    {
        id = InvalidTypeId;
        dim = InvalidDimension;
        indices = embed_id = embed_count = -1;
        embedding = NULL;
    }
    else
    {
        id = type_id;
        dim = TOPO_SHAPE_DIMS[type_id];
        indices = TOPO_SHAPE_INDEX_COUNTS[type_id];

        embed_id = TOPO_SHAPE_EMBED_TYPES[type_id];
        embed_count = TOPO_SHAPE_EMBED_COUNTS[type_id];
        embedding = const_cast<index_t*>(TOPO_SHAPE_EMBEDDINGS[type_id]);
    }
}


//---------------------------------------------------------------------------//
index_t
ShapeType::type_name_to_id(const std::string &type_name)
{
    for(index_t i = 0; i < (index_t)TOPO_SHAPES.size(); i++)
    {
        if(type_name == TOPO_SHAPES[i])
        {
            return i;
        }
    }
    return InvalidTypeId;
}


//---------------------------------------------------------------------------//
bool
ShapeType::is_poly() const
{
    return is_polygonal() || is_polyhedral();
}


//---------------------------------------------------------------------------//
bool
ShapeType::is_polygonal() const
{
    return id == polygonal_id() && dim == 2;
}


//---------------------------------------------------------------------------//
bool
ShapeType::is_polyhedral() const
{
    return id == polyhedral_id() && dim == 3;
}


//---------------------------------------------------------------------------//
bool
ShapeType::is_mixed() const
{
    return id == mixed_id();
}


//---------------------------------------------------------------------------//
bool
ShapeType::is_valid() const
{
    return id >= 0;
}


//---------------------------------------------------------------------------//
const std::string &
ShapeType::type() const
{
    return is_valid() ? TOPO_SHAPES[id] : empty_type;
}

//---------------------------------------------------------------------------//
index_t ShapeType::num_faces() const
{
    // wedge and pyramid are special cases.
    return (id == wedge_id() || id == pyramid_id()) ? 5 : embed_count;
}

//---------------------------------------------------------------------------//
const index_t *
ShapeType::get_face(index_t face, index_t &nIds) const
{
    const index_t *ids = nullptr;

    // wedge and pyramid are special cases.
    if(id == wedge_id())
    {
        nIds = TOPO_WEDGE_FACES[face][0];
        ids = &TOPO_WEDGE_FACES[face][1];
    }
    else if(id == pyramid_id())
    {
        nIds = TOPO_PYRAMID_FACES[face][0];
        ids = &TOPO_PYRAMID_FACES[face][1];
    }
    else
    {
        // For all other shapes, their embedded type works.
        nIds = TOPO_SHAPE_INDEX_COUNTS[TOPO_SHAPE_EMBED_TYPES[id]];
        ids = embedding + face * nIds;
    }
    return ids;
}

//---------------------------------------------------------------------------//
index_t ShapeType::wedge_id()
{
    static index_t id = ShapeType::type_name_to_id("wedge");
    return id;
}

//---------------------------------------------------------------------------//
index_t ShapeType::pyramid_id()
{
    static index_t id = ShapeType::type_name_to_id("pyramid");
    return id;
}

//---------------------------------------------------------------------------//
index_t ShapeType::polygonal_id()
{
    static index_t id = ShapeType::type_name_to_id("polygonal");
    return id;
}

//---------------------------------------------------------------------------//
index_t ShapeType::polyhedral_id()
{
    static index_t id = ShapeType::type_name_to_id("polyhedral");
    return id;
}

//---------------------------------------------------------------------------//
index_t ShapeType::mixed_id()
{
    static index_t id = ShapeType::type_name_to_id("mixed");
    return id;
}

//---------------------------------------------------------------------------//
ShapeCascade::ShapeCascade(const conduit::Node &topology)
{
    ShapeType base_type(topology);
    init(base_type);
}

//---------------------------------------------------------------------------//
ShapeCascade::ShapeCascade(const ShapeType &base_type)
{
    init(base_type);
}

//---------------------------------------------------------------------------//
index_t
ShapeCascade::get_num_embedded(const index_t level) const
{
    index_t num_embedded = -1;

    if(!get_shape().is_poly())
    {
        num_embedded = 1;
        for(index_t di = level + 1; di <= dim; di++)
        {
            num_embedded *= dim_types[di].embed_count;
        }
    }

    return num_embedded;
}


//---------------------------------------------------------------------------//
const ShapeType&
ShapeCascade::get_shape(const index_t level) const
{
    return dim_types[level < 0 ? dim : level];
}

//---------------------------------------------------------------------------//
void
ShapeCascade::init(const ShapeType &base_type)
{
    dim = base_type.dim;

    dim_types[base_type.dim] = base_type;
    for(index_t di = base_type.dim - 1; di >= 0; di--)
    {
        dim_types[di] = ShapeType(dim_types[di + 1].embed_id);
    }
}

//-----------------------------------------------------------------------------
/// blueprint mesh utility query functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Node link_nodes(const Node &lhs, const Node &rhs)
{
    Node linker;
    linker.append().set_external(lhs);
    linker.append().set_external(rhs);
    return linker;
}


//-----------------------------------------------------------------------------
DataType
find_widest_dtype(const Node &node, const std::vector<DataType> &default_dtypes)
{
    DataType widest_dtype(default_dtypes[0].id(), 0, 0, 0, 0, default_dtypes[0].endianness());

    std::vector<const Node*> node_bag(1, &node);
    while(!node_bag.empty())
    {
        const Node *curr_node = node_bag.back(); node_bag.pop_back();
        const DataType curr_dtype = curr_node->dtype();
        if( curr_dtype.is_list() || curr_dtype.is_object() )
        {
            NodeConstIterator curr_node_it = curr_node->children();
            while(curr_node_it.has_next())
            {
                node_bag.push_back(&curr_node_it.next());
            }
        }
        else
        {
            for(index_t ti = 0; ti < (index_t)default_dtypes.size(); ti++)
            {
                const DataType &valid_dtype = default_dtypes[ti];
                bool is_valid_dtype =
                    (curr_dtype.is_floating_point() && valid_dtype.is_floating_point()) ||
                    (curr_dtype.is_signed_integer() && valid_dtype.is_signed_integer()) ||
                    (curr_dtype.is_unsigned_integer() && valid_dtype.is_unsigned_integer()) ||
                    (curr_dtype.is_string() && valid_dtype.is_string());
                if(is_valid_dtype && (widest_dtype.element_bytes() < curr_dtype.element_bytes()))
                {
                    widest_dtype.set(DataType(curr_dtype.id(), 1));
                }
            }
        }
    }

    bool no_type_found = widest_dtype.element_bytes() == 0;
    return no_type_found ? default_dtypes[0] : widest_dtype;
}


//-----------------------------------------------------------------------------
DataType
find_widest_dtype(const Node &node, const DataType &default_dtype)
{
    return find_widest_dtype(node, std::vector<DataType>(1, default_dtype));
}


//-----------------------------------------------------------------------------
const Node *
find_reference_node(const Node &node, const std::string &ref_key)
{
    const Node *res = nullptr;

    // NOTE: This segment of code is necessary to transform "topology" into
    // "topologies" while keeping all other dependency names (e.g. "coordset")
    // simply plural by just appending an "s" character.
    const std::string ref_section = (ref_key[ref_key.length()-1] != 'y') ?
        ref_key + "s" : ref_key.substr(0, ref_key.length()-1) + "ies";

    if(node.has_child(ref_key))
    {
        const std::string &ref_value = node.fetch(ref_key).as_string();

        const Node *traverse_node = node.parent();
        while(traverse_node != NULL)
        {
            if(traverse_node->has_child(ref_section))
            {
                const Node &ref_parent = traverse_node->fetch(ref_section);
                if(ref_parent.has_child(ref_value))
                {
                    res = &ref_parent[ref_value];
                }
                break;
            }
            traverse_node = traverse_node->parent();
        }
    }

    return res;
}


//-----------------------------------------------------------------------------
// NOTE: 'node' can be any subtree of a Blueprint-compliant mesh
index_t
find_domain_id(const Node &node)
{
    index_t domain_id = -1;

    Node info;
    const Node *curr_node = &node;
    while(curr_node != NULL && domain_id == -1)
    {
        if(blueprint::mesh::verify(*curr_node, info))
        {
            const std::vector<const Node *> domains = blueprint::mesh::domains(*curr_node);
            const Node &domain = *domains.front();
            if(domain.has_path("state/domain_id"))
            {
                domain_id = domain["state/domain_id"].to_index_t();
            }
        }

        curr_node = curr_node->parent();
    }

    return domain_id;
}

//---------------------------------------------------------------------------
// @brief Slice the n_src array using the indices stored in ids. We use the
//        array classes for their [] operators that deal with interleaved
//        and non-interleaved arrays.
template <typename ArrayType, typename IndexType>
inline void
typed_slice_array(const ArrayType &src, const std::vector<IndexType> &ids, ArrayType &dest)
{
    size_t n = ids.size();
    for(size_t i = 0; i < n; i++)
        dest[i] = src[ids[i]];
}

//---------------------------------------------------------------------------
/**
 @brief Slice a node as an array using its native data type.
 */
template <typename IndexType>
void
slice_array_internal(const conduit::Node &n_src_values,
                     const std::vector<IndexType> &ids,
                     Node &n_dest_values)
{
    // Copy the DataType of the input conduit::Node but override the number of elements
    // before copying it in so assigning to n_dest_values triggers a memory
    // allocation.
    auto dt = n_src_values.dtype();

    // Make sure the destination node is reset so the node will get the
    // right dtype when we reinitialize it below.
    n_dest_values.reset();
    // Allocate the new data.
    n_dest_values = DataType(n_src_values.dtype().id(), ids.size());

    // Do the slice.
    if(dt.is_int8())
    {
        auto dest(n_dest_values.as_int8_array());
        typed_slice_array(n_src_values.as_int8_array(), ids, dest);
    }
    else if(dt.is_int16())
    {
        auto dest(n_dest_values.as_int16_array());
        typed_slice_array(n_src_values.as_int16_array(), ids, dest);
    }
    else if(dt.is_int32())
    {
        auto dest(n_dest_values.as_int32_array());
        typed_slice_array(n_src_values.as_int32_array(), ids, dest);
    }
    else if(dt.is_int64())
    {
        auto dest(n_dest_values.as_int64_array());
        typed_slice_array(n_src_values.as_int64_array(), ids, dest);
    }
    else if(dt.is_uint8())
    {
        auto dest(n_dest_values.as_uint8_array());
        typed_slice_array(n_src_values.as_uint8_array(), ids, dest);
    }
    else if(dt.is_uint16())
    {
        auto dest(n_dest_values.as_uint16_array());
        typed_slice_array(n_src_values.as_uint16_array(), ids, dest);
    }
    else if(dt.is_uint32())
    {
        auto dest(n_dest_values.as_uint32_array());
        typed_slice_array(n_src_values.as_uint32_array(), ids, dest);
    }
    else if(dt.is_uint64())
    {
        auto dest(n_dest_values.as_uint64_array());
        typed_slice_array(n_src_values.as_uint64_array(), ids, dest);
    }
    else if(dt.is_float32())
    {
        auto dest(n_dest_values.as_float32_array());
        typed_slice_array(n_src_values.as_float32_array(), ids, dest);
    }
    else if(dt.is_float64())
    {
        auto dest(n_dest_values.as_float64_array());
        typed_slice_array(n_src_values.as_float64_array(), ids, dest);
    }
    else if(dt.is_char())
    {
        auto dest(n_dest_values.as_char_array());
        typed_slice_array(n_src_values.as_char_array(), ids, dest);
    }
    else if(dt.is_short())
    {
        auto dest(n_dest_values.as_short_array());
        typed_slice_array(n_src_values.as_short_array(), ids, dest);
    }
    else if(dt.is_int())
    {
        auto dest(n_dest_values.as_int_array());
        typed_slice_array(n_src_values.as_int_array(), ids, dest);
    }
    else if(dt.is_long())
    {
        auto dest(n_dest_values.as_long_array());
        typed_slice_array(n_src_values.as_long_array(), ids, dest);
    }
    else if(dt.is_unsigned_char())
    {
        auto dest(n_dest_values.as_unsigned_char_array());
        typed_slice_array(n_src_values.as_unsigned_char_array(), ids, dest);
    }
    else if(dt.is_unsigned_short())
    {
        auto dest(n_dest_values.as_unsigned_short_array());
        typed_slice_array(n_src_values.as_unsigned_short_array(), ids, dest);
    }
    else if(dt.is_unsigned_int())
    {
        auto dest(n_dest_values.as_unsigned_int_array());
        typed_slice_array(n_src_values.as_unsigned_int_array(), ids, dest);
    }
    else if(dt.is_unsigned_long())
    {
        auto dest(n_dest_values.as_unsigned_long_array());
        typed_slice_array(n_src_values.as_unsigned_long_array(), ids, dest);
    }
    else if(dt.is_float())
    {
        auto dest(n_dest_values.as_float_array());
        typed_slice_array(n_src_values.as_float_array(), ids, dest);
    }
    else if(dt.is_double())
    {
        auto dest(n_dest_values.as_double_array());
        typed_slice_array(n_src_values.as_double_array(), ids, dest);
    }
}

//---------------------------------------------------------------------------
bool same_nodes(const conduit::Node &n1, const conduit::Node &n2)
{
   return (&n1 == &n2) ||
          (n1.contiguous_data_ptr() != nullptr &&
           n1.contiguous_data_ptr() == n2.contiguous_data_ptr());
}

//---------------------------------------------------------------------------
void
slice_array(const conduit::Node &n_src_values,
            const std::vector<int> &ids,
            conduit::Node &n_dest_values)
{
    // Check whether the src and dest nodes are the same. If so, we slice into
    // a tmp node and move its contents.
    if(same_nodes(n_src_values, n_dest_values))
    {
        conduit::Node tmp;
        slice_array_internal(n_src_values, ids, tmp);
        n_dest_values.move(tmp);
    }
    else
    {
        slice_array_internal(n_src_values, ids, n_dest_values);
    }
}

//---------------------------------------------------------------------------
void
slice_array(const conduit::Node &n_src_values,
            const std::vector<conduit::index_t> &ids,
            conduit::Node &n_dest_values)
{
    // Check whether the src and dest nodes are the same. If so, we slice into
    // a tmp node and move its contents.
    if(same_nodes(n_src_values, n_dest_values))
    {
        conduit::Node tmp;
        slice_array_internal(n_src_values, ids, tmp);
        n_dest_values.move(tmp);
    }
    else
    {
        slice_array_internal(n_src_values, ids, n_dest_values);
    }
}

//---------------------------------------------------------------------------
template <typename IndexType>
void
slice_field_internal(const conduit::Node &n_src_values,
                     const std::vector<IndexType> &ids,
                     conduit::Node &n_dest_values)
{
    if(n_src_values.number_of_children() > 0)
    {
        // Slice an mcarray
        for(conduit::index_t ci = 0; ci < n_src_values.number_of_children(); ci++)
        {
            const conduit::Node &comp = n_src_values[ci];
            slice_array(comp, ids, n_dest_values[comp.name()]);
        }
    }
    else
    {
        slice_array(n_src_values, ids, n_dest_values);
    }
}

//---------------------------------------------------------------------------
void
slice_field(const conduit::Node &n_src_values,
            const std::vector<int> &ids,
            conduit::Node &n_dest_values)
{
    slice_field_internal(n_src_values, ids, n_dest_values);
}

//---------------------------------------------------------------------------
void
slice_field(const conduit::Node &n_src_values,
            const std::vector<conduit::index_t> &ids,
            conduit::Node &n_dest_values)
{
    slice_field_internal(n_src_values, ids, n_dest_values);
}

//---------------------------------------------------------------------------
void
copy_fields(const conduit::Node &srcFields,
            conduit::Node &destFields,
            const conduit::Node &options)
{
    // Pull out any exclusions
    std::vector<std::string> exclusions;
    if(options.has_child("exclusions"))
    {
        const conduit::Node &ex = options["exclusions"];
        for(conduit::index_t i = 0; i < ex.number_of_children(); i++)
            exclusions.push_back(ex[i].as_string());
    }
    std::string topoName;
    if(options.has_child("topology"))
    {
        topoName = options.fetch_existing("topology").as_string();
    }

    // Copy any fields that are not excluded (and match the topo).
    for (conduit::index_t i = 0; i < srcFields.number_of_children(); i++)
    {
        const conduit::Node &f = srcFields[i];
        if(topoName.empty() || topoName == f.fetch_existing("topology").as_string())
        {
            if(std::find(exclusions.begin(), exclusions.end(), f.name()) == exclusions.end())
            {
                destFields[f.name()].set(f);
            }
        }
    }
}

//---------------------------------------------------------------------------
void
convert(conduit::Node &root,
        const conduit::DataType &desired_type,
        const std::vector<std::string> &keys)
{
    for (const auto &path : keys)
    {
        if (root.has_path(path))
        {
            conduit::Node &n = root.fetch_existing(path);
            if (n.dtype().id() != desired_type.id())
            {
                conduit::Node changed;
                n.to_data_type(desired_type.id(), changed);
                n.set(changed);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::connectivity --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void
connectivity::make_element_2d(std::vector<index_t>& elem,
                              index_t element,
                              index_t iwidth)
{
    index_t ilo = element % iwidth;
    index_t jlo = element / iwidth;
    index_t ihi = ilo + 1;
    index_t jhi = jlo + 1;

    index_t ilo_jlo = (iwidth+1)*jlo + ilo;
    index_t ihi_jlo = (iwidth+1)*jlo + ihi;
    index_t ihi_jhi = (iwidth+1)*jhi + ihi;
    index_t ilo_jhi = (iwidth+1)*jhi + ilo;

    elem.push_back(ilo_jlo);
    elem.push_back(ihi_jlo);
    elem.push_back(ihi_jhi);
    elem.push_back(ilo_jhi);
}


void
connectivity::make_element_3d(ElemType& connect,
                              index_t element,
                              index_t iwidth,
                              index_t jwidth,
                              index_t kwidth,
                              SubelemMap& faces)
{
    index_t ilo = element % iwidth;
    index_t jlo = (element / iwidth) % jwidth;
    index_t klo = element / (iwidth*jwidth);
    index_t ihi = ilo + 1;
    index_t jhi = jlo + 1;
    index_t khi = klo + 1;

    index_t jlo_offset = (iwidth+1)*jlo;
    index_t jhi_offset = (iwidth+1)*jhi;
    index_t klo_offset = (iwidth+1)*(jwidth+1)*klo;
    index_t khi_offset = (iwidth+1)*(jwidth+1)*khi;


    index_t iface_start = 0; 
    index_t jface_start = (iwidth+1)*jwidth*kwidth;
    index_t kface_start = jface_start + iwidth*(jwidth+1)*kwidth;

    //ifaces
    {
        index_t j_offset = jlo_offset; 
        index_t k_offset = (iwidth+1)*jwidth*klo;

        index_t lo_face = iface_start + ilo + j_offset + k_offset;
        index_t hi_face = iface_start + ihi + j_offset + k_offset;


        //ilo face
        if (faces.find(lo_face) == faces.end())
        {
            auto& ilo_face = faces[lo_face];
            ilo_face.push_back(ilo + jlo_offset + klo_offset);
            ilo_face.push_back(ilo + jhi_offset + klo_offset);
            ilo_face.push_back(ilo + jhi_offset + khi_offset);
            ilo_face.push_back(ilo + jlo_offset + khi_offset);
        }
        //ihi face
        if (faces.find(hi_face) == faces.end())
        {
            auto& ihi_face = faces[hi_face];
            ihi_face.push_back(ihi + jlo_offset + klo_offset);
            ihi_face.push_back(ihi + jhi_offset + klo_offset);
            ihi_face.push_back(ihi + jhi_offset + khi_offset);
            ihi_face.push_back(ihi + jlo_offset + khi_offset);
        }
        connect.push_back(lo_face);
        connect.push_back(hi_face);
    }
    //jfaces
    {
        index_t i_offset = ilo;  
        index_t jlo_face_offset = iwidth*jlo; 
        index_t jhi_face_offset = iwidth*jhi; 
        index_t k_offset = iwidth*(jwidth+1)*klo;

        index_t lo_face = jface_start + i_offset + jlo_face_offset + k_offset;
        index_t hi_face = jface_start + i_offset + jhi_face_offset + k_offset;

        //jlo face
        if (faces.find(lo_face) == faces.end())
        {
            auto& jlo_face = faces[lo_face];
            jlo_face.push_back(ilo + jlo_offset + klo_offset);
            jlo_face.push_back(ihi + jlo_offset + klo_offset);
            jlo_face.push_back(ihi + jlo_offset + khi_offset);
            jlo_face.push_back(ilo + jlo_offset + khi_offset);
        }
        //jhi face
        if (faces.find(hi_face) == faces.end())
        {
            auto& jhi_face = faces[hi_face];
            jhi_face.push_back(ilo + jhi_offset + klo_offset);
            jhi_face.push_back(ihi + jhi_offset + klo_offset);
            jhi_face.push_back(ihi + jhi_offset + khi_offset);
            jhi_face.push_back(ilo + jhi_offset + khi_offset);
        }
        connect.push_back(lo_face);
        connect.push_back(hi_face);
    }
    //kfaces
    {
        index_t i_offset = ilo;  
        index_t j_offset = iwidth*jlo; 
        index_t klo_face_offset = iwidth*jwidth*klo;
        index_t khi_face_offset = iwidth*jwidth*khi;

        index_t lo_face = kface_start + i_offset + j_offset + klo_face_offset;
        index_t hi_face = kface_start + i_offset + j_offset + khi_face_offset;

        //klo face
        if (faces.find(lo_face) == faces.end())
        {
            auto& klo_face = faces[lo_face];
            klo_face.push_back(ilo + jlo_offset + klo_offset);
            klo_face.push_back(ihi + jlo_offset + klo_offset);
            klo_face.push_back(ihi + jhi_offset + klo_offset);
            klo_face.push_back(ilo + jhi_offset + klo_offset);
        }
        //khi face
        if (faces.find(hi_face) == faces.end())
        {
            auto& khi_face = faces[hi_face];
            khi_face.push_back(ilo + jlo_offset + khi_offset);
            khi_face.push_back(ihi + jlo_offset + khi_offset);
            khi_face.push_back(ihi + jhi_offset + khi_offset);
            khi_face.push_back(ilo + jhi_offset + khi_offset);
        }
        connect.push_back(/*kface_start +*/lo_face);
        connect.push_back(/*kface_start +*/hi_face);
    }
}




//-----------------------------------------------------------------------------
void
connectivity::create_elements_2d(const Node& ref_win,
                                 index_t i_lo,
                                 index_t j_lo,
                                 index_t iwidth,
                                 std::map<index_t, std::vector<index_t> >& elems)
{
    index_t origin_iref = ref_win["origin/i"].to_index_t();
    index_t origin_jref = ref_win["origin/j"].to_index_t();

    index_t ref_size_i = ref_win["dims/i"].to_index_t();
    index_t ref_size_j = ref_win["dims/j"].to_index_t();

    if (ref_size_i == 1)
    {
        index_t jstart = origin_jref - j_lo;
        index_t jend = origin_jref - j_lo + ref_size_j - 1;
        if (origin_iref == i_lo)
        {
            for (index_t jidx = jstart; jidx < jend; ++jidx)
            {
                index_t offset = jidx * iwidth;
                auto& elem_conn = elems[offset];
                if (elem_conn.empty())
                {
                    connectivity::make_element_2d(elem_conn,
                                                  offset,
                                                  iwidth);
                }
            }
        }
        else
        {
            for (index_t jidx = jstart; jidx < jend; ++jidx)
            {
                index_t offset = jidx * iwidth + (origin_iref - i_lo - 1);
                auto& elem_conn = elems[offset];
                if (elem_conn.empty())
                {
                    connectivity::make_element_2d(elem_conn,
                                                  offset,
                                                  iwidth);
                }
            }
        }
    }
    else if (ref_size_j == 1)
    {
        index_t istart = origin_iref - i_lo;
        index_t iend = origin_iref - i_lo + ref_size_i - 1;
        if (origin_jref == j_lo)
        {
            for (index_t iidx = istart; iidx < iend; ++iidx)
            {
                auto& elem_conn = elems[iidx];
                if (elem_conn.empty())
                {
                    connectivity::make_element_2d(elem_conn,
                                                  iidx,
                                                  iwidth);
                }
            }
        }
        else
        {
            for (index_t iidx = istart; iidx < iend; ++iidx)
            {
                index_t offset = iidx + ((origin_jref - j_lo - 1) * iwidth);
                auto& elem_conn = elems[offset];
                if (elem_conn.empty())
                {
                    connectivity::make_element_2d(elem_conn,
                                                  offset,
                                                  iwidth);
                }
            }
        }
    }

    index_t istart = origin_iref - i_lo;
    index_t jstart = origin_jref - j_lo;
    index_t iend = istart + ref_size_i - 1;
    index_t jend = jstart + ref_size_j - 1;

    if (ref_size_i == 1)
    {
        if (origin_iref != i_lo)
        {
            --istart;
        }
        iend = istart + 1;
    }
    if (ref_size_j == 1)
    {
        if (origin_jref != j_lo)
        {
            --jstart;
        }
        jend = jstart + 1;
    }

    for (index_t jidx = jstart; jidx < jend; ++jidx)
    {
        index_t joffset = jidx * iwidth;
        for (index_t iidx = istart; iidx < iend; ++iidx)
        {
            index_t offset = joffset + iidx;
            auto& elem_conn = elems[offset];
            if (elem_conn.empty())
            {
                 connectivity::make_element_2d(elem_conn,
                                               offset,
                                               iwidth);
            }
        }
    }
}


//-----------------------------------------------------------------------------
void
connectivity::create_elements_3d(const Node& ref_win,
                                       index_t i_lo,
                                       index_t j_lo,
                                       index_t k_lo,
                                       index_t iwidth,
                                       index_t jwidth,
                                       index_t kwidth,
                                       std::map<index_t, ElemType>& elems,
                                       SubelemMap& faces)
{
    index_t origin_iref = ref_win["origin/i"].to_index_t();
    index_t origin_jref = ref_win["origin/j"].to_index_t();
    index_t origin_kref = ref_win["origin/k"].to_index_t();

    index_t ref_size_i = ref_win["dims/i"].to_index_t();
    index_t ref_size_j = ref_win["dims/j"].to_index_t();
    index_t ref_size_k = ref_win["dims/k"].to_index_t();

    index_t istart = origin_iref - i_lo;
    index_t jstart = origin_jref - j_lo;
    index_t kstart = origin_kref - k_lo;
    index_t iend = istart + ref_size_i - 1;
    index_t jend = jstart + ref_size_j - 1;
    index_t kend = kstart + ref_size_k - 1;

    if (ref_size_i == 1)
    {
        iend = istart + 1;
    }
    if (ref_size_j == 1)
    {
        jend = jstart + 1;
    }
    if (ref_size_k == 1)
    {
        kend = kstart + 1;
    }

    for (index_t kidx = kstart; kidx < kend; ++kidx)
    {
        index_t koffset = kidx * iwidth * jwidth;
        for (index_t jidx = jstart; jidx < jend; ++jidx)
        {
            index_t joffset = jidx * iwidth; 
            for (index_t iidx = istart; iidx < iend; ++iidx)
            {
                index_t offset = koffset + joffset + iidx;
                auto& elem_conn = elems[offset];
                if (elem_conn.empty())
                {
                     connectivity::make_element_3d(elem_conn,
                                                   offset,
                                                   iwidth,
                                                   jwidth,
                                                   kwidth,
                                                   faces);
                }
            }
        }
    }
}

void
connectivity::connect_elements_3d(const Node& ref_win,
                                        index_t i_lo,
                                        index_t j_lo,
                                        index_t k_lo,
                                        index_t iwidth,
                                        index_t jwidth,
                                        index_t& new_vertex,
                                        std::map<index_t, ElemType>& elems)
{
    index_t origin_iref = ref_win["origin/i"].to_index_t();
    index_t origin_jref = ref_win["origin/j"].to_index_t();
    index_t origin_kref = ref_win["origin/k"].to_index_t();

    index_t ref_size_i = ref_win["dims/i"].to_index_t();
    index_t ref_size_j = ref_win["dims/j"].to_index_t();
    index_t ref_size_k = ref_win["dims/k"].to_index_t();

    index_t kstart = origin_kref - k_lo;
    index_t kend = origin_kref - k_lo + ref_size_k - 1;
    if (kstart == kend) kend = kstart + 1;
    index_t jstart = origin_jref - j_lo;
    index_t jend = origin_jref - j_lo + ref_size_j - 1;
    if (jstart == jend) jend = jstart + 1;
    index_t istart = origin_iref - i_lo;
    index_t iend = origin_iref - i_lo + ref_size_i - 1;
    if (istart == iend) iend = istart + 1;

    for (index_t kidx = kstart; kidx < kend; ++kidx)
    {
        for (index_t jidx = jstart; jidx < jend; ++jidx)
        {
            for (index_t iidx = istart; iidx < iend; ++iidx)
            {
                index_t offset = kidx*iwidth*jwidth + jidx*iwidth + iidx;
                auto& elem_conn = elems[offset];
                elem_conn.push_back(new_vertex);
                ++new_vertex;
            }
        }
    }
}

void
connectivity::connect_elements_2d(const Node& ref_win,
                                  index_t i_lo,
                                  index_t j_lo,
                                  index_t iwidth,
                                  index_t ratio,
                                  index_t& new_vertex,
                                  std::map<index_t, std::vector<index_t> >& elems,
                                  bool flip)
{
    index_t origin_iref = ref_win["origin/i"].to_index_t();
    index_t origin_jref = ref_win["origin/j"].to_index_t();

    index_t ref_size_i = ref_win["dims/i"].to_index_t();
    index_t ref_size_j = ref_win["dims/j"].to_index_t();

    //NSE:  Use of these values may not be correct for ratio > 3.
    index_t part_lo = ref_win.has_child("partial_lo") ? 
        ref_win["partial_lo"].to_index_t() : 0;
    index_t part_hi = ref_win.has_child("partial_hi") ? 
        ref_win["partial_hi"].to_index_t() : 0;

    if (ref_size_i == 1)
    {
        index_t jstart = origin_jref - j_lo;
        index_t jend = origin_jref - j_lo + ref_size_j - 1;

        if (part_lo > 1)
        {
            if (flip)
            {
                --jend;
            }
            else
            {
                ++jstart;
                new_vertex += ratio - part_lo; 
            }
        }
        if (part_hi > 1)
        {
            if (flip)
            {
                ++jstart;
                new_vertex += ratio - part_hi;
            }
            else
            {
                --jend;
            }
        }
        if (origin_iref == i_lo)
        {
            for (index_t jidx = jstart; jidx < jend; ++jidx)
            {
                index_t offset = jidx * (iwidth);
                auto& elem_conn = elems[offset];
                if (ratio > 1)
                {
                    for (index_t nr = ratio-1; nr > 0; --nr)
                    {
                        elem_conn.push_back(new_vertex+nr-1);
                    }
                    new_vertex += ratio - 1;
                }
            }
        }
        else
        {
            for (index_t jidx = jstart; jidx < jend; ++jidx)
            {
                index_t offset = jidx * iwidth + (origin_iref - i_lo - 1);
                auto& elem_conn = elems[offset];
                if (ratio > 1)
                {
                    size_t new_size = elem_conn.size() + ratio - 1;
                    elem_conn.resize(new_size);
                    index_t corner = 1;
                    if (elem_conn[1] - elem_conn[0] != 1)
                    {
                        index_t ioff = offset % iwidth;
                        index_t joff = offset / iwidth;
                        index_t target = (iwidth+1)*joff + ioff + 1;
                        for (index_t nr = 1; nr < 1+ratio; ++nr)
                        {
                            if (elem_conn[nr] == target)
                            {
                                corner = nr;
                                break;
                            }
                        }
                    }
                    for (index_t nr = new_size-1; nr > corner+ratio-1; --nr)
                    {
                        elem_conn[nr] = elem_conn[nr-ratio+1];
                    }
                    for (index_t nr = corner+1; nr < corner+ratio; ++nr)
                    {
                        elem_conn[nr] = new_vertex;
                        ++new_vertex;
                    }
                }
            }
        }
    }
    else if (ref_size_j == 1)
    {
        index_t istart = origin_iref - i_lo;
        index_t iend = origin_iref - i_lo + ref_size_i - 1;

        if (part_lo > 1)
        {
            if (flip)
            {
                --iend;
            }
            else
            {
                ++istart;
                new_vertex += ratio - part_lo;
            }
        }
        if (part_hi > 1)
        {
            if (flip)
            {
                ++istart;
                new_vertex += ratio - part_hi;
            }
            else
            {
                --iend;
            }
        }

        if (origin_jref == j_lo)
        {
            for (index_t iidx = istart; iidx < iend; ++iidx)
            {
                auto& elem_conn = elems[iidx];
                if (ratio > 1)
                {
                    size_t new_size = elem_conn.size() + ratio - 1;
                    elem_conn.resize(new_size);
                    for (index_t nr = new_size-1; nr > 1; --nr)
                    {
                        elem_conn[nr] = elem_conn[nr-ratio+1];
                    }
                    for (index_t nr = 1; nr < ratio; ++nr)
                    {
                        elem_conn[nr] = (new_vertex+nr-1);
                    }
                    new_vertex += ratio - 1;
                }
            }
        }
        else
        {
            for (index_t iidx = istart; iidx < iend; ++iidx)
            {
                index_t offset = iidx + ((origin_jref - j_lo - 1) * iwidth);
                auto& elem_conn = elems[offset];
                if (ratio > 1)
                {
                    size_t old_size = elem_conn.size();
                    size_t new_size = old_size + ratio - 1;
                    elem_conn.resize(new_size);
                    index_t corner = 2;
                    if (old_size != 4)
                    {
                        index_t ioff = offset % iwidth;
                        index_t joff = offset / iwidth;
                        index_t target = (iwidth+1)*(joff+1) + ioff + 1;
                        for (index_t nr = 3; nr < 3+ratio; ++nr)
                        {
                            if (elem_conn[nr] == target) {
                                corner = nr;
                                break;
                            }
                        }
                    }
                    for (index_t nr = new_size-1; nr > corner+ratio-1; --nr)
                    {
                        elem_conn[nr] = elem_conn[nr-ratio+1];
                    }
                    for (index_t nr = corner+ratio-1; nr > corner; --nr) {
                        elem_conn[nr] = new_vertex;
                        ++new_vertex;
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::connectivity --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::coordset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
std::pair<std::string, std::vector<std::string>>
get_coordset_info(const Node &n)
{
    std::pair<std::string, std::vector<std::string>> info;
    std::string &cset_coordsys = info.first;
    std::vector<std::string> &cset_axes = info.second;

    std::string coords_path = "";
    if(n["type"].as_string() == "uniform")
    {
        if(n.has_child("origin"))
        {
            coords_path = "origin";
        }
        else if(n.has_child("spacing"))
        {
            coords_path = "spacing";
        }
        else
        {
            coords_path = "dims";
        }
    }
    else // if(n.has_child("values"))
    {
        coords_path = "values";
    }

    Node axis_names;
    const Node &cset_coords = n[coords_path];
    NodeConstIterator itr = cset_coords.children();
    while(itr.has_next())
    {
        itr.next();
        const std::string axis_name = itr.name();

        if(axis_name[0] == 'd' && axis_name.size() > 1)
        {
            axis_names[axis_name.substr(1, axis_name.length())];
        }
        else
        {
            axis_names[axis_name];
        }
    }

    std::vector<std::string> cset_base_axes;
    cset_coordsys = "unknown";
    if(axis_names.has_child("theta") || axis_names.has_child("phi"))
    {
        cset_coordsys = "spherical";
        cset_base_axes = SPHERICAL_AXES;
    }
    else if(axis_names.has_child("r")) // rz, or r w/o theta, phi
    {
        cset_coordsys = "cylindrical";
        cset_base_axes = CYLINDRICAL_AXES;
    }
    else if(axis_names.has_child("x") || axis_names.has_child("y") || axis_names.has_child("z"))
    {
        cset_coordsys = "cartesian";
        cset_base_axes = CARTESIAN_AXES;
    }
    else if(axis_names.has_child("i") || axis_names.has_child("j") || axis_names.has_child("k"))
    {
        cset_coordsys = "logical";
        cset_base_axes = LOGICAL_AXES;
    }

    // intersect 'cset_base_axes' and 'axis_names.child_names()'
    for(const std::string &base_axis : cset_base_axes)
    {
        for(const std::string &cset_axis : axis_names.child_names())
        {
            if(base_axis == cset_axis)
            {
                cset_axes.push_back(cset_axis);
                break;
            }
        }
    }

    // TODO(JRC): The following is an alterate (though potentially more error-prone) solution:
    // std::vector<std::string>(cset_axes.begin(), cset_axes.begin() + cset_coords.number_of_children());

    return info;
}


//-----------------------------------------------------------------------------
index_t
coordset::dims(const Node &n)
{
    const std::vector<std::string> csys_axes = coordset::axes(n);
    return (index_t)csys_axes.size();
}


//-----------------------------------------------------------------------------
index_t
coordset::length(const Node &n)
{
    index_t coordset_length = 1;

    const std::string csys_type = n["type"].as_string();
    const std::vector<std::string> csys_axes = coordset::axes(n);
    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        if(csys_type == "uniform")
        {
            coordset_length *= n["dims"][LOGICAL_AXES[i]].to_index_t();
        }
        else if(csys_type == "rectilinear")
        {
            coordset_length *= n["values"][csys_axes[i]].dtype().number_of_elements();
        }
        else // if(csys_type == "explicit")
        {
            coordset_length = n["values"][csys_axes[i]].dtype().number_of_elements();
        }
    }

    return coordset_length;
}


//-----------------------------------------------------------------------------
std::vector<std::string>
coordset::axes(const Node &n)
{
    return get_coordset_info(n).second;
}


//-----------------------------------------------------------------------------
std::string
coordset::coordsys(const Node &n)
{
    return get_coordset_info(n).first;
}


//-----------------------------------------------------------------------------
std::vector<float64>
coordset::_explicit::coords(const Node &n, const index_t i)
{
    std::vector<float64> cvals;

    Node temp;
    for(const std::string &axis : coordset::axes(n))
    {
        const Node &axis_node = n["values"][axis];
        temp.set_external(DataType(axis_node.dtype().id(), 1),
            (void*)axis_node.element_ptr(i));
        cvals.push_back(temp.to_float64());
    }

    return std::vector<float64>(std::move(cvals));
}

//-----------------------------------------------------------------------------
void
coordset::logical_dims(const conduit::Node &n, index_t *d, index_t maxdims)
{
    for(index_t i = 0; i < maxdims; i++)
    {
        d[i] = 1;
    }

    auto info = get_coordset_info(n);
    const std::string cset_type = n["type"].as_string();
    const std::vector<std::string> &cset_axes = info.second;
    if(cset_type == "uniform" || cset_type == "rectilinear")
    {
        const index_t dim = ((index_t)cset_axes.size() > maxdims) ? maxdims
            : (index_t)cset_axes.size();
        for(index_t i = 0; i < dim; i++)
        {
            if(cset_type == "uniform")
            {
                d[i] = n["dims"][LOGICAL_AXES[i]].to_index_t();
            }
            else // if(cset_type == "rectilinear")
            {
                d[i] = n["values"][cset_axes[i]].dtype().number_of_elements();
            }
        }
    }
    else // if(cset_type == "explicit")
    {
        d[0] = n["values"][cset_axes[0]].dtype().number_of_elements();
    }
}

//-----------------------------------------------------------------------------
template<typename data_array>
static void
typed_minmax(const data_array &da, float64 &out_min, float64 &out_max)
{
    // Figure out what primitive type we are dealing with
    using T = decltype(std::declval<data_array>().min());
    const index_t nelem = da.number_of_elements();
    T min = std::numeric_limits<T>::max();
    T max = std::numeric_limits<T>::lowest();
    for(index_t i = 0; i < nelem; i++)
    {
        min = std::min(min, da[i]);
        max = std::max(max, da[i]);
    }
    out_min = (float64)min;
    out_max = (float64)max;
}

//-----------------------------------------------------------------------------
std::vector<float64>
coordset::extents(const Node &n)
{
    std::vector<float64> cset_extents;
    const std::string csys_type = n["type"].as_string();
    const std::vector<std::string> csys_axes = coordset::axes(n);
    const index_t naxes = (index_t)csys_axes.size();
    cset_extents.reserve(naxes*2);
    for(index_t i = 0; i < naxes; i++)
    {
        float64 min, max;
        if(csys_type == "uniform")
        {
            index_t origin = 0;
            float64 spacing = 1.0;
            index_t dim = n["dims"][LOGICAL_AXES[i]].to_index_t();
            if(n.has_child("origin")
                && n["origin"].has_child(csys_axes[i]))
            {
                origin = n["origin"][csys_axes[i]].to_index_t();
            }
            if(n.has_child("spacing")
                && n["spacing"].has_child("d"+csys_axes[i]))
            {
                spacing = n["spacing"]["d" + csys_axes[i]].to_float64();
            }
            min = (float64)origin;
            max = (float64)origin + (spacing * ((float64)dim - 1.));
            if(spacing < 0.)
            {
                std::swap(min, max);
            }
        }
        else // csys_type == "rectilinear" || csys_type == "explicit"
        {
            const auto &axis = n["values"][csys_axes[i]];
            const auto id = axis.dtype().id();
            switch(id)
            {
            case conduit::DataType::INT8_ID:
                typed_minmax(axis.as_int8_array(), min, max);
                break;
            case conduit::DataType::INT16_ID:
                typed_minmax(axis.as_int16_array(), min, max);
                break;
            case conduit::DataType::INT32_ID:
                typed_minmax(axis.as_int32_array(), min, max);
                break;
            case conduit::DataType::INT64_ID:
                typed_minmax(axis.as_int64_array(), min, max);
                break;
            case conduit::DataType::UINT8_ID:
                typed_minmax(axis.as_uint8_array(), min, max);
                break;
            case conduit::DataType::UINT16_ID:
                typed_minmax(axis.as_uint16_array(), min, max);
                break;
            case conduit::DataType::UINT32_ID:
                typed_minmax(axis.as_uint32_array(), min, max);
                break;
            case conduit::DataType::UINT64_ID:
                typed_minmax(axis.as_uint64_array(), min, max);
                break;
            case conduit::DataType::FLOAT32_ID:
                typed_minmax(axis.as_float32_array(), min, max);
                break;
            case conduit::DataType::FLOAT64_ID:
                typed_minmax(axis.as_float64_array(), min, max);
                break;
            }
        }
        cset_extents.push_back(min);
        cset_extents.push_back(max);
    }
    return cset_extents;
}

//-----------------------------------------------------------------------------
std::tuple<bool, conduit::DataType>
coordset::supports_pointer_access(const conduit::Node &coordset)
{
    bool suitable = false;
    conduit::DataType dt;
    if(coordset.has_child("values"))
    {
        suitable = true;
        const conduit::Node &values = coordset.fetch_existing("values");
        for(conduit::index_t i = 0; i < values.number_of_children(); i++)
        {
            if(i == 0)
            {
                suitable &= values[i].dtype().is_compact();
                dt = values[i].dtype();
            }
            else
            {
                suitable &= (values[i].dtype().is_compact() && dt.id() == values[i].dtype().id());
            }
        }
    }
    return std::make_tuple(suitable, dt);
}

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::coordset::uniform --
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
std::vector<double>
coordset::uniform::spacing(const Node &n)
{
    auto info = get_coordset_info(n);
    const auto &cset_axes = info.second;
    std::vector<double> retval(cset_axes.size(), 1);
    if(n.has_child("spacing"))
    {
        const Node &n_spacing = n["spacing"];
        for(index_t i = 0; i < (index_t)cset_axes.size(); i++)
        {
            const std::string child_name = "d"+cset_axes[i];
            if(n_spacing.has_child(child_name))
            {
                retval[i] = n_spacing[child_name].to_double();
            }
        }
    }
    return retval;
}

//-----------------------------------------------------------------------------
std::vector<index_t>
coordset::uniform::origin(const Node &n)
{
    auto info = get_coordset_info(n);
    const auto &cset_axes = info.second;
    std::vector<index_t> retval(cset_axes.size(), 0);
    if(n.has_child("origin"))
    {
        const Node &n_spacing = n["origin"];
        for(index_t i = 0; i < (index_t)cset_axes.size(); i++)
        {
            const std::string child_name = cset_axes[i];
            if(n_spacing.has_child(child_name))
            {
                retval[i] = n_spacing[child_name].to_index_t();
            }
        }
    }
    return retval;
}

//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::coordset::uniform --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::coordset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::topology --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
index_t
topology::dims(const Node &n)
{
    index_t topology_dims = 1;

    const std::string type = n["type"].as_string();
    if(type != "unstructured")
    {
        const Node *coordset = find_reference_node(n, "coordset");
        topology_dims = coordset::dims(*coordset);
    }
    else // if(type == "unstructured")
    {
        ShapeType shape(n);
        topology_dims = (index_t)shape.dim;

        if(topology_dims == -1 && n.has_path("elements/shape_map"))
        {
          // We appear to have a mixed shape.
          const Node &n_shape_map = n.fetch_existing("elements/shape_map");
          for(index_t i = 0; i < n_shape_map.number_of_children(); i++)
          {
            ShapeType shape2(n_shape_map[i].name());
            topology_dims = std::max(topology_dims, static_cast<index_t>(shape2.dim));
          }
        }
    }

    return topology_dims;
}


//-----------------------------------------------------------------------------
void
topology::logical_dims(const Node &n, index_t *d, index_t maxdims)
{
    for(index_t i = 0; i < maxdims; i++)
        d[i] = 1;

    const std::string type = n["type"].as_string();
    if(type == "uniform" || type == "rectilinear")
    {
        const Node *coordset = find_reference_node(n, "coordset");
        const std::vector<std::string> csys_axes = coordset::axes(*coordset);
        for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
        {
            d[i] = ((type == "uniform") ?
                coordset->fetch_existing("dims")[LOGICAL_AXES[i]].to_index_t() :
                coordset->fetch_existing("values")[csys_axes[i]].dtype().number_of_elements()) - 1;
        }
    }
    else if(type == "structured")
    {
        const Node &dims = n["elements/dims"];

        for(index_t i = 0; i < (index_t)dims.number_of_children(); i++)
        {
            // Be more careful about looking up the axis names in case there are
            // extra children in the dims node, as can happen when we have a
            // strided structured topo.
            auto it = std::find(LOGICAL_AXES.cbegin(), LOGICAL_AXES.cend(), dims[i].name());
            if(it != LOGICAL_AXES.cend())
            {
                auto idx = it - LOGICAL_AXES.cbegin();
                d[idx] = std::max(static_cast<index_t>(1), dims[i].to_index_t());
            }
        }
    }
    else if(type == "points")
    {
        const Node *coordset = find_reference_node(n, "coordset");
        if(coordset)
        {
            coordset::logical_dims(*coordset, d, maxdims);
        }
        else
        {
            CONDUIT_ERROR("Unable to find reference node 'coordset'.");
        }
    }
    else // if(type == "unstructured")
    {
        // calc total number of elements

        // polygonal and polyhedral, or otherwise explicit "sizes"
        if(n["elements"].has_child("sizes"))
        {
            d[0] = n["elements/sizes"].dtype().number_of_elements();
        }
        else if( n["elements"].has_child("element_index") )  // stream style (deprecated)
        {
            const Node &elems_idx =  n["elements/element_index"];
            if(elems_idx.has_child("element_counts"))
            {
                index_t_accessor elem_counts_vals  = elems_idx["element_counts"].value();
                index_t total_elems = 0;
                for(index_t i=0;i< elem_counts_vals.number_of_elements();i++)
                {
                    total_elems += elem_counts_vals[i];
                }
                d[0] = total_elems;
            } // else, use size of stream_ids
            else if(elems_idx.has_child("stream_ids"))
            {
                d[0] = elems_idx["stream_ids"].dtype().number_of_elements();
            }
            else
            {
                CONDUIT_ERROR("invalid stream id topology: "
                              "missing elements/element_index/stream_ids");
            }

        }
        else // zoo
        {
            index_t conn_size = n["elements/connectivity"].dtype().number_of_elements();
            std::string shape_type_name = n["elements/shape"].as_string();
            // total number of elements == conn array size / shape size
            ShapeType shape_info(shape_type_name);
            d[0] = conn_size / shape_info.indices;
        }
    }
}

//-----------------------------------------------------------------------------
index_t
topology::length(const Node &n)
{
    index_t d[3]={1,1,1};
    logical_dims(n, d, 3);
    return d[0] * d[1] * d[2];
}

//-----------------------------------------------------------------------------
const Node &
topology::coordset(const Node &n)
{
    const Node *res = find_reference_node(n,"coordset");
    return *res;
}

//-----------------------------------------------------------------------------
void
topology::reindex_coords(const Node& topo,
                         const Node& new_coordset,
                         const Node& old_gvids,
                         const Node& new_gvids,
                         Node& out_topo)
{
    if (&out_topo != &topo)
    {
        out_topo.reset();
        out_topo.set(topo);
    }

    // Build a mapping of global vids -> new coordset indices
    // TODO: if we assume clustering of gvids, maybe a vector would be faster?
    std::unordered_map<index_t, index_t> remap_vids;

    index_t_accessor new_gvid_vals = new_gvids["values"].as_index_t_accessor();
    for (index_t idx = 0; idx < new_gvid_vals.number_of_elements(); idx++)
    {
        remap_vids[new_gvid_vals[idx]] = idx;
    }

    std::string node_path = "elements/connectivity";
    if (out_topo["elements/shape"].as_string() == "polyhedral")
    {
        node_path = "subelements/connectivity";
    }

    index_t_accessor old_vids = out_topo[node_path].as_index_t_accessor();
    index_t_accessor old_to_gvids = old_gvids["values"].as_index_t_accessor();
    std::vector<index_t> new_vids(old_vids.number_of_elements());
    for (index_t idx = 0; idx < static_cast<index_t>(new_vids.size()); idx++)
    {
        index_t old_vid = old_vids[idx];
        index_t gvid = old_to_gvids[old_vid];
        new_vids[idx] = remap_vids[gvid];
    }

    // Set the new vertex connectivity
    out_topo[node_path].set(new_vids);

    // Set the new associated coordset name
    out_topo["coordset"] = new_coordset.name();
}

//-----------------------------------------------------------------------------
template <typename Indexable, typename ElementType>
static std::vector<conduit::index_t>
spatial_ordering_impl(std::vector<Indexable> &coords, ElementType /*notused*/, conduit::index_t npts)
{
    // Sort the coordinates spatially
    std::vector<conduit::index_t> reorder;
    if(coords.size() == 2)
    {
        conduit::blueprint::mesh::utils::kdtree<Indexable, ElementType, 2> spatial_sort;
        spatial_sort.initialize(&coords[0], npts);
        reorder = std::move(spatial_sort.getIndices());
    }
    else if(coords.size() == 3)
    {
        conduit::blueprint::mesh::utils::kdtree<Indexable, ElementType, 3> spatial_sort;
        spatial_sort.initialize(&coords[0], npts);
        reorder = std::move(spatial_sort.getIndices());
    }
    return reorder;
}

//-----------------------------------------------------------------------------
std::vector<conduit::index_t>
topology::spatial_ordering(const conduit::Node &topo)
{
    // Make a new centroid topo and coordset. The coordset will contain the
    // element centers. This ought to be an explicit coordset.
    Node topo_dest, coords_dest, s2dmap, d2smap;
    mesh::topology::unstructured::generate_centroids(topo,
                                                     topo_dest,
                                                     coords_dest,
                                                     s2dmap,
                                                     d2smap);

    std::vector<conduit::index_t> reorder;
    conduit::Node &values = coords_dest.fetch_existing("values");
    const auto pa = coordset::supports_pointer_access(coords_dest);
    const auto suitable = std::get<0>(pa);
    conduit::index_t npts{};
    if(suitable)
    {
        // The coordinates will be accessed using pointers.
        const auto &dt = std::get<1>(pa);
        if(dt.is_double())
        {
            std::vector<double *> coords;
            double elem{};
            for(conduit::index_t i = 0; i < values.number_of_children(); i++)
            {
                npts = (i == 0) ? (values[i].as_double_array().number_of_elements()) : npts;
                coords.push_back(values[i].as_double_ptr());
            }
            reorder = spatial_ordering_impl(coords, elem, npts);
        }
        else if(dt.is_float())
        {
            std::vector<float *> coords;
            float elem{};
            for(conduit::index_t i = 0; i < values.number_of_children(); i++)
            {
                npts = (i == 0) ? (values[i].as_float_array().number_of_elements()) : npts;
                coords.push_back(values[i].as_float_ptr());
            }
            reorder = spatial_ordering_impl(coords, elem, npts);
        }
    }
    if(reorder.empty())
    {
        // Use a double accessor to access coordinates.
        std::vector<conduit::double_accessor> coords;
        double elem{};
        for(conduit::index_t i = 0; i < values.number_of_children(); i++)
        {
            npts = (i == 0) ? (values[i].as_double_accessor().number_of_elements()) : npts;
            coords.push_back(values[i].as_double_accessor());
        }
        reorder = spatial_ordering_impl(coords, elem, npts);
    }

    return reorder;
}

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::detail --
//-----------------------------------------------------------------------------
namespace detail
{

// Adapted from MFEM
struct HilbertCmp
{
   int coord;
   bool dir;
   const double *points;
   double mid;

   HilbertCmp(int _coord, bool _dir, const double *_points, double _mid)
      : coord(_coord), dir(_dir), points(_points), mid(_mid) {}

   bool operator()(int i) const
   {
      return (points[3*i + coord] < mid) != dir;
   }
};

//-----------------------------------------------------------------------------
// Adapted from MFEM
static void HilbertSort2D(int coord1, // major coordinate to sort points by
                          bool dir1,  // sort coord1 ascending/descending?
                          bool dir2,  // sort coord2 ascending/descending?
                          const double *points, conduit::index_t *beg, conduit::index_t *end,
                          double xmin, double ymin, double xmax, double ymax)
{
   if (end - beg <= 1) { return; }

   double xmid = (xmin + xmax)*0.5;
   double ymid = (ymin + ymax)*0.5;

   int coord2 = (coord1 + 1) % 2; // the 'other' coordinate

   // sort (partition) points into four quadrants
   conduit::index_t *p0 = beg, *p4 = end;
   conduit::index_t *p2 = std::partition(p0, p4, HilbertCmp(coord1,  dir1, points, xmid));
   conduit::index_t *p1 = std::partition(p0, p2, HilbertCmp(coord2,  dir2, points, ymid));
   conduit::index_t *p3 = std::partition(p2, p4, HilbertCmp(coord2, !dir2, points, ymid));

   if (p1 != p4)
   {
      HilbertSort2D(coord2, dir2, dir1, points, p0, p1,
                    ymin, xmin, ymid, xmid);
   }
   if (p1 != p0 || p2 != p4)
   {
      HilbertSort2D(coord1, dir1, dir2, points, p1, p2,
                    xmin, ymid, xmid, ymax);
   }
   if (p2 != p0 || p3 != p4)
   {
      HilbertSort2D(coord1, dir1, dir2, points, p2, p3,
                    xmid, ymid, xmax, ymax);
   }
   if (p3 != p0)
   {
      HilbertSort2D(coord2, !dir2, !dir1, points, p3, p4,
                    ymid, xmax, ymin, xmid);
   }
}

//-----------------------------------------------------------------------------
// Adapted from MFEM
static void HilbertSort3D(int coord1, bool dir1, bool dir2, bool dir3,
                          const double *points, conduit::index_t *beg, conduit::index_t *end,
                          double xmin, double ymin, double zmin,
                          double xmax, double ymax, double zmax)
{
   if (end - beg <= 1) { return; }

   double xmid = (xmin + xmax)*0.5;
   double ymid = (ymin + ymax)*0.5;
   double zmid = (zmin + zmax)*0.5;

   int coord2 = (coord1 + 1) % 3;
   int coord3 = (coord1 + 2) % 3;

   // sort (partition) points into eight octants
   conduit::index_t *p0 = beg, *p8 = end;
   conduit::index_t *p4 = std::partition(p0, p8, HilbertCmp(coord1,  dir1, points, xmid));
   conduit::index_t *p2 = std::partition(p0, p4, HilbertCmp(coord2,  dir2, points, ymid));
   conduit::index_t *p6 = std::partition(p4, p8, HilbertCmp(coord2, !dir2, points, ymid));
   conduit::index_t *p1 = std::partition(p0, p2, HilbertCmp(coord3,  dir3, points, zmid));
   conduit::index_t *p3 = std::partition(p2, p4, HilbertCmp(coord3, !dir3, points, zmid));
   conduit::index_t *p5 = std::partition(p4, p6, HilbertCmp(coord3,  dir3, points, zmid));
   conduit::index_t *p7 = std::partition(p6, p8, HilbertCmp(coord3, !dir3, points, zmid));

   if (p1 != p8)
   {
      HilbertSort3D(coord3, dir3, dir1, dir2, points, p0, p1,
                    zmin, xmin, ymin, zmid, xmid, ymid);
   }
   if (p1 != p0 || p2 != p8)
   {
      HilbertSort3D(coord2, dir2, dir3, dir1, points, p1, p2,
                    ymin, zmid, xmin, ymid, zmax, xmid);
   }
   if (p2 != p0 || p3 != p8)
   {
      HilbertSort3D(coord2, dir2, dir3, dir1, points, p2, p3,
                    ymid, zmid, xmin, ymax, zmax, xmid);
   }
   if (p3 != p0 || p4 != p8)
   {
      HilbertSort3D(coord1, dir1, !dir2, !dir3, points, p3, p4,
                    xmin, ymax, zmid, xmid, ymid, zmin);
   }
   if (p4 != p0 || p5 != p8)
   {
      HilbertSort3D(coord1, dir1, !dir2, !dir3, points, p4, p5,
                    xmid, ymax, zmid, xmax, ymid, zmin);
   }
   if (p5 != p0 || p6 != p8)
   {
      HilbertSort3D(coord2, !dir2, dir3, !dir1, points, p5, p6,
                    ymax, zmid, xmax, ymid, zmax, xmid);
   }
   if (p6 != p0 || p7 != p8)
   {
      HilbertSort3D(coord2, !dir2, dir3, !dir1, points, p6, p7,
                    ymid, zmid, xmax, ymin, zmax, xmid);
   }
   if (p7 != p0)
   {
      HilbertSort3D(coord3, !dir3, !dir1, dir2, points, p7, p8,
                    zmid, xmax, ymin, zmin, xmid, ymid);
   }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Adapted from MFEM
std::vector<conduit::index_t>
topology::hilbert_ordering(const conduit::Node &topo)
{
    // Make a new centroid topo and coordset. The coordset will contain the
    // element centers. This ought to be an explicit coordset.
    conduit::Node topo_dest, coords_dest, s2dmap, d2smap;
    conduit::blueprint::mesh::topology::unstructured::generate_centroids(topo,
                                                                         topo_dest,
                                                                         coords_dest,
                                                                         s2dmap,
                                                                         d2smap);

    const auto spaceDim = conduit::blueprint::mesh::coordset::dims(coords_dest);
    const auto nelem = conduit::blueprint::mesh::topology::length(topo);

    std::vector<conduit::index_t> indices(nelem);
    std::vector<double> points(3 * nelem, 0.);
    double min[3] = {0., 0., 0.}, max[3] = {0., 0., 0.};
    conduit::double_accessor acc[3];
    for (int i = 0; i < spaceDim; i++)
    {
        acc[i] = coords_dest["values"][i].as_double_accessor();
        min[i] = acc[i].min();
        max[i] = acc[i].max();
    }
    for (conduit::index_t i = 0; i < nelem; i++)
    {
        for (int j = 0; j < spaceDim; j++)
        {
            points[3*i + j] = acc[j][i];
        }
        indices[i] = i;
    }

    if (spaceDim == 1)
    {
        std::sort(indices.begin(), indices.end(), [&](int a, int b)
        { return points[3*a] < points[3*b]; });
    }
    else if (spaceDim == 2)
    {
        // Recursively partition the points in 2D
        detail::HilbertSort2D(0, false, false,
                      &points[0], indices.data(), indices.data() + indices.size(),
                      min[0], min[1], max[0], max[1]);
    }
    else
    {
        // Recursively partition the points in 3D
        detail::HilbertSort3D(0, false, false, false,
                    &points[0], indices.data(), indices.data() + indices.size(),
                    min[0], min[1], min[2], max[0], max[1], max[2]);
    }

    std::vector<conduit::index_t> ordering;
    ordering.resize(nelem);
    for (conduit::index_t i = 0; i < nelem; i++)
    {
        ordering[indices[i]] = i;
    }
    return ordering;
}

//---------------------------------------------------------------------------
topology::TopologyBuilder::TopologyBuilder(const conduit::Node &_topo) : topo(_topo),
    old_to_new(), topo_conn(), topo_sizes()
{

}

//---------------------------------------------------------------------------
topology::TopologyBuilder::TopologyBuilder(const conduit::Node *_topo) : topo(*_topo),
    old_to_new(), topo_conn(), topo_sizes()
{
    
}

//---------------------------------------------------------------------------
index_t
topology::TopologyBuilder::newPointId(index_t oldPointId)
{
    auto it = old_to_new.find(oldPointId);
    index_t newpt;
    if(it == old_to_new.end())
    {
        newpt = old_to_new.size();
        old_to_new[oldPointId] = newpt;
    }
    else
    {
        newpt = it->second;
    }
    return newpt;
}

//---------------------------------------------------------------------------
size_t
topology::TopologyBuilder::add(const index_t *ids, index_t nids)
{
    // Iterate over the ids and renumber them and add the renumbered points
    // to the new connectivity.
    size_t retval = topo_sizes.size();
    for(index_t i = 0; i < nids; i++)
    {
        index_t newpid = newPointId(ids[i]);
        topo_conn.push_back(newpid);
    }
    topo_sizes.push_back(nids);
    return retval;
}

//---------------------------------------------------------------------------
size_t
topology::TopologyBuilder::add(const std::vector<index_t> &ids)
{
    return add(&ids[0], ids.size());
}

//---------------------------------------------------------------------------
void
topology::TopologyBuilder::execute(conduit::Node &n_out, const std::string &shape)
{
    n_out.reset();

    // Get the topo and coordset names for the input topo.
    const conduit::Node &origcset = coordset(topo);
    std::string topoName(topo.name());
    std::string coordsetName(origcset.name());

    // Build the new topology.
    conduit::Node &newcset = n_out["coordsets/"+coordsetName];
    conduit::Node &newtopo = n_out["topologies/"+topoName];

    // Iterate over the selected original points and make a new coordset
    newcset["type"] = "explicit";
    auto axes = coordset::axes(origcset);
    auto npts = static_cast<index_t>(old_to_new.size());
    for(const auto &axis : axes)
    {
        std::string key("values/" + axis);
        auto acc = origcset[key].as_double_accessor();

        conduit::Node &coords = newcset[key];
        coords.set(DataType::float64(npts));
        auto coords_ptr = static_cast<double *>(coords.element_ptr(0));
        for(auto it = old_to_new.begin(); it != old_to_new.end(); it++)
        {
            coords_ptr[it->second] = acc[it->first];
        }
    }

    // Fill in the topo information.
    newtopo["type"] = "unstructured";
    newtopo["coordset"] = coordsetName;
    conduit::Node &n_ele = newtopo["elements"];
    n_ele["shape"] = shape;
    n_ele["connectivity"].set(topo_conn);
    n_ele["sizes"].set(topo_sizes);
    unstructured::generate_offsets_inline(newtopo);

    clear();
}

//---------------------------------------------------------------------------
void
topology::TopologyBuilder::clear()
{
    old_to_new.clear();
    topo_conn.clear();
    topo_sizes.clear();
}

//---------------------------------------------------------------------------
void
topology::extract_pointmesh(const Node &n_topo, const Node &n_ids, Node &n_output)
{
    const auto coordsetName = n_topo.fetch_existing("coordset").as_string();
    const Node *n_src_coordset = conduit::blueprint::mesh::utils::find_reference_node(n_topo, "coordset");
    CONDUIT_ASSERT(n_src_coordset != nullptr, "Source coordset not found");

    const auto axisNames = conduit::blueprint::mesh::utils::coordset::axes(*n_src_coordset);
    const auto nComps = axisNames.size();

    const auto ids = n_ids.as_index_t_accessor();
    const auto nIds = ids.number_of_elements();

    // Allocate outputs.
    Node &n_dest_coordset = n_output["coordsets/" + coordsetName];
    n_dest_coordset["type"] = "explicit";
    double_accessor src_coords[3];
    double *dest_coords[3] = {nullptr, nullptr, nullptr};
    for(size_t comp = 0; comp < axisNames.size(); comp++)
    {
        src_coords[comp] = n_src_coordset->fetch_existing("values/" + axisNames[comp]).as_double_accessor();

        Node &n_comp = n_dest_coordset["values"][axisNames[comp]];
        n_comp.set(conduit::DataType::float64(nIds));
        dest_coords[comp] = n_comp.as_float64_ptr();
    }
    Node &n_dest_topo = n_output["topologies/" + n_topo.name()];
    n_dest_topo["type"] = "unstructured";
    n_dest_topo["coordset"] = coordsetName;
    n_dest_topo["elements/shape"] = "point";
    n_dest_topo["elements/connectivity"].set(DataType::index_t(nIds));
    n_dest_topo["elements/sizes"].set(DataType::index_t(nIds));
    n_dest_topo["elements/offsets"].set(DataType::index_t(nIds));
    index_t *dest_conn = n_dest_topo["elements/connectivity"].as_index_t_ptr();
    index_t *dest_sizes = n_dest_topo["elements/sizes"].as_index_t_ptr();
    index_t *dest_offsets = n_dest_topo["elements/offsets"].as_index_t_ptr();

    // Fill in topo and coordset data.
    for(index_t i = 0; i < nIds; i++)
    {
        dest_conn[i] = i;
        dest_sizes[i] = 1;
        dest_offsets[i] = i;

        // Make the average coordinate.
        const bool unique = false;
        const auto ptids = topology::unstructured::points(n_topo, ids[i], unique);
        for(size_t comp = 0; comp < nComps; comp++)
        {
            double *coord = dest_coords[comp];
            coord[i] = 0.;
            for(const auto &ptid : ptids)
            {
                coord[i] += src_coords[comp][ptid];
            }
            coord[i] /= static_cast<double>(ptids.size());
        }
    }
}

//---------------------------------------------------------------------------
template <typename Body>
static void topology_iterate_elements(const conduit::Node &topo, Body &&body)
{
    const conduit::Node &n_topo_conn = topo["elements/connectivity"];
    const auto topo_conn = n_topo_conn.as_index_t_accessor();
    index_t idx = 0;
    std::vector<index_t> ids;
    if(topo.has_path("elements/sizes"))
    {
        // Variable sized shapes.
        const conduit::Node &n_topo_size = topo["elements/sizes"];
        const auto topo_size = n_topo_size.as_index_t_accessor();
        const index_t nelem = topo_size.dtype().number_of_elements();

        for(index_t i = 0; i < nelem; i++)
        {
            ids.clear();
            index_t s = topo_size[i];
            for(index_t pi = 0; pi < s; pi++)
                ids.push_back(topo_conn[idx++]);

            body(i, ids);
        }
    }
    else
    {
        // Homogeneous shapes
        const ShapeType shape(topo);
        const index_t nelem = topo_conn.dtype().number_of_elements() / shape.indices;

        for(index_t i = 0; i < nelem; i++)
        {
            ids.clear();
            for(index_t pi = 0; pi < shape.indices; pi++)
                ids.push_back(topo_conn[idx++]);

            body(i, ids);
        }
    }
}

//---------------------------------------------------------------------------
std::vector<int>
topology::search(const conduit::Node &topo1, const conduit::Node &topo2)
{
    // The domain_id is not too important in this case.
    int domain_id = 0;

    // Get the mesh for topo1 (2 levels up)
    const conduit::Node *mesh1 = nullptr;
    if(topo1.parent() != nullptr &&
       topo1.parent()->parent() != nullptr)
    {
        // Go from the topology up a couple levels to the mesh.
        mesh1 = topo1.parent()->parent();
    }
    else
    {
        CONDUIT_ERROR("No parent for topo1.");
    }

    // Iterate over mesh2's points to see if its points exist in mesh1.
    conduit::blueprint::mesh::utils::query::PointQuery P(*mesh1);
    const conduit::Node &cset = topology::coordset(topo1);
    const conduit::Node &cset2 = topology::coordset(topo2);
    index_t npts = conduit::blueprint::mesh::utils::coordset::length(cset2);
    for(index_t i = 0; i < npts; i++)
    {
        const std::vector<float64> pc = coordset::_explicit::coords(cset2, i);
        auto pclen = pc.size();
        double pt3[3];
        pt3[0] = pc[0];
        pt3[1] = (pclen > 1) ? pc[1] : 0.;
        pt3[2] = (pclen > 2) ? pc[2] : 0.;
        P.add(domain_id, pt3);
    }

    // Do the query.
    P.execute(cset.name());

    // Iterate over the entities in topo1 and make hash ids for the entities
    // by hashing their sorted points.
    std::map<uint64, index_t> topo1_entity_ids;
    topology_iterate_elements(topo1, [&](index_t i, std::vector<index_t> &ids)
    {
        std::sort(ids.begin(), ids.end());
        uint64 h = conduit::utils::hash(&ids[0], static_cast<unsigned int>(ids.size()));
        topo1_entity_ids[h] = i;
    });

    // Get the query results for each mesh2 point. This is a vector of point
    // ids from mesh 1 or NotFound.
    const auto &r = P.results(domain_id);

    // Iterate over the entities in mesh2 and map their points to mesh 1 points
    // if possible before computing hashids for them. If a mesh2 entity's points
    // can all be defined in mesh1 then the entity exists in both meshes.
    std::vector<int> exists;
    topology_iterate_elements(topo2, [&](index_t /*i*/, const std::vector<index_t> &topo2Ids)
    {
        // Try and map all of the topo2 entity's points to topo1's coordset.
        // If we can do that then the entity may exist.
        bool badEntity = false;
        std::vector<index_t> ids;
        ids.reserve(topo2Ids.size());
        for(const index_t &pt : topo2Ids)
        {
            // See if the point exists in mesh1.
            badEntity |= (r[pt] == P.NotFound);
            ids.push_back(r[pt]);
        }

        if(badEntity)
            exists.push_back(0);
        else
        {
            // The entity can be defined in terms of topo1's coordset. Make a
            // hash id for it and see if it matches any entities in topo1.
            std::sort(ids.begin(), ids.end());
            uint64 h = conduit::utils::hash(&ids[0], static_cast<unsigned int>(ids.size()));

            bool found = topo1_entity_ids.find(h) != topo1_entity_ids.end();
            exists.push_back(found ? 1 : 0);
        }
    });

    return exists;
}

//-----------------------------------------------------------------------------
void
topology::compute_mesh_info(const conduit::Node &n_topo, topology::MeshInfo &info)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // Get the coordset
    const Node *n_coordset_ptr = find_reference_node(n_topo, "coordset");
    if(n_coordset_ptr == nullptr)
    {
       CONDUIT_ERROR("Unable to get coordset for topology.");
    }
    const Node &n_coordset = *n_coordset_ptr;
    if(n_coordset.fetch_existing("type").as_string() != "explicit")
    {
       CONDUIT_ERROR("Requires explicit coordset");
    }

    // Make coordset accessors for the components.
    std::vector<double_accessor> coords;
    for(const auto &axisName : conduit::blueprint::mesh::utils::coordset::axes(n_coordset))
    {
       coords.push_back(n_coordset.fetch_existing("values/" + axisName).as_double_accessor());
    }

    // Initialize the mesh information.
    for(size_t i = 0; i < 3; i++)
    {
      info.minExtents[i] = (i < coords.size()) ? std::numeric_limits<double>::max() : 0.;
      info.maxExtents[i] = (i < coords.size()) ? std::numeric_limits<double>::lowest() : 0.;
    }
    info.minEdgeLength = std::numeric_limits<double>::max();
    info.maxEdgeLength = std::numeric_limits<double>::lowest();
    info.minDiagonalLength = std::numeric_limits<double>::max();
    info.maxDiagonalLength = std::numeric_limits<double>::lowest();

    // This lambda computes length information between p0, p1.
    auto computeEdgeInfo = [&](index_t p0, index_t p1)
    {
        double lenSquared = 0.;
        for(size_t comp = 0; comp < coords.size(); comp++)
        {
            const auto &acc = coords[comp];
            const auto v0 = acc[p0];
            const auto v1 = acc[p1];
            const auto dValue = v0 - v1;
            lenSquared += dValue * dValue;

            info.minExtents[comp] = std::min(info.minExtents[comp], std::min(v0, v1));
            info.maxExtents[comp] = std::max(info.maxExtents[comp], std::max(v0, v1));
        }

        if(lenSquared > CONDUIT_EPSILON)
        {
            info.minEdgeLength = std::min(info.minEdgeLength, lenSquared);
            info.maxEdgeLength = std::max(info.maxEdgeLength, lenSquared);
        }
    };

    auto diagLength = [&](index_t p0, index_t p1)
    {
        double lenSquared = 0.;
        for(size_t comp = 0; comp < coords.size(); comp++)
        {
            const auto &acc = coords[comp];
            const auto v0 = acc[p0];
            const auto v1 = acc[p1];
            const auto dValue = v0 - v1;
            lenSquared += dValue * dValue;
        }

        if(lenSquared > CONDUIT_EPSILON)
        {
            info.minDiagonalLength = std::min(info.minDiagonalLength, lenSquared);
            info.maxDiagonalLength = std::max(info.maxDiagonalLength, lenSquared);
        }
    };

    // This lambda computes diagonal information in a polygon.
    auto polygonDiagInfo = [&](const std::vector<index_t> &ids)
    {
        // Walk around the shape
        const index_t n = static_cast<index_t>(ids.size());
        const index_t m = n - 2;
        const index_t p = n - 3;
        for(index_t i = 0; i < m; i++)
        {
            for(index_t j = 0; j < p; j++)
            {
                const index_t jj = (j + 2) % n;
                if(i < jj)
                {
                    const index_t p0 = ids[i];
                    const index_t p1 = ids[jj];
                    diagLength(p0, p1);
                }
            }
        }
    };

    // This lambda computes diagonal information in a 3D zoo element
    auto shapeDiagInfo = [&](const std::vector<index_t> &ids)
    {
        switch(ids.size())
        {
        default:
            break;
        case 5: // pyramid
            diagLength(ids[0], ids[2]);
            diagLength(ids[1], ids[3]);
            break;
        case 6: // wedge
            diagLength(ids[0], ids[5]);
            diagLength(ids[2], ids[3]);
            diagLength(ids[2], ids[4]);
            diagLength(ids[1], ids[5]);
            diagLength(ids[0], ids[4]);
            diagLength(ids[1], ids[3]);
            break;
        case 8: // hex
            // interior diags
            diagLength(ids[0], ids[6]);
            diagLength(ids[1], ids[7]);
            diagLength(ids[2], ids[4]);
            diagLength(ids[3], ids[5]);
            // face diags
            diagLength(ids[4], ids[6]);
            diagLength(ids[5], ids[7]);

            diagLength(ids[2], ids[5]);
            diagLength(ids[1], ids[6]);

            diagLength(ids[0], ids[2]);
            diagLength(ids[1], ids[3]);

            diagLength(ids[0], ids[7]);
            diagLength(ids[3], ids[4]);

            diagLength(ids[0], ids[5]);
            diagLength(ids[1], ids[4]);

            diagLength(ids[2], ids[7]);
            diagLength(ids[3], ids[6]);
            break;
        }
    };

    // Iterate over all of the elements in the topology and compute the edge info
    // for each element.
    CONDUIT_ANNOTATE_MARK_BEGIN("edgeLength");
    iterate_elements(n_topo, [&](const entity &e)
    {
        // NOTE: For shapes where we do not compute the diagonal info, we just
        //       use the edge info as "good enough".
        if(e.shape.is_polyhedral())
        {
            for(size_t f = 0; f < e.subelement_ids.size(); f++)
            {
                const auto &faceIds = e.subelement_ids[f];
                index_t nIds = faceIds.size();
                for(index_t i = 0; i < nIds; i++)
                {
                    computeEdgeInfo(faceIds[i], faceIds[(i + 1) % nIds]);
                }
            }
        }
        else if(e.shape.dim == 3)
        {
            for(index_t f = 0; f < e.shape.num_faces(); f++)
            {
                index_t nIds;
                const auto *faceIds = e.shape.get_face(f, nIds);
                for(index_t i = 0; i < nIds; i++)
                {
                    computeEdgeInfo(e.element_ids[faceIds[i]],
                                    e.element_ids[faceIds[(i + 1) % nIds]]);
                }
            }
            // Compute diagonal info.
            shapeDiagInfo(e.element_ids);
        }
        else if(e.shape.dim == 2)
        {
            const index_t nIds = static_cast<index_t>(e.element_ids.size());
            for(index_t i = 0; i < nIds; i++)
            {
                computeEdgeInfo(e.element_ids[i],
                                e.element_ids[(i + 1) % nIds]);
            }

            // Compute diagonal info.
            if(nIds >= 4)
            {
               polygonDiagInfo(e.element_ids);
            }
        }
        else if(e.shape.dim == 1)
        {
            computeEdgeInfo(e.element_ids[0], e.element_ids[1]);
        }
        else if(e.shape.dim == 0)
        {
            computeEdgeInfo(e.element_ids[0], e.element_ids[0]);
        }
    });

    // If we had edges then the min,max edge lengths will be greater than zero.
    // Turn them from len squared to len by taking the square root.
    info.minEdgeLength = (info.minEdgeLength > 0.) ? sqrt(info.minEdgeLength) : 1.;
    info.maxEdgeLength = (info.maxEdgeLength > 0.) ? sqrt(info.maxEdgeLength) : 1.;

    // Initialize the diagonal lengths.
    if(info.minDiagonalLength == std::numeric_limits<double>::max())
    {
       // The value has not been set. Use minEdgeLength.
       info.minDiagonalLength = info.minEdgeLength;
    }
    else
    {
       info.minDiagonalLength = sqrt(info.minDiagonalLength);
    }
    if(info.maxDiagonalLength == std::numeric_limits<double>::lowest())
    {
       // The value has not been set. Use maxEdgeLength.
       info.maxDiagonalLength = info.maxEdgeLength;
    }
    else
    {
       info.maxDiagonalLength = sqrt(info.maxDiagonalLength);
    }
    CONDUIT_ANNOTATE_MARK_END("edgeLength");
}

//-----------------------------------------------------------------------------
namespace topology
{
std::ostream &operator << (std::ostream &os, const MeshInfo &obj)
{
    os << "{minExtents: {" << obj.minExtents[0] << ", " << obj.minExtents[1] << ", " << obj.minExtents[2] << "}, "
       << "maxExtents: {" << obj.maxExtents[0] << ", " << obj.maxExtents[1] << ", " << obj.maxExtents[2] << "}, "
       << "minEdgeLength: " << obj.minEdgeLength << ", "
       << "maxEdgeLength: " << obj.maxEdgeLength << ", "
       << "minDiagonalLength: " << obj.minDiagonalLength << ", "
       << "maxDiagonalLength: " << obj.maxDiagonalLength << "}";
    return os;
}

std::ostream &operator << (std::ostream &os, const MeshInfoCollection &obj)
{
    obj.print(os);
    return os;
}

} // end topology namespace

//-----------------------------------------------------------------------------
topology::MeshInfoCollection::MeshInfoCollection() : meshInfos(), mergedMeshInfo()
{
}

void
topology::MeshInfoCollection::clear()
{
    meshInfos.clear();
    mergedMeshInfo = MeshInfo();
}

void
topology::MeshInfoCollection::begin()
{
    clear();
}

void
topology::MeshInfoCollection::add(index_t domainId, const MeshInfo &info)
{
    meshInfos[domainId] = info;
}

void
topology::MeshInfoCollection::end()
{
    bool first = true;
    for(auto it = meshInfos.begin(); it != meshInfos.end(); it++)
    {
        if(first)
        {
            first = false;
            mergedMeshInfo = it->second;
        }
        else
        {
            mergedMeshInfo = merge(mergedMeshInfo, it->second);
        }
    }
}

const topology::MeshInfo &
topology::MeshInfoCollection::getMeshInfo(index_t domainId) const
{
    const auto it = meshInfos.find(domainId);
    if(it == meshInfos.end())
    {
        CONDUIT_ERROR("MeshInfo for domain " << domainId << " was not found.");
    }
    return it->second;
}

const topology::MeshInfo &
topology::MeshInfoCollection::getMergedMeshInfo() const
{
    return mergedMeshInfo;
}

topology::MeshInfo
topology::MeshInfoCollection::merge(const topology::MeshInfo &a, const topology::MeshInfo &b)
{
    MeshInfo info;
    info.minExtents[0] = std::min(a.minExtents[0], b.minExtents[0]);
    info.minExtents[1] = std::min(a.minExtents[1], b.minExtents[1]);
    info.minExtents[2] = std::min(a.minExtents[2], b.minExtents[2]);
    info.maxExtents[0] = std::max(a.maxExtents[0], b.maxExtents[0]);
    info.maxExtents[1] = std::max(a.maxExtents[1], b.maxExtents[1]);
    info.maxExtents[2] = std::max(a.maxExtents[2], b.maxExtents[2]);
    info.minEdgeLength = std::min(a.minEdgeLength, b.minEdgeLength);
    info.maxEdgeLength = std::max(a.maxEdgeLength, b.maxEdgeLength);
    info.minDiagonalLength = std::min(a.minDiagonalLength, b.minDiagonalLength);
    info.maxDiagonalLength = std::max(a.maxDiagonalLength, b.maxDiagonalLength);
    return info;
}

void
topology::MeshInfoCollection::print(std::ostream &os) const
{
    os << "meshInfos:\n";
    for(auto it = meshInfos.begin(); it != meshInfos.end(); it++)
    {
       os << "\t" << it->first << ": " << it->second << std::endl;
    }
    os << "mergedMeshInfo: " << mergedMeshInfo << std::endl;
}

//-----------------------------------------------------------------------------
template <typename T>
inline bool will_multiply_overflow(T a, T b)
{
    if (a == 0 || b == 0)
        return false;
    return b > (std::numeric_limits<T>::max() / a);
}

topology::Quantizer::Quantizer(const topology::MeshInfo &info) : meshInfo(info)
{
    // Take the smaller of the lengths.
    length = static_cast<FloatValue>(std::min(meshInfo.minDiagonalLength, meshInfo.minEdgeLength));
    offset = length / 2.;

    // Set some values derived from the MeshInfo.
    QX = 1 + std::max(QuantizedValue{1}, static_cast<QuantizedValue>(ceil((meshInfo.maxExtents[0] - meshInfo.minExtents[0]) / length)));
    QY = 1 + std::max(QuantizedValue{1}, static_cast<QuantizedValue>(ceil((meshInfo.maxExtents[1] - meshInfo.minExtents[1]) / length)));
    QXQY = QX * QY;
}

topology::Quantizer::QuantizedValue
topology::Quantizer::quantize(topology::Quantizer::FloatValue value, index_t component) const
{
#ifndef NDEBUG
    CONDUIT_ASSERT(component >= 0 && component < 3, "Valid components are 0,1,2");
#endif
    return static_cast<QuantizedValue>((value - static_cast<FloatValue>(meshInfo.minExtents[component]) + offset) / length);
}

topology::Quantizer::QuantizedIndex
topology::Quantizer::quantize(const topology::Quantizer::Coordinate &coord) const
{
    // NOTE: We check some of the multiplications with CONDUIT_ASSERT in case
    //       they will overflow when computing indices.
    //
    // FUTURE: If meshes get so large that computing an index overflows uint64_t
    //         then QuantizedIndex could become std::tuple<uint64_t,uint64_t>
    //         where the Z component gets stored in the 0th tuple value and
    //         the XY contributions get added in the 1st tuple value.
#ifndef NDEBUG
    CONDUIT_ASSERT(coord.size() <= 3, "Node has invalid number of components");
#endif
    QuantizedValue q = 0;
    QuantizedIndex qIndex = 0;
    switch(coord.size())
    {
    default:
        CONDUIT_ERROR("Unsupported number of components.");
        break;
    case 3:
        q = quantize(coord[2], 2);
#ifndef NDEBUG
        CONDUIT_ASSERT(!will_multiply_overflow(QXQY, q), "Quantizer overflow");
#endif
        qIndex += QXQY * q;
        // Falls through
    case 2:
        q = quantize(coord[1], 1);
#ifndef NDEBUG
        CONDUIT_ASSERT(!will_multiply_overflow(QX, q), "Quantizer overflow");
#endif
        qIndex += QX * q;
        // Falls through
    case 1:
        qIndex += quantize(coord[0], 0);
        break;
    }
    return qIndex;
}

namespace topology
{
std::ostream &
operator << (std::ostream &os, const Quantizer &obj)
{
    os << "{meshInfo: " << obj.meshInfo
      << ", length: " << obj.length
      << ", offset: " << obj.offset
      << ", QX: " << obj.QX
      << ", QY: " << obj.QY
      << ", QXQY: " << obj.QXQY
      << "}";
    return os;
}

} // end namespace topology

//-----------------------------------------------------------------------------
void
topology::unstructured::generate_offsets_inline(Node &topo)
{
    // check for polyhedral case
    if(topo.has_child("subelements"))
    {
        // if ele or subelee offsets are missing or empty we want to generate
        if( (!topo["elements"].has_child("offsets") || 
             topo["elements/offsets"].dtype().is_empty()) ||
           (!topo["subelements"].has_child("offsets") || 
             topo["subelements/offsets"].dtype().is_empty())
           )
        {
            blueprint::mesh::utils::topology::unstructured::generate_offsets(topo,
                                                                             topo["elements/offsets"],
                                                                             topo["subelements/offsets"]);
        }

    }
    else
    {
        // if ele offsets is missing or empty we want to generate
        if( !topo["elements"].has_child("offsets") || 
            topo["elements/offsets"].dtype().is_empty())
        {
            blueprint::mesh::utils::topology::unstructured::generate_offsets(topo,
                                                                             topo["elements/offsets"]);
        }
    }
}


//-----------------------------------------------------------------------------
void
topology::unstructured::generate_offsets(const Node &topo,
                                         Node &ele_offsets)
{
    Node subele_offsets;
    generate_offsets(topo,ele_offsets,subele_offsets);
}

//-----------------------------------------------------------------------------
void
topology::unstructured::generate_offsets(const Node &topo,
                                         Node &dest_ele_offsets,
                                         Node &dest_subele_offsets)
{
    const ShapeType topo_shape(topo);
    const DataType int_dtype = find_widest_dtype(topo, DEFAULT_INT_DTYPES);
    std::string key("elements/connectivity"), stream_key("elements/stream");

    if(!topo.has_path(key))
        key = stream_key;
    const Node &topo_conn = topo[key];

    const DataType topo_dtype(topo_conn.dtype().id(), 1, 0, 0,
        topo_conn.dtype().element_bytes(), topo_conn.dtype().endianness());

    bool elem_offsets_exist = topo["elements"].has_child("offsets") &&
                              !topo["elements/offsets"].dtype().is_empty();
    bool subelem_offsets_exist = false;

    // if these have already been generate, use set external to copy out results
    if(topo_shape.is_polyhedral())
    {
        subelem_offsets_exist = topo["subelements"].has_child("offsets") &&
                                !topo["subelements/offsets"].dtype().is_empty();
        if(elem_offsets_exist && subelem_offsets_exist)
        {
            // they are both already here, set external and return
            if(&dest_ele_offsets != &topo["elements/offsets"])
            {
                dest_ele_offsets.set_external(topo["elements/offsets"]);
            }
            if(&dest_subele_offsets != &topo["subelements/offsets"])
            {
                dest_subele_offsets.set_external(topo["subelements/offsets"]);
            }
            // we are done
            return;
        }
    }
    else // non polyhedral
    {
        if(elem_offsets_exist)
        {
            // they are already here, set external and return
            if(&dest_ele_offsets != &topo["elements/offsets"])
            {
                dest_ele_offsets.set_external(topo["elements/offsets"]);
            }
            return;
        }
    }

    // Selectively reset now that early returns have happened. We do the
    // checks for the polyhedral case since some offsets might exist.
    if(!elem_offsets_exist)
        dest_ele_offsets.reset();
    if(!subelem_offsets_exist)
        dest_subele_offsets.reset();

    ///
    /// Generate Cases
    ///
    if(topo.has_path(stream_key))
    {
        ///
        /// TODO STREAM TOPOS ARE DEPRECATED
        ///
        // Mixed element types
        std::map<int,int> stream_id_npts;
        const conduit::Node &n_element_types = topo["elements/element_types"];
        for(index_t i = 0; i < n_element_types.number_of_children(); i++)
        {
            const Node &n_et = n_element_types[i];
            auto stream_id = n_et["stream_id"].to_int();
            std::string shape(n_et["shape"].as_string());
            for(size_t j = 0; j < TOPO_SHAPES.size(); j++)
            {
                if(shape == TOPO_SHAPES[j])
                {
                    stream_id_npts[stream_id] = TOPO_SHAPE_EMBED_COUNTS[j];
                    break;
                }
            }
        }

        const Node &n_stream_ids = topo["elements/element_index/stream_ids"];
        std::vector<index_t> offsets;
        if(topo.has_path("elements/element_index/element_counts"))
        {
            const Node &n_element_counts = topo["elements/element_index/element_counts"];

            index_t offset = 0;
            for(index_t j = 0; j < n_stream_ids.dtype().number_of_elements(); j++)
            {
                // Get the j'th elements from n_stream_ids, n_element_counts
                const Node n_elem_ct_j(int_dtype,
                            const_cast<void*>(n_element_counts.element_ptr(j)), true);
                const Node n_stream_ids_j(int_dtype,
                            const_cast<void*>(n_stream_ids.element_ptr(j)), true);
                auto ec = static_cast<index_t>(n_elem_ct_j.to_int64());
                auto sid = static_cast<index_t>(n_stream_ids_j.to_int64());
                auto npts = stream_id_npts[sid];
                for(index_t i = 0; i < ec; i++)
                {
                    offsets.push_back(offset);
                    offset += npts;
                }
            }
        }
        else if(topo.has_path("elements/element_index/offsets"))
        {
            const Node &n_stream = topo["elements/stream"];
            const Node &n_element_offsets = topo["elements/element_index/offsets"];
            index_t offset = 0;
            for(index_t j = 0; j < n_stream_ids.dtype().number_of_elements(); j++)
            {
                // Get the j'th elements from n_stream_ids, n_element_offsets
                const Node n_stream_ids_j(int_dtype,
                            const_cast<void*>(n_stream_ids.element_ptr(j)), true);
                const Node n_element_offsets_j(int_dtype,
                            const_cast<void*>(n_element_offsets.element_ptr(j)), true);
                offset = n_element_offsets.to_index_t();
                index_t next_offset = offset;
                if(j == n_stream_ids.dtype().number_of_elements() - 1)
                {
                    next_offset = n_stream.dtype().number_of_elements();
                }
                else
                {
                    const Node n_element_offsets_j1(int_dtype,
                                const_cast<void*>(n_element_offsets.element_ptr(j)), true);
                    next_offset = n_element_offsets_j1.to_index_t();
                }
                const auto sid = static_cast<index_t>(n_stream_ids_j.to_int64());
                const auto npts = stream_id_npts[sid];
                while(offset < next_offset) {
                    offsets.push_back(offset);
                    offset += npts;
                }
            }
        }
        else
        {
            CONDUIT_ERROR("Stream based mixed topology has no element_counts or offsets.")
        }

        Node off_node;
        off_node.set_external(offsets);
        off_node.to_data_type(int_dtype.id(), dest_ele_offsets);
    }
    else if(!topo_shape.is_poly())
    {
        // Single element type
        const index_t num_topo_shapes =
            topo_conn.dtype().number_of_elements() / topo_shape.indices;

        Node shape_node(DataType::int64(num_topo_shapes));
        int64_array shape_array = shape_node.as_int64_array();
        for(index_t s = 0; s < num_topo_shapes; s++)
        {
            shape_array[s] = s * topo_shape.indices;
        }
        shape_node.to_data_type(int_dtype.id(), dest_ele_offsets);
    }
    else if(topo_shape.is_polygonal())
    {
        const Node &topo_size = topo["elements/sizes"];
        int64_accessor topo_sizes = topo_size.as_int64_accessor();
        std::vector<int64> shape_array;
        index_t i = 0;
        index_t s = 0;
        while(i < topo_size.dtype().number_of_elements())
        {
            shape_array.push_back(s);
            s += topo_sizes[i];
            i++;
        }

        Node shape_node;
        shape_node.set_external(shape_array);
        shape_node.to_data_type(int_dtype.id(), dest_ele_offsets);
    }
    else if(topo_shape.is_polyhedral())
    {
        // Construct any offsets that do not exist.
        if(!elem_offsets_exist)
        {
            index_t_accessor topo_elem_size = topo["elements/sizes"].value();
            index_t es_count = topo_elem_size.number_of_elements();

            dest_ele_offsets.set(DataType::index_t(es_count));
            index_t_array shape_array = dest_ele_offsets.value();

            index_t es = 0;
            for (index_t ei = 0; ei < es_count; ++ei)
            {
                shape_array[ei] = es;
                es += topo_elem_size[ei];
            }
        }
        if(!subelem_offsets_exist)
        {
            index_t_accessor topo_subelem_size = topo["subelements/sizes"].value();
            index_t ses_count = topo_subelem_size.number_of_elements();

            dest_subele_offsets.set(DataType::index_t(ses_count));
            index_t_array subshape_array = dest_subele_offsets.value();

            index_t ses = 0;
            for (index_t ei = 0; ei < ses_count; ++ei)
            {
                subshape_array[ei] = ses;
                ses += topo_subelem_size[ei];
            }
        }
    }
}


//-----------------------------------------------------------------------------
std::vector<index_t>
topology::unstructured::points(const Node &n,
                               const index_t ei,
                               bool unique)
{
    // NOTE(JRC): This is a workaround to ensure offsets are generated up-front
    // if they don't exist and aren't regenerated for each subcall that needs them.
    Node ntemp;
    ntemp.set_external(n);
    generate_offsets_inline(ntemp);

    const ShapeType topo_shape(ntemp);

    std::vector<index_t> retval;
    if(topo_shape.is_polyhedral())
    {
        // For polyhedra, the points returned will be unique.
        Node enode;
        std::set<index_t> eidxs;
        enode.set_external(ntemp["subelements"]);

        // Gather the face ids for the element.
        index_t_accessor eidxs_vals = ntemp["elements/connectivity"].value();
        o2mrelation::O2MIterator eiter(ntemp["elements"]);
        eiter.to(ei, o2mrelation::ONE);
        eiter.to_front(o2mrelation::MANY);
        while(eiter.has_next(o2mrelation::MANY))
        {
            eiter.next(o2mrelation::MANY);
            const index_t tmp = eidxs_vals[eiter.index(o2mrelation::DATA)];
            eidxs.insert(tmp);
        }

        // Iterate over the faces and make unique set of point ids.
        index_t_accessor pidxs_vals = enode["connectivity"].value();
        o2mrelation::O2MIterator piter(enode);
        std::set<index_t> pidxs;
        for(const index_t eidx : eidxs)
        {
            piter.to(eidx, o2mrelation::ONE);
            piter.to_front(o2mrelation::MANY);
            while(piter.has_next(o2mrelation::MANY))
            {
                piter.next(o2mrelation::MANY);
                const index_t tmp = pidxs_vals[piter.index(o2mrelation::DATA)];
                pidxs.insert(tmp);
            }
        }

        // Return point ids.
        retval = std::vector<index_t>(pidxs.begin(), pidxs.end());
    }
    else
    {
        const index_t_accessor poff_vals = ntemp["elements/offsets"].value();
        const index_t eoff = poff_vals[ei];

        // Determine the number of points for the shape.
        index_t npts;
        if(topo_shape.is_polygonal())
        {
            const index_t_accessor psize_vals = ntemp["elements/sizes"].value();
            npts = psize_vals[ei];
        }
        else
        {
            npts = topo_shape.indices;
        }

        // Get the points for the shape.
        index_t_accessor pidxs_vals = ntemp["elements/connectivity"].value();
        if(unique)
        {
            std::set<index_t> pidxs;
            for(index_t pi = 0; pi < npts; pi++)
            {
                const auto pid = pidxs_vals[eoff + pi];
                pidxs.insert(pid);
            }
            // Return point ids.
            retval = std::vector<index_t>(pidxs.begin(), pidxs.end());
        }
        else
        {
            retval.reserve(npts);
            for(index_t pi = 0; pi < npts; pi++)
            {
                const auto pid = pidxs_vals[eoff + pi];
                retval.push_back(pid);
            }
        }
    }

    return retval;
}

//-----------------------------------------------------------------------------
void
topology::unstructured::rewrite_connectivity(conduit::Node &btopo,
                                             const conduit::Node &coordset)
{
    const conduit::Node &bcoordset = topology::coordset(btopo);
    // Needed for PointQuery.
    const conduit::Node &mesh = *(coordset.parent()->parent());

    // Iterate over the boundary mesh coordinates and look them up in the
    // volume mesh's coordset.
    conduit::blueprint::mesh::utils::query::PointQuery Q(mesh);
    const auto axes = conduit::blueprint::mesh::utils::coordset::axes(bcoordset);
    const auto ndims = axes.size();
    const conduit::Node &bcvalues = bcoordset.fetch_existing("values");
    const int domain_id = static_cast<int>(find_domain_id(mesh));
    const auto bx = bcvalues[axes[0]].as_double_accessor();
    const auto by = bcvalues[axes[ndims > 1 ? 1 : 0]].as_double_accessor();
    const auto bz = bcvalues[axes[ndims > 2 ? 2 : 0]].as_double_accessor();
    conduit::index_t nSearchPoints = bx.number_of_elements();
    for (conduit::index_t i = 0; i < nSearchPoints; i++)
    {
       double pt[3];
       pt[0] = bx[i];
       pt[1] = ndims > 1 ? by[i] : 0.;
       pt[2] = ndims > 2 ? bz[i] : 0.;
       Q.add(domain_id, pt);
    }
    Q.execute(coordset.name());

    // Scan the points to see if there are any NotFound.
    const auto &res = Q.results(domain_id);
    for (conduit::index_t i = 0; i < nSearchPoints; i++)
    {
        if(res[i] == conduit::blueprint::mesh::utils::query::PointQuery::NotFound)
        {
            CONDUIT_ERROR("Point " << i << " was not found in new coordset " << coordset.name() << ".");
        }
    }

    // Make a new the boundary topology that uses the volume mesh coordset.
    // We remap the connectivity.
    const auto &n_bconnSrc = btopo["elements/connectivity"];
    const auto bconnSrc = n_bconnSrc.as_index_t_accessor();
    conduit::index_t nbconn = bconnSrc.number_of_elements();
    conduit::Node newconn;
    newconn.set(conduit::DataType(n_bconnSrc.dtype().id(), nbconn));
    if(newconn.dtype().is_int32())
    {
        auto bconnNew = newconn.as_int32_array();
        for (conduit::index_t i = 0; i < nbconn; i++)
            bconnNew[i] = static_cast<conduit::int32>(res[bconnSrc[i]]);
    }
    else if(newconn.dtype().is_int64())
    {
        auto bconnNew = newconn.as_int64_array();
        for (conduit::index_t i = 0; i < nbconn; i++)
            bconnNew[i] = static_cast<conduit::int64>(res[bconnSrc[i]]);
    }
    else if(newconn.dtype().is_uint32())
    {
        auto bconnNew = newconn.as_uint32_array();
        for (conduit::index_t i = 0; i < nbconn; i++)
            bconnNew[i] = static_cast<conduit::uint32>(res[bconnSrc[i]]);
    }
    else if(newconn.dtype().is_uint64())
    {
        auto bconnNew = newconn.as_uint64_array();
        for (conduit::index_t i = 0; i < nbconn; i++)
            bconnNew[i] = static_cast<conduit::uint64>(res[bconnSrc[i]]);
    }
    else if(newconn.dtype().is_index_t())
    {
        auto bconnNew = newconn.as_index_t_array();
        for (conduit::index_t i = 0; i < nbconn; i++)
            bconnNew[i] = static_cast<conduit::index_t>(res[bconnSrc[i]]);
    }
    else
    {
        CONDUIT_ERROR(newconn.dtype().name() << " is not supported for connectivity.");
    }
    btopo["coordset"] = coordset.name();
    btopo["elements/connectivity"].move(newconn);
}

//-----------------------------------------------------------------------------
void
topology::unstructured::reorder(const conduit::Node &topo,
                                const conduit::Node &coordset,
                                const conduit::Node &fields,
                                const std::vector<conduit::index_t> &reorder,
                                conduit::Node &dest_topo,
                                conduit::Node &dest_coordset,
                                conduit::Node &dest_fields,
                                std::vector<conduit::index_t> &old2NewPoints)
{
    conduit::blueprint::mesh::utils::ShapeType shape(topo);

    // Handle unstructured meshes (but not polyhedral meshes yet)
    if(topo.fetch_existing("type").as_string() == "unstructured" && !shape.is_polyhedral())
    {
        // Input connectivity information.
        const auto &n_conn = topo.fetch_existing("elements/connectivity");
        const auto &n_sizes = topo.fetch_existing("elements/sizes");
        const auto &n_offsets = topo.fetch_existing("elements/offsets");
        const auto conn = n_conn.as_index_t_accessor();
        const auto sizes = n_sizes.as_index_t_accessor();
        const auto offsets = n_offsets.as_index_t_accessor();

        // Temp vectors to store reordered connectivity. We use temp vectors so
        // we can convert to a matching datatype after we've constructed the data.
        std::vector<conduit::index_t> newconn, newoffsets, newsizes;
        newconn.reserve(conn.number_of_elements());
        newsizes.reserve(sizes.number_of_elements());
        newoffsets.reserve(offsets.number_of_elements());

        // Mapping information for the points.
        constexpr conduit::index_t invalidNode = -1;
        auto npts = conduit::blueprint::mesh::coordset::length(coordset);
        // Fill in the old2New point mapping. It gets passed out of the function.
        old2NewPoints.resize(npts);
        for(conduit::index_t i = 0; i < npts; i++)
            old2NewPoints[i] = invalidNode;
        // ptReorder is used to reorder/slice vertex-associated data. We'll allow
        // up to npts values but it might not be that large if we are selecting a
        // subset of elements.
        std::vector<conduit::index_t> ptReorder;
        ptReorder.reserve(npts);
        conduit::index_t newPointIndex = 0;

        // We iterate over elements in the specified order. We iterate over the
        // points in each element and renumber the points.
        conduit::index_t newoffset = 0;
        for(const auto cellIndex : reorder)
        {
            for(conduit::index_t i = 0; i < sizes[cellIndex]; i++)
            {
                auto id = conn[offsets[cellIndex] + i];
                // if the old point has not been seen, renumber it.
                if(old2NewPoints[id] == invalidNode)
                {
                    ptReorder.push_back(id);
                    old2NewPoints[id] = newPointIndex++;
                }
                newconn.push_back(old2NewPoints[id]);
            }
            newsizes.push_back(sizes[cellIndex]);
            newoffsets.push_back(newoffset);
            newoffset += sizes[cellIndex];
        }

        // Store the new connectivity.
        dest_topo["type"] = topo["type"];
        dest_topo["coordset"] = dest_coordset.name();
        dest_topo["elements/shape"] = topo["elements/shape"];
        conduit::Node tmp;
        tmp.set_external(newconn.data(), newconn.size());
        tmp.to_data_type(n_conn.dtype().id(), dest_topo["elements/connectivity"]);
        tmp.set_external(newsizes.data(), newsizes.size());
        tmp.to_data_type(n_sizes.dtype().id(), dest_topo["elements/sizes"]);
        tmp.set_external(newoffsets.data(), newoffsets.size());
        tmp.to_data_type(n_offsets.dtype().id(), dest_topo["elements/offsets"]);

        // Reorder the coordset now, making it explicit if needed.
        dest_coordset["type"] = "explicit";
        conduit::Node coordset_explicit;
        conduit::blueprint::mesh::coordset::to_explicit(coordset, coordset_explicit);
        conduit::blueprint::mesh::utils::slice_field(coordset_explicit["values"], ptReorder, dest_coordset["values"]);

        // Reorder fields that match this topo.
        std::vector<std::string> fieldNames;
        for(conduit::index_t fi = 0; fi < fields.number_of_children(); fi++)
        {
            const conduit::Node &src = fields[fi];
            if(src["topology"].as_string() == topo.name())
            {
                fieldNames.push_back(src.name());
            }
        }
        for(const auto &fieldName : fieldNames)
        {
            const conduit::Node &src = fields.fetch_existing(fieldName);
            conduit::Node &dest = dest_fields[fieldName];
            dest["association"] = src["association"];
            dest["topology"] = dest_topo.name();
            if(dest["association"].as_string() == "element")
            {
                conduit::blueprint::mesh::utils::slice_field(src["values"], reorder, dest["values"]);
            }
            else
            {
                conduit::blueprint::mesh::utils::slice_field(src["values"], ptReorder, dest["values"]);
            }
        }

        // TODO: renumber adjsets.
    }
}

//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::topology --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::adjset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void
adjset::canonicalize(Node &adjset)
{
    const index_t domain_id = find_domain_id(adjset);

    const std::vector<std::string> &adjset_group_names = adjset["groups"].child_names();
    for(const std::string &old_group_name : adjset_group_names)
    {
        const Node &group_node = adjset["groups"][old_group_name];
        const Node &neighbors_node = group_node["neighbors"];

        std::string new_group_name;
        {
            std::ostringstream oss;
            oss << conduit::blueprint::mesh::adjset::group_prefix();

            Node temp;
            DataType temp_dtype(neighbors_node.dtype().id(), 1);

            // NOTE(JRC): Need to use a vector instead of direct 'Node::to_index_t'
            // because the local node ID isn't included in the neighbor list and
            // 'DataArray' uses a static array size.
            std::vector<index_t> group_neighbors(1, domain_id);
            for(index_t ni = 0; ni < neighbors_node.dtype().number_of_elements(); ni++)
            {
                temp.set_external(temp_dtype, (void*)neighbors_node.element_ptr(ni));
                group_neighbors.push_back(temp.to_index_t());
            }
            std::sort(group_neighbors.begin(), group_neighbors.end());

            for(const index_t &neighbor_id : group_neighbors)
            {
                oss << "_" << neighbor_id;
            }

            new_group_name = oss.str();
        }

        if(old_group_name != new_group_name)
            adjset["groups"].rename_child(old_group_name, new_group_name);
    }
}

//---------------------------------------------------------------------------
bool adjset::is_canonical(const Node &adjset)
{
    bool retval = true;
    const conduit::Node &groups = adjset.fetch_existing("groups");
    for(conduit::index_t i = 0; i < groups.number_of_children() && retval; i++)
    {
        auto pos = groups[i].name().find(conduit::blueprint::mesh::adjset::group_prefix());

        bool canonical = (pos == 0);
        if(canonical)
        {
            std::vector<std::string> tokens;
            conduit::utils::split_string(groups[i].name(), '_', tokens);
            canonical = tokens.size() >= 3;
            if(canonical)
            {
                std::vector<int> doms;
                doms.reserve(tokens.size() - 1);
                // Start at 1 to skip the groups prefix.
                for(size_t j = 1; j < tokens.size(); j++)
                    doms.push_back(atoi(tokens[j].c_str()));
                // Start at 1 because we'll compare pairs. Make sure domains are sorted.
                for(size_t j = 1; j < doms.size(); j++)
                    canonical &= doms[j - 1] < doms[j];
            }
        }

        retval &= canonical;
    }
    return retval;
}

//---------------------------------------------------------------------------
bool
adjset::validate(const conduit::Node &doms,
                 const std::string &adjsetName,
                 conduit::Node &info,
                 const conduit::Node &options)
{
    auto to_string = [](const conduit::Node &n) -> std::string
    {
        std::string s(n.to_string());
        if(s.find("\"") == 0)
            s = s.substr(1, s.size() - 2);
        return s;
    };

    // Set values from options.
    double tolerance = conduit::blueprint::mesh::utils::query::PointQueryBase::DEFAULT_POINT_TOLERANCE;
    if(options.has_path("tolerance"))
    {
        tolerance = options["tolerance"].to_float64();
    }

    // Create serial queries.
    query::PointQuery PQ(doms);
    PQ.setPointTolerance(tolerance);
    query::MatchQuery MQ(doms);

    // We need to figure out the association, topologyName, and coordsetName.
    std::string association, topologyName, coordsetName;
    auto domains = conduit::blueprint::mesh::domains(doms);
    if(!domains.empty())
    {
        const auto dom = domains[0];

        std::string adjsetPath("adjsets/" + adjsetName);
        const conduit::Node &adjset = dom->fetch_existing(adjsetPath);

        association = to_string(adjset.fetch_existing("association"));
        topologyName = to_string(adjset.fetch_existing("topology"));

        const conduit::Node &topo = dom->fetch_existing("topologies/"+topologyName);
        coordsetName = to_string(topo["coordset"]);
    }

    const bool checkMultiDomain = true;
    return adjset::validate(doms,
                            adjsetName, association, topologyName, coordsetName,
                            info, PQ, MQ, checkMultiDomain);
}

//---------------------------------------------------------------------------
template <typename StreamType, typename ContainerType>
void print_point(StreamType &s, const ContainerType &container)
{
     s << "(";
     for(size_t i = 0; i < container.size(); i++)
     {
         if(i > 0)
         {
             s << ", ";
         }
         s << container[i];
     }
     s << ")";
}

//---------------------------------------------------------------------------
bool
adjset::validate(const conduit::Node &doms,
                 const std::string &adjsetName,
                 const std::string &association,
                 const std::string &topologyName,
                 const std::string &coordsetName,
                 conduit::Node &info,
                 query::PointQuery &PQ,
                 query::MatchQuery &MQ,
                 bool checkMultiDomain)
{
    bool retval = false;

    // Make sure that there are multiple domains. Note that we do this
    // test in serial but not parallel.
    if(checkMultiDomain && !conduit::blueprint::mesh::is_multi_domain(doms))
    {
        info["errors"].append().set("The dataset is not multidomain.");
        return retval;
    }

    // Ensure adjset exists in all domains. This still succeeds when there
    // are no domains.
    auto domains = conduit::blueprint::mesh::domains(doms);
    std::string adjsetPath("adjsets/" + adjsetName);
    size_t count = 0;
    for(size_t i = 0; i < domains.size(); i++)
    {
        if(domains[i]->has_path(adjsetPath))
            count++;
        else
        {
            std::stringstream ss;
            ss << "Domain " << i << " lacks adjset " << adjsetName;
            info["errors"].append().set(ss.str());
        }
    }
    if(count != domains.size())
        return retval;

    if(association == "vertex")
    {
        // Iterate over the domains so we can add their adjset points to the
        // point query.
        std::vector<std::tuple<index_t, index_t, index_t, index_t, size_t, std::string, std::vector<double>>> query_guide;
        std::map<std::pair<index_t,index_t>, bool> bidirectional;
        for(size_t domIdx = 0; domIdx < domains.size(); domIdx++)
        {
            const auto dom = domains[domIdx];
            auto domainId = conduit::blueprint::mesh::utils::find_domain_id(*dom);

            // Get the domain's coordset.
            const conduit::Node &coordset = dom->fetch_existing("coordsets/"+coordsetName);

            // Get the domain's adjset and groups.
            const conduit::Node &adjset = dom->fetch_existing(adjsetPath);
            const conduit::Node &adjset_groups = adjset.fetch_existing("groups");

            // Iterate over this domain's adjset to help build up the point query.
            for(const std::string &groupName : adjset_groups.child_names())
            {
                const conduit::Node &src_group = adjset_groups[groupName];
                conduit::index_t_accessor src_neighbors = src_group["neighbors"].value();
                conduit::index_t_accessor src_values = src_group["values"].value();

                // Neighbors
                for(index_t ni = 0; ni < src_neighbors.dtype().number_of_elements(); ni++)
                {
                    auto nbr = src_neighbors[ni];

                    // Record some information on the bidirectional nature of the interface.
                    auto domainToNeighbor = std::make_pair(domainId, nbr);
                    auto neighborToDomain = std::make_pair(nbr, domainId);
                    bidirectional[domainToNeighbor] = true;
                    if(bidirectional.find(neighborToDomain) == bidirectional.end())
                        bidirectional[neighborToDomain] = false; // not yet found

                    // Point ids
                    for(index_t pi = 0; pi < src_values.dtype().number_of_elements(); pi++)
                    {
                        // Look up the point in the local coordset to get the coordinate.
                        auto ptid = src_values[pi];
                        auto pt = conduit::blueprint::mesh::utils::coordset::_explicit::coords(coordset, ptid);
                        double pt3[3];
                        pt3[0] = pt[0];
                        pt3[1] = (pt.size() > 1) ? pt[1] : 0.;
                        pt3[2] = (pt.size() > 2) ? pt[2] : 0.;

                        // Ask domain nbr if they have point pt3
                        auto idx = PQ.add(static_cast<int>(nbr), pt3);
                        query_guide.emplace_back(domainId, ptid, nbr, idx, domIdx, groupName, pt);
                    }
                }
            }
        }

        retval = true;

        // NOTE: We can only make this check in serial because when this method is
        //       called in parallel, we do not have all of the domains on the same
        //       rank.
        if(checkMultiDomain)
        {
            // Detect any non-bidirectional interfaces.
            for(auto it = bidirectional.begin(); it != bidirectional.end(); it++)
            {
                if(!it->second)
                {
                    retval = false;

                    auto domain_id = it->first.first;
                    auto neighbor_id = it->first.second;
                    std::stringstream ss;
                    ss << "Domain " << domain_id << " adjset " << adjsetName
                       << " does not contain a group that references domain " << neighbor_id << ".";

                    conduit::Node &vn = info[adjsetName].append();
                    vn.set(ss.str());
                }
            }
        }

        // Execute the query.
        PQ.execute(coordsetName);

        // Iterate over the query results to flag any problems.
        for(const auto &obj : query_guide)
        {
            const index_t domain_id = std::get<0>(obj);
            const index_t ptid = std::get<1>(obj);
            const index_t nbr = std::get<2>(obj);
            const index_t idx = std::get<3>(obj);
            const size_t domIdx = std::get<4>(obj);
            const std::string &groupName = std::get<5>(obj);
            const std::vector<double> &coord = std::get<6>(obj);

            const auto &res = PQ.results(static_cast<int>(nbr));
            if(res[idx] == conduit::blueprint::mesh::utils::query::PointQuery::NotFound)
            {
                retval = false;
                std::string domainName(domains[domIdx]->name());
                if(domainName.empty())
                {
                    domainName = conduit_fmt::format("domain_{:05}", domain_id);
                }
                conduit::Node &vn = info[domainName][adjsetName][groupName].append();

                std::stringstream ss;
                ss << "Domain " << domain_id << " adjset " << adjsetName
                   << " group " << groupName
                   << ": vertex " << ptid << " ";
                print_point(ss, coord);
                ss << " at index " << idx
                   << " could not be located in neighbor domain "
                   << nbr << ".";

                vn["message"].set(ss.str());
                vn["vertex"] = ptid;
                vn["neighbor"] = nbr;
                vn["coordinate"] = coord;
            }
        }
    }
    else if(association == "element")
    {
        // Here we are dealing with an element adjset. An element adjset
        // contains the element ids for the topology elements that touch
        // a neighbor domain. If the adjset is valid, the neighbor will
        // have a corresponding entity that matches up with the element.
        // If that is not the case then the adjset is not valid.

        // Set up MatchQuery that will examine the extdoms domains.
        MQ.selectTopology(topologyName);

        std::vector<std::tuple<int, int, int, conduit::uint64, size_t, std::string>> query_guide;
        for(size_t domIdx = 0; domIdx < domains.size(); domIdx++)
        {
            const auto dom = domains[domIdx];
            auto domain_id = conduit::blueprint::mesh::utils::find_domain_id(*dom);

            // Get the domain's adjset and groups.
            const conduit::Node &adjset = dom->fetch_existing(adjsetPath);
            const conduit::Node &adjset_groups = adjset.fetch_existing("groups");

            // Get the domain's topo.
            const conduit::Node &topo = dom->fetch_existing("topologies/"+topologyName);

            // Get the number of elements in the topology.
            index_t topo_len = conduit::blueprint::mesh::utils::topology::length(topo);

            // Iterate over the adjset data to build up the query.
            for(const std::string &groupName : adjset_groups.child_names())
            {
                const conduit::Node &group = adjset_groups[groupName];
                conduit::index_t_accessor neighbors = group["neighbors"].value();
                conduit::index_t_accessor values    = group["values"].value();

                for(index_t ni = 0; ni < neighbors.dtype().number_of_elements(); ni++)
                {
                    index_t nbr = neighbors[ni];
                    for(index_t vi = 0; vi < values.dtype().number_of_elements(); vi++)
                    {
                        index_t ei = values[vi];

                        // Get the points for the element id if the element id is valid.
                        std::vector<index_t> entity_pidxs;
                        if(ei >= 0 && ei < topo_len)
                            entity_pidxs = conduit::blueprint::mesh::utils::topology::unstructured::points(topo, ei);

                        // Add the entity to the query for consideration.
                        conduit::uint64 qid = MQ.add(static_cast<int>(domain_id),
                                                     static_cast<int>(nbr),
                                                     entity_pidxs);

                        // Add the candidate entity to the match query, which
                        // will help resolve things across domains.
                        query_guide.push_back(std::make_tuple(domain_id, nbr, ei, qid, domIdx, groupName));
                    }
                }
            }
        }

        // Execute the query.
        MQ.execute();

        // Iterate over the query results to flag any problems.
        retval = true;
        for(const auto &obj : query_guide)
        {
            int domain_id = std::get<0>(obj);
            int nbr = std::get<1>(obj);
            int ei = std::get<2>(obj);
            conduit::uint64 eid = std::get<3>(obj);
            size_t domIdx = std::get<4>(obj);
            const std::string &groupName = std::get<5>(obj);

            if(!MQ.exists(domain_id, nbr, eid))
            {
                retval = false;
                std::string domainName(domains[domIdx]->name());
                conduit::Node &vn = info[domainName][adjsetName][groupName].append();

                std::stringstream ss;
                ss << "Domain " << domain_id << " adjset " << adjsetName
                   << " group " << groupName
                   << ": element " << ei << " could not be located in neighbor domain "
                   << nbr << ".";

                vn["message"].set(ss.str());
                vn["element"] = ei;
                vn["neighbor"] = nbr;
            }
        }
    }
    else
    {
        info["errors"].append().
            set(std::string("Unsupported association: ") + association);
    }

    return retval;
}

//-----------------------------------------------------------------------------
void adjset::to_pairwise_canonical(conduit::Node &doms,
                                   const std::string &adjsetName,
                                   const std::string &newAdjsetName)
{
    std::vector<Node *> domains = conduit::blueprint::mesh::domains(doms);
    for(auto &domPtr : domains)
    {
        Node &domain = *domPtr;
        const std::string src_key("adjsets/" + adjsetName);
        if(domain.has_path(src_key))
        {
            const std::string dest_key("adjsets/" + newAdjsetName);              
            const conduit::Node &src_adj = domain[src_key];
            conduit::Node &dest_adj = domain[dest_key];
            if(conduit::blueprint::mesh::adjset::is_pairwise(src_adj))
            {
                if(adjset::is_canonical(src_adj))
                {
                    // Keep an external reference to adjset a since it is already pairwise.
                    dest_adj.set_external(src_adj);
                }
                else
                {
                    dest_adj["topology"] = src_adj["topology"];
                    dest_adj["association"] = src_adj["association"];
                    // Shallow copy the group data into the new adjset.
                    const conduit::Node &src_groups = src_adj["groups"];
                    conduit::Node &dest_groups = dest_adj["groups"];
                    for(conduit::index_t i = 0; i < src_groups.number_of_children(); i++)
                    {
                        const conduit::Node &src_group = src_groups[i];
                        conduit::Node &dest_group = dest_groups[src_group.name()];
                        dest_group["neighbors"].set_external(src_group["neighbors"]);
                        dest_group["values"].set_external(src_group["values"]);
                    }
                    conduit::blueprint::mesh::utils::adjset::canonicalize(dest_adj);
                }
            }
            else
            {
                conduit::blueprint::mesh::adjset::to_pairwise(src_adj, dest_adj);
                conduit::blueprint::mesh::utils::adjset::canonicalize(dest_adj);
            }
        }
    }
}

//-----------------------------------------------------------------------------
/// Removes the query adjset from each domain.
void adjset::remove(conduit::Node &doms,
                    const std::string &adjsetName)
{
    std::vector<Node *> domains = conduit::blueprint::mesh::domains(doms);
    const std::string key("adjsets/" + adjsetName);
    for(auto &domPtr : domains)
    {
        if(domPtr->has_path(key))
            domPtr->remove(key);
    }
}

//-----------------------------------------------------------------------------
template <typename ExtractFunc, typename CompareFunc>
bool
foreach_adjset_mesh_pair_impl(conduit::Node &mesh, const std::string &adjsetName,
    ExtractFunc &&extractFunc, CompareFunc &&compareFunc)
{
    namespace bputils = conduit::blueprint::mesh::utils;
    bool retval = true;

    // Determine total number of domains.
    std::vector<Node *> domains = conduit::blueprint::mesh::domains(mesh);
    const int maxDomains = static_cast<int>(domains.size());

    // Iterate over each of the possible adjset relationships. Not all of these
    // will have adjset groups.
    for(int d0 = 0; d0 < maxDomains; d0++)
    {
        for(int d1 = d0 + 1; d1 < maxDomains; d1++)
        {
            // make the adjset group name.
            std::stringstream ss;
            ss << conduit::blueprint::mesh::adjset::group_prefix() << "_" << d0 << "_" << d1;
            std::string groupName(ss.str());

            // There are up to 2 meshes for the shared boundary.
            conduit::Node boundaries[2];
            int mi = 0;
            for(size_t dom = 0; dom < domains.size(); dom++)
            {
                const Node &domain = *domains[dom];

                // If the domain has the adjset, make a point mesh of its points
                // that we can send to the neighbor.
                std::string akey("adjsets/" + adjsetName);
                std::string key(akey + "/groups/" + groupName + "/values");
                if(domain.has_path(key))
                {
                    const Node &n_adjset = domain[akey];
                    const std::string association = n_adjset["association"].as_string();

                    // Get the topology that the adjset wants.
                    std::string topoName = n_adjset.fetch_existing("topology").as_string();
                    const Node &topo = domain.fetch_existing("topologies/" + topoName);

                    // Get the group values and add them as points to the topo builder
                    // so we pull out a point mesh.
                    const Node &n_values = domain.fetch_existing(key);
                    extractFunc(topo, n_adjset, n_values, boundaries[mi]);

                    // Add the adjset values as a field.
                    if(association == "vertex")
                    {
                        conduit::Node &n_field = boundaries[mi]["fields/vertex_ids"];
                        n_field["topology"] = topoName;
                        n_field["association"] = association;
                        n_field["values"].set_external(n_values);
                    }

                    mi++;
                }
            }

            // Incorporate input from the supplied function. We need to have 2 meshes.
            if(mi == 2)
            {
                retval &= compareFunc(groupName, d0, boundaries[0], d1, boundaries[1]);
            }
        }
    }

    return retval;
}

//-----------------------------------------------------------------------------
template <typename ExtractFunc, typename CompareFunc>
bool
foreach_adjset_mesh_pair(conduit::Node &mesh, const std::string &adjsetName,
    ExtractFunc &&extractFunc, CompareFunc &&compareFunc)
{
    bool retval = true;
    const std::string tempAdjsetName("__" + adjsetName + "__");

    try
    {
        // Make sure we have a suitable query adjset.
        adjset::to_pairwise_canonical(mesh, adjsetName, tempAdjsetName);

        // Call the real implementation on the temporary adjset.
        retval = foreach_adjset_mesh_pair_impl(mesh, tempAdjsetName, extractFunc, compareFunc);

        // Remove the adjset that was added.
        adjset::remove(mesh, tempAdjsetName);
    }
    catch(...)
    {
        // Remove the adjset that was added.
        adjset::remove(mesh, tempAdjsetName);
        // Rethrow the exception.
        throw;
    }

    return retval;
}

//-----------------------------------------------------------------------------
bool
adjset::compare_pointwise(conduit::Node &mesh, const std::string &adjsetName,
    conduit::Node &info, const conduit::Node &options)
{
    // Set values from options.
    double tolerance = conduit::blueprint::mesh::utils::query::PointQueryBase::DEFAULT_POINT_TOLERANCE;
    if(options.has_path("tolerance"))
    {
        tolerance = options["tolerance"].to_float64();
    }

    // Figures out a coordset axis path within a diff hierarchy.
    auto coordPath = [&](const std::string &axisName)
    {
        std::stringstream ss;
        ss << "/children/diff/values/children/diff/" << axisName;
        return ss.str();
    };

    // If there are infodiff differences in the coordset, add information about differences.
    auto logDifferences = [&](const conduit::Node &infodiff,
                              const std::string &groupName,
                              int dom1,
                              const conduit::Node &mesh1,
                              int dom2,
                              const conduit::Node &/*mesh2*/)
    {
        // We know that the way this is called, there is 1 coordset and 1 topology.
        const conduit::Node &n_coordset = mesh1["coordsets"][0];
        const std::string coordsetName = n_coordset.name();

        // Check to see whether any of the coordinate diff arrays are different.
        const auto axisNames = conduit::blueprint::mesh::utils::coordset::axes(n_coordset);
        bool coordinatesDiffer = false;
        for(size_t i = 0; i < axisNames.size(); i++)
        {
            const auto validPath = coordPath(axisNames[i]) + "/valid";
            coordinatesDiffer |= infodiff.has_path(validPath) ?
                (infodiff.fetch_existing(validPath).as_string() == "false") : false;
        }

        if(coordinatesDiffer)
        {
            // Get the diff coordinate values in accessors.
            double_accessor coords[3];
            int ncomps = 0;
            for(size_t i = 0; i < axisNames.size(); i++)
            {
                const auto valuePath = coordPath(axisNames[i]) + "/value";
                if(infodiff.has_path(valuePath))
                {
                    coords[i] = infodiff.fetch_existing(valuePath).as_double_accessor();
                    ncomps++;
                }
            }

            if(ncomps == static_cast<int>(axisNames.size()))
            {
                const index_t domain_id = dom1;
                const index_t nbr = dom2;

                // Get the original mesh's vertex ids from a field.
                const auto vertex_ids = mesh1.fetch_existing("fields/vertex_ids/values").as_index_t_accessor();

                // Make a node in which to stash the differences.
                std::stringstream dss;
                dss << domain_id;
                std::string domainName = dss.str();
                conduit::Node &n_info_group = info[domainName][adjsetName][groupName];

                // For each node that differs, add information to info.
                const index_t nnodes = coords[0].number_of_elements();
                for(index_t idx = 0; idx < nnodes; idx++)
                {
                    bool nodeDiffers = false;
                    for(int d = 0; d < ncomps; d++)
                    {
                        nodeDiffers |= (coords[d][idx] > tolerance);
                    }

                    if(nodeDiffers)
                    {
                        const index_t ptid = vertex_ids[idx];
                        const auto coord = coordset::_explicit::coords(n_coordset, idx);

                        conduit::Node &vn = n_info_group.append();
                        std::stringstream ss;
                        ss << "Domain " << domain_id << " adjset " << adjsetName
                           << " group " << groupName
                           << ": vertex " << ptid << " ";
                        print_point(ss, coord);
                        ss << " at index " << idx
                           << " could not be located in neighbor domain "
                           << dom2 << ".";

                        vn["message"].set(ss.str());
                        vn["vertex"] = ptid;
                        vn["neighbor"] = nbr;
                        vn["coordinate"] = coord;
                    }
                }
            }
        }
    };

    auto extractMesh =
        [](const conduit::Node &n_topo,
           const conduit::Node &n_adjset,
           const conduit::Node &n_values, // values for an adjset group.
           conduit::Node &n_output)
    {
        const std::string association = n_adjset["association"].as_string();

        if(association == "element" &&
           n_topo.fetch_existing("type").as_string() == "unstructured" &&
           n_topo.fetch_existing("elements/shape").as_string() == "line")
        {
            // Special case - treat element adjsets for line meshes as points so
            // we can compare them in point order.
            topology::extract_pointmesh(n_topo, n_values, n_output);
        }
        else
        {
            topology::TopologyBuilder B(n_topo);
            const auto ids = n_values.as_index_t_accessor();
            for(index_t i = 0; i < ids.number_of_elements(); i++)
            {
                index_t ptid = ids[i];
                B.add(&ptid, 1);
            }

            // Make the local point mesh.
            B.execute(n_output, "point");
         }
    };

    auto compareMesh =
        [&](const std::string &groupName, int dom1, conduit::Node &mesh1, int dom2, conduit::Node &mesh2)
    {
        bool retval = false;
        if(mesh1.has_path("coordsets") && mesh2.has_path("coordsets"))
        {
            const conduit::Node cs1 = mesh1["coordsets"][0];
            const conduit::Node cs2 = mesh2["coordsets"][0];

            // Make sure the coordsets are not different.
            conduit::Node infodiff;
            const bool different = cs1.diff(cs2, infodiff, tolerance);

            // Add some diagnostic info.
            if(different)
            {
                logDifferences(infodiff, groupName, dom1, mesh1, dom2, mesh2);
            }

            retval = !different;
        }
        return retval;
    };

    return foreach_adjset_mesh_pair(mesh, adjsetName, extractMesh, compareMesh);
}

//-----------------------------------------------------------------------------
void
adjset::to_topo(conduit::Node &mesh, const std::string &adjsetName, conduit::Node &out)
{
    auto moveMesh = [&](const std::string &groupName, int dom, conduit::Node &n_mesh, conduit::Node &n_out)
    {
        if(n_mesh.has_path("coordsets") && n_mesh.has_path("topologies"))
        {
            // Copy the mesh into out node in a way that preserves its name.
            std::stringstream ss;
            ss << adjsetName << "_" << dom << "_" << groupName;
            std::string meshName(ss.str());

            conduit::Node &coordsets = n_out["coordsets"];
            conduit::Node &topologies = n_out["topologies"];
            conduit::Node &cset = coordsets[meshName+"_coords"];
            cset.move(n_mesh["coordsets"][0]);
            conduit::Node &topo = topologies[meshName];
            topo.move(n_mesh["topologies"][0]);
            topo["coordset"] = meshName + "_coords";
        }
    };

    auto extractMesh =
        [](const conduit::Node &n_topo,
           const conduit::Node &n_adjset,
           const conduit::Node &n_values,
           conduit::Node &n_output)
    {
        const std::string association = n_adjset["association"].as_string();

        const auto values = n_values.as_index_t_accessor();
        topology::TopologyBuilder B(n_topo);
        std::string shapeType;
        if(association == "vertex")
        {
            shapeType = "point";
            for(index_t i = 0; i < values.number_of_elements(); i++)
            {
                index_t ptid = values[i];
                B.add(&ptid, 1);
            }
        }
        else if(association == "element")
        {
            size_t minSides = 0, maxSides = 0;
            for(index_t i = 0; i < values.number_of_elements(); i++)
            {
                index_t ei = values[i];
                // Get the element points in their original order.
                const bool unique = false;
                const auto ptids = topology::unstructured::points(n_topo, ei, unique);
                B.add(&ptids[0], ptids.size());

                minSides = (i == 0) ? ptids.size() : std::min(minSides, ptids.size());
                maxSides = (i == 0) ? ptids.size() : std::max(maxSides, ptids.size());
            }

            if(minSides == maxSides)
            {
                if(minSides == 2)
                    shapeType = "line";
                else if(minSides == 3)
                    shapeType = "tri";
                else if(minSides == 4)
                    shapeType = "quad";
            }
            if(shapeType.empty())
                shapeType = "polygon";
        }

        // Make the local point mesh.
        B.execute(n_output, shapeType);
    };

    // This compareMesh moves the meshes into the output node.
    auto compareMesh =
        [&](const std::string &groupName, int dom1, conduit::Node &mesh1, int dom2, conduit::Node &mesh2)
    {
        moveMesh(groupName, dom1, mesh1, out);
        moveMesh(groupName, dom2, mesh2, out);
        return true;
    };

    foreach_adjset_mesh_pair(mesh, adjsetName, extractMesh, compareMesh);
}

//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::adjset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::query --
//-----------------------------------------------------------------------------
namespace query
{

const int PointQueryBase::NotFound = -1;
const double PointQueryBase::DEFAULT_POINT_TOLERANCE = 1.e-8;

//---------------------------------------------------------------------------
PointQueryBase::PointQueryBase(const conduit::Node &mesh) : m_mesh(mesh),
    m_domInputs(), m_domResults()
{
}

//---------------------------------------------------------------------------
void
PointQueryBase::reset()
{
    m_domInputs.clear();
    m_domResults.clear();
}

//---------------------------------------------------------------------------
conduit::index_t
PointQueryBase::add(int dom, const double pt[3])
{
    std::vector<double> &coords = m_domInputs[dom];
    conduit::index_t idx = coords.size() / 3;
    coords.push_back(pt[0]);
    coords.push_back(pt[1]);
    coords.push_back(pt[2]);
    return idx;
}

//---------------------------------------------------------------------------
const std::vector<double> &
PointQueryBase::inputs(int dom) const
{
    auto it = m_domInputs.find(dom);
    if(it == m_domInputs.end())
    {
        CONDUIT_ERROR("Domain " << dom << " inputs were requested but not found.");
    }
    return it->second;
}

//---------------------------------------------------------------------------
const std::vector<int> &
PointQueryBase::results(int dom) const
{
    auto it = m_domResults.find(dom);
    if(it == m_domResults.end())
    {
        CONDUIT_ERROR("Domain " << dom << " results were requested but not found.");
    }
    return it->second;
}

//---------------------------------------------------------------------------
void
PointQueryBase::execute(const std::string & /*coordsetName*/)
{
    for(auto it = m_domInputs.begin(); it != m_domInputs.end(); it++)
    {
        size_t npts = it->second.size() / 3;
        std::vector<int> &result = m_domResults[it->first];
        result.resize(npts, 0); // success is anything != NotFound
    }
}

//---------------------------------------------------------------------------
std::vector<int>
PointQueryBase::queryDomainIds() const
{
    std::vector<int> retval;
    for(auto it = m_domInputs.begin(); it != m_domInputs.end(); it++)
        retval.push_back(it->first);
    return retval;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

// NOTE: The exact threshold could be platform-specific. This is in the ballpark.
const int PointQuery::SEARCH_THRESHOLD = 25 * 25 * 25;

//---------------------------------------------------------------------------
PointQuery::PointQuery(const conduit::Node &mesh) : PointQueryBase(mesh)
{
    setPointTolerance(DEFAULT_POINT_TOLERANCE);
}

//---------------------------------------------------------------------------
void
PointQuery::setPointTolerance(double tolerance)
{
    m_pointTolerance = std::max(tolerance, std::numeric_limits<double>::epsilon());
}

//---------------------------------------------------------------------------
void
PointQuery::execute(const std::string &coordsetName)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    for(auto it = m_domInputs.begin(); it != m_domInputs.end(); it++)
    {
        const conduit::Node *dom = getDomain(it->first);
        if(dom == nullptr)
        {
            CONDUIT_ERROR("Domain " << it->first << " was requested but not found.");
        }

        const std::vector<double> &input = it->second;
        std::vector<int> &result = m_domResults[it->first];
        findPointsInDomain(*dom, coordsetName, input, result);
    }
}

//---------------------------------------------------------------------------
const conduit::Node *
PointQuery::getDomain(int dom) const
{
    if(is_multi_domain(m_mesh))
    {
        std::vector<const conduit::Node *> doms = domains(m_mesh);
        for(const auto d : doms)
        {
            if(d->has_path("state/domain_id"))
            {
                int domain_id = d->fetch_existing("state/domain_id").to_int();
                if(domain_id == dom)
                    return d;
            }
        }
        return nullptr;
    }
    // single domain, return the mesh.
    return &m_mesh;
}

//---------------------------------------------------------------------------
void
PointQuery::findPointsInDomain(const conduit::Node &mesh,
    const std::string &coordsetName,
    const std::vector<double> &input,
    std::vector<int> &result) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    conduit::index_t numInputPts = input.size() / 3;
    result.resize(numInputPts, NotFound);

    // Get the coords that will be used for queries.
    const conduit::Node &cset = mesh.fetch_existing("coordsets/" + coordsetName);
    const conduit::Node *coords[3] = {nullptr, nullptr, nullptr};
    conduit::index_t coordTypes[3];
    std::vector<std::string> axes(coordset::axes(cset));
    int ndims = 0;
    const conduit::Node &cvals = cset.fetch_existing("values");
    for(const std::string &axis : axes)
    {
        coords[ndims] = cvals.fetch_ptr(axis);
        coordTypes[ndims] = coords[ndims]->dtype().id();
        ndims++;
    }
    if(coords[0] == nullptr)
    {
        CONDUIT_ERROR("Coordinates not found in coordset " << coordsetName << ".");
        // result is full of NotFound.
        return;
    }
    // Check whether all of the coordinate types are the same.
    bool sameTypes = true;
    for(int i = 1; i < ndims; i++)
        sameTypes &= (coordTypes[0] == coordTypes[i]);

    // Try an accelerated search first, if it applies.
    if(!acceleratedSearch(ndims, sameTypes, coords, coordTypes, input, result))
    {
        // AcceleratedSearch did not handle it. Do normal search.
        normalSearch(ndims, sameTypes, coords, coordTypes, input, result);
    }
}

//---------------------------------------------------------------------------
bool
PointQuery::acceleratedSearch(int ndims,
    bool sameTypes,
    const conduit::Node *coords[3],
    const conduit::index_t coordTypes[3],
    const std::vector<double> &input,
    std::vector<int> &result) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    bool handled = false;

    conduit::index_t numInputPts = input.size() / 3;
    const double *input_ptr = &input[0];
    int *result_ptr = &result[0];
    conduit::index_t numCoordsetPts = coords[0]->dtype().number_of_elements();

#if defined(CONDUIT_USE_OPENMP)
    using policy = conduit::execution::OpenMPExec;
#else
    using policy = conduit::execution::SerialExec;
#endif

    // Special case a few large searches where the types are the same.
    if(ndims == 3 &&
       sameTypes &&
       numCoordsetPts >= SEARCH_THRESHOLD &&
       coordTypes[0] == conduit::DataType::FLOAT64_ID)
    {
        // 3D points are all doubles.
        float64_array typedCoords[3];
        typedCoords[0] = coords[0]->as_float64_array();
        typedCoords[1] = coords[1]->as_float64_array();
        typedCoords[2] = coords[2]->as_float64_array();
        conduit::blueprint::mesh::utils::kdtree<float64_array, float64, 3> search;
        search.initialize(typedCoords, numCoordsetPts);
        search.setPointTolerance(m_pointTolerance);
        conduit::execution::for_all<policy>(0, numInputPts, [&](conduit::index_t i)
        {
            float64 searchPt[3] = {static_cast<float64>(input_ptr[i * 3 + 0]),
                                   static_cast<float64>(input_ptr[i * 3 + 1]),
                                   static_cast<float64>(input_ptr[i * 3 + 2])};
            auto found = static_cast<int>(search.findPoint(searchPt));
            result_ptr[i] = (found != search.NotFound) ? found : NotFound;
        });
        handled = true;
    }
    else if(ndims == 3 &&
            sameTypes &&
            numCoordsetPts >= SEARCH_THRESHOLD &&
            coordTypes[0] == conduit::DataType::FLOAT32_ID)
    {
        // 3D points are all float.
        float32_array typedCoords[3];
        typedCoords[0] = coords[0]->as_float32_array();
        typedCoords[1] = coords[1]->as_float32_array();
        typedCoords[2] = coords[2]->as_float32_array();
        conduit::blueprint::mesh::utils::kdtree<float32_array, float32, 3> search;
        search.initialize(typedCoords, numCoordsetPts);
        search.setPointTolerance(static_cast<float32>(m_pointTolerance));
        conduit::execution::for_all<policy>(0, numInputPts, [&](conduit::index_t i)
        {
            float32 searchPt[3] = {static_cast<float32>(input_ptr[i * 3 + 0]),
                                   static_cast<float32>(input_ptr[i * 3 + 1]),
                                   static_cast<float32>(input_ptr[i * 3 + 2])};
            auto found = static_cast<int>(search.findPoint(searchPt));
            result_ptr[i] = (found != search.NotFound) ? found : NotFound;
        });
        handled = true;
    }
    // Large searches of 2D coordinates.
    else if(ndims == 2 &&
            sameTypes &&
            numCoordsetPts >= SEARCH_THRESHOLD &&
            coordTypes[0] == conduit::DataType::FLOAT64_ID)
    {
        // 2D points are all doubles.
        float64_array typedCoords[2];
        typedCoords[0] = coords[0]->as_float64_array();
        typedCoords[1] = coords[1]->as_float64_array();
        conduit::blueprint::mesh::utils::kdtree<float64_array, float64, 2> search;
        search.initialize(typedCoords, numCoordsetPts);
        search.setPointTolerance(m_pointTolerance);
        conduit::execution::for_all<policy>(0, numInputPts, [&](conduit::index_t i)
        {
            float64 searchPt[2] = {static_cast<float64>(input_ptr[i * 3 + 0]),
                                   static_cast<float64>(input_ptr[i * 3 + 1])};
            auto found = static_cast<int>(search.findPoint(searchPt));
            result_ptr[i] = (found != search.NotFound) ? found : NotFound;
        });
        handled = true;
    }
    else if(ndims == 2 &&
            sameTypes &&
            numCoordsetPts >= SEARCH_THRESHOLD &&
            coordTypes[0] == conduit::DataType::FLOAT32_ID)
    {
        // 2D points are all float.
        float32_array typedCoords[2];
        typedCoords[0] = coords[0]->as_float32_array();
        typedCoords[1] = coords[1]->as_float32_array();
        conduit::blueprint::mesh::utils::kdtree<float32_array, float32, 2> search;
        search.initialize(typedCoords, numCoordsetPts);
        search.setPointTolerance(static_cast<float32>(m_pointTolerance));
        conduit::execution::for_all<policy>(0, numInputPts, [&](conduit::index_t i)
        {
            float32 searchPt[2] = {static_cast<float32>(input_ptr[i * 3 + 0]),
                                   static_cast<float32>(input_ptr[i * 3 + 1])};
            auto found = static_cast<int>(search.findPoint(searchPt));
            result_ptr[i] = (found != search.NotFound) ? found : NotFound;
        });
        handled = true;
    }
    return handled;
}

//---------------------------------------------------------------------------
bool
PointQuery::normalSearch(int ndims,
    bool /*sameTypes*/,
    const conduit::Node *coords[3],
    const conduit::index_t /*coordTypes*/[3],
    const std::vector<double> &input,
    std::vector<int> &result) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    bool handled = false;

    conduit::index_t numInputPts = input.size() / 3;
    const double *input_ptr = &input[0];
    int *result_ptr = &result[0];
    conduit::index_t numCoordsetPts = coords[0]->dtype().number_of_elements();
    double EPS_SQ = m_pointTolerance * m_pointTolerance;

#if defined(CONDUIT_USE_OPENMP)
    using policy = conduit::execution::OpenMPExec;
#else
    using policy = conduit::execution::SerialExec;
#endif

    // Back up to a brute force search
    if(ndims == 3)
    {
        conduit::execution::for_all<policy>(0, numInputPts, [&](conduit::index_t i)
        {
            const double *searchPt = &input_ptr[i * 3];
            int found = NotFound;
            const auto x = coords[0]->as_double_accessor();
            const auto y = coords[1]->as_double_accessor();
            const auto z = coords[2]->as_double_accessor();
            for(conduit::index_t ptid = 0; ptid < numCoordsetPts; ptid++)
            {
                double dx = x[ptid] - searchPt[0];
                double dy = y[ptid] - searchPt[1];
                double dz = z[ptid] - searchPt[2];
                double dSquared = dx * dx + dy * dy + dz * dz;
                if(dSquared < EPS_SQ)
                {
                    found = static_cast<int>(ptid);
                    break;
                }
            }
            result_ptr[i] = found;
        });
        handled = true;
    }
    else if(ndims == 2)
    {
        conduit::execution::for_all<policy>(0, numInputPts, [&](conduit::index_t i)
        {
            const double *searchPt = &input_ptr[i * 3];
            int found = NotFound;
            const auto x = coords[0]->as_double_accessor();
            const auto y = coords[1]->as_double_accessor();
            for(conduit::index_t ptid = 0; ptid < numCoordsetPts; ptid++)
            {
                double dx = x[ptid] - searchPt[0];
                double dy = y[ptid] - searchPt[1];
                double dSquared = dx * dx + dy * dy;
                if(dSquared < EPS_SQ)
                {
                    found = static_cast<int>(ptid);
                    break;
                }
            }
            result_ptr[i] = found;
        });
        handled = true;
    }
    else if(ndims == 1)
    {
        conduit::execution::for_all<policy>(0, numInputPts, [&](conduit::index_t i)
        {
            const double *searchPt = &input_ptr[i * 3];
            int found = NotFound;
            const auto x = coords[0]->as_double_accessor();
            for(conduit::index_t ptid = 0; ptid < numCoordsetPts; ptid++)
            {
                double dx = x[ptid] - searchPt[0];
                double dSquared = dx * dx;
                if(dSquared < EPS_SQ)
                {
                    found = static_cast<int>(ptid);
                    break;
                }
            }
            result_ptr[i] = found;
        });
        handled = true;
    }
    return handled;
}

//---------------------------------------------------------------------------
std::vector<int>
PointQuery::domainIds() const
{
    std::vector<const conduit::Node *> doms = domains(m_mesh);
    std::vector<int> domainIds;
    domainIds.reserve(doms.size());
    for(const auto d : doms)
    {
        int domain_id = 0;
        if(d->has_path("state/domain_id"))
            domain_id = d->fetch_existing("state/domain_id").to_int();
        domainIds.push_back(domain_id); 
    }

    return domainIds;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MatchQuery::MatchQuery(const conduit::Node &mesh) : m_mesh(mesh),
    m_topoName(), m_query()
{
}

//---------------------------------------------------------------------------
void
MatchQuery::selectTopology(const std::string &name)
{
    m_topoName = name;
}

//---------------------------------------------------------------------------
const conduit::Node *
MatchQuery::getDomainTopology(int domain) const
{
    auto doms = domains(m_mesh);
    for(const auto &dom : doms)
    {
        if(dom->has_path("state/domain_id"))
        {
            int domain_id = dom->fetch_existing("state/domain_id").to_int();
            if(domain_id == domain)
            {
                const conduit::Node &topos = dom->fetch_existing("topologies");
                if(!m_topoName.empty())
                {
                    if(topos.has_child(m_topoName))
                        return topos.fetch_ptr(m_topoName);
                    else
                    {
                        CONDUIT_ERROR("Topology " << m_topoName
                            << " was not found in domain " << domain);
                    }
                }
                else
                {
                    return topos.child_ptr(0);
                }
            }
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------
size_t
MatchQuery::add(int dom, int query_dom, const index_t *ids, index_t nids)
{
    auto key = std::make_pair(dom, query_dom);
    auto it = m_query.find(key);
    if(it == m_query.end())
    {
        const conduit::Node *dtopo = getDomainTopology(dom);
        auto &q = m_query[key];
        q.builder = std::make_shared<topology::TopologyBuilder>(dtopo);
        it = m_query.find(key);
    }
    return it->second.builder->add(ids, nids);
}

//---------------------------------------------------------------------------
size_t
MatchQuery::add(int dom, int query_dom, const std::vector<index_t> &ids)
{
    auto key = std::make_pair(dom, query_dom);
    auto it = m_query.find(key);
    if(it == m_query.end())
    {
        const conduit::Node *dtopo = getDomainTopology(dom);
        auto &q = m_query[key];
        q.builder = std::make_shared<topology::TopologyBuilder>(dtopo);
        it = m_query.find(key);
    }
    return it->second.builder->add(ids);
}

//---------------------------------------------------------------------------
bool
MatchQuery::exists(int dom, int query_dom, size_t ei) const
{
    auto key = std::make_pair(dom, query_dom);
    auto it = m_query.find(key);
    if(it == m_query.end())
    {
        CONDUIT_ERROR("MatchQuery is missing the results for "
            << dom << ":" << query_dom);
    }
    bool exists = false;
    if(ei < it->second.results.size())
    {
        exists = it->second.results[ei] > 0;
    }
    return exists;
}

//---------------------------------------------------------------------------
void
MatchQuery::execute()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // Build the query geometries. Store them in the query_mesh node.
    std::string shape;
    for(auto it = m_query.begin(); it != m_query.end(); it++)
    {
        int dom = it->first.first;

        if(shape.empty())
        {
            // We have not determined the shape yet. Do that now so the subset
            // topologies can be built.
            const auto dtopo = getDomainTopology(dom);
            ShapeCascade c(*dtopo);
            const auto &s = c.get_shape((c.dim == 0) ? c.dim : (c.dim - 1));
            shape = s.type();
        }

        it->second.builder->execute(it->second.query_mesh, shape);
        it->second.query_mesh["state/domain_id"] = dom;
    }

    // Now that the query geometries are built, on a single rank, we should
    // have A,B and B,A in the keys.
    for(auto it = m_query.begin(); it != m_query.end(); it++)
    {
        int dom = it->first.first;
        int query_domain = it->first.second;

        // Try and get the opposite side's geometry.
        auto oppositeKey = std::make_pair(query_domain, dom);
        auto oppit = m_query.find(oppositeKey);
        if(oppit == m_query.end())
        {
            CONDUIT_ERROR("MatchQuery is missing the topology for "
                << dom << ":" << query_domain);
        }

        // Get both of the topologies.
        const conduit::Node &mesh1 = it->second.query_mesh;
        const conduit::Node &mesh2 = oppit->second.query_mesh;
        const std::string topoKey("topologies/" + m_topoName);
        const conduit::Node &topo1 = mesh1[topoKey];
        const conduit::Node &topo2 = mesh2[topoKey];

        // Perform the search and store the results.
        it->second.results = topology::search(topo2, topo1);
    }
}

//---------------------------------------------------------------------------
std::vector<std::pair<int,int>>
MatchQuery::queryDomainIds() const
{
    std::vector<std::pair<int,int>> ids;
    for(auto it = m_query.begin(); it != m_query.end(); it++)
    {
        // If we issued the request on this rank, the topo builder will not be nullptr.
        if(it->second.builder)
            ids.push_back(it->first);
    }
    return ids;
}

//---------------------------------------------------------------------------
const std::vector<int> &
MatchQuery::results(int dom, int query_dom) const
{
    auto it = m_query.find(std::make_pair(dom, query_dom));
    if(it == m_query.end())
    {
        CONDUIT_ERROR("Results are not available for query "
            << dom << ", " << query_dom);
    }
    return it->second.results;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::query --
//-----------------------------------------------------------------------------

void CONDUIT_BLUEPRINT_API lerp_one(const Node& A,
                                    const Node& B,
                                    int n,
                                    Node& out,
                                    int base,
                                    bool allocate)
{
    if (n < 0)
    {
        CONDUIT_ERROR("Linear interpolation requires output of at least one point.  "
            "Requested n is " << n);
    }

    const int Asize = static_cast<int>(A.dtype().number_of_elements());
    const int Bsize = static_cast<int>(B.dtype().number_of_elements());
    if (Asize < 1 || Bsize != Asize)
    {
        CONDUIT_ERROR("Linear interpolation requires same number of components "
            "(greater than zero) in points A and B.  A has " << Asize <<
            " and B has " << Bsize << " components.");
    }
    const int component_count = Asize;

    if (allocate)
    {
        out.reset();
        for (int c = 0; c < component_count; ++c)
        {
            out.append();
        }
    }
    else if (out.number_of_children() != component_count)
    {
        CONDUIT_ERROR("Attempted to linearly interpolate A (" << Asize <<
            " components) and B (" << Bsize << " components) storing the result "
            "into a Node with " << out.number_of_children() << " children.  "
            "Please make sure this matches the component count or pass "
            "allocate == true.");
    }

    const float64* A_ptr = A.value();
    const float64* B_ptr = B.value();
    NodeIterator comp_itr = out.children();
    for (int c = 0; c < component_count; ++c)
    {
        Node& comp = comp_itr.next();
        if (allocate) {
            comp.reset();   // do we need this?
            comp.set(DataType::float64(n)); 
        }
        float64* comp_ptr = comp.value();
        double delta = 0.;
        if (n > 1)
        {
            delta = (B_ptr[c] - A_ptr[c]) / (n - 1);
        }

        comp_ptr[base] = A_ptr[c];
        for (int i = 1; i < n; ++i)
        {
            comp_ptr[base + i] = comp_ptr[base + i - 1] + delta;
        }
    }
}

void CONDUIT_BLUEPRINT_API lerp(const Node& As,
                                const Node& Bs,
                                int n,
                                Node& out)
{
    if (As.dtype().is_number() && Bs.dtype().is_number())
    {
        lerp_one(As, Bs, n, out);
    }
    else
    {
        const int dims = As.number_of_children();
        if (dims < 1 || Bs.number_of_children() != dims)
        {
            CONDUIT_ERROR("Linear interpolation requires same dimensionality "
                "(greater than zero) in point lists As and Bs.  As has dimension " <<
                dims << " and Bs has dimension " << Bs.number_of_children() << ".");
        }

        const int num_segments = As.child(0).dtype().number_of_elements();
        if (num_segments < 1 || Bs.child(0).dtype().number_of_elements() != num_segments)
        {
            CONDUIT_ERROR("Linear interpolation requires same number of points "
                "(greater than zero) in point lists As and Bs.  As has " << num_segments <<
                " and Bs has " << Bs.child(0).dtype().number_of_elements() << " points.");
        }

        out.reset();
        for (int c = 0; c < dims; ++c)
        {
            Node& comp = out.append();
            comp.set(DataType::float64(num_segments * n));
        }

        int offset = 0;
        for (int p = 0; p < num_segments; ++p)
        {
            Node tempa;
            tempa.set(DataType::float64(dims));
            double* ptempa = tempa.value();
            Node tempb;
            tempb.set(DataType::float64(dims));
            double* ptempb = tempb.value();

            for (int d = 0; d < dims; ++d)
            {
                const double_array pAcomp = As.child(d).as_double_array();
                const double_array pBcomp = Bs.child(d).as_double_array();

                ptempa[d] = pAcomp[p];
                ptempb[d] = pBcomp[p];
            }
            lerp_one(tempa, tempb, n, out, offset, false);

            offset += n;
        }
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

