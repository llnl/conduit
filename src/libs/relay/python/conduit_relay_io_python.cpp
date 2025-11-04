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
#include "conduit_relay.hpp"

#include "conduit_relay_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"


using namespace conduit;
using namespace conduit::relay::io;

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
    #ifdef Py_LIMITED_API
    PyTypeObject* PyRelay_IOHandle_TYPE;
    #endif
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
struct PyRelay_IOHandle
{
    PyObject_HEAD
    IOHandle *handle;
};


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
// IOHandle Object 
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_IOHandle_new(PyTypeObject *type,
                     PyObject*, // args -- unused
                     PyObject*) // kwds -- unused
{
    PyRelay_IOHandle *self = (PyRelay_IOHandle*)PyType_GenericAlloc(type, 0);

    if (self)
    {
        self->handle = 0;
    }

    return ((PyObject*)self);
}

//---------------------------------------------------------------------------//
static void
PyRelay_IOHandle_dealloc(PyRelay_IOHandle *self)
{
    if(self->handle != NULL)
    {
        delete self->handle;
    }
    #ifdef Py_LIMITED_API 
    freefunc tp_free = (freefunc)PyType_GetSlot(Py_TYPE((PyObject*)self), Py_tp_free);
    tp_free((PyObject*)self);
    #else
    Py_TYPE(self)->tp_free((PyObject*)self);
    #endif
}

//---------------------------------------------------------------------------//
static int
PyRelay_IOHandle_init(PyRelay_IOHandle *self,
                     PyObject*, // args -- unused
                     PyObject*) // kwds -- unused
{
    self->handle = new IOHandle();
    return 0;
}

//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_open(PyRelay_IOHandle *self,
                      PyObject *args,
                      PyObject *kwargs)
{

    static const char *kwlist[] = {"path",
                                   "protocol",
                                   "options",
                                   NULL};

    char     *path       = NULL;
    char     *protocol   = NULL;
    PyObject *py_options = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s|sO",
                                     const_cast<char**>(kwlist),
                                     &path,
                                     &protocol,
                                     &py_options))
    {
        return NULL;
    }
    
    
    if( (py_options != NULL) && !PyConduit_Node_Check(py_options) )
    {
        PyErr_SetString(PyExc_TypeError,
                        "IOHandle.open 'options' argument must "
                        "be a conduit.Node");
        return NULL;
    }

    Node opts;
    Node *opts_ptr = &opts;

    if(py_options != NULL)
    {
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_options);
    }
    
    std::string protocol_str;

    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    
    try
    {
        self->handle->open(std::string(path),
                           protocol_str,
                           *opts_ptr);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE; 
}

//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_is_open(PyRelay_IOHandle *self)
{
    //Note: is_open does not throw
    bool res = self->handle->is_open();

    if(res)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;

}


//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_read(PyRelay_IOHandle *self,
                       PyObject *args,
                       PyObject *kwargs)
{

    static const char *kwlist[] = {"node",
                                   "path",
                                   "options",
                                   NULL};

    PyObject *py_node = NULL;
    char     *path    = NULL;
    PyObject *py_opts = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "O|sO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
                                     &py_opts))
    {
        return NULL;
    }
    
    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "IOHandle.read 'node' argument must "
                        "be a conduit.Node");
        return NULL;
    }

    Node opts;
    Node *opts_ptr = &opts;

    if(py_opts!= NULL)
    {
        if(!PyConduit_Node_Check(py_opts))
        {
            PyErr_SetString(PyExc_TypeError,
                            "IOHandle.read 'options' argument must "
                                "be a conduit.Node");
                            return NULL;
        }

        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_opts);
    }

    Node *node_ptr = PyConduit_Node_Get_Node_Ptr(py_node);
    
    try
    {
        if(path == NULL)
        {
            self->handle->read(*node_ptr,
                               *opts_ptr);
        }
        else
        {
            self->handle->read(std::string(path),
                               *node_ptr,
                               *opts_ptr);
        }
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE; 
}

//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_write(PyRelay_IOHandle *self,
                       PyObject *args,
                       PyObject *kwargs)
{

    static const char *kwlist[] = {"node",
                                   "path",
                                   "options",
                                   NULL};

    PyObject *py_node = NULL;
    char     *path    = NULL;
    PyObject *py_opts = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "O|sO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
                                     &py_opts))
    {
        return NULL;
    }
    
    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "IOHandle.write 'node' argument must "
                        "be a conduit.Node");
        return NULL;
    }

    Node opts;
    Node *opts_ptr = &opts;

    if(py_opts!= NULL)
    {
        if(!PyConduit_Node_Check(py_opts))
        {
            PyErr_SetString(PyExc_TypeError,
                            "IOHandle.write 'options' argument must "
                                "be a conduit.Node");
                            return NULL;
        }

        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_opts);
    }

    Node *node_ptr = PyConduit_Node_Get_Node_Ptr(py_node);

    try
    {
        if(path == NULL)
        {
            self->handle->write(*node_ptr,
                                *opts_ptr);
        }
        else
        {
            self->handle->write(*node_ptr,
                                std::string(path),
                                *opts_ptr);
        }
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
static PyObject * 
PyRelay_IOHandle_list_child_names(PyRelay_IOHandle *self,
                                  PyObject *args,
                                  PyObject *kwargs)
{
    static const char *kwlist[] = {"path", NULL};
    char *path = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|s",
                                     const_cast<char**>(kwlist),
                                     &path))
    {
        return NULL;
    }
    
    std::vector<std::string> cld_names;
    
    try
    {
        if(path == NULL)
        {
            self->handle->list_child_names(cld_names);
        }
        else
        {
            self->handle->list_child_names(std::string(path),
                                           cld_names);
        }
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }


    /// TODO: I think there is a faster way in the Python CAPI
    /// since we know the size of the list.
    PyObject *retval = PyList_New(0);
    
    for (std::vector<std::string>::const_iterator itr = cld_names.begin();
         itr < cld_names.end(); ++itr)
    {
        PyList_Append(retval,PyString_FromString( (*itr).c_str()));
    }

    return retval;
}

//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_has_path(PyRelay_IOHandle *self,
                          PyObject *args,
                          PyObject *kwargs)
{

    static const char *kwlist[] = {"path", NULL};

    char *path = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s",
                                     const_cast<char**>(kwlist),
                                     &path))
    {
        return NULL;
    }

    bool res = false;

    try
    {
        res = self->handle->has_path(std::string(path));
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    if(res)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_remove(PyRelay_IOHandle *self,
                        PyObject *args,
                        PyObject *kwargs)
{

    static const char *kwlist[] = {"path", NULL};

    char *path = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s",
                                     const_cast<char**>(kwlist),
                                     &path))
    {
        return NULL;
    }

    try
    {
        self->handle->remove(std::string(path));
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE; 
}

//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_flush(PyRelay_IOHandle *self)
{
    try
    {
        self->handle->flush();
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE; 
}


//-----------------------------------------------------------------------------
static PyObject *
PyRelay_IOHandle_close(PyRelay_IOHandle *self)
{
    try
    {
        self->handle->close();
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE; 
}


//----------------------------------------------------------------------------//
// IOHandle methods table
//----------------------------------------------------------------------------//
static PyMethodDef PyRelay_IOHandle_METHODS[] = {
    //-----------------------------------------------------------------------//
    {"open",
     (PyCFunction)PyRelay_IOHandle_open,
     METH_VARARGS | METH_KEYWORDS,
     "Opens a Relay IO Handle"},
    {"is_open",
     (PyCFunction)PyRelay_IOHandle_is_open,
     METH_NOARGS,
     "Checks if a Relay IO Handle is currently open"},
    {"read",
     (PyCFunction)PyRelay_IOHandle_read,
     METH_VARARGS | METH_KEYWORDS,
     "Reads from an active Relay IO Handle"},
    {"write",
     (PyCFunction)PyRelay_IOHandle_write,
     METH_VARARGS | METH_KEYWORDS,
     "Writes to an active Relay IO Handle"},
    {"list_child_names",
     (PyCFunction)PyRelay_IOHandle_list_child_names,
     METH_VARARGS | METH_KEYWORDS,
     "Returns a list of child names"},
    {"remove",
     (PyCFunction)PyRelay_IOHandle_remove,
     METH_VARARGS | METH_KEYWORDS,
     "Removes a path"},
    {"has_path",
     (PyCFunction)PyRelay_IOHandle_has_path,
     METH_VARARGS | METH_KEYWORDS,
     "Checks if a path exists"},
     {"flush",
      (PyCFunction)PyRelay_IOHandle_flush,
       METH_NOARGS,
      "Flush an active Relay IO Handle"},
    {"close",
     (PyCFunction)PyRelay_IOHandle_close,
      METH_NOARGS,
     "Closes an active Relay IO Handle"},
    //-----------------------------------------------------------------------//
    // end Generator methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, 0, NULL}
};


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
#ifdef Py_LIMITED_API
static PyType_Slot PyRelay_IOHandle_SLOTS[]  = {
  {Py_tp_dealloc,        (void*) PyRelay_IOHandle_dealloc},
  {Py_tp_methods,        (void*) PyRelay_IOHandle_METHODS},
  {Py_tp_init,           (void*) PyRelay_IOHandle_init},
  {Py_tp_new,            (void*) PyRelay_IOHandle_new},
  {0,0},
};

static PyType_Spec PyRelay_IOHandle_SPEC = 
{
   "IOHandle",                                 /* tp_name */
   sizeof(PyRelay_IOHandle),                   /* tp_basicsize */
   0,                                          /* tp_itemsize */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   PyRelay_IOHandle_SLOTS,                     /* tp_slots */
};

#else
static PyTypeObject PyRelay_IOHandle_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "IOHandle",
   sizeof(PyRelay_IOHandle),  /* tp_basicsize */
   0, /* tp_itemsize */
   (destructor)PyRelay_IOHandle_dealloc,   /* tp_dealloc */
   // tp_print was removed in Python 3.9, its now used as
   // tp_vectorcall_offset (which we also don't use here)
   0, /* tp_print or tp_vectorcall_offset */
   0, /* tp_getattr */
   0, /* tp_setattr */
   0, /* tp_compare */
   0, /* tp_repr */
   0, /* tp_as_number */
   0, /* tp_as_sequence */
   0, /* as_mapping */
   0, /* hash */
   0, /* call */
   0, // (reprfunc)PyRelay_IOHandle_str,                         /* str */
   0, /* getattro */
   0, /* setattro */
   0, /* asbuffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,     /* flags */
   "Conduit Relay IO Handle objects",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   0, /* iter */
   0, /* iternext */
   PyRelay_IOHandle_METHODS, /* METHODS */
   0, /* MEMBERS */
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   (initproc)PyRelay_IOHandle_init, /* init */
   0, /* alloc */
   PyRelay_IOHandle_new,   /* new */
   0, /* tp_free */
   0, /* tp_is_gc */
   0, /* tp_bases */
   0, /* tp_mro */
   0, /* tp_cache */
   0, /* tp_subclasses */
   0, /* tp_weaklist */
   0, /* tp_del */
   0  /* tp_version_tag */
   PyVarObject_TAIL
};
#endif


//---------------------------------------------------------------------------//
// conduit::relay::io::about
//---------------------------------------------------------------------------//
static PyObject *
PyRelay_io_about()
{
    //create and return a node with the result of about
    PyObject *py_node_res = PyConduit_Node_Python_Create();
    Node *node = PyConduit_Node_Get_Node_Ptr(py_node_res);
    conduit::relay::io::about(*node);
    return (PyObject*)py_node_res;
}

//---------------------------------------------------------------------------//
// conduit::relay::io::save
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_io_save(PyObject *, //self
                PyObject *args,
                PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    PyObject   *py_opts  = NULL;
    
    static const char *kwlist[] = {"node",
                                   "path",
                                   "protocol",
                                   "options",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Os|sO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
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
    
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // default protocol string is empty which auto detects
    std::string protocol_str("");

    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    try
    {
    
        relay::io::save(node,
                        std::string(path),
                        protocol_str,
                        *opts_ptr);
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
// conduit::relay::io::save_merged
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_io_save_merged(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    PyObject   *py_opts  = NULL;
    
    static const char *kwlist[] = {"node",
                                   "path",
                                   "protocol",
                                   "options",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Os|sO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &path,
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
    
    
    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // default protocol string is empty which auto detects
    std::string protocol_str("");    
    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }
    
    try
    {
        relay::io::save_merged(node,
                               std::string(path),
                               protocol_str,
                               *opts_ptr);
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
// conduit::relay::io::load
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_io_load(PyObject *, //self
                PyObject *args,
                PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    
    static const char *kwlist[] = {"node","path","protocol", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Os|s",
                                     const_cast<char**>(kwlist),
                                     &py_node, &path, &protocol))
    {
        return (NULL);
    }
    
    if(py_node != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::io::load 'node' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
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
        relay::io::load(std::string(path),
                        protocol_str,
                        node);
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
// conduit::relay::io::load_merged
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_io_load_merged(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    const char *path     = NULL;
    const char *protocol = NULL;
    
    static const char *kwlist[] = {"node","path","protocol", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Os|s",
                                     const_cast<char**>(kwlist),
                                     &py_node, &path, &protocol))
    {
        return (NULL);
    }
    
    if(py_node != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "relay::io::load 'node' argument must be a "
                            "conduit.Node instance");
            return NULL;
        }
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
        relay::io::load_merged(std::string(path),
                               protocol_str,
                               node);
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
static PyMethodDef relay_io_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    //-----------------------------------------------------------------------//
    {"about",
     (PyCFunction)PyRelay_io_about,
      METH_NOARGS,
      NULL},
    {"save",
     (PyCFunction)PyRelay_io_save,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    {"load",
     (PyCFunction)PyRelay_io_load,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    {"save_merged",
     (PyCFunction)PyRelay_io_save_merged,
      METH_VARARGS | METH_KEYWORDS,
      NULL},
    {"load_merged",
     (PyCFunction)PyRelay_io_load_merged,
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
relay_io_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    #ifdef Py_LIMITED_API
    Py_VISIT(GETSTATE(m)->PyRelay_IOHandle_TYPE);
    #endif
    return 0;
}

//---------------------------------------------------------------------------//
static int 
relay_io_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    #ifdef Py_LIMITED_API
    Py_CLEAR(GETSTATE(m)->PyRelay_IOHandle_TYPE);
    #endif
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef relay_io_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "relay_io_python",
        NULL,
        sizeof(struct module_state),
        relay_io_python_funcs,
        NULL,
        relay_io_python_traverse,
        relay_io_python_clear,
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
CONDUIT_RELAY_PYTHON_API PyObject * PyInit_conduit_relay_io_python(void)
#else
CONDUIT_RELAY_PYTHON_API void initconduit_relay_io_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *relay_io_module = PyModule_Create(&relay_io_python_module_def);
#else
    PyObject *relay_io_module = Py_InitModule((char*)"conduit_relay_io_python",
                                              relay_io_python_funcs);
#endif


    if(relay_io_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(relay_io_module);
    
    st->error = PyErr_NewException((char*)"relay_io_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(relay_io_module);
        PY_MODULE_INIT_RETURN_ERROR;
    }

    // setup for conduit python c api
    if(import_conduit() < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }
    
    //-----------------------------------------------------------------------//
    // init our custom types
    //-----------------------------------------------------------------------//

#ifdef Py_LIMITED_API
    module_state* state = GETSTATE(relay_io_module);
    state->PyRelay_IOHandle_TYPE = (PyTypeObject *)PyType_FromModuleAndSpec((PyObject*)relay_io_module, &PyRelay_IOHandle_SPEC, NULL);
    if (state->PyRelay_IOHandle_TYPE == NULL)
    {
       PY_MODULE_INIT_RETURN_ERROR;
    }
    if (PyModule_AddType((PyObject*)relay_io_module,state->PyRelay_IOHandle_TYPE) < 0)
    {
       PY_MODULE_INIT_RETURN_ERROR;
    }
#else

    if (PyType_Ready(&PyRelay_IOHandle_TYPE) < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    //-----------------------------------------------------------------------//
    // add IOHandle
    //-----------------------------------------------------------------------//

    Py_INCREF(&PyRelay_IOHandle_TYPE);
    PyModule_AddObject(relay_io_module,
                       "IOHandle",
                       (PyObject*)&PyRelay_IOHandle_TYPE);
#endif

#ifdef Py_LIMITED_API
    GLOBAL_MODULE = relay_io_module;
#endif
#if defined(IS_PY3K)
    return relay_io_module;
#endif

}

