// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.
//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_generate.cpp
///
//-----------------------------------------------------------------------------

#if defined(CONDUIT_PLATFORM_WINDOWS)
#define NOMINMAX
#undef min
#undef max
#include "windows.h"
#endif

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_o2mrelation_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::blueprint;
using namespace conduit::utils;
namespace bputils = conduit::blueprint::mesh::utils;
namespace o2mutils = conduit::blueprint::o2mrelation::utils;

// Enable this macro when debugging to make Conduit hang where it would throw.
//#define USE_ERROR_HANDLER

/// Testing Constants ///

static const index_t ELEM_TYPE_TRI_ID = 0;
static const index_t ELEM_TYPE_QUAD_ID = 1;
static const index_t ELEM_TYPE_TET_ID = 2;
static const index_t ELEM_TYPE_HEX_ID = 3;

static const std::string ELEM_TYPE_LIST[]      = {"tris", "quads", "tets", "hexs"};
static const index_t ELEM_TYPE_DIMS[]          = {     2,       2,      3,      3};
static const index_t ELEM_TYPE_CELL_ELEMS[]    = {     2,       1,      6,      1};
static const index_t ELEM_TYPE_CELL_LINES[]    = {     1,       0,     -1,      0};
static const index_t ELEM_TYPE_INDICES[]       = {     3,       4,      4,      8};
static const index_t ELEM_TYPE_LINES[]         = {     3,       4,      6,     12};
static const index_t ELEM_TYPE_FACES[]         = {     1,       1,      4,      6};
static const index_t ELEM_TYPE_FACE_INDICES[]  = {     3,       4,      3,      4};
static const index_t ELEM_TYPE_COUNT = sizeof(ELEM_TYPE_LIST) / sizeof(ELEM_TYPE_LIST[0]);

static const std::vector<std::string> CSET_AXES = bputils::CARTESIAN_AXES;
// FIXME(JRC): Why don't the 'generate' functions have an 'indices' component?
// This should be required, alongside 'sizes'.
// static const std::vector<std::string> O2M_PATHS = o2mutils::O2M_PATHS;
static const std::vector<std::string> O2M_PATHS = {"sizes", "offsets", "values"};

const static index_t TRIVIAL_GRID[] = {2, 2, 2};
const static index_t SIMPLE_GRID[] = {3, 3, 3};
const static index_t COMPLEX_GRID[] = {4, 4, 4};

typedef std::vector<index_t> index_list;

/// Testing Helpers ///
void
tmp_err_handler(const std::string &s1, const std::string &s2, int i1)
{
    std::cout << "s1=" << s1 << ", s2=" << s2 << ", i1=" << i1 << std::endl;

    while(1);
}

index_t braid_bound_npts_z(const std::string &mesh_type, index_t npts_z)
{
    if(mesh_type == "tris"  ||
       mesh_type == "quads" ||
       mesh_type == "quads_poly")
    {
        return 0;
    }
    else
    {
        return npts_z;
    }
}


// NOTE(JRC): This is basically an implementation of the combinatorical concept
// of "n choose i" with all results being returned as lists over index space.
std::vector<index_list> calc_combinations(const index_t combo_length, const index_t total_length)
{
    std::vector<index_list> combinations;

    index_t max_binary_combo = 1;
    for(index_t li = 1; li < total_length; li++)
    {
        max_binary_combo <<= 1;
        max_binary_combo += 1;
    }
    max_binary_combo += 1;

    for(index_t ci = 0; ci < max_binary_combo; ci++)
    {
        index_list combination;
        for(index_t bi = 0; bi < total_length; bi++)
        {
            if((ci >> bi) & 1)
            {
                combination.push_back(bi);
            }
        }

        if((index_t)combination.size() == combo_length)
        {
            combinations.push_back(combination);
        }
    }

    return (combo_length > 0) ? combinations : std::vector<index_list>(1);
}

std::vector<index_list> expand_o2mrelation(const Node &o2m)
{
    // TODO(JRC): This is to get around annoying const correctness... there
    // really ought to be a better way.
    Node o2m_temp;
    o2m_temp.set_external(o2m);

    Node data;
    const DataType o2m_dtype(o2m["values"].dtype().id(), 1);

    std::vector<index_list> o2m_list;
    o2mrelation::O2MIterator o2m_iter(o2m);
    while(o2m_iter.has_next(o2mrelation::DATA))
    {
        o2m_iter.next(o2mrelation::ONE);
        o2m_iter.to_front(o2mrelation::MANY);

        index_list one_list(o2m_iter.elements(o2mrelation::MANY));
        while(o2m_iter.has_next(o2mrelation::MANY))
        {
            index_t many_index = o2m_iter.next(o2mrelation::MANY);
            index_t data_index = o2m_iter.index(o2mrelation::DATA);
            data.set_external(o2m_dtype, o2m_temp["values"].element_ptr(data_index));
            one_list[many_index] = data.to_index_t();
        }

        o2m_list.push_back(one_list);
    }

    return o2m_list;
}

// NOTE(JRC): This function isn't really used by the 'generate_{sides|corners}'
// checker functions because there are floating-point differences between the
// manually and procedurally calculated field vectors, which makes vector comparison
// via 'EXPECT_EQ' untenable, and thus 'conduit::Node:diff' is used instead.
std::vector<float64> expand_field(const Node &field)
{
    // TODO(JRC): This is to get around annoying const correctness... there
    // really ought to be a better way.
    Node field_temp;
    field_temp.set_external(field);

    Node data;
    Node &field_vals = field_temp["values"];
    const DataType field_dtype(field_vals.dtype().id(), 1);
    const index_t field_length = field_vals.dtype().number_of_elements();

    std::vector<float64> field_list(field_length);
    for(index_t fi = 0; fi < field_length; fi++)
    {
        data.set_external(field_dtype, field_vals.element_ptr(fi));
        field_list[fi] = data.to_float64();
    }

    return field_list;
}

struct GridMesh
{
    GridMesh(index_t type, const index_t *npts, bool poly = false)
    {
        Node info;
        mesh::examples::braid(ELEM_TYPE_LIST[type],
                              npts[0],
                              npts[1],
                              braid_bound_npts_z(ELEM_TYPE_LIST[type],npts[2]),
                              mesh);

        Node &topo = mesh["topologies"].child(0);
        mesh::topology::unstructured::generate_offsets(topo, topo["elements/offsets"]);

        this->type = type;
        for(index_t di = 0; di < ELEM_TYPE_DIMS[type]; di++)
        {
            this->npts[di] = npts[di];
        }
        for(index_t di = ELEM_TYPE_DIMS[type]; di < 3; di++)
        {
            this->npts[di] = 0;
        }

        is_poly = poly;
        if(is_poly)
        {
            const std::string topo_name = topo.name();
            Node &poly_topo = mesh["topologies"]["poly_" + topo_name];
            mesh::topology::unstructured::to_polygonal(topo, poly_topo);
            mesh["topologies"].remove(topo_name);
        }
    }

    index_t cells() const
    {
        index_t num_cells = 1;
        for(index_t di = 0; di < dims(); di++)
        {
            num_cells *= (npts[di] - 1);
        }
        return num_cells;
    }

    index_t elems() const
    {
        return cells() * ELEM_TYPE_CELL_ELEMS[type];
    }

    index_t faces() const
    {
        index_t num_faces = 0;
        for(index_t di = 0; di < dims(); di++)
        {
            index_t dim_num_faces = npts[di];
            for(index_t dj = 0; dj < dims(); dj++)
            {
                dim_num_faces *= (di != dj) ? npts[dj] - 1 : 1;
            }
            num_faces += dim_num_faces;
        }
        return (dims() == 2) ? elems() : num_faces;
    }

    index_t lines() const
    {
        index_t num_lines = 0;
        for(index_t di = 0; di < dims(); di++)
        {
            index_t dim_num_lines = npts[di] - 1;
            for(index_t dj = 0; dj < dims(); dj++)
            {
                dim_num_lines *= (di != dj) ? npts[dj] : 1;
            }
            num_lines += dim_num_lines;
        }
        return num_lines + ELEM_TYPE_CELL_LINES[type] * cells();
    }

    index_t points() const
    {
        index_t num_points = 1;
        for(index_t di = 0; di < dims(); di++)
        {
            num_points *= npts[di];
        }
        return num_points;
    }

    index_t elem_valence(const index_t entity_dim) const
    {
        index_t total_valence = 0;
        for(index_t di = entity_dim; di <= dims(); di++)
        {
            index_t dim_valence = 1;
            for(index_t ddi = entity_dim; ddi < di; ddi++, dim_valence *= 2) {}
            dim_valence += (entity_dim == 0) ? di * ELEM_TYPE_CELL_LINES[type] : 0;

            index_t dim_entities = 0;
            std::vector<index_list> axis_combos = calc_combinations(di, dims());
            std::vector<index_list> major_combos = calc_combinations(entity_dim, di);
            for(index_t ci = 0; ci < (index_t)axis_combos.size(); ci++)
            {
                const index_list &axis_list = axis_combos[ci];
                for(index_t mi = 0; mi < (index_t)major_combos.size(); mi++)
                {
                    index_t dim_combo_entities = 1;
                    const index_list &major_axis_list = major_combos[mi];
                    for(index_t ai = 0; ai < (index_t)axis_list.size(); ai++)
                    {
                        bool is_major_axis = std::find(major_axis_list.begin(),
                            major_axis_list.end(), ai) != major_axis_list.end();
                        dim_combo_entities *= npts[axis_list[ai]] - 1 -
                            (index_t)(!is_major_axis);
                    }
                    dim_entities += dim_combo_entities;
                }
            }

            index_t dim_duplicates = 1;
            for(index_t ddi = 0; ddi < dims() - di; ddi++, dim_duplicates *= 2) {}

            total_valence += dim_valence * (dim_duplicates * dim_entities);
        }

        // NOTE(JRC): The 'total_valence' value accounts only for the quad grid
        // valence; additional edges/faces need to be considered for tri grids
        // (i.e. corners for 0d, internal edges for 1d, internal faces for 1d).
        return total_valence + ((ELEM_TYPE_LIST[type] != "tris") ? 0 : (
            (entity_dim == 0) ? 2 : (
            (entity_dim == 1) ? 2 * cells() : (
            (entity_dim == 2) ? 1 * cells() : 0))));
    }

    float64 cell_volume() const
    {
        // NOTE(JRC): This is explicitly given in the definition of the 'braid'
        // example generation function.
        const float64 dim_length = 20.0;

        float64 cell_vol = 1.0;
        for(index_t di = 0; di < dims(); di++)
        {
            cell_vol *= dim_length / (npts[di] - 1.0);
        }
        return cell_vol;
    }

    float64 elem_volume() const
    {
        return cell_volume() / ELEM_TYPE_CELL_ELEMS[type];
    }

    index_t points_per_face() const
    {
        return ELEM_TYPE_FACE_INDICES[type];
    }

    index_t faces_per_elem() const
    {
        return ELEM_TYPE_FACES[type];
    }

    index_t lines_per_elem() const
    {
        return ELEM_TYPE_LINES[type];
    }

    index_t points_per_elem() const
    {
        return ELEM_TYPE_INDICES[type];
    }

    index_t dims() const
    {
        return ELEM_TYPE_DIMS[type];
    }

    Node mesh;
    index_t type;
    index_t npts[3];
    bool is_poly;
};

struct GridMeshCollection
{
    struct Iterator
    {
        Iterator() : ptr(NULL) {}
        Iterator(const GridMesh *p) : ptr(p) {}

        Iterator &operator++(int) { ptr++; return *this; }
        Iterator  operator++() { Iterator t(ptr); ptr++; return t; }
        Iterator operator+(size_t d) const { return Iterator(ptr + d); }

        const GridMesh &operator*()  const { print_pos(); return *ptr; }
        const GridMesh *operator->() const { return ptr; }

        bool operator==(const Iterator &other) const { return ptr == other.ptr; }
        bool operator!=(const Iterator &other) const { return ptr != other.ptr; }

        void print_pos() const
        {
            std::cout << "  Testing " <<
                std::string(ptr->is_poly ? "polygonal" : "non-polygonal") << " " <<
                ELEM_TYPE_LIST[ptr->type] << " grid..." << std::endl;
        }

        const GridMesh *ptr;
    };

    GridMeshCollection(const index_t *npts, const bool debug=false)
    {
        if(debug)
        {
            // HARD DEBUG
            meshes.push_back(GridMesh(ELEM_TYPE_QUAD_ID, npts, false));

            // SOFT DEBUG
            // for(index_t ti = 0; ti < ELEM_TYPE_COUNT; ti++)
            // {
            //     meshes.push_back(GridMesh(ti, npts, false));
            // }
        }
        else
        {
            for(index_t ti = 0; ti < ELEM_TYPE_COUNT; ti++)
            {
                for(index_t pi = 0; pi < 2; pi++)
                {
                    meshes.push_back(GridMesh(ti, npts, (bool)pi));
                }
            }
        }
    }

    Iterator begin() const
    {
        return Iterator(&meshes[0]);
    }

    Iterator end() const
    {
        return Iterator(&meshes[0] + meshes.size());
    }

    std::vector<GridMesh> meshes;
};

struct GridDims
{
    GridDims(const index_t *npts)
    {
        for(index_t i = 0; i < 3; i++)
        {
            dims[i] = npts[i];
        }
    }

    bool operator<(const GridDims &other) const
    {
        for(index_t i = 0; i < 3; i++)
        {
            if(this->dims[i] != other.dims[i])
            {
                return this->dims[i] < other.dims[i];
            }
        }
        return false;
    };

    index_t dims[3];
};

void query_point(const Node &coordset, index_t point_index, Node &data)
{
    if(data.dtype().is_empty())
    {
        data.set(DataType::float64(3));
    }
    const DataType cset_dtype(coordset["values"].child(0).dtype().id(), 1);
    const DataType data_dtype(data.dtype().id(), 1);

    Node temp1, temp2;
    for(index_t di = 0; di < coordset["values"].number_of_children(); di++)
    {
        temp1.set_external(cset_dtype,
            (void*)coordset["values"][CSET_AXES[di]].element_ptr(point_index));
        temp2.set_external(data_dtype, data.element_ptr(di));
        temp1.to_data_type(data_dtype.id(), temp2);
    }
}

// TODO(JRC): The fact that there isn't a standard C++ library for simple
// linear algebra operations and that this is the ~20th time I've had to
// write such operations makes me sad indeed.

bool fuzzy_eq(float64 f1, float64 f2, float64 epsilon = CONDUIT_EPSILON)
{
    return std::abs(f1 - f2) <= epsilon;
}

bool fuzzy_le(float64 f1, float64 f2, float64 epsilon = CONDUIT_EPSILON)
{
    return f1 < f2 || fuzzy_eq(f1, f2, epsilon);
}

void calc_vec_add(const float64 *u, const float64* v, float64 *r)
{
    r[0] = u[0] + v[0];
    r[1] = u[1] + v[1];
    r[2] = u[2] + v[2];
}

void calc_vec_sub(const float64 *u, const float64* v, float64 *r)
{
    r[0] = u[0] - v[0];
    r[1] = u[1] - v[1];
    r[2] = u[2] - v[2];
}

void calc_vec_cross(const float64 *u, const float64 *v, float64 *r)
{
    r[0] = u[1] * v[2] - u[2] * v[1];
    r[1] = u[2] * v[0] - u[0] * v[2];
    r[2] = u[0] * v[1] - u[1] * v[0];
}

float64 calc_vec_mag(const float64 *u)
{
    return std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
}

void calc_inversion_field(index_t type, const Node &topo, const Node &coords, Node &dest)
{
    // NOTE(JRC): This function isn't currently in use because it introduced too
    // much of a performance burden for even small input topology sizes (e.g.
    // ~30 elements). Should this performance be improvable via better entity
    // composition/association or otherwise, then inversion checks should be
    // reintroduced to the code.

    // NOTE(JRC): This function assumes the existence of an offsets field in
    // the source topology, which is true for all callees in this test suite.
    // TODO(JRC): The performance of this method would be greatly enhanced if a
    // sparse data table was given by "generate_lines" that indicated the relations
    // between source entities and the resulting lines (and vice versa).
    // TODO(JRC): If the type is 3D, then this field isn't absolutely correct about
    // the number of inversions because line-plane intersections aren't calculated.
    const Node &topo_conn = topo["elements/connectivity"];
    const Node &topo_off = topo["elements/offsets"];
    const DataType conn_dtype(topo_conn.dtype().id(), 1);
    const DataType off_dtype(topo_off.dtype().id(), 1);
    const index_t topo_num_elems = topo_off.dtype().number_of_elements();

    Node data_node;
    Node s2d_map, d2s_map;

    std::vector< std::vector< std::set<index_t> > > elem_lines(topo_num_elems);
    {
        Node elem_mesh;
        Node &elem_coords = elem_mesh["coordsets"][coords.name()];
        Node &elem_topo = elem_mesh["topologies"][coords.name()];
        elem_coords.set_external(coords);

        Node line_mesh;
        Node &line_coords = line_mesh["coordsets"][coords.name()];
        Node &line_topo = line_mesh["topologies"][topo.name()];
        line_coords.set_external(coords);

        Node elem_topo_templ;
        elem_topo_templ.set_external(topo);
        elem_topo_templ.remove("elements/connectivity");
        elem_topo_templ.remove("elements/offsets");

        int64 line_data_raw[2] = {-1, -1};
        Node line_data(DataType::int64(2), &line_data_raw[0], true);

        elem_topo.set(elem_topo_templ);
        Node &elem_conn = elem_topo["elements/connectivity"];
        for(index_t ei = 0; ei < topo_num_elems; ei++)
        {
            // TODO(JRC): This code was lifted directly from the private structure
            // 'conduit_blueprint_mesh.cpp:TopologyMetadata'. Ultimately, it would
            // be better if these two pieces of functionality were integrated.
            data_node.set_external(off_dtype, (void*)topo_off.element_ptr(ei));
            index_t elem_start_index = data_node.to_int64();
            data_node.set_external(off_dtype, (void*)topo_off.element_ptr(ei+1));
            index_t elem_end_index = (ei < topo_num_elems - 1) ?
                data_node.to_int64() : topo_conn.dtype().number_of_elements();

            index_t elem_size = elem_end_index - elem_start_index;
            data_node.set_external(DataType(conn_dtype.id(), elem_size),
                (void*)topo_conn.element_ptr(elem_start_index));
            data_node.to_data_type(DataType::int64(1).id(), elem_conn);

            mesh::topology::unstructured::generate_lines(elem_topo, line_topo, s2d_map, d2s_map);

            Node &line_conn = line_topo["elements/connectivity"];
            for(index_t li = 0; li < line_conn.dtype().number_of_elements(); li += 2)
            {
                data_node.set_external(DataType(line_conn.dtype().id(), 2),
                    (void*)line_conn.element_ptr(li));
                data_node.to_data_type(line_data.dtype().id(), line_data);

                std::set<index_t> curr_line;
                curr_line.insert((index_t)line_data_raw[0]);
                curr_line.insert((index_t)line_data_raw[1]);
                elem_lines[ei].push_back(curr_line);
            }
        }
    }

    dest.reset();
    dest["association"].set("element");
    dest["volume_dependent"].set("false");
    dest["topology"].set(topo.name());
    dest["values"].set(DataType::int32(topo_num_elems));

    int32_array dest_vals = dest["values"].as_int32_array();
    for(index_t ei = 0; ei < topo_num_elems; ei++)
    {
        const std::set< std::set<index_t> > elem_lineset(
            elem_lines[ei].begin(), elem_lines[ei].end());

        dest_vals[ei] = 0 - (elem_lines[ei].size() != elem_lineset.size());
        for(std::set< std::set<index_t> >::iterator iline_it = elem_lineset.begin();
            iline_it != elem_lineset.end() && dest_vals[ei] == 0; ++iline_it)
        {
            const std::set<index_t> &iline = *iline_it;
            for(std::set< std::set<index_t> >::iterator jline_it = elem_lineset.begin();
                jline_it != elem_lineset.end() && dest_vals[ei] == 0; ++jline_it)
            {
                const std::set<index_t> &jline = *jline_it;

                std::vector<index_t> ij_shared_points(2);
                std::vector<index_t>::iterator ij_shared_end = std::set_intersection(
                    iline.begin(), iline.end(), jline.begin(), jline.end(),
                    ij_shared_points.begin());

                // If there are no shared endpoints between the two input edges, then
                // we test for intersections.
                if(ij_shared_points.begin() == ij_shared_end)
                {
                    // Extract Coordinate Data //
                    index_t line_indices[2][2] = {{-1, -1}, {-1, -1}};
                    float64 line_starts[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
                    float64 line_ends[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
                    float64 line_vecs[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
                    for(index_t li = 0; li < 2; li++)
                    {
                        const std::set<index_t> &curr_line = (li == 0) ? iline : jline;
                        index_t *curr_indices = &line_indices[li][0];
                        float64 *curr_start = &line_starts[li][0];
                        float64 *curr_end = &line_ends[li][0];
                        float64 *curr_vec = &line_vecs[li][0];

                        curr_indices[0] = *(curr_line.begin());
                        curr_indices[1] = *(++curr_line.begin());

                        for(index_t di = 0; di < ELEM_TYPE_DIMS[type]; di++)
                        {
                            const Node &axis_coords = coords["values"][CSET_AXES[di]];
                            const DataType axis_dtype(axis_coords.dtype().id(), 1);

                            Node axis_data;
                            for(index_t ei = 0; ei < 2; ei++)
                            {
                                float64 *curr_point = (ei == 0) ? curr_start : curr_end;
                                axis_data.set_external(axis_dtype,
                                    (void*)axis_coords.element_ptr(curr_indices[ei]));
                                curr_point[di] = axis_data.to_float64();
                            }

                            curr_vec[di] = curr_end[di] - curr_start[di];
                        }
                    }

                    // Calculate Line Intersection //
                    float64 i2j_vec[3] = {0.0, 0.0, 0.0};
                    calc_vec_sub(&line_starts[1][0], &line_starts[0][0], &i2j_vec[0]);

                    float64 ixj_vec[3] = {0.0, 0.0, 0.0};
                    calc_vec_cross(&line_vecs[0][0], &line_vecs[1][0], &ixj_vec[0]);
                    float64 ixj_mag = calc_vec_mag(&i2j_vec[0]);
                    float64 i2jxi_vec[3] = {0.0, 0.0, 0.0};
                    calc_vec_cross(&i2j_vec[0], &line_vecs[0][0], &i2jxi_vec[0]);
                    float64 i2jxi_mag = calc_vec_mag(&i2jxi_vec[0]);
                    float64 i2jxj_vec[3] = {0.0, 0.0, 0.0};
                    calc_vec_cross(&i2j_vec[0], &line_vecs[1][0], &i2jxj_vec[0]);
                    float64 i2jxj_mag = calc_vec_mag(&i2jxj_vec[0]);

                    // NOTE: Evaluations based on algebraic derivations here:
                    // https://stackoverflow.com/a/565282
                    if(fuzzy_eq(ixj_mag, 0.0)) // parallel case
                    {
                        // colinear if true; parallel adjoint if false
                        dest_vals[ei] += fuzzy_eq(i2jxi_mag, 0.0);
                    }
                    else // non-parallel case
                    {
                        float64 iparam = i2jxj_mag / ixj_mag;
                        float64 jparam = i2jxi_mag / ixj_mag;

                        // intersect if line-line intersection exists in line
                        // segment parameter space of [0.0, 1.0] for both lines
                        dest_vals[ei] +=
                            (fuzzy_le(0.0, iparam) && fuzzy_le(iparam, 1.0)) &&
                            (fuzzy_le(0.0, jparam) && fuzzy_le(jparam, 1.0));
                    }
                }
            }
        }
    }
}

void calc_orientation_field(index_t type, const Node &topo, const Node &coords, Node &dest)
{
    // NOTE(JRC): This function assumes the existence of an offsets field in
    // the source topology, which is true for all callees in this test suite.
    // TODO(JRC): This currently is only capable of calculating the orientations
    // of 2D topologies.
    const Node &topo_conn = topo["elements/connectivity"];
    const Node &topo_off = topo["elements/offsets"];
    const DataType conn_dtype(topo_conn.dtype().id(), 1);
    const DataType off_dtype(topo_off.dtype().id(), 1);

    const bool is_topo_3d = type == ELEM_TYPE_TET_ID || type == ELEM_TYPE_HEX_ID;
    const index_t topo_num_elems = topo_off.dtype().number_of_elements();
    const float64 inf = std::numeric_limits<float64>::infinity();

    dest.reset();
    dest["association"].set("element");
    dest["volume_dependent"].set("false");
    dest["topology"].set(topo.name());
    dest["values"].set(DataType::float64(topo_num_elems));

    // TODO(JRC): Only need to poly if the incoming geometry is 3D; otherwise,
    // we are guaranteed that the input is end-to-end anyway

    // +1: RHR positive (CCW 2D, out-normal 3D)
    // -1: RHR negative (CW 2D, in-normal 3D)
    // 0: unoriented (inconsistent orientation)
    Node temp, data;
    float64_array dest_vals = dest["values"].as_float64_array();
    for(index_t ei = 0; ei < topo_num_elems; ei++)
    {
        std::vector< std::vector<int64> > elem_faces(ELEM_TYPE_FACES[type]);
        if(!is_topo_3d)
        {
            temp.set_external(off_dtype, (void*)topo_off.element_ptr(ei));
            const index_t estart = temp.to_index_t();
            const index_t elength = ELEM_TYPE_FACE_INDICES[type];

            elem_faces[0].resize(elength);
            data.set_external(DataType::int64(elength),
                elem_faces.back().data());
            temp.set_external(DataType(conn_dtype.id(), elength),
                (void*)topo_conn.element_ptr(estart));
            temp.to_data_type(DataType::int64(1).id(), data);
        }
        else // if(is_topo_3d)
        {
            // TODO(JRC): Construct polyhedral faces using 'subelements/connectivity'.
        }

        int elem_orientation = 0;
        for(index_t fi = 0; fi < (index_t)elem_faces.size(); fi++)
        {
            const std::vector<int64> &face_verts = elem_faces[fi];

            index_t min_vert_id = 0;
            index_t min_vert_index = 0;
            { // locate the min vertex (i.e. min(x, y, z))
                float64 min_vert[3] = {inf, inf, inf};
                float64 vert[3] = {0.0f, 0.0f, 0.0f};
                data.set_external(DataType::float64(3), &vert[0]);
                for(index_t vi = 0; vi < (index_t)face_verts.size(); vi++)
                {
                    query_point(coords, face_verts[vi], data);
                    for(index_t di = 0; di < ELEM_TYPE_DIMS[type] && vert[di] <= min_vert[di]; di++)
                    {
                        if(vert[di] < min_vert[di])
                        {
                            min_vert_id = face_verts[vi];
                            min_vert_index = vi;
                            std::memcpy(&min_vert[0], &vert[0], sizeof(min_vert));
                        }
                    }
                }
            }

            std::vector<index_t> min_vert_adjs;
            { // calculate adjacent vertices along the face
                // FIXME(JRC): The 'algorithm' below depends on the fact that
                // the face is oriented end-to-end wrt vertices (i.e. MFEM/VTK style).
                const index_t prev_vert_index = (min_vert_index == 0) ?
                    (face_verts.size() - 1) : (min_vert_index - 1);
                const index_t next_vert_index =
                    (min_vert_index + 1) % face_verts.size();

                min_vert_adjs.push_back(face_verts[prev_vert_index]);
                min_vert_adjs.push_back(face_verts[next_vert_index]);
            }

            int face_orientation = 0;
            { // calculate determinant in 2d and cross product in 3d
                float64 face_verts[3][3] = {
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f, 0.0f}
                };

                // a = min_vert_adjs[0]
                data.set_external(DataType::float64(3), &face_verts[0][0]);
                query_point(coords, min_vert_adjs[0], data);
                // b = min_vert_id
                data.set_external(DataType::float64(3), &face_verts[1][0]);
                query_point(coords, min_vert_id, data);
                // c = min_vert_adjs[1]
                data.set_external(DataType::float64(3), &face_verts[2][0]);
                query_point(coords, min_vert_adjs[1], data);

                // det = (xb - xa) * (yc - ya) - (xc - xa) * (yb - ya)
                float64 det =
                    (face_verts[1][0] - face_verts[0][0]) * (face_verts[2][1] - face_verts[0][1]) -
                    (face_verts[2][0] - face_verts[0][0]) * (face_verts[1][1] - face_verts[0][1]);
                face_orientation = ((det > 0) ? 1 : ((det < 0) ? -1 : 0));
            }

            if(fi == 0)
            {
                elem_orientation = face_orientation;
            }
            else if(elem_orientation != 0 && elem_orientation != face_orientation)
            {
                elem_orientation = 0;
            }
        }

        dest_vals[ei] = static_cast<float64>(elem_orientation);
    }
}

void calc_volume_field(index_t type, const Node &topo, const Node &coords, Node &dest)
{
    // NOTE(JRC): This function assumes the existence of an offsets field in
    // the source topology, which is true for all callees in this test suite.
    // TODO(JRC): This currently is only capable of calculating the hypervolume
    // of 2D topologies.
    const Node &topo_conn = topo["elements/connectivity"];
    const Node &topo_off = topo["elements/offsets"];
    const DataType conn_dtype(topo_conn.dtype().id(), 1);
    const DataType off_dtype(topo_off.dtype().id(), 1);
    const bool topo_is_poly = topo["elements/shape"].as_string() == "polygonal";
    const index_t topo_num_elems = topo_off.dtype().number_of_elements();

    dest.reset();
    dest["association"].set("element");
    dest["volume_dependent"].set("false");
    dest["topology"].set(topo.name());
    dest["values"].set(DataType::float64(topo_num_elems));

    Node data_node;

    float64_array dest_vals = dest["values"].as_float64_array();
    for(index_t ei = 0; ei < topo_num_elems; ei++)
    {
        data_node.set_external(off_dtype, (void*)topo_off.element_ptr(ei));
        index_t elem_start_index = data_node.to_int64();
        data_node.set_external(off_dtype, (void*)topo_off.element_ptr(ei+1));
        index_t elem_end_index = (ei < topo_num_elems - 1) ?
            data_node.to_int64() : topo_conn.dtype().number_of_elements();
        index_t elem_size = elem_end_index - elem_start_index;

        // NOTE(JRC): The polygonal volume calculation in this function is derived
        // from the "Shoelace Formula" (see: https://en.wikipedia.org/wiki/Shoelace_formula).
        dest_vals[ei] = 0.0;
        for(index_t eci = 0; eci < elem_size; eci++)
        {
            float64 coord_vals[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
            for(index_t cdi = 0; cdi < 2; cdi++)
            {
                index_t ci = ((eci + cdi) % elem_size) + elem_start_index;
                data_node.set_external(conn_dtype, (void*)topo_conn.element_ptr(ci));
                index_t icoord = data_node.to_int64();

                data_node.set_external(DataType::float64(3), &coord_vals[cdi][0]);
                query_point(coords, icoord, data_node);
            }

            dest_vals[ei] += coord_vals[0][0] * coord_vals[1][1];
            dest_vals[ei] -= coord_vals[0][1] * coord_vals[1][0];
        }
        dest_vals[ei] = std::abs(dest_vals[ei] / 2.0);
    }
}

/// Test Cases ///

// NOTE(JRC): This strategy of populating a list of grid collections on demand
// was adopted because using a suite of statically-defined presets causes a
// "static initialization order fiasco" with the static variables in the
// "conduit_blueprint.cpp" source file (see: https://isocpp.org/wiki/faq/ctors#static-init-order).
// TODO(JRC): If the test suite is ever extended to support parallel testing,
// this function needs to be wrapped in a mutex.
const GridMeshCollection &get_test_grids(const index_t *npts, const bool debug=false)
{
    static std::map<std::pair<GridDims, bool>, const GridMeshCollection> dims_grids_map;

    std::pair<GridDims, bool> dims_id = std::make_pair(GridDims(npts), debug);
    if(dims_grids_map.find(dims_id) == dims_grids_map.end())
    {
        dims_grids_map.insert(std::pair<std::pair<GridDims, bool>, const GridMeshCollection>(
            dims_id, GridMeshCollection(npts, debug)));
    }

    return dims_grids_map.find(dims_id)->second;
}

typedef GridMeshCollection::Iterator GridIterator;

// //-----------------------------------------------------------------------------
// TEST(conduit_blueprint_generate_unstructured, generate_cascade)
// {
//     // NOTE(JRC): This is an unused test case that can be implemented in order
//     // to help debug the current ordering being used for the topological cascade.
//     const GridMeshCollection &grids = get_test_grids(SIMPLE_GRID, true);
//     for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
//     {
//         GridMesh grid_mesh = *grid_it;
//         Node &grid_coords = grid_mesh.mesh["coordsets"].child(0);
//         Node &grid_topo = grid_mesh.mesh["topologies"].child(0);
// 
//         Node s2t_map, t2s_map;
//         Node &point_topo = grid_mesh.mesh["topologies"]["points"];
//         mesh::topology::unstructured::generate_points(grid_topo, point_topo, s2t_map, t2s_map);
//         Node &line_topo = grid_mesh.mesh["topologies"]["lines"];
//         mesh::topology::unstructured::generate_lines(grid_topo, line_topo, s2t_map, t2s_map);
//         Node &face_topo = grid_mesh.mesh["topologies"]["faces"];
//         mesh::topology::unstructured::generate_faces(grid_topo, face_topo, s2t_map, t2s_map);
// 
//         Node &side_coords = grid_mesh.mesh["coordsets"]["sides"];
//         Node &side_topo = grid_mesh.mesh["topologies"]["sides"];
//         mesh::topology::unstructured::generate_sides(grid_topo, side_topo, side_coords, s2t_map, t2s_map);
//         Node &corner_coords = grid_mesh.mesh["coordsets"]["corners"];
//         Node &corner_topo = grid_mesh.mesh["topologies"]["corners"];
//         mesh::topology::unstructured::generate_corners(grid_topo, corner_topo, corner_coords, s2t_map, t2s_map);
// 
//         grid_mesh.mesh.remove("fields");
//         grid_mesh.mesh.remove("state");
// 
//         calc_orientation_field(grid_mesh.type, grid_topo, grid_coords, grid_mesh.mesh["fields"]["orientations"]);
//         calc_volume_field(grid_mesh.type, grid_topo, grid_coords, grid_mesh.mesh["fields"]["volumes"]);
// 
//         grid_mesh.mesh.print();
//     }
// }

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_offsets_nonpoly)
{
    const GridMeshCollection &grids = get_test_grids(SIMPLE_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        // NOTE(JRC): We separate polytopal and non-polytopal topologies in
        // testing to make attribution and problem tracing easier.
        if(grid_it->is_poly) { continue; }

        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);
        const Node &grid_conn = grid_topo["elements/connectivity"];

        Node grid_offsets;
        mesh::topology::unstructured::generate_offsets(grid_topo, grid_offsets);
        const DataType offset_dtype = grid_offsets.dtype();
        
        // relax exact type req, conn transforms will become index_t
        //EXPECT_EQ(offset_dtype.id(), grid_conn.dtype().id());
        EXPECT_EQ(offset_dtype.number_of_elements(), grid_mesh.elems());

        Node expected_offsets_int64(DataType::int64(grid_mesh.elems()));
        int64_array expected_offsets_data = expected_offsets_int64.as_int64_array();
        for(index_t oi = 0; oi < offset_dtype.number_of_elements(); oi++)
        {
            expected_offsets_data[oi] = oi * grid_mesh.points_per_elem();
        }
        Node expected_offsets;
        expected_offsets_int64.to_data_type(offset_dtype.id(), expected_offsets);

        Node info;
        EXPECT_FALSE(grid_offsets.diff(expected_offsets, info));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_offsets_poly)
{
    const GridMeshCollection &grids = get_test_grids(SIMPLE_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        // NOTE(JRC): We separate polytopal and non-polytopal topologies in
        // testing to make attribution and problem tracing easier.
        if(!grid_it->is_poly) { continue; }

        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);
        const Node &grid_conn = grid_topo["elements/connectivity"];

        Node grid_offsets;
        mesh::topology::unstructured::generate_offsets(grid_topo, grid_offsets);
        const DataType offset_dtype = grid_offsets.dtype();
        // relax exact type req, conn transforms will become index_t
        // EXPECT_EQ(offset_dtype.id(), grid_conn.dtype().id());
        EXPECT_EQ(offset_dtype.number_of_elements(), grid_mesh.elems());

        Node expected_offsets_int64(DataType::int64(grid_mesh.elems()));
        int64_array expected_offsets_data = expected_offsets_int64.as_int64_array();
        for(index_t oi = 0; oi < offset_dtype.number_of_elements(); oi++)
        {
            expected_offsets_data[oi] = oi * (grid_mesh.dims() == 3 ? 
                grid_mesh.faces_per_elem() : grid_mesh.points_per_face());
        }
        Node expected_offsets;
        expected_offsets_int64.to_data_type(offset_dtype.id(), expected_offsets);

        Node info;
        EXPECT_FALSE(grid_offsets.diff(expected_offsets, info));
    }
}

//-----------------------------------------------------------------------------
void
make_ph_topo(conduit::Node &topo, bool add_offsets, bool add_sub_offsets)
{
    std::vector<int> conn{0, 1, 2, 3, 4, 5,
                          2, 6, 7, 8, 9, 10,
                          11, 3, 12, 13, 14, 15,
                          12, 8, 16, 17, 18, 19,
                          20, 21, 22, 23, 5, 24,
                          22, 25, 26, 27, 10, 28,
                          29, 23, 30, 31, 15, 32,
                          30, 27, 33, 34, 19, 35};
    std::vector<int> sizes{6, 6, 6, 6, 6, 6, 6, 6};
    std::vector<int> offsets{0, 6, 12, 18, 24, 30, 36, 42};

    std::vector<int> subconn{0, 9, 12, 3,
                             0, 1, 10, 9,
                             1, 4, 13, 10,
                             3, 12, 13, 4,
                             0, 3, 4, 1,
                             9, 10, 13, 12,
                             1, 2, 11, 10,
                             2, 5, 14, 11,
                             4, 13, 14, 5,
                             1, 4, 5, 2,
                             10, 11, 14, 13,
                             3, 12, 15, 6,
                             4, 7, 16, 13,
                             6, 15, 16, 7,
                             3, 6, 7, 4,
                             12, 13, 16, 15,
                             5, 8, 17, 14,
                             7, 16, 17, 8,
                             4, 7, 8, 5,
                             13, 14, 17, 16,
                             9, 18, 21, 12,
                             9, 10, 19, 18,
                             10, 13, 22, 19,
                             12, 21, 22, 13,
                             18, 19, 22, 21,
                             10, 11, 20, 19,
                             11, 14, 23, 20,
                             13, 22, 23, 14,
                             19, 20, 23, 22,
                             12, 21, 24, 15,
                             13, 16, 25, 22,
                             15, 24, 25, 16,
                             21, 22, 25, 24,
                             14, 17, 26, 23,
                             16, 25, 26, 17,
                             22, 23, 26, 25};
    std::vector<int> subsizes{4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                              4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                              4, 4, 4, 4};
    std::vector<int> suboffsets{0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44,
                                48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88,
                                92, 96, 100, 104, 108, 112, 116, 120, 124,
                                128, 132, 136, 140};

    topo["coordset"] = "coords";
    topo["type"] = "unstructured";

    topo["elements/shape"] = "polyhedral";
    topo["elements/connectivity"].set(conn);
    topo["elements/sizes"].set(sizes);
    if(add_offsets)
        topo["elements/offsets"].set(offsets);

    topo["subelements/shape"] = "polygonal";
    topo["subelements/connectivity"].set(subconn);
    topo["subelements/sizes"].set(subsizes);
    if(add_sub_offsets)
        topo["subelements/offsets"].set(suboffsets);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_offsets_types)
{
    // Make a PH topo that has element/offsets and subelement/offsets. The
    // types should not change.
    { // new scope
        conduit::Node topo;
        make_ph_topo(topo, true, true);
        auto &offsets = topo["elements/offsets"];
        auto t0 = offsets.dtype().id();
        auto st0 = topo["subelements/offsets"].dtype().id();
        conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(topo, offsets);
        auto t1 = topo["elements/offsets"].dtype().id();
        auto st1 = topo["subelements/offsets"].dtype().id();
        // The types better be equal
        EXPECT_EQ(t0, t1);
        EXPECT_EQ(st0, st1);
    }

    // Make a PH topo with no offsets.
    { // new scope
        conduit::Node topo;
        make_ph_topo(topo, false, false);
        auto &offsets = topo["elements/offsets"];
        conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(topo, offsets);
        EXPECT_EQ(topo["elements"].has_child("offsets"), true);
    }

    // Make a PH topo with element/offsets.
    { // new scope
        conduit::Node topo;
        make_ph_topo(topo, true, false);
        auto &offsets = topo["elements/offsets"];
        auto t0 = offsets.dtype().id();
        conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(topo, offsets);
        EXPECT_EQ(topo["elements"].has_child("offsets"), true);
        auto t1 = topo["elements/offsets"].dtype().id();
        // The types better be equal
        EXPECT_EQ(t0, t1);
    }

    // Make a PH topo with subelements/offsets. Make sure we generate elements/offsets.
    { // new scope
        conduit::Node topo;
        make_ph_topo(topo, false, true);
        auto &offsets = topo["elements/offsets"];
        auto st0 = topo["subelements/offsets"].dtype().id();
        conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(topo, offsets);
        EXPECT_EQ(topo["elements"].has_child("offsets"), true);
        EXPECT_EQ(topo["subelements"].has_child("offsets"), true);
        auto st1 = topo["subelements/offsets"].dtype().id();
        // The types better be equal
        EXPECT_EQ(st0, st1);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_centroids)
{
    const std::string CENTROID_COORDSET_NAME = "ccoords";
    const std::string CENTROID_TOPOLOGY_NAME = "ctopo";

    const GridMeshCollection &grids = get_test_grids(SIMPLE_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_coords = grid_mesh.mesh["coordsets"].child(0);
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);

        Node cent_mesh, t2c_map, c2t_map;
        Node &cent_coords = cent_mesh["coordsets"][CENTROID_COORDSET_NAME];
        Node &cent_topo = cent_mesh["topologies"][CENTROID_TOPOLOGY_NAME];
        mesh::topology::unstructured::generate_centroids(
            grid_topo, cent_topo, cent_coords, t2c_map, c2t_map);

        Node data, info;
        EXPECT_TRUE(mesh::coordset::_explicit::verify(cent_coords, info));
        EXPECT_TRUE(mesh::topology::unstructured::verify(cent_topo, info));

        // Verify Correctness of Coordset //

        for(index_t ci = 0; ci < grid_mesh.dims(); ci++)
        {
            const std::string &coord_axis = CSET_AXES[ci];
            EXPECT_TRUE(cent_coords["values"].has_child(coord_axis));

            const Node &grid_axis = grid_coords["values"][coord_axis];
            Node &cent_axis = cent_coords["values"][coord_axis];

            EXPECT_EQ(cent_axis.dtype().id(), grid_axis.dtype().id());
            EXPECT_EQ(cent_axis.dtype().number_of_elements(), grid_mesh.elems());
        }

        // Verify Correctness of Topology //

        const Node &grid_conn = grid_topo["elements/connectivity"];
        Node &cent_conn = cent_topo["elements/connectivity"];

        EXPECT_EQ(cent_topo["coordset"].as_string(), CENTROID_COORDSET_NAME);
        // relax exact type req, conn transforms will become index_t
        //EXPECT_EQ(cent_conn.dtype().id(), grid_conn.dtype().id());
        EXPECT_EQ(cent_conn.dtype().number_of_elements(), grid_mesh.elems());

        // Verify Data Integrity //

        // TODO(JRC): Extend this test case to validate that each centroid is
        // contained within the convex hull of its source element.

        // Verify Correctness of Mappings //

        conduit::Node* map_nodes[2] = { &t2c_map, &c2t_map };
        for(index_t mi = 0; mi < 2; mi++)
        {
            conduit::Node& map_node = *map_nodes[mi];
            // relax exact type req, conn transforms will become index_t
            //EXPECT_EQ(map_node.dtype().id(), grid_conn.dtype().id());
            EXPECT_EQ(map_node.dtype().number_of_elements(), 2 * grid_mesh.elems());

            std::vector<index_t> map_values, expected_values;
            for(index_t ei = 0; ei < grid_mesh.elems(); ei++)
            {
                for(index_t esi = 0; esi < 2; esi++)
                {
                    data.set_external(DataType(map_node.dtype().id(), 1),
                        map_node.element_ptr(2 * ei + esi));
                    map_values.push_back(data.to_int64());
                }

                expected_values.push_back(1);
                expected_values.push_back(ei);
            }
            EXPECT_EQ(map_values, expected_values);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_points)
{
    const std::string POINT_TOPOLOGY_NAME = "ptopo";

    const GridMeshCollection &grids = get_test_grids(COMPLEX_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_coords = grid_mesh.mesh["coordsets"].child(0);
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);

        const index_t grid_points = grid_mesh.points();

        Node point_mesh, t2p_map, p2t_map;
        Node &point_coords = point_mesh["coordsets"][grid_coords.name()];
        point_coords.set_external(grid_coords);

        // Verify Correctness of Topology //

        Node &point_topo = point_mesh["topologies"][POINT_TOPOLOGY_NAME];
        mesh::topology::unstructured::generate_points(grid_topo, point_topo, t2p_map, p2t_map);

        Node data, info;
        EXPECT_TRUE(mesh::topology::unstructured::verify(point_topo, info));

        // General Data/Schema Checks //

        EXPECT_EQ(point_topo["coordset"].as_string(), grid_coords.name());
        EXPECT_EQ(point_topo["elements/shape"].as_string(), "point");

        const Node &grid_conn = grid_topo["elements/connectivity"];
        Node &point_conn = point_topo["elements/connectivity"];

        // relax exact type req, conn transforms will become index_t
        // EXPECT_EQ(point_conn.dtype().id(), grid_conn.dtype().id());
        EXPECT_EQ(point_conn.dtype().number_of_elements(), grid_points);

        // Content Consistency Checks //

        std::set<index_t> actual_conn_set, expected_conn_set;
        for(index_t pi = 0; pi < grid_points; pi++)
        {
            data.set_external(DataType(point_conn.dtype().id(), 1),
                point_conn.element_ptr(pi));
            actual_conn_set.insert(data.to_int64());

            expected_conn_set.insert(pi);
        }

        EXPECT_EQ(actual_conn_set, expected_conn_set);

        // Verify Correctness of Mappings //

        // NOTE(JRC): Skip testing for tetrahedral meshes because their element
        // interfaces are complicated and make counting too difficult.
        if(grid_it->type == ELEM_TYPE_TET_ID) { continue; }

        for(index_t mi = 0; mi < 2; mi++)
        {
            const conduit::Node& map_node = (mi == 0) ? t2p_map : p2t_map;
            EXPECT_TRUE(o2mrelation::verify(map_node, info));
            for(const std::string &o2m_path : O2M_PATHS)
            {
                EXPECT_TRUE(map_node.has_child(o2m_path));
            }
        }

        EXPECT_EQ(t2p_map["values"].dtype().number_of_elements(),
            grid_mesh.points_per_elem() * grid_mesh.elems());
        EXPECT_EQ(t2p_map["sizes"].dtype().number_of_elements(),
            grid_mesh.elems());

        EXPECT_EQ(p2t_map["values"].dtype().number_of_elements(),
            grid_mesh.elem_valence(0));
        EXPECT_EQ(p2t_map["sizes"].dtype().number_of_elements(),
            grid_mesh.points());

        // TODO(JRC): It's currently possible, albeit very annoying, to do a
        // reasonable check of the points against the initial topology to make
        // sure they're correct; this should be done at some point.
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_lines)
{
    const std::string LINE_TOPOLOGY_NAME = "ltopo";

    const GridMeshCollection &grids = get_test_grids(COMPLEX_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        // NOTE(JRC): Skip testing for tetrahedral meshes because their element
        // interfaces are complicated and make counting too difficult.
        if(grid_it->type == ELEM_TYPE_TET_ID) { continue; }

        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_coords = grid_mesh.mesh["coordsets"].child(0);
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);

        Node line_mesh, t2l_map, l2t_map;
        Node &line_coords = line_mesh["coordsets"][grid_coords.name()];
        line_coords.set_external(grid_coords);

        // Verify Correctness of Topology //

        Node &line_topo = line_mesh["topologies"][LINE_TOPOLOGY_NAME];
        mesh::topology::unstructured::generate_lines(grid_topo, line_topo, t2l_map, l2t_map);

        Node info;
        EXPECT_TRUE(mesh::topology::unstructured::verify(line_topo, info));

        // General Data/Schema Checks //

        EXPECT_EQ(line_topo["coordset"].as_string(), grid_coords.name());
        EXPECT_EQ(line_topo["elements/shape"].as_string(), "line");

        const Node &grid_conn = grid_topo["elements/connectivity"];
        Node &line_conn = line_topo["elements/connectivity"];

        // relax exact type req, conn transforms will become index_t
        // EXPECT_EQ(line_conn.dtype().id(), grid_conn.dtype().id());
        EXPECT_EQ(line_conn.dtype().number_of_elements(), 2 * grid_mesh.lines());

        // Content Consistency Checks //

        // TODO(JRC): Extend this test case so that it more thoroughly checks
        // the contents of the unique line mesh.

        // Verify Correctness of Mappings //

        for(index_t mi = 0; mi < 2; mi++)
        {
            const conduit::Node& map_node = (mi == 0) ? t2l_map : l2t_map;
            EXPECT_TRUE(o2mrelation::verify(map_node, info));
            for(const std::string &o2m_path : O2M_PATHS)
            {
                EXPECT_TRUE(map_node.has_child(o2m_path));
            }
        }

        EXPECT_EQ(t2l_map["values"].dtype().number_of_elements(),
            grid_mesh.lines_per_elem() * grid_mesh.elems());
        EXPECT_EQ(t2l_map["sizes"].dtype().number_of_elements(),
            grid_mesh.elems());

        EXPECT_EQ(l2t_map["values"].dtype().number_of_elements(),
            grid_mesh.elem_valence(1));
        EXPECT_EQ(l2t_map["sizes"].dtype().number_of_elements(),
            grid_mesh.lines());

        // TODO(JRC): If consistency checks are in place, extend those checks
        // to validate that the contents of the maps are correct.
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_faces)
{
#ifdef USE_ERROR_HANDLER
    conduit::utils::set_error_handler(tmp_err_handler);
#endif

    const std::string FACE_TOPOLOGY_NAME = "ftopo";

    const GridMeshCollection &grids = get_test_grids(COMPLEX_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        // NOTE(JRC): Skip testing for tetrahedral meshes because their element
        // interfaces are complicated and make counting too difficult.
        if(grid_it->type == ELEM_TYPE_TET_ID) { continue; }

        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_coords = grid_mesh.mesh["coordsets"].child(0);
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);

        const index_t grid_faces = grid_mesh.faces();
        const std::string face_tstr = ELEM_TYPE_LIST[
            (grid_mesh.dims() == 2) ? grid_mesh.type : grid_mesh.type - 2];
        const std::string face_type = grid_mesh.is_poly ?
            "polygonal" : face_tstr.substr(0, face_tstr.size() - 1);

        Node face_mesh, t2f_map, f2t_map;
        Node &face_coords = face_mesh["coordsets"][grid_coords.name()];
        face_coords.set_external(grid_coords);

        // Verify Correctness of Topology //

        Node &face_topo = face_mesh["topologies"][FACE_TOPOLOGY_NAME];
        mesh::topology::unstructured::generate_faces(grid_topo, face_topo, t2f_map, f2t_map);

        Node info;
        EXPECT_TRUE(mesh::topology::unstructured::verify(face_topo, info));

        // General Data/Schema Checks //

        const Node &grid_conn = grid_topo["elements/connectivity"];
        Node &face_conn = face_topo["elements/connectivity"];
        Node &face_off = face_topo["elements/offsets"];
        mesh::topology::unstructured::generate_offsets(face_topo, face_off);

        EXPECT_EQ(face_topo["coordset"].as_string(), grid_coords.name());
        if(grid_mesh.is_poly)
        {
            // NOTE: In the newer TopologyMetadata, meshes where all PH faces are
            //       
            std::string shape(face_topo["elements/shape"].as_string());
            std::string ft(face_tstr.substr(0, face_tstr.size() - 1));
            bool ftsame = shape == "polygonal" || shape == ft;
            EXPECT_EQ(ftsame, true);
        }
        else
        {
            EXPECT_EQ(face_topo["elements/shape"].as_string(), face_type);
        }
        // relax exact type req, conn transforms will become index_t
        // EXPECT_EQ(face_conn.dtype().id(), grid_conn.dtype().id());
        EXPECT_EQ(face_off.dtype().number_of_elements(), grid_mesh.faces());

        // Content Consistency Checks //

        // TODO(JRC): Extend this test case so that it more thoroughly checks
        // the contents of the unique face mesh.

        // Verify Correctness of Mappings //

        for(index_t mi = 0; mi < 2; mi++)
        {
            const conduit::Node& map_node = (mi == 0) ? t2f_map : f2t_map;
            EXPECT_TRUE(o2mrelation::verify(map_node, info));
            for(const std::string &o2m_path : O2M_PATHS)
            {
                EXPECT_TRUE(map_node.has_child(o2m_path));
            }
        }

        EXPECT_EQ(t2f_map["values"].dtype().number_of_elements(),
            grid_mesh.faces_per_elem() * grid_mesh.elems());
        EXPECT_EQ(t2f_map["sizes"].dtype().number_of_elements(),
            grid_mesh.elems());

        EXPECT_EQ(f2t_map["values"].dtype().number_of_elements(),
            grid_mesh.elem_valence(2));
        EXPECT_EQ(f2t_map["sizes"].dtype().number_of_elements(),
            grid_mesh.faces());

        // TODO(JRC): If consistency checks are in place, extend those checks
        // to validate that the contents of the maps are correct.
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_sides)
{
    const std::string SIDE_COORDSET_NAME = "scoords";
    const std::string SIDE_TOPOLOGY_NAME = "stopo";
    const std::string SIDE_FIELD_NAME = "sfield";

    const GridMeshCollection &grids = get_test_grids(COMPLEX_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        // NOTE(JRC): Skip testing for tetrahedral meshes because their element
        // interfaces are complicated and make counting too difficult.
        if(grid_it->type == ELEM_TYPE_TET_ID) { continue; }

        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_coords = grid_mesh.mesh["coordsets"].child(0);
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);

        const index_t grid_elems = grid_mesh.elems();
        const index_t grid_faces = grid_mesh.faces();

        const index_t side_type = (grid_mesh.dims() == 2) ? ELEM_TYPE_TRI_ID : ELEM_TYPE_TET_ID;
        const std::string side_type_name = (grid_mesh.dims() == 2) ? "tri" : "tet";
        const index_t sides_per_elem = grid_mesh.faces_per_elem() * grid_mesh.points_per_face();
        const index_t grid_sides = grid_elems * sides_per_elem;
        const float64 side_volume = grid_mesh.elem_volume() / sides_per_elem;

        Node side_mesh, t2s_map, s2t_map;
        Node &side_coords = side_mesh["coordsets"][SIDE_COORDSET_NAME];
        Node &side_topo = side_mesh["topologies"][SIDE_TOPOLOGY_NAME];
        mesh::topology::unstructured::generate_sides(
            grid_topo, side_topo, side_coords, t2s_map, s2t_map);

        Node info;
        EXPECT_TRUE(mesh::coordset::_explicit::verify(side_coords, info));
        EXPECT_TRUE(mesh::topology::unstructured::verify(side_topo, info));

        // Verify Correctness of Coordset //

        for(index_t ci = 0; ci < grid_mesh.dims(); ci++)
        {
            const std::string &coord_axis = CSET_AXES[ci];
            EXPECT_TRUE(side_coords["values"].has_child(coord_axis));

            const Node &grid_axis = grid_coords["values"][coord_axis];
            Node &side_axis = side_coords["values"][coord_axis];

            EXPECT_EQ(side_axis.dtype().id(), grid_axis.dtype().id());
            EXPECT_EQ(side_axis.dtype().number_of_elements(),
                grid_axis.dtype().number_of_elements() +
                grid_elems + (grid_mesh.dims() == 3) * grid_faces);
        }

        // Verify Correctness of Topology //

        const Node &grid_conn = grid_topo["elements/connectivity"];
        Node &side_conn = side_topo["elements/connectivity"];
        Node &side_off = side_topo["elements/offsets"];
        mesh::topology::unstructured::generate_offsets(side_topo, side_off);

        EXPECT_EQ(side_topo["coordset"].as_string(), SIDE_COORDSET_NAME);
        EXPECT_EQ(side_topo["elements/shape"].as_string(), side_type_name);
        // relax exact type req, conn transforms will become index_t
        // EXPECT_EQ(side_conn.dtype().id(), grid_conn.dtype().id());
        EXPECT_EQ(side_off.dtype().number_of_elements(), grid_sides);

        // Validate Correctness of Element Integrity //

        if(grid_mesh.dims() < 3)
        {
            Node side_vols;
            calc_volume_field(side_type, side_topo, side_coords, side_vols);

            std::vector<float64> expected_vols_vec(grid_sides, side_volume);
            Node expected_vols(DataType::float64(expected_vols_vec.size()),
                &expected_vols_vec[0], true);

            EXPECT_FALSE(side_vols["values"].diff(expected_vols, info));
        }

        // Verify Correctness of Mappings //

        for(index_t mi = 0; mi < 2; mi++)
        {
            const conduit::Node& map_node = (mi == 0) ? t2s_map : s2t_map;
            EXPECT_TRUE(o2mrelation::verify(map_node, info));
            for(const std::string &o2m_path : O2M_PATHS)
            {
                EXPECT_TRUE(map_node.has_child(o2m_path));
            }
        }

        // NOTE(JRC): In 3D, each side propogates internally to each hex twice
        // in order to cover both attached faces.
        const index_t vfactor = (grid_mesh.dims() < 3) ? 1 : 2;

        EXPECT_EQ(t2s_map["values"].dtype().number_of_elements(),
            vfactor * grid_mesh.elem_valence(1));
        EXPECT_EQ(t2s_map["sizes"].dtype().number_of_elements(),
            grid_mesh.elems());

        EXPECT_EQ(s2t_map["values"].dtype().number_of_elements(),
            vfactor * grid_mesh.elem_valence(1));
        EXPECT_EQ(s2t_map["sizes"].dtype().number_of_elements(),
            vfactor * grid_mesh.elem_valence(1));

        std::vector<index_list> expected_elem_side_lists;
        for(index_t ei = 0, si = 0; ei < grid_mesh.elems(); ei++)
        {
            std::vector<index_t> expected_elem_sides;
            for(index_t esi = 0; esi < vfactor * grid_mesh.lines_per_elem(); esi++, si++)
            {
                expected_elem_sides.push_back(si);
            }
            expected_elem_side_lists.push_back(expected_elem_sides);
        }
        std::vector<index_list> actual_elem_side_lists = expand_o2mrelation(t2s_map);
        EXPECT_EQ(actual_elem_side_lists, expected_elem_side_lists);

        std::vector<index_list> expected_side_elem_lists;
        for(index_t ei = 0, si = 0; ei < grid_mesh.elems(); ei++)
        {
            for(index_t esi = 0; esi < vfactor * grid_mesh.lines_per_elem(); esi++, si++)
            {
                index_list expected_side_elems;
                expected_side_elems.push_back(ei);
                expected_side_elem_lists.push_back(expected_side_elems);
            }
        }
        std::vector<index_list> actual_side_elem_lists = expand_o2mrelation(s2t_map);
        EXPECT_EQ(actual_side_elem_lists, expected_side_elem_lists);

        // Verify Correctness of Element Orientations //

        if(grid_mesh.dims() < 3)
        {
            Node grid_orients, side_orients;
            calc_orientation_field(grid_mesh.type, grid_topo, grid_coords, grid_orients);
            calc_orientation_field(side_type, side_topo, side_coords, side_orients);

            std::vector<float64> expected_orients_vec(grid_sides);
            for(index_t ei = 0; ei < grid_mesh.elems(); ei++)
            {
                Node data(DataType::float64(1), grid_orients["values"].element_ptr(ei), true);
                const float64 elem_orient = data.to_float64();
                for(index_t si = 0; si < sides_per_elem; si++)
                {
                    expected_orients_vec[sides_per_elem * ei + si] = elem_orient;
                }
            }

            Node expected_orients(DataType::float64(expected_orients_vec.size()),
                expected_orients_vec.data(), true);
            EXPECT_FALSE(side_orients["values"].diff(expected_orients, info));
        }
    }
}

//-----------------------------------------------------------------------------
void
test_save_mesh_helper(const conduit::Node &dsets,
                      const std::string &base_name)
{
    Node opts;
    opts["file_style"] = "root_only";
    opts["suffix"] = "none";

    relay::io::blueprint::save_mesh(dsets, base_name + "_yaml", "yaml", opts);
}

TEST(conduit_blueprint_generate_structured, gen_corners)
{
    Node mesh;
    const index_t nx = 2;
    const index_t ny = 3;
    const index_t nz = 0;

    mesh::examples::basic("quads", nx, ny, nz, mesh);

    test_save_mesh_helper(mesh, "before_corners");

    Node corner_mesh, t2c_map, c2t_map;
    Node & topo = mesh["topologies/mesh"];
    Node & corner_coords = corner_mesh["coordsets/ccoords"];
    Node & corner_topo = corner_mesh["topologies/ctopo"];

    mesh::topology::unstructured::generate_corners(
        topo, corner_topo, corner_coords, t2c_map, c2t_map);

    test_save_mesh_helper(corner_mesh, "after_corners");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate_unstructured, generate_corners)
{
    const std::string CORNER_COORDSET_NAME = "ccoords";
    const std::string CORNER_TOPOLOGY_NAME = "ctopo";
    const std::string CORNER_FIELD_NAME = "cfield";

    const GridMeshCollection &grids = get_test_grids(COMPLEX_GRID);
    for(GridIterator grid_it = grids.begin(); grid_it != grids.end(); ++grid_it)
    {
        // NOTE(JRC): Skip testing for tetrahedral meshes because their element
        // interfaces are complicated and make counting too difficult.
        if(grid_it->type == 2) { continue; }

        const GridMesh &grid_mesh = *grid_it;
        const Node &grid_coords = grid_mesh.mesh["coordsets"].child(0);
        const Node &grid_topo = grid_mesh.mesh["topologies"].child(0);

        const index_t grid_elems = grid_mesh.elems();
        const index_t grid_faces = grid_mesh.faces();
        const index_t grid_lines = grid_mesh.lines();

        const index_t corner_type = (grid_mesh.dims() == 2) ? ELEM_TYPE_QUAD_ID : ELEM_TYPE_HEX_ID;
        const std::string corner_type_name = (grid_mesh.dims() == 2) ? "polygonal" : "polyhedral";
        const index_t corners_per_elem = grid_mesh.points_per_elem();
        const index_t grid_corners = grid_elems * corners_per_elem;
        const float64 corner_volume = grid_mesh.elem_volume() / corners_per_elem;

        test_save_mesh_helper(grid_mesh.mesh, "mesh_before_corners");

        Node corner_mesh, t2c_map, c2t_map;
        Node &corner_coords = corner_mesh["coordsets"][CORNER_COORDSET_NAME];
        Node &corner_topo = corner_mesh["topologies"][CORNER_TOPOLOGY_NAME];
        mesh::topology::unstructured::generate_corners(
            grid_topo, corner_topo, corner_coords, t2c_map, c2t_map);

        test_save_mesh_helper(grid_mesh.mesh, "mesh_with_corners");

        Node info;
        EXPECT_TRUE(mesh::coordset::_explicit::verify(corner_coords, info));
        EXPECT_TRUE(mesh::topology::unstructured::verify(corner_topo, info));

        // Verify Correctness of Coordset //

        for(index_t ci = 0; ci < grid_mesh.dims(); ci++)
        {
            const std::string &coord_axis = CSET_AXES[ci];
            EXPECT_TRUE(corner_coords["values"].has_child(coord_axis));

            const Node &grid_axis = grid_coords["values"][coord_axis];
            Node &corner_axis = corner_coords["values"][coord_axis];

            EXPECT_EQ(corner_axis.dtype().id(), grid_axis.dtype().id());
            EXPECT_EQ(corner_axis.dtype().number_of_elements(),
                grid_axis.dtype().number_of_elements() +
                grid_elems + (grid_mesh.dims() == 3) * grid_faces + grid_lines);
        }

        // Verify Correctness of Topology //

        const Node &grid_conn = grid_topo["elements/connectivity"];
        Node &corner_conn = corner_topo["elements/connectivity"];
        Node &corner_off = corner_topo["elements/offsets"];
        mesh::topology::unstructured::generate_offsets(corner_topo, corner_off);

        EXPECT_EQ(corner_topo["coordset"].as_string(), CORNER_COORDSET_NAME);
        EXPECT_EQ(corner_topo["elements/shape"].as_string(), corner_type_name);
        // relax exact type req, conn transforms will become index_t
        // EXPECT_EQ(corner_conn.dtype().id(), grid_conn.dtype().id());
        EXPECT_EQ(corner_off.dtype().number_of_elements(), grid_corners);

        // Validate Correctness of Element Integrity //

        if(grid_mesh.dims() < 3)
        {
            Node corner_vols;
            calc_volume_field(corner_type, corner_topo, corner_coords, corner_vols);

            std::vector<float64> expected_vols_vec(grid_corners, corner_volume);
            Node expected_vols(DataType::float64(expected_vols_vec.size()),
                &expected_vols_vec[0], true);

            EXPECT_FALSE(corner_vols["values"].diff(expected_vols, info));
        }

        // Verify Correctness of Mappings //

        for(index_t mi = 0; mi < 2; mi++)
        {
            const conduit::Node& map_node = (mi == 0) ? t2c_map : c2t_map;
            EXPECT_TRUE(o2mrelation::verify(map_node, info));
            for(const std::string &o2m_path : O2M_PATHS)
            {
                EXPECT_TRUE(map_node.has_child(o2m_path));
            }
        }

        EXPECT_EQ(t2c_map["values"].dtype().number_of_elements(),
            grid_mesh.elem_valence(0));
        EXPECT_EQ(t2c_map["sizes"].dtype().number_of_elements(),
            grid_mesh.elems());

        EXPECT_EQ(c2t_map["values"].dtype().number_of_elements(),
            grid_mesh.elem_valence(0));
        EXPECT_EQ(c2t_map["sizes"].dtype().number_of_elements(),
            grid_mesh.elem_valence(0));

        std::vector<index_list> expected_elem_corner_lists;
        for(index_t ei = 0, ci = 0; ei < grid_mesh.elems(); ei++)
        {
            index_list expected_elem_corners;
            for(index_t eci = 0; eci < grid_mesh.points_per_elem(); eci++, ci++)
            {
                expected_elem_corners.push_back(ci);
            }
            expected_elem_corner_lists.push_back(expected_elem_corners);
        }
        std::vector<index_list> actual_elem_corner_lists = expand_o2mrelation(t2c_map);
        EXPECT_EQ(actual_elem_corner_lists, expected_elem_corner_lists);

        std::vector<index_list> expected_corner_elem_lists;
        for(index_t ei = 0, ci = 0; ei < grid_mesh.elems(); ei++)
        {
            for(index_t eci = 0; eci < grid_mesh.points_per_elem(); eci++, ci++)
            {
                index_list expected_corner_elems;
                expected_corner_elems.push_back(ei);
                expected_corner_elem_lists.push_back(expected_corner_elems);
            }
        }
        std::vector<index_list> actual_corner_elem_lists = expand_o2mrelation(c2t_map);
        EXPECT_EQ(actual_corner_elem_lists, expected_corner_elem_lists);

        // Verify Correctness of Element Orientations //

        if(grid_mesh.dims() < 3)
        {
            Node grid_orients, corner_orients;
            calc_orientation_field(grid_mesh.type, grid_topo, grid_coords, grid_orients);
            calc_orientation_field(corner_type, corner_topo, corner_coords, corner_orients);

            std::vector<float64> expected_orients_vec(grid_corners);
            for(index_t ei = 0; ei < grid_mesh.elems(); ei++)
            {
                Node data(DataType::float64(1), grid_orients["values"].element_ptr(ei), true);
                const float64 elem_orient = data.to_float64();
                for(index_t si = 0; si < corners_per_elem; si++)
                {
                    expected_orients_vec[corners_per_elem * ei + si] = elem_orient;
                }
            }

            Node expected_orients(DataType::float64(expected_orients_vec.size()),
                expected_orients_vec.data(), true);
            EXPECT_FALSE(corner_orients["values"].diff(expected_orients, info));
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_generate, generate_exceptions)
{
    for(int i = 0; i < 4; i++)
    {
        conduit::Node out;
        conduit::blueprint::mesh::examples::braid("uniform", 2, 2, 2, out);
        const conduit::Node &topologies = out["topologies"];
        const conduit::Node &n_topo = topologies[0];
        bool generatedException = false;
        try
        {
            // Pass a uniform topology to a routine that expects unstructured.
            conduit::Node s2dmap, d2smap;
            switch(i)
            {
            case 0:
                conduit::blueprint::mesh::topology::unstructured::generate_lines(n_topo,
                                                                                 out["topologies/line"],
                                                                                 s2dmap,
                                                                                 d2smap);
                break;
            case 1:
                conduit::blueprint::mesh::topology::unstructured::generate_faces(n_topo,
                                                                                 out["topologies/face"],
                                                                                 s2dmap,
                                                                                 d2smap);
                break;
            case 2:
                conduit::blueprint::mesh::topology::unstructured::generate_sides(n_topo,
                                                                                 out["topologies/side"],
                                                                                 out["coordsets/side_coords"],
                                                                                 s2dmap,
                                                                                 d2smap);
                break;
            case 3:
                conduit::blueprint::mesh::topology::unstructured::generate_corners(n_topo,
                                                                                 out["topologies/corner"],
                                                                                 out["coordsets/corner_coords"],
                                                                                 s2dmap,
                                                                                 d2smap);
                break;
            }
        }
        catch(conduit::Error &)
        {
            generatedException = true;
        }
        EXPECT_TRUE(generatedException);
    }
}
