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
#include "conduit_relay_mpi.hpp"
#include "conduit_relay_mpi_io.hpp"
#include "conduit_relay_mpi_io_blueprint.hpp"

#include "conduit_relay_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"

using namespace conduit;
using namespace conduit::relay::mpi::io;

//---------------------------------------------------------------------------//
// conduit::relay::mpi::io::blueprint::write_mesh
//---------------------------------------------------------------------------//
// append semantics
static PyObject * 
PyRelay_mpi_io_blueprint_write_mesh(PyObject *, //self
                                PyObject *args,
                                PyObject *kwargs)
{
    PyObject   *py_node    = NULL;
    const char *path       = NULL;
    const char *protocol   = NULL;
    PyObject   *py_options = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "path",
                                   "comm",
                                   "protocol",
                                   "options",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Osn|sO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
                                     &mpi_comm_id,
                                     &protocol,
                                     &py_options))
    {
        return (NULL);
    }
    
    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::write_mesh "
                        "'node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if( (py_options != NULL) && !PyConduit_Node_Check(py_options) )
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::write_mesh "
                        "'options' argument must "
                        "be a conduit.Node");
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

    Node opts;
    Node *opts_ptr = &opts;

    if(py_options != NULL)
    {
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_options);
    }

    // default protocol string is empty which auto detects
    std::string protocol_str("");

    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    try
    {
        relay::mpi::io::blueprint::write_mesh(node,
                                         std::string(path),
                                         protocol_str,
                                         *opts_ptr,
                                         comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::relay::mpi::io::blueprint::save_mesh
//---------------------------------------------------------------------------//
// truncate semantics
static PyObject * 
PyRelay_mpi_io_blueprint_save_mesh(PyObject *, //self
                               PyObject *args,
                               PyObject *kwargs)
{
    PyObject   *py_node    = NULL;
    const char *path       = NULL;
    const char *protocol   = NULL;
    PyObject   *py_options = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "path",
                                   "comm",
                                   "protocol",
                                   "options",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Osn|sO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
                                     &mpi_comm_id,
                                     &protocol,
                                     &py_options))
    {
        return (NULL);
    }
    
    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::write_mesh "
                        "'node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if( (py_options != NULL) && !PyConduit_Node_Check(py_options) )
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::write_mesh "
                        "'options' argument must "
                        "be a conduit.Node");
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

    Node opts;
    Node *opts_ptr = &opts;

    if(py_options != NULL)
    {
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_options);
    }

    // default protocol string is empty which auto detects
    std::string protocol_str("");

    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    try
    {
        relay::mpi::io::blueprint::save_mesh(node,
                                        std::string(path),
                                        protocol_str,
                                        *opts_ptr,
                                        comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}



//---------------------------------------------------------------------------//
// conduit::relay::mpi::io::blueprint::read_mesh
//---------------------------------------------------------------------------//

static PyObject * 
PyRelay_mpi_io_blueprint_read_mesh(PyObject *, //self
                               PyObject *args,
                               PyObject *kwargs)
{
    PyObject   *py_node    = NULL;
    const char *path       = NULL;
    PyObject   *py_options = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    
    static const char *kwlist[] = {"node",
                                   "path",
                                   "comm",
                                   "options",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Osn|O",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
                                     &mpi_comm_id,
                                     &py_options))
    {
        return (NULL);
    }
    
    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::read_mesh "
                        "'node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if( (py_options != NULL) && !PyConduit_Node_Check(py_options) )
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::read_mesh "
                        "'options' argument must "
                        "be a conduit.Node");
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

    Node opts;
    Node *opts_ptr = &opts;

    if(py_options != NULL)
    {
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_options);
    }


    try
    {
        relay::mpi::io::blueprint::read_mesh(std::string(path),
                                        *opts_ptr,
                                        node,
                                        comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// conduit::relay::mpi::io::blueprint::load_mesh
//---------------------------------------------------------------------------//

static PyObject * 
PyRelay_mpi_io_blueprint_load_mesh(PyObject *, //self
                               PyObject *args,
                               PyObject *kwargs)
{
    PyObject   *py_node    = NULL;
    const char *path       = NULL;
    PyObject   *py_options = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    
    static const char *kwlist[] = {"node",
                                   "path",
                                   "comm",
                                   "options",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Osn|O",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
                                     &mpi_comm_id,
                                     &py_options))
    {
        return (NULL);
    }
    
    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::read_mesh "
                        "'node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if( (py_options != NULL) && !PyConduit_Node_Check(py_options) )
    {
        PyErr_SetString(PyExc_TypeError,
                        "relay::mpi::io::blueprint::read_mesh "
                        "'options' argument must "
                        "be a conduit.Node");
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

    Node opts;
    Node *opts_ptr = &opts;

    if(py_options != NULL)
    {
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_options);
    }


    try
    {
        relay::mpi::io::blueprint::load_mesh(std::string(path),
                                        *opts_ptr,
                                        node,
                                        comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef relay_mpi_io_blueprint_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    //-----------------------------------------------------------------------//
    {"write_mesh",
     (PyCFunction)PyRelay_mpi_io_blueprint_write_mesh,
      METH_VARARGS | METH_KEYWORDS,
      "Write blueprint mesh to files using 'write' (append) semantics"},
    //-----------------------------------------------------------------------//
    {"save_mesh",
     (PyCFunction)PyRelay_mpi_io_blueprint_save_mesh,
      METH_VARARGS | METH_KEYWORDS,
      "Write blueprint mesh to files using 'save' (truncate) semantics"},
    {"read_mesh",
     (PyCFunction)PyRelay_mpi_io_blueprint_read_mesh,
      METH_VARARGS | METH_KEYWORDS,
      "Read blueprint mesh from files into passed node"},
    {"load_mesh",
     (PyCFunction)PyRelay_mpi_io_blueprint_load_mesh,
      METH_VARARGS | METH_KEYWORDS,
      "Reset passed node and load blueprint mesh from files into it"},
    //-----------------------------------------------------------------------//
    // end relay io blueprint methods table
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

#ifdef Py_LIMITED_API
// A pointer to the initialized module.
PyObject* GLOBAL_MODULE = NULL;
#endif

//---------------------------------------------------------------------------//
// Extra Module Setup Logic for Python3
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
//---------------------------------------------------------------------------//
static int
relay_mpi_io_blueprint_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
relay_mpi_io_blueprint_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef relay_mpi_io_blueprint_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "relay_mpi_io_blueprint_python",
        NULL,
        sizeof(struct module_state),
        relay_mpi_io_blueprint_python_funcs,
        NULL,
        relay_mpi_io_blueprint_python_traverse,
        relay_mpi_io_blueprint_python_clear,
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
CONDUIT_RELAY_PYTHON_API PyObject * PyInit_conduit_relay_mpi_io_blueprint_python(void)
#else
CONDUIT_RELAY_PYTHON_API void initconduit_relay_mpi_io_blueprint_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *res_mod = PyModule_Create(&relay_mpi_io_blueprint_python_module_def);
#else
    PyObject *res_mod = Py_InitModule((char*)"conduit_relay_mpi_io_blueprint_python",
                                      relay_mpi_io_blueprint_python_funcs);
#endif


    if(res_mod == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(res_mod);
    
    st->error = PyErr_NewException((char*)"relay_mpi_io_blueprint_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(res_mod);
        PY_MODULE_INIT_RETURN_ERROR;
    }

    // setup for conduit python c api
    if(import_conduit() < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

#ifdef Py_LIMITED_API
    GLOBAL_MODULE = res_mod;
#endif

#if defined(IS_PY3K)
    return res_mod;
#endif

}

