// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_python_common.h
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_PYTHON_COMMON_H
#define CONDUIT_PYTHON_COMMON_H

//-----------------------------------------------------------------------------
// -- Python includes (these must be included first) -- 
//-----------------------------------------------------------------------------
#include <Python.h>
#include <structmember.h>
#include "bytesobject.h"
#include <string.h> // for strdup

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
    // python 3.12 adds tq_watched 
    // python 3.13 adds tp_versions_used
    #if PY_VERSION_HEX >= 0x03080000
        #if PY_VERSION_HEX >= 0x030d0000
            // python 3.13 and newer tail
            #define PyVarObject_TAIL ,0, 0, 0, 0
        #else
            #if PY_VERSION_HEX == 0x030c0000 || PY_VERSION_HEX == 0x03090000
                // python 3.8 tail + python 3.12
                #define PyVarObject_TAIL ,0, 0, 0,
            #else
                // python 3.9 to 3.11 tail
                #define PyVarObject_TAIL ,0, 0
            #endif
        #endif
    #else
        // python tail when finalize is part of struct
        #define PyVarObject_TAIL ,0
    #endif
#else
// python tail when finalize is not part of struct
#define PyVarObject_TAIL
#endif

//-----------------------------------------------------------------------------
// Cast helper for  Python < 3.11 to silence -Wcast-function-type */
//-----------------------------------------------------------------------------
#ifndef _PyCFunction_CAST
#  define _PyCFunction_CAST(func) \
     reinterpret_cast<PyCFunction>(reinterpret_cast<void(*)()>(func))
#endif


#endif



