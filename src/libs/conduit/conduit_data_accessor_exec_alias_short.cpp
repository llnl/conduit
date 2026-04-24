// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_alias_short.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the native short-family alias instantiations for the staged
// DataAccessor device helpers. Keeping short separate reduces the remaining
// alias compile load and lets the build distribute these templates better.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for native short-family alias destinations.
//-----------------------------------------------------------------------------
#ifndef CONDUIT_USE_SHORT
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<signed short> &accessor, conduit::index_t idx, signed short value);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::int8>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::int16>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::int32>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::int64>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::uint8>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::uint16>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::uint32>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::uint64>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::float32>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed short, conduit::float64>(conduit::execution::ExecutionPolicy policy, signed short *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<signed short> &accessor, conduit::execution::ExecutionPolicy policy, const signed short *staged_values, conduit::index_t num_elements);

template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<unsigned short> &accessor, conduit::index_t idx, unsigned short value);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::int8>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::int16>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::int32>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::int64>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::uint8>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::uint16>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::uint32>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::uint64>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::float32>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned short, conduit::float64>(conduit::execution::ExecutionPolicy policy, unsigned short *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<unsigned short> &accessor, conduit::execution::ExecutionPolicy policy, const unsigned short *staged_values, conduit::index_t num_elements);
#endif
