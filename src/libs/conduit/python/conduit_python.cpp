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
// include numpy
//---------------------------------------------------------------------------//
// TODO: Use 1.7 deprecated API, or not ?
//---------------------------------------------------------------------------//
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

//---------------------------------------------------------------------------//
// conduit includes
//---------------------------------------------------------------------------//
#include "conduit.hpp"
#include "conduit_python_exports.h"

#define CONDUIT_MODULE
#include "conduit_python.hpp"

using namespace conduit;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Begin Functions to help with Python 2/3 Compatibility.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#if defined(IS_PY3K)

//-----------------------------------------------------------------------------
static int
PyString_Check(PyObject *o)
{
    return PyUnicode_Check(o);
}

//-----------------------------------------------------------------------------
static char *
PyString_AsString(PyObject *py_obj)
{
    char *res = NULL;
    if(PyUnicode_Check(py_obj))
    {
        PyObject * temp_bytes = PyUnicode_AsEncodedString(py_obj,
                                                          "ASCII",
                                                          "strict"); // Owned reference
        if(temp_bytes != NULL)
        {
            res = _conduit_strdup(PyBytes_AsString(temp_bytes));
            Py_DECREF(temp_bytes);
        }
        else
        {
            // TODO: Error
        }
    }
    else if(PyBytes_Check(py_obj))
    {
        res = _conduit_strdup(PyBytes_AsString(py_obj));
    }
    else
    {
        // TODO: ERROR or auto convert?
    }

    return res;
}

//-----------------------------------------------------------------------------
static PyObject *
PyString_FromString(const char *s)
{
    return PyUnicode_FromString(s);
}

//-----------------------------------------------------------------------------
static void
PyString_AsString_Cleanup(char *bytes)
{
    free(bytes);
}

//-----------------------------------------------------------------------------
static PyObject *
PyUnicode_From_UTF32_Unicode_Buffer(const char *unicode_buffer,
                                    int string_len)
{
    #ifdef Py_LIMITED_API
    return PyUnicode_Decode(unicode_buffer,
                             string_len,
                             "utf-32",
                             "strict");
    #else
    return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND,
                                     unicode_buffer,
                                     string_len/4);
    #endif
}

//-----------------------------------------------------------------------------
static int
PyInt_Check(PyObject *o)
{
    return PyLong_Check(o);
}

//-----------------------------------------------------------------------------
static long
PyInt_AsLong(PyObject *o)
{
    return PyLong_AsLong(o);
}

#else // python 2.6+

//-----------------------------------------------------------------------------
#define PyString_AsString_Cleanup(c) { /* noop */ }



//-----------------------------------------------------------------------------
static PyObject *
PyUnicode_From_UTF32_Unicode_Buffer(const char *unicode_buffer,
                                    int string_len)
{
    return PyUnicode_Decode(unicode_buffer,
                             string_len,
                             "utf-32",
                             "strict");
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// End Functions to help with Python 2/3 Compatibility.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// Module Code
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct module_state {
    PyObject *error;
    #ifdef Py_LIMITED_API
    PyTypeObject* PyConduit_DataType_TYPE;
    PyTypeObject* PyConduit_Generator_TYPE;
    PyTypeObject* PyConduit_Schema_TYPE;
    PyTypeObject* PyConduit_NodeIterator_TYPE;
    PyTypeObject* PyConduit_Node_TYPE;
    PyTypeObject* PyConduit_Endianness_TYPE;
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
// Ideally it should be aquired from the type each time using get_state_from_type
// But many functions like PyConduit_Node_Check don't have access to the type
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

struct PyConduit_DataType
{
    PyObject_HEAD
    DataType dtype; // DataType is light weight, we can deal with copies
};

//---------------------------------------------------------------------------//
struct PyConduit_Generator
{
    PyObject_HEAD
    Generator *generator;
};

//---------------------------------------------------------------------------//
struct PyConduit_Schema
{
    PyObject_HEAD
    Schema *schema;
    int python_owns;
};

//---------------------------------------------------------------------------//
struct PyConduit_NodeIterator
{
    PyObject_HEAD
    NodeIterator itr; // NoteIterator is light weight, we can deal with copies
};

//---------------------------------------------------------------------------//
struct PyConduit_Node
{
   PyObject_HEAD
   Node *node;
   int python_owns;
};

//---------------------------------------------------------------------------//
static PyConduit_DataType *PyConduit_DataType_Python_Create();
static int       PyConduit_DataType_Check(PyObject* obj);

//---------------------------------------------------------------------------//
static int       PyConduit_Generator_Check(PyObject* obj);

//---------------------------------------------------------------------------//
static PyObject* PyConduit_Schema_Python_Wrap(Schema *schema,int python_owns);
static int       PyConduit_Schema_Check(PyObject* obj);

//---------------------------------------------------------------------------//
static int       PyConduit_Node_Set_From_Python(Node& node, PyObject* value);
static PyObject* PyConduit_Create_Numpy_Type(Node& node, int type);
static PyObject* PyConduit_Convert_Node_To_Python(Node& node);

//-----------------------------------------------------------------------------
// c api decls from conduit_python.hpp
//-----------------------------------------------------------------------------
//static int       PyConduit_Node_Check(PyObject* obj);
//static PyObject *PyConduit_Node_Python_Create();
//static PyObject *PyConduit_Node_Python_Wrap(Node *n, int owns);
//static Node     *PyConduit_Node_Get_Node_Ptr(PyObject* obj);


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
// DataType Object
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_new(PyTypeObject* type,
                       PyObject* args,
                       PyObject* kwds)
{
    /// TODO: args and kwargs

    static const char *kwlist[] = {"value", NULL};
    PyObject* value = NULL;
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &value))
    {
        return (NULL);
    }

    PyConduit_DataType* self = (PyConduit_DataType*)PyType_GenericAlloc(type,0);
    return ((PyObject*)self);
}

//---------------------------------------------------------------------------//
static bool
PyConduit_DataType_Set_Parse_Args(PyConduit_DataType* self,
                                  PyObject* args,
                                  PyObject* kwargs)
{
    int32 parse_case = -1;
    ///
    /// Three ways to call:
    ///
    /// Copy Constructor style:
    ///   args[0] == PyObject || kwargs[0] == PyObject

    static const char *kwlist_obj[] = {"dtype",
                                        NULL};
    ///
    /// DataType Name First:
    ///
    static const char *kwlist_name[] = {"dtype_name",
                                        "num_elements",
                                        "offset",
                                        "stride",
                                        "element_bytes",
                                        "endianness",
                                        NULL};
    ///
    /// DataType Id First:
    ///
    static const char *kwlist_id[] = {"dtype_id",
                                      "num_elements",
                                      "offset",
                                      "stride",
                                      "element_bytes",
                                      "endianness",
                                       NULL};

    PyObject *py_obj = 0;

    if(args != NULL)
    {
        if(PySequence_Size(args)>0)
        {
            py_obj = PySequence_GetItem(args,0);
            if(py_obj)
            {
                // check for object
                if(PyConduit_DataType_Check(py_obj))
                {
                    parse_case = 0;
                }
                // else check for string
                else if(PyString_Check(py_obj))
                {
                    parse_case = 1;
                }
                // else check for index_t
                else if(PyIndex_Check(py_obj))
                {
                    parse_case = 2;
                }
            }
        }
    }

    if(parse_case == -1 && kwargs !=NULL)
    {
        // check for object
        if(PyDict_GetItemString(kwargs,"dtype"))
        {
            parse_case = 0;
        }
        // else check for string
        else if(PyDict_GetItemString(kwargs,"dtype_name"))
        {
            parse_case = 1;
        }
        // else check for index_t
        else if(PyDict_GetItemString(kwargs,"dtype_id"))
        {
            parse_case = 2;
        }
    }

    //std::cout << "parse_case = " << parse_case <<  std::endl;
    /// if we aren't parsing any args, simply return
    if(parse_case == -1)
        return true;

    Py_ssize_t  dtype_id = 0;
    char       *dtype_name = NULL;
    Py_ssize_t  num_elements = 0;
    Py_ssize_t  offset = 0;
    Py_ssize_t  stride = 0;
    Py_ssize_t  element_bytes = 0;
    Py_ssize_t  endianness =0;

    if(parse_case == 0)
    {
        if (!PyArg_ParseTupleAndKeywords(args,
                                         kwargs,
                                         "O",
                                         const_cast<char**>(kwlist_obj),
                                         py_obj))
        {
            // TODO: Set Error?
            return false;
        }

        if(!PyConduit_DataType_Check(py_obj))
        {
            // TODO: Set Error?
            return false;
        }

        PyConduit_DataType *py_dtype = (PyConduit_DataType *)py_obj;

        self->dtype.set(py_dtype->dtype);

    }
    else if(parse_case == 1)
    {
        if (!PyArg_ParseTupleAndKeywords(args,
                                         kwargs,
                                         "s|nnnnn",
                                         const_cast<char**>(kwlist_name),
                                         &dtype_name,
                                         &num_elements,
                                         &offset,
                                         &stride,
                                         &element_bytes,
                                         &endianness))
        {
            // TODO: Set Error?
            return false;
        }

        dtype_id = (Py_ssize_t) DataType::name_to_id(std::string(dtype_name));

        self->dtype.set(dtype_id,
                        num_elements,
                        offset,
                        stride,
                        element_bytes,
                        endianness);
    }
    else if(parse_case ==2)
    {
        if (!PyArg_ParseTupleAndKeywords(args,
                                         kwargs,
                                         "n|nnnnn",
                                         const_cast<char**>(kwlist_id),
                                         &dtype_id,
                                         &num_elements,
                                         &offset,
                                         &stride,
                                         &element_bytes,
                                         &endianness))
        {
            // TODO: Set Error?
            return false;
        }

        self->dtype.set(dtype_id,
                        num_elements,
                        offset,
                        stride,
                        element_bytes,
                        endianness);
    }

    return true;
}


//---------------------------------------------------------------------------//
static int
PyConduit_DataType_init(PyConduit_DataType* self,
                        PyObject* args,
                        PyObject* kwargs)
{
    if(!PyConduit_DataType_Set_Parse_Args(self,args,kwargs))
    {
        // todo: error?
        return 0;
    }
    return 0;
}

//---------------------------------------------------------------------------//
static void
PyConduit_DataType_dealloc(PyConduit_DataType *self)
{
    #ifdef Py_LIMITED_API
    freefunc tp_free = ((freefunc)PyType_GetSlot(Py_TYPE((PyObject*)self), Py_tp_free));
    tp_free((PyObject*)self);
    #else
    Py_TYPE(self)->tp_free((PyObject*)self);
    #endif
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_str(PyConduit_DataType *self)
{
   return (Py_BuildValue("s", self->dtype.to_string().c_str()));
}

//-----------------------------------------------------------------------------
static bool
PyConduit_DataType_Parse_Standard_Set_Keyword_Args(PyObject *args,
                                                   PyObject *kwargs,
                                                   Py_ssize_t &num_elements,
                                                   Py_ssize_t &offset,
                                                   Py_ssize_t &stride,
                                                   Py_ssize_t &element_bytes,
                                                   Py_ssize_t &endianness)


{
    static const char *kwlist[] = {"num_elements",
                                   "offset",
                                   "stride",
                                   "element_bytes",
                                   "endianness",
                                     NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|nnnnn",
                                     const_cast<char**>(kwlist),
                                     &num_elements,
                                     &offset,
                                     &stride,
                                     &element_bytes,
                                     &endianness))
    {
        return false;
    }
    return true;
}



//-----------------------------------------------------------------------------
// --- DataType constructor helpers for object types --- //
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_empty(PyObject *) // cls -- unused
{
    PyConduit_DataType *res = PyConduit_DataType_Python_Create();
    res->dtype.set_id(DataType::EMPTY_ID);
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_object(PyObject *) // cls -- unused
{
    PyConduit_DataType *res = PyConduit_DataType_Python_Create();
    res->dtype.set_id(DataType::OBJECT_ID);
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_list(PyObject *) // cls -- unused
{
    PyConduit_DataType *res = PyConduit_DataType_Python_Create();
    res->dtype.set_id(DataType::LIST_ID);
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
// --- DataType constructor helpers for signed integers                  --- //
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_int8(PyObject *, // cls -- unused
                        PyObject *args,
                        PyObject *kwargs)
{
    // default args for int8
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::int8);
    Py_ssize_t element_bytes = sizeof(conduit::int8);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::int8(num_elements,
                                  offset,
                                  stride,
                                  element_bytes,
                                  endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_int16(PyObject *, // cls -- unused
                         PyObject *args,
                         PyObject *kwargs)
{
    // default args for int16
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::int16);
    Py_ssize_t element_bytes = sizeof(conduit::int16);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))

    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::int16(num_elements,
                                   offset,
                                   stride,
                                   element_bytes,
                                   endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_int32(PyObject *, // cls -- unused
                         PyObject *args,
                         PyObject *kwargs)
{
    // default args for int32
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::int32);
    Py_ssize_t element_bytes = sizeof(conduit::int32);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::int32(num_elements,
                                   offset,
                                   stride,
                                   element_bytes,
                                   endianness));
    return (PyObject*)res;
}
//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_int64(PyObject *, // cls -- unused
                         PyObject *args,
                         PyObject *kwargs)
{
    // default args for int64
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::int64);
    Py_ssize_t element_bytes = sizeof(conduit::int64);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::int64(num_elements,
                                   offset,
                                   stride,
                                   element_bytes,
                                   endianness));
    return (PyObject*)res;
}


//-----------------------------------------------------------------------------
// --- DataType constructor helpers for unsigned integers                --- //
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_uint8(PyObject *, // cls -- unused
                         PyObject *args,
                         PyObject *kwargs)
{
    // default args for uint8
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::uint8);
    Py_ssize_t element_bytes = sizeof(conduit::uint8);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::uint8(num_elements,
                                   offset,
                                   stride,
                                   element_bytes,
                                   endianness));
    return (PyObject*)res;
;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_uint16(PyObject *, // cls -- unused
                          PyObject *args,
                          PyObject *kwargs)
{
    // default args for uint16
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::uint16);
    Py_ssize_t element_bytes = sizeof(conduit::uint16);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::uint16(num_elements,
                                    offset,
                                    stride,
                                    element_bytes,
                                    endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_uint32(PyObject *, // cls -- unused
                          PyObject *args,
                          PyObject *kwargs)
{
    // default args for uint32
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::uint32);
    Py_ssize_t element_bytes = sizeof(conduit::uint32);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::uint32(num_elements,
                                    offset,
                                    stride,
                                    element_bytes,
                                    endianness));
    return (PyObject*)res;
}
//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_uint64(PyObject *, // cls -- unused
                          PyObject *args,
                          PyObject *kwargs)
{
    // default args for uint64
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::uint64);
    Py_ssize_t element_bytes = sizeof(conduit::uint64);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::uint64(num_elements,
                                    offset,
                                    stride,
                                    element_bytes,
                                    endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
// --- DataType constructor helpers for floating point numbers           --- //
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_float32(PyObject *, // cls -- unused
                           PyObject *args,
                           PyObject *kwargs)
{
    // default args for float32
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::float32);
    Py_ssize_t element_bytes = sizeof(conduit::float32);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::float32(num_elements,
                                     offset,
                                     stride,
                                     element_bytes,
                                     endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_float64(PyObject *, // cls -- unused
                           PyObject *args,
                           PyObject *kwargs)
{
    // default args for float64
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::float64);
    Py_ssize_t element_bytes = sizeof(conduit::float64);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::float64(num_elements,
                                     offset,
                                     stride,
                                     element_bytes,
                                     endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_char8_str(PyObject *, // cls -- unused
                             PyObject *args,
                             PyObject *kwargs)
{
    // default args for char8_str
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = 1;
    Py_ssize_t element_bytes = 1;
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::char8_str(num_elements,
                                       offset,
                                       stride,
                                       element_bytes,
                                       endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_index_t(PyObject *, // cls -- unused
                           PyObject *args,
                           PyObject *kwargs)
{
    // default args for char8_str
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(conduit::index_t);
    Py_ssize_t element_bytes = sizeof(conduit::index_t);;
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::index_t(num_elements,
                                     offset,
                                     stride,
                                     element_bytes,
                                     endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
// --- DataType constructor helpers for native c signed integers         --- //
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_c_char(PyObject *, // cls -- unused
                          PyObject *args,
                          PyObject *kwargs)
{
    // default args for c_char
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_CHAR_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_CHAR_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_char(num_elements,
                                    offset,
                                    stride,
                                    element_bytes,
                                    endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_c_short(PyObject *, // cls -- unused
                           PyObject *args,
                           PyObject *kwargs)
{
    // default args for c_short
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_SHORT_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_SHORT_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_short(num_elements,
                                     offset,
                                     stride,
                                     element_bytes,
                                     endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_c_int(PyObject *, // cls -- unused
                         PyObject *args,
                         PyObject *kwargs)
{
    // default args for c_int
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_INT_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_INT_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_int(num_elements,
                                   offset,
                                   stride,
                                   element_bytes,
                                   endianness));
    return (PyObject*)res;
}
//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_c_long(PyObject *, // cls -- unused
                          PyObject *args,
                          PyObject *kwargs)
{
    // default args for c_long
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_LONG_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_LONG_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_long(num_elements,
                                    offset,
                                    stride,
                                    element_bytes,
                                    endianness));
    return (PyObject*)res;
}


//-----------------------------------------------------------------------------
// --- DataType constructor helpers for native c unsigned integers       --- //
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_c_unsigned_char(PyObject *, // cls -- unused
                                   PyObject *args,
                                   PyObject *kwargs)
{
    // default args for c_unsigned_char
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_unsigned_char(num_elements,
                                             offset,
                                             stride,
                                             element_bytes,
                                             endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_c_unsigned_short(PyObject *, // cls -- unused
                                    PyObject *args,
                                    PyObject *kwargs)
{
    // default args for c_unsigned_short
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_unsigned_short(num_elements,
                                              offset,
                                              stride,
                                              element_bytes,
                                              endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_c_unsigned_int(PyObject *, // cls -- unused
                                  PyObject *args,
                                  PyObject *kwargs)
{
    // default args for c_unsigned_int
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_INT_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_INT_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_unsigned_int(num_elements,
                                            offset,
                                            stride,
                                            element_bytes,
                                            endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_c_unsigned_long(PyObject *, // cls -- unused
                                   PyObject *args,
                                   PyObject *kwargs)
{
    // default args for c_unsigned_long
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();
    res->dtype.set(DataType::c_unsigned_long(num_elements,
                                             offset,
                                             stride,
                                             element_bytes,
                                             endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
// --- DataType constructor helpers for floating point numbers --- //
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
PyObject *
PyConduit_DataType_c_float(PyObject *, // cls -- unused
                           PyObject *args,
                           PyObject *kwargs)
{
    // default args for c_float
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_FLOAT_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_FLOAT_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;


    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_float(num_elements,
                                     offset,
                                     stride,
                                     element_bytes,
                                     endianness));
    return (PyObject*)res;
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_c_double(PyObject *, // cls -- unused
                            PyObject *args,
                            PyObject *kwargs)
{
    // default args for c_double
    Py_ssize_t num_elements = 1;
    Py_ssize_t offset = 0;
    Py_ssize_t stride = sizeof(CONDUIT_NATIVE_DOUBLE_ID);
    Py_ssize_t element_bytes = sizeof(CONDUIT_NATIVE_DOUBLE_ID);
    Py_ssize_t endianness = Endianness::DEFAULT_ID;

    if(!PyConduit_DataType_Parse_Standard_Set_Keyword_Args(args,
                                                           kwargs,
                                                           num_elements,
                                                           offset,
                                                           stride,
                                                           element_bytes,
                                                           endianness))
    {
        // parsing error
        return NULL;
    }

    PyConduit_DataType *res = PyConduit_DataType_Python_Create();

    res->dtype.set(DataType::c_double(num_elements,
                                      offset,
                                      stride,
                                      element_bytes,
                                      endianness));
    return (PyObject*)res;
}


//-----------------------------------------------------------------------------
// Setters
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_set(PyConduit_DataType *self,
                       PyObject *args,
                       PyObject *kwargs)
{
    if(!PyConduit_DataType_Set_Parse_Args(self,args,kwargs))
    {
        /// TODO: error?
        return NULL;
    }
    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_set_id(PyConduit_DataType *self,
                                          PyObject *args)
{
    Py_ssize_t value;

    if (!PyArg_ParseTuple(args, "n", &value))
    {
        PyErr_SetString(PyExc_TypeError,
            "dtype_id must be a signed integer");
        return NULL;
    }

    self->dtype.set_id(value);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_set_number_of_elements(PyConduit_DataType *self,
                                          PyObject *args)
{
    Py_ssize_t value;

    if (!PyArg_ParseTuple(args, "n", &value))
    {
        PyErr_SetString(PyExc_TypeError,
            "number_of_elements must be a signed integer");
        return NULL;
    }

    self->dtype.set_number_of_elements(value);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_set_offset(PyConduit_DataType *self,
                              PyObject *args)
{
    Py_ssize_t value;

    if (!PyArg_ParseTuple(args, "n", &value))
    {
        PyErr_SetString(PyExc_TypeError,
            "offset must be be a signed integer");
        return NULL;
    }

    self->dtype.set_offset(value);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_set_stride(PyConduit_DataType *self,
                              PyObject *args)
{
    Py_ssize_t value;

    if (!PyArg_ParseTuple(args, "n", &value))
    {
        PyErr_SetString(PyExc_TypeError,
            "stride must be a signed integer");
        return NULL;
    }

    self->dtype.set_stride(value);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_set_element_bytes(PyConduit_DataType *self,
                                     PyObject *args)
{
    Py_ssize_t value;

    if (!PyArg_ParseTuple(args, "n", &value))
    {
        PyErr_SetString(PyExc_TypeError,
            "element bytes must be a signed integer");
        return NULL;
    }

    self->dtype.set_element_bytes(value);

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_set_endianness(PyConduit_DataType *self,
                                  PyObject *args)
{
    Py_ssize_t value;

    if (!PyArg_ParseTuple(args, "n", &value))
    {
        PyErr_SetString(PyExc_TypeError,
            "endianness must be a signed integer");
        return NULL;
    }

    self->dtype.set_endianness(value);

    Py_RETURN_NONE;
}

//-----------------------------------------------------------------------------
// Getters and info methods.
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_id(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.id());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_name(PyConduit_DataType *self)
{
    return Py_BuildValue("s", self->dtype.name().c_str());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_bytes_compact(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.bytes_compact());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_strided_bytes(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.strided_bytes());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_spanned_bytes(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.spanned_bytes());
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_compact(PyConduit_DataType *self)
{
    if(self->dtype.is_compact())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_equals(PyConduit_DataType *self,
                          PyObject *args)
{
    PyObject *py_dtype;
    if ( (!PyArg_ParseTuple(args, "O", &py_dtype)) ||
         (!PyConduit_DataType_Check(py_dtype)) )
    {
         PyErr_SetString(PyExc_TypeError, "equals e needs a DataType arg");
         return (NULL);
    }

    if(self->dtype.equals( ((PyConduit_DataType*)py_dtype)->dtype ) )
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_compatible(PyConduit_DataType *self,
                              PyObject *args)
{
    PyObject *py_dtype;
    if ( (!PyArg_ParseTuple(args, "O", &py_dtype)) ||
         (!PyConduit_DataType_Check(py_dtype)) )
    {
         PyErr_SetString(PyExc_TypeError, "is_compatible needs a DataType arg");
         return (NULL);
    }


    if(self->dtype.compatible( ((PyConduit_DataType*)py_dtype)->dtype))
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_empty(PyConduit_DataType *self)
{
    if(self->dtype.is_empty())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_object(PyConduit_DataType *self)
{
    if(self->dtype.is_object())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_list(PyConduit_DataType *self)
{
    if(self->dtype.is_list())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_number(PyConduit_DataType *self)
{
    if(self->dtype.is_number())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_floating_point(PyConduit_DataType *self)
{
    if(self->dtype.is_floating_point())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_integer(PyConduit_DataType *self)
{
    if(self->dtype.is_integer())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_signed_integer(PyConduit_DataType *self)
{
    if(self->dtype.is_signed_integer())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_unsigned_integer(PyConduit_DataType *self)
{
    if(self->dtype.is_unsigned_integer())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
// -- `is` for bw ints
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_int8(PyConduit_DataType *self)
{
    if(self->dtype.is_int8())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_int16(PyConduit_DataType *self)
{
    if(self->dtype.is_int16())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_int32(PyConduit_DataType *self)
{
    if(self->dtype.is_int32())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_int64(PyConduit_DataType *self)
{
    if(self->dtype.is_int64())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
// -- `is` for bw uints
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_uint8(PyConduit_DataType *self)
{
    if(self->dtype.is_uint8())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_uint16(PyConduit_DataType *self)
{
    if(self->dtype.is_uint16())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_uint32(PyConduit_DataType *self)
{
    if(self->dtype.is_uint32())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_uint64(PyConduit_DataType *self)
{
    if(self->dtype.is_uint64())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_float32(PyConduit_DataType *self)
{
    if(self->dtype.is_float32())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_float64(PyConduit_DataType *self)
{
    if(self->dtype.is_float64())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_index_t(PyConduit_DataType *self)
{
    if(self->dtype.is_index_t())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
// -- `is` for c style types
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_char(PyConduit_DataType *self)
{
    if(self->dtype.is_char())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_short(PyConduit_DataType *self)
{
    if(self->dtype.is_short())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_int(PyConduit_DataType *self)
{
    if(self->dtype.is_int())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_long(PyConduit_DataType *self)
{
    if(self->dtype.is_long())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_long_long(PyConduit_DataType *self)
{
    if(self->dtype.is_long_long())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
// -- `is` for signed c style types
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_signed_char(PyConduit_DataType *self)
{
    if(self->dtype.is_signed_char())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_signed_short(PyConduit_DataType *self)
{
    if(self->dtype.is_signed_short())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_signed_int(PyConduit_DataType *self)
{
    if(self->dtype.is_signed_int())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_signed_long(PyConduit_DataType *self)
{
    if(self->dtype.is_signed_long())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_signed_long_long(PyConduit_DataType *self)
{
    if(self->dtype.is_signed_long_long())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
// -- `is` for unsigned c style types
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_unsigned_char(PyConduit_DataType *self)
{
    if(self->dtype.is_unsigned_char())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_unsigned_short(PyConduit_DataType *self)
{
    if(self->dtype.is_unsigned_short())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_unsigned_int(PyConduit_DataType *self)
{
    if(self->dtype.is_unsigned_int())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_unsigned_long(PyConduit_DataType *self)
{
    if(self->dtype.is_unsigned_long())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_unsigned_long_long(PyConduit_DataType *self)
{
    if(self->dtype.is_unsigned_long_long())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
// -- `is` for c style fp types
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_float(PyConduit_DataType *self)
{
    if(self->dtype.is_float())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_double(PyConduit_DataType *self)
{
    if(self->dtype.is_double())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_long_double(PyConduit_DataType *self)
{
    if(self->dtype.is_long_double())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}


//---------------------------------------------------------------------------//
// -- `is` for string types
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_string(PyConduit_DataType *self)
{
    if(self->dtype.is_string())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_char8_str(PyConduit_DataType *self)
{
    if(self->dtype.is_char8_str())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_little_endian(PyConduit_DataType *self)
{
    if(self->dtype.is_little_endian())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_is_big_endian(PyConduit_DataType *self)
{
    if(self->dtype.is_big_endian())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_endianness_matches_machine(PyConduit_DataType *self)
{
    if(self->dtype.endianness_matches_machine())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_number_of_elements(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.number_of_elements());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_offset(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.offset());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_stride(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.stride());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_element_bytes(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.element_bytes());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_endianness(PyConduit_DataType *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.endianness());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_element_index(PyConduit_DataType *self,
                                 PyObject *args)
{
    Py_ssize_t idx;

    if (!PyArg_ParseTuple(args, "n", &idx))
    {
        PyErr_SetString(PyExc_TypeError,
                "index must be a signed integer");
        return NULL;
    }

    return PyLong_FromSsize_t((Py_ssize_t)self->dtype.element_index(idx));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_name_to_id(PyObject *, // cls -- unused
                              PyObject *args)
{
    const char *dtype_name;
    if (!PyArg_ParseTuple(args, "s", &dtype_name))
    {
        PyErr_SetString(PyExc_TypeError, "DataType name must be a string");
        return NULL;
    }

    return PyLong_FromSsize_t((Py_ssize_t)DataType::name_to_id(std::string(dtype_name)));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_id_to_name(PyObject *, // cls -- unused
                              PyObject *args)
{
    Py_ssize_t dtype_id;

    if (!PyArg_ParseTuple(args, "n", &dtype_id))
    {
        PyErr_SetString(PyExc_TypeError,
                "DataType id must be a signed integer");
        return NULL;
    }

    return Py_BuildValue("s", DataType::id_to_name(dtype_id).c_str());
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_to_string(PyConduit_DataType* self,
                             PyObject* args,
                             PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string protocol = "yaml";
    std::string pad = " ";
    std::string eoe = "\n";

    char *protocol_c_str = NULL;
    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"protocol",
                                   "indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &protocol_c_str,
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(protocol_c_str != NULL)
    {
        protocol = std::string(protocol_c_str);
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->dtype.to_string_stream(oss,
                                     protocol,
                                     indent,
                                     depth,
                                     pad,
                                     eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_to_json(PyConduit_DataType* self,
                           PyObject* args,
                           PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string pad = " ";
    std::string eoe = "\n";

    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->dtype.to_json_stream(oss,
                                   indent,
                                   depth,
                                   pad,
                                   eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_DataType_to_yaml(PyConduit_DataType* self,
                           PyObject* args,
                           PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string pad = " ";
    std::string eoe = "\n";

    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->dtype.to_yaml_stream(oss,
                                   indent,
                                   depth,
                                   pad,
                                   eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}


// -- id enum value access -- /

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_empty_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::EMPTY_ID);
}


//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_object_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::OBJECT_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_list_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::LIST_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_char8_str_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::CHAR8_STR_ID);
}

// -- signed int ids -- //

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_int8_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::INT8_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_int16_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::INT16_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_int32_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::INT32_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_int64_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::INT64_ID);
}

// -- uint ids -- //

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_uint8_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::UINT8_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_uint16_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::UINT16_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_uint32_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::UINT32_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_uint64_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::UINT64_ID);
}

// -- fp ids -- //

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_float32_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::FLOAT32_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_DataType_float64_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(DataType::FLOAT64_ID);
}


//----------------------------------------------------------------------------//
// DataType methods table
//----------------------------------------------------------------------------//
static PyMethodDef PyConduit_DataType_METHODS[] = {
    //-----------------------------------------------------------------------//
    {"set",
     _PyCFunction_CAST(PyConduit_DataType_set),
     METH_VARARGS | METH_KEYWORDS,
     "{todo}"},
    {"set_id",
     _PyCFunction_CAST(PyConduit_DataType_set_id),
      METH_VARARGS,
      "Sets the dtype id of this DataType"},
    //-----------------------------------------------------------------------//
    {"set_number_of_elements",
     _PyCFunction_CAST(PyConduit_DataType_set_number_of_elements),
     METH_VARARGS,
     "Sets the number of elements property of this DataType"},
    //-----------------------------------------------------------------------//
    {"set_offset",
     _PyCFunction_CAST(PyConduit_DataType_set_offset),
     METH_VARARGS,
     "Sets the byte offset property of this DataType"},
    //-----------------------------------------------------------------------//
    {"set_stride",
     _PyCFunction_CAST(PyConduit_DataType_set_stride),
     METH_VARARGS,
     "Sets the byte stride property of this DataType"},
    //-----------------------------------------------------------------------//
    {"set_element_bytes",
     _PyCFunction_CAST(PyConduit_DataType_set_element_bytes),
     METH_VARARGS,
     "Sets the number of bytes per element property of this DataType"},
    //-----------------------------------------------------------------------//
    {"set_endianness",
     _PyCFunction_CAST(PyConduit_DataType_set_endianness),
     METH_VARARGS,
     "Sets the endianness property of this DataType"},
    //-----------------------------------------------------------------------//
    {"id",
     _PyCFunction_CAST(PyConduit_DataType_id),
     METH_NOARGS,
     "Returns the dtype id of this DataType"},
    //-----------------------------------------------------------------------//
    {"name",
     _PyCFunction_CAST(PyConduit_DataType_name),
     METH_NOARGS,
     "Returns the name of this DataType"},
    //-----------------------------------------------------------------------//
    {"number_of_elements",
     _PyCFunction_CAST(PyConduit_DataType_number_of_elements),
     METH_NOARGS,
     "Returns the number of elements property of this DataType"},
    //-----------------------------------------------------------------------//
    {"offset",
     _PyCFunction_CAST(PyConduit_DataType_offset),
     METH_VARARGS,
     "Returns the byte offset property of this DataType"},
    //-----------------------------------------------------------------------//
    {"stride",
     _PyCFunction_CAST(PyConduit_DataType_stride),
     METH_NOARGS,
     "Returns the byte stride property of this DataType"},
    //-----------------------------------------------------------------------//
    {"element_bytes",
     _PyCFunction_CAST(PyConduit_DataType_element_bytes),
     METH_NOARGS,
     "Returns the number of bytes per element property of this DataType"},
    //-----------------------------------------------------------------------//
    {"endianness",
     _PyCFunction_CAST(PyConduit_DataType_endianness),
     METH_NOARGS,
     "Returns the endianness property of this DataType"},
    //-----------------------------------------------------------------------//
    {"element_index",
     _PyCFunction_CAST(PyConduit_DataType_element_index),
     METH_VARARGS,
     "Returns the byte offset for a given element index of this DataType"},
    //-----------------------------------------------------------------------//
    {"strided_bytes",
     _PyCFunction_CAST(PyConduit_DataType_strided_bytes),
     METH_NOARGS,
     "Returns the strided bytes property of this DataType"},
    //-----------------------------------------------------------------------//
    {"bytes_compact",
     _PyCFunction_CAST(PyConduit_DataType_bytes_compact),
     METH_NOARGS,
     "Returns the bytes compact property of this DataType"},
    //-----------------------------------------------------------------------//
    {"spanned_bytes",
     _PyCFunction_CAST(PyConduit_DataType_spanned_bytes),
     METH_NOARGS,
     "Returns the bytes spanned property of this DataType"},
    //-----------------------------------------------------------------------//
    {"is_compact",
     _PyCFunction_CAST(PyConduit_DataType_is_compact),
     METH_NOARGS,
     "Returns if this DataType is compact"},
    //-----------------------------------------------------------------------//
    {"compatible",
     _PyCFunction_CAST(PyConduit_DataType_compatible),
     METH_VARARGS,
     "Returns if this DataType is compatible with another DataType instance"},
    //-----------------------------------------------------------------------//
    {"equals",
     _PyCFunction_CAST(PyConduit_DataType_equals),
     METH_VARARGS,
     "Returns if this DataType is equal one passed"},
    //-----------------------------------------------------------------------//
    {"is_empty",
     _PyCFunction_CAST(PyConduit_DataType_is_empty),
     METH_NOARGS,
     "Returns if this DataType is empty"},
    //-----------------------------------------------------------------------//
    {"is_object",
     _PyCFunction_CAST(PyConduit_DataType_is_object),
     METH_NOARGS,
     "Returns if this DataType is an object"},
    //-----------------------------------------------------------------------//
    {"is_list",
     _PyCFunction_CAST(PyConduit_DataType_is_list),
     METH_NOARGS,
     "Returns if this DataType is a list"},
    //-----------------------------------------------------------------------//
    {"is_number",
     _PyCFunction_CAST(PyConduit_DataType_is_number),
     METH_NOARGS,
     "Returns if this DataType is a number"},
    //-----------------------------------------------------------------------//
    {"is_floating_point",
     _PyCFunction_CAST(PyConduit_DataType_is_floating_point),
     METH_NOARGS,
     "Returns if this DataType is a floating point number"},
    //-----------------------------------------------------------------------//
    {"is_integer",
     _PyCFunction_CAST(PyConduit_DataType_is_integer),
     METH_NOARGS,
     "Returns if this DataType is an integer"},
    //-----------------------------------------------------------------------//
    {"is_signed_integer",
     _PyCFunction_CAST(PyConduit_DataType_is_signed_integer),
     METH_NOARGS,
     "Returns if this DataType is a signed integer"},
    //-----------------------------------------------------------------------//
    {"is_unsigned_integer",
     _PyCFunction_CAST(PyConduit_DataType_is_unsigned_integer),
     METH_NOARGS,
     "Returns if this DataType is an unsigned integer"},

    //-----------------------------------------------------------------------//
    {"is_int8",
     _PyCFunction_CAST(PyConduit_DataType_is_int8),
     METH_NOARGS,
     "Returns if this DataType is an int8"},
    //-----------------------------------------------------------------------//
    {"is_int16",
     _PyCFunction_CAST(PyConduit_DataType_is_int16),
     METH_NOARGS,
     "Returns if this DataType is an int16"},
    //-----------------------------------------------------------------------//
    {"is_int32",
     _PyCFunction_CAST(PyConduit_DataType_is_int32),
     METH_NOARGS,
     "Returns if this DataType is an int32"},
    //-----------------------------------------------------------------------//
    {"is_int64",
     _PyCFunction_CAST(PyConduit_DataType_is_int64),
     METH_NOARGS,
     "Returns if this DataType is an int64"},
    //-----------------------------------------------------------------------//
    {"is_uint8",
     _PyCFunction_CAST(PyConduit_DataType_is_uint8),
     METH_NOARGS,
     "Returns if this DataType is an uint8"},
    //-----------------------------------------------------------------------//
    {"is_uint16",
     _PyCFunction_CAST(PyConduit_DataType_is_uint16),
     METH_NOARGS,
     "Returns if this DataType is an uint16"},
    //-----------------------------------------------------------------------//
    {"is_uint32",
     _PyCFunction_CAST(PyConduit_DataType_is_uint32),
     METH_NOARGS,
     "Returns if this DataType is an uint32"},
    //-----------------------------------------------------------------------//
    {"is_uint64",
     _PyCFunction_CAST(PyConduit_DataType_is_uint64),
     METH_NOARGS,
     "Returns if this DataType is an uint64"},

    //-----------------------------------------------------------------------//
    {"is_float32",
     _PyCFunction_CAST(PyConduit_DataType_is_float32),
     METH_NOARGS,
     "Returns if this DataType is a float32"},
    //-----------------------------------------------------------------------//
    {"is_float64",
     _PyCFunction_CAST(PyConduit_DataType_is_float64),
     METH_NOARGS,
     "Returns if this DataType is a float64"},

    //-----------------------------------------------------------------------//
    {"is_index_t",
     _PyCFunction_CAST(PyConduit_DataType_is_index_t),
     METH_NOARGS,
     "Returns if this DataType is an index_t"},

    //-----------------------------------------------------------------------//
    {"is_char",
     _PyCFunction_CAST(PyConduit_DataType_is_char),
     METH_NOARGS,
     "Returns if this DataType is a char"},
    //-----------------------------------------------------------------------//
    {"is_short",
     _PyCFunction_CAST(PyConduit_DataType_is_short),
     METH_NOARGS,
     "Returns if this DataType is a short"},
    //-----------------------------------------------------------------------//
    {"is_int",
     _PyCFunction_CAST(PyConduit_DataType_is_int),
     METH_NOARGS,
     "Returns if this DataType is an int"},
    //-----------------------------------------------------------------------//
    {"is_long",
     _PyCFunction_CAST(PyConduit_DataType_is_long),
     METH_NOARGS,
     "Returns if this DataType is a long"},
    //-----------------------------------------------------------------------//
    {"is_long_long",
     _PyCFunction_CAST(PyConduit_DataType_is_long_long),
     METH_NOARGS,
     "Returns if this DataType is a long long"},

    //-----------------------------------------------------------------------//
    {"is_signed_char",
     _PyCFunction_CAST(PyConduit_DataType_is_signed_char),
     METH_NOARGS,
     "Returns if this DataType is a signed char"},
    //-----------------------------------------------------------------------//
    {"is_signed_short",
     _PyCFunction_CAST(PyConduit_DataType_is_signed_short),
     METH_NOARGS,
     "Returns if this DataType is a signed short"},
    //-----------------------------------------------------------------------//
    {"is_signed_int",
     _PyCFunction_CAST(PyConduit_DataType_is_signed_int),
     METH_NOARGS,
     "Returns if this DataType is a signed int"},
    //-----------------------------------------------------------------------//
    {"is_signed_long",
     _PyCFunction_CAST(PyConduit_DataType_is_signed_long),
     METH_NOARGS,
     "Returns if this DataType is a signed long"},
    //-----------------------------------------------------------------------//
    {"is_signed_long_long",
     _PyCFunction_CAST(PyConduit_DataType_is_signed_long_long),
     METH_NOARGS,
     "Returns if this DataType is a signed long long"},

    //-----------------------------------------------------------------------//
    {"is_unsigned_char",
     _PyCFunction_CAST(PyConduit_DataType_is_unsigned_char),
     METH_NOARGS,
     "Returns if this DataType is an unsigned char"},
    //-----------------------------------------------------------------------//
    {"is_unsigned_short",
     _PyCFunction_CAST(PyConduit_DataType_is_unsigned_short),
     METH_NOARGS,
     "Returns if this DataType is an unsigned short"},
    //-----------------------------------------------------------------------//
    {"is_unsigned_int",
     _PyCFunction_CAST(PyConduit_DataType_is_unsigned_int),
     METH_NOARGS,
     "Returns if this DataType is an unsigned int"},
    //-----------------------------------------------------------------------//
    {"is_unsigned_long",
     _PyCFunction_CAST(PyConduit_DataType_is_unsigned_long),
     METH_NOARGS,
     "Returns if this DataType is an unsigned long"},
    //-----------------------------------------------------------------------//
    {"is_unsigned_long_long",
     _PyCFunction_CAST(PyConduit_DataType_is_unsigned_long_long),
     METH_NOARGS,
     "Returns if this DataType is an unsigned long long"},

    //-----------------------------------------------------------------------//
    {"is_float",
     _PyCFunction_CAST(PyConduit_DataType_is_float),
     METH_NOARGS,
     "Returns if this DataType is a float"},
    //-----------------------------------------------------------------------//
    {"is_double",
     _PyCFunction_CAST(PyConduit_DataType_is_double),
     METH_NOARGS,
     "Returns if this DataType is a double"},
    //-----------------------------------------------------------------------//
    {"is_long_double",
     _PyCFunction_CAST(PyConduit_DataType_is_long_double),
     METH_NOARGS,
     "Returns if this DataType is a long double"},

    //-----------------------------------------------------------------------//
    {"is_string",
     _PyCFunction_CAST(PyConduit_DataType_is_string),
     METH_NOARGS,
     "Returns if this DataType is a string"},

    //-----------------------------------------------------------------------//
    {"is_char8_str",
     _PyCFunction_CAST(PyConduit_DataType_is_char8_str),
     METH_NOARGS,
     "Returns if this DataType is a char8 string"},

    //-----------------------------------------------------------------------//
    {"is_little_endian",
     _PyCFunction_CAST(PyConduit_DataType_is_little_endian),
     METH_NOARGS,
     "Returns if this DataType uses little endian byte ordering"},

    //-----------------------------------------------------------------------//
    {"is_big_endian",
     _PyCFunction_CAST(PyConduit_DataType_is_big_endian),
     METH_NOARGS,
     "Returns if this DataType uses big endian byte ordering"},

    //-----------------------------------------------------------------------//
    {"endianness_matches_machine",
     _PyCFunction_CAST(PyConduit_DataType_endianness_matches_machine),
     METH_NOARGS,
     "Returns if this DataType's endianness matches this machines endianness"},

    //-----------------------------------------------------------------------//
    {"to_string",
     _PyCFunction_CAST(PyConduit_DataType_to_string),
     METH_VARARGS| METH_KEYWORDS,
     "Returns a string representation of the DataType. "
     "Optionally takes protocol and spacing options. "
     "(Default protocol='yaml'.)"},
    //-----------------------------------------------------------------------//
    {"to_json",
     _PyCFunction_CAST(PyConduit_DataType_to_json),
     METH_VARARGS| METH_KEYWORDS,
     "Returns a JSON string representation of the DataType. "
     "Optionally takes protocol and spacing options."},
    //-----------------------------------------------------------------------//
    {"to_yaml",
     _PyCFunction_CAST(PyConduit_DataType_to_yaml),
     METH_VARARGS| METH_KEYWORDS,
     "Returns a YAML string representation of the DataType. "
     "Optionally takes protocol and spacing options."},
    //-----------------------------------------------------------------------//
    // --- static methods --- ///
    //-----------------------------------------------------------------------//
    //-----------------------------------------------------------------------//
    {"name_to_id",
     _PyCFunction_CAST(PyConduit_DataType_name_to_id),
     METH_VARARGS | METH_STATIC,
     "Converts a DataType name string to its corresponding id value"},
    //-----------------------------------------------------------------------//
    {"id_to_name",
     _PyCFunction_CAST(PyConduit_DataType_id_to_name),
     METH_VARARGS | METH_STATIC,
     "Converts a DataType id value to its corresponding name string"},
    //-----------------------------------------------------------------------//
    {"empty",
     _PyCFunction_CAST(PyConduit_DataType_empty),
     METH_NOARGS | METH_STATIC,
     "Creates a DataType for the empty role"},
    //-----------------------------------------------------------------------//
    {"object",
     _PyCFunction_CAST(PyConduit_DataType_object),
     METH_NOARGS | METH_STATIC,
     "Creates a DataType for the object role"},
    //-----------------------------------------------------------------------//
    {"list",
     _PyCFunction_CAST(PyConduit_DataType_list),
     METH_NOARGS | METH_STATIC,
     "Creates a DataType for the list role"},
    //-----------------------------------------------------------------------//
    {"int8",
     _PyCFunction_CAST(PyConduit_DataType_int8),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an int8 leaf"},
    //-----------------------------------------------------------------------//
    {"int16",
     _PyCFunction_CAST(PyConduit_DataType_int16),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an int16 leaf"},
    //-----------------------------------------------------------------------//
    {"int32",
     _PyCFunction_CAST(PyConduit_DataType_int32),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an int32 leaf"},
    //-----------------------------------------------------------------------//
    {"int64",
     _PyCFunction_CAST(PyConduit_DataType_int64),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an int64 leaf"},
    //-----------------------------------------------------------------------//
    {"uint8",
     _PyCFunction_CAST(PyConduit_DataType_uint8),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an uint8 leaf"},
    //-----------------------------------------------------------------------//
    {"uint16",
     _PyCFunction_CAST(PyConduit_DataType_uint16),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an uint16 leaf"},
    //-----------------------------------------------------------------------//
    {"uint32",
     _PyCFunction_CAST(PyConduit_DataType_uint32),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an uint32 leaf"},
    //-----------------------------------------------------------------------//
    {"uint64",
     _PyCFunction_CAST(PyConduit_DataType_uint64),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an uint64 leaf"},
    //-----------------------------------------------------------------------//
    {"float32",
     _PyCFunction_CAST(PyConduit_DataType_float32),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an float32 leaf"},
    //-----------------------------------------------------------------------//
    {"float64",
     _PyCFunction_CAST(PyConduit_DataType_float64),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an float64 leaf"},
    //-----------------------------------------------------------------------//
    {"char8_str",
     _PyCFunction_CAST(PyConduit_DataType_char8_str),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a char8_str string leaf"},
    //-----------------------------------------------------------------------//
    {"index_t",
     _PyCFunction_CAST(PyConduit_DataType_index_t),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for an index_t leaf"},
    //-----------------------------------------------------------------------//
    {"c_char",
     _PyCFunction_CAST(PyConduit_DataType_c_char),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native char leaf"},
    //-----------------------------------------------------------------------//
    {"c_short",
     _PyCFunction_CAST(PyConduit_DataType_c_short),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native short leaf"},
    //-----------------------------------------------------------------------//
    {"c_int",
     _PyCFunction_CAST(PyConduit_DataType_c_int),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native int leaf"},
    //-----------------------------------------------------------------------//
    {"c_long",
     _PyCFunction_CAST(PyConduit_DataType_c_long),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native long leaf"},
    //-----------------------------------------------------------------------//
    {"c_unsigned_char",
     _PyCFunction_CAST(PyConduit_DataType_c_unsigned_char),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native unsigned char leaf"},
    //-----------------------------------------------------------------------//
    {"c_unsigned_short",
     _PyCFunction_CAST(PyConduit_DataType_c_unsigned_short),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native unsigned short leaf"},
    //-----------------------------------------------------------------------//
    {"c_unsigned_int",
     _PyCFunction_CAST(PyConduit_DataType_c_unsigned_int),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native unsigned int leaf"},
    //-----------------------------------------------------------------------//
    {"c_unsigned_long",
     _PyCFunction_CAST(PyConduit_DataType_c_unsigned_long),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native unsigned long leaf"},
    //-----------------------------------------------------------------------//
    {"c_float",
     _PyCFunction_CAST(PyConduit_DataType_c_float),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native float leaf"},
    //-----------------------------------------------------------------------//
    {"c_double",
     _PyCFunction_CAST(PyConduit_DataType_c_double),
     METH_VARARGS | METH_KEYWORDS| METH_STATIC,
     "Creates a DataType for a c-style native double leaf"},
    // -- ids -- //
    //-----------------------------------------------------------------------//
     {"empty_id",
      _PyCFunction_CAST(PyConduit_DataType_empty_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::EMPTY_ID"},
    //-----------------------------------------------------------------------//
     {"object_id",
      _PyCFunction_CAST(PyConduit_DataType_object_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::OBJECT_ID"},
    //-----------------------------------------------------------------------//
     {"list_id",
      _PyCFunction_CAST(PyConduit_DataType_list_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::LIST_ID"},
    //-----------------------------------------------------------------------//
     {"char8_str_id",
      _PyCFunction_CAST(PyConduit_DataType_char8_str_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::CHAR8_STR_ID"},
    // -- signed int ids -- //
    //-----------------------------------------------------------------------//
     {"int8_id",
      _PyCFunction_CAST(PyConduit_DataType_int8_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::INT8_ID"},
    //-----------------------------------------------------------------------//
     {"int16_id",
      _PyCFunction_CAST(PyConduit_DataType_int16_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::INT16_ID"},
    //-----------------------------------------------------------------------//
     {"int32_id",
      _PyCFunction_CAST(PyConduit_DataType_int32_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::INT32_ID"},
    //-----------------------------------------------------------------------//
     {"int64_id",
      _PyCFunction_CAST(PyConduit_DataType_int64_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::INT64_ID"},
    // -- uint ids -- //
    //-----------------------------------------------------------------------//
     {"uint8_id",
      _PyCFunction_CAST(PyConduit_DataType_uint8_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::UINT8_ID"},
    //-----------------------------------------------------------------------//
     {"uint16_id",
      _PyCFunction_CAST(PyConduit_DataType_uint16_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::UINT16_ID"},
    //-----------------------------------------------------------------------//
     {"uint32_id",
      _PyCFunction_CAST(PyConduit_DataType_uint32_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::UINT32_ID"},
    //-----------------------------------------------------------------------//
     {"uint64_id",
      _PyCFunction_CAST(PyConduit_DataType_uint64_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::UINT64_ID"},
    // -- fp ids -- //
    //-----------------------------------------------------------------------//
     {"float32_id",
      _PyCFunction_CAST(PyConduit_DataType_float32_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::FLOAT32_ID"},
    //-----------------------------------------------------------------------//
     {"float64_id",
      _PyCFunction_CAST(PyConduit_DataType_float64_id),
      METH_NOARGS | METH_STATIC,
      "Returns DataType::FLOAT64_ID"},
    //-----------------------------------------------------------------------//
    // end DataType methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, 0, NULL}
};



//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef Py_LIMITED_API
static PyType_Slot PyConduit_DataType_SLOTS[]  = {
  {Py_tp_dealloc, (void*) PyConduit_DataType_dealloc},
  {Py_tp_str,     (void*) PyConduit_DataType_str},
  {Py_tp_methods, (void*) PyConduit_DataType_METHODS},
  {Py_tp_init,    (void*) PyConduit_DataType_init},
  {Py_tp_new,     (void*) PyConduit_DataType_new},
  {0,0},
};

static PyType_Spec PyConduit_DataType_SPEC =
{
   "DataType",                                /* tp_name */
   sizeof(PyConduit_DataType),                /* tp_basicsize */
   0,                                         /* tp_itemsize */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
   PyConduit_DataType_SLOTS,                  /* tp_slots */
};
#else
static PyTypeObject PyConduit_DataType_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "DataType",
   sizeof(PyConduit_DataType),  /* tp_basicsize */
   0, /* tp_itemsize */
   (destructor)PyConduit_DataType_dealloc, /* tp_dealloc */
   // tp_print was removed in Python 3.9, its now used as
   // tp_vectorcall_offset (which we also don't use here)
   0, /* tp_print or tp_vectorcall_offset */
   0, /* tp_getattr */
   0, /* tp_setattr */
   0, /* tp_compare or tp_reserved pr tp_as_async*/
   0, /* tp_repr */
   0, /* tp_as_number */
   0, /* tp_as_sequence */
   0, /* as_mapping */
   0, /* hash */
   0, /* call */
   (reprfunc)PyConduit_DataType_str, /* str */
   0, /* getattro */
   0, /* setattro */
   0, /* asbuffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* flags */
   "Conduit DataType objects",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   0, /* iter */
   0, /* iternext */
   PyConduit_DataType_METHODS, /* METHODS */
   0, /* MEMBERS */
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   (initproc)PyConduit_DataType_init, /* tp_init */
   0, /* alloc */
   PyConduit_DataType_new, /* new */
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
static PyConduit_DataType *
PyConduit_DataType_Python_Create()
{
    PyTypeObject* type = NULL;
    Set_PyTypeObject_Macro(type,PyConduit_DataType_TYPE);
    return (PyConduit_DataType*)PyType_GenericAlloc(type,0);
}

//---------------------------------------------------------------------------//
static int
PyConduit_DataType_Check(PyObject *obj)
{
    PyTypeObject* type = NULL;
    Set_PyTypeObject_Macro(type,PyConduit_DataType_TYPE);
    return (PyObject_TypeCheck(obj, type));
}


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
// Generator Object
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Generator_new(PyTypeObject *type,
                        PyObject*, // args -- unused
                        PyObject*) // kwds -- unused
{
    PyConduit_Generator *self = (PyConduit_Generator*)PyType_GenericAlloc(type,0);

    if (self)
    {
        self->generator = 0;
    }

    return ((PyObject*)self);
}

//---------------------------------------------------------------------------//
static void
PyConduit_Generator_dealloc(PyConduit_Generator *self)
{
    if(self->generator != NULL)
    {
        delete self->generator;
    }

    #ifdef Py_LIMITED_API
    freefunc tp_free = ((freefunc)PyType_GetSlot(Py_TYPE((PyObject*)self), Py_tp_free));
    tp_free((PyObject*)self);
    #else
    Py_TYPE(self)->tp_free((PyObject*)self);
    #endif
}


//---------------------------------------------------------------------------//
static int
PyConduit_Generator_init(PyConduit_Generator *self,
                         PyObject *args,
                         PyObject *kwargs)
{
    // TODO: Support "data" arg
    static const char *kwlist[] = {"schema",
                                   "protocol",
                                   // "data",
                                    NULL};

    char *schema = NULL;
    char *protocol = NULL;


    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s|s",
                                     const_cast<char**>(kwlist),
                                     &schema,
                                     &protocol))
    {
        return -1;
    }

    if(protocol == NULL)
    {
        self->generator = new Generator(std::string(schema));
    }
    else if(protocol != NULL)
    {
        self->generator = new Generator(std::string(schema),
                                        std::string(protocol));
    }

    return 0;

}



//-----------------------------------------------------------------------------
static PyObject *
PyConduit_Generator_walk(PyConduit_Generator *self,
                         PyObject *args,
                         PyObject *kwargs)
{

    static const char *kwlist[] = {"node",
                                   "schema",
                                    NULL};

     PyObject *py_node   = NULL;
     PyObject *py_schema = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|OO",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &py_schema))
    {
        return NULL;
    }


    if(py_node != NULL)
    {
        if(!PyConduit_Node_Check(py_node))
        {
            PyErr_SetString(PyExc_TypeError,
                            "Generator::walk 'node' argument must be a "
                            "Conduit::Node");
            return NULL;
        }
    }

    if(py_schema != NULL)
    {
        if(!PyConduit_Schema_Check(py_schema))
        {
            PyErr_SetString(PyExc_TypeError,
                            "Generator::walk 'schema' argument must be a "
                            "Conduit::Schema");
            return NULL;
        }
    }

    if(py_node != NULL)
    {
        Node *node_ptr = ((PyConduit_Node*)py_node)->node;
        self->generator->walk(*node_ptr);
    }

    if(py_schema != NULL)
    {
        Schema *schema_ptr = ((PyConduit_Schema*)py_schema)->schema;
        self->generator->walk(*schema_ptr);
    }

    Py_RETURN_NONE;
}

//-----------------------------------------------------------------------------
static PyObject *
PyConduit_Generator_walk_external(PyConduit_Generator *self,
                                  PyObject *args,
                                  PyObject *kwargs)
{

    static const char *kwlist[] = {"node",
                                    NULL};

     PyObject *py_node   = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "O",
                                     const_cast<char**>(kwlist),
                                     &py_node))
    {
        return NULL;
    }

    if(!PyConduit_Node_Check(py_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "Generator::walk_external 'node' argument must "
                        "be a Conduit::Node");
        return NULL;
    }


    Node *node_ptr = ((PyConduit_Node*)py_node)->node;
    self->generator->walk_external(*node_ptr);

    Py_RETURN_NONE;
}



//----------------------------------------------------------------------------//
// Generator methods table
//----------------------------------------------------------------------------//
static PyMethodDef PyConduit_Generator_METHODS[] = {
    //-----------------------------------------------------------------------//
    {"walk",
     _PyCFunction_CAST(PyConduit_Generator_walk),
     METH_VARARGS | METH_KEYWORDS,
     "Use Generator to parse a JSON schema into passed 'node' or 'schema'"},
    {"walk_external",
     _PyCFunction_CAST(PyConduit_Generator_walk_external),
     METH_VARARGS | METH_KEYWORDS,
     "Use Generator to parse a JSON schema into passed 'node' using"
     " external data"},
    //-----------------------------------------------------------------------//
    // end Generator methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, 0, NULL}
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//


#ifdef Py_LIMITED_API
static PyType_Slot PyConduit_Generator_SLOTS[]  = {
  {Py_tp_dealloc, (void*) PyConduit_Generator_dealloc},
  {Py_tp_methods, (void*) PyConduit_Generator_METHODS},
  {Py_tp_init,    (void*) PyConduit_Generator_init},
  {Py_tp_new,     (void*) PyConduit_Generator_new},
  {0,0},
};

static PyType_Spec PyConduit_Generator_SPEC =
{
   "Generator",                                /* tp_name */
   sizeof(PyConduit_Generator),                /* tp_basicsize */
   0,                                          /* tp_itemsize */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   PyConduit_Generator_SLOTS,                  /* tp_slots */
};

#else
static PyTypeObject PyConduit_Generator_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "Generator",
   sizeof(PyConduit_Generator),  /* tp_basicsize */
   0, /* tp_itemsize */
   (destructor)PyConduit_Generator_dealloc, /* tp_dealloc */
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
   0, /* str */
   0, /* getattro */
   0, /* setattro */
   0, /* asbuffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,     /* flags */
   "Conduit Generator objects",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   0, /* iter */
   0, /* iternext */
   PyConduit_Generator_METHODS, /* METHODS */
   0, /* MEMBERS */
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   (initproc)PyConduit_Generator_init,  /* tp_init */
   0, /* alloc */
   PyConduit_Generator_new,  /* new */
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
static int
PyConduit_Generator_Check(PyObject* obj)
{
    PyTypeObject* type = NULL;
    Set_PyTypeObject_Macro(type, PyConduit_Generator_TYPE);
    return (PyObject_TypeCheck(obj, type));
}



//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
// Schema Object
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_new(PyTypeObject* type,
                     PyObject*, // args -- unused
                     PyObject*) // kwds -- unused
{

    PyConduit_Schema *self = (PyConduit_Schema*)PyType_GenericAlloc(type,0);

    if (self)
    {
        self->schema = 0;
        self->python_owns = 0;
    }

    return ((PyObject*)self);
}

//---------------------------------------------------------------------------//
static int
PyConduit_Schema_init(PyConduit_Schema* self,
                      PyObject* args,
                      PyObject* kwds)
{
     static const char *kwlist[] = {"value", NULL};
     PyObject* value = NULL;
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &value))
    {
         return (0);
    }

     if (value)
     {
         if (PyConduit_Schema_Check(value))
         {
             self->schema = new Schema(*((PyConduit_Schema*)value)->schema);
         }
         else if (PyString_Check(value))
         {
             char *cstr = PyString_AsString(value);
             self->schema = new Schema(cstr);
             PyString_AsString_Cleanup(cstr);
         }
         else
         {
             PyErr_SetString(PyExc_TypeError, "Invalid initializer for schema");
             return (-1);
         }
     }
     else
     {
         self->schema = new Schema();
     }

     //self->python_owns = 1;

     return (0);

}

//---------------------------------------------------------------------------//
static void
PyConduit_Schema_dealloc(PyConduit_Schema* self)
{
    if (self->python_owns && self->schema != NULL)
    {
        delete self->schema;
    }

    #ifdef Py_LIMITED_API
    freefunc tp_free = ((freefunc)PyType_GetSlot(Py_TYPE((PyObject*)self), Py_tp_free));
    tp_free((PyObject*)self);
    #else
    Py_TYPE(self)->tp_free((PyObject*)self);
    #endif
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_str(PyConduit_Schema *self)
{
   return Py_BuildValue("s", self->schema->to_string().c_str());
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_python_owns(PyConduit_Schema *self)
{
    if(self->python_owns)
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_python_detach(PyConduit_Schema *self)
{
    self->python_owns = 0;
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_python_attach(PyConduit_Schema *self)
{
    self->python_owns = 1;
    Py_RETURN_NONE;
}


//-----------------------------------------------------------------------------
//
/// Information Methods
//
//-----------------------------------------------------------------------------
    // const DataType &dtype() const
    // index_t         total_strided_bytes() const;
    // index_t         total_bytes_compact() const;
    // index_t         element_index(index_t idx) const
    // Schema         *parent() const
    // bool            is_root() const

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_dtype(PyConduit_Schema *self)
{
    PyConduit_DataType *retval = PyConduit_DataType_Python_Create();
    retval->dtype = self->schema->dtype();
    return (PyObject*)retval;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_total_strided_bytes(PyConduit_Schema *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->schema->total_strided_bytes());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_total_bytes_compact(PyConduit_Schema *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->schema->total_bytes_compact());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_element_index(PyConduit_Schema *self,
                               PyObject *args)
{
    Py_ssize_t idx;

    if (!PyArg_ParseTuple(args, "n", &idx))
    {
        PyErr_SetString(PyExc_TypeError,
                "index must be a signed integer");
        return NULL;
    }

    return PyLong_FromSsize_t((Py_ssize_t)self->schema->element_index(idx));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_is_root(PyConduit_Schema *self)
{
    if(self->schema->is_root())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject*
PyConduit_Schema_parent(PyConduit_Schema* self)
{
    if(self->schema->is_root())
    {
        Py_RETURN_NONE;
    }
    else
    {
        // python owned == 0 (at least for this instance)
        return PyConduit_Schema_Python_Wrap(self->schema->parent(),0);
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_get_item(PyConduit_Schema* self,
                         PyObject* key)
{
    if (!PyString_Check(key))
    {
        PyErr_SetString(PyExc_TypeError, "Key must be a string");
        return (NULL);
    }

    PyObject* retval = NULL;
    char* ckey = PyString_AsString(key);

    retval = PyConduit_Schema_Python_Wrap(self->schema->fetch_ptr(ckey),
                                          0); // schema owns

    PyString_AsString_Cleanup(ckey);

    return (retval);
}

//---------------------------------------------------------------------------//
static int
PyConduit_Schema_set_item(PyConduit_Schema *self,
                          PyObject *key,
                          PyObject *value)
{
    if (!PyString_Check(key))
    {
        PyErr_SetString(PyExc_TypeError, "Key must be a string");
        return (-1);
    }
    // value must be a data type or schema

    char* ckey = PyString_AsString(key);

    if(PyConduit_Schema_Check(value))
    {
        self->schema->fetch_ptr(ckey)->set(*((PyConduit_Schema*)value)->schema);
    }
    else if(PyConduit_DataType_Check(value))
    {
        self->schema->fetch_ptr(ckey)->set(((PyConduit_DataType*)value)->dtype);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError,
                        "value must be a Conduit Schema or DataType");
        return (-1);
    }


    PyString_AsString_Cleanup(ckey);
    return (0);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_set(PyConduit_Schema* self,
                     PyObject* args)
{
    PyObject* value = NULL;

    if (!PyArg_ParseTuple(args, "O", &value))
    {
         return (NULL);
    }

    if(PyConduit_Schema_Check(value))
    {
        self->schema->set(*((PyConduit_Schema*)value)->schema);
    }
    else if(PyConduit_DataType_Check(value))
    {
        self->schema->set(((PyConduit_DataType*)value)->dtype);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError,
                        "value must be a Conduit Schema or DataType");
        return NULL;
    }

    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_add_child(PyConduit_Schema *self,
                           PyObject *args,
                           PyObject *kwargs)
{
    PyObject* retval = NULL;
    const char *name = NULL;

    static const char *kwlist[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s",
                                     const_cast<char**>(kwlist),
                                     &name))
    {
        return (NULL);
    }

    try
    {
        retval = PyConduit_Schema_Python_Wrap(&(*self->schema).add_child(std::string(name)),
                                              0); // schema owns
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    return retval;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_child(PyConduit_Schema *self,
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject* retval = NULL;

    Py_ssize_t idx = -1;
    const char *name = NULL;

    static const char *kwlist[] = {"index","name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|ns",
                                     const_cast<char**>(kwlist),
                                     &idx, &name))
    {
        return (NULL);
    }

    try
    {
        if(name != NULL)
        {
            retval = PyConduit_Schema_Python_Wrap(&(*self->schema).child(std::string(name)),
                                                  0); // schema owns
        }
        else if(idx >=0)
        {
            retval = PyConduit_Schema_Python_Wrap(&(*self->schema).child(idx),
                                                  0); // schema owns
        }
        else
        {
            PyErr_SetString(PyExc_Exception,
                            "expected name(string) or index(positive integer)");
            return NULL;
        }
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    return retval;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_rename_child(PyConduit_Schema *self,
                              PyObject *args,
                              PyObject *kwargs)
{
    const char *curr_name = NULL;
    const char *new_name = NULL;

    static const char *kwlist[] = {"current_name","new_name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "ss",
                                     const_cast<char**>(kwlist),
                                     &curr_name, &new_name))
    {
        return (NULL);
    }


    try
    {
        self->schema->rename_child(std::string(curr_name),
                                   std::string(new_name));

    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_remove_child(PyConduit_Schema *self,
                              PyObject *args,
                              PyObject *kwargs)
{
    const char *name = NULL;

    static const char *kwlist[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s",
                                     const_cast<char**>(kwlist),
                                     &name))
    {
        return (NULL);
    }

    try
    {
        self->schema->remove_child(std::string(name));
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_remove(PyConduit_Schema *self,
                        PyObject *args,
                        PyObject *kwargs)
{
    Py_ssize_t idx=-1;
    const char *path = NULL;

    static const char *kwlist[] = {"index","path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|ns",
                                     const_cast<char**>(kwlist),
                                     &idx, &path))
    {
        return (NULL);
    }

    try
    {
        if(path != NULL)
        {
            self->schema->remove(std::string(path));
        }
        else if(idx >= 0)
        {
            self->schema->remove(idx);
        }
        else
        {
            PyErr_SetString(PyExc_Exception,
                            "expected path(string) or index(positive integer)");
            return NULL;
        }
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_number_of_children(PyConduit_Schema *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->schema->number_of_children());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_name(PyConduit_Schema *self)
{
    return PyString_FromString(self->schema->name().c_str());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_path(PyConduit_Schema *self)
{
    return PyString_FromString(self->schema->path().c_str());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_has_path(PyConduit_Schema *self,
                          PyObject* args)
{
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path))
    {
        PyErr_SetString(PyExc_TypeError, "path must be a string");
        return NULL;
    }

    if(self->schema->has_path(std::string(path)))
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_has_child(PyConduit_Schema *self,
                           PyObject* args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s", &name))
    {
        PyErr_SetString(PyExc_TypeError, "name must be a string");
        return NULL;
    }

    if(self->schema->has_child(std::string(name)))
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_child_names(PyConduit_Schema *self)
{
    /// TODO: I think there is a faster way in the Python CAPI
    /// since we know the size of the list.
    PyObject *retval = PyList_New(0);

    if(self->schema->dtype().is_object())
    {
        const std::vector<std::string> &cld_names = self->schema->child_names();
        for (std::vector<std::string>::const_iterator itr = cld_names.begin();
             itr < cld_names.end(); ++itr)
        {
            PyList_Append(retval, PyString_FromString( (*itr).c_str()));
        };

    }
    return retval;
}


//-----------------------------------------------------------------------------
//
/// Transformation Methods
//
//-----------------------------------------------------------------------------
//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_compact_to(PyConduit_Schema* self,
                     PyObject* args)
{
    PyObject* value = NULL;

    if (!PyArg_ParseTuple(args, "O", &value))
    {
         return (NULL);
    }

    if(PyConduit_Schema_Check(value))
    {
        self->schema->compact_to(*((PyConduit_Schema*)value)->schema);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError,
                        "value must be a Conduit Schema");
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_to_string(PyConduit_Schema* self,
                           PyObject* args,
                           PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string protocol = "yaml";
    std::string pad = " ";
    std::string eoe = "\n";

    char *protocol_c_str = NULL;
    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"protocol",
                                   "indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &protocol_c_str,
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(protocol_c_str != NULL)
    {
        protocol = std::string(protocol_c_str);
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->schema->to_string_stream(oss,
                                       protocol,
                                       indent,
                                       depth,
                                       pad,
                                       eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_to_json(PyConduit_Schema* self,
                         PyObject* args,
                         PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string pad = " ";
    std::string eoe = "\n";

    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->schema->to_json_stream(oss,
                                     indent,
                                     depth,
                                     pad,
                                     eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_to_yaml(PyConduit_Schema* self,
                         PyObject* args,
                         PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string pad = " ";
    std::string eoe = "\n";

    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->schema->to_yaml_stream(oss,
                                     indent,
                                     depth,
                                     pad,
                                     eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}




//-----------------------------------------------------------------------------
//
/// Basic I/O methods
//
//-----------------------------------------------------------------------------
    // void            save(const std::string &ofname,
    //                      bool detailed=true,
    //                      index_t indent=2,
    //                      index_t depth=0,
    //                      const std::string &pad=" ",
    //                      const std::string &eoe="\n") const;
    //
    // void            load(const std::string &ifname);


//----------------------------------------------------------------------------//
// Schema methods table
//----------------------------------------------------------------------------//
static PyMethodDef PyConduit_Schema_METHODS[] = {
    //-----------------------------------------------------------------------//
    {"python_owns",
     _PyCFunction_CAST(PyConduit_Schema_python_owns),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"python_attach",
     _PyCFunction_CAST(PyConduit_Schema_python_attach),
      METH_NOARGS,
      "{todo}"},
    //-----------------------------------------------------------------------//
    {"python_detach",
     _PyCFunction_CAST(PyConduit_Schema_python_detach),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"dtype",
    _PyCFunction_CAST(PyConduit_Schema_dtype),
    METH_NOARGS,
    "{todo}"},
    //-----------------------------------------------------------------------//
    {"total_strided_bytes",
    _PyCFunction_CAST(PyConduit_Schema_total_strided_bytes),
    METH_NOARGS,
    "{todo}"},
    //-----------------------------------------------------------------------//
    {"total_bytes_compact",
    _PyCFunction_CAST(PyConduit_Schema_total_bytes_compact),
    METH_NOARGS,
    "{todo}"},
    //-----------------------------------------------------------------------//
    {"element_index",
    _PyCFunction_CAST(PyConduit_Schema_element_index),
    METH_VARARGS,
    "{todo}"},
    //-----------------------------------------------------------------------//
    {"is_root",
    _PyCFunction_CAST(PyConduit_Schema_is_root),
    METH_NOARGS,
    "{todo}"},
    //-----------------------------------------------------------------------//
    {"parent",
    _PyCFunction_CAST(PyConduit_Schema_parent),
    METH_NOARGS,
    "{todo}"},
    //-----------------------------------------------------------------------//
    {"set",
     _PyCFunction_CAST(PyConduit_Schema_set),
     METH_VARARGS,
     "Sets the schema"},
    //-----------------------------------------------------------------------//
    {"child_names",
     _PyCFunction_CAST(PyConduit_Schema_child_names),
     METH_NOARGS, 
     "Returns a list with this schema's child names"},
    //-----------------------------------------------------------------------//
    {"add_child",
    _PyCFunction_CAST(PyConduit_Schema_add_child),
    METH_VARARGS | METH_KEYWORDS,
    "Add a new direct child (name is not parsed as path, allows slashes) "},
    //-----------------------------------------------------------------------//
    {"child",
    _PyCFunction_CAST(PyConduit_Schema_child),
    METH_VARARGS | METH_KEYWORDS,
    "Access existing direct child by index or name"},
    //-----------------------------------------------------------------------//
    {"remove_child",
    _PyCFunction_CAST(PyConduit_Schema_remove_child),
    METH_VARARGS | METH_KEYWORDS,
    "Remove a direct child by index or name"},
    //-----------------------------------------------------------------------//
    {"rename_child",
    _PyCFunction_CAST(PyConduit_Schema_rename_child),
    METH_VARARGS | METH_KEYWORDS,
    "Rename an existing child (object role)"},
    //-----------------------------------------------------------------------//
    {"remove",
    _PyCFunction_CAST(PyConduit_Schema_remove),
    METH_VARARGS | METH_KEYWORDS,
    "Remove a child by index or path"},
    //-----------------------------------------------------------------------//
    {"number_of_children",
      _PyCFunction_CAST(PyConduit_Schema_number_of_children),
      METH_NOARGS, 
      "Number of child schemas"},
    //-----------------------------------------------------------------------//
    {"name",
      _PyCFunction_CAST(PyConduit_Schema_name),
      METH_NOARGS, 
      "This schema's name"},
    //-----------------------------------------------------------------------//
    {"path",
      _PyCFunction_CAST(PyConduit_Schema_path),
      METH_NOARGS, 
      "Path to this schema"},
    //-----------------------------------------------------------------------//
    {"has_path",
     _PyCFunction_CAST(PyConduit_Schema_has_path),
     METH_VARARGS, 
     "Returns if this schema has the given path"},
    //-----------------------------------------------------------------------//
    {"has_child",
     _PyCFunction_CAST(PyConduit_Schema_has_child),
     METH_VARARGS, 
     "Returns if this schema has the given child"},
    //-----------------------------------------------------------------------//
    {"compact_to",
     _PyCFunction_CAST(PyConduit_Schema_compact_to),
     METH_VARARGS| METH_KEYWORDS,
     "Compacts this schema into the passed schema instance."},
    //-----------------------------------------------------------------------//
    {"to_string",
     _PyCFunction_CAST(PyConduit_Schema_to_string),
     METH_VARARGS| METH_KEYWORDS,
     "Returns a string representation of the schema. "
     "Optionally takes protocol and spacing options. "
     "(Default protocol='yaml'.)"},
    //-----------------------------------------------------------------------//
    {"to_json",
     _PyCFunction_CAST(PyConduit_Schema_to_json),
     METH_VARARGS| METH_KEYWORDS,
     "Returns a JSON string representation of the schema. "
     "Optionally takes protocol and spacing options."},
    //-----------------------------------------------------------------------//
    {"to_yaml",
     _PyCFunction_CAST(PyConduit_Schema_to_yaml),
     METH_VARARGS| METH_KEYWORDS,
     "Returns a YAML string representation of the schema. "
     "Optionally takes protocol and spacing options."},
    //-----------------------------------------------------------------------//
    // end Schema methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, 0, NULL}
};

//---------------------------------------------------------------------------//
#ifndef Py_LIMITED_API
// The limited API does not include PyMappingMethods,
// The corresponding slots are set directly in _SLOTS
static PyMappingMethods PyConduit_Schema_as_mapping = {
   (lenfunc)0,    // len operator is not supported
   (binaryfunc)PyConduit_Schema_get_item,
   (objobjargproc)PyConduit_Schema_set_item
};
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef Py_LIMITED_API
static PyType_Slot PyConduit_Schema_SLOTS[]  = {
  {Py_tp_dealloc, (void*) PyConduit_Schema_dealloc},
  {Py_tp_methods, (void*) PyConduit_Schema_METHODS},
  {Py_tp_init,    (void*) PyConduit_Schema_init},
  {Py_tp_new,     (void*) PyConduit_Schema_new},
  {Py_tp_str,     (void*) PyConduit_Schema_str},
  {Py_mp_subscript, (void*) PyConduit_Schema_get_item},
  {Py_mp_ass_subscript, (void*) PyConduit_Schema_set_item},
  {0,0},
};

static PyType_Spec PyConduit_Schema_SPEC =
{
   "Schema",                                   /* tp_name */
   sizeof(PyConduit_Schema),                   /* tp_basicsize */
   0,                                          /* tp_itemsize */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   PyConduit_Schema_SLOTS,                     /* tp_slots */
};

#else

static PyTypeObject PyConduit_Schema_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "Schema",
   sizeof(PyConduit_Schema),  /* tp_basicsize */
   0, /* tp_itemsize */
   (destructor)PyConduit_Schema_dealloc, /* tp_dealloc */
   // tp_print was removed in Python 3.9, its now used as
   // tp_vectorcall_offset (which we also don't use here)
   0, /* tp_print or tp_vectorcall_offset */
   0, /* tp_getattr */
   0, /* tp_setattr */
   0, /* tp_compare */
   0, /* tp_repr */
   0, /* tp_as_number */
   0, /* tp_as_sequence */
   &PyConduit_Schema_as_mapping, /* as_mapping */
   0, /* hash */
   0, /* call */
   (reprfunc)PyConduit_Schema_str, /* str */
   0, /* getattro */
   0, /* setattro */
   0, /* asbuffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,     /* flags */
   "Conduit schema objects",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   0, /* iter */
   0, /* iternext */
   PyConduit_Schema_METHODS, /* METHODS */
   0, /* MEMBERS */
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   (initproc)PyConduit_Schema_init,
   0, /* alloc */
   PyConduit_Schema_new,                                   /* new */
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
static PyObject *
PyConduit_NodeIterator_new(PyTypeObject *type,
                            PyObject *args,
                            PyObject *kwds)
{
    static const char *kwlist[] = {"value", NULL};
    PyObject* value = NULL;
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &value))
    {
        return (NULL);
    }

    PyConduit_DataType *self = (PyConduit_DataType*)PyType_GenericAlloc(type,0);
    return ((PyObject*)self);
}

//---------------------------------------------------------------------------//
static int
PyConduit_NodeIterator_init(PyConduit_NodeIterator* self,
                            PyObject* args,
                            PyObject* kwds)
{
    /// TODO: args and kwargs
    static const char *kwlist[] = {"value", NULL};
    PyObject* value = NULL;
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &value))

    {
        return 0;
    }
    if (value)
    {
        if (PyConduit_Node_Check(value))
        {
            self->itr = NodeIterator(((PyConduit_Node*)value)->node);
        }
    }

    return (0);
}

//---------------------------------------------------------------------------//
static void
PyConduit_NodeIterator_dealloc(PyConduit_NodeIterator *self)
{
    #ifdef Py_LIMITED_API
    freefunc tp_free = ((freefunc)PyType_GetSlot(Py_TYPE((PyObject*)self), Py_tp_free));
    tp_free((PyObject*)self);
    #else
    Py_TYPE(self)->tp_free((PyObject*)self);
    #endif
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_str(PyConduit_NodeIterator *self)
{
    Node n;
    self->itr.info(n);
    return Py_BuildValue("s", n.to_string().c_str());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_iternext(PyObject *self)
{
    PyConduit_NodeIterator *py_nitr = (PyConduit_NodeIterator *)self;

    if(py_nitr->itr.has_next())
    {
        py_nitr->itr.next();
        Py_INCREF(self);
        return self;
    }
    else
    {
        // Raise StopIteration
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_name(PyConduit_NodeIterator *self)
{
    return (Py_BuildValue("s", self->itr.name().c_str()));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_index(PyConduit_NodeIterator *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->itr.index());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_node(PyConduit_NodeIterator *self)
{
    return PyConduit_Node_Python_Wrap(&(self->itr.node()),0);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_to_front(PyConduit_NodeIterator *self)
{
    self->itr.to_front();
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_has_next(PyConduit_NodeIterator *self)
{
    if(self->itr.has_next())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_next(PyConduit_NodeIterator *self)
{
    if(self->itr.has_next())
    {
        Node &n = self->itr.next();
        return PyConduit_Node_Python_Wrap(&n,0);
    }
    else
    {
        // if done return None
        Py_RETURN_NONE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_peek_next(PyConduit_NodeIterator *self)
{
    if(self->itr.has_next())
    {
        Node &n = self->itr.peek_next();
        return PyConduit_Node_Python_Wrap(&n,0);
    }
    else
    {
        // if done return None
        Py_RETURN_NONE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_has_previous(PyConduit_NodeIterator *self)
{
    if(self->itr.has_previous())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_previous(PyConduit_NodeIterator *self)
{
    if(self->itr.has_previous())
    {
        Node &n = self->itr.previous();
        return PyConduit_Node_Python_Wrap(&n,0);
    }
    else
    {
        // if done return None
        Py_RETURN_NONE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_peek_previous(PyConduit_NodeIterator *self)
{
    if(self->itr.has_previous())
    {
        Node &n = self->itr.peek_previous();
        return PyConduit_Node_Python_Wrap(&n,0);
    }
    else
    {
        // if done return None
        Py_RETURN_NONE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_to_back(PyConduit_NodeIterator *self)
{
    self->itr.to_back();
    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_NodeIterator_info(PyConduit_NodeIterator *self)
{
    //create and return a node with the result of info
    PyConduit_Node *retval = (PyConduit_Node *)PyConduit_Node_Python_Create();
    self->itr.info(*retval->node);
    return (PyObject*)retval;
}

//----------------------------------------------------------------------------//
// NodeIterator methods table
//----------------------------------------------------------------------------//
static PyMethodDef PyConduit_NodeIterator_METHODS[] = {
    //-----------------------------------------------------------------------//
    {"name",
     _PyCFunction_CAST(PyConduit_NodeIterator_name),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"index",
     _PyCFunction_CAST(PyConduit_NodeIterator_index),
     METH_NOARGS,
     "{todo}"},
     //-----------------------------------------------------------------------//
     {"node",
      _PyCFunction_CAST(PyConduit_NodeIterator_node),
      METH_NOARGS,
      "{todo}"},
    //-----------------------------------------------------------------------//
    {"to_front",
     _PyCFunction_CAST(PyConduit_NodeIterator_to_front),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"has_next",
     _PyCFunction_CAST(PyConduit_NodeIterator_has_next),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"next",
     _PyCFunction_CAST(PyConduit_NodeIterator_next),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"peek_next",
     _PyCFunction_CAST(PyConduit_NodeIterator_peek_next),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"has_previous",
     _PyCFunction_CAST(PyConduit_NodeIterator_has_previous),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"previous",
     _PyCFunction_CAST(PyConduit_NodeIterator_previous),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"peek_previous",
     _PyCFunction_CAST(PyConduit_NodeIterator_peek_previous),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"to_back",
     _PyCFunction_CAST(PyConduit_NodeIterator_to_back),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"info",
     _PyCFunction_CAST(PyConduit_NodeIterator_info),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    // end NodeIterator methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, 0, NULL}
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef Py_LIMITED_API
static PyType_Slot PyConduit_NodeIterator_SLOTS[]  = {
  {Py_tp_dealloc,  (void*) PyConduit_NodeIterator_dealloc},
  {Py_tp_methods,  (void*) PyConduit_NodeIterator_METHODS},
  {Py_tp_init,     (void*) PyConduit_NodeIterator_init},
  {Py_tp_new,      (void*) PyConduit_NodeIterator_new},
  {Py_tp_str,      (void*) PyConduit_NodeIterator_str},
  {Py_tp_iter,     (void*) PyConduit_NodeIterator_iter},
  {Py_tp_iternext, (void*) PyConduit_NodeIterator_iternext},
  {0,0},
};

static PyType_Spec PyConduit_NodeIterator_SPEC =
{
   "NodeIterator",                             /* tp_name */
   sizeof(PyConduit_NodeIterator),             /* tp_basicsize */
   0,                                          /* tp_itemsize */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   PyConduit_NodeIterator_SLOTS,               /* tp_slots */
};

#else
static PyTypeObject PyConduit_NodeIterator_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "Iterator",
   sizeof(PyConduit_NodeIterator),  /* tp_basicsize */
   0, /* tp_itemsize */
   (destructor)PyConduit_NodeIterator_dealloc,   /* tp_dealloc */
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
   (reprfunc)PyConduit_NodeIterator_str,                         /* str */
   0, /* getattro */
   0, /* setattro */
   0, /* asbuffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,     /* flags */
   "Conduit Iterator objects",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   PyConduit_NodeIterator_iter, /* iter */
   PyConduit_NodeIterator_iternext, /* iternext */
   PyConduit_NodeIterator_METHODS, /* METHODS */
   0, /* MEMBERS */
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   (initproc)PyConduit_NodeIterator_init, /* tp_init */
   0, /* alloc */
   PyConduit_NodeIterator_new,   /* new */
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
static PyConduit_NodeIterator *
PyConduit_NodeIterator_Python_Create()
{
    PyTypeObject* type = NULL;
    Set_PyTypeObject_Macro(type, PyConduit_NodeIterator_TYPE);
    return (PyConduit_NodeIterator*)PyType_GenericAlloc(type,0);
}


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
// conduit:::Node class
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <class T>
static void
PyConduit_Fill_DataArray_From_PyArray(DataArray<T> &conduit_array,
                                      PyArrayObject *numpy_array)
{
    // proper strided iteration, adapted from:
    //https://docs.scipy.org/doc/numpy-1.10.0/reference/c-api.iterator.html


    /* Handle zero-sized arrays specially */
    if (PyArray_SIZE(numpy_array) == 0)
    {
        // nothing to copy in this case!
        return;
    }

    /*
     * Create and use an iterator to copy out the data to our conduit array
     *   flag NPY_ITER_READONLY
     *     - The array is never written to.
     *   flag NPY_ITER_EXTERNAL_LOOP
     *     - Inner loop is done outside the iterator for efficiency.
     *   flag NPY_ITER_NPY_ITER_REFS_OK
     *     - Reference types are acceptable.
     *   order NPY_KEEPORDER
     *     - Visit elements in memory order, regardless of strides.
     *       This is good for performance when the specific order
     *       elements are visited is unimportant.
     *   casting NPY_NO_CASTING
     *     - No casting is required for this operation.
     */
     NpyIter* iter = NpyIter_New(numpy_array,
                                 NPY_ITER_READONLY|
                                 NPY_ITER_EXTERNAL_LOOP|
                                 NPY_ITER_REFS_OK,
                                 NPY_KEEPORDER,
                                 NPY_NO_CASTING,
                                 NULL);
    if (iter == NULL)
    {
        // TODO ERROR!
    }

    /*
     * The iternext function gets stored in a local variable
     * so it can be called repeatedly in an efficient manner.
     */
    NpyIter_IterNextFunc *iternext = NpyIter_GetIterNext(iter, NULL);
    if (iternext == NULL)
    {
        NpyIter_Deallocate(iter);
        // TODO ERROR!
    }

    // The location of the data pointer which the iterator may update
    char** dataptr = NpyIter_GetDataPtrArray(iter);
    // The location of the stride which the iterator may update
    npy_intp* strideptr = NpyIter_GetInnerStrideArray(iter);
    // The location of the inner loop size which the iterator may update
    npy_intp* innersizeptr = NpyIter_GetInnerLoopSizePtr(iter);

    int idx=0;
    do
    {
        // Get the inner loop data/stride/count values
        char* data = *dataptr;
        npy_intp stride = *strideptr;
        npy_intp count = *innersizeptr;

        // This is a typical inner loop for NPY_ITER_EXTERNAL_LOOP
        while (count--)
        {
            conduit_array[idx] = ((T*)data)[0];
            data += stride;
            idx++;
        }
        // Increment the iterator to the next inner loop
    }
    while(iternext(iter));

    // TODO, add just in case consistency check?
    // npy_int num_ele = (npy_int)conduit_array.number_of_elements();
    // if(num_ele != idx)
    // {
    //  // this doesn't match our expectations
    // }

    NpyIter_Deallocate(iter);
}

//---------------------------------------------------------------------------//
// begin Node python special methods
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_new(PyTypeObject* type,
                   PyObject* args,
                   PyObject* kwds)
{
    static const char *kwlist[] = {"value", NULL};
    PyObject* value = NULL;
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &value))
    {
        return (NULL);
    }

    PyConduit_Node* self = (PyConduit_Node*)PyType_GenericAlloc(type,0);

    if (self)
    {
        self->node = 0;
        self->python_owns = 0;
    }

    return ((PyObject*)self);
}

//---------------------------------------------------------------------------//
static int
PyConduit_Node_init(PyConduit_Node* self,
                    PyObject* args,
                    PyObject* kwds)
{
    static const char *kwlist[] = {"value", NULL};
    PyObject* value = NULL;
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwds,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &value))
    {
        return 0;
    }

    self->node = new Node();
    self->python_owns = 1;

    if (value)
    {
        return (PyConduit_Node_Set_From_Python(*self->node, value));
    }
    else
    {
        return 0;
    }
}

//---------------------------------------------------------------------------//
static void
PyConduit_Node_dealloc(PyConduit_Node* self)
{
    if(self->python_owns)
    {
       delete self->node;
    }

    #ifdef Py_LIMITED_API
    freefunc tp_free = ((freefunc)PyType_GetSlot(Py_TYPE((PyObject*)self), Py_tp_free));
    tp_free((PyObject*)self);
    #else
    Py_TYPE(self)->tp_free((PyObject*)self);
    #endif
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_str(PyConduit_Node* self)
{
   return (Py_BuildValue("s", self->node->to_summary_string().c_str()));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_repr(PyConduit_Node* self)
{
   return (Py_BuildValue("s", self->node->to_summary_string().c_str()));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_GetItem(PyConduit_Node* self,
                       PyObject* key)
{
    PyObject* retval = NULL;

    if(PyString_Check(key))
    {
        char* ckey = PyString_AsString(key);

        if(self->node->has_path(ckey))
        {
            Node& node = (*self->node)[ckey];
            retval = PyConduit_Convert_Node_To_Python(node);
        }
        else
        {
            retval = PyConduit_Node_Python_Wrap(&(*self->node)[ckey],0);
        }

        PyString_AsString_Cleanup(ckey);
    }
    else if(PyNumber_Check(key))
    {
        Py_ssize_t idx = PyNumber_AsSsize_t(key,NULL);
        Node& node = (*self->node)[idx];
        retval = PyConduit_Convert_Node_To_Python(node);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError,
                        "Key must be a string or integer index");
    }

    return (retval);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_python_owns(PyConduit_Node *self)
{
    if(self->python_owns)
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_python_detach(PyConduit_Node *self)
{
    self->python_owns = 0;
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_python_attach(PyConduit_Node *self)
{
    self->python_owns = 1;
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
// end Node python special methods
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_value(PyConduit_Node* self)
{
    PyObject* retval = NULL;
    retval = PyConduit_Convert_Node_To_Python(*self->node);
    return (retval);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_generate(PyConduit_Node* self,
                        PyObject* args)
{
    /// TODO: sigs to support
    /// schema
    /// schema, protocol

    /// schema, data
    /// schema, protocol, data

    PyObject   *py_gen      = NULL;
    const char *schema = NULL;

    if(PyArg_ParseTuple(args, "O", &py_gen))
    {
        if(!PyConduit_Generator_Check(py_gen))
        {
            PyErr_SetString(PyExc_TypeError,
                            "Node::generate() argument must be a "
                            "Conduit::Generator or a JSON schema string.");
            return NULL;
        }

         Generator *gen_ptr = ((PyConduit_Generator*)py_gen)->generator;
         self->node->generate(*gen_ptr);
    }
    else if(PyArg_ParseTuple(args, "s", &schema))
    {
        self->node->generate(std::string(schema));
    }
    else
    {
        PyErr_SetString(PyExc_TypeError,
                        "Node::generate() argument must be a "
                        "Conduit::Generator or a JSON schema string.");
        return NULL;

    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_parse(PyConduit_Node *self,
                     PyObject *args,
                     PyObject *kwargs)
{
    const char *text = NULL;
    const char *protocol  = NULL;

    // support:
    // text
    // text, protocol

    static const char *kwlist[] = {"text","protocol", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s|s",
                                     const_cast<char**>(kwlist),
                                     &text,&protocol))
    {
        return NULL;
    }

    std::string text_str(text);
    std::string protocol_str;

    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }

    try
    {
        self->node->parse(text_str,
                          protocol_str);
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
PyConduit_Node_save(PyConduit_Node *self,
                    PyObject *args,
                    PyObject *kwargs)
{
    const char *path      = NULL;
    const char *protocol  = NULL;

    // support:
    // path
    // path, protocol

    static const char *kwlist[] = {"path","protocol", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s|s",
                                     const_cast<char**>(kwlist),
                                     &path,&protocol))
    {
        return NULL;
    }

    std::string path_str(path);
    // keep blank, allow conduit to detect based on file name
    std::string protocol_str("");

    if(protocol != NULL)
    {
        protocol_str = std::string(protocol);
    }

    try
    {
        self->node->save(path_str,protocol_str);
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
PyConduit_Node_load(PyConduit_Node *self,
                    PyObject *args,
                    PyObject *kwargs)
{
    PyObject   *py_schema = NULL;
    const char *path      = NULL;
    const char *protocol  = NULL;

    // support:
    // path
    // path, schema
    // path, protocol

    static const char *kwlist[] = {"path","schema","protocol", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s|Os",
                                     const_cast<char**>(kwlist),
                                     &path,&py_schema, &protocol))
    {
        return NULL;
    }


    if(py_schema != NULL)
    {
        if(!PyConduit_Schema_Check(py_schema))
        {
            PyErr_SetString(PyExc_TypeError,
                            "Node::load 'schema' argument must be a "
                            "Conduit::Schema");
            return NULL;
        }
    }

    std::string path_str(path);

    if(py_schema != NULL)
    {
        Schema *schema_ptr = ((PyConduit_Schema*)py_schema)->schema;

        try
        {
            self->node->load(path_str,
                             *schema_ptr);
        }
        catch(conduit::Error &e)
        {
            PyErr_SetString(PyExc_IOError,
                            e.message().c_str());
            return NULL;
        }
    }
    else
    {
    // keep blank, allow conduit to detect based on file name
        std::string protocol_str("");

        if( protocol != NULL)
        {
            protocol_str =  std::string(protocol);
        }

        try
        {
            self->node->load(path_str,
                             protocol_str);
        }
        catch(conduit::Error &e)
        {
            PyErr_SetString(PyExc_IOError,
                            e.message().c_str());
            return NULL;
        }
    }


    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_mmap(PyConduit_Node* self,
                    PyObject* args)
{
    /// TODO: sigs to support via kwargs: path, or path and schema
    const char *ibase;
    if (!PyArg_ParseTuple(args, "s", &ibase))
    {
        PyErr_SetString(PyExc_TypeError, "Load file path must be a string");
        return NULL;
    }

    self->node->mmap(std::string(ibase));
    Py_RETURN_NONE;
}

//-----------------------------------------------------------------------------
//
// -- Node Entry Access --
//
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_fetch(PyConduit_Node* self,
                     PyObject* args)
{
     const char *path;
     PyObject* retval = NULL;
     if (!PyArg_ParseTuple(args, "s", &path))
     {
         PyErr_SetString(PyExc_TypeError, "path must be a string");
         return NULL;
     }

    try
    {
        retval = PyConduit_Node_Python_Wrap(&(*self->node).fetch(path),
                                            0); // node owns
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }
    return (retval);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_fetch_existing(PyConduit_Node* self,
                     PyObject* args)
{
     const char *path;
     PyObject* retval = NULL;
     if (!PyArg_ParseTuple(args, "s", &path))
     {
         PyErr_SetString(PyExc_TypeError, "path must be a string");
         return NULL;
     }

    try
    {
        retval = PyConduit_Node_Python_Wrap(&(*self->node).fetch_existing(path),
                                            0); // node owns
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    return (retval);
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_add_child(PyConduit_Node *self,
                         PyObject *args,
                         PyObject *kwargs)
{
    PyObject* retval = NULL;
    const char *name = NULL;

    static const char *kwlist[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s",
                                     const_cast<char**>(kwlist),
                                     &name))
    {
        return (NULL);
    }

    try
    {
        retval = PyConduit_Node_Python_Wrap(&(*self->node).add_child(std::string(name)),
                                            0); // node owns
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    return retval;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_child(PyConduit_Node *self,
                     PyObject *args,
                     PyObject *kwargs)
{
    PyObject* retval = NULL;

    Py_ssize_t idx = -1;
    const char *name = NULL;

    static const char *kwlist[] = {"index","name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|ns",
                                     const_cast<char**>(kwlist),
                                     &idx, &name))
    {
        return (NULL);
    }

    try
    {
        if(name != NULL)
        {
            retval = PyConduit_Node_Python_Wrap(&(*self->node).child(std::string(name)),
                                                0); // node owns
        }
        else if(idx >=0)
        {
            retval = PyConduit_Node_Python_Wrap(&(*self->node).child(idx),
                                                0); // node owns
        }
        else
        {
            PyErr_SetString(PyExc_Exception,
                            "expected name(string) or index(positive integer)");
            return NULL;
        }
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    return retval;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_number_of_children(PyConduit_Node *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->node->number_of_children());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_name(PyConduit_Node *self)
{
    return PyString_FromString(self->node->name().c_str());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_path(PyConduit_Node *self)
{
    return PyString_FromString(self->node->path().c_str());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_has_path(PyConduit_Node *self,
                       PyObject* args)
{
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path))
    {
        PyErr_SetString(PyExc_TypeError, "Path must be a string");
        return NULL;
    }

    if(self->node->has_path(std::string(path)))
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_has_child(PyConduit_Node *self,
                         PyObject* args)
{
    const char *child_name;

    if (!PyArg_ParseTuple(args, "s", &child_name))
    {
        PyErr_SetString(PyExc_TypeError, "child_name must be a string");
        return NULL;
    }

    if(self->node->has_child(std::string(child_name)))
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_child_names(PyConduit_Node *self)
{
    /// TODO: I think there is a faster way in the Python CAPI
    /// since we know the size of the list.
    PyObject *retval = PyList_New(0);

    if(self->node->dtype().is_object())
    {
        const std::vector<std::string> &cld_names = self->node->child_names();
        for (std::vector<std::string>::const_iterator itr = cld_names.begin();
             itr < cld_names.end(); ++itr)
        {
            PyList_Append(retval, PyString_FromString( (*itr).c_str()));
        };

    }
    return retval;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_append(PyConduit_Node* self)
{
    return  PyConduit_Node_Python_Wrap(&(self->node->append()),0);
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_remove(PyConduit_Node *self,
                      PyObject *args,
                      PyObject *kwargs)
{
    Py_ssize_t idx=-1;
    const char *path = NULL;

    static const char *kwlist[] = {"index","path", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|ns",
                                     const_cast<char**>(kwlist),
                                     &idx, &path))
    {
        return (NULL);
    }

    try
    {
        if(path != NULL)
        {
            self->node->remove(std::string(path));
        }
        else if(idx >= 0)
        {
            self->node->remove(idx);
        }
        else
        {
            PyErr_SetString(PyExc_Exception,
                            "expected path(string) or index(positive integer)");
            return NULL;
        }
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_remove_child(PyConduit_Node *self,
                            PyObject *args,
                            PyObject *kwargs)
{
    const char *name = NULL;

    static const char *kwlist[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "s",
                                     const_cast<char**>(kwlist),
                                     &name))
    {
        return (NULL);
    }

    try
    {
        self->node->remove_child(std::string(name));
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_rename_child(PyConduit_Node *self,
                            PyObject *args,
                            PyObject *kwargs)
{
    const char *curr_name = NULL;
    const char *new_name = NULL;

    static const char *kwlist[] = {"current_name","new_name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "ss",
                                     const_cast<char**>(kwlist),
                                     &curr_name, &new_name))
    {
        return (NULL);
    }


    try
    {
        self->node->rename_child(std::string(curr_name),
                                 std::string(new_name));

    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}



//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_diff(PyConduit_Node* self,
                    PyObject* args,
                    PyObject* kwargs)
{
     PyObject *py_other = NULL;
     PyObject *py_info  = NULL;
     double    eps      = CONDUIT_EPSILON;

     static const char *kwlist[] = {"other",
                                    "info",
                                    "epsilon",
                                    NULL};

     if (!PyArg_ParseTupleAndKeywords(args,
                                      kwargs,
                                      "OO|d",
                                      const_cast<char**>(kwlist),
                                      &py_other, &py_info, &eps))
     {
         return (NULL);
     }

     if(!PyConduit_Node_Check(py_other))
     {
         PyErr_SetString(PyExc_TypeError,
                         "Node diff 'other' argument must be a "
                         "Conduit Node");
         return NULL;
     }

     if(!PyConduit_Node_Check(py_info))
     {
         PyErr_SetString(PyExc_TypeError,
                         "Node diff 'info' argument must be a "
                         "Conduit Node");
         return NULL;
     }

    Node *other_ptr = ((PyConduit_Node*)py_other)->node;
    Node *info_ptr  = ((PyConduit_Node*)py_info)->node;

    if( self->node->diff(*other_ptr,*info_ptr,eps) )
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_diff_compatible(PyConduit_Node* self,
                               PyObject* args,
                               PyObject* kwargs)
{
     PyObject *py_other = NULL;
     PyObject *py_info  = NULL;
     double    eps      = CONDUIT_EPSILON;

     static const char *kwlist[] = {"other",
                                    "info",
                                    "epsilon",
                                    NULL};

     if (!PyArg_ParseTupleAndKeywords(args,
                                      kwargs,
                                      "OO|d",
                                      const_cast<char**>(kwlist),
                                      &py_other, &py_info, &eps))
     {
         return (NULL);
     }

     if(!PyConduit_Node_Check(py_other))
     {
         PyErr_SetString(PyExc_TypeError,
                         "Node diff_compatible 'other' argument must be a "
                         "Conduit Node");
         return NULL;
     }

     if(!PyConduit_Node_Check(py_info))
     {
         PyErr_SetString(PyExc_TypeError,
                         "Node diff_compatible 'info' argument must be a "
                         "Conduit Node");
         return NULL;
     }

    Node *other_ptr = ((PyConduit_Node*)py_other)->node;
    Node *info_ptr  = ((PyConduit_Node*)py_info)->node;

    if( self->node->diff_compatible(*other_ptr,*info_ptr,eps) )
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}



//-----------------------------------------------------------------------------
//
// -- Node information methods --
//
//-----------------------------------------------------------------------------
// schema access
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_schema(PyConduit_Node *self)
{
    // python_owns = 0
    return (PyConduit_Schema_Python_Wrap(self->node->schema_ptr(),0));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_dtype(PyConduit_Node *self)
{
    PyConduit_DataType *retval = PyConduit_DataType_Python_Create();
    retval->dtype = self->node->dtype();
    return (PyObject*)retval;
}

//---------------------------------------------------------------------------//
// parent access
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_is_root(PyConduit_Node *self)
{
    if(self->node->is_root())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject*
PyConduit_Node_parent(PyConduit_Node* self)
{
    if(self->node->is_root())
    {
        Py_RETURN_NONE;
    }
    else
    {
        return PyConduit_Node_Python_Wrap(self->node->parent(),0);
    }
}

//---------------------------------------------------------------------------//
//memory space info
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_total_strided_bytes(PyConduit_Node *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->node->total_strided_bytes());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_total_bytes_compact(PyConduit_Node *self)
{
    return PyLong_FromSsize_t((Py_ssize_t)self->node->total_bytes_compact());
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_is_compact(PyConduit_Node *self)
{
    if(self->node->is_compact())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_info(PyConduit_Node *self)
{
    PyConduit_Node *retval = (PyConduit_Node*)PyConduit_Node_Python_Create();
    self->node->info(*retval->node);
    return (PyObject*)retval;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_describe(PyConduit_Node *self,
                        PyObject* args,
                        PyObject* kwargs)
{
    PyObject *py_opts = NULL;
    static const char *kwlist[] = {"opts", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &py_opts))
    {
        return (NULL);
    }

    Node *node_opts_ptr = NULL;

    if(py_opts != NULL)
    {
        if(!PyConduit_Node_Check(py_opts))
        {
            PyErr_SetString(PyExc_TypeError,
                            "Node::describe 'opts' argument must be a "
                            "Conduit::Node");
            return NULL;
        }

        node_opts_ptr = ((PyConduit_Node*)py_opts)->node;
    }

    PyConduit_Node *retval = (PyConduit_Node*)PyConduit_Node_Python_Create();
    if(node_opts_ptr == NULL)
    {
        self->node->describe(*retval->node);
    }
    else
    {
        self->node->describe(*node_opts_ptr,
                             *retval->node);
    }

    return (PyObject*)retval;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_print_detailed(PyConduit_Node *self)
{
    std::ostringstream oss;
    self->node->to_string_stream(oss,"conduit_json");
    std::fprintf(stdout,"%s",oss.str().c_str());
    Py_RETURN_NONE;
}



//---------------------------------------------------------------------------//
static int PyConduit_Node_SetItem(PyConduit_Node *self,
                                  PyObject *key,
                                  PyObject *value)
{
    if (!PyString_Check(key))
    {
        PyErr_SetString(PyExc_TypeError, "Key must be a string");
        return (-1);
    }

    char* ckey = PyString_AsString(key);
    Node& node = (*self->node)[ckey];
    PyString_AsString_Cleanup(ckey);
    return (PyConduit_Node_Set_From_Python(node, value));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_iter(PyObject *self)
{
    PyConduit_Node *py_n = (PyConduit_Node *)self;

    PyConduit_NodeIterator *retval = NULL;
    retval = PyConduit_NodeIterator_Python_Create();
    retval->itr =  py_n->node->children();

    return ((PyObject *)retval);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_reset(PyConduit_Node *self)
{
    self->node->reset();
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_move(PyConduit_Node *self,
                    PyObject *args,
                    PyObject *kwargs)
{
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"other",
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
                        "'other' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &n_other = *PyConduit_Node_Get_Node_Ptr(py_node);

    self->node->move(n_other);
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_swap(PyConduit_Node *self,
                    PyObject *args,
                    PyObject *kwargs)
{
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"other",
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
                        "'other' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &n_other = *PyConduit_Node_Get_Node_Ptr(py_node);

    self->node->swap(n_other);
    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_set(PyConduit_Node* self,
                   PyObject* args)
{
    PyObject* py_value = NULL;
    PyObject* py_buff  = NULL;

    if (!PyArg_ParseTuple(args, "O|O", &py_value,&py_buff))
    {
         return (NULL);
    }

    // check for schema and buffer case
    if(PyConduit_Schema_Check(py_value) && py_buff != NULL )
    {
        if( !PyObject_CheckBuffer(py_buff) )
        {
            PyErr_SetString(PyExc_TypeError,
            "Node set with schema requires buffer argument");
            return NULL;
        }

        Schema &schema = *((PyConduit_Schema*)py_value)->schema;

        Py_buffer buff_view;
        // PyObject_GetBuffer(py_buff, &buff_view, PyBUF_SIMPLE);
        if( PyObject_GetBuffer(py_buff, &buff_view, PyBUF_SIMPLE) < 0)
        {
            PyErr_SetString(PyExc_TypeError,
            "set, failed to obtain Py_buffer from passed Python buffer");
            return NULL;
        }

        unsigned char *ptr = reinterpret_cast<unsigned char*>(buff_view.buf);

        self->node->set(schema,ptr);
        PyBuffer_Release(&buff_view);
        Py_RETURN_NONE;
    }


    if (PyConduit_Node_Set_From_Python(*self->node, py_value))
    {
         return (NULL);
    }
    else
    {
        Py_RETURN_NONE;
    }
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_set_external(PyConduit_Node* self,
                            PyObject* args)
{
    PyObject* py_value = NULL;
    PyObject* py_buff  = NULL;

    if( !PyArg_ParseTuple(args, "O|O", &py_value, &py_buff) ||
        ( !PyConduit_Node_Check(py_value) && // not a node
          !PyConduit_Schema_Check(py_value) && // not a schema
          !PyArray_Check(py_value) ) ) // not a numpy array
    {
        PyErr_SetString(PyExc_TypeError,
        "set_external requires a numpy array, conduit Node, or conduit Schema and Buffer");
        return NULL;
    }

    // schema + buffer cases
    if(PyConduit_Schema_Check(py_value))
    {
        if( py_buff == NULL || !PyObject_CheckBuffer(py_buff))
        {
            PyErr_SetString(PyExc_TypeError,
            "set_external requires a numpy array, conduit Node, or conduit Schema and Buffer");
            return NULL;
        }

        Schema &schema = *((PyConduit_Schema*)py_value)->schema;

        Py_buffer buff_view;
        if( PyObject_GetBuffer(py_buff, &buff_view, PyBUF_SIMPLE) < 0)
        {
            PyErr_SetString(PyExc_TypeError,
            "set_external, failed to obtain Py_buffer from passed Python buffer");
            return NULL;
        }
        unsigned char *ptr = reinterpret_cast<unsigned char*>(buff_view.buf);

        self->node->set_external(schema,ptr);
        PyBuffer_Release(&buff_view);
        Py_RETURN_NONE;
    }

    // node case
    if(PyConduit_Node_Check(py_value))
    {
        Node &n_other = *PyConduit_Node_Get_Node_Ptr(py_value);
        self->node->update_external(n_other);
        Py_RETURN_NONE;
    }

    // buffer cases (scheam)

    // numpy array case
    PyArray_Descr *desc = PyArray_DESCR((PyArrayObject*)py_value);
    PyArrayObject *py_arr = (PyArrayObject*)py_value;
    npy_intp num_ele = PyArray_SIZE(py_arr);
    index_t offset = 0;
    index_t stride = (index_t) PyArray_STRIDE(py_arr, 0);
    int nd = PyArray_NDIM(py_arr);

    if (nd > 1)
    {
        PyErr_SetString(PyExc_TypeError,
                        "set_external does not handle multidimensional numpy"
                        " arrays or multidimensional complex strided views"
                        " into numpy arrays."
                        " Views that are effectively 1D-strided are"
                        "supported.");
        return (NULL);
    }

    Node& node = *self->node;

    switch (desc->type_num)
    {
        case NPY_UINT8 :
        {
            uint8 *numpy_data = (uint8*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_UINT16 :
        {
            uint16 *numpy_data = (uint16*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_UINT32 :
        {
            uint32 *numpy_data = (uint32*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_UINT64 :
        {
            uint64 *numpy_data = (uint64*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_INT8 :
        {
            int8 *numpy_data = (int8*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_INT16 :
        {
            int16 *numpy_data = (int16*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_INT32 :
        {
            int32 *numpy_data = (int32*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_INT64 :
        {
            int64 *numpy_data = (int64*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_FLOAT32 :
        {
            float32 *numpy_data = (float32*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        case NPY_FLOAT64 :
        {
            float64 *numpy_data = (float64*)PyArray_BYTES(py_arr);
            node.set_external(numpy_data, num_ele, offset, stride);
            break;
        }
        default :
        {
            PyErr_SetString(PyExc_TypeError, "Unsupported type");
            return (NULL);
        }
    }

    Py_RETURN_NONE;

}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_set_path(PyConduit_Node* self,
                        PyObject* args)
{
    PyObject* value = NULL;
    PyObject *path  = NULL;

    if (!PyArg_ParseTuple(args, "OO", &path, &value))
    {
         return (NULL);
    }

    if (PyConduit_Node_SetItem(self, path, value))
    {
        return (NULL);
    }
    else
    {
        Py_RETURN_NONE;
    }
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_compact_to(PyConduit_Node* self,
                          PyObject *args,
                          PyObject *kwargs)
{
    PyObject   *py_node  = NULL;

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

    Node &n_dest = *PyConduit_Node_Get_Node_Ptr(py_node);

    self->node->compact_to(n_dest);
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_update(PyConduit_Node* self,
                      PyObject *args,
                      PyObject *kwargs)
{
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"other",
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
                        "'other' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &n_other = *PyConduit_Node_Get_Node_Ptr(py_node);

    self->node->update(n_other);
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_update_compatible(PyConduit_Node* self,
                                 PyObject *args,
                                 PyObject *kwargs)
{
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"other",
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
                        "'other' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &n_other = *PyConduit_Node_Get_Node_Ptr(py_node);

    self->node->update_compatible(n_other);
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_update_external(PyConduit_Node* self,
                               PyObject *args,
                               PyObject *kwargs)
{
    PyObject   *py_node  = NULL;

    static const char *kwlist[] = {"other",
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
                        "'other' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &n_other = *PyConduit_Node_Get_Node_Ptr(py_node);

    self->node->update_external(n_other);
    Py_RETURN_NONE;
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_to_string(PyConduit_Node* self,
                         PyObject* args,
                         PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string protocol = "yaml";
    std::string pad = " ";
    std::string eoe = "\n";

    char *protocol_c_str = NULL;
    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"protocol",
                                   "indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &protocol_c_str,
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(protocol_c_str != NULL)
    {
        protocol = std::string(protocol_c_str);
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->node->to_string_stream(oss,
                                     protocol,
                                     indent,
                                     depth,
                                     pad,
                                     eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_to_summary_string(PyConduit_Node* self,
                                 PyObject* args,
                                 PyObject* kwargs)
{
    PyObject   *py_opts  = NULL;
    static const char *kwlist[] = {"opts",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|O",
                                     const_cast<char**>(kwlist),
                                     &py_opts))
    {
        return (NULL);
    }

    if(py_opts != NULL && !PyConduit_Node_Check(py_opts))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'opts' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node opts;
    Node *opts_ptr = &opts;

    if(py_opts != NULL)
    {
        opts_ptr = PyConduit_Node_Get_Node_Ptr(py_opts);
    }

    std::ostringstream oss;

    try
    {
        self->node->to_summary_string_stream(oss,
                                             *opts_ptr);

    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_to_json(PyConduit_Node* self,
                       PyObject* args,
                       PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string protocol = "json";
    std::string pad = " ";
    std::string eoe = "\n";

    char *protocol_c_str = NULL;
    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"protocol",
                                   "indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &protocol_c_str,
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(protocol_c_str != NULL)
    {
        protocol = std::string(protocol_c_str);
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->node->to_json_stream(oss,
                                   protocol,
                                   indent,
                                   depth,
                                   pad,
                                   eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_to_yaml(PyConduit_Node* self,
                       PyObject* args,
                       PyObject* kwargs)
{

    Py_ssize_t indent = 2;
    Py_ssize_t depth  = 0;

    std::string protocol = "yaml";
    std::string pad = " ";
    std::string eoe = "\n";

    char *protocol_c_str = NULL;
    char *pad_c_str = NULL;
    char *eoe_c_str = NULL;

    static const char *kwlist[] = {"protocol",
                                   "indent",
                                   "depth",
                                   "pad",
                                   "eoe",
                                    NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|snnss",
                                     const_cast<char**>(kwlist),
                                     &protocol_c_str,
                                     &indent,
                                     &depth,
                                     &pad_c_str,
                                     &eoe_c_str))
    {
        return NULL;
    }

    if(protocol_c_str != NULL)
    {
        protocol = std::string(protocol_c_str);
    }

    if(pad_c_str != NULL)
    {
        pad = std::string(pad_c_str);
    }

    if(eoe_c_str != NULL)
    {
        eoe = std::string(eoe_c_str);
    }

    std::ostringstream oss;

    try
    {
        self->node->to_yaml_stream(oss,
                                   protocol,
                                   indent,
                                   depth,
                                   pad,
                                   eoe);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_IOError,
                        e.message().c_str());
        return NULL;
    }

    return (Py_BuildValue("s", oss.str().c_str()));
}


//----------------------------------------------------------------------------//
// Endianness methods
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_endian_swap(PyConduit_Node *self,
                           PyObject *args)
{
    Py_ssize_t value;

    if (!PyArg_ParseTuple(args, "n", &value))
    {
        PyErr_SetString(PyExc_TypeError,
            "endianness must be a signed integer");
        return NULL;
    }


    self->node->endian_swap(value);


    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_endian_swap_to_machine_default(PyConduit_Node *self)
{
    self->node->endian_swap_to_machine_default();
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_endian_swap_to_little(PyConduit_Node *self)
{
    self->node->endian_swap_to_little();
    Py_RETURN_NONE;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_endian_swap_to_big(PyConduit_Node *self)
{
    self->node->endian_swap_to_big();
     Py_RETURN_NONE;
}

//----------------------------------------------------------------------------//
// Node methods table
//----------------------------------------------------------------------------//
static PyMethodDef PyConduit_Node_METHODS[] = {
    //-----------------------------------------------------------------------//
    {"python_owns",
     _PyCFunction_CAST(PyConduit_Node_python_owns),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"python_attach",
     _PyCFunction_CAST(PyConduit_Node_python_attach),
      METH_NOARGS,
      "{todo}"},
    //-----------------------------------------------------------------------//
    {"python_detach",
     _PyCFunction_CAST(PyConduit_Node_python_detach),
     METH_NOARGS,
     "{todo}"},
    //-----------------------------------------------------------------------//
    {"reset",
     _PyCFunction_CAST(PyConduit_Node_reset),
     METH_NOARGS,
     "Reset the name"},
    //-----------------------------------------------------------------------//
    {"set",
     _PyCFunction_CAST(PyConduit_Node_set),
     METH_VARARGS,
     "Sets the node"},
    //-----------------------------------------------------------------------//
    {"set_external",
     _PyCFunction_CAST(PyConduit_Node_set_external),
     METH_VARARGS,
     "Sets the node's data to an external numpy array"},
    //-----------------------------------------------------------------------//
    {"compact_to",
     _PyCFunction_CAST(PyConduit_Node_compact_to),
     METH_VARARGS | METH_KEYWORDS, 
     "Compact the contents of this node into the destination node"},
    //-----------------------------------------------------------------------//
    {"update",
     _PyCFunction_CAST(PyConduit_Node_update),
     METH_VARARGS | METH_KEYWORDS, 
     "Update node with the contents of another node"},
    //-----------------------------------------------------------------------//
    {"update_compatible",
     _PyCFunction_CAST(PyConduit_Node_update_compatible),
     METH_VARARGS | METH_KEYWORDS, 
     "Update node with the compatible contents of another node"},
    //-----------------------------------------------------------------------//
    {"update_external",
     _PyCFunction_CAST(PyConduit_Node_update_external),
     METH_VARARGS | METH_KEYWORDS, 
     "Update node with to externally point to another node's contents"},
    //-----------------------------------------------------------------------//
    {"fetch",
     _PyCFunction_CAST(PyConduit_Node_fetch),
     METH_VARARGS, 
     "Fetches the node at a given path"},
    //-----------------------------------------------------------------------//
    {"fetch_existing",
     _PyCFunction_CAST(PyConduit_Node_fetch_existing),
     METH_VARARGS, 
     "Fetches an existing node at a given path, error on bad path"},
    //-----------------------------------------------------------------------//
    {"child",
     _PyCFunction_CAST(PyConduit_Node_child),
      METH_VARARGS | METH_KEYWORDS, 
     "Retrieves the child node at a given index or with given name"},
    //-----------------------------------------------------------------------//
    {"number_of_children",
      _PyCFunction_CAST(PyConduit_Node_number_of_children),
      METH_NOARGS, 
      "Number of child nodes"},
    //-----------------------------------------------------------------------//
    {"name",
      _PyCFunction_CAST(PyConduit_Node_name),
      METH_NOARGS, 
      "This node's name"},
    //-----------------------------------------------------------------------//
    {"path",
      _PyCFunction_CAST(PyConduit_Node_path),
      METH_NOARGS, 
      "Path to this node"},
    //-----------------------------------------------------------------------//
    {"has_path",
     _PyCFunction_CAST(PyConduit_Node_has_path),
     METH_VARARGS, 
     "Returns if this node has the given path"},
    //-----------------------------------------------------------------------//
    {"has_child",
     _PyCFunction_CAST(PyConduit_Node_has_child),
     METH_VARARGS, 
     "Returns if this node has the given child"},
    //-----------------------------------------------------------------------//
    {"child_names",
     _PyCFunction_CAST(PyConduit_Node_child_names),
     METH_NOARGS, 
     "Returns a list with this node's child names"},
    //-----------------------------------------------------------------------//
    {"info",
     _PyCFunction_CAST(PyConduit_Node_info),
     METH_VARARGS, 
     "Returns a node populated with the memory space details for this node"},
    //-----------------------------------------------------------------------//
    {"describe",
     _PyCFunction_CAST(PyConduit_Node_describe),
     METH_VARARGS | METH_KEYWORDS, 
     "Returns a node that mirrors the current Node, however each leaf is"
     " replaced by summary stats and a truncated display of the values."},
    //-----------------------------------------------------------------------//
    {"print_detailed",
     _PyCFunction_CAST(PyConduit_Node_print_detailed),
     METH_NOARGS, 
     "Prints detailed json description of this node to standard out"},
    //-----------------------------------------------------------------------//
    {"append",
     _PyCFunction_CAST(PyConduit_Node_append),
     METH_NOARGS, 
     "Appends a node (coarse to conduit list)"},
    //-----------------------------------------------------------------------//
    {"add_child",
     _PyCFunction_CAST(PyConduit_Node_add_child),
     METH_VARARGS | METH_KEYWORDS,
     "Add a new direct child (name is not parsed as path, allows slashes) "},
    //-----------------------------------------------------------------------//
    {"child",
     _PyCFunction_CAST(PyConduit_Node_child),
     METH_VARARGS | METH_KEYWORDS,
     "Access existing direct child by index or name"},
    //-----------------------------------------------------------------------//
    {"move", 
     _PyCFunction_CAST(PyConduit_Node_move),
     METH_VARARGS | METH_KEYWORDS, 
     "Move the contents of passed node into this node."
     " Passed node is empty after the move."},
    //-----------------------------------------------------------------------//
    {"swap", 
     _PyCFunction_CAST(PyConduit_Node_swap),
     METH_VARARGS | METH_KEYWORDS, 
     "Swap contents of this node with those of the passed node"},
    //-----------------------------------------------------------------------//
    {"remove", 
     _PyCFunction_CAST(PyConduit_Node_remove),
     METH_VARARGS | METH_KEYWORDS, 
     "Remove node at a given index or path."},
    //-----------------------------------------------------------------------//
    {"rename_child",
     _PyCFunction_CAST(PyConduit_Node_rename_child),
     METH_VARARGS | METH_KEYWORDS,
     "Rename an existing child (object role)"},
    //-----------------------------------------------------------------------//
    {"remove_child",
     _PyCFunction_CAST(PyConduit_Node_remove_child),
     METH_VARARGS | METH_KEYWORDS,
     "Remove direct child by name or index"},
    //-----------------------------------------------------------------------//
    {"diff", 
     _PyCFunction_CAST(PyConduit_Node_diff),
     METH_VARARGS | METH_KEYWORDS, 
     "Diff node with another node."},
    //-----------------------------------------------------------------------//
    {"diff_compatible", 
     _PyCFunction_CAST(PyConduit_Node_diff_compatible),
     METH_VARARGS | METH_KEYWORDS, 
     "Diff node with compatible parts of another node."},
    //-----------------------------------------------------------------------//
    {"value",
     _PyCFunction_CAST(PyConduit_Node_value),
     METH_NOARGS, 
     "Value access for leaf nodes"},
    //-----------------------------------------------------------------------//
    {"generate",
     _PyCFunction_CAST(PyConduit_Node_generate),
     METH_VARARGS,  // will become kwargs
     "Generate a node"},
    //-----------------------------------------------------------------------//
    {"parse",
     _PyCFunction_CAST(PyConduit_Node_parse),
     METH_VARARGS | METH_KEYWORDS, 
     "Creates a node tree by parsing a YAML or JSON string"},
    //-----------------------------------------------------------------------//
    {"save",
     _PyCFunction_CAST(PyConduit_Node_save),
     METH_VARARGS | METH_KEYWORDS, 
     "Saves a node to a file pair"},
    //-----------------------------------------------------------------------//
    {"load",
     _PyCFunction_CAST(PyConduit_Node_load),
     METH_VARARGS | METH_KEYWORDS, 
     "Loads a node from a file pair, file with schema, or file with protocol"},
    //-----------------------------------------------------------------------//
    {"mmap",
     _PyCFunction_CAST(PyConduit_Node_mmap),
     METH_VARARGS, // will become kwargs
     "Memory Maps a node from a file pair, file with schema, or file with protocol"},
    //-----------------------------------------------------------------------//
    {"set_path",
     _PyCFunction_CAST(PyConduit_Node_set_path),
     METH_VARARGS,
     "Sets the node at the given path"},
    //-----------------------------------------------------------------------//
    {"schema",
     _PyCFunction_CAST(PyConduit_Node_schema),
     METH_NOARGS,
     "Returns the schema for the node"},
     //-----------------------------------------------------------------------//
     {"dtype",
      _PyCFunction_CAST(PyConduit_Node_dtype),
      METH_NOARGS,
      "Returns the conduit DataType for the node"},
    //-----------------------------------------------------------------------//
    {"is_root",
     _PyCFunction_CAST(PyConduit_Node_is_root),
     METH_NOARGS,
     "Returns if this node is the root of the hierarchy."},
    //-----------------------------------------------------------------------//
    {"parent",
     _PyCFunction_CAST(PyConduit_Node_parent),
     METH_NOARGS,
     "Returns this nodes parent, or None if no parent"},
    //-----------------------------------------------------------------------//
    {"total_strided_bytes",
     _PyCFunction_CAST(PyConduit_Node_total_strided_bytes), 
     METH_NOARGS,
     "Returns the total bytes strided by the all leaves in this node's tree"},
    //-----------------------------------------------------------------------//
    {"total_bytes_compact",
     _PyCFunction_CAST(PyConduit_Node_total_bytes_compact),
     METH_NOARGS,
     "Returns the total bytes of compact rep of all the leaves in this node's tree"},
    //-----------------------------------------------------------------------//
    {"is_compact",
     _PyCFunction_CAST(PyConduit_Node_is_compact),
     METH_NOARGS,
     "Returns if this node's data is in compact form"},
     //-----------------------------------------------------------------------//
     {"to_string",
      _PyCFunction_CAST(PyConduit_Node_to_string),
      METH_VARARGS| METH_KEYWORDS,
      "Returns a string representation of the node. "
      "Optionally takes protocol and spacing options. "
      "(Default protocol='yaml'.)"},
     //------------------------------------------------------------t-----------//
     {"to_summary_string",
      _PyCFunction_CAST(PyConduit_Node_to_summary_string),
      METH_VARARGS| METH_KEYWORDS,
      "Returns a summary string representation of the node. "
      "Optionally takes a Node that provides spacing and threshold options. "},
     //-----------------------------------------------------------------------//
     {"to_json",
      _PyCFunction_CAST(PyConduit_Node_to_json),
      METH_VARARGS| METH_KEYWORDS,
      "Returns a JSON string representation of the node. "
      "Optionally takes protocol and spacing options."},
     //-----------------------------------------------------------------------//
     {"to_yaml",
      _PyCFunction_CAST(PyConduit_Node_to_yaml),
      METH_VARARGS| METH_KEYWORDS,
      "Returns a YAML string representation of the node. "
      "Optionally takes protocol and spacing options."},
     //-----------------------------------------------------------------------//
     {"children",
      _PyCFunction_CAST(PyConduit_Node_iter),
      METH_NOARGS,
      "Returns a NodeIterator for this node"},
      //-----------------------------------------------------------------------//
      {"endian_swap",
       _PyCFunction_CAST(PyConduit_Node_endian_swap),
       METH_VARARGS,
       "Swaps data byte order to given endianness"},
      //-----------------------------------------------------------------------//
      {"endian_swap_to_machine_default",
       _PyCFunction_CAST(PyConduit_Node_endian_swap_to_machine_default),
       METH_NOARGS,
       "Swaps data byte order to default endianness"},
      //-----------------------------------------------------------------------//
      {"endian_swap_to_little",
       _PyCFunction_CAST(PyConduit_Node_endian_swap_to_little),
       METH_NOARGS,
       "Swaps data byte order to little endian"},
      //-----------------------------------------------------------------------//
      {"endian_swap_to_big",
       _PyCFunction_CAST(PyConduit_Node_endian_swap_to_big),
       METH_NOARGS,
       "Swaps data byte order to big endian"},

    //-----------------------------------------------------------------------//
    // end node methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, 0, NULL}
};

#ifdef Py_LIMITED_API
static PyType_Slot PyConduit_Node_SLOTS[]  = {
  {Py_tp_dealloc,        (void*) PyConduit_Node_dealloc},
  {Py_tp_methods,        (void*) PyConduit_Node_METHODS},
  {Py_tp_init,           (void*) PyConduit_Node_init},
  {Py_tp_new,            (void*) PyConduit_Node_new},
  {Py_tp_str,            (void*) PyConduit_Node_str},
  {Py_mp_subscript,      (void*) PyConduit_Node_GetItem},
  {Py_mp_ass_subscript,  (void*) PyConduit_Node_SetItem},
  {Py_tp_iter,           (void*) PyConduit_Node_iter},
  {0,0},
};

static PyType_Spec PyConduit_Node_SPEC =
{
   "Node",                                     /* tp_name */
   sizeof(PyConduit_Node),                     /* tp_basicsize */
   0,                                          /* tp_itemsize */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
   PyConduit_Node_SLOTS,                       /* tp_slots */
};

#else
//---------------------------------------------------------------------------//
static PyMappingMethods node_as_mapping = {
   (lenfunc)0,    // len operator is not supported
   (binaryfunc)PyConduit_Node_GetItem,
   (objobjargproc)PyConduit_Node_SetItem,
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//


static PyTypeObject PyConduit_Node_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "Node",
   sizeof(PyConduit_Node),  /* tp_basicsize */
   0, /* tp_itemsize */
   (destructor)PyConduit_Node_dealloc,                   /* tp_dealloc */
   // tp_print was removed in Python 3.9, its now used as
   // tp_vectorcall_offset (which we also don't use here)
   0, /* tp_print or tp_vectorcall_offset */
   0, /* tp_getattr */
   0, /* tp_setattr */
   0, /* tp_compare */
   (reprfunc)PyConduit_Node_repr, /* tp_repr */
   0, /* tp_as_number */
   0, /* tp_as_sequence */
   &node_as_mapping, /* as_mapping */
   0, /* hash */
   0, /* call */
   (reprfunc)PyConduit_Node_str,                         /* str */
   0, /* getattro */
   0, /* setattro */
   0, /* asbuffer */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,     /* flags */
   "Conduit node objects",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   (getiterfunc)PyConduit_Node_iter, /* iter */
   0, /* iternext */
   PyConduit_Node_METHODS,
   0,
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   (initproc)PyConduit_Node_init,
   0, /* alloc */
   PyConduit_Node_new,                                   /* new */
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
// conduit:::about
//---------------------------------------------------------------------------//
static PyObject*
PyConduit_about()
{
    //create and return a node with the result of about
    PyObject *py_node_res = PyConduit_Node_Python_Create();
    Node *node = PyConduit_Node_Get_Node_Ptr(py_node_res);
    conduit::about(*node);
    return (PyObject*)py_node_res;
}


//---------------------------------------------------------------------------//
// Python Module Method Defs
//---------------------------------------------------------------------------//
static PyMethodDef conduit_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    {"about",
     _PyCFunction_CAST(PyConduit_about),
      METH_NOARGS,
      NULL},
    //-----------------------------------------------------------------------//
    // end conduit methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, METH_VARARGS, NULL}
};


//---------------------------------------------------------------------------//
static int
PyConduit_Schema_Check(PyObject* obj)
{
    PyTypeObject* type = NULL;
    Set_PyTypeObject_Macro(type, PyConduit_Schema_TYPE);
    return (PyObject_TypeCheck(obj, type));
}


//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Schema_Python_Wrap(Schema *schema, int python_owns)
{
    PyTypeObject *type = NULL;
    Set_PyTypeObject_Macro(type, PyConduit_Schema_TYPE);
    PyConduit_Schema *retval = (PyConduit_Schema*)PyType_GenericAlloc(type,0);
    retval->schema = schema;
    retval->python_owns = python_owns;
    return ((PyObject*)retval);
}


//---------------------------------------------------------------------------//
static int
PyConduit_Node_Check(PyObject *obj)
{
    PyTypeObject* type = NULL;
    Set_PyTypeObject_Macro(type, PyConduit_Node_TYPE);
    return (PyObject_TypeCheck(obj, type));
}

//---------------------------------------------------------------------------//
static Node *
PyConduit_Node_Get_Node_Ptr(PyObject *obj)
{
    return ((PyConduit_Node*)obj)->node;
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Node_Python_Wrap(Node *node, int python_owns)
{
    PyTypeObject* type = NULL;
    Set_PyTypeObject_Macro(type, PyConduit_Node_TYPE);
    PyConduit_Node *retval = (PyConduit_Node*)PyType_GenericAlloc(type,0);
    retval->node = node;
    retval->python_owns = python_owns;
    return ((PyObject*)retval);
}


//---------------------------------------------------------------------------//
static PyObject*
PyConduit_Node_Python_Create()
{
    Node *node = new Node();
    // python_owns = 1
    return PyConduit_Node_Python_Wrap(node,1);
}


//---------------------------------------------------------------------------//
static int
PyConduit_Node_Set_From_Python_List(Node &node,
                                    PyObject *value)
{
    // like json and yaml cases, identify if we are
    // a numeric case, or a more general case

    Py_ssize_t list_size = PyList_Size(value);

    if(list_size == 0)
    {
        node.reset();
        return 0;
    }

    bool ok = true;
    bool homogenous_numeric = true;

    index_t  dtype_id = DataType::INT64_ID;

    for(Py_ssize_t idx=0; idx < list_size && homogenous_numeric; idx++)
    {
        PyObject *py_entry = PyList_GetItem(value, idx);
        if (PyInt_Check(py_entry) || PyLong_Check(py_entry))
        {
            // int64 still ok
        }
        else if (PyFloat_Check(py_entry))
        {
            // promote to float64
            dtype_id = DataType::FLOAT64_ID;
        }
        else // general
        {
            homogenous_numeric = false;
        }
    }

    if(homogenous_numeric)
    {
        if(dtype_id ==  DataType::INT64_ID)
        {
            node.set(DataType::int64((index_t)list_size));
            int64 *vals_ptr = node.value();
            for(Py_ssize_t idx=0; idx < list_size; idx++)
            {
                PyObject *py_entry = PyList_GetItem(value, idx);
                if (PyInt_Check(py_entry))
                {
                    vals_ptr[idx] = (int64)PyInt_AsLong(py_entry);
                }
                else // PyLong_Check(py_entry) == TRUE
                {
                    vals_ptr[idx] = (int64)PyLong_AsLong(py_entry);
                }
            }
        }
        else
        {
            node.set(DataType::float64((index_t)list_size));
            float64 *vals_ptr = node.value();
            for(Py_ssize_t idx=0; idx < list_size; idx++)
            {
                PyObject *py_entry = PyList_GetItem(value, idx);

                if (PyInt_Check(py_entry))
                {
                    vals_ptr[idx] = (float64)PyInt_AsLong(py_entry);
                }
                else if( PyLong_Check(py_entry) )
                {
                    vals_ptr[idx] = (float64)PyLong_AsLong(py_entry);
                }
                else // float
                {
                    vals_ptr[idx] = (float64)PyFloat_AsDouble(py_entry);
                }
            }
        }
    }
    else
    {
        // try general case as conduit list
        node.reset();
        ok = true;
        for(Py_ssize_t idx=0; idx < list_size && ok; idx++)
        {
            PyObject *py_entry = PyList_GetItem(value, idx);
            Node &cld = node.append();
            if( PyConduit_Node_Set_From_Python(cld,py_entry) != 0 )
            {
                ok = false;
            }
        }
    }

    // if not ok, we assume py error was already set
    if(!ok)
        return -1;
    else
        return 0;
}

//---------------------------------------------------------------------------//
static int
PyConduit_Node_Set_From_Python_Tuple(Node &node,
                                     PyObject *value)
{
    // like json and yaml cases, identify if we are
    // a numeric case, or a more general case


    Py_ssize_t tuple_size = PyTuple_Size(value);

    if(tuple_size == 0)
    {
        node.reset();
        return 0;
    }

    bool ok = true;
    bool homogenous_numeric = true;

    index_t  dtype_id = DataType::INT64_ID;

    for(Py_ssize_t idx=0; idx < tuple_size && homogenous_numeric; idx++)
    {
        PyObject *py_entry = PyTuple_GetItem(value, idx);
        if (PyInt_Check(py_entry) || PyLong_Check(py_entry))
        {
            // int64 still ok
        }
        else if (PyFloat_Check(py_entry))
        {
            // promote to float64
            dtype_id = DataType::FLOAT64_ID;
        }
        else // general
        {
            homogenous_numeric = false;
        }
    }

    if(homogenous_numeric)
    {


        if(dtype_id ==  DataType::INT64_ID)
        {
            node.set(DataType::int64((index_t)tuple_size));
            int64 *vals_ptr = node.value();
            for(Py_ssize_t idx=0; idx < tuple_size; idx++)
            {
                PyObject *py_entry = PyTuple_GetItem(value, idx);
                if (PyInt_Check(py_entry))
                {
                    vals_ptr[idx] = (int64)PyInt_AsLong(py_entry);
                }
                else // PyLong_Check(py_entry) == TRUE
                {
                    vals_ptr[idx] = (int64)PyLong_AsLong(py_entry);
                }
            }
        }
        else
        {
            node.set(DataType::float64((index_t)tuple_size));
            float64 *vals_ptr = node.value();
            for(Py_ssize_t idx=0; idx < tuple_size; idx++)
            {
                PyObject *py_entry = PyTuple_GetItem(value, idx);

                if (PyInt_Check(py_entry))
                {
                    vals_ptr[idx] = (float64)PyInt_AsLong(py_entry);
                }
                else if( PyLong_Check(py_entry) )
                {
                    vals_ptr[idx] = (float64)PyLong_AsLong(py_entry);
                }
                else // float
                {
                    vals_ptr[idx] = (float64)PyFloat_AsDouble(py_entry);
                }
            }
        }
    }
    else
    {
        // try general case as conduit list
        node.reset();
        ok = true;
        for(Py_ssize_t idx=0; idx < tuple_size && ok; idx++)
        {
            PyObject *py_entry = PyTuple_GetItem(value, idx);
            Node &cld = node.append();
            if( PyConduit_Node_Set_From_Python(cld,py_entry) != 0 )
            {
                ok = false;
            }
        }
    }

    // if not ok, we assume py error was already set
    if(!ok)
        return -1;
    else
        return 0;
}

//---------------------------------------------------------------------------//
static int
PyConduit_Node_Set_From_Numpy_String_Array(Node &node,
                                           PyArrayObject *py_arr)
{
    node.reset();
    // get the number of strings, and create a conduit list of strings
    npy_intp num_strings = PyArray_SIZE(py_arr);
    npy_intp *dims = PyArray_DIMS(py_arr);
    npy_intp string_len = dims[1];
    for(npy_intp i=0; i < num_strings; i++)
    {
        // read each string into a child
        Node &cld = node.append();
        // numpy strings are fixed len + may not include
        // NULL term.
        cld.set(DataType::char8_str(string_len),
                PyArray_GETPTR1(py_arr,i));
    }

    return 0;
}

//---------------------------------------------------------------------------//
static int
PyConduit_Node_Set_From_Numpy_Unicode_Array(Node &node,
                                            PyArrayObject *py_arr)
{
    node.reset();
    // get the number of strings, and create a conduit list of strings
    npy_intp num_strings = PyArray_SIZE(py_arr);
    npy_intp *dims = PyArray_DIMS(py_arr);
    npy_intp buffer_len = dims[1];
    for(npy_intp i=0; i < num_strings; i++)
    {
        // read each string into a child
        Node &cld = node.append();

        // get unicode data and construct a python unicode object from it
        void *unicode_buffer_ptr = PyArray_GETPTR1(py_arr,i);
        PyObject *py_temp_unicode = PyUnicode_From_UTF32_Unicode_Buffer((const char*)unicode_buffer_ptr,
                                                                        buffer_len);

        if(py_temp_unicode == NULL)
        {
            PyErr_SetString(PyExc_TypeError,
                            "Failed to construct PyUnicode from NPY_UNICODE Array entry.");
            return -1;
        }

        // convert our unicode string to ascii for conduit
        PyObject *py_temp_bytes = PyUnicode_AsEncodedString(py_temp_unicode,
                                                          "ASCII",
                                                          "strict"); // Owned reference
        // cleanup unicode obj
        Py_DECREF(py_temp_unicode);
        if(py_temp_bytes == NULL)
        {
            // error
            PyErr_SetString(PyExc_TypeError,
                            "Failed to encode unicode string to ASCII for use in conduit");
            return -1;
        }

        // copy data into conduit node
        cld.set_char8_str(PyBytes_AsString(py_temp_bytes));
        // cleanup temp bytes
        Py_DECREF(py_temp_bytes);
    }

    return 0;
}


//---------------------------------------------------------------------------//
static int
PyConduit_Node_Set_From_Numpy_String(Node &node,
                                     PyObject *value)
{
    char *cstr = PyString_AsString(value);
    node.set_char8_str(cstr);
    PyString_AsString_Cleanup(cstr);
    return 0;
}


//---------------------------------------------------------------------------//
static int
PyConduit_Node_Set_From_Python(Node &node,
                               PyObject *value)
{
    if (PyConduit_Node_Check(value))
    {
        node = *((PyConduit_Node*)value)->node;
    }
    else if (PyConduit_DataType_Check(value))
    {
        node = ((PyConduit_DataType*)value)->dtype;
    }
    else if (PyConduit_Schema_Check(value))
    {
        node = *((PyConduit_Schema*)value)->schema;
    }
    else if (PyString_Check(value))
    {
        char *cstr = PyString_AsString(value);
        node = cstr;
        PyString_AsString_Cleanup(cstr);
    }
    else if (PyInt_Check(value))
    {
        node = PyInt_AsLong(value);
    }
    else if (PyLong_Check(value))
    {
        node = PyLong_AsLong(value);
    }
    else if (PyFloat_Check(value))
    {
      node = PyFloat_AsDouble(value);
    }
    else if (PyArray_Check(value))
    {
        PyArray_Descr *desc = PyArray_DESCR((PyArrayObject*)value);
        PyArrayObject *py_arr = (PyArrayObject*)value;

        npy_intp num_ele = PyArray_SIZE(py_arr);
        switch (desc->type_num)
        {
            case NPY_STRING:
            {
                return PyConduit_Node_Set_From_Numpy_String_Array(node,
                                                                  py_arr);
                break;
            }
            case NPY_UNICODE:
            {
                return PyConduit_Node_Set_From_Numpy_Unicode_Array(node,
                                                                   py_arr);
            }
            case NPY_UINT8 :
            {
                node.set(DataType::uint8(num_ele));
                uint8_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_UINT16 :
            {
                node.set(DataType::uint16(num_ele));
                uint16_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_UINT32 :
            {
                node.set(DataType::uint32(num_ele));
                uint32_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_UINT64 :
            {
                node.set(DataType::uint64(num_ele));
                uint64_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_INT8 :
            {
                node.set(DataType::int8(num_ele));
                int8_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_INT16 :
            {
                node.set(DataType::int16(num_ele));
                int16_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_INT32 :
            {
                node.set(DataType::int32(num_ele));
                int32_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_INT64 :
            {
                node.set(DataType::int64(num_ele));
                int64_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_FLOAT32 :
            {
                node.set(DataType::float32(num_ele));
                float32_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            case NPY_FLOAT64 :
            {
                node.set(DataType::float64(num_ele));
                float64_array c_arr = node.value();
                PyConduit_Fill_DataArray_From_PyArray(c_arr, py_arr);
                break;
            }
            default:
            {
                std::ostringstream err_msg;
                err_msg << "PyArray Array Type not supported: "
                        << desc->kind;
                PyErr_SetString(PyExc_TypeError,
                                err_msg.str().c_str());
                return (-1);
            }
        }
    }
    else if (PyArray_CheckScalar(value))
    {
        PyArray_Descr* desc = PyArray_DescrFromScalar(value);

        switch (desc->type_num)
        {
            case NPY_STRING:
            {
                return PyConduit_Node_Set_From_Numpy_String(node,
                                                            value);
                break;
            }
            case NPY_UNICODE:
            {
                return PyConduit_Node_Set_From_Numpy_String(node,
                                                            value);
            }
            case NPY_INT8 : {
                int8 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_INT16 : {
                int16 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_INT32 : {
                int32 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_INT64 : {
                int64 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_UINT8 : {
                uint8 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_UINT16 : {
                uint16 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_UINT32 : {
                uint32 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_UINT64 : {
                uint64 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_FLOAT32 : {
                float32 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            case NPY_FLOAT64 : {
                float64 val;
                PyArray_ScalarAsCtype(value, &val);
                node = val;
                break;
            }
            default:
            {
                std::ostringstream err_msg;
                err_msg << "PyArray Scalar Type not supported: "
                        << desc->kind;
                PyErr_SetString(PyExc_TypeError,
                                err_msg.str().c_str());
                return (-1);
            }
        }

    }
    else if( PyList_Check(value) )
    {
        return PyConduit_Node_Set_From_Python_List(node,value);
    }
    else if( PyTuple_Check(value) )
    {
        return PyConduit_Node_Set_From_Python_Tuple(node,value);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "Value type not supported");
        return (-1);
    }

    return (0);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Create_Numpy_Type(Node& node,
                          int type)
{
    const DataType& dtype = node.dtype();
    PyArray_Descr* descr = PyArray_DescrFromType(type);
    PyObject* retval = NULL;
    void* data = node.element_ptr(0);
    npy_intp len = (npy_intp)dtype.number_of_elements();
    // TODO: This only deals with contiguous data?
    if (len == 1)
    {
        retval = PyArray_Scalar(data, descr, NULL);
    }
    else
    {
        retval = PyArray_SimpleNewFromData(1, &len, type, data);

        // Since there doesn't appear to be a constructor that takes data
        // and a descriptor, we'll just modify the strides appropriately.
        // This should be OK since we only support 1D arrays currently.
        npy_intp * strides = PyArray_STRIDES((PyArrayObject*)retval);
        strides[0] = (npy_intp)dtype.stride();
    }
    return (retval);
}

//---------------------------------------------------------------------------//
static PyObject *
PyConduit_Convert_Node_To_Python(Node& node)
{
    const DataType& type = node.dtype();
    int numpy_type = -1;
    PyObject* retval = NULL;

    switch (type.id()) {
        case DataType::EMPTY_ID:
        case DataType::OBJECT_ID:
        case DataType::LIST_ID:
        {
            retval = PyConduit_Node_Python_Wrap(&node,0);
            break;
        }
        case DataType::CHAR8_STR_ID: {
            retval = PyString_FromString(node.as_char8_str());
            break;
        }
        case DataType::INT8_ID: {
            numpy_type = NPY_INT8;
            break;
        }
        case DataType::INT16_ID: {
            numpy_type = NPY_INT16;
            break;
        }
        case DataType::INT32_ID: {
            numpy_type = NPY_INT32;
            break;
        }
        case DataType::INT64_ID: {
            numpy_type = NPY_INT64;
            break;
        }
        case DataType::UINT8_ID: {
            numpy_type = NPY_UINT8;
            break;
        }
        case DataType::UINT16_ID: {
            numpy_type = NPY_UINT16;
            break;
        }
        case DataType::UINT32_ID: {
            numpy_type = NPY_UINT32;
            break;
        }
        case DataType::UINT64_ID: {
            numpy_type = NPY_UINT64;
            break;
        }
        case DataType::FLOAT32_ID: {
            numpy_type = NPY_FLOAT32;
            break;
        }
        case DataType::FLOAT64_ID: {
            numpy_type = NPY_FLOAT64;
            break;
        }
        default : {
            PyErr_SetString(PyExc_TypeError, "Python Conduit does not support this type");
            return (NULL);
        }
    }

    // if we don't already have a result, we need to create a numpy result
    if (retval == NULL)
    {

        retval = PyConduit_Create_Numpy_Type(node, numpy_type);
    }

    return (retval);
}


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
// Endianness
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_machine_default(PyObject *) // unused
{
    return PyLong_FromSsize_t((Py_ssize_t)Endianness::machine_default());
}

//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_machine_is_little_endian(PyObject *) // unused
{
    if(Endianness::machine_is_little_endian())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}


//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_machine_is_big_endian(PyObject *) // unused
{
    if(Endianness::machine_is_big_endian())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_name_to_id(PyObject *, // unused
                        PyObject *args)
{
    const char *end_name;
    if (!PyArg_ParseTuple(args, "s", &end_name))
    {
        PyErr_SetString(PyExc_TypeError, "Endianness name must be a string");
        return NULL;
    }

    return PyLong_FromSsize_t((Py_ssize_t)Endianness::name_to_id(std::string(end_name)));
}

//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_id_to_name(PyObject *, // unused
                        PyObject *args)
{
    Py_ssize_t end_id;

    if (!PyArg_ParseTuple(args, "n", &end_id))
    {
        PyErr_SetString(PyExc_TypeError,
                "Endianness id must be a signed integer");
        return NULL;
    }

    return Py_BuildValue("s", Endianness::id_to_name(end_id).c_str());
}


//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_default_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(Endianness::DEFAULT_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_little_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(Endianness::LITTLE_ID);
}

//-----------------------------------------------------------------------------
static PyObject *
PyEndianness_big_id(PyObject *) // unused
{
    return PyLong_FromSsize_t(Endianness::BIG_ID);
}



static PyMethodDef PyConduit_Endianness_METHODS[] =
{
    //-----------------------------------------------------------------------//
    {"machine_default",
     _PyCFunction_CAST(PyEndianness_machine_default),
      METH_NOARGS | METH_STATIC,
     "Returns the id (Endianness::LITTLE_ID or Endianness::BIG_ID) for this machine's byte order."},
    {"machine_is_little_endian",
     _PyCFunction_CAST(PyEndianness_machine_is_little_endian),
      METH_NOARGS | METH_STATIC,
       "Returns if this machine uses little endian byte order."},
    {"machine_is_big_endian",
     _PyCFunction_CAST(PyEndianness_machine_is_big_endian),
      METH_NOARGS | METH_STATIC,
      "Returns if this machine uses big endian byte order."},
    {"name_to_id",
      _PyCFunction_CAST(PyEndianness_name_to_id),
       METH_VARARGS | METH_STATIC,
       "Returns the endianness id that correspondes to the passed string"},
    {"id_to_name",
      _PyCFunction_CAST(PyEndianness_id_to_name),
       METH_VARARGS | METH_STATIC,
       "Returns the string name of given endianness id"},
     // -- direct access to enum ids -- //
    {"default_id",
      _PyCFunction_CAST(PyEndianness_default_id),
       METH_VARARGS | METH_STATIC,
       "Returns Endianness::DEFAULT_ID"},
    {"little_id",
      _PyCFunction_CAST(PyEndianness_little_id),
       METH_VARARGS | METH_STATIC,
       "Returns Endianness::LITTLE_ID"},
    {"big_id",
      _PyCFunction_CAST(PyEndianness_big_id),
       METH_VARARGS | METH_STATIC,
       "Returns Endianness::BIG_ID"},


    //-----------------------------------------------------------------------//
    // end methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, METH_VARARGS, NULL}
};


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef Py_LIMITED_API
static PyType_Slot PyConduit_Endianness_SLOTS[]  = {
  {Py_tp_methods,        (void*) PyConduit_Endianness_METHODS},
  {0,0},
};

static PyType_Spec PyConduit_Endianness_SPEC =
{
   "Endianness",               /* tp_name */
   0,                          /* tp_basicsize */
   0,                          /* tp_itemsize */
   Py_TPFLAGS_DEFAULT,         /* tp_flags */
   PyConduit_Endianness_SLOTS, /* tp_slots */
};

#else
static PyTypeObject PyConduit_Endianness_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "Endianness",
   0, // sizeof(PyConduit_Endianness), /* tp_basicsize */
   0, /* tp_itemsize */
   0, /* tp_dealloc */
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
   0, /* str */
   0, /* getattro */
   0, /* setattro */
   0, /* asbuffer */
   Py_TPFLAGS_DEFAULT,     /* flags */
   "Conduit Endianness",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   0, /* iter */
   0, /* iternext */
   PyConduit_Endianness_METHODS, /* METHODS */
   0, /* MEMBERS */
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   0,
   0, /* alloc */
   0, /* new */
   0, /* tp_free */
   0, /* tp_is_gc */
   0, /* tp_bases */
   0, /* tp_mro */
   0, /* tp_cache */
   0, /* tp_subclasses */
   0,  /* tp_weaklist */
   0, /* tp_del */
   0  /* tp_version_tag */
   PyVarObject_TAIL
};
#endif



//---------------------------------------------------------------------------//
// Extra Module Setup Logic for Python3
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
//---------------------------------------------------------------------------//
static int
conduit_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    #ifdef Py_LIMITED_API
    Py_VISIT(GETSTATE(m)->PyConduit_DataType_TYPE);
    Py_VISIT(GETSTATE(m)->PyConduit_Generator_TYPE);
    Py_VISIT(GETSTATE(m)->PyConduit_Schema_TYPE);
    Py_VISIT(GETSTATE(m)->PyConduit_NodeIterator_TYPE);
    Py_VISIT(GETSTATE(m)->PyConduit_Node_TYPE);
    Py_VISIT(GETSTATE(m)->PyConduit_Endianness_TYPE);
    #endif
    return 0;
}

//---------------------------------------------------------------------------//
static int
conduit_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    #ifdef Py_LIMITED_API
    Py_CLEAR(GETSTATE(m)->PyConduit_DataType_TYPE);
    Py_CLEAR(GETSTATE(m)->PyConduit_Generator_TYPE);
    Py_CLEAR(GETSTATE(m)->PyConduit_Schema_TYPE);
    Py_CLEAR(GETSTATE(m)->PyConduit_NodeIterator_TYPE);
    Py_CLEAR(GETSTATE(m)->PyConduit_Endianness_TYPE);
    #endif
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef conduit_python_module_def =
{
        PyModuleDef_HEAD_INIT,
        "conduit_python",
        NULL,
        sizeof(struct module_state),
        conduit_python_funcs,
        NULL,
        conduit_python_traverse,
        conduit_python_clear,
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
// This macro allows register a type whether we are using heap- or static-
// allocated Python types
//---------------------------------------------------------------------------//
#define CONDUIT_SPEC_OBJECT(name) PyConduit_##name##_SPEC
#define CONDUIT_TYPE_OBJECT(name) PyConduit_##name##_TYPE
#define CONDUIT_STRINGIFY(x) #x
#define CONDUIT_TOSTRING(x) CONDUIT_STRINGIFY(x)

#ifdef Py_LIMITED_API

#define ADD_TYPE(module, name)                                                                                  \
do {                                                                                                            \
    module_state* state = GETSTATE(module);                                                                     \
    PyType_Spec* spec = &(CONDUIT_SPEC_OBJECT(name));                                                           \
    state->CONDUIT_TYPE_OBJECT(name) = (PyTypeObject *)PyType_FromModuleAndSpec((PyObject*)module, spec, NULL); \
    if (state->CONDUIT_TYPE_OBJECT(name) == NULL)                                                               \
    {                                                                                                           \
       PY_MODULE_INIT_RETURN_ERROR;                                                                             \
    }                                                                                                           \
    if (PyModule_AddType((PyObject*)module,state->CONDUIT_TYPE_OBJECT(name)) < 0)                               \
    {                                                                                                           \
       PY_MODULE_INIT_RETURN_ERROR;                                                                             \
    }                                                                                                           \
} while (0)
#else

#define ADD_TYPE(module, name)                                      \
do {                                                                \
    if (PyType_Ready(&CONDUIT_TYPE_OBJECT(name)) < 0)               \
    {                                                               \
        PY_MODULE_INIT_RETURN_ERROR;                                \
    }                                                               \
    Py_INCREF(&CONDUIT_TYPE_OBJECT(name));                          \
    PyModule_AddObject(module,                                      \
                       CONDUIT_TOSTRING(name),                      \
                       (PyObject*)&CONDUIT_TYPE_OBJECT(name))  ;    \
} while (0)
#endif
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
// Main entry point
//---------------------------------------------------------------------------//
extern "C"
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
CONDUIT_PYTHON_API PyObject * PyInit_conduit_python(void)
#else
void CONDUIT_PYTHON_API initconduit_python(void)
#endif
//---------------------------------------------------------------------------//
{
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *conduit_module = PyModule_Create(&conduit_python_module_def);
#else
    PyObject *conduit_module = Py_InitModule((char*)"conduit_python",
                                             conduit_python_funcs);
#endif

    if(conduit_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(conduit_module);

    st->error = PyErr_NewException((char*)"conduit_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(conduit_module);
        PY_MODULE_INIT_RETURN_ERROR;
    }

    //-----------------------------------------------------------------------//
    // init our custom types
    //-----------------------------------------------------------------------//

    ADD_TYPE(conduit_module, DataType);
    ADD_TYPE(conduit_module, Schema);
    ADD_TYPE(conduit_module, Generator);
    ADD_TYPE(conduit_module, NodeIterator);
    ADD_TYPE(conduit_module, Node);
    ADD_TYPE(conduit_module, Endianness);

    static void *PyConduit_API[PyConduit_API_number_of_entries];

    /* Initialize the C API pointer array */
    PyConduit_API[PyConduit_Node_Check_INDEX] = (void *)PyConduit_Node_Check;
    PyConduit_API[PyConduit_Node_Python_Create_INDEX] = (void *)PyConduit_Node_Python_Create;
    PyConduit_API[PyConduit_Node_Python_Wrap_INDEX] = (void *)PyConduit_Node_Python_Wrap;
    PyConduit_API[PyConduit_Node_Get_Node_Ptr_INDEX] = (void *)PyConduit_Node_Get_Node_Ptr;

    /* Create a Capsule containing the API pointer array's address */
    PyObject *py_c_api_capsule = PyCapsule_New((void *)PyConduit_API, "conduit._C_API", NULL);

    if (py_c_api_capsule != NULL)
    {
        PyModule_AddObject(conduit_module, "_C_API", py_c_api_capsule);
    }

    // req setup for numpy
    import_array();

#ifdef Py_LIMITED_API
    GLOBAL_MODULE = conduit_module;
#endif

#if defined(IS_PY3K)
    return conduit_module;
#endif

}
#undef ADD_TYPE
#undef CONDUIT_TYPE_OBJECT
#undef CONDUIT_SPEC_OBJECT
#undef CONDUIT_STRINGIFY
#undef CONDUIT_TOSTRING

