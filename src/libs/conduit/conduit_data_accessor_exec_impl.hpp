// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor_exec_impl.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_DATA_ACCESSOR_EXEC_IMPL_HPP
#define CONDUIT_DATA_ACCESSOR_EXEC_IMPL_HPP

#include "conduit_data_accessor.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::detail --
//-----------------------------------------------------------------------------
namespace detail
{

//-----------------------------------------------------------------------------
template <typename T>
void
set_value_forall_device_helper(const DataAccessor<T> &accessor, index_t idx, T value)
{
    // Use execution::forall even for a single element because the destination
    // may be device-backed. Launching through the execution layer guarantees
    // the write executes in the active device space instead of dereferencing a
    // device pointer directly from host code.
    execution::ExecutionPolicy policy = execution::ExecutionPolicy::device();

    switch(accessor.dtype().id())
    {
        case DataType::INT8_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int8*)(accessor.element_ptr(i))) = static_cast<int8>(value);
            });
            break;
        case DataType::INT16_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int16*)(accessor.element_ptr(i))) = static_cast<int16>(value);
            });
            break;
        case DataType::INT32_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int32*)(accessor.element_ptr(i))) = static_cast<int32>(value);
            });
            break;
        case DataType::INT64_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int64*)(accessor.element_ptr(i))) = static_cast<int64>(value);
            });
            break;
        case DataType::UINT8_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint8*)(accessor.element_ptr(i))) = static_cast<uint8>(value);
            });
            break;
        case DataType::UINT16_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint16*)(accessor.element_ptr(i))) = static_cast<uint16>(value);
            });
            break;
        case DataType::UINT32_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint32*)(accessor.element_ptr(i))) = static_cast<uint32>(value);
            });
            break;
        case DataType::UINT64_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint64*)(accessor.element_ptr(i))) = static_cast<uint64>(value);
            });
            break;
        case DataType::FLOAT32_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(float32*)(accessor.element_ptr(i))) = static_cast<float32>(value);
            });
            break;
        case DataType::FLOAT64_ID:
            execution::forall(policy, idx, idx + 1, [=] EXEC_LAMBDA(index_t i)
            {
                (*(float64*)(accessor.element_ptr(i))) = static_cast<float64>(value);
            });
            break;
        default:
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << accessor.dtype().name());
    }

    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
stage_values_forall_helper(execution::ExecutionPolicy policy,
                           T *staged_values,
                           const DataAccessor<U> &source,
                           index_t num_elements)
{
    // The staging pass converts the generic source accessor into a contiguous
    // buffer of the destination accessor's logical type T. The later write
    // pass can then focus only on the runtime destination dtype dispatch.
    execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
    {
        staged_values[i] = static_cast<T>(source[i]);
    });

    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T>
void
set_staged_values_forall_helper(const DataAccessor<T> &accessor,
                                execution::ExecutionPolicy policy,
                                const T *staged_values,
                                index_t num_elements)
{
    // This pass owns the runtime destination dtype switch. By separating it
    // from source conversion, each device kernel stays smaller and compile
    // cost is spread across simpler template instantiations.
    switch(accessor.dtype().id())
    {
        case DataType::INT8_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int8*)(accessor.element_ptr(i))) =
                    static_cast<int8>(staged_values[i]);
            });
            break;
        case DataType::INT16_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int16*)(accessor.element_ptr(i))) =
                    static_cast<int16>(staged_values[i]);
            });
            break;
        case DataType::INT32_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int32*)(accessor.element_ptr(i))) =
                    static_cast<int32>(staged_values[i]);
            });
            break;
        case DataType::INT64_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(int64*)(accessor.element_ptr(i))) =
                    static_cast<int64>(staged_values[i]);
            });
            break;
        case DataType::UINT8_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint8*)(accessor.element_ptr(i))) =
                    static_cast<uint8>(staged_values[i]);
            });
            break;
        case DataType::UINT16_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint16*)(accessor.element_ptr(i))) =
                    static_cast<uint16>(staged_values[i]);
            });
            break;
        case DataType::UINT32_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint32*)(accessor.element_ptr(i))) =
                    static_cast<uint32>(staged_values[i]);
            });
            break;
        case DataType::UINT64_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(uint64*)(accessor.element_ptr(i))) =
                    static_cast<uint64>(staged_values[i]);
            });
            break;
        case DataType::FLOAT32_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(float32*)(accessor.element_ptr(i))) =
                    static_cast<float32>(staged_values[i]);
            });
            break;
        case DataType::FLOAT64_ID:
            execution::forall(policy, 0, num_elements, [=] EXEC_LAMBDA(index_t i)
            {
                (*(float64*)(accessor.element_ptr(i))) =
                    static_cast<float64>(staged_values[i]);
            });
            break;
        default:
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << accessor.dtype().name());
    }

    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::detail --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
