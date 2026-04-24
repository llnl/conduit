// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_alias_long.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the native long-family alias instantiations for the staged
// DataAccessor device helpers. Long is one of the heavier alias families on
// this platform, so giving it its own TU helps shorten the compile tail.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for native long-family alias destinations.
//-----------------------------------------------------------------------------
#ifndef CONDUIT_USE_LONG
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<signed long> &accessor, conduit::index_t idx, signed long value);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::int8>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::int16>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::int32>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::int64>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::uint8>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::uint16>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::uint32>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::uint64>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::float32>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long, conduit::float64>(conduit::execution::ExecutionPolicy policy, signed long *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<signed long> &accessor, conduit::execution::ExecutionPolicy policy, const signed long *staged_values, conduit::index_t num_elements);

template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<unsigned long> &accessor, conduit::index_t idx, unsigned long value);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::int8>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::int16>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::int32>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::int64>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::uint8>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::uint16>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::uint32>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::uint64>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::float32>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long, conduit::float64>(conduit::execution::ExecutionPolicy policy, unsigned long *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<unsigned long> &accessor, conduit::execution::ExecutionPolicy policy, const unsigned long *staged_values, conduit::index_t num_elements);
#endif
