// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_adios2.cpp
///
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
#include "conduit_relay_mpi_io_adios2.hpp"

// Define argument macros that add a communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG0(ARG) ARG
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG) , ARG

#else
#include "conduit_relay_io_adios2.hpp"

// Define an argument macro that does not add the communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG0(ARG)
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG)

// for non-mpi adios2 we need to define _NOMPI
#define _NOMPI

#endif

//-----------------------------------------------------------------------------
// standard lib includes
//-----------------------------------------------------------------------------

#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include <adios2.h>

//-----------------------------------------------------------------------------
// -- conduit includes --
//-----------------------------------------------------------------------------
#include "conduit_error.hpp"
#include "conduit_utils.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit {

//-----------------------------------------------------------------------------
// -- begin conduit::relay --
//-----------------------------------------------------------------------------
namespace relay {

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
//-----------------------------------------------------------------------------
namespace mpi {
#endif

//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::io --
//-----------------------------------------------------------------------------
namespace io {

//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::io::internals --
//-----------------------------------------------------------------------------
namespace adios2_internals {

bool is_integer(const std::string &s, int &ivalue) {
  return sscanf(s.c_str(), "%d", &ivalue) == 1;
}

//-----------------------------------------------------------------------------
bool is_positive_integer(const std::string &s, int &ivalue) {
  bool ret = is_integer(s, ivalue);
  return ret && ivalue >= 0;
}

//-----------------------------------------------------------------------------
// This is the same as ADIOS's splitpath
// NOTE: Move to conduit::utils?
void splitpath(const std::string &path, std::string &filename, int &time_step,
               int &domain, std::vector<std::string> &subpaths,
               bool prefer_time = false) {
  std::vector<std::string> tok;
  conduit::utils::split_string(path, ':', tok);

  if (tok.empty())
    filename = path; // Would have had to be an empty string.
  else if (tok.size() == 1)
    filename = path;
  else if (tok.size() == 2) {
    filename = tok[0];
    int ivalue = 0;
    if (is_integer(tok[1], ivalue)) {
      if (prefer_time) {
        // filename:timestep
        time_step = ivalue;
      } else {
        // filename:domain
        domain = ivalue;
      }
    } else {
      // filename:subpaths
      subpaths.push_back(tok[1]);
    }
  } else if (tok.size() >= 3) {
    filename = tok[0];
    int ivalue1 = 0, ivalue2 = 0;
    bool arg1 = is_integer(tok[1], ivalue1);
    bool arg2 = is_positive_integer(tok[2], ivalue2);
    if (arg1 && arg2) {
      // filename:timestep:domain
      time_step = ivalue1;
      domain = ivalue2;
    } else if (arg1 && !arg2) {
      // filename:domain:subpaths
      domain = ivalue1;
      subpaths.push_back(tok[2]);
    } else if (!arg1 && arg2) {
      // filename:<non-int>:int
      // Assume these are just numeric subpaths for now.
      // We could test for tok[1] == "current" to denote time...
      subpaths.push_back(tok[1]);
      subpaths.push_back(tok[2]);
    } else // !arg1 && !arg2
    {
      // filename:subpath:subpath
      subpaths.push_back(tok[1]);
      subpaths.push_back(tok[2]);
    }

    // Save the remaining tokens as subpaths
    for (size_t i = 3; i < tok.size(); ++i)
      subpaths.push_back(tok[i]);
  }
}

//-----------------------------------------------------------------------------
static std::unique_ptr<adios2::ADIOS> adios;

static void initialize(CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)) {
  if (adios)
    return;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
  adios = std::make_unique<adios2::ADIOS>(comm);
#else
  adios = std::make_unique<adios2::ADIOS>();
#endif
}

static void finalize(CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)) {
  if (!adios)
    return;
  adios.reset();
}

//-----------------------------------------------------------------------------
struct Options {
  Options() = default; // for now

  Options(const Options &) = delete;
  Options(Options &&) = delete;
  Options &operator=(const Options &) = delete;
  Options &operator=(Options &&) = delete;

  void set(const Node &opts) {}
  void get(Node &opts) const {}
};

// Default I/O settings
static std::unique_ptr<Options> the_options;

// @brief Access the ADIOS2 options, creating them first if needed.
static Options *options() {
  if (!the_options) {
    the_options = std::make_unique<Options>();
  }
  return the_options.get();
}

} // namespace adios2_internals
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io::adios2_internals --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void adios2_set_options(
    const Node &opts CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  adios2_internals::initialize(CONDUIT_RELAY_COMMUNICATOR_ARG0(comm));
  adios2_internals::options()->set(opts);
}

//-----------------------------------------------------------------------------
void adios2_options(Node &opts CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  adios2_internals::initialize(CONDUIT_RELAY_COMMUNICATOR_ARG0(comm));
  adios2_internals::options()->get(opts);
}

//-----------------------------------------------------------------------------
void adios2_initialize_library(CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)) {
  adios2_internals::initialize(CONDUIT_RELAY_COMMUNICATOR_ARG0(comm));
}

//-----------------------------------------------------------------------------
void adios2_finalize_library(CONDUIT_RELAY_COMMUNICATOR_ARG0(MPI_Comm comm)) {
  adios2_internals::finalize(CONDUIT_RELAY_COMMUNICATOR_ARG0(comm));
}

//-----------------------------------------------------------------------------
void adios2_save(const Node &node,
                 const std::string &path
                     CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  unsigned int nodehash = 0;
  int nodehash_rank = 0, nodehash_size = 1;
  assert(false);
  // TODO adios2_internals::compute_nodehash(
  // TODO     node, nodehash, nodehash_rank,
  // TODO     nodehash_size CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
  // TODO adios2_internals::save(node, path, "w", nodehash, nodehash_rank,
  // TODO                        nodehash_size
  // CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
}

//-----------------------------------------------------------------------------
void adios2_save_merged(const Node &node,
                        const std::string &path
                            CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  assert(false);
  // TODO // save_merged() is not allowed for streaming.
  // TODO if
  // (!adios2_internals::streamIsFileBased(adios2_internals::options()->read_method))
  // {
  // TODO   CONDUIT_ERROR("save_merged() is not allowed for streaming.");
  // TODO   return;
  // TODO }

  unsigned int nodehash = 0;
  int nodehash_rank = 0, nodehash_size = 1;

  assert(false);
  // TODO // NOTE: we use "u" to update the file so the time step is not
  // incremented.
  // TODO adios2_internals::compute_nodehash(node, nodehash, nodehash_rank,
  // TODO                                    nodehash_size,
  // TODO CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
  // TODO // TODO: read the number of domains in the file for this node hash (if
  // hashing
  // TODO //       is present in the file) and adjust the nodehash_rank by that
  // number.
  // TODO adios2_internals::save(node, path, "u", nodehash, nodehash_rank,
  // TODO                        nodehash_size,
  // CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
}

void adios2_add_step(const Node &node,
                     const std::string &path
                         CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  // check for ":" split
  std::string file_path, adios2_path;
  conduit::utils::split_file_path(path, std::string(":"), file_path,
                                  adios2_path);

  unsigned int nodehash = 0;
  int nodehash_rank = 0, nodehash_size = 1;

  assert(false);
  // TODO // NOTE: we use "a" to update the file to the next time step.
  // TODO adios2_internals::compute_nodehash(node, nodehash, nodehash_rank,
  // TODO                                    nodehash_size,
  // TODO CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
  // TODO adios2_internals::save(node, path, "a", nodehash, nodehash_rank,
  // TODO                        nodehash_size,
  // CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
}

//-----------------------------------------------------------------------------
void adios2_load(const std::string &path, int time_step, int domain,
                 Node &node CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  assert(false);
  // TODO adios2_internals::adios2_load_state state;
  // TODO state.time_step = time_step; // Force specific timestep/domain.
  // TODO state.domain = domain;
  // TODO
  // TODO // Split the incoming path in case it includes other information.
  // TODO // This may override the timestep and domain.
  // TODO adios2_internals::splitpath(path, state.filename, state.time_step,
  // TODO                             state.domain, state.subpaths);
  // TODO
  // TODO adios2_internals::load(&state, &node,
  // CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
}

//-----------------------------------------------------------------------------
void adios2_load(const std::string &path,
                 Node &node CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  assert(false);
  // TODO   adios2_internals::adios2_load_state state;
  // TODO #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
  // TODO   // Read the rank'th domain if there is one.
  // TODO   MPI_Comm_rank(comm, &state.domain);
  // TODO #endif
  // TODO
  // TODO   // Split the incoming path in case it includes other information.
  // TODO   // This may override the timestep and domain.
  // TODO   adios2_internals::splitpath(path, state.filename, state.time_step,
  // TODO                               state.domain, state.subpaths);
  // TODO
  // TODO   adios2_internals::load(&state, &node,
  // CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
}

//-----------------------------------------------------------------------------
int adios2_query_number_of_steps(
    const std::string &path CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  assert(false);
  // TODO adios2_internals::adios2_load_state state;
  // TODO
  // TODO // check for ":" split
  // TODO std::string tmp;
  // TODO conduit::utils::split_file_path(path, std::string(":"),
  // state.filename, tmp);
  // TODO
  // TODO return adios2_internals::query_number_of_steps(
  // TODO     &state, CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
}

//-----------------------------------------------------------------------------
int adios2_query_number_of_domains(
    const std::string &path CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm comm)) {
  assert(false);
  // TODO adios2_internals::adios2_load_state state;
  // TODO
  // TODO // Split the incoming path in case it includes other information.
  // TODO // This may override the timestep and domain.
  // TODO adios2_internals::splitpath(path, state.filename, state.time_step,
  // TODO                             state.domain, state.subpaths);
  // TODO
  // TODO return adios2_internals::query_number_of_domains(
  // TODO     &state, CONDUIT_RELAY_COMMUNICATOR_ARG(comm));
}

} // namespace io
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io --
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi --
//-----------------------------------------------------------------------------
#endif

} // namespace relay
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------

} // namespace conduit
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
