// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io.h
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_IO_H
#define CONDUIT_RELAY_IO_H

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.h"
#include "conduit_relay_exports.h"
#include "conduit_relay_config.h"

//-----------------------------------------------------------------------------
// -- begin extern C
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// -- conduit_relay io c interface  --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API conduit_relay_io_about(conduit_node *cnode);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API conduit_relay_io_initialize(void);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API conduit_relay_io_finalize(void);

///
/// ``save`` works like a 'set' to the file.
///

//-----------------------------------------------------------------------------
// conduit_relay_io_save
// For simpler use cases, accepts NULL for protocol and options
void CONDUIT_RELAY_API conduit_relay_io_save(conduit_node *cnode,
                                             const char *path,
                                             const char *protocol,
                                             conduit_node *coptions);

///
/// ``save_merged`` works like an update to the file.
///

//-----------------------------------------------------------------------------
// conduit_relay_io_save_merged 
// For simpler use cases, accepts NULL for protocol and options
void CONDUIT_RELAY_API conduit_relay_io_save_merged(conduit_node *cnode,
                                                    const char *path,
                                                    const char *protocol,
                                                    conduit_node *coptions);
                                                     

//-----------------------------------------------------------------------------
///
/// ``add_step`` adds a new time step of data to the file.
///
// void CONDUIT_RELAY_API conduit_relay_io_add_step(conduit_node *cnode,
//                                                  const char *path);

//-----------------------------------------------------------------------------
// conduit_relay_io_add_step                                        
// For simpler use cases, accepts NULL for protocol and options
void CONDUIT_RELAY_API conduit_relay_io_add_step(conduit_node *cnode,
                                                 const char *path,
                                                 const char *protocol,
                                                 conduit_node *coptions);

///
/// ``load`` works like a 'set', the node is reset and then populated
///

//-----------------------------------------------------------------------------
// conduit_relay_io_load
// For simpler use cases, accepts NULL for protocol and options
void CONDUIT_RELAY_API conduit_relay_io_load(const char *path,
                                             const char *protocol,
                                             conduit_node *coptions,
                                             conduit_node *cnode);

//-----------------------------------------------------------------------------
// conduit_relay_io_load_step_and_domain
// For simpler use cases, accepts NULL for protocol and options
void CONDUIT_RELAY_API conduit_relay_io_load_step_and_domain(const char *path,
                                                             const char *protocol,
                                                             int step,
                                                             int domain,
                                                             conduit_node *coptions,
                                                             conduit_node *cnode);

///
/// ``query_number_of_steps`` returns the number of time steps.
///
//-----------------------------------------------------------------------------
int CONDUIT_RELAY_API conduit_relay_io_query_number_of_steps(const char *path);

///
/// ``query_number_of_domains`` returns the number of domains.
///
//-----------------------------------------------------------------------------
int CONDUIT_RELAY_API conduit_relay_io_query_number_of_domains(
                                                            const char *path);

#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
// -- end extern C
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// -- end header guard ifdef
//-----------------------------------------------------------------------------
#endif
