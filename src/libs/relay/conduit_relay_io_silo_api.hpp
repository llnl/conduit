// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.
#ifndef CONDUIT_RELAY_SILO_API_HPP
#define CONDUIT_RELAY_SILO_API_HPP

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_silo_api.hpp
///
//-----------------------------------------------------------------------------

/// NOTE: This file is included from other headers that provide namespaces.
///       Do not directly include this file!

//-----------------------------------------------------------------------------
/// Checks if the given path is a silo file.
//-----------------------------------------------------------------------------
bool CONDUIT_RELAY_API is_silo_file(const std::string &path);
//-----------------------------------------------------------------------------
/// Checks if the given path is a silo file using specific driver
/// silo_driver options: {"hdf5"|"pdb"|"unknown"}
//-----------------------------------------------------------------------------
bool CONDUIT_RELAY_API is_silo_file(const std::string &path,
                                    const std::string &silo_driver);

//-----------------------------------------------------------------------------
/// Creates a silo file and returns a silo file handle
//-----------------------------------------------------------------------------
CONDUIT_RELAY_API DBfile* silo_create_file(const std::string &path,
                                           int silo_type);

//-----------------------------------------------------------------------------
/// Opens a silo file and returns a silo file handle (file type specified)
//-----------------------------------------------------------------------------
CONDUIT_RELAY_API DBfile* silo_open_file_for_read(const std::string &path,
                                                  int silo_type);

//-----------------------------------------------------------------------------
/// Opens a silo file and returns a silo file handle (detect file type)
//-----------------------------------------------------------------------------
CONDUIT_RELAY_API DBfile* silo_open_file_for_read(const std::string &path);

//-----------------------------------------------------------------------------
/// Opens an existing a silo file for write and returns a silo file handle
//-----------------------------------------------------------------------------
CONDUIT_RELAY_API DBfile* hdf5_open_file_for_read_write(const std::string &path,
                                                        int silo_type);

//-----------------------------------------------------------------------------
/// Close silo file handle
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API silo_close_file(DBfile *silo_handle);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API silo_write(const Node &node,
                                  const std::string &path);

void CONDUIT_RELAY_API silo_read(const std::string &path,
                                 Node &node);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API silo_write(const  Node &node,
                                  const std::string &file_path,
                                  const std::string &silo_obj_path);

void CONDUIT_RELAY_API silo_read(const std::string &file_path,
                                 const std::string &silo_obj_path,
                                 Node &node);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API silo_write(const  Node &node,
                                  DBfile *dbfile,
                                  const std::string &silo_obj_path);

void CONDUIT_RELAY_API silo_read(DBfile *dbfile,
                                 const std::string &silo_obj_path,
                                 Node &node);


#endif
