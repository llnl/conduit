// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples_gyre.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// std lib includes
//-----------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <math.h>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh_examples.hpp"
#include "conduit_blueprint_mesh.hpp"


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

const float64 PI_VALUE = 3.14159265359;

//---------------------------------------------------------------------------//
void gyre(index_t nx_verts,
          index_t ny_verts,
          index_t nz_verts,
          float64 t, // time
          Node &res)
{
    res.reset();

    // min of two verts for each dim

    // create a uniform coordset
    res["state/time"] = t;
    res["coordsets/coords/type"] = "uniform";
    res["coordsets/coords/dims/i"] = nx_verts;
    res["coordsets/coords/dims/j"] = ny_verts;
    res["coordsets/coords/dims/k"] = nz_verts;

    res["coordsets/coords/origin/x"] = 0.0;
    res["coordsets/coords/origin/y"] = 0.0;
    res["coordsets/coords/origin/z"] = 0.0;

    res["coordsets/coords/spacing/dx"] = 1.0 / float64(nx_verts-1.0);
    res["coordsets/coords/spacing/dy"] = 1.0 / float64(ny_verts-1.0);
    res["coordsets/coords/spacing/dz"] = 1.0 / float64(nz_verts-1.0);

    // create a uniform topo
    res["topologies/topo/type"] = "uniform";
    res["topologies/topo/coordset"] = "coords";

    // create gyre vector field and gyre mag field
    Node &gyre_mag_field = res["fields/gyre_mag"];
    gyre_mag_field["association"] = "vertex";
    gyre_mag_field["topology"] = "topo";
    gyre_mag_field["values"].set(DataType::float64(nx_verts * ny_verts * nz_verts));
    float64_array gyre_mag_vals = gyre_mag_field["values"].value();

    Node &gyre_vec_field = res["fields/gyre"];
    gyre_vec_field["association"] = "vertex";
    gyre_vec_field["topology"] = "topo";
    gyre_vec_field["values/u"].set(DataType::float64(nx_verts * ny_verts * nz_verts));
    gyre_vec_field["values/v"].set(DataType::float64(nx_verts * ny_verts * nz_verts));
    gyre_vec_field["values/w"].set(DataType::float64(nx_verts * ny_verts * nz_verts));
    // w stays zero
    float64_array gyre_vec_vals_u = gyre_vec_field["values/u"].value();
    float64_array gyre_vec_vals_v = gyre_vec_field["values/v"].value();

    float64 e = 0.25;
    float64 a = 0.1;
    float64 w = (2.0 * PI_VALUE) / 10.0;
    float64 a_t = e * sin(w * t);
    float64 b_t = 1.0 - 2 * e * sin(w * t);

    index_t idx = 0;
    for(index_t z=0; z < nz_verts; z++)
    {
        for(index_t y=0; y < ny_verts; y++)
        {
            // scale y to 0-1
            float64 y_n = float64(y)/float64(ny_verts);

            for(index_t x=0; x < nx_verts; x++)
            {
                // scale x to 0-1
                float64 x_f = float64(x)/ (float64(nx_verts) * .5);
                float64 f_t = a_t * x_f * x_f + b_t * x_f;
                float64 u = -PI_VALUE * a * sin(PI_VALUE * f_t) * cos(PI_VALUE* y_n);
                // for more variation
                //float64 u = -PI_VALUE * a * sin(PI_VALUE * f_t) * cos(PI_VALUE* y_t);
                float64 df_dx = 2.0 * a_t + b_t;
                // for more variation
                //float64 v = PI_VALUE * a * cos(PI_VALUE * f_t) * sin(PI_VALUE * y_t) * df_dx;
                float64 v = PI_VALUE * a * cos(PI_VALUE * f_t) * sin(PI_VALUE * y_n) * df_dx;
                gyre_mag_vals[idx] = sqrt(u * u + v * v);
                gyre_vec_vals_u[idx] = u;
                gyre_vec_vals_v[idx] = v;
                idx++;
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
