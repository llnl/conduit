// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.


//-----------------------------------------------------------------------------
// -- Python includes (these must be included first) -- 
//-----------------------------------------------------------------------------
#include <Python.h>
#include <structmember.h>
#include "bytesobject.h"

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#endif

// use  proper strdup
#ifdef CONDUIT_PLATFORM_WINDOWS
    #define _conduit_strdup _strdup
#else
    #define _conduit_strdup strdup
#endif

//-----------------------------------------------------------------------------
// -- standard lib includes -- 
//-----------------------------------------------------------------------------
#include <iostream>
#include <vector>

//---------------------------------------------------------------------------//
// conduit includes
//---------------------------------------------------------------------------//
#include "conduit.hpp"
#include "conduit_blueprint.hpp"

#include "conduit_blueprint_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"


using namespace conduit;


//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::generate
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_generate_doc_str =
"generate(example_name, dest)\n"
"generate(example_name, dest, opts)\n"
"\n"
"Driver to generates mesh blueprint example meshes by name.\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_generate(PyObject *, //self
                                   PyObject *args,
                                   PyObject *kwargs)
{
    const char *example_name = NULL;
    PyObject   *py_node_res  = NULL;
    PyObject   *py_node_opts = NULL;

    static const char *kwlist[] = {"example_name",
                                   "dest",
                                   "opts",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                    kwargs,
                                    "sO|O",
                                    const_cast<char**>(kwlist),
                                    &example_name,
                                    &py_node_res,
                                    &py_node_opts))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node_res))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(py_node_opts != NULL && !PyConduit_Node_Check(py_node_opts))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'opts' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node_res  = *PyConduit_Node_Get_Node_Ptr(py_node_res);
    // w/o opts 
    if(py_node_opts == NULL)
    {
        blueprint::mesh::examples::generate(std::string(example_name),
                                            node_res);
    }
    else // w/ opts
    {
        Node &node_opts = *PyConduit_Node_Get_Node_Ptr(py_node_opts);
        blueprint::mesh::examples::generate(std::string(example_name),
                                            node_opts,
                                            node_res);
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::generate_default_options
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_generate_default_options_doc_str =
"generate(opts) # default options for all examples\n"
"generate(opts,example_name) # default options for specific example\n"
"\n"
"Populates default options for mesh blueprint examples generate.\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_generate_default_options(PyObject *, //self
                                                   PyObject *args,
                                                   PyObject *kwargs)
{
    const char *example_name = NULL;
    PyObject   *py_node_res  = NULL;

    static const char *kwlist[] = {"dest",
                                   "example_name",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                    kwargs,
                                    "O|s",
                                    const_cast<char**>(kwlist),
                                    &py_node_res,
                                    &example_name))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node_res))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node_res  = *PyConduit_Node_Get_Node_Ptr(py_node_res);

    if(example_name != NULL)
    {
        blueprint::mesh::examples::generate_default_options(std::string(example_name),
                                                            node_res);
    }
    else
    {
        blueprint::mesh::examples::generate_default_options(node_res);
    }
    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::basic
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_basic_doc_str =
"braid(mesh_type, nx, ny, nz, dest)\n"
"\n"
"Creates a basic mesh blueprint example.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#basic\n"
"\n"
"Arguments:\n"
" mesh_type: string description of the type of mesh to generate\n"
"  valid mesh_type values:\n"
"    \"uniform\"\n"
"    \"rectilinear\"\n"
"    \"structured\"\n"
"    \"tris\"\n"
"    \"quads\"\n"
"    \"polygons\"\n"
"    \"tets\"\n"
"    \"hexs\"\n"
"    \"wedges\"\n"
"    \"pyramids\"\n"
"    \"polyhedra\"\n"
" dest: Mesh output (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_basic(PyObject *, //self
                                PyObject *args,
                                PyObject *kwargs)
{
    const char *mesh_type = NULL;
    
    Py_ssize_t nx = 0;
    Py_ssize_t ny = 0;
    Py_ssize_t nz = 0;
    
    PyObject   *py_node  = NULL;
    
    static const char *kwlist[] = {"mesh_type",
                                   "nx",
                                   "ny",
                                   "nz",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "snnnO",
                                     const_cast<char**>(kwlist),
                                     &mesh_type,
                                     &nx,
                                     &ny,
                                     &nz,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    
    blueprint::mesh::examples::basic(std::string(mesh_type),
                                     nx,
                                     ny,
                                     nz,
                                     node);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::braid
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_braid_doc_str =
"braid(mesh_type, nx, ny, nz, dest)\n"
"\n"
"Creates a braid mesh blueprint example.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#braid\n"
"\n"
"Arguments:\n"
" mesh_type: string description of the type of mesh to generate\n"
"  valid mesh_type values:\n"
"    \"uniform\"\n"
"    \"rectilinear\"\n"
"    \"structured\"\n"
"    \"point\"\n"
"    \"lines\"\n"
"    \"tris\"\n"
"    \"quads\"\n"
"    \"tets\"\n"
"    \"hexs\"\n"
"    \"wedges\"\n"
"    \"pyramids\"\n"
" dest: Mesh output (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_braid(PyObject *, //self
                                PyObject *args,
                                PyObject *kwargs)
{
    const char *mesh_type = NULL;
    
    Py_ssize_t nx = 0;
    Py_ssize_t ny = 0;
    Py_ssize_t nz = 0;
    
    PyObject   *py_node  = NULL;
    
    static const char *kwlist[] = {"mesh_type",
                                   "nx",
                                   "ny",
                                   "nz",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "snnnO",
                                     const_cast<char**>(kwlist),
                                     &mesh_type,
                                     &nx,
                                     &ny,
                                     &nz,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    
    blueprint::mesh::examples::braid(std::string(mesh_type),
                                     nx,
                                     ny,
                                     nz,
                                     node);

    Py_RETURN_NONE;
}




//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::julia
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_julia_doc_str =
"julia(nx, ny, x_min, x_max, y_min, y_max, c_re, c_im, dest)\n"
"\n"
"Creates a julia set mesh blueprint example.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#julia\n"
"\n"
"Arguments:\n"
" nx, ny: x and y grid dimensions\n"
" x_min, x_max: x extents\n"
" y_min, y_max: y extents\n"
" c_re, c_im: real and imaginary components of c\n"
" dest: Mesh output (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_julia(PyObject *, //self
                                PyObject *args,
                                PyObject *kwargs)
{
    Py_ssize_t nx = 0;
    Py_ssize_t ny = 0;
    double     x_min = 0.0;
    double     x_max = 0.0;
    double     y_min = 0.0;
    double     y_max = 0.0;
    double     c_re = 0.0;
    double     c_im = 0.0;

    PyObject   *py_node  = NULL;
    
    static const char *kwlist[] = {"nx",
                                   "ny",
                                   "x_min",
                                   "x_max",
                                   "y_min",
                                   "y_max",
                                   "c_re",
                                   "c_im",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "nnddddddO",
                                     const_cast<char**>(kwlist),
                                     &nx,
                                     &ny,
                                     &x_min,
                                     &x_max,
                                     &y_min,
                                     &y_max,
                                     &c_re,
                                     &c_im,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    
    blueprint::mesh::examples::julia(nx,ny,
                                     x_min,x_max,
                                     y_min,y_max,
                                     c_re,c_im,
                                     node);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::spiral
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_spiral_doc_str =
"spiral(ndoms, dest)\n"
"\n"
"Creates a multi-domain mesh blueprint spiral example.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#spiral\n"
"\n"
"Arguments:\n"
" ndoms: number of domains to generate\n"
" dest: Mesh output (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_spiral(PyObject *, //self
                                 PyObject *args,
                                 PyObject *kwargs)
{
    Py_ssize_t ndoms = 0;
    PyObject   *py_node  = NULL;
    
    static const char *kwlist[] = {"ndoms",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "nO",
                                     const_cast<char**>(kwlist),
                                     &ndoms,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    
    blueprint::mesh::examples::spiral(ndoms,
                                     node);

    Py_RETURN_NONE;
}



//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::julia_nestsets_simple
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_julia_nestsets_simple_doc_str =
"julia_nestsets_simple(x_min, x_max, y_min, y_max, c_re, c_im, dest)\n"
"\n"
"Provides a basic AMR example mesh with two levels and "
"one parent/child nesting relationship.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#julia-amr-examples\n"
"\n"
"Arguments:\n"
" x_min, x_max: x extents\n"
" y_min, y_max: y extents\n"
" c_re, c_im: real and imaginary components of c\n"
" dest: Mesh output (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_julia_nestsets_simple(PyObject *, //self
                                                PyObject *args,
                                                PyObject *kwargs)
{
    double     x_min = 0.0;
    double     x_max = 0.0;
    double     y_min = 0.0;
    double     y_max = 0.0;
    double     c_re = 0.0;
    double     c_im = 0.0;

    PyObject   *py_node  = NULL;
    
    static const char *kwlist[] = {"x_min",
                                   "x_max",
                                   "y_min",
                                   "y_max",
                                   "c_re",
                                   "c_im",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "ddddddO",
                                     const_cast<char**>(kwlist),
                                     &x_min,
                                     &x_max,
                                     &y_min,
                                     &y_max,
                                     &c_re,
                                     &c_im,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    
    blueprint::mesh::examples::julia_nestsets_simple(x_min,x_max,
                                                     y_min,y_max,
                                                     c_re,c_im,
                                                     node);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::julia_nestsets_complex
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_julia_nestsets_complex_doc_str =
"julia_nestsets_complex(nx, ny, x_min, x_max, y_min, y_max, c_re, c_im, levels, dest)\n"
"\n"
"Provides a basic AMR example that refines the mesh using "
"more resolution in complex areas.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#julia-amr-examples\n"
"\n"
"Arguments:\n"
" nx, ny: x and y grid dimensions\n"
" x_min, x_max: x extents\n"
" y_min, y_max: y extents\n"
" c_re, c_im: real and imaginary components of c\n"
" levels: the number of refinement levels to use.\n"
" dest: Mesh output (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_julia_nestsets_complex(PyObject *, //self
                                                 PyObject *args,
                                                 PyObject *kwargs)
{
    Py_ssize_t nx = 0;
    Py_ssize_t ny = 0;
    double     x_min = 0.0;
    double     x_max = 0.0;
    double     y_min = 0.0;
    double     y_max = 0.0;
    double     c_re = 0.0;
    double     c_im = 0.0;
    Py_ssize_t levels = 0;

    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"nx",
                                   "ny",
                                   "x_min",
                                   "x_max",
                                   "y_min",
                                   "y_max",
                                   "c_re",
                                   "c_im",
                                   "levels",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "nnddddddnO",
                                     const_cast<char**>(kwlist),
                                     &nx,
                                     &ny,
                                     &x_min,
                                     &x_max,
                                     &y_min,
                                     &y_max,
                                     &c_re,
                                     &c_im,
                                     &levels,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    blueprint::mesh::examples::julia_nestsets_complex(nx,ny,
                                                      x_min,x_max,
                                                      y_min,y_max,
                                                      c_re,c_im,
                                                      levels,
                                                      node);

    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::venn
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_venn_doc_str =
"venn(matset_type, nx, ny, radius, dest)\n"
"\n"
"Provides a basic AMR example that refines the mesh using "
"more resolution in complex areas.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#venn\n"
"\n"
"Arguments:\n"
" matset_type: string with style of matset to generate.\n"
"               'full', 'sparse_by_material', or 'sparse_by_element'\n"
" nx, ny: x and y grid dimensions\n"
" radius: specifies the radius of the three circles.\n"
" dest: Mesh output (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_venn(PyObject *, //self
                               PyObject *args,
                               PyObject *kwargs)
{
    
    char       *matset_type = NULL;
    Py_ssize_t           nx = 0;
    Py_ssize_t           ny = 0;
    double           radius = 0.0;
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"matset_type",
                                   "nx",
                                   "ny",
                                   "radius",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "snndO",
                                     const_cast<char**>(kwlist),
                                     &matset_type,
                                     &nx,
                                     &ny,
                                     &radius,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    blueprint::mesh::examples::venn(std::string(matset_type),
                                    nx,ny,
                                    radius,
                                    node);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::polytess
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_polytess_doc_str =
"polytess(nlevels, nz, dest)\n"
"\n"
"Generates a mesh of a polygonal tessellation in the 2D plane comprised of "
"octagons and squares\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#polytess\n"
"\n"
"Arguments:\n"
" nlevels: specifies the number of tessellation levels/layers to generate. If this value is specified as 1 or less, only the central tessellation level (i.e. the octagon in the center of the geometry) will be generated in the result.\n"
" nz: if 1, create 2D tessellation\n"
"      if greater than 1, stack to create a 3D tessellation.\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_polytess(PyObject *, //self
                                   PyObject *args,
                                   PyObject *kwargs)
{
    Py_ssize_t nlevels = 0;
    Py_ssize_t nz = 0;
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"nlevels",
                                   "nz",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "nnO",
                                     const_cast<char**>(kwlist),
                                     &nlevels,
                                     &nz,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    blueprint::mesh::examples::polytess(nlevels,
                                        nz,
                                        node);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::polychain
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_polychain_doc_str =
"polychain(length, dest)\n"
"\n"
"Generates a chain of cubes and triangular prisms that extends diagonally.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#polychain\n"
"\n"
"Arguments:\n"
" length: specifies how long of a chain to generate\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_polychain(PyObject *, //self
                                   PyObject *args,
                                   PyObject *kwargs)
{
    Py_ssize_t length = 0;
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"length",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "nO",
                                     const_cast<char**>(kwlist),
                                     &length,
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    blueprint::mesh::examples::polychain(length,
                                         node);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::polystar
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_polystar_doc_str =
"polystar(dest)\n"
"\n"
"Generates a mesh with a polyhedral star pattern that demonstrates "
"hanging vertices in a topology. Includes various fields that "
"count element to vertex and vertex to element relationships.\n"
"Also generates corner and side meshes from the main topology.\n";

// python func
static PyObject * 
PyBlueprint_mesh_examples_polystar(PyObject *, //self
                                   PyObject *args,
                                   PyObject *kwargs)
{
    PyObject *py_node  = NULL;

    static const char *kwlist[] = {"dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "O",
                                     const_cast<char**>(kwlist),
                                     &py_node))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    blueprint::mesh::examples::polystar(node);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mesh::examples::strided_structured
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mesh_examples_strided_structured_doc_str =
"strided_structured(desc,npts_x,npts_y,npts_z,dest)\n"
"\n"
"Generates a strided structured grid with an element field and a vertex "
"field, each element of which contains a sequentially increasing value. "
"Calling code can specify the shape of the storage array for the fields "
"and the location of the field values within the array.\n"
"\n"
"Pass the extra specifications with a conduit Node desc:\n"
"\n"
"vertex_data:\n"
"  shape: [vx, vy, vz]\n"
"  origin: [wx, wy, wz]\n"
"element_data:\n"
"   shape: [ex, ey, ez]\n"
"   origin: [fx, fy, fz]\n";


// python func
static PyObject * 
PyBlueprint_mesh_examples_strided_structured(PyObject *, //self
                                             PyObject *args,
                                             PyObject *kwargs)
{

  // Wraps:
  // void CONDUIT_BLUEPRINT_API strided_structured(conduit::Node &desc,
  //                                               conduit::index_t npts_x,
  //                                               conduit::index_t npts_y,
  //                                               conduit::index_t npts_z,
  //                                               conduit::Node &res);

    PyObject   *py_node_desc = NULL;
    Py_ssize_t  npts_x       = 0;
    Py_ssize_t  npts_y       = 0;
    Py_ssize_t  npts_z       = 0;
    PyObject   *py_node_dest = NULL;

    static const char *kwlist[] = {"desc",
                                   "npts_x",
                                   "npts_y",
                                   "npts_z",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OnnnO",
                                     const_cast<char**>(kwlist),
                                     &py_node_desc,
                                     &npts_x,
                                     &npts_y,
                                     &npts_z,
                                     &py_node_dest))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_node_desc))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'desc' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_node_dest))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &node_desc = *PyConduit_Node_Get_Node_Ptr(py_node_desc);
    Node &node_dest = *PyConduit_Node_Get_Node_Ptr(py_node_dest);

    blueprint::mesh::examples::strided_structured(node_desc,
                                                  npts_x,
                                                  npts_y,
                                                  npts_z,
                                                  node_dest);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef blueprint_mesh_examples_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"generate",
     (PyCFunction)PyBlueprint_mesh_examples_generate,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_generate_doc_str},
    //-----------------------------------------------------------------------//
    {"generate_default_options",
     (PyCFunction)PyBlueprint_mesh_examples_generate_default_options,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_generate_default_options_doc_str},
    //-----------------------------------------------------------------------//
    {"basic",
     (PyCFunction)PyBlueprint_mesh_examples_basic,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_basic_doc_str},
    //-----------------------------------------------------------------------//
    {"braid",
     (PyCFunction)PyBlueprint_mesh_examples_braid,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_braid_doc_str},
    //-----------------------------------------------------------------------//
    {"julia",
     (PyCFunction)PyBlueprint_mesh_examples_julia,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_julia_doc_str},
    //-----------------------------------------------------------------------//
    {"spiral",
     (PyCFunction)PyBlueprint_mesh_examples_spiral,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_spiral_doc_str},
    //-----------------------------------------------------------------------//
    {"julia_nestsets_simple",
     (PyCFunction)PyBlueprint_mesh_examples_julia_nestsets_simple,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_julia_nestsets_simple_doc_str},
    //-----------------------------------------------------------------------//
    {"julia_nestsets_complex",
     (PyCFunction)PyBlueprint_mesh_examples_julia_nestsets_complex,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_julia_nestsets_complex_doc_str},
    //-----------------------------------------------------------------------//
    {"venn",
     (PyCFunction)PyBlueprint_mesh_examples_venn,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_venn_doc_str},
    //-----------------------------------------------------------------------//
    {"polytess",
     (PyCFunction)PyBlueprint_mesh_examples_polytess,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_polytess_doc_str},
    //-----------------------------------------------------------------------//
    {"polychain",
     (PyCFunction)PyBlueprint_mesh_examples_polychain,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_polychain_doc_str},
    //-----------------------------------------------------------------------//
    {"polystar",
     (PyCFunction)PyBlueprint_mesh_examples_polystar,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_polystar_doc_str},
    //-----------------------------------------------------------------------//
    {"strided_structured",
     (PyCFunction)PyBlueprint_mesh_examples_strided_structured,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_examples_strided_structured_doc_str},
    //-----------------------------------------------------------------------//
    // end methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, METH_VARARGS, NULL}
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// Module Init Code
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct module_state {
    PyObject *error;
};

//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Extra Module Setup Logic for Python3
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
//---------------------------------------------------------------------------//
static int
blueprint_mesh_examples_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
blueprint_mesh_examples_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef blueprint_mesh_examples_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "blueprint_mesh_examples_python",
        NULL,
        sizeof(struct module_state),
        blueprint_mesh_examples_python_funcs,
        NULL,
        blueprint_mesh_examples_python_traverse,
        blueprint_mesh_examples_python_clear,
        NULL
};


#endif

//---------------------------------------------------------------------------//
// The module init function signature is different between py2 and py3
// This macro simplifies the process of returning when an init error occurs.
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
#define PY_MODULE_INIT_RETURN_ERROR return NULL
#else
#define PY_MODULE_INIT_RETURN_ERROR return
#endif
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
// Main entry point
//---------------------------------------------------------------------------//
extern "C" 
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
CONDUIT_BLUEPRINT_PYTHON_API PyObject *PyInit_conduit_blueprint_mesh_examples_python(void)
#else
CONDUIT_BLUEPRINT_PYTHON_API void initconduit_blueprint_mesh_examples_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *py_module = PyModule_Create(&blueprint_mesh_examples_python_module_def);
#else
    PyObject *py_module = Py_InitModule((char*)"conduit_blueprint_mesh_examples_python",
                                               blueprint_mesh_examples_python_funcs);
#endif


    if(py_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(py_module);
    
    st->error = PyErr_NewException((char*)"blueprint_mesh_examples_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(py_module);
        PY_MODULE_INIT_RETURN_ERROR;
    }

    // setup for conduit python c api
    if(import_conduit() < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }


#if defined(IS_PY3K)
    return py_module;
#endif

}

