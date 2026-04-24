// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_alias_long_long.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the native long-long alias instantiations for the staged
// DataAccessor device helpers when long long differs from Conduit's chosen
// bitwidth type mapping.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for native long-long alias destinations.
//-----------------------------------------------------------------------------
#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<signed long long> &accessor, conduit::index_t idx, signed long long value);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::int8>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::int16>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::int32>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::int64>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::uint8>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::uint16>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::uint32>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::uint64>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::float32>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed long long, conduit::float64>(conduit::execution::ExecutionPolicy policy, signed long long *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<signed long long> &accessor, conduit::execution::ExecutionPolicy policy, const signed long long *staged_values, conduit::index_t num_elements);

template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<unsigned long long> &accessor, conduit::index_t idx, unsigned long long value);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::int8>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::int16>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::int32>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::int64>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::uint8>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::uint16>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::uint32>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::uint64>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::float32>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned long long, conduit::float64>(conduit::execution::ExecutionPolicy policy, unsigned long long *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<unsigned long long> &accessor, conduit::execution::ExecutionPolicy policy, const unsigned long long *staged_values, conduit::index_t num_elements);
#endif
