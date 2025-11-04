// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples_generate.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh_examples.hpp"
#include "conduit_blueprint_mesh_examples_generate.hpp"

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

//-------------------------------
// detail == helpers namespace
//-------------------------------
namespace detail
{

//---------------------------------------------------------------------------//
const Node &
fetch_object(const Node &opts,
             const std::string &example_name,
             const std::string &child_name)
{
    if(!opts.has_child(child_name) || !opts[child_name].dtype().is_object())
    {
        CONDUIT_ERROR("example: `" << example_name
                      << "` expects options to contain a `object` named `"
                      << child_name << "`");
    }
    return opts[child_name];
}

//---------------------------------------------------------------------------//
std::string
fetch_string(const Node &opts,
             const std::string &example_name,
             const std::string &child_name)
{
    if(!opts.has_child(child_name) || !opts[child_name].dtype().is_string())
    {
        CONDUIT_ERROR("example: `" << example_name
                      << "` expects options to contain a `string` named `"
                      << child_name << "`");
    }
    return opts[child_name].as_string();
}

//---------------------------------------------------------------------------//
index_t
fetch_index_t(const Node &opts,
              const std::string &example_name,
              const std::string &child_name)
{
    if(!opts.has_child(child_name) || !opts[child_name].dtype().is_number())
    {
        CONDUIT_ERROR("example: `" << example_name
                      << "` expects options to contain a `integer` named `"
                      << child_name << "`");
    }
    return opts[child_name].to_index_t();
}

//---------------------------------------------------------------------------//
index_t
fetch_index_t(const Node &opts,
              const std::string &example_name,
              const std::string &child_name,
              index_t def_value)
{
    if(!opts.has_child(child_name))
    {
        return def_value;
    }

    if(!opts.has_child(child_name) || !opts[child_name].dtype().is_number())
    {
        CONDUIT_ERROR("example: `" << example_name
                      << "` expects options to contain a `integer` named `"
                      << child_name << "`");
    }
    return opts[child_name].to_index_t();
}

//---------------------------------------------------------------------------//
float64
fetch_float64(const Node &opts,
              const std::string &example_name,
              const std::string &child_name)
{
    if(!opts.has_child(child_name) || !opts[child_name].dtype().is_number())
    {
        CONDUIT_ERROR("example: `" << example_name
                      << "` expects options to contain a `float64` named `"
                      << child_name << "`");
    }
    return opts[child_name].to_float64();
}

//---------------------------------------------------------------------------//
float64
fetch_float64(const Node &opts,
              const std::string &example_name,
              const std::string &child_name,
              float64 def_value)
{
    if(!opts.has_child(child_name))
    {
        return def_value;
    }
        
    if(!opts[child_name].dtype().is_number())
    {
        CONDUIT_ERROR("example: `" << example_name
                      << "` expects options to contain a `float64` named `"
                      << child_name << "`");
    }
    return opts[child_name].to_float64();
}

//-----------------------
}
//-----------------------
// end detail
//-----------------------

//---------------------------------------------------------------------------//
void
generate(const std::string &example_name,
         Node &res)
{
    Node opts;
    generate_default_options(example_name,opts);
    generate(example_name,opts,res);
}

//---------------------------------------------------------------------------//
void
generate(const std::string &example_name,
         const Node &opts,
         Node &res)
{
    res.reset();

    if(example_name == "braid")
    {
        std::string      mesh_type = detail::fetch_string(opts,example_name,"mesh_type");
        conduit::index_t nx        = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny        = detail::fetch_index_t(opts,example_name,"ny");
        conduit::index_t nz        = detail::fetch_index_t(opts,example_name,"nz",0); // optional

        // wrap
        // void braid(const std::string &mesh_type,
        //            conduit::index_t nx,
        //            conduit::index_t ny,
        //            conduit::index_t nz,
        //            conduit::Node &res);
        braid(mesh_type,nx,ny,nz,res);
    }
    else if(example_name == "basic")
    {
        std::string      mesh_type = detail::fetch_string(opts,example_name,"mesh_type");
        conduit::index_t nx        = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny        = detail::fetch_index_t(opts,example_name,"ny");
        conduit::index_t nz        = detail::fetch_index_t(opts,example_name,"nz",0); // optional

        // wrap
        // void basic(const std::string &mesh_type,
        //            conduit::index_t nx,
        //            conduit::index_t ny,
        //            conduit::index_t nz,
        //            conduit::Node &res);
        basic(mesh_type,nx,ny,nz,res);
    }
    else if(example_name == "strided_structured")
    {
        conduit::index_t    npts_x = detail::fetch_index_t(opts,example_name,"npts_x");
        conduit::index_t    npts_y = detail::fetch_index_t(opts,example_name,"npts_y");
        conduit::index_t    npts_z = detail::fetch_index_t(opts,example_name,"npts_z",0); // optional

        // desc is an output?
        conduit::Node desc;

        // wrap
        // void strided_structured(conduit::Node &desc,
        //                         conduit::index_t npts_x,
        //                         conduit::index_t npts_y,
        //                         conduit::index_t npts_z,
        //                         conduit::Node &res);
        strided_structured(desc,npts_x,npts_y,npts_z,res);
    }
    else if(example_name == "grid")
    {
        std::string      mesh_type = detail::fetch_string(opts,example_name,"mesh_type");
        conduit::index_t nx        = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny        = detail::fetch_index_t(opts,example_name,"ny");
        conduit::index_t nz        = detail::fetch_index_t(opts,example_name,"nz",0); // optional
        conduit::float64 dx        = detail::fetch_float64(opts,example_name,"dx");
        conduit::float64 dy        = detail::fetch_float64(opts,example_name,"dy");
        conduit::float64 dz        = detail::fetch_float64(opts,example_name,"dz",0.0); // optional?

        // wrap
        // void grid(const std::string &mesh_type,
        //           conduit::index_t nx,
        //           conduit::index_t ny,
        //           conduit::index_t nz,
        //           conduit::index_t dx,
        //           conduit::index_t dy,
        //           conduit::index_t dz,
        //           conduit::Node &res);
        grid(mesh_type,nx,ny,nz,dx,dy,dz,res);
    }
    else if(example_name == "spiral")
    {
        conduit::index_t ndomains = detail::fetch_index_t(opts,example_name,"ndomains");
        // wrap
        // void spiral(conduit::index_t ndomains,
        //             conduit::Node &res);
        spiral(ndomains,res);
    }
    else if(example_name == "polytess")
    {
        conduit::index_t nlevels = detail::fetch_index_t(opts,example_name,"nlevels");
        conduit::index_t nz = detail::fetch_index_t(opts,example_name,"nz");
        //warp 
        // void polytess(conduit::index_t nlevels,
        //               conduit::index_t nz,
        //               conduit::Node &res);
        polytess(nlevels,nz,res);
    }
    else if(example_name == "polychain")
    {
        conduit::index_t length = detail::fetch_index_t(opts,example_name,"length");

        // wrap
        // void polychain(const conduit::index_t length,
        //                conduit::Node &res);
        polychain(length,res);
    }
    else if(example_name == "misc")
    {
        std::string      mesh_type = detail::fetch_string(opts,example_name,"mesh_type");
        conduit::index_t nx        = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny        = detail::fetch_index_t(opts,example_name,"ny");
        conduit::index_t nz        = detail::fetch_index_t(opts,example_name,"nz",0); // optional

        // wrap
        // void misc(const std::string &mesh_type,
        //           conduit::index_t nx,
        //           conduit::index_t ny,
        //           conduit::index_t nz,
        //           conduit::Node &res);
        misc(mesh_type,nx,ny,nz,res);
    }
    else if(example_name == "adjset_uniform")
    {
        // wrap:
        //void adjset_uniform(conduit::Node &res);
        adjset_uniform(res);
    }
    else if(example_name == "gyre")
    {
        conduit::index_t nx = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny = detail::fetch_index_t(opts,example_name,"ny");
        conduit::index_t nz = detail::fetch_index_t(opts,example_name,"nz");
        conduit::float64 t  = detail::fetch_float64(opts,example_name,"t");

        // wrap:
        // void gyre(conduit::index_t nx,
        //           conduit::index_t ny,
        //           conduit::index_t nz,
        //           conduit::float64 t,
        //           conduit::Node &res);
        gyre(nx,ny,nz,t,res);
    }
    else if(example_name == "julia")
    {
        conduit::index_t nx    = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny    = detail::fetch_index_t(opts,example_name,"ny");
        conduit::float64 x_min = detail::fetch_float64(opts,example_name,"x_min");
        conduit::float64 x_max = detail::fetch_float64(opts,example_name,"x_max");
        conduit::float64 y_min = detail::fetch_float64(opts,example_name,"y_min");
        conduit::float64 y_max = detail::fetch_float64(opts,example_name,"y_max");
        conduit::float64 c_re  = detail::fetch_float64(opts,example_name,"c_re");
        conduit::float64 c_im  = detail::fetch_float64(opts,example_name,"c_im");

        // wrap
        // void julia(conduit::index_t nx,
        //            conduit::index_t ny,
        //            conduit::float64 x_min,
        //            conduit::float64 x_max,
        //            conduit::float64 y_min,
        //            conduit::float64 y_max,
        //            conduit::float64 c_re,
        //            conduit::float64 c_im,
        //            conduit::Node &res);
        julia(nx,ny,
              x_min, x_max,
              y_min, y_max,
              c_re, c_im,
              res);
    }
    else if(example_name == "julia_nestsets_simple")
    {
        conduit::float64 x_min = detail::fetch_float64(opts,example_name,"x_min");
        conduit::float64 x_max = detail::fetch_float64(opts,example_name,"x_max");
        conduit::float64 y_min = detail::fetch_float64(opts,example_name,"y_min");
        conduit::float64 y_max = detail::fetch_float64(opts,example_name,"y_max");
        conduit::float64 c_re  = detail::fetch_float64(opts,example_name,"c_re");
        conduit::float64 c_im  = detail::fetch_float64(opts,example_name,"c_im");

        // wrap
        // void julia_nestsets_simple(conduit::float64 x_min,
        //                            conduit::float64 x_max,
        //                            conduit::float64 y_min,
        //                            conduit::float64 y_max,
        //                            conduit::float64 c_re,
        //                            conduit::float64 c_im,
        //                            conduit::Node &res);
        julia_nestsets_simple(x_min, x_max,
                              y_min, y_max,
                              c_re, c_im,
                              res);
    }
    else if(example_name == "julia_nestsets_complex")
    {
        conduit::index_t nx     = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny     = detail::fetch_index_t(opts,example_name,"ny");
        conduit::float64 x_min  = detail::fetch_float64(opts,example_name,"x_min");
        conduit::float64 x_max  = detail::fetch_float64(opts,example_name,"x_max");
        conduit::float64 y_min  = detail::fetch_float64(opts,example_name,"y_min");
        conduit::float64 y_max  = detail::fetch_float64(opts,example_name,"y_max");
        conduit::float64 c_re   = detail::fetch_float64(opts,example_name,"c_re");
        conduit::float64 c_im   = detail::fetch_float64(opts,example_name,"c_im");
        conduit::index_t levels = detail::fetch_index_t(opts,example_name,"levels");

        // wrap
        // void julia_nestsets_complex(conduit::index_t nx,
        //                             conduit::index_t ny,
        //                             conduit::float64 x_min,
        //                             conduit::float64 x_max,
        //                             conduit::float64 y_min,
        //                             conduit::float64 y_max,
        //                             conduit::float64 c_re,
        //                             conduit::float64 c_im,
        //                             conduit::index_t levels,
        //                             conduit::Node &res);
        julia_nestsets_complex(nx,ny,
                               x_min, x_max,
                               y_min, y_max,
                               c_re, c_im,
                               levels,
                               res);
    }
    else if(example_name == "polystar")
    {
        // wrap
        // void polystar(conduit::Node &res);
        polystar(res);
    }
    else if(example_name == "related_boundary")
    {
        conduit::index_t base_ele_dims_i = detail::fetch_index_t(opts,example_name,"base_ele_dims_i");
        conduit::index_t base_ele_dims_j = detail::fetch_index_t(opts,example_name,"base_ele_dims_j");

        // wrap
        // void related_boundary(conduit::index_t base_ele_dims_i,
        //                       conduit::index_t base_ele_dims_j,
        //                       conduit::Node &res);
        related_boundary(base_ele_dims_i,base_ele_dims_j,res);
    }
    else if(example_name == "rz_cylinder")
    {
        std::string      mesh_type = detail::fetch_string(opts,example_name,"mesh_type");
        conduit::index_t nz        = detail::fetch_index_t(opts,example_name,"nz");
        conduit::index_t nr        = detail::fetch_index_t(opts,example_name,"nr");

        // wrap
        // void rz_cylinder(const std::string &mesh_type,
        //                  index_t nz,
        //                  index_t nr,
        //                  Node &res);
        rz_cylinder(mesh_type,nz,nr,res);
    }
    else if(example_name == "tiled")
    {
        conduit::index_t nx = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny = detail::fetch_index_t(opts,example_name,"ny");
        conduit::index_t nz = detail::fetch_index_t(opts,example_name,"nz");
        // pull these from ops and pass thru rest
        Node tiled_opts;
        tiled_opts.set(opts);
        tiled_opts.remove_child("nx");
        tiled_opts.remove_child("ny");
        tiled_opts.remove_child("nz");

        // warp
        // void tiled(conduit::index_t nx,
        //            conduit::index_t ny,
        //            conduit::index_t nz,
        //            conduit::Node &res,
        //            const conduit::Node &options);
        tiled(nx,ny,nz,res,tiled_opts);
    }
    else if(example_name == "venn")
    {
        std::string      matset_type = detail::fetch_string(opts,example_name,"matset_type");
        conduit::index_t nx          = detail::fetch_index_t(opts,example_name,"nx");
        conduit::index_t ny          = detail::fetch_index_t(opts,example_name,"ny");
        conduit::float64 radius      = detail::fetch_float64(opts,example_name,"radius");

        // wrap
        // void venn(const std::string &matset_type,
        //           index_t nx,
        //           index_t ny,
        //           float64 radius,
        //           Node &res);
        venn(matset_type,nx,ny,radius,res);
    }
    else if (example_name == "bent_multi_grid")
    {
        // bent_multi_grid has its own defaults generator, which gives
        // sensible defaults if passed an empty Node for the first argument.
        bent_multi_grid(opts, res);
    }
    else if (example_name == "bent_multi_grid_amr")
    {
        // bent_multi_grid_amr has its own defaults generator, which gives
        // sensible defaults if passed an empty Node for the first argument.
        bent_multi_grid_amr(opts, res);
    }
    else
    {
        // ERROR UNSUPPORTED!
        CONDUIT_ERROR("Unsupported example name: `" << example_name << "`");
    }
}
// -- end generate

//---------------------------------------------------------------------------//
void
generate_default_options(const std::string &example_name,
         Node &opts)
{
    opts.reset();

    if(example_name == "braid")
    {
        opts["mesh_type"] = "hexs";
        opts["nx"] = 10;
        opts["ny"] = 7;
        opts["nz"] = 3;
    }
    else if(example_name == "basic")
    {
        opts["mesh_type"] = "uniform";
        opts["nx"] = 10;
        opts["ny"] = 7;
        opts["nz"] = 3;
    }
    else if(example_name == "strided_structured")
    {
        opts["npts_x"] = 4;
        opts["npts_y"] = 3;
        opts["npts_z"] = 0;
    }
    else if(example_name == "grid")
    {
        opts["mesh_type"] = "uniform";
        opts["nx"] = 10;
        opts["ny"] = 7;
        opts["nz"] = 3;

        opts["dx"] = 1.0;
        opts["dy"] = 1.0;
        opts["dz"] = 1.0;
    }
    else if(example_name == "spiral")
    {
        opts["ndomains"] = 7;
    }
    else if(example_name == "polytess")
    {
        opts["nlevels"] = 5;
        opts["nz"] = 2;
    }
    else if(example_name == "polychain")
    {
        opts["length"] = 7;
    }
    else if(example_name == "misc")
    {
        opts["mesh_type"] = "specsets";
        opts["nx"] = 10;
        opts["ny"] = 7;
        opts["nz"] = 3;
    }
    else if(example_name == "adjset_uniform")
    {
        // no args
    }
    else if(example_name == "gyre")
    {
        opts["nx"] = 25;
        opts["ny"] = 25;
        opts["nz"] = 2;
        opts["t"]  = 0.0;
    }
    else if(example_name == "julia")
    {
        opts["nx"] = 500;
        opts["ny"] = 500;
        opts["x_min"] = -2.0;
        opts["x_max"] =  2.0;
        opts["y_min"] = -2.0;
        opts["y_max"] =  2.0;
        opts["c_re"] = 0.285;
        opts["c_im"] = 0.01;
    }
    else if(example_name == "julia_nestsets_simple")
    {
        opts["x_min"] = -2.0;
        opts["x_max"] =  2.0;
        opts["y_min"] = -2.0;
        opts["y_max"] =  2.0;
        opts["c_re"] = 0.285;
        opts["c_im"] = 0.01;
    }
    else if(example_name == "julia_nestsets_complex")
    {
        opts["nx"] = 20;
        opts["ny"] = 20;
        opts["x_min"] = -2.0;
        opts["x_max"] =  2.0;
        opts["y_min"] = -2.0;
        opts["y_max"] =  2.0;
        opts["c_re"] = 0.285;
        opts["c_im"] = 0.01;
        opts["levels"] = 3;
    }
    else if(example_name == "polystar")
    {
        // no args
    }
    else if(example_name == "related_boundary")
    {
        opts["base_ele_dims_i"] = 10;
        opts["base_ele_dims_j"] = 5;
    }
    else if(example_name == "rz_cylinder")
    {
        opts["mesh_type"] = "structured";
        opts["nz"] = 10;
        opts["nr"] = 15;
    }
    else if(example_name == "tiled")
    {
        opts["nx"] = 15;
        opts["ny"] = 7;
        opts["nz"] = 3;
    }
    else if(example_name == "venn")
    {
        opts["matset_type"] = "full";
        opts["nx"]          = 100;
        opts["ny"]          = 100;
        opts["radius"]      = 0.25;
    }
    else if (example_name == "bent_multi_grid")
    {
        // bent_multi_grid provides its own default args
    }
    else if (example_name == "bent_multi_grid_amr")
    {
        // bent_multi_grid_amr provides its own default args
    }
    else
    {
        // ERROR UNSUPPORTED!
        CONDUIT_ERROR("Unsupported example name: `" << example_name << "`");
    }
}
// -- end generate_default_options


//---------------------------------------------------------------------------//
void
generate_default_options(Node &opts)
{
    opts.reset();
    generate_default_options("braid",opts["braid"]);
    generate_default_options("basic",opts["basic"]);
    generate_default_options("strided_structured",opts["strided_structured"]);
    generate_default_options("grid",opts["grid"]);
    generate_default_options("spiral",opts["spiral"]);
    generate_default_options("polytess",opts["polytess"]);
    generate_default_options("polychain",opts["polychain"]);
    generate_default_options("misc",opts["misc"]);
    generate_default_options("adjset_uniform",opts["adjset_uniform"]);
    generate_default_options("gyre",opts["gyre"]);
    generate_default_options("julia",opts["julia"]);
    generate_default_options("julia_nestsets_simple",opts["julia_nestsets_simple"]);
    generate_default_options("julia_nestsets_complex",opts["julia_nestsets_complex"]);
    generate_default_options("polystar",opts["polystar"]);
    generate_default_options("related_boundary",opts["related_boundary"]);
    generate_default_options("rz_cylinder",opts["rz_cylinder"]);
    generate_default_options("tiled",opts["tiled"]);
    generate_default_options("venn",opts["venn"]);
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
