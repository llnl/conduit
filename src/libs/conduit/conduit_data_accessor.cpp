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
#include "conduit_execution_dispatch.hpp"
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

//
// Kernels
//

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct Fill
{
    U vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.set(i, value);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
accessor_fill_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const U vals,
                     const T value)
{
    // Small host arrays are faster to fill without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        Fill<T, U> kernel{vals, value};
        for (index_t i = 0; i < num_elements; i++)
        {
            kernel(i);
        }
        return;
    }

    execution::forall(policy, 0, num_elements, Fill<T, U>{vals, value});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T>
struct FillAccessor
{
    DataAccessor<T> vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.set(i, value);
    }
};

//-----------------------------------------------------------------------------
template <typename T>
void
accessor_fill_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const DataAccessor<T> vals,
                     const T value)
{
    // Small host arrays are faster to fill without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        FillAccessor<T> kernel{vals, value};
        for (index_t i = 0; i < num_elements; i++)
        {
            kernel(i);
        }
        return;
    }

    execution::forall(policy, 0, num_elements, FillAccessor<T>{vals, value});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromView
{
    T vals;
    U src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.set(i, src[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
accessor_copy_from_view_kernel(execution::ExecutionPolicy &policy,
                               index_t num_elements,
                               const U src,
                               const T vals)
{
    // Small host arrays are faster to copy without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        CopyFromView<T, U> kernel{vals, src};
        for (index_t i = 0; i < num_elements; i++)
        {
            kernel(i);
        }
        return;
    }

    execution::forall(policy, 0, num_elements, CopyFromView<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromViewAccessor
{
    DataAccessor<T> vals;
    U src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.set(i, static_cast<T>(src[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
accessor_copy_from_view_kernel(execution::ExecutionPolicy &policy,
                               index_t num_elements,
                               const U src,
                               const DataAccessor<T> vals)
{
    // Small host arrays are faster to copy without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        CopyFromViewAccessor<T, U> kernel{vals, src};
        for (index_t i = 0; i < num_elements; i++)
        {
            kernel(i);
        }
        return;
    }

    execution::forall(policy, 0, num_elements, CopyFromViewAccessor<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct Min
{
    execution::ReduceMin<T> reducer;
    U vals;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer.min(static_cast<T>(vals[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
T
accessor_min_kernel(execution::ExecutionPolicy &policy,
                    index_t num_elements,
                    const U vals)
{
    // Small host arrays are faster to compute min without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        T res = std::numeric_limits<T>::max();
        for (index_t i = 0; i < num_elements; i++)
        {
            res = std::min(res, static_cast<T>(vals[i]));
        }
        return res;
    }

    execution::ReduceMin<T> reducer(std::numeric_limits<T>::max());
    execution::forall(policy, 0, num_elements, Min<T, U>{reducer, vals});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct Max
{
    execution::ReduceMax<T> reducer;
    U vals;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer.max(static_cast<T>(vals[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
T
accessor_max_kernel(execution::ExecutionPolicy &policy,
                    index_t num_elements,
                    const U vals)
{
    // Small host arrays are faster to compute max without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        T res = std::numeric_limits<T>::lowest();
        for (index_t i = 0; i < num_elements; i++)
        {
            res = std::max(res, static_cast<T>(vals[i]));
        }
        return res;
    }

    execution::ReduceMax<T> reducer(std::numeric_limits<T>::lowest());
    execution::forall(policy, 0, num_elements, Max<T, U>{reducer, vals});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct Sum
{
    execution::ReduceSum<T> reducer;
    U vals;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer += static_cast<T>(vals[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
T
accessor_sum_kernel(execution::ExecutionPolicy &policy,
                    index_t num_elements,
                    const U vals)
{
    // Small host arrays are faster to sum without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        T res = static_cast<T>(0);
        for (index_t i = 0; i < num_elements; i++)
        {
            res += static_cast<T>(vals[i]);
        }
        return res;
    }

    execution::ReduceSum<T> reducer(static_cast<T>(0));
    execution::forall(policy, 0, num_elements, Sum<T, U>{reducer, vals});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct Count
{
    execution::ReduceSum<index_t> reducer;
    U vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer += (static_cast<T>(vals[i]) == value) ? 1 : 0;
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
index_t
accessor_count_kernel(execution::ExecutionPolicy &policy,
                      index_t num_elements,
                      const U vals,
                      const T value)
{
    // Small host arrays are faster to count without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        index_t res = 0;
        for (index_t i = 0; i < num_elements; i++)
        {
            res += (static_cast<T>(vals[i]) == value) ? 1 : 0;
        }
        return res;
    }

    execution::ReduceSum<index_t> reducer(0);
    execution::forall(policy, 0, num_elements, Count<T, U>{reducer, vals, value});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

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

    execution::ExecutionPolicy policy = accessor.active_policy();

    const bool dst_on_device = policy.is_device_policy();
    const bool src_on_device = execution::DeviceMemory::is_device_ptr(values);

    if (dst_on_device == src_on_device)
    {
        execution::dispatch(accessor, [&](auto vals)
        {
            accessor_copy_from_view_kernel(policy, num_elements, values, vals);
        });
    }
    else // dst and src are in different memory spaces
    {
        // Set up
        const DataType &dtype = accessor.dtype();
        const size_t type_size = sizeof(T);
        const size_t num_bytes = num_elements * type_size;

        // When the source type matches the destination's underlying dtype and
        // the destination has a compact layout, no conversion is necessary,
        // so we can just memcpy the values directly to the destination.
        const bool same_type =
            (std::is_floating_point<T>::value && dtype.is_floating_point()) ||
            (std::is_integral<T>::value && std::is_signed<T>::value &&
             dtype.is_signed_integer()) ||
            (std::is_integral<T>::value && std::is_unsigned<T>::value &&
             dtype.is_unsigned_integer());
        const bool same_layout = same_type &&
                                 dtype.stride() == type_size &&
                                 dtype.element_bytes() == type_size;
        if (same_layout)
        {
            utils::conduit_memcpy(const_cast<void*>(accessor.element_ptr(0)),
                                  values,
                                  num_bytes);
            return;
        }

        void *temp_ptr = dst_on_device
            ? execution::DeviceMemory::allocate(num_bytes)
            : execution::HostMemory::allocate(num_bytes);
        utils::conduit_memcpy_strided_elements(temp_ptr,
                                               num_elements,
                                               type_size,
                                               type_size,
                                               values,
                                               type_size);
        const T *temp_vals = static_cast<const T*>(temp_ptr);
        execution::dispatch(accessor, [&](auto vals)
        {
            accessor_copy_from_view_kernel(policy, num_elements, temp_vals, vals);
        });

        // Clean up
        if (dst_on_device)
        {
            execution::DeviceMemory::deallocate(temp_ptr);
        }
        else // if (!dst_on_device)
        {
            execution::HostMemory::deallocate(temp_ptr);
        }
    }
}

//-----------------------------------------------------------------------------
template <typename U, template <typename> class View, typename T>
void
set_values_view_helper(const DataAccessor<T> &accessor,
                       const View<U> &values,
                       index_t num_elements)
{
    // Avoid performing unnecessary work for empty arrays
    if (num_elements <= 0)
    {
        return;
    }

    execution::ExecutionPolicy policy = accessor.active_policy();

    const bool dst_on_device = policy.is_device_policy();
    const bool src_on_device = values.active_policy().is_device_policy();

    if (dst_on_device == src_on_device)
    {
        accessor_copy_from_view_kernel(policy, num_elements, values, accessor);
    }
    else // dst and src are in different memory spaces
    {
        // Set up
        const DataType &src_dt = values.dtype();
        const size_t type_size = src_dt.element_bytes();
        const size_t num_bytes = num_elements * type_size;
        void *temp_ptr = dst_on_device
            ? execution::DeviceMemory::allocate(num_bytes)
            : execution::HostMemory::allocate(num_bytes);
        const DataType temp_dtype(src_dt.id(),
                                  num_elements,
                                  0, // offset is 0
                                  DataType::default_bytes(src_dt.id()), // stride
                                  src_dt.element_bytes(),
                                  src_dt.endianness());
        utils::conduit_memcpy_strided_elements(temp_ptr,
                                               num_elements,
                                               type_size,
                                               temp_dtype.stride(),
                                               values.element_ptr(0),
                                               src_dt.stride());
        const View<U> temp_view(static_cast<const void*>(temp_ptr),
                                temp_dtype);
        accessor_copy_from_view_kernel(policy, num_elements, temp_view, accessor);

        // Clean up
        if (dst_on_device)
        {
            execution::DeviceMemory::deallocate(temp_ptr);
        }
        else // if (!dst_on_device)
        {
            execution::HostMemory::deallocate(temp_ptr);
        }
    }
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const U *values,
                  index_t num_elements)
{
    // Avoid performing unnecessary work for empty arrays
    if (num_elements <= 0)
    {
        return;
    }

    execution::ExecutionPolicy policy = accessor.active_policy();

    const bool dst_on_device = policy.is_device_policy();
    const bool src_on_device = execution::DeviceMemory::is_device_ptr(values);

    if (dst_on_device == src_on_device)
    {
        // The kernel converts U to T per element
        accessor_copy_from_view_kernel(policy, num_elements, values, accessor);
    }
    else // dst and src are in different memory spaces
    {
        // Set up
        const size_t type_size = sizeof(U);
        const size_t num_bytes = num_elements * type_size;
        void *temp_ptr = dst_on_device
            ? execution::DeviceMemory::allocate(num_bytes)
            : execution::HostMemory::allocate(num_bytes);
        utils::conduit_memcpy_strided_elements(temp_ptr,
                                               num_elements,
                                               type_size,
                                               type_size,
                                               values,
                                               type_size);
        const U *temp_vals = static_cast<const U*>(temp_ptr);
        accessor_copy_from_view_kernel(policy, num_elements, temp_vals, accessor);

        // Clean up
        if (dst_on_device)
        {
            execution::DeviceMemory::deallocate(temp_ptr);
        }
        else // if (!dst_on_device)
        {
            execution::HostMemory::deallocate(temp_ptr);
        }
    }
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const DataAccessor<U> &values,
                  index_t num_elements)
{
    set_values_view_helper(accessor, values, num_elements);
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const DataArray<U> &values,
                  index_t num_elements)
{
    set_values_view_helper(accessor, values, num_elements);
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
  m_stride(0),
  m_policy(execution::ExecutionPolicy::empty())
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
  m_stride(0),
  m_policy(execution::ExecutionPolicy::empty())
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
  m_stride(0),
  m_policy(execution::ExecutionPolicy::empty())
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
  m_stride(node.dtype().stride()),
  m_policy(execution::ExecutionPolicy::empty())
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
  m_stride(node.dtype().stride()),
  m_policy(execution::ExecutionPolicy::empty())
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
  m_stride(node->dtype().stride()),
  m_policy(execution::ExecutionPolicy::empty())
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
  m_stride(node->dtype().stride()),
  m_policy(execution::ExecutionPolicy::empty())
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

    execution::ExecutionPolicy policy = active_policy();
    T res = std::numeric_limits<T>::max();
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::accessor_min_kernel<T>(policy, num_elements, vals);
    });

    return res;
}

//---------------------------------------------------------------------------// 
template <typename T>
T
DataAccessor<T>::max() const
{
    const index_t num_elements = number_of_elements();

    execution::ExecutionPolicy policy = active_policy();
    T res = std::numeric_limits<T>::lowest();
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::accessor_max_kernel<T>(policy, num_elements, vals);
    });

    return res;
}


//---------------------------------------------------------------------------// 
template <typename T>
T
DataAccessor<T>::sum() const
{
    const index_t num_elements = number_of_elements();

    execution::ExecutionPolicy policy = active_policy();
    T res = 0;
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::accessor_sum_kernel<T>(policy, num_elements, vals);
    });

    return res;
}

//---------------------------------------------------------------------------// 
template <typename T>
float64
DataAccessor<T>::mean() const
{
    const index_t num_elements = number_of_elements();

    execution::ExecutionPolicy policy = active_policy();
    float64 res = 0.0;
    execution::dispatch(*this, [&](auto vals)
    {
        // Accumulate in float64 for accuracy
        res = detail::accessor_sum_kernel<float64>(policy, num_elements, vals);
    });

    return res / static_cast<float64>(num_elements);
}

//---------------------------------------------------------------------------// 
template <typename T>
index_t
DataAccessor<T>::count(T val) const
{
    const index_t num_elements = number_of_elements();

    execution::ExecutionPolicy policy = active_policy();
    index_t res = 0;
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::accessor_count_kernel<T>(policy, num_elements, vals, val);
    });

    return res;
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::fill(T value)
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = active_policy();

    execution::dispatch(*this, [&](auto vals)
    {
        detail::accessor_fill_kernel(policy, num_elements, vals, value);
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
        if (active_policy().is_device_policy())
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

        // m_data is now (or already was) in device memory
        m_policy = policy;
    }
    else // we are being asked to execute on the host
    {
        // data is already on the host
        if (! active_policy().is_device_policy())
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

        // m_data is now (or already was) in host memory
        m_policy = policy;
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
        utils::conduit_memcpy_strided_elements(m_node_ptr->element_ptr(0),
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
            active_policy().is_device_policy()
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
DataAccessor<T>::active_policy() const
{
    // Starting as EMPTY_ID allows us to lazily determine m_policy so that we
    // only query is_device_ptr() once we actually need to.
    if (execution::ExecutionPolicy::PolicyID::EMPTY_ID == m_policy.policy_id())
    {
        // Caching the result allows us to avoid calling is_device_ptr()
        // repeatedly across the lifetime of this object, which has a small
        // but measurable overhead.
        m_policy = execution::DeviceMemory::is_device_ptr(m_data)
                      ? execution::ExecutionPolicy::device()
                      : execution::ExecutionPolicy::host();
    }

    return m_policy;
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
