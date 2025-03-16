// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.


//-----------------------------------------------------------------------------
///
/// file: conduit_node.h
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_NODE_H
#define CONDUIT_NODE_H

#include <stdlib.h>
#include <stddef.h>

#include "conduit_bitwidth_style_types.h"
#include "conduit_endianness_types.h"
#include "conduit_exports.h"
#include "conduit_datatype.h"

//-----------------------------------------------------------------------------
// -- begin extern C
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// -- typedef for conduit_node --
//-----------------------------------------------------------------------------

struct conduit_node_impl;
typedef struct conduit_node_impl  conduit_node;

//-----------------------------------------------------------------------------
// -- conduit_node creation and destruction --
//-----------------------------------------------------------------------------

CONDUIT_API conduit_node *conduit_node_create();
CONDUIT_API void          conduit_node_destroy(conduit_node *cnode);


//-----------------------------------------------------------------------------
// -- object and list interface methods --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CONDUIT_API conduit_node *conduit_node_fetch(conduit_node *cnode,
                                             const char *path);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_node *conduit_node_fetch_existing(conduit_node *cnode,
                                                      const char *path);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_node *conduit_node_append(conduit_node *cnode);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_node *conduit_node_add_child(conduit_node *cnode,
                                                 const char *name);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_node *conduit_node_child(conduit_node *cnode,
                                             conduit_index_t idx);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_node *conduit_node_child_by_name(conduit_node *cnode,
                                                     const char *name);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_index_t conduit_node_number_of_children(conduit_node *cnode);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_index_t conduit_node_number_of_elements(conduit_node *cnode);

//-----------------------------------------------------------------------------
CONDUIT_API void conduit_node_reset(conduit_node *cnode);

//-----------------------------------------------------------------------------
CONDUIT_API void conduit_node_move(conduit_node *cnode_a,
                                   conduit_node *cnode_b);

//-----------------------------------------------------------------------------
CONDUIT_API void conduit_node_swap(conduit_node *cnode_a,
                                   conduit_node *cnode_b);

//-----------------------------------------------------------------------------
/// remove path
CONDUIT_API void conduit_node_remove_path(conduit_node *cnode,
                                          const char *path);

//-----------------------------------------------------------------------------
/// remove child by index
CONDUIT_API void conduit_node_remove_child(conduit_node *cnode,
                                           conduit_index_t idx);

//-----------------------------------------------------------------------------
/// remove child by name
CONDUIT_API void conduit_node_remove_child_by_name(conduit_node *cnode,
                                                   const char *name);

//-----------------------------------------------------------------------------
// TODO:  for Node::name() in c, the caller must free the result, 
// before we expose this, we need to understand the implications of this in 
// fortran
// NOTE: the fortran version could pass in the buffer to contain the path.
CONDUIT_API char *conduit_node_name(const conduit_node *cnode);

//-----------------------------------------------------------------------------
// TODO:  for Node::path() in c, the caller must free the result, 
// before we expose this, we need to understand the implications of this in 
// fortran
// NOTE: the fortran version could pass in the buffer to contain the path.
CONDUIT_API char *conduit_node_path(const conduit_node *cnode);

//-----------------------------------------------------------------------------
CONDUIT_API int conduit_node_has_child(const conduit_node *cnode, 
                                       const char *name);
CONDUIT_API int conduit_node_has_path(const conduit_node *cnode, 
                                      const char *path);

//-----------------------------------------------------------------------------
/// rename a child (object interface)
CONDUIT_API void conduit_node_rename_child(conduit_node *cnode,
                                           const char *current_name,
                                           const char *new_name);

//-----------------------------------------------------------------------------
// -- node info --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
CONDUIT_API int conduit_node_is_root(conduit_node *cnode);
CONDUIT_API int conduit_node_is_data_external(const conduit_node *cnode);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_node *conduit_node_parent(conduit_node *cnode);

//-----------------------------------------------------------------------------
CONDUIT_API conduit_index_t conduit_node_total_strided_bytes(const conduit_node *cnode);
CONDUIT_API conduit_index_t conduit_node_total_bytes_compact(const conduit_node *cnode);
CONDUIT_API conduit_index_t conduit_node_total_bytes_allocated(const conduit_node *cnode);
    
//-----------------------------------------------------------------------------
CONDUIT_API int conduit_node_is_compact(const conduit_node *cnode);


//-----------------------------------------------------------------------------
CONDUIT_API int conduit_node_compatible(const conduit_node *cnode,
                                        const conduit_node *cother);

//-----------------------------------------------------------------------------
CONDUIT_API int conduit_node_is_contiguous(const conduit_node *cnode);
CONDUIT_API int conduit_node_contiguous_with_node(const conduit_node *cnode,
                                                  const conduit_node *cother);
CONDUIT_API int conduit_node_contiguous_with_address(const conduit_node *cnode,
                                                     void *address);

//-----------------------------------------------------------------------------
CONDUIT_API int conduit_node_diff(const conduit_node *cnode,
                                  const conduit_node *cother,
                                  conduit_node *cinfo,
                                  conduit_float64 epsilon);

CONDUIT_API int conduit_node_diff_compatible(const conduit_node *cnode,
                                             const conduit_node *cother,
                                             conduit_node *cinfo,
                                             conduit_float64 epsilon);

CONDUIT_API void conduit_node_info(const conduit_node *cnode,
                                   conduit_node *cnres);
//-----------------------------------------------------------------------------
CONDUIT_API void conduit_node_print(conduit_node *cnode);
CONDUIT_API void conduit_node_print_detailed(conduit_node *cnode);


//-----------------------------------------------------------------------------
// -- compaction methods ---
//-----------------------------------------------------------------------------
CONDUIT_API void conduit_node_compact_to(const conduit_node *cnode,
                                         conduit_node *cnres);

//-----------------------------------------------------------------------------
// -- update methods ---
//-----------------------------------------------------------------------------
CONDUIT_API void conduit_node_update(conduit_node *cnode,
                                     const conduit_node *cother);
CONDUIT_API void conduit_node_update_compatible(conduit_node *cnode,
                                                const conduit_node *cother);
CONDUIT_API void conduit_node_update_external(conduit_node *cnode,
                                              conduit_node *cother);

//-----------------------------------------------------------------------------
// -- basic io, parsing, and generation ---
//-----------------------------------------------------------------------------
CONDUIT_API void conduit_node_parse(conduit_node *cnode,
                                    const char* schema,
                                    const char* protocol);

CONDUIT_API void conduit_node_generate(conduit_node *cnode,
                                       const char* schema,
                                       const char* protocol,
                                       void *data);

CONDUIT_API void conduit_node_generate_external(conduit_node *cnode,
                                                const char* schema,
                                                const char* protocol,
                                                void *data);

CONDUIT_API void conduit_node_save(conduit_node *cnode,
                                   const char* path,
                                   const char* protocol);

CONDUIT_API void conduit_node_load(conduit_node *cnode,
                                   const char* path,
                                   const char* protocol);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Conduit Node to_string Methods
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// caller must free the resulting string for all of these methods
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// the caller must free the result
CONDUIT_API char* conduit_node_to_json(const conduit_node *cnode);
// the caller must free the result
CONDUIT_API char* conduit_node_to_json_with_options(const conduit_node *cnode,
                                                    const conduit_node *copts);

// the caller must free the result
CONDUIT_API char* conduit_node_to_yaml(const conduit_node *cnode);
// the caller must free the result
CONDUIT_API char* conduit_node_to_yaml_with_options(const conduit_node *cnode,
                                                    const conduit_node *copts);

// the caller must free the result
CONDUIT_API char* conduit_node_to_string(const conduit_node *cnode);
// the caller must free the result
CONDUIT_API char* conduit_node_to_string_with_options(const conduit_node *cnode,
                                                      const conduit_node *copts);

// the caller must free the result
CONDUIT_API char* conduit_node_to_summary_string(const conduit_node *cnode);
// the caller must free the result
CONDUIT_API char* conduit_node_to_summary_string_with_options(const conduit_node *cnode,
                                                              const conduit_node *copts);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Conduit Node "Set" Methods
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// -- set for generic types --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_node(conduit_node *cnode,
                                           conduit_node *data);

    // TODO: These req c-interfaces for datatype, schema, etc
    //void set_dtype(const DataType &dtype);
    //void set_schema(const Schema &schema);
    //void set_data_using_dtype(const DataType &dtype, void *data);
    //void set_data_using_schema(const Schema &schema, void *data);

//-----------------------------------------------------------------------------
// -- set_path for generic types --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_node(conduit_node *cnode,
                                                const char* path,
                                                conduit_node *data);

    // TODO: These req c-interfaces for datatype, schema, etc
    // //-------------------------------------------------------------------------
    // void set_path_dtype(const std::string &path,
    //                     const DataType& dtype);
    //
    // //-------------------------------------------------------------------------
    // void set_path_schema(const std::string &path,
    //                      const Schema &schema);
    //
    // //-------------------------------------------------------------------------
    // void set_path_data_using_schema(const std::string &path,
    //                                 const Schema &schema,
    //                                 void *data);
    // //-------------------------------------------------------------------------
    // void set_path_data_using_dtype(const std::string &path,
    //                                const DataType &dtype,
    //                                void *data);

//-----------------------------------------------------------------------------
// -- set_external for generic types --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_node(conduit_node *cnode,
                                                    conduit_node *data);

    // TODO: These req c-interfaces for datatype, schema, etc
    // //-------------------------------------------------------------------------
    // void set_external_data_using_schema(const Schema &schema,
    //                                     void *data);
    //
    // //-------------------------------------------------------------------------
    // void set_external_data_using_dtype(const DataType &dtype,
    //                                    void *data);


//-----------------------------------------------------------------------------
// -- set_path_external for generic types --
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_node(conduit_node *cnode,
                                                         const char *path,
                                                         conduit_node *data);

    // TODO: These req c-interfaces for datatype, schema, etc
    // //-------------------------------------------------------------------------
    // void    set_path_external_data_using_schema(const std::string &path,
    //                                             const Schema &schema,
    //                                             void *data);
    //
    // //-------------------------------------------------------------------------
    // void    set_path_external_data_using_dtype(const std::string &path,
    //                                            const DataType &dtype,


//-----------------------------------------------------------------------------
// -- set for string cases --
//-----------------------------------------------------------------------------

   //-------------------------------------------------------------------------
   // set
   //-------------------------------------------------------------------------
   CONDUIT_API void conduit_node_set_char8_str(conduit_node *cnode,
                                               const char *value);

   //-------------------------------------------------------------------------
   // set_path
   //-------------------------------------------------------------------------
   CONDUIT_API void conduit_node_set_path_char8_str(conduit_node *cnode,
                                                    const char *path,
                                                    const char *value);

   //-------------------------------------------------------------------------
   // set_external
   //-------------------------------------------------------------------------
   CONDUIT_API void conduit_node_set_external_char8_str(conduit_node *cnode,
                                                        char *value);

   //-------------------------------------------------------------------------
   // set_path_external
   //-------------------------------------------------------------------------
   CONDUIT_API void conduit_node_set_path_external_char8_str(conduit_node *cnode,
                                                             const char *path,
                                                             char *value);


//-----------------------------------------------------------------------------
// -- set for numeric types --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// For both bitwidth style and c style numeric types, we provide these variants
// of "set":
//
// * set: scalar, pointer, and pointer detailed variants
// * set_path: scalar, pointer, and pointer detailed variants
// * set_external: pointer, and pointer detailed variants
// * set_path_external: pointer, and pointer detailed variants
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// -- set for bitwidth style types --
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- set for scalar bitwidth style types ---
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set bitwidth style signed integer scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int8(conduit_node *cnode,
                                           conduit_int8 value);

    CONDUIT_API void conduit_node_set_int16(conduit_node *cnode,
                                            conduit_int16 value);

    CONDUIT_API void conduit_node_set_int32(conduit_node *cnode,
                                            conduit_int32 value);

    CONDUIT_API void conduit_node_set_int64(conduit_node *cnode,
                                            conduit_int64 value);
    //-------------------------------------------------------------------------
    // set bitwidth style unsigned integer scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint8(conduit_node *cnode,
                                            conduit_uint8 value);

    CONDUIT_API void conduit_node_set_uint16(conduit_node *cnode,
                                             conduit_uint16 value);

    CONDUIT_API void conduit_node_set_uint32(conduit_node *cnode,
                                             conduit_uint32 value);

    CONDUIT_API void conduit_node_set_uint64(conduit_node *cnode,
                                             conduit_uint64 value);

    //-------------------------------------------------------------------------
    // set bitwidth style floating point scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float32(conduit_node *cnode,
                                               conduit_float32 value);

    CONDUIT_API void conduit_node_set_float64(conduit_node *cnode,
                                              conduit_float64 value);


//-----------------------------------------------------------------------------
// -- set via bitwidth style pointers for (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set bitwidth signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int8_ptr(conduit_node *cnode,
                                               conduit_int8 *data,
                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int8_ptr_detailed(conduit_node *cnode,
                                                        conduit_int8 *data,
                                                        conduit_index_t num_elements,
                                                        conduit_index_t offset,
                                                        conduit_index_t stride,
                                                        conduit_index_t element_bytes,
                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int16_ptr(conduit_node *cnode,
                                                conduit_int16 *data,
                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int16_ptr_detailed(conduit_node *cnode,
                                                         conduit_int16 *data,
                                                         conduit_index_t num_elements,
                                                         conduit_index_t offset,
                                                         conduit_index_t stride,
                                                         conduit_index_t element_bytes,
                                                         conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int32_ptr(conduit_node *cnode,
                                                conduit_int32 *data,
                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int32_ptr_detailed(conduit_node *cnode,
                                                         conduit_int32 *data,
                                                         conduit_index_t num_elements,
                                                         conduit_index_t offset,
                                                         conduit_index_t stride,
                                                         conduit_index_t element_bytes,
                                                         conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int64_ptr(conduit_node *cnode,
                                                conduit_int64 *data,
                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int64_ptr_detailed(conduit_node *cnode,
                                                         conduit_int64 *data,
                                                         conduit_index_t num_elements,
                                                         conduit_index_t offset,
                                                         conduit_index_t stride,
                                                         conduit_index_t element_bytes,
                                                         conduit_index_t endianness);


    //-------------------------------------------------------------------------
    // set bitwidth unsigned signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint8_ptr(conduit_node *cnode,
                                                conduit_uint8 *data,
                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint8_ptr_detailed(conduit_node *cnode,
                                                         conduit_uint8 *data,
                                                         conduit_index_t num_elements,
                                                         conduit_index_t offset,
                                                         conduit_index_t stride,
                                                         conduit_index_t element_bytes,
                                                         conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint16_ptr(conduit_node *cnode,
                                                 conduit_uint16 *data,
                                                 conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint16_ptr_detailed(conduit_node *cnode,
                                                          conduit_uint16 *data,
                                                          conduit_index_t num_elements,
                                                          conduit_index_t offset,
                                                          conduit_index_t stride,
                                                          conduit_index_t element_bytes,
                                                          conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint32_ptr(conduit_node *cnode,
                                                 conduit_uint32 *data,
                                                 conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint32_ptr_detailed(conduit_node *cnode,
                                                          conduit_uint32 *data,
                                                          conduit_index_t num_elements,
                                                          conduit_index_t offset,
                                                          conduit_index_t stride,
                                                          conduit_index_t element_bytes,
                                                          conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint64_ptr(conduit_node *cnode,
                                                 conduit_uint64 *data,
                                                 conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_uint64_ptr_detailed(conduit_node *cnode,
                                                          conduit_uint64 *data,
                                                          conduit_index_t num_elements,
                                                          conduit_index_t offset,
                                                          conduit_index_t stride,
                                                          conduit_index_t element_bytes,
                                                          conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set bitwidth floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float32_ptr(conduit_node *cnode,
                                                  conduit_float32 *data,
                                                  conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float32_ptr_detailed(conduit_node *cnode,
                                                           conduit_float32 *data,
                                                           conduit_index_t num_elements,
                                                           conduit_index_t offset,
                                                           conduit_index_t stride,
                                                           conduit_index_t element_bytes,
                                                           conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float64_ptr(conduit_node *cnode,
                                                  conduit_float64 *data,
                                                  conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float64_ptr_detailed(conduit_node *cnode,
                                                           conduit_float64 *data,
                                                           conduit_index_t num_elements,
                                                           conduit_index_t offset,
                                                           conduit_index_t stride,
                                                           conduit_index_t element_bytes,
                                                           conduit_index_t endianness);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// -- set_path for bitwidth style types --
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- set path for bitwidth scalar types ---
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set_path bitwidth signed integer scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int8(conduit_node *cnode,
                                                const char *path,
                                                conduit_int8 value);

    CONDUIT_API void conduit_node_set_path_int16(conduit_node *cnode,
                                                 const char *path,
                                                 conduit_int16 value);

     CONDUIT_API void conduit_node_set_path_int32(conduit_node *cnode,
                                                  const char *path,
                                                  conduit_int32 value);

     CONDUIT_API void conduit_node_set_path_int64(conduit_node *cnode,
                                                  const char *path,
                                                  conduit_int64 value);
    //-------------------------------------------------------------------------
    // set_path bitwidth unsigned integer scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint8(conduit_node *cnode,
                                                 const char *path,
                                                 conduit_uint8 value);

    CONDUIT_API void conduit_node_set_path_uint16(conduit_node *cnode,
                                                  const char *path,
                                                  conduit_uint16 value);

    CONDUIT_API void conduit_node_set_path_uint32(conduit_node *cnode,
                                                  const char *path,
                                                  conduit_uint32 value);

    CONDUIT_API void conduit_node_set_path_uint64(conduit_node *cnode,
                                                  const char *path,
                                                  conduit_uint64 value);

    //-------------------------------------------------------------------------
    // set_path floating point scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float32(conduit_node *cnode,
                                                   const char *path,
                                                   conduit_float32 value);

    CONDUIT_API void conduit_node_set_path_float64(conduit_node *cnode,
                                                   const char *path,
                                                   conduit_float64 value);


//-----------------------------------------------------------------------------
// -- set path via bitwidth style pointers (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set_path bitwidth signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int8_ptr(conduit_node *cnode,
                                                    const char *path,
                                                    conduit_int8 *data,
                                                    conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int8_ptr_detailed(conduit_node *cnode,
                                                             const char *path,
                                                             conduit_int8 *data,
                                                             conduit_index_t num_elements,
                                                             conduit_index_t offset,
                                                             conduit_index_t stride,
                                                             conduit_index_t element_bytes,
                                                             conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int16_ptr(conduit_node *cnode,
                                                     const char *path,
                                                     conduit_int16 *data,
                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int16_ptr_detailed(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_int16 *data,
                                                              conduit_index_t num_elements,
                                                              conduit_index_t offset,
                                                              conduit_index_t stride,
                                                              conduit_index_t element_bytes,
                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int32_ptr(conduit_node *cnode,
                                                     const char *path,
                                                     conduit_int32 *data,
                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int32_ptr_detailed(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_int32 *data,
                                                              conduit_index_t num_elements,
                                                              conduit_index_t offset,
                                                              conduit_index_t stride,
                                                              conduit_index_t element_bytes,
                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int64_ptr(conduit_node *cnode,
                                                     const char *path,
                                                     conduit_int64 *data,
                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int64_ptr_detailed(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_int64 *data,
                                                              conduit_index_t num_elements,
                                                              conduit_index_t offset,
                                                              conduit_index_t stride,
                                                              conduit_index_t element_bytes,
                                                              conduit_index_t endianness);


    //-------------------------------------------------------------------------
    // set_path bitwidth unsigned signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint8_ptr(conduit_node *cnode,
                                                     const char *path,
                                                     conduit_uint8 *data,
                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint8_ptr_detailed(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_uint8 *data,
                                                              conduit_index_t num_elements,
                                                              conduit_index_t offset,
                                                              conduit_index_t stride,
                                                              conduit_index_t element_bytes,
                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint16_ptr(conduit_node *cnode,
                                                      const char *path,
                                                      conduit_uint16 *data,
                                                      conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint16_ptr_detailed(conduit_node *cnode,
                                                               const char *path,
                                                               conduit_uint16 *data,
                                                               conduit_index_t num_elements,
                                                               conduit_index_t offset,
                                                               conduit_index_t stride,
                                                               conduit_index_t element_bytes,
                                                               conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint32_ptr(conduit_node *cnode,
                                                      const char *path,
                                                      conduit_uint32 *data,
                                                      conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint32_ptr_detailed(conduit_node *cnode,
                                                               const char *path,
                                                               conduit_uint32 *data,
                                                               conduit_index_t num_elements,
                                                               conduit_index_t offset,
                                                               conduit_index_t stride,
                                                               conduit_index_t element_bytes,
                                                               conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint64_ptr(conduit_node *cnode,
                                                      const char *path,
                                                      conduit_uint64 *data,
                                                      conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_uint64_ptr_detailed(conduit_node *cnode,
                                                               const char *path,
                                                               conduit_uint64 *data,
                                                               conduit_index_t num_elements,
                                                               conduit_index_t offset,
                                                               conduit_index_t stride,
                                                               conduit_index_t element_bytes,
                                                               conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_path bitwidth floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float32_ptr(conduit_node *cnode,
                                                       const char *path,
                                                       conduit_float32 *data,
                                                       conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float32_ptr_detailed(conduit_node *cnode,
                                                                const char *path,
                                                                conduit_float32 *data,
                                                                conduit_index_t num_elements,
                                                                conduit_index_t offset,
                                                                conduit_index_t stride,
                                                                conduit_index_t element_bytes,
                                                                conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float64_ptr(conduit_node *cnode,
                                                       const char *path,
                                                       conduit_float64 *data,
                                                       conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float64_ptr_detailed(conduit_node *cnode,
                                                                const char *path,
                                                                conduit_float64 *data,
                                                                conduit_index_t num_elements,
                                                                conduit_index_t offset,
                                                                conduit_index_t stride,
                                                                conduit_index_t element_bytes,
                                                                conduit_index_t endianness);


//-----------------------------------------------------------------------------
// -- set_external via pointers (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set_external bitwidth signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int8_ptr(conduit_node *cnode,
                                                        conduit_int8 *data,
                                                        conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int8_ptr_detailed(conduit_node *cnode,
                                                                  conduit_int8 *data,
                                                                  conduit_index_t num_elements,
                                                                  conduit_index_t offset,
                                                                  conduit_index_t stride,
                                                                  conduit_index_t element_bytes,
                                                                  conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int16_ptr(conduit_node *cnode,
                                                         conduit_int16 *data,
                                                         conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int16_ptr_detailed(conduit_node *cnode,
                                                                 conduit_int16 *data,
                                                                 conduit_index_t num_elements,
                                                                 conduit_index_t offset,
                                                                 conduit_index_t stride,
                                                                 conduit_index_t element_bytes,
                                                                 conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int32_ptr(conduit_node *cnode,
                                                         conduit_int32 *data,
                                                         conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int32_ptr_detailed(conduit_node *cnode,
                                                                  conduit_int32 *data,
                                                                  conduit_index_t num_elements,
                                                                  conduit_index_t offset,
                                                                  conduit_index_t stride,
                                                                  conduit_index_t element_bytes,
                                                                  conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int64_ptr(conduit_node *cnode,
                                                         conduit_int64 *data,
                                                         conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int64_ptr_detailed(conduit_node *cnode,
                                                                  conduit_int64 *data,
                                                                  conduit_index_t num_elements,
                                                                  conduit_index_t offset,
                                                                  conduit_index_t stride,
                                                                  conduit_index_t element_bytes,
                                                                  conduit_index_t endianness);


    //-------------------------------------------------------------------------
    // set_external bitwidth unsigned integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint8_ptr(conduit_node *cnode,
                                                         conduit_uint8 *data,
                                                         conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint8_ptr_detailed(conduit_node *cnode,
                                                                  conduit_uint8 *data,
                                                                  conduit_index_t num_elements,
                                                                  conduit_index_t offset,
                                                                  conduit_index_t stride,
                                                                  conduit_index_t element_bytes,
                                                                  conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint16_ptr(conduit_node *cnode,
                                                          conduit_uint16 *data,
                                                          conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint16_ptr_detailed(conduit_node *cnode,
                                                                   conduit_uint16 *data,
                                                                   conduit_index_t num_elements,
                                                                   conduit_index_t offset,
                                                                   conduit_index_t stride,
                                                                   conduit_index_t element_bytes,
                                                                   conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint32_ptr(conduit_node *cnode,
                                                          conduit_uint32 *data,
                                                          conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint32_ptr_detailed(conduit_node *cnode,
                                                                   conduit_uint32 *data,
                                                                   conduit_index_t num_elements,
                                                                   conduit_index_t offset,
                                                                   conduit_index_t stride,
                                                                   conduit_index_t element_bytes,
                                                                   conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint64_ptr(conduit_node *cnode,
                                                          conduit_uint64 *data,
                                                          conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_uint64_ptr_detailed(conduit_node *cnode,
                                                                   conduit_uint64 *data,
                                                                   conduit_index_t num_elements,
                                                                   conduit_index_t offset,
                                                                   conduit_index_t stride,
                                                                   conduit_index_t element_bytes,
                                                                   conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_external bitwidth floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_float32_ptr(conduit_node *cnode,
                                                           conduit_float32 *data,
                                                           conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_float32_ptr_detailed(conduit_node *cnode,
                                                                    conduit_float32 *data,
                                                                    conduit_index_t num_elements,
                                                                    conduit_index_t offset,
                                                                    conduit_index_t stride,
                                                                    conduit_index_t element_bytes,
                                                                    conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_float64_ptr(conduit_node *cnode,
                                                           conduit_float64 *data,
                                                           conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_float64_ptr_detailed(conduit_node *cnode,
                                                                    conduit_float64 *data,
                                                                    conduit_index_t num_elements,
                                                                    conduit_index_t offset,
                                                                    conduit_index_t stride,
                                                                    conduit_index_t element_bytes,
                                                                    conduit_index_t endianness);

  //-------------------------------------------------------------------------
  // set_path_external
  //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // set_path_external bitwidth signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int8_ptr(conduit_node *cnode,
                                                             const char *path,
                                                             conduit_int8 *data,
                                                             conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int8_ptr_detailed(conduit_node *cnode,
                                                                      const char *path,
                                                                      conduit_int8 *data,
                                                                      conduit_index_t num_elements,
                                                                      conduit_index_t offset,
                                                                      conduit_index_t stride,
                                                                      conduit_index_t element_bytes,
                                                                      conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int16_ptr(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_int16 *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int16_ptr_detailed(conduit_node *cnode,
                                                                       const char *path,
                                                                       conduit_int16 *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int32_ptr(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_int32 *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int32_ptr_detailed(conduit_node *cnode,
                                                                       const char *path,
                                                                       conduit_int32 *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int64_ptr(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_int64 *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int64_ptr_detailed(conduit_node *cnode,
                                                                       const char *path,
                                                                       conduit_int64 *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_path_external bitwidth unsigned integer pointer cases
    //-------------------------------------------------------------------------

    CONDUIT_API void conduit_node_set_path_external_uint8_ptr(conduit_node *cnode,
                                                              const char *path,
                                                              conduit_uint8 *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_uint8_ptr_detailed(conduit_node *cnode,
                                                                       const char *path,
                                                                       conduit_uint8 *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_uint16_ptr(conduit_node *cnode,
                                                               const char *path,
                                                               conduit_uint16 *data,
                                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_uint16_ptr_detailed(conduit_node *cnode,
                                                                        const char *path,
                                                                        conduit_uint16 *data,
                                                                        conduit_index_t num_elements,
                                                                        conduit_index_t offset,
                                                                        conduit_index_t stride,
                                                                        conduit_index_t element_bytes,
                                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_uint32_ptr(conduit_node *cnode,
                                                               const char *path,
                                                               conduit_uint32 *data,
                                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_uint32_ptr_detailed(conduit_node *cnode,
                                                                        const char *path,
                                                                        conduit_uint32 *data,
                                                                        conduit_index_t num_elements,
                                                                        conduit_index_t offset,
                                                                        conduit_index_t stride,
                                                                        conduit_index_t element_bytes,
                                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_uint64_ptr(conduit_node *cnode,
                                                               const char *path,
                                                               conduit_uint64 *data,
                                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_uint64_ptr_detailed(conduit_node *cnode,
                                                                        const char *path,
                                                                        conduit_uint64 *data,
                                                                        conduit_index_t num_elements,
                                                                        conduit_index_t offset,
                                                                        conduit_index_t stride,
                                                                        conduit_index_t element_bytes,
                                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_path_external bitwidth floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_float32_ptr(conduit_node *cnode,
                                                                const char *path,
                                                                conduit_float32 *data,
                                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_float32_ptr_detailed(conduit_node *cnode,
                                                                         const char *path,
                                                                         conduit_float32 *data,
                                                                         conduit_index_t num_elements,
                                                                         conduit_index_t offset,
                                                                         conduit_index_t stride,
                                                                         conduit_index_t element_bytes,
                                                                         conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_float64_ptr(conduit_node *cnode,
                                                                const char *path,
                                                                conduit_float64 *data,
                                                                conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_float64_ptr_detailed(conduit_node *cnode,
                                                                         const char *path,
                                                                         conduit_float64 *data,
                                                                         conduit_index_t num_elements,
                                                                         conduit_index_t offset,
                                                                         conduit_index_t stride,
                                                                         conduit_index_t element_bytes,
                                                                         conduit_index_t endianness);



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// -- set for cstyle types --
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- set for scalar cstyle types ---
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // set cstyle native integer scalar types
    //-------------------------------------------------------------------------

    CONDUIT_API void conduit_node_set_char(conduit_node *cnode,
                                           char value);
    CONDUIT_API void conduit_node_set_short(conduit_node *cnode,
                                            short value);

    CONDUIT_API void conduit_node_set_int(conduit_node *cnode,
                                          int value);

    CONDUIT_API void conduit_node_set_long(conduit_node *cnode,
                                           long value);

    //-------------------------------------------------------------------------
    // set cstyle signed integer scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_signed_char(conduit_node *cnode,
                                                  signed char value);

    CONDUIT_API void conduit_node_set_signed_short(conduit_node *cnode,
                                                   signed short value);

    CONDUIT_API void conduit_node_set_signed_int(conduit_node *cnode,
                                                 signed int value);

    CONDUIT_API void conduit_node_set_signed_long(conduit_node *cnode,
                                                  signed long value);

    //-------------------------------------------------------------------------
    // set cstyle unsigned integer scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_char(conduit_node *cnode,
                                                    unsigned char value);

    CONDUIT_API void conduit_node_set_unsigned_short(conduit_node *cnode,
                                                     unsigned short value);

    CONDUIT_API void conduit_node_set_unsigned_int(conduit_node *cnode,
                                                   unsigned int value);

    CONDUIT_API void conduit_node_set_unsigned_long(conduit_node *cnode,
                                                    unsigned long value);

    //-------------------------------------------------------------------------
    // set cstyle floating point scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float(conduit_node *cnode,
                                            float value);

    CONDUIT_API void conduit_node_set_double(conduit_node *cnode,
                                             double value);

//-----------------------------------------------------------------------------
// -- set via cstyle pointers for (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set cstyle native pointer cases
    //-------------------------------------------------------------------------

    CONDUIT_API void conduit_node_set_char_ptr(conduit_node *cnode,
                                               char *data,
                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_char_ptr_detailed(conduit_node *cnode,
                                                        char *data,
                                                        conduit_index_t num_elements,
                                                        conduit_index_t offset,
                                                        conduit_index_t stride,
                                                        conduit_index_t element_bytes,
                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_short_ptr(conduit_node *cnode,
                                                short *data,
                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_short_ptr_detailed(conduit_node *cnode,
                                                         short *data,
                                                         conduit_index_t num_elements,
                                                         conduit_index_t offset,
                                                         conduit_index_t stride,
                                                         conduit_index_t element_bytes,
                                                         conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int_ptr(conduit_node *cnode,
                                              int *data,
                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_int_ptr_detailed(conduit_node *cnode,
                                                       int *data,
                                                       conduit_index_t num_elements,
                                                       conduit_index_t offset,
                                                       conduit_index_t stride,
                                                       conduit_index_t element_bytes,
                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_long_ptr(conduit_node *cnode,
                                               long *data,
                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_long_ptr_detailed(conduit_node *cnode,
                                                        long *data,
                                                        conduit_index_t num_elements,
                                                        conduit_index_t offset,
                                                        conduit_index_t stride,
                                                        conduit_index_t element_bytes,
                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set cstyle signed integer pointer cases
    //-------------------------------------------------------------------------

    CONDUIT_API void conduit_node_set_signed_char_ptr(conduit_node *cnode,
                                                      signed char *data,
                                                      conduit_index_t num_elements);

    CONDUIT_API void conduit_node_set_signed_char_ptr_detailed(conduit_node *cnode,
                                                               signed char *data,
                                                               conduit_index_t num_elements,
                                                               conduit_index_t offset,
                                                               conduit_index_t stride,
                                                               conduit_index_t element_bytes,
                                                               conduit_index_t endianness);


    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_signed_short_ptr(conduit_node *cnode,
                                                       signed short *data,
                                                       conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_signed_short_ptr_detailed(conduit_node *cnode,
                                                                signed short *data,
                                                                conduit_index_t num_elements,
                                                                conduit_index_t offset,
                                                                conduit_index_t stride,
                                                                conduit_index_t element_bytes,
                                                                conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_signed_int_ptr(conduit_node *cnode,
                                                     signed int *data,
                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_signed_int_ptr_detailed(conduit_node *cnode,
                                                              signed int *data,
                                                              conduit_index_t num_elements,
                                                              conduit_index_t offset,
                                                              conduit_index_t stride,
                                                              conduit_index_t element_bytes,
                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_signed_long_ptr(conduit_node *cnode,
                                                      signed long *data,
                                                      conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_signed_long_ptr_detailed(conduit_node *cnode,
                                                               signed long *data,
                                                               conduit_index_t num_elements,
                                                               conduit_index_t offset,
                                                               conduit_index_t stride,
                                                               conduit_index_t element_bytes,
                                                               conduit_index_t endianness);


    //-------------------------------------------------------------------------
    // set cstyle unsigned integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_char_ptr(conduit_node *cnode,
                                                        unsigned char *data,
                                                        conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_char_ptr_detailed(conduit_node *cnode,
                                                                 unsigned char *data,
                                                                 conduit_index_t num_elements,
                                                                 conduit_index_t offset,
                                                                 conduit_index_t stride,
                                                                 conduit_index_t element_bytes,
                                                                 conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_short_ptr(conduit_node *cnode,
                                                         unsigned short *data,
                                                         conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_short_ptr_detailed(conduit_node *cnode,
                                                                  unsigned short *data,
                                                                  conduit_index_t num_elements,
                                                                  conduit_index_t offset,
                                                                  conduit_index_t stride,
                                                                  conduit_index_t element_bytes,
                                                                  conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_int_ptr(conduit_node *cnode,
                                                       unsigned int *data,
                                                       conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_int_ptr_detailed(conduit_node *cnode,
                                                                unsigned int *data,
                                                                conduit_index_t num_elements,
                                                                conduit_index_t offset,
                                                                conduit_index_t stride,
                                                                conduit_index_t element_bytes,
                                                                conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_long_ptr(conduit_node *cnode,
                                                        unsigned long *data,
                                                        conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_unsigned_long_ptr_detailed(conduit_node *cnode,
                                                                 unsigned long *data,
                                                                 conduit_index_t num_elements,
                                                                 conduit_index_t offset,
                                                                 conduit_index_t stride,
                                                                 conduit_index_t element_bytes,
                                                                 conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set cstyle floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float_ptr(conduit_node *cnode,
                                                float *data,
                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_float_ptr_detailed(conduit_node *cnode,
                                                         float *data,
                                                         conduit_index_t num_elements,
                                                         conduit_index_t offset,
                                                         conduit_index_t stride,
                                                         conduit_index_t element_bytes,
                                                         conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_double_ptr(conduit_node *cnode,
                                                 double *data,
                                                 conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_double_ptr_detailed(conduit_node *cnode,
                                                          double *data,
                                                          conduit_index_t num_elements,
                                                          conduit_index_t offset,
                                                          conduit_index_t stride,
                                                          conduit_index_t element_bytes,
                                                          conduit_index_t endianness);

//-----------------------------------------------------------------------------
// -- set_path for scalar cstyle types ---
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // set_path cstyle native integer scalar types
    //-------------------------------------------------------------------------

    CONDUIT_API void conduit_node_set_path_char(conduit_node *cnode,
                                                const char *path,
                                                char value);

    CONDUIT_API void conduit_node_set_path_short(conduit_node *cnode,
                                                 const char *path,
                                                 short value);

    CONDUIT_API void conduit_node_set_path_int(conduit_node *cnode,
                                               const char *path,
                                               int value);

    CONDUIT_API void conduit_node_set_path_long(conduit_node *cnode,
                                                const char *path,
                                                long value);

    //-------------------------------------------------------------------------
    // set_path cstyle signed integer scalar types
    //-------------------------------------------------------------------------

    CONDUIT_API void conduit_node_set_path_signed_char(conduit_node *cnode,
                                                       const char *path,
                                                       signed char value);

    CONDUIT_API void conduit_node_set_path_signed_short(conduit_node *cnode,
                                                        const char *path,
                                                        signed short value);

    CONDUIT_API void conduit_node_set_path_signed_int(conduit_node *cnode,
                                                      const char *path,
                                                      signed int value);

    CONDUIT_API void conduit_node_set_path_signed_long(conduit_node *cnode,
                                                       const char *path,
                                                       signed long value);
    //-------------------------------------------------------------------------
    // set_path cstyle unsigned integer scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_char(conduit_node *cnode,
                                                        const char *path,
                                                        unsigned char value);

    CONDUIT_API void conduit_node_set_path_unsigned_short(conduit_node *cnode,
                                                          const char *path,
                                                          unsigned short value);

    CONDUIT_API void conduit_node_set_path_unsigned_int(conduit_node *cnode,
                                                        const char *path,
                                                        unsigned int value);

    CONDUIT_API void conduit_node_set_path_unsigned_long(conduit_node *cnode,
                                                         const char *path,
                                                         unsigned long value);

    //-------------------------------------------------------------------------
    // set_path cstyle floating point scalar types
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float(conduit_node *cnode,
                                                const char *path,
                                                float value);

    CONDUIT_API void conduit_node_set_path_double(conduit_node *cnode,
                                                  const char *path,
                                                  double value);

//-----------------------------------------------------------------------------
// -- set_path via cstyle pointers for (scalar and array types) --
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // set_path cstyle native pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_char_ptr(conduit_node *cnode,
                                                    const char *path,
                                                    char *data,
                                                    conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_char_ptr_detailed(conduit_node *cnode,
                                                             const char *path,
                                                             char *data,
                                                             conduit_index_t num_elements,
                                                             conduit_index_t offset,
                                                             conduit_index_t stride,
                                                             conduit_index_t element_bytes,
                                                             conduit_index_t endianness);


    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_short_ptr(conduit_node *cnode,
                                                     const char *path,
                                                     short *data,
                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_short_ptr_detailed(conduit_node *cnode,
                                                              const char *path,
                                                              short *data,
                                                              conduit_index_t num_elements,
                                                              conduit_index_t offset,
                                                              conduit_index_t stride,
                                                              conduit_index_t element_bytes,
                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int_ptr(conduit_node *cnode,
                                                   const char *path,
                                                   int *data,
                                                   conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_int_ptr_detailed(conduit_node *cnode,
                                                            const char *path,
                                                            int *data,
                                                            conduit_index_t num_elements,
                                                            conduit_index_t offset,
                                                            conduit_index_t stride,
                                                            conduit_index_t element_bytes,
                                                            conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_long_ptr(conduit_node *cnode,
                                                    const char *path,
                                                    long *data,
                                                    conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_long_ptr_detailed(conduit_node *cnode,
                                                             const char *path,
                                                             long *data,
                                                             conduit_index_t num_elements,
                                                             conduit_index_t offset,
                                                             conduit_index_t stride,
                                                             conduit_index_t element_bytes,
                                                             conduit_index_t endianness);


    //-------------------------------------------------------------------------
    // set_path cstyle signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_char_ptr(conduit_node *cnode,
                                                           const char *path,
                                                           signed char *data,
                                                           conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_char_ptr_detailed(conduit_node *cnode,
                                                                    const char *path,
                                                                    signed char *data,
                                                                    conduit_index_t num_elements,
                                                                    conduit_index_t offset,
                                                                    conduit_index_t stride,
                                                                    conduit_index_t element_bytes,
                                                                    conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_short_ptr(conduit_node *cnode,
                                                            const char *path,
                                                            signed short *data,
                                                            conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_short_ptr_detailed(conduit_node *cnode,
                                                                    const char *path,
                                                                    signed short *data,
                                                                    conduit_index_t num_elements,
                                                                    conduit_index_t offset,
                                                                    conduit_index_t stride,
                                                                    conduit_index_t element_bytes,
                                                                    conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_int_ptr(conduit_node *cnode,
                                                          const char *path,
                                                          signed int *data,
                                                          conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_int_ptr_detailed(conduit_node *cnode,
                                                                   const char *path,
                                                                   signed int *data,
                                                                   conduit_index_t num_elements,
                                                                   conduit_index_t offset,
                                                                   conduit_index_t stride,
                                                                   conduit_index_t element_bytes,
                                                                   conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_long_ptr(conduit_node *cnode,
                                                           const char *path,
                                                           signed long *data,
                                                           conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_signed_long_ptr_detailed(conduit_node *cnode,
                                                                    const char *path,
                                                                    signed long *data,
                                                                    conduit_index_t num_elements,
                                                                    conduit_index_t offset,
                                                                    conduit_index_t stride,
                                                                    conduit_index_t element_bytes,
                                                                    conduit_index_t endianness);


    //-------------------------------------------------------------------------
    // set_path cstyle unsigned integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_char_ptr(conduit_node *cnode,
                                                             const char *path,
                                                             unsigned char *data,
                                                             conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_char_ptr_detailed(conduit_node *cnode,
                                                                      const char *path,
                                                                      unsigned char *data,
                                                                      conduit_index_t num_elements,
                                                                      conduit_index_t offset,
                                                                      conduit_index_t stride,
                                                                      conduit_index_t element_bytes,
                                                                      conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_short_ptr(conduit_node *cnode,
                                                              const char *path,
                                                              unsigned short *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_short_ptr_detailed(conduit_node *cnode,
                                                                       const char *path,
                                                                       unsigned short *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_int_ptr(conduit_node *cnode,
                                                            const char *path,
                                                            unsigned int *data,
                                                            conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_int_ptr_detailed(conduit_node *cnode,
                                                                     const char *path,
                                                                     unsigned int *data,
                                                                     conduit_index_t num_elements,
                                                                     conduit_index_t offset,
                                                                     conduit_index_t stride,
                                                                     conduit_index_t element_bytes,
                                                                     conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_long_ptr(conduit_node *cnode,
                                                             const char *path,
                                                             unsigned long *data,
                                                             conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_unsigned_long_ptr_detailed(conduit_node *cnode,
                                                                      const char *path,
                                                                      unsigned long *data,
                                                                      conduit_index_t num_elements,
                                                                      conduit_index_t offset,
                                                                      conduit_index_t stride,
                                                                      conduit_index_t element_bytes,
                                                                      conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_path cstyle floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float_ptr(conduit_node *cnode,
                                                     const char *path,
                                                     float *data,
                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_float_ptr_detailed(conduit_node *cnode,
                                                              const char *path,
                                                              float *data,
                                                              conduit_index_t num_elements,
                                                              conduit_index_t offset,
                                                              conduit_index_t stride,
                                                              conduit_index_t element_bytes,
                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_double_ptr(conduit_node *cnode,
                                                      const char *path,
                                                      double *data,
                                                      conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_double_ptr_detailed(conduit_node *cnode,
                                                               const char *path,
                                                               double *data,
                                                               conduit_index_t num_elements,
                                                               conduit_index_t offset,
                                                               conduit_index_t stride,
                                                               conduit_index_t element_bytes,
                                                               conduit_index_t endianness);

//-----------------------------------------------------------------------------
// -- set_external via cstyle pointers for (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set_external cstyle native pointer cases
    //-------------------------------------------------------------------------
    
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_char_ptr(conduit_node *cnode,
                                                        char *data,
                                                        conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_char_ptr_detailed(conduit_node *cnode,
                                                                 char *data,
                                                                 conduit_index_t num_elements,
                                                                 conduit_index_t offset,
                                                                 conduit_index_t stride,
                                                                 conduit_index_t element_bytes,
                                                                 conduit_index_t endianness);
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_short_ptr(conduit_node *cnode,
                                                         short *data,
                                                         conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_short_ptr_detailed(conduit_node *cnode,
                                                                  short *data,
                                                                  conduit_index_t num_elements,
                                                                  conduit_index_t offset,
                                                                  conduit_index_t stride,
                                                                  conduit_index_t element_bytes,
                                                                  conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int_ptr(conduit_node *cnode,
                                                       int *data,
                                                       conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_int_ptr_detailed(conduit_node *cnode,
                                                                int *data,
                                                                conduit_index_t num_elements,
                                                                conduit_index_t offset,
                                                                conduit_index_t stride,
                                                                conduit_index_t element_bytes,
                                                                conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_long_ptr(conduit_node *cnode,
                                                        long *data,
                                                        conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_long_ptr_detailed(conduit_node *cnode,
                                                                 long *data,
                                                                 conduit_index_t num_elements,
                                                                 conduit_index_t offset,
                                                                 conduit_index_t stride,
                                                                 conduit_index_t element_bytes,
                                                                 conduit_index_t endianness);



    //-------------------------------------------------------------------------
    // set_external cstyle signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_char_ptr(conduit_node *cnode,
                                                               signed char *data,
                                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_char_ptr_detailed(conduit_node *cnode,
                                                                        signed char *data,
                                                                        conduit_index_t num_elements,
                                                                        conduit_index_t offset,
                                                                        conduit_index_t stride,
                                                                        conduit_index_t element_bytes,
                                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_short_ptr(conduit_node *cnode,
                                                                signed short *data,
                                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_short_ptr_detailed(conduit_node *cnode,
                                                                         signed short *data,
                                                                         conduit_index_t num_elements,
                                                                         conduit_index_t offset,
                                                                         conduit_index_t stride,
                                                                         conduit_index_t element_bytes,
                                                                         conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_int_ptr(conduit_node *cnode,
                                                              signed int *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_int_ptr_detailed(conduit_node *cnode,
                                                                       signed int *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_long_ptr(conduit_node *cnode,
                                                               signed long *data,
                                                               conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_signed_long_ptr_detailed(conduit_node *cnode,
                                                                        signed long *data,
                                                                        conduit_index_t num_elements,
                                                                        conduit_index_t offset,
                                                                        conduit_index_t stride,
                                                                        conduit_index_t element_bytes,
                                                                        conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_external cstyle unsigned integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_char_ptr(conduit_node *cnode,
                                                                 unsigned char *data,
                                                                 conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_char_ptr_detailed(conduit_node *cnode,
                                                                          unsigned char *data,
                                                                          conduit_index_t num_elements,
                                                                          conduit_index_t offset,
                                                                          conduit_index_t stride,
                                                                          conduit_index_t element_bytes,
                                                                          conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_short_ptr(conduit_node *cnode,
                                                                  unsigned short *data,
                                                                  conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_short_ptr_detailed(conduit_node *cnode,
                                                                           unsigned short *data,
                                                                           conduit_index_t num_elements,
                                                                           conduit_index_t offset,
                                                                           conduit_index_t stride,
                                                                           conduit_index_t element_bytes,
                                                                           conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_int_ptr(conduit_node *cnode,
                                                                unsigned int *data,
                                                                conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_int_ptr_detailed(conduit_node *cnode,
                                                                          unsigned int *data,
                                                                          conduit_index_t num_elements,
                                                                          conduit_index_t offset,
                                                                          conduit_index_t stride,
                                                                          conduit_index_t element_bytes,
                                                                          conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_long_ptr(conduit_node *cnode,
                                                                 unsigned long *data,
                                                                 conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_unsigned_long_ptr_detailed(conduit_node *cnode,
                                                                          unsigned long *data,
                                                                          conduit_index_t num_elements,
                                                                          conduit_index_t offset,
                                                                          conduit_index_t stride,
                                                                          conduit_index_t element_bytes,
                                                                          conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_external cstyle floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_float_ptr(conduit_node *cnode,
                                                         float *data,
                                                         conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_float_ptr_detailed(conduit_node *cnode,
                                                                  float *data,
                                                                  conduit_index_t num_elements,
                                                                  conduit_index_t offset,
                                                                  conduit_index_t stride,
                                                                  conduit_index_t element_bytes,
                                                                  conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_double_ptr(conduit_node *cnode,
                                                          double *data,
                                                          conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_external_double_ptr_detailed(conduit_node *cnode,
                                                                   double *data,
                                                                   conduit_index_t num_elements,
                                                                   conduit_index_t offset,
                                                                   conduit_index_t stride,
                                                                   conduit_index_t element_bytes,
                                                                   conduit_index_t endianness);

//-----------------------------------------------------------------------------
// -- set_path_external via cstyle pointers for (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // set_path_external cstyle char pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_char_ptr(conduit_node *cnode,
                                                             const char *path,
                                                             char *data,
                                                             conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_char_ptr_detailed(conduit_node *cnode,
                                                                      const char *path,
                                                                      char *data,
                                                                      conduit_index_t num_elements,
                                                                      conduit_index_t offset,
                                                                      conduit_index_t stride,
                                                                      conduit_index_t element_bytes,
                                                                      conduit_index_t endianness);
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_short_ptr(conduit_node *cnode,
                                                              const char *path,
                                                              short *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_short_ptr_detailed(conduit_node *cnode,
                                                                       const char *path,
                                                                       short *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int_ptr(conduit_node *cnode,
                                                            const char *path,
                                                            int *data,
                                                            conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_int_ptr_detailed(conduit_node *cnode,
                                                                     const char *path,
                                                                     int *data,
                                                                     conduit_index_t num_elements,
                                                                     conduit_index_t offset,
                                                                     conduit_index_t stride,
                                                                     conduit_index_t element_bytes,
                                                                     conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_long_ptr(conduit_node *cnode,
                                                             const char *path,
                                                             long *data,
                                                             conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_long_ptr_detailed(conduit_node *cnode,
                                                                      const char *path,
                                                                      long *data,
                                                                      conduit_index_t num_elements,
                                                                      conduit_index_t offset,
                                                                      conduit_index_t stride,
                                                                      conduit_index_t element_bytes,
                                                                      conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_path_external cstyle signed integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_char_ptr(conduit_node *cnode,
                                                                    const char *path,
                                                                    signed char *data,
                                                                    conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_char_ptr_detailed(conduit_node *cnode,
                                                                             const char *path,
                                                                             signed char *data,
                                                                             conduit_index_t num_elements,
                                                                             conduit_index_t offset,
                                                                             conduit_index_t stride,
                                                                             conduit_index_t element_bytes,
                                                                             conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_short_ptr(conduit_node *cnode,
                                                                     const char *path,
                                                                     signed short *data,
                                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_short_ptr_detailed(conduit_node *cnode,
                                                                              const char *path,
                                                                              signed short *data,
                                                                              conduit_index_t num_elements,
                                                                              conduit_index_t offset,
                                                                              conduit_index_t stride,
                                                                              conduit_index_t element_bytes,
                                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_int_ptr(conduit_node *cnode,
                                                                   const char *path,
                                                                   signed int *data,
                                                                   conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_int_ptr_detailed(conduit_node *cnode,
                                                                            const char *path,
                                                                            signed int *data,
                                                                            conduit_index_t num_elements,
                                                                            conduit_index_t offset,
                                                                            conduit_index_t stride,
                                                                            conduit_index_t element_bytes,
                                                                            conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_long_ptr(conduit_node *cnode,
                                                                    const char *path,
                                                                    signed long *data,
                                                                    conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_signed_long_ptr_detailed(conduit_node *cnode,
                                                                             const char *path,
                                                                             signed long *data,
                                                                             conduit_index_t num_elements,
                                                                             conduit_index_t offset,
                                                                             conduit_index_t stride,
                                                                             conduit_index_t element_bytes,
                                                                             conduit_index_t endianness);


    //-------------------------------------------------------------------------
    // set_path cstyle unsigned integer pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_char_ptr(conduit_node *cnode,
                                                                      const char *path,
                                                                      unsigned char *data,
                                                                      conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_char_ptr_detailed(conduit_node *cnode,
                                                                               const char *path,
                                                                               unsigned char *data,
                                                                               conduit_index_t num_elements,
                                                                               conduit_index_t offset,
                                                                               conduit_index_t stride,
                                                                               conduit_index_t element_bytes,
                                                                               conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_short_ptr(conduit_node *cnode,
                                                                       const char *path,
                                                                       unsigned short *data,
                                                                       conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_short_ptr_detailed(conduit_node *cnode,
                                                                                const char *path,
                                                                                unsigned short *data,
                                                                                conduit_index_t num_elements,
                                                                                conduit_index_t offset,
                                                                                conduit_index_t stride,
                                                                                conduit_index_t element_bytes,
                                                                                conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_int_ptr(conduit_node *cnode,
                                                                     const char *path,
                                                                     unsigned int *data,
                                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_int_ptr_detailed(conduit_node *cnode,
                                                                              const char *path,
                                                                              unsigned int *data,
                                                                              conduit_index_t num_elements,
                                                                              conduit_index_t offset,
                                                                              conduit_index_t stride,
                                                                              conduit_index_t element_bytes,
                                                                              conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_long_ptr(conduit_node *cnode,
                                                                     const char *path,
                                                                     unsigned long *data,
                                                                     conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_unsigned_long_ptr_detailed(conduit_node *cnode,
                                                                               const char *path,
                                                                               unsigned long *data,
                                                                               conduit_index_t num_elements,
                                                                               conduit_index_t offset,
                                                                               conduit_index_t stride,
                                                                               conduit_index_t element_bytes,
                                                                               conduit_index_t endianness);

    //-------------------------------------------------------------------------
    // set_path cstyle floating point pointer cases
    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_float_ptr(conduit_node *cnode,
                                                              const char *path,
                                                              float *data,
                                                              conduit_index_t num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_float_ptr_detailed(conduit_node *cnode,
                                                                       const char *path,
                                                                       float *data,
                                                                       conduit_index_t num_elements,
                                                                       conduit_index_t offset,
                                                                       conduit_index_t stride,
                                                                       conduit_index_t element_bytes,
                                                                       conduit_index_t endianness);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_double_ptr(conduit_node *cnode,
                                                               const char *path,
                                                               double *data,
                                                               conduit_index_t  num_elements);

    //-------------------------------------------------------------------------
    CONDUIT_API void conduit_node_set_path_external_double_ptr_detailed(conduit_node *cnode,
                                                                        const char *path,
                                                                        double *data,
                                                                        conduit_index_t num_elements,
                                                                        conduit_index_t offset,
                                                                        conduit_index_t stride,
                                                                        conduit_index_t element_bytes,
                                                                        conduit_index_t endianness);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// leaf access
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// leaf value access (generic)
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // direct data access
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // direct data pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API void *conduit_node_data_ptr(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // element pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API void *conduit_node_element_ptr(conduit_node *cnode,
                                               conduit_index_t idx);

    //-------------------------------------------------------------------------
    // fetch_path direct data pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API void *conduit_node_fetch_path_data_ptr(conduit_node *cnode,
                                                       const char* path);

    //-------------------------------------------------------------------------
    // fetch_path element pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API void *conduit_node_fetch_path_element_ptr(conduit_node *cnode,
                                                          const char* path,
                                                          conduit_index_t idx);

    //-------------------------------------------------------------------------
    // string access
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // as string
    //-------------------------------------------------------------------------
    CONDUIT_API char *conduit_node_as_char8_str(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // fetch_path_as string
    //-------------------------------------------------------------------------
    CONDUIT_API char *conduit_node_fetch_path_as_char8_str(conduit_node *cnode,
                                                           const char *path);

//-----------------------------------------------------------------------------
// leaf value access (bitwidth style types)
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // as bitwidth signed integer scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_int8   conduit_node_as_int8(conduit_node *cnode);
    CONDUIT_API conduit_int16  conduit_node_as_int16(conduit_node *cnode);
    CONDUIT_API conduit_int32  conduit_node_as_int32(conduit_node *cnode);
    CONDUIT_API conduit_int64  conduit_node_as_int64(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as bitwidth unsigned integer scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_uint8   conduit_node_as_uint8(conduit_node *cnode);
    CONDUIT_API conduit_uint16  conduit_node_as_uint16(conduit_node *cnode);
    CONDUIT_API conduit_uint32  conduit_node_as_uint32(conduit_node *cnode);
    CONDUIT_API conduit_uint64  conduit_node_as_uint64(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as bitwidth floating point scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_float32  conduit_node_as_float32(conduit_node *cnode);
    CONDUIT_API conduit_float64  conduit_node_as_float64(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as bitwidth signed integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_int8   *conduit_node_as_int8_ptr(conduit_node *cnode);
    CONDUIT_API conduit_int16  *conduit_node_as_int16_ptr(conduit_node *cnode);
    CONDUIT_API conduit_int32  *conduit_node_as_int32_ptr(conduit_node *cnode);
    CONDUIT_API conduit_int64  *conduit_node_as_int64_ptr(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as bitwidth unsigned integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_uint8   *conduit_node_as_uint8_ptr(conduit_node *cnode);
    CONDUIT_API conduit_uint16  *conduit_node_as_uint16_ptr(conduit_node *cnode);
    CONDUIT_API conduit_uint32  *conduit_node_as_uint32_ptr(conduit_node *cnode);
    CONDUIT_API conduit_uint64  *conduit_node_as_uint64_ptr(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as bitwidth floating point pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_float32  *conduit_node_as_float32_ptr(conduit_node *cnode);
    CONDUIT_API conduit_float64  *conduit_node_as_float64_ptr(conduit_node *cnode);


//-----------------------------------------------------------------------------
// leaf value access via path (bitwidth style types)
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // fetch_path_as bitwidth signed integer scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_int8   conduit_node_fetch_path_as_int8(conduit_node *cnode,
                                                               const char *path);
    CONDUIT_API conduit_int16  conduit_node_fetch_path_as_int16(conduit_node *cnode,
                                                                const char *path);
    CONDUIT_API conduit_int32  conduit_node_fetch_path_as_int32(conduit_node *cnode,
                                                                const char *path);
    CONDUIT_API conduit_int64  conduit_node_fetch_path_as_int64(conduit_node *cnode,
                                                                const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as bitwidth unsigned integer scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_uint8   conduit_node_fetch_path_as_uint8(conduit_node *cnode,
                                                                 const char *path);
    CONDUIT_API conduit_uint16  conduit_node_fetch_path_as_uint16(conduit_node *cnode,
                                                                  const char *path);
    CONDUIT_API conduit_uint32  conduit_node_fetch_path_as_uint32(conduit_node *cnode,
                                                                  const char *path);
    CONDUIT_API conduit_uint64  conduit_node_fetch_path_as_uint64(conduit_node *cnode,
                                                                  const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as bitwidth floating point scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_float32  conduit_node_fetch_path_as_float32(conduit_node *cnode,
                                                                    const char *path);
    CONDUIT_API conduit_float64  conduit_node_fetch_path_as_float64(conduit_node *cnode,
                                                                    const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as bitwidth signed integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_int8   *conduit_node_fetch_path_as_int8_ptr(conduit_node *cnode,
                                                                    const char *path);
    CONDUIT_API conduit_int16  *conduit_node_fetch_path_as_int16_ptr(conduit_node *cnode,
                                                                     const char *path);
    CONDUIT_API conduit_int32  *conduit_node_fetch_path_as_int32_ptr(conduit_node *cnode,
                                                                     const char *path);
    CONDUIT_API conduit_int64  *conduit_node_fetch_path_as_int64_ptr(conduit_node *cnode,
                                                                     const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as bitwidth unsigned integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_uint8   *conduit_node_fetch_path_as_uint8_ptr(conduit_node *cnode,
                                                                      const char *path);
    CONDUIT_API conduit_uint16  *conduit_node_fetch_path_as_uint16_ptr(conduit_node *cnode,
                                                                       const char *path);
    CONDUIT_API conduit_uint32  *conduit_node_fetch_path_as_uint32_ptr(conduit_node *cnode,
                                                                       const char *path);
    CONDUIT_API conduit_uint64  *conduit_node_fetch_path_as_uint64_ptr(conduit_node *cnode,
                                                                       const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as bitwidth floating point pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API conduit_float32  *conduit_node_fetch_path_as_float32_ptr(conduit_node *cnode,
                                                                         const char *path);
    CONDUIT_API conduit_float64  *conduit_node_fetch_path_as_float64_ptr(conduit_node *cnode,
                                                                         const char *path);


//-----------------------------------------------------------------------------
// leaf value access (native c style types)
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // as cstyle native scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API char         conduit_node_as_char(conduit_node *cnode);
    CONDUIT_API short        conduit_node_as_short(conduit_node *cnode);
    CONDUIT_API int          conduit_node_as_int(conduit_node *cnode);
    CONDUIT_API long         conduit_node_as_long(conduit_node *cnode);
    
    //-------------------------------------------------------------------------
    // as cstyle signed integer scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API signed char  conduit_node_as_signed_char(conduit_node *cnode);
    CONDUIT_API signed short conduit_node_as_signed_short(conduit_node *cnode);
    CONDUIT_API signed int   conduit_node_as_signed_int(conduit_node *cnode);
    CONDUIT_API signed long  conduit_node_as_signed_long(conduit_node *cnode);
    
    //-------------------------------------------------------------------------
    // as cstyle unsigned integer scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API unsigned char   conduit_node_as_unsigned_char(conduit_node *cnode);
    CONDUIT_API unsigned short  conduit_node_as_unsigned_short(conduit_node *cnode);
    CONDUIT_API unsigned int    conduit_node_as_unsigned_int(conduit_node *cnode);
    CONDUIT_API unsigned long   conduit_node_as_unsigned_long(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as cstyle floating point scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API float  conduit_node_as_float(conduit_node *cnode);
    CONDUIT_API double conduit_node_as_double(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as cstyle native pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API char         *conduit_node_as_char_ptr(conduit_node *cnode);
    CONDUIT_API short        *conduit_node_as_short_ptr(conduit_node *cnode);
    CONDUIT_API int          *conduit_node_as_int_ptr(conduit_node *cnode);
    CONDUIT_API long         *conduit_node_as_long_ptr(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as cstyle signed integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API signed char  *conduit_node_as_signed_char_ptr(conduit_node *cnode);
    CONDUIT_API signed short *conduit_node_as_signed_short_ptr(conduit_node *cnode);
    CONDUIT_API signed int   *conduit_node_as_signed_int_ptr(conduit_node *cnode);
    CONDUIT_API signed long  *conduit_node_as_signed_long_ptr(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as cstyle unsigned integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API unsigned char   *conduit_node_as_unsigned_char_ptr(conduit_node *cnode);
    CONDUIT_API unsigned short  *conduit_node_as_unsigned_short_ptr(conduit_node *cnode);
    CONDUIT_API unsigned int    *conduit_node_as_unsigned_int_ptr(conduit_node *cnode);
    CONDUIT_API unsigned long   *conduit_node_as_unsigned_long_ptr(conduit_node *cnode);

    //-------------------------------------------------------------------------
    // as cstyle floating point pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API float   *conduit_node_as_float_ptr(conduit_node *cnode);
    CONDUIT_API double  *conduit_node_as_double_ptr(conduit_node *cnode);

//-----------------------------------------------------------------------------
// leaf value access via path (native c style types)
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle char scalar access
    //-------------------------------------------------------------------------

    CONDUIT_API char  conduit_node_fetch_path_as_char(conduit_node *cnode,
                                                      const char *path);
    CONDUIT_API short conduit_node_fetch_path_as_short(conduit_node *cnode,
                                                       const char *path);
    CONDUIT_API int   conduit_node_fetch_path_as_int(conduit_node *cnode,
                                                     const char *path);
    CONDUIT_API long  conduit_node_fetch_path_as_long(conduit_node *cnode,
                                                      const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle signed integer scalar access
    //-------------------------------------------------------------------------

    CONDUIT_API signed char conduit_node_fetch_path_as_signed_char(conduit_node *cnode,
                                                                   const char *path);
    CONDUIT_API signed short conduit_node_fetch_path_as_signed_short(conduit_node *cnode,
                                                                     const char *path);
    CONDUIT_API signed int   conduit_node_fetch_path_as_signed_int(conduit_node *cnode,
                                                                   const char *path);
    CONDUIT_API signed long  conduit_node_fetch_path_as_signed_long(conduit_node *cnode,
                                                                    const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle unsigned integer scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API unsigned char   conduit_node_fetch_path_as_unsigned_char(conduit_node *cnode,
                                                                         const char *path);
    CONDUIT_API unsigned short  conduit_node_fetch_path_as_unsigned_short(conduit_node *cnode,
                                                                          const char *path);
    CONDUIT_API unsigned int    conduit_node_fetch_path_as_unsigned_int(conduit_node *cnode,
                                                                        const char *path);
    CONDUIT_API unsigned long   conduit_node_fetch_path_as_unsigned_long(conduit_node *cnode,
                                                                         const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle floating point scalar access
    //-------------------------------------------------------------------------
    CONDUIT_API float  conduit_node_fetch_path_as_float(conduit_node *cnode,
                                                        const char *path);
    CONDUIT_API double conduit_node_fetch_path_as_double(conduit_node *cnode,
                                                         const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle native pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API char   *conduit_node_fetch_path_as_char_ptr(conduit_node *cnode,
                                                            const char *path);
    CONDUIT_API short  *conduit_node_fetch_path_as_short_ptr(conduit_node *cnode,
                                                             const char *path);
    CONDUIT_API int    *conduit_node_fetch_path_as_int_ptr(conduit_node *cnode,
                                                           const char *path);
    CONDUIT_API long   *conduit_node_fetch_path_as_long_ptr(conduit_node *cnode,
                                                            const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle signed integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API signed char  *conduit_node_fetch_path_as_signed_char_ptr(conduit_node *cnode,
                                                                         const char *path);
    CONDUIT_API signed short *conduit_node_fetch_path_as_signed_short_ptr(conduit_node *cnode,
                                                                          const char *path);
    CONDUIT_API signed int   *conduit_node_fetch_path_as_signed_int_ptr(conduit_node *cnode,
                                                                        const char *path);
    CONDUIT_API signed long  *conduit_node_fetch_path_as_signed_long_ptr(conduit_node *cnode,
                                                                         const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle unsigned integer pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API unsigned char   *conduit_node_fetch_path_as_unsigned_char_ptr(conduit_node *cnode,
                                                                              const char *path);
    CONDUIT_API unsigned short  *conduit_node_fetch_path_as_unsigned_short_ptr(conduit_node *cnode,
                                                                               const char *path);
    CONDUIT_API unsigned int    *conduit_node_fetch_path_as_unsigned_int_ptr(conduit_node *cnode,
                                                                             const char *path);
    CONDUIT_API unsigned long   *conduit_node_fetch_path_as_unsigned_long_ptr(conduit_node *cnode,
                                                                              const char *path);

    //-------------------------------------------------------------------------
    // fetch_path_as cstyle floating point pointer access
    //-------------------------------------------------------------------------
    CONDUIT_API float   *conduit_node_fetch_path_as_float_ptr(conduit_node *cnode,
                                                              const char *path);
    CONDUIT_API double  *conduit_node_fetch_path_as_double_ptr(conduit_node *cnode,
                                                              const char *path);

    //-------------------------------------------------------------------------
    // Get the dtype for the node.
    //-------------------------------------------------------------------------
    CONDUIT_API const conduit_datatype *conduit_node_dtype(const conduit_node *cnode);

#ifdef __cplusplus
}
#endif

//-----------------------------------------------------------------------------
// -- end extern C
//-----------------------------------------------------------------------------


#endif
