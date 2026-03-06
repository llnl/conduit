// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_schema_c.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_schema.h"

#include "conduit.hpp"
#include "conduit_cpp_to_c.hpp"

#include <stdlib.h>
#include <string.h>

#include <mutex>
#include <unordered_set>

#ifdef CONDUIT_PLATFORM_WINDOWS
    #define _conduit_strdup _strdup
#else
    #define _conduit_strdup strdup
#endif

//-----------------------------------------------------------------------------
// -- begin extern C
//-----------------------------------------------------------------------------

extern "C" {

using namespace conduit;

namespace
{
std::mutex g_conduit_schema_ownership_mutex;
std::unordered_set<Schema *> g_conduit_schema_owned;

void register_owned_schema(Schema *schema)
{
    std::lock_guard<std::mutex> lock(g_conduit_schema_ownership_mutex);
    g_conduit_schema_owned.insert(schema);
}

bool unregister_owned_schema(Schema *schema)
{
    std::lock_guard<std::mutex> lock(g_conduit_schema_ownership_mutex);
    return g_conduit_schema_owned.erase(schema) > 0;
}
}

//-----------------------------------------------------------------------------
conduit_schema *
conduit_schema_create()
{
    Schema *schema = new Schema();
    register_owned_schema(schema);
    return c_schema(schema);
}

//-----------------------------------------------------------------------------
void
conduit_schema_destroy(conduit_schema *cschema)
{
    Schema *schema = cpp_schema(cschema);
    if(schema == NULL)
    {
        return;
    }

    if(unregister_owned_schema(schema))
    {
        delete schema;
    }
}

//-----------------------------------------------------------------------------
void
conduit_schema_reset(conduit_schema *cschema)
{
    cpp_schema_ref(cschema).reset();
}

//-----------------------------------------------------------------------------
void
conduit_schema_set(conduit_schema *cschema,
                   const conduit_schema *cother)
{
    cpp_schema_ref(cschema).set(cpp_schema_ref(cother));
}

//-----------------------------------------------------------------------------
void
conduit_schema_set_dtype_id(conduit_schema *cschema,
                            conduit_index_t dtype_id)
{
    cpp_schema_ref(cschema).set(dtype_id);
}

//-----------------------------------------------------------------------------
void
conduit_schema_set_datatype(conduit_schema *cschema,
                            const conduit_datatype *cdtype)
{
    cpp_schema_ref(cschema).set(cpp_datatype_ref(cdtype));
}

//-----------------------------------------------------------------------------
void
conduit_schema_set_json(conduit_schema *cschema,
                        const char *json_schema)
{
    cpp_schema_ref(cschema).set(std::string(json_schema));
}

//-----------------------------------------------------------------------------
const conduit_datatype *
conduit_schema_dtype(const conduit_schema *cschema)
{
    return c_datatype(&cpp_schema_ref(cschema).dtype());
}

//-----------------------------------------------------------------------------
int
conduit_schema_is_root(const conduit_schema *cschema)
{
    return cpp_schema_ref(cschema).is_root() ? 1 : 0;
}

//-----------------------------------------------------------------------------
int
conduit_schema_is_compact(const conduit_schema *cschema)
{
    return cpp_schema_ref(cschema).is_compact() ? 1 : 0;
}

//-----------------------------------------------------------------------------
int
conduit_schema_compatible(const conduit_schema *cschema,
                          const conduit_schema *cother)
{
    return cpp_schema_ref(cschema).compatible(cpp_schema_ref(cother)) ? 1 : 0;
}

//-----------------------------------------------------------------------------
int
conduit_schema_equals(const conduit_schema *cschema,
                      const conduit_schema *cother)
{
    return cpp_schema_ref(cschema).equals(cpp_schema_ref(cother)) ? 1 : 0;
}

//-----------------------------------------------------------------------------
conduit_index_t
conduit_schema_total_strided_bytes(const conduit_schema *cschema)
{
    return cpp_schema_ref(cschema).total_strided_bytes();
}

//-----------------------------------------------------------------------------
conduit_index_t
conduit_schema_total_bytes_compact(const conduit_schema *cschema)
{
    return cpp_schema_ref(cschema).total_bytes_compact();
}

//-----------------------------------------------------------------------------
conduit_schema *
conduit_schema_fetch(conduit_schema *cschema,
                     const char *path)
{
    return c_schema(cpp_schema(cschema)->fetch_ptr(path));
}

//-----------------------------------------------------------------------------
conduit_schema *
conduit_schema_fetch_existing(conduit_schema *cschema,
                              const char *path)
{
    return c_schema(&cpp_schema(cschema)->fetch_existing(path));
}

//-----------------------------------------------------------------------------
conduit_schema *
conduit_schema_append(conduit_schema *cschema)
{
    return c_schema(&cpp_schema(cschema)->append());
}

//-----------------------------------------------------------------------------
conduit_schema *
conduit_schema_add_child(conduit_schema *cschema,
                         const char *name)
{
    return c_schema(&cpp_schema(cschema)->add_child(name));
}

//-----------------------------------------------------------------------------
conduit_schema *
conduit_schema_child(conduit_schema *cschema,
                     conduit_index_t idx)
{
    return c_schema(cpp_schema(cschema)->child_ptr(idx));
}

//-----------------------------------------------------------------------------
conduit_schema *
conduit_schema_child_by_name(conduit_schema *cschema,
                             const char *name)
{
    return c_schema(&cpp_schema(cschema)->child(std::string(name)));
}

//-----------------------------------------------------------------------------
conduit_index_t
conduit_schema_number_of_children(const conduit_schema *cschema)
{
    return cpp_schema_ref(cschema).number_of_children();
}

//-----------------------------------------------------------------------------
int
conduit_schema_has_child(const conduit_schema *cschema,
                         const char *name)
{
    return cpp_schema_ref(cschema).has_child(std::string(name)) ? 1 : 0;
}

//-----------------------------------------------------------------------------
int
conduit_schema_has_path(const conduit_schema *cschema,
                        const char *path)
{
    return cpp_schema_ref(cschema).has_path(std::string(path)) ? 1 : 0;
}

//-----------------------------------------------------------------------------
void
conduit_schema_rename_child(conduit_schema *cschema,
                            const char *current_name,
                            const char *new_name)
{
    cpp_schema_ref(cschema).rename_child(current_name, new_name);
}

//-----------------------------------------------------------------------------
void
conduit_schema_remove_path(conduit_schema *cschema,
                           const char *path)
{
    cpp_schema_ref(cschema).remove(std::string(path));
}

//-----------------------------------------------------------------------------
void
conduit_schema_remove_child(conduit_schema *cschema,
                            conduit_index_t idx)
{
    cpp_schema_ref(cschema).remove(idx);
}

//-----------------------------------------------------------------------------
void
conduit_schema_remove_child_by_name(conduit_schema *cschema,
                                    const char *name)
{
    cpp_schema_ref(cschema).remove_child(std::string(name));
}

//-----------------------------------------------------------------------------
char *
conduit_schema_name(const conduit_schema *cschema)
{
    return _conduit_strdup(cpp_schema_ref(cschema).name().c_str());
}

//-----------------------------------------------------------------------------
char *
conduit_schema_path(const conduit_schema *cschema)
{
    return _conduit_strdup(cpp_schema_ref(cschema).path().c_str());
}

//-----------------------------------------------------------------------------
char *
conduit_schema_to_json(const conduit_schema *cschema)
{
    return _conduit_strdup(cpp_schema_ref(cschema).to_json_default().c_str());
}

//-----------------------------------------------------------------------------
char *
conduit_schema_to_yaml(const conduit_schema *cschema)
{
    return _conduit_strdup(cpp_schema_ref(cschema).to_yaml_default().c_str());
}

//-----------------------------------------------------------------------------
void
conduit_schema_string_destroy(char *str)
{
    free(str);
}

}

