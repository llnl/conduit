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
#include "conduit_blueprint_mpi_mesh_examples.hpp"
#include "conduit_relay_mpi.hpp"
#include "conduit_blueprint_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"


using namespace conduit;

//---------------------------------------------------------------------------//
// conduit::blueprint::mpi::mesh::examples::braid_uniform_multi_domain
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mpi_mesh_examples_braid_uniform_multi_domain_doc_str =
"braid_uniform_multi_domain(dest, comm)\n"
"\n"
"Generates a uniform grid per MPI task using blueprint::mesh::examples::braid.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#braid\n"
"\n"
"Adds an element-associated scalar field painted with the domain id.\n"
"\n"
"Arguments:\n"
"  dest: Mesh output (conduit.Node instance)\n"
"  comm: MPI Communicator\n";

// python func
static PyObject * 
PyBlueprint_mpi_mesh_examples_braid_uniform_multi_domain(PyObject *, //self
                                PyObject *args,
                                PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    
    static const char *kwlist[] = {"dest",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "On",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &mpi_comm_id))
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
    
    blueprint::mpi::mesh::examples::braid_uniform_multi_domain(node, comm);

    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
// conduit::blueprint::mpi::mesh::examples::spiral_round_robin
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mpi_mesh_examples_spiral_round_robin_doc_str =
"spiral(ndoms, dest)\n"
"\n"
"Generates a multi-domain fibonacci estimation of a golden spiral.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#spiral\n"
"\n"
"Domains are assigned round-robin to MPI tasks.\n"
"\n"
"Arguments:\n"
"  ndoms: number of domains to generate\n"
"  dest: Mesh output (conduit.Node instance)\n"
"  comm: MPI Communicator\n";

// python func
static PyObject * 
PyBlueprint_mpi_mesh_examples_spiral_round_robin(PyObject *, //self
                                 PyObject *args,
                                 PyObject *kwargs)
{
    Py_ssize_t ndoms = 0;
    PyObject   *py_node  = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    
    static const char *kwlist[] = {"ndoms",
                                   "dest",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "nOn",
                                     const_cast<char**>(kwlist),
                                     &ndoms,
                                     &py_node,
                                     &mpi_comm_id))
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
    
    blueprint::mpi::mesh::examples::spiral_round_robin(ndoms,
                                                       node,
                                                       comm);

    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef blueprint_mpi_mesh_examples_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"braid_uniform_multi_domain",
     (PyCFunction)PyBlueprint_mpi_mesh_examples_braid_uniform_multi_domain,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mpi_mesh_examples_braid_uniform_multi_domain_doc_str},
    //-----------------------------------------------------------------------//
    {"spiral_round_robin",
     (PyCFunction)PyBlueprint_mpi_mesh_examples_spiral_round_robin,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mpi_mesh_examples_spiral_round_robin_doc_str},
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
blueprint_mpi_mesh_examples_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
blueprint_mpi_mesh_examples_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef blueprint_mpi_mesh_examples_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "blueprint_mpi_mesh_examples_python",
        NULL,
        sizeof(struct module_state),
        blueprint_mpi_mesh_examples_python_funcs,
        NULL,
        blueprint_mpi_mesh_examples_python_traverse,
        blueprint_mpi_mesh_examples_python_clear,
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
CONDUIT_BLUEPRINT_PYTHON_API PyObject *PyInit_conduit_blueprint_mpi_mesh_examples_python(void)
#else
CONDUIT_BLUEPRINT_PYTHON_API void initconduit_blueprint_mpi_mesh_examples_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *py_module = PyModule_Create(&blueprint_mpi_mesh_examples_python_module_def);
#else
    PyObject *py_module = Py_InitModule((char*)"conduit_blueprint_mpi_mesh_examples_python",
                                               blueprint_mpi_mesh_examples_python_funcs);
#endif


    if(py_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(py_module);
    
    st->error = PyErr_NewException((char*)"blueprint_mpi_mesh_examples_python.Error",
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

