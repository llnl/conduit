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
template <typename U>
struct StagedSourceAccessor
{
    // Keep a temporary Node alive so the staged accessor can reuse the normal
    // node-backed DataAccessor machinery without introducing a second copy
    // path for raw pointers, DataArray sources, and DataAccessor sources.
    Node node;
    DataAccessor<U> accessor;

    StagedSourceAccessor()
    : node(),
      accessor()
    {}
};

//-----------------------------------------------------------------------------
template <typename T>
void
set_value_helper(const DataAccessor<T> &accessor, index_t idx, T value)
{
    // Host-side scalar writes use the same runtime dtype dispatch as the
    // historic implementation and write directly into the wrapped storage.
    switch(accessor.dtype().id())
    {
        case DataType::INT8_ID:
            (*(int8*)(accessor.element_ptr(idx))) = static_cast<int8>(value);
            break;
        case DataType::INT16_ID:
            (*(int16*)(accessor.element_ptr(idx))) = static_cast<int16>(value);
            break;
        case DataType::INT32_ID:
            (*(int32*)(accessor.element_ptr(idx))) = static_cast<int32>(value);
            break;
        case DataType::INT64_ID:
            (*(int64*)(accessor.element_ptr(idx))) = static_cast<int64>(value);
            break;
        case DataType::UINT8_ID:
            (*(uint8*)(accessor.element_ptr(idx))) = static_cast<uint8>(value);
            break;
        case DataType::UINT16_ID:
            (*(uint16*)(accessor.element_ptr(idx))) = static_cast<uint16>(value);
            break;
        case DataType::UINT32_ID:
            (*(uint32*)(accessor.element_ptr(idx))) = static_cast<uint32>(value);
            break;
        case DataType::UINT64_ID:
            (*(uint64*)(accessor.element_ptr(idx))) = static_cast<uint64>(value);
            break;
        case DataType::FLOAT32_ID:
            (*(float32*)(accessor.element_ptr(idx))) = static_cast<float32>(value);
            break;
        case DataType::FLOAT64_ID:
            (*(float64*)(accessor.element_ptr(idx))) = static_cast<float64>(value);
            break;
        default:
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << accessor.dtype().name());
    }
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
set_values_helper(const DataAccessor<T> &accessor,
                  const U &values,
                  index_t num_elements)
{
    // This is the simple fallback path for builds without a usable execution
    // policy and for mixed-space cases that are handled serially on the host.
    switch(accessor.dtype().id())
    {
        case DataType::INT8_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(int8*)(accessor.element_ptr(i))) =
                    static_cast<int8>(static_cast<T>(values[i]));
            }
            break;
        case DataType::INT16_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(int16*)(accessor.element_ptr(i))) =
                    static_cast<int16>(static_cast<T>(values[i]));
            }
            break;
        case DataType::INT32_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(int32*)(accessor.element_ptr(i))) =
                    static_cast<int32>(static_cast<T>(values[i]));
            }
            break;
        case DataType::INT64_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(int64*)(accessor.element_ptr(i))) =
                    static_cast<int64>(static_cast<T>(values[i]));
            }
            break;
        case DataType::UINT8_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(uint8*)(accessor.element_ptr(i))) =
                    static_cast<uint8>(static_cast<T>(values[i]));
            }
            break;
        case DataType::UINT16_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(uint16*)(accessor.element_ptr(i))) =
                    static_cast<uint16>(static_cast<T>(values[i]));
            }
            break;
        case DataType::UINT32_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(uint32*)(accessor.element_ptr(i))) =
                    static_cast<uint32>(static_cast<T>(values[i]));
            }
            break;
        case DataType::UINT64_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(uint64*)(accessor.element_ptr(i))) =
                    static_cast<uint64>(static_cast<T>(values[i]));
            }
            break;
        case DataType::FLOAT32_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(float32*)(accessor.element_ptr(i))) =
                    static_cast<float32>(static_cast<T>(values[i]));
            }
            break;
        case DataType::FLOAT64_ID:
            for(index_t i = 0; i < num_elements; i++)
            {
                (*(float64*)(accessor.element_ptr(i))) =
                    static_cast<float64>(static_cast<T>(values[i]));
            }
            break;
        default:
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << accessor.dtype().name());
    }
}

//-----------------------------------------------------------------------------
template <typename U>
StagedSourceAccessor<U>
make_staged_source_accessor(const U *values, index_t num_elements)
{
    // Wrap raw pointer input in a temporary node-backed accessor so all bulk
    // setters can share the same copy orchestration below.
    StagedSourceAccessor<U> staged;
    staged.node.set_external(const_cast<U*>(values), num_elements);
    staged.accessor = DataAccessor<U>(staged.node);
    return staged;
}

//-----------------------------------------------------------------------------
template <typename U>
StagedSourceAccessor<U>
make_staged_source_accessor(const DataArray<U> &values)
{
    // DataArray already exposes dtype and data pointer, so the temporary node
    // simply forwards that description into a uniform accessor wrapper.
    StagedSourceAccessor<U> staged;
    staged.node.set_external(values.dtype(), values.data_ptr());
    staged.accessor = DataAccessor<U>(staged.node);
    return staged;
}

//-----------------------------------------------------------------------------
template <typename U>
StagedSourceAccessor<U>
make_staged_source_accessor(const DataAccessor<U> &values)
{
    // Re-wrap the source accessor in a node-backed view so use_with(...) can
    // reuse the normal memory-space migration logic when the source and
    // destination need to execute in the same space.
    StagedSourceAccessor<U> staged;
    const DataType staged_dtype(values.dtype().id(),
                                values.number_of_elements(),
                                0,
                                values.dtype().stride(),
                                values.dtype().element_bytes(),
                                values.dtype().endianness());
    staged.node.set_external(staged_dtype,
                             const_cast<void*>(values.element_ptr(0)));
    staged.accessor = DataAccessor<U>(staged.node);
    return staged;
}

//-----------------------------------------------------------------------------
inline bool
policy_is_supported(const execution::ExecutionPolicy &policy)
{
    // active_space() can report host or device based on the wrapped pointer,
    // but the corresponding backend may still be disabled in this build.
    if(policy.is_device_policy())
    {
        return execution::ExecutionPolicy::is_device_enabled();
    }

    return execution::ExecutionPolicy::is_host_enabled();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
set_values_from_accessor_helper(DataAccessor<T> &accessor,
                                DataAccessor<U> &source,
                                index_t num_elements)
{
    if(num_elements == 0)
    {
        return;
    }

    // The destination accessor decides where execution should happen because
    // it owns the storage that will be written.
    execution::ExecutionPolicy policy = accessor.active_space();
    if(!policy_is_supported(policy))
    {
        set_values_helper(accessor, source, num_elements);
        return;
    }

    source.use_with(policy);

    if(policy.is_device_policy())
    {
        //
        // The device path uses a staged setup on purpose.
        //
        // DataAccessor has two independent kinds of type flexibility:
        // 1. the logical accessor type T/U used by the C++ API
        // 2. the runtime destination dtype() stored in the wrapped Node/data
        //
        // A direct device kernel that reads a generic DataAccessor<U> source
        // and writes directly into a generic DataAccessor<T> destination has
        // to carry both concerns at once. In practice that means every T/U
        // device instantiation also drags along the full destination dtype
        // switch and the corresponding write kernels. That shape preserves the
        // semantics, but it makes HIP/CUDA compilation scale very poorly.
        //
        // We only need that tradeoff on the device path. Host/OpenMP builds
        // can keep the straightforward one-pass implementation in this .cpp
        // file without causing unreasonable compile times.
        //
        // To keep the device compile surface smaller while preserving the same
        // conversion semantics, we split device execution into two passes:
        // 1. stage the source values into a temporary contiguous buffer of the
        //    destination accessor's logical type T in the active device space
        // 2. dispatch once on the runtime destination dtype and write that
        //    staged T buffer into the final destination storage
        //
        // This keeps the source-conversion kernel small (templated on T/U but
        // with no destination dtype switch) and keeps the destination write
        // kernel small (templated on T with a single outer dtype dispatch).
        // The extra temporary allocation is only used for device execution to
        // control compile complexity; it is not needed for host/OpenMP.
        //
        T *staged_values =
            static_cast<T*>(execution::DeviceMemory::allocate(sizeof(T) * num_elements));

        detail::stage_values_forall_helper(policy,
                                           staged_values,
                                           source,
                                           num_elements);
        detail::set_staged_values_forall_helper(accessor,
                                                policy,
                                                staged_values,
                                                num_elements);

        execution::DeviceMemory::deallocate(staged_values);
        return;
    }

    switch(accessor.dtype().id())
    {
        case DataType::INT8_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(int8*)(accessor.element_ptr(i))) =
                    static_cast<int8>(static_cast<T>(source[i]));
            });
            break;
        case DataType::INT16_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(int16*)(accessor.element_ptr(i))) =
                    static_cast<int16>(static_cast<T>(source[i]));
            });
            break;
        case DataType::INT32_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(int32*)(accessor.element_ptr(i))) =
                    static_cast<int32>(static_cast<T>(source[i]));
            });
            break;
        case DataType::INT64_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(int64*)(accessor.element_ptr(i))) =
                    static_cast<int64>(static_cast<T>(source[i]));
            });
            break;
        case DataType::UINT8_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(uint8*)(accessor.element_ptr(i))) =
                    static_cast<uint8>(static_cast<T>(source[i]));
            });
            break;
        case DataType::UINT16_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(uint16*)(accessor.element_ptr(i))) =
                    static_cast<uint16>(static_cast<T>(source[i]));
            });
            break;
        case DataType::UINT32_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(uint32*)(accessor.element_ptr(i))) =
                    static_cast<uint32>(static_cast<T>(source[i]));
            });
            break;
        case DataType::UINT64_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(uint64*)(accessor.element_ptr(i))) =
                    static_cast<uint64>(static_cast<T>(source[i]));
            });
            break;
        case DataType::FLOAT32_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(float32*)(accessor.element_ptr(i))) =
                    static_cast<float32>(static_cast<T>(source[i]));
            });
            break;
        case DataType::FLOAT64_ID:
            execution::forall(policy, 0, num_elements, [=](index_t i)
            {
                (*(float64*)(accessor.element_ptr(i))) =
                    static_cast<float64>(static_cast<T>(source[i]));
            });
            break;
        default:
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << accessor.dtype().name());
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
    switch(dtype().id())
    {
        // ints
        case DataType::INT8_ID:
        {
            int8 v = static_cast<int8>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(int8*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::INT16_ID:
        {
            int16 v = static_cast<int16>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(int16*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::INT32_ID:
        {
            int32 v = static_cast<int32>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(int32*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::INT64_ID:
        {
            int64 v = static_cast<int64>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(int64*)(element_ptr(i))) = v;
            }
            break;
        }
        // uints
        case DataType::UINT8_ID:
        {
            uint8 v = static_cast<uint8>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(uint8*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::UINT16_ID:
        {
            uint16 v = static_cast<uint16>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(uint16*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::UINT32_ID:
        {
            uint32 v = static_cast<uint32>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(uint32*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::UINT64_ID:
        {
            uint64 v = static_cast<uint64>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(uint64*)(element_ptr(i))) = v;
            }
            break;
        }
        // floats
        case DataType::FLOAT32_ID:
        {
            float32 v = static_cast<float32>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(float32*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::FLOAT64_ID:
        {
            float64 v = static_cast<float64>(value);
            for(index_t i=0;i < dtype().number_of_elements(); i++)
            {
                 (*(float64*)(element_ptr(i))) = v;
            }
            break;
        }
        default:
            // error
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << dtype().name());
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
        m_node_ptr->set_data_ptr(m_data);

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
conduit::execution::ExecutionPolicy
DataAccessor<T>::active_space()
{
    // DataAccessor execution follows the space of the wrapped storage pointer.
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
template <typename T>
void
DataAccessor<T>::set(index_t idx, int8 value)
{
    // Scalar setters route through the device helper when the destination
    // storage lives on device so the write occurs in the correct memory space.
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, int16 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, int32 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, int64 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, uint8 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, uint16 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, uint32 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, uint64 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, float32 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, float64 value)
{
    if(active_space().is_device_policy())
    {
        detail::set_value_forall_helper(*this, idx, static_cast<T>(value));
        return;
    }

    detail::set_value_helper(*this, idx, static_cast<T>(value));
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const int8 *values, index_t num_elements)
{
    // Raw-pointer bulk setters first adapt the source into the shared
    // accessor-based copy path so host, OpenMP, and device execution all
    // preserve the same conversion semantics.
    detail::StagedSourceAccessor<int8> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const int16 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<int16> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const int32 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<int32> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const int64 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<int64> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const uint8 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<uint8> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const uint16 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<uint16> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const uint32 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<uint32> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const uint64 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<uint64> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const float32 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<float32> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const float64 *values, index_t num_elements)
{
    detail::StagedSourceAccessor<float64> staged =
        detail::make_staged_source_accessor(values, num_elements);
    detail::set_values_from_accessor_helper(*this, staged.accessor, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int8> &values)
{
    // DataAccessor sources already know their layout and active memory space,
    // so this overload only has to funnel them into the shared bulk copy
    // helper that chooses host, OpenMP, or device execution.
    detail::StagedSourceAccessor<int8> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int16> &values)
{
    detail::StagedSourceAccessor<int16> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int32> &values)
{
    detail::StagedSourceAccessor<int32> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int64> &values)
{
    detail::StagedSourceAccessor<int64> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint8> &values)
{
    detail::StagedSourceAccessor<uint8> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint16> &values)
{
    detail::StagedSourceAccessor<uint16> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint32> &values)
{
    detail::StagedSourceAccessor<uint32> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint64> &values)
{
    detail::StagedSourceAccessor<uint64> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<float32> &values)
{
    detail::StagedSourceAccessor<float32> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<float64> &values)
{
    detail::StagedSourceAccessor<float64> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int8> &values)
{
    // DataArray sources share the same bulk-set implementation after being
    // wrapped in a temporary accessor with matching dtype metadata.
    detail::StagedSourceAccessor<int8> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int16> &values)
{
    detail::StagedSourceAccessor<int16> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int32> &values)
{
    detail::StagedSourceAccessor<int32> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int64> &values)
{
    detail::StagedSourceAccessor<int64> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint8> &values)
{
    detail::StagedSourceAccessor<uint8> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint16> &values)
{
    detail::StagedSourceAccessor<uint16> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint32> &values)
{
    detail::StagedSourceAccessor<uint32> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint64> &values)
{
    detail::StagedSourceAccessor<uint64> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<float32> &values)
{
    detail::StagedSourceAccessor<float32> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<float64> &values)
{
    detail::StagedSourceAccessor<float64> staged =
        detail::make_staged_source_accessor(values);
    detail::set_values_from_accessor_helper(*this,
                                            staged.accessor,
                                            dtype().number_of_elements());
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
    ltemplate class DataAccessor<long double>;
#endif


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
