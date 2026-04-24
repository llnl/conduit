// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_uint8.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This HIP TU owns the explicit device-helper instantiations for uint8
// destinations. Splitting the bitwidth destinations into one-type-per-file
// reduces the amount of device code each compile job has to materialize.
//-----------------------------------------------------------------------------

#include "conduit_data_accessor_exec_impl.hpp"

//-----------------------------------------------------------------------------
// Explicit instantiations for conduit::uint8 destinations.
//-----------------------------------------------------------------------------
template void conduit::detail::set_value_forall_helper(const conduit::DataAccessor<conduit::uint8> &accessor, conduit::index_t idx, conduit::uint8 value);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::int8>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::int8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::int16>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::int16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::int32>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::int32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::int64>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::int64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::uint8>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::uint8> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::uint16>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::uint16> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::uint32>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::uint32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::uint64>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::uint64> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::float32>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::float32> &source, conduit::index_t num_elements);
template void conduit::detail::stage_values_forall_helper<conduit::uint8, conduit::float64>(conduit::execution::ExecutionPolicy policy, conduit::uint8 *staged_values, const conduit::DataAccessor<conduit::float64> &source, conduit::index_t num_elements);
template void conduit::detail::set_staged_values_forall_helper(const conduit::DataAccessor<conduit::uint8> &accessor, conduit::execution::ExecutionPolicy policy, const conduit::uint8 *staged_values, conduit::index_t num_elements);
