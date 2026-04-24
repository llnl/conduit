// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_alias_integral.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the remaining native integral alias instantiations for the
// staged DataAccessor device helpers. These aliases are only needed when the
// platform-native type is distinct from Conduit's bitwidth types, so they stay
// separate from the core bitwidth files.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for native integral alias destinations.
//-----------------------------------------------------------------------------
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<char> &accessor, conduit::index_t idx, char value);
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
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<signed char> &accessor, conduit::index_t idx, signed char value);
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

template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<unsigned char> &accessor, conduit::index_t idx, unsigned char value);
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
