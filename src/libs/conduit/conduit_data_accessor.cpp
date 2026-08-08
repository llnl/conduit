// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_data_accessor.hpp"

//-----------------------------------------------------------------------------
// -- standard includes -- 
//-----------------------------------------------------------------------------
#include <algorithm>
#include <limits>
#include <type_traits>

//-----------------------------------------------------------------------------
// -- conduit  includes -- 
//-----------------------------------------------------------------------------
#include "conduit_memory_manager.hpp"
#include "conduit_node.hpp"
#include "conduit_data_array.hpp"
#include "conduit_execution.hpp"
#include "conduit_annotations.hpp"

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
template <typename DestType>
void
fill_typed(void *data,
           index_t stride,
           index_t num_elements,
           DestType value)
{
    if (stride == static_cast<index_t>(sizeof(DestType)))
    {
        // The stride matches the size of the destination type, so we can treat
        // the data as a contiguous array of DestType and assign the fill value
        // directly.
        DestType *ptr = static_cast<DestType*>(data);
        for (index_t i = 0; i < num_elements; i++)
        {
            ptr[i] = value;
        }
    }
    else // stride does not match sizeof(DestType)
    {
        // The stride does not match the size of the destination type, so we need
        // to manually iterate over the data considering the given stride and
        // assign the fill value for each element.
        char *ptr = static_cast<char*>(data);
        for (index_t i = 0; i < num_elements; i++)
        {
            (*(DestType*)(ptr)) = value;
            ptr += stride;
        }
    }
}

//-----------------------------------------------------------------------------
template <typename DestType, typename T, typename U>
void
set_values_typed(void *data,
                 index_t stride,
                 const U &values,
                 index_t num_elements)
{
    if (stride == static_cast<index_t>(sizeof(DestType)))
    {
        // The stride matches the size of the destination type, so we can treat
        // the data as a contiguous array of DestType and perform a simple copy
        // with type conversion.
        DestType *ptr = static_cast<DestType*>(data);
        for (index_t i = 0; i < num_elements; i++)
        {
            ptr[i] = static_cast<DestType>(static_cast<T>(values[i]));
        }
    }
    else // stride does not match sizeof(DestType)
    {
        // The stride does not match the size of the destination type, so we need
        // to manually iterate over the data considering the given stride and perform
        // the type conversion for each element.
        char *ptr = static_cast<char*>(data);
        for (index_t i = 0; i < num_elements; i++)
        {
            (*(DestType*)(ptr)) = static_cast<DestType>(static_cast<T>(values[i]));
            ptr += stride;
        }
    }
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const U &values,
                  index_t num_elements)
{
    // Preserve DataAccessor semantics by converting source values through the
    // accessor's logical type T before converting to the destination dtype.

    // element_ptr(0) points at the first element with the dtype's byte offset
    // already applied (base + offset + stride * 0). element_ptr() const-qualifies
    // its return value even though the underlying buffer is not const. const_cast
    // strips the constness away. We will later cast the void* to the appropriate
    // destination dtype.
    void *data = const_cast<void*>(accessor.element_ptr(0));
    const DataType &dt = accessor.dtype();
    const index_t stride = dt.stride();

    switch(dt.id())
    {
        // ints
        case DataType::INT8_ID:
            set_values_typed<int8, T>(data, stride, values, num_elements);
            break;
        case DataType::INT16_ID:
            set_values_typed<int16, T>(data, stride, values, num_elements);
            break;
        case DataType::INT32_ID:
            set_values_typed<int32, T>(data, stride, values, num_elements);
            break;
        case DataType::INT64_ID:
            set_values_typed<int64, T>(data, stride, values, num_elements);
            break;
        // uints
        case DataType::UINT8_ID:
            set_values_typed<uint8, T>(data, stride, values, num_elements);
            break;
        case DataType::UINT16_ID:
            set_values_typed<uint16, T>(data, stride, values, num_elements);
            break;
        case DataType::UINT32_ID:
            set_values_typed<uint32, T>(data, stride, values, num_elements);
            break;
        case DataType::UINT64_ID:
            set_values_typed<uint64, T>(data, stride, values, num_elements);
            break;
        // floats
        case DataType::FLOAT32_ID:
            set_values_typed<float32, T>(data, stride, values, num_elements);
            break;
        case DataType::FLOAT64_ID:
            set_values_typed<float64, T>(data, stride, values, num_elements);
            break;
        // error
        default:
            CONDUIT_ERROR("DataAccessor does not support dtype: " << dt.name());
    }
}
}
//-----------------------------------------------------------------------------
// -- end conduit::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- conduit::DataAccessor public methods --
//
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor()
: m_data(nullptr),
  m_orig_data_ptr(nullptr),
  m_dtype(DataType::empty()),
  m_node_ptr(nullptr),
  m_other_ptr(nullptr),
  m_other_dtype(DataType::empty()),
  m_do_i_own_it(false),
  m_offset(0),
  m_stride(0)
{}

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(void *data, const DataType &dtype)
: m_data(data),
  m_orig_data_ptr(data),
  m_dtype(dtype),
  m_node_ptr(nullptr),
  m_other_ptr(nullptr),
  m_other_dtype(DataType::empty()),
  m_do_i_own_it(false),
  m_offset(0),
  m_stride(0)
{}


//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(const void *data, const DataType &dtype)
: m_data(const_cast<void*>(data)),
  m_orig_data_ptr(const_cast<void*>(data)),
  m_dtype(dtype),
  m_node_ptr(nullptr),
  m_other_ptr(nullptr),
  m_other_dtype(DataType::empty()),
  m_do_i_own_it(false),
  m_offset(0),
  m_stride(0)
{}

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(Node &node)
: m_data(node.data_ptr()),
  m_orig_data_ptr(node.data_ptr()),
  m_dtype(node.dtype()),
  m_node_ptr(&node),
  m_other_ptr(nullptr),
  m_other_dtype(DataType::empty()),
  m_do_i_own_it(false),
  m_offset(node.dtype().offset()),
  m_stride(node.dtype().stride())
{}

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(const Node &node)
: m_data(const_cast<void*>(node.data_ptr())),
  m_orig_data_ptr(const_cast<void*>(node.data_ptr())),
  m_dtype(node.dtype()),
  m_node_ptr(const_cast<Node*>(&node)),
  m_other_ptr(nullptr),
  m_other_dtype(DataType::empty()),
  m_do_i_own_it(false),
  m_offset(node.dtype().offset()),
  m_stride(node.dtype().stride())
{}

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(Node *node)
: m_data(node->data_ptr()),
  m_orig_data_ptr(node->data_ptr()),
  m_dtype(node->dtype()), 
  m_node_ptr(node),
  m_other_ptr(nullptr),
  m_other_dtype(DataType::empty()),
  m_do_i_own_it(false),
  m_offset(node->dtype().offset()),
  m_stride(node->dtype().stride())
{}

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(const Node *node)
: m_data(const_cast<void*>(node->data_ptr())),
  m_orig_data_ptr(const_cast<void*>(node->data_ptr())),
  m_dtype(node->dtype()), 
  m_node_ptr(const_cast<Node*>(node)),
  m_other_ptr(nullptr),
  m_other_dtype(DataType::empty()),
  m_do_i_own_it(false),
  m_offset(node->dtype().offset()),
  m_stride(node->dtype().stride())
{}

//---------------------------------------------------------------------------// 
///
/// Summary Stats Helpers
///
//---------------------------------------------------------------------------// 

//---------------------------------------------------------------------------// 
template <typename T>
T
DataAccessor<T>::min()  const
{
    T res = std::numeric_limits<T>::max();
    for(index_t i = 0; i < number_of_elements(); i++)
    {
        const T &val = element(i);
        if(val < res)
        {
            res = val;
        }
    }

    return res;
}

//---------------------------------------------------------------------------// 
template <typename T>
T
DataAccessor<T>::max() const
{
    T res = std::numeric_limits<T>::lowest();
    for(index_t i = 0; i < number_of_elements(); i++)
    {
        const T &val = element(i);
        if(val > res)
        {
            res = val;
        }
    }

    return res;
}


//---------------------------------------------------------------------------// 
template <typename T>
T
DataAccessor<T>::sum() const
{
    T res =0;
    for(index_t i = 0; i < number_of_elements(); i++)
    {
        const T &val = element(i);
        res += val;
    }

    return res;
}

//---------------------------------------------------------------------------// 
template <typename T>
float64
DataAccessor<T>::mean() const
{
    float64 res =0;
    for(index_t i = 0; i < number_of_elements(); i++)
    {
        const T &val = element(i);
        res += val;
    }

    res = res / float64(number_of_elements());
    return res;
}

//---------------------------------------------------------------------------// 
template <typename T>
index_t
DataAccessor<T>::count(T val) const
{
    index_t res= 0;
    for(index_t i = 0; i < number_of_elements(); i++)
    {
        if(element(i) == val)
        {
            res++;
        }
    }
    return res;
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::fill(T value)
{
    // element_ptr(0) points at the first element with the dtype's byte offset
    // already applied (base + offset + stride * 0). element_ptr() const-qualifies
    // its return value even though the underlying buffer is not const. const_cast
    // strips the constness away. We will later cast the void* to the appropriate
    // destination dtype.
    void *data = const_cast<void*>(element_ptr(0));
    const DataType &dt = dtype();
    const index_t stride = dt.stride();
    const index_t num_elements = dt.number_of_elements();

    switch(dt.id())
    {
        // ints
        case DataType::INT8_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<int8>(value));
            break;
        case DataType::INT16_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<int16>(value));
            break;
        case DataType::INT32_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<int32>(value));
            break;
        case DataType::INT64_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<int64>(value));
            break;
        // uints
        case DataType::UINT8_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<uint8>(value));
            break;
        case DataType::UINT16_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<uint16>(value));
            break;
        case DataType::UINT32_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<uint32>(value));
            break;
        case DataType::UINT64_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<uint64>(value));
            break;
        // floats
        case DataType::FLOAT32_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<float32>(value));
            break;
        case DataType::FLOAT64_ID:
            detail::fill_typed(data, stride, num_elements, static_cast<float64>(value));
            break;
        // error
        default:
            CONDUIT_ERROR("DataAccessor does not support dtype: " << dt.name());
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::use_with(conduit::execution::ExecutionPolicy policy)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if (nullptr == m_node_ptr)
    {
        // TODO error; we can't do anything
        return;
    }

    // we are being asked to execute on the device
    if (policy.is_device_policy())
    {
        // data is already on the device
        if (execution::DeviceMemory::is_device_ptr(m_data))
        {
            // Do nothing
        }
        else // m_data is on the host
        {
            // if we started out on the host
            if (m_node_ptr->data_ptr() == m_data)
            {
                CONDUIT_ASSERT(m_other_ptr == nullptr,
                    "Using execution accessor in this way will result in a memory leak.");

                // allocate new memory and create a new dtype
                m_other_ptr = execution::DeviceMemory::allocate(
                    dtype().element_bytes() * number_of_elements());
                m_do_i_own_it = true;
                m_other_dtype = DataType(dtype().id(),
                                         number_of_elements(),
                                         0, // offset is 0
                                         DataType::default_bytes(dtype().id()), // stride
                                         dtype().element_bytes(),
                                         dtype().endianness());

                // copy data
                utils::conduit_memcpy_strided_elements(m_other_ptr,
                                                       number_of_elements(),
                                                       dtype().element_bytes(),
                                                       m_other_dtype.stride(),
                                                       m_data,
                                                       dtype().stride());

                // change where our data pointer points and update offset and stride
                m_data = m_other_ptr;
                m_offset = m_other_dtype.offset();
                m_stride = m_other_dtype.stride();
            }
            else // we started out on the device
            {
                CONDUIT_ASSERT(m_data == m_other_ptr,
                    "Using execution accessor in this way will result in a memory leak.");

                // call sync to bring our copy of the data on the host back to the device
                sync();

                // dealloc the ptr on the host now that we have copied back
                execution::HostMemory::deallocate(m_data);
                m_do_i_own_it = false;
                m_other_dtype = DataType::empty();

                // set m_data to device data and update offset and stride
                m_data = m_node_ptr->data_ptr();
                // the order of operations is important here; changing the pointer
                // will change the result of calling dtype().
                m_offset = dtype().offset();
                m_stride = dtype().stride();

                // reset m_other_ptr
                m_other_ptr = nullptr;
            }
        }
    }
    else // we are being asked to execute on the host
    {
        // data is already on the host
        if (! execution::DeviceMemory::is_device_ptr(m_data))
        {
            // Do nothing
        }
        else // m_data is on the device
        {
            // if we started out on the device
            if (m_node_ptr->data_ptr() == m_data)
            {
                CONDUIT_ASSERT(m_other_ptr == nullptr,
                    "Using execution accessor in this way will result in a memory leak.");

                // allocate new memory and create a new dtype
                m_other_ptr = execution::HostMemory::allocate(
                    dtype().element_bytes() * number_of_elements());
                m_do_i_own_it = true;
                m_other_dtype = DataType(dtype().id(),
                                         number_of_elements(),
                                         0, // offset is 0
                                         DataType::default_bytes(dtype().id()), // stride
                                         dtype().element_bytes(),
                                         dtype().endianness());

                // copy data
                utils::conduit_memcpy_strided_elements(m_other_ptr,
                                                       number_of_elements(),
                                                       dtype().element_bytes(),
                                                       m_other_dtype.stride(),
                                                       m_data,
                                                       dtype().stride());

                // change where our data pointer points and update offset and stride
                m_data = m_other_ptr;
                m_offset = m_other_dtype.offset();
                m_stride = m_other_dtype.stride();
            }
            else // we started out on the host
            {
                CONDUIT_ASSERT(m_data == m_other_ptr,
                    "Using execution accessor in this way will result in a memory leak.");

                // call sync to bring our copy of the data on the device back to the host
                sync();

                // dealloc the ptr on the host now that we have copied back
                execution::DeviceMemory::deallocate(m_data);
                m_do_i_own_it = false;
                m_other_dtype = DataType::empty();

                // set m_data to host data and update offset and stride
                m_data = m_node_ptr->data_ptr();
                m_offset = dtype().offset();
                m_stride = dtype().stride();

                // reset m_other_ptr
                m_other_ptr = nullptr;
            }
        }
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::sync()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if (nullptr == m_node_ptr)
    {
        // TODO error; we can't do anything
        return;
    }

    // if the ptrs don't point to the same place
    if (m_data != m_node_ptr->data_ptr())
    {
        if (!(m_node_ptr->dtype().compatible(dtype()) && 
              number_of_elements() == m_node_ptr->dtype().number_of_elements()))
        {
            m_node_ptr->set(dtype());
        }
        utils::conduit_memcpy_strided_elements(m_node_ptr->data_ptr(),
                                               number_of_elements(),
                                               m_node_ptr->dtype().element_bytes(),
                                               m_node_ptr->dtype().stride(),
                                               m_data,
                                               m_stride);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::assume()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if (nullptr == m_node_ptr)
    {
        // TODO error; we can't do anything
        return;
    }

    // if the ptrs don't point to the same place
    if (m_data != m_node_ptr->data_ptr())
    {
        CONDUIT_ASSERT(m_data == m_other_ptr,
            "Using execution accessor in this way will result in a memory leak.");

        // reset will deallocate the data the node points to
        m_node_ptr->reset();
        m_node_ptr->schema_ptr()->set(dtype());

        // Allow m_node_ptr to take ownership of m_data so that future
        // release()/reset() calls will free it, lest we leak memory.
        const index_t owning_allocator_id =
            execution::DeviceMemory::is_device_ptr(m_data)
                ? execution::get_device_allocator_id()
                : execution::get_host_allocator_id();
        m_node_ptr->assume_data_ptr(m_data,
                                    dtype().element_bytes() * number_of_elements(),
                                    owning_allocator_id);

        // the assumed data is now the accessor's new original backing storage
        m_orig_data_ptr = m_data;
        m_dtype = other_dtype();

        // we no longer own the data since we have given it to node
        m_other_ptr = nullptr;
        m_do_i_own_it = false;
        m_other_dtype = DataType::empty();
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::data_movement(const conduit::execution::SyncStrategy strategy)
{
    if (conduit::execution::SyncStrategy::Sync == strategy)
    {
        sync();
    }
    else if (conduit::execution::SyncStrategy::Assume == strategy)
    {
        assume();
    }
    else
    {
        CONDUIT_ERROR("Unknown data movement strategy: "
                      << conduit::execution::sync_strategy_to_string(strategy)
                      << " (" << static_cast<int>(strategy) << ").");
    }
}

//---------------------------------------------------------------------------//
template <typename T>
conduit::execution::ExecutionPolicy
DataAccessor<T>::active_space()
{
    if (execution::DeviceMemory::is_device_ptr(m_data))
    {
        return execution::ExecutionPolicy::device();
    }
    else
    {
        return execution::ExecutionPolicy::host();
    }
}

//---------------------------------------------------------------------------//
// DataAccessor::set() signed integers multi element
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T> 
void
DataAccessor<T>::set(const int8 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void
DataAccessor<T>::set(const  int16 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const int32 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const  int64 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
// DataAccessor::set() unsigned integers multi element
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const  uint8 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const  uint16 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const uint32 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const uint64 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
// DataAccessor::set() floating point multi element
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const float32 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataAccessor<T>::set(const float64 *values, index_t num_elements) const
{ 
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
//***************************************************************************//
// Set from DataAccessor
//***************************************************************************//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Set from DataAccessor signed integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int8> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int16> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int32> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int64> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
// Set from DataAccessor unsigned integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint8> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint16> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint32> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint64> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
// Set from DataAccessor floating point
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<float32> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<float64> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
//***************************************************************************//
// Set from DataArray
//***************************************************************************//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Set from DataArray signed integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int8> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int16> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int32> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int64> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
// Set from DataArray unsigned integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint8> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint16> &values) const
{ 
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint32> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint64> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
// Set from DataArray floating point
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<float32> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<float64> &values) const
{
    index_t num_elems = dtype().number_of_elements();
    detail::set_values_helper(*this, values, num_elems);
}


//---------------------------------------------------------------------------//
template <typename T>
std::string
DataAccessor<T>::to_string(const std::string &protocol) const
{
    std::ostringstream oss;
    to_string_stream(oss,protocol);
    return oss.str();
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::to_string_stream(std::ostream &os,
                                  const std::string &protocol) const
{
    if(protocol == "yaml")
    {
        to_yaml_stream(os);
    }
    else if(protocol == "json")
    {
        to_json_stream(os);
    }
    else
    {
        // unsupported
        CONDUIT_ERROR("Unknown DataType::to_string protocol:" << protocol
                     <<"\nSupported protocols:\n"
                     <<" json, yaml");
    }

}

//---------------------------------------------------------------------------//
template <typename T>
std::string
DataAccessor<T>::to_string_default() const
{
    return to_string();
}

//---------------------------------------------------------------------------//
template <typename T>
std::string
DataAccessor<T>::to_json() const
{
    std::ostringstream oss;
    to_json_stream(oss);
    return oss.str();
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::to_json_stream(std::ostream &os) const
{
    index_t nele = number_of_elements();
    // note: nele == 0 case:
    // https://github.com/LLNL/conduit/issues/992
    // we want empty arrays to display as [] not empty string
    if(nele == 0 || nele > 1)
        os << "[";

    bool first=true;
    for(index_t idx = 0; idx < nele; idx++)
    {
        if(!first)
            os << ", ";

        // need to deal with nan and infs for fp cases
        if(std::is_floating_point<T>::value)
        {
            std::string fs = utils::float64_to_string((float64)element(idx));
            //check for inf and nan
            // looking for 'n' covers inf and nan
            bool inf_or_nan = fs.find('n') != std::string::npos;

            if(inf_or_nan)
                os << "\"";

            os << fs;

            if(inf_or_nan)
                os << "\"";
        }
        else
        {
            os << element(idx);
        }

        first=false;
    }
    // note: nele == 0 case:
    // https://github.com/LLNL/conduit/issues/992
    // we want empty arrays to display as [] not empty string
    if(nele == 0 || nele > 1)
        os << "]";
}

//---------------------------------------------------------------------------//
template <typename T>
std::string
DataAccessor<T>::to_yaml() const
{
    std::ostringstream oss;
    to_yaml_stream(oss);
    return oss.str();
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::to_yaml_stream(std::ostream &os) const
{
    // yep, its the same as to_json_stream ...
    to_json_stream(os);;
}

//---------------------------------------------------------------------------//
template <typename T>
std::string
DataAccessor<T>::to_summary_string_default() const
{
    return to_summary_string();
}

//---------------------------------------------------------------------------//
template <typename T>
std::string
DataAccessor<T>::to_summary_string(index_t threshold) const
{
    std::ostringstream oss;
    to_summary_string_stream(oss, threshold);
    return oss.str();
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::to_summary_string_stream(std::ostream &os,
                                          index_t threshold) const
{
    // if we are less than or equal to threshold, we use to_yaml
    index_t nele = number_of_elements();

    if(nele <= threshold)
    {
        to_yaml_stream(os);
    }
    else
    {
        // if above threshold only show threshold # of values
        index_t half = threshold / 2;
        index_t bottom = half;
        index_t top = half;

        //
        // if odd, show 1/2 +1 first
        //

        if( (threshold % 2) > 0)
        {
            bottom++;
        }

        // note: nele == 0 case:
        // https://github.com/LLNL/conduit/issues/992
        // we want empty arrays to display as [] not empty string
        if(nele == 0 || nele > 1)
            os << "[";

        bool done  = (nele == 0);
        index_t idx = 0;

        while(!done)
        {
            // if not first, add a comma prefix
            if(idx > 0 )
                os << ", ";

            // need to deal with nan and infs for fp cases
            if(std::is_floating_point<T>::value)
            {
                std::string fs = utils::float64_to_string((float64)element(idx));
                //check for inf and nan
                // looking for 'n' covers inf and nan
                bool inf_or_nan = fs.find('n') != std::string::npos;

                if(inf_or_nan)
                    os << "\"";

                os << fs;

                if(inf_or_nan)
                    os << "\"";
            }
            else
            {
                os << element(idx);
            }

            idx++;

            if(idx == bottom)
            {
                idx = nele - top;
                os << ", ...";
            }

            if(idx == nele)
            {
                done = true;
            }
        }

        // note: nele == 0 case:
        // https://github.com/LLNL/conduit/issues/992
        // we want empty arrays to display as [] not empty string
        if(nele == 0 || nele > 1)
            os << "]";

    }
}


//-----------------------------------------------------------------------------
//
// -- conduit::DataAccessor explicit instantiations for supported types --
//
//-----------------------------------------------------------------------------
template class DataAccessor<int8>;
template class DataAccessor<int16>;
template class DataAccessor<int32>;
template class DataAccessor<int64>;

template class DataAccessor<uint8>;
template class DataAccessor<uint16>;
template class DataAccessor<uint32>;
template class DataAccessor<uint64>;

template class DataAccessor<float32>;
template class DataAccessor<float64>;

// gap template instantiations for c-native types

// we never use 'char' directly as a type,
// so we always need to inst the char case
template class DataAccessor<char>;

#ifndef CONDUIT_USE_CHAR
template class DataAccessor<signed char>;
template class DataAccessor<unsigned char>;
#endif

#ifndef CONDUIT_USE_SHORT
template class DataAccessor<signed short>;
template class DataAccessor<unsigned short>;
#endif

#ifndef CONDUIT_USE_INT
template class DataAccessor<signed int>;
template class DataAccessor<unsigned int>;
#endif

#ifndef CONDUIT_USE_LONG
template class DataAccessor<signed long>;
template class DataAccessor<unsigned long>;
#endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
template class DataAccessor<signed long long>;
template class DataAccessor<unsigned long long>;
#endif

#ifndef CONDUIT_USE_FLOAT
template class DataAccessor<float>;
#endif

#ifndef CONDUIT_USE_DOUBLE
template class DataAccessor<double>;
#endif

#ifdef CONDUIT_USE_LONG_DOUBLE
    template class DataAccessor<long double>;
#endif


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
