// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_alias_floating.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the native floating-point alias instantiations for the
// staged DataAccessor device helpers. These aliases are only emitted when the
// platform-native floating types differ from the bitwidth typedefs.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for native floating alias destinations.
//-----------------------------------------------------------------------------
#ifndef CONDUIT_USE_FLOAT
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<float> &accessor, conduit::index_t idx, float value);
template void conduit::detail::stage_values_forall_helper<float, conduit::int8>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::int16>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::int32>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::int64>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::uint8>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::uint16>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::uint32>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::uint64>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::float32>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<float, conduit::float64>(conduit::execution::ExecutionPolicy policy, float *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<float> &accessor, conduit::execution::ExecutionPolicy policy, const float *staged_values, conduit::index_t num_elements);
#endif

#ifndef CONDUIT_USE_DOUBLE
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<double> &accessor, conduit::index_t idx, double value);
template void conduit::detail::stage_values_forall_helper<double, conduit::int8>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::int16>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::int32>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::int64>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::uint8>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::uint16>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::uint32>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::uint64>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::float32>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<double, conduit::float64>(conduit::execution::ExecutionPolicy policy, double *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<double> &accessor, conduit::execution::ExecutionPolicy policy, const double *staged_values, conduit::index_t num_elements);
#endif

#ifdef CONDUIT_USE_LONG_DOUBLE
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<long double> &accessor, conduit::index_t idx, long double value);
template void conduit::detail::stage_values_forall_helper<long double, conduit::int8>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::int16>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::int32>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::int64>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::uint8>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::uint16>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::uint32>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::uint64>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::float32>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<long double, conduit::float64>(conduit::execution::ExecutionPolicy policy, long double *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<long double> &accessor, conduit::execution::ExecutionPolicy policy, const long double *staged_values, conduit::index_t num_elements);
#endif
