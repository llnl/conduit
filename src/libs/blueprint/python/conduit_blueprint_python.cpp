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
// conduit::blueprint::about
//---------------------------------------------------------------------------//
// doc str
const char *PyBlueprint_about_doc_str =
"about()\n"
"\n"
"Returns node with details about as built blueprint features.\n";

// python func
static PyObject *
PyBlueprint_about()
{
    //create and return a node with the result of about
    PyObject *py_node_res = PyConduit_Node_Python_Create();
    Node *node = PyConduit_Node_Get_Node_Ptr(py_node_res);
    conduit::blueprint::about(*node);
    return (PyObject*)py_node_res;
}

//---------------------------------------------------------------------------//
// conduit::blueprint::verify
//---------------------------------------------------------------------------//
// doc str
const char *PyBlueprint_mesh_verify_doc_str =
"verify(node, info, protocol)\n"
"\n"
"Returns True if passed node conforms to a blueprint protocol.\n"
"Populates info node with verification details\n"
"\n"
"Arguments:\n"
"  protocol: input string with protocol name\n"
"  node: input node (conduit.Node instance)\n"
"  info: node to hold verify info (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_verify(PyObject *, //self
                   PyObject *args,
                   PyObject *kwargs)
{
    const char *protocol = NULL;
    PyObject   *py_node  = NULL;
    PyObject   *py_info  = NULL;

    static const char *kwlist[] = {"protocol",
                                   "node",
                                   "info",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "sOO",
                                     const_cast<char**>(kwlist),
                                     &protocol,
                                     &py_node,
                                     &py_info))
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
    
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    Node &info = *PyConduit_Node_Get_Node_Ptr(py_info);
    

    if(blueprint::verify(std::string(protocol), node,info))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}


//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef blueprint_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"about",
     (PyCFunction)PyBlueprint_about,
      METH_NOARGS,
      PyBlueprint_about_doc_str},
    {"verify",
     (PyCFunction)PyBlueprint_verify,
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mesh_verify_doc_str},
    //-----------------------------------------------------------------------//
    // end blueprint methods table
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
blueprint_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
blueprint_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef blueprint_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "blueprint_python",
        NULL,
        sizeof(struct module_state),
        blueprint_python_funcs,
        NULL,
        blueprint_python_traverse,
        blueprint_python_clear,
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
CONDUIT_BLUEPRINT_PYTHON_API PyObject * PyInit_conduit_blueprint_python(void)
#else
CONDUIT_BLUEPRINT_PYTHON_API void initconduit_blueprint_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *blueprint_module = PyModule_Create(&blueprint_python_module_def);
#else
    PyObject *blueprint_module = Py_InitModule((char*)"conduit_blueprint_python",
                                             blueprint_python_funcs);
#endif

    if(blueprint_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(blueprint_module);
    
    st->error = PyErr_NewException((char*)"conduit_blueprint_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(blueprint_module);
        PY_MODULE_INIT_RETURN_ERROR;
    }

    // setup for conduit python c api
    if(import_conduit() < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }


#if defined(IS_PY3K)
    return blueprint_module;
#endif

}

