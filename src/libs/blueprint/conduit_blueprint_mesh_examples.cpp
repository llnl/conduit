// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples.cpp
///
//-----------------------------------------------------------------------------

#if defined(CONDUIT_PLATFORM_WINDOWS)
#define NOMINMAX
#undef min
#undef max
#include "windows.h"
#endif

//-----------------------------------------------------------------------------
// std lib includes
//-----------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <vector>
#include <queue>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh_examples.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mesh_utils.hpp"


//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{


//-----------------------------------------------------------------------------
// -- begin conduit::blueprint:: --
//-----------------------------------------------------------------------------
namespace blueprint
{


//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh --
//-----------------------------------------------------------------------------
namespace mesh
{


//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::examples --
//-----------------------------------------------------------------------------
namespace examples
{

//---------------------------------------------------------------------------//
const float64 PI_VALUE = 3.14159265359;

//---------------------------------------------------------------------------//
struct point
{
    point(float64 px = 0.0, float64 py = 0.0, float64 pz = 0.0) : x(px), y(py), z(pz) {};
    point(float64* ps) : x(ps[0]), y(ps[1]), z(ps[2]) {};

    bool operator<(const point& other) const
    {
        float64 mvals[3] = {this->x, this->y, this->z};
        float64 ovals[3] = {other.x, other.y, other.z};

        for(index_t i = 0; i < 3; i++)
        {
            if(fabs(mvals[i] - ovals[i]) >= 1e-6)
            {
                return mvals[i] < ovals[i];
            }
        }
        return false;
    };

    float64 x, y, z;
};

//---------------------------------------------------------------------------//
void basic_init_example_element_scalar_field(index_t nele_x,
                                             index_t nele_y,
                                             index_t nele_z,
                                             Node &res,
                                             index_t prims_per_ele=1)
{
    index_t nele = nele_x;

    if(nele_y > 0)
    {
        nele = nele * nele_y;
    }

    if(nele_z > 0)
    {
        nele = nele * nele_z;
    }

    res["association"] = "element";
    res["topology"] = "mesh";
    res["volume_dependent"] = "false";
    res["values"].set(DataType::float64(nele*prims_per_ele));

    float64 *vals = res["values"].value();
    for(index_t i = 0; i < nele*prims_per_ele; i++)
    {
        vals[i] = i + 0.0;
    }
}

//---------------------------------------------------------------------------//
bool
braid_1d_allowed_shape_type(const std::string& mesh_type)
{
    if ( mesh_type == "uniform"  ||
         mesh_type == "rectilinear")
    {
        return true;
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------//
bool
braid_2d_only_shape_type(const std::string& mesh_type)
{
    if ( mesh_type == "tris"  ||
         mesh_type == "quads" ||
         mesh_type == "quads_poly" ||
         mesh_type == "mixed_2d")
    {
        return true;
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------//
bool
braid_3d_only_shape_type(const std::string& mesh_type)
{
    if( mesh_type == "tets" ||
        mesh_type == "hexs" ||
        mesh_type == "hexs_poly" ||
        mesh_type == "wedges" ||
        mesh_type == "pyramids" ||
        mesh_type == "mixed")
    {
        return true;
    }
    else
    {
        return false;
    }

}

//---------------------------------------------------------------------------//
void braid_init_example_state(Node &res)
{
    res["state/time"]   = (float64)3.1415;
    res["state/cycle"]  = (uint64) 100;
}


//---------------------------------------------------------------------------//
inline float64 braid_init_example_point_scalar_field_calc_dx(index_t npts_x)
{
    return (float) (4.0 * PI_VALUE) / float64(npts_x - 1);
}


//---------------------------------------------------------------------------//
inline float64 braid_init_example_point_scalar_field_calc_dy(index_t npts_y)
{
    return (float) (2.0 * PI_VALUE) / float64(npts_y-1);
}


//---------------------------------------------------------------------------//
inline float64 braid_init_example_point_scalar_field_calc_dz(index_t npts_z)
{
    return (float) (3.0 * PI_VALUE) / float64(npts_z-1);
}


//---------------------------------------------------------------------------//
inline float64 braid_init_example_point_scalar_field_calc_single_val(
    float64 dx, float64 dy, float64 dz,
    float64 i, float64 j, float64 k,
    index_t npts_z)
{
    float64 cz =  (k * dz) - (1.5 * PI_VALUE);
    float64 cy =  (j * dy) - ( PI_VALUE);
    float64 cx =  (i * dx) + (2.0 * PI_VALUE);
    float64 cv =  sin( cx ) +
                  sin( cy ) +
                  2 * cos(sqrt( (cx*cx)/2.0 +cy*cy) / .75) +
                  4 * cos( cx*cy / 4.0);
    if(npts_z > 1)
    {
        cv += sin( cz ) +
              1.5 * cos(sqrt(cx*cx + cy*cy + cz*cz) / .75);
    }
    return cv;
}


//---------------------------------------------------------------------------//
void braid_init_example_point_scalar_field(index_t npts_x,
                                           index_t npts_y,
                                           index_t npts_z,
                                           Node &res)
{

    if(npts_y < 1)
    {
        npts_y = 1;
    }

    if(npts_z < 1)
    {
        npts_z = 1;
    }

    index_t npts = npts_x * npts_y * npts_z;

    res["association"] = "vertex";
    res["topology"] = "mesh";
    res["values"].set(DataType::float64(npts));

    float64 *vals = res["values"].value();

    float64 dx = braid_init_example_point_scalar_field_calc_dx(npts_x);
    float64 dy = braid_init_example_point_scalar_field_calc_dy(npts_y);
    float64 dz = braid_init_example_point_scalar_field_calc_dz(npts_z);

    index_t idx = 0;

    for(index_t k = 0; k < npts_z ; k++)
    {
        float64 cz =  (k * dz) - (1.5 * PI_VALUE);

        for(index_t j = 0; j < npts_y ; j++)
        {
            float64 cy =  (j * dy) - ( PI_VALUE);
            for(index_t i = 0; i < npts_x ; i++)
            {

                float64 cx =  (i * dx) + (2.0 * PI_VALUE);

                float64 cv =  sin( cx ) +
                              sin( cy ) +
                              2 * cos(sqrt( (cx*cx)/2.0 +cy*cy) / .75) +
                              4 * cos( cx*cy / 4.0);

                if(npts_z > 1)
                {
                    cv += sin( cz ) +
                          1.5 * cos(sqrt(cx*cx + cy*cy + cz*cz) / .75);
                }

                vals[idx] = cv;
                idx++;
            }
        }
    }
}


//---------------------------------------------------------------------------//
inline float64 braid_init_example_point_vector_field_calc_dxyz(index_t npts_xyz)
{
    return 20.0  / float64(npts_xyz - 1);
}


//---------------------------------------------------------------------------//
inline void braid_init_example_point_vector_field_calc_single_val(
    float64 dx, float64 dy, float64 dz,
    float64 i, float64 j, float64 k,
    float64 *u_vals, float64 *v_vals, float64 *w_vals,
    index_t idx)
{
    u_vals[idx] = -10.0 + i * dx;
    v_vals[idx] = -10.0 + j * dy;
    w_vals[idx] = -10.0 + k * dz;
}


//---------------------------------------------------------------------------//
void braid_init_example_point_vector_field(index_t npts_x,
                                           index_t npts_y,
                                           index_t npts_z,
                                           Node &res)
{
   index_t npts = npts_x;

    if(npts_y > 0)
    {
        npts *= npts_y;
    }

    if(npts_z > 0)
    {
        npts *= npts_z;
    }

    res["association"] = "vertex";
    res["topology"] = "mesh";

    res["values/u"].set(DataType::float64(npts));
    float64 *u_vals = res["values/u"].value();

    float64 * v_vals = NULL;
    if (npts_y > 1)
    {
        res["values/v"].set(DataType::float64(npts));
        v_vals = res["values/v"].value();
    }

    float64 *w_vals = NULL;
    if(npts_z > 1)
    {
        res["values/w"].set(DataType::float64(npts));
        w_vals = res["values/w"].value();
    }

    // this logic is from the explicit coord set setup function
    // we are using the coords (distance from origin)
    // to create an example vector field

    float64 dx = 0.0;
    float64 dy = 0.0;

    if(npts_x > 1)
    {
        dx = braid_init_example_point_vector_field_calc_dxyz(npts_x);
    }

    // AGC is this right?  Note change to variable being tested
    // and used in divisor.
    if(npts_y > 1)
    {
        dy = braid_init_example_point_vector_field_calc_dxyz(npts_y);
    }


    float64 dz = 0.0;

    if(npts_z > 1)
    {
        dz = braid_init_example_point_vector_field_calc_dxyz(npts_z);
    }

    // make sure outerloop exex
    if(npts_y < 1)
    {
        npts_y = 1;
    }

    if(npts_z < 1)
    {
        npts_z = 1;
    }

    index_t idx = 0;
    for(index_t k = 0; k < npts_z ; k++)
    {
        float64 cz = -10.0 + k * dz;

        for(index_t j = 0; j < npts_y ; j++)
        {
            float64 cy =  -10.0 + j * dy;

            for(index_t i = 0; i < npts_x ; i++)
            {
                float64 cx =  -10.0 + i * dx;

                u_vals[idx] = cx;

                if(dy > 0.0)
                {
                    v_vals[idx] = cy;
                }

                if(dz > 0.0)
                {
                    w_vals[idx] = cz;
                }

                idx++;
            }

        }
    }
}


//---------------------------------------------------------------------------//
void braid_init_example_element_scalar_field(index_t nele_x,
                                             index_t nele_y,
                                             index_t nele_z,
                                             Node &res,
                                             index_t prims_per_ele=1)
{
    index_t nele = nele_x;

    if(nele_y > 0)
    {
        nele = nele * nele_y;
    }

    if(nele_z > 0)
    {
        nele = nele * nele_z;
    }

    res["association"] = "element";
    res["topology"] = "mesh";

    index_t vals_size = nele * prims_per_ele;

    res["values"].set(DataType::float64(vals_size));

    float64 *vals = res["values"].value();

    float64 dx = 20.0 / float64(nele_x);

    float64 dy = 0.0f;
    if(nele_y > 0 )
    {
        dy = 20.0 / float64(nele_y);
    }

    float64 dz = 0.0f;
    if(nele_z > 0 )
    {
        dz = 20.0 / float64(nele_z);
    }

    index_t idx = 0;
    for(index_t k = 0; (idx == 0 || k < nele_z); k++)
    {
        float64 cz =  (k * dz) + -10.0;

        for(index_t j = 0; (idx == 0 || j < nele_y) ; j++)
        {
            float64 cy =  (j * dy) + -10.0;

            for(index_t i = 0; (idx == 0 || i < nele_x) ; i++)
            {
                float64 cx = (i * dx) + -10.0;

                float64 cv = 10.0 * sqrt( cx*cx );

                if(nele_y != 0)
                {
                    cv = 10.0 * sqrt( cx*cx + cy*cy );
                }

                if(nele_z != 0)
                {
                    cv = 10.0 * sqrt( cx*cx + cy*cy +cz*cz );
                }

                for(index_t ppe = 0; ppe < prims_per_ele; ppe++ )
                {
                    vals[idx] = cv;
                    idx++;
                }
            }
        }
    }
}


//---------------------------------------------------------------------------//
void debug_print_array(index_t ext_x, index_t ext_y, index_t ext_z,
    index_t prims_per_ele, float64 *vals)
{
    int k = 0;
    do {
        for (int j = 0; j < ext_y; ++j) {
            for (int i = 0; i < ext_x; ++i) {
                for (int pidx = 0; pidx < prims_per_ele; ++pidx) {
                    if (pidx > 0) { std::cout << ","; }
                    int idx =
                        k * prims_per_ele * ext_x * ext_y +
                        j * prims_per_ele * ext_x +
                        i * prims_per_ele +
                        pidx;
                    std::cout << vals[idx];
                }
                std::cout << "  ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
        k += 1;
    } while (k < ext_z);
}

void strided_structured_field(index_t nval_x,
    index_t nval_y,
    index_t nval_z,
    index_t origin_x,
    index_t origin_y,
    index_t origin_z,
    index_t ext_x,
    index_t ext_y,
    index_t ext_z,
    const char * association,
    const char * toponame,
    Node &res,
    index_t prims_per_val = 1)
{
    index_t nvals = ext_x * ext_y;

    if (ext_z > 0)
    {
        nvals = nvals * ext_z;
    }

    res["association"] = association; // vertex or element
    res["topology"] = toponame; // often something like "mesh"

    index_t dimensions = 2;
    if (nval_z > 0)
    {
        dimensions += 1;
    }
    res["offsets"].set(DataType::index_t(dimensions));
    res["strides"].set(DataType::index_t(dimensions));
    index_t *offsets = res["offsets"].value();
    index_t *strides = res["strides"].value();
    // fill offsets and strides
    offsets[0] = origin_x;
    offsets[1] = origin_y;
    strides[0] = prims_per_val;
    strides[1] = ext_x * prims_per_val;
    if (nval_z > 0)
    {
        offsets[2] = origin_z;
        strides[2] = ext_y * ext_x * prims_per_val;
    }

    index_t vals_size = nvals * prims_per_val;

    res["values"].set(DataType::float64(vals_size));

    float64 *vals = res["values"].value();

    std::fill(vals, vals + vals_size, 0.);

    index_t stride_i_elts = prims_per_val;
    index_t stride_j_elts = ext_x * stride_i_elts;
    index_t stride_k_elts = ext_y * stride_j_elts;

    // 2D data sets need at least one element in the z-direction
    if (nval_z == 0)
    {
        nval_z = 1;
    }

    float64 seqval = 0.;
    for (index_t k = 0, k_pos = origin_z * stride_k_elts;
        k < nval_z;
        k++, k_pos += stride_k_elts)
    {
        for (index_t j = 0, j_pos = k_pos + origin_y * stride_j_elts;
            j < nval_y;
            j++, j_pos += stride_j_elts)
        {
            for (index_t i = 0, i_pos = j_pos + origin_x * stride_i_elts;
                i < nval_x;
                i++, i_pos += stride_i_elts)
            {
                seqval += 1;

                for (index_t ppe = 0; ppe < prims_per_val; ppe++)
                {
                    vals[i_pos + ppe] = seqval;
                }
            }
        }
    }

    //std::cout << resulttype << " field for " << association << " on \"" <<
    //    toponame << "\":" << std::endl;
    //debug_print_array(ext_x, ext_y, ext_z, prims_per_val, vals);
}


void strided_structured_element_scalar_field(index_t nele_x,
                                             index_t nele_y,
                                             index_t nele_z,
                                             index_t origin_x,
                                             index_t origin_y,
                                             index_t origin_z,
                                             index_t ext_x,
                                             index_t ext_y,
                                             index_t ext_z,
                                             Node &res,
                                             index_t prims_per_ele = 1)
{
    strided_structured_field(nele_x, nele_y, nele_z, origin_x, origin_y, origin_z,
        ext_x, ext_y, ext_z, "element", "mesh", res, prims_per_ele);
}


//---------------------------------------------------------------------------//
void strided_structured_point_scalar_field(index_t npts_x,
                                           index_t npts_y,
                                           index_t npts_z,
                                           index_t origin_x,
                                           index_t origin_y,
                                           index_t origin_z,
                                           index_t ext_x,
                                           index_t ext_y,
                                           index_t ext_z,
                                           Node &res,
                                           index_t prims_per_pt = 1)
{
    strided_structured_field(npts_x, npts_y, npts_z, origin_x, origin_y, origin_z,
        ext_x, ext_y, ext_z, "vertex", "mesh", res, prims_per_pt);
}


//---------------------------------------------------------------------------//
void braid_init_example_matset(index_t nele_x,
                               index_t nele_y,
                               index_t nele_z,
                               Node &res)
{
    index_t nele = nele_x * nele_y * ((nele_z > 0) ? nele_z : 1);

    res["topology"] = "mesh";

    Node &vfs = res["volume_fractions"];
    vfs["mat1"].set(DataType::float64(nele));
    vfs["mat2"].set(DataType::float64(nele));

    float64 *mat1_vals = vfs["mat1"].value();
    float64 *mat2_vals = vfs["mat2"].value();

    for(index_t k = 0, idx = 0; (idx == 0 || k < nele_z); k++)
    {
        for(index_t j = 0; (idx == 0 || j < nele_y) ; j++)
        {
            for(index_t i = 0; (idx == 0 || i < nele_x) ; i++, idx++)
            {
                float64 mv = (nele_x == 1) ? 0.5 : i / (nele_x - 1.0);

                mat1_vals[idx] = mv;
                mat2_vals[idx] = 1.0 - mv;
            }
        }
    }
}


//---------------------------------------------------------------------------//
void braid_init_example_specset(index_t nele_x,
                                index_t nele_y,
                                index_t nele_z,
                                Node &res)
{
    index_t nele = nele_x * nele_y * ((nele_z > 0) ? nele_z : 1);

    res["matset"] = "mesh";

    Node &mfs = res["matset_values"];
    mfs["mat1/spec1"].set(DataType::float64(nele));
    mfs["mat1/spec2"].set(DataType::float64(nele));
    mfs["mat2/spec1"].set(DataType::float64(nele));
    mfs["mat2/spec2"].set(DataType::float64(nele));

    float64 *spec1_vals[2] = {mfs["mat1/spec1"].value(), mfs["mat2/spec1"].value()};
    float64 *spec2_vals[2] = {mfs["mat1/spec2"].value(), mfs["mat2/spec2"].value()};

    for(index_t k = 0, idx = 0; (idx == 0 || k < nele_z); k++)
    {
        for(index_t j = 0; (idx == 0 || j < nele_y) ; j++)
        {
            for(index_t i = 0; (idx == 0 || i < nele_x) ; i++, idx++)
            {
                float64 mv = (nele_y == 1) ? 0.5 : i / (nele_y - 1.0);
                for(index_t s = 0; s < 2; s++)
                {
                    spec1_vals[s][idx] = mv;
                    spec2_vals[s][idx] = 1.0 - mv;
                }
            }
        }
    }
}


//---------------------------------------------------------------------------//
void braid_init_uniform_coordset(index_t npts_x,
                                 index_t npts_y,
                                 index_t npts_z,
                                 Node &coords)
{
    coords["type"] = "uniform";
    Node &dims = coords["dims"];
    dims["i"] = npts_x;
    if(npts_y > 1)
    {
        dims["j"] = npts_y;
    }

    if(npts_z > 1)
    {
        dims["k"] = npts_z;
    }

    // -10 to 10 in each dim,
    Node &origin = coords["origin"];
    origin["x"] = -10.0;

    if(npts_y > 1)
    {
        origin["y"] = -10.0;
    }

    if(npts_z > 1)
    {
        origin["z"] = -10.0;
    }

    Node &spacing = coords["spacing"];
    spacing["dx"] = 20.0 / (float64)(npts_x-1);
    if(npts_y > 1)
    {
        spacing["dy"] = 20.0 / (float64)(npts_y-1);
    }

    if(npts_z > 1 )
    {
        spacing["dz"] = 20.0 / (float64)(npts_z-1);
    }
}


//---------------------------------------------------------------------------//
void braid_init_rectilinear_coordset(index_t npts_x,
                                     index_t npts_y,
                                     index_t npts_z,
                                     Node &coords)
{
    coords["type"] = "rectilinear";
    Node &coord_vals = coords["values"];
    coord_vals["x"].set(DataType::float64(npts_x));

    if(npts_y > 1)
    {
        coord_vals["y"].set(DataType::float64(npts_y));
    }

    if(npts_z > 1)
    {
        coord_vals["z"].set(DataType::float64(npts_z));
    }

    float64 *x_vals = coord_vals["x"].value();
    float64 *y_vals = NULL;
    if(npts_y > 1)
    {
        y_vals = coord_vals["y"].value();
    }

    float64 *z_vals = NULL;
    if(npts_z > 1)
    {
        z_vals = coord_vals["z"].value();
    }


    float64 dx = 20.0 / (float64)(npts_x-1);
    float64 dy = 0.0;
    if(npts_y > 1)
    {
        dy = 20.0 / (float64)(npts_y-1);
    }

    float64 dz = 0.0;
    if(npts_z > 1)
    {
        dz = 20.0 / (float64)(npts_z-1);
    }

    for(int i=0; i < npts_x; i++)
    {
        x_vals[i] = -10.0 + i * dx;
    }

    if(npts_y > 1)
    {
        for(int j=0; j < npts_y; j++)
        {
            y_vals[j] = -10.0 + j * dy;
        }
    }

    if(npts_z > 1)
    {
        for(int k=0; k < npts_z; k++)
        {
            z_vals[k] = -10.0 + k * dz;
        }
    }
}

//---------------------------------------------------------------------------//
void
braid_init_explicit_coordset(index_t npts_x,
    index_t npts_y,
    index_t npts_z,
    Node& coords)
{
    coords["type"] = "explicit";

    index_t npts = npts_x;
    if (npts_y > 0)
    {
        npts *= npts_y;
    }

    if (npts_z > 1)
    {
        npts *= npts_z;
    }

    // also support interleaved
    Node& coord_vals = coords["values"];
    coord_vals["x"].set(DataType::float64(npts));
    if (npts_y > 0)
    {
        coord_vals["y"].set(DataType::float64(npts));
    }

    if (npts_z > 1)
    {
        coord_vals["z"].set(DataType::float64(npts));
    }

    float64* x_vals = coord_vals["x"].value();
    float64* y_vals = NULL;
    if (npts_y > 0)
    {
        y_vals = coord_vals["y"].value();
    }

    float64* z_vals = NULL;

    if (npts_z > 1)
    {
        z_vals = coord_vals["z"].value();
    }

    float64 dx = 20.0 / float64(npts_x - 1);
    float64 dy = 20.0 / float64(npts_y - 1);
    if (npts_y > 0)
    {
        dy = 20.0 / float64(npts_y - 1);
    }

    float64 dz = 0.0;

    if (npts_z > 1)
    {
        dz = 20.0 / float64(npts_z - 1);
    }

    index_t idx = 0;
    // default to one loop iteration (2d case)
    index_t outer = 1;
    // expand loop iteration for 3d case
    if (npts_z > 1)
    {
        outer = npts_z;
    }
    // default to one loop iteration (1d case)
    index_t middle = 1;
    // expand loop iteration for 2d and 3d case
    if (npts_y > 1)
    {
        middle = npts_y;
    }

    for (index_t k = 0; k < outer; k++)
    {
        float64 cz = -10.0 + k * dz;

        for (index_t j = 0; j < middle; j++)
        {
            float64 cy = -10.0 + j * dy;

            for (index_t i = 0; i < npts_x; i++)
            {
                x_vals[idx] = -10.0 + i * dx;

                if (npts_y > 1)
                {
                    y_vals[idx] = cy;
                }

                if (npts_z > 1)
                {
                    z_vals[idx] = cz;
                }

                idx++;
            }

        }
    }
}

//---------------------------------------------------------------------------//

void
fill_corner_point(Node& n,
    const double* x, const double* y, const double* z, int idx,
    double dftx, double dfty, double dftz)
{
    int dims = n.dtype().number_of_elements();
    double_array an = n.as_double_array();

    an[0] = dftx;
    if (dims > 1)
    {
        an[1] = dfty;
        if (dims == 3)
        {
            an[2] = dftz;
        }
    }

    if ((x != nullptr) &&
        (dims == 1 || y != nullptr) &&
        (dims == 2 || z != nullptr))
    {
        an[0] = x[idx];
        if (dims > 1)
        {
            an[1] = y[idx];
            if (dims == 3)
            {
                an[2] = z[idx];
            }
        }
    }

}

void
braid_init_explicit_lerp_coordset(index_t npts_x,
                             index_t npts_y,
                             index_t npts_z,
                             Node &coords,
                             const double* corner_xs = nullptr,
                             const double* corner_ys = nullptr,
                             const double* corner_zs = nullptr);

void
braid_init_explicit_lerp_coordset(index_t npts_x,
                             index_t npts_y,
                             index_t npts_z,
                             Node &coords,
                             const double* corner_xs,
                             const double* corner_ys,
                             const double* corner_zs)
{
    coords["type"] = "explicit";

    index_t dims = 1;
    if (npts_y > 0)
    {
        dims = 2;
        if (npts_z > 1)
        {
            dims = 3;
        }
    }


    // Extract corner points
    Node A; A.set(DataType::float64(dims));
    fill_corner_point(A, corner_xs, corner_ys, corner_zs, 0, -10, -10, -10);
    Node B; B.set(DataType::float64(dims));
    fill_corner_point(B, corner_xs, corner_ys, corner_zs, 1,  10, -10, -10);

    // Set up result vectors
    Node left, right, grid;

    if (dims > 1)
    {
        Node C; C.set(DataType::float64(dims));
        fill_corner_point(C, corner_xs, corner_ys, corner_zs, 2, 10, 10, -10);
        Node D; D.set(DataType::float64(dims));
        fill_corner_point(D, corner_xs, corner_ys, corner_zs, 3, -10, 10, -10);
        if (dims > 2)
        {
            Node E; E.set(DataType::float64(dims));
            fill_corner_point(E, corner_xs, corner_ys, corner_zs, 4, -10, -10, 10);
            Node F; F.set(DataType::float64(dims));
            fill_corner_point(F, corner_xs, corner_ys, corner_zs, 5, 10, -10, 10);
            Node G; G.set(DataType::float64(dims));
            fill_corner_point(G, corner_xs, corner_ys, corner_zs, 6, 10, 10, 10);
            Node H; H.set(DataType::float64(dims));
            fill_corner_point(H, corner_xs, corner_ys, corner_zs, 7, -10, 10, 10);
            Node SW;
            conduit::blueprint::mesh::utils::lerp(A, E, npts_z, SW);
            Node SE;
            conduit::blueprint::mesh::utils::lerp(B, F, npts_z, SE);
            Node NE;
            conduit::blueprint::mesh::utils::lerp(C, G, npts_z, NE);
            Node NW;
            conduit::blueprint::mesh::utils::lerp(D, H, npts_z, NW);


            conduit::blueprint::mesh::utils::lerp(SW, NW, npts_y, left);
            conduit::blueprint::mesh::utils::lerp(SE, NE, npts_y, right);
        }
        else
        {
            // dims == 2
            conduit::blueprint::mesh::utils::lerp(A, D, npts_y, left);
            conduit::blueprint::mesh::utils::lerp(B, C, npts_y, right);
        }
    }
    else
    {
        left.set(A);
        right.set(B);
    }
    conduit::blueprint::mesh::utils::lerp(left, right, npts_x, grid);

    // also support interleaved
    Node &coord_vals = coords["values"];

    coord_vals["x"].set(grid[0]);
    if (dims > 1)
    {
        coord_vals["y"].set(grid[1]);
        if (dims > 2)
        {
            coord_vals["z"].set(grid[2]);
        }
    }
}


//---------------------------------------------------------------------------//
void
braid_init_example_adjset(Node &mesh)
{
    typedef std::map< point, std::map<index_t, index_t> > point_doms_map;
    typedef std::map<std::set<index_t>, std::vector<std::vector<index_t> > > group_idx_map;

    if (mesh.dtype().is_empty()) { return; }

    const std::string dim_names[3] = {"x", "y", "z"};
    const index_t dim_count = blueprint::mesh::coordset::dims(
        mesh.child(0).fetch("coordsets").child(0));

    // FIXME(JRC): Update this code to use the utilities provided in
    // 'conduit_blueprint_mesh_utils.hpp'.

    // From mesh data, create a map from domain combination tuple to point list.
    // These domain combination tuples represent groups and the point lists contain
    // the points that lie on the shared boundary between these domains.
    point_doms_map mesh_point_doms_map;
    NodeConstIterator doms_it = mesh.children();
    while(doms_it.has_next())
    {
        const Node& dom_node = doms_it.next();
        const Node& dom_cset = dom_node["coordsets"].child(0);
        const std::string dom_type = dom_cset["type"].as_string();
        const index_t dom_id = dom_node["state/domain_id"].to_index_t();

        Node dom_coords;
        if(dom_type == "uniform")
        {
            blueprint::mesh::coordset::uniform::to_explicit(dom_cset, dom_coords);
        }
        else if(dom_type == "rectilinear")
        {
            blueprint::mesh::coordset::rectilinear::to_explicit(dom_cset, dom_coords);
        }
        else // if(dom_type == "explicit")
        {
            dom_coords.set_external(dom_cset);
        }

        float64_array dom_dim_coords[3];
        for(index_t d = 0; d < dim_count; d++)
        {
            dom_dim_coords[d] = dom_coords["values"][dim_names[d]].as_float64_array();
        }

        for(index_t i = 0; i < dom_dim_coords[0].number_of_elements(); i++)
        {
            float64 cvals[3] = {0.0, 0.0, 0.0};
            for(index_t d = 0; d < dim_count; d++)
            {
                cvals[d] = dom_dim_coords[d][i];
            }
            mesh_point_doms_map[point(&cvals[0])][dom_id] = i;
        }
    }

    // Now mesh_point_doms_map has an entry (x, y, z) -> {dom -> i} for each point in ALL domains.

    group_idx_map groups_map;
    point_doms_map::const_iterator pm_itr;
    for(pm_itr = mesh_point_doms_map.begin();
        pm_itr != mesh_point_doms_map.end(); ++pm_itr)
    {
        // The only points we're interested in are the ones with more than one
        // value.  Those are the points that sit on the domain boundary.
        const std::map<index_t, index_t>& point_dom_idx_map = pm_itr->second;
        if(point_dom_idx_map.size() > 1)
        {
            std::set<index_t> point_group;

            // Put all the the domains for this point into point_group
            std::map<index_t, index_t>::const_iterator pg_itr;
            for(pg_itr = point_dom_idx_map.begin();
                pg_itr != point_dom_idx_map.end(); ++pg_itr)
            {
                point_group.insert(pg_itr->first);
            }

            // Get/create the vector-of-vector-of-index for this group (set of adjoining domains)
            std::vector<std::vector<index_t> >& group_indices = groups_map[point_group];
            if(group_indices.empty())
            {
                group_indices.resize(point_group.size());
            }

            // For each domain that contains this point, push that domain's point index back
            // in that domain's vector.
            std::set<index_t>::const_iterator gd_itr;
            std::set<index_t>::size_type gi = 0;
            for(gd_itr = point_group.begin();
                gd_itr != point_group.end(); ++gd_itr, ++gi)
            {
                index_t g_idx = static_cast<index_t>(point_dom_idx_map.find(*gd_itr)->second);
                group_indices[gi].push_back(g_idx);
            }
        }
    }

    // Now groups_map has an entry for each border {dom} -> [ (domain) [i] ] ,
    // where the set of domains comprising that border is the key and the value is
    // a vector (one for each domain) of vectors (the indices of the border points
    // in that domain).
    //
    // In the map value, the order of domains (the outer vector) is determined by
    // the iterator over the key (the set of owning domains).  The main point is
    // that the order of the inner vectors (the points in that domain) is consistent.
    // That is, index k of all the inner vectors refers to the same physical point.

    group_idx_map::const_iterator gm_itr;
    index_t gid = 0;
    for(gm_itr = groups_map.begin();
        gm_itr != groups_map.end(); ++gm_itr, ++gid)
    {
        const std::set<index_t>& group_doms = gm_itr->first;
        const std::vector<std::vector<index_t> >& group_indices = gm_itr->second;

        std::ostringstream oss;
        oss << "group" << gid;
        const std::string group_name = oss.str();

        std::set<index_t>::const_iterator dg_itr;
        std::set<index_t>::size_type d = 0;
        for(dg_itr = group_doms.begin();
            dg_itr != group_doms.end(); ++dg_itr, ++d)
        {
          const index_t& dom_id = *dg_itr;
          const std::vector<index_t>& dom_idxs = group_indices[d];

          oss.str("");
          oss << "domain" << dom_id;
          const std::string dom_name = oss.str();

          std::vector<index_t> dom_neighbors(group_doms.begin(), group_doms.end());
          dom_neighbors.erase(dom_neighbors.begin()+d);

          Node& dom_node = mesh[dom_name]["adjsets/mesh_adj"];
          dom_node["association"].set("vertex");
          dom_node["topology"].set("mesh");
          dom_node["groups"][group_name]["neighbors"].set(
            const_cast<index_t*>(dom_neighbors.data()), dom_neighbors.size());
          dom_node["groups"][group_name]["values"].set(
            const_cast<index_t*>(dom_idxs.data()), dom_idxs.size());
        }
    }
}


//---------------------------------------------------------------------------//
void braid_init_example_nestset(Node &mesh)
{
    typedef std::map<point, index_t> point_id_map;
    typedef std::pair<index_t, index_t> window;

    // TODO(JRC): Extend this function to support input domains with cylindrical
    // and spherical coordinates as well.
    const std::string cartesian_dims[3] = {"x", "y", "z"};
    const std::string logical_dims[3] = {"i", "j", "k"};
    const index_t dim_count = blueprint::mesh::coordset::dims(
        mesh.child(0).fetch("coordsets").child(0));

    // initialize data to easily index domains by id/level //

    std::map<index_t, const Node*> mesh_id_map;
	index_t max_dom_id = 0, max_level_id = 0;
	{
        conduit::NodeConstIterator doms_it = mesh.children();
        while(doms_it.has_next())
        {
            const conduit::Node& dom_node = doms_it.next();
            const index_t dom_id = dom_node["state/domain_id"].to_uint64();
            mesh_id_map[dom_id] = &dom_node;
            max_dom_id = std::max(dom_id, max_dom_id);

            const index_t dom_level = dom_node["state/level_id"].to_uint64();
            max_level_id = std::max(dom_level, max_level_id);
        }
    }

    // transform rectilinear input data into unstructured data //

    std::vector<point_id_map> mesh_point_maps(max_dom_id + 1);
    std::vector< std::vector<const Node*> > mesh_level_map(max_level_id + 1);
    {
        conduit::NodeConstIterator doms_it = mesh.children();
        while(doms_it.has_next())
        {
            const conduit::Node &dom_node = doms_it.next();
            const index_t dom_id = dom_node["state/domain_id"].to_uint64();
            const index_t level_id = dom_node["state/level_id"].to_uint64();
            const conduit::Node &dom_coordset = dom_node["coordsets"].child(0);

            conduit::Node dom_coordset_explicit;
            if(dom_coordset["type"].as_string() == "uniform")
            {
                blueprint::mesh::coordset::uniform::to_explicit(
                    dom_coordset, dom_coordset_explicit);
            }
            else if(dom_coordset["type"].as_string() == "rectilinear")
            {
                blueprint::mesh::coordset::rectilinear::to_explicit(
                    dom_coordset, dom_coordset_explicit);
            }
            else
            {
                dom_coordset_explicit.set_external(dom_coordset);
            }
            const index_t num_points = dom_coordset_explicit["values"].
                child(0).dtype().number_of_elements();

            point_id_map &dom_point_map = mesh_point_maps[dom_id];
            {
                for(index_t i = 0; i < num_points; i++)
                {
                    float64 dom_point_vals[3] = {0.0, 0.0, 0.0};
                    for(index_t d = 0; d < dim_count; d++)
                    {
                        conduit::Node &dim_coords =
                            dom_coordset_explicit["values"][cartesian_dims[d]];
                        conduit::Node dim_cval(
                            conduit::DataType(dim_coords.dtype().id(), 1),
                            dim_coords.element_ptr(i), true);
                        dom_point_vals[d] = dim_cval.to_float64();
                    }
                    dom_point_map[point(&dom_point_vals[0])] = i;
                }
            }

            mesh_level_map[level_id].push_back(&dom_node);
        }
    }

    // NOTE(JRC): 'mesh_window_maps' maps a given domain ID to all of the windows
    // for that domain, which are returned via a map from other domain ID to
    // the indices of the extents in the primary domain.
    std::map< index_t, std::map<index_t, window> > mesh_window_maps;
    {
        for(index_t l = 0; l < (index_t)mesh_level_map.size() - 1; l++)
        {
            const std::vector<const Node*> &hi_nodes = mesh_level_map[l];
            const std::vector<const Node*> &lo_nodes = mesh_level_map[l+1];
            for(index_t hi = 0; hi < (index_t)hi_nodes.size(); hi++)
            {
                for(index_t lo = 0; lo < (index_t)lo_nodes.size(); lo++)
                {
                    const Node &hi_node = *hi_nodes[hi];
                    const Node &lo_node = *lo_nodes[lo];

                    const index_t hi_dom_id = hi_node["state/domain_id"].to_uint64();
                    const index_t lo_dom_id = lo_node["state/domain_id"].to_uint64();
                    const point_id_map &hi_point_map = mesh_point_maps[hi_dom_id];
                    const point_id_map &lo_point_map = mesh_point_maps[lo_dom_id];

                    std::vector<point> point_intx_list;
                    point_id_map::const_iterator hi_pt_itr = hi_point_map.begin();
                    point_id_map::const_iterator lo_pt_itr = lo_point_map.begin();
                    while(hi_pt_itr != hi_point_map.end() && lo_pt_itr != lo_point_map.end())
                    {
                        if(hi_pt_itr->first < lo_pt_itr->first)
                        {
                            ++hi_pt_itr;
                        }
                        else if(lo_pt_itr->first < hi_pt_itr->first)
                        {
                            ++lo_pt_itr;
                        }
                        else
                        {
                            point_intx_list.push_back(hi_pt_itr->first);
                            ++hi_pt_itr;
                            ++lo_pt_itr;
                        }
                    }
                    // TODO(JRC): Handle cases wherein the low mesh doesn't
                    // have a sensible window with the high mesh.
                    const point min_intx_point = point_intx_list.front();
                    const point max_intx_point = point_intx_list.back();

                    window &hi_window = mesh_window_maps[hi_dom_id][lo_dom_id];
                    hi_window.first = hi_point_map.at(min_intx_point);
                    hi_window.second = hi_point_map.at(max_intx_point);

                    window &lo_window = mesh_window_maps[lo_dom_id][hi_dom_id];
                    lo_window.first = lo_point_map.at(min_intx_point);
                    lo_window.second = lo_point_map.at(max_intx_point);
                }
            }
        }
    }

    conduit::NodeIterator doms_it = mesh.children();
    while(doms_it.has_next())
    {
        conduit::Node &dom_node = doms_it.next();
        index_t dom_id = dom_node["state/domain_id"].to_uint64();
        index_t dom_level = dom_node["state/level_id"].to_uint64();

        index_t dom_dims[3] = {0, 0, 0}; // needed for 1d to 3d xform per domain
        {
            const conduit::Node &dom_coords = dom_node["coordsets/coords/values"];
            for(index_t d = 0; d < 3; d++)
            {
                dom_dims[d] = !dom_coords.has_child(cartesian_dims[d]) ? 1 :
                    dom_coords[cartesian_dims[d]].dtype().number_of_elements();
            }
        }

        conduit::Node &dom_nestset = dom_node["nestsets/mesh_nest"];
        dom_nestset["association"].set("element");
        dom_nestset["topology"].set("mesh");

        std::map<index_t, window>::const_iterator dom_window_itr;
        for(dom_window_itr = mesh_window_maps[dom_id].begin();
            dom_window_itr != mesh_window_maps[dom_id].end(); ++dom_window_itr)
        {
            index_t odom_id = dom_window_itr->first;
            const conduit::Node &odom_node = *mesh_id_map[odom_id];
            index_t odom_level = odom_node["state/level_id"].to_uint64();

            window window_extrema = dom_window_itr->second;
            std::string window_name;
            {
                std::ostringstream oss;
                // window_{min_dom_id}_{max_dom_id}
                oss << "window_" << std::min(dom_id, odom_id)
                                 << "_"
                                 << std::max(dom_id, odom_id);
                window_name = oss.str();
            }

            conduit::Node &dom_window = dom_nestset["windows"][window_name];
            dom_window["domain_id"].set(odom_id);
            dom_window["domain_type"].set(dom_level < odom_level ? "child" : "parent");

            index_t window_extents[2][3] = {{0, 0, 0}, {0, 0, 0}};
            for(index_t e = 0; e < 2; e++)
            {
                index_t window_extreme = e == 0 ? window_extrema.first : window_extrema.second;
                index_t *window_extent = &window_extents[e][0];

                index_t dim_remainder = window_extreme;
                for(index_t d = 3; d-- > 0;)
                {
                    index_t dim_stride = 1;
                    for(index_t dd = 0; dd < d; dd++)
                    {
                        dim_stride *= dom_dims[dd];
                    }

                    window_extent[d] = dim_remainder / dim_stride;
                    dim_remainder = dim_remainder % dim_stride;
                }
            }

            for(index_t d = 0; d < dim_count; d++)
            {
                // NOTE(JRC): These values may seem incorrect since they're relative
                // to point space, but they actually work out to calculate the proper
                // values because the coordinate indices for an element will always
                // match its minimum point indices and h-l points is number of elements.
                dom_window["origin"][logical_dims[d]].set(window_extents[0][d]);
                dom_window["dims"][logical_dims[d]].set(
                    window_extents[1][d] - window_extents[0][d]);
            }
        }
    }

    doms_it = mesh.children();
    while(doms_it.has_next())
    {
        conduit::Node &dom_node = doms_it.next();
        conduit::Node &dom_windows_node = dom_node["nestsets/mesh_nest/windows"];
        conduit::NodeIterator windows_it = dom_windows_node.children();
        while(windows_it.has_next())
        {
            conduit::Node &dom_window_node = windows_it.next();
            const std::string dom_window_name = windows_it.name();
            index_t odom_id = dom_window_node["domain_id"].to_uint64();

            const conduit::Node &odom_node = *mesh_id_map[odom_id];
            const conduit::Node &odom_window_node =
                odom_node["nestsets/mesh_nest/windows"][dom_window_name];

            const conduit::Node *parent_window_node, *child_window_node;
            if(dom_window_node["domain_type"].as_string() == "child")
            {
                parent_window_node = &dom_window_node;
                child_window_node = &odom_window_node;
            }
            else
            {
                parent_window_node = &odom_window_node;
                child_window_node = &dom_window_node;
            }

            for(index_t d = 0; d < dim_count; d++)
            {
                dom_window_node["ratio"][logical_dims[d]].set(
                    (*child_window_node)["dims"][logical_dims[d]].to_uint64() /
                    (*parent_window_node)["dims"][logical_dims[d]].to_uint64());
            }
        }
    }
}


//---------------------------------------------------------------------------//
void
braid_uniform(index_t npts_x,
              index_t npts_y,
              index_t npts_z,
              Node &res)
{
    res.reset();

    index_t nele_x = npts_x -1;
    index_t nele_y = npts_y -1;
    index_t nele_z = npts_z -1;

    braid_init_example_state(res);
    braid_init_uniform_coordset(npts_x,
                                npts_y,
                                npts_z,
                                res["coordsets/coords"]);

    res["topologies/mesh/type"] = "uniform";
    res["topologies/mesh/coordset"] = "coords";

    Node &fields = res["fields"];


    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_x,
                                            nele_y,
                                            nele_z,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);

}



//---------------------------------------------------------------------------//
void
braid_rectilinear(index_t npts_x,
                  index_t npts_y,
                  index_t npts_z,
                  Node &res)
{
    res.reset();

    index_t nele_x = npts_x -1;
    index_t nele_y = npts_y -1;
    index_t nele_z = npts_z -1;

    braid_init_example_state(res);
    braid_init_rectilinear_coordset(npts_x,
                                    npts_y,
                                    npts_z,
                                    res["coordsets/coords"]);

    res["topologies/mesh/type"] = "rectilinear";
    res["topologies/mesh/coordset"] = "coords";

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_x,
                                            nele_y,
                                            nele_z,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);

}

//---------------------------------------------------------------------------//
void
braid_structured(index_t npts_x,
                 index_t npts_y,
                 index_t npts_z,
                 Node &res)
{
    res.reset();

    index_t nele_x = npts_x -1;
    index_t nele_y = npts_y -1;
    index_t nele_z = npts_z -1;

    braid_init_example_state(res);
    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 npts_z,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "structured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/dims/i"] = (int32)nele_x;

    if(nele_y > 0)
    {
        res["topologies/mesh/elements/dims/j"] = (int32)nele_y;
    }

    if(nele_z > 0)
    {
        res["topologies/mesh/elements/dims/k"] = (int32)nele_z;
    }

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_x,
                                            nele_y,
                                            nele_z,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);

}


//---------------------------------------------------------------------------//
void
braid_points_explicit(index_t npts_x,
                      index_t npts_y,
                      index_t npts_z,
                      Node &res)
{
    res.reset();

    braid_init_example_state(res);

    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 npts_z,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "unstructured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/shape"] = "point";

    if(npts_z <= 0)
    {
        npts_z = 1;
    }

    index_t npts_total = npts_x * npts_y * npts_z;

    res["topologies/mesh/elements/connectivity"].set(DataType::int32(npts_total));
    int32 *conn = res["topologies/mesh/elements/connectivity"].value();

    for(int32 i = 0; i < (int32)npts_total ; i++)
    {
        conn[i] = i;
    }

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(npts_x,
                                            npts_y,
                                            npts_z,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);

}


//---------------------------------------------------------------------------//
void
braid_points_implicit(index_t npts_x,
                      index_t npts_y,
                      index_t npts_z,
                      Node &res)
{
    res.reset();

    braid_init_example_state(res);

    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 npts_z,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "points";
    res["topologies/mesh/coordset"] = "coords";

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(npts_x,
                                            npts_y,
                                            npts_z,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);

}


//---------------------------------------------------------------------------//
void
braid_quads(index_t npts_x,
            index_t npts_y,
            Node &res)
{
    res.reset();

    int32 nele_x = (int32)(npts_x - 1);
    int32 nele_y = (int32)(npts_y - 1);
    int32 nele = nele_x * nele_y;

    braid_init_example_state(res);
    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 1,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "unstructured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/shape"] = "quad";
    res["topologies/mesh/elements/connectivity"].set(DataType::int32(nele*4));
    int32 *conn = res["topologies/mesh/elements/connectivity"].value();

    int32 idx = 0;
    for(int32 j = 0; j < nele_y ; j++)
    {
        int32 yoff = j * (nele_x+1);
        for(int32 i = 0; i < nele_x; i++)
        {
            conn[idx+0] = yoff + i;
            conn[idx+1] = yoff + i + (nele_x+1);
            conn[idx+2] = yoff + i + 1 + (nele_x+1);
            conn[idx+3] = yoff + i + 1;

            idx+=4;
        }
    }


    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_x,
                                            nele_y,
                                            0,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["vel"]);
}


//---------------------------------------------------------------------------//
void
braid_bent_quads(const Node & spec, Node &res)
{
    res.reset();

    const int32 npts_x = spec["npts_x"].as_int32();
    const int32 npts_y = spec["npts_y"].as_int32();

    const int32 domain_id = spec["domain_id"].as_int32();

    const double* x = spec["corner_xs"].as_double_ptr();
    const double* y = spec["corner_ys"].as_double_ptr();

    int32 nele_x = (int32)(npts_x - 1);
    int32 nele_y = (int32)(npts_y - 1);

    braid_init_example_state(res);
    res["state/domain_id"] = domain_id;
    braid_init_explicit_lerp_coordset(npts_x,
                                      npts_y,
                                      1,
                                      res["coordsets/coords"],
                                      x, y, nullptr);

    res["topologies/mesh/type"] = "structured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/dims/i"] = (int32)nele_x;
    res["topologies/mesh/elements/dims/j"] = (int32)nele_y;


    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_x,
                                            nele_y,
                                            0,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["vel"]);
}


//---------------------------------------------------------------------------//
void
braid_lines_2d(index_t npts_x,
               index_t npts_y,
               Node &res)
{
    res.reset();

    // require npts_x > 0 && npts_y > 0

    int32 nele_quads_x = (int32)(npts_x-1);
    int32 nele_quads_y = (int32)(npts_y-1);
    int32 nele_quads = nele_quads_x * nele_quads_y;

    braid_init_example_state(res);
    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 1,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "unstructured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/shape"] = "line";
    res["topologies/mesh/elements/connectivity"].set(DataType::int32(nele_quads*4*2));
    int32 *conn = res["topologies/mesh/elements/connectivity"].value();

    int32 idx = 0;
    for(int32 j = 0; j < nele_quads_y ; j++)
    {
        int32 yoff = j * (nele_quads_x+1);

        for(int32 i = 0; i < nele_quads_x; i++)
        {
            // 4 lines per quad.

            // Note: this pattern allows for simple per-quad construction,
            // but it creates spatially overlapping lines

            conn[idx++] = yoff + i;
            conn[idx++] = yoff + i + (nele_quads_x+1);

            conn[idx++] = yoff + i + (nele_quads_x+1);
            conn[idx++] = yoff + i + 1 + (nele_quads_x+1);

            conn[idx++] = yoff + i;
            conn[idx++] = yoff + i + 1;

            conn[idx++] = yoff + i + 1;
            conn[idx++] = yoff + i + 1 + (nele_quads_x+1);
        }
    }


    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_quads_x,
                                            nele_quads_y,
                                            0,
                                            fields["radial"],4);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["vel"]);

}

//---------------------------------------------------------------------------//
void
braid_tris(index_t npts_x,
           index_t npts_y,
           Node &res)
{
    res.reset();

    // require npts_x > 0 && npts_y > 0

    int32 nele_quads_x = (int32) npts_x-1;
    int32 nele_quads_y = (int32) npts_y-1;
    int32 nele_quads = nele_quads_x * nele_quads_y;

    braid_init_example_state(res);
    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 1,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "unstructured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/shape"] = "tri";
    res["topologies/mesh/elements/connectivity"].set(DataType::int32(nele_quads*6));
    int32 *conn = res["topologies/mesh/elements/connectivity"].value();

    int32 idx = 0;
    for(int32 j = 0; j < nele_quads_y ; j++)
    {
        int32 yoff = j * (nele_quads_x+1);

        for(int32 i = 0; i < nele_quads_x; i++)
        {
            // two tris per quad.
            conn[idx+0] = yoff + i;
            conn[idx+1] = yoff + i + (nele_quads_x+1);
            conn[idx+2] = yoff + i + 1 + (nele_quads_x+1);

            conn[idx+3] = yoff + i;
            conn[idx+4] = yoff + i + 1;
            conn[idx+5] = yoff + i + 1 + (nele_quads_x+1);

            idx+=6;
        }
    }


    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_quads_x,
                                            nele_quads_y,
                                            0,
                                            fields["radial"],2);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          1,
                                          fields["vel"]);

}

//---------------------------------------------------------------------------//
inline int32
calc(int32 i, int32 j, int32 k, int32 I, int32 J, int32 K, int32 nx, int32 ny)
{
  return (i + I) + (j + J) * nx + (k + K) * (nx * ny);
}

//---------------------------------------------------------------------------//
void
braid_mixed_2d(const int32 npts_x,
               const int32 npts_y,
               Node& res)
{
  res.reset();

  braid_init_example_state(res);
  braid_init_explicit_coordset(npts_x,
    npts_y,
    1,
    res["coordsets/coords"]);

  const index_t nele_x = npts_x - 1;
  const index_t nele_y = npts_y - 1;

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";

  res["topologies/mesh/elements/shape"] = "mixed";
  res["topologies/mesh/elements/shape_map/quad"] = 9; // VTK_QUAD
  res["topologies/mesh/elements/shape_map/tri"] = 5; // VTK_TRIANGLE

  /*
   *   ^^^^^^^^^^^^^^^^^^^^^^^^
   *   |--/|---|--/|---|--/|>>>
   *   | / |   | / |   | / |>>>
   *   |/--|---|/--|---|/--|>>>
   */

  const index_t nele_x2 = nele_x / 2;
  const index_t nquads = nele_y * nele_x2;
  const index_t ntris = nele_y * 2 * (nele_x2 + nele_x % 2);
  const index_t nele = nquads + ntris;

  Node &elements = res["topologies/mesh/elements"];
  elements["shapes"].set(DataType::int32(nele));
  elements["sizes"].set(DataType::int32(nele));
  elements["offsets"].set(DataType::int32(nele));
  elements["connectivity"].set(DataType::int32(nquads * 4 + ntris * 3));

  int32* shapes = elements["shapes"].value();
  int32 *sizes = elements["sizes"].value();
  int32 *offsets = elements["offsets"].value();
  offsets[0] = 0;
  int32 *connectivity = elements["connectivity"].value();

  index_t idx_elem(0);
  index_t idx(0);
  for(index_t j = 0; j < nele_y ; ++j)
  {
    for(index_t i = 0; i < nele_x; ++i)
    {
      if (i%2==0)
      {
        constexpr int32 TrianglePointCount = 3;
        shapes[idx_elem+0] = 5; // VTK_TRIANGLE;
        shapes[idx_elem+1] = 5; // VTK_TRIANGLE;
        sizes[idx_elem + 0] = TrianglePointCount;
        sizes[idx_elem + 1] = TrianglePointCount;

        offsets[idx_elem + 1] = offsets[idx_elem + 0] + TrianglePointCount;
        if (idx_elem + 2 < nele)
        {
          offsets[idx_elem + 2] = offsets[idx_elem + 1] + TrianglePointCount;
        }

        connectivity[idx + 0] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 1] = calc(1, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 2] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);

        connectivity[idx + 3] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 4] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 5] = calc(0, 1, 0, i, j, 0, npts_x, npts_y);

        idx_elem += 2;
        idx += 6;
      }
      else
      {
        constexpr int32 QuadPointCount = 4;
        shapes[idx_elem] = 9; // VTK_QUAD;
        sizes[idx_elem] = QuadPointCount;

        if (idx_elem + 1 < nele)
        {
          offsets[idx_elem + 1] = offsets[idx_elem + 0] + QuadPointCount;
        }

        connectivity[idx + 0] = calc(0, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 1] = calc(1, 0, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 2] = calc(1, 1, 0, i, j, 0, npts_x, npts_y);
        connectivity[idx + 3] = calc(0, 1, 0, i, j, 0, npts_x, npts_y);

        idx_elem += 1;
        idx += 4;
      }
    }
  }

  Node& fields = res["fields"];

  braid_init_example_point_scalar_field(npts_x,
    npts_y,
    1,
    fields["braid"]);

  braid_init_example_point_vector_field(npts_x,
    npts_y,
    1,
    fields["vel"]);
}

//---------------------------------------------------------------------------//
void
braid_mixed(int32 npts_x,
            int32 npts_y,
            int32 npts_z,
            Node& res)
{
  res.reset();

  braid_init_example_state(res);
  braid_init_explicit_coordset(npts_x,
    npts_y,
    npts_z,
    res["coordsets/coords"]);

  res["topologies/mesh/type"] = "unstructured";
  res["topologies/mesh/coordset"] = "coords";

  const int32 nele_x = npts_x - 1;
  const int32 nele_y = npts_y - 1;
  const int32 nele_z = npts_z - 1;


  const int32 nele_x2 = nele_x / 2;

  // one hexa subdivided into 3 tetras and polyhedron (prism)
  const int32 ntet = 3 * nele_z * nele_y * (nele_x2 + nele_x % 2);
  const int32 npolyhedra = nele_z * nele_y * (nele_x2 + nele_x % 2);
  // one hexa as hexahedron.
  const int32 nhex = nele_z * nele_y * nele_x2;

  const int32 nfaces = 5 * npolyhedra;
  const int32 nele = ntet + nhex + npolyhedra;

  res["topologies/mesh/elements/shape"] = "mixed";
  res["topologies/mesh/elements/shape_map/polyhedral"] = 42; // VTK_POLYHEDRON
  res["topologies/mesh/elements/shape_map/tet"] = 10; // VTK_TETRA
  res["topologies/mesh/elements/shape_map/hex"] = 12; // VTK_HEXAHEDRON

  Node& elements = res["topologies/mesh/elements"];

  elements["shapes"].set(DataType::int32(nele));
  elements["sizes"].set(DataType::int32(nele));
  elements["offsets"].set(DataType::int32(nele));
  elements["connectivity"].set(DataType::int32(ntet * 4  + npolyhedra * 5 + nhex * 8));

  int32* elem_shapes = elements["shapes"].value();
  int32* elem_sizes = elements["sizes"].value();
  int32* elem_offsets = elements["offsets"].value();
  elem_offsets[0] = 0;
  int32* elem_connectivity = elements["connectivity"].value();

  res["topologies/mesh/subelements/shape"] = "mixed";
  res["topologies/mesh/subelements/shape_map/quad"] = 9; // VTK_QUAD
  res["topologies/mesh/subelements/shape_map/tri"] = 5; // VTK_TRIANGLE

  Node& subelements = res["topologies/mesh/subelements/"];

  subelements["shapes"].set(DataType::int32(nfaces));
  subelements["sizes"].set(DataType::int32(nfaces));
  subelements["offsets"].set(DataType::int32(nfaces));
  subelements["connectivity"].set(DataType::int32(npolyhedra * 18));

  int32* subelem_shapes = subelements["shapes"].value();
  int32* subelem_sizes   = subelements["sizes"].value();
  int32* subelem_offsets = subelements["offsets"].value();
  subelem_offsets[0] = 0;
  int32* subelem_connectivity = subelements["connectivity"].value();

  int32 idx_elem(0);
  int32 idx(0);
  int32 idx_elem2(0);
  int32 idx2(0);
  int32 polyhedronCounter(0);

  for (int32 k = 0; k < nele_z; ++k)
  {
    for (int32 j = 0; j < nele_y; ++j)
    {
      for(int32 i = 0; i < nele_x; ++i)
      {
        if (i%2 == 1) // hexahedron
        {
          constexpr int32 HexaPointCount = 8;
          elem_shapes[idx_elem] = 12; // VTK_HEXAHEDRON
          elem_sizes[idx_elem] = HexaPointCount;
          if (idx_elem + 1 < nele)
          {
            elem_offsets[idx_elem + 1] = elem_offsets[idx_elem] + HexaPointCount;
          }

          elem_connectivity[idx + 0] = calc(0, 0, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 1] = calc(1, 0, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 2] = calc(1, 1, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 3] = calc(0, 1, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 4] = calc(0, 0, 1, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 5] = calc(1, 0, 1, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 6] = calc(1, 1, 1, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 7] = calc(0, 1, 1, i, j, k, npts_x, npts_y);

          idx_elem += 1;
          idx += HexaPointCount;
        }
        else // 3 tets, one polyhedron
        {
          constexpr int TetraPointCount = 4;
          constexpr int WedgeFaceCount = 5;
          constexpr int TrianglePointCount = 3;
          constexpr int QuadPointCount = 4;

          elem_shapes[idx_elem + 0] = 10; // VTK_TETRA
          elem_shapes[idx_elem + 1] = 10; // VTK_TETRA
          elem_shapes[idx_elem + 2] = 10; // VTK_TETRA
          elem_shapes[idx_elem + 3] = 42; // VTK_POLYHEDRON

          elem_sizes[idx_elem + 0] = TetraPointCount;
          elem_sizes[idx_elem + 1] = TetraPointCount;
          elem_sizes[idx_elem + 2] = TetraPointCount;
          elem_sizes[idx_elem + 3] = WedgeFaceCount;

          elem_offsets[idx_elem + 1] = elem_offsets[idx_elem + 0] + TetraPointCount;
          elem_offsets[idx_elem + 2] = elem_offsets[idx_elem + 1] + TetraPointCount;
          elem_offsets[idx_elem + 3] = elem_offsets[idx_elem + 2] + TetraPointCount;
          if (idx_elem + 4 < nele)
          {
            elem_offsets[idx_elem + 4] = elem_offsets[idx_elem + 3] + WedgeFaceCount;
          }

          elem_connectivity[idx + 0]  = calc(0, 0, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 1]  = calc(1, 0, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 2]  = calc(0, 1, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 3]  = calc(0, 0, 1, i, j, k, npts_x, npts_y);

          elem_connectivity[idx + 4]  = calc(1, 0, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 5]  = calc(1, 0, 1, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 6]  = calc(0, 0, 1, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 7]  = calc(0, 1, 1, i, j, k, npts_x, npts_y);

          elem_connectivity[idx + 8]  = calc(0, 0, 1, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 9]  = calc(0, 1, 1, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 10] = calc(0, 1, 0, i, j, k, npts_x, npts_y);
          elem_connectivity[idx + 11] = calc(1, 0, 0, i, j, k, npts_x, npts_y);

          elem_connectivity[idx + 12] = 0 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 13] = 1 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 14] = 2 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 15] = 3 + WedgeFaceCount * polyhedronCounter;
          elem_connectivity[idx + 16] = 4 + WedgeFaceCount * polyhedronCounter;

          subelem_shapes[idx_elem2 + 0] = 9; // VTK_QUAD
          subelem_shapes[idx_elem2 + 1] = 9; // VTK_QUAD
          subelem_shapes[idx_elem2 + 2] = 9; // VTK_QUAD
          subelem_shapes[idx_elem2 + 3] = 5; // VTK_TRIANGLE
          subelem_shapes[idx_elem2 + 4] = 5; // VTK_TRIANGLE

          subelem_sizes[idx_elem2 + 0] = QuadPointCount;
          subelem_sizes[idx_elem2 + 1] = QuadPointCount;
          subelem_sizes[idx_elem2 + 2] = QuadPointCount;
          subelem_sizes[idx_elem2 + 3] = TrianglePointCount;
          subelem_sizes[idx_elem2 + 4] = TrianglePointCount;

          subelem_offsets[idx_elem2 + 1] = subelem_offsets[idx_elem2 + 0] + QuadPointCount;
          subelem_offsets[idx_elem2 + 2] = subelem_offsets[idx_elem2 + 1] + QuadPointCount;
          subelem_offsets[idx_elem2 + 3] = subelem_offsets[idx_elem2 + 2] + QuadPointCount;
          subelem_offsets[idx_elem2 + 4] = subelem_offsets[idx_elem2 + 3] + TrianglePointCount;
          if (idx_elem2 + 5 < nfaces)
          {
            subelem_offsets[idx_elem2 + 5] = subelem_offsets[idx_elem2 + 4] + TrianglePointCount;
          }

          subelem_connectivity[idx2 + 0] = calc(1, 0, 0, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 + 1] = calc(1, 0, 1, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 + 2] = calc(0, 1, 1, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 + 3] = calc(0, 1, 0, i, j, k, npts_x, npts_y);

          subelem_connectivity[idx2 + 4] = calc(1, 0, 0, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 + 5] = calc(1, 1, 0, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 + 6] = calc(1, 1, 1, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 + 7] = calc(1, 0, 1, i, j, k, npts_x, npts_y);

          subelem_connectivity[idx2 + 8] = calc(1, 1, 0, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 + 9] = calc(0, 1, 0, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 +10] = calc(0, 1, 1, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 +11] = calc(1, 1, 1, i, j, k, npts_x, npts_y);

          subelem_connectivity[idx2 +12] = calc(1, 0, 0, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 +13] = calc(0, 1, 0, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 +14] = calc(1, 1, 0, i, j, k, npts_x, npts_y);

          subelem_connectivity[idx2 +15] = calc(1, 1, 1, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 +16] = calc(0, 1, 1, i, j, k, npts_x, npts_y);
          subelem_connectivity[idx2 +17] = calc(1, 0, 1, i, j, k, npts_x, npts_y);

          idx_elem += 4; // three tets, 1 polyhedron
          idx += 3 * TetraPointCount + WedgeFaceCount;
          polyhedronCounter += 1;
          idx_elem2 += WedgeFaceCount; // five faces on the polyhedron
          idx2 += 3 * QuadPointCount + 2 * TrianglePointCount;
        }
      }
    }
  }

  Node& fields = res["fields"];

  braid_init_example_point_scalar_field(npts_x,
    npts_y,
    npts_z,
    fields["braid"]);

  braid_init_example_point_vector_field(npts_x,
    npts_y,
    npts_z,
    fields["vel"]);
}

//---------------------------------------------------------------------------//
void
braid_hexs(index_t npts_x,
           index_t npts_y,
           index_t npts_z,
           Node &res)
{
    res.reset();

    int32 nele_x = (int32)(npts_x - 1);
    int32 nele_y = (int32)(npts_y - 1);
    int32 nele_z = (int32)(npts_z - 1);
    int32 nele = nele_x * nele_y * nele_z;

    braid_init_example_state(res);
    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 npts_z,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "unstructured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/shape"] = "hex";
    res["topologies/mesh/elements/connectivity"].set(DataType::int32(nele*8));
    int32 *conn = res["topologies/mesh/elements/connectivity"].value();

    int32 idx = 0;
    for(int32 k = 0; k < nele_z ; k++)
    {
        int32 zoff = k * (nele_x+1)*(nele_y+1);
        int32 zoff_n = (k+1) * (nele_x+1)*(nele_y+1);

        for(int32 j = 0; j < nele_y ; j++)
        {
            int32 yoff = j * (nele_x+1);
            int32 yoff_n = (j+1) * (nele_x+1);


            for(int32 i = 0; i < nele_x; i++)
            {
                // ordering is same as VTK_HEXAHEDRON

                conn[idx+0] = zoff + yoff + i;
                conn[idx+1] = zoff + yoff + i + 1;
                conn[idx+2] = zoff + yoff_n + i + 1;
                conn[idx+3] = zoff + yoff_n + i;

                conn[idx+4] = zoff_n + yoff + i;
                conn[idx+5] = zoff_n + yoff + i + 1;
                conn[idx+6] = zoff_n + yoff_n + i + 1;
                conn[idx+7] = zoff_n + yoff_n + i;

                idx+=8;
            }
        }
    }

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_x,
                                            nele_y,
                                            nele_z,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);
}

//---------------------------------------------------------------------------//
void
braid_bent_hexs(const Node &spec, Node &res)
{
    res.reset();

    const int32 npts_x = spec["npts_x"].as_int32();
    const int32 npts_y = spec["npts_y"].as_int32();
    const int32 npts_z = spec["npts_z"].as_int32();

    const int32 domain_id = spec["domain_id"].as_int32();

    const double* x = spec["corner_xs"].as_double_ptr();
    const double* y = spec["corner_ys"].as_double_ptr();
    const double* z = spec["corner_zs"].as_double_ptr();

    int32 nele_x = (int32)(npts_x - 1);
    int32 nele_y = (int32)(npts_y - 1);
    int32 nele_z = (int32)(npts_z - 1);

    braid_init_example_state(res);
    res["state/domain_id"] = domain_id;
    braid_init_explicit_lerp_coordset(npts_x,
                                      npts_y,
                                      npts_z,
                                      res["coordsets/coords"],
                                      x, y, z);

    res["topologies/mesh/type"] = "structured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/dims/i"] = (int32)nele_x;
    res["topologies/mesh/elements/dims/j"] = (int32)nele_y;
    res["topologies/mesh/elements/dims/k"] = (int32)nele_z;

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_x,
                                            nele_y,
                                            nele_z,
                                            fields["radial"]);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);
}

//---------------------------------------------------------------------------//
void
setup_nestset(Node &n, const std::string &windowname, int domain_id,
              const std::string &domain_type,
              int dims_i, int dims_j, int dims_k, int nest_ratio)
{
    Node &nset = n["nestsets/nestset"];
    nset["association"] = "element";
    nset["topology"] = "mesh"; // match what braid_bent_hexs does
    Node &window = nset["windows"][windowname];
    window["domain_id"] = domain_id;
    window["domain_type"] = domain_type;
    window["origin/i"] = 0;
    window["origin/j"] = 0;
    if (dims_k > 0) { window["origin/k"] = 0; }
    window["dims/i"] = dims_i;
    window["dims/j"] = dims_j;
    if (dims_k > 0) { window["dims/k"] = dims_j; }
    window["ratio/i"] = nest_ratio;
    window["ratio/j"] = nest_ratio;
    if (dims_k > 0) { window["ratio/k"] = nest_ratio; }
}

//---------------------------------------------------------------------------//
void
nest_quads(const Node &spec, Node &parent, Node &res)
{
    // spec must contain details about nesting, like this:
    //   nest_child_id: 13
    //   nest_ratio: 3
    // res gets filled with a new domain refined from parent, including a nestset
    // parent node gets a nestset tying it to res

    // Call braid_bent_quads, then make the nestset.
    const int32 nest_parent_id = parent["state/domain_id"].as_int32();
    const int32 nest_child_id = spec["nest_child_id"].as_int32();
    const int32 nest_ratio = spec["nest_ratio"].as_int32();
    const int32 parent_npts_x = spec["npts_x"].as_int32();
    const int32 parent_npts_y = spec["npts_y"].as_int32();
    const int32 child_npts_x = nest_ratio * (parent_npts_x - 1) + 1;
    const int32 child_npts_y = nest_ratio * (parent_npts_y - 1) + 1;

    Node child_spec = spec; // we can now change child_spec without changing spec
    child_spec["domain_id"] = nest_child_id;
    child_spec["npts_x"] = child_npts_x;
    child_spec["npts_y"] = child_npts_y;

    braid_bent_quads(child_spec, res);

    // Set up child nestset
    std::ostringstream oss;
    oss << "window_" << nest_child_id << "_" << nest_parent_id;
    const std::string child_wname = oss.str();
    setup_nestset(res, child_wname, nest_parent_id, "parent",
                  child_npts_x - 1, child_npts_y - 1, 0, nest_ratio);

    // Set up parent nestset
    std::ostringstream pss;
    pss << "window_" << nest_parent_id << "_" << nest_child_id;
    const std::string parent_wname = pss.str();
    setup_nestset(parent, parent_wname, nest_child_id, "child",
                  parent_npts_x - 1, parent_npts_y - 1, 0, nest_ratio);

    // Ensure nest level exists for parent, set for child.
    if (!parent.has_path("state/level_id"))
    {
        int32 level_id = 0;
        parent["state/level_id"] = level_id;
    }
    const int32 parent_level_id = parent["state/level_id"].as_int32();
    res["state/level_id"] = parent_level_id + 1;
}

//---------------------------------------------------------------------------//
void
nest_hexs(const Node &spec, Node &parent, Node &res)
{
    // spec must contain details about nesting, like this:
    //   nest_child_id: 13
    //   nest_ratio: 3
    // res gets filled with a new domain refined from parent, including a nestset
    // parent node gets a nestset tying it to res

    // Call braid_bent_hexs, then make the nestset.
    const int32 nest_parent_id = parent["state/domain_id"].as_int32();
    const int32 nest_child_id = spec["nest_child_id"].as_int32();
    const int32 nest_ratio = spec["nest_ratio"].as_int32();
    const int32 parent_npts_x = spec["npts_x"].as_int32();
    const int32 parent_npts_y = spec["npts_y"].as_int32();
    const int32 parent_npts_z = spec["npts_z"].as_int32();
    const int32 child_npts_x = nest_ratio * (parent_npts_x - 1) + 1;
    const int32 child_npts_y = nest_ratio * (parent_npts_y - 1) + 1;
    const int32 child_npts_z = nest_ratio * (parent_npts_z - 1) + 1;

    Node child_spec = spec; // we can now change child_spec without changing spec
    child_spec["domain_id"] = nest_child_id;
    child_spec["npts_x"] = child_npts_x;
    child_spec["npts_y"] = child_npts_y;
    child_spec["npts_z"] = child_npts_z;

    braid_bent_hexs(child_spec, res);

    // Set up child nestset
    std::ostringstream oss;
    oss << "window_" << nest_child_id << "_" << nest_parent_id;
    const std::string child_wname = oss.str();
    setup_nestset(res, child_wname, nest_parent_id, "parent",
                  child_npts_x - 1, child_npts_y - 1, child_npts_z - 1,
                  nest_ratio);

    // Set up parent nestset
    std::ostringstream pss;
    pss << "window_" << nest_parent_id << "_" << nest_child_id;
    const std::string parent_wname = pss.str();
    setup_nestset(parent, parent_wname, nest_child_id, "child",
                  parent_npts_x - 1, parent_npts_y - 1, parent_npts_z - 1,
                  nest_ratio);

    // Ensure nest level exists for parent, set for child.
    if (!parent.has_path("state/level_id"))
    {
        int32 level_id = 0;
        parent["state/level_id"] = level_id;
    }
    const int32 parent_level_id = parent["state/level_id"].as_int32();
    res["state/level_id"] = parent_level_id + 1;
}

//---------------------------------------------------------------------------//
void
braid_tets(index_t npts_x,
           index_t npts_y,
           index_t npts_z,
           Node &res)
{
    res.reset();

    int32 nele_hexs_x = (int32) (npts_x - 1);
    int32 nele_hexs_y = (int32) (npts_y - 1);
    int32 nele_hexs_z = (int32) (npts_z - 1);
    int32 nele_hexs = nele_hexs_x * nele_hexs_y * nele_hexs_z;

    int32 tets_per_hex = 6;
    int32 verts_per_tet = 4;
    int32 n_tets_verts = nele_hexs * tets_per_hex * verts_per_tet;

    braid_init_example_state(res);
    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 npts_z,
                                 res["coordsets/coords"]);


    res["topologies/mesh/type"] = "unstructured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/shape"] = "tet";
    res["topologies/mesh/elements/connectivity"].set(DataType::int32(n_tets_verts));
    int32 *conn = res["topologies/mesh/elements/connectivity"].value();


    int32 idx = 0;
    for(int32 k = 0; k < nele_hexs_z ; k++)
    {
        int32 zoff = k * (nele_hexs_x+1)*(nele_hexs_y+1);
        int32 zoff_n = (k+1) * (nele_hexs_x+1)*(nele_hexs_y+1);

        for(int32 j = 0; j < nele_hexs_y ; j++)
        {
            int32 yoff = j * (nele_hexs_x+1);
            int32 yoff_n = (j+1) * (nele_hexs_x+1);


            for(int32 i = 0; i < nele_hexs_x; i++)
            {
                // Create a local array of the vertex indices
                // ordering is same as VTK_HEXAHEDRON
                int32 vidx[8] =   {zoff + yoff + i
                                  ,zoff + yoff + i + 1
                                  ,zoff + yoff_n + i + 1
                                  ,zoff + yoff_n + i
                                  ,zoff_n + yoff + i
                                  ,zoff_n + yoff + i + 1
                                  ,zoff_n + yoff_n + i + 1
                                  ,zoff_n + yoff_n + i};

                // Create six tets all sharing diagonal from vertex 0 to 6
                // Uses SILO convention for vertex order (normals point in)
                conn[idx++] = vidx[0];
                conn[idx++] = vidx[2];
                conn[idx++] = vidx[1];
                conn[idx++] = vidx[6];

                conn[idx++] = vidx[0];
                conn[idx++] = vidx[3];
                conn[idx++] = vidx[2];
                conn[idx++] = vidx[6];

                conn[idx++] = vidx[0];
                conn[idx++] = vidx[7];
                conn[idx++] = vidx[3];
                conn[idx++] = vidx[6];

                conn[idx++] = vidx[0];
                conn[idx++] = vidx[4];
                conn[idx++] = vidx[7];
                conn[idx++] = vidx[6];

                conn[idx++] = vidx[0];
                conn[idx++] = vidx[5];
                conn[idx++] = vidx[4];
                conn[idx++] = vidx[6];

                conn[idx++] = vidx[0];
                conn[idx++] = vidx[1];
                conn[idx++] = vidx[5];
                conn[idx++] = vidx[6];

            }
        }
    }

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_hexs_x,
                                            nele_hexs_y,
                                            nele_hexs_z,
                                            fields["radial"],
                                            tets_per_hex);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);

}


//---------------------------------------------------------------------------//
void
braid_lines_3d(index_t npts_x,
               index_t npts_y,
               index_t npts_z,
               Node &res)
{
    res.reset();

    int32 nele_hexs_x = (int32)(npts_x - 1);
    int32 nele_hexs_y = (int32)(npts_y - 1);
    int32 nele_hexs_z = (int32)(npts_z - 1);
    int32 nele_hexs = nele_hexs_x * nele_hexs_y * nele_hexs_z;


    braid_init_example_state(res);
    braid_init_explicit_coordset(npts_x,
                                 npts_y,
                                 npts_z,
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "unstructured";
    res["topologies/mesh/coordset"] = "coords";
    res["topologies/mesh/elements/shape"] = "line";
    res["topologies/mesh/elements/connectivity"].set(DataType::int32(nele_hexs * 12 * 2));
    int32 *conn = res["topologies/mesh/elements/connectivity"].value();

    int32 idx = 0;
    for(int32 k = 0; k < nele_hexs_z ; k++)
    {
        int32 zoff = k * (nele_hexs_x+1)*(nele_hexs_y+1);
        int32 zoff_n = (k+1) * (nele_hexs_x+1)*(nele_hexs_y+1);

        for(int32 j = 0; j < nele_hexs_y ; j++)
        {
            int32 yoff = j * (nele_hexs_x+1);
            int32 yoff_n = (j+1) * (nele_hexs_x+1);


            for(int32 i = 0; i < nele_hexs_z; i++)
            {
                // 12 lines per hex
                // Note: this pattern allows for simple per-hex construction,
                // but it creates spatially overlapping lines

                // front face
                conn[idx++] = zoff + yoff + i;
                conn[idx++] = zoff + yoff + i +1;

                conn[idx++] = zoff + yoff + i + 1;
                conn[idx++] = zoff + yoff_n + i + 1;

                conn[idx++] = zoff + yoff_n + i + 1;
                conn[idx++] = zoff + yoff_n + i;

                conn[idx++] = zoff + yoff_n + i;
                conn[idx++] = zoff + yoff + i;

                // back face
                conn[idx++] = zoff_n + yoff + i;
                conn[idx++] = zoff_n + yoff + i +1;

                conn[idx++] = zoff_n + yoff + i + 1;
                conn[idx++] = zoff_n + yoff_n + i + 1;

                conn[idx++] = zoff_n + yoff_n + i + 1;
                conn[idx++] = zoff_n + yoff_n + i;

                conn[idx++] = zoff_n + yoff_n + i;
                conn[idx++] = zoff_n + yoff + i;

                // sides
                conn[idx++] = zoff   + yoff + i;
                conn[idx++] = zoff_n + yoff + i;

                conn[idx++] = zoff   + yoff + i + 1;
                conn[idx++] = zoff_n + yoff + i + 1;

                conn[idx++] = zoff   + yoff_n + i + 1;
                conn[idx++] = zoff_n + yoff_n + i + 1;

                conn[idx++] = zoff   + yoff_n + i;
                conn[idx++] = zoff_n + yoff_n + i;

            }
        }
    }

    Node &fields = res["fields"];

    braid_init_example_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["braid"]);

    braid_init_example_element_scalar_field(nele_hexs_x,
                                            nele_hexs_y,
                                            nele_hexs_z,
                                            fields["radial"],
                                            12);

    braid_init_example_point_vector_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          fields["vel"]);

}


//---------------------------------------------------------------------------//
void
braid_to_poly(Node &res)
{
    const index_t topo_count = res["topologies"].number_of_children();
    std::vector<Node> poly_topos(topo_count);
    std::vector<std::string> topo_names(topo_count);

    conduit::NodeConstIterator topos_it = res["topologies"].children();
    while(topos_it.has_next())
    {
        const conduit::Node &topo_node = topos_it.next();
        const std::string topo_name = topos_it.name();
        const index_t topo_index = topos_it.index();

        conduit::Node &poly_node = poly_topos[topo_index];
        blueprint::mesh::topology::unstructured::to_polygonal(topo_node, poly_node);
        topo_names[topo_index] = topo_name;
    }

    res["topologies"].reset();
    for(index_t ti = 0; ti < topo_count; ti++)
    {
        res["topologies"][topo_names[ti]].set(poly_topos[ti]);
    }
}


//---------------------------------------------------------------------------//
void
braid_to_wedges(const Node &braid_regular, Node &res)
{
    // preserve state
    res["state"].set(braid_regular["state"]);

    // preserve coordsets
    res["coordsets"].set(braid_regular["coordsets"]);

    const int old_conn_size = braid_regular["topologies/mesh/elements/connectivity"].dtype().number_of_elements();
    const int points_per_hex = 8;
    const int points_per_wedge = 6;
    const int num_hexes = old_conn_size / points_per_hex;
    const int num_wedges = num_hexes * 2;
    const int new_conn_size = num_wedges * points_per_wedge;

    //
    // Set up topology
    //
    Node &res_topo = res["topologies/mesh"];
    res_topo["type"] = braid_regular["topologies/mesh/type"];
    res_topo["coordset"] = braid_regular["topologies/mesh/coordset"];
    res_topo["elements/shape"] = "wedge";
    res_topo["elements/connectivity"].set(conduit::DataType::int32(new_conn_size));

    const int32 *old_conn_ptr = braid_regular["topologies/mesh/elements/connectivity"].value();
    int32 *new_conn_ptr = res_topo["elements/connectivity"].value();
    int j = 0; // iterator to go thru wedges
    // for each hex
    for (int i = 0; i < old_conn_size; i += points_per_hex)
    {
        // from hex: {0,1,2,3,4,5,6,7}
        // we want two wedges:
        // 1: {0,1,2,4,5,6}
        // 2: {0,3,2,4,7,6}
        new_conn_ptr[j] = old_conn_ptr[i];
        new_conn_ptr[j + 1] = old_conn_ptr[i + 1];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 2];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 4];
        new_conn_ptr[j + 4] = old_conn_ptr[i + 5];
        new_conn_ptr[j + 5] = old_conn_ptr[i + 6];
        j += points_per_wedge;
        new_conn_ptr[j] = old_conn_ptr[i];
        new_conn_ptr[j + 1] = old_conn_ptr[i + 3];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 2];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 4];
        new_conn_ptr[j + 4] = old_conn_ptr[i + 7];
        new_conn_ptr[j + 5] = old_conn_ptr[i + 6];
        j += points_per_wedge;
    }

    //
    // Set up fields
    //
    Node &res_fields = res["fields"];

    // preserve vertex-associated field
    res_fields["braid"].set(braid_regular["fields/braid"]);

    // double up elements in element associated fields
    res_fields["radial/association"] = braid_regular["fields/radial/association"];
    res_fields["radial/topology"] = braid_regular["fields/radial/topology"];

    res_fields["radial/values"].set(conduit::DataType::float64(num_wedges));
    const float64 *old_vals_ptr = braid_regular["fields/radial/values"].value();
    float64 *new_vals_ptr = res_fields["radial/values"].value();
    // for each hex
    for (int i = 0; i < num_hexes; i ++)
    {
        // there are two new values for every old value
        new_vals_ptr[i * 2] = old_vals_ptr[i];
        new_vals_ptr[i * 2 + 1] = old_vals_ptr[i];
    }

    // preserve vertex-associated field
    res_fields["vel"].set(braid_regular["fields/vel"]);
}


//---------------------------------------------------------------------------//
void
braid_to_pyramids(index_t npts_x,
                  index_t npts_y,
                  index_t npts_z,
                  const Node &braid_regular,
                  Node &res)
{
    // preserve state
    res["state"].set(braid_regular["state"]);

    //
    // Set up coordset
    //
    Node &res_coords = res["coordsets/coords"];
    res_coords["type"] = braid_regular["coordsets/coords/type"];
    const int old_num_pts = braid_regular["coordsets/coords/values/x"].dtype().number_of_elements();
    const int old_conn_size = braid_regular["topologies/mesh/elements/connectivity"].dtype().number_of_elements();
    const int points_per_hex = 8;
    const int points_per_pyramid = 5;
    const int num_hexes = old_conn_size / points_per_hex;
    const int num_pyramids = num_hexes * 6;
    const int new_conn_size = num_pyramids * points_per_pyramid;
    // we are adding one new point in the center of each hex
    const int new_num_pts = old_num_pts + num_hexes;

    const int32 *old_conn_ptr = braid_regular["topologies/mesh/elements/connectivity"].value();

    res_coords["values/x"].set(conduit::DataType::float64(new_num_pts));
    const float64 *old_xvals = braid_regular["coordsets/coords/values/x"].value();
    float64 *new_xvals = res_coords["values/x"].value();

    res_coords["values/y"].set(conduit::DataType::float64(new_num_pts));
    const float64 *old_yvals = braid_regular["coordsets/coords/values/y"].value();
    float64 *new_yvals = res_coords["values/y"].value();

    res_coords["values/z"].set(conduit::DataType::float64(new_num_pts));
    const float64 *old_zvals = braid_regular["coordsets/coords/values/z"].value();
    float64 *new_zvals = res_coords["values/z"].value();

    // add the old points
    for (int i = 0; i < old_num_pts; i ++)
    {
        new_xvals[i] = old_xvals[i];
        new_yvals[i] = old_yvals[i];
        new_zvals[i] = old_zvals[i];
    }

    // add the new points at the end
    for (int i = 0; i < num_hexes; i ++)
    {
        float64 x, y, z;
        x = y = z = 0;

        // take the average of the 8 pts of the hex
        for (int j = 0; j < points_per_hex; j ++)
        {
            x += old_xvals[old_conn_ptr[i * points_per_hex + j]];
            y += old_yvals[old_conn_ptr[i * points_per_hex + j]];
            z += old_zvals[old_conn_ptr[i * points_per_hex + j]];
        }
        x /= points_per_hex;
        y /= points_per_hex;
        z /= points_per_hex;

        new_xvals[old_num_pts + i] = x;
        new_yvals[old_num_pts + i] = y;
        new_zvals[old_num_pts + i] = z;
    }

    //
    // Set up topology
    //
    Node &res_topo = res["topologies/mesh"];
    res_topo["type"] = braid_regular["topologies/mesh/type"];
    res_topo["coordset"] = braid_regular["topologies/mesh/coordset"];
    res_topo["elements/shape"] = "pyramid";
    res_topo["elements/connectivity"].set(conduit::DataType::int32(new_conn_size));

    int32 *new_conn_ptr = res_topo["elements/connectivity"].value();
    int j = 0; // iterator to go thru pyramids
    int k = old_num_pts; // index representing the entries in the new pts arrays
    // for each hex
    for (int i = 0; i < old_conn_size; i += points_per_hex)
    {
        // from hex: {0,1,2,3,4,5,6,7} + bonus midpoint 8
        // we want 6 pyramids:
        // 1: {0,3,2,1,8}
        new_conn_ptr[j] = old_conn_ptr[i];
        new_conn_ptr[j + 1] = old_conn_ptr[i + 3];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 2];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 1];
        new_conn_ptr[j + 4] = k;
        j += points_per_pyramid;
        // 2: {0,1,5,4,8}
        new_conn_ptr[j] = old_conn_ptr[i];
        new_conn_ptr[j + 1] = old_conn_ptr[i + 1];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 5];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 4];
        new_conn_ptr[j + 4] = k;
        j += points_per_pyramid;
        // 3: {1,2,6,5,8}
        new_conn_ptr[j] = old_conn_ptr[i + 1];
        new_conn_ptr[j + 1] = old_conn_ptr[i + 2];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 6];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 5];
        new_conn_ptr[j + 4] = k;
        j += points_per_pyramid;
        // 4: {2,3,7,6,8}
        new_conn_ptr[j] = old_conn_ptr[i + 2];
        new_conn_ptr[j + 1] = old_conn_ptr[i + 3];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 7];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 6];
        new_conn_ptr[j + 4] = k;
        j += points_per_pyramid;
        // 5: {3,0,4,7,8}
        new_conn_ptr[j] = old_conn_ptr[i + 3];
        new_conn_ptr[j + 1] = old_conn_ptr[i];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 4];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 7];
        new_conn_ptr[j + 4] = k;
        j += points_per_pyramid;
        // 6: {4,5,6,7,8}
        new_conn_ptr[j] = old_conn_ptr[i + 4];
        new_conn_ptr[j + 1] = old_conn_ptr[i + 5];
        new_conn_ptr[j + 2] = old_conn_ptr[i + 6];
        new_conn_ptr[j + 3] = old_conn_ptr[i + 7];
        new_conn_ptr[j + 4] = k;
        j += points_per_pyramid;
        // We stored all the new points at the end of the points arrays.
        // k indexes into them. There are num_hexes new points, and
        // k is incremented old_conn_size / points_per_hex = num_hexes
        // times.
        k ++;
    }

    //
    // Set up fields
    //
    Node &res_fields = res["fields"];

    // handle vertex-associated field
    res_fields["braid/association"] = braid_regular["fields/braid/association"];
    res_fields["braid/topology"] = braid_regular["fields/braid/topology"];
    res_fields["braid/values"].set(conduit::DataType::float64(new_num_pts));

    const float64 *old_braid_ptr = braid_regular["fields/braid/values"].value();
    float64 *new_braid_ptr = res_fields["braid/values"].value();
    // copy over the old field values
    for (int i = 0; i < old_num_pts; i ++)
    {
        new_braid_ptr[i] = old_braid_ptr[i];
    }
    // calculate the new field values for the new points
    float64 dx = braid_init_example_point_scalar_field_calc_dx(npts_x);
    float64 dy = braid_init_example_point_scalar_field_calc_dy(npts_y);
    float64 dz = braid_init_example_point_scalar_field_calc_dz(npts_z);
    if (num_hexes != (npts_x - 1) * (npts_y - 1) * (npts_z - 1))
    {
        CONDUIT_ERROR("Cannot create pyramid; mismatch with provided dimensions and number of hexahedrons.");
    }
    // the following loop iterates
    // (npts_x - 1) * (npts_y - 1) * (npts_z - 1) == num_hexes
    // == new_num_pts - old_num_pts times.
    int index = old_num_pts;
    for (index_t k = 0; k < npts_z - 1; k ++)
    {
        for (index_t j = 0; j < npts_y - 1; j ++)
        {
            for (index_t i = 0; i < npts_x - 1; i ++)
            {
                new_braid_ptr[index] = braid_init_example_point_scalar_field_calc_single_val(
                    dx, dy, dz, i + 0.5, j + 0.5, k + 0.5, npts_z);
                index ++;
            }
        }
    }

    // 6x elements in element associated fields
    res_fields["radial/association"] = braid_regular["fields/radial/association"];
    res_fields["radial/topology"] = braid_regular["fields/radial/topology"];
    res_fields["radial/values"].set(conduit::DataType::float64(num_pyramids));

    const float64 *old_vals_ptr = braid_regular["fields/radial/values"].value();
    float64 *new_vals_ptr = res_fields["radial/values"].value();
    // for each hex
    for (int i = 0; i < num_hexes; i ++)
    {
        // there are six new values for every old value - one for each face
        new_vals_ptr[i * 6] = old_vals_ptr[i];
        new_vals_ptr[i * 6 + 1] = old_vals_ptr[i];
        new_vals_ptr[i * 6 + 2] = old_vals_ptr[i];
        new_vals_ptr[i * 6 + 3] = old_vals_ptr[i];
        new_vals_ptr[i * 6 + 4] = old_vals_ptr[i];
        new_vals_ptr[i * 6 + 5] = old_vals_ptr[i];
    }

    // handle vertex-associated field
    res_fields["vel/association"] = braid_regular["fields/vel/association"];
    res_fields["vel/topology"] = braid_regular["fields/vel/topology"];
    res_fields["vel/values/u"].set(conduit::DataType::float64(new_num_pts));
    res_fields["vel/values/v"].set(conduit::DataType::float64(new_num_pts));
    res_fields["vel/values/w"].set(conduit::DataType::float64(new_num_pts));
    const float64 *old_vel_u_ptr = braid_regular["fields/vel/values/u"].value();
    float64 *new_vel_u_ptr = res_fields["vel/values/u"].value();
    const float64 *old_vel_v_ptr = braid_regular["fields/vel/values/v"].value();
    float64 *new_vel_v_ptr = res_fields["vel/values/v"].value();
    const float64 *old_vel_w_ptr = braid_regular["fields/vel/values/w"].value();
    float64 *new_vel_w_ptr = res_fields["vel/values/w"].value();
    // copy over the old field values
    for (int i = 0; i < old_num_pts; i ++)
    {
        new_vel_u_ptr[i] = old_vel_u_ptr[i];
        new_vel_v_ptr[i] = old_vel_v_ptr[i];
        new_vel_w_ptr[i] = old_vel_w_ptr[i];
    }
    // calculate the new field values for the new points
    dx = braid_init_example_point_vector_field_calc_dxyz(npts_x);
    dy = braid_init_example_point_vector_field_calc_dxyz(npts_y);
    dz = braid_init_example_point_vector_field_calc_dxyz(npts_z);
    index = old_num_pts;
    for (index_t k = 0; k < npts_z - 1; k ++)
    {
        for (index_t j = 0; j < npts_y - 1; j ++)
        {
            for (index_t i = 0; i < npts_x - 1; i ++)
            {
                braid_init_example_point_vector_field_calc_single_val(
                    dx, dy, dz, i + 0.5, j + 0.5, k + 0.5,
                    new_vel_u_ptr, new_vel_v_ptr, new_vel_w_ptr, index);
                index ++;
            }
        }
    }
}


//---------------------------------------------------------------------------//
void
basic(const std::string &mesh_type,
      index_t npts_x, // number of points in x
      index_t npts_y, // number of points in y
      index_t npts_z, // number of points in z
      Node &res)
{
    // NOTE(JRC): The basic mesh example only supports simple, homogenous
    // element types that can be spanned by zone-centered fields.
    const std::string mesh_types[] = {
        "uniform", "rectilinear", "structured",
        "tris", "quads", "polygons",
        "tets", "hexs", "polyhedra",
        "wedges", "pyramids"};
    const std::string braid_types[] = {
        "uniform", "rectilinear", "structured",
        "tris", "quads", "quads_poly",
        "tets", "hexs", "hexs_poly",
        "wedges", "pyramids"};
    const index_t mesh_types_dims[] = {
        2, 2, 2,
        2, 2, 2,
        3, 3, 3,
        3, 3};
    const index_t mesh_types_subelems_per_elem[] = {
        1, 1, 1,
        2, 1, 1,
        6, 1, 1,
        2, 6};

    const index_t num_mesh_types = sizeof(mesh_types) / sizeof(std::string);

    index_t mesh_type_index = -1;
    for(index_t i = 0; i < num_mesh_types; i++)
    {
        if(mesh_type == mesh_types[i])
        {
            mesh_type_index = i;
        }
    }
    if(mesh_type_index < 0 || mesh_type_index >= num_mesh_types)
    {
        CONDUIT_ERROR("blueprint::mesh::examples::basic unknown mesh_type = "
                      << mesh_type);
    }

    int mesh_type_dim = mesh_types_dims[mesh_type_index];
    if (npts_y == 0 && npts_z == 0)
    {
        mesh_type_dim = 1;
    }

    const bool npts_x_ok = npts_x > 1;
    const bool npts_y_ok = mesh_type_dim == 1 || npts_y > 1;
    bool npts_z_ok = mesh_type_dim == 1 || mesh_type_dim == 2 || npts_z > 1;


    if( npts_z != 0 &&
        braid_2d_only_shape_type(mesh_type) )
    {
        npts_z_ok = false;
    }

    // don't let de-morgan get you ...
    if( ! (npts_x_ok && npts_y_ok && npts_z_ok) )
    {
        // error, not enough points to create the topo
        CONDUIT_ERROR("blueprint::mesh::examples::basic requires: " << std::endl <<
                      "For 1D only topologies,"
                      " mesh_type={\"uniform\" or \"structured\"} and "
                      " npts_x > 1 and npts_y == 0 and npts_z == 0"
                      << std::endl <<
                      "For 2D only topologies"
                      " ( mesh_type={\"tris\", \"quads\", or \"polygons\"} )"
                      " npts_x > 1 and npts_y > 1 and npts_z == 0"
                      << std::endl <<
                      "For 3D only topologies"
                      " ( mesh_type={\"tets\", \"hexs\", \"wedges\", \"pyramids\", or \"polyhedra\"} )"
                      "npts_x > 1 and npts_y > 1 and "
                      " npts_z > 1"
                      << std::endl <<
                      "values provided:" << std::endl <<
                      " mesh_type: " << mesh_type << std::endl <<
                      " npts_x: " << npts_x << std::endl <<
                      " npts_y: " << npts_y << std::endl <<
                      " npts_z: " << npts_z << std::endl);
    }

    braid(braid_types[mesh_type_index], npts_x, npts_y, npts_z, res);
    res.remove("fields");
    res.remove("state");

    basic_init_example_element_scalar_field(npts_x-1, npts_y-1, npts_z-1,
        res["fields/field"], mesh_types_subelems_per_elem[mesh_type_index]);
}

void
fill_if_array_exists(Node &desc, const std::string &path, bool threeD, index_t parm[3])
{
    if (desc.has_path(path))
    {
        index_t_array pvals = desc[path].value();
        parm[0] = pvals[0];
        parm[1] = pvals[1];
        if (threeD)
        {
            parm[2] = pvals[2];
        }
    }
}

//---------------------------------------------------------------------------//
void
strided_structured(Node &desc, // shape of requested data arrays
                   index_t npts_x, // number of points in x
                   index_t npts_y, // number of points in y
                   index_t npts_z, // number of points in z
                   Node &res)
{
    // =================================================================
    // default shapes and origins of vertex and element arrays
    index_t pts_extent[] = {npts_x + 3, npts_y + 3, 0};
    index_t pts_origin[] = {2, 2, 0};
    index_t ele_extent[] = {npts_x + 3, npts_y + 3, 0};
    index_t ele_origin[] = {2, 2, 0};
    if (npts_z > 0)
    {
        pts_extent[2] = npts_z + 3;
        pts_origin[2] = 2;
        ele_extent[2] = npts_z + 3;
        ele_origin[2] = 2;
    }

    index_t nele_x = npts_x - 1;
    index_t nele_y = npts_y - 1;
    index_t nele_z = 0;
    if (npts_z > 0)
    {
        nele_z = npts_z - 1;
    }

    fill_if_array_exists(desc, "vertex_data/shape", (npts_z > 0), pts_extent);
    fill_if_array_exists(desc, "vertex_data/origin", (npts_z > 0), pts_origin);
    fill_if_array_exists(desc, "element_data/shape", (npts_z > 0), ele_extent);
    fill_if_array_exists(desc, "element_data/origin", (npts_z > 0), ele_origin);

    const bool npts_x_ok = npts_x > 1;
    const bool npts_y_ok = npts_y > 1;
    const bool npts_z_ok = npts_z >= 0;

    bool ele_ext_orig_ok = true;
    ele_ext_orig_ok = ele_ext_orig_ok && ele_extent[0] - ele_origin[0] >= npts_x - 1;
    ele_ext_orig_ok = ele_ext_orig_ok && ele_extent[1] - ele_origin[1] >= npts_y - 1;
    if (npts_z > 0)
    {
        ele_ext_orig_ok = ele_ext_orig_ok && ele_extent[2] - ele_origin[2] >= npts_z - 1;
    }

    bool pts_ext_orig_ok = true;
    pts_ext_orig_ok = pts_ext_orig_ok && pts_extent[0] - pts_origin[0] >= npts_x;
    pts_ext_orig_ok = pts_ext_orig_ok && pts_extent[1] - pts_origin[1] >= npts_y;
    if (npts_z > 0)
    {
        pts_ext_orig_ok = pts_ext_orig_ok && pts_extent[2] - pts_origin[2] >= npts_z;
    }

    // don't let de-morgan get you ...
    if( ! (npts_x_ok && npts_y_ok && npts_z_ok && ele_ext_orig_ok && pts_ext_orig_ok) )
    {
        // error, not enough points or storage to create the topo
        CONDUIT_ERROR("blueprint::mesh::examples::strided_structured requires: " << std::endl <<
                      "For 2D, npts_x > 1 and npts_y > 1 and npts_z == 0"
                      << std::endl <<
                      "For 3D, npts_x > 1 and npts_y > 1 and npts_z > 1"
                      << std::endl <<
                      "For all dimensions, elements extent - elements origin >= npts - 1"
                      << std::endl <<
                      "For all dimensions, points extent - points origin >= npts"
                      << std::endl <<
                      "values provided:" << std::endl <<
                      " npts_x: " << npts_x << std::endl <<
                      " npts_y: " << npts_y << std::endl <<
                      " npts_z: " << npts_z << std::endl <<
                      " elements extent: (" << ele_extent[0] << ", " <<
                      ele_extent[1] << ", " << ele_extent[2] << ")"
                      << std::endl <<
                      " elements origin: (" << ele_origin[0] << ", " <<
                      ele_origin[1] << ", " << ele_origin[2] << ")"
                      << std::endl <<
                      " points extent: (" << pts_extent[0] << ", " <<
                      pts_extent[1] << ", " << pts_extent[2] << ")"
                      << std::endl <<
                      " points origin: (" << pts_origin[0] << ", " <<
                      pts_origin[1] << ", " << pts_origin[2] << ")"
                      << std::endl
            );
    }

    braid_init_example_state(res);
    braid_init_explicit_coordset(pts_extent[0],
                                 pts_extent[1],
                                 pts_extent[2],
                                 res["coordsets/coords"]);

    res["topologies/mesh/type"] = "structured";
    res["topologies/mesh/coordset"] = "coords";

    Node &dims = res["topologies/mesh/elements/dims"];

    dims["i"] = (int32)nele_x;
    dims["j"] = (int32)nele_y;
    if(nele_z > 0)
    {
        dims["k"] = (int32)nele_z;
    }

    index_t dimensions = 2;
    if (npts_z > 0)
    {
        dimensions += 1;
    }
    dims["offsets"].set(DataType::int32(dimensions));
    dims["strides"].set(DataType::int32(dimensions));
    int32 *offsets = dims["offsets"].value();
    int32 *strides = dims["strides"].value();
    // fill offsets
    offsets[0] = ele_origin[0];
    offsets[1] = ele_origin[1];
    if (npts_z > 0)
    {
        offsets[2] = ele_origin[2];
    }
    // fill strides
    strides[0] = 1;
    strides[1] = strides[0] * pts_extent[0];
    if (npts_z > 0)
    {
        strides[2] = strides[1] * pts_extent[1];
    }

    Node &fields = res["fields"];

    strided_structured_point_scalar_field(npts_x,
                                          npts_y,
                                          npts_z,
                                          pts_origin[0],
                                          pts_origin[1],
                                          pts_origin[2],
                                          pts_extent[0],
                                          pts_extent[1],
                                          pts_extent[2],
                                          fields["vert_vals"]);

    strided_structured_element_scalar_field(nele_x,
                                            nele_y,
                                            nele_z,
                                            ele_origin[0],
                                            ele_origin[1],
                                            ele_origin[2],
                                            ele_extent[0],
                                            ele_extent[1],
                                            ele_extent[2],
                                            fields["ele_vals"]);

}



//---------------------------------------------------------------------------//
void bent_multi_grid_defaults(conduit::Node &spec)
{
    // Provide defaults for bent_multi_grid so a user can call that function
    // without having to remember what goes in spec
    auto fill_domain = [&spec] (int npts_x, int npts_y, int domain_id,
        const char * label,
        std::initializer_list<double> corner_xs,
        std::initializer_list<double> corner_ys)
        {
            conduit::Node &dom = spec[label];
            dom["npts_x"] = npts_x;
            dom["npts_y"] = npts_y;
            dom["domain_id"] = domain_id;
            conduit::Node& c_xs = dom["corner_xs"];
            c_xs.set(corner_xs.begin(), corner_xs.size());
            conduit::Node& c_ys = dom["corner_ys"];
            c_ys.set(corner_ys.begin(), corner_ys.size());
        };

    fill_domain(3, 4, 0, "domain0", { 0, 2, 2, 0 }, { 0, 0, 1, 1 });
    fill_domain(3, 4, 1, "domain1", { 2, 4, 4, 2 }, { 0, 0, 2, 1 });
    fill_domain(3, 3, 2, "domain2", { 0, 2, 4, 0 }, { 1, 1, 2, 2 });
    fill_domain(3, 2, 3, "domain3", { 0, 4, 4, 0 }, { 2, 2, 3, 3 });
    fill_domain(2, 4, 4, "domain4", { 4, 5, 5, 4 }, { 0, 0, 2, 2 });
    fill_domain(2, 2, 5, "domain5", { 4, 5, 5, 4 }, { 2, 3, 3, 2 });
}

void CONDUIT_BLUEPRINT_API bent_multi_grid(const conduit::Node &spec,
                                           conduit::Node &res)
{
    int last_dim = 0;

    if (spec.dtype().is_empty())
    {
        conduit::Node default_spec;
        bent_multi_grid_defaults(default_spec);
        bent_multi_grid(default_spec, res);
    }
    else
    {
        // verify each child node in spec.
        // 1. make sure each is dimensionally self-consistent
        // 2. make sure dimensionality is equal among all domains
        std::vector<int> dom_dimensions;
        NodeConstIterator ver_it = spec.children();
        while (ver_it.has_next())
        {
            const Node& ver_node = ver_it.next();
            // Check for necessary domain information
            bool has_corner_xs = ver_node.has_child("corner_xs");
            bool has_corner_ys = ver_node.has_child("corner_ys");
            bool has_corner_zs = ver_node.has_child("corner_zs");
            bool has_npts_x = ver_node.has_child("npts_x");
            bool has_npts_y = ver_node.has_child("npts_y");
            bool has_npts_z = ver_node.has_child("npts_z");
            bool has_domain_id = ver_node.has_child("domain_id");

            // Check for nest set information.
            bool has_nest_child_id = ver_node.has_child("nest_child_id");
            bool has_nest_ratio = ver_node.has_child("nest_ratio");
            // Need both nest_child_id and nest_ratio, or neither.
            bool nest_ok = (has_nest_child_id == has_nest_ratio);

            bool dims_ok = false;
            int dims = 0;
            if (has_corner_xs && has_corner_ys && has_npts_x && has_npts_y) { dims = 2; }
            if (dims == 2 && has_corner_zs && has_npts_z) { dims = 3; }

            bool children_ok = (dims == 2 || dims == 3) && has_domain_id && nest_ok;

            if (children_ok)
            {
                dims_ok = ver_node["corner_xs"].as_float64_accessor().number_of_elements() > 3;
                dims_ok = dims_ok && ver_node["corner_ys"].as_float64_accessor().number_of_elements() > 3;
                int npts_x = ver_node["npts_x"].as_int32();
                dims_ok = dims_ok && npts_x >= 2;
                int npts_y = ver_node["npts_y"].as_int32();
                dims_ok = dims_ok && npts_y >= 2;
                if (dims > 2)
                {
                    dims_ok = dims_ok && ver_node["corner_xs"].as_float64_accessor().number_of_elements() > 7;
                    dims_ok = dims_ok && ver_node["corner_ys"].as_float64_accessor().number_of_elements() > 7;
                    dims_ok = dims_ok && ver_node["corner_zs"].as_float64_accessor().number_of_elements() > 7;
                    int npts_z = ver_node["npts_z"].as_int32();
                    dims_ok = dims_ok && npts_z >= 2;
                }

                // If the nesting specification child nodes are present,
                // make sure they're integral.
                if (has_nest_child_id && has_nest_ratio)
                {
                    nest_ok = ver_node["nest_child_id"].dtype().is_int() &&
                        ver_node["nest_ratio"].dtype().is_int();
                }
            }

            if (!(children_ok && dims_ok && nest_ok))
            {
                // error: some domain doesn't have the right children.
                CONDUIT_ERROR("blueprint::mesh::examples::bent_multi_grid requires each domain in spec " << std::endl <<
                    "to have floating-point children corner_xs, corner_ys each with length > 3." << std::endl <<
                    "For a 3D mesh, each domain must have corner_xs, corner_ys, corner_zs each" << std::endl <<
                    "with length > 7.  Each domain must also have an integral domain_id and integral" << std::endl <<
                    "npts_x, npts_y, and optionally npts_z." << std::endl <<
                    "To specify a nesting domain, the parent domain must have integral children" << std::endl <<
                    "nest_child_id and nest_ratio." << std::endl << "Values provided : " << std::endl <<
                    ver_node.to_yaml() << std::endl);
            }

            last_dim = dims;
            dom_dimensions.push_back(dims);
            // TODO: report error if all dimensions are not equal
        }

        // For each child node in spec,
        // call braid()
        NodeConstIterator doms_it = spec.children();
        while (doms_it.has_next())
        {
            const Node& dom_node = doms_it.next();
            // I need to construct a name and pass in res[thename], not just res.
            std::string thename;
            // If spec is a list, use a numerical value that we increment.  Otherwise, use the name of this particular child.
            bent_braid(dom_node, res[doms_it.name()]);
        }

        // Tie adjacent domains together with adjsets.
        braid_init_example_adjset(res);

        // Now refine selected domains.
        doms_it.to_front();
        conduit::Node nested_doms;
        while (doms_it.has_next())
        {
            const Node& dom_node = doms_it.next();
            if (dom_node.has_child("nest_child_id"))
            {
                int child_id = dom_node["nest_child_id"].as_int();
                std::ostringstream oss;
                // Match the naming style used by bent_braid(), which is domain0, domain1, etc.
                oss << "domain" << child_id;
                const std::string nest_name = oss.str();
                if (last_dim == 2)
                {
                    nest_quads(dom_node, res[doms_it.name()], nested_doms[nest_name]);
                }
                else
                {
                    nest_hexs(dom_node, res[doms_it.name()], nested_doms[nest_name]);
                }
            }
        }

        // Tie adjacent refined domains together with adjsets.
        braid_init_example_adjset(nested_doms);

        // Now store the new domains back into the result.
        NodeConstIterator nested_doms_it = nested_doms.children();
        while (nested_doms_it.has_next())
        {
            const Node& ndom_node = nested_doms_it.next();
            res[nested_doms_it.name()] = ndom_node;
        }
    }
}

void CONDUIT_BLUEPRINT_API bent_multi_grid_amr(const conduit::Node &spec,
                                               conduit::Node &res)
{
    if (spec.dtype().is_empty())
    {
        conduit::Node default_spec;
        bent_multi_grid_defaults(default_spec);
        default_spec["domain0/nest_child_id"] = 6;
        default_spec["domain0/nest_ratio"] = 3;
        default_spec["domain1/nest_child_id"] = 7;
        default_spec["domain1/nest_ratio"] = 3;
        default_spec["domain5/nest_child_id"] = 8;
        default_spec["domain5/nest_ratio"] = 3;
        bent_multi_grid(default_spec, res);
    }
    else
    {
        bent_multi_grid(spec, res);
    }
}

//---------------------------------------------------------------------------//
void
grid(const std::string &mesh_type,
     index_t npts_x, // number of per-domain points in x
     index_t npts_y, // number of per-domain points in y
     index_t npts_z, // number of per-domain points in z
     index_t ndoms_x, // number of domains in x
     index_t ndoms_y, // number of domains in y
     index_t ndoms_z, // number of domains in z
     Node &res)
{
    const bool ndoms_x_ok = ndoms_x > 0;
    const bool ndoms_y_ok = ndoms_y > 0;
    const bool ndoms_z_ok = ndoms_z > 0;

    if( ! (ndoms_x_ok && ndoms_y_ok && ndoms_z_ok) )
    {
        // error, not enough points to create the topo
        CONDUIT_ERROR("blueprint::mesh::examples::grid requires "
                      "ndoms_x > 1 and ndoms_y > 1 and ndoms_z > 1 " << std::endl <<
                      "values provided:" << std::endl <<
                      " mesh_type: " << mesh_type << std::endl <<
                      " ndoms_x: " << ndoms_x << std::endl <<
                      " ndoms_y: " << ndoms_y << std::endl <<
                      " ndoms_z: " << ndoms_z << std::endl);
    }

    // validate braid input here for nicer exception loc
    bool npts_ok = (npts_x > 1 && npts_y > 1);

    if( npts_z != 0 &&
        braid_2d_only_shape_type(mesh_type) )
    {
        npts_ok = false;
    }

    if( braid_3d_only_shape_type(mesh_type) )
    {
        // z must be valid for these cases
        if(npts_z < 2)
        {
            npts_ok = false;
        }
    }

    if(!npts_ok)
    {
        CONDUIT_ERROR("blueprint::mesh::examples::grid requires: " << std::endl <<
                      "For 2D only topologies"
                      " npts_x > 1 and npts_y > 1 and npts_z == 0"
                      << std::endl <<
                      "For 3D only topologies"
                      "npts_x > 1 and npts_y > 1 and "
                      " npts_z > 1"
                      << std::endl <<
                      "values provided:" << std::endl <<
                      " mesh_type: " << mesh_type << std::endl <<
                      " npts_x: " << npts_x << std::endl <<
                      " npts_y: " << npts_y << std::endl <<
                      " npts_z: " << npts_z << std::endl);
    }


    for(index_t dz = 0, domain_id = 0; dz < ndoms_z; dz++)
    {
        for(index_t dy = 0; dy < ndoms_y; dy++)
        {
            for(index_t dx = 0; dx < ndoms_x; dx++, domain_id++)
            {
                Node &domain_node = res["domain" + std::to_string(domain_id)];
                braid(mesh_type, npts_x, npts_y, npts_z, domain_node);
                domain_node["state/domain_id"].set(domain_id);

                Node &domain_coords_node = domain_node["coordsets/coords"];
                const std::string domain_coords_path =
                    (domain_coords_node["type"].as_string() == "uniform") ? "origin" : "values";
                const std::vector<std::string> domain_axes =
                    conduit::blueprint::mesh::utils::coordset::axes(domain_coords_node);

                for(const std::string &domain_axis : domain_axes)
                {
                    const index_t domain_axis_offset = 20.0 * (
                        (domain_axis == "x") ? dx : (
                        (domain_axis == "y") ? dy : (
                        (domain_axis == "z") ? dz : 0)));

                    float64_array domain_axis_coords =
                        domain_coords_node[domain_coords_path][domain_axis].as_float64_array();
                    for(index_t dai = 0; dai < domain_axis_coords.number_of_elements(); dai++)
                    {
                        domain_axis_coords[dai] += domain_axis_offset;
                    }
                }
            }
        }
    }

    braid_init_example_adjset(res);
}


//---------------------------------------------------------------------------//
void
bent_braid(const Node& spec,
      Node &res)
{
    if(!spec.has_child("npts_z"))
    {
        braid_bent_quads(spec, res);
    }
    else
    {
        braid_bent_hexs(spec, res);
    }
}


//---------------------------------------------------------------------------//
void
braid(const std::string &mesh_type,
      index_t npts_x, // number of points in x
      index_t npts_y, // number of points in y
      index_t npts_z, // number of points in z
      Node &res)
{
    bool npts_x_ok = true;
    bool npts_y_ok = true;
    bool npts_z_ok = true;

    if( mesh_type == "points" || mesh_type == "points_implicit" )
    {
        if( npts_x < 1 )
        {
            npts_x_ok = false;
        }
        if( npts_y < 1 )
        {
            npts_y_ok = false;
        }
        if( npts_z < 0 )
        {
            npts_z_ok = false;
        }
    }
    else
    {
        if( npts_x < 2 )
        {
            npts_x_ok = false;
        }

        if( npts_y < 2 )
        {
            npts_y_ok = false;
            if (npts_y == 0 && braid_1d_allowed_shape_type(mesh_type))
            {
                npts_y_ok = true;
            }
        }

        // check 2d cases which require npts z = 0
        if ( npts_z != 0 &&
             braid_2d_only_shape_type(mesh_type) )
        {
            npts_z_ok = false;
        }

        // check 3d cases which require z
        if( braid_3d_only_shape_type(mesh_type) )
        {
            // z must be valid for these cases
            if(npts_z < 2)
            {
                npts_z_ok = false;
            }
        }
    }


    if( ! (npts_x_ok && npts_y_ok && npts_z_ok) )
    {
        if( mesh_type == "points" || mesh_type == "points_implicit" )
        {
            // error, not enough points to create the topo
            CONDUIT_ERROR("braid with points-based topology requires"
                          "npts_x > 0,  npts_y > 0  and npts_z >= 0 "
                          "values provided:" << std::endl <<
                          " mesh_type: " << mesh_type << std::endl  <<
                          " npts_x: " << npts_x << std::endl <<
                          " npts_y: " << npts_y << std::endl <<
                          " npts_z: " << npts_z << std::endl);
        }
        else if( npts_z != 0 && braid_2d_only_shape_type(mesh_type) )
        {
            // we won't pass z on, so error if z is non zero.
            CONDUIT_ERROR("braid with 2D topology requires "
                          "npts_x > 1 and npts_y > 1 "
                          " and npts_z == 0 " << std::endl <<
                          "values provided:" << std::endl <<
                          " mesh_type: " << mesh_type << std::endl <<
                          " npts_x: " << npts_x << std::endl <<
                          " npts_y: " << npts_y << std::endl <<
                          " npts_z: " << npts_z << std::endl);
        }
        else
        {
            // error, not enough points to create the topo
            CONDUIT_ERROR("braid with non-points topology requires "
                          "npts_x > 1 for 1D, "
                          "npts_x > 1 and npts_y > 1 for 2D"
                          " and for mesh_type={\"tets\", \"hexs\", \"wedges\", \"pyramids\","
                          " \"hexs_poly\", or \"mixed\""
                          " npts_z must be > 1" << std::endl <<
                          "values provided:" << std::endl <<
                          " mesh_type: " << mesh_type << std::endl <<
                          " npts_x: " << npts_x << std::endl <<
                          " npts_y: " << npts_y << std::endl <<
                          " npts_z: " << npts_z << std::endl);
        }
    }

    if(mesh_type == "uniform")
    {
        braid_uniform(npts_x,npts_y,npts_z,res);
    }
    else if(mesh_type == "rectilinear")
    {
        braid_rectilinear(npts_x,npts_y,npts_z,res);
    }
    else if(mesh_type == "structured")
    {
        braid_structured(npts_x,npts_y,npts_z,res);
    }
    else if(mesh_type == "lines")
    {
        if( npts_z <= 1 )
            braid_lines_2d(npts_x,npts_y,res);
        else
            braid_lines_3d(npts_x,npts_y,npts_x,res);
    }
    else if(mesh_type == "tris")
    {
        braid_tris(npts_x,npts_y,res);
    }
    else if(mesh_type == "quads")
    {
        braid_quads(npts_x,npts_y,res);
    }
    else if(mesh_type == "quads_poly")
    {
        braid_quads(npts_x,npts_y,res);
        braid_to_poly(res);
    }
    else if(mesh_type == "tets")
    {
        braid_tets(npts_x,npts_y,npts_z,res);
    }
    else if(mesh_type == "hexs")
    {
        braid_hexs(npts_x,npts_y,npts_z,res);
    }
    else if(mesh_type == "hexs_poly")
    {
        braid_hexs(npts_x,npts_y,npts_z,res);
        braid_to_poly(res);
    }
    else if(mesh_type == "points")
    {
        braid_points_explicit(npts_x,npts_y,npts_z,res);
    }
    else if(mesh_type == "points_implicit")
    {
        braid_points_implicit(npts_x,npts_y,npts_z,res);
    }
    else if (mesh_type == "mixed")
    {
        braid_mixed(npts_x, npts_y, npts_z, res);
    }
    else if (mesh_type == "mixed_2d")
    {
        braid_mixed_2d(npts_x, npts_y, res);
    }
    else if (mesh_type == "wedges")
    {
        Node braid_regular;
        braid_hexs(npts_x,npts_y,npts_z,braid_regular);
        braid_to_wedges(braid_regular, res);
    }
    else if (mesh_type == "pyramids")
    {
        Node braid_regular;
        braid_hexs(npts_x,npts_y,npts_z,braid_regular);
        braid_to_pyramids(npts_x,npts_y,npts_z,braid_regular, res);
    }
    else
    {
        CONDUIT_ERROR("unknown mesh_type = " << mesh_type);
    }
}


//---------------------------------------------------------------------------//
void spiral(index_t ndoms,
            Node &res)
{
    res.reset();

    int f_1 = 1;
    int f = 1;

    float64 x = 0.0;
    float64 y = 0.0;

    float64 loc_xo = x + f;
    float64 loc_yo = y + f;

    int rot_case = 0;

    for(int d=0; d < ndoms; d++)
    {
        // create the current domain
        std::ostringstream oss;
        oss << "domain_" << std::setw(6) << std::setfill('0') << d;
        std::string domain_name = oss.str();

        Node &dom = res[domain_name];
        // set cycle and domain id
        dom["state/cycle"] = 0;
        dom["state/domain_id"] = d;

        // create a rectilinear coordset
        dom["coordsets/coords/type"] = "rectilinear";
        dom["coordsets/coords/values/x"] = DataType::float64(f+1);
        dom["coordsets/coords/values/y"] = DataType::float64(f+1);

        float64_array x_coords = dom["coordsets/coords/values/x"].value();
        float64_array y_coords = dom["coordsets/coords/values/y"].value();

        float64 xv = x;
        float64 yv = y;

        for(int i=0; i < f+1; i++)
        {
            x_coords[i] = xv;
            y_coords[i] = yv;
            xv+=1;
            yv+=1;
        }

        // create the topology
        dom["topologies/topo/type"] = "rectilinear";
        dom["topologies/topo/coordset"] = "coords";
        // todo, add topo logical origin


        // create the fields
        dom["fields/dist/association"] = "vertex";
        dom["fields/dist/topology"] = "topo";
        dom["fields/dist/values"] = DataType::float64((f+1) * (f+1));

        float64_array dist_vals = dom["fields/dist/values"].value();

        index_t idx = 0;
        // fill the scalar with approx dist to spiral
        yv = y;

        for(int j=0; j < f+1; j++)
        {
            xv = x;
            for(int i=0; i < f+1; i++)
            {
                float64 l_x = xv - loc_xo;
                float64 l_y = yv - loc_yo;
                dist_vals[idx] = sqrt( l_x * l_x + l_y * l_y) - f;
                xv+=1;
                idx++;
            }
            yv+=1;
        }

        // setup for next domain using one of 4 rotation cases
        switch(rot_case)
        {
            case 0:
            {
                x += f;
                // next loc orig == top left
                loc_xo = x;
                if (f <= 1)
                    loc_yo = y + f;
                else
                    loc_yo = y + f + f_1;
                break;
            }
            case 1:
            {
                y += f;
                x -= f_1;
                // next loc orig == bottom left
                loc_xo = x;
                loc_yo = y;
                break;
            }
            case 2:
            {
                x -= (f + f_1);
                y -= f_1;
                // next loc orig == bottom right
                loc_xo = x + (f + f_1);
                loc_yo = y;
                break;
            }
            case 3:
            {
                y -= (f + f_1);
                // next loc orig == top right
                loc_xo = x + (f + f_1);
                loc_yo = y + (f + f_1);
                break;
            }
        }
        // update the rotate case
        rot_case =  (rot_case +1) % 4;

        // calc next fib #
        // domain id is one less than the fib #
        if( (d+1) > 1)
        {
            int f_prev = f;
            f = f + f_1;
            f_1 = f_prev;
        }
    }
}

//---------------------------------------------------------------------------//
point
polytess_calc_polygon_center(const std::vector<index_t> polygon,
                             std::map< point, index_t > &/*point_map*/,
                             std::map< index_t, point > &point_rmap)
{
    point polygon_center(0.0, 0.0);

    for(index_t pi = 0; pi < (index_t)polygon.size(); pi++)
    {
        const point &polygon_point = point_rmap[polygon[pi]];
        polygon_center.x += polygon_point.x;
        polygon_center.y += polygon_point.y;
    }

    polygon_center.x /= (index_t)polygon.size();
    polygon_center.y /= (index_t)polygon.size();

    return polygon_center;
}


//---------------------------------------------------------------------------//
point
polytess_displace_point(const point &start_point,
                        index_t displace_dir,
                        float64 displace_mag)
{
    const bool is_dir_x = displace_dir % 2 == 0;
    const bool is_dir_pos = displace_dir > 1;
    return point(
        start_point.x + (is_dir_pos ? 1 : -1) * (is_dir_x ? 1.0 : 0.0) * displace_mag,
        start_point.y + (is_dir_pos ? 1 : -1) * (is_dir_x ? 0.0 : 1.0) * displace_mag);
}


//---------------------------------------------------------------------------//
std::vector<point>
polytess_make_polygon(point poly_center,
                      float64 side_length,
                      index_t ncorners)
{
    const float64 poly_radius = side_length / (2.0 * sin(PI_VALUE / ncorners));

    std::vector<point> poly_points;
    for(index_t c = 0; c < ncorners; c++)
    {
        point cpoint = poly_center;
        float64 cangle = PI_VALUE + (c + 0.5) * (2.0 * PI_VALUE / ncorners);
        cpoint.x += poly_radius * cos(cangle);
        cpoint.y += poly_radius * sin(cangle);
        poly_points.push_back(cpoint);
    }

    return poly_points;
}


//---------------------------------------------------------------------------//
bool
polytess_add_polygon(const std::vector<point> &polygon_points,
                     const index_t polygon_level,
                     std::map< point, index_t > &point_map,
                     std::map< index_t, point > &point_rmap,
                     std::vector< std::vector<index_t> > &polygons,
                     std::vector< index_t > &levels)
{
    std::vector<index_t> polygon_indices(polygon_points.size());

    bool is_polygon_duplicate = true;
    for(index_t pi = 0; pi < (index_t)polygon_points.size(); pi++)
    {
        const point &polygon_point = polygon_points[pi];
        index_t &point_index = polygon_indices[pi];

        if(point_map.find(polygon_point) != point_map.end())
        {
            point_index = point_map.find(polygon_point)->second;
        }
        else
        {
            point_index = point_map.size();
            point_map[polygon_point] = point_index;
            point_rmap[point_index] = polygon_point;
            is_polygon_duplicate = false;
        }
    }

    if(!is_polygon_duplicate)
    {
        polygons.push_back(polygon_indices);
        levels.push_back(polygon_level);
    }

    return !is_polygon_duplicate;
}


//---------------------------------------------------------------------------//
void polytess_recursive(index_t nlevels,
                        std::map< point, index_t > &point_map,
                        std::map< index_t, point > &point_rmap,
                        std::vector< std::vector<index_t> > &polygons,
                        std::vector< index_t > &levels)
{
    const float64 side_length = 1.0;
    const float64 octogon_to_center = side_length / (2.0 * tan(PI_VALUE / 8.0));
    const float64 adj_poly_distance = octogon_to_center + (side_length / 2.0);

    // base case
    if(nlevels <= 1)
    {
        std::vector<point> center_polygon_points = polytess_make_polygon(
            point(0.0, 0.0), side_length, 8);
        polytess_add_polygon(center_polygon_points, nlevels,
            point_map, point_rmap, polygons, levels);
    }
    // recursive case
    else // if(nlevels > 1)
    {
        polytess_recursive(nlevels - 1, point_map, point_rmap, polygons, levels);

        for(index_t o = polygons.size() - 1; o >= 0 && levels[o] == nlevels - 1; o--)
        {
            if(polygons[o].size() != 8) { continue; }

            const std::vector<index_t> &octogon = polygons[o];
            const point octogon_center = polytess_calc_polygon_center(octogon, point_map, point_rmap);
            for(index_t d = 0; d < 4; d++)
            {
                const point dir_square_center = polytess_displace_point(
                    octogon_center, d, adj_poly_distance);

                std::vector<point> dir_square_points = polytess_make_polygon(
                    dir_square_center, side_length, 4);

                if(polytess_add_polygon(dir_square_points, nlevels,
                    point_map, point_rmap, polygons, levels))
                {
                    const point square_octogon_center = polytess_displace_point(
                        dir_square_center, (d + 1) % 4, adj_poly_distance);

                    std::vector<point> square_octogon_points = polytess_make_polygon(
                        square_octogon_center, side_length, 8);

                    polytess_add_polygon(square_octogon_points, nlevels,
                        point_map, point_rmap, polygons, levels);
                }
            }
        }
    }
}


//---------------------------------------------------------------------------//
void polytess(index_t nlevels,
              index_t nz,
              Node &res)
{
    if (nz == 1)
    {
        std::map< point, index_t > point_map;
        std::map< index_t, point > point_rmap;
        std::vector< std::vector<index_t> > polygons;
        std::vector< index_t > levels;

        polytess_recursive(nlevels, point_map, point_rmap, polygons, levels);

        index_t conn_size = 0;
        for(index_t p = 0; p < (index_t)polygons.size(); p++)
        {
            conn_size += polygons[p].size();
        }

        // Populate Coordinates //

        Node &coordset = res["coordsets/coords"];
        coordset["type"].set("explicit");
        coordset["values/x"].set(DataType::float64(point_map.size()));
        coordset["values/y"].set(DataType::float64(point_map.size()));

        float64_array x_coords = coordset["values/x"].value();
        float64_array y_coords = coordset["values/y"].value();
        for(index_t pi = 0; pi < (index_t)point_map.size(); pi++)
        {
            const point &p = point_rmap[pi];
            x_coords[pi] = p.x;
            y_coords[pi] = p.y;
        }

        // Populate Topology //

        Node &topology = res["topologies/topo"];
        topology["coordset"].set("coords");
        topology["type"].set("unstructured");
        topology["elements/shape"].set("polygonal");
        topology["elements/connectivity"].set(DataType::uint64(conn_size));
        topology["elements/sizes"].set(DataType::uint64(polygons.size()));

        uint64_array conn_array = topology["elements/connectivity"].value();
        uint64_array size_array = topology["elements/sizes"].value();
        for(index_t pi = 0, ci = 0; pi < (index_t)polygons.size(); pi++)
        {
            const std::vector<index_t> &p = polygons[pi];

            size_array[pi] = p.size();
            for(index_t ii = 0; ii < (index_t)p.size(); ii++)
            {
                conn_array[ci++] = p[ii];
            }
        }

        blueprint::mesh::topology::unstructured::generate_offsets_inline(topology);

        // Populate Field //

        Node &field =  res["fields/level"];
        field["topology"].set("topo");
        field["association"].set("element");
        field["volume_dependent"].set("false");
        // TODO: should we try to use index_t as the data type here?
        field["values"].set(DataType::uint32(polygons.size()));

        uint32_array level_array = field["values"].value();
        for(index_t pi = 0; pi < (index_t)polygons.size(); pi++)
        {
            level_array[pi] = (uint32) levels[pi];
        }
    }
    else if (nz > 1)
    {
        // Our goal here is to take the original polytess and extend it
        // into 3 dimensions. The way we will accomplish this is by
        // placing the original polytess into the z = 0 plane, placing a
        // copy of it into the z = 1 plane, another into the z = 2 plane,
        // etc., and constructing "walls" between the top and bottom edges
        // of each polytessalation. Then we will specify polyhedra that use
        // all the faces at our disposal.
        Node poly;
        polytess(nlevels, 1, poly);

        Node &res_coords = res["coordsets/coords"];
        Node &res_topo = res["topologies/topo"];
        Node &res_fields = res["fields"];

        // SET UP COORDINATES

        res_coords["type"] = poly["coordsets/coords/type"];

        int num_orig_points = poly["coordsets/coords/values/x"].dtype().number_of_elements();
        int num_points = nz * num_orig_points;

        res_coords["values/x"].set(conduit::DataType::float64(num_points));
        res_coords["values/y"].set(conduit::DataType::float64(num_points));
        res_coords["values/z"].set(conduit::DataType::float64(num_points));

        float64 *poly_x_vals = poly["coordsets/coords/values/x"].value();
        float64 *poly_y_vals = poly["coordsets/coords/values/y"].value();

        float64 *x_vals = res_coords["values/x"].value();
        float64 *y_vals = res_coords["values/y"].value();
        float64 *z_vals = res_coords["values/z"].value();

        // all the original points are added nz times, the first time with a z-value of 0,
        // and the second time with a z-value of 1, etc.
        for (int i = 0; i < num_points; i ++)
        {
            int i_mod_num_orig_points = i % num_orig_points;
            x_vals[i] = poly_x_vals[i_mod_num_orig_points];
            y_vals[i] = poly_y_vals[i_mod_num_orig_points];
            z_vals[i] = i / num_orig_points;
        }

        res_topo["type"] = poly["topologies/topo/type"];
        res_topo["coordset"] = poly["topologies/topo/coordset"];

        // SUBELEMENTS

            // In the nz = 2 case,
            // if we take our polytess and reflect it, we have two polytessalations on top of one another,
            // with a distance of 1 unit in between. If we go through each polygon, and for each one,
            // select every pair of adjacent points and add a new polygon that uses the points from that pair
            // as well as the points directly above in the reflected polytess, then we will have duplicate
            // polygons, simply because the polygons share vertices with one another, so multiple polygons
            // will have the same "walls". This section accounts for that. The nz > 2 cases are similar,
            // except this situation is repeated for each level of the stacked polytess layers.

            res_topo["subelements/shape"] = poly["topologies/topo/elements/shape"];

            // CALCULATE THE NUMBER OF NEW POLYGONS
            int num_duplicate_polygons = 0;
            for (int n = 1; n < nlevels; n ++)
            {
                // this formula is the sum of two other formulas:
                // 1) the number of outward edges for an (n - 1) polytess, and
                // 2) the number of edges that squares in the polytess share with
                //    neighboring octagons in their level.
                // This sum is again summed for each level, which will produce the
                // number of duplicates we would have gotten had we simply made a
                // polygon for each pair of adjacent points in each polygon,
                // as described above.
                num_duplicate_polygons += 8 * (3 * n - 1);
            }
            int num_new_polygons = -1 * num_duplicate_polygons;
            int sizeof_poly_sizes = poly["topologies/topo/elements/sizes"].dtype().number_of_elements();
            uint64 *poly_sizes = poly["topologies/topo/elements/sizes"].value();
            for (int i = 0; i < sizeof_poly_sizes; i ++)
            {
                num_new_polygons += poly_sizes[i];
            }
            // we need this number of polygons for each level we add
            num_new_polygons *= nz - 1;

            // SET UP SIZES
            const int points_per_quad = 4;
            // the sizes must have space for the original sizes array, nz copies of it extending upwards,
            // and all the walls; hence the addition of the num_new_polygons.
            int length_of_new_sizes = sizeof_poly_sizes * nz + num_new_polygons;
            res_topo["subelements/sizes"].set(conduit::DataType::uint64(length_of_new_sizes));
            uint64 *sizes = res_topo["subelements/sizes"].value();
            for (int i = 0; i < length_of_new_sizes; i ++)
            {
                // the original and reflected polytess sizes
                if (i < sizeof_poly_sizes * nz)
                {
                    sizes[i] = poly_sizes[i % sizeof_poly_sizes];
                }
                // all the new polygons are quads
                else
                {
                    sizes[i] = points_per_quad;
                }
            }

            // SET UP OFFSETS
            res_topo["subelements/offsets"].set(conduit::DataType::uint64(length_of_new_sizes));
            uint64 *offsets = res_topo["subelements/offsets"].value();
            offsets[0] = 0;
            for (int i = 1; i < length_of_new_sizes; i ++)
            {
                offsets[i] = sizes[i - 1] + offsets[i - 1];
            }

            // SET UP CONNECTIVITY
            const int sizeof_poly_connec = poly["topologies/topo/elements/connectivity"].dtype().number_of_elements();
            const int sizeof_sub_connec = sizeof_poly_connec * nz + num_new_polygons * points_per_quad;
            res_topo["subelements/connectivity"].set(conduit::DataType::uint64(sizeof_sub_connec));
            uint64 *connec = res_topo["subelements/connectivity"].value();
            uint64 *poly_connec = poly["topologies/topo/elements/connectivity"].value();

            // first, copy the original connectivity, then the reflected connectivities, which luckily
            // is as simple as adding an offset to the original.
            for (int i = 0; i < sizeof_poly_connec * nz; i ++)
            {
                connec[i] = poly_connec[i % sizeof_poly_connec] + (i / sizeof_poly_connec) * num_orig_points;
            }

            // now the tricky part, where we want to add new faces for the quads that make
            // up the walls, and, most importantly, keep track of them.
            // To do this, we use a map. Put simply, it maps quad faces to polyhedra that
            // will use them.

            // map a quad (a set of 4 ints) to a pair,
            // where car is the index of the quad in connec (an int)
            // and cdr is the list of associated polyhedra (a vector of integers)
            std::map<std::set<int>, std::pair<int, std::vector<int>>> quad_map;

            int k = 0;
            for (int i = 0; i < sizeof_poly_sizes * (nz - 1); i ++)
            {
                for (uint64 j = 0; j < sizes[i]; j ++)
                {
                    int curr = connec[offsets[i] + j];
                    int next = connec[(j + 1) % sizes[i] + offsets[i]];

                    // adding num_orig_points will give us the points directly above (one level up) in our mesh
                    std::set<int> quad = {curr, next, next + num_orig_points, curr + num_orig_points};
                    std::vector<int> associated_polyhedra{i};
                    int currpos = sizeof_poly_sizes * nz + k / points_per_quad;

                    if (quad_map.insert(std::make_pair(quad, std::make_pair(currpos, associated_polyhedra))).second)
                    {
                        connec[sizeof_poly_connec * nz + k] = curr;
                        k ++;
                        connec[sizeof_poly_connec * nz + k] = next;
                        k ++;
                        connec[sizeof_poly_connec * nz + k] = next + num_orig_points;
                        k ++;
                        connec[sizeof_poly_connec * nz + k] = curr + num_orig_points;
                        k ++;
                    }
                    else
                    {
                        quad_map[quad].second.push_back(i);
                    }
                }
            }

        // ELEMENTS

            res_topo["elements/shape"] = "polyhedral";

            const int num_polyhedra = sizeof_poly_sizes * (nz - 1);

            // SET UP SIZES
            const int points_per_octagon = 8;
            const int faces_per_octaprism = 10;
            const int faces_per_hex = 6;
            res_topo["elements/sizes"].set(conduit::DataType::uint64(num_polyhedra));
            uint64 *elements_sizes = res_topo["elements/sizes"].value();
            for (int i = 0; i < num_polyhedra; i ++)
            {
                // this ensures that each original octagon is associated with an octagonal prism,
                // and each original square gets a cube.
                elements_sizes[i] = (poly_sizes[i % sizeof_poly_sizes] == points_per_octagon) ? faces_per_octaprism : faces_per_hex;
            }

            // SET UP OFFSETS
            res_topo["elements/offsets"].set(conduit::DataType::uint64(num_polyhedra));
            uint64 *elements_offsets = res_topo["elements/offsets"].value();
            elements_offsets[0] = 0;
            for (int i = 1; i < num_polyhedra; i ++)
            {
                elements_offsets[i] = elements_sizes[i - 1] + elements_offsets[i - 1];
            }

            // SET UP CONNECTIVITY
            std::map<int, std::vector<int>> polyhedra_to_quads;
            std::map<std::set<int>, std::pair<int, std::vector<int>>>::iterator itr;
            // the one-to-many map we set up before must be reversed. Before, we mapped
            // quads to polyhedra that used them, now, we wish to map polyhedra to
            // quads they use.
            for (itr = quad_map.begin(); itr != quad_map.end(); itr ++)
            {
                int pos = itr->second.first;
                std::vector<int> curr = itr->second.second;
                for (uint64 i = 0; i < curr.size(); i ++)
                {
                    std::vector<int> quads{pos};
                    if (!polyhedra_to_quads.insert(std::make_pair(curr[i], quads)).second)
                    {
                        polyhedra_to_quads[curr[i]].push_back(pos);
                    }
                }
            }

            int sizeof_elements_connec = 0;
            for (int i = 0; i < num_polyhedra; i ++)
            {
                sizeof_elements_connec += elements_sizes[i];
            }

            res_topo["elements/connectivity"].set(conduit::DataType::uint64(sizeof_elements_connec));
            uint64 *elements_connec = res_topo["elements/connectivity"].value();
            int l = 0;
            for (int i = 0; i < num_polyhedra; i ++)
            {
                // to the polyhedral connectivity array, for each polyhedron, we first add
                // the polygon from the original polytess, then the polygon directly above,
                // then each of the vertical faces, thanks to the maps we set up earlier
                elements_connec[l] = i;
                l ++;
                elements_connec[l] = i + sizeof_poly_sizes;
                l ++;
                std::vector<int> quads = polyhedra_to_quads[i];
                for (uint64 j = 0; j < quads.size(); j ++)
                {
                    elements_connec[l] = quads[j];
                    l ++;
                }
            }

        // SET UP FIELDS
        res_fields["level/topology"] = poly["fields/level/topology"];
        res_fields["level/association"] = poly["fields/level/association"];
        res_fields["level/volume_dependent"] = poly["fields/level/volume_dependent"];

        const int sizeof_poly_field_values = poly["fields/level/values"].dtype().number_of_elements();
        res_fields["level/values"].set(conduit::DataType::uint32(num_polyhedra));

        uint32 *values = res_fields["level/values"].value();
        uint32 *poly_values = poly["fields/level/values"].value();

        // because for each original polygon we have made nz new polyhedra,
        // setting up the field is a simple matter of just copying over our
        // original field data a few times.
        for (int i = 0; i < num_polyhedra; i ++)
        {
            values[i] = poly_values[i % sizeof_poly_field_values];
        }
    }
    else
    {
        CONDUIT_ERROR("polytess: nz must be an integer greater than 0.")
    }
}


//-----------------------------------------------------------------------------
void
polychain(const index_t length, // how long the chain ought to be
          Node &res)
{
    res.reset();

    Node &chain_coords = res["coordsets/coords"];
    Node &chain_topo = res["topologies/topo"];
    Node &chain_fields = res["fields"];

    chain_coords["type"] = "explicit";

    const index_t num_verts_per_hex = 8;
    const index_t num_verts_per_triprism = 6;
    const index_t num_verts_per_chain_pair = num_verts_per_hex + 2 * num_verts_per_triprism;

    chain_coords["values/x"].set(conduit::DataType::int64(length * num_verts_per_chain_pair));
    chain_coords["values/y"].set(conduit::DataType::int64(length * num_verts_per_chain_pair));
    chain_coords["values/z"].set(conduit::DataType::int64(length * num_verts_per_chain_pair));

    int64 *values_x = chain_coords["values/x"].value();
    int64 *values_y = chain_coords["values/y"].value();
    int64 *values_z = chain_coords["values/z"].value();

    for (int i = 0; i < length; i ++)
    {
        // points for the cubes
        values_x[i * num_verts_per_chain_pair] = 1 + i * 2;           values_y[i * num_verts_per_chain_pair] = 1;       values_z[i * num_verts_per_chain_pair] = 1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 1] = 1 + i * 2;       values_y[i * num_verts_per_chain_pair + 1] = 1;   values_z[i * num_verts_per_chain_pair + 1] = -1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 2] = -1 + i * 2;      values_y[i * num_verts_per_chain_pair + 2] = 1;   values_z[i * num_verts_per_chain_pair + 2] = -1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 3] = -1 + i * 2;      values_y[i * num_verts_per_chain_pair + 3] = 1;   values_z[i * num_verts_per_chain_pair + 3] = 1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 4] = 1 + i * 2;       values_y[i * num_verts_per_chain_pair + 4] = -1;  values_z[i * num_verts_per_chain_pair + 4] = 1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 5] = 1 + i * 2;       values_y[i * num_verts_per_chain_pair + 5] = -1;  values_z[i * num_verts_per_chain_pair + 5] = -1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 6] = -1 + i * 2;      values_y[i * num_verts_per_chain_pair + 6] = -1;  values_z[i * num_verts_per_chain_pair + 6] = -1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 7] = -1 + i * 2;      values_y[i * num_verts_per_chain_pair + 7] = -1;  values_z[i * num_verts_per_chain_pair + 7] = 1 + i * 2;

        // points for half the triangular prisms
        values_x[i * num_verts_per_chain_pair + 8] = 1 + i * 2;       values_y[i * num_verts_per_chain_pair + 8] = 1;   values_z[i * num_verts_per_chain_pair + 8] = -1 + i * 2 + 2;
        values_x[i * num_verts_per_chain_pair + 9] = -1 + i * 2;      values_y[i * num_verts_per_chain_pair + 9] = 1;   values_z[i * num_verts_per_chain_pair + 9] = -1 + i * 2 + 2;
        values_x[i * num_verts_per_chain_pair + 10] = -1 + i * 2 + 2; values_y[i * num_verts_per_chain_pair + 10] = 1;  values_z[i * num_verts_per_chain_pair + 10] = 1 + i * 2 + 2;
        values_x[i * num_verts_per_chain_pair + 11] = 1 + i * 2;      values_y[i * num_verts_per_chain_pair + 11] = -1; values_z[i * num_verts_per_chain_pair + 11] = -1 + i * 2 + 2;
        values_x[i * num_verts_per_chain_pair + 12] = -1 + i * 2;     values_y[i * num_verts_per_chain_pair + 12] = -1; values_z[i * num_verts_per_chain_pair + 12] = -1 + i * 2 + 2;
        values_x[i * num_verts_per_chain_pair + 13] = -1 + i * 2 + 2; values_y[i * num_verts_per_chain_pair + 13] = -1; values_z[i * num_verts_per_chain_pair + 13] = 1 + i * 2 + 2;

        // points for the other half
        values_x[i * num_verts_per_chain_pair + 14] = 1 + i * 2;      values_y[i * num_verts_per_chain_pair + 14] = 1;  values_z[i * num_verts_per_chain_pair + 14] = -1 + i * 2 + 2;
        values_x[i * num_verts_per_chain_pair + 15] = -1 + i * 2 + 2; values_y[i * num_verts_per_chain_pair + 15] = 1;  values_z[i * num_verts_per_chain_pair + 15] = -1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 16] = -1 + i * 2 + 4; values_y[i * num_verts_per_chain_pair + 16] = 1;  values_z[i * num_verts_per_chain_pair + 16] = 1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 17] = 1 + i * 2;      values_y[i * num_verts_per_chain_pair + 17] = -1; values_z[i * num_verts_per_chain_pair + 17] = -1 + i * 2 + 2;
        values_x[i * num_verts_per_chain_pair + 18] = -1 + i * 2 + 2; values_y[i * num_verts_per_chain_pair + 18] = -1; values_z[i * num_verts_per_chain_pair + 18] = -1 + i * 2;
        values_x[i * num_verts_per_chain_pair + 19] = -1 + i * 2 + 4; values_y[i * num_verts_per_chain_pair + 19] = -1; values_z[i * num_verts_per_chain_pair + 19] = 1 + i * 2;
    }

    chain_topo["type"] = "unstructured";
    chain_topo["coordset"] = "coords";
    chain_topo["elements/shape"] = "polyhedral";

    const index_t num_faces_per_hex = 6;
    const index_t num_faces_per_triprism = 5;
    const index_t num_faces_per_chain_pair = num_faces_per_hex + 2 * num_faces_per_triprism;

    chain_topo["elements/connectivity"].set(conduit::DataType::int64(length * num_faces_per_chain_pair));
    int64 *connec = chain_topo["elements/connectivity"].value();
    for (int i = 0; i < length * num_faces_per_chain_pair; i ++)
    {
        // our faces are specified in order and no faces are reused
        connec[i] = i;
    }

    // this is 3 because every time length is increased by 1, 3 more polyhedra are added,
    const index_t num_polyhedra_per_chain_pair = 3;

    // the cube and two prisms
    chain_topo["elements/sizes"].set(conduit::DataType::int64(length * num_polyhedra_per_chain_pair));
    int64 *sizes = chain_topo["elements/sizes"].value();
    for (int i = 0; i < length * num_polyhedra_per_chain_pair; i ++)
    {
        // this ensures that sizes will be of the form {6,5,5, 6,5,5, 6,5,5, ..., 6,5,5}
        sizes[i] = ((i % num_polyhedra_per_chain_pair) > 0) ? num_faces_per_triprism : num_faces_per_hex;
    }

    chain_topo["subelements/shape"] = "polygonal";

    const index_t num_points_per_quad_face = 4;
    const index_t num_points_per_tri_face = 3;
    const index_t num_tri_faces_in_triprism = 2;
    const index_t num_quad_faces_in_triprism = 3;
    const index_t sizeof_hex_connectivity = num_faces_per_hex * num_points_per_quad_face;
    const index_t sizeof_triprism_connectivity = num_tri_faces_in_triprism * num_points_per_tri_face +
                                                 num_quad_faces_in_triprism * num_points_per_quad_face;
    const index_t sizeof_chainpair_connec = sizeof_hex_connectivity + sizeof_triprism_connectivity * 2;

    chain_topo["subelements/connectivity"].set(conduit::DataType::int64(length * sizeof_chainpair_connec));
    int64 *sub_connec = chain_topo["subelements/connectivity"].value();
    for (int i = 0; i < length; i ++)
    {
        // CUBE
        // top                                                          // bottom
        sub_connec[i * sizeof_chainpair_connec] = 0 + i * 20;        sub_connec[i * sizeof_chainpair_connec + 4] = 4 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 1] = 1 + i * 20;    sub_connec[i * sizeof_chainpair_connec + 5] = 5 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 2] = 2 + i * 20;    sub_connec[i * sizeof_chainpair_connec + 6] = 6 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 3] = 3 + i * 20;    sub_connec[i * sizeof_chainpair_connec + 7] = 7 + i * 20;

        // side where x = 1                                             // side where x = -1
        sub_connec[i * sizeof_chainpair_connec + 8] = 0 + i * 20;    sub_connec[i * sizeof_chainpair_connec + 12] = 2 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 9] = 1 + i * 20;    sub_connec[i * sizeof_chainpair_connec + 13] = 3 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 10] = 5 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 14] = 7 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 11] = 4 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 15] = 6 + i * 20;

        // side where z = 1                                             // side where z = -1
        sub_connec[i * sizeof_chainpair_connec + 16] = 0 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 20] = 1 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 17] = 3 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 21] = 2 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 18] = 7 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 22] = 6 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 19] = 4 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 23] = 5 + i * 20;

        // PRISM 1
        sub_connec[i * sizeof_chainpair_connec + 24] = 9 + i * 20;  sub_connec[i * sizeof_chainpair_connec + 28] = 8 + i * 20;  sub_connec[i * sizeof_chainpair_connec + 32] = 8 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 25] = 10 + i * 20; sub_connec[i * sizeof_chainpair_connec + 29] = 9 + i * 20;  sub_connec[i * sizeof_chainpair_connec + 33] = 10 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 26] = 13 + i * 20; sub_connec[i * sizeof_chainpair_connec + 30] = 12 + i * 20; sub_connec[i * sizeof_chainpair_connec + 34] = 13 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 27] = 12 + i * 20; sub_connec[i * sizeof_chainpair_connec + 31] = 11 + i * 20; sub_connec[i * sizeof_chainpair_connec + 35] = 11 + i * 20;

        sub_connec[i * sizeof_chainpair_connec + 36] = 8 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 39] = 11 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 37] = 9 + i * 20;   sub_connec[i * sizeof_chainpair_connec + 40] = 12 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 38] = 10 + i * 20;  sub_connec[i * sizeof_chainpair_connec + 41] = 13 + i * 20;

        // PRISM 2
        sub_connec[i * sizeof_chainpair_connec + 42] = 15 + i * 20; sub_connec[i * sizeof_chainpair_connec + 46] = 14 + i * 20; sub_connec[i * sizeof_chainpair_connec + 50] = 14 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 43] = 16 + i * 20; sub_connec[i * sizeof_chainpair_connec + 47] = 15 + i * 20; sub_connec[i * sizeof_chainpair_connec + 51] = 16 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 44] = 19 + i * 20; sub_connec[i * sizeof_chainpair_connec + 48] = 18 + i * 20; sub_connec[i * sizeof_chainpair_connec + 52] = 19 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 45] = 18 + i * 20; sub_connec[i * sizeof_chainpair_connec + 49] = 17 + i * 20; sub_connec[i * sizeof_chainpair_connec + 53] = 17 + i * 20;

        sub_connec[i * sizeof_chainpair_connec + 54] = 14 + i * 20; sub_connec[i * sizeof_chainpair_connec + 57] = 17 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 55] = 15 + i * 20; sub_connec[i * sizeof_chainpair_connec + 58] = 18 + i * 20;
        sub_connec[i * sizeof_chainpair_connec + 56] = 16 + i * 20; sub_connec[i * sizeof_chainpair_connec + 59] = 19 + i * 20;
    }

    chain_topo["subelements/sizes"].set(conduit::DataType::int64(length * num_faces_per_chain_pair));

    int64 *sub_sizes = chain_topo["subelements/sizes"].value();

    for (int i = 0; i < length * num_faces_per_chain_pair; i ++)
    {
        // this ensures sizes will be of the form {4,4,4,4,4,4,4,4,4,3,3,4,4,4,3,3, 4,4,4,4,4,4,4,4,4,3,3,4,4,4,3,3, ...}
        int imodfaces = i % num_faces_per_chain_pair;
        sub_sizes[i] = ((imodfaces < 9) || ((imodfaces > 10) && (imodfaces < 14))) ? num_points_per_quad_face : num_points_per_tri_face;
    }

    blueprint::mesh::topology::unstructured::generate_offsets_inline(chain_topo);

    chain_fields["chain/topology"] = "topo";
    chain_fields["chain/association"] = "element";
    chain_fields["chain/volume_dependent"] = "false";
    chain_fields["chain/values"].set(conduit::DataType::int64(length * num_polyhedra_per_chain_pair));
    int64 *field_values = chain_fields["chain/values"].value();

    for (int i = 0; i < length * num_polyhedra_per_chain_pair; i ++)
    {
        // ensures that the field is of the form {0,1,1, 0,1,1, ..., 0,1,1}
        field_values[i] = (i % num_polyhedra_per_chain_pair) == 0 ? 0 : 1;
    }
}


//---------------------------------------------------------------------------//
void
misc(const std::string &mesh_type,
     index_t npts_x, // number of points in x
     index_t npts_y, // number of points in y
     index_t /*npts_z*/, // number of points in z
     Node &res)
{
    // TODO(JRC): Improve these examples so that they use different example
    // geometry than is used in the "braid" examples.
    if(mesh_type == "matsets")
    {
        braid_quads(npts_x,npts_y,res);
        braid_init_example_matset(npts_x-1,npts_y-1,0,res["matsets/mesh"]);
    }
    else if(mesh_type == "specsets")
    {
        braid_quads(npts_x,npts_y,res);
        braid_init_example_matset(npts_x-1,npts_y-1,0,res["matsets/mesh"]);
        braid_init_example_specset(npts_x-1,npts_y-1,0,res["specsets/mesh"]);
    }
    else if(mesh_type == "nestsets")
    {
        braid_rectilinear(npts_x,npts_y,1,res["domain0"]);
        res["domain0/state/domain_id"].set(0);
        res["domain0/state/level_id"].set(0);

        for(index_t j = 0; j < 2; j++)
        {
            for(index_t i = 0; i < 2; i++)
            {
                const index_t domain_id = j * 2 + i + 1;

                std::ostringstream oss;
                oss << "domain" << domain_id;
                const std::string domain_name = oss.str();

                Node &domain_node = res[domain_name];
                braid_rectilinear(npts_x,npts_y,1,domain_node);
                domain_node["state/domain_id"].set(domain_id);
                domain_node["state/level_id"].set(1);

                Node &domain_coords = domain_node["coordsets/coords/values"];
                float64_array domain_coords_x = domain_coords["x"].as_float64_array();
                for(index_t x = 0; x < domain_coords_x.number_of_elements(); x++)
                {
                    domain_coords_x[x] = ( domain_coords_x[x] / 2.0 ) - 5.0 + i * 10.0;
                }
                float64_array domain_coords_y = domain_coords["y"].as_float64_array();
                for(index_t y = 0; y < domain_coords_y.number_of_elements(); y++)
                {
                    domain_coords_y[y] = ( domain_coords_y[y] / 2.0 ) - 5.0 + j * 10.0;
                }
            }
        }

        braid_init_example_nestset(res);
    }
    else
    {
        CONDUIT_ERROR("unknown mesh_type = " << mesh_type);
    }
}

//-----------------------------------------------------------------------------
void
adjset_uniform(Node &res)
{
    for(int32 i = 0; i < 8; i++)
    {
        std::ostringstream oss;
        oss << "domain_" << std::setfill('0') << std::setw(6) << i;
        const std::string domain_name = oss.str();

        Node &domain_node = res[domain_name];

        domain_node["state/domain_id"].set(i);
        Node &domain_coords = domain_node["coordsets/coords"];
        domain_coords["type"].set_string("uniform");
        domain_coords["dims/i"].set_int32(21);
        domain_coords["dims/j"].set_int32(21);
        domain_coords["spacing/dx"].set_float64(0.0125);
        domain_coords["spacing/dy"].set_float64(0.025);
        domain_coords["origin/x"].set_float64(0.25*(i/2));
        domain_coords["origin/y"].set_float64(0.5*(i%2));

        Node &domain_topo = domain_node["topologies/topo"];
        domain_topo["elements/origin/i"].set_int32(20*(i/2));
        domain_topo["elements/origin/j"].set_int32(20*(i%2));
        domain_topo["type"].set_string("uniform");
        domain_topo["coordset"].set_string("coords");

        // add a simple field that has the domain id
        Node &domain_field = domain_node["fields/id"];
        domain_field["association"] = "element";
        domain_field["topology"] = "topo";
        domain_field["values"] = DataType::int32(20 * 20);

        int32_array vals =  domain_field["values"].value();
        for(int j=0;j<20*20;j++)
        {
            vals[j] = i;
        }

        Node &domain_adjsets = domain_node["adjsets/adjset"];
        domain_adjsets["association"].set_string("vertex");
        domain_adjsets["topology"].set_string("topo");
        Node &adjset_groups = domain_adjsets["groups"];

        if (i == 0)
        {
            adjset_groups["group_000001/neighbors"] = DataType::int32(2);
            adjset_groups["group_000002/neighbors"] = DataType::int32(2);
            adjset_groups["group_000003/neighbors"] = DataType::int32(2);
            adjset_groups["group_000001/neighbors"].as_int_array()[0] = 0;
            adjset_groups["group_000002/neighbors"].as_int_array()[0] = 0;
            adjset_groups["group_000003/neighbors"].as_int_array()[0] = 0;
            adjset_groups["group_000001/neighbors"].as_int_array()[1] = 1;
            adjset_groups["group_000002/neighbors"].as_int_array()[1] = 2;
            adjset_groups["group_000003/neighbors"].as_int_array()[1] = 3;
            Node &windows_node1 = adjset_groups["group_000001/windows"];
            for(index_t w = 0; w <= 1; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node1[window_name]["origin/i"] = 0;
                windows_node1[window_name]["origin/j"] = 20;
                windows_node1[window_name]["dims/i"] = 21;
                windows_node1[window_name]["dims/j"] = 1;
                windows_node1[window_name]["ratio/i"] = 1;
                windows_node1[window_name]["ratio/j"] = 1;
            }
            Node &windows_node2 = adjset_groups["group_000002/windows"];
            for(index_t w = 0; w <= 2; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node2[window_name]["origin/i"] = 0;
                windows_node2[window_name]["origin/j"] = 20;
                windows_node2[window_name]["dims/i"] = 21;
                windows_node2[window_name]["dims/j"] = 1;
                windows_node2[window_name]["ratio/i"] = 1;
                windows_node2[window_name]["ratio/j"] = 1;
            }
            Node &windows_node3 = adjset_groups["group_000003/windows"];
            for(index_t w = 0; w <= 3; w += 3)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node3[window_name]["origin/i"] = 20;
                windows_node3[window_name]["origin/j"] = 20;
                windows_node3[window_name]["dims/i"] = 1;
                windows_node3[window_name]["dims/j"] = 1;
                windows_node3[window_name]["ratio/i"] = 1;
                windows_node3[window_name]["ratio/j"] = 1;
            }
        }
        else if (i == 1)
        {
            adjset_groups["group_000000/neighbors"] = DataType::int32(2);
            adjset_groups["group_000002/neighbors"] = DataType::int32(2);
            adjset_groups["group_000003/neighbors"] = DataType::int32(2);
            adjset_groups["group_000000/neighbors"].as_int_array()[0] = 1;
            adjset_groups["group_000002/neighbors"].as_int_array()[0] = 1;
            adjset_groups["group_000003/neighbors"].as_int_array()[0] = 1;
            adjset_groups["group_000000/neighbors"].as_int_array()[1] = 0;
            adjset_groups["group_000002/neighbors"].as_int_array()[1] = 2;
            adjset_groups["group_000003/neighbors"].as_int_array()[1] = 3;
            Node &windows_node0 = adjset_groups["group_000000/windows"];
            for(index_t w = 0; w <= 1; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node0[window_name]["origin/i"] = 0;
                windows_node0[window_name]["origin/j"] = 20;
                windows_node0[window_name]["dims/i"] = 21;
                windows_node0[window_name]["dims/j"] = 1;
                windows_node0[window_name]["ratio/i"] = 1;
                windows_node0[window_name]["ratio/j"] = 1;
            }
            Node &windows_node2 = adjset_groups["group_000002/windows"];
            for(index_t w = 1; w <= 2; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node2[window_name]["origin/i"] = 20;
                windows_node2[window_name]["origin/j"] = 20;
                windows_node2[window_name]["dims/i"] = 1;
                windows_node2[window_name]["dims/j"] = 1;
                windows_node2[window_name]["ratio/i"] = 1;
                windows_node2[window_name]["ratio/j"] = 1;
            }
            Node &windows_node3 = adjset_groups["group_000003/windows"];
            for(index_t w = 1; w <= 3; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node3[window_name]["origin/i"] = 20;
                windows_node3[window_name]["origin/j"] = 20;
                windows_node3[window_name]["dims/i"] = 1;
                windows_node3[window_name]["dims/j"] = 21;
                windows_node3[window_name]["ratio/i"] = 1;
                windows_node3[window_name]["ratio/j"] = 1;
            }
        }
        else if (i == 2)
        {
            adjset_groups["group_000000/neighbors"] = DataType::int32(2);
            adjset_groups["group_000001/neighbors"] = DataType::int32(2);
            adjset_groups["group_000003/neighbors"] = DataType::int32(2);
            adjset_groups["group_000004/neighbors"] = DataType::int32(2);
            adjset_groups["group_000005/neighbors"] = DataType::int32(2);
            adjset_groups["group_000000/neighbors"].as_int_array()[0] = 2;
            adjset_groups["group_000001/neighbors"].as_int_array()[0] = 2;
            adjset_groups["group_000003/neighbors"].as_int_array()[0] = 2;
            adjset_groups["group_000004/neighbors"].as_int_array()[0] = 2;
            adjset_groups["group_000005/neighbors"].as_int_array()[0] = 2;
            adjset_groups["group_000000/neighbors"].as_int_array()[1] = 0;
            adjset_groups["group_000001/neighbors"].as_int_array()[1] = 1;
            adjset_groups["group_000003/neighbors"].as_int_array()[1] = 3;
            adjset_groups["group_000004/neighbors"].as_int_array()[1] = 4;
            adjset_groups["group_000005/neighbors"].as_int_array()[1] = 5;
            Node &windows_node0 = adjset_groups["group_000000/windows"];
            for(index_t w = 0; w <= 2; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node0[window_name]["origin/i"] = 20;
                windows_node0[window_name]["origin/j"] = 0;
                windows_node0[window_name]["dims/i"] = 1;
                windows_node0[window_name]["dims/j"] = 21;
                windows_node0[window_name]["ratio/i"] = 1;
                windows_node0[window_name]["ratio/j"] = 1;
            }
            Node &windows_node1 = adjset_groups["group_000001/windows"];
            for(index_t w = 1; w <= 2; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node1[window_name]["origin/i"] = 20;
                windows_node1[window_name]["origin/j"] = 20;
                windows_node1[window_name]["dims/i"] = 1;
                windows_node1[window_name]["dims/j"] = 1;
                windows_node1[window_name]["ratio/i"] = 1;
                windows_node1[window_name]["ratio/j"] = 1;
            }
            Node &windows_node3 = adjset_groups["group_000003/windows"];
            for(index_t w = 2; w <= 3; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node3[window_name]["origin/i"] = 20;
                windows_node3[window_name]["origin/j"] = 20;
                windows_node3[window_name]["dims/i"] = 21;
                windows_node3[window_name]["dims/j"] = 1;
                windows_node3[window_name]["ratio/i"] = 1;
                windows_node3[window_name]["ratio/j"] = 1;
            }
            Node &windows_node4 = adjset_groups["group_000004/windows"];
            for(index_t w = 2; w <= 4; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node4[window_name]["origin/i"] = 40;
                windows_node4[window_name]["origin/j"] = 0;
                windows_node4[window_name]["dims/i"] = 1;
                windows_node4[window_name]["dims/j"] = 21;
                windows_node4[window_name]["ratio/i"] = 1;
                windows_node4[window_name]["ratio/j"] = 1;
            }
            Node &windows_node5 = adjset_groups["group_000005/windows"];
            for(index_t w = 2; w <= 5; w += 3)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node5[window_name]["origin/i"] = 40;
                windows_node5[window_name]["origin/j"] = 20;
                windows_node5[window_name]["dims/i"] = 1;
                windows_node5[window_name]["dims/j"] = 1;
                windows_node5[window_name]["ratio/i"] = 1;
                windows_node5[window_name]["ratio/j"] = 1;
            }
        }
        else if (i == 3)
        {
            adjset_groups["group_000000/neighbors"] = DataType::int32(2);
            adjset_groups["group_000001/neighbors"] = DataType::int32(2);
            adjset_groups["group_000002/neighbors"] = DataType::int32(2);
            adjset_groups["group_000004/neighbors"] = DataType::int32(2);
            adjset_groups["group_000005/neighbors"] = DataType::int32(2);
            adjset_groups["group_000000/neighbors"].as_int_array()[0] = 3;
            adjset_groups["group_000001/neighbors"].as_int_array()[0] = 3;
            adjset_groups["group_000002/neighbors"].as_int_array()[0] = 3;
            adjset_groups["group_000004/neighbors"].as_int_array()[0] = 3;
            adjset_groups["group_000005/neighbors"].as_int_array()[0] = 3;
            adjset_groups["group_000000/neighbors"].as_int_array()[1] = 0;
            adjset_groups["group_000001/neighbors"].as_int_array()[1] = 1;
            adjset_groups["group_000002/neighbors"].as_int_array()[1] = 2;
            adjset_groups["group_000004/neighbors"].as_int_array()[1] = 4;
            adjset_groups["group_000005/neighbors"].as_int_array()[1] = 5;
            Node &windows_node0 = adjset_groups["group_000000/windows"];
            for(index_t w = 0; w <= 3; w += 3)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node0[window_name]["origin/i"] = 20;
                windows_node0[window_name]["origin/j"] = 20;
                windows_node0[window_name]["dims/i"] = 1;
                windows_node0[window_name]["dims/j"] = 1;
                windows_node0[window_name]["ratio/i"] = 1;
                windows_node0[window_name]["ratio/j"] = 1;
            }
            Node &windows_node1 = adjset_groups["group_000001/windows"];
            for(index_t w = 1; w <= 3; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node1[window_name]["origin/i"] = 20;
                windows_node1[window_name]["origin/j"] = 20;
                windows_node1[window_name]["dims/i"] = 1;
                windows_node1[window_name]["dims/j"] = 21;
                windows_node1[window_name]["ratio/i"] = 1;
                windows_node1[window_name]["ratio/j"] = 1;
            }
            Node &windows_node2 = adjset_groups["group_000002/windows"];
            for(index_t w = 2; w <= 3; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node2[window_name]["origin/i"] = 20;
                windows_node2[window_name]["origin/j"] = 20;
                windows_node2[window_name]["dims/i"] = 21;
                windows_node2[window_name]["dims/j"] = 1;
                windows_node2[window_name]["ratio/i"] = 1;
                windows_node2[window_name]["ratio/j"] = 1;
            }
            Node &windows_node4 = adjset_groups["group_000004/windows"];
            for(index_t w = 3; w <= 4; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node4[window_name]["origin/i"] = 40;
                windows_node4[window_name]["origin/j"] = 20;
                windows_node4[window_name]["dims/i"] = 1;
                windows_node4[window_name]["dims/j"] = 1;
                windows_node4[window_name]["ratio/i"] = 1;
                windows_node4[window_name]["ratio/j"] = 1;
            }
            Node &windows_node5 = adjset_groups["group_000005/windows"];
            for(index_t w = 3; w <= 4; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node5[window_name]["origin/i"] = 40;
                windows_node5[window_name]["origin/j"] = 20;
                windows_node5[window_name]["dims/i"] = 1;
                windows_node5[window_name]["dims/j"] = 21;
                windows_node5[window_name]["ratio/i"] = 1;
                windows_node5[window_name]["ratio/j"] = 1;
            }
        }
        else if (i == 4)
        {
            adjset_groups["group_000002/neighbors"] = DataType::int32(2);
            adjset_groups["group_000003/neighbors"] = DataType::int32(2);
            adjset_groups["group_000005/neighbors"] = DataType::int32(2);
            adjset_groups["group_000006/neighbors"] = DataType::int32(2);
            adjset_groups["group_000007/neighbors"] = DataType::int32(2);
            adjset_groups["group_000002/neighbors"].as_int_array()[0] = 4;
            adjset_groups["group_000003/neighbors"].as_int_array()[0] = 4;
            adjset_groups["group_000005/neighbors"].as_int_array()[0] = 4;
            adjset_groups["group_000006/neighbors"].as_int_array()[0] = 4;
            adjset_groups["group_000007/neighbors"].as_int_array()[0] = 4;
            adjset_groups["group_000002/neighbors"].as_int_array()[1] = 2;
            adjset_groups["group_000003/neighbors"].as_int_array()[1] = 3;
            adjset_groups["group_000005/neighbors"].as_int_array()[1] = 5;
            adjset_groups["group_000006/neighbors"].as_int_array()[1] = 6;
            adjset_groups["group_000007/neighbors"].as_int_array()[1] = 7;
            Node &windows_node2 = adjset_groups["group_000002/windows"];
            for(index_t w = 2; w <= 4; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node2[window_name]["origin/i"] = 40;
                windows_node2[window_name]["origin/j"] = 0;
                windows_node2[window_name]["dims/i"] = 1;
                windows_node2[window_name]["dims/j"] = 21;
                windows_node2[window_name]["ratio/i"] = 1;
                windows_node2[window_name]["ratio/j"] = 1;
            }
            Node &windows_node3 = adjset_groups["group_000003/windows"];
            for(index_t w = 3; w <= 4; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node3[window_name]["origin/i"] = 40;
                windows_node3[window_name]["origin/j"] = 20;
                windows_node3[window_name]["dims/i"] = 1;
                windows_node3[window_name]["dims/j"] = 1;
                windows_node3[window_name]["ratio/i"] = 1;
                windows_node3[window_name]["ratio/j"] = 1;
            }
            Node &windows_node5 = adjset_groups["group_000005/windows"];
            for(index_t w = 4; w <= 5; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node5[window_name]["origin/i"] = 40;
                windows_node5[window_name]["origin/j"] = 20;
                windows_node5[window_name]["dims/i"] = 21;
                windows_node5[window_name]["dims/j"] = 1;
                windows_node5[window_name]["ratio/i"] = 1;
                windows_node5[window_name]["ratio/j"] = 1;
            }
            Node &windows_node6 = adjset_groups["group_000006/windows"];
            for(index_t w = 4; w <= 6; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node6[window_name]["origin/i"] = 60;
                windows_node6[window_name]["origin/j"] = 0;
                windows_node6[window_name]["dims/i"] = 1;
                windows_node6[window_name]["dims/j"] = 21;
                windows_node6[window_name]["ratio/i"] = 1;
                windows_node6[window_name]["ratio/j"] = 1;
            }
            Node &windows_node7 = adjset_groups["group_000007/windows"];
            for(index_t w = 4; w <= 7; w += 3)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node7[window_name]["origin/i"] = 60;
                windows_node7[window_name]["origin/j"] = 20;
                windows_node7[window_name]["dims/i"] = 1;
                windows_node7[window_name]["dims/j"] = 1;
                windows_node7[window_name]["ratio/i"] = 1;
                windows_node7[window_name]["ratio/j"] = 1;
            }
        }
        else if (i == 5)
        {
            adjset_groups["group_000002/neighbors"] = DataType::int32(2);
            adjset_groups["group_000003/neighbors"] = DataType::int32(2);
            adjset_groups["group_000004/neighbors"] = DataType::int32(2);
            adjset_groups["group_000006/neighbors"] = DataType::int32(2);
            adjset_groups["group_000007/neighbors"] = DataType::int32(2);
            adjset_groups["group_000002/neighbors"].as_int_array()[0] = 5;
            adjset_groups["group_000003/neighbors"].as_int_array()[0] = 5;
            adjset_groups["group_000004/neighbors"].as_int_array()[0] = 5;
            adjset_groups["group_000006/neighbors"].as_int_array()[0] = 5;
            adjset_groups["group_000007/neighbors"].as_int_array()[0] = 5;
            adjset_groups["group_000002/neighbors"].as_int_array()[1] = 2;
            adjset_groups["group_000003/neighbors"].as_int_array()[1] = 3;
            adjset_groups["group_000004/neighbors"].as_int_array()[1] = 4;
            adjset_groups["group_000006/neighbors"].as_int_array()[1] = 6;
            adjset_groups["group_000007/neighbors"].as_int_array()[1] = 7;
            Node &windows_node2 = adjset_groups["group_000002/windows"];
            for(index_t w = 2; w <= 5; w += 3)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node2[window_name]["origin/i"] = 40;
                windows_node2[window_name]["origin/j"] = 20;
                windows_node2[window_name]["dims/i"] = 1;
                windows_node2[window_name]["dims/j"] = 1;
                windows_node2[window_name]["ratio/i"] = 1;
                windows_node2[window_name]["ratio/j"] = 1;
            }
            Node &windows_node3 = adjset_groups["group_000003/windows"];
            for(index_t w = 3; w <= 5; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node3[window_name]["origin/i"] = 40;
                windows_node3[window_name]["origin/j"] = 20;
                windows_node3[window_name]["dims/i"] = 1;
                windows_node3[window_name]["dims/j"] = 21;
                windows_node3[window_name]["ratio/i"] = 1;
                windows_node3[window_name]["ratio/j"] = 1;
            }
            Node &windows_node4 = adjset_groups["group_000004/windows"];
            for(index_t w = 4; w <= 5; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node4[window_name]["origin/i"] = 40;
                windows_node4[window_name]["origin/j"] = 20;
                windows_node4[window_name]["dims/i"] = 1;
                windows_node4[window_name]["dims/j"] = 21;
                windows_node4[window_name]["ratio/i"] = 1;
                windows_node4[window_name]["ratio/j"] = 1;
            }
            Node &windows_node6 = adjset_groups["group_000006/windows"];
            for(index_t w = 5; w <= 6; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node6[window_name]["origin/i"] = 60;
                windows_node6[window_name]["origin/j"] = 20;
                windows_node6[window_name]["dims/i"] = 1;
                windows_node6[window_name]["dims/j"] = 1;
                windows_node6[window_name]["ratio/i"] = 1;
                windows_node6[window_name]["ratio/j"] = 1;
            }
            Node &windows_node7 = adjset_groups["group_000007/windows"];
            for(index_t w = 5; w <= 7; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node7[window_name]["origin/i"] = 60;
                windows_node7[window_name]["origin/j"] = 20;
                windows_node7[window_name]["dims/i"] = 1;
                windows_node7[window_name]["dims/j"] = 21;
                windows_node7[window_name]["ratio/i"] = 1;
                windows_node7[window_name]["ratio/j"] = 1;
            }
        }
        else if (i == 6)
        {
            adjset_groups["group_000004/neighbors"] = DataType::int32(2);
            adjset_groups["group_000005/neighbors"] = DataType::int32(2);
            adjset_groups["group_000007/neighbors"] = DataType::int32(2);
            adjset_groups["group_000004/neighbors"].as_int_array()[0] = 6;
            adjset_groups["group_000005/neighbors"].as_int_array()[0] = 6;
            adjset_groups["group_000007/neighbors"].as_int_array()[0] = 6;
            adjset_groups["group_000004/neighbors"].as_int_array()[1] = 4;
            adjset_groups["group_000005/neighbors"].as_int_array()[1] = 5;
            adjset_groups["group_000007/neighbors"].as_int_array()[1] = 7;
            Node &windows_node4 = adjset_groups["group_000004/windows"];
            for(index_t w = 4; w <= 6; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node4[window_name]["origin/i"] = 60;
                windows_node4[window_name]["origin/j"] = 0;
                windows_node4[window_name]["dims/i"] = 1;
                windows_node4[window_name]["dims/j"] = 21;
                windows_node4[window_name]["ratio/i"] = 1;
                windows_node4[window_name]["ratio/j"] = 1;
            }
            Node &windows_node5 = adjset_groups["group_000005/windows"];
            for(index_t w = 5; w <= 6; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node5[window_name]["origin/i"] = 60;
                windows_node5[window_name]["origin/j"] = 20;
                windows_node5[window_name]["dims/i"] = 1;
                windows_node5[window_name]["dims/j"] = 1;
                windows_node5[window_name]["ratio/i"] = 1;
                windows_node5[window_name]["ratio/j"] = 1;
            }
            Node &windows_node7 = adjset_groups["group_000007/windows"];
            for(index_t w = 6; w <= 7; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node7[window_name]["origin/i"] = 60;
                windows_node7[window_name]["origin/j"] = 20;
                windows_node7[window_name]["dims/i"] = 21;
                windows_node7[window_name]["dims/j"] = 1;
                windows_node7[window_name]["ratio/i"] = 1;
                windows_node7[window_name]["ratio/j"] = 1;
            }
        }
        else if (i == 7)
        {
            adjset_groups["group_000004/neighbors"] = DataType::int32(2);
            adjset_groups["group_000005/neighbors"] = DataType::int32(2);
            adjset_groups["group_000006/neighbors"] = DataType::int32(2);
            adjset_groups["group_000004/neighbors"].as_int_array()[0] = 7;
            adjset_groups["group_000005/neighbors"].as_int_array()[0] = 7;
            adjset_groups["group_000006/neighbors"].as_int_array()[0] = 7;
            adjset_groups["group_000004/neighbors"].as_int_array()[1] = 4;
            adjset_groups["group_000005/neighbors"].as_int_array()[1] = 5;
            adjset_groups["group_000006/neighbors"].as_int_array()[1] = 6;
            Node &windows_node4 = adjset_groups["group_000004/windows"];
            for(index_t w = 4; w <= 7; w += 3)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node4[window_name]["origin/i"] = 60;
                windows_node4[window_name]["origin/j"] = 20;
                windows_node4[window_name]["dims/i"] = 1;
                windows_node4[window_name]["dims/j"] = 1;
                windows_node4[window_name]["ratio/i"] = 1;
                windows_node4[window_name]["ratio/j"] = 1;
            }
            Node &windows_node5 = adjset_groups["group_000005/windows"];
            for(index_t w = 5; w <= 7; w += 2)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node5[window_name]["origin/i"] = 60;
                windows_node5[window_name]["origin/j"] = 20;
                windows_node5[window_name]["dims/i"] = 1;
                windows_node5[window_name]["dims/j"] = 21;
                windows_node5[window_name]["ratio/i"] = 1;
                windows_node5[window_name]["ratio/j"] = 1;
            }
            Node &windows_node6 = adjset_groups["group_000006/windows"];
            for(index_t w = 6; w <= 7; w++)
            {
                std::ostringstream w_oss;
                w_oss << "window_00000" << w;
                const std::string window_name = w_oss.str();
                windows_node6[window_name]["origin/i"] = 60;
                windows_node6[window_name]["origin/j"] = 20;
                windows_node6[window_name]["dims/i"] = 21;
                windows_node6[window_name]["dims/j"] = 1;
                windows_node6[window_name]["ratio/i"] = 1;
                windows_node6[window_name]["ratio/j"] = 1;
            }
        }
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
// -- end conduit::blueprint:: --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
