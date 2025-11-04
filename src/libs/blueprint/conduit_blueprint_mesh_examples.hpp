// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_EXAMPLES_HPP
#define CONDUIT_BLUEPRINT_MESH_EXAMPLES_HPP

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_exports.h"
#include "conduit_blueprint_mesh_examples_julia.hpp"
#include "conduit_blueprint_mesh_examples_venn.hpp"
#include "conduit_blueprint_mesh_examples_related_boundary.hpp"
#include "conduit_blueprint_mesh_examples_polystar.hpp"
#include "conduit_blueprint_mesh_examples_rz_cylinder.hpp"
#include "conduit_blueprint_mesh_examples_tiled.hpp"
#include "conduit_blueprint_mesh_examples_gyre.hpp"
#include "conduit_blueprint_mesh_examples_generate.hpp"

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

    /// Generates a uniform grid with a scalar field that assigns a unique,
    /// monotonically increasing value to each element.
    void CONDUIT_BLUEPRINT_API basic(const std::string &mesh_type,
                                     conduit::index_t nx,
                                     conduit::index_t ny,
                                     conduit::index_t nz,
                                     conduit::Node &res);

    /// Generates a strided structured grid with an element field and a vertex
    /// field, each element of which contains a sequentially increasing value.
    /// Calling code can specify the shape of the storage array for the fields
    /// and the location of the field values within the array.
    ///
    /// Pass the extra specifications with a conduit::Node desc:
    ///
    /// \code{.yaml} 
    /// vertex_data:
    ///   shape: [vx, vy, vz]
    ///   origin: [wx, wy, wz]
    /// element_data:
    ///   shape: [ex, ey, ez]
    ///   origin: [fx, fy, fz]
    /// \endcode
    ///
    /// \warning It is an error if the vertex or element data array shapes are
    ///          too small to contain the requested mesh data at the specified
    ///          origin.
    ///
    /// The following example shows how to produce a structured grid with 3x2
    /// elements for data and two elements padding each end of each dimension.
    /// Thus, element field values will be located at origin (2, 2) in a 7x6
    /// array.
    ///
    /// We will also use a custom-sized array for vertex fields.  In other
    /// Blueprint meshes, fields over vertices are stored in arrays larger by
    /// one element in each dimension than arrays that store fields over
    /// elements.  In strided structured meshes, the array dimensions of vertex
    /// and element fields are decoupled.  In this example, vertex fields will
    /// use a 7x6 array, not an 8x7 array as required by other Blueprint meshes.
    /// Since there are 3x2 mesh elements, there will be 4x3 vertex field
    /// values.  To match the element fields' padding as closely as possible,
    /// the vertex field values are located at origin (2, 2).
    ///
    /// The diagram below shows the mesh.  Note that the diagram origin is at
    /// the top left.  For elements, a space indicates a padding element and a
    /// `d` indicates a data element.  For vertices, an `o` indicates a padding
    /// vertex, an asterisk `*` indicates a data vertex, and a space indicates a
    /// mesh vertex that has no field data.
    ///
    /// \verbatim
    /// o--o--o--o--o--o--o-- 
    /// |  |  |  |  |  |  |  |
    /// o--o--o--o--o--o--o-- 
    /// |  |  |  |  |  |  |  |
    /// o--o--*--*--*--*--o-- 
    /// |  |  |d |d |d |  |  |
    /// o--o--*--*--*--*--o-- 
    /// |  |  |d |d |d |  |  |
    /// o--o--*--*--*--*--o-- 
    /// |  |  |  |  |  |  |  |
    /// o--o--o--o--o--o--o-- 
    /// |  |  |  |  |  |  |  |
    ///  -- -- -- -- -- -- -- 
    /// \endverbatim
    ///
    /// In summary, this will be a mesh with 4x3 vertices (therefore, 3x2
    /// elements).  The shape of each array storing a vertex field will be
    /// [7, 6, 0] with a data origin of [2, 2, 0].  The shape of each array
    /// storing a field over elements will be [7, 6, 0] with a data origin of
    /// [2, 2, 0].
    ///
    /// Here is code to produce this mesh.
    /// \code
    /// // Data mesh dimensions (the part we care about, not the padding) 
    /// index_t npts_x = 4;
    /// index_t npts_y = 3;
    /// index_t npts_z = 0;
    ///
    /// index_t nelts_x = npts_x - 1;
    /// index_t nelts_y = npts_y - 1;
    /// index_t nelts_z = 0;
    ///
    /// // Total padding in each dimension 
    /// index_t total_elt_pad = 4; // two on each end
    /// index_t total_pt_pad = 3;  // two on the low end, one on the high end
    ///
    /// // Origin: where the data starts in the arrays 
    /// index_t origin_x = 2;
    /// index_t origin_y = 2;
    ///
    /// // A mesh with "two elements of padding and equal sized element and vertex
    /// // field arrays" is a common use case.  It is the default that will be produced
    /// // if code passes an empty desc node to strided_structured().
    ///
    /// conduit::Node desc;  // description node
    /// conduit::Node res;   // result node will contain mesh
    ///
    /// // Equivalently, we can fill in the description node:
    ///
    /// desc["vertex_data/shape"].set(DataType::index_t(3));
    /// index_t_array vertex_shape = desc["vertex_data/shape"].as_index_t_array();
    /// vertex_shape[0] = npts_x + total_pt_pad;
    /// vertex_shape[1] = npts_y + total_pt_pad;
    /// vertex_shape[2] = 0;
    /// desc["vertex_data/origin"].set(DataType::index_t(3));
    /// index_t_array vertex_origin = desc["vertex_data/origin"].as_index_t_array();
    /// vertex_origin[0] = origin_x;
    /// vertex_origin[1] = origin_y;
    /// vertex_origin[2] = 0;
    /// desc["element_data/shape"].set(DataType::index_t(3));
    /// index_t_array element_shape = desc["element_data/shape"].as_index_t_array();
    /// element_shape[0] = nelts_x + total_elt_pad;
    /// element_shape[1] = nelts_y + total_elt_pad;
    /// element_shape[2] = 0;
    /// desc["element_data/origin"].set(DataType::index_t(3));
    /// index_t_array element_origin = desc["element_data/origin"].as_index_t_array();
    /// element_origin[0] = origin_x;
    /// element_origin[1] = origin_y;
    /// element_origin[2] = 0;
    ///
    /// // Generate the mesh into res
    /// strided_structured(desc, npts_x, npts_y, npts_z, res);
    /// \endcode
    ///
    /// The node `res` will contain the following structure (edited slightly
    /// for clarity):
    /// \verbatim
    /// state:
    ///   time: 3.1415
    ///   cycle: 100
    /// coordsets:
    ///   coords:
    ///     type: "explicit"
    ///     values:
    ///       x: [-10.0,  -6.67, -3.33,  0.0,   3.33,  6.67, 10.0,
    ///           -10.0,  -6.67, -3.33,  0.0,   3.33,  6.67, 10.0,
    ///           -10.0,  -6.67, -3.33,  0.0,   3.33,  6.67, 10.0,
    ///           -10.0,  -6.67, -3.33,  0.0,   3.33,  6.67, 10.0,
    ///           -10.0,  -6.67, -3.33,  0.0,   3.33,  6.67, 10.0,
    ///           -10.0,  -6.67, -3.33,  0.0,   3.33,  6.67, 10.0]
    ///       y: [-10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0,
    ///            -6.0,  -6.0,  -6.0,  -6.0,  -6.0,  -6.0,  -6.0,
    ///            -2.0,  -2.0,  -2.0,  -2.0,  -2.0,  -2.0,  -2.0,
    ///             2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,
    ///             6.0,   6.0,   6.0,   6.0,   6.0,   6.0,   6.0,
    ///            10.0,  10.0,  10.0,  10.0,  10.0,  10.0,  10.0]
    /// topologies:
    ///   mesh:
    ///     type: "structured"
    ///     coordset: "coords"
    ///     elements:
    ///       dims:
    ///         i: 3
    ///         j: 2
    ///         offsets: [2, 2]
    ///         strides: [1, 7]
    /// fields:
    ///   vert_vals:
    ///     association: "vertex"
    ///     topology: "mesh"
    ///     offsets: [2, 2]
    ///     strides: [1, 7]
    ///     values: [0.0, 0.0, 0.0,  0.0,  0.0,  0.0, 0.0,
    ///              0.0, 0.0, 0.0,  0.0,  0.0,  0.0, 0.0,
    ///              0.0, 0.0, 1.0,  2.0,  3.0,  4.0, 0.0,
    ///              0.0, 0.0, 5.0,  6.0,  7.0,  8.0, 0.0,
    ///              0.0, 0.0, 9.0, 10.0, 11.0, 12.0, 0.0,
    ///              0.0, 0.0, 0.0,  0.0,  0.0,  0.0, 0.0]
    ///   ele_vals:
    ///     association: "element"
    ///     topology: "mesh"
    ///     offsets: [2, 2]
    ///     strides: [1, 7]
    ///     values: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    ///              0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    ///              0.0, 0.0, 1.0, 2.0, 3.0, 0.0, 0.0,
    ///              0.0, 0.0, 4.0, 5.0, 6.0, 0.0, 0.0,
    ///              0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    ///              0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    /// \endverbatim
    ///
    /// The values for the `ele_vals` field start at [2, 2], with a stride of
    /// 1 in the first dimension and a stride of 7 in the second dimension.
    /// These values apply to the elements of `mesh`, so there are two rows
    /// with three values in each row.  This example sets all the values to
    /// sequentially increasing values.  Any element that isn't used for the
    /// elements of `mesh` is set to zero.
    ///
    /// Similar to `ele_vals`, the values for the `vert_vals` field start at
    /// [2, 2] with a stride of [1, 7].  The vertices of `mesh` require three
    /// rows of four elements, which are set to increasing non-zero values.
    /// Elements outside that region are ignored.  To emphasize this, the
    /// example sets those unused elements to zero.
    ///
    /// In summary, the result mesh fields can be bigger than Mesh Blueprint
    /// requires.
    /// - The `values` array of each field supplies values for the elements
    ///   or vertices of the mesh named by `topology`.
    /// - `strides` tells how big the `values` array is.
    /// - `offsets` tells where to start looking within `values`.
    /// - The size of the mesh named by `topology` tells what elements to use
    ///   from `array`.  Any array element outside this range is ignored.
    void CONDUIT_BLUEPRINT_API strided_structured(conduit::Node &desc,
                                                  conduit::index_t npts_x,
                                                  conduit::index_t npts_y,
                                                  conduit::index_t npts_z,
                                                  conduit::Node &res);

    /// Generates a multidomain uniform grid of 'basic' examples for each
    /// domain/grid element.
    void CONDUIT_BLUEPRINT_API grid(const std::string &mesh_type,
                                     conduit::index_t nx,
                                     conduit::index_t ny,
                                     conduit::index_t nz,
                                     conduit::index_t dx,
                                     conduit::index_t dy,
                                     conduit::index_t dz,
                                     conduit::Node &res);

    /// Generates a multidomain grid of examples.  spec is a list or object
    /// specifying one or more domains.  Each child of spec represents a domain
    /// in either two or three dimensions.  Each child has
    ///  - integral children npts_x, npts_y, optionally npts_z
    ///  - integral child domain_id
    ///  - numerical array children corner_xs, corner_ys, optionally corner_zs
    ///  - optionally, integral children nest_child_id and nest_ratio
    ///
    /// Taken together, the corner_xs, corner_ys, and optionally corner_zs
    /// represent the points at the corners of that domain.
    ///
    /// If nest_child_id and nest_ratio are present in a domain spec,
    /// bent_multi_grid makes a refined domain nested within the domain.
    void CONDUIT_BLUEPRINT_API bent_multi_grid(const conduit::Node& spec,
                                               conduit::Node& res);

    /// Generates a multidomain grid of examples.  If spec is an empty node,
    /// makes a default spec that includes enhanced and reduced connectivity
    /// as well as a nested domain, then calls bent_multi_grid.  If spec
    /// is not empty, directly calls bent_multi_grid.
    void CONDUIT_BLUEPRINT_API bent_multi_grid_amr(const conduit::Node& spec,
                                                   conduit::Node& res);

    /// Generates a braid-like example mesh that covers elements defined in a
    /// rectilinear grid.  Currently hexs are supported for 3D meshes and quads
    /// are supported for 2D meshes.  Specify a mesh as outlined in the docs
    /// for bent_multi_grid().
    void CONDUIT_BLUEPRINT_API bent_braid(const conduit::Node& spec,
                                          conduit::Node& res);

    /// Generates a braid-like example mesh that covers elements defined in a
    /// rectilinear grid. The element type (e.g. triangles, quads, their 3D
    /// counterparts, or a mixture) and the coordinate set/topology
    /// types can be configured by specifying different "mesh_type" values
    /// (see the Conduit documentation for details).
    void CONDUIT_BLUEPRINT_API braid(const std::string &mesh_type,
                                     conduit::index_t nx,
                                     conduit::index_t ny,
                                     conduit::index_t nz,
                                     conduit::Node &res);

    /// Generates a multi-domain fibonacci estimation of a golden spiral.
    void CONDUIT_BLUEPRINT_API spiral(conduit::index_t ndomains,
                                      conduit::Node &res);

    /// Generates a tessellated heterogeneous polygonal mesh consisting of
    /// packed octogons and rectangles. The parameter nz can be any nonzero
    /// natural number. An nz value of 1 will produce a polytess in 2D,
    /// while an nz value of 2 will produce a polytess in 3D, which can be
    /// explained as follows: first, polytess is placed into 3D space, and
    /// then a copy of it is placed into a plane parallel to the original.
    /// Then "walls" are added, and finally polyhedra are specified that use
    /// faces from the original polytess, the reflected copy, and the walls.
    /// An nz value of 3 or more will simply add layers to this setup,
    /// essentially stacking "sheets" of polytess on top of one another.
    void CONDUIT_BLUEPRINT_API polytess(conduit::index_t nlevels,
                                        conduit::index_t nz,
                                        conduit::Node &res);

    /// Generates a chain of cubes and triangular prisms
    void CONDUIT_BLUEPRINT_API polychain(const conduit::index_t length,
                                         conduit::Node &res);

    /// Generates an assortment of extra meshes that demonstrate the use of
    /// less common concepts (e.g. adjacency sets, amr blocks, etc.).
    void CONDUIT_BLUEPRINT_API misc(const std::string &mesh_type,
                                    conduit::index_t nx,
                                    conduit::index_t ny,
                                    conduit::index_t nz,
                                    conduit::Node &res);

    /// Generates a mesh that uses uniform adjsets
    void CONDUIT_BLUEPRINT_API adjset_uniform(conduit::Node &res);
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::examples --
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



