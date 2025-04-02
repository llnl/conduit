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
#include "conduit_blueprint_mpi_mesh.hpp"
#include "conduit_relay_mpi.hpp"
#include "conduit_blueprint_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"


using namespace conduit;


//---------------------------------------------------------------------------//
// conduit::blueprint::mpi::mesh::verify
//---------------------------------------------------------------------------//
// doc str
const char *PyBlueprint_MPI_mesh_verify_doc_str =
"verify(node, info, protocol, comm)\n"
"\n"
"Returns True if passed node conforms to the mesh blueprint.\n"
"Populates info node with verification details.\n"
"\n"
"Arguments:\n"
"  node: input node (conduit.Node instance)\n"
"  info: node to hold verify info (conduit.Node instance)\n"
"  protocol: optional string with sub-protocol name\n"
"  comm: MPI Communicator\n";

// python func
static PyObject * 
PyBlueprint_MPI_mesh_verify(PyObject *, //self
                           PyObject *args,
                           PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    PyObject   *py_info  = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    
    static const char *kwlist[] = {"node",
                                   "info",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOn",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &py_info,
                                     &mpi_comm_id))
    {
        return (NULL);
    }
    
    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    if(!PyConduit_Node_Check(py_info))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'info' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    int rank = -1;

    try
    {
        rank = relay::mpi::rank(comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }
    

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    Node &info = *PyConduit_Node_Get_Node_Ptr(py_info);


    if(blueprint::mpi::mesh::verify(node,info,comm))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mpi::mesh::generate_index
//---------------------------------------------------------------------------//

// doc str
const char *PyBlueprint_MPI_mesh_generate_index_doc_str =
"generate_index(mesh, ref_path, dest, comm)\n"
"\n"
"Assumes mesh::verify() is True\n"
"\n"
"Generates a blueprint index for a given blueprint mesh.\n"
"\n"
"Arguments:\n"
"  mesh: input node (conduit.Node instance)\n"
"  ref_path: string with reference path to mesh root\n"
"  dest: output node (conduit.Node instance)\n"
"  comm: MPI Communicator\n";

// py func
static PyObject * 
PyBlueprint_MPI_mesh_generate_index(PyObject *, //self
                                PyObject *args,
                                PyObject *kwargs)
{

    PyObject   *py_mesh     = NULL;
    const char *ref_path    = NULL;
    PyObject   *py_dest     = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    
    static const char *kwlist[] = {"mesh",
                                   "ref_path",
                                   "dest",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OsOn",
                                     const_cast<char**>(kwlist),
                                     &py_mesh, 
                                     &ref_path,
                                     &py_dest,
                                     &mpi_comm_id))
    {
        return (NULL);
    }
    
    if(!PyConduit_Node_Check(py_mesh))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'mesh' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    if(!PyConduit_Node_Check(py_dest))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    int rank = -1;

    try
    {
        rank = relay::mpi::rank(comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }
    

    Node &mesh = *PyConduit_Node_Get_Node_Ptr(py_mesh);
    Node &dest = *PyConduit_Node_Get_Node_Ptr(py_dest);
    

    blueprint::mpi::mesh::generate_index(mesh,
                                         std::string(ref_path),
                                         dest,
                                         comm);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mpi::mesh::partition
//---------------------------------------------------------------------------//

// doc str
const char *PyBlueprint_MPI_mesh_partition_doc_str =
"partition(mesh, options, output, comm)\n"
"\n"
"Assumes mesh::verify() is True\n"
"\n"
"Partitions mesh according to options and stores result in output.\n"
"\n"
"Arguments:\n"
"  mesh: input node (conduit.Node instance)\n"
"  options: options node (conduit.Node instance)\n"
"  output: output node (conduit.Node instance)\n"
"  comm: MPI Communicator\n";

// py func
static PyObject * 
PyBlueprint_MPI_mesh_partition(PyObject *, //self
                           PyObject *args,
                           PyObject *kwargs)
{

    PyObject   *py_mesh     = NULL;
    PyObject   *py_options  = NULL;
    PyObject   *py_output   = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    
    static const char *kwlist[] = {"mesh",
                                   "options",
                                   "output",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOOn",
                                     const_cast<char**>(kwlist),
                                     &py_mesh, 
                                     &py_options,
                                     &py_output,
                                     &mpi_comm_id))
    {
        return NULL;
    }
    
    if(!PyConduit_Node_Check(py_mesh))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'mesh' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_options))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'options' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_output))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'output' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    int rank = -1;

    try
    {
        rank = relay::mpi::rank(comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }
    

    Node &mesh = *PyConduit_Node_Get_Node_Ptr(py_mesh);
    Node &options = *PyConduit_Node_Get_Node_Ptr(py_options);
    Node &output = *PyConduit_Node_Get_Node_Ptr(py_output);

    blueprint::mpi::mesh::partition(mesh,
                                    options,
                                    output,
                                    comm);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::mpi::mesh::flatten
//---------------------------------------------------------------------------//

// doc str
const char *PyBlueprint_MPI_mesh_flatten_doc_str =
"flatten(mesh, options, output, comm)\n"
"\n"
"Assumes mesh::verify() is True\n"
"\n"
"Flattens a mesh to a table and stores the result in output.\n"
"\n"
"Arguments:\n"
"  mesh: Input node, a blueprint mesh. (conduit.Node instance)\n"
"  options: Options node. (conduit.Node instance)\n"
"  output: Output node, a blueprint table. (conduit.Node instance)\n"
"  comm: MPI Communicator\n";

// py func
static PyObject *
PyBlueprint_MPI_mesh_flatten(PyObject *, //self
                         PyObject *args,
                         PyObject *kwargs)
{

    PyObject   *py_mesh     = NULL;
    PyObject   *py_options  = NULL;
    PyObject   *py_output   = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"mesh",
                                   "options",
                                   "output",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOOn",
                                     const_cast<char**>(kwlist),
                                     &py_mesh,
                                     &py_options,
                                     &py_output,
                                     &mpi_comm_id))
    {
        return NULL;
    }

    if(!PyConduit_Node_Check(py_mesh))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'mesh' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_options))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'options' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_output))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'output' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    int rank = -1;

    try
    {
        rank = relay::mpi::rank(comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    const Node &mesh = *PyConduit_Node_Get_Node_Ptr(py_mesh);
    const Node &options = *PyConduit_Node_Get_Node_Ptr(py_options);
    Node &output = *PyConduit_Node_Get_Node_Ptr(py_output);

    blueprint::mpi::mesh::flatten(mesh, options, output, comm);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef blueprint_mpi_mesh_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"verify",
     (PyCFunction)PyBlueprint_MPI_mesh_verify,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_MPI_mesh_verify_doc_str},
    {"generate_index",
     (PyCFunction)PyBlueprint_MPI_mesh_generate_index,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_MPI_mesh_generate_index_doc_str},
    {"partition",
     (PyCFunction)PyBlueprint_MPI_mesh_partition,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_MPI_mesh_partition_doc_str},
    {"flatten",
     (PyCFunction)PyBlueprint_MPI_mesh_flatten,
     METH_VARARGS | METH_KEYWORDS,
     PyBlueprint_MPI_mesh_flatten_doc_str},
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
blueprint_mpi_mesh_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
blueprint_mpi_mesh_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef blueprint_mpi_mesh_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "blueprint_mpi_mesh_python",
        NULL,
        sizeof(struct module_state),
        blueprint_mpi_mesh_python_funcs,
        NULL,
        blueprint_mpi_mesh_python_traverse,
        blueprint_mpi_mesh_python_clear,
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
CONDUIT_BLUEPRINT_PYTHON_API PyObject *PyInit_conduit_blueprint_mpi_mesh_python(void)
#else
CONDUIT_BLUEPRINT_PYTHON_API void initconduit_blueprint_mpi_mesh_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *py_module = PyModule_Create(&blueprint_mpi_mesh_python_module_def);
#else
    PyObject *py_module = Py_InitModule((char*)"conduit_blueprint_mpi_mesh_python",
                                               blueprint_mpi_mesh_python_funcs);
#endif


    if(py_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(py_module);
    
    st->error = PyErr_NewException((char*)"blueprint_mpi_mesh_python.Error",
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


#ifdef Py_LIMITED_API
    GLOBAL_MODULE = py_module;
#endif

#if defined(IS_PY3K)
    return py_module;
#endif

}

