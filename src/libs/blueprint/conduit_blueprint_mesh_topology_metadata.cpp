// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_topology_metadata.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// std includes
//-----------------------------------------------------------------------------
#include <deque>
#include <algorithm>
#include <limits>
#include <numeric>
#include <cstring>

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh_topology_metadata.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_annotations.hpp"
#include "conduit_execution.hpp"
#include "conduit_utils.hpp"

//#define DEBUG_PRINT

#define EA_INDEX(E,A) ((E)*(MAX_ENTITY_DIMS)+(A))

// This macro is used to highlight some code in the new implementation that
// is geared at reproducing unintuitive behavior present in the reference
// implementation.
#define REPRODUCE_REFERENCE

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

// We may tag certain algorithms as ParallelExec if it is safe to do so.
using SerialExec = conduit::execution::SerialExec;
#if defined(CONDUIT_USE_OPENMP)
using ParallelExec = conduit::execution::OpenMPExec;
#else
using ParallelExec = conduit::execution::SerialExec;
#endif

//---------------------------------------------------------------------------
void
yaml_print(std::ostream &os, const conduit::Node &node)
{
    // Override these parameters so we 
    conduit::Node opts;
    opts["num_elements_threshold"] = 10000;
    opts["num_children_threshold"] = 10000;

    std::string s;
    node.to_summary_string_stream(os, opts);
}

//---------------------------------------------------------------------------
template <typename T>
std::ostream &
operator << (std::ostream &os, const std::vector<T> &obj)
{
    os << "[size=" << obj.size() << "]{";
    for(size_t i = 0; i < obj.size(); i++)
        os << obj[i] << ", ";
    os << "}";
    return os;
}

//---------------------------------------------------------------------------
template <>
std::ostream &
operator << (std::ostream &os, const std::vector<std::pair<uint64, uint64>> &obj)
{
    os << "{" << std::endl;
    for(size_t i = 0; i < obj.size(); i++)
    {
        os << "[" << std::setw(2) << i << "]("
           << std::setw(20) << obj[i].first << ", "
           << std::setw(20) << obj[i].second << ")"
           << ", " << std::endl;
    }
    os << "}" << std::endl;
    return os;
}

//---------------------------------------------------------------------------
template <>
std::ostream &
operator << (std::ostream &os, const std::vector<std::pair<index_t, index_t>> &obj)
{
    os << "{" << std::endl;
    for(size_t i = 0; i < obj.size(); i++)
    {
        os << "[" << std::setw(2) << i << "]("
           << std::setw(20) << obj[i].first << ", "
           << std::setw(20) << obj[i].second << ")"
           << ", " << std::endl;
    }
    os << "}" << std::endl;
    return os;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

/**
 This class accepts a node containing a topology and it puts that topology
 through a shape cascade. This means turning elements->faces->edges->points
 and producing associations (maps) that allow the caller to query various
 relations such as the elements that own a face, or the lines in an element.
 All of the various entities in the different topology levels have been made
 unique.

 This class differs from the previous implementation in the following ways:

 * It uses recursion and template functions to make sure we can access
   connectivity data using either native pointers or an index_t_accessor
   so no Conduit casts are needed.

 * Associations are mostly created only when asked for (or indirectly needed)
   and this is done largely after the cascade has been performed. This is
   a huge source of complexity but it lets us compute fewer maps.

 * Associations are stored contiguously in memory or are implicit.

 * Access to associations is made using a "vector_view" which returns a window
   into the bulk association data so the caller can use the data without
   having to copy it out. Local association data is returned as a "range_vector"
   so the local ranges can be represented implicitly while still looking
   like a vector.
*/
class TopologyMetadata::Implementation : public TopologyMetadataBase
{
    constexpr static size_t MAX_ENTITY_DIMS = 4;

    struct association
    {
        // The association owns this storage.
        std::vector<index_t> data;
        std::vector<index_t> sizes;
        std::vector<index_t> offsets;
        // Other fields
        int                  single_size{1};
        bool                 requested{false};

        inline std::pair<index_t *, index_t> get_data(index_t entity_id) const;
        inline index_t get_size(index_t entity_id) const;
        inline index_t get_offset(index_t entity_id) const;
        inline index_t sum_sizes(index_t num_entities) const;
    };

    // Data members
    const conduit::Node *topo, *coords;
    const ShapeCascade topo_cascade;
    const ShapeType topo_shape;
    size_t lowest_cascade_dim;
    index_t coords_length;
    DataType int_dtype;
    DataType float_dtype;
    conduit::Node dim_topos[MAX_ENTITY_DIMS];
    conduit::Node dim_topos_int_dtype[MAX_ENTITY_DIMS];
    index_t dim_topo_lengths[MAX_ENTITY_DIMS];
    association G[MAX_ENTITY_DIMS][MAX_ENTITY_DIMS];  
    association L[MAX_ENTITY_DIMS][MAX_ENTITY_DIMS];  // For explicit local maps
    std::vector<index_t> local_to_global[MAX_ENTITY_DIMS];

public:
    //-----------------------------------------------------------------------
    /**
     @brief This constructor builds all of the topology levels in the shape
            cascade as well as all associations.

     @param topology     The input topology node.
     @param coordset     The input coordset associated with the topology.
     */
    Implementation(const conduit::Node &topology, const conduit::Node &coordset);

    //-----------------------------------------------------------------------
    /**
     @brief Constructor for the TopologyMetadata::Implementation class. This
            constructor lets the caller be more selective about which topology
            levels and associations are created, saving time.

     @param topology     The input topology node.
     @param coordset     The input coordset associated with the topology.
     @param lowest_dim   The lowest level of shape cascade that we're interested in.
                         If we only want faces, for example, then we don't need to
                         refine to edges or points.
     @param desired      A vector of (entity_dim,assoc_dim) pairs that indicate
                         the associations that are requested by the client.
     */
    Implementation(const conduit::Node &topology,
                   const conduit::Node &coordset,
                   size_t lowest_dim,
                   const std::vector<std::pair<size_t,size_t> > &desired_maps);

    //-----------------------------------------------------------------------
    /**
     @brief Get the highest shape dimension.
     @return The highest shape dimension.
     */
    int dimension() const { return static_cast<int>(topo_shape.dim); }

    //-----------------------------------------------------------------------
    /**
     @brief Get the topologies array (the possibly int_dtype converted version)
     @return The topologies array.
     */
    const conduit::Node &get_topology(size_t dim);

    //-----------------------------------------------------------------------
    /**
     @brief Get the topology for the dimension in the provided node.
     @param dim The dimension.
     @param dest The destination node.
     */
    void get_topology(size_t dim, conduit::Node &dest);

    //-----------------------------------------------------------------------
    /**
     @brief Get the legnths arrays for the topologies. Any topologies that
            were not produced will have length 0.
     @return The topology lengths array.
     */
    index_t get_topology_length(size_t dim) const;

    //-----------------------------------------------------------------------
    /**
     @brief Get a global association for a particular entity_id in the entity_dim
            dimension. This lets us ask a question like: which face ids are in
            element 5? Or, which faces contain edge 12?

     @param entity_id The entity id whose information we want to obtain. This
                      value must be valid for the entity dimension in question.
     @param entity_dim The dimension of the entity whose information we want.
     @param assoc_dim  The dimension of the association we want.

     Example: Which faces are in element 5?
              get_global_association(5,  // element 5
                                     3,  // elements are 3 dimensional
                                     2); // faces are 2 dimensional

     Example 2: Which faces contain edge 2?
              get_global_association(2,  // edge 2
                                     1,  // edges are 1 dimensional
                                     2); // faces are 2 dimensional

     @note The data are returned as a vector_view, which is an object that acts
           like a vector but does not have to copy bulk data. The data it points
           to for a given return are in the associations.
     */
    conduit::vector_view<index_t>
    get_global_association(index_t entity_id, index_t entity_dim, index_t assoc_dim) const;

    //-----------------------------------------------------------------------
    /**
     @brief Get a local association for a particular entity_id in the entity_dim
            dimension.

     @param entity_id The entity id whose information we want to obtain. This
                      value must be valid for the entity dimension in question.
     @param entity_dim The dimension of the entity whose information we want.
     @param assoc_dim  The dimension of the association we want.
    */
    conduit::range_vector<index_t>
    get_local_association(index_t entity_id, index_t entity_dim, index_t assoc_dim) const;

    //-----------------------------------------------------------------------
    /**
     @brief Get whether the association e,a was requested when the object
            was initialized. If so, the association will exist. Otherwise,
            the association does not exist.

     @param entity_dim The starting dimension.
     @param assoc_dim  The destination dimension.

     @return True if the association exists; False otherwise.
     */
    bool association_requested(index_t entity_dim, index_t assoc_dim) const;

    //-----------------------------------------------------------------------
    /**
      @brief Return all of the data for an association by COPYING it into the 
             provided Conduit node.

      @param type Whether we want GLOBAL or LOCAL data.
      @param src_dim The source dimension of the desired association.
      @param dst_dim The destination dimension of the desired association.
      @param[out] map_node The Conduit node that will contain the copied association data.

      @note This method produces data arrays as int_dtype.

      @return True if the map exists(was requested); False otherwise.
     */
    bool get_dim_map(IndexType type, index_t src_dim, index_t dst_dim, Node &map_node) const;

    //-----------------------------------------------------------------------
    /**
      @brief Gets the length of the topology as specified by dimension. If
             dim is -1 then the length of all topologies are summed.

      @param dim The dimension whose length we want. Or if -1 then all
                 dimensions are assumed.
      @return The topology length for the requested dimension(s).
     */
    index_t get_length(index_t dim) const;

    //-----------------------------------------------------------------------
    /**
     @brief The the preferred integer storage data type.
     @return The preferred integer storage data type.
     */
    const DataType &get_int_dtype() const;

    //-----------------------------------------------------------------------
    /**
     @brief The the preferred float storage data type.
     @return The preferred float storage data type.
     */
    const DataType &get_float_dtype() const;

    //-----------------------------------------------------------------------
    /**
     @brief Gets the total number of embeddings for each entity at the top level
            to the embedding level.
     @param entity_dim The entity dimension.
     @param embed_dim The embedding dimension.
     @return The total number of embeddings.
     */
    index_t get_embed_length(index_t entity_dim, index_t embed_dim) const;

    //-----------------------------------------------------------------------
    /**
     @brief Make a representation of the metadata in the supplied node.
     @param rep The node that will contain the representation.
     */
    void make_node(conduit::Node &rep) const;

    //-----------------------------------------------------------------------
    /**
     @brief Turn the metadata into a JSON string and return it.
     @return A string that contains a JSON representation of the metadata.
     */
    std::string to_json() const;

    //-----------------------------------------------------------------------
    /**
     @brief Return a vector that lets us map local entity ids to global ids.
     @param dim The dimension of the map we want.
     @return The map for the requested dimension.
     */
    const std::vector<index_t> &get_local_to_global_map(index_t) const;
private:

    //-----------------------------------------------------------------------
    /**
     @brief This method causes the topologies and maps to be created,
            initializing the object.

     @param desired A vector of (e,a) pairs that indicate which associations
                    we want to compute.
     */
    void initialize(const std::vector<std::pair<size_t, size_t> > &desired);

    //-----------------------------------------------------------------------
    /**
     @brief Examine the vector of desired maps and mark the associations
            as requested.

     @param desired A vector of (e,a) pairs that indicate which associations
                    we want to compute.
     */
    void request_associations(const std::vector<std::pair<size_t, size_t> > &desired);

    //-----------------------------------------------------------------------
    /**
     @brief Build any associations that were not produced during the cascade.
     */
    void build_associations();

    //-----------------------------------------------------------------------
    /**
     @brief Build any local associations that were requested.
     */
    void build_local_associations();

    //-----------------------------------------------------------------------
    /**
     @brief Build the association from dimension e to dimension 0.

     @param e The source dimension.
     */
    void build_association_e_0(int e);

    //-----------------------------------------------------------------------
    /**
     @brief Given a 3D non-PH shape, return the set of unique line segments
            in traversal order.

     @param shape The input shape, which should be a 3D non-PH element.
     @return A vector of edge endpoint pairs that represent the unique edges
             for the element.
     */
    std::vector<index_t> embedding_3_1_edges(const ShapeType &shape) const;

    //-----------------------------------------------------------------------
    /**
     @brief Build the G(3,1) and G(3,0) associations together because they
            need to do most of the same work.
     */
    void build_association_3_1_and_3_0();
    void build_association_3_1_and_3_0_ph();
    void build_association_3_1_and_3_0_nonph();

    //-----------------------------------------------------------------------
    /**
     @param Build a map of edge keys to edge ids. The keys are the hashed endpoints.
     @param[out] edge_key_to_id A sorted vector of pairs where each pair.first
                                is the hashed edge key and pair.second is the
                                edge's global index.
     */
    void build_edge_key_to_id(std::vector<std::pair<uint64, index_t>> &edge_key_to_id) const;

    //-----------------------------------------------------------------------
    /**
     @brief Look up an edge by key in the map by binary search and return its
            edge id.

     @param edge_key_to_id The map built by build_edge_key_to_id.
     @param key            The edge key.

     @return The edge id.
     */
    index_t lookup_edge_id(std::vector<std::pair<uint64, index_t>> &edge_key_to_id,
                           const uint64 key) const;

    //-----------------------------------------------------------------------
    /**
     @brief Print association G[e][a].
     */
    void print_association(int e, int a, bool global = true) const;

    //-----------------------------------------------------------------------
    /**
     @brief This method builds a child to parent association by reversing
            an existing parent to child association.

     @param e The entity dimension (the thing we have: element=3,face=2,...)
     @param a The association dimension (the thing we want to know about with
              respect to the entity dimension). e < a.
     */
    void build_child_to_parent_association(int e, int a);

    /**
      @brief Return all of the data for a GLOBAL association by COPYING it into
             the provided Conduit node.

      @param src_dim The source dimension of the desired association.
      @param dst_dim The destination dimension of the desired association.
      @param[out] map_node The Conduit node that will contain the copied association data.

      @note This method guarantees that all bulk arrays for values, sizes, and
            offsets will be index_t.
     */
    bool get_global_dim_map(index_t src_dim, index_t dst_dim, Node &map_node) const;
    bool get_local_dim_map(index_t src_dim, index_t dst_dim, Node &map_node) const;

    //-----------------------------------------------------------------------
    /**
     @brief Returns the number of entities for L(e,a) so we can iterate through them.
     @param e The entity dimension
     @param a The association dimension
     @return The number of entities in L(e,a).
     */
    index_t get_local_association_entity_range(index_t e, index_t a) const;

    //-----------------------------------------------------------------------
    /**
     @brief Takes the input topology and uses it as the highest topology.
            The method makes sure that the topology will have offsets too.
     */
    void make_highest_topology();

    //-----------------------------------------------------------------------
    /**
     @brief Sets up the dim_topos_int_dtype topologies, converting types
            if needed.
     @param dim The dimension topology to convert.
     @param dest The node that holds the converted topology - or a reference
                 to the topology if forceCopy == false and the data were in
                 the right format.
     @param forceCopy Whether to force copying the data.
     */
    void convert_topology_dtype(size_t dim, conduit::Node &dest, bool forceCopy);

    //-----------------------------------------------------------------------
    /**
     @brief Copy a source topology into a destination node and convert the
            types of important connectivity arrays to the specified dest_type.

     @param src_topo  The source topology node.
     @param shape     The shape that describes the src_topo.
     @param dest_type The destination type.
     @param dest_topo The destination topology node.
     */
    void copy_topology(const conduit::Node &src_topo,
                       const ShapeType &shape,
                       const DataType &dest_type,
                       conduit::Node &dest_topo);

    //-----------------------------------------------------------------------
    /**
     @brief Convert a set of keys under one node to another node, converting
            data types.

     @param src_keys  The source keys in n_src.
     @param n_src     The source node.
     @param dest_type The type to use for conversion.
     @param dest_keys The names of the new nodes in n_dest.
     @param n_dest    The node to contain the converted data.
     */
    void copy_convert(const std::vector<std::string> &src_keys,
                      const conduit::Node &n_src,
                      const DataType &dest_type,
                      const std::vector<std::string> &dest_keys,
                      conduit::Node &n_dest);

    //-----------------------------------------------------------------------
    /**
     @brief Make a basic topology for the points.
     */
    void make_point_topology();

    //-----------------------------------------------------------------------
    /**
     @brief Identify sequences of unique ids and give them the same unique id
            that is an increasing number rather than a random id.

     @param faceid_to_ef A vector of pairs that contain a faceid to element face.
                         Or, rather, a hashid to element entity.
     @param ef_to_unique: A reverse map of element face to unique id.

     @return The number of unique ids.
    */
    index_t
    make_unique(const std::vector<std::pair<uint64, uint64>> &faceid_to_ef, 
                std::vector<std::pair<uint64, uint64>> &ef_to_unique) const;

    //-----------------------------------------------------------------------
    /**
     @brief Build local to global maps.
     */
    void build_local_to_global();

    //-----------------------------------------------------------------------
    /**
     @brief This method casts the sizes node into some usable types and then
            invokes dispatch_shape_ph built for those types.

     @param subel The polyhedral subelement node.
     @param sizes The sizes node under the subelement node.
     */
    void dispatch_connectivity_ph(const conduit::Node &subel,
                                  const conduit::Node &sizes);

    //-----------------------------------------------------------------------
    /**
     @brief This method is called with a shape and connectivity and it casts
            the connectivity to usable types and calls dispatch_shape methods
            that operate on those types.

     @param shape The shape that the connectivity represents.
     @param conn  A node that contains the bulk connectivity data.
     */
    void  dispatch_connectivity(const ShapeType &shape,
                                const conduit::Node &conn);

    //-----------------------------------------------------------------------
    /**
     @brief This method accepts a node containing polyhedral subelement data,
            which contains the actual face definitions that make up the PH
            mesh. We invoke the method to make the face connectivity and then
            dispatch again to make the lines.

     @param subel The polyhedral subelement node.
     @param sizes The sizes in the subelement node cast to a more usable type
                  such as a pointer or an accessor.
     @param sizeslen The number of elements in sizes.
     */
    template <typename ConnType>
    void
    dispatch_shape_ph(const conduit::Node &subel,
                      const ConnType &sizes,
                      size_t sizeslen)
    {
        const int embed_dim = 2;

        // Use PH faces to make 2D faces.
        make_embedded_connectivity_ph(subel, sizes, sizeslen);

        // Make lines. Note that we get the embed shape from the 2D topo
        // in case we set it to quads or tris. The topo_cascade would
        // return polygons but we may do a little better with quads/tris
        // if we can assume those.
        const conduit::Node &econn = dim_topos[embed_dim].fetch_existing("elements/connectivity");
        ShapeType embed_shape(dim_topos[embed_dim]);
        dispatch_connectivity(embed_shape, econn);
    }

    //-----------------------------------------------------------------------
    /**
     @brief This method is called with a shape and connectivity data that
            is of a concrete type (or an accessor). The method makes calls
            to create the connectivity for the embedded shape (e.g. faces 
            from elements) and then calls the next level down when done.

     @param shape The shape that the connectivity represents.
     @param conn  An array or accessor that contains the connectivity data.
     @param connlen The number of connectivity array elements.
     */
    template <typename ConnType>
    void
    dispatch_shape(const ShapeType &shape,
                   const ConnType &conn,
                   size_t connlen)
    {
        if(!shape.is_polygonal())
        {
            // Make faces for any of the 3D implicit cell types.
            if(shape.dim == 3)
            {
                make_embedded_connectivity(shape, conn, connlen);

                // Make lines.
                auto embed_dim = static_cast<index_t>(shape.dim - 1);
                const conduit::Node &econn = dim_topos[embed_dim].
                   fetch_existing("elements/connectivity");
                ShapeType embed_shape = topo_cascade.get_shape(embed_dim);
                dispatch_connectivity(embed_shape, econn);
            }
            else if(shape.dim == 2)
            {
                // All input shapes are the same (e.g. quads or tris).
                // Make lines.
                make_embedded_connectivity(shape, conn, connlen);
            }
        }
        else
        {
            // The shape contain polygons so we have to be a bit more
            // general in how we traverse the connectivity.
            // Make lines.
            make_embedded_connectivity_polygons_to_lines(conn);
        }
    }

    //-----------------------------------------------------------------------
    /**
     @brief This makes 2D topology from the input polyhedral topology. If all
            faces are triangles or quads then we make a mesh that just contains
            those element types. This makes the refinement to edges a little
            easier. If there are mized face sizes then the output 2D topology
            is polygonal.

     @note This method takes the face sizes as a concrete type so we can scan
           them easily to determine if all faces are the same shape.

     @param subel The polyhedral subelement node.
     @param sizes The sizes in the subelement node cast to a more usable type
                  such as a pointer or an accessor.
     @param sizeslen The number of elements in sizes.
     */
    template <typename ConnType>
    void
    make_embedded_connectivity_ph(const conduit::Node &subel,
                                  const ConnType &sizes,
                                  size_t sizeslen)
    {
        CONDUIT_ANNOTATE_MARK_FUNCTION;

        const int embed_dim = 2;

        // Use the subelement information from the PH mesh as the embedded
        // connectivity for the 2D faces.
        conduit::Node &node = dim_topos[embed_dim];
        node["type"] = "unstructured";
        node["coordset"] = coords->name();
        node["elements/shape"] = subel["shape"].as_string();

        // Check whether the sizes are the same. If they are then we can convert
        // from "polygon" types to tri, or quad.
        bool istri = sizes[0] == 3;
        bool isquad = sizes[0] == 4;
        if(istri || isquad)
        {
            bool same = true;
            for(size_t i = 1; i < sizeslen && same; i++)
                same &= sizes[0] == sizes[i];

            if(same && istri)
            {
                node["elements/shape"].reset(); // Workaround!
                node["elements/shape"] = "tri";
            }
            else if(same && isquad)
            {
                node["elements/shape"].reset(); // Workaround!
                node["elements/shape"] = "quad";
            }
        }

        association &map32 = G[3][2];
#ifdef REPRODUCE_REFERENCE
        // Match the old TopologyMetadata, which reordered (unnecessary?)
        // PH faces relative to the order in which they are used in elements.
        if(node["elements/shape"].as_string() == "polygonal")
        {
            index_t_accessor elem_conn = topo->fetch_existing("elements/connectivity").value();
            index_t_accessor elem_sizes = topo->fetch_existing("elements/sizes").value();

            index_t_accessor face_conn = subel["connectivity"].as_index_t_accessor();
            index_t_accessor face_sizes = subel["sizes"].as_index_t_accessor();
            index_t_accessor face_offsets = subel["offsets"].as_index_t_accessor();

            index_t nelem = elem_sizes.number_of_elements();
            index_t nfaces = face_sizes.number_of_elements();
            std::vector<index_t> face_old_to_new(nfaces, -1);

            // Sum the number of faces in the top PH elements. That is how many
            // possible local faces there would be.
            index_t nLocalFaces = elem_sizes.sum();
            local_to_global[2].reserve(nLocalFaces);

            // Set up nodes to store the new topology.
            conduit::Node &n_newface_conn = node["elements/connectivity"];
            conduit::Node &n_newface_sizes = node["elements/sizes"];
            conduit::Node &n_newface_offsets = node["elements/offsets"];
            n_newface_conn.set(DataType::index_t(face_conn.number_of_elements()));
            n_newface_sizes.set(DataType::index_t(nfaces));
            n_newface_offsets.set(DataType::index_t(nfaces));
            index_t *newface_conn = n_newface_conn.value();
            index_t *newface_sizes = n_newface_sizes.value();
            index_t *newface_offsets = n_newface_offsets.value();

            // Iterate over the elements and each time we see a face for the
            // first time, emit the face into the new connectivity. Also
            // build the local_to_global[2] map.
            index_t globalFaceIndex = 0;
            index_t ptIndex = 0;
            index_t elem_offset = 0;
            for(index_t ei = 0; ei < nelem; ei++)
            {
                index_t nelem_faces = elem_sizes[ei];

                for(index_t fi = 0; fi < nelem_faces; fi++)
                {
                    index_t faceid = elem_conn[elem_offset + fi];
                    index_t newfaceid;
                    if(face_old_to_new[faceid] == -1)
                    {
                        index_t face_size = face_sizes[faceid];

                        newface_offsets[globalFaceIndex] = ptIndex;
                        newface_sizes[globalFaceIndex] = face_size;

                        for(index_t pi = 0; pi < face_size; pi++)
                        {
                            newface_conn[ptIndex++] = face_conn[face_offsets[faceid] + pi];
                        }

                        newfaceid = globalFaceIndex++;
                        face_old_to_new[faceid] = newfaceid;
                    }
                    else
                    {
                        newfaceid = face_old_to_new[faceid];
                    }

                    local_to_global[2].push_back(newfaceid);
                }
                elem_offset += nelem_faces;
            }

            if(map32.requested)
            {
                map32.data.resize(local_to_global[2].size());
                std::copy(local_to_global[2].begin(),
                          local_to_global[2].end(),
                          map32.data.begin());
                map32.sizes.resize(nelem);
                map32.offsets.resize(nelem);
                index_t off = 0;
                for(index_t ei = 0; ei < nelem; ei++)
                {
                    map32.sizes[ei] = elem_sizes[ei];
                    map32.offsets[ei] = off;
                    off += elem_sizes[ei];
                }
            }
        }
        else
        {
#endif
            // We can directly use the polyhedral faces as the 2D topology.
            // Copy these fields into the topo but convert to index_t.
            std::vector<std::string> src_keys{"connectivity", "sizes", "offsets"};
            std::vector<std::string> dest_keys{"elements/connectivity",
                                               "elements/sizes",
                                               "elements/offsets"};
            copy_convert(src_keys, subel, DataType::index_t(), dest_keys, node);

            // While we're at it, we can use the elements/connectivity as the
            // G(3,2) map.
            if(map32.requested)
            {
                index_t_accessor conn = topo->fetch_existing("elements/connectivity").value();
                map32.data.resize(conn.number_of_elements());
                for(index_t i = 0; i < conn.number_of_elements(); i++)
                    map32.data[i] = conn[i];

                index_t_accessor esizes = topo->fetch_existing("elements/sizes").value();
                map32.sizes.resize(esizes.number_of_elements());
                for(index_t i = 0; i < esizes.number_of_elements(); i++)
                    map32.sizes[i] = esizes[i];

                if(topo->has_path("elements/offsets"))
                {
                    index_t_accessor offsets = topo->fetch_existing("elements/offsets").value();
                    map32.offsets.resize(offsets.number_of_elements());
                    for(index_t i = 0; i < offsets.number_of_elements(); i++)
                        map32.offsets[i] = offsets[i];
                }
            }
#ifdef REPRODUCE_REFERENCE
        }
#endif
    }

    //-----------------------------------------------------------------------
    /**
      @brief Make the embedded connectivity from the input connectivity and
             store the results in dim_topos[shape.dim-1]. If we're passed a
             3D cell, we make 2D faces. If we're passed 2D faces, we make 1D
             lines.

      @param shape The input shape.
      @param conn  The connectivity data. This is usually a pointer but can
                   be an accessor too.
      @param connlen The length of the connectivity data.

      - This method assumes that all elements in the connectivity are the 
        same type.   

      - This is a template method so we can pass in basic types (like 
        const int*) for the connectivity.

      - The terms used in this method are "faces" but it is really whichever
        entity has shape.dim-1 in the shape cascade
    */
    template <typename ConnType>
    void
    make_embedded_connectivity(const ShapeType &shape, const ConnType &conn, size_t connlen)
    {
        CONDUIT_ANNOTATE_MARK_FUNCTION;

        auto embed_dim = shape.dim - 1;
        ShapeType embed_shape = topo_cascade.get_shape(embed_dim);

        // Get sizes from the shape and embed_shape.
        index_t points_per_elem = shape.indices;
        index_t faces_per_elem = shape.embed_count;
        index_t points_per_face = embed_shape.indices;
        index_t nfacepts = faces_per_elem * points_per_face;

        index_t nelem = static_cast<index_t>(connlen) / points_per_elem;
        auto nelem_faces = nelem * faces_per_elem;
#ifdef DEBUG_PRINT
        std::cout << "=======================================================" << std::endl;
        std::cout << "make_embedded_connectivity: shape_dim=" << shape.dim << std::endl;
        std::cout << "=======================================================" << std::endl;
        std::cout << "shape=" << shape.type() << std::endl;
        std::cout << "embed_shape=" << embed_shape.type() << std::endl;
        std::cout << "points_per_elem="<<points_per_elem<<std::endl;
        std::cout << "faces_per_elem="<<faces_per_elem<<std::endl;
        std::cout << "points_per_face="<<points_per_face<<std::endl;
        std::cout << "nfacepts="<<nfacepts<<std::endl;
        std::cout << "nelem="<<nelem<<std::endl;
        std::cout << "nelem_faces="<<nelem_faces<<std::endl;
        std::cout << "conn={";
        for(index_t i = 0; i < connlen; i++)
            std::cout << conn[i] << ", ";
        std::cout << "}" << endl << std::endl;
#endif

        // Iterate over each element and compute a faceid for it. Store
        // these in faceid_to_ef. The "ef" stands for element face, which
        // is the element id * faces_per_elem + face.
        CONDUIT_ANNOTATE_MARK_BEGIN("Labeling");
        std::vector<std::pair<uint64, uint64>> faceid_to_ef(nelem_faces);

        conduit::execution::for_all<ParallelExec>(0, nelem, [&](index_t elem)
        {
            // Get the element faces, storing them all in face_pts.
            index_t elemstart = elem * points_per_elem;
            constexpr index_t max_nfacepts = 6 * 4;
            index_t face_pts[max_nfacepts];
            for(index_t i = 0; i < nfacepts; i++)
                face_pts[i] = conn[elemstart + shape.embedding[i]];

            // Make a unique id for each face.
            index_t facestart = elem * faces_per_elem;
            for(index_t face = 0; face < faces_per_elem; face++)
            {
                // Sort the face's points
                index_t *face_pts_start = &face_pts[face * points_per_face];
                index_t *face_pts_end = face_pts_start + points_per_face;

                // encode element and face into element_face.
                uint64 element_face = facestart + face;

                std::sort(face_pts_start, face_pts_end);
                uint64 faceid = conduit::utils::hash(face_pts_start,
                                    static_cast<unsigned int>(points_per_face));

                // Store the faceid and ef values.
                faceid_to_ef[element_face] = std::make_pair(faceid, element_face);
            }
        });
        CONDUIT_ANNOTATE_MARK_END("Labeling");

#ifdef DEBUG_PRINT
        std::cout << "faceid_to_ef = " << faceid_to_ef << std::endl;
#endif

        // Sort faceid_to_ef so any like faces will be sorted, first by their
        // general faceid, then by their elemface "ef", which should keep the
        // elements in order.
        CONDUIT_ANNOTATE_MARK_BEGIN("Sort labels");
        conduit::execution::sort<ParallelExec>(faceid_to_ef.begin(), faceid_to_ef.end());
        CONDUIT_ANNOTATE_MARK_END("Sort labels");
#ifdef DEBUG_PRINT
        std::cout << "faceid_to_ef.sorted = " << faceid_to_ef << std::endl;
#endif

        // Faces are sorted. We probably do not want to necessarily create faces
        // in this order though since it would create faces in random order.
        // The spans with like ids should only be 1 or 2 faces long, assuming the
        // hashing did its job correctly.
        CONDUIT_ANNOTATE_MARK_BEGIN("Sort ef->unique");
        std::vector<std::pair<uint64, uint64>> ef_to_unique(nelem_faces);
        index_t unique = make_unique(faceid_to_ef, ef_to_unique);
#ifdef DEBUG_PRINT
        std::cout << "unique = " << unique << std::endl;
        std::cout << "ef_to_unique = " << ef_to_unique << std::endl;
#endif

        // Sort on ef to get back to a ef->unique mapping.
        conduit::execution::sort<ParallelExec>(
            ef_to_unique.begin(), ef_to_unique.end(),
            [&](const std::pair<uint64, uint64> &lhs, const std::pair<uint64, uint64> &rhs)
            {
                // Only sort using the ef value.
                return lhs.first < rhs.first;
            });
        CONDUIT_ANNOTATE_MARK_END("Sort ef->unique");
#ifdef DEBUG_PRINT
        std::cout << "ef_to_unique.sorted = " << ef_to_unique << std::endl;
#endif

        // Store the new embed connectivity data in Conduit nodes.
        conduit::Node &node = dim_topos[embed_shape.dim];
        node["type"] = "unstructured";
        node["coordset"] = coords->name();
        node["elements/shape"] = embed_shape.type();
        node["elements/connectivity"].set(DataType::index_t(unique * points_per_face));
        index_t *embed_conn = node["elements/connectivity"].as_index_t_ptr();
        index_t embed_conn_idx = 0;

        // Now ef_to_unique contains a list of unique face ids but they are not
        // expressed in element-creation order.
        //
        // elem 0 faces            elem 1 faces
        // {99, 1, 4, 2, 11, 16}, {4, 22, 44, 67, 55, 12}, ...
        //
        // Now we want to iterate over the ef_to_unique in element face order 
        // and renumber the faces in another pass.
        //
        // If the (3,2) or (2,1) association was requested then we do a little
        // more work to save the map as we build the connectivity. Otherwise,
        // the 2 code blocks do the same thing. We separate to remove branches.
        if(G[shape.dim][embed_shape.dim].requested)
        {
            CONDUIT_ANNOTATE_MARK_SCOPE("Build connectivity and map");

            // Association data. This is the set of indices that point to the
            // embedded shapes from each input shape. Think of it like the set
            // of polyhedral face ids for each element but it works for other
            // dimensions too.
            auto &embed_refs = G[shape.dim][embed_shape.dim].data;
            embed_refs.resize(nelem_faces, 0);
#ifdef DEBUG_PRINT
            std::cout << "Building G(" << shape.dim << ", " << embed_shape.dim << ")" << std::endl;
            std::cout << "points_per_face=" << points_per_face << std::endl;
            std::cout << "unique=" << unique << std::endl;
            std::cout << "faces_per_elem=" << faces_per_elem << std::endl;
            std::cout << "nelem_faces=" << nelem_faces << std::endl;
#endif

            // Save how many embedded shapes an element would have.
            G[shape.dim][embed_shape.dim].single_size = shape.embed_count;

            // Make the embedded connectivity (and map data)
            index_t embed_refs_idx = 0, final_faceid = 0;
            constexpr index_t FACE_AVAILABLE = -1;
            std::vector<index_t> face_reorder(unique, FACE_AVAILABLE);
            for(index_t ef = 0; ef < nelem_faces; ef++)
            {
                uint64 unique_face_id = ef_to_unique[ef].second;

                if(face_reorder[unique_face_id] == FACE_AVAILABLE)
                {
                    // Store the map data.
                    face_reorder[unique_face_id] = final_faceid++;
                    embed_refs[embed_refs_idx++] = face_reorder[unique_face_id];

                    // Emit the face definition (as defined by the first element
                    // that referenced it.
                    index_t faceelem = ef / faces_per_elem;
                    index_t facecase = ef % faces_per_elem;
                    index_t elemstart = faceelem * points_per_elem;
                    index_t *embed = &shape.embedding[facecase * points_per_face];
                    for(index_t i = 0; i < points_per_face; i++)
                        embed_conn[embed_conn_idx++] = conn[elemstart + embed[i]];
                }
                else
                {
                    // Store the map data. The entity has already been output,
                    // add its reordered number to the refs.
                    embed_refs[embed_refs_idx++] = face_reorder[unique_face_id];
                }
            }
#ifdef DEBUG_PRINT
            std::cout << "final embed_refs_idx=" << embed_refs_idx << endl << std::endl;
            std::cout << "embed_refs=" << embed_refs << std::endl;
#endif
        }
        else
        {
            CONDUIT_ANNOTATE_MARK_SCOPE("Build connectivity");

            // Make the embedded connectivity
            std::vector<unsigned char> avail(unique, 1);
            for(index_t ef = 0; ef < nelem_faces; ef++)
            {
                uint64 unique_face_id = ef_to_unique[ef].second;
                if(avail[unique_face_id])
                {
                    avail[unique_face_id] = 0;

                    // Emit the face definition (as defined by the first element
                    // that referenced it.
                    index_t faceelem = ef / faces_per_elem;
                    index_t facecase = ef % faces_per_elem;
                    index_t elemstart = faceelem * points_per_elem;
                    index_t *embed = &shape.embedding[facecase * points_per_face];
                    for(index_t i = 0; i < points_per_face; i++)
                        embed_conn[embed_conn_idx++] = conn[elemstart + embed[i]];
                }
            }
        }

        // Generate offsets in the output connectivity. Some downstream algorithms want it.
        CONDUIT_ANNOTATE_MARK_BEGIN("Build offsets");
        node["elements/offsets"].set(DataType::index_t(unique));
        index_t *offsets = node["elements/offsets"].as_index_t_ptr();
        for(index_t ei = 0; ei < unique; ei++)
            offsets[ei] = points_per_face * ei;
        CONDUIT_ANNOTATE_MARK_END("Build offsets");
    }

    //-----------------------------------------------------------------------
    /**
     @brief This method makes embedded connectivity 2D->1D but uses sizes/
            offsets to access the shape data as when the input 2D topology
            contains polygons.

            It's really only used to go from 2D polygons to 1D lines.
     */
    template <typename ConnType>
    void
    make_embedded_connectivity_polygons_to_lines(const ConnType &conn)
    {
        CONDUIT_ANNOTATE_MARK_FUNCTION;

#ifdef DEBUG_PRINT
        std::cout << "=======================================================" << std::endl;
        std::cout << "make_embedded_connectivity_polygons_to_lines:" << std::endl;
        std::cout << "=======================================================" << std::endl;
#endif

        index_t_accessor sizes = dim_topos[2].fetch_existing("elements/sizes").value();
        const index_t *offsets = dim_topos[2].fetch_existing("elements/offsets").value();
        index_t nelem = sizes.number_of_elements();

        // Iterate over each polygon and make unique edges.
        CONDUIT_ANNOTATE_MARK_BEGIN("Labeling");
        index_t nelem_edges = sizes.sum();
        std::vector<std::pair<uint64, uint64>> edgeid_to_ee(nelem_edges);
        std::vector<std::pair<index_t, index_t>> ee_to_edge(nelem_edges);

        conduit::execution::for_all<ParallelExec>(0, nelem, [&](index_t elem)
        {
            constexpr size_t MAX_VERTS = 32;

            // Get the element points, storing them in pts.
            index_t elem_size = sizes[elem];
            index_t elem_offset = offsets[elem];
            index_t pts[MAX_VERTS];
            for(index_t i = 0; i < elem_size; i++)
                pts[i] = conn[elem_offset + i];

            // Make a unique id for each edge.
            for(index_t edge_index = 0; edge_index < elem_size; edge_index++)
            {
                // encode element and edge into element_edge.
                uint64 elem_edge = elem_offset + edge_index;

                // Make the edge.
                index_t next_edge_index = (edge_index + 1) % elem_size;
                index_t edge[2];
                edge[0] = pts[edge_index];
                edge[1] = pts[next_edge_index];
                ee_to_edge[elem_edge] = std::make_pair(edge[0], edge[1]);

                // Store the edgeid.
                if(edge[0] > edge[1])
                    std::swap(edge[0], edge[1]);
                uint64 edgeid = conduit::utils::hash(edge, 2);
                edgeid_to_ee[elem_edge] = std::make_pair(edgeid, elem_edge);
            }
        });
        CONDUIT_ANNOTATE_MARK_END("Labeling");
#ifdef DEBUG_PRINT
        std::cout << "edgeid_to_ee = " << edgeid_to_ee << std::endl;
#endif

        // Sort edgeid_to_ee so any like edges will be sorted.
        CONDUIT_ANNOTATE_MARK_BEGIN("Sort labels");
        conduit::execution::sort<ParallelExec>(edgeid_to_ee.begin(), edgeid_to_ee.end());
        CONDUIT_ANNOTATE_MARK_END("Sort labels");
#ifdef DEBUG_PRINT
        std::cout << "edgeid_to_ee.sorted = " << edgeid_to_ee << std::endl;
#endif

        // Edges are sorted. Pick out the unique edge ids.
        CONDUIT_ANNOTATE_MARK_BEGIN("Sort ef->unique");
        std::vector<std::pair<uint64, uint64>> ee_to_unique(nelem_edges);
        index_t unique = make_unique(edgeid_to_ee, ee_to_unique);
#ifdef DEBUG_PRINT
        std::cout << "unique = " << unique << std::endl;
        std::cout << "ee_to_unique = " << ee_to_unique << std::endl;
#endif

        // Sort on ef to get back to a ef->unique mapping.
        conduit::execution::sort<ParallelExec>(
            ee_to_unique.begin(), ee_to_unique.end(),
            [&](const std::pair<uint64, uint64> &lhs, const std::pair<uint64, uint64> &rhs)
            {
                // Only sort using the ee value.
                return lhs.first < rhs.first;
            });
        CONDUIT_ANNOTATE_MARK_END("Sort ef->unique");
#ifdef DEBUG_PRINT
        std::cout << "ee_to_unique.sorted = " << ee_to_unique << std::endl;
#endif

        // Store the new embed connectivity data in Conduit nodes.
        conduit::Node &node = dim_topos[1];
        node["type"] = "unstructured";
        node["coordset"] = coords->name();
        node["elements/shape"] = "line";
        node["elements/connectivity"].set(DataType::index_t(unique * 2));
        index_t *embed_conn = node["elements/connectivity"].as_index_t_ptr();
        index_t embed_conn_idx = 0;

        // Now ee_to_unique contains a list of unique edge ids.
        std::vector<unsigned char> avail(unique, 1);
        if(G[2][1].requested)
        {
            CONDUIT_ANNOTATE_MARK_SCOPE("Build connectivity and map");

            // Association data. This is the set of indices that point to the
            // embedded shapes from each input shape. Think of it like the set
            // of polyhedral face ids for each element but it works for other
            // dimensions too.
            std::vector<index_t> edge_reorder(unique);
            auto &embed_refs = G[2][1].data;
            embed_refs.resize(nelem_edges, 0);

            // Make the embedded connectivity (and map data)
            index_t embed_refs_idx = 0, final_edgeid = 0;
            for(index_t ee = 0; ee < nelem_edges; ee++)
            {
                uint64 elem_edge = ee_to_unique[ee].first;
                uint64 unique_edge_id = ee_to_unique[ee].second;

                if(avail[unique_edge_id])
                {
                    // Store the map data.
                    edge_reorder[unique_edge_id] = final_edgeid++;
                    embed_refs[embed_refs_idx++] = edge_reorder[unique_edge_id];

                    avail[unique_edge_id] = 0;

                    // Emit the edge definition (as defined by the first element
                    // that referenced it).
                    embed_conn[embed_conn_idx++] = ee_to_edge[elem_edge].first;
                    embed_conn[embed_conn_idx++] = ee_to_edge[elem_edge].second;
                }
                else
                {
                    // Store the map data. The entity has already been output,
                    // add its reordered number to the refs.
                    embed_refs[embed_refs_idx++] = edge_reorder[unique_edge_id];
                }
            }
#ifdef DEBUG_PRINT
            std::cout << "final embed_refs_idx=" << embed_refs_idx << endl << std::endl;
            std::cout << "embed_refs=" << embed_refs << std::endl;
#endif

            // Make sizes/offsets for G(2,1).
            auto &embed_sizes = G[2][1].sizes;
            auto &embed_offsets = G[2][1].offsets;
            embed_sizes.resize(nelem);
            embed_offsets.resize(nelem);
            for(index_t elem = 0; elem < nelem; elem++)
            {
                embed_sizes[elem] = sizes[elem];
                embed_offsets[elem] = offsets[elem];
            }
        }
        else
        {
            CONDUIT_ANNOTATE_MARK_SCOPE("Build connectivity");

            // Make the embedded connectivity
            for(index_t ee = 0; ee < nelem_edges; ee++)
            {
                uint64 elem_edge = ee_to_unique[ee].first;
                uint64 unique_edge_id = ee_to_unique[ee].second;
                if(avail[unique_edge_id])
                {
                    avail[unique_edge_id] = 0;

                    // Emit the face definition (as defined by the first element
                    // that referenced it).
                    embed_conn[embed_conn_idx++] = ee_to_edge[elem_edge].first;
                    embed_conn[embed_conn_idx++] = ee_to_edge[elem_edge].second;
                }
            }
        }

        // Generate offsets in the output connectivity. Some downstream algorithms want it.
        CONDUIT_ANNOTATE_MARK_BEGIN("Build offsets");
        node["elements/offsets"].set(DataType::index_t(unique));
        index_t *line_offsets = node["elements/offsets"].as_index_t_ptr();
        for(index_t ei = 0; ei < unique; ei++)
            line_offsets[ei] = 2 * ei;
        CONDUIT_ANNOTATE_MARK_END("Build offsets");
    }

    //-----------------------------------------------------------------------
    /**
     @brief Call the local association API to copy all of the local data for
            a specific map into contiguous data arrays. This is used to
            store the local map data into a Conduit node.

     @param src_dim The entity dimension e
     @param dst_dim The association dimension a
     @param values_ptr The array that will hold the map values.
     @param sizes_ptr The array that will hold the map sizes.
     @param offsets_ptr The array that will hold the map offsets.
     @param N The number of local map entities for L(e,a).
     */
    template <typename T>
    void
    copy_local_map(index_t src_dim, index_t dst_dim,
        T *values_ptr,
        T *sizes_ptr,
        T *offsets_ptr,
        index_t N) const
    {
        // Do another pass to store the data in the nodes.
        index_t off = 0;
        for(index_t eid = 0; eid < N; eid++)
        {
            auto lm = get_local_association(eid, src_dim, dst_dim);
            // Copy lm values into values array.
            for(auto lmval : lm)
                *values_ptr++ = static_cast<T>(lmval);

            sizes_ptr[eid] = static_cast<T>(lm.size());
            offsets_ptr[eid] = static_cast<T>(off);
            off += lm.size();
        }
    }

    //-----------------------------------------------------------------------
    /**
     @brief This function is used to iterate over global map levels according
            to the values stored in levels. At each level, we apply a function.
            This is used to count local indices and to generate local maps.

     @param entity_id The entity id.
     @param levels    The vector e,a pairs for each level.
     @param level     The current level.
     @param localIdx  An array of local indices.
     @param func      The function to apply to a set of e,a levels.
     */
    template <typename Func>
    void iterate_global_map_levels(index_t entity_id,
        const std::vector<std::vector<std::pair<int,int>>> &levels,
        size_t level,
        index_t *localIdx,
        Func &&func)
    {
        int e = levels[level][0].first;
        int a = levels[level][0].second;
        auto ents = G[e][a].get_data(entity_id);
        for(index_t j = 0; j < ents.second; j++)
        {
            index_t child_entity_id = ents.first[j];
            func(levels[level], localIdx);
            size_t nextLevel = level + 1;
            if(nextLevel < levels.size())
            {
                iterate_global_map_levels(child_entity_id, levels, nextLevel,
                                          localIdx, func);
            }
        }
        localIdx[e]++;
    }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/**
 @brief Get the data for one entity.

 @param entity_id The entity id whose data we want to return.
 @return A pair containing the data pointer and array size.
 */
std::pair<index_t *, index_t>
TopologyMetadata::Implementation::association::get_data(index_t entity_id) const
{
    std::pair<index_t *, index_t> retval;
    if(!data.empty())
    {
        index_t size = get_size(entity_id);
        index_t offset = get_offset(entity_id);
        retval = std::make_pair(const_cast<index_t *>(&data[offset]), size);
    }
    else
    {
        retval = std::make_pair(nullptr, 0);
    }
    return retval;
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::Implementation::association::get_size(index_t entity_id) const
{
    return sizes.empty() ? single_size : sizes[entity_id];
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::Implementation::association::get_offset(index_t entity_id) const
{
    return offsets.empty() ? (entity_id * single_size) : offsets[entity_id];
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::Implementation::association::sum_sizes(index_t num_entities) const
{
    index_t sum = 0;
    if(sizes.empty())
    {
        // single size case.
        sum = num_entities * single_size;
    }
    else
    {
        for(size_t i = 0; i < sizes.size(); i++)
            sum += sizes[i];
    }
    return sum;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

TopologyMetadata::Implementation::Implementation(const conduit::Node &topology,
    const conduit::Node &coordset) : TopologyMetadataBase(),
    topo(&topology), coords(&coordset), topo_cascade(topology), topo_shape(topology),
    lowest_cascade_dim(0), coords_length(0),
    int_dtype(find_widest_dtype(link_nodes(topology, coordset), DEFAULT_INT_DTYPES)),
    float_dtype(find_widest_dtype(link_nodes(topology, coordset), DEFAULT_FLOAT_DTYPE))
{
    // Select all maps that could be valid for this shape.
    std::vector<std::pair<size_t, size_t> > desired;
    auto maxdim = static_cast<size_t>(topo_shape.dim);
    for(size_t e = 0; e <= maxdim; e++)
    for(size_t a = 0; a <= maxdim; a++)
        desired.push_back(std::make_pair(e,a));

    initialize(desired);
}

//------------------------------------------------------------------------------
TopologyMetadata::Implementation::Implementation(const conduit::Node &topology,
    const conduit::Node &coordset,
    size_t lowest_dim,
    const std::vector<std::pair<size_t, size_t> > &desired) :
    TopologyMetadataBase(),
    topo(&topology), coords(&coordset), topo_cascade(topology), topo_shape(topology),
    lowest_cascade_dim(lowest_dim), coords_length(0),
    int_dtype(find_widest_dtype(link_nodes(topology, coordset), DEFAULT_INT_DTYPES)),
    float_dtype(find_widest_dtype(link_nodes(topology, coordset), DEFAULT_FLOAT_DTYPE))
{
    initialize(desired);
}

//------------------------------------------------------------------------------
void
TopologyMetadata::Implementation::initialize(const std::vector<std::pair<size_t, size_t> > &desired)
{
    // The lowest cascade dim is less than or equal to the topo_shape.dim.
    if(lowest_cascade_dim > static_cast<size_t>(topo_shape.dim))
    {
        CONDUIT_ERROR("lowest_cascade_dim is greater than the topo_shape.dim!");
    }

    // Initialize nodes/lengths.
    for(size_t dim = 0; dim < MAX_ENTITY_DIMS; dim++)
    {
        dim_topos[dim].reset();
        dim_topo_lengths[dim] = 0;
    }

    // Request the associations that we need to build.
    request_associations(desired);

    // Make the highest topology
    if(topo_shape.dim > 0)
        make_highest_topology();

    // Make point topology
    coords_length = coordset::length(*coords);
    if(lowest_cascade_dim == 0)
        make_point_topology();

    // If we have lines or faces to make, do it.
    if(lowest_cascade_dim < static_cast<size_t>(topo_shape.dim) && topo_shape.dim > 1)
    {
        if(topo_shape.is_polyhedral())
        {
            const conduit::Node &subel = topo->fetch_existing("subelements");
            const conduit::Node &sizes = subel.fetch_existing("sizes");
            dispatch_connectivity_ph(subel, sizes);
        }
        else
        {
            const conduit::Node &conn = topo->fetch_existing("elements/connectivity");
            dispatch_connectivity(topo_shape, conn);
        }
    }

    build_associations();
    build_local_to_global();
}

//---------------------------------------------------------------------------
const conduit::Node &
TopologyMetadata::Implementation::get_topology(size_t dim)
{
    if(dim > 3)
        CONDUIT_ERROR("Invalid dimension");
    if(dim_topos_int_dtype[dim].dtype().is_empty())
        convert_topology_dtype(dim, dim_topos_int_dtype[dim], false);

    return dim_topos_int_dtype[dim];
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::get_topology(size_t dim, conduit::Node &dest)
{
    if(dim > 3)
        CONDUIT_ERROR("Invalid dimension");
    convert_topology_dtype(dim, dest, true);
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::Implementation::get_topology_length(size_t dim) const
{
    if(dim > 3)
        CONDUIT_ERROR("Invalid dimension");
    return dim_topo_lengths[dim];
}

//------------------------------------------------------------------------------
void
TopologyMetadata::Implementation::convert_topology_dtype(size_t dim,
    conduit::Node &dest, bool forceCopy)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if(dim > 3)
        CONDUIT_ERROR("Invalid dimension");
    dest.reset();
    if(int_dtype.id() == DataType::index_t().id())
    {
        // The topology is already in the desired int_dtype. Reference it
        // in the dim_topos_int_dtype nodes.
        if(forceCopy)
            dest.set(dim_topos[dim]);
        else
            dest.set_external(dim_topos[dim]);
    }
    else
    {
        // The topology is not in the desired int_dtype. Convert it.
        const ShapeType shape(dim_topos[dim]);
        copy_topology(dim_topos[dim], shape, int_dtype, dest);
    }
}

//------------------------------------------------------------------------------
void
TopologyMetadata::Implementation::request_associations(const std::vector<std::pair<size_t, size_t> > &desired)
{
    auto maxdim = static_cast<size_t>(topo_shape.dim);
    for(size_t i = 0; i < desired.size(); i++)
    {
        auto e = desired[i].first;
        auto a = desired[i].second;
        if(e > maxdim || a > maxdim)
        {
            CONDUIT_ERROR("An invalid (e,a) association index was selected: (" <<
               e << ", " << a << ")");
        }
        G[e][a].requested = L[e][a].requested = true;
        if(e < a)
        {
            // This is a child to parent association.
            // Example: G[1][3] requested - all of the elements that contain
            //          edge i. G[1][3] relies on G[3][1] existing.
            G[a][e].requested = L[e][a].requested = true;
        }
    }

    if(topo_shape.is_polyhedral())
    {
        // Some associations are built using other associations.
        if(G[3][1].requested)
        {
            G[3][2].requested = L[3][2].requested = true;
            G[2][1].requested = L[3][2].requested = true;
        }
        if(G[3][0].requested)
        {
            G[3][2].requested = L[3][2].requested = true;
            G[2][1].requested = L[2][1].requested = true;
            G[1][0].requested = L[1][0].requested = true;
        }
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::dispatch_connectivity_ph(const conduit::Node &subel,
    const conduit::Node &sizes)
{
    // Dispatch to different versions based on connectivity type.
    index_t sizeslen = sizes.dtype().number_of_elements();
    if(sizes.dtype().is_int32())
        dispatch_shape_ph(subel, sizes.as_int32_ptr(), sizeslen);
    else if(sizes.dtype().is_uint32())
        dispatch_shape_ph(subel, sizes.as_uint32_ptr(), sizeslen);
    else if(sizes.dtype().is_int64())
        dispatch_shape_ph(subel, sizes.as_int64_ptr(), sizeslen);
    else if(sizes.dtype().is_uint64())
        dispatch_shape_ph(subel, sizes.as_uint64_ptr(), sizeslen);
    else
    {
        // Backup case. Use index_t accessor.
        dispatch_shape_ph(subel, sizes.as_index_t_accessor(), sizeslen);
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::dispatch_connectivity(const ShapeType &shape,
    const conduit::Node &conn)
{
    // Dispatch to different versions based on connectivity type.
    if(conn.dtype().is_int32())
        dispatch_shape(shape, conn.as_int32_ptr(), conn.dtype().number_of_elements());
    else if(conn.dtype().is_uint32())
        dispatch_shape(shape, conn.as_uint32_ptr(), conn.dtype().number_of_elements());
    else if(conn.dtype().is_int64())
        dispatch_shape(shape, conn.as_int64_ptr(), conn.dtype().number_of_elements());
    else if(conn.dtype().is_uint64())
        dispatch_shape(shape, conn.as_uint64_ptr(), conn.dtype().number_of_elements());
    else
    {
        // Backup case. Use index_t accessor.
        dispatch_shape(shape, conn.as_index_t_accessor(), conn.dtype().number_of_elements());
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::make_highest_topology()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // Copy the top level topo into dim_topos as index_t.
    conduit::Node &node = dim_topos[topo_shape.dim];
    copy_topology(*topo, topo_shape, DataType::index_t(), node);
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::copy_convert(
    const std::vector<std::string> &src_keys,
    const conduit::Node &n_src,
    const DataType &dest_type,
    const std::vector<std::string> &dest_keys,
    conduit::Node &n_dest)
{
    size_t n = std::min(src_keys.size(), dest_keys.size());
    for(size_t i = 0; i < n; i++)
    {
        if(n_src.has_path(src_keys[i]))
        {
            const conduit::Node &src = n_src[src_keys[i]];
            conduit::Node &dest = n_dest[dest_keys[i]];
            if(src.dtype().is_integer() && src.dtype().id() != dest_type.id())
            {
                // Bulk convert integer arrays
                dest.set(DataType(dest_type.id(), src.dtype().number_of_elements()));
                src.to_data_type(dest_type.id(), dest);
            }
            else
            {
                dest.set(src);
            }
        }
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::copy_topology(const conduit::Node &src_topo,
    const ShapeType &shape, const DataType &dest_type, conduit::Node &dest_topo)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // Reuse the input topology as the highest dimension's topology.
    dest_topo["type"] = "unstructured";
    dest_topo["coordset"] = coords->name();
    dest_topo["elements/shape"] = shape.type();

    // Names of fields to copy.
    std::vector<std::string> copy_keys{"elements/shape",
                                       "elements/connectivity",
                                       "elements/sizes",
                                       "elements/offsets"
                                      };
    if(shape.is_polyhedral())
    {
        copy_keys.push_back("subelements/connectivity");
        copy_keys.push_back("subelements/sizes");
        copy_keys.push_back("subelements/shape");
        copy_keys.push_back("subelements/offsets");
    }
    // Copy keys that exist, converting to the destination type if needed.
    copy_convert(copy_keys, src_topo, dest_type, copy_keys, dest_topo);

    // Make sure we have offsets if they are not there. The Conduit helper
    // routines make them in various precisions.
    conduit::Node n_offsets;
    if(!dest_topo.has_path("elements/offsets"))
    {
        if(shape.is_polyhedral())
        {
            conduit::Node &topo_suboffsets = dest_topo["subelements/offsets"];
            topology::unstructured::generate_offsets(dest_topo,
                                                     n_offsets,
                                                     topo_suboffsets);
        }
        else
        {
            topology::unstructured::generate_offsets(dest_topo, n_offsets);
        }

        // Convert the types if needed.
        conduit::Node &offsets = dest_topo["elements/offsets"];
        if(n_offsets.dtype().id() != dest_type.id())
        {
            index_t nvalues = n_offsets.dtype().number_of_elements();
            offsets.set(DataType(dest_type.id(), nvalues));
            n_offsets.to_data_type(dest_type.id(), offsets);
        }
        else
        {
            offsets.set(n_offsets);
        }
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::make_point_topology()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    conduit::Node &node = dim_topos[0];
    node["type"] = "unstructured";
    node["coordset"] = coords->name();
    node["elements/shape"] = "point";
    node["elements/connectivity"].set(DataType::index_t(coords_length));
    // Also use the connectivity as offsets (works for points).
    node["elements/offsets"].set_external(node["elements/connectivity"]);

    index_t *conn = node["elements/connectivity"].as_index_t_ptr();
    for(index_t i = 0; i < coords_length; i++)
        conn[i] = i;
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::Implementation::make_unique(
    const std::vector<std::pair<uint64, uint64>> &faceid_to_ef,
    std::vector<std::pair<uint64, uint64>> &ef_to_unique) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    size_t nids = faceid_to_ef.size();
    index_t unique = 0;
    size_t start = 0;
    for(size_t i = 1; i < nids; i++)
    {
        if(faceid_to_ef[start].first != faceid_to_ef[i].first)
        {
            for(size_t j = start; j < i; j++)
                ef_to_unique[j] = std::make_pair(faceid_to_ef[j].second, unique);
            unique++;
            start = i;
        }
    }
    for(size_t i = start; i < nids; i++)
        ef_to_unique[i] = std::make_pair(faceid_to_ef[i].second, unique);
    unique += (start < nids) ? 1 : 0;

    return unique;
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_associations()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // Some maps will need lengths of the topologies that were produced.
#ifdef DEBUG_PRINT
    std::cout << "build_associations: topo_shape.dim=" << topo_shape.dim << std::endl;
#endif
    for(int dim = static_cast<int>(topo_shape.dim); dim >= 0; dim--)
    {
#ifdef DEBUG_PRINT
        std::cout << "topo " << dim << std::endl;
        std::cout << "=======" << std::endl;
        yaml_print(std::cout, dim_topos[dim]);
        std::cout << std::endl;
#endif
        conduit::Node info;
        if(conduit::blueprint::mesh::topology::verify(dim_topos[dim], info))
            dim_topo_lengths[dim] = conduit::blueprint::mesh::utils::topology::length(dim_topos[dim]);
        else
            dim_topo_lengths[dim] = 0;
    }

    // e,a association build order. See G[e][a] table.
    const int eaorder[][2] = {{3,3},{2,2},{1,1},{0,0},
                              {3,2},{2,1},{1,0},
                              {3,1},{2,0},
                              {3,0},
                              {2,3},{1,2},{0,1},
                              {1,3},{0,2},
                              {0,3}};

    // Set the single_size values. This is the number of items in the data
    // that are grouped together.
    if(topo_shape.dim == 3)
    {
        // NOTE: For polyhedra, we don't use single_shape.
        G[3][3].single_size = 1;
        if(!topo_shape.is_polyhedral())
        {
            G[3][2].single_size = static_cast<int>(topo_shape.embed_count);
            G[3][1].single_size = static_cast<int>(embedding_3_1_edges(topo_shape).size() / 2); // #unique edges
            G[3][0].single_size = static_cast<int>(topo_shape.indices);
        }
    }
    if(topo_shape.dim >= 2)
    {
        G[2][3].single_size = 1;
        G[2][2].single_size = 1;
        if(!dim_topos[2].dtype().is_empty())
        {
            // Get the shape from the topology (in case we changed it).
            ShapeType shape(dim_topos[2]);
            G[2][1].single_size = static_cast<int>(shape.embed_count);
            G[2][0].single_size = static_cast<int>(shape.indices);
        }
    }
    if(topo_shape.dim >= 1)
    {
        G[1][3].single_size = 1;
        G[1][2].single_size = 1;
        G[1][1].single_size = 1;
        if(!dim_topos[1].dtype().is_empty())
        {
            // Get the shape from the topology (in case we changed it).
            ShapeType shape(dim_topos[1]);
            G[1][0].single_size = static_cast<int>(shape.indices);
        }
    }
    G[0][3].single_size = 1;
    G[0][2].single_size = 1;
    G[0][1].single_size = 1;
    G[0][0].single_size = 1;

#ifdef DEBUG_PRINT
    for(int e = 3; e >= 0; e--)
    {
        for(int a = 3; a >= 0; a--)
            std::cout << G[e][a].single_size << ", ";
        std::cout << std::endl;
    }
#endif

    bool associations_31_30_need_built = true;
    for(int bi = 0; bi < 16; bi++)
    {
        int e = eaorder[bi][0];
        int a = eaorder[bi][1];
        if(G[e][a].requested)
        {
#ifdef DEBUG_PRINT
            std::cout << "Building association " << e << ", " << a << std::endl;
#endif
            index_t mapcase = EA_INDEX(e,a);
            switch(mapcase)
            {
            // Self cases.
            case EA_INDEX(0,0):
                // Falls through
            case EA_INDEX(1,1):
                // Falls through
            case EA_INDEX(2,2):
                // Falls through
            case EA_INDEX(3,3):
                G[e][a].data.resize(dim_topo_lengths[e]);
                std::iota(G[e][a].data.begin(), G[e][a].data.end(), 0);
                // We don't need to fill out sizes, offsets.
                break;

            // Connectivity cases
            case EA_INDEX(1,0):
                build_association_e_0(1);
                break;
            case EA_INDEX(2,0):
                build_association_e_0(2);
                break;

            // Cases that depend on face matching.
            case EA_INDEX(3,1):
                // Falls through
            case EA_INDEX(3,0):
                if(associations_31_30_need_built)
                {
                    build_association_3_1_and_3_0();
                    associations_31_30_need_built = false;
                }
                break;

            // Child to parent cases.
            case EA_INDEX(0,1):
                // Falls through
            case EA_INDEX(0,2):
                // Falls through
            case EA_INDEX(0,3):
                // Falls through
            case EA_INDEX(1,2):
                // Falls through
            case EA_INDEX(1,3):
                // Falls through
            case EA_INDEX(2,3):
                build_child_to_parent_association(e, a);
                break;

            // These are handled during construction. (no-op)
            case EA_INDEX(2,1):
                break;
            case EA_INDEX(3,2):
                break;
            }
        }
    }

    // Now that global associations are built, make any local associations.
    build_local_associations();
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_association_e_0(int e)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    auto copy_as_index_t = [&](const conduit::Node &node, const std::string &key,
                               std::vector<index_t> &dest)
    {
        if(node.has_path(key))
        {
            const conduit::Node &n = node[key];
            index_t_accessor src = n.as_index_t_accessor();
            size_t sz = static_cast<size_t>(src.number_of_elements());
            dest.resize(sz);
            for(size_t i = 0; i < sz; i++)
                dest[i] = src[i];
        }
    };

    // Save connectivity data in the association.
    association &assoc = G[e][0];
    copy_as_index_t(dim_topos[e], "elements/connectivity", assoc.data);
    copy_as_index_t(dim_topos[e], "elements/sizes", assoc.sizes);
    copy_as_index_t(dim_topos[e], "elements/offsets", assoc.offsets);
}

//---------------------------------------------------------------------------
std::vector<index_t>
TopologyMetadata::Implementation::embedding_3_1_edges(const ShapeType &shape) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    std::vector<index_t> retval;
    std::set<std::pair<index_t, index_t>> edges;
    auto npts_per_face = TOPO_SHAPE_INDEX_COUNTS[shape.embed_id];
    for(index_t fi = 0; fi < shape.embed_count; fi++)
    {
        const index_t *facepts = shape.embedding + fi * npts_per_face;
        for(index_t pi = 0; pi < npts_per_face; pi++)
        {
            index_t pi_next = (pi + 1) % npts_per_face;
            index_t i0 = facepts[pi];
            index_t i1 = facepts[pi_next];
            auto key = std::make_pair(std::min(i0, i1), std::max(i0, i1));
            if(edges.find(key) == edges.end())
            {
                edges.insert(key);
                retval.push_back(i0);
                retval.push_back(i1);
            }
        }
    }
    return retval;
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_association_3_1_and_3_0()
{
    if(topo_shape.is_polyhedral())
        build_association_3_1_and_3_0_ph();
    else
        build_association_3_1_and_3_0_nonph();
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_association_3_1_and_3_0_ph()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // G(3,2) contains the PH faces.
    const association &map32 = G[3][2];
    index_t nelem = dim_topo_lengths[3];

    // G(2,1) contains the face to edges map
    const association &map21 = G[2][1];
    index_t nedges = dim_topo_lengths[1];
    std::vector<int> edge_count(nedges, 0);
    //std::vector<int> edge_count_ever(nedges, 0);

    // Assume hex ph, which have 12 edges per element. For ph elements that
    // do not fit the assumption, we'll just let the vector resize.
    association &map31 = G[3][1];
    map31.data.reserve(nelem * 12);
    map31.sizes.resize(nelem, 0);
    map31.offsets.resize(nelem, 0);

    // Prepare G(3,0)
    association &map30 = G[3][0];
    std::vector<int> point_count;
    if(map30.requested)
    {
        map30.data.reserve(nelem * 12);
        map30.sizes.resize(nelem, 0);
        map30.offsets.resize(nelem, 0);

        index_t npts = dim_topo_lengths[0];
        point_count.resize(npts, 0);
    }

    // Reserve space so we can build the local_to_global[0] and
    // local_to_global[1] maps too. We do it here since they need much of the
    // same infrastructure as the G(3,1) and G(3,0) maps. We make some guesses.
    index_t edges_per_elem = map32.get_size(0) * map21.get_size(0);
    local_to_global[1].reserve(2 * nelem * edges_per_elem);
    local_to_global[0].reserve(coords_length + 2 * 2 * nelem * edges_per_elem);
    for(index_t i = 0; i < coords_length; i++)
        local_to_global[0].push_back(i);

    // Look through the edge connectivity and make a map that we can use to turn
    // an edge expressed as global point ids into a unique edge id.
    conduit::index_t_accessor conn1D = dim_topos[1]["elements/connectivity"].value();
    std::vector<std::pair<uint64, index_t>> edge_key_to_id(nedges);
    build_edge_key_to_id(edge_key_to_id);

    const index_t *face_conn = dim_topos[2]["elements/connectivity"].value();
    const index_t *face_sizes = dim_topos[2]["elements/sizes"].value();
    const index_t *face_offsets = dim_topos[2]["elements/offsets"].value();

    // Iterate over the elements and then the faces for each element so we
    // build up a set of edges used in this element.
    for(index_t ei = 0; ei < nelem; ei++)
    {
        // Store where this element's values start.
        map31.offsets[ei] = map31.data.size();
        if(map30.requested)
            map30.offsets[ei] = map30.data.size();
            
        // Iterate over this element's faces.
        auto elemfaces = map32.get_data(ei);
        for(index_t fi = 0; fi < elemfaces.second; fi++)
        {
            index_t faceid = elemfaces.first[fi];
            index_t this_face_offset = face_offsets[faceid];
            // Iterate over the edges in this face.
            for(index_t pi = 0; pi < face_sizes[faceid]; pi++)
            {
                index_t pi_next = (pi + 1) % face_sizes[faceid];

                // Make the edge.
                index_t edge[2];
                edge[0] = face_conn[this_face_offset + pi];
                edge[1] = face_conn[this_face_offset + pi_next];

                // Build the local_to_global[0] map (before possible swap)
                local_to_global[0].push_back(edge[0]);
                local_to_global[0].push_back(edge[1]);

                if(map30.requested && point_count[edge[0]] == 0)
                {
                    map30.data.push_back(edge[0]);
                    map30.sizes[ei]++;
                    point_count[edge[0]]++;
                }
                if(map30.requested && point_count[edge[1]] == 0)
                {
                    map30.data.push_back(edge[1]);
                    map30.sizes[ei]++;
                    point_count[edge[1]]++;
                }

                // Look up the edge id in our list of real edges.
                if(edge[1] > edge[0])
                    std::swap(edge[1], edge[0]);
                uint64 key = conduit::utils::hash(edge, 2);
                index_t edgeid = lookup_edge_id(edge_key_to_id, key);

                // Store the edge id in the map if it is the first time we've.
                if(edge_count[edgeid] == 0)
                {
                    map31.data.push_back(edgeid);
                    map31.sizes[ei]++;
                    edge_count[edgeid]++;
                }

                // Build the local_to_global[1] map.
                local_to_global[1].push_back(edgeid);
            }
        }

        // We're done with this element. Zero out the counts for the edges
        // and points that we saw in this element.
        for(index_t edge_index = 0; edge_index < map31.sizes[ei]; edge_index++)
        {
            index_t edgeid = map31.data[map31.offsets[ei] + edge_index];
            edge_count[edgeid] = 0;
        }
        if(map30.requested)
        {
            for(index_t pi = 0; pi < map30.sizes[ei]; pi++)
            {
                index_t ptid = map30.data[map30.offsets[ei] + pi];
                point_count[ptid] = 0;
            }
        }
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_edge_key_to_id(
    std::vector<std::pair<uint64, index_t>> &edge_key_to_id) const
{
    conduit::index_t_accessor conn1D = dim_topos[1]["elements/connectivity"].value();
    index_t nedges = conn1D.number_of_elements() / 2;
#ifdef DEBUG_PRINT
    std::cout << "edges_key_to_id = {" << std::endl;
    // Because of the printing.
    using Exec = SerialExec;
#else
    using Exec = ParallelExec;
#endif
    conduit::execution::for_all<Exec>(0, nedges, [&](index_t edge_index)
    {
        // Make a key for this edge.
        index_t edge[2];
        edge[0] = conn1D[edge_index * 2];
        edge[1] = conn1D[edge_index * 2 + 1];
        if(edge[1] > edge[0])
            std::swap(edge[0], edge[1]);
        uint64 key = conduit::utils::hash(edge, 2);
        // Store the edge in the map.
        edge_key_to_id[edge_index] = std::make_pair(key, edge_index);
#ifdef DEBUG_PRINT
        std::cout << std::setw(4) << edge_index << ": key=" <<  std::setw(20) << key
             << ", pts=" << std::setw(8) << edge[0] << ", "
             << std::setw(8) << edge[1] << std::endl;
#endif
    });
#ifdef DEBUG_PRINT
    std::cout << "}" << std::endl;
#endif

    // Sort the edges by the ids.
    conduit::execution::sort<ParallelExec>(
        edge_key_to_id.begin(), edge_key_to_id.end(),
        [&](const std::pair<uint64, index_t> &lhs,
            const std::pair<uint64, index_t> &rhs) 
        {
            return lhs.first < rhs.first;
        });
}

//---------------------------------------------------------------------------
    // This function looks up a key in edge_key_to_id to return the edge id.
index_t
TopologyMetadata::Implementation::lookup_edge_id(
    std::vector<std::pair<uint64, index_t>> &edge_key_to_id,
    const uint64 key) const
{
    index_t index = -1;
    index_t left = 0;
    index_t right = edge_key_to_id.size() - 1;
    while(left <= right)
    {
        index_t m = (left + right) / 2;
        if(edge_key_to_id[m].first < key)
            left = m + 1;
        else if(edge_key_to_id[m].first > key)
            right = m - 1;
        else
        {
            index = m;
            break;
        }
    }
    return edge_key_to_id[index].second;
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_association_3_1_and_3_0_nonph()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // This is the non PH method.

// Q: can we assume that we have index_t connectivity at this stage?

    // This method kind of refines the 3D shape from the start to make edge
    // id pairs that we look up in the edge connectivity. This is done because
    // it turned out to be problematic using the maps alone due to shared faces
    // being rotated among elements and then their edges could have mismatched
    // orientations.
    conduit::index_t_accessor conn3D = dim_topos[3]["elements/connectivity"].value();
    index_t connlen = conn3D.number_of_elements();
    index_t points_per_elem = topo_shape.indices;
    index_t nelem = connlen / points_per_elem;

    // Look through the edge connectivity and make a map that we can use to turn
    // an edge expressed as global point ids into a unique edge id.
    conduit::index_t_accessor conn1D = dim_topos[1]["elements/connectivity"].value();
    index_t nedges = conn1D.number_of_elements() / 2;
    std::vector<std::pair<uint64, index_t>> edge_key_to_id(nedges);
    build_edge_key_to_id(edge_key_to_id);

    // Get the unique edges template for the element type.
    std::vector<index_t> elem_edges = embedding_3_1_edges(topo_shape);
#ifdef DEBUG_PRINT
    std::cout << "elem_edges=" << elem_edges << std::endl;
#endif

    // Prepare the G(3,1) association.
    index_t edges_per_elem = elem_edges.size() / 2;
    association &map31 = G[3][1];
    map31.data.resize(nelem * edges_per_elem, 0);
    map31.sizes.resize(nelem, 0);
    map31.offsets.resize(nelem, 0);

    // Prepare the G(3,0) association.
    association &map30 = G[3][0];
    if(map30.requested)
    {
        map30.data.resize(nelem * points_per_elem, 0);
        map30.sizes.resize(nelem, 0);
        map30.offsets.resize(nelem, 0);
    }

    // Reserve space so we can build the local_to_global[0] and
    // local_to_global[1] maps too. We do it here since they need much of the
    // same infrastructure as the G(3,1) and G(3,0) maps.
    local_to_global[1].reserve(2 * nelem * edges_per_elem);
    local_to_global[0].reserve(coords_length + 2 * 2 * nelem * edges_per_elem);
    for(index_t i = 0; i < coords_length; i++)
        local_to_global[0].push_back(i);

    // Iterate over the elements, applying the edge template to make unique
    // edges for the element. We look up the edge in edge_key_to_id to get
    // its id.
    conduit::execution::for_all<ParallelExec>(0, nelem, [&](index_t ei)
    {
        index_t elem_offset = ei * points_per_elem;

        // Each bit in the ptadded variable indicates whether the point has
        // been added into the 3,0 map. If map30 is not requested add a value
        // with 1's in all the bits so points can't be added.
        size_t ptadded = map30.requested ? 0 : std::numeric_limits<size_t>::max();
        index_t elem_point = elem_offset;

        for(index_t edge_index = 0; edge_index < edges_per_elem; edge_index++)
        {
            // These point ids are in [0,points_per_elem) and are the local
            // point indices for the edges in the cell.
            index_t edge_pt0 = elem_edges[(edge_index << 1)];
            index_t edge_pt1 = elem_edges[(edge_index << 1) + 1];

            // Make the edge.
            index_t edge[2];
            edge[0] = conn3D[elem_offset + edge_pt0];
            edge[1] = conn3D[elem_offset + edge_pt1];

            // Add the points in the edge to the 3,0 mapping if it was requested
            // and if we have not seen them before in this element. We do this
            // before we make the edge key so the point order is preserved.
            size_t pt0mask = static_cast<size_t>(1) << edge_pt0;
            size_t pt1mask = static_cast<size_t>(1) << edge_pt1;
            if((ptadded & pt0mask) == 0)
            {
                ptadded |= pt0mask;
                map30.data[elem_point++] = edge[0];
            }
            if((ptadded & pt1mask) == 0)
            {
                ptadded |= pt1mask;
                map30.data[elem_point++] = edge[1];
            }

            // Make a key from the edge.
            if(edge[1] > edge[0])
                std::swap(edge[0], edge[1]);
            uint64 key = conduit::utils::hash(edge, 2);

            // Look up the edge id in our list of real edges.
            index_t edgeid = lookup_edge_id(edge_key_to_id, key);

            // Store the edge id in the map.
            index_t elem_edge = ei * edges_per_elem + edge_index;
            map31.data[elem_edge] = edgeid;
        }
        map31.sizes[ei] = edges_per_elem;
        map31.offsets[ei] = ei * edges_per_elem;
    });

    // This loop is not parallel safe because we append to local_to_global.
    for(index_t ei = 0; ei < nelem; ei++)
    {
        index_t elem_offset = ei * points_per_elem;

        // To build the local_to_global maps, we need to iterate all edges of
        // the element - not just the unique ones.
        const ShapeType embed_shape = topo_cascade.get_shape(2);
        for(index_t fi = 0; fi < topo_shape.embed_count; fi++)
        {
            index_t face_start = fi * embed_shape.indices;
            for(index_t pi = 0; pi < embed_shape.indices; pi++)
            {
                index_t pi_next = (pi + 1) % embed_shape.indices;
                index_t embed_pt0 = topo_shape.embedding[face_start + pi];
                index_t embed_pt1 = topo_shape.embedding[face_start + pi_next];

                index_t edge[2];
                edge[0] = conn3D[elem_offset + embed_pt0];
                edge[1] = conn3D[elem_offset + embed_pt1];

                // Build the local_to_global[0] map (before possible swap)
                local_to_global[0].push_back(edge[0]);
                local_to_global[0].push_back(edge[1]);

                // Make a key from the edge.
                if(edge[1] > edge[0])
                    std::swap(edge[0], edge[1]);
                uint64 key = conduit::utils::hash(edge, 2);

                // Look up the edge id in our list of real edges.
                index_t edgeid = lookup_edge_id(edge_key_to_id, key);

                // Build the local_to_global[1] map.
                local_to_global[1].push_back(edgeid);
            }
        }
    }

    // If the G(3,0) map was requested then build its sizes and offsets.
    if(map30.requested)
    {
        for(index_t ei = 0; ei < nelem; ei++)
        {
            map30.sizes[ei] = points_per_elem;
            map30.offsets[ei] = ei * points_per_elem;
        }
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::print_association(int e, int a, bool global) const
{
    if(global)
    {
        const association &assoc = G[e][a];
        std::cout << "\tdata=" << assoc.data << std::endl;
        std::cout << "\tsizes=" << assoc.sizes << std::endl;
        std::cout << "\toffsets=" << assoc.offsets << std::endl;
        std::cout << "\tsingle_size=" << assoc.single_size << std::endl;
        std::cout << "\trequested=" << assoc.requested << std::endl;
    }
    else
    {
        const association &assoc = L[e][a];
        std::cout << "\tdata=" << assoc.data << std::endl;
        std::cout << "\tsizes=" << assoc.sizes << std::endl;
        std::cout << "\toffsets=" << assoc.offsets << std::endl;
        std::cout << "\trequested=" << assoc.requested << std::endl;
    }
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_child_to_parent_association(int e, int a)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    const association &mapPC = G[a][e]; // parent to child (already exists)
    association &mapCP = G[e][a];       // child to parent (what we're making).

#ifdef DEBUG_PRINT
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "build_child_to_parent_association(" << e << ", " << a <<")" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "mapPC:" << std::endl;
    print_association(a,e);
#endif

    mapCP.sizes.resize(dim_topo_lengths[e], 0);
    mapCP.offsets.resize(dim_topo_lengths[e], 0);

    // Make sizes by counting how many times an id occurs.
    for(size_t i = 0; i < mapPC.data.size(); i++)
        mapCP.sizes[mapPC.data[i]]++;

    // Make offsets from sizes
    index_t off = 0;
    for(size_t i = 0; i < mapCP.sizes.size(); i++)
    {
        mapCP.offsets[i] = off;
        off += mapCP.sizes[i];
    }

#ifdef DEBUG_PRINT
    std::cout << "mapPC_data_size=" << mapPC.data.size() << std::endl;
    for(int i =0 ; i < 4; i++)
        std::cout << i << ", topolen=" << dim_topo_lengths[i] << std::endl;
    std::cout << "mapCP.sizes=" << mapCP.sizes << std::endl;
    std::cout << "mapCP.offsets=" << mapCP.offsets << std::endl;
#endif
    // Make a series of ids using the parent-child sizes vector. This will
    // make a pattern like: 0,1,2,3...  or 0,0,0,0,1,1,1,1,2,2,2,2,...
    // according to the size values. We use this to make pairs of
    // parent/child ids.
    auto mapPC_sizes_size = static_cast<index_t>(mapPC.sizes.size());
    auto mapPC_data_size = static_cast<index_t>(mapPC.data.size());
    std::vector<std::pair<index_t, index_t>> p2c(mapPC_data_size);
    index_t idx = 0;
    if(!mapPC.sizes.empty())
    {
        for(index_t id = 0; id < mapPC_sizes_size; id++)
        {
            for(index_t i = 0; i < mapPC.sizes[id]; i++)
            {
                p2c[idx].first = id;               // parent id
                p2c[idx].second = mapPC.data[idx]; // child id
                idx++;
            }
        }
    }
    else
    {
        // Single shape case.
        for(; idx < mapPC_data_size; idx++)
        {
            p2c[idx] = std::make_pair(idx / mapPC.single_size, // parent id
                                      mapPC.data[idx]);        // child id
        }
    }
#ifdef DEBUG_PRINT
    std::cout << "p2c=" << p2c << std::endl;
#endif
    // Sort p2c by child.
    conduit::execution::sort<ParallelExec>(p2c.begin(), p2c.end(),
        [&](const std::pair<index_t, index_t> &lhs,
            const std::pair<index_t, index_t> &rhs)
    {
        return lhs.second < rhs.second;
    });
#ifdef DEBUG_PRINT
    std::cout << "p2c.sorted=" << p2c << std::endl;
#endif

    // Extract the permuted list of parents.
    mapCP.data.resize(p2c.size(), 0);
    for(size_t i = 0; i < p2c.size(); i++)
        mapCP.data[i] = p2c[i].first;
#ifdef DEBUG_PRINT
    std::cout << "mapCP.data=" << mapCP.data << std::endl;
#endif

    // Sort ids in each bin of parent ids.
    for(size_t i = 0; i < mapCP.sizes.size(); i++)
    {
        if(mapCP.sizes[i] > 1)
        {
            index_t *start = &mapCP.data[mapCP.offsets[i]];
            index_t *end = start + mapCP.sizes[i];
            std::sort(start, end);
        }
    }
#ifdef DEBUG_PRINT
    std::cout << "mapCP.data.final=" << mapCP.data << std::endl;
    std::cout << "mapCP" << std::endl;
    print_association(e, a);
#endif
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_local_associations()
{
    auto build_offsets = [](const std::vector<index_t> &sizes,
                            std::vector<index_t> &offsets)
    {
        size_t n = sizes.size();
        offsets.resize(n);
        index_t off = 0;
        for(size_t i = 0; i < n; i++)
        {
            offsets[i] = off;
            off += sizes[i];
        }
    };

    auto buildcases = [&](const std::vector<std::pair<int,int>> &levels,
                          index_t *localIdx)
    {
        for(size_t i = 0; i < levels.size(); i++)
        {
            // e > a in how we call it.
            int e = levels[i].first;
            int a = levels[i].second;
            index_t index = localIdx[e];
            // parent to child
            if(L[e][a].requested)
            {
                L[e][a].sizes[index]++;
            }
            // child to parent
            if(L[a][e].requested)
            {
                L[a][e].data.push_back(index);
                L[a][e].sizes.push_back(1);
            }
        }
    };

    // Resize the L(e,a) associations
    auto resize = [&](int dim, index_t sizes[4], index_t nc)
    {
        for(int e = dim; e >= 0; e--)
        for(int a = dim; a >= 0; a--)
        {
            if(L[e][a].requested)
            {
#ifdef REPRODUCE_REFERENCE
                size_t extra = (e == 0 || a == 0) ? nc : 0;
#else
                constexpr size_t extra = 0;
#endif
                if(e > a)
                {
                    // Parent to child.
                    L[e][a].sizes.resize(sizes[e], 0);
                }
                else if(e == a)
                {
                    // Self
                    L[e][a].sizes.resize(sizes[e] + extra, 1);
                }
                else
                {
                    // Child to parent
                    L[e][a].data.reserve(sizes[e] + extra);
                    L[e][a].sizes.reserve(sizes[e] + extra);
#ifdef REPRODUCE_REFERENCE
                    // L(0,1), L(0,2), L(0,3) do this.
                    if(e == 0 && a >= 1)
                    {
                        for(index_t i = 0; i < nc; i++)
                            L[e][a].sizes.push_back(0);
                    }
#endif 
                }
            }
        }
    };

    // The associations that need to be built at a given dimension.
    const std::vector<std::vector<std::vector<std::pair<int,int>>>> allLevels{
        { // dim 0
            {}
        },
        { // dim 1
            {{1,0}}
        },
        { // dim 2
            {{2,1}},
            {{1,0},{2,0}}
        },
        { // dim 3
            {{3,2}},
            {{2,1},{3,1}},
            {{1,0},{2,0},{3,0}}
        }
    };

    // See if any associations are requested
    int dim = dimension();
    bool any_associations_requested = false;
    for(int e = 0; e <= dim; e++)
    for(int a = 0; a <= dim; a++)
    {
        any_associations_requested |= L[e][a].requested;
    }

    // We only need to do this work for polyhedral and polygonal topologies.
    // Other topologies' local maps are handled implicitly.
    if(topo_shape.is_poly() && any_associations_requested)
    {
        // Determine sizes.
        index_t sizes[] = {0,0,0,0};
        for(index_t ei = 0; ei < dim_topo_lengths[dim]; ei++)
        {
            iterate_global_map_levels(ei, allLevels[dim], 0, sizes,
                [](const std::vector<std::pair<int,int>> &, index_t *){});
        }
        sizes[0] = 2 * sizes[1];

        // Resize the associations.
        resize(dim, sizes, coords_length);
        // Populate the associations.
        index_t localIdx[] = {0, 0, 0, 0};
        for(index_t ei = 0; ei < dim_topo_lengths[dim]; ei++)
            iterate_global_map_levels(ei, allLevels[dim], 0, localIdx, buildcases);

        // Build offsets for the local maps.
        for(int e = dim; e >= 0; e--)
        for(int a = dim; a >= 0; a--)
        {
            if(L[e][a].requested)
            {
                build_offsets(L[e][a].sizes, L[e][a].offsets);
#ifdef DEBUG_PRINT
                std::cout << "L(" <<e << ", " << a << ")" << std::endl;
                print_association(e,a,false);
#endif
            }
        }
    }
}

//---------------------------------------------------------------------------
bool
TopologyMetadata::Implementation::association_requested(index_t entity_dim, index_t assoc_dim) const
{
#ifndef NDEBUG
    if(entity_dim > topo_shape.dim || assoc_dim > topo_shape.dim)
    {
        CONDUIT_ERROR("A global association map G(" << entity_dim << ", " << assoc_dim
            << ") does not exist because one or more indices is invalid.");
    }
#endif
    return G[entity_dim][assoc_dim].requested;
}

//---------------------------------------------------------------------------
vector_view<index_t>
TopologyMetadata::Implementation::get_global_association(index_t entity_id,
    index_t entity_dim, index_t assoc_dim) const
{
#ifndef NDEBUG
    if(entity_dim > topo_shape.dim || assoc_dim > topo_shape.dim)
    {
        CONDUIT_ERROR("A global association map G(" << entity_dim << ", " << assoc_dim
            << ") does not exist because one or more indices is invalid.");
    }
    if(!G[entity_dim][assoc_dim].requested)
    {
        CONDUIT_ERROR("A global association map G(" << entity_dim << ", " << assoc_dim
            << ") does not exist because it was not built during metadata initialization.");
    }
#endif

    const association &assoc = G[entity_dim][assoc_dim];
    auto data = assoc.get_data(entity_id);
    return vector_view<index_t>(data.first, data.second);
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::Implementation::get_local_association_entity_range(
    index_t src_dim, index_t dst_dim) const
{
    // Explicit maps case.
    if(topo_shape.is_poly())
    {
        return std::max(L[src_dim][dst_dim].sizes.size(), L[src_dim][dst_dim].data.size());
    }

    // Implicit maps case.
    index_t ne = 0, dim = dimension();
    index_t mapcase = EA_INDEX(src_dim, dst_dim);
    switch(mapcase)
    {
    case EA_INDEX(0,0):
        // Falls through
    case EA_INDEX(0,1):
        // Falls through
    case EA_INDEX(0,2):
        // Falls through
    case EA_INDEX(0,3):
        if(dim == 3)
            ne = dim_topo_lengths[3] * G[3][2].single_size * G[2][1].single_size * G[1][0].single_size + coords_length;
        else if(dim == 2)
            ne = dim_topo_lengths[2] * G[2][1].single_size * G[1][0].single_size + coords_length;
        else if(dim == 1)
            ne = dim_topo_lengths[1] * G[1][0].single_size + coords_length;
        else if(dim == 0)
            ne = dim_topo_lengths[0] + coords_length;
        break;

    case EA_INDEX(1,0):
        // Falls through
    case EA_INDEX(1,1):
        // Falls through
    case EA_INDEX(1,2):
        // Falls through
    case EA_INDEX(1,3):
        if(dim == 3)
            ne = dim_topo_lengths[3] * G[3][2].single_size * G[2][1].single_size;
        else if(dim == 2)
            ne = dim_topo_lengths[2] * G[2][1].single_size;
        else if(dim == 1)
            ne = dim_topo_lengths[1];
        else if(dim == 0)
            ne = dim_topo_lengths[0];
        break;

    case EA_INDEX(2,0):
        // Falls through
    case EA_INDEX(2,1):
        // Falls through
    case EA_INDEX(2,2):
        // Falls through
    case EA_INDEX(2,3):
        if(dim == 3)
            ne = dim_topo_lengths[3] * G[3][2].single_size;
        else if(dim == 2)
            ne = dim_topo_lengths[2];
        else if(dim == 1)
            ne = dim_topo_lengths[1];
        else if(dim == 0)
            ne = dim_topo_lengths[0];
        break;

    case EA_INDEX(3,0):
        // Falls through
    case EA_INDEX(3,1):
        // Falls through
    case EA_INDEX(3,2):
        // Falls through
    case EA_INDEX(3,3):
        ne = dim_topo_lengths[src_dim];
        break;
    } 
    return ne;
}

//---------------------------------------------------------------------------
conduit::range_vector<index_t>
TopologyMetadata::Implementation::get_local_association(index_t entity_id,
    index_t entity_dim, index_t assoc_dim) const
{
    const association &assoc = L[entity_dim][assoc_dim];
#ifndef NDEBUG
    if(entity_dim > topo_shape.dim || assoc_dim > topo_shape.dim)
    {
        CONDUIT_ERROR("A local association map L(" << entity_dim << ", " << assoc_dim
            << ") does not exist because one or more indices is invalid.");
    }
    if(!assoc.requested)
    {
        CONDUIT_ERROR("A local association map L(" << entity_dim << ", " << assoc_dim
            << ") does not exist because it was not built during metadata initialization.");
    }
#endif

    // Explicit maps case
    if(topo_shape.is_poly())
    {
        if(assoc.data.empty())
        {
#ifdef REPRODUCE_REFERENCE
            // The starting numbering in these maps is offset by the
            // coords_length in the reference implementation. Match it.
            index_t offset = (entity_dim > 0 && assoc_dim == 0) ? coords_length : 0;
#else
            index_t offset = 0;
#endif
            return conduit::range_vector<index_t>(assoc.offsets[entity_id] + offset,
                                                  1, assoc.sizes[entity_id]);
        }

        // There was data. This is a child to parent association.
        return conduit::range_vector<index_t>(assoc.data[assoc.offsets[entity_id]],
                                              1, assoc.sizes[entity_id]);
    }

    // Implicit maps case

    // Sometimes we need size/offset information from the global association.
    conduit::range_vector<index_t> vec(entity_id, 1, 1);

    // NOTE: A pattern that occurs when e, a are more than 1 apart is that we
    //       need to use a product of the sizes. Take e,a = 2,0. These are 2 apart
    //       so we multiply G[2][1].get_size(entity_id) * G[1][0].get_size(entity_id)
    //                        |                             |
    //                        -------------------------------
    //
    //       In the code below, an immediate 2 value is given in place of
    //       G[1][0].single_size (number of vertices in an edge).

    index_t mapcase = EA_INDEX(entity_dim, assoc_dim);
    switch(mapcase)
    {
    // Self cases.
    case EA_INDEX(0,0): break; // no-op (handled by vec default value)
    case EA_INDEX(1,1): break; // no-op
    case EA_INDEX(2,2): break; // no-op
    case EA_INDEX(3,3): break; // no-op

    // Connectivity cases. These guys' start is offset by the coords_length.
    case EA_INDEX(1,0):
        {
            auto start = coords_length;
            auto N = 2; // 2 = G[1][0].single_size;
            vec = conduit::range_vector<index_t>(start + entity_id * N, 1, N);
        }
        break;
    case EA_INDEX(2,0):
        {
            auto start = coords_length;
            auto N = G[2][1].single_size * 2; // 2 = G[1][0].single_size;
            vec = conduit::range_vector<index_t>(start + entity_id * N, 1, N);
        }
        break;
    case EA_INDEX(3,0):
        {
            auto start = coords_length;
            auto N = G[3][2].single_size * G[2][1].single_size * 2; // 2 = G[1][0].single_size;
            vec = conduit::range_vector<index_t>(start + entity_id * N, 1, N);
        }
        break;

    // Child to parent cases.
    case EA_INDEX(0,1):
        {
            if(entity_id < coords_length)
                vec = conduit::range_vector<index_t>(0,0,0); // empty
            else
            {
                auto start = (entity_id - coords_length) / 2; // 2 = G[1][0].single_size;
                vec = conduit::range_vector<index_t>(start, 1, 1);
            }
        }
        break;
    case EA_INDEX(0,2):
        {
            if(entity_id < coords_length)
                vec = conduit::range_vector<index_t>(0,0,0); // empty
            else
            {
               auto start = (entity_id - coords_length) / (G[2][1].single_size * 2); // 2= G[1][0].single_size;
               vec = conduit::range_vector<index_t>(start, 1, 1);
            }
        }
        break;
    case EA_INDEX(0,3):
        {
           if(entity_id < coords_length)
               vec = conduit::range_vector<index_t>(0,0,0); // empty
           else
           {
               auto start = (entity_id - coords_length) / (G[3][2].single_size * G[2][1].single_size * 2); // 2 = G[1][0].single_size;
               vec = conduit::range_vector<index_t>(start, 1, 1);
           }
        }
        break;
    case EA_INDEX(1,2):
        {
            index_t start = entity_id / G[2][1].single_size;
            vec = conduit::range_vector<index_t>(start, 0, 1);
        }
        break;
    case EA_INDEX(1,3):
        {
            index_t start = entity_id / (G[3][2].single_size * G[2][1].single_size);
            vec = conduit::range_vector<index_t>(start, 0, 1);
        }
        break;
    case EA_INDEX(2,3):
        {
            index_t start = entity_id / G[3][2].single_size;
            vec = conduit::range_vector<index_t>(start, 0, 1);
        }
        break;

    case EA_INDEX(3,1):
        { // new scope
            index_t N = G[3][1].single_size * 2; // 2 = G[1][0].single_size;
            vec = conduit::range_vector<index_t>(entity_id * N, 1, N);
        } // end scope
        break;

    // e,e-1 cases
    case EA_INDEX(2,1):
        // Falls through
    case EA_INDEX(3,2):
        { // new scope
            const association &gassoc = G[entity_dim][assoc_dim];
            index_t gsize = gassoc.single_size;
            index_t goffset = entity_id * gassoc.single_size;
            vec = conduit::range_vector<index_t>(goffset, 1, gsize);
        } // end scope
        break;
    }

    return vec;
}

//---------------------------------------------------------------------------
bool
TopologyMetadata::Implementation::get_dim_map(TopologyMetadata::Implementation::IndexType type,
    index_t src_dim, index_t dst_dim, conduit::Node &map_node) const
{
    bool retval = false;
    if(type == GLOBAL)
        retval = get_global_dim_map(src_dim, dst_dim, map_node);
    else if(type == LOCAL)
        retval = get_local_dim_map(src_dim, dst_dim, map_node);
    return retval;
}

//---------------------------------------------------------------------------
bool
TopologyMetadata::Implementation::get_global_dim_map(index_t src_dim, index_t dst_dim,
    conduit::Node &map_node) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    const association &assoc = G[src_dim][dst_dim];
    if(assoc.requested)
    {
        // Copy the vectors out.

        conduit::Node &values = map_node["values"];
        if(int_dtype.id() != DataType::index_t().id())
        {
            conduit::Node wrap;
            wrap.set_external(const_cast<index_t *>(&assoc.data[0]), assoc.data.size());
            wrap.to_data_type(int_dtype.id(), values);
        }
        else
        {
            values.set(assoc.data);
        }

        // Copy sizes out in the desired int_dtype.
        conduit::Node &sizes = map_node["sizes"];
        std::vector<index_t> tmp;
        const std::vector<index_t> *src_sizes = &assoc.sizes;
        if(assoc.sizes.empty())
        {
            src_sizes = &tmp;
            size_t nembed = dim_topo_lengths[src_dim];
            tmp.resize(nembed, assoc.single_size);
        }
        if(int_dtype.id() != DataType::index_t().id())
        {
            conduit::Node wrap;
            wrap.set_external(const_cast<index_t *>(&src_sizes->operator[](0)), src_sizes->size());
            wrap.to_data_type(int_dtype.id(), sizes);
        }
        else
        {
            sizes.set(*src_sizes);
        }

        // Copy offsets out in the desired int_dtype.
        conduit::Node &offsets = map_node["offsets"];
        const std::vector<index_t> *src_offsets = &assoc.offsets;
        if(assoc.offsets.empty())
        {
            src_offsets = &tmp;
            size_t nembed = dim_topo_lengths[src_dim];
            tmp.resize(nembed);
            index_t sz = assoc.single_size;
            for(size_t i = 0; i < nembed; i++)
                tmp[i] = i * sz;
        }
        if(int_dtype.id() != DataType::index_t().id())
        {
            conduit::Node wrap;
            wrap.set_external(const_cast<index_t *>(&src_offsets->operator[](0)), src_offsets->size());
            wrap.to_data_type(int_dtype.id(), offsets);
        }
        else
        {
            offsets.set(*src_offsets);
        }
    }
    return assoc.requested;
}

//------------------------------------------------------------------------------
bool
TopologyMetadata::Implementation::get_local_dim_map(index_t src_dim, index_t dst_dim,
    conduit::Node &map_node) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    bool requested = L[src_dim][dst_dim].requested;
    if(requested)
    {
        // Do a sizing pass.
        index_t N = get_local_association_entity_range(src_dim, dst_dim);
        index_t total_size = 0;
        for(index_t eid = 0; eid < N; eid++)
        {
            auto lm = get_local_association(eid, src_dim, dst_dim);
            total_size += lm.size();
        }

        // Make the nodes.
        conduit::Node &values = map_node["values"];
        conduit::Node &sizes = map_node["sizes"];
        conduit::Node &offsets = map_node["offsets"];

        // Allocate the data using the appropriate int_dtype.
        values.set(DataType(int_dtype.id(), total_size));
        sizes.set(DataType(int_dtype.id(), N));
        offsets.set(DataType(int_dtype.id(), N));

        // Copy the local map data into the values, sizes, offsets arrays.
        // NOTE: It is done this way because using to_data_type()
        //       was not working as expected - maybe used incorrectly.
        if(int_dtype.id() == DataType::index_t().id())
            copy_local_map(src_dim, dst_dim, values.as_index_t_ptr(), sizes.as_index_t_ptr(), offsets.as_index_t_ptr(), N);
        else if(int_dtype.id() == DataType::int32().id())
            copy_local_map(src_dim, dst_dim, values.as_int32_ptr(), sizes.as_int32_ptr(), offsets.as_int32_ptr(), N);
        else if(int_dtype.id() == DataType::int64().id())
            copy_local_map(src_dim, dst_dim, values.as_int64_ptr(), sizes.as_int64_ptr(), offsets.as_int64_ptr(), N);
        else if(int_dtype.id() == DataType::int16().id())
            copy_local_map(src_dim, dst_dim, values.as_int16_ptr(), sizes.as_int16_ptr(), offsets.as_int16_ptr(), N);
        else if(int_dtype.id() == DataType::int8().id())
            copy_local_map(src_dim, dst_dim, values.as_int8_ptr(), sizes.as_int8_ptr(), offsets.as_int8_ptr(), N);
        else
        {
            CONDUIT_ERROR("Unsupported map type " << int_dtype.name());
        }
    }

    return requested;
}

//---------------------------------------------------------------------------//
index_t
TopologyMetadata::Implementation::get_length(index_t dim) const
{
    // NOTE: The default version of 'get_length' gets the total length of all
    // unique entities in the topology. The parameterized version fetches the
    // length for just that parameter's dimension.

    index_t start_dim = (dim >= 0) ? dim : 0;
    index_t end_dim = (dim >= 0) ? dim : topo_shape.dim;

    index_t topo_length = 0;
    for(index_t di = start_dim; di <= end_dim; di++)
    {
        topo_length += dim_topo_lengths[di];
    }

    return topo_length;
}

//---------------------------------------------------------------------------
const DataType &
TopologyMetadata::Implementation::get_int_dtype() const
{
    return int_dtype;
}

//---------------------------------------------------------------------------
const DataType &
TopologyMetadata::Implementation::get_float_dtype() const
{
    return float_dtype;
}

//---------------------------------------------------------------------------
// NOTE: This method is largely borrowed from the previous implementation
//       so review for performance issues.
index_t
TopologyMetadata::Implementation::get_embed_length(index_t entity_dim, index_t embed_dim) const
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // NOTE: The default version of 'get_embed_length' gets the total number of
    // embeddings for each entity at the top level to the embedding level. The
    // parameterized version just fetches the number of embeddings for one
    // specific entity at the top level.

    index_t len = get_length(entity_dim);

    std::vector<index_t> entity_index_bag;
    std::vector<index_t> entity_dim_bag;
    entity_index_bag.reserve(len * 3 / 2);
    entity_dim_bag.reserve(len * 3 / 2);
    for(index_t ei = 0; ei < len; ei++)
    {
        entity_index_bag.push_back(ei);
        entity_dim_bag.push_back(entity_dim);
    }

    std::set<index_t> embed_set;
    index_t embed_length = 0;
    while(!entity_index_bag.empty())
    {
        index_t entity_index = entity_index_bag.back();
        entity_index_bag.pop_back();
        index_t entity_dim_back = entity_dim_bag.back();
        entity_dim_bag.pop_back();

        if(entity_dim_back == embed_dim)
        {
            if(embed_set.find(entity_index) == embed_set.end())
            {
                embed_length++;
                embed_set.insert(entity_index);
            }
        }
        else
        {
            auto lower_dim = entity_dim_back - 1;
            const auto embed_ids = get_local_association(entity_index,
                entity_dim_back, lower_dim);
            for(auto id : embed_ids)
            {
                entity_index_bag.push_back(id);
                entity_dim_bag.push_back(lower_dim);
            }
        }
    }

    return embed_length;
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::make_node(conduit::Node &rep) const
{
    auto maxdim = dimension();

    // Add all topologies to the rep.
    for(index_t d = maxdim; d >= 0; d--)
    {
        std::stringstream oss;
        oss << "topologies/topo" << d;
        std::string tname(oss.str());
        rep[tname].set_external(dim_topos[d]);
    }
    for(index_t d = maxdim; d >= 0; d--)
    {
        std::stringstream oss;
        oss << "lengths/topo" << d;
        std::string tname(oss.str());
        rep[tname].set(dim_topo_lengths[d]);
    }

    // Get all the maps and add them to the rep.
    const std::vector<std::string> mapkeys{"values", "sizes", "offsets"};
    for(int e = maxdim; e >= 0; e--)
    for(int a = maxdim; a >= 0; a--)
    {
        {
            std::stringstream oss;
            oss << "associations/global/map" << e << a << "/data";
            std::string mname(oss.str());
            get_dim_map(TopologyMetadata::GLOBAL, e, a, rep[mname]);

            // Add some lengths so we do not have to count when looking at the output.
            for(const auto &key : mapkeys)
            {
                if(rep[mname].has_child(key))
                {
                    std::stringstream oss2;
                    oss2 << "associations/global/map" << e << a << "/sizes/" << key;
                    std::string mname2(oss2.str());
                    rep[mname2].set(rep[mname][key].dtype().number_of_elements());
                }
            }
        }
        {
            std::stringstream oss;
            oss << "associations/local/map" << e << a << "/data";
            std::string mname(oss.str());
            get_dim_map(TopologyMetadata::LOCAL, e, a, rep[mname]);

            // Add some lengths so we do not have to count when looking at the output.
            for(const auto &key : mapkeys)
            {
                if(rep[mname].has_child(key))
                {
                    std::stringstream oss2;
                    oss2 << "associations/local/map" << e << a << "/sizes/" << key;
                    std::string mname2(oss2.str());
                    rep[mname2].set(rep[mname][key].dtype().number_of_elements());
                }
            }
        }
    }

    for(int d = maxdim; d >= 0; d--)
    {
        const std::vector<index_t> &le2ge = get_local_to_global_map(d);

        std::stringstream oss;
        oss << "local_to_global/map" << d << "/data";
        std::string mname(oss.str());
        conduit::Node &m = rep[mname];
        m.set(le2ge);

        std::stringstream oss2;
        oss2 << "local_to_global/map" << d << "/size";
        std::string mname2(oss2.str());
        conduit::Node &s = rep[mname2];
        s.set(le2ge.size());
    }
}

//---------------------------------------------------------------------------
std::string
TopologyMetadata::Implementation::to_json() const
{
    conduit::Node rep;
    make_node(rep);
    return rep.to_json();
}

//---------------------------------------------------------------------------
const std::vector<index_t> &
TopologyMetadata::Implementation::get_local_to_global_map(index_t dim) const
{
    return local_to_global[dim];
}

//---------------------------------------------------------------------------
void
TopologyMetadata::Implementation::build_local_to_global()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    int dim = dimension();

    // NOTE: Some cases uses G(e,a) maps. If they were not requested, some levels
    //       of the local_to_global maps will not be built.

    if(dim == 0)
    {
        // Copy topo 0's connectivity into the map. Prepend point indices.
        const conduit::Node &conn = dim_topos[0].fetch_existing("elements/connectivity");
        const index_t *src = conn.as_index_t_ptr();

        auto &map0 = local_to_global[0];
        map0.resize(coords_length + dim_topo_lengths[0]);
        std::iota(map0.begin(), map0.begin() + coords_length, 0);
        memcpy(&map0[0] + coords_length, src, sizeof(index_t) * dim_topo_lengths[0]);
    }
    else if(dim == 1)
    {
        // The highest level the same as the global element ordering.
        auto &map1 = local_to_global[1];
        map1.resize(dim_topo_lengths[1]);
        std::iota(map1.begin(), map1.end(), 0);

        // Copy topo 1's connectivity into the map. Prepend point indices.
        const conduit::Node &conn = dim_topos[1].fetch_existing("elements/connectivity");
        index_t len = conn.dtype().number_of_elements();
        const index_t *src = conn.as_index_t_ptr();

        auto &map0 = local_to_global[0];
        map0.resize(coords_length + len);
        std::iota(map0.begin(), map0.begin() + coords_length, 0);
        memcpy(&map0[0] + coords_length, src, sizeof(index_t) * len);
    }
    else if(dim == 2)
    {
        // The highest level the same as the global element ordering.
        auto &map2 = local_to_global[2];
        map2.resize(dim_topo_lengths[2]);
        std::iota(map2.begin(), map2.end(), 0);

        // The next level down matches G(2,1) - it's the edge ids used by the 
        // 2D elements.
        if(G[2][1].requested)
        {
            auto &map1 = local_to_global[1];
            map1 = G[2][1].data;
        }

        // The final level here needs traverse elements in point order (since
        // they are 2D) and walk around the element. The point ids are prepended.
        //
        // We don't use edges because they can be backwards if they were
        // originally made by different element.
        //
        // NOTE: We could use topo2 connectivity instead of G(2,0) for 2D
        //       since they are the same.
        if(G[2][0].requested)
        {
            auto &map0 = local_to_global[0];
            index_t nelem = dim_topo_lengths[2];
            map0.reserve(coords_length + G[2][0].sum_sizes(nelem));
            for(index_t i = 0; i < coords_length; i++)
                map0.push_back(i);
            for(index_t ei = 0; ei < nelem; ei++)
            {
                const auto elem_pts = G[2][0].get_data(ei);
                for(index_t i = 0; i < elem_pts.second; i++)
                {
                    index_t nexti = (i + 1) % elem_pts.second;
                    map0.push_back(elem_pts.first[i]);
                    map0.push_back(elem_pts.first[nexti]);
                }
            }
        }
    }
    else if(dim == 3)
    {
        // The highest level the same as the global element ordering.
        auto &map3 = local_to_global[3];
        map3.resize(dim_topo_lengths[3]);
        std::iota(map3.begin(), map3.end(), 0);

        // The next level down matches G(3,2) - it's the face ids used by the 
        // 3D elements.
        if(G[3][2].requested)
        {
            auto &map2 = local_to_global[2];
            map2 = G[3][2].data;
        }

        // local_to_global[1] and local_to_global[0] are built under
        // build_association_3_1_and_3_0 since they can piggyback off of
        // those algorithms.
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Public interface for TopologyMetadata.

TopologyMetadata::TopologyMetadata(const conduit::Node &topology,
    const conduit::Node &coordset) : TopologyMetadataBase()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    impl = new Implementation(topology, coordset);
}

//---------------------------------------------------------------------------
TopologyMetadata::TopologyMetadata(const conduit::Node &topology,
    const conduit::Node &coordset,
    size_t lowest_dim,
    const std::vector<std::pair<size_t,size_t> > &desired_maps) :
    TopologyMetadataBase()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    impl = new Implementation(topology, coordset, lowest_dim, desired_maps);
}

//---------------------------------------------------------------------------
TopologyMetadata::~TopologyMetadata()
{
    delete impl;
}

//---------------------------------------------------------------------------
int
TopologyMetadata::dimension() const
{
    return impl->dimension();
}

//---------------------------------------------------------------------------
const conduit::Node &
TopologyMetadata::get_topology(size_t dim)
{
    return impl->get_topology(dim);
}

//---------------------------------------------------------------------------
void TopologyMetadata::get_topology(size_t dim, conduit::Node &dest)
{
    impl->get_topology(dim, dest);
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::get_topology_length(size_t dim) const
{
    return impl->get_topology_length(dim);
}

//---------------------------------------------------------------------------
conduit::vector_view<index_t>
TopologyMetadata::get_global_association(index_t entity_id, index_t entity_dim,
    index_t assoc_dim) const
{
    return impl->get_global_association(entity_id, entity_dim, assoc_dim);
}

//---------------------------------------------------------------------------
conduit::range_vector<index_t>
TopologyMetadata::get_local_association(index_t entity_id, index_t entity_dim,
    index_t assoc_dim) const
{
    return impl->get_local_association(entity_id, entity_dim, assoc_dim);
}

//---------------------------------------------------------------------------
bool
TopologyMetadata::association_requested(index_t entity_dim, index_t assoc_dim) const
{
    return impl->association_requested(entity_dim, assoc_dim);
}

//---------------------------------------------------------------------------
bool
TopologyMetadata::get_dim_map(IndexType type, index_t src_dim, index_t dst_dim,
    Node &map_node) const
{
    return impl->get_dim_map(type, src_dim, dst_dim, map_node);
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::get_length(index_t dim) const
{
    return impl->get_length(dim);
}

//---------------------------------------------------------------------------
const DataType &
TopologyMetadata::get_int_dtype() const
{
    return impl->get_int_dtype();
}

//---------------------------------------------------------------------------
const DataType &
TopologyMetadata::get_float_dtype() const
{
    return impl->get_float_dtype();
}

//---------------------------------------------------------------------------
index_t
TopologyMetadata::get_embed_length(index_t entity_dim, index_t embed_dim) const
{
    return impl->get_embed_length(entity_dim, embed_dim);
}

//---------------------------------------------------------------------------
void
TopologyMetadata::make_node(conduit::Node &rep) const
{
    impl->make_node(rep);
}

//---------------------------------------------------------------------------
std::string
TopologyMetadata::to_json() const
{
    return impl->to_json();
}

//---------------------------------------------------------------------------
const std::vector<index_t> &
TopologyMetadata::get_local_to_global_map(index_t dim) const
{
    return impl->get_local_to_global_map(dim);
}

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::reference --
//-----------------------------------------------------------------------------
namespace reference
{

// The reference implementation of TopologyMetadata.
//---------------------------------------------------------------------------//
TopologyMetadata::TopologyMetadata(const conduit::Node &topology,
    const conduit::Node &coordset,
    size_t /*lowest_dim*/,
    const std::vector<std::pair<size_t,size_t> > &/*desired_maps*/) :
    TopologyMetadata(topology, coordset)
{
}

//---------------------------------------------------------------------------//
TopologyMetadata::TopologyMetadata(const conduit::Node &topology,
    const conduit::Node &coordset) :
    TopologyMetadataBase(),
    topo(&topology), cset(&coordset),
    int_dtype(find_widest_dtype(link_nodes(topology, coordset), DEFAULT_INT_DTYPES)),
    float_dtype(find_widest_dtype(link_nodes(topology, coordset), DEFAULT_FLOAT_DTYPE)),
    topo_cascade(topology), topo_shape(topology)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    CONDUIT_ANNOTATE_MARK_BEGIN("Stage 1");
    // NOTE(JRC): This type current only works at forming associations within
    // an unstructured topology's hierarchy.
    Node topo_offsets, topo_suboffsets;
    bool is_polyhedral = topo->has_child("subelements");

    if(is_polyhedral)
    {
        topology::unstructured::generate_offsets(*topo,
                                                 topo_offsets,
                                                 topo_suboffsets);
    }
    else
    {
        topology::unstructured::generate_offsets(*topo, topo_offsets);
    }

    const index_t topo_num_elems = topo_offsets.dtype().number_of_elements();
    const index_t topo_num_coords = coordset::length(coordset);

    // Allocate Data Templates for Outputs //

    // per-dimension maps from an entity's point id set to its global entity id
    std::vector< std::map< std::vector<index_t>, index_t > > dim_geid_maps;
    // per-dimension vector to build up connectivity.
    std::vector< std::vector<int64> > dim_buffers(topo_shape.dim + 1);

    dim_topos.resize(topo_shape.dim + 1);
    dim_geid_maps.resize(topo_shape.dim + 1);
    dim_geassocs_maps.resize(topo_shape.dim + 1);
    dim_leassocs_maps.resize(topo_shape.dim + 1);
    dim_le2ge_maps.resize(topo_shape.dim + 1);
    CONDUIT_ANNOTATE_MARK_END("Stage 1");

    // Start out reserving space for the association spines. These multiples
    // were calibrated using a 2D dataset. Other topological dims may need
    // different values. These were determined using the final sizes of the
    // dim_leassocs_maps, dim_geassocs_maps and comparing to topo_num_elems.
    // 
    CONDUIT_ANNOTATE_MARK_BEGIN("Stage 2");
    size_t le_est_size_multiples[] = {9, 4, 1, 1};
    size_t ge_est_size_multiples[] = {1, 2, 1, 1};
    for(index_t dim = 0; dim <= topo_shape.dim; dim++)
    {
        dim_leassocs_maps[dim].reserve(le_est_size_multiples[dim] *
                                       (topo_shape.dim + 1) *
                                       topo_num_elems);
        dim_geassocs_maps[dim].reserve(ge_est_size_multiples[dim] *
                                       (topo_shape.dim + 1) *
                                       topo_num_elems);
    }

    // Set up the output nodes.
    for(index_t di = 0; di < topo_shape.dim; di++)
    {
        Node &dim_topo = dim_topos[di];
        dim_topo.reset();
        dim_topo["type"].set("unstructured");
        dim_topo["coordset"].set(topology["coordset"].as_string());
        dim_topo["elements/shape"].set(topo_cascade.get_shape(di).type());
    }
    // NOTE: This is done so that the index values for the top-level entities
    // can be extracted by the 'get_entity_data' function before DFS
    // processing below.
    dim_topos[topo_shape.dim].set_external(topology);
    dim_topos[topo_shape.dim]["elements/offsets"].set(topo_offsets);
    if(is_polyhedral)
    {
        dim_topos[topo_shape.dim]["subelements/offsets"].set(topo_suboffsets);
    }
    CONDUIT_ANNOTATE_MARK_END("Stage 2");

    // Prepare Initial Values for Processing //

    // Temporary nodes to manage pointers to important information (temp) and
    // associated conversations (data).
    Node temp, data;

    CONDUIT_ANNOTATE_MARK_BEGIN("Stage 3");
    // NOTE(JRC): A 'deque' is used so that queue behavior (FIFO)
    // is responsible for ordering the identifiers in the cascade of entities,
    // which more closely follows the average intuition.
    const index_t bag_num_elems = topo_num_coords + topo_num_elems;
    std::deque< std::vector<int64> > entity_index_bag(bag_num_elems);
    std::deque< index_t > entity_dim_bag(bag_num_elems, -1);
    std::deque< index_t > entity_origid_bag(bag_num_elems, -1);
    std::deque< std::vector< std::pair<int64, int64> > > entity_parent_bag(bag_num_elems);

    // NOTE(JRC): We start with processing the points of the topology followed
    // by the top-level elements in order to ensure that order is preserved
    // relative to the original topology for these entities.
    for(index_t pi = 0; pi < topo_num_coords; pi++)
    {
        index_t bi = pi;
        entity_index_bag[bi].push_back(bi);
        entity_dim_bag[bi] = 0;
        // Keep track of original id.
        entity_origid_bag[bi] = pi;
    }
    // Prepopulate point connectivity
    dim_buffers[0].reserve(topo_num_coords);
    for(index_t pi = 0; pi < topo_num_coords; pi++)
        dim_buffers[0].push_back(pi);
    CONDUIT_ANNOTATE_MARK_BEGIN("Stage 3");

    CONDUIT_ANNOTATE_MARK_BEGIN("Stage 4");
    // Add entities for the top-level elements (these will be refined later).
    const Node &topo_elem_conn = dim_topos[topo_shape.dim]["elements/connectivity"];
    const Node &topo_elem_offsets = dim_topos[topo_shape.dim]["elements/offsets"];
    const auto elem_conn_access = topo_elem_conn.as_index_t_accessor();
    const auto elem_offsets_access = topo_elem_offsets.as_index_t_accessor();
    for(index_t ei = 0; ei < topo_num_elems; ei++)
    {
        index_t bi = topo_num_coords + ei;
        // Use the offsets to compute a size.
        index_t entity_start_index = elem_offsets_access[ei];
        index_t entity_end_index = (ei < topo_num_elems - 1) ? elem_offsets_access[ei + 1] : 
                                       topo_elem_conn.dtype().number_of_elements();
        index_t entity_size = entity_end_index - entity_start_index;

        // Get the vector we'll populate.
        std::vector<int64> &elem_indices = entity_index_bag[bi];
        elem_indices.resize(entity_size);

        // Store the connectivity into the vector.
        for(index_t i = 0; i < entity_size; i++)
            elem_indices[i] = elem_conn_access[entity_start_index + i];

        // Set the entity dimension
        entity_dim_bag[bi] = topo_shape.dim;

        // Keep track of original id.
        entity_origid_bag[bi] = ei;
    }
    // Prepopulate element connectivity
    auto &dim_buffers_topo = dim_buffers[topo_shape.dim];
    dim_buffers_topo.reserve(topo_num_coords);
    for(index_t ei = 0; ei < topo_num_elems; ei++)
    {
        index_t bi = topo_num_coords + ei;
        std::vector<int64> &elem_indices = entity_index_bag[bi];
        dim_buffers_topo.insert(dim_buffers_topo.end(), elem_indices.begin(), elem_indices.end());
    }
    CONDUIT_ANNOTATE_MARK_END("Stage 4");

    constexpr index_t ENTITY_REQUIRES_ID = -1;

    CONDUIT_ANNOTATE_MARK_BEGIN("Stage 5");
    while(!entity_index_bag.empty())
    {
        // Pop some work off of the deques
        std::vector<int64> entity_indices(std::move(entity_index_bag.front()));
        entity_index_bag.pop_front();
        index_t entity_dim = entity_dim_bag.front();
        entity_dim_bag.pop_front();
        std::vector< std::pair<int64, int64> > entity_parents(std::move(entity_parent_bag.front()));
        entity_parent_bag.pop_front();
        index_t entity_origid = entity_origid_bag.front();
        entity_origid_bag.pop_front();

        // Make some references based on entity_dim.
        std::vector<int64> &dim_buffer = dim_buffers[entity_dim];
        std::map< std::vector<index_t>, index_t > &dim_geid_map = dim_geid_maps[entity_dim];
        //auto &dim_geassocs = dim_geassocs_maps[entity_dim];
        //auto &dim_leassocs = dim_leassocs_maps[entity_dim];
        std::vector<index_t> &dim_le2ge_map = dim_le2ge_maps[entity_dim];
        const ShapeType &dim_shape = topo_cascade.get_shape(entity_dim);

        // Add Element to Topology //

        index_t global_id = entity_origid;
        const index_t local_id = next_local_id(entity_dim);
        if(global_id == ENTITY_REQUIRES_ID)
        {
            // Make a unique map key from the entity_indices.

            // NOTE: This code assumes that all entities can be uniquely
            // identified by the list of coordinate indices of which they
            // are comprised. This is certainly true of all implicit topologies
            // and of 2D polygonal topologies, but it may not be always the
            // case for 3D polygonal topologies.
            std::vector<int64> vert_ids;
            if(!dim_shape.is_polyhedral())
            {
                vert_ids = entity_indices;
            }
            else // if(dim_shape.is_polyhedral())
            {
                const index_t elem_outer_count = entity_indices.size();
                index_t_accessor elem_inner_sizes   = topo->fetch_existing("subelements/sizes").value();
                index_t_accessor elem_inner_offsets = topo_suboffsets.value();
                index_t_accessor elem_inner_conn    = topo->fetch_existing("subelements/connectivity").value();
            
                for(index_t oi = 0; oi < elem_outer_count; oi++)
                {
                    const index_t elem_inner_size   = elem_inner_sizes[entity_indices[oi]];
                    const index_t elem_inner_offset = elem_inner_offsets[entity_indices[oi]];

                    for(index_t ii = 0; ii < elem_inner_size; ii++)
                    {
                        const index_t vi = elem_inner_conn[elem_inner_offset + ii];
                        if(std::find(vert_ids.begin(), vert_ids.end(), vi) == vert_ids.end())
                            vert_ids.push_back(vi);
                    }
                }
            }
            std::sort(vert_ids.begin(), vert_ids.end());

            // Look up the entity in the map, make global_id.
            const auto dim_geid_it = dim_geid_map.find(vert_ids);
            if(dim_geid_it == dim_geid_map.end())
            {
                // Generate new id.
                global_id = next_global_id(entity_dim);

                // Append the entity indices to the connectivity.
                dim_buffer.insert(dim_buffer.end(), entity_indices.begin(), entity_indices.end());

                // Add vert_ids to the map so it is known.
                std::pair<std::vector<index_t>, index_t> obj(std::move(vert_ids), global_id);
                dim_geid_map.insert(std::move(obj));
            }
            else
            {
                // We've seen the entity before, reuse the id.
                global_id = dim_geid_it->second;
            }
        }

        { // create_entity(global_id, local_id, entity_dim)
            expand_assoc_capacity(IndexType::GLOBAL, global_id, entity_dim);
            expand_assoc_capacity(IndexType::LOCAL, local_id, entity_dim);

            if((index_t)dim_le2ge_map.size() <= local_id)
            {
                dim_le2ge_map.resize(local_id + 1);
            }
            dim_le2ge_map[local_id] = global_id;
        }

        // Add Element to Associations //

        add_entity_assoc(IndexType::GLOBAL, global_id, entity_dim, global_id, entity_dim);
        add_entity_assoc(IndexType::LOCAL, local_id, entity_dim, local_id, entity_dim);
        for(index_t pi = 0; pi < (index_t)entity_parents.size(); pi++)
        {
            index_t plevel = entity_parents.size() - pi - 1;
            const index_t parent_global_id = entity_parents[plevel].first;
            const index_t parent_local_id = entity_parents[plevel].second;
            const index_t parent_dim = entity_dim + pi + 1;
            add_entity_assoc(IndexType::GLOBAL, global_id, entity_dim, parent_global_id, parent_dim);
            add_entity_assoc(IndexType::LOCAL, local_id, entity_dim, parent_local_id, parent_dim);
        }

        // Add Embedded Elements for Further Processing //

        if(entity_dim > 0)
        {
            std::vector< std::pair<int64, int64> > embed_parents(std::move(entity_parents));
            embed_parents.push_back(std::make_pair(global_id, local_id));
            ShapeType embed_shape = topo_cascade.get_shape(entity_dim - 1);

            index_t elem_outer_count = dim_shape.is_poly() ?
                entity_indices.size() : dim_shape.embed_count;

            // NOTE(JRC): This is horribly complicated for the poly case and needs
            // to be refactored so that it's legible. There's a lot of overlap in
            // used variables where it feels unnecessary (e.g. 'poly' being
            // shoehorned into using 'implicit' variables), for example.
            
            
            //
            // NOTE(CYRUSH): Refactored to use accessors, however we still have
            // inner loop accessor fetches, which is less than ideal
            //
            for(index_t oi = 0, ooff = 0; oi < elem_outer_count; oi++)
            {
                index_t elem_inner_count = embed_shape.indices;

                if (dim_shape.is_polyhedral())
                {
                    index_t_accessor subelem_sizes   = topo->fetch_existing("subelements/sizes").value();
                    index_t_accessor subelem_offsets = topo_suboffsets.value();
                    elem_inner_count = subelem_sizes[entity_indices[oi]];
                    ooff = subelem_offsets[entity_indices[oi]];
                }

                std::vector<index_t> embed_indices;
                for(index_t ii = 0; ii < elem_inner_count; ii++)
                {
                    index_t ioff = ooff + (dim_shape.is_poly() ?
                        ii : dim_shape.embedding[oi * elem_inner_count + ii]);

                    if (dim_shape.is_polyhedral())
                    {
                        index_t_accessor subele_conn = topo->fetch_existing("subelements/connectivity").value();
                        embed_indices.push_back(subele_conn[ioff]);
                    }
                    else
                    {
                        embed_indices.push_back(entity_indices[ioff % entity_indices.size()]);
                    }
                }

                ooff += dim_shape.is_polygonal() ? 1 : 0;

                entity_index_bag.push_back(embed_indices);
                entity_dim_bag.push_back(embed_shape.dim);
                entity_parent_bag.push_back(embed_parents);

                // For any entity other than points, we'll want to make a new id.
                entity_origid_bag.push_back((embed_shape.dim == 0) ? embed_indices[0] : ENTITY_REQUIRES_ID);
            }
        }
    }
    CONDUIT_ANNOTATE_MARK_END("Stage 5");

    // Move Topological Data into Per-Dim Nodes //

    CONDUIT_ANNOTATE_MARK_BEGIN("Stage 6");
    for(index_t di = 0; di <= topo_shape.dim; di++)
    {
        Node &dim_conn = dim_topos[di]["elements/connectivity"];
        dim_conn.set(DataType(int_dtype.id(), dim_buffers[di].size()));

        data.reset();
        data.set_external(DataType::int64(dim_buffers[di].size()),
            &(dim_buffers[di][0]));
        data.to_data_type(int_dtype.id(), dim_conn);

        // Initialize element sizes for 2D polygonal mesh generating
        // from 3D polyhedral mesh
        if(di == 2 && topo_shape.is_polyhedral())
        {
            Node &poly_sizes = dim_topos[di]["elements/sizes"];
            poly_sizes.set(DataType(int_dtype.id(), dim_geid_maps[di].size()));

            temp.reset();
            data.reset();

            for(const auto &poly_pair : dim_geid_maps[di])
            {
                const std::vector<index_t> &poly_verts = poly_pair.first;
                const index_t &poly_geid = poly_pair.second;

                temp.set_external(DataType(int_dtype.id(), 1),
                    poly_sizes.element_ptr(poly_geid));
                data.set((index_t)poly_verts.size());
                data.to_data_type(int_dtype.id(), temp);
            }
        }

        topology::unstructured::generate_offsets_inline(dim_topos[di]);
    }
    CONDUIT_ANNOTATE_MARK_END("Stage 6");
}

//---------------------------------------------------------------------------//
void
TopologyMetadata::expand_assoc_capacity(IndexType type, index_t idx, index_t dim)
{
    auto &dim_assocs = (type == IndexType::LOCAL) ? dim_leassocs_maps[dim] : dim_geassocs_maps[dim];
    index_t tdim1 = topo_shape.dim + 1;
    index_t idxT = tdim1 * idx;
    if(idxT >= (index_t)dim_assocs.size())
    {
        index_t start = dim_assocs.size();
        index_t end = idxT + tdim1;
        dim_assocs.resize(idxT + tdim1);
        // Give each association vector a little memory now to reduce the
        // number of reallocations later.
        for(index_t i = start; i < end; i++)
            dim_assocs[i].reserve(4);
    }
}

//---------------------------------------------------------------------------//
void
TopologyMetadata::add_entity_assoc(IndexType type, index_t e0_id, index_t e0_dim, index_t e1_id, index_t e1_dim)
{
    auto &e0_cross_assocs = get_entity_assocs(type, e0_id, e0_dim, e1_dim);
    if(std::find(e0_cross_assocs.begin(), e0_cross_assocs.end(), e1_id) == e0_cross_assocs.end())
    {
        e0_cross_assocs.push_back(e1_id);
    }

    auto &e1_cross_assocs = get_entity_assocs(type, e1_id, e1_dim, e0_dim);
    if(std::find(e1_cross_assocs.begin(), e1_cross_assocs.end(), e0_id) == e1_cross_assocs.end())
    {
        e1_cross_assocs.push_back(e0_id);
    }
}

//---------------------------------------------------------------------------//
const std::vector<index_t>&
TopologyMetadata::get_entity_assocs(IndexType type, index_t entity_id, index_t entity_dim, index_t assoc_dim) const
{
    auto &dim_assoc = (type == IndexType::LOCAL) ? dim_leassocs_maps[entity_dim] : dim_geassocs_maps[entity_dim];
    index_t tdim1 = topo_shape.dim + 1;
    index_t sidx = entity_id * tdim1 + assoc_dim;
    return dim_assoc[sidx];
}

//---------------------------------------------------------------------------//
std::vector<index_t>&
TopologyMetadata::get_entity_assocs(IndexType type, index_t entity_id, index_t entity_dim, index_t assoc_dim)
{
    auto &dim_assoc = (type == IndexType::LOCAL) ? dim_leassocs_maps[entity_dim] : dim_geassocs_maps[entity_dim];
    index_t tdim1 = topo_shape.dim + 1;
    index_t sidx = entity_id * tdim1 + assoc_dim;
    return dim_assoc[sidx];
}


//---------------------------------------------------------------------------//
void
TopologyMetadata::get_dim_map(IndexType type, index_t src_dim, index_t dst_dim, Node &map_node) const
{
    auto &dim_assocs = (type == IndexType::LOCAL) ? dim_leassocs_maps : dim_geassocs_maps;

    index_t tdim1 = topo_shape.dim + 1;
    index_t dimlen = static_cast<index_t>(dim_assocs[src_dim].size()) / tdim1;
    std::vector<index_t> values, sizes, offsets;
    for(index_t sdi = 0, so = 0; sdi < dimlen; sdi++, so += sizes.back())
    {
        const std::vector<index_t> &src_assocs = get_entity_assocs(type, sdi, src_dim, dst_dim);
        values.insert(values.end(), src_assocs.begin(), src_assocs.end());
        sizes.push_back((index_t)src_assocs.size());
        offsets.push_back(so);
    }

    std::vector<index_t>* path_data[] = { &values, &sizes, &offsets };
    std::string path_names[] = { "values", "sizes", "offsets" };
    const index_t path_count = sizeof(path_data) / sizeof(path_data[0]);
    for(index_t pi = 0; pi < path_count; pi++)
    {
        Node data;
        data.set(*path_data[pi]);
        data.to_data_type(int_dtype.id(), map_node[path_names[pi]]);
    }
}

//---------------------------------------------------------------------------//
void
TopologyMetadata::get_point_data(IndexType type, index_t point_id, Node &data) const
{
    const index_t point_gid = (type == IndexType::LOCAL) ?
        dim_le2ge_maps[0][point_id] : point_id;

    if(data.dtype().is_empty())
    {
        data.set(DataType::float64(3));
    }
    const DataType data_dtype(data.dtype().id(), 1);

    Node temp1, temp2;
    const std::vector<std::string> csys_axes = coordset::axes(*cset);
    for(index_t di = 0; di < topo_shape.dim; di++)
    {
        temp1.set_external(float_dtype,
            (void*)(*cset)["values"][csys_axes[di]].element_ptr(point_gid));
        temp2.set_external(data_dtype, data.element_ptr(di));
        temp1.to_data_type(data_dtype.id(), temp2);
    }
}


//---------------------------------------------------------------------------//
index_t
TopologyMetadata::get_length(index_t dim) const
{
    // NOTE: The default version of 'get_length' gets the total length of all
    // unique entities in the topology. The parameterized version fetches the
    // length for just that parameter's dimension.

    index_t start_dim = (dim >= 0) ? dim : 0;
    index_t end_dim = (dim >= 0) ? dim : topo_shape.dim;

    index_t topo_length = 0;
    for(index_t di = start_dim; di <= end_dim; di++)
    {
        topo_length += topology::length(dim_topos[di]);
    }

    return topo_length;
}


//---------------------------------------------------------------------------//
index_t
TopologyMetadata::get_embed_length(index_t entity_dim, index_t embed_dim) const
{
    // NOTE: The default version of 'get_embed_length' gets the total number of
    // embeddings for each entity at the top level to the embedding level. The
    // parameterized version just fetches the number of embeddings for one
    // specific entity at the top level.

    std::vector<index_t> entity_index_bag;
    std::vector<index_t> entity_dim_bag;
    for(index_t ei = 0; ei < this->get_length(entity_dim); ei++)
    {
        entity_index_bag.push_back(ei);
        entity_dim_bag.push_back(entity_dim);
    }

    std::set<index_t> embed_set;
    index_t embed_length = 0;
    while(!entity_index_bag.empty())
    {
        index_t entity_index = entity_index_bag.back();
        entity_index_bag.pop_back();
        index_t entity_dim_back = entity_dim_bag.back();
        entity_dim_bag.pop_back();

        if(entity_dim_back == embed_dim)
        {
            if(embed_set.find(entity_index) == embed_set.end())
            {
                embed_length++;
            }
            embed_set.insert(entity_index);
        }
        else
        {
            const std::vector<index_t> &embed_ids = get_entity_assocs(
                TopologyMetadata::LOCAL, entity_index, entity_dim_back, entity_dim_back - 1);
            for(index_t ei = 0; ei < (index_t)embed_ids.size(); ei++)
            {
                entity_index_bag.push_back(embed_ids[ei]);
                entity_dim_bag.push_back(entity_dim_back - 1);
            }
        }
    }

    return embed_length;
}


//---------------------------------------------------------------------------//
std::string
TopologyMetadata::to_json() const
{
    Node mesh;

    Node &mesh_coords = mesh["coordsets"][(*topo)["coordset"].as_string()];
    mesh_coords.set_external(*cset);

    Node &mesh_topos = mesh["topologies"];
    for(index_t di = 0; di <= topo_shape.dim; di++)
    {
        std::ostringstream oss;
        oss << "d" << di;
        mesh_topos[oss.str()].set_external(dim_topos[di]);
    }

    return mesh.to_json();
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::reference --
//-----------------------------------------------------------------------------

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
