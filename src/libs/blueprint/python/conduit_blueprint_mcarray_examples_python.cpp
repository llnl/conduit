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
// conduit::blueprint::mcarray::examples::xyz
//---------------------------------------------------------------------------//

// doc string
const char *PyBlueprint_mcarray_examples_xyz_doc_str =
"xyz(mcarray_type, npts, dest)\n"
"\n"
"Creates an xyz mcarray blueprint example.\n"
"\n"
"https://llnl-conduit.readthedocs.io/en/latest/blueprint_mcarray.html#examples\n"
"\n"
"Arguments:\n"
" mcarray_type: string description of the type of mcarray to generate\n"
"  valid mcarray_type values:\n"
"    \"interleaved\"\n"
"    \"separate\"\n"
"    \"contiguous\"\n"
"    \"interleaved_mixed\"\n"
" dest: Output node (conduit.Node instance)\n";
// python func
static PyObject * 
PyBlueprint_mcarray_examples_xyz(PyObject *, //self
                                 PyObject *args,
                                 PyObject *kwargs)
{
    const char *mcarray_type = NULL;
    Py_ssize_t  npts = 0;
    PyObject   *py_node  = NULL;
    
    static const char *kwlist[] = {"mcarray_type",
                                   "npts",
                                   "dest",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "snO",
                                     const_cast<char**>(kwlist),
                                     &mcarray_type, &npts, &py_node))
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
    
    blueprint::mcarray::examples::xyz(std::string(mcarray_type),
                                      npts,
                                      node);

    Py_RETURN_NONE;
}




//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef blueprint_mcarray_examples_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"xyz",
      _PyCFunction_CAST(PyBlueprint_mcarray_examples_xyz),
      METH_VARARGS | METH_KEYWORDS,
      PyBlueprint_mcarray_examples_xyz_doc_str},
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
blueprint_mcarray_examples_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static int 
blueprint_mcarray_examples_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef blueprint_mcarray_examples_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "blueprint_mcarray_examples_python",
        NULL,
        sizeof(struct module_state),
        blueprint_mcarray_examples_python_funcs,
        NULL,
        blueprint_mcarray_examples_python_traverse,
        blueprint_mcarray_examples_python_clear,
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
CONDUIT_BLUEPRINT_PYTHON_API PyObject *PyInit_conduit_blueprint_mcarray_examples_python(void)
#else
CONDUIT_BLUEPRINT_PYTHON_API void initconduit_blueprint_mcarray_examples_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *py_module = PyModule_Create(&blueprint_mcarray_examples_python_module_def);
#else
    PyObject *py_module = Py_InitModule((char*)"conduit_blueprint_mcarray_examples_python",
                                               blueprint_mcarray_examples_python_funcs);
#endif


    if(py_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(py_module);
    
    st->error = PyErr_NewException((char*)"blueprint_mcarray_examples_python.Error",
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

