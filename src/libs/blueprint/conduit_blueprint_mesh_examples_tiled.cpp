// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples_tiled.hpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh_examples_tiled.hpp"
#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_exports.h"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_curves.hpp"

#include <cmath>
#include <algorithm>
#include <array>
#include <numeric>

// Uncomment this to add some fields on the filed mesh prior to reordering.
// #define CONDUIT_TILER_DEBUG_FIELDS

// Uncomment this to try an experimental mode that uses the partitioner to
// do reordering.
// #define CONDUIT_USE_PARTITIONER_FOR_REORDER

// Uncomment this to use a simpler tiled pattern for debugging.
// #define CONDUIT_SIMPLE_TILED_PATTERN

// Uncomment this to print information about block splitting.
// #define CONDUIT_DEBUG_BLOCK_SPLITTER

// Uncomment this to write blocks to a Blueprint file.
// #define CONDUIT_WRITE_BLOCKS
#ifdef CONDUIT_WRITE_BLOCKS
#include <conduit_relay_io_blueprint.hpp>
#endif

//-----------------------------------------------------------------------------
// -- begin conduit::--
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
/// Methods that generate example meshes.
//-----------------------------------------------------------------------------
namespace examples
{

//-----------------------------------------------------------------------------
/// Detail
//-----------------------------------------------------------------------------
namespace detail
{

/**
 \brief Keep track of some tile information.
 */
class Tile
{
public:
    static const conduit::index_t INVALID_POINT;

    Tile() : ptids()
    {
    }

    /// Reset the tile. 
    void reset(size_t npts)
    {
        ptids = std::vector<conduit::index_t>(npts, INVALID_POINT);
    }

    /// Return the point ids.
          std::vector<conduit::index_t> &getPointIds() { return ptids; }
    const std::vector<conduit::index_t> &getPointIds() const { return ptids; }

    /// Get the specified point ids for this tile using the supplied indices.
    std::vector<conduit::index_t> getPointIds(const std::vector<conduit::index_t> &indices) const
    {
        std::vector<conduit::index_t> ids;
        if(!ptids.empty())
        {
            ids.reserve(indices.size());
            for(const auto &idx : indices)
                ids.push_back(ptids[idx]);
        }
        return ids;
    }

    // Set the point ids (if the ids are valid)
    void setPointIds(const std::vector<conduit::index_t> &indices, const std::vector<conduit::index_t> &ids)
    {
        for(size_t i = 0; i < indices.size(); i++)
        {
            if(ids[i] != INVALID_POINT)
                ptids[indices[i]] = ids[i];
        }
    }

private:
    std::vector<conduit::index_t> ptids;  //!< This tile's point ids.
};

const conduit::index_t Tile::INVALID_POINT = -1;

//---------------------------------------------------------------------------
class TilerBase
{
public:
    static const int BoundaryLeft = 0;
    static const int BoundaryRight = 1;
    static const int BoundaryBottom = 2;
    static const int BoundaryTop = 3;
    static const int BoundaryBack = 4;
    static const int BoundaryFront = 5;

    static const conduit::index_t InvalidDomain = -1;

    TilerBase();
    virtual ~TilerBase() = default;

protected:
    /// Fill a default tile pattern into the filter.
    void initialize();

    /// Fill in the tile pattern from a Node.
    void initialize(const conduit::Node &t);

    /// Return the topology (the first one)
    const conduit::Node &getTopology() const
    {
        const conduit::Node &t = m_tile.fetch_existing("topologies");
        return t[0];
    }

    /// Return the coordset
    const conduit::Node &getCoordset() const
    {
        const conduit::Node &t = getTopology();
        std::string coordsetName(t.fetch_existing("coordset").as_string());
        return m_tile.fetch_existing("coordsets/" + coordsetName);
    }

    /// Return point indices of points along left edge.
    const std::vector<conduit::index_t> &left() const { return m_left; }

    /// Return point indices of points along right edge.
    const std::vector<conduit::index_t> &right() const { return m_right; }

    /// Return point indices of points along bottom edge.
    const std::vector<conduit::index_t> &bottom() const { return m_bottom; }

    /// Return point indices of points along top edge.
    const std::vector<conduit::index_t> &top() const { return m_top; }

    /// Return tile width
    double width() const { return m_width; }

    /// Return tile height
    double height() const { return m_height; }

    /// Make the adjset name for 2 domains.
    std::string adjset_name(conduit::index_t d0, conduit::index_t d1) const;

    /// Turn a node into an index_t vector.
    std::vector<conduit::index_t> toIndexVector(const conduit::Node &n) const
    {
        auto acc = n.as_index_t_accessor();
        std::vector<index_t> vec;
        vec.reserve(acc.number_of_elements());
        for(conduit::index_t i = 0; i < acc.number_of_elements(); i++)
            vec.push_back(acc[i]);
        return vec;
    }

    /// Iterate over faces
    template <typename Connectivity, typename Body>
    void iterateFaces(const Connectivity &conn,
                      conduit::index_t nelem,
                      conduit::index_t sides,
                      conduit::index_t *idlist,
                      const std::vector<conduit::index_t> &ptids,
                      conduit::index_t offset,
                      conduit::index_t offset2,
                      bool reverse,
                      int stype,
                      Body &&body) const
    {
        if(reverse)
        {
            auto s1 = sides - 1;
            for(conduit::index_t i = 0; i < nelem; i++)
            {
                auto start = i * sides;
                for(conduit::index_t s = 0; s < sides; s++)
                    idlist[s1 - s] = offset + ptids[conn[start + s] + offset2];
                body(idlist, sides, stype);
            }
        }
        else
        {
            for(conduit::index_t i = 0; i < nelem; i++)
            {
                auto start = i * sides;
                for(conduit::index_t s = 0; s < sides; s++)
                    idlist[s] = offset + ptids[conn[start + s] + offset2];
                body(idlist, sides, stype);
            }
        }
    }

    /// Iterate over the tile's elements and apply a lambda.
    template <typename Body>
    void iterateFaces(const std::vector<conduit::index_t> &ptids,
                      conduit::index_t offset,
                      conduit::index_t offset2,
                      bool reverse,
                      int stype,
                      Body &&body) const
    {
        const conduit::Node &topo = getTopology();
        std::string shape = topo.fetch_existing("elements/shape").as_string();
        conduit::index_t sides = 0;
        if(shape == "tri")
            sides = 3;
        else if(shape == "quad")
            sides = 4;

        // Handle triangles and quads.
        const conduit::Node &n_conn = topo.fetch_existing("elements/connectivity");
        if(sides == 3 || sides == 4)
        {
            conduit::index_t idlist[4];
            bool handled = false;
            if(n_conn.dtype().spanned_bytes() == n_conn.dtype().strided_bytes())
            {
                if(n_conn.dtype().is_index_t())
                {
                    iterateFaces(n_conn.as_index_t_ptr(),
                                 n_conn.dtype().number_of_elements() / sides,
                                 sides, idlist, ptids,
                                 offset, offset2, reverse, stype, body);
                    handled = true;
                }
                else if(n_conn.dtype().is_int32())
                {
                    iterateFaces(n_conn.as_int32_ptr(),
                                 n_conn.dtype().number_of_elements() / sides,
                                 sides, idlist, ptids,
                                 offset, offset2, reverse, stype, body);
                    handled = true;
                }
            }
            if(!handled)
            {
                iterateFaces(n_conn.as_index_t_accessor(),
                             n_conn.dtype().number_of_elements() / sides,
                             sides, idlist, ptids,
                             offset, offset2, reverse, stype, body);
            }
        }
        else if(shape == "polygonal" || shape == "mixed")
        {
            // handle polygons
            const auto conn = n_conn.as_index_t_accessor();
            const auto sizes = topo.fetch_existing("elements/sizes").as_index_t_accessor();
            const conduit::index_t nelem = sizes.number_of_elements();
            conduit::index_t start = 0;
            std::vector<index_t> idlist(10);
            if(reverse)
            {
                for(conduit::index_t i = 0; i < nelem; i++)
                {
                    const auto esides = sizes[i];
                    idlist.reserve(esides);
                    for(conduit::index_t s = 0; s < esides; s++)
                        idlist[s] = offset + ptids[conn[start + esides - s] + offset2];
                    body(&idlist[0], esides, stype);
                    start += esides;
                }
            }
            else
            {
                for(conduit::index_t i = 0; i < nelem; i++)
                {
                    const auto esides = sizes[i];
                    idlist.reserve(esides);
                    for(conduit::index_t s = 0; s < esides; s++)
                        idlist[s] = offset + ptids[conn[start + s] + offset2];
                    body(&idlist[0], esides, stype);
                    start += esides;
                }
            }
        }
    }

protected:
    conduit::Node m_tile;
    double m_width, m_height;
    std::vector<conduit::index_t> m_left, m_right, m_bottom, m_top;
    std::string meshName, boundaryMeshName;
};

//---------------------------------------------------------------------------
TilerBase::TilerBase() : m_tile(), m_width(0.), m_height(0.),
                         m_left(), m_right(), m_bottom(), m_top(),
                         meshName("mesh"), boundaryMeshName("boundary")
{
    initialize();
}

//---------------------------------------------------------------------------
void
TilerBase::initialize()
{
#ifdef CONDUIT_SIMPLE_TILED_PATTERN
    // Simpler case for debugging.
    const double x[] = {0., 1., 0., 1.};
    const double y[] = {0., 0., 1., 1.};
    const conduit::index_t conn[] = {0,1,3,2};
    const conduit::index_t color[] = {0};

    const conduit::index_t left[] = {0,2};
    const conduit::index_t right[] = {1,3};
    const conduit::index_t bottom[] = {0,1};
    const conduit::index_t top[] = {2,3};
#else
    // Default pattern
    const double x[] = {
        0., 3., 10., 17., 20.,
        0., 3., 17., 20.,
        5., 15.,
        7., 10., 13.,
        0., 7., 10., 13., 20.,
        7., 10., 13.,
        5., 15.,
        0., 3., 17., 20.,
        0., 3., 10., 17., 20.,
    };

    const double y[] = {
        0., 0., 0., 0., 0.,
        3., 3., 3., 3.,
        5., 5.,
        7., 7., 7.,
        10., 10., 10., 10., 10.,
        13., 13., 13.,
        15., 15.,
        17., 17., 17., 17.,
        20., 20., 20., 20., 20.,
    };

    const conduit::index_t conn[] = {
        // lower-left quadrant
        0,1,6,5,
        1,2,9,6,
        2,12,11,9,
        5,6,9,14,
        9,11,15,14,
        11,12,16,15,
        // lower-right quadrant
        2,3,7,10,
        3,4,8,7,
        7,8,18,10,
        2,10,13,12,
        12,13,17,16,
        10,18,17,13,
        // upper-left quadrant
        14,22,25,24,
        14,15,19,22,
        15,16,20,19,
        24,25,29,28,
        22,30,29,25,
        19,20,30,22,
        // upper-right quadrant
        16,17,21,20,
        17,18,23,21,
        18,27,26,23,
        20,21,23,30,
        23,26,31,30,
        26,27,32,31
    };

    const conduit::index_t color[] = {
        0, 1, 1, 1, 1, 0,
        1, 0, 1, 1, 0, 1,
        1, 1, 0, 0, 1, 1,
        0, 1, 1, 1, 1, 0
    };

    const conduit::index_t left[] = {0,5,14,24,28};
    const conduit::index_t right[] = {4,8,18,27,32};
    const conduit::index_t bottom[] = {0,1,2,3,4};
    const conduit::index_t top[] = {28,29,30,31,32};
#endif

    // Define the tile as a topology.
    conduit::Node opts;
    opts["coordsets/coords/type"] = "explicit";
    opts["coordsets/coords/values/x"].set(x, sizeof(x) / sizeof(double));
    opts["coordsets/coords/values/y"].set(y, sizeof(y) / sizeof(double));
    opts["topologies/tile/type"] = "unstructured";
    opts["topologies/tile/coordset"] = "coords";
    opts["topologies/tile/elements/shape"] = "quad";
    constexpr auto nelem = (sizeof(conn) / sizeof(conduit::index_t)) / 4;
    opts["topologies/tile/elements/connectivity"].set(conn, nelem * 4);
    std::vector<conduit::index_t> size(nelem, 4);
    opts["topologies/tile/elements/sizes"].set(size.data(), size.size());
    opts["fields/color/association"] = "element";
    opts["fields/color/topology"] = "tile";
    opts["fields/color/values"].set(color, sizeof(color) / sizeof(conduit::index_t));

    // Define tile boundary indices.
    opts["left"].set(left, sizeof(left) / sizeof(conduit::index_t));
    opts["right"].set(right, sizeof(right) / sizeof(conduit::index_t));
    opts["bottom"].set(bottom, sizeof(bottom) / sizeof(conduit::index_t));
    opts["top"].set(top, sizeof(top) / sizeof(conduit::index_t));

    initialize(opts);
}

//---------------------------------------------------------------------------
void
TilerBase::initialize(const conduit::Node &t)
{
    std::vector<std::string> required{"coordsets", "topologies", "left", "right", "bottom", "top"};
    for(const auto &name : required)
    {
        if(!t.has_child(name))
        {
            CONDUIT_ERROR("Node does not contain key: " << name);
        }
    }

    // Get the tile boundaries and convert them.
    m_left = toIndexVector(t.fetch_existing("left"));
    m_right = toIndexVector(t.fetch_existing("right"));
    m_bottom = toIndexVector(t.fetch_existing("bottom"));
    m_top = toIndexVector(t.fetch_existing("top"));
    if(m_left.size() != m_right.size())
    {
        CONDUIT_ERROR("left/right vectors have different lengths.");
    }
    if(m_bottom.size() != m_top.size())
    {
        CONDUIT_ERROR("bottom/top vectors have different lengths.");
    }

    // Save the tile definition.
    m_tile.set(t);

    // Make sure the coordset is 2D, explicit.
    if(conduit::blueprint::mesh::coordset::dims(getCoordset()) != 2)
    {
        CONDUIT_ERROR("The tile coordset must be 2D.");
    }
    if(getCoordset()["type"].as_string() != "explicit")
    {
        CONDUIT_ERROR("The tile coordset must be explicit.");
    }
    // Make sure the topology is 2D, unstructured
    if(conduit::blueprint::mesh::topology::dims(getTopology()) != 2)
    {
        CONDUIT_ERROR("The tile topology must be 2D.");
    }
    if(getTopology()["type"].as_string() != "unstructured")
    {
        CONDUIT_ERROR("The tile topology must be unstructured.");
    }

    // Compute the tile extents.
    if(t.has_path("translate/x"))
        m_width = t["translate/x"].to_double();
    else
    {
        const auto &xc = getCoordset().fetch_existing("values/x").as_double_array();
        m_width = xc.max() - xc.min();
    }

    if(t.has_path("translate/y"))
         m_height = t["translate/y"].to_double();
    else
    {
        const auto &yc = getCoordset().fetch_existing("values/y").as_double_array();
        m_height = yc.max() - yc.min();
    }
}

//---------------------------------------------------------------------------
std::string TilerBase::adjset_name(conduit::index_t d0, conduit::index_t d1) const
{
    if(d0 > d1)
        std::swap(d0, d1);
    std::stringstream ss;
    ss << conduit::blueprint::mesh::adjset::group_prefix() << "_" << d0 << "_" << d1;
    return ss.str();
}

//---------------------------------------------------------------------------
/**
 \brief Build a mesh from tiles. There is a default tile pattern, although it can
        be replaced using an options Node containing new tile information.
 */
class Tiler : public TilerBase
{
public:
    Tiler();

    /// Generate the tiled mesh.
    void generate(conduit::index_t nx, conduit::index_t ny, conduit::index_t nz,
                  conduit::Node &res,
                  const conduit::Node &options);
protected:
    /// Creates the points for the tile (if they need to be created).
    void addPoints(const double M[3][3],
                   std::vector<conduit::index_t> &ptids,
                   std::vector<double> &x,
                   std::vector<double> &y,
                   std::vector<conduit::index_t> &srcPointId)
    {
        // Iterate through points in the template and add them if they have
        // not been created yet.
        const auto &xpts = getCoordset().fetch_existing("values/x").as_double_array();
        const auto &ypts = getCoordset().fetch_existing("values/y").as_double_array();
        for(conduit::index_t i = 0; i < xpts.number_of_elements(); i++)
        {
            if(ptids[i] == Tile::INVALID_POINT)
            {
                ptids[i] = static_cast<int>(x.size());

                // (x,y,1) * M
                double xc = xpts[i] * M[0][0] + ypts[i] * M[1][0] + M[2][0];
                double yc = xpts[i] * M[0][1] + ypts[i] * M[1][1] + M[2][1];
                double h  = xpts[i] * M[0][2] + ypts[i] * M[1][2] + M[2][2];
                xc /= h;
                yc /= h;
                x.push_back(xc);
                y.push_back(yc);

                srcPointId.push_back(i);
            }
        }
    }

    /// Emit the hex cells using this tile's point ids.
    void addVolumeElements(const std::vector<conduit::index_t> &ptids,
                 conduit::index_t plane1Offset,
                 conduit::index_t plane2Offset,
                 std::vector<conduit::index_t> &conn,
                 std::vector<conduit::index_t> &sizes) const
    {
        const conduit::Node &topo = getTopology();
        std::string shape = topo.fetch_existing("elements/shape").as_string();
        conduit::index_t sides = 0;
        if(shape == "tri")
            sides = 3;
        else if(shape == "quad")
            sides = 4;

        if(sides == 3 || sides == 4)
        {
            const conduit::Node &n_conn = topo.fetch_existing("elements/connectivity");
            const auto tileconn = n_conn.as_index_t_accessor();
            const conduit::index_t nelem = tileconn.number_of_elements() / sides;
            for(conduit::index_t i = 0; i < nelem; i++)
            {
                conduit::index_t start = i * sides;
                for(conduit::index_t s = 0; s < sides; s++)
                    conn.push_back(plane1Offset + ptids[tileconn[start + s]]);

                for(conduit::index_t s = 0; s < sides; s++)
                    conn.push_back(plane2Offset + ptids[tileconn[start + s]]);

                sizes.push_back(2 * sides);
            }
        }
        else
        {
            CONDUIT_ERROR("Tiling polygonal shapes into 3D polyhedra is not yet supported.");
        }
    }

    /// Determine which boundaries are needed.
    void boundaryFlags(const conduit::Node &options, bool flags[6]) const;

    // Iterate over 2D boundaries
    template <typename Body>
    void iterateBoundary2D(const std::vector<Tile> &tiles,
                           conduit::index_t nx,
                           conduit::index_t ny,
                           const bool flags[6],
                           Body &&body) const;

    /// Iterate over 3D boundaries.
    template <typename Body>
    void iterateBoundary3D(const std::vector<Tile> &tiles,
                           conduit::index_t nx,
                           conduit::index_t ny,
                           conduit::index_t nz,
                           conduit::index_t nPtsPerPlane,
                           const bool flags[6],
                           bool reverseAllowed,
                           Body &&body) const;

    /// Compute domain id, if it exists, or return InvalidDomain.
    conduit::index_t domainIndex(conduit::index_t d0, conduit::index_t d1, conduit::index_t d2,
                                  const std::vector<conduit::index_t> &domain,
                                  const std::vector<conduit::index_t> &domains) const;

    /// Compute domain id, if it exists, or return InvalidDomain.
    conduit::index_t domainIndex(const conduit::index_t delta[3],
                                  const std::vector<conduit::index_t> &domain,
                                  const std::vector<conduit::index_t> &domains) const;

    /// Add ptids to a values node for an adjset, possibly transforming the point ids.
    void addAdjsetValues(const std::vector<conduit::index_t> &ptids,
                         conduit::index_t planeOffset,
                         bool reorder,
                         const std::vector<conduit::index_t> &old2NewPoint,
                         conduit::Node &values) const;

    /// Add adjacency set
    void addAdjset(const std::vector<Tile> &tiles,
                   conduit::index_t nx,
                   conduit::index_t ny,
                   conduit::index_t nz,
                   conduit::index_t ptsPerPlane,
                   bool reorder,
                   const std::vector<conduit::index_t> &old2NewPoint,
                   const conduit::Node &options,
                   conduit::Node &out) const;
};

//---------------------------------------------------------------------------
Tiler::Tiler() : TilerBase()
{
}

//---------------------------------------------------------------------------
/**
 \brief Generate coordinate and connectivity arrays using a tiled mesh pattern,
        given by the Tile class.

 \param nx The number of tiles in the X dimension.
 \param ny The number of tiles in the Y dimension.
 \param nz The number of tiles in the Z dimension.
 \param[out] res The output node.
 \param options A node that may contain additional control options.
 */
void
Tiler::generate(conduit::index_t nx, conduit::index_t ny, conduit::index_t nz,
    conduit::Node &res,
    const conduit::Node &options)
{
    std::vector<double> x, y, z;
    std::vector<conduit::index_t> conn, sizes, shapes, bconn, bsizes, srcPointIds;
    std::vector<int> btype;

    // Process any options.
    if(options.has_path("tile"))
        initialize(options.fetch_existing("tile"));

    std::string reorder;
    if(options.has_path("reorder"))
        reorder = options.fetch_existing("reorder").as_string();

    if(options.has_path("meshname"))
        meshName = options.fetch_existing("meshname").as_string();
    if(options.has_path("boundarymeshname"))
        boundaryMeshName = options.fetch_existing("boundarymeshname").as_string();

    conduit::DataType indexDT(conduit::DataType::index_t());
    if(options.has_child("datatype"))
    {
        auto s = options.fetch_existing("datatype").as_string();
        if((s == "int") || (s == "int32") || (s == "integer"))
        {
            indexDT = conduit::DataType::int32();
        }
    }

    // Make a transformation matrix for the tile points.
    double origin[] = {0., 0., 0.};
    double tx = width(), ty = height();
    double z1 = std::max(width(), height()) * nz;
    double M[3][3] = {{1., 0., 0.}, {0., 1., 0.}, {0., 0., 1.}};
    if(options.has_path("extents"))
    {
        // Extents for the domain were given. Fit the domain into it.
        auto extents = options.fetch_existing("extents").as_double_accessor();
        tx = (extents[1] - extents[0]) / nx;
        ty = (extents[3] - extents[2]) / ny;
        origin[0] = extents[0];
        origin[1] = extents[2];
        origin[2] = extents[4];
        z1 = extents[5];
    }
    else
    {
        // There are no extents so figure out some based on the domains, if present.
        if(options.has_path("domain") && options.has_path("domains"))
        {
            auto domain = options.fetch_existing("domain").as_int_accessor();
            auto domains = options.fetch_existing("domains").as_int_accessor();
            if(domain.number_of_elements() == 3 &&
               domain.number_of_elements() == domains.number_of_elements())
            {
                origin[0] = domain[0] * nx * width();
                origin[1] = domain[1] * ny * height();
                origin[2] = domain[2] * z1;
                z1 = origin[2] + z1;
            }
        }
    }
    // Scaling
    M[0][0] = tx / width();
    M[1][1] = ty / height();
    // Translation
    M[2][0] = origin[0];
    M[2][1] = origin[1];

    // Number of tile points.
    const auto nTilePts = getCoordset().fetch_existing("values/x").dtype().number_of_elements();

    // Make a pass where we make nx*ny tiles so we can generate their points.
    std::vector<Tile> tiles(nx * ny);
    for(conduit::index_t j = 0; j < ny; j++)
    {
        M[2][0] = origin[0];
        for(conduit::index_t i = 0; i < nx; i++)
        {
            Tile &current = tiles[(j*nx + i)];

            // The first time we've used the tile, set its size.
            current.reset(nTilePts);

            // Copy some previous points over so they can be shared.
            if(i > 0)
            {
               Tile &prevX = tiles[(j*nx + i - 1)];
               current.setPointIds(left(), prevX.getPointIds(right()));
            }
            if(j > 0)
            {
               Tile &prevY = tiles[((j-1)*nx + i)];
               current.setPointIds(bottom(), prevY.getPointIds(top()));
            }

            addPoints(M, current.getPointIds(), x, y, srcPointIds);
            M[2][0] += tx;
        }
        M[2][1] += ty;
    }

    conduit::index_t ptsPerPlane = 0;
    std::string shape2, shape3;
    shape2 = getTopology().fetch_existing("elements/shape").as_string();
    if(nz < 1)
    {
        const bool hasShapes = getTopology().has_path("elements/shapes");
        index_t_accessor shapeAccess;
        if(hasShapes)
        {
            shapeAccess = getTopology().fetch_existing("elements/shapes").as_index_t_accessor();
        }

        // Iterate over the tiles and add their quads.
        // TODO: reserve size for conn, sizes
        for(conduit::index_t j = 0; j < ny; j++)
        {
            for(conduit::index_t i = 0; i < nx; i++)
            {
                Tile &current = tiles[(j*nx + i)];
                int si = 0;
                iterateFaces(current.getPointIds(), 0, 0, false, BoundaryBack,
                    [&](const conduit::index_t *ids, conduit::index_t npts, int)
                    {
                        for(conduit::index_t pi = 0; pi < npts; pi++)
                            conn.push_back(ids[pi]);
                        sizes.push_back(npts);

                        if(hasShapes)
                        {
                          shapes.push_back(shapeAccess[si++]);
                        }
                    });
            }
        }
        // NOTE: z coords in output will be empty.
    }
    else
    {
        ptsPerPlane = static_cast<conduit::index_t>(x.size());

        shape3 = (shape2 == "tri") ? "wedge" : "hex";

        // We have x,y points now. We need to replicate them to make multiple planes.
        // We make z coordinates too.
        conduit::index_t nplanes = nz + 1;
        x.reserve(ptsPerPlane * nplanes);
        y.reserve(ptsPerPlane * nplanes);
        z.reserve(ptsPerPlane * nplanes);
        srcPointIds.reserve(ptsPerPlane * nplanes);
        for(conduit::index_t i = 0; i < ptsPerPlane; i++)
            z.push_back(origin[2]);
        for(conduit::index_t p = 1; p < nplanes; p++)
        {
            double t = static_cast<double>(p) / static_cast<double>(nplanes - 1);
            double zvalue = (1. - t) * origin[2] + t * z1;
            for(conduit::index_t i = 0; i < ptsPerPlane; i++)
            {
                x.push_back(x[i]);
                y.push_back(y[i]);
                z.push_back(zvalue);
                srcPointIds.push_back(srcPointIds[i]);
            }
        }

        // Iterate over the tiles and add their hexs.
        // TODO: reserve size for conn, sizes
        for(conduit::index_t k = 0; k < nz; k++)
        {
            conduit::index_t offset1 = k * ptsPerPlane;
            conduit::index_t offset2 = offset1 + ptsPerPlane;

            for(conduit::index_t j = 0; j < ny; j++)
            {
                for(conduit::index_t i = 0; i < nx; i++)
                {
                    Tile &current = tiles[(j*nx + i)];
                    addVolumeElements(current.getPointIds(), offset1, offset2, conn, sizes);
                }
            }
        }
    }

    // Make the Blueprint mesh.
    res["coordsets/coords/type"] = "explicit";
    res["coordsets/coords/values/x"].set(x);
    res["coordsets/coords/values/y"].set(y);
    if(!z.empty())
        res["coordsets/coords/values/z"].set(z);

    conduit::Node &topo = res["topologies/" + meshName];
    topo["type"] = "unstructured";
    topo["coordset"] = "coords";
    topo["elements/shape"] = z.empty() ? shape2 : shape3;
    conduit::Node tmp;
    tmp.set_external(conn.data(), conn.size());
    tmp.to_data_type(indexDT.id(), topo["elements/connectivity"]);
    tmp.set_external(sizes.data(), sizes.size());
    tmp.to_data_type(indexDT.id(), topo["elements/sizes"]);
    // 2D only for now.
    if(!shapes.empty() && z.empty())
    {
      tmp.set_external(shapes.data(), shapes.size());
      tmp.to_data_type(indexDT.id(), topo["elements/shapes"]);

      // Make offsets
      std::vector<index_t> offsets;
      offsets.reserve(sizes.size());
      index_t offset = 0;
      for(auto s : sizes)
      {
        offsets.push_back(offset);
        offset += s;
      }
      tmp.set_external(offsets.data(), offsets.size());
      tmp.to_data_type(indexDT.id(), topo["elements/offsets"]);

      // Copy shape_map.
      if(getTopology().has_path("elements/shape_map"))
      {
        topo["elements/shape_map"].set((getTopology().fetch_existing("elements/shape_map")));
      }
    }
#ifdef CONDUIT_TILER_DEBUG_FIELDS
    // Add fields to test the reordering.
    std::vector<conduit::index_t> nodeids, elemids;
    auto npts = static_cast<conduit::index_t>(x.size());
    nodeids.reserve(npts);
    for(conduit::index_t i = 0; i < npts; i++)
        nodeids.push_back(i);
    res["fields/nodeids/topology"] = meshName;
    res["fields/nodeids/association"] = "vertex";
    res["fields/nodeids/values"].set(nodeids);

    auto nelem = static_cast<conduit::index_t>(sizes.size());
    elemids.reserve(nelem);
    for(conduit::index_t i = 0; i < nelem; i++)
        elemids.push_back(i);
    res["fields/elemids/topology"] = meshName;
    res["fields/elemids/association"] = "element";
    res["fields/elemids/values"].set(elemids);

    std::vector<double> dist;
    dist.reserve(npts);
    if(nz < 1)
    {
        for(conduit::index_t i = 0; i < npts; i++)
            dist.push_back(sqrt(x[i]*x[i] + y[i]*y[i]));
    }
    else
    {
        for(conduit::index_t i = 0; i < npts; i++)
            dist.push_back(sqrt(x[i]*x[i] + y[i]*y[i] + z[i]*z[i]));
    }
    res["fields/dist/topology"] = meshName;
    res["fields/dist/association"] = "vertex";
    res["fields/dist/values"].set(dist);
#endif

    // If there are any fields on the mesh then replicate them.
    if(m_tile.has_child("fields"))
    {
        // Make source cell ids list.
        auto nTileElem = conduit::blueprint::mesh::utils::topology::length(getTopology());
        std::vector<conduit::index_t> srcCellIds;
        srcCellIds.reserve(nTileElem * nx * ny * std::max(nz, conduit::index_t{1}));
        for(conduit::index_t k = 0; k < std::max(nz, conduit::index_t{1}); k++)
        for(conduit::index_t j = 0; j < ny; j++)
        for(conduit::index_t i = 0; i < nx; i++)
        {
            for(conduit::index_t ci = 0; ci < nTileElem; ci++)
                srcCellIds.push_back(ci);
        }

        // Make new fields.
        const conduit::Node &fields = m_tile.fetch_existing("fields");
        for(conduit::index_t fi = 0; fi < fields.number_of_children(); fi++)
        {
            const conduit::Node &f = fields[fi];
            if(f["topology"].as_string() == getTopology().name())
            {
                conduit::Node &newfields = res["fields"];
                conduit::Node &destField = newfields[f.name()];
                destField["topology"] = meshName;
                destField["association"] = f["association"];
                if(f["association"].as_string() == "element")
                    conduit::blueprint::mesh::utils::slice_field(f["values"], srcCellIds, destField["values"]);
                else if(f["association"].as_string() == "vertex")
                    conduit::blueprint::mesh::utils::slice_field(f["values"], srcPointIds, destField["values"]);
            }
        }
    }

    // Reorder the elements unless it was turned off.
    std::vector<conduit::index_t> old2NewPoint;
    bool doReorder = (reorder == "kdtree" || reorder == "hilbert");
    if(doReorder)
    {
        // We need offsets.
        conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(topo, topo["elements/offsets"]);

        // Create a new order for the mesh elements.
        std::vector<conduit::index_t> elemOrder;
        if(reorder == "kdtree")
            elemOrder = conduit::blueprint::mesh::utils::topology::spatial_ordering(topo);
        else
            elemOrder = conduit::blueprint::mesh::utils::topology::hilbert_ordering(topo);

#ifdef CONDUIT_USE_PARTITIONER_FOR_REORDER
        // NOTE: This was an idea I had after I made reorder. Reordering is like
        //       making an explicit selection for the partitioner. Can we just use
        //       the partitioner? Kind of, it turns out.
        //
        //       1. Elements are reordered like we want
        //       2. Nodes are not reordered in their order of use by elements.
        //       3. Passing the same node as input/output does bad things.

        // Make an explicit selection for partition to do the reordering.
        conduit::Node options;
        conduit::Node &sel = options["selections"].append();
        sel["type"] = "explicit";
        sel["topology"] = meshName;
        sel["elements"].set_external(const_cast<conduit::index_t *>(elemOrder.data()), elemOrder.size());
        conduit::Node output;
        conduit::blueprint::mesh::partition(res, options, output);

        // Extract the vertex mapping.
        auto ids = output.fetch_existing("fields/original_vertex_ids/values/ids").as_index_t_accessor();
        for(conduit::index_t i = 0; i < ids.number_of_elements(); i++)
        {
            old2NewPoint.push_back(ids[i]);
        }
        res.reset();
        res.move(output);
#else
        conduit::blueprint::mesh::utils::topology::unstructured::reorder(
            topo, res["coordsets/coords"], res["fields"],
            elemOrder,
            topo, res["coordsets/coords"], res["fields"],
            old2NewPoint);
#endif
    }

    // Boundaries
    std::string bshape;
    bool flags[6];
    boundaryFlags(options, flags);
    
    if(nz < 1)
    {
        // 2D
        bshape = "line";
        if(doReorder)
        {
            iterateBoundary2D(tiles, nx, ny, flags,
                [&](const conduit::index_t *ids, conduit::index_t npts, int bnd)
                {
                    for(conduit::index_t i = 0; i < npts; i++)
                        bconn.push_back(old2NewPoint[ids[i]]); // Renumber
                    bsizes.push_back(npts);
                    btype.push_back(bnd + 1); // Make 1-origin
                });
        }
        else
        {
            iterateBoundary2D(tiles, nx, ny, flags,
                [&](const conduit::index_t *ids, conduit::index_t npts, int bnd)
                {
                    for(conduit::index_t i = 0; i < npts; i++)
                        bconn.push_back(ids[i]);
                    bsizes.push_back(npts);
                    btype.push_back(bnd + 1); // Make 1-origin
                });
        }
    }
    else
    {
        // 3D
        bshape = "quad";
        bool anyNonQuads = false;
        const bool reverseAllowed = true;
        if(doReorder)
        {
            iterateBoundary3D(tiles, nx, ny, nz, ptsPerPlane, flags, reverseAllowed,
                [&](const conduit::index_t *ids, conduit::index_t npts, int bnd)
                {
                    for(conduit::index_t i = 0; i < npts; i++)
                        bconn.push_back(old2NewPoint[ids[i]]); // Renumber
                    bsizes.push_back(npts);
                    btype.push_back(bnd + 1); // Make 1-origin
                    anyNonQuads |= (npts != 4);
                });
        }
        else
        {
            iterateBoundary3D(tiles, nx, ny, nz, ptsPerPlane, flags, reverseAllowed,
                [&](const conduit::index_t *ids, conduit::index_t npts, int bnd)
                {
                    for(conduit::index_t i = 0; i < npts; i++)
                        bconn.push_back(ids[i]);
                    bsizes.push_back(npts);
                    btype.push_back(bnd + 1); // Make 1-origin
                    anyNonQuads |= (npts != 4);
                });
        }
        if(anyNonQuads)
            bshape = "polygonal";
    }
    if(!bconn.empty())
    {
        conduit::Node &btopo = res["topologies/" + boundaryMeshName];
        btopo["type"] = "unstructured";
        btopo["coordset"] = "coords";
        btopo["elements/shape"] = bshape;

        tmp.set_external(bconn.data(), bconn.size());
        tmp.to_data_type(indexDT.id(), btopo["elements/connectivity"]);

        tmp.set_external(bsizes.data(), bsizes.size());
        tmp.to_data_type(indexDT.id(), btopo["elements/sizes"]);

        if(bshape == "polygonal")
            conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(btopo, btopo["elements/offsets"]);

        res["fields/boundary_attribute/topology"] = boundaryMeshName;
        res["fields/boundary_attribute/association"] = "element";
        res["fields/boundary_attribute/values"].set(btype);
    }

    // Build an adjacency set.
    addAdjset(tiles, nx, ny, nz, ptsPerPlane, doReorder, old2NewPoint, options, res);

#if 0
    // Print for debugging.
    conduit::Node opts;
    opts["num_children_threshold"] = 100000;
    opts["num_elements_threshold"] = 500;
    std::cout << res.to_summary_string(opts) << std::endl;
#endif
}

//---------------------------------------------------------------------------
void
Tiler::boundaryFlags(const conduit::Node &options, bool flags[6]) const
{
    bool handled = false;
    if(options.has_path("domain") && options.has_path("domains"))
    {
        auto domain = options.fetch_existing("domain").as_int_accessor();
        auto domains = options.fetch_existing("domains").as_int_accessor();
        if(domain.number_of_elements() == 3 &&
           domain.number_of_elements() == domains.number_of_elements())
        {
            int ndoms = domains[0] * domains[1] * domains[2];
            if(ndoms > 1)
            {
                flags[BoundaryLeft]   = (domain[0] == 0);
                flags[BoundaryRight]  = (domain[0] == domains[0]-1);
                flags[BoundaryBottom] = (domain[1] == 0);
                flags[BoundaryTop]    = (domain[1] == domains[1]-1);
                flags[BoundaryBack]   = (domain[2] == 0);
                flags[BoundaryFront]  = (domain[2] == domains[2]-1);

                handled = true;
            }
        }
    }
    if(!handled)
    {
        for(int i = 0; i < 6; i++)
            flags[i] = true;
    }
}

//---------------------------------------------------------------------------
template <typename Body>
void
Tiler::iterateBoundary2D(const std::vector<Tile> &tiles,
    conduit::index_t nx,
    conduit::index_t ny,
    const bool flags[6],
    Body &&body) const
{
    conduit::index_t idlist[2];

    if(flags[BoundaryLeft])
    {
        for(conduit::index_t i = 0, j = 0; j < ny; j++)
        {
            const Tile &current = tiles[(j*nx + i)];
            const auto ids = current.getPointIds(left());
            for(size_t bi = 0; bi < ids.size() - 1; bi++)
            {
                idlist[0] = ids[bi];
                idlist[1] = ids[bi + 1];
                body(idlist, 2, BoundaryLeft);
            }
        }
    }
    if(flags[BoundaryRight])
    {
        for(conduit::index_t i = nx - 1, j = 0; j < ny; j++)
        {
            const Tile &current = tiles[(j*nx + i)];
            const auto ids = current.getPointIds(right());
            for(size_t bi = 0; bi < ids.size() - 1; bi++)
            {
                idlist[0] = ids[bi];
                idlist[1] = ids[bi + 1];
                body(idlist, 2, BoundaryRight);
            }
        }
    }
    if(flags[BoundaryBottom])
    {
        for(conduit::index_t i = 0, j = 0; i < nx; i++)
        {
            const Tile &current = tiles[(j*nx + i)];
            const auto ids = current.getPointIds(bottom());
            for(size_t bi = 0; bi < ids.size() - 1; bi++)
            {
                idlist[0] = ids[bi];
                idlist[1] = ids[bi + 1];
                body(idlist, 2, BoundaryBottom);
            }
        }
    }
    if(flags[BoundaryTop])
    {
        for(conduit::index_t i = 0, j = ny - 1; i < nx; i++)
        {
            const Tile &current = tiles[(j*nx + i)];
            const auto ids = current.getPointIds(top());
            for(size_t bi = 0; bi < ids.size() - 1; bi++)
            {
                idlist[0] = ids[bi];
                idlist[1] = ids[bi + 1];
                body(idlist, 2, BoundaryTop);
            }
        }
    }
}

//---------------------------------------------------------------------------
template <typename Body>
void
Tiler::iterateBoundary3D(const std::vector<Tile> &tiles,
    conduit::index_t nx,
    conduit::index_t ny,
    conduit::index_t nz,
    conduit::index_t nPtsPerPlane,
    const bool flags[6],
    bool reverseAllowed,
    Body &&body) const
{
    conduit::index_t idlist[4];

    if(flags[BoundaryLeft])
    {
        for(conduit::index_t k = 0; k < nz; k++)
        {
            conduit::index_t offset1 = k * nPtsPerPlane;
            conduit::index_t offset2 = (k + 1) * nPtsPerPlane;
            for(conduit::index_t i = 0, j = 0; j < ny; j++)
            {
                const Tile &current = tiles[(j*nx + i)];
                const auto ids = current.getPointIds(left());
                for(size_t bi = 0; bi < ids.size() - 1; bi++)
                {
                    idlist[0] = offset1 + ids[bi];
                    idlist[1] = offset2 + ids[bi];
                    idlist[2] = offset2 + ids[bi + 1];
                    idlist[3] = offset1 + ids[bi + 1];
                    body(idlist, 4, BoundaryLeft);
                }
            }
        }
    }
    if(flags[BoundaryRight])
    {
        for(conduit::index_t k = 0; k < nz; k++)
        {
            conduit::index_t offset1 = k * nPtsPerPlane;
            conduit::index_t offset2 = (k + 1) * nPtsPerPlane;
            for(conduit::index_t i = nx - 1, j = 0; j < ny; j++)
            {
                const Tile &current = tiles[(j*nx + i)];
                const auto ids = current.getPointIds(right());
                for(size_t bi = 0; bi < ids.size() - 1; bi++)
                {
                    idlist[0] = offset1 + ids[bi];
                    idlist[1] = offset1 + ids[bi + 1];
                    idlist[2] = offset2 + ids[bi + 1];
                    idlist[3] = offset2 + ids[bi];
                    body(idlist, 4, BoundaryRight);
                }
            }
        }
    }
    if(flags[BoundaryBottom])
    {
        for(conduit::index_t k = 0; k < nz; k++)
        {
            conduit::index_t offset1 = k * nPtsPerPlane;
            conduit::index_t offset2 = (k + 1) * nPtsPerPlane;
            for(conduit::index_t i = 0, j = 0; i < nx; i++)
            {
                const Tile &current = tiles[(j*nx + i)];
                const auto ids = current.getPointIds(bottom());
                for(size_t bi = 0; bi < ids.size() - 1; bi++)
                {
                    idlist[0] = offset1 + ids[bi];
                    idlist[1] = offset1 + ids[bi + 1];
                    idlist[2] = offset2 + ids[bi + 1];
                    idlist[3] = offset2 + ids[bi];
                    body(idlist, 4, BoundaryBottom);
                }
            }
        }
    }
    if(flags[BoundaryTop])
    {
        for(conduit::index_t k = 0; k < nz; k++)
        {
            conduit::index_t offset1 = k * nPtsPerPlane;
            conduit::index_t offset2 = (k + 1) * nPtsPerPlane;
            for(conduit::index_t i = 0, j = ny - 1; i < nx; i++)
            {
                const Tile &current = tiles[(j*nx + i)];
                const auto ids = current.getPointIds(top());
                for(size_t bi = 0; bi < ids.size() - 1; bi++)
                {
                    idlist[0] = offset1 + ids[bi];
                    idlist[1] = offset2 + ids[bi];
                    idlist[2] = offset2 + ids[bi + 1];
                    idlist[3] = offset1 + ids[bi + 1];
                    body(idlist, 4, BoundaryTop);
                }
            }
        }
    }
    if(flags[BoundaryBack])
    {
        for(conduit::index_t j = 0; j < ny; j++)
        for(conduit::index_t i = 0; i < nx; i++)
        {
           const Tile &current = tiles[(j*nx + i)];
           iterateFaces(current.getPointIds(), 0, 0, reverseAllowed && true, BoundaryBack, body);
        }
    }
    if(flags[BoundaryFront])
    {
        for(conduit::index_t j = 0; j < ny; j++)
        for(conduit::index_t i = 0; i < nx; i++)
        {
           const Tile &current = tiles[(j*nx + i)];
           iterateFaces(current.getPointIds(), nz * nPtsPerPlane, 0, false, BoundaryFront, body);
        }
    }
}

//---------------------------------------------------------------------------
conduit::index_t
Tiler::domainIndex(conduit::index_t d0, conduit::index_t d1, conduit::index_t d2,
                    const std::vector<conduit::index_t> &domain,
                    const std::vector<conduit::index_t> &domains) const
{
    const conduit::index_t dom[3] = {domain[0] + d0, domain[1] + d1, domain[2] + d2};
    // If the domain exists, make its domain id.
    conduit::index_t domainId = InvalidDomain;
    if((dom[0] >= 0 && dom[0] < domains[0]) &&
       (dom[1] >= 0 && dom[1] < domains[1]) &&
       (dom[2] >= 0 && dom[2] < domains[2]))
    {
        domainId = dom[2] * (domains[0] * domains[1]) + dom[1] * domains[0] + dom[0];
    }
    return domainId;
}

//---------------------------------------------------------------------------
conduit::index_t
Tiler::domainIndex(const conduit::index_t delta[3],
                    const std::vector<conduit::index_t> &domain,
                    const std::vector<conduit::index_t> &domains) const
{
    return domainIndex(delta[0], delta[1], delta[2], domain, domains);
}

//---------------------------------------------------------------------------
void
Tiler::addAdjsetValues(const std::vector<conduit::index_t> &ptids,
                       conduit::index_t planeOffset,
                       bool reorder,
                       const std::vector<conduit::index_t> &old2NewPoint,
                       conduit::Node &values) const
{
    if(reorder)
    {
        values.set(conduit::DataType::index_t(ptids.size()));
        conduit::index_t *dest = values.as_index_t_ptr();
        if(planeOffset > 0)
        {
            for(size_t i = 0; i < ptids.size(); i++)
                *dest++ = old2NewPoint[ptids[i] + planeOffset];
        }
        else
        {
            for(size_t i = 0; i < ptids.size(); i++)
                *dest++ = old2NewPoint[ptids[i]];
        }
    }
    else if(planeOffset > 0)
    {
        values.set(conduit::DataType::index_t(ptids.size()));
        conduit::index_t *dest = values.as_index_t_ptr();
        for(size_t i = 0; i < ptids.size(); i++)
            *dest++ = ptids[i] + planeOffset;
    }
    else
    {
        values.set(ptids);
    }
}

//---------------------------------------------------------------------------
void
Tiler::addAdjset(const std::vector<Tile> &tiles,
                 conduit::index_t nx,
                 conduit::index_t ny,
                 conduit::index_t nz,
                 conduit::index_t ptsPerPlane,
                 bool reorder,
                 const std::vector<conduit::index_t> &old2NewPoint,
                 const conduit::Node &options,
                 conduit::Node &out) const
{
    // Build up unique points, given a list of points. Use unique and ptvec by capture.
    std::set<conduit::index_t> unique;
    std::vector<conduit::index_t> ptvec;
    auto addPoints = [&](const conduit::index_t *ids, conduit::index_t npts, int /*bnd*/)
    {
        for(conduit::index_t i = 0; i < npts; i++)
        {
            const auto id = ids[i];
            if(unique.find(id) == unique.end())
            {
                unique.insert(id);
                ptvec.push_back(id);
            }
        }
    };

    // We need to know where this domain is in the domains to make the adjset.
    if(options.has_child("domain") && options.has_child("domains"))
    {
        auto domain = toIndexVector(options.fetch_existing("domain"));
        auto domains = toIndexVector(options.fetch_existing("domains"));
        if(domain.size() == 3 && domain.size() == domains.size())
        {
            if(domains[0] * domains[1] * domains[2] > 1)
            {
                auto thisDom = domainIndex(0, 0, 0, domain, domains);

                // Make a state node.
                out["state/domain_id"] = thisDom;

                // Make the top level adjset nodes.
                conduit::Node &adjset = out["adjsets/" + meshName + "_adjset"];
                adjset["association"] = "vertex";
                adjset["topology"] = meshName;
                conduit::Node &groups = adjset["groups"];

                // Trace around the tile to get the points for the edges of the tile
                // in original node order.
                //-----------------------------------------------------------
                std::vector<conduit::index_t> left, right, bottom, top;
                const bool left_flags[] = {true, false, false, false};
                iterateBoundary2D(tiles, nx, ny, left_flags, addPoints);
                left.swap(ptvec); // Steal the points.
                unique.clear();
                const bool right_flags[] = {false, true, false, false};
                iterateBoundary2D(tiles, nx, ny, right_flags, addPoints);
                right.swap(ptvec); // Steal the points.
                unique.clear();
                const bool bottom_flags[] = {false, false, true, false};
                iterateBoundary2D(tiles, nx, ny, bottom_flags, addPoints);
                bottom.swap(ptvec); // Steal the points.
                unique.clear();
                const bool top_flags[] = {false, false, false, true};
                iterateBoundary2D(tiles, nx, ny, top_flags, addPoints);
                top.swap(ptvec); // Steal the points.
                unique.clear();

                // Make corner neighbors.
                //-----------------------------------------------------------
                conduit::index_t frontPlane = ptsPerPlane * nz;
                conduit::index_t corner[8];
                corner[0] = left[0];                 // back, lower left
                corner[1] = right[0];                // back, lower right
                corner[2] = left[left.size() - 1];   // back, upper left
                corner[3] = right[right.size() - 1]; // back, upper right
                corner[4] = left[0] + frontPlane;     // front, lower left
                corner[5] = right[0] + frontPlane;    // front, lower right
                corner[6] = left[left.size() - 1] + frontPlane;   // front, upper left
                corner[7] = right[right.size() - 1] + frontPlane; // front, upper right
                int maxCornerNeighbors = (nz < 1) ? 4 : 8;
                conduit::index_t z0 = (nz < 1) ? 0 : -1;
                conduit::index_t z1 = (nz < 1) ? 0 : 1;
                conduit::index_t neighborId = InvalidDomain;
                const conduit::index_t cornerNeighbors[][3] = {
                    {-1, -1, z0},
                    {1, -1, z0},
                    {-1, 1, z0},
                    {1, 1, z0},
                    {-1, -1, z1},
                    {1, -1, z1},
                    {-1, 1, z1},
                    {1, 1, z1}
                };
                for(int ni = 0; ni < maxCornerNeighbors; ni++)
                {
                    if((neighborId = domainIndex(cornerNeighbors[ni], domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        group["values"] = reorder ? old2NewPoint[corner[ni]] : corner[ni];
                    }    
                }

                // Make edge neighbors for 3D
                //-----------------------------------------------------------
                if(nz > 0)
                {
                    // Back, left edge.
                    if((neighborId = domainIndex(-1, 0, -1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(left, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Back, right edge.
                    if((neighborId = domainIndex(1, 0, -1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(right, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Back, bottom edge.
                    if((neighborId = domainIndex(0, -1, -1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(bottom, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Back, top edge.
                    if((neighborId = domainIndex(0, 1, -1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(top, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Lower left edge
                    if((neighborId = domainIndex(-1, -1, 0, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        std::vector<conduit::index_t> values;
                        values.reserve(nz);
                        for(conduit::index_t zi = 0; zi <= nz; zi++)
                            values.push_back(corner[0] + zi * ptsPerPlane);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(values, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Lower right edge
                    if((neighborId = domainIndex(1, -1, 0, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        std::vector<conduit::index_t> values;
                        values.reserve(nz);
                        for(conduit::index_t zi = 0; zi <= nz; zi++)
                            values.push_back(corner[1] + zi * ptsPerPlane);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(values, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Upper left edge
                    if((neighborId = domainIndex(-1, 1, 0, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        std::vector<conduit::index_t> values;
                        values.reserve(nz);
                        for(conduit::index_t zi = 0; zi <= nz; zi++)
                            values.push_back(corner[2] + zi * ptsPerPlane);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(values, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Upper right edge
                    if((neighborId = domainIndex(1, 1, 0, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        std::vector<conduit::index_t> values;
                        values.reserve(nz);
                        for(conduit::index_t zi = 0; zi <= nz; zi++)
                            values.push_back(corner[3] + zi * ptsPerPlane);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(values, 0, reorder, old2NewPoint, group["values"]);
                    }
                    // Front, left edge.
                    if((neighborId = domainIndex(-1, 0, 1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(left, frontPlane, reorder, old2NewPoint, group["values"]);
                    }
                    // Front, right edge.
                    if((neighborId = domainIndex(1, 0, 1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(right, frontPlane, reorder, old2NewPoint, group["values"]);
                    }
                    // Front, bottom edge.
                    if((neighborId = domainIndex(0, -1, 1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(bottom, frontPlane, reorder, old2NewPoint, group["values"]);
                    }
                    // Front, top edge.
                    if((neighborId = domainIndex(0, 1, 1, domain, domains)) != InvalidDomain)
                    {
                        auto name = adjset_name(thisDom, neighborId);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighborId;
                        addAdjsetValues(top, frontPlane, reorder, old2NewPoint, group["values"]);
                    }
                }

                // Make "face" neighbors.
                //-----------------------------------------------------------
                conduit::index_t neighbor[6];
                neighbor[BoundaryLeft]   = domainIndex(-1, 0, 0, domain, domains);
                neighbor[BoundaryRight]  = domainIndex(1, 0, 0, domain, domains);
                neighbor[BoundaryBottom] = domainIndex(0, -1, 0, domain, domains);
                neighbor[BoundaryTop]    = domainIndex(0, 1, 0, domain, domains);
                neighbor[BoundaryBack]   = domainIndex(0, 0, -1, domain, domains);
                neighbor[BoundaryFront]  = domainIndex(0, 0, 1, domain, domains);
                int maxNeighbors = (nz < 1) ? 4 : 6;
                // NOTE: We call iterateBoundary3D with reverseAllowed == false
                // so we get the same point order for front/back faces.
                const bool reverseAllowed = false;
                for(int ni = 0; ni < maxNeighbors; ni++)
                {
                    // If this domain has no neighbor in the current direction, skip.
                    if(neighbor[ni] == InvalidDomain)
                         continue;

                    // Iterate over faces and come up with unique points.
                    bool flags[6] = {false, false, false, false, false, false};
                    flags[ni] = true;
                    unique.clear();
                    ptvec.clear();
                    if(nz < 1)
                        iterateBoundary2D(tiles, nx, ny, flags, addPoints);
                    else
                    {
                        if(ni == BoundaryRight || ni == BoundaryTop)
                        {
                            // The point ids are geared towards making good boundary
                            // faces. We want the order of points for right to match
                            // left and for top to match bottom. We need to reorder.
                            iterateBoundary3D(tiles, nx, ny, nz, ptsPerPlane, flags, reverseAllowed,
                                [&](const conduit::index_t *ids, conduit::index_t npts, int bnd)
                                {
                                    conduit::index_t rids[4];
                                    rids[0] = ids[0];
                                    rids[1] = ids[3];
                                    rids[2] = ids[2];
                                    rids[3] = ids[1];
                                    addPoints(rids, npts, bnd);
                                });
                        }
                        else
                        {
                            iterateBoundary3D(tiles, nx, ny, nz, ptsPerPlane, flags, reverseAllowed, addPoints);
                        }
                    }
                    if(!ptvec.empty())
                    {
                        auto name = adjset_name(thisDom, neighbor[ni]);
                        conduit::Node &group = groups[name];
                        group["neighbors"] = neighbor[ni];
                        addAdjsetValues(ptvec, 0, reorder, old2NewPoint, group["values"]);
                    }
                } // end for
            }
        }
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
typedef conduit::index_t IndexType;
typedef std::array<IndexType, 3> LogicalIndex;

std::ostream &
operator << (std::ostream &os, const LogicalIndex &obj)
{
    os << "{" << obj[0] << ", " << obj[1] << ", " << obj[2] << "}";
    return os;
}

//---------------------------------------------------------------------------
template <typename T>
std::ostream &operator << (std::ostream &os, const std::vector<T> &obj)
{
    os << "{";
    for(const auto &v : obj)
       os << v << ", ";
    os << "}";
    return os;
};

//---------------------------------------------------------------------------

/**
 @brief This class represents a cartesian Block, though it can be subset using
        the image member.
 */
struct Block
{
    static const IndexType Empty;
    static const IndexType Self;
    static const IndexType Neighbor;
    static const IndexType InvalidDomainId;

    LogicalIndex           start{{0,0,0}};
    LogicalIndex           end{{0,0,0}};
    IndexType              hilbertBlockId{InvalidDomainId};
    std::vector<IndexType> image{};

    /**
     @brief Return the number of total zones spanned by the mesh start,end.
            For "complex" blocks, which use the image to identify the actual
            zones, size can be larger than the actual number of zones.

     @return The size.
    */
    IndexType size() const;

    /**
     @brief Return the length of the specified dimension.
     @param dimension A dimension in (0,1,2).
     @return The length of the block in the requested dimension.
     */
    IndexType length(IndexType dimension) const;

    /**
     @brief Returns the dimension index of the longest dimension.
     @return The index of the longest dimension.
     */
    IndexType longest() const;

    /**
     @brief Returns whether the block is "complex" (has non-empty image)
     @return True of the block is complex; False otherwise.
     */
    bool complex() const;

    /**
     @brief Turn an index to a logical index.
     @param index An index in [0,nx*ny*nz)
     @return A logical index that matches the input index.
     */
    LogicalIndex IndexToIJK(IndexType index) const;

    /**
     @brief Turn a local logical index into a local index.
     @param ijk The local logical index with values all within start,end.
     @return The local index that matches the logical index.
     */
    IndexType IJKToIndex(const LogicalIndex &ijk) const;

    /**
     @brief Expand the Block by some number of layers and return the expanded Block.
     @param n The number of layers to expand (for non-complex blocks)
     @param threeD Whether the block is allowed to expand in Z.
     @return A new expanded block.
     */
    Block expand(IndexType n, bool threeD) const;

    /**
     @brief Returns the actual number of zones in the block, taking into account
            empty zones if the block is complex.
     @return The actual number of zones.
     */
    IndexType numZones() const;

    /**
     @brief Determines whether the logical index is contained in the block.
     @param index The logical index being checked.
     @return True if index is contained in block; False otherwise.
     */
    bool contains(const LogicalIndex &index) const;

    /**
     @brief Iterate over all zones in the Block and apply a function that takes
            the logical id and the zone type.
     @param func A function/lambda that operates on a zone given by a logical index.
     */
    template <typename Func>
    void iterate(Func &&func) const;

    /**
     @brief Intersects the zones in block B with those in this block and returns a
            vector of logical indices for the intersected region.

     @note A vector of logical indices is used since one or more blocks might be
           complex.

     @param B The block we're checking against this block.
     @return A vector of logical indices that make up the intersection region.
     */
    std::vector<LogicalIndex> intersect(const Block &B) const;
private:
    /**
     @brief Intersects the zones in block B with those in this block and returns a
            vector of logical indices for the intersected region.

     @note A vector of logical indices is used since one or more blocks might be
           complex.

     @param B The block we're checking against this block.
     @return A vector of logical indices that make up the intersection region.
     */
    std::vector<LogicalIndex> intersectInternal(const Block &B) const;
};

const IndexType Block::Empty = -1;
const IndexType Block::Self = -2;
const IndexType Block::Neighbor = -3;
const IndexType Block::InvalidDomainId = -1;

//---------------------------------------------------------------------------
std::ostream &operator << (std::ostream &os, const Block &obj)
{
    os << "{start=" << obj.start
       << ", end=" << obj.end
       << ", size=" << obj.size()
       << ", length={" << obj.length(0) << ", " << obj.length(1) << ", " << obj.length(2) << "}"
       << ", hilbertBlockId=" << obj.hilbertBlockId
       << ", image.size=" << obj.image.size()
       << ", numZones=" << obj.numZones()
       << "}";
    return os;
};

//---------------------------------------------------------------------------
IndexType Block::size() const
{
    IndexType nx = end[0] - start[0] + 1;
    IndexType ny = end[1] - start[1] + 1;
    IndexType nz = end[2] - start[2] + 1;
    return nx * ny * nz;
}

//---------------------------------------------------------------------------
IndexType Block::length(IndexType dimension) const
{
    IndexType nx = end[0] - start[0] + 1;
    IndexType ny = end[1] - start[1] + 1;
    IndexType nz = end[2] - start[2] + 1;
    return (dimension == 2) ? nz : ((dimension == 1) ? ny : nx);
}

//---------------------------------------------------------------------------
IndexType Block::longest() const
{
    IndexType L[3] = {end[0] - start[0] + 1,
                      end[1] - start[1] + 1,
                      end[2] - start[2] + 1};
    IndexType dim = 0;
    if(L[1] > L[dim])
        dim = 1;
    if(L[2] > L[dim])
        dim = 2;
    return dim;
}

//---------------------------------------------------------------------------
bool Block::complex() const
{
    return !image.empty();
}

//---------------------------------------------------------------------------
LogicalIndex Block::IndexToIJK(IndexType index) const
{
    auto nx = length(0);
    auto ny = length(1);
    auto nxny = nx * ny;

    auto i = index % nx;
    auto j = (index % (nxny)) / nx;
    auto k = index / (nxny);

    return LogicalIndex{{i, j, k}};
}

//---------------------------------------------------------------------------
IndexType
Block::IJKToIndex(const LogicalIndex &ijk) const
{
    auto nx = length(0);
    auto ny = length(1);
    return ijk[2] * nx * ny + ijk[1] * nx + ijk[0];
}

//---------------------------------------------------------------------------
Block Block::expand(IndexType n, bool threeD) const
{
    int ndims = threeD ? 3 : 2;
    Block e;
    e.start = start;
    e.end = end;
    for(int d = 0; d < ndims; d++)
    {
        e.start[d] -= n;
    }
    for(int d = 0; d < ndims; d++)
    {
        e.end[d] += n;
    }

    if(complex())
    {
        e.image.resize(e.size(), Empty);
        iterate([&](const LogicalIndex &index, IndexType zonetype) {
            auto logicalIndex = LogicalIndex{{index[0] - start[0] + n,
                                              index[1] - start[1] + n,
                                              index[2] - start[2] + (threeD ? n : 0)}};
            auto dest = e.IJKToIndex(logicalIndex);
            e.image[dest] = zonetype;
        });
    }

    return e;
}

//---------------------------------------------------------------------------
IndexType Block::numZones() const
{
    IndexType retval;
    if(complex())
    {
        retval = 0;
        for(const auto &z : image)
            retval += (z == Self) ? 1 : 0;
    }
    else
    {
        retval = size();
    }
    return retval;
}

//---------------------------------------------------------------------------
inline bool interval_contains(IndexType start, IndexType end, IndexType index)
{
    return index >= start && index <= end;
}

//---------------------------------------------------------------------------
bool Block::contains(const LogicalIndex &index) const
{
    bool inside = interval_contains(start[0], end[0], index[0]) && 
                  interval_contains(start[1], end[1], index[1]) && 
                  interval_contains(start[2], end[2], index[2]);
    return inside;
}

//---------------------------------------------------------------------------
template <typename Func>
void Block::iterate(Func &&func) const
{
    if(complex())
    {
        IndexType ii = 0;
        for(IndexType k = start[2]; k <= end[2]; k++)
        for(IndexType j = start[1]; j <= end[1]; j++)
        for(IndexType i = start[0]; i <= end[0]; i++, ii++)
        {
            func(LogicalIndex{{i,j,k}}, image[ii]);
        }
    }
    else
    {
        for(IndexType k = start[2]; k <= end[2]; k++)
        for(IndexType j = start[1]; j <= end[1]; j++)
        for(IndexType i = start[0]; i <= end[0]; i++)
        {
            func(LogicalIndex{{i,j,k}}, Self);
        }
    }
}

//---------------------------------------------------------------------------
std::vector<LogicalIndex>
Block::intersectInternal(const Block &B) const
{
    std::vector<LogicalIndex> ids;

    // Check whether ranges overlap.
    Block clip;
    int overlaps = 0;
    for(int i = 0; i < 3; i++)
    {
        if(interval_contains(start[i], end[i], B.start[i]) ||
           interval_contains(start[i], end[i], B.end[i]) ||
           interval_contains(B.start[i], B.end[i], start[i]) ||
           interval_contains(B.start[i], B.end[i], end[i]))
        {
            clip.start[i] = std::max(start[i], B.start[i]);
            clip.end[i] = std::min(end[i], B.end[i]);
            overlaps++;
        }
    }

    // If there was overlap in 3 dimensions then there is intersection.
    if(overlaps == 3)
    {
        // If we had a hit but one or more blocks was complex then we
        // need to do zone-based intersection.
        if(complex() && B.complex())
        {
            clip.iterate([&](const LogicalIndex &index, IndexType) {
                LogicalIndex aindex{{index[0] - start[0],
                                     index[1] - start[1],
                                     index[2] - start[2]}};
                auto ai = IJKToIndex(aindex);
                LogicalIndex bindex{{index[0] - B.start[0],
                                     index[1] - B.start[1],
                                     index[2] - B.start[2]}};
                auto bi = B.IJKToIndex(bindex);
                if(image[ai] != Block::Empty && B.image[bi] != Block::Empty)
                    ids.push_back(index);
            });
        }
        else if(complex()) // B is not complex.
        {
            clip.iterate([&](const LogicalIndex &index, IndexType) {
                LogicalIndex aindex{{index[0] - start[0],
                                     index[1] - start[1],
                                     index[2] - start[2]}};
                auto ai = IJKToIndex(aindex);
                if(image[ai] != Block::Empty)
                    ids.push_back(index);
            });
        }
        else if(B.complex()) // B is complex
        {
            clip.iterate([&](const LogicalIndex &index, IndexType) {
                LogicalIndex bindex{{index[0] - B.start[0],
                                     index[1] - B.start[1],
                                     index[2] - B.start[2]}};
                auto bi = B.IJKToIndex(bindex);
                if(B.image[bi] != Block::Empty)
                    ids.push_back(index);
            });
        }
        else // Neither is complex. clip is the intersection.
        {
            clip.iterate([&](const LogicalIndex &index, IndexType) {
                ids.push_back(index);
            });
        }
    }

    return ids;
}

//---------------------------------------------------------------------------
std::vector<LogicalIndex>
Block::intersect(const Block &B) const
{
    std::vector<LogicalIndex> ids = intersectInternal(B);
    if(ids.empty())
        ids = B.intersectInternal(*this);
    return ids;
}

//---------------------------------------------------------------------------
#ifdef CONDUIT_WRITE_BLOCKS
static void
BlockToBlueprintUnstructured(const Block &obj, conduit::Node &dom)
{
    // Make a uniform coordset and turn it into explicit.
    conduit::Node cs;
    cs["type"] = "uniform";
    cs["dims/i"] = obj.length(0) + 1;
    cs["dims/j"] = obj.length(1) + 1;
    cs["dims/k"] = obj.length(2) + 1;
    cs["origin/x"] = obj.start[0];
    cs["origin/y"] = obj.start[1];
    cs["origin/z"] = obj.start[2];
    conduit::blueprint::mesh::coordset::to_explicit(cs, dom["coordsets/coords"]);

    Block ptBlock;
    ptBlock.start = LogicalIndex{0,0,0};
    ptBlock.end = LogicalIndex{obj.length(0),obj.length(1),obj.length(2)};

    std::vector<IndexType> conn, sizes, offsets, image;
    obj.iterate([&](const LogicalIndex &idx, IndexType type)
    {
        static const IndexType dx[] = {0, 1, 1, 0, 0, 1, 1, 0};
        static const IndexType dy[] = {0, 0, 1, 1, 0, 0, 1, 1};
        static const IndexType dz[] = {0, 0, 0, 0, 1, 1, 1, 1};
        if(type != Block::Empty)
        {
            LogicalIndex origin = LogicalIndex{idx[0] - obj.start[0],
                                               idx[1] - obj.start[1],
                                               idx[2] - obj.start[2]};
            for(int corner = 0; corner < 8; corner++)
            {
                LogicalIndex pt(origin);
                pt[0] += dx[corner];
                pt[1] += dy[corner];
                pt[2] += dz[corner];

                auto ptid = ptBlock.IJKToIndex(pt);
                conn.push_back(ptid);
            }
            offsets.push_back(sizes.size() * 8);
            sizes.push_back((sizes.size() + 1) * 8);
            image.push_back(type);
        }
    });

    dom["topologies/topo/coordset"] = "coords";
    dom["topologies/topo/type"] = "unstructured";
    dom["topologies/topo/elements/shape"] = "hex";
    dom["topologies/topo/elements/connectivity"].set(conn);
    dom["topologies/topo/elements/sizes"].set(sizes);
    dom["topologies/topo/elements/offsets"].set(offsets);

    // We always have an image here.
    dom["fields/image/association"] = "element";
    dom["fields/image/topology"] = "topo";
    dom["fields/image/values"].set(image);
}

//------------------------------------------------------------------------------
static void
BlockToBlueprint(const std::vector<Block> &blocks, conduit::Node &n)
{
    // Determine whether any blocks are complex.
    bool anyComplex = false;
    for(const auto &b : blocks)
        anyComplex |= b.complex();

    int blockId = 0;
    for(const auto &b : blocks)
    {
        conduit::Node &dom = n.append();
 
        if(b.complex())
        {
            BlockToBlueprintUnstructured(b, dom);
        }
        else
        {
            // Make a simple Conduit mesh.
            dom["coordsets/coords/type"] = "uniform";
            dom["coordsets/coords/dims/i"] = b.length(0) + 1;
            dom["coordsets/coords/dims/j"] = b.length(1) + 1;
            dom["coordsets/coords/dims/k"] = b.length(2) + 1;

            dom["coordsets/coords/origin/x"] = b.start[0];
            dom["coordsets/coords/origin/y"] = b.start[1];
            dom["coordsets/coords/origin/z"] = b.start[2];

            dom["topologies/topo/coordset"] = "coords";
            dom["topologies/topo/type"] = "uniform";

            if(anyComplex)
            {
                std::vector<IndexType> image(b.size(), Block::Self);
                dom["fields/image/association"] = "element";
                dom["fields/image/topology"] = "topo";
                dom["fields/image/values"].set(image);
            }
        }

        dom["state/domain_id"] = blockId++;
    }
}
#endif

//---------------------------------------------------------------------------
/**
 @brief This class splits a block into N smaller blocks.
 */
class BlockSplitter
{
public:
    struct Options
    {
        bool curveSplitting{false};         // Whether curve splitting is enabled.
        IndexType maximumSize{100*100*100}; // The max size a domain can be to be considered for curve splitting.
    };

    /**
     @brief Split the input whole block into nblocks smaller blocks.
     */
    std::vector<Block> split(const Block &whole, IndexType nblocks) const;

public:
    Options options;
private:
    /**
     @brief Split whole into 2 blocks a,b such that we split along a dimension
            that yields a split proportional to the sizes of a, b.

     @param whole The block to split.
     @param a The number of blocks to make from the "left" split.
     @param b The number of blocks to make from the "right" split.
     @return A vector of new blocks.
     */
    std::vector<Block> split2(const Block &whole, IndexType a, IndexType b) const;

    /**
     @brief Split whole into numBlocks blocks by replicating the input whole
            block and marking the output blocks as blocks that need to be split
            together later using a hilbert curve.

     @param whole The block to split.
     @param numBlocks The number of blocks to make from whole.
     @return A vector of new blocks.
     */
    std::vector<Block> split_hilbert(const Block &whole, IndexType numBlocks) const;

};

//---------------------------------------------------------------------------
std::vector<Block>
BlockSplitter::split2(const Block &whole, IndexType a, IndexType b) const
{
    std::vector<Block> blocks;

    double blockRatio = static_cast<double>(a) / static_cast<double>(b);
    auto d = whole.longest();

    // Split whole along the longest dimension such that the volumes on
    // both sides are proportional.
    Block A, B;
    A.start = whole.start;
    A.end = whole.end;
    B.start = whole.start;
    B.end = whole.end;
    double bestDiff = std::numeric_limits<double>::max();
    IndexType bestS = whole.start[d];
    for(IndexType s = whole.start[d]; s < whole.end[d] - 1; s++)
    {
        A.end[d] = s;
        B.start[d] = A.end[d] + 1;

        double volRatio = static_cast<double>(A.size()) / static_cast<double>(B.size());
        double currDiff = fabs(volRatio - blockRatio);

        if(currDiff < bestDiff)
        {
            bestDiff = currDiff;
            bestS = s;
        }
    }
    A.end[d] = bestS;
    B.start[d] = A.end[d] + 1;

#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
    std::cout << "  blockRatio: " << blockRatio << std::endl;
    std::cout << "  d: " << d << std::endl;
    std::cout << "  a: " << a << std::endl;
    std::cout << "  b: " << b << std::endl;
    std::cout << "  bestS:" << bestS << std::endl;
    std::cout << "  A:" << A << std::endl;
    std::cout << "  B:" << B << std::endl;
#endif

    // Deal with cases where we ask for more domains than there are zones.
    if(a > A.size())
    {
        auto n = a + b;
        a = A.size();
        b = n - a;
    }
    else if(b > B.size())
    {
        auto n = a + b;
        b = B.size();
        a = n - b;
    }

    if(a == 1)
        blocks.push_back(A);
    else
    {
        auto ablocks = split(A, a);
        for(const auto &blk : ablocks)
            blocks.push_back(blk);
    }
    if(b == 1)
        blocks.push_back(B);
    else
    {
        auto bblocks = split(B, b);
        for(const auto &blk : bblocks)
            blocks.push_back(blk);
    }

    return blocks;
}

//---------------------------------------------------------------------------
std::vector<Block> BlockSplitter::split_hilbert(const Block &whole, IndexType numblocks) const
{
    static IndexType hilbertBlockId = 1;
#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
    std::cout << "  hilbert:\n";
#endif
    std::vector<Block> blocks;
    // Repeat the whole Block numblocks times but give it a valid hilbertBlockId to
    // indicate that it will be split later into a real Block.
    for(IndexType i = 0; i < numblocks; i++)
    {
        Block b(whole);                    // All these blocks cover the same space initially.
        b.hilbertBlockId = hilbertBlockId; // Supply a valid id so we know which blocks are related.
#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
        std::cout << "    " << b << std::endl;
#endif
        blocks.push_back(b);
    }

    hilbertBlockId++;
    return blocks;
}

//---------------------------------------------------------------------------
std::vector<Block> BlockSplitter::split(const Block &whole, IndexType nblocks) const
{
    std::vector<Block> blocks;
    const IndexType ndims = 3;

    auto large_factors = [](const std::vector<IndexType> &f) -> bool
    {
        for(const auto &a : f)
        {
            if(a > 7)
                return true;
        }
        return false;
    };

    auto f = conduit::utils::factor(nblocks);

#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
    std::cout << "make_blocks:\n";
    std::cout << "  whole: " << whole << std::endl;
    std::cout << "  nblocks: " << nblocks << std::endl;
    std::cout << "  factors: " << f << std::endl;
#endif

    if(f.size() == 1 && f[0] == 1)
    {
        // Single Block
        blocks.push_back(whole);
    }
    else if(options.curveSplitting && whole.size() <= options.maximumSize)
    {
        // Hilbert curve splitting is enabled so do it.
        auto newblocks = split_hilbert(whole, nblocks);
        for(const auto &blk : newblocks)
            blocks.push_back(blk);
    }
    else if(f.size() == 1)
    {
        // nblocks is prime. Split it into 2 numbers.
        IndexType b = nblocks / 2;
        IndexType a = nblocks - b;
        auto newblocks = split2(whole, a, b);
        for(const auto &blk : newblocks)
            blocks.push_back(blk);
    }
    else if(f.size() >= 2 && large_factors(f))
    {
        // nblocks is not prime but its largest factor is quite a bit larger
        // than the next smallest factor. If we let it proceed into the next
        // if-Block then we could get some really thin domains. Here we can
        // reorganize the number.
        //
        // Example: nblocks = p * q
        //          nblocks = p * (r + s)  where q = r + s
        //          nblocks = p * r + p * s
        //          a = p * r
        //          b = p * s

        IndexType p = 1;
        for(size_t i = 0; i < f.size() - 1; i++)
            p *= f[i];
        IndexType q = f[f.size() - 1];
        IndexType s = q / 2;
        IndexType r = q - s;
        IndexType a = p * r;
        IndexType b = p * s;

        auto newblocks = split2(whole, a, b);
        for(const auto &blk : newblocks)
            blocks.push_back(blk);
    }
    else
    {
        // Determine the best arrangement for making blocks.
        IndexType divisions[] = {1, 1, 1};
        for (size_t i = 0; i < f.size(); i++)
        {
            auto currentFactor = f[f.size() - 1 - i];
            IndexType selectedDim = 0;

            // Favor giving the factor to the dimension that makes the shortest diagonal.
            double shortestDiag = std::numeric_limits<double>::max();
            for(int di = 0; di < ndims; di++)
            {
                auto side = whole.length(di);
                if(side > 1 && divisions[di] * currentFactor <= side)
                {
                    IndexType sum = 0;
                    for(IndexType c = 0; c < ndims; c++)
                    {
                        IndexType len = divisions[c];
                        if(c == di)
                            len *= currentFactor;
                        sum += len * len;
                    }

                    double diag = pow(sum, 1. / 3.);
                    if(diag < shortestDiag)
                    {
                        selectedDim = di;
                        shortestDiag = diag;
                    }
                }
            }

            divisions[selectedDim] *= currentFactor;
        }
#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
        std::cout << "  divisions: [" << divisions[0]
                  << ", " << divisions[1]
                  << ", " << divisions[2] << "]" << std::endl;
#endif
        if(divisions[0] > whole.length(0) ||
           divisions[1] > whole.length(1) ||
           divisions[2] > whole.length(2))
        {
            // If any of the divisions are larger than the side lengths that we
            // came up with divisions that can't work.
#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
            std::cout << "   !! Divisions do not work. Try a different partition." << std::endl;
#endif
            // Split nblocks into 2 numbers.
            IndexType b = nblocks / 2;
            IndexType a = nblocks - b;
            auto newblocks = split2(whole, a, b);
            for(const auto &blk : newblocks)
                blocks.push_back(blk);
        }
        else
        {
            // Figure out the sizes for each domain in I,J,K
            std::vector<IndexType> sizes[3];
            for(int d = 0; d < 3; d++)
            {
                IndexType zonesInWhole = whole.length(d);
                IndexType zonesPerDomain = zonesInWhole / divisions[d];
                IndexType zoneSum = 0;
                for(IndexType i = 0; i < divisions[d]; i++)
                {
                    sizes[d].push_back(zonesPerDomain);
                    zoneSum += zonesPerDomain;
                }
                // Offset the dimensions where they start increasing sizes so they
                // don't all increase at the start, making the first domain larger
                // than all others.
                IndexType offset = (d * (sizes[d].size() - 1)) / 3;
                for(IndexType i = 0; zoneSum < zonesInWhole; i++)
                {
                    sizes[d][(offset + i) % sizes[d].size()]++;
                    zoneSum++;
                }
            }
#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
            std::cout << "  sizes:" << std::endl;
            std::cout << "    x: " << sizes[0] << std::endl;
            std::cout << "    y: " << sizes[1] << std::endl;
            std::cout << "    z: " << sizes[2] << std::endl;
            std::cout << "  domains:" << std::endl;
#endif
            // Make the domains.
            Block dom;
            dom.start[2] = whole.start[2];
            for(const auto &ksize : sizes[2])
            {
                dom.end[2] = dom.start[2] + ksize - 1;
    
                dom.start[1] = whole.start[1];
                for(const auto &jsize : sizes[1])
                {
                    dom.end[1] = dom.start[1] + jsize - 1;
    
                    dom.start[0] = whole.start[0];
                    for(const auto &isize : sizes[0])
                    {
                        dom.end[0] = dom.start[0] + isize - 1;
#ifdef CONDUIT_DEBUG_BLOCK_SPLITTER
                        std::cout << "    -\n    " << dom << std::endl;
#endif
                        blocks.push_back(dom);
                        dom.start[0] = dom.end[0] + 1;
                    }
                    dom.start[1] = dom.end[1] + 1;
                }
                dom.start[2] = dom.end[2] + 1;
            }
        }
    }

    return blocks;
}

//------------------------------------------------------------------------------
/**
 @brief Accepts a list of logical indices within domain that identify specific
        zones that are selected. From that, we'll make a new Block that contains
        just those zones (by setting the image in the Block).

 @param zoneIds The vector of zone ids that are enabled in the domain. These are global indices.

 @note We'd get the zoneIds from the hilbert code.
 */
Block logicalIndicesToBlock(const std::vector<LogicalIndex> &zoneIds)
{
    // Figure out the bounding box of the zones that are selected in domain.
    // we'll figure it out relative to the whole.
    Block selected;
    selected.start = LogicalIndex{{std::numeric_limits<IndexType>::max(),
                                   std::numeric_limits<IndexType>::max(),
                                   std::numeric_limits<IndexType>::max()}};
    selected.end = LogicalIndex{{-std::numeric_limits<IndexType>::max(),
                                 -std::numeric_limits<IndexType>::max(),
                                 -std::numeric_limits<IndexType>::max()}};
    // Determine the selected extents in the domain.
    for(const auto &zid : zoneIds)
    {
        selected.start[0] = std::min(selected.start[0], zid[0]);
        selected.start[1] = std::min(selected.start[1], zid[1]);
        selected.start[2] = std::min(selected.start[2], zid[2]);

        selected.end[0] = std::max(selected.end[0], zid[0]);
        selected.end[1] = std::max(selected.end[1], zid[1]);
        selected.end[2] = std::max(selected.end[2], zid[2]);
    }

    // Make another selected Block to help with indexing.
    Block selectedLocal;
    selectedLocal.start = LogicalIndex{{0,0,0}};
    selectedLocal.end = LogicalIndex{{selected.length(0) - 1,
                                      selected.length(1) - 1,
                                      selected.length(2) - 1}};

    // Fill in the selected image.
    selected.image.resize(selected.size(), Block::Empty);
    for(const auto &zid : zoneIds)
    {
        // The IJK within the selected Block.
        auto local = LogicalIndex{{zid[0] - selected.start[0],
                                   zid[1] - selected.start[1],
                                   zid[2] - selected.start[2]}};

        // Make the local index in the selected region.
        auto localIndex = selectedLocal.IJKToIndex(local);

        // Mark the zone as self.
        selected.image[localIndex] = Block::Self;
    }

    return selected;
}

//------------------------------------------------------------------------------
/**
 @brief Iterate over a block in the style of a hilbert curve and divide the
        zones into a set of segments, each of which is aggregated and used to
        construct a new block that contains the subset.

 @param whole The block whose zones will be iterated in hilbert order.
 @param numSegments The number of segments to produce from the input block.
 @param func A function/lambda to apply to a vector of LogicalIndex that
             comprise the segment.
 */
template <typename Func>
void
hilbert_iterate(const Block &whole, IndexType numSegments, Func &&func)
{
    auto totalZones = whole.size();
    auto zonesPerSegment = totalZones / numSegments;
    IndexType zoneCount = 0, segmentId = 0;
    IndexType dims[3] = {whole.length(0), whole.length(1), whole.length(2)};

    std::vector<LogicalIndex> segmentIds;
    segmentIds.reserve(zonesPerSegment);

    auto lastSegmentId = numSegments - 1;

    if(dims[2] <= 1)
    {
        // Tweak zonesPerSegment the decomposition is a little better behaved.
        // zonesPerSegment = zonesPerSegment + (4 - (zonesPerSegment % 4));

        // Store zone ids in segmentIds and call func when we have enough.
        auto store2d = [&](IndexType i, IndexType j)
        {
            LogicalIndex zid{{i + whole.start[0], j + whole.start[1], 0}};
            segmentIds.push_back(zid);
            zoneCount++;

            if((segmentId < lastSegmentId && static_cast<IndexType>(segmentIds.size()) == zonesPerSegment) ||
               (segmentId == lastSegmentId && zoneCount == totalZones))
            {
                func(segmentId, segmentIds);
                segmentIds.clear();
                segmentId++;
            }
        };

        conduit::blueprint::curves::gilbert2d(dims[0], dims[1], store2d);

        // Process any leftover segments.
        if(!segmentIds.empty())
            func(segmentId, segmentIds);
    }
    else if(dims[2] > 1)
    {
        // Tweak zonesPerSegment the decomposition is a little better behaved.
        // This bakes in some inequality though since the last curve segment
        // will be shorter.
        // zonesPerSegment = zonesPerSegment + (8 - (zonesPerSegment % 8));

        // Store zone ids in segmentIds and call func when we have enough.
        auto store3d = [&](IndexType i, IndexType j, IndexType k)
        {
            LogicalIndex zid{{i + whole.start[0], j + whole.start[1], k + whole.start[2]}};
            segmentIds.push_back(zid);
            zoneCount++;

            if((segmentId < lastSegmentId && static_cast<IndexType>(segmentIds.size()) == zonesPerSegment) ||
               (segmentId == lastSegmentId && zoneCount == totalZones))
            {
                func(segmentId, segmentIds);
                segmentIds.clear();
                segmentId++;
            }
        };

        conduit::blueprint::curves::gilbert3d(dims[0], dims[1], dims[2], store3d);

        // Process any leftover segments.
        if(!segmentIds.empty())
            func(segmentId, segmentIds);
    }
}

//------------------------------------------------------------------------------
/**
 @brief This step looks through the Block and performs any hilbert iteration
        needed to further partition them using a hilbert curve.

 @param blocks         The vector of all blocks. Some will be updated here.
 @param selectedblocks An optional vector of selected Block ids. If the vector is not
                       empty then only these blocsk will be realized.
 */
void
realizeHilbertBlocks(std::vector<Block> &blocks,
    const std::vector<IndexType> &selectedblocks = std::vector<IndexType>())
{
    // Scan the blocks to make a map of hilbert ids to Block indices.
    std::map<IndexType, std::set<IndexType>> hilberts;
    std::set<IndexType> hkeys;
    for(size_t bi = 0; bi < blocks.size(); bi++)
    {
        const auto &b = blocks[bi];
        if(b.hilbertBlockId > Block::InvalidDomainId)
        {
            // If there are no selected blocks then we selected the hilbertBlockId.
            // Or, if there are selected blocks then we selected the hilbertBlockId
            // it contains one of the blocks that we do want.
            if(selectedblocks.empty() ||
               std::find(selectedblocks.begin(), selectedblocks.end(), bi) != selectedblocks.end())
            {
                hkeys.insert(b.hilbertBlockId);
            }

            hilberts[b.hilbertBlockId].insert(bi);
        }
    }

    for(auto it = hilberts.begin(); it != hilberts.end(); it++)
    {
        // Check whether the curve is selected.
        if(hkeys.find(it->first) != hkeys.end())
        {
            auto block_it = it->second.begin();
            size_t nblocks = it->second.size();
            // Make a copy of the original Block since we're going to replace the blocks.
            Block b = blocks[*block_it];

            // Iterate through the Block with a hilbert curve and each time we get
            // a new segment, make a new Block that records the 
            hilbert_iterate(b, nblocks,
                [&](IndexType, const std::vector<LogicalIndex> &segmentZones)
            {
                auto bi = *block_it;
                blocks[bi] = logicalIndicesToBlock(segmentZones);

                block_it++;
            });
        }
    }
}

//------------------------------------------------------------------------------
/**
 @brief Scans through the Block's image and makes a layer of possible neighbor
        zones around the zones that are part of the domain. The Block should be
        expanded by a layer before calling this function.

 @param obj The Block
 @param n The number of neighbor layers
 @param threeD Whether we're allowing 3D
 */
void highlightNeighborZones(Block &obj, IndexType n, bool threeD)
{
    IndexType nz = 1, dkmin = 0, dkmax = 0;
    IndexType ny = obj.length(1);
    IndexType nx = obj.length(0);

    if(threeD)
    {
        nz = obj.length(2);
        dkmin = -1;
        dkmax = 1;
    }

    if(obj.complex())
    {
        for(IndexType k = 0; k < nz; k++)
        for(IndexType j = 0; j < ny; j++)
        for(IndexType i = 0; i < nx; i++)
        {
            // Point cp is the current i,j,k point.
            IndexType cp = obj.IJKToIndex(LogicalIndex{{i, j, k}});
            // If the point is part of the domain then activate the neighbor zones around it.
            if(obj.image[cp] == Block::Self)
            {
                // Look at the neighboring zones and make them neighbors if they are empty.
                for(IndexType dk = dkmin; dk <= dkmax; dk++)
                for(IndexType dj = -1; dj <= 1; dj++)
                for(IndexType di = -1; di <= 1; di++)
                {
                    auto ii = i + di;
                    auto jj = j + dj;
                    auto kk = k + dk;

                    if(ii >= 0 && ii <= nx &&
                       jj >= 0 && jj <= ny &&
                       kk >= 0 && kk <= nz)
                    {
                        auto np = obj.IJKToIndex(LogicalIndex{{ii, jj, kk}});
                        if(obj.image[np] == Block::Empty)
                        {
                            obj.image[np] = Block::Neighbor;
                        }
                    }
                }
            }
        }
    }
    else
    {
        // We need to blow it up anyway.
        obj.image.resize(obj.size(), Block::Self);
        for(IndexType k = 0; k < nz; k++)
        for(IndexType j = 0; j < ny; j++)
        for(IndexType i = 0; i < nx; i++)
        {
            bool end = (threeD && (k < n || k == nz-n)) || (j < n || j == ny-n) || (i < n || i == nx-n);
            if(end)
            {
                auto index = obj.IJKToIndex(LogicalIndex{{i,j,k}});
                obj.image[index] = Block::Neighbor;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
 @brief Return a Block whose image has neighbors painted into it.
 */
Block neighbors(const std::vector<Block> &blocks, size_t blockId, bool threeD)
{
    const int neighborLayers = 1;

    // Expand the selected Block by a layer, also creating its image.
    Block selectedBlock = blocks[blockId].expand(neighborLayers, threeD);

    // Iterate over the zones in the selected Block and highlight the zones that
    // should count as neighbor zones. This handles non-brick-like blocks.
    highlightNeighborZones(selectedBlock, neighborLayers, threeD);

    for(size_t bi = 0; bi < blocks.size(); bi++)
    {
        if(bi != blockId)
        {
            // Get a list of IJK indices in global coordinates that are the intersecting zones.
            auto zids = selectedBlock.intersect(blocks[bi]);

            // If there were intersections, paint the Block index into the neighbor slots.
            for(const auto &zid : zids)
            {
                auto local = LogicalIndex{{zid[0] - selectedBlock.start[0],
                                           zid[1] - selectedBlock.start[1],
                                           zid[2] - selectedBlock.start[2]}};
                auto index = selectedBlock.IJKToIndex(local);
                if(selectedBlock.image[index] == Block::Neighbor)
                    selectedBlock.image[index] = static_cast<IndexType>(bi);
            }
        }
    }

    return selectedBlock;
}

//---------------------------------------------------------------------------
struct matrix4x4
{
     double M[4][4]{{1.,0.,0.,0.},{0.,1.,0.,0.},{0.,0.,1.,0.},{0.,0.,0.,1.}};
};

matrix4x4 translate(const double t[3])
{
    matrix4x4 mat;
    mat.M[3][0] = t[0];
    mat.M[3][1] = t[1];
    mat.M[3][2] = t[2];
    return mat;
}

matrix4x4 scale(const double s[3])
{
    matrix4x4 mat;
    mat.M[0][0] = s[0];
    mat.M[1][1] = s[1];
    mat.M[2][2] = s[2];
    mat.M[3][3] = 1.;
    return mat;
}

matrix4x4 operator *(const matrix4x4 &A, const matrix4x4 &B)
{
    matrix4x4 mat;
    for(int r = 0; r < 4; r++)
    {
        for(int c = 0; c < 4; c++)
        {
            double sum = 0.;
            for(int i = 0; i < 4; i++)
            {
                sum += A.M[r][i] * B.M[i][c];
            }
            mat.M[r][c] = sum;
        }
    }
    return mat;
}

void vec_matrix4x4_mult(const double A[3], const matrix4x4 &B, double C[3])
{
    // Treat A as (A.x, A.y, A.x, 1.)
    C[0] = A[0] * B.M[0][0] + A[1] * B.M[1][0] + A[2] * B.M[2][0] + B.M[3][0];
    C[1] = A[0] * B.M[0][1] + A[1] * B.M[1][1] + A[2] * B.M[2][1] + B.M[3][1];
    C[2] = A[0] * B.M[0][2] + A[1] * B.M[1][2] + A[2] * B.M[2][2] + B.M[3][2];
    double h = A[0] * B.M[0][3] + A[1] * B.M[1][3] + A[2] * B.M[2][3] + B.M[3][3];
    C[0] /= h;
    C[1] /= h;
    C[2] /= h;
}

//---------------------------------------------------------------------------
/**
 \brief This class accepts the overall mesh size for the whole problem and
        divides it up to make N domains.
 */
class TopDownTiler : public TilerBase
{
public:
    TopDownTiler();

    /**
     @brief Generates new domain(s) for an overall mesh {nx,ny,nz} zones, split
            a set of domains and stored in the res node.

     @param nx The number of tiles in X.
     @param ny The number of tiles in Y.
     @param nz The number of tiles in Z.
     @param[out] res The Conduit node that will contain the new domain(s).
     @param options A Conduit node containing options for generating the mesh.
     */
    void generate(conduit::index_t nx, conduit::index_t ny, conduit::index_t nz,
                  conduit::Node &res, const conduit::Node &options);
protected:
    /**
     @brief This class represents the various sets of node indices within a tile.
     */
    struct TileIndices
    {
        /**
                                                          top f3   / back f4
                c2                c3                      |       /
                *------e3---------*                 *-----|-----------*
               /|                /|                /|     |     /    /|
              / |               / |               / |     |    /    / |
            e9  |             e11 |              /  |     +   /    /  |
            /   e0            /   |             /   |        +    /   |
           /    |            /    e1           /    |            /    |
        c6*--------e7-------*c7   |           *-----------------*     |
          |     |           |     |           |     |           |     |
          |     |           |     |     left--|--+  |           |  +----- right
          |     *- - - e2- -|- - -* c1   f0   |     *- - - - - -|- - -*     f1
          e4   / c0         e5   /            |    /    +       |    /
          |   e8            |   e10           |   /    /        |   /
          |  /              |  /              |  /    /   +     |  /
          | /               | /               | /    /    |     | /
          |/                |/                |/    /     |     |/
          *--------e6-------*                 *----/-----------*
          c4                c5                    /       |
                                              front f5    bottom f2

         */
        std::vector<conduit::index_t> left, right, bottom, top, back, front;
        std::vector<conduit::index_t> edges[12];
        std::vector<conduit::index_t> corners[8];
    };

    /// Initialize some member fields from values in the node.
    void initializeFromOptions(const conduit::Node &t);

    /**
     @brief Generate a single domain for selectedBlock and add it to res node.

     @param nx The number of tiles in X.
     @param ny The number of tiles in Y.
     @param nz The number of tiles in Z.
     @param[output] res The conduit node that will contain the new domain.
     @param selectedBlock The selected block that describes the domain.
     @param domainId The domain number for the new domain.
     @param options A Conduit node containing any options that affect generation.
     */
    void generateDomain(IndexType nx, IndexType ny, IndexType nz,
                        conduit::Node &res,
                        const Block &selectedBlock,
                        IndexType domainId,
                        const conduit::Node &options);

    /**
     @brief Build the tile indices.

     @param[out] The TileIndices to construct.
     @param threeD Whether to make all of the indices needed for 3D.

     @return The number of points in a tile.
     */
    conduit::index_t buildTileIndices(TileIndices &ti, bool threeD) const;

    /**
     @brief Add points for a tile if they do not yet exist.

     @param A A 4x4 transformation matrix to apply to tile points when creating
              new points.
     @param zvalues A vector of Z values over which to iterate each 2d tile's vertices.
     @param ptids A vector of point ids. If any are -1 then they will make a new point.
     @param[out] x A vector that holds the X components of a coordinate.
     @param[out] y A vector that holds the Y components of a coordinate.
     @param[out] z A vector that holds the Z components of a coordinate.
     @param[out] srcPointId A vector that holds the id of the original tile points
                            that created each point.
     */
    void addPoints(const matrix4x4 &A,
                   const std::vector<double> &zvalues,
                   std::vector<conduit::index_t> &ptids,
                   std::vector<double> &x,
                   std::vector<double> &y,
                   std::vector<double> &z,
                   std::vector<conduit::index_t> &srcPointId) const;

    /**
     @brief This method adds the volume elements for a single tile, using the
            supplied point ids.

     @param ptids The point ids that make up the tile.
     @param[out] conn A vector to which connectivity for this tile is appended.
     @param[out] sizes A vector to which the size for this tile is appended.
     */
    void addVolumeElements(const std::vector<conduit::index_t> &ptids,
                           std::vector<conduit::index_t> &conn,
                           std::vector<conduit::index_t> &sizes) const;

    // These take lambdas to make the actual boundary info so node reordering can
    // be done there.
    template <typename Body>
    void iterateBoundary2D(const Block &selectedBlock,
                           const std::vector<Tile> &tiles,
                           Body &&body) const;

    template <typename Body>
    void iterateBoundary3D(const Block &selectedBlock,
                           const std::vector<Tile> &tiles,
                           bool ccFaces,
                           Body &&body) const;

    /**
     @brief Iterates over the selectedBlock and encodes its neighbor information
            into an adjset.

     @param selectedBlock The selected block with neighbor information.
     @param tiles The tiles that were created for the selected block.
     @param ti An object that contains tile indices for 1 tile.
     @param threeD Whether to make the adjset in 3D.
     @param domainId The domain id for the current domain.
     @param[out] out The Conduit node where the adjset will be made.

     */
    void addAdjset(const Block &selectedBlock,
                   const std::vector<Tile> &tiles,
                   const TopDownTiler::TileIndices &ti,
                   bool threeD,
                   conduit::index_t domainId,
                   conduit::Node &out) const;
private:
    IndexType m_numDomains;                   //!< The number of domains to make in total.
    bool m_curveSplitting;                    //!< Whether to allow Hilbert curve domains.
    std::vector<IndexType> m_selectedDomains; //!< The list domains we want to make.
};

//---------------------------------------------------------------------------
TopDownTiler::TopDownTiler() : TilerBase(), m_numDomains(1), m_curveSplitting(true), m_selectedDomains()
{
}

//---------------------------------------------------------------------------
void
TopDownTiler::initializeFromOptions(const conduit::Node &t)
{
    if(t.has_path("numDomains"))
    {
        m_numDomains = static_cast<IndexType>(t["numDomains"].to_index_t());
    }
    else
    {
        CONDUIT_ERROR("TopDownTiler requires numDomains so it knows how many domains to create.");
    }

    // Get the list of domains we're building. If it is not given, assume all domains.
    if(t.has_path("selectedDomains"))
    {
        m_selectedDomains = toIndexVector(t.fetch_existing("selectedDomains"));
        // Range check
        for(const auto dom : m_selectedDomains)
        {
            if(dom < 0 || dom >= m_numDomains)
            {
                CONDUIT_ERROR("selectedDomains value " << dom << " is out of range.");
            }
        }
    }

    // Get the list of domains we're building. If it is not given, assume all domains.
    if(t.has_path("curveSplitting"))
    {
        m_curveSplitting = t.fetch_existing("curveSplitting").to_int() > 0;
    }

    if(t.has_path("meshname"))
        meshName = t.fetch_existing("meshname").as_string();
    if(t.has_path("boundarymeshname"))
        boundaryMeshName = t.fetch_existing("boundarymeshname").as_string();
}

//---------------------------------------------------------------------------
void
TopDownTiler::generate(conduit::index_t nx, conduit::index_t ny, conduit::index_t nz,
                       conduit::Node &res, const conduit::Node &options)
{
    initializeFromOptions(options);

    // Make the whole block
    const IndexType dims[3] = {static_cast<IndexType>(nx),
                               static_cast<IndexType>(ny),
                               std::max(static_cast<IndexType>(nz), static_cast<IndexType>(1))};
    Block whole;
    whole.start = LogicalIndex{{0, 0, 0}};
    whole.end = LogicalIndex{{dims[0] - 1, dims[1] - 1, dims[2] - 1}};

    // Split whole into m_numDomains blocks.
    BlockSplitter S;
    S.options.curveSplitting = m_curveSplitting;
    auto blocks = S.split(whole, m_numDomains);

    // Realize any blocks we're asking for that might be hilbert blocks.
    realizeHilbertBlocks(blocks, m_selectedDomains);

    // Build the domains. We first expand the domain and look for its neighbors
    // so we know how the selected domain is connected.
#ifdef CONDUIT_WRITE_BLOCKS
    std::vector<Block> writeBlocks;
#endif
    if(m_selectedDomains.empty())
    {
        const bool multi = (blocks.size() > 1);
        for(size_t bi = 0; bi < blocks.size(); bi++)
        {
            auto selectedBlock = neighbors(blocks, bi, dims[2] > 1);
            generateDomain(nx, ny, nz, multi ? res.append() : res, selectedBlock,
                           static_cast<IndexType>(bi), options);
#ifdef CONDUIT_WRITE_BLOCKS
            writeBlocks.push_back(selectedBlock);
#endif
        }
    }
    else
    {
        const bool multi = (m_selectedDomains.size() > 1);
        for(const auto bi : m_selectedDomains)
        {
            auto selectedBlock = neighbors(blocks, bi, dims[2] > 1);
            generateDomain(nx, ny, nz, multi ? res.append() : res, selectedBlock,
                           static_cast<IndexType>(bi), options);
#ifdef CONDUIT_WRITE_BLOCKS
            writeBlocks.push_back(selectedBlock);
#endif
        }
    }

#ifdef CONDUIT_WRITE_BLOCKS
    // The blocks are simplified versions of the tiles that we used to see
    // how the decomposition should go. Save them out.
    conduit::Node out;
    BlockToBlueprint(writeBlocks, out);
    conduit::relay::io::blueprint::save_mesh(out, "blocks", "hdf5");
#endif
}

//---------------------------------------------------------------------------
void
TopDownTiler::generateDomain(IndexType nx, IndexType ny, IndexType nz, conduit::Node &res,
    const Block &selectedBlock, IndexType domainId, const conduit::Node &options)
{
    std::vector<double> x, y, z;
    std::vector<conduit::index_t> conn, sizes, shapes, bconn, bsizes, srcPointIds;
    std::vector<int> btype;

    // Process any options.
    if(options.has_path("tile"))
        initialize(options.fetch_existing("tile"));
    conduit::DataType indexDT(conduit::DataType::index_t());
    if(options.has_child("datatype"))
    {
        auto s = options.fetch_existing("datatype").as_string();
        if((s == "int") || (s == "int32") || (s == "integer"))
        {
            indexDT = conduit::DataType::int32();
        }
    }
    // Populate the extents.
    double extents[] = {0., 1, 0., 1., 0., 1.};
    if(options.has_path("extents"))
    {
        // Extents for the domain were given. Fit the domain into it.
        auto e = options.fetch_existing("extents").as_double_accessor();
        for(int i = 0; i < 6; i++)
            extents[i] = e[i];
    }

    // Make some tile indexing vectors.
    bool threeD = (nz >= 1);
    TileIndices ti;
    auto nTilePts = buildTileIndices(ti, threeD);

    // Data for transforming a single tile to fit in the extents.
    double normalize[3];
    normalize[0] = 1. / m_width;
    normalize[1] = 1. / m_height;
    normalize[2] = 1.;
    double size[3];
    size[0] = (extents[1] - extents[0]) / static_cast<double>(nx);
    size[1] = (extents[3] - extents[2]) / static_cast<double>(ny);
    size[2] = (extents[5] - extents[4]) / std::max(static_cast<double>(nz), 1.);
    double origin[3];
    origin[0] = extents[0];
    origin[1] = extents[2];
    origin[2] = extents[4];
    matrix4x4 SO = scale(size) * translate(origin);

    // Copy relevant points from neighbor tiles to the current tile if it makes sense.
    auto copyPoints = [&](const Block &b,
                          std::vector<Tile> &tiles,
                          const LogicalIndex &index,
                          const LogicalIndex &offset,
                          const std::vector<conduit::index_t> &destIds,
                          const std::vector<conduit::index_t> &srcIds)
    {
        auto localPrev = LogicalIndex{{index[0] - b.start[0] + offset[0],
                                       index[1] - b.start[1] + offset[1],
                                       index[2] - b.start[2] + offset[2]}};
        auto prevIndex = selectedBlock.IJKToIndex(localPrev);
        // Copy points from neighbor tile if possible.
        if(selectedBlock.image[prevIndex] == Block::Self)
        {
            auto local = LogicalIndex{{index[0] - b.start[0],
                                       index[1] - b.start[1],
                                       index[2] - b.start[2]}};
            auto localIndex = selectedBlock.IJKToIndex(local);
            Tile &current = tiles[localIndex];
            current.setPointIds(destIds, tiles[prevIndex].getPointIds(srcIds));
        }
    };

    // -------------------
    // Make points / conn
    // -------------------
    // We can iterate over the block's zones. It will traverse them in usual IJK order.
    // Since the selectedBlock is expanded to know about neighbors, each real/self zone
    // will have neighbors in IJK.
    std::vector<Tile> tiles(selectedBlock.size());
    selectedBlock.iterate([&](const LogicalIndex &index, IndexType zonetype) {
        // If the zone is part of the domain then we need to stamp out our tile.
        if(zonetype == Block::Self)
        {
            auto local = LogicalIndex{{index[0] - selectedBlock.start[0],
                                       index[1] - selectedBlock.start[1],
                                       index[2] - selectedBlock.start[2]}};
            auto localIndex = selectedBlock.IJKToIndex(local);

            // Make a transformation matrix.
            double T[3] = {static_cast<double>(index[0]),
                           static_cast<double>(index[1]),
                           static_cast<double>(index[2])};
            matrix4x4 M = (scale(normalize) * translate(T)) * SO;

            // Get the current tile and allocate its point ids.
            Tile &current = tiles[localIndex];
            current.reset(nTilePts);

            if(threeD)
            {
                // Copy points from adjacent tiles into the current tile.
                copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, 0, 0}}, ti.left, ti.right);
                copyPoints(selectedBlock, tiles, index, LogicalIndex{{0, -1, 0}}, ti.bottom, ti.top);
                if(nz > 1)
                {
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{0, 0, -1}}, ti.back, ti.front);

                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, -1, 0}}, ti.edges[8], ti.edges[11]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{1, -1, 0}}, ti.edges[10], ti.edges[9]);

                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, 0, -1}}, ti.edges[0], ti.edges[5]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{1, 0, -1}}, ti.edges[1], ti.edges[4]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{0, -1, -1}}, ti.edges[2], ti.edges[7]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{0, 1, -1}}, ti.edges[3], ti.edges[6]);

                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, -1, -1}}, ti.corners[0], ti.corners[7]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{1, -1, -1}}, ti.corners[1], ti.corners[6]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, 1, -1}}, ti.corners[2], ti.corners[5]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{1, 1, -1}}, ti.corners[3], ti.corners[4]);
                }
                else
                {
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, -1, 0}}, ti.edges[8], ti.edges[11]);
                    copyPoints(selectedBlock, tiles, index, LogicalIndex{{1, -1, 0}}, ti.edges[10], ti.edges[9]);
                }

                // Make any new points that are needed.
                const std::vector<double> zvalues{0., 1.};
                addPoints(M, zvalues, current.getPointIds(), x, y, z, srcPointIds);

                // Add elements
                addVolumeElements(current.getPointIds(), conn, sizes);
            }
            else
            {
                // Copy points from adjacent tiles into the current tile.
                copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, 0, 0}}, ti.left, ti.right);
                copyPoints(selectedBlock, tiles, index, LogicalIndex{{0, -1, 0}}, ti.bottom, ti.top);
                copyPoints(selectedBlock, tiles, index, LogicalIndex{{-1, -1, 0}}, ti.corners[0], ti.corners[3]);
                copyPoints(selectedBlock, tiles, index, LogicalIndex{{1, -1, 0}}, ti.corners[1], ti.corners[2]);

                const std::vector<double> zvalues{0.};
                addPoints(M, zvalues, current.getPointIds(), x, y, z, srcPointIds);

                const bool hasShapes = getTopology().has_path("elements/shapes");
                index_t_accessor shapeAccess;
                if(hasShapes)
                {
                    shapeAccess = getTopology().fetch_existing("elements/shapes").as_index_t_accessor();
                }
                int si = 0;
                iterateFaces(current.getPointIds(), 0, 0, false, BoundaryBack,
                    [&](const conduit::index_t *ids, conduit::index_t npts, int)
                    {
                        for(conduit::index_t pi = 0; pi < npts; pi++)
                            conn.push_back(ids[pi]);
                        sizes.push_back(npts);

                        if(hasShapes)
                        {
                          shapes.push_back(shapeAccess[si++]);
                        }
                    });
            }
        }
    });   

    // -------------------
    // Make Blueprint mesh
    // -------------------
    res["coordsets/coords/type"] = "explicit";
    res["coordsets/coords/values/x"].set(x);
    res["coordsets/coords/values/y"].set(y);
    if(threeD)
        res["coordsets/coords/values/z"].set(z);

    std::string shape2, shape3;
    shape2 = getTopology().fetch_existing("elements/shape").as_string();
    shape3 = (shape2 == "tri") ? "wedge" : "hex";

    conduit::Node &topo = res["topologies/" + meshName];
    topo["type"] = "unstructured";
    topo["coordset"] = "coords";
    topo["elements/shape"] = threeD ? shape3 : shape2;
    conduit::Node tmp;
    tmp.set_external(conn.data(), conn.size());
    tmp.to_data_type(indexDT.id(), topo["elements/connectivity"]);
    tmp.set_external(sizes.data(), sizes.size());
    tmp.to_data_type(indexDT.id(), topo["elements/sizes"]);
    if(!threeD)
    {
      if(!shapes.empty())
      {
        tmp.set_external(shapes.data(), shapes.size());
        tmp.to_data_type(indexDT.id(), topo["elements/shapes"]);
      }
      // Make offsets
      std::vector<index_t> offsets;
      offsets.reserve(sizes.size());
      index_t offset = 0;
      for(auto s : sizes)
      {
        offsets.push_back(offset);
        offset += s;
      }
      tmp.set_external(offsets.data(), offsets.size());
      tmp.to_data_type(indexDT.id(), topo["elements/offsets"]);

      // Copy shape_map.
      if(getTopology().has_path("elements/shape_map"))
      {
        topo["elements/shape_map"].set((getTopology().fetch_existing("elements/shape_map")));
      }
    }
    res["state/domain_id"] = domainId;

    // -------------------
    // Make boundaries
    // -------------------
    std::string bshape;
    if(threeD)
    {
        bshape = "quad";
        bool anyNonQuads = false;
        iterateBoundary3D(selectedBlock, tiles, true,
            [&](const conduit::index_t *ids, conduit::index_t npts, int bnd)
            {
                for(conduit::index_t pi = 0; pi < npts; pi++)
                    bconn.push_back(ids[pi]);
                bsizes.push_back(npts);
                btype.push_back(bnd + 1); // Make 1-origin
                anyNonQuads |= (npts != 4);
            });
        if(anyNonQuads)
            bshape = "polygonal";
    }
    else
    {
        bshape = "line";
        iterateBoundary2D(selectedBlock, tiles,
            [&](const conduit::index_t *ids, conduit::index_t npts, int bnd)
            {
                for(conduit::index_t pi = 0; pi < npts; pi++)
                    bconn.push_back(ids[pi]);
                bsizes.push_back(npts);
                btype.push_back(bnd + 1); // Make 1-origin
            });
    }
    if(!bconn.empty())
    {
        conduit::Node &btopo = res["topologies/" + boundaryMeshName];
        btopo["type"] = "unstructured";
        btopo["coordset"] = "coords";
        btopo["elements/shape"] = bshape;

        tmp.set_external(bconn.data(), bconn.size());
        tmp.to_data_type(indexDT.id(), btopo["elements/connectivity"]);

        tmp.set_external(bsizes.data(), bsizes.size());
        tmp.to_data_type(indexDT.id(), btopo["elements/sizes"]);

        if(bshape == "polygonal")
            conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(btopo, btopo["elements/offsets"]);

        res["fields/boundary_attribute/topology"] = boundaryMeshName;
        res["fields/boundary_attribute/association"] = "element";
        res["fields/boundary_attribute/values"].set(btype);
    }

    // -------------------
    // Make adjsets
    // -------------------
    if(m_numDomains > 1)
    {
        addAdjset(selectedBlock, tiles, ti, threeD, domainId, res);
    }
}

//---------------------------------------------------------------------------
conduit::index_t
TopDownTiler::buildTileIndices(TileIndices &ti, bool threeD) const
{
    auto nTilePts = getCoordset().fetch_existing("values/x").dtype().number_of_elements();

    // Make some tile face indexing vectors.
    std::vector<conduit::index_t> leftIds, rightIds, bottomIds, topIds, backIds, frontIds;
    ti.left = left();
    ti.right = right();
    ti.bottom = bottom();
    ti.top = top();

    // Back face corners
    ti.corners[0].push_back(ti.left[0]);
    ti.corners[1].push_back(ti.right[0]);
    ti.corners[2].push_back(ti.left[ti.left.size() - 1]);
    ti.corners[3].push_back(ti.right[ti.right.size() - 1]);

    if(threeD)
    {
        // Front face corners
        ti.corners[4].push_back(ti.left[0] + nTilePts);
        ti.corners[5].push_back(ti.right[0] + nTilePts);
        ti.corners[6].push_back(ti.left[ti.left.size() - 1] + nTilePts);
        ti.corners[7].push_back(ti.right[ti.right.size() - 1] + nTilePts);

        // Edges
        ti.edges[0] = left();
        ti.edges[1] = right();
        ti.edges[2] = bottom();
        ti.edges[3] = top();

        for(const auto value : left())
            ti.edges[4].push_back(value + nTilePts);
        for(const auto value : right())
            ti.edges[5].push_back(value + nTilePts);
        for(const auto value : bottom())
            ti.edges[6].push_back(value + nTilePts);
        for(const auto value : top())
            ti.edges[7].push_back(value + nTilePts);

        ti.edges[8] = std::vector<conduit::index_t>{ti.corners[0][0], ti.corners[4][0]};
        ti.edges[9] = std::vector<conduit::index_t>{ti.corners[2][0], ti.corners[6][0]};
        ti.edges[10] = std::vector<conduit::index_t>{ti.corners[1][0], ti.corners[5][0]};
        ti.edges[11] = std::vector<conduit::index_t>{ti.corners[3][0], ti.corners[7][0]};

        // Extend faces
        for(const auto value : left())
            ti.left.push_back(value + nTilePts);
        for(const auto value : right())
            ti.right.push_back(value + nTilePts);
        for(const auto value : bottom())
            ti.bottom.push_back(value + nTilePts);
        for(const auto value : top())
            ti.top.push_back(value + nTilePts);

        // Front/back faces
        ti.back.resize(nTilePts);
        std::iota(ti.back.begin(), ti.back.end(), 0);
        ti.front.resize(nTilePts);
        std::iota(ti.front.begin(), ti.front.end(), nTilePts);

        nTilePts *= 2;
    }

    return nTilePts;
}

//---------------------------------------------------------------------------
void
TopDownTiler::addPoints(const matrix4x4 &A,
                        const std::vector<double> &zvalues,
                        std::vector<conduit::index_t> &ptids,
                        std::vector<double> &x,
                        std::vector<double> &y,
                        std::vector<double> &z,
                        std::vector<conduit::index_t> &srcPointId) const
{
    // Iterate through points in the template and add them if they have
    // not been created yet.
    const auto &xpts = getCoordset().fetch_existing("values/x").as_double_array();
    const auto &ypts = getCoordset().fetch_existing("values/y").as_double_array();
    conduit::index_t index = 0;
    for(const double zval : zvalues)
    {
        for(conduit::index_t i = 0; i < xpts.number_of_elements(); i++, index++)
        {
            if(ptids[index] == Tile::INVALID_POINT)
            {
                ptids[index] = static_cast<int>(x.size());

                double P0[3] = {xpts[i], ypts[i], zval};
                double P[3];
                vec_matrix4x4_mult(P0, A, P);

                x.push_back(P[0]);
                y.push_back(P[1]);
                z.push_back(P[2]);

                srcPointId.push_back(i);
            }
        }
    }
}

//---------------------------------------------------------------------------
void
TopDownTiler::addVolumeElements(const std::vector<conduit::index_t> &ptids,
                                std::vector<conduit::index_t> &conn,
                                std::vector<conduit::index_t> &sizes) const
{
    const conduit::Node &topo = getTopology();
    std::string shape = topo.fetch_existing("elements/shape").as_string();
    conduit::index_t sides = 0;
    if(shape == "tri")
        sides = 3;
    else if(shape == "quad")
        sides = 4;

    if(sides == 3 || sides == 4)
    {
        const conduit::Node &n_conn = topo.fetch_existing("elements/connectivity");
        const auto tileconn = n_conn.as_index_t_accessor();
        const conduit::index_t nelem = tileconn.number_of_elements() / sides;
        conduit::index_t offset = static_cast<conduit::index_t>(ptids.size()) / 2;
        for(conduit::index_t i = 0; i < nelem; i++)
        {
            conduit::index_t start = i * sides;
            // back face
            for(conduit::index_t s = 0; s < sides; s++)
                conn.push_back(ptids[tileconn[start + s]]);

            // front face
            for(conduit::index_t s = 0; s < sides; s++)
                conn.push_back(ptids[offset + tileconn[start + s]]);

            sizes.push_back(2 * sides);
        }
    }
    else
    {
        CONDUIT_ERROR("Tiling polygonal shapes into 3D polyhedra is not yet supported.");
    }
}

//---------------------------------------------------------------------------
template <typename Body>
void
TopDownTiler::iterateBoundary2D(const Block &selectedBlock, const std::vector<Tile> &tiles,
    Body &&body) const
{
    const auto dY = selectedBlock.length(0);

    selectedBlock.iterate([&](const LogicalIndex &index, IndexType zonetype) {
        // If the zone is part of the domain then check its face neighbors to
        // see if any are still marked as Neighbor. They should be >= 0 if
        // there are real neighbors. So, if they are still Neighbor, they
        // must be external and will border boundaries.
        if(zonetype == Block::Self)
        {
            const auto local = LogicalIndex{{index[0] - selectedBlock.start[0],
                                             index[1] - selectedBlock.start[1],
                                             index[2] - selectedBlock.start[2]}};
            const auto localIndex = selectedBlock.IJKToIndex(local);
            const auto prevX = localIndex - 1;
            const auto nextX = localIndex + 1;
            const auto prevY = localIndex - dY;
            const auto nextY = localIndex + dY;

            // Get the current tile
            const Tile &current = tiles[localIndex];

            // Left boundary.
            if(selectedBlock.image[prevX] == Block::Neighbor)
            {
                const auto ptids = current.getPointIds(left());
                for(size_t i = 0; i < ptids.size() - 1; i++)
                    body(&ptids[i], 2, BoundaryLeft);
            }

            // Right boundary.
            if(selectedBlock.image[nextX] == Block::Neighbor)
            {
                const auto ptids = current.getPointIds(right());
                for(size_t i = 0; i < ptids.size() - 1; i++)
                    body(&ptids[i], 2, BoundaryRight);
            }

            // Bottom boundary.
            if(selectedBlock.image[prevY] == Block::Neighbor)
            {
                const auto ptids = current.getPointIds(bottom());
                for(size_t i = 0; i < ptids.size() - 1; i++)
                    body(&ptids[i], 2, BoundaryBottom);
            }

            // Top boundary.
            if(selectedBlock.image[nextY] == Block::Neighbor)
            {
                const auto ptids = current.getPointIds(top());
                for(size_t i = 0; i < ptids.size() - 1; i++)
                    body(&ptids[i], 2, BoundaryTop);
            }
        }
    });
}

//---------------------------------------------------------------------------
template <typename Body>
void
TopDownTiler::iterateBoundary3D(const Block &selectedBlock, const std::vector<Tile> &tiles,
    bool ccFaces, Body &&body) const
{
    const auto dY = selectedBlock.length(0);
    const auto dZ = selectedBlock.length(0) * selectedBlock.length(1);

    selectedBlock.iterate([&](const LogicalIndex &index, IndexType zonetype) {
        // If the zone is part of the domain then check its face neighbors to
        // see if any are still marked as Neighbor. They should be >= 0 if
        // there are real neighbors. So, if they are still Neighbor, they
        // must be external and will border boundaries.
        if(zonetype == Block::Self)
        {
            const auto local = LogicalIndex{{index[0] - selectedBlock.start[0],
                                            index[1] - selectedBlock.start[1],
                                            index[2] - selectedBlock.start[2]}};
            const auto localIndex = selectedBlock.IJKToIndex(local);
            const auto prevX = localIndex - 1;
            const auto nextX = localIndex + 1;
            const auto prevY = localIndex - dY;
            const auto nextY = localIndex + dY;
            const auto prevZ = localIndex - dZ;
            const auto nextZ = localIndex + dZ;

            // Get the current tile
            const Tile &current = tiles[localIndex];
            const auto zoffset = current.getPointIds().size() / 2;

            conduit::index_t pts[4];
            // Left boundary.
            if(selectedBlock.image[prevX] == Block::Neighbor)
            {
                const auto &ids = left();
                const auto &ptids = current.getPointIds();
                for(size_t i = 0; i < ids.size() - 1; i++)
                {
                    pts[0] = ptids[ids[i]];
                    pts[1] = ptids[ids[i] + zoffset];
                    pts[2] = ptids[ids[i + 1] + zoffset];
                    pts[3] = ptids[ids[i + 1]];
                    body(pts, 4, BoundaryLeft);
                }
            }

            // Right boundary.
            if(selectedBlock.image[nextX] == Block::Neighbor)
            {
                const auto &ids = right();
                const auto &ptids = current.getPointIds();
                for(size_t i = 0; i < ids.size() - 1; i++)
                {
                    if(ccFaces)
                    {
                        pts[0] = ptids[ids[i]];
                        pts[1] = ptids[ids[i + 1]];
                        pts[2] = ptids[ids[i + 1] + zoffset];
                        pts[3] = ptids[ids[i] + zoffset];
                    }
                    else
                    {
                        // Matches BoundaryLeft
                        pts[0] = ptids[ids[i]];
                        pts[1] = ptids[ids[i] + zoffset];
                        pts[2] = ptids[ids[i + 1] + zoffset];
                        pts[3] = ptids[ids[i + 1]];
                    }
                    body(pts, 4, BoundaryRight);
                }
            }

            // Bottom boundary.
            if(selectedBlock.image[prevY] == Block::Neighbor)
            {
                const auto &ids = bottom();
                const auto &ptids = current.getPointIds();
                for(size_t i = 0; i < ids.size() - 1; i++)
                {
                    pts[0] = ptids[ids[i]];
                    pts[1] = ptids[ids[i + 1]];
                    pts[2] = ptids[ids[i + 1] + zoffset];
                    pts[3] = ptids[ids[i] + zoffset];
                    body(pts, 4, BoundaryBottom);
                }
            }

            // Top boundary.
            if(selectedBlock.image[nextY] == Block::Neighbor)
            {
                const auto &ids = top();
                const auto &ptids = current.getPointIds();
                for(size_t i = 0; i < ids.size() - 1; i++)
                {
                    if(ccFaces)
                    {
                        pts[0] = ptids[ids[i]];
                        pts[1] = ptids[ids[i] + zoffset];
                        pts[2] = ptids[ids[i + 1] + zoffset];
                        pts[3] = ptids[ids[i + 1]];
                    }
                    else
                    {
                        // Matches BoundaryBottom
                        pts[0] = ptids[ids[i]];
                        pts[1] = ptids[ids[i + 1]];
                        pts[2] = ptids[ids[i + 1] + zoffset];
                        pts[3] = ptids[ids[i] + zoffset];
                    }
                    body(pts, 4, BoundaryTop);
                }
            }

            // Back boundary
            if(selectedBlock.image[prevZ] == Block::Neighbor)
            {
                iterateFaces(current.getPointIds(), 0, 0, true, BoundaryBack, body);
            }

            // Front boundary
            if(selectedBlock.image[nextZ] == Block::Neighbor)
            {
                iterateFaces(current.getPointIds(), 0, zoffset, !ccFaces, BoundaryFront, body);
            }
        }
    });
}

//---------------------------------------------------------------------------
void
TopDownTiler::addAdjset(const Block &selectedBlock,
                        const std::vector<Tile> &tiles,
                        const TopDownTiler::TileIndices &ti,
                        bool threeD,
                        conduit::index_t domainId,
                        conduit::Node &out) const
{
    // Iterate over the block and if there is a neighbor in the supplied offset
    // direction, pull out a subset of the tile's nodes and append them to
    // the neighbor values.
    auto addPoints = [&](const Block &b,
                         std::map<conduit::index_t, std::vector<conduit::index_t>> &neighborValues,
                         const LogicalIndex &offset,
                         const std::vector<conduit::index_t> &srcIds)
    {
        b.iterate([&](const LogicalIndex &index, IndexType zonetype) {
            if(zonetype == Block::Self)
            {
                // Use the offset to define a block that includes the current
                // index and the offset zone. Count how many of the zones in that
                // block are either self or a neighbor.
                conduit::index_t nk = 1 + ((offset[2] < 0) ? (-offset[2]) : offset[2]);
                conduit::index_t nj = 1 + ((offset[1] < 0) ? (-offset[1]) : offset[1]);
                conduit::index_t ni = 1 + ((offset[0] < 0) ? (-offset[0]) : offset[0]);

                conduit::index_t n = nk * nj * ni;
                conduit::index_t count = 0;
                for(conduit::index_t kk = 0; kk < nk; kk++)
                for(conduit::index_t jj = 0; jj < nj; jj++)
                for(conduit::index_t ii = 0; ii < ni; ii++)
                {
                    // Get the zone that is next to this one in the offset direction.
                    const auto local = LogicalIndex{{index[0] - b.start[0] + ii * offset[0],
                                                     index[1] - b.start[1] + jj * offset[1],
                                                     index[2] - b.start[2] + kk * offset[2]}};
                    const auto zi = b.IJKToIndex(local);
                    // Check whether there is a neighbor in the offset direction.
                    conduit::index_t id = b.image[zi];
                    if(id > Block::InvalidDomainId)
                    {
                        count++;
                    }
                }
                // The offset zone is a keeper.
                if(count == 1 || count == n - 1)
                {
                    // Get the neighborId at the offset zone
                    const auto interest = LogicalIndex{{index[0] - b.start[0] + offset[0],
                                                        index[1] - b.start[1] + offset[1],
                                                        index[2] - b.start[2] + offset[2]}};
                    const auto interestIndex = b.IJKToIndex(interest);
                    conduit::index_t neighborId = b.image[interestIndex];
                    if(neighborId > Block::InvalidDomainId)
                    {
                        // Get the neighbor values that match the neighbor. If we
                        // can't get them, create them.
                        auto it = neighborValues.find(neighborId);
                        if(it == neighborValues.end())
                        {
                            neighborValues[neighborId] = std::vector<conduit::index_t>();
                            it = neighborValues.find(neighborId);
                            size_t sizeGuess = b.numZones() / 6;
                            it->second.reserve(sizeGuess);
                        }

                        // Get relevant points from the current tile at index.
                        const auto local = LogicalIndex{{index[0] - b.start[0],
                                                         index[1] - b.start[1],
                                                         index[2] - b.start[2]}};
                        const auto localIndex = b.IJKToIndex(local);
                        const auto ptids = tiles[localIndex].getPointIds(srcIds);
                        // Add the points to the values.
                        for(const auto &id : ptids)
                            it->second.push_back(id);
                    }
                }
            }
        });
    };

    // Make a version of the input vector that filters out duplicates but
    // otherwise preserves order.
    auto makeUnique = [](const std::vector<conduit::index_t> &ids)
    {
        std::set<conduit::index_t> unique;
        std::vector<conduit::index_t> uniqueIds;
        uniqueIds.reserve(ids.size());
        for(const auto &id : ids)
        {
            if(unique.find(id) == unique.end())
            {
                unique.insert(id);
                uniqueIds.push_back(id);
            }
        }
        return uniqueIds;
    };

    // NOTE: We make the adjsets in this order so they are compatible with
    //       generate_corners(), although it seems like it would make the
    //       most sense to add faces first. Revisit this later since for non-planar
    //       interfaces there is something not 100% right.
    std::map<conduit::index_t, std::vector<conduit::index_t>> neighborValues[3];

    // Corners
    IndexType z0 = threeD ? -1 : 0;
    IndexType z1 = threeD ? 1 : 0;
    addPoints(selectedBlock, neighborValues[0], LogicalIndex{{-1, -1, z0}}, ti.corners[0]);
    addPoints(selectedBlock, neighborValues[0], LogicalIndex{{1, -1, z0}}, ti.corners[1]);
    addPoints(selectedBlock, neighborValues[0], LogicalIndex{{-1, 1, z0}}, ti.corners[2]);
    addPoints(selectedBlock, neighborValues[0], LogicalIndex{{1, 1, z0}}, ti.corners[3]);
    if(threeD)
    {
        addPoints(selectedBlock, neighborValues[0], LogicalIndex{{-1, -1, z1}}, ti.corners[4]);
        addPoints(selectedBlock, neighborValues[0], LogicalIndex{{1, -1, z1}}, ti.corners[5]);
        addPoints(selectedBlock, neighborValues[0], LogicalIndex{{-1, 1, z1}}, ti.corners[6]);
        addPoints(selectedBlock, neighborValues[0], LogicalIndex{{1, 1, z1}}, ti.corners[7]);  
    }

    // Edges
    if(threeD)
    {
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{-1, 0, -1}}, ti.edges[0]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{1, 0, -1}}, ti.edges[1]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{0, -1, -1}}, ti.edges[2]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{0, 1, -1}}, ti.edges[3]);  
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{-1, 0, 1}}, ti.edges[4]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{1, 0, 1}}, ti.edges[5]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{0, -1, 1}}, ti.edges[6]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{0, 1, 1}}, ti.edges[7]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{-1, -1, 0}}, ti.edges[8]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{-1, 1, 0}}, ti.edges[9]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{1, -1, 0}}, ti.edges[10]);
        addPoints(selectedBlock, neighborValues[1], LogicalIndex{{1, 1, 0}}, ti.edges[11]);  
    }

    // Faces (or edges in 2D)
    addPoints(selectedBlock, neighborValues[2], LogicalIndex{{-1, 0, 0}}, ti.left);
    addPoints(selectedBlock, neighborValues[2], LogicalIndex{{1, 0, 0}}, ti.right);
    addPoints(selectedBlock, neighborValues[2], LogicalIndex{{0, -1, 0}}, ti.bottom);
    addPoints(selectedBlock, neighborValues[2], LogicalIndex{{0, 1, 0}}, ti.top);
    if(threeD)
    {
        // Front/back faces
        addPoints(selectedBlock, neighborValues[2], LogicalIndex{{0, 0, -1}}, ti.back);
        addPoints(selectedBlock, neighborValues[2], LogicalIndex{{0, 0, 1}}, ti.front);
    }

    // Make the Conduit adjset.
    if(!neighborValues[0].empty() || !neighborValues[1].empty() || !neighborValues[2].empty())
    {
        // Make the top level adjset nodes.
        conduit::Node &adjset = out["adjsets/" + meshName + "_adjset"];
        adjset["association"] = "vertex";
        adjset["topology"] = meshName;
        conduit::Node &groups = adjset["groups"];
        for(int ni = 0; ni < 3; ni++)
        {
            for(auto it = neighborValues[ni].begin(); it != neighborValues[ni].end(); it++)
            {
                const std::string name = adjset_name(domainId, it->first);
                conduit::Node &group = groups[name];
                group["neighbors"] = it->first;
                // Add the unique ids
                group["values"].set(makeUnique(it->second));
            }
        }
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::examples::detail --
//-----------------------------------------------------------------------------

void
tiled(conduit::index_t nx, conduit::index_t ny, conduit::index_t nz,
      conduit::Node &res, const conduit::Node &options)
{
    if(options.has_path("numDomains"))
    {
        detail::TopDownTiler T;
        T.generate(nx, ny, nz, res, options);
    }
    else
    {
        detail::Tiler T;
        T.generate(nx, ny, nz, res, options);
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::examples --
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



