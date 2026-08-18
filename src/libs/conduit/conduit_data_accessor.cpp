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
#include <vector>

//-----------------------------------------------------------------------------
// -- conduit  includes -- 
//-----------------------------------------------------------------------------
#include "conduit_memory_manager.hpp"
#include "conduit_node.hpp"
#include "conduit_data_array.hpp"
#include "conduit_execution.hpp"
#include "conduit_execution_dispatch.hpp"
#include "conduit_data_kernels.hpp"
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
template <typename T>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const T *values,
                  index_t num_elements)
{
    // Avoid performing unnecessary work for empty arrays
    if (num_elements <= 0)
    {
        return;
    }

    execution::ExecutionPolicy policy = detail::select_policy(accessor.active_space(),
                                                              num_elements);

    const bool dst_on_device = execution::DeviceMemory::is_device_ptr(accessor.element_ptr(0));
    const bool src_on_device = execution::DeviceMemory::is_device_ptr(values);

    if (dst_on_device == src_on_device)
    {
        execution::dispatch(accessor, [&](auto vals)
        {
            copy_from_ptr_kernel(policy, num_elements, values, vals);
        });
    }
    else // dst and src are in different memory spaces
    {
        // This could be implemented, but forcing the caller to decide where
        // the data should live first keeps the cost of allocating and copying
        // from becoming an unexpected side-effect of set().
        CONDUIT_ERROR("DataAccessor::set() cannot copy data to and from "
                      "different memory spaces. Use use_with() and sync() "
                      "to ensure that the source and destination data live in "
                      "the same memory space first.");
    }
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const U *values,
                  index_t num_elements)
{
    if (execution::DeviceMemory::is_device_ptr(values))
    {
        CONDUIT_ERROR("DataAccessor::set() cannot convert from a "
                      "device-resident source of a different type. Use "
                      "use_with() and sync() to ensure that the source and "
                      "destination data live in the same memory space first.");
    }

    // Pre-convert to the accessor's logical type so the kernel only ever needs
    // to be instantiated for a const T* source. The conversion loop below runs
    // on the host, so a device-resident source must be rejected before it.
    std::vector<T> converted(num_elements);
    for (index_t i = 0; i < num_elements; i++)
    {
        converted[i] = static_cast<T>(values[i]);
    }
    set_values_helper(accessor, converted.data(), num_elements);
}

//-----------------------------------------------------------------------------
template <typename U, template <typename> class Accessor, typename T>
void
set_values_acc_helper(const DataAccessor<T> &accessor,
                      const Accessor<U> &values,
                      index_t num_elements)
{
    // Avoid performing unnecessary work for empty arrays
    if (num_elements <= 0)
    {
        return;
    }

    execution::ExecutionPolicy policy = detail::select_policy(accessor.active_space(),
                                                              num_elements);

    const bool dst_on_device = execution::DeviceMemory::is_device_ptr(accessor.element_ptr(0));
    const bool src_on_device = execution::DeviceMemory::is_device_ptr(values.element_ptr(0));

    if (dst_on_device == src_on_device)
    {
        copy_from_acc_kernel(policy, num_elements, values, accessor);
    }
    else // dst and src are in different memory spaces
    {
        // This could be implemented, but forcing the caller to decide where
        // the data should live first keeps the cost of allocating and copying
        // from becoming an unexpected side-effect of set().
        CONDUIT_ERROR("DataAccessor::set() cannot copy data to and from "
                      "different memory spaces. Use use_with() and sync() "
                      "to ensure that the source and destination data live in "
                      "the same memory space first.");
    }
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const DataAccessor<U> &values,
                  index_t num_elements)
{
    set_values_acc_helper(accessor, values, num_elements);
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const DataArray<U> &values,
                  index_t num_elements)
{
    set_values_acc_helper(accessor, values, num_elements);
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
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = detail::select_policy(active_space(),
                                                              num_elements);
    return detail::min_kernel<T>(policy, num_elements, *this);
}

//---------------------------------------------------------------------------// 
template <typename T>
T
DataAccessor<T>::max() const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = detail::select_policy(active_space(),
                                                              num_elements);
    return detail::max_kernel<T>(policy, num_elements, *this);
}


//---------------------------------------------------------------------------// 
template <typename T>
T
DataAccessor<T>::sum() const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = detail::select_policy(active_space(),
                                                              num_elements);
    return detail::sum_kernel<T>(policy, num_elements, *this);
}

//---------------------------------------------------------------------------// 
template <typename T>
float64
DataAccessor<T>::mean() const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = detail::select_policy(active_space(),
                                                              num_elements);
    return detail::mean_kernel<T>(policy, num_elements, *this) /
           static_cast<float64>(num_elements);
}

//---------------------------------------------------------------------------// 
template <typename T>
index_t
DataAccessor<T>::count(T val) const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = detail::select_policy(active_space(),
                                                              num_elements);
    return detail::count_kernel<T>(policy, num_elements, *this, val);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::fill(T value)
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = detail::select_policy(active_space(),
                                                              num_elements);
    execution::dispatch(*this, [&](auto vals)
    {
        detail::fill_kernel(policy, num_elements, vals, value);
    });
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
                                                       element_ptr(0),
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
                                                       element_ptr(0),
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
        // data_ptr() is the node's base pointer, so we add the node dtype's
        // offset to write back to the correct elements
        utils::conduit_memcpy_strided_elements(
            static_cast<char*>(m_node_ptr->data_ptr()) + m_node_ptr->dtype().offset(),
            number_of_elements(),
            m_node_ptr->dtype().element_bytes(),
            m_node_ptr->dtype().stride(),
            element_ptr(0),
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
DataAccessor<T>::active_space() const
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
