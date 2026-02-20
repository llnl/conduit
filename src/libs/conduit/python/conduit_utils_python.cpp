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
#include "conduit_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"

using namespace conduit;

//---------------------------------------------------------------------------//
// conduit::utils::format
//---------------------------------------------------------------------------//
// doc str
const char *PyConduit_utils_format_doc_str =
"format(pattern (string), args (Node))\n"
"format(pattern (string), maps (Node), map_index (integer))\n"
"\n";
// python func
static PyObject * 
PyConduit_Utils_format(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    const char *pattern  = NULL;
    PyObject   *py_args  = NULL;
    PyObject   *py_maps  = NULL;
    Py_ssize_t map_index = 0;

    static const char *kwlist[] = {"pattern",
                                   "args",
                                   "maps",
                                   "map_index",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s|OOn",
                                     const_cast<char**>(kwlist),
                                     &pattern,
                                     &py_args,
                                     &py_maps,
                                     &map_index))
    {
        return (NULL);
    }

    if( (py_args == NULL && py_maps == NULL) )
    {
        PyErr_SetString(PyExc_TypeError,
                        "Missing 'args' or 'maps'.\n"
                        "conduit.utils.format requires:\n"
                        " 'args'\n"
                        "  <OR>\n"
                        "'maps' and 'map_index' arguments");
        return NULL;
    }

    if( (py_args != NULL && py_maps != NULL) )
    {
        PyErr_SetString(PyExc_TypeError,
                        "Both 'args' and 'maps' passed.\n"
                        "conduit.utils.format requires:\n"
                        " 'args'\n"
                        "  <OR>\n"
                        "'maps' and 'map_index' arguments");
        return NULL;
    }


    if( py_args != NULL &&  !PyConduit_Node_Check(py_args))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'args' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if( py_maps != NULL &&  !PyConduit_Node_Check(py_maps))
    {
        PyErr_SetString(PyExc_TypeError,
                        "maps' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }


    std::string res = "";;
    try
    {
        // args case
        if(py_args != NULL)
        {
            Node &args_node = *PyConduit_Node_Get_Node_Ptr(py_args);
            res = conduit::utils::format(std::string(pattern),
                                         args_node);
        }
        else // maps case
        {
            Node &maps_node = *PyConduit_Node_Get_Node_Ptr(py_maps);
            res = conduit::utils::format(std::string(pattern),
                                         maps_node,
                                         (index_t)map_index);
        }
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", res.c_str()));
}


//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef conduit_utils_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"format",
      _PyCFunction_CAST(PyConduit_Utils_format),
      METH_VARARGS | METH_KEYWORDS,
      PyConduit_utils_format_doc_str},
    //-----------------------------------------------------------------------//
    // end realy methods table
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
conduit_utils_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
conduit_utils_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef conduit_utils_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "conduit_utils_python",
        NULL,
        sizeof(struct module_state),
        conduit_utils_python_funcs,
        NULL,
        conduit_utils_python_traverse,
        conduit_utils_python_clear,
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
CONDUIT_PYTHON_API PyObject * PyInit_conduit_utils_python(void)
#else
CONDUIT_PYTHON_API void initconduit_utils_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *utils_module = PyModule_Create(&conduit_utils_python_module_def);
#else
    PyObject *utils_module = Py_InitModule((char*)"conduit_utils_python",
                                             conduit_utils_python_funcs);
#endif

    if(utils_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(utils_module);
    
    st->error = PyErr_NewException((char*)"conduit_utils_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(utils_module);
        PY_MODULE_INIT_RETURN_ERROR;
    }

    // setup for conduit python c api
    if(import_conduit() < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }


#if defined(IS_PY3K)
    return utils_module;
#endif

}

