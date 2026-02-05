// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.


//-----------------------------------------------------------------------------
// -- Python includes (these must be included first) -- 
//-----------------------------------------------------------------------------
#include "conduit_python_common.h"

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
// conduit::blueprint::mcarray::verify
//---------------------------------------------------------------------------//

// doc str
const char *PyBlueprint_mcarray_verify_doc_str =
"verify(node, info, protocol)\n"
"\n"
"Returns True if passed node conforms to the mcarray blueprint.\n"
"Populates info node with verification details.\n"
"\n"
"Arguments:\n"
"  node: input node (conduit.Node instance)\n"
"  info: node to hold verify info (conduit.Node instance)\n"
"  protocol: optional string with sub-protocol name\n";
// python func
static PyObject * 
PyBlueprint_mcarray_verify(PyObject *, //self
                           PyObject *args,
                           PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    PyObject   *py_info  = NULL;
    const char *protocol = NULL;
    
    static const char *kwlist[] = {"node",
                                   "info",
                                   "protocol",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OO|s",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &py_info,
                                     &protocol))
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
    
    bool res = false;
    
    if(protocol != NULL)
    {
        res = blueprint::mcarray::verify(std::string(protocol), node,info);
    }
    else
    {
        res = blueprint::mcarray::verify(node,info);
    }

    if(res)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

//-----------------------------------------------------------------------------
/// mcarray blueprint property and transform methods
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// doc str
const char *PyBlueprint_mcarray_is_interleaved_doc_str =
"is_interleaved(node)\n"
"\n"
"Assumes mcarray::verify() is True\n"
"\n"
"Returns True if passed node is interleaved in memory.\n"
"\n"
"Arguments:\n"
"  node: input node (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mcarray_is_interleaved(PyObject *, //self
                                   PyObject *args,
                                   PyObject *kwargs)
{
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"node", NULL};

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
                        "'node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    

    if(blueprint::mcarray::is_interleaved(node))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// doc str
const char *PyBlueprint_mcarray_to_contiguous_doc_str =
"to_contiguous(node, dest)\n"
"\n"
"Assumes mcarray::verify() is True\n"
"\n"
"Converts any mcarray to a contiguous memory layout in output node dest.\n"
"\n"
"Arguments:\n"
"  node: input node (conduit.Node instance)\n"
"  dest: output node (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mcarray_to_contiguous(PyObject *, //self
                                  PyObject *args,
                                  PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    PyObject   *py_dest  = NULL;

    static const char *kwlist[] = {"node",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &py_dest))
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

    
    if(!PyConduit_Node_Check(py_dest))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    Node &dest = *PyConduit_Node_Get_Node_Ptr(py_dest);

    if(blueprint::mcarray::to_contiguous(node,dest))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}


//----------------------------------------------------------------------------
// doc str
const char *PyBlueprint_mcarray_to_interleaved_doc_str =
"to_interleaved(node, dest)\n"
"\n"
"Assumes mcarray::verify() is True\n"
"\n"
"Converts any mcarray to an interleaved memory layout in output node dest.\n"
"\n"
"Arguments:\n"
"  node: input node (conduit.Node instance)\n"
"  dest: output node (conduit.Node instance)\n";

// python func
static PyObject * 
PyBlueprint_mcarray_to_interleaved(PyObject *, //self
                                   PyObject *args,
                                   PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    PyObject   *py_dest  = NULL;

    static const char *kwlist[] = {"node",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OO",
                                     const_cast<char**>(kwlist),
                                     &py_node, &py_dest))
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
    
    if(!PyConduit_Node_Check(py_dest))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'dest' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
    Node &dest = *PyConduit_Node_Get_Node_Ptr(py_dest);

    if(blueprint::mcarray::to_interleaved(node,dest))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef blueprint_mcarray_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"verify",
      _PyCFunction_CAST(PyBlueprint_mcarray_verify),
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mcarray_verify_doc_str},
    {"is_interleaved",
      _PyCFunction_CAST(PyBlueprint_mcarray_is_interleaved),
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mcarray_is_interleaved_doc_str},
    {"to_interleaved",
      _PyCFunction_CAST(PyBlueprint_mcarray_to_interleaved),
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mcarray_to_interleaved_doc_str},
    {"to_contiguous",
      _PyCFunction_CAST(PyBlueprint_mcarray_to_contiguous),
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mcarray_to_contiguous_doc_str},

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
blueprint_mcarray_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
blueprint_mcarray_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef blueprint_mcarray_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "blueprint_mcarray_python",
        NULL,
        sizeof(struct module_state),
        blueprint_mcarray_python_funcs,
        NULL,
        blueprint_mcarray_python_traverse,
        blueprint_mcarray_python_clear,
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
CONDUIT_BLUEPRINT_PYTHON_API PyObject *PyInit_conduit_blueprint_mcarray_python(void)
#else
CONDUIT_BLUEPRINT_PYTHON_API void initconduit_blueprint_mcarray_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *py_module = PyModule_Create(&blueprint_mcarray_python_module_def);
#else
    PyObject *py_module = Py_InitModule((char*)"conduit_blueprint_mcarray_python",
                                               blueprint_mcarray_python_funcs);
#endif


    if(py_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(py_module);
    
    st->error = PyErr_NewException((char*)"blueprint_mcarray_python.Error",
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

