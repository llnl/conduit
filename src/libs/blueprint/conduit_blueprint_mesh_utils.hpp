// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_util.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_UTILS_HPP
#define CONDUIT_BLUEPRINT_MESH_UTILS_HPP

//-----------------------------------------------------------------------------
// std includes
//-----------------------------------------------------------------------------
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <memory>

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_exports.h"

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
/// blueprint mesh utility constants
//-----------------------------------------------------------------------------

static const DataType DEFAULT_INT_DTYPE = DataType::int32(1);
static const DataType DEFAULT_UINT_DTYPE = DataType::uint32(1);
static const DataType DEFAULT_FLOAT_DTYPE = DataType::float32(1);
static const std::vector<DataType> DEFAULT_INT_DTYPES = {DEFAULT_INT_DTYPE, DEFAULT_UINT_DTYPE};
static const std::vector<DataType> DEFAULT_NUMBER_DTYPES = {DEFAULT_FLOAT_DTYPE, DEFAULT_INT_DTYPE, DEFAULT_UINT_DTYPE};

static const std::vector<DataType> INT_DTYPES = {DataType::int32(1), DataType::int64(1)};
static const std::vector<DataType> FLOAT_DTYPES = {DataType::float32(1), DataType::float64(1)};

static const std::vector<std::string> ASSOCIATIONS = {"vertex", "element"};
static const std::vector<std::string> BOOLEANS = {"true", "false"};
static const std::vector<std::string> NESTSET_TYPES = {"parent", "child"};

static const std::vector<std::string> COORDINATE_AXES = {"x", "y", "z", "r", "theta", "phi"};
static const std::vector<std::string> CARTESIAN_AXES = {"x", "y", "z"};
static const std::vector<std::string> CYLINDRICAL_AXES = {"z", "r"};
static const std::vector<std::string> SPHERICAL_AXES = {"r", "theta", "phi"};
static const std::vector<std::string> LOGICAL_AXES = {"i", "j", "k"};

static const std::vector<std::string> COORD_TYPES = {"uniform", "rectilinear", "explicit"};
static const std::vector<std::string> COORD_SYSTEMS = {"cartesian", "cylindrical", "spherical"};

static const std::vector<std::string> TOPO_TYPES = {"points", "uniform", "rectilinear", "structured", "unstructured"};

// Note: To add a new topo shape type, you must do the following:
//  1) Add an entry to TOPO_SHAPES, TOPO_SHAPE_IDS, TOPO_SHAPE_DIMS, TOPO_SHAPE_INDEX_COUNTS, 
//     TOPO_SHAPE_EMBED_TYPES, TOPO_SHAPE_EMBED_COUNTS, and TOPO_SHAPE_EMBEDDINGS. These arrays
//     are indexed by the same values, so be very careful to add elements in the same place
//     in each array.
//  2) If you are adding elements in the middle of these arrays, then make sure that for 
//     TOPO_SHAPE_EMBED_TYPES, you update the indices of any shapes that come after the ones
//     you are adding.
//  3) Head over to conduit_blueprint_mesh_utils_iterate_elements.hpp and find the enum class ShapeId.
//     Add an element for your shape there, and update the others if adding in the middle.

static const std::vector<std::string> TOPO_SHAPES = {"point", "line", "tri", "quad", 
    "tet", "hex", "wedge", "pyramid", "polygonal", "polyhedral", "mixed"};

// "p" is for point
// "l" is for line
// "f" is for face
// "c" is for cell
static const std::vector<std::string> TOPO_SHAPE_IDS = {/*point*/ "p", /*line*/ "l", /*tri*/ "f", /*quad*/ "f", 
    /*tet*/ "c", /*hex*/ "c", /*wedge*/ "c", /*pyramid*/ "c", /*polygonal*/ "f", /*polyhedral*/ "c", /*mixed*/ "c"};

// The dimensions for each element in TOPO_SHAPES
static const std::vector<index_t> TOPO_SHAPE_DIMS = {/*point*/ 0, /*line*/ 1, /*tri*/ 2, /*quad*/ 2, 
    /*tet*/ 3, /*hex*/ 3, /*wedge*/ 3, /*pyramid*/ 3, /*polygonal*/ 2, /*polyhedral*/ 3, /*mixed*/ -1};

// How many points are in each element in TOPO_SHAPES
static const std::vector<index_t> TOPO_SHAPE_INDEX_COUNTS = {/*point*/ 1, /*line*/ 2, /*tri*/ 3, /*quad*/ 4, 
    /*tet*/ 4, /*hex*/ 8, /*wedge*/ 6, /*pyramid*/ 5, /*polygonal*/ -1, /*polyhedral*/ -1, /*mixed*/ -1};

// For each element in TOPO_SHAPES, the index into TOPO_SHAPES of the underlying shape. 
// Points have no underlying shape so they get -1.
// Lines have points under the hood so they get 0.
// Triangles are made of lines so they get 1.
// Hexahedrons are made of quads so they get 3.
static const std::vector<index_t> TOPO_SHAPE_EMBED_TYPES = {/*point*/ -1, /*line*/ 0, /*tri*/ 1, /*quad*/ 1, 
    /*tet*/ 2, /*hex*/ 3, /*wedge*/ 2, /*pyramid*/ 2, /*polygonal*/ 1, /*polyhedral*/ 8, /*mixed*/ -1};

// How many of those underlying shapes are there?
// Lines are made of two points so they get 2.
// Triangles are made of three lines so they get 3.
// Hexahedrons are made of 6 quads so they get 6.
// Wedges are made of two end caps (tris) plus three quad sides each split into two tris, so they get 8.
// Pyramids are made of four triangular sides plus a quad base split into two tris, so they get 6.
static const std::vector<index_t> TOPO_SHAPE_EMBED_COUNTS = {/*point*/ 0, /*line*/ 2, /*tri*/ 3, /*quad*/ 4, 
    /*tet*/ 4, /*hex*/ 6, /*wedge*/ 8, /*pyramid*/ 6, /*polygonal*/ -1, /*polyhedral*/ -1, /*mixed*/ -1};

// TODO(JRC): These orientations currently assume the default Conduit-Blueprit
// windings are used for the input geometry, which happens to be the case
// for all example geometry but cannot be assumed for all inputs. In order
// for these arrangements to be used generally, the winding feature needs to
// be implemented and used to perform index space transforms.
static const index_t TOPO_POINT_EMBEDDING[1][1] = {
    {0}};
static const index_t TOPO_LINE_EMBEDDING[2][1] = {
    {0}, {1}};
static const index_t TOPO_TRI_EMBEDDING[3][2] = {
    {0, 1}, {1, 2}, {2, 0}};
static const index_t TOPO_QUAD_EMBEDDING[4][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}};
static const index_t TOPO_TET_EMBEDDING[4][3] = {
    {0, 2, 1}, {0, 1, 3},
    {0, 3, 2}, {1, 2, 3}};
static const index_t TOPO_HEX_EMBEDDING[6][4] = {
    {0, 3, 2, 1}, {0, 1, 5, 4}, {1, 2, 6, 5},
    {2, 3, 7, 6}, {3, 0, 4, 7}, {4, 5, 6, 7}};
static const index_t TOPO_WEDGE_EMBEDDING[8][3] = {
    {0, 1, 2}, {0, 1, 3}, {1, 3, 4}, {1, 2, 4},
    {2, 4, 5}, {2, 0, 5}, {0, 5, 3}, {3, 4, 5}};
static const index_t TOPO_PYRAMID_EMBEDDING[6][3] = {
    {0, 1, 2}, {3, 2, 0}, {0, 1, 4},
    {1, 2, 4}, {2, 3, 4}, {3, 0, 4}};

// Express the faces of the wedge and pyramid shapes without breaking the faces
// into triangles. (npts, points, ...) unused points are -1. For the rest of
// the shapes, the embeddings may be used.
static const index_t TOPO_WEDGE_FACES[5][5] = {
    {3, 0, 2, 1, -1}, {4, 0, 1, 4, 3}, {4, 1, 2, 5, 4}, {4, 2, 0, 3, 5}, {3, 3, 4, 5, -1}};
static const index_t TOPO_PYRAMID_FACES[5][5] = {
    {4, 0, 3, 2, 1}, {3, 0, 1, 4, -1}, {3, 1, 2, 4, -1}, {3, 2, 3, 4, -1}, {3, 3, 0, 4, -1}};

static const std::vector<const index_t*> TOPO_SHAPE_EMBEDDINGS = {
    &TOPO_POINT_EMBEDDING[0][0], &TOPO_LINE_EMBEDDING[0][0],
    &TOPO_TRI_EMBEDDING[0][0], &TOPO_QUAD_EMBEDDING[0][0],
    &TOPO_TET_EMBEDDING[0][0], &TOPO_HEX_EMBEDDING[0][0],
    &TOPO_WEDGE_EMBEDDING[0][0], &TOPO_PYRAMID_EMBEDDING[0][0],
    /*polygonal*/ nullptr, /*polyhedral*/ nullptr, /*mixed*/ nullptr};

//-----------------------------------------------------------------------------
/// blueprint mesh utility structures
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
struct CONDUIT_BLUEPRINT_API ShapeType
{
public:
    static constexpr index_t InvalidTypeId = -1;
    static constexpr index_t InvalidDimension = -1;

    ShapeType();
    ShapeType(const index_t type_id);
    ShapeType(const std::string &type_name);
    ShapeType(const conduit::Node &topology);

    bool is_poly() const;
    bool is_polygonal() const;
    bool is_polyhedral() const;
    bool is_mixed() const;
    bool is_valid() const;

    /// Return the number of actual faces (not triangulated)
    index_t num_faces() const;

    /// Return the ids and number of ids for the requested face.
    const index_t *get_face(index_t face, index_t &nIds) const;

    // Turn a type name into a shape id, or InvalidType.
    static index_t type_name_to_id(const std::string &type_name);

    // Get the shape type.
    const std::string &type() const;

    index_t id, dim, indices;
    index_t embed_id, embed_count, *embedding;

private:
    void init(const index_t type_id);
    void init(const std::string &type_name);

    // Functions to get specific element id types. They are functions so they
    // can contain some static data.
    static index_t wedge_id();
    static index_t pyramid_id();
    static index_t polygonal_id();
    static index_t polyhedral_id();
    static index_t mixed_id();

    static std::string empty_type;
};

//---------------------------------------------------------------------------//
struct CONDUIT_BLUEPRINT_API ShapeCascade
{
public:
    ShapeCascade(const conduit::Node &topology);
    ShapeCascade(const ShapeType &shape_type);

    index_t get_num_embedded(const index_t level) const;
    const ShapeType& get_shape(const index_t level = -1) const;

    ShapeType dim_types[4];
    index_t dim;
private:
    void init(const ShapeType &shape_type);
};

//-----------------------------------------------------------------------------
/// blueprint mesh utility functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Node CONDUIT_BLUEPRINT_API link_nodes(const Node &lhs, const Node &rhs);

//-----------------------------------------------------------------------------
DataType CONDUIT_BLUEPRINT_API find_widest_dtype(const Node &node, const DataType &default_dtype);
//-----------------------------------------------------------------------------
DataType CONDUIT_BLUEPRINT_API find_widest_dtype(const Node &node, const std::vector<DataType> &default_dtypes);

//-----------------------------------------------------------------------------
CONDUIT_BLUEPRINT_API const Node * find_reference_node(const Node &node, const std::string &ref_key);
//-----------------------------------------------------------------------------
index_t CONDUIT_BLUEPRINT_API find_domain_id(const Node &node);

//-----------------------------------------------------------------------------
/**
 @brief Slice a node containing array data and copy data for the supplied ids
        to a new node.

 @param n_src_values The node containing the source values.
 @param ids The ids that will be extracted from the source values.
 @param dest The new node that will contain the sliced data.
 */
void CONDUIT_BLUEPRINT_API slice_array(const conduit::Node &n_src_values,
                                       const std::vector<int> &ids,
                                       Node &n_dest_values);

/// Same as above.
void CONDUIT_BLUEPRINT_API slice_array(const conduit::Node &n_src_values,
                                       const std::vector<conduit::index_t> &ids,
                                       Node &n_dest_values);

//-----------------------------------------------------------------------------
/**
 @brief Slice a values node for a field where values may be an mcarray. The new
        node will contain the data for the supplied indices.

 @param n_src_values The node containing the source values.
 @param ids The ids that will be extracted from the source values.
 @param dest The new node that will contain the sliced data.
 */
void CONDUIT_BLUEPRINT_API slice_field(const conduit::Node &n_src_values,
                                       const std::vector<int> &ids,
                                       conduit::Node &dest);

/// Same as above.
void CONDUIT_BLUEPRINT_API slice_field(const conduit::Node &n_src_values,
                                       const std::vector<conduit::index_t> &ids,
                                       conduit::Node &n_dest_values);

//-----------------------------------------------------------------------------
/**
 @brief Copy fields from one node to another, making full copies.

 @param srcFields The node containing the source fields.
 @param destFields The node that will contain the copied fields.
 @param options A node that contains options. Currently, it supports an "exclusions"
                node. The names under the exclusions node will not be copied. 
 */
void CONDUIT_BLUEPRINT_API copy_fields(const conduit::Node &srcFields,
                                       conduit::Node &destFields,
                                       const conduit::Node &options);

//-----------------------------------------------------------------------------
/**
 @brief Convert a list of nodes to the desired type if they are not already that type.

 @param root The root node that contains the keys.
 @param desired_type The desired data type.
 @param keys A vector of paths in the root node that will be converted to desired type.
 */
void CONDUIT_BLUEPRINT_API convert(conduit::Node &root,
                                   const conduit::DataType &desired_type,
                                   const std::vector<std::string> &keys);

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::connectivity --
//-----------------------------------------------------------------------------
namespace connectivity
{
    //-------------------------------------------------------------------------
    typedef std::vector<index_t> ElemType;
    typedef std::map< index_t, std::vector<index_t> > SubelemMap;

    //-------------------------------------------------------------------------
    void CONDUIT_BLUEPRINT_API make_element_2d(std::vector<index_t>& elem,
                                               index_t element,
                                               index_t iwidth);
    //-------------------------------------------------------------------------
    void CONDUIT_BLUEPRINT_API make_element_3d(ElemType& connect,
                                               index_t element,
                                               index_t iwidth,
                                               index_t jwidth,
                                               index_t kwidth,
                                               SubelemMap& faces);

    //-------------------------------------------------------------------------
    void CONDUIT_BLUEPRINT_API create_elements_2d(const Node& ref_win,
                                                  index_t i_lo,
                                                  index_t j_lo,
                                                  index_t iwidth,
                                                  std::map<index_t, std::vector<index_t> >& elems);
    //-------------------------------------------------------------------------
    void CONDUIT_BLUEPRINT_API create_elements_3d(const Node& ref_win,
                                                  index_t i_lo,
                                                  index_t j_lo,
                                                  index_t k_lo,
                                                  index_t iwidth,
                                                  index_t jwidth,
                                                  index_t kwidth,
                                                  std::map<index_t, ElemType>& elems,
                                                  SubelemMap& faces);

    //-------------------------------------------------------------------------
    void CONDUIT_BLUEPRINT_API connect_elements_2d(const Node& ref_win,
                                                   index_t i_lo,
                                                   index_t j_lo,
                                                   index_t iwidth,
                                                   index_t ratio,
                                                   index_t& new_vertex,
                                                   std::map<index_t, std::vector<index_t> >& elems,
                                                   bool flip);

    //-------------------------------------------------------------------------
    void CONDUIT_BLUEPRINT_API connect_elements_3d(const Node& ref_win,
                                                   index_t i_lo,
                                                   index_t j_lo,
                                                   index_t k_lo,
                                                   index_t iwidth,
                                                   index_t jwidth,
                                                   index_t& new_vertex,
                                                   std::map<index_t, ElemType>& elems);
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::connectivity --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::coordset --
//-----------------------------------------------------------------------------
namespace coordset
{
    //-------------------------------------------------------------------------
    index_t CONDUIT_BLUEPRINT_API dims(const conduit::Node &coordset);

    //-------------------------------------------------------------------------
    index_t CONDUIT_BLUEPRINT_API length(const conduit::Node &coordset);

    //-----------------------------------------------------------------------------
    std::vector<std::string> CONDUIT_BLUEPRINT_API axes(const Node &coordset);

    //-----------------------------------------------------------------------------
    std::string CONDUIT_BLUEPRINT_API coordsys(const Node &n);

    //-----------------------------------------------------------------------------
    /**
    @brief Updates array d to hold the number of verticies in each dimension
        for the given coordset. Explicit coordsets will just report their
        number_of_elements() in d[0].
    */
    void logical_dims(const conduit::Node &n, index_t *d, index_t maxdims);

    //-----------------------------------------------------------------------------
    /**
    @brief Reads the coordset's data and determines min/max for each axis.
    NOTE: This simply takes the min/max of each data array for recilinear/explicit,
    are there any special considerations for cylindrical and spherical coordinates?
    For uniform it calculates min/max based off of origin/spacing/dims.
    @return A vector of float64 in the format {d0min, d0max, ... , dNmin, dNmax}
    */
    std::vector<float64> CONDUIT_BLUEPRINT_API extents(const Node &n);

    //-----------------------------------------------------------------------------
    /**
     @brief Check whether all component types match and are compact, making it
            suitable for pointer access.

     @param cset The coordset we're checking.

     @return A tuple containing 1) whether the components are suitable for pointer
             access and 2) the data type.
     */
    std::tuple<bool, conduit::DataType> CONDUIT_BLUEPRINT_API supports_pointer_access(const conduit::Node &coordset);

    namespace uniform
    {
        /**
        @brief Reads the given uniform coordset and extracts to spacing
               to an index_t vector
        */
        std::vector<double> CONDUIT_BLUEPRINT_API spacing(const Node &n);

        std::vector<index_t> CONDUIT_BLUEPRINT_API origin(const Node &n);
    }

    std::string CONDUIT_BLUEPRINT_API coordsys(const Node &coordset);

    //-------------------------------------------------------------------------
    // -- begin conduit::blueprint::mesh::utils::coordset::_explicit --
    //-------------------------------------------------------------------------
    namespace _explicit
    {
        //-------------------------------------------------------------------------
        std::vector<float64> CONDUIT_BLUEPRINT_API coords(const Node &coordset, const index_t i);
    }
    //-------------------------------------------------------------------------
    // -- end conduit::blueprint::mesh::utils::coordset::_explicit --
    //-------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::coordset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::topology --
//-----------------------------------------------------------------------------
namespace topology
{
    //-------------------------------------------------------------------------
    index_t CONDUIT_BLUEPRINT_API dims(const conduit::Node &topo);

    //-------------------------------------------------------------------------
    void CONDUIT_BLUEPRINT_API logical_dims(const Node &n, index_t *d, index_t maxdims);

    //-------------------------------------------------------------------------
    index_t CONDUIT_BLUEPRINT_API length(const conduit::Node &topo);

    // return the coordset for this topology
    const Node & coordset(const conduit::Node &topo);

    //-------------------------------------------------------------------------
    /**
     * @brief Reindexes the vertices in a topology to be associated with a new
     * coordset, based on a global vertex ID numbering.
     * The old coordset must be a subset of the new coordset.
     */
    void CONDUIT_BLUEPRINT_API reindex_coords(const conduit::Node& topo,
                                              const conduit::Node& new_coordset,
                                              const conduit::Node& old_gvids,
                                              const conduit::Node& new_gvids,
                                              conduit::Node& out_topo);
    //-------------------------------------------------------------------------
    /**
     * @brief Applies a spatial sorting algorithm (based on a kdtree) to the
     *        topology's centroids and returns a vector containing the sorted
     *        order.
     *
     * @param topo The topology whose elements are being sorted.
     *
     * @return A vector containing the new element order.
     */
    std::vector<conduit::index_t> CONDUIT_BLUEPRINT_API spatial_ordering(const conduit::Node &topo);

    //-------------------------------------------------------------------------
    /**
     * @brief Applies a spatial sorting algorithm (based on a Hilbert curve) to
     *        the topology's centroids and returns a vector containing the sorted
     *        order.
     *
     * @param topo The topology whose elements are being sorted.
     *
     * @return A vector containing the new element order.
     */
    std::vector<conduit::index_t> CONDUIT_BLUEPRINT_API hilbert_ordering(const conduit::Node &topo);

    //-------------------------------------------------------------------------
    /**
     @brief Given a single topology, make a series of Add calls that add an
            entity using the source mesh's coordinates but with a potentially
            different shape from the original topology. We can for example have
            a volume mesh and make a face mesh from it. This class takes care
            of creating the new sub-topology and renumbering coordinates, etc.
     */
    class CONDUIT_BLUEPRINT_API TopologyBuilder
    {
    public:
        /**
         @brief Constructor
         @param _topo The input mesh node.
         */
        TopologyBuilder(const conduit::Node &_topo);
        TopologyBuilder(const conduit::Node *_topo);

        /**
         @brief Add a new entity with the given point list.
    
         @param pts A list of point ids in the selected coordset.
         @param npts The number of points in the list.
         */
        size_t add(const index_t *pts, index_t npts);

        /**
         @brief Add a new entity with the given point list.
    
         @param pts The list of points in the entity.     
         */
        size_t add(const std::vector<index_t> &pts);

        /**
         @brief Make the new topologies, using the specified shape type.

         @param newMesh A node to contain the new topo and coordset. This node
                        will get topologies and coordset nodes too.
         @param shape The name of the shape type to use in the new topologies.
         */
        void execute(conduit::Node &newMesh, const std::string &shape);

    protected:
        index_t newPointId(index_t oldPointId);
        void clear();
    protected:
        const conduit::Node       &topo;
        std::map<index_t, index_t> old_to_new;
        std::vector<index_t>       topo_conn;
        std::vector<index_t>       topo_sizes;
    };

    /**
     * @brief Extract a point mesh from the topology using a list of ids. If the
     *        topology is not points then points in an element are averaged to
     *        make a point.
     *
     * @param n_topo The topology being sampled.
     * @param n_ids  A node containing a list of element ids to pull out as a
     *               point mesh.
     * @param[out] n_output The output node containing the new mesh.
     */
    void CONDUIT_BLUEPRINT_API extract_pointmesh(const conduit::Node &n_topo,
                                                 const conduit::Node &n_ids,
                                                 conduit::Node &n_output);

    //-------------------------------------------------------------------------
    /**
     @brief Determines whether topo2 elements exist in topo1. First the topo2
            coordinates are matched against topo1 using PointQuery. Then topo2
            entities are defined in terms of topo1 coordinates, if possible.
            Then, if that works for an entity, the entity is looked for in topo1.

     @param topo1 A single topology. (haystack)
     @param topo2 A single topology. (needle)

     @return A vector of ints, sized length(topo2), that contains 1 if the
             entity exists in topo1 and 0 otherwise.
     */
    std::vector<int> CONDUIT_BLUEPRINT_API search(const conduit::Node &topo1,
                                                  const conduit::Node &topo2);

    //-------------------------------------------------------------------------
    /**
     * @brief Compute information about a mesh; its coordinate extents and edge
     *        lengths. It is implemented as a struct so other quantities can be
     *        added as the need arises.
     */
    struct CONDUIT_BLUEPRINT_API MeshInfo
    {
        float64 minExtents[3];     // Min coordinate extents
        float64 maxExtents[3];     // Max coordinate extents
        float64 minEdgeLength;     // Min zone edge length
        float64 maxEdgeLength;     // Max zone edge length
        float64 minDiagonalLength; // Min zone diagonal length
        float64 maxDiagonalLength; // Max zone diagonal length
    };

    /**
     * @brief Compute the mesh information.
     *
     * @param n_topo A topology to use for computing the mesh information.
     * @param[out] info A struct that contains the mesh information.
     */
    void CONDUIT_BLUEPRINT_API compute_mesh_info(const conduit::Node &n_topo, MeshInfo &info);

    /**
     * @brief Output stream operator for MeshInfo
     *
     * @param os The output stream to use.
     * @param obj The MeshInfo object to write.
     *
     * @return The output stream reference.
     */
    std::ostream &operator << (std::ostream &os, const MeshInfo &obj);

    /**
     * @brief This structure manages a collection of MeshInfo objects.
     */
    class CONDUIT_BLUEPRINT_API MeshInfoCollection
    {
    public:
        /// Constructor.
        MeshInfoCollection();

        /// Destructor
        virtual ~MeshInfoCollection() = default;

        /**
         * @brief Clear the domain information.
         */
        void clear();

        /**
         * @brief This method is called before adding domains.
         */
        void begin();

        /**
         * @brief Add a domain's information to the collection.
         *
         * @param domainId The domain id for the mesh info.
         * @param info     The mesh info to add.
         */
        void add(index_t domainId, const MeshInfo &info);

        /**
         * @brief This method is called once all local domains have been added.
         */
        virtual void end();

        /**
         * @brief Get the info for a domain.
         * @param domainId The id for the domain info to retrieve. It must have
         *                 been added.
         * @return A reference to the domain's mesh info.
         */
        const MeshInfo &getMeshInfo(index_t domainId) const;

        /**
         * @brief Get the merged info for all domains.
         * @return A reference to the merged mesh info.
         */
        const MeshInfo &getMergedMeshInfo() const;

        /**
         * @brief Merge 2 MeshInfo objects into one.
         * @param a The first object to merge.
         * @param b The second object to merge.
         * @return The merged mesh info for a, b.
         */
        static MeshInfo merge(const MeshInfo &a, const MeshInfo &b);

        /**
         * @brief Print the object.
         * @param os The stream to use for printing.
         */
        void print(std::ostream &os) const;
    protected:
        std::map<index_t, MeshInfo> meshInfos;
        MeshInfo mergedMeshInfo;
    };

    /**
     * @brief Output stream operator for MeshInfoCollection
     *
     * @param os The output stream to use.
     * @param obj The MeshInfoCollection object to write.
     *
     * @return The output stream reference.
     */
    std::ostream &operator << (std::ostream &os, const MeshInfoCollection &obj);

    //-------------------------------------------------------------------------
    /**
     * @brief Quantize a mesh node into an index by binning the points in a regular
     *        grid determined using MeshInfo.
     *
     */
    struct CONDUIT_BLUEPRINT_API Quantizer
    {
        using QuantizedValue = uint64_t;
        using QuantizedIndex = QuantizedValue;
        using FloatValue = float64;
        using Coordinate = std::vector<FloatValue>;

        /// Constructor.
        Quantizer(const MeshInfo &info);

        /**
         * @brief Quantize a single value using the binned spatial mesh, implied
         *        by the MeshInfo.
         *
         * @param value The value to be quantized.
         * @param dim The mesh dimension index (0,1,2) to be used for quantization.
         *
         * @return A quantized index along the dimension.
         */
        QuantizedValue quantize(FloatValue value, index_t dim) const;

        /**
         * @brief Convert the input node into a quantized index in the binned
         *        spatial mesh, implied by the MeshInfo.
         *
         * @param coord A vector containing a coordinate with 1-3 components.
         *
         * @return A quantized node index that can identify the node.
         */
        QuantizedIndex quantize(const Coordinate &coord) const;

        MeshInfo       meshInfo;
        FloatValue     length;
        FloatValue     offset;
        QuantizedValue QX;
        QuantizedValue QY;
        QuantizedValue QXQY;
    };

    /**
     * @brief Output stream operator for Quantizer
     *
     * @param os The output stream to use.
     * @param obj The Quantizer object to write.
     *
     * @return The output stream reference.
     */
    std::ostream &operator << (std::ostream &os, const Quantizer &obj);

    //-------------------------------------------------------------------------
    // -- begin conduit::blueprint::mesh::utils::topology::unstructured --
    //-------------------------------------------------------------------------
    namespace unstructured
    {
        //-------------------------------------------------------------------------
        // Generates element offsets for given topo
        void CONDUIT_BLUEPRINT_API generate_offsets(const Node &topo,
                                                    Node &dest);

        //-------------------------------------------------------------------------
        // Generates element and subelement offsets for given topo
        void CONDUIT_BLUEPRINT_API generate_offsets(const Node &topo,
                                                    Node &dest_ele_offsets,
                                                    Node &dest_subele_offsets);

        //-------------------------------------------------------------------------
        // Adds offsets to given topo
        void CONDUIT_BLUEPRINT_API generate_offsets_inline(Node &topo);

        //-------------------------------------------------------------------------
        /**
         @brief This function returns the points for a specified element index from
                the given topology. For non-polyhedral element types, the \a unique
                parameter indicates whether the function will return a unique+sorted
                vector of point ids. The order can therefore differ from the point
                order in the connectivity. When \a unique is false, the original
                cell connectivity is preserved. For polyhedral types, the points
                are always unique and sorted according to point id.

         @param topo The input topology.
         @param ei The element index.
         @param unique Whether points should be unique+sorted. The default is true
                       to continue earlier behavior.

         @return A vector of point ids for the given element.
         */
        std::vector<index_t> CONDUIT_BLUEPRINT_API points(const Node &topo,
                                                          const index_t ei,
                                                          bool unique = true);

        //-------------------------------------------------------------------------
        /**
         * @brief Rewrite the topology's connectivity in terms of the supplied
         *        coordset, using point queries to look up the new node ids.
         *
         * @param topo The input topology to be modified.
         * @param cset The new coordinate set to use for the topology.
         */
        void CONDUIT_BLUEPRINT_API rewrite_connectivity(conduit::Node &topo,
                                                        const conduit::Node &cset);

        //-------------------------------------------------------------------------
        /**
         * @brief Reorder the topology's elements and nodes, according to the new
         *        element \order vector.
         *
         * @param topo     A node containing the topo to be reordered.
         * @param coordset A node containing the coordset to be reordered.
         * @param fields   A node containing the fields to be reordered. Only fields
         *                 that match the topology node name will be modified.
         * @param order    A vector containing element indices in their new order.
         * @param dest_topo A node that will contain reordered topo. It can
         *                  be the same node as topo.
         * @param dest_coordset A node that will contain reordered coordset. It can
         *                  be the same node as coordset.
         * @param dest_fields A node that will contain the reordered fields. It can
         *                  be the same node as fields.
         * @param[out] old2NewPoints A vector that contains new point indices for
         *                           old point indices, which can be used for mapping
         *                           data from the original order to the new order.
         *
         * @note This reorder function is similar to calling partition with an explicit
         *       index selection if the indices are provided in a new order. This
         *       function will also reorder the nodes in their order of use by elements
         *       in the new order and partition does not currently do that.
         */
        void CONDUIT_BLUEPRINT_API reorder(const conduit::Node &topo,
                                           const conduit::Node &coordset,
                                           const conduit::Node &fields,
                                           const std::vector<conduit::index_t> &order,
                                           conduit::Node &dest_topo,
                                           conduit::Node &dest_coordset,
                                           conduit::Node &dest_fields,
                                           std::vector<conduit::index_t> &old2NewPoints);
    }
    //-------------------------------------------------------------------------
    // -- end conduit::blueprint::mesh::utils::topology::unstructured --
    //-------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::topology --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::query --
//-----------------------------------------------------------------------------
namespace query
{

//---------------------------------------------------------------------------
/**
 @brief Base class for point queries. The class can build up a set of point
        queries but it does not actually execute them. It will return results
        that indicate that all the queries succeeded (no real work is done).
 */
class CONDUIT_BLUEPRINT_API PointQueryBase
{
public:
    static const int NotFound;
    static const double DEFAULT_POINT_TOLERANCE;

    /**
     @brief Constructor
     @param mesh The input mesh(es). Each mesh domain must have state/domain_id
                 that uniquely identifies the domain.
     */
    PointQueryBase(const conduit::Node &mesh);

    /**
     @brief Destructor
     */
    virtual ~PointQueryBase() = default;

    /**
     @brief Reset the query to try again.
     */
    virtual void reset();

    /**
     @brief Add a point to the list of points that will be queried on domain \a dom.
     @param dom The domain that will be queried.
     @param pt The point that will be queried.
     */
    conduit::index_t add(int dom, const double pt[3]);

    /**
     @brief Execute all of the point queries.
     @param coordsetName The name of the coordset we're searching in the domains.
     */
    virtual void execute(const std::string &coordsetName);

    /**
     @brief Get the input points vector for a domain.
     @param dom The domain id.

     @return A vector of coordinates for the domain.
     */
    const std::vector<double> &inputs(int dom) const;

    /**
     @brief Get the results vector for a domain.
     @param dom The domain id.

     @return A vector of results for the domain. Each element represents a
             point id. If the point is not found, it contains -1.
     */
    const std::vector<int> &results(int dom) const;

    /**
     @brief Return a vector of the unique domain ids (the keys of the 
            m_domInputs map) that were requested via calls to Add.

     @return A vector of unique domain ids.
     */
    std::vector<int> queryDomainIds() const;
protected:
    const conduit::Node &m_mesh;
    std::map<int, std::vector<double> > m_domInputs;
    std::map<int, std::vector<int> >    m_domResults;
};

//---------------------------------------------------------------------------
/**
 @brief This class can build up a set of point queries and execute them in
        serial against the domains in the input mesh. This class actually
        executes the queries.
 */
class CONDUIT_BLUEPRINT_API PointQuery : public PointQueryBase
{
public:
    /// Point threshold after which it makes sense to switch search methods.
    static const int SEARCH_THRESHOLD;

    /**
     @brief Constructor
     @param mesh The input mesh(es). Each mesh domain must have state/domain_id
                 that uniquely identifies the domain.
     */
    PointQuery(const conduit::Node &mesh);

    /**
     @brief Destructor
     */
    virtual ~PointQuery() override = default;

    /**
     @brief Set the point tolerance used to decide which points are the same.
     @param tolerance The tolderance value.
     */
    void setPointTolerance(double tolerance);

    /**
     @brief Execute all of the point queries.
     @param coordsetName The name of the coordset we're searching in the domains.
     */
    virtual void execute(const std::string &coordsetName) override;

protected:
    /**
     @brief Get the domain that has state/domain_id == dom.
     @param dom The domain id.
     @return A Node pointer or nullptr if it is not found.
     */
    const conduit::Node *getDomain(int dom) const;

    /**
     @brief Find the input points in the input mesh's coordset and make a result
            vector that contains the point ids of the looked-up points.
     @param mesh The node that contains the mesh.
     @param coordsetName The name of the coordset that will be searched.
     @param input The input vector of x,y,z triples that we're looking for.
     @param result The output vector containing the node id of each point or
                   -1 if not found.
     */
    void findPointsInDomain(const conduit::Node &mesh,
                            const std::string &coordsetName,
                            const std::vector<double> &input,
                            std::vector<int> &result) const;

    /**
     @brief Find the input points in the input mesh's coordset using kdtree strategy.
     @param ndims The number of dimensions in the coordset.
     @param sameTypes Whether the coordinates are all the same type.
     @param coords The nodes for the individual coordset axis values.
     @param coordTypes The coordinate types.
     @param input The input vector of x,y,z triples that we're looking for.
     @param result The output vector containing the node id of each point or
                   -1 if not found.
     @return True if the method completed the search; False otherwise.
     */
    bool acceleratedSearch(int ndims,
                           bool sameTypes,
                           const conduit::Node *coords[3],
                           const conduit::index_t coordTypes[3],
                           const std::vector<double> &input,
                           std::vector<int> &result) const;

    /**
     @brief Find the input points in the input mesh's coordset using normal strategy.
     @param ndims The number of dimensions in the coordset.
     @param sameTypes Whether the coordinates are all the same type.
     @param coords The nodes for the individual coordset axis values.
     @param coordTypes The coordinate types.
     @param input The input vector of x,y,z triples that we're looking for.
     @param result The output vector containing the node id of each point or
                   -1 if not found.
     @return True if the method completed the search; False otherwise.
     */
    bool normalSearch(int ndims,
                      bool sameTypes,
                      const conduit::Node *coords[3],
                      const conduit::index_t coordTypes[3],
                      const std::vector<double> &input,
                      std::vector<int> &result) const;
    /**
     @brief Return a list of domain ids that exist locally.
     @return A list of local domain ids.
     */
    std::vector<int> domainIds() const;

protected:
    double m_pointTolerance;
};

//---------------------------------------------------------------------------
/**
 @brief A match membership query that uses the questions asked by various
        domains to build new topologies that are exchanged. The topologies
        are then compared with their counterparts on remote ranks to determine
        overlap. The topologies are sent back to the original rank with a
        results field. The query results for an entity can be determined
        by passing the id returned from Add() to the Exists() method along with
        the domain and query domain.
 */
class CONDUIT_BLUEPRINT_API MatchQuery
{
public:
    /**
     @brief Constructor
     @param mesh The input mesh(es). Each mesh domain must have state/domain_id
                 that uniquely identifies the domain.
     */
    MatchQuery(const conduit::Node &mesh);

    /**
     @brief Destructor (marked as virtual)
     */
    virtual ~MatchQuery() = default;

    /**
     @brief Select the topology that will be used. This should be an existing
            topology. The name will be used internally to construct query
            topologies and to identify the coordset to be used for pulling
            points. This must be called before calling <Execute>"()".
     @param name The topology name.
     */
    void selectTopology(const std::string &name);

    /**
     @brief Add an entity that will be queried on domain \a dom.
     @param dom The domain that is asking the questions.
     @param query_dom The domain that is being queried.
     @param ids A list of point ids that make up the entity.
     @param nids The number of point ids that make up the entity.

     @return An identifier that represents the entity.
     */
    size_t add(int dom, int query_dom, const index_t *ids, index_t nids);
    size_t add(int dom, int query_dom, const std::vector<index_t> &ids);

    /**
     @brief Execute all of the queries.
     @param shape The shape type for the queried entities.
     */
    virtual void execute();

    /**
     @brief Return whether the entity exists on query_dom.
     @param dom The domain that is asking the questions.
     @param query_dom The domain that is being queried.
     @param entityId A global identifier that represents the entity.
                     This was returned from Add()

     @return True if the entityId exists in query_dom.
     */
    virtual bool exists(int dom, int query_dom, size_t entityId) const;

    /**
     @brief Return a vector of pairs that contain dom,query_dom values.
     @return A vector of pairs.
     */
    std::vector<std::pair<int,int>> queryDomainIds() const;

    /**
     @brief Return the results vector for a given dom,query_dom pair.
     @return A results vector containing 1 (found), 0 (not found).
     */
    const std::vector<int> &results(int dom, int query_dom) const;

protected:
    /**
     @brief Attempt to return the selected topology for the requested domain.
            The topology is selected by topoName.
     @param domain The domain number.
     @return A node that points to the domain's selected topology. If no
             such node is located, nullptr is returned.
     */
    const conduit::Node *getDomainTopology(int domain) const;

    /**
     @brief Contains information for one domain:query_domain query.
     */
    struct QueryInfo
    {
        std::shared_ptr<topology::TopologyBuilder> builder;
        std::vector<int>                           results;
        conduit::Node                              query_mesh;
    };

    const conduit::Node &m_mesh;
    std::string m_topoName;
    std::map<std::pair<int,int>, QueryInfo> m_query;
};

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::query --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::adjset --
//-----------------------------------------------------------------------------
namespace adjset
{
    //-------------------------------------------------------------------------
    /**
     @brief Makes sure the adjset is in canonical form. This means renaming all
            groups so they begin with "group_" followed by a sorted list of the
            neighbors.
     @param adjset The adjset to be modified.
     */
    void CONDUIT_BLUEPRINT_API canonicalize(Node &adjset);

    //-------------------------------------------------------------------------
    /**
      @brief Return whether the adjset appears to be canonical.
      @param adjset The adjset node.
      @return True if the groups are named canonically; False otherwise.
     */
    bool CONDUIT_BLUEPRINT_API is_canonical(const Node &adjset);

    //-------------------------------------------------------------------------
    /**
     @brief Adds a canonical pairwise adjset to each input domain, converting
            as needed. If the input adjset is already pairwise then the data
            in the adjset groups is shallow-copied from the original adjset.

     @param doms A node containing the domains.
     @param adjsetName The name of the source adjset.
     @param newAdjsetName The name of the adjset that will be created.
     */
    void CONDUIT_BLUEPRINT_API to_pairwise_canonical(conduit::Node &doms,
                                                     const std::string &adjsetName,
                                                     const std::string &newAdjsetName);

    //-------------------------------------------------------------------------
    /**
      @brief Removes the adjset from each domain in the mesh.
      @param doms A node containing the domains.
      @param adjsetName The name of the adjset to remove.
     */
    void CONDUIT_BLUEPRINT_API remove(conduit::Node &doms, const std::string &adjsetName);

    //-------------------------------------------------------------------------
    /**
     @brief Given a set of domains, make sure that the specified adjset in them
            is valid and flag any errors in the info node. This function will
            make sure that each domain's adjset references valid entities in
            neighboring domains.

     @param doms A node containing the domains. There must be multiple domains.
     @param adjsetName The name of the adjset in all domains. It must exist.
     @param[out] info A node that contains any errors.
     @param options An options node. Include "tolerance" to set the point tolerance.

     @return True if the adjsets in all domains contained no errors; False if
             there were errors.
     */
    bool CONDUIT_BLUEPRINT_API validate(const conduit::Node &doms,
                                        const std::string &adjsetName,
                                        conduit::Node &info,
                                        const conduit::Node &options = conduit::Node());

    //-------------------------------------------------------------------------
    /**
     @brief Given a set of domains, make sure that the specified adjset in them
            is valid and flag any errors in the info node. This function will
            make sure that each domain's adjset references valid entities in
            neighboring domains.

     @param doms A node containing the domains. There must be multiple domains.
     @param adjsetName The name of the adjset in all domains. It must exist.
     @param association Then type of the adjset's association.
     @param topologyName The name of the adjset's topology.
     @param coordsetName The name of the topology coordset.
     @param[out] info A node that contains any errors.
     @param PQ The PointQuery that will handle vertex association queries.
     @param MQ The MatchQuery that will handle element association queries.
     @param checkMultiDomain Whether we want to check that an input blueprint
                             contains multiple domains or not. For parallel,
                             we do not want to check this since each rank may
                             have a single domain locally.

     @note The association, topologyName, and coordsetName are passed in so
           the routine does not have to figure them out. In parallel, the
           rank might not have any domain to get them from. We can handle
           the parallel problem from the parallel calling routine.

     @return True if the adjsets in all domains contained no errors; False if
             there were errors.
     */
    bool CONDUIT_BLUEPRINT_API validate(const conduit::Node &doms,
                                        const std::string &adjsetName,
                                        const std::string &association,
                                        const std::string &topologyName,
                                        const std::string &coordsetName,
                                        conduit::Node &info,
                                        query::PointQuery &PQ,
                                        query::MatchQuery &MQ,
                                        bool checkMultiDomain);

    /**
     @brief Traverse the adjset groups and make sure that the points are the same
            on both sides of the interface. This is more restrictive than just
            checking whether they exist on the other domain. Now, they have to be
            the same point.

     @param mesh A node that contains one or more mesh domains.
     @param adjsetName The name of the adjset to check. This must be a pairwise adjset.
     @param[out] info Information about the failed adjset comparison.
     @param options An options node. Include "tolerance" to set the point tolerance.

     @return True if the adjset are the same pointwise across each interface;
             False otherwise.
     */
     bool CONDUIT_BLUEPRINT_API compare_pointwise(conduit::Node &mesh,
                                                  const std::string &adjsetName,
                                                  conduit::Node &info,
                                                  const conduit::Node &options = conduit::Node());

     /**
      @brief Converts adjsets for domain boundary pairs into meshes in
             the out node. We get point meshes or face meshes, depending on the
             adjset type. This can aid visualization.

      @param mesh A node that contains all domains.
      @param adjsetName The name of the adjset to select.
      @param[out] out A node to contain the resulting mesh domains that represent
                      the adjsets as point meshes.
      */
     void CONDUIT_BLUEPRINT_API to_topo(conduit::Node &mesh,
                                        const std::string &adjsetName,
                                        conduit::Node &out);
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::adjset --
//-----------------------------------------------------------------------------


    //-------------------------------------------------------------------------
    /**
      @brief Linearly interpolate n points along a line segment from A to B
      @param A Location of first point
      @param B Location of second point
      @param n Number of interpolated points (including end points)
      @param out The interpolated points, with one vector per component
      @param base The index to store the interpolated points in \a out
      @param allocate Whether to clear and allocate \a out

      By default, this routine clears and reallocates the output vector.
      If desired, the caller can add values into an output vector that is
      already allocated by specifying a base index to store new values and
      setting \a allocate to false.
     */
    void CONDUIT_BLUEPRINT_API lerp_one(const Node& A,
                                        const Node& B,
                                        int n,
                                        Node& out,
                                        int base = 0,
                                        bool allocate = true);

    //-------------------------------------------------------------------------
    /**
      @brief Linearly interpolate n points along many line segments
      @param As Locations of first point of each segment
      @param Bs Locations of second point of each segment
      @param n Number of interpolated points (including end points)
               along each segment
      @param out The interpolated points.  As with \a As and \a Bs, each
                 inner vector stores a component of the point and the
                 outer vector stores all the points.
     */
    void CONDUIT_BLUEPRINT_API lerp(const Node& As,
                                    const Node& Bs,
                                    int n,
                                    Node& out);

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
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif
