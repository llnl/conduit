// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.


//-----------------------------------------------------------------------------
// -- Python includes (these must be included first) -- 
//-----------------------------------------------------------------------------
#include <Python.h>
#include <structmember.h>
#include "bytesobject.h"


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

#include "conduit_relay_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"


using namespace conduit;
using namespace conduit::relay::mpi::io;

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
// PyVarObject_TAIL is used at the end of each PyVarObject def
// to make sure we have the correct number of initializers across python
// versions.
//-----------------------------------------------------------------------------


#ifdef Py_TPFLAGS_HAVE_FINALIZE
    // python 3.8 adds tp_vectorcall, at end and special slot for tp_print
    // python 3.9 removes tp_print special slot
    #if PY_VERSION_HEX >= 0x03080000
        #if PY_VERSION_HEX < 0x03090000
             // python 3.8 tail
            #define PyVarObject_TAIL ,0, 0, 0 
        #else
            // python 3.9 and newer tail
            #define PyVarObject_TAIL ,0, 0
        #endif
    #else
        // python tail when finalize is part of struct
        #define PyVarObject_TAIL ,0
    #endif
#else
// python tail when finalize is not part of struct
#define PyVarObject_TAIL
#endif

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

static inline module_state *
get_module_state()
{
    void *state = PyModule_GetState(GLOBAL_MODULE);
    assert(state != NULL);
    return (module_state *)state;
}

static inline module_state *
get_state_from_type(PyTypeObject *tp)
{
    void *state = PyType_GetModuleState(tp);
    assert(state != NULL);
    return (module_state*)state;
}
#endif


#ifdef Py_LIMITED_API

#define Set_PyTypeObject_Macro(type,NAME)                \
module_state* state = get_module_state();  \
assert(state != NULL);                                   \
type = state->NAME
#else
#define Set_PyTypeObject_Macro(type,NAME) type = (PyTypeObject*)&NAME
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Begin Functions to help with Python 2/3 Compatibility.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


#if defined(IS_PY3K)
//-----------------------------------------------------------------------------
static PyObject *
PyString_FromString(const char *s)
{
    return PyUnicode_FromString(s);
}

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// End Functions to help with Python 2/3 Compatibility.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
// conduit::relay::mpi::io::about
//---------------------------------------------------------------------------//
static PyObject *
PyRelay_mpi_io_about(PyObject *, //self
                PyObject *args,
                PyObject *kwargs)
{
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "n",
                                     const_cast<char**>(kwlist),
                                     &mpi_comm_id))
    {
        return (NULL);
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

    //create and return a node with the result of about
    PyObject *py_node_res = PyConduit_Node_Python_Create();
    Node *node = PyConduit_Node_Get_Node_Ptr(py_node_res);
    conduit::relay::mpi::io::about(*node, comm);
    return (PyObject*)py_node_res;
}

//---------------------------------------------------------------------------//
// conduit::relay::mpi::io::save
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_mpi_io_save(PyObject *, //self
                PyObject *args,
                PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    PyObject   *py_opts  = NULL;
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
                                     &py_opts))
    {
        return (NULL);
    }
    
    if(py_node != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::save 'node' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
    }
    
    // default opts is an empty node which is ignored
    Node opts;
    Node *opts_ptr = &opts;
    if(py_opts != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::save 'options' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
        
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_opts);
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

    // default protocol string is empty which auto detects
    std::string protocol_str("");

    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    try
    {
    
        relay::mpi::io::save(node,
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
// conduit::relay::mpi::io::save_merged
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_mpi_io_save_merged(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    PyObject   *py_opts  = NULL;
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
                                     &py_opts))
    {
        return (NULL);
    }
    
    if(py_node != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::save 'node' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
    }
    
    // default opts is an empty node which is ignored
    Node opts;
    Node *opts_ptr = &opts;
    if(py_opts != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::save_merged 'options' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
        
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_opts);
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

    // default protocol string is empty which auto detects
    std::string protocol_str("");    
    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    try
    {
        relay::mpi::io::save_merged(node,
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
// conduit::relay::mpi::io::load
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_mpi_io_load(PyObject *, //self
                PyObject *args,
                PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    
    static const char *kwlist[] = {"node","path","comm","protocol", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Osn|s",
                                     const_cast<char**>(kwlist),
                                     &py_node, &path, &mpi_comm_id, &protocol))
    {
        return (NULL);
    }
    
    if(py_node != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::mpi::io::load 'node' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
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
    // default protocol string is empty which auto detects
    std::string protocol_str("");
    
    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    
    try
    {
        relay::mpi::io::load(std::string(path),
                        protocol_str,
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
// conduit::relay::mpi::io::load_merged
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_mpi_io_load_merged(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    
    static const char *kwlist[] = {"node","path","comm","protocol", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Osn|s",
                                     const_cast<char**>(kwlist),
                                     &py_node, &path, &mpi_comm_id, &protocol))
    {
        return (NULL);
    }
    
    if(py_node != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::mpi::io::load 'node' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
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
    // default protocol string is empty which auto detects
    std::string protocol_str("");
    
    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    try
    {
        relay::mpi::io::load_merged(std::string(path),
                               protocol_str,
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
static PyMethodDef relay_mpi_io_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    //-----------------------------------------------------------------------//
    {"about",
     (PyCFunction)PyRelay_mpi_io_about,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    {"save",
     (PyCFunction)PyRelay_mpi_io_save,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    {"load",
     (PyCFunction)PyRelay_mpi_io_load,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    {"save_merged",
     (PyCFunction)PyRelay_mpi_io_save_merged,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    {"load_merged",
     (PyCFunction)PyRelay_mpi_io_load_merged,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    //-----------------------------------------------------------------------//
    // end relay io methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, METH_VARARGS, NULL}
};


//---------------------------------------------------------------------------//
// Extra Module Setup Logic for Python3
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
//---------------------------------------------------------------------------//
static int
relay_mpi_io_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
relay_mpi_io_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef relay_mpi_io_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "relay_mpi_io_python",
        NULL,
        sizeof(struct module_state),
        relay_mpi_io_python_funcs,
        NULL,
        relay_mpi_io_python_traverse,
        relay_mpi_io_python_clear,
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
CONDUIT_RELAY_PYTHON_API PyObject * PyInit_conduit_relay_mpi_io_python(void)
#else
CONDUIT_RELAY_PYTHON_API void initconduit_relay_mpi_io_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *relay_mpi_io_module = PyModule_Create(&relay_mpi_io_python_module_def);
#else
    PyObject *relay_mpi_io_module = Py_InitModule((char*)"conduit_relay_mpi_io_python",
                                              relay_mpi_io_python_funcs);
#endif


    if(relay_mpi_io_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(relay_mpi_io_module);
    
    st->error = PyErr_NewException((char*)"relay_mpi_io_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(relay_mpi_io_module);
        PY_MODULE_INIT_RETURN_ERROR;
    }

    // setup for conduit python c api
    if(import_conduit() < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }


#ifdef Py_LIMITED_API
    GLOBAL_MODULE = relay_mpi_io_module;
#endif
#if defined(IS_PY3K)
    return relay_mpi_io_module;
#endif

}

