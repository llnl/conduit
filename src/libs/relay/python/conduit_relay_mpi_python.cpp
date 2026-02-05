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
#include "conduit_relay_mpi.hpp"

#include "conduit_relay_python_exports.h"

// conduit python module capi header
#include "conduit_python.hpp"

using namespace conduit;
using namespace conduit::relay::mpi;

//---------------------------------------------------------------------------//
struct PyRelay_MPI_Request
{
    PyObject_HEAD
    Request request;
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//
// relay::mpi::Request Object 
//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_Request_new(PyTypeObject *type,
                        PyObject*, // args -- unused
                        PyObject*) // kwds -- unused
{
    PyRelay_MPI_Request *self = (PyRelay_MPI_Request*)PyType_GenericAlloc(type, 0);

    return ((PyObject*)self);
}

//---------------------------------------------------------------------------//
static void
PyRelay_MPI_Request_dealloc(PyRelay_MPI_Request *self)
{
    #ifdef Py_LIMITED_API
    freefunc tp_free = ((freefunc)PyType_GetSlot(Py_TYPE((PyObject*)self), Py_tp_free));
    tp_free((PyObject*)self);
    #else
    Py_TYPE(self)->tp_free((PyObject*)self);
    #endif
}

//---------------------------------------------------------------------------//
static int
PyRelay_MPI_Request_init(PyRelay_MPI_Request * /*self*/,  //unused
                         PyObject * /*args*/, //unused
                         PyObject * /*kwd*/) //unused
{
    return 0;
}

//---------------------------------------------------------------------------//
// TODO: Enable when async support is enabled
// static Request *
// PyRelay_MPI_Request_request_pointer(PyRelay_MPI_Request *py_req)
// {
//    return &py_req->request;
// }

//----------------------------------------------------------------------------//
// Request methods table
//----------------------------------------------------------------------------//
static PyMethodDef PyRelay_MPI_Request_METHODS[] = {
    //-----------------------------------------------------------------------//
    //-----------------------------------------------------------------------//
    // end Request methods table
    //-----------------------------------------------------------------------//
    {NULL, NULL, 0, NULL}
};

//---------------------------------------------------------------------------//

#ifdef Py_LIMITED_API
static PyType_Slot PyRelay_MPI_Request_SLOTS[]  = {
  {Py_tp_dealloc,        (void*) PyRelay_MPI_Request_dealloc},
  {Py_tp_methods,        (void*) PyRelay_MPI_Request_METHODS},
  {Py_tp_init,           (void*) PyRelay_MPI_Request_init},
  {Py_tp_new,            (void*) PyRelay_MPI_Request_new},
  {Py_tp_doc,            (void*) "Conduit Relay MPI Request objects"},
  {0,0},
};

static PyType_Spec PyRelay_MPI_Request_SPEC =
{
   "Request",                                /* tp_name */
   sizeof(PyRelay_MPI_Request),              /* tp_basicsize */
   0,                                        /* tp_itemsize */
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
   PyRelay_MPI_Request_SLOTS,                /* tp_slots */
};

#else

static PyTypeObject PyRelay_MPI_Request_TYPE = {
   PyVarObject_HEAD_INIT(NULL, 0)
   "Request",
   sizeof(PyRelay_MPI_Request),  /* tp_basicsize */
   0, /* tp_itemsize */
   (destructor)PyRelay_MPI_Request_dealloc, /* tp_dealloc */
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
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* flags */
   "Conduit Relay MPI Request objects",
   0, /* traverse */
   0, /* clear */
   0, /* tp_richcompare */
   0, /* tp_weaklistoffset */
   0, /* iter */
   0, /* iternext */
   PyRelay_MPI_Request_METHODS, /* METHODS */
   0, /* MEMBERS */
   0, /* get/set */
   0, /* tp_base */
   0, /* dict */
   0, /* descr_get */
   0, /* gescr_set */
   0, /* dictoffset */
   (initproc)PyRelay_MPI_Request_init,
   0, /* alloc */
   PyRelay_MPI_Request_new, /* new */
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
// TODO: Enable when async support is enabled
// //---------------------------------------------------------------------------//
// static int
// PyRelay_MPI_Request_Check(PyObject* obj)
// {
//     return (PyObject_TypeCheck(obj, &PyRelay_MPI_Request_TYPE));
// }


//-----------------------------------------------------------------------------
/// MPI Module Functions
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
// conduit::relay::mpi::about
//---------------------------------------------------------------------------//
static PyObject *
PyRelay_MPI_about()
{
    //create and return a node with the result of about
    PyObject *py_node_res = PyConduit_Node_Python_Create();
    Node *node = PyConduit_Node_Get_Node_Ptr(py_node_res);
    conduit::relay::mpi::about(*node);
    return (PyObject*)py_node_res;
}

//---------------------------------------------------------------------------//
// conduit::relay::mpi::rank
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_rank(PyObject *, //self
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

    // obtain rank to check that the passed mpi comm is valid
    // return error state to python if check fails
    try
    {
        relay::mpi::rank(comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }


    return PyLong_FromLong((long)rank);
}

//---------------------------------------------------------------------------//
// conduit::relay::mpi::size
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_size(PyObject *, //self
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

    int size = 0;

    try
    {
        size = relay::mpi::size(comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    return PyLong_FromLong((long)size);
}

//-----------------------------------------------------------------------------
/// Standard MPI Send Recv
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
// conduit::relay::mpi::send
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_send(PyObject *, //self
                 PyObject *args,
                 PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    Py_ssize_t  dest;
    Py_ssize_t  tag;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "dest",
                                   "tag",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Onnn",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &dest,
                                     &tag,
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

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::send(node, dest, tag, comm);
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
// conduit::relay::mpi::recv
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_recv(PyObject *, //self
                 PyObject *args,
                 PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    Py_ssize_t  source;
    Py_ssize_t  tag;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "source",
                                   "tag",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Onnn",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &source,
                                     &tag,
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


    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::recv(node, source, tag, comm);
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
// conduit::relay::mpi::send_using_schema
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_send_using_schema(PyObject *, //self
                              PyObject *args,
                              PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    Py_ssize_t  dest;
    Py_ssize_t  tag;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "dest",
                                   "tag",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Onnn",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &dest,
                                     &tag,
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

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::send_using_schema(node, dest, tag, comm);
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
// conduit::relay::mpi::recv_using_schema
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_recv_using_schema(PyObject *, //self
                              PyObject *args,
                              PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    Py_ssize_t  source;
    Py_ssize_t  tag;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "source",
                                   "tag",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Onnn",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &source,
                                     &tag,
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

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::recv_using_schema(node, source, tag, comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//-----------------------------------------------------------------------------
// TODO: Generic MPI Reduce
//-----------------------------------------------------------------------------
// //-----------------------------------------------------------------------------
// /// MPI Reduce
// //-----------------------------------------------------------------------------
//
//     /// MPI reduce and all reduce methods.
//
//     /// While the input does not need to be compact,
//     /// reductions require all MPI ranks have identical compact representations.
//
//     /// These methods do not check across ranks for identical compact
//     /// representation.
//
//     /// Conduit empty, object, and list dtypes can not be reduced.
//
//     /// If the send_node is not compact, it will be compacted prior to sending.
//
//     /// for reduce on the root rank and all_reduce for all ranks:
//     ///   if the recv_node is compatible but not compact, data will be placed
//     ///   into a compact buffer, then read back out into the recv_node node.
//     ///
//     ///   if the recv_node is not compatible, it will be reset to
//     ///   a compact compatible type.
//
//     int CONDUIT_RELAY_API reduce(const Node &send_node,
//                                  Node &recv_node,
//                                  MPI_Op mpi_op,
//                                  int root,
//                                  MPI_Comm comm);
//
//     int CONDUIT_RELAY_API all_reduce(const Node &send_node,
//                                      Node &recv_node,
//                                      MPI_Op mpi_op,
//                                      MPI_Comm comm);
//



//-----------------------------------------------------------------------------
/// MPI Reduce Helpers
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
// conduit::relay::mpi::sum_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_sum_reduce(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOnn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &root,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::sum_reduce(send_node,
                               recv_node,
                               root,
                               comm);
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
// conduit::relay::mpi::min_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_min_reduce(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOnn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &root,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::min_reduce(send_node,
                               recv_node,
                               root,
                               comm);
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
// conduit::relay::mpi::max_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_max_reduce(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOnn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &root,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::max_reduce(send_node,
                               recv_node,
                               root,
                               comm);
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
// conduit::relay::mpi::prod_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_prod_reduce(PyObject *, //self
                        PyObject *args,
                        PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOnn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &root,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::prod_reduce(send_node,
                               recv_node,
                               root,
                               comm);
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
// conduit::relay::mpi::sum_all_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_sum_all_reduce(PyObject *, //self
                           PyObject *args,
                           PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::sum_all_reduce(send_node,
                                   recv_node,
                                   comm);
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
// conduit::relay::mpi::min_all_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_min_all_reduce(PyObject *, //self
                           PyObject *args,
                           PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  mpi_comm_id;


    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::min_all_reduce(send_node,
                                   recv_node,
                                   comm);
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
// conduit::relay::mpi::max_all_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_max_all_reduce(PyObject *, //self
                           PyObject *args,
                           PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::max_all_reduce(send_node,
                                   recv_node,
                                   comm);
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
// conduit::relay::mpi::prod_all_reduce
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_prod_all_reduce(PyObject *, //self
                            PyObject *args,
                            PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm
    // TODO: We could use shared arg parsing for reduce helpers
    //       I started down that path, but opted to keep sep parsing
    //       to get over the finish line

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::prod_all_reduce(send_node,
                                    recv_node,
                                    comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//-----------------------------------------------------------------------------
// TODO: Async MPI Send Recv
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
/// Async MPI Send Recv
//-----------------------------------------------------------------------------


//---------------------------------------------------------------------------//
// conduit::relay::mpi::isend
//---------------------------------------------------------------------------//
// static PyObject *
// PyRelay_MPI_isend(PyObject *, //self
//                   PyObject *args,
//                   PyObject *kwargs)
// {
//     PyObject   *py_node  = NULL;
//     Py_ssize_t  dest;
//     Py_ssize_t  tag;
//     Py_ssize_t  mpi_comm_id;
//     PyObject   *py_mpi_request = NULL;
//
//     // TODO: future also accept mpi4py comm
//
//     static const char *kwlist[] = {"node",
//                                    "dest",
//                                    "tag",
//                                    "comm",
//                                    "request",
//                                    NULL};
//
//     if (!PyArg_ParseTupleAndKeywords(args,
//                                      kwargs,
//                                      "OnnnO",
//                                      const_cast<char**>(kwlist),
//                                      &py_node,
//                                      &dest,
//                                      &tag,
//                                      &mpi_comm_id,
//                                      &py_mpi_request))
//     {
//         return (NULL);
//     }
//
//     if(!PyConduit_Node_Check(py_node))
//     {
//         PyErr_SetString(PyExc_TypeError,
//                         "'node' argument must be a "
//                         "conduit.Node instance");
//         return NULL;
//     }
//
//     if(!PyRelay_MPI_Request_Check(py_mpi_request))
//     {
//         PyErr_SetString(PyExc_TypeError,
//                         "'request' argument must be a "
//                         "conduit.relay.mpi Request instance");
//         return NULL;
//     }
//
//     Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
//     Request *request = PyRelay_MPI_Request_request_pointer((PyRelay_MPI_Request*)py_mpi_request);
//
//     // get c mpi comm hnd
//     MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);
//
//     try
//     {
//         relay::mpi::isend(node,
//                           dest,
//                           tag,
//                           comm,
//                           request);
//     }
//     catch(conduit::Error &e)
//     {
//         PyErr_SetString(PyExc_Exception,
//                         e.message().c_str());
//         return NULL;
//     }
//
//     Py_RETURN_NONE;
// }




//     int CONDUIT_RELAY_API irecv(Node &node,
//                                 int src,
//                                 int tag,
//                                 MPI_Comm comm,
//                                 Request *request);


//---------------------------------------------------------------------------//
// conduit::relay::mpi::irecv
//---------------------------------------------------------------------------//
// static PyObject *
// PyRelay_MPI_irecv(PyObject *, //self
//                   PyObject *args,
//                   PyObject *kwargs)
// {
//     PyObject   *py_node  = NULL;
//     Py_ssize_t  source;
//     Py_ssize_t  tag;
//     Py_ssize_t  mpi_comm_id;
//     PyObject   *py_mpi_request = NULL;
//
//     // TODO: future also accept mpi4py comm
//
//     static const char *kwlist[] = {"node",
//                                    "source",
//                                    "tag",
//                                    "comm",
//                                    "request",
//                                    NULL};
//
//     if (!PyArg_ParseTupleAndKeywords(args,
//                                      kwargs,
//                                      "OnnnO",
//                                      const_cast<char**>(kwlist),
//                                      &py_node,
//                                      &source,
//                                      &tag,
//                                      &mpi_comm_id,
//                                      &py_mpi_request))
//     {
//         return (NULL);
//     }
//
//     if(!PyConduit_Node_Check(py_node))
//     {
//         PyErr_SetString(PyExc_TypeError,
//                         "'node' argument must be a "
//                         "conduit.Node instance");
//         return NULL;
//     }
//
//     if(!PyRelay_MPI_Request_Check(py_mpi_request))
//     {
//         PyErr_SetString(PyExc_TypeError,
//                         "'request' argument must be a "
//                         "conduit.relay.mpi Request instance");
//         return NULL;
//     }
//
//     Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);
//     Request *request = PyRelay_MPI_Request_request_pointer((PyRelay_MPI_Request*)py_mpi_request);
//
//     // get c mpi comm hnd
//     MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);
//
//     try
//     {
//         relay::mpi::irecv(node,
//                           source,
//                           tag,
//                           comm,
//                           request);
//     }
//     catch(conduit::Error &e)
//     {
//         PyErr_SetString(PyExc_Exception,
//                         e.message().c_str());
//         return NULL;
//     }
//
//     Py_RETURN_NONE;
// }



//     int CONDUIT_RELAY_API wait_send(Request *request,
//                                     MPI_Status *status);
//
//     int CONDUIT_RELAY_API wait_recv(Request *request,
//                                     MPI_Status *status);
//
//     int CONDUIT_RELAY_API wait_all_send(int count,
//                                         Request requests[],
//                                         MPI_Status statuses[]);
//
//     int CONDUIT_RELAY_API wait_all_recv(int count,
//                                         Request requests[],
//                                         MPI_Status statuses[]);
//

//-----------------------------------------------------------------------------
/// MPI gather (these expect identical schemas)
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
// conduit::relay::mpi::gather
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_gather(PyObject *, //self
                   PyObject *args,
                   PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOnn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &root,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::gather(send_node,
                           recv_node,
                           root,
                           comm);
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
// conduit::relay::mpi::gather_using_schema
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_gather_using_schema(PyObject *, //self
                                PyObject *args,
                                PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOnn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &root,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::gather_using_schema(send_node,
                                        recv_node,
                                        root,
                                        comm);
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
// conduit::relay::mpi::all_gather
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_all_gather(PyObject *, //self
                       PyObject *args,
                       PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::all_gather(send_node,
                               recv_node,
                               comm);
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
// conduit::relay::mpi::gather_using_schema
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_all_gather_using_schema(PyObject *, //self
                                    PyObject *args,
                                    PyObject *kwargs)
{
    PyObject   *py_src_node  = NULL;
    PyObject   *py_recv_node = NULL;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"send_node",
                                   "recv_node",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "OOn",
                                     const_cast<char**>(kwlist),
                                     &py_src_node,
                                     &py_recv_node,
                                     &mpi_comm_id))
    {
        return (NULL);
    }

    if(!PyConduit_Node_Check(py_src_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'send_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    if(!PyConduit_Node_Check(py_recv_node))
    {
        PyErr_SetString(PyExc_TypeError,
                        "'recv_node' argument must be a "
                        "conduit.Node instance");
        return NULL;
    }

    Node &send_node = *PyConduit_Node_Get_Node_Ptr(py_src_node);
    Node &recv_node = *PyConduit_Node_Get_Node_Ptr(py_recv_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::all_gather_using_schema(send_node,
                                            recv_node,
                                            comm);
    }
    catch(conduit::Error &e)
    {
        PyErr_SetString(PyExc_Exception,
                        e.message().c_str());
        return NULL;
    }

    Py_RETURN_NONE;
}

//-----------------------------------------------------------------------------
/// MPI broadcast
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
// conduit::relay::mpi::broadcast
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_broadcast(PyObject *, //self
                      PyObject *args,
                      PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Onn",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &root,
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

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::broadcast(node, root, comm);
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
// conduit::relay::mpi::broadcast_using_schema
//---------------------------------------------------------------------------//
static PyObject * 
PyRelay_MPI_broadcast_using_schema(PyObject *, //self
                                   PyObject *args,
                                   PyObject *kwargs)
{
    PyObject   *py_node  = NULL;
    Py_ssize_t  root;
    Py_ssize_t  mpi_comm_id;

    // TODO: future also accept mpi4py comm

    static const char *kwlist[] = {"node",
                                   "root",
                                   "comm",
                                   NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "Onn",
                                     const_cast<char**>(kwlist),
                                     &py_node,
                                     &root,
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

    Node &node = *PyConduit_Node_Get_Node_Ptr(py_node);

    // get c mpi comm hnd
    MPI_Comm comm = MPI_Comm_f2c(mpi_comm_id);

    try
    {
        relay::mpi::broadcast_using_schema(node, root, comm);
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
// Python Module Method Defs
//---------------------------------------------------------------------------//

// TODO: Future Support ?
// reduce (generic, reduce op as arg)
// all_reduce (generic, reduce op as arg)

// isend
// irecv
// wait_send
// wait_recv
// wait_all_send
// wait_all_recv
static PyMethodDef relay_mpi_python_funcs[] =
{
    //-----------------------------------------------------------------------//
    //-----------------------------------------------------------------------//
    {"about",
     _PyCFunction_CAST(PyRelay_MPI_about),
      METH_NOARGS,
      "About Relay MPI"},
    {"rank",
     _PyCFunction_CAST(PyRelay_MPI_rank),
      METH_VARARGS | METH_KEYWORDS,
      "MPI Comm Rank"},
    {"size",
     _PyCFunction_CAST(PyRelay_MPI_size),
      METH_VARARGS | METH_KEYWORDS,
      "MPI Comm Size"},
     // -- send + recv ---
    {"send",
     _PyCFunction_CAST(PyRelay_MPI_send),
      METH_VARARGS | METH_KEYWORDS,
      "Send Conduit Node via MPI Send"},
    {"recv",
     _PyCFunction_CAST(PyRelay_MPI_recv),
      METH_VARARGS | METH_KEYWORDS,
      "Receive Conduit Node via MPI Recv"},
    {"send_using_schema",
     _PyCFunction_CAST(PyRelay_MPI_send_using_schema),
      METH_VARARGS | METH_KEYWORDS,
      "Send Conduit Node and Schema via MPI Send"},
    {"recv_using_schema",
     _PyCFunction_CAST(PyRelay_MPI_recv_using_schema),
      METH_VARARGS | METH_KEYWORDS,
      "Receive Conduit Node and Schema via MPI Recv"},
     // -- reduce --
    {"sum_reduce",
     _PyCFunction_CAST(PyRelay_MPI_sum_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Sum Reduce a Conduit Node"},
    {"min_reduce",
     _PyCFunction_CAST(PyRelay_MPI_min_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Min Reduce a Conduit Node"},
    {"max_reduce",
     _PyCFunction_CAST(PyRelay_MPI_max_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Max Reduce a Conduit Node"},
    {"prod_reduce",
     _PyCFunction_CAST(PyRelay_MPI_prod_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Prod Reduce a Conduit Node"},
     // -- all reduce --
    {"sum_all_reduce",
     _PyCFunction_CAST(PyRelay_MPI_sum_all_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Sum All Reduce a Conduit Node"},
    {"min_all_reduce",
     _PyCFunction_CAST(PyRelay_MPI_min_all_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Min All Reduce a Conduit Node"},
    {"max_all_reduce",
     _PyCFunction_CAST(PyRelay_MPI_max_all_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Max All Reduce a Conduit Node"},
    {"prod_all_reduce",
     _PyCFunction_CAST(PyRelay_MPI_prod_all_reduce),
      METH_VARARGS | METH_KEYWORDS,
      "Prod All Reduce a Conduit Node"},
     // -- gather --
    {"gather",
     _PyCFunction_CAST(PyRelay_MPI_gather),
      METH_VARARGS | METH_KEYWORDS,
      "MPI Gather using Conduit Nodes"},
    {"all_gather",
     _PyCFunction_CAST(PyRelay_MPI_all_gather),
      METH_VARARGS | METH_KEYWORDS,
      "MPI All Gather using Conduit Nodes"},
    {"gather_using_schema",
     _PyCFunction_CAST(PyRelay_MPI_gather_using_schema),
      METH_VARARGS | METH_KEYWORDS,
      "MPI Gather using Conduit Nodes and their Schemas"},
    {"all_gather_using_schema",
     _PyCFunction_CAST(PyRelay_MPI_all_gather_using_schema),
      METH_VARARGS | METH_KEYWORDS,
      "MPI All Gather using Conduit Nodes and their Schemas"},
     // -- broadcast --
    {"broadcast",
     _PyCFunction_CAST(PyRelay_MPI_broadcast),
      METH_VARARGS | METH_KEYWORDS,
      "MPI Broadcast a Conduit Node"},
    {"broadcast_using_schema",
     _PyCFunction_CAST(PyRelay_MPI_broadcast_using_schema),
      METH_VARARGS | METH_KEYWORDS,
      "MPI Broadcast a Conduit Node and its Schema"},
     // -- isend + irecv ---
    // {"isend",
    //  _PyCFunction_CAST(PyRelay_MPI_send,
    //   METH_VARARGS | METH_KEYWORDS,
    //   "Send Conduit Node via MPI ISend"},
    // {"irecv",
    //  _PyCFunction_CAST(PyRelay_MPI_send,
    //   METH_VARARGS | METH_KEYWORDS,
    //   "Receive Conduit Node via MPI IRecv"},
    //-----------------------------------------------------------------------//
    // end relay mpi methods table
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
    #ifdef Py_LIMITED_API
    PyTypeObject*  PyRelay_MPI_Request_TYPE;
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
#endif

//---------------------------------------------------------------------------//
// Extra Module Setup Logic for Python3
//---------------------------------------------------------------------------//
#if defined(IS_PY3K)
//---------------------------------------------------------------------------//
static int
relay_mpi_python_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    #ifdef Py_LIMITED_API
    Py_VISIT(GETSTATE(m)->PyRelay_MPI_Request_TYPE);
    #endif
    return 0;
}

//---------------------------------------------------------------------------//
static int 
relay_mpi_python_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    #ifdef Py_LIMITED_API
    Py_CLEAR(GETSTATE(m)->PyRelay_MPI_Request_TYPE);
    #endif
    return 0;
}

//---------------------------------------------------------------------------//
static struct PyModuleDef relay_mpi_python_module_def = 
{
        PyModuleDef_HEAD_INIT,
        "relay_mpi_python",
        NULL,
        sizeof(struct module_state),
        relay_mpi_python_funcs,
        NULL,
        relay_mpi_python_traverse,
        relay_mpi_python_clear,
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
CONDUIT_RELAY_PYTHON_API PyObject * PyInit_conduit_relay_mpi_python(void)
#else
CONDUIT_RELAY_PYTHON_API void initconduit_relay_mpi_python(void)
#endif
//---------------------------------------------------------------------------//
{    
    //-----------------------------------------------------------------------//
    // create our main module
    //-----------------------------------------------------------------------//

#if defined(IS_PY3K)
    PyObject *relay_mpi_module = PyModule_Create(&relay_mpi_python_module_def);
#else
    PyObject *relay_mpi_module = Py_InitModule((char*)"conduit_relay_mpi_python",
                                               relay_mpi_python_funcs);
#endif


    if(relay_mpi_module == NULL)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    struct module_state *st = GETSTATE(relay_mpi_module);
    
    st->error = PyErr_NewException((char*)"relay_mpi_python.Error",
                                   NULL,
                                   NULL);
    if (st->error == NULL)
    {
        Py_DECREF(relay_mpi_module);
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
    module_state* state = GETSTATE(relay_mpi_module);
    state->PyRelay_MPI_Request_TYPE = (PyTypeObject *)PyType_FromModuleAndSpec((PyObject*)relay_mpi_module, &PyRelay_MPI_Request_SPEC, NULL);
    if (state->PyRelay_MPI_Request_TYPE == NULL)
    {
       PY_MODULE_INIT_RETURN_ERROR;
    }
    if (PyModule_AddType((PyObject*)relay_mpi_module,state->PyRelay_MPI_Request_TYPE) < 0)
    {
       PY_MODULE_INIT_RETURN_ERROR;
    }
    #else
    if (PyType_Ready(&PyRelay_MPI_Request_TYPE) < 0)
    {
        PY_MODULE_INIT_RETURN_ERROR;
    }

    //-----------------------------------------------------------------------//
    // add Request
    //-----------------------------------------------------------------------//
    Py_INCREF(&PyRelay_MPI_Request_TYPE);
    PyModule_AddObject(relay_mpi_module,
                       "Request",
                       (PyObject*)&PyRelay_MPI_Request_TYPE);
    #endif

#ifdef Py_LIMITED_API
    GLOBAL_MODULE = relay_mpi_module;
#endif

#if defined(IS_PY3K)
    return relay_mpi_module;
#endif

}

