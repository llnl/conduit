// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.
#ifndef CONDUIT_RELAY_CGNS_API_HPP
#define CONDUIT_RELAY_CGNS_API_HPP

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_cgns_api.hpp
///
//-----------------------------------------------------------------------------


/// NOTE: This file is included from other headers that provide namespaces.
///       Do not directly include this file!

void CONDUIT_RELAY_API cgns_write(const Node &node,
                                  const std::string &path);

#endif // CONDUIT_RELAY_CGNS_API_HPP