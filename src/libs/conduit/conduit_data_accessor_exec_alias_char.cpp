// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_alias_char.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the native char-family alias instantiations for the staged
// DataAccessor device helpers. Splitting the alias families across several HIP
// files reduces the remaining long compile tail from alias_integral.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for native char-family alias destinations.
//-----------------------------------------------------------------------------
template void conduit::detail::set_value_forall_device_helper(const conduit::DataAccessor<char> &accessor, conduit::index_t idx, char value);
template void conduit::detail::stage_values_forall_helper<char, conduit::int8>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::int16>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::int32>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::int64>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::uint8>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::uint16>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::uint32>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::uint64>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::float32>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<char, conduit::float64>(conduit::execution::ExecutionPolicy policy, char *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<char> &accessor, conduit::execution::ExecutionPolicy policy, const char *staged_values, conduit::index_t num_elements);

#ifndef CONDUIT_USE_CHAR
template void conduit::detail::set_value_forall_device_helper(const conduit::DataAccessor<signed char> &accessor, conduit::index_t idx, signed char value);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::int8>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::int16>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::int32>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::int64>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::uint8>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::uint16>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::uint32>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::uint64>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::float32>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<signed char, conduit::float64>(conduit::execution::ExecutionPolicy policy, signed char *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<signed char> &accessor, conduit::execution::ExecutionPolicy policy, const signed char *staged_values, conduit::index_t num_elements);

template void conduit::detail::set_value_forall_device_helper(const conduit::DataAccessor<unsigned char> &accessor, conduit::index_t idx, unsigned char value);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::int8>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::int16>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::int32>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::int64>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::uint8>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::uint16>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::uint32>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::uint64>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::float32>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<unsigned char, conduit::float64>(conduit::execution::ExecutionPolicy policy, unsigned char *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<unsigned char> &accessor, conduit::execution::ExecutionPolicy policy, const unsigned char *staged_values, conduit::index_t num_elements);
#endif
