// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_utils_iterate_elements.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_UTILS_ITERATE_ELEMENTS_HPP
#define CONDUIT_BLUEPRINT_MESH_UTILS_ITERATE_ELEMENTS_HPP

// Internal utility header

//-----------------------------------------------------------------------------
// std includes
//-----------------------------------------------------------------------------
#include <algorithm>
#include <array>
#include <utility>


//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_mesh_utils.hpp"

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
// -- begin conduit::blueprint::mesh::utils::topology --
//-----------------------------------------------------------------------------
namespace topology
{

struct entity
{
    ShapeType                  shape;
    std::vector<index_t>              element_ids;
    std::vector<std::vector<index_t>> subelement_ids;
    index_t                           entity_id; // Local entity id.
};

// Q: Should this exist in conduit_blueprint_mesh_utils.hpp ?
// static const std::vector<std::string> TOPO_SHAPES = {"point", "line", "tri",
//        "quad", "tet", "hex", "wedge", "pyramid", "polygonal", "polyhedral", "mixed"};
enum class ShapeId : index_t
{
    Point      = 0,
    Line       = 1,
    Tri        = 2,
    Quad       = 3,
    Tet        = 4,
    Hex        = 5,
    Wedge      = 6,
    Pyramid    = 7,
    Polygonal  = 8,
    Polyhedral = 9,
    Mixed      = 10
};

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::topology::impl --
//-----------------------------------------------------------------------------
namespace impl
{

using id_elem_pair =  std::pair<index_t, entity>;

//-----------------------------------------------------------------------------
// NOTE: Not a template, could go in an actual implementation file.
inline void
build_element_vector(const Node &element_types, std::vector<id_elem_pair> &eles)
{
    /*
        element_types:
        (String name): (Q: Could this also be a list entry?)
            stream_id: (index_t id)
            shape: (String name of shape)
    */
    eles.clear();
    auto itr = element_types.children();
    while(itr.has_next())
    {
        const Node &n = itr.next();
        const index_t id = n["stream_id"].to_index_t();
        const ShapeType shape(n["shape"].as_string());
        eles.push_back({{id}, {}});
        eles.back().second.shape = shape;
        if(!shape.is_poly())
        {
            eles.back().second.element_ids.resize(shape.indices);
        }
        else
        {
            CONDUIT_ERROR("I cannot handle a stream of polygonal/polyhedral elements!");
            return;
        }
    }
}

//-----------------------------------------------------------------------------
// NOTE: Not a template, could go in an actual implementation file.
inline int
determine_case_number(const Node &topo)
{
/*
Multiple topology formats
0. Single shape topology - Fixed celltype, polygonal celltype, polyhedral celltype
  topo:
    coordset: (String name of coordset)
    type: unstructured
    elements:
      shape: (String name of shape)
      connectivity: (Integer array of vertex ids)
1. Mixed shape topology 1 - Fixed celltype, polygonal celltype, polyhedral celltype
  topo:
    coordset: (String name of coordset)
    type: unstructured
    elements:
      -
        shape: (String name of shape)
        connectivity: (Integer array of vertex ids)
2. Mixed shape topology 2 - Fixed celltype, polygonal celltype, polyhedral celltype
  topo:
    coordset: (String name of coordset)
    type: unstructured
    elements:
      (String name):
        shape: (String name of shape)
        connectivity: (Integer array of vertex ids)
3. Stream based toplogy 1
  mesh:
    type: "unstructured"
    coordset: "coords"
    elements:
      element_types:
        (String name): (Q: Could this also be a list entry?)
          stream_id: (index_t id)
          shape: (String name of shape)
      element_index:
        stream_ids: (index_t array of stream ids, must be one of the ids listed in element_types)
        element_counts: (index_t array of element counts at a given index (IE 2 would mean 2 of the associated stream ID))
      stream: (index_t array of vertex ids)
4. Stream based topology 2
  topo:
    type: unstructured
    coordset: (String name of coordset)
    elements:
      element_types:
        (String name): (Q: Could this also be a list entry?)
          stream_id: (index_t id)
          shape: (String name of shape)
      element_index:
        stream_ids: (index_t array of stream ids, must be one of the ids listed in element_types)
        offsets: (index_t array of offsets into stream for each element)
      stream: (index_t array of vertex ids)
5. Points - vertex celltype
  topo:
    type: "points"
    coords: "coords"
6. Uniform - line, quad, hex celltype
  topo:
    type: "uniform"
    coords: "coords"
7. Rectilinear - line, quad, hex celltype
  topo:
    type: "rectilinear"
    coords: "coords"
8. Structured - line, quad, hex celltype
  topo:
    type: "structured"
    coords: "coords"
    dims:
      i: (Integer)
      j: (Integer)
      k: (Integer)
9. Unstructured - mixed
  topo:
    type: "unstructured"
    elements:
      shape: "mixed"
      shapes: [array of shapes]
      shape_map:
        tri: 1
        ...
*/
    int case_num = -1;
    const std::string topo_type = topo["type"].as_string();
    if(topo_type == "unstructured")
    {
        const Node *shape = topo.fetch_ptr("elements/shape");
        if(shape)
        {
            // This is a single shape topology
            const ShapeType st(shape->as_string());
            if(!st.is_valid())
            {
                CONDUIT_ERROR("Invalid topology passed to iterate_elements.");
            }
            else if(shape->as_string() == "mixed")
            {
                case_num = 9;
            }
            else
            {
                case_num = 0;
            }
        }
        else
        {
            const Node *elements = topo.fetch_ptr("elements");
            if(!elements)
            {
                CONDUIT_ERROR("Invalid topology passed to iterate elements, no \"elements\" node.");
            }

            const Node *etypes = elements->fetch_ptr("element_types");
            const Node *eindex = elements->fetch_ptr("element_index");
            const Node *estream = elements->fetch_ptr("stream");
            if(!etypes && !eindex && !estream)
            {
                // Not a stream based toplogy, either a list or object of element buckets
                if(elements->dtype().is_list())
                {
                    case_num = 1;
                }
                else if(elements->dtype().is_object())
                {
                    case_num = 2;
                }
            }
            else if(etypes && eindex && estream)
            {
                // Stream based topology, either offsets or counts
                const Node *eoffsets = eindex->fetch_ptr("offsets");
                const Node *ecounts  = eindex->fetch_ptr("element_counts");
                if(ecounts)
                {
                    case_num = 3;
                }
                else if(eoffsets)
                {
                    case_num = 4;
                }
            }
        }
    }
    else if(topo_type == "points")
    {
        case_num = 5;
    }
    else if(topo_type == "uniform")
    {
        case_num = 6;
    }
    else if(topo_type == "rectilinear")
    {
        case_num = 7;
    }
    else if(topo_type == "structured")
    {
        case_num = 8;
    }
    return case_num;
}

//-----------------------------------------------------------------------------
template<typename FuncType>
inline void
traverse_fixed_elements(FuncType &&func, const Node &eles, const ShapeType &shape,
                        index_t &ent_id)
{
    // Single celltype
    entity e;
    e.shape = shape;
    const auto ent_size = e.shape.indices;
    e.element_ids.resize(ent_size, 0);

    index_t_accessor conn = eles["connectivity"].as_index_t_accessor();
    const index_t nents = conn.number_of_elements() / ent_size;
    index_t ei = 0;
    for(index_t i = 0; i < nents; i++)
    {
        e.entity_id = ent_id;
        for(index_t j = 0; j < ent_size; j++)
        {
            // Pull out vertex id at ei
            e.element_ids[j] = conn[ei];
            ei++;
        }

        func(e);
        ent_id++;
    }
}

//-----------------------------------------------------------------------------
template<typename FuncType>
inline void
traverse_polygonal_elements(FuncType &&func, const Node &elements,
                            index_t &ent_id)
{
    entity e;
    e.shape = utils::ShapeType((index_t)ShapeId::Polygonal);
    const index_t_accessor conn = elements["connectivity"].as_index_t_accessor();
    const index_t_accessor sizes = elements["sizes"].as_index_t_accessor();
    index_t ei = 0;
    for(index_t i = 0; i < sizes.number_of_elements(); i++)
    {
        e.entity_id = ent_id;
        const index_t sz = sizes[i];
        e.element_ids.resize(sz);
        for(index_t j = 0; j < sz; j++)
        {
            // Pull out vertex id at ei then cast to index_t
            e.element_ids[j] = conn[ei];
            ei++;
        }

        func(e);
        ent_id++;
    }
}

//-----------------------------------------------------------------------------
template<typename FuncType>
inline void
traverse_polyhedral_elements(FuncType &&func, const Node &elements,
                             const Node &subelements, index_t &ent_id)
{
    entity e;
    e.shape = utils::ShapeType((index_t)ShapeId::Polyhedral);
    const index_t_accessor conn = elements["connectivity"].as_index_t_accessor();
    const index_t_accessor sizes = elements["sizes"].as_index_t_accessor();
    const index_t_accessor subconn = subelements["connectivity"].as_index_t_accessor();
    const index_t_accessor subsizes = subelements["sizes"].as_index_t_accessor();
    const index_t_accessor suboffsets = subelements["offsets"].as_index_t_accessor();
    index_t ei = 0;
    for(index_t i = 0; i < sizes.number_of_elements(); i++)
    {
        e.entity_id = ent_id;
        const index_t sz = sizes[i];
        e.element_ids.resize(sz);
        for(index_t j = 0; j < sz; j++)
        {
            // Pull out vertex id at ei then cast to index_t
            e.element_ids[j] = conn[ei];
            ei++;
        }

        e.subelement_ids.resize(sz);
        for(index_t j = 0; j < sz; j++)
        {
            // Get the size of the subelement so we can define it in the proper index of subelement_ids
            auto &subele = e.subelement_ids[j];
            const index_t subsz = subsizes[e.element_ids[j]];
            subele.resize(subsz);

            // Find the offset of the face definition so we can write the vertex ids
            index_t offset = suboffsets[e.element_ids[j]];
            for(index_t k = 0; k < subsz; k++)
            {
                subele[k] = subconn[offset];
                offset++;
            }
        }

        func(e);
        ent_id++;
    }
}

//-----------------------------------------------------------------------------
template<typename FuncType>
inline void
traverse_stream(FuncType &&func, const Node &elements)
{
    // Stream with element counts or offsets
    std::vector<id_elem_pair> etypes;
    build_element_vector(elements["element_types"], etypes);
    const Node &eindex = elements["element_index"];
    const index_t_accessor stream = elements["stream"].as_index_t_accessor();
    const index_t_accessor stream_ids = eindex["stream_ids"].as_index_t_accessor();
    const Node *p_stream_offs = eindex.fetch_ptr("offsets");
    const Node *p_stream_counts = eindex.fetch_ptr("element_counts");
    const index_t nstream = stream_ids.number_of_elements();
    index_t ent_id = 0;
    // For count based this number just keeps rising, for offset based it gets overwritten
    //   by what is stored in the offsets node.
    index_t idx = 0;
    for(index_t i = 0; i < stream_ids.number_of_elements(); i++)
    {
        // Determine which shape we are working with
        const index_t stream_id = stream_ids[i];
        auto itr = std::find_if(etypes.begin(), etypes.end(), [=](const id_elem_pair &p){
            return p.first == stream_id;
        });
        entity &e = itr->second;

        // Determine how many elements are in this section of the stream
        index_t start = 0, end = 0;
        if(p_stream_offs)
        {
            index_t_accessor stream_offs = p_stream_offs->as_index_t_accessor();
            start = stream_offs[i];
            if(i == nstream - 1)
            {
                end = stream_offs.number_of_elements();
            }
            else
            {
                end = stream_offs[i+1];
            }
        }
        else if(p_stream_counts)
        {
            index_t_accessor stream_counts = p_stream_counts->as_index_t_accessor();
            start = idx;
            end   = start + (stream_counts[i] * e.shape.indices);
        }

        // Iterate the elements in this section
        idx = start;
        while(idx < end)
        {
            const index_t sz = e.shape.indices;
            for(index_t j = 0; j < sz; j++)
            {
                e.element_ids[j] = stream[idx];
                idx++;
            }
            e.entity_id = ent_id;
            func(e);
            ent_id++;
        }
        idx = end;
    }
}

//-----------------------------------------------------------------------------
template<typename FuncType>
inline void
traverse_points(FuncType &&func, const Node &topo)
{
    std::array<index_t, 3> dims{1, 1, 1};
    // TODO: Double check that 1 doesn't need to be added to each dimension.
    topology::logical_dims(topo, dims.data(), 3);

    index_t npts = 1;
    for(const auto d : dims)
    {
        if(d > 0)
        {
            npts *= d;
        }
    }

    entity e;
    e.shape = ShapeType((index_t)ShapeId::Point);
    e.element_ids.resize(1);
    e.subelement_ids.clear();
    for(index_t i = 0; i < npts; i++)
    {
        e.entity_id = i;
        e.element_ids[0] = i;
        func(e);
    }
}

//-----------------------------------------------------------------------------
template<typename FuncType>
inline void
traverse_structured(FuncType &&func, const Node &topo)
{
    std::array<index_t, 3> dims{1, 1, 1};
    topology::logical_dims(topo, dims.data(), 3);
    const index_t dimension = topology::dims(topo);

    if(dimension == 1)
    {
        // Line elements
        entity e;
        e.shape = ShapeType((index_t)ShapeId::Line);
        e.element_ids.resize(2);
        e.subelement_ids.clear();
        for(index_t i = 0; i < dims[0]; i++)
        {
            e.entity_id = i;
            e.element_ids[0] = i;
            e.element_ids[1] = i + 1;
            func(e);
        }
    }
    else if(dimension == 2)
    {
        // Quad elements
        entity e;
        e.shape = ShapeType((index_t)ShapeId::Quad);
        e.element_ids.resize(4);
        e.subelement_ids.clear();
        const index_t nx = dims[0] + 1;
        index_t id = 0;
        for(index_t j = 0; j < dims[1]; j++)
        {
            const index_t jnx  = j * nx;
            const index_t j1nx = (j + 1) * nx;
            for(index_t i = 0; i < dims[0]; i++)
            {
                e.entity_id = id++;
                e.element_ids[0] = jnx + i;
                e.element_ids[1] = jnx + i + 1;
                e.element_ids[2] = j1nx + i + 1;
                e.element_ids[3] = j1nx + i;
                func(e);
            }
        }
    }
    else if(dimension == 3)
    {
        // Hex elements
        entity e;
        e.shape = ShapeType((index_t)ShapeId::Hex);
        e.element_ids.resize(8);
        e.subelement_ids.clear();
        const index_t nx = dims[0] + 1;
        const index_t ny = dims[1] + 1;
        index_t id = 0;
        for(index_t k = 0; k < dims[2]; k++)
        {
            const index_t knxny  = k * nx * ny;
            const index_t k1nxny = (k + 1) * nx * ny;
            for(index_t j = 0; j < dims[1]; j++)
            {
                const index_t jnx  = j * nx;
                const index_t j1nx = (j + 1) * nx;
                for(index_t i = 0; i < dims[0]; i++)
                {
                    e.entity_id = id++;
                    e.element_ids[0] = knxny  + jnx  + i;
                    e.element_ids[1] = knxny  + jnx  + i + 1;
                    e.element_ids[2] = knxny  + j1nx + i + 1;
                    e.element_ids[3] = knxny  + j1nx + i;
                    e.element_ids[4] = k1nxny + jnx  + i;
                    e.element_ids[5] = k1nxny + jnx  + i + 1;
                    e.element_ids[6] = k1nxny + j1nx + i + 1;
                    e.element_ids[7] = k1nxny + j1nx + i;
                    func(e);
                }
            }
        }
    }
    else
    {
        CONDUIT_ERROR("Unsupported dimension given to iterate_elements (traverse_structured) "
            << dimension << ".");
    }
}

//-----------------------------------------------------------------------------
template<typename FuncType>
inline void
traverse_mixed_elements(FuncType &&func, const Node &topo)
{
    auto copy_subelements = [](entity &e,
                               const index_t_accessor &se_conn,
                               const index_t_accessor &se_sizes,
                               const index_t_accessor &se_offsets)
    {
        const index_t numFaces = static_cast<index_t>(e.element_ids.size());
        e.subelement_ids.resize(numFaces);
        for(index_t j = 0; j < numFaces; j++)
        {
            const auto faceId = e.element_ids[j];
            const index_t se_size = se_sizes[faceId];
            index_t se_offset = se_offsets[faceId];

            auto &se_ids = e.subelement_ids[j];
            se_ids.resize(se_size);

            for(index_t k = 0; k < se_size; k++)
            {
                se_ids[k] = se_conn[se_offset++];
            }
        }
    };

    const Node &elements = topo.fetch_existing("elements");
    if(!elements.has_child("shapes"))
    {
        CONDUIT_ERROR("Mixed topology requires \"shapes\"");
    }
    if(!elements.has_child("shape_map"))
    {
        CONDUIT_ERROR("Mixed topology requires \"shape_map\"");
    }

    // Make a value2id map using the shape_map.
    const Node &n_shape_map = elements.fetch_existing("shape_map");
    std::map<index_t, index_t> value2id;
    index_t phShape = -1;
    bool hasPolyhedra = false;
    for(index_t i = 0; i < n_shape_map.number_of_children(); i++)
    {
        const Node &n_shape = n_shape_map[i];
        conduit::blueprint::mesh::utils::ShapeType shape(n_shape.name());
        value2id[n_shape.to_int()] = shape.id;
        if(n_shape.name() == "polyhedral")
        {
            phShape = n_shape.to_int();
            hasPolyhedra = true;
        }
    }

    const index_t_accessor conn = elements["connectivity"].as_index_t_accessor();
    const index_t_accessor shapes = elements["shapes"].as_index_t_accessor();
    const auto nzones = shapes.number_of_elements();

    if(elements.has_child("sizes") && elements.has_child("offsets"))
    {
        const index_t_accessor sizes = elements["sizes"].as_index_t_accessor();
        const index_t_accessor offsets = elements["offsets"].as_index_t_accessor();

        if(hasPolyhedra && topo.has_path("subelements/connectivity"))
        {
            // The elements contain polyhedra.
            const index_t_accessor se_conn = topo["subelements/connectivity"].as_index_t_accessor();
            const index_t_accessor se_sizes = topo["subelements/sizes"].as_index_t_accessor();
            const index_t_accessor se_offsets = topo["subelements/offsets"].as_index_t_accessor();

            for(index_t ei = 0; ei < nzones; ei++)
            {
                const auto size = sizes[ei];
                const auto offset = offsets[ei];
                const auto shape = shapes[ei];

                // Copy basic shape data into the entity.
                entity e;
                e.entity_id = ei;
                e.shape = utils::ShapeType(value2id[shape]);
                e.element_ids.resize(size);
                for(index_t j = 0; j < size; j++)
                {
                    e.element_ids[j] = conn[offset + j];
                }

                // Copy subelement data for polyhedral zones.
                if(shape == phShape)
                {
                    copy_subelements(e, se_conn, se_sizes, se_offsets);
                }

                func(e);
            }
        }
        else
        {
            // There are no polyhedra.
            for(index_t ei = 0; ei < nzones; ei++)
            {
                const auto size = sizes[ei];
                const auto offset = offsets[ei];
                const auto shape = shapes[ei];

                entity e;
                e.entity_id = ei;
                e.shape = utils::ShapeType(value2id[shape]);
                e.element_ids.resize(size);
                for(index_t j = 0; j < size; j++)
                {
                    // Pull out vertex id at ei then cast to index_t
                    e.element_ids[j] = conn[offset + j];
                }

                func(e);
            }
        }
    }
    else
    {
        // No offsets so we'll just iterate through the connectivity.

        if(hasPolyhedra && topo.has_path("subelements/connectivity"))
        {
            // The elements contain polyhedra.
            const index_t_accessor se_conn = topo["subelements/connectivity"].as_index_t_accessor();
            const index_t_accessor se_sizes = topo["subelements/sizes"].as_index_t_accessor();
            const index_t_accessor se_offsets = topo["subelements/offsets"].as_index_t_accessor();

            index_t offset = 0;
            for(index_t ei = 0; ei < nzones; ei++)
            {
                const auto shape = shapes[ei];

                // Copy basic shape data into the entity.
                entity e;
                e.entity_id = ei;
                e.shape = utils::ShapeType(value2id[shape]);

                // Get size from shape.
                const auto size = e.shape.indices;

                e.element_ids.resize(size);
                for(index_t j = 0; j < size; j++)
                {
                    e.element_ids[j] = conn[offset + j];
                }

                // Copy subelement data for polyhedral zones.
                if(shape == phShape)
                {
                    copy_subelements(e, se_conn, se_sizes, se_offsets);
                }

                func(e);

                offset += size;
            }
        }
        else
        {
            // No offsets so we'll just iterate through the connectivity.
            index_t offset = 0;
            for(index_t ei = 0; ei < nzones; ei++)
            {
                const auto shape = shapes[ei];
                entity e;
                e.entity_id = ei;
                e.shape = utils::ShapeType(value2id[shape]);

                // Get size from shape.
                const auto size = e.shape.indices;

                e.element_ids.resize(size);
                for(index_t j = 0; j < size; j++)
                {
                    // Pull out vertex id at ei then cast to index_t
                    e.element_ids[j] = conn[offset + j];
                }

                func(e);

                offset += size;
            }
        }
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::topology::impl --
//-----------------------------------------------------------------------------

// NOTE: namespace conduit::blueprint::mesh::utils::topology

//-----------------------------------------------------------------------------
template<typename Func>
inline void
iterate_elements(const Node &topo, Func &&func)
{
    // Determine case number, case numbers documented
    //   at the top of determine_case_number
    int case_num = impl::determine_case_number(topo);
    if(case_num < 0)
    {
        CONDUIT_ERROR("Could not figure out the type of toplogy passed to iterate elements.");
        return;
    }

    index_t ent_id = 0;
    switch(case_num)
    {
    case 0:
    {
        ShapeType shape(topo);
        if(shape.is_polyhedral())
        {
            impl::traverse_polyhedral_elements(func, topo["elements"], topo["subelements"], ent_id);
        }
        else if(shape.is_polygonal())
        {
            impl::traverse_polygonal_elements(func, topo["elements"], ent_id);
        }
        else // (known celltype case)
        {
            impl::traverse_fixed_elements(func, topo["elements"], shape, ent_id);
        }
        break;
    }
    case 1: /* Fallthrough */
    case 2:
    {
        // Mixed celltype
        const Node &elements = topo["elements"];
        auto ele_itr = elements.children();
        while(ele_itr.has_next())
        {
            const Node &bucket = ele_itr.next();
            const std::string &shape_name = bucket["shape"].as_string();
            utils::ShapeType shape(shape_name);

            if(shape.is_polyhedral())
            {
                // Need to find corresponding subelements
                const std::string bucket_name = bucket.name();
                if(!topo.has_child("subelements"))
                {
                    CONDUIT_ERROR("Invalid toplogy, shape == polygonal but no subelements node present.");
                    return;
                }
                const Node &subelements = topo["subelements"];
                if(!subelements.has_child(bucket_name))
                {
                    CONDUIT_ERROR("Invalid toplogy, shape == polygonal but no matching subelements node present.");
                    return;
                }
                const Node &subbucket = subelements[bucket_name];
                impl::traverse_polyhedral_elements(func, bucket, subbucket, ent_id);
            }
            else if(shape.is_polygonal())
            {
                impl::traverse_polygonal_elements(func, bucket, ent_id);
            }
            else
            {
                impl::traverse_fixed_elements(func, bucket, shape, ent_id);
            }
        }
        break;
    }
    case 3: /* Fallthrough */
    case 4:
    {
        // Stream with element counts or offsets
        impl::traverse_stream(func, topo["elements"]);
        break;
    }
    case 5:
    {
        // Points topology
        impl::traverse_points(func, topo);
        break;
    }
    case 6: /* Fallthrough */
    case 7: /* Fallthrough */
    case 8:
    {
        // Uniform, Rectilinear, Structured topology
        impl::traverse_structured(func, topo);
        break;
    }
    case 9:
        impl::traverse_mixed_elements(func, topo);
        break;
    default:
        CONDUIT_ERROR("Unsupported topology passed to iterate_elements")
        return;
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::topology --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils --
//-----------------------------------------------------------------------------

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
// -- end conduit --
//-----------------------------------------------------------------------------

#endif
