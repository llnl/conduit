// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_schema.h
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_SCHEMA_H
#define CONDUIT_SCHEMA_H

#include <stdlib.h>
#include <stddef.h>

#include "conduit_bitwidth_style_types.h"
#include "conduit_exports.h"
#include "conduit_datatype.h"

//-----------------------------------------------------------------------------
// -- begin extern C
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// -- typedef for conduit_schema --
//-----------------------------------------------------------------------------

struct conduit_schema_impl;
typedef struct conduit_schema_impl conduit_schema;

//-----------------------------------------------------------------------------
// -- conduit_schema creation and destruction --
//-----------------------------------------------------------------------------

CONDUIT_API conduit_schema *conduit_schema_create();
CONDUIT_API void           conduit_schema_destroy(conduit_schema *cschema);

//-----------------------------------------------------------------------------
// -- schema set / reset --
//-----------------------------------------------------------------------------

CONDUIT_API void conduit_schema_reset(conduit_schema *cschema);
CONDUIT_API void conduit_schema_set(conduit_schema *cschema,
                                    const conduit_schema *cother);
CONDUIT_API void conduit_schema_set_dtype_id(conduit_schema *cschema,
                                             conduit_index_t dtype_id);
CONDUIT_API void conduit_schema_set_datatype(conduit_schema *cschema,
                                             const conduit_datatype *cdtype);
CONDUIT_API void conduit_schema_set_json(conduit_schema *cschema,
                                         const char *json_schema);

//-----------------------------------------------------------------------------
// -- schema info --
//-----------------------------------------------------------------------------

CONDUIT_API const conduit_datatype *conduit_schema_dtype(const conduit_schema *cschema);
CONDUIT_API int                    conduit_schema_is_root(const conduit_schema *cschema);
CONDUIT_API int                    conduit_schema_is_compact(const conduit_schema *cschema);
CONDUIT_API int                    conduit_schema_compatible(const conduit_schema *cschema,
                                                             const conduit_schema *cother);
CONDUIT_API int                    conduit_schema_equals(const conduit_schema *cschema,
                                                         const conduit_schema *cother);
CONDUIT_API conduit_index_t        conduit_schema_total_strided_bytes(const conduit_schema *cschema);
CONDUIT_API conduit_index_t        conduit_schema_total_bytes_compact(const conduit_schema *cschema);

//-----------------------------------------------------------------------------
// -- object and list interface methods --
//-----------------------------------------------------------------------------

CONDUIT_API conduit_schema *conduit_schema_fetch(conduit_schema *cschema,
                                                 const char *path);
CONDUIT_API conduit_schema *conduit_schema_fetch_existing(conduit_schema *cschema,
                                                          const char *path);
CONDUIT_API conduit_schema *conduit_schema_append(conduit_schema *cschema);
CONDUIT_API conduit_schema *conduit_schema_add_child(conduit_schema *cschema,
                                                     const char *name);
CONDUIT_API conduit_schema *conduit_schema_child(conduit_schema *cschema,
                                                 conduit_index_t idx);
CONDUIT_API conduit_schema *conduit_schema_child_by_name(conduit_schema *cschema,
                                                         const char *name);
CONDUIT_API conduit_index_t conduit_schema_number_of_children(const conduit_schema *cschema);
CONDUIT_API int conduit_schema_has_child(const conduit_schema *cschema,
                                         const char *name);
CONDUIT_API int conduit_schema_has_path(const conduit_schema *cschema,
                                        const char *path);
CONDUIT_API void conduit_schema_rename_child(conduit_schema *cschema,
                                             const char *current_name,
                                             const char *new_name);
CONDUIT_API void conduit_schema_remove_path(conduit_schema *cschema,
                                            const char *path);
CONDUIT_API void conduit_schema_remove_child(conduit_schema *cschema,
                                             conduit_index_t idx);
CONDUIT_API void conduit_schema_remove_child_by_name(conduit_schema *cschema,
                                                     const char *name);

//-----------------------------------------------------------------------------
// -- schema strings --
//-----------------------------------------------------------------------------

CONDUIT_API char *conduit_schema_name(const conduit_schema *cschema);
CONDUIT_API char *conduit_schema_path(const conduit_schema *cschema);
CONDUIT_API char *conduit_schema_to_json(const conduit_schema *cschema);
CONDUIT_API char *conduit_schema_to_yaml(const conduit_schema *cschema);

//-----------------------------------------------------------------------------
/// Destroys strings returned by the conduit schema C API.
//-----------------------------------------------------------------------------
CONDUIT_API void conduit_schema_string_destroy(char *str);

#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
// -- end extern C
//-----------------------------------------------------------------------------

#endif

