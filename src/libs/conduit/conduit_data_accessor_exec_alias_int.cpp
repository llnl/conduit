// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_alias_int.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the native int-family alias instantiations for the staged
// DataAccessor device helpers. Separating these aliases keeps the heaviest
// native integral families from compiling in one monolithic HIP file.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for native int-family alias destinations.
//-----------------------------------------------------------------------------
#ifndef CONDUIT_USE_INT
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<signed int> &accessor, conduit::index_t idx, signed int value);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::int8>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::int16>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::int32>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::int64>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::uint8>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::uint16>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::uint32>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::uint64>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::float32>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed int, conduit::float64>(conduit::execution::ExecutionPolicy policy, signed int *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<signed int> &accessor, conduit::execution::ExecutionPolicy policy, const signed int *staged_values, conduit::index_t num_elements);

template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<unsigned int> &accessor, conduit::index_t idx, unsigned int value);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::int8>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::int16>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::int32>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::int64>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::uint8>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::uint16>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::uint32>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::uint64>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::float32>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned int, conduit::float64>(conduit::execution::ExecutionPolicy policy, unsigned int *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<unsigned int> &accessor, conduit::execution::ExecutionPolicy policy, const unsigned int *staged_values, conduit::index_t num_elements);
#endif
