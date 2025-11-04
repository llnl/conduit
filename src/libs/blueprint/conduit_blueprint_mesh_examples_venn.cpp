// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples_julia.cpp
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
void venn_full_matset(Node &res, index_t nx, index_t ny)
{
    // create the material sets

    index_t elements = nx * ny;

    // Nodes are evenly spaced from 0 through 1.
    float64 dx = 1.0 / float64(nx);
    float64 dy = 1.0 / float64(ny);
    float64 element_area = dx * dy;

    // create importance reference
    Node &mat_importance = res["meta/importance"];

    res["matsets/matset/topology"] = "topo";
    res["matsets/matset/volume_fractions/background"] = DataType::float64(elements);
    res["matsets/matset/volume_fractions/circle_a"] = DataType::float64(elements);
    res["matsets/matset/volume_fractions/circle_b"] = DataType::float64(elements);
    res["matsets/matset/volume_fractions/circle_c"] = DataType::float64(elements);

    res["fields/area/matset_values/background"] = DataType::float64(elements);
    res["fields/area/matset_values/circle_a"] = DataType::float64(elements);
    res["fields/area/matset_values/circle_b"] = DataType::float64(elements);
    res["fields/area/matset_values/circle_c"] = DataType::float64(elements);

    res["fields/importance/matset_values/background"] = DataType::float64(elements);
    res["fields/importance/matset_values/circle_a"] = DataType::float64(elements);
    res["fields/importance/matset_values/circle_b"] = DataType::float64(elements);
    res["fields/importance/matset_values/circle_c"] = DataType::float64(elements);

    res["fields/mat_check/matset_values/background"] = DataType::int64(elements);
    res["fields/mat_check/matset_values/circle_a"] = DataType::int64(elements);
    res["fields/mat_check/matset_values/circle_b"] = DataType::int64(elements);
    res["fields/mat_check/matset_values/circle_c"] = DataType::int64(elements);

    float64_array cir_a = res["fields/circle_a/values"].value();
    float64_array cir_b = res["fields/circle_b/values"].value();
    float64_array cir_c = res["fields/circle_c/values"].value();
    float64_array bg    = res["fields/background/values"].value();

    float64_array area = res["fields/area/values"].value();
    float64_array matset_area_bg = res["fields/area/matset_values/background"].value();
    float64_array matset_area_cir_a = res["fields/area/matset_values/circle_a"].value();
    float64_array matset_area_cir_b = res["fields/area/matset_values/circle_b"].value();
    float64_array matset_area_cir_c = res["fields/area/matset_values/circle_c"].value();

    float64_array importance = res["fields/importance/values"].value();
    float64_array matset_importance_bg = res["fields/importance/matset_values/background"].value();
    float64_array matset_importance_cir_a = res["fields/importance/matset_values/circle_a"].value();
    float64_array matset_importance_cir_b = res["fields/importance/matset_values/circle_b"].value();
    float64_array matset_importance_cir_c = res["fields/importance/matset_values/circle_c"].value();

    int64_array mat_check = res["fields/mat_check/values"].value();
    int64_array mat_check_bg = res["fields/mat_check/matset_values/background"].value();
    int64_array mat_check_bg_cir_a = res["fields/mat_check/matset_values/circle_a"].value();
    int64_array mat_check_bg_cir_b = res["fields/mat_check/matset_values/circle_b"].value();
    int64_array mat_check_bg_cir_c = res["fields/mat_check/matset_values/circle_c"].value();

    float64_array mat_bg = res["matsets/matset/volume_fractions/background"].value();
    float64_array mat_ca = res["matsets/matset/volume_fractions/circle_a"].value();
    float64_array mat_cb = res["matsets/matset/volume_fractions/circle_b"].value();
    float64_array mat_cc = res["matsets/matset/volume_fractions/circle_c"].value();
    
    for(index_t idx = 0; idx < elements; idx++)
    {
        mat_ca[idx] = cir_a[idx];
        mat_cb[idx] = cir_b[idx];
        mat_cc[idx] = cir_c[idx];
        mat_bg[idx] = bg[idx];

        if (mat_ca[idx] > 0.) { matset_area_cir_a[idx] = element_area; }
        if (mat_cb[idx] > 0.) { matset_area_cir_b[idx] = element_area; }
        if (mat_cc[idx] > 0.) { matset_area_cir_c[idx] = element_area; }
        if (mat_bg[idx] > 0.) { matset_area_bg[idx] = element_area; }

        // the overall sum provides a value that can id all mats
        // in a zone, these are the per-material components
        if (mat_bg[idx] > 0.) { mat_check_bg[idx]       = 1; }
        if (mat_ca[idx] > 0.) { mat_check_bg_cir_a[idx] = 20; }
        if (mat_cb[idx] > 0.) { mat_check_bg_cir_b[idx] = 300; }
        if (mat_cc[idx] > 0.) { mat_check_bg_cir_c[idx] = 4000; }

        area[idx] = mat_ca[idx] * matset_area_cir_a[idx] +
            mat_cb[idx] * matset_area_cir_b[idx] +
            mat_cc[idx] * matset_area_cir_c[idx] +
            mat_bg[idx] * matset_area_bg[idx];

        if (mat_ca[idx] > 0.) { matset_importance_cir_a[idx] = mat_importance["a"].value(); }
        if (mat_cb[idx] > 0.) { matset_importance_cir_b[idx] = mat_importance["b"].value(); }
        if (mat_cc[idx] > 0.) { matset_importance_cir_c[idx] = mat_importance["c"].value(); }
        float64 x_pos = ((float64)(idx % nx)) / nx;
        float64 y_pos = ((float64)(idx / nx)) / ny;
        if (mat_bg[idx] > 0.) { matset_importance_bg[idx] = x_pos + y_pos; }

        importance[idx] = mat_ca[idx] * matset_importance_cir_a[idx] +
            mat_cb[idx] * matset_importance_cir_b[idx] +
            mat_cc[idx] * matset_importance_cir_c[idx] +
            mat_bg[idx] * matset_importance_bg[idx];
    }
}

//---------------------------------------------------------------------------//
void build_material_sparse(Node & src, index_t len,
    const std::string & mat_name,
    float64 element_area, float64 material_importance,
    Node & matset_area, Node & matset_importance, Node & matset)
{
    float64_array src_val = src.value();

    index_t nsparse = 0;
    for (index_t idx = 0; idx < len; ++idx)
    {
        if (src_val[idx] > 0)
        {
            nsparse += 1;
        }
    }

    matset["volume_fractions/" + mat_name].set(DataType::float64(nsparse));
    matset["element_ids/" + mat_name].set(DataType::int32(nsparse));
    float64_array sparse_val = matset["volume_fractions/" + mat_name].value();
    int32_array sparse_element_ids = matset["element_ids/" + mat_name].value();

    matset_area.set(DataType::float64(nsparse));
    float64_array matset_area_val = matset_area.value();
    matset_importance.set(DataType::float64(nsparse));
    float64_array matset_importance_val = matset_importance.value();

    index_t sparse_idx = 0;
    for (index_t idx = 0; idx < len; ++idx)
    {
        if (src_val[idx] > 0)
        {
            sparse_element_ids[sparse_idx] = (int32)idx;
            sparse_val[sparse_idx] = src_val[idx];

            matset_area_val[sparse_idx] = element_area;
            matset_importance_val[sparse_idx] = material_importance;

            sparse_idx += 1;
        }
    }
}

//---------------------------------------------------------------------------//
void compute_material_sparse_matset_field(Node &res,
                                          const std::string & field_name,
                                          index_t nx,
                                          index_t ny)
{
    index_t elements = nx * ny;

    Node & n = res["fields/" + field_name + "/values"];
    n.set(DataType::float64(elements));
    float64_array n_val = n.value();

    Node & matset_values = res["fields/" + field_name + "/matset_values"];

    NodeIterator itr = matset_values.children();
    while (itr.has_next())
    {
        Node &cld = itr.next();
        const std::string & cld_name = itr.name();
        float64_array matset_vals = cld.value();

        float64_array vf_vals = res["matsets/matset/volume_fractions/" + cld_name].value();
        int32_array vf_elt_ids = res["matsets/matset/element_ids/" + cld_name].value();
        index_t sparse_elements = vf_elt_ids.number_of_elements();

        index_t sparse_index = 0;
        for (index_t elt = 0; elt < elements && sparse_index < sparse_elements; ++elt)
        {
            if (vf_elt_ids[sparse_index] == elt)
            {
                n_val[elt] += matset_vals[sparse_index] * vf_vals[sparse_index];
                sparse_index += 1;
            }
        }
    }
}

//---------------------------------------------------------------------------//
void venn_sparse_by_material_matset(Node &res, index_t nx, index_t ny)
{    
    // create the materials
    
    // make sure our materials appear in the correct order,
    // by pre-creating the nodes.
    res["matsets/matset/volume_fractions/background"];
    res["matsets/matset/volume_fractions/circle_a"];
    res["matsets/matset/volume_fractions/circle_b"];
    res["matsets/matset/volume_fractions/circle_c"];

    res["matsets/matset/element_ids/background"];
    res["matsets/matset/element_ids/circle_a"];
    res["matsets/matset/element_ids/circle_b"];
    res["matsets/matset/element_ids/circle_c"];

    // we could also use the following material map:
    //
    // res["matsets/matset/material_map/background"] = 0;
    // res["matsets/matset/material_map/circle_a"] = 1;
    // res["matsets/matset/material_map/circle_b"] = 2;
    // res["matsets/matset/material_map/circle_c"] = 3;
    //
    // however, this current setup is a good test of the
    // non explicit map case


    float64_array cir_a = res["fields/circle_a/values"].value();
    float64_array cir_b = res["fields/circle_b/values"].value();
    float64_array cir_c = res["fields/circle_c/values"].value();

    res["matsets/matset/topology"] = "topo";
    
    // Nodes are evenly spaced from 0 through 1.
    float64 dx = 1.0 / float64(nx);
    float64 dy = 1.0 / float64(ny);
    float64 element_area = dx * dy;
    index_t elements = nx * ny;

    // create importance reference
    Node &mat_importance = res["meta/importance"];

    build_material_sparse(res["fields/circle_a/values"],
        elements,
        "circle_a",
        element_area,
        mat_importance["a"].value(),
        res["fields/area/matset_values/circle_a"], 
        res["fields/importance/matset_values/circle_a"],
        res["matsets/matset"]);

    build_material_sparse(res["fields/circle_b/values"],
        elements,
        "circle_b",
        element_area,
        mat_importance["b"].value(),
        res["fields/area/matset_values/circle_b"],
        res["fields/importance/matset_values/circle_b"],
        res["matsets/matset"]);

    build_material_sparse(res["fields/circle_c/values"],
        elements,
        "circle_c",
        element_area,
        mat_importance["c"].value(),
        res["fields/area/matset_values/circle_c"],
        res["fields/importance/matset_values/circle_c"],
        res["matsets/matset"]);

    // The background material volume fraction depends on the other three
    // materials, so we deal with it in a custom loop.
    index_t bgcount = 0;
    for (index_t idx = 0; idx < elements; ++idx)
    {
        if (cir_a[idx] + cir_b[idx] + cir_c[idx] < 1.) bgcount += 1;
    }

    res["matsets/matset/volume_fractions/background"].set(DataType::float64(bgcount));
    res["matsets/matset/element_ids/background"].set(DataType::int32(bgcount));
    float64_array bg_val = res["matsets/matset/volume_fractions/background"].value();
    int32_array bg_idx = res["matsets/matset/element_ids/background"].value();

    Node &matset_area_bg = res["fields/area/matset_values/background"];
    matset_area_bg.set(DataType::float64(bgcount));
    float64_array matset_area_bg_value = matset_area_bg.value();

    Node &matset_importance_bg = res["fields/importance/matset_values/background"];
    matset_importance_bg.set(DataType::float64(bgcount));
    float64_array matset_importance_bg_value = matset_importance_bg.value();

    index_t nidx = 0;
    for (index_t idx = 0; idx < elements; ++idx)
    {
        float64 x_pos = ((float64)(idx % nx)) / nx;
        float64 y_pos = ((float64)(idx / nx)) / ny;

        float64 fgvf = cir_a[idx] + cir_b[idx] + cir_c[idx];
        if (fgvf < 1.)
        {
            bg_idx[nidx] = (int32)idx;

            bg_val[nidx] = 1. - fgvf;

            matset_area_bg_value[nidx] = element_area;
            matset_importance_bg_value[nidx] = x_pos + y_pos;

            nidx += 1;
        }
    }

    // --------- //
    // mat_check //
    // --------- //

    // the matset now has element id's for each material computed.
    // to create mat_check, we simply need to fill arrays of the correct size
    // with our unique value for each mat
    
    // Note: we already have bgcount
    index_t cir_a_count = res["matsets/matset/element_ids/circle_a"].dtype().number_of_elements();
    index_t cir_b_count = res["matsets/matset/element_ids/circle_b"].dtype().number_of_elements();
    index_t cir_c_count = res["matsets/matset/element_ids/circle_c"].dtype().number_of_elements();

    // init our matset_values arrays
    res["fields/mat_check/matset_values/background"].set(DataType::int64(bgcount));
    res["fields/mat_check/matset_values/circle_a"].set(DataType::int64(cir_a_count));
    res["fields/mat_check/matset_values/circle_b"].set(DataType::int64(cir_b_count));
    res["fields/mat_check/matset_values/circle_c"].set(DataType::int64(cir_c_count));

    // set with unique values

    // NOTE: this is good use case for adding DataArray.fill ...
    int64_array m_chk_bg_vals = res["fields/mat_check/matset_values/background"].value();
        
    for(index_t idx=0;idx < bgcount; idx++)
    {
        m_chk_bg_vals[idx] = 1;
    }

    int64_array m_chk_cir_a_vals = res["fields/mat_check/matset_values/circle_a"].value();
    for(index_t idx=0;idx < cir_a_count; idx++)
    {
        m_chk_cir_a_vals[idx] = 20;
    }

    int64_array m_chk_cir_b_vals = res["fields/mat_check/matset_values/circle_b"].value();
    for(index_t idx=0;idx < cir_b_count; idx++)
    {
        m_chk_cir_b_vals[idx] = 300;
    }

    int64_array m_chk_cir_c_vals = res["fields/mat_check/matset_values/circle_c"].value();
    for(index_t idx=0;idx < cir_c_count; idx++)
    {
        m_chk_cir_c_vals[idx] = 4000;
    }

    // Now we've computed the matset values for the fields area and
    // importance, sum the product of the volume fraction and the matset
    // value for each element to compute the field itself.

    compute_material_sparse_matset_field(res, "area", nx, ny);
    compute_material_sparse_matset_field(res, "importance", nx, ny);
}

void venn_sparse_by_element_matset(Node &res, index_t nx, index_t ny)
{
    // create the materials

    float64_array cir_a = res["fields/circle_a/values"].value();
    float64_array cir_b = res["fields/circle_b/values"].value();
    float64_array cir_c = res["fields/circle_c/values"].value();

    // Nodes are evenly spaced from 0 through 1.
    float64 dx = 1.0 / float64(nx);
    float64 dy = 1.0 / float64(ny);
    float64 element_area = dx * dy;
    index_t elements = nx * ny;
    index_t vfcount = 0;

    // create importance reference
    Node &mat_importance = res["meta/importance"];

    // Count up all the non-zero volume fragments
    // (so we can allocate a correctly-sized buffer)
    for (index_t idx = 0; idx < elements; ++idx)
    {
        if (cir_a[idx] > 0.) vfcount += 1;
        if (cir_b[idx] > 0.) vfcount += 1;
        if (cir_c[idx] > 0.) vfcount += 1;
        if (cir_a[idx] + cir_b[idx] + cir_c[idx] < 1.) vfcount += 1;
    }

    // Build the rest of the single-buffer matset
    res["matsets/matset/topology"] = "topo";
    // This is the "key" that tells what material each volume fraction refers to
    res["matsets/matset/material_map/circle_a"] = 1;
    res["matsets/matset/material_map/circle_b"] = 2;
    res["matsets/matset/material_map/circle_c"] = 3;
    res["matsets/matset/material_map/background"] = 0;

    // All the volume fractions go here ("one big buffer")
    res["matsets/matset/volume_fractions"].set(DataType::float64(vfcount));
    // The material measured by each volume fraction
    res["matsets/matset/material_ids"].set(DataType::int32(vfcount));
    // The number of volume fractions in an element
    res["matsets/matset/sizes"].set(DataType::int32(elements));
    // The offset of the first vf in an element
    res["matsets/matset/offsets"].set(DataType::int32(elements));

    // The matset values (for fields that have them) get built up
    // in the same way as the sparse fields, into "one big array."
    res["fields/area/matset_values"].set(DataType::float64(vfcount));
    res["fields/importance/matset_values"].set(DataType::float64(vfcount));
    res["fields/mat_check/matset_values"].set(DataType::float64(vfcount));

    // The actual fields
    res["fields/area/values"].set(DataType::float64(elements));
    res["fields/importance/values"].set(DataType::float64(elements));

    float64_array vf = res["matsets/matset/volume_fractions"].value();
    int32_array id = res["matsets/matset/material_ids"].value();
    int32_array sizes = res["matsets/matset/sizes"].value();
    int32_array offsets = res["matsets/matset/offsets"].value();

    float64_array matset_area = res["fields/area/matset_values"].value();
    float64_array matset_impt = res["fields/importance/matset_values"].value();

    float64_array field_area = res["fields/area/values"].value();
    float64_array field_impt = res["fields/importance/values"].value();

    float64_array matset_check_vals = res["fields/mat_check/matset_values"].value();

    // Build up the arrays!
    index_t vfidx = 0;
    for (index_t idx = 0; idx < elements; ++idx)
    {
        int size = 0;
        float64 ca = cir_a[idx];
        float64 cb = cir_b[idx];
        float64 cc = cir_c[idx];

        field_area[idx] = 0.;
        field_impt[idx] = 0.;

        auto fill_in = [&](float64 frac, float64 imp, int64 mcheck, int32 mat_id)
        {
            vf[vfidx + size] = frac;
            matset_area[vfidx + size] = element_area;
            matset_impt[vfidx + size] = imp;
            matset_check_vals[vfidx + size] = static_cast<float64>(mcheck);
            id[vfidx + size] = mat_id;

            field_area[idx] += matset_area[vfidx + size] * vf[vfidx + size];
            field_impt[idx] += matset_impt[vfidx + size] * vf[vfidx + size];

            size += 1;
        };

        if (ca > 0.)
        {
            fill_in(ca, mat_importance["a"].value(), 20, 1);
        }
        if (cb > 0.)
        {
            fill_in(cb, mat_importance["b"].value(), 300, 2);
        }
        if (cc > 0.)
        {
            fill_in(cc, mat_importance["c"].value(), 4000, 3);
        }
        if (ca + cb + cc < 1.)
        {
            float64 x_pos = ((float64)(idx % nx)) / nx;
            float64 y_pos = ((float64)(idx / nx)) / ny;
            fill_in(1 - (ca + cb + cc), x_pos + y_pos, 1, 0);
        }

        sizes[idx] = size;
        offsets[idx] = (int32) vfidx;
        vfidx += size;
    }
}

void venn(const std::string &matset_type,
          index_t nx,
          index_t ny,
          float64 radius, 
          Node &res)
{
    res.reset();
    // create a rectilinear coordset 
    res["coordsets/coords/type"] = "rectilinear";
    res["coordsets/coords/values/x"] = DataType::float64(nx+1);
    res["coordsets/coords/values/y"] = DataType::float64(ny+1);
    
    float64_array x_coords = res["coordsets/coords/values/x"].value();
    float64_array y_coords = res["coordsets/coords/values/y"].value(); 
    
    // 0 <-> 1
    float64 dx = 1.0/float64(nx);
    float64 dy = 1.0/float64(ny);

    float64 vx = 0;
    for(index_t i =0; i< nx+1; i++)
    {
        x_coords[i] = vx;
        vx+=dx;
    }
    
    float64 vy = 0;
    for(index_t i =0; i< ny+1; i++)
    {
        y_coords[i] = vy;
        vy+=dy;
    }
    
    // create the topology
    
    res["topologies/topo/type"] = "rectilinear";
    res["topologies/topo/coordset"] = "coords";

    // create the fields

    // circle a distance field
    res["fields/radius_a/association"] = "element";
    res["fields/radius_a/topology"] = "topo";
    res["fields/radius_a/values"] = DataType::float64(nx * ny);
    // circle a vf
    res["fields/circle_a/association"] = "element";
    res["fields/circle_a/topology"] = "topo";
    res["fields/circle_a/values"] = DataType::float64(nx * ny);

    // circle b distance field
    res["fields/radius_b/association"] = "element";
    res["fields/radius_b/topology"] = "topo";
    res["fields/radius_b/values"] = DataType::float64(nx * ny);
    // circle b vf
    res["fields/circle_b/association"] = "element";
    res["fields/circle_b/topology"] = "topo";
    res["fields/circle_b/values"] = DataType::float64(nx * ny);

    // circle c distance field
    res["fields/radius_c/association"] = "element";
    res["fields/radius_c/topology"] = "topo";
    res["fields/radius_c/values"] = DataType::float64(nx * ny);
    // circle b vf
    res["fields/circle_c/association"] = "element";
    res["fields/circle_c/topology"] = "topo";
    res["fields/circle_c/values"] = DataType::float64(nx * ny);

    // per element how many circles overlap
    res["fields/overlap/association"] = "element";
    res["fields/overlap/topology"] = "topo";
    res["fields/overlap/values"] = DataType::float64(nx * ny);

    // per element background
    res["fields/background/association"] = "element";
    res["fields/background/topology"] = "topo";
    res["fields/background/values"] = DataType::float64(nx * ny);

    // per element field with matset values.
    //
    // For a field with matset values, each element will have a value equal
    // to the sum of each material's volume fraction times the material's
    // matset value.
    //
    // As with ordinary fields, fields/field/values holds the overall value
    // for each cell.  Additionally, fields/field/matset_values holds the 
    // matset value for each contributing material.  The matset values are
    // stored in the same way as matsets/matset/volume_fractions (full,
    // sparse-by-material, one-buffer-sparse-by-element).
    //
    // Area is trivial to compute and easy to verify.
    res["fields/area/association"] = "element";
    res["fields/area/topology"] = "topo";
    res["fields/area/matset"] = "matset";
    res["fields/area/values"] = DataType::float64(nx * ny);
    // "Importance" is a made-up field.
    // Circles a, b, and c have differing importance.
    // Background material has importance that varies with position.
    res["fields/importance/association"] = "element";
    res["fields/importance/topology"] = "topo";
    res["fields/importance/matset"] = "matset";
    res["fields/importance/values"] = DataType::float64(nx * ny);

    // "mat_check" is a made-up field.
    // It computes unique sum that encodes the material ids
    // for verification, and provides per-material components 
    // via  matset_values
    res["fields/mat_check/association"] = "element";
    res["fields/mat_check/topology"] = "topo";
    res["fields/mat_check/matset"] = "matset";
    res["fields/mat_check/values"] = DataType::int64(nx * ny);

    float64_array rad_a = res["fields/radius_a/values"].value();
    float64_array cir_a = res["fields/circle_a/values"].value();
    
    float64_array rad_b = res["fields/radius_b/values"].value();
    float64_array cir_b = res["fields/circle_b/values"].value();

    float64_array rad_c = res["fields/radius_c/values"].value();
    float64_array cir_c = res["fields/circle_c/values"].value();

    float64_array bg = res["fields/background/values"].value();

    float64_array area = res["fields/area/values"].value();
    float64_array importance = res["fields/importance/values"].value();

    float64_array olap  = res["fields/overlap/values"].value();

    int64_array mat_check = res["fields/mat_check/values"].value();


    // circle a
    // centered at:
    //   x = 2/3 of width
    //   y = 2/3 of width
    
    float64 a_center_x = 0.3333333333;
    float64 a_center_y = 0.6666666666;
    float64 a_importance = 0.1f;

    // circle b
    // centered at:
    //   x = 2/3 of width
    //   y = 2/3 of width

    float64 b_center_x = 0.6666666666;
    float64 b_center_y = 0.6666666666;
    float64 b_importance = 0.2f;

    // circle c
    // centered at:
    //   x = 1/2 of width
    //   y = 2/3 of width

    float64 c_center_x = 0.5;
    float64 c_center_y = 0.3333333333;
    float64 c_importance = 0.6f;

    // Fill in fields

    float64 y = 0;
    
    index_t idx = 0;
    for(index_t j = 0; j < ny; j++)
    {
        float64 x = 0;
        for(index_t i = 0; i < nx; i++)
        {
            // dist to circle a
            rad_a[idx] = sqrt( (x - a_center_x)*(x - a_center_x) +
                               (y - a_center_y)*(y - a_center_y));
            if(rad_a[idx] > radius)
                rad_a[idx] = 0.0; // clamp outside of radius

            // dist to circle b
            rad_b[idx] = sqrt( (x - b_center_x)*(x - b_center_x) +
                               (y - b_center_y)*(y - b_center_y));
            if(rad_b[idx] > radius)
                rad_b[idx] = 0.0; // clamp outside of radius

            // dist to circle c
            rad_c[idx] = sqrt( (x - c_center_x)*(x - c_center_x) +
                               (y - c_center_y)*(y - c_center_y));
            if(rad_c[idx] > radius)
                rad_c[idx] = 0.0; // clamp outside of radius

            // populate overlap with count
            if(rad_a[idx] > 0)
                olap[idx]++;

            if(rad_b[idx] > 0)
                olap[idx]++;

            if(rad_c[idx] > 0)
                olap[idx]++;

            // circle vfs
            if(rad_a[idx] > 0)
            {
                mat_check[idx] += 20;
                cir_a[idx] = 1.0/olap[idx];
            }
            else
            {
                cir_a[idx] = 0.0;
            }

            if(rad_b[idx] > 0)
            {
                mat_check[idx] += 300;
                cir_b[idx] = 1.0/olap[idx];
            }
            else
            {
                cir_b[idx] = 0.0;
            }

            if(rad_c[idx] > 0)
            {
                mat_check[idx] += 4000;
                cir_c[idx] = 1.0/olap[idx];
            }
            else
            {
                cir_c[idx] = 0.0;
            }

            // bg vf
            bg[idx] = 1. - (cir_a[idx] + cir_b[idx] + cir_c[idx]);
            
            if(bg[idx] > 0.0 )
            {
                mat_check[idx] += 1;
            }

            // initialize area and importance to 0.
            area[idx] = 0.;
            importance[idx] = 0.;

            x+=dx;
            idx++;
        }
        y+=dy;
    }

    // Fill in metadata (used by helper functions)

    res["meta/importance/a"] = a_importance;
    res["meta/importance/b"] = b_importance;
    res["meta/importance/c"] = c_importance;

    // Shape in materials; compute fields with matset values.

    if (matset_type == "full")
    {
        venn_full_matset(res, nx, ny);
    }
    else if (matset_type == "sparse_by_material")
    {
        venn_sparse_by_material_matset(res, nx, ny);
    }
    else if (matset_type == "sparse_by_element")
    {
        venn_sparse_by_element_matset(res, nx, ny);
    }
    else
    {
        CONDUIT_ERROR("unknown matset_type = " << matset_type);
    }
    
    // remove temp tree used during construction
    res.remove("meta");
}

//---------------------------------------------------------------------------//
void venn_full_specset(Node &res, index_t nx, index_t ny)
{
    // create the species sets

    index_t elements = nx * ny;

    res["specsets/specset/matset"] = "matset";
    res["specsets/specset/matset_values/background/bg_spec1"] = DataType::float64(elements);
    res["specsets/specset/matset_values/circle_a/a_spec1"] = DataType::float64(elements);
    res["specsets/specset/matset_values/circle_a/a_spec2"] = DataType::float64(elements);
    res["specsets/specset/matset_values/circle_b/b_spec1"] = DataType::float64(elements);
    res["specsets/specset/matset_values/circle_b/b_spec2"] = DataType::float64(elements);
    res["specsets/specset/matset_values/circle_c/c_spec1"] = DataType::float64(elements);
    res["specsets/specset/matset_values/circle_c/c_spec2"] = DataType::float64(elements);
    res["specsets/specset/matset_values/circle_c/c_spec3"] = DataType::float64(elements);

    float64_array bg_spec1 = res["specsets/specset/matset_values/background/bg_spec1"].value();
    float64_array ca_spec1 = res["specsets/specset/matset_values/circle_a/a_spec1"].value();
    float64_array ca_spec2 = res["specsets/specset/matset_values/circle_a/a_spec2"].value();
    float64_array cb_spec1 = res["specsets/specset/matset_values/circle_b/b_spec1"].value();
    float64_array cb_spec2 = res["specsets/specset/matset_values/circle_b/b_spec2"].value();
    float64_array cc_spec1 = res["specsets/specset/matset_values/circle_c/c_spec1"].value();
    float64_array cc_spec2 = res["specsets/specset/matset_values/circle_c/c_spec2"].value();
    float64_array cc_spec3 = res["specsets/specset/matset_values/circle_c/c_spec3"].value();

    const float64 nxf = static_cast<float64>(nx);
    const float64 nyf = static_cast<float64>(ny);

    for (index_t y = 0; y < ny; y ++)
    {
        const float64 yf = static_cast<float64>(y);
        for (index_t x = 0; x < nx; x ++)
        {
            const index_t elem_id = x + nx * y;
            const float64 xf = static_cast<float64>(x);

            // circle_a species vary horizontally
            float64 specvalue = xf / nxf;
            ca_spec1[elem_id] = specvalue;
            ca_spec2[elem_id] = 1.0 - specvalue;

            // circle_b species vary vertically
            specvalue = yf / nyf;
            cb_spec1[elem_id] = specvalue;
            cb_spec2[elem_id] = 1.0 - specvalue;

            // circle_c species vary diagonally
            specvalue = (yf / nyf + xf / nxf) / 2.0;
            cc_spec1[elem_id] = 1.0 - specvalue;
            cc_spec2[elem_id] = 0.75 * specvalue;
            cc_spec3[elem_id] = 0.25 * specvalue;

            // background species do not vary
            bg_spec1[elem_id] = 1.0;
        }
    }
}

void venn_sparse_by_material_specset(Node &res, index_t nx, index_t ny)
{
    auto contains = [](const Node &leaf_array, index_t value) -> bool
    {
        const index_t_accessor leaf_arr_acc = leaf_array.value();
        return leaf_arr_acc.count(value) > 0;
    };

    const Node &element_ids = res["matsets/matset/element_ids"];

    std::vector<float64> bg_spec1;
    std::vector<float64> ca_spec1;
    std::vector<float64> ca_spec2;
    std::vector<float64> cb_spec1;
    std::vector<float64> cb_spec2;
    std::vector<float64> cc_spec1;
    std::vector<float64> cc_spec2;
    std::vector<float64> cc_spec3;

    const float64 nxf = static_cast<float64>(nx);
    const float64 nyf = static_cast<float64>(ny);
    for (index_t y = 0; y < ny; y ++)
    {
        const float64 yf = static_cast<float64>(y);
        for (index_t x = 0; x < nx; x ++)
        {
            const index_t elem_id = x + nx * y;
            const float64 xf = static_cast<float64>(x);

            if (contains(element_ids["circle_a"], elem_id))
            {
                // circle_a species vary horizontally
                const float64 specvalue = xf / nxf;
                ca_spec1.push_back(specvalue);
                ca_spec2.push_back(1.0 - specvalue);
            }

            if (contains(element_ids["circle_b"], elem_id))
            {
                // circle_b species vary vertically
                const float64 specvalue = yf / nyf;
                cb_spec1.push_back(specvalue);
                cb_spec2.push_back(1.0 - specvalue);
            }

            if (contains(element_ids["circle_c"], elem_id))
            {
                // circle_c species vary diagonally
                const float64 specvalue = (yf / nyf + xf / nxf) / 2.0;
                cc_spec1.push_back(1.0 - specvalue);
                cc_spec2.push_back(0.75 * specvalue);
                cc_spec3.push_back(0.25 * specvalue);
            }

            if (contains(element_ids["background"], elem_id))
            {
                // background species do not vary
                bg_spec1.push_back(1.0);
            }
        }
    }

    res["specsets/specset/matset"] = "matset";
    res["specsets/specset/matset_values/background/bg_spec1"].set(bg_spec1);
    res["specsets/specset/matset_values/circle_a/a_spec1"].set(ca_spec1);
    res["specsets/specset/matset_values/circle_a/a_spec2"].set(ca_spec2);
    res["specsets/specset/matset_values/circle_b/b_spec1"].set(cb_spec1);
    res["specsets/specset/matset_values/circle_b/b_spec2"].set(cb_spec2);
    res["specsets/specset/matset_values/circle_c/c_spec1"].set(cc_spec1);
    res["specsets/specset/matset_values/circle_c/c_spec2"].set(cc_spec2);
    res["specsets/specset/matset_values/circle_c/c_spec3"].set(cc_spec3);
}

void venn_sparse_by_element_specset(Node &res, index_t nx, index_t ny)
{
    // get references to the matset
    const Node &matset = res["matsets/matset"];
    const index_t_accessor m_sizes = matset["sizes"].value();
    const index_t_accessor m_offsets = matset["offsets"].value();
    const index_t_accessor m_material_ids = matset["material_ids"].value();

    // create reverse material map
    std::map<int, std::string> reverse_matmap;
    auto matmap_itr = matset["material_map"].children();
    while (matmap_itr.has_next())
    {
        const Node &matmap_entry = matmap_itr.next();
        const std::string matname = matmap_itr.name();

        reverse_matmap[matmap_entry.to_int()] = matname;
    }

    // create species_names
    res["specsets/specset/matset"] = "matset";
    res["specsets/specset/species_names/background/bg_spec1"];
    res["specsets/specset/species_names/circle_a/a_spec1"];
    res["specsets/specset/species_names/circle_a/a_spec2"];
    res["specsets/specset/species_names/circle_b/b_spec1"];
    res["specsets/specset/species_names/circle_b/b_spec2"];
    res["specsets/specset/species_names/circle_c/c_spec1"];
    res["specsets/specset/species_names/circle_c/c_spec2"];
    res["specsets/specset/species_names/circle_c/c_spec3"];

    std::vector<float64> matset_values;
    std::vector<index_t> sizes;
    std::vector<index_t> offsets;

    const float64 nxf = static_cast<float64>(nx);
    const float64 nyf = static_cast<float64>(ny);
    for (index_t y = 0; y < ny; y ++)
    {
        const float64 yf = static_cast<float64>(y);
        for (index_t x = 0; x < nx; x ++)
        {
            const index_t elem_id = x + nx * y;
            const float64 xf = static_cast<float64>(x);

            const index_t zone_size = m_sizes[elem_id];
            const index_t zone_offset = m_offsets[elem_id];

            index_t num_specs_in_zone = 0;
            for (index_t local_offset = 0; local_offset < zone_size; local_offset ++)
            {
                const index_t mat_id = m_material_ids[zone_offset + local_offset];
                const std::string matname = reverse_matmap.at(mat_id);

                if ("circle_a" == matname)
                {
                    // circle_a species vary horizontally
                    const float64 specvalue = xf / nxf;
                    matset_values.push_back(specvalue);
                    matset_values.push_back(1.0 - specvalue);
                    num_specs_in_zone += 2;
                }
                else if ("circle_b" == matname)
                {
                    // circle_b species vary vertically
                    const float64 specvalue = yf / nyf;
                    matset_values.push_back(specvalue);
                    matset_values.push_back(1.0 - specvalue);
                    num_specs_in_zone += 2;
                }
                else if ("circle_c" == matname)
                {
                    // circle_c species vary diagonally
                    const float64 specvalue = (yf / nyf + xf / nxf) / 2.0;
                    matset_values.push_back(1.0 - specvalue);
                    matset_values.push_back(0.75 * specvalue);
                    matset_values.push_back(0.25 * specvalue);
                    num_specs_in_zone += 3;
                }
                else // ("background" == matname)
                {
                    // background species do not vary
                    matset_values.push_back(1.0);
                    num_specs_in_zone += 1;
                }
            }

            sizes.push_back(num_specs_in_zone);
        }
    }

    offsets.push_back(0);
    for (index_t elem_id = 1; elem_id < nx * ny; elem_id ++)
    {
        offsets.push_back(offsets[elem_id - 1] + sizes[elem_id - 1]);
    }

    res["specsets/specset/matset_values"].set(matset_values);
    res["specsets/specset/sizes"].set(sizes);
    res["specsets/specset/offsets"].set(offsets);
}

//-----------------------------------------------------------------------------
void venn_specsets(const std::string &matset_type,
                   index_t nx,
                   index_t ny,
                   float64 radius, 
                   Node &res)
{
    venn(matset_type, nx, ny, radius, res);

    if (matset_type == "full")
    {
        venn_full_specset(res, nx, ny);
    }
    else if (matset_type == "sparse_by_material")
    {
        venn_sparse_by_material_specset(res, nx, ny);
    }
    else if (matset_type == "sparse_by_element")
    {
        venn_sparse_by_element_specset(res, nx, ny);
    }
    else
    {
        CONDUIT_ERROR("unknown matset_type = " << matset_type);
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
