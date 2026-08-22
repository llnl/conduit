// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_array.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_data_array.hpp"

//-----------------------------------------------------------------------------
// -- standard includes -- 
//-----------------------------------------------------------------------------
#include <algorithm>
#include <cstring>
#include <limits>


//-----------------------------------------------------------------------------
// -- conduit includes -- 
//-----------------------------------------------------------------------------
#include "conduit_node.hpp"
#include "conduit_utils.hpp"
#include "conduit_log.hpp"
#include "conduit_data_accessor.hpp"
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
array_fill_kernel(execution::ExecutionPolicy &policy,
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
struct FillArray
{
    DataArray<T> vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.element(i) = value;
    }
};

//-----------------------------------------------------------------------------
template <typename T>
void
array_fill_kernel(execution::ExecutionPolicy &policy,
                  index_t num_elements,
                  const DataArray<T> vals,
                  const T value)
{
    // Small host arrays are faster to fill without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        FillArray<T> kernel{vals, value};
        for (index_t i = 0; i < num_elements; i++)
        {
            kernel(i);
        }
        return;
    }

    execution::forall(policy, 0, num_elements, FillArray<T>{vals, value});
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
array_copy_from_view_kernel(execution::ExecutionPolicy &policy,
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
struct CopyFromViewArray
{
    DataArray<T> vals;
    U src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.element(i) = static_cast<T>(src[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
array_copy_from_view_kernel(execution::ExecutionPolicy &policy,
                            index_t num_elements,
                            const U src,
                            const DataArray<T> vals)
{
    // Small host arrays are faster to copy without a forall
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        CopyFromViewArray<T, U> kernel{vals, src};
        for (index_t i = 0; i < num_elements; i++)
        {
            kernel(i);
        }
        return;
    }

    execution::forall(policy, 0, num_elements, CopyFromViewArray<T, U>{vals, src});
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
array_min_kernel(execution::ExecutionPolicy &policy,
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
array_max_kernel(execution::ExecutionPolicy &policy,
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
array_sum_kernel(execution::ExecutionPolicy &policy,
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
array_count_kernel(execution::ExecutionPolicy &policy,
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
template <typename U, typename T>
void
fill_value_helper(const DataArray<T> &array,
                  U value)
{
    const index_t num_elements = array.number_of_elements();
    execution::ExecutionPolicy policy = array.active_space();
    const T val = static_cast<T>(value);

    execution::dispatch(array, [&](auto vals)
    {
        array_fill_kernel(policy, num_elements, vals, val);
    });
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataArray<T> &array,
                  const U *values,
                  index_t num_elements)
{
    // Avoid performing unnecessary work for empty arrays
    if (num_elements <= 0)
    {
        return;
    }

    execution::ExecutionPolicy policy = array.active_space();

    const bool dst_on_device = execution::DeviceMemory::is_device_ptr(array.element_ptr(0));
    const bool src_on_device = execution::DeviceMemory::is_device_ptr(values);

    if (dst_on_device == src_on_device)
    {
        execution::dispatch(array, [&](auto vals)
        {
            array_copy_from_view_kernel(policy, num_elements, values, vals);
        });
    }
    else // dst and src are in different memory spaces
    {
        CONDUIT_ERROR("DataArray::set() requires the source and destination to "
                      "share a memory space. Use use_with() and sync() first.");
    }
}

//-----------------------------------------------------------------------------
template <typename U, template <typename> class View, typename T>
void
set_values_view_helper(const DataArray<T> &array,
                       const View<U> &values,
                       index_t num_elements)
{
    // Avoid performing unnecessary work for empty arrays
    if (num_elements <= 0)
    {
        return;
    }

    execution::ExecutionPolicy policy = array.active_space();

    const bool dst_on_device = execution::DeviceMemory::is_device_ptr(array.element_ptr(0));
    const bool src_on_device = execution::DeviceMemory::is_device_ptr(values.element_ptr(0));

    if (dst_on_device == src_on_device)
    {
        execution::dispatch(array, [&](auto vals)
        {
            array_copy_from_view_kernel(policy, num_elements, values, vals);
        });
    }
    else // dst and src are in different memory spaces
    {
        CONDUIT_ERROR("DataArray::set() requires the source and destination to "
                      "share a memory space. Use use_with() and sync() first.");
    }
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataArray<T> &array,
                  const DataArray<U> &values,
                  index_t num_elements)
{
    set_values_view_helper(array, values, num_elements);
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
void
set_values_helper(const DataArray<T> &array,
                  const DataAccessor<U> &values,
                  index_t num_elements)
{
    set_values_view_helper(array, values, num_elements);
}
}
//-----------------------------------------------------------------------------
// -- end conduit::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- conduit::DataArray public methods --
//
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
template <typename T> 
DataArray<T>::DataArray()
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
DataArray<T>::DataArray(void *data, const DataType &dtype)
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
DataArray<T>::DataArray(const void *data, const DataType &dtype)
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
DataArray<T>::DataArray(Node &node)
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
DataArray<T>::DataArray(const Node &node)
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
DataArray<T>::DataArray(Node *node)
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
DataArray<T>::DataArray(const Node *node)
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
template <typename T> 
bool
DataArray<T>::compatible(const DataArray<T> &array) const 
{ 
    return dtype().compatible(array.dtype());
}

//---------------------------------------------------------------------------//
template <typename T> 
bool
DataArray<T>::diff(const DataArray<T> &array, Node &info, const float64 epsilon) const 
{ 
    const std::string protocol = "data_array::diff";
    bool res = false;
    info.reset();

    index_t t_nelems = number_of_elements();
    index_t o_nelems = array.number_of_elements();


    if(dtype().is_char8_str())
    {
        // since char8_str are null termed strings
        // diff should obey those semantics
        //
        // buffers may not always be 100% equal

        const char *t_data = NULL;
        const char *o_data = NULL;

        uint8 *t_compact_data = NULL;
        uint8 *o_compact_data = NULL;

        // conduit stores char8_strs with null terms
        // if the data array size is 0, we have a empty
        // case that is distinct from an empty string.
        if(t_nelems > 0)
        {
            if(dtype().is_compact())
            {
                t_data = (const char*)element_ptr(0);
            }
            else
            {
                t_compact_data = new uint8[(size_t)dtype().bytes_compact()];
                compact_elements_to(t_compact_data);
                t_data = (const char*)t_compact_data;
            }
        }

        if(o_nelems > 0)
        {
            if(array.dtype().is_compact())
            {
                o_data = (const char*)array.element_ptr(0);
            }
            else
            {
                o_compact_data = new uint8[(size_t)array.dtype().bytes_compact()];
                array.compact_elements_to(o_compact_data);
                o_data = (const char*)o_compact_data;
            }
        }

        // for debugging:
        // if(t_data)
        //     std::cout << "t_data " << t_data <<std::endl;
        // if(o_data)
        //     std::cout << "o_data " << o_data <<std::endl;

        if(t_nelems == 0 && o_nelems == 0)
        {
            // ok, both are empty buffers
        }
        // t_data is null, array is len 0
        else if( t_nelems == 0 )
        {
            std::ostringstream oss;
            oss << "data string mismatch ("
                << " [empty buffer] "
                << " vs "
                << "\"" << o_data << "\""
                << ")";
            utils::log::error(info, protocol, oss.str());
            res = true;
        }
        // o_data is null, array is len 0
        else if( o_nelems == 0)
        {
            std::ostringstream oss;
            oss << "data string mismatch ("
                << "\"" << t_data << "\""
                << " vs "
                << " [empty buffer] "
                << ")";
            utils::log::error(info, protocol, oss.str());
            res = true;
        }
        // all other cases use strcmp
        else if(strcmp(t_data,o_data) != 0)
        {
            std::ostringstream oss;
            oss << "data string mismatch ("
                << "\"" << t_data << "\""
                << " vs "
                << "\"" << o_data << "\""
                << ")";
            utils::log::error(info, protocol, oss.str());
            res = true;
        }

        if(t_compact_data)
        {
            delete [] t_compact_data;
        }

        if(o_compact_data)
        {
            delete [] o_compact_data;
        }
    }
    else if(t_nelems != o_nelems)
    {
        std::ostringstream oss;
        oss << "data length mismatch ("
            << t_nelems
            << " vs "
            << o_nelems
            << ")";
        utils::log::error(info, protocol, oss.str());
        res = true;
    }
    else
    {
        Node &info_value = info["value"];
        info_value.set(DataType(array.dtype().id(), t_nelems));
        T* info_ptr = (T*)info_value.data_ptr();

        for(index_t i = 0; i < t_nelems; i++)
        {
            info_ptr[i] = (*this)[i] - array[i];
            if(dtype().is_floating_point())
            {
                res |= info_ptr[i] > epsilon || info_ptr[i] < -epsilon;
            }
            else
            {
                res |= (*this)[i] != array[i];
            }
        }

        if(res)
        {
            utils::log::error(info, protocol, "data item(s) mismatch; see 'value' section");
        }
    }

    utils::log::validation(info, !res);

    return res;
}

//---------------------------------------------------------------------------//
template <typename T> 
bool
DataArray<T>::diff_compatible(const DataArray<T> &array, Node &info, const float64 epsilon) const 
{ 
    const std::string protocol = "data_array::diff_compatible";
    bool res = false;
    info.reset();

    index_t t_nelems = number_of_elements();
    index_t o_nelems = array.number_of_elements();

    // if we have a string, look that compat string
    // is a substring that starts at the beginning
    // of the test string
    if(dtype().is_char8_str())
    {

        const char *t_data = NULL;
        const char *o_data = NULL;

        uint8 *t_compact_data = NULL;
        uint8 *o_compact_data = NULL;

        // conduit stores char8_strs with null terms
        // if the data array size is 0, we have a empty
        // case that is distinct from an empty string.
        if(t_nelems > 0)
        {
            if(dtype().is_compact())
            {
                t_data = (const char*)element_ptr(0);
            }
            else
            {
                t_compact_data = new uint8[(size_t)dtype().bytes_compact()];
                compact_elements_to(t_compact_data);
                t_data = (const char*)t_compact_data;
            }
        }

        if(o_nelems > 0)
        {
            if(array.dtype().is_compact())
            {
                o_data = (const char*)array.element_ptr(0);
            }
            else
            {
                o_compact_data = new uint8[(size_t)array.dtype().bytes_compact()];
                array.compact_elements_to(o_compact_data);
                o_data = (const char*)o_compact_data;
            }
        }

        // for debugging:
        // if(t_data)
        //     std::cout << "t_data " << t_data <<std::endl;
        // if(o_data)
        //     std::cout << "o_data " << o_data <<std::endl;

        if(t_nelems == 0 && o_nelems == 0)
        {
            // ok, both are empty buffers
        }
        // t_data is null, array is len 0
        else if( t_nelems == 0 )
        {
            std::ostringstream oss;
            oss << "data string mismatch ("
                << " [empty buffer] "
                << " vs "
                << "\"" << o_data << "\""
                << ")";
            utils::log::error(info, protocol, oss.str());
            res = true;
        }
        // o_data is null, array is len 0
        else if( o_nelems == 0)
        {
            std::ostringstream oss;
            oss << "data string mismatch ("
                << "\"" << t_data << "\""
                << " vs "
                << " [empty buffer] "
                << ")";
            utils::log::error(info, protocol, oss.str());
            res = true;
        }
        // standard compat size check
        // (t_data len must be less than o_data len)
        else if(strlen(t_data) > strlen(o_data))
        {
            std::ostringstream oss;
            oss << "arg string length incompatible ("
                << t_nelems
                << " vs "
                << o_nelems
                << ")";
            utils::log::error(info, protocol, oss.str());
            res = true;
        }
        // all other cases use strstr
        else
        {
            // check that t_data is a substr of
            // o_data, and that substr 
            // starts at the beginning of o_data
            const char *found = strstr(o_data,t_data);
            // the substr should be found at the start
            // of o_data
            if(found != o_data)
            {
                std::ostringstream oss;
                oss << "data string mismatch ("
                    << "\"" << t_data << "\""
                    << " vs "
                    << "\"" << o_data << "\""
                    << ")";
                utils::log::error(info, protocol, oss.str());
                res = true;
            }
        }

        if(t_compact_data)
        {
            delete [] t_compact_data;
        }

        if(o_compact_data)
        {
            delete [] o_compact_data;
        }
    }
    else if(t_nelems > o_nelems)
    {
        std::ostringstream oss;
        oss << "arg data length incompatible ("
            << t_nelems
            << " vs "
            << o_nelems
            << ")";
        utils::log::error(info, protocol, oss.str());
        res = true;
    }
    else
    {
        Node &info_value = info["value"];
        info_value.set(DataType(array.dtype().id(), t_nelems));
        T* info_ptr = (T*)info_value.data_ptr();

        for(index_t i = 0; i < t_nelems; i++)
        {
            info_ptr[i] = (*this)[i] - array[i];
            if(dtype().is_floating_point())
            {
                res |= info_ptr[i] > epsilon || info_ptr[i] < -epsilon;
            }
            else
            {
                res |= (*this)[i] != array[i];
            }
        }

        if(res)
        {
            utils::log::error(info, protocol, "data item(s) mismatch; see diff below");
        }
    }

    utils::log::validation(info, !res);

    return res;
}

//---------------------------------------------------------------------------// 
///
/// Summary Stats Helpers
///
//---------------------------------------------------------------------------// 

//---------------------------------------------------------------------------// 
template <typename T>
T
DataArray<T>::min()  const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = active_space();

    T res = std::numeric_limits<T>::max();
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::array_min_kernel<T>(policy, num_elements, vals);
    });

    return res;
}

//---------------------------------------------------------------------------// 
template <typename T>
T
DataArray<T>::max() const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = active_space();

    T res = std::numeric_limits<T>::lowest();
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::array_max_kernel<T>(policy, num_elements, vals);
    });

    return res;
}


//---------------------------------------------------------------------------// 
template <typename T>
T
DataArray<T>::sum() const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = active_space();

    T res = 0;
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::array_sum_kernel<T>(policy, num_elements, vals);
    });

    return res;
}

//---------------------------------------------------------------------------// 
template <typename T>
float64
DataArray<T>::mean() const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = active_space();

    float64 res = 0.0;
    execution::dispatch(*this, [&](auto vals)
    {
        // Accumulate in float64 for accuracy
        res = detail::array_sum_kernel<float64>(policy, num_elements, vals);
    });

    return res / static_cast<float64>(num_elements);
}

//---------------------------------------------------------------------------// 
template <typename T>
index_t
DataArray<T>::count(T val) const
{
    const index_t num_elements = number_of_elements();
    execution::ExecutionPolicy policy = active_space();

    index_t res = 0;
    execution::dispatch(*this, [&](auto vals)
    {
        res = detail::array_count_kernel<T>(policy, num_elements, vals, val);
    });

    return res;
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::use_with(conduit::execution::ExecutionPolicy policy)
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
                    "Using execution array in this way will result in a memory leak.");

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
                    "Using execution array in this way will result in a memory leak.");

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
                    "Using execution array in this way will result in a memory leak.");

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
                    "Using execution array in this way will result in a memory leak.");

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
DataArray<T>::sync()
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
DataArray<T>::assume()
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
            "Using execution array in this way will result in a memory leak.");

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

        // the assumed data is now the array's new original backing storage
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
DataArray<T>::data_movement(const conduit::execution::SyncStrategy strategy)
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
DataArray<T>::active_space() const
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
template <typename T>
std::string 
DataArray<T>::to_string(const std::string &protocol) const
{
    std::ostringstream oss;
    to_string_stream(oss,protocol);
    return oss.str();
}

//---------------------------------------------------------------------------// 
template <typename T>
void
DataArray<T>::to_string_stream(std::ostream &os, 
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
DataArray<T>::to_string_default() const
{ 
    return to_string();
}


//---------------------------------------------------------------------------//
template <typename T> 
std::string
DataArray<T>::to_json() const 
{ 
    std::ostringstream oss;
    to_json_stream(oss);
    return oss.str(); 
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::to_json(std::ostream &os) const 
{ 
    to_json_stream(os);
}
//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::to_json_stream(std::ostream &os) const 
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
        switch(dtype().id())
        {
            // ints 
            case DataType::INT8_ID:
            case DataType::INT16_ID: 
            case DataType::INT32_ID:
            case DataType::INT64_ID:
            {
                 os << (int64) element(idx);
                 break;
            }
            // uints
            case DataType::UINT8_ID:
            case DataType::UINT16_ID:
            case DataType::UINT32_ID:
            case DataType::UINT64_ID:
            {
                os << (uint64) element(idx);
                break;
            }
            // floats 
            case DataType::FLOAT32_ID: 
            case DataType::FLOAT64_ID: 
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
                break;
            }
            default:
            {
                CONDUIT_ERROR("Leaf type \"" 
                              <<  dtype().name()
                              << "\"" 
                              << "is not supported in conduit::DataArray.")
            }
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
DataArray<T>::to_yaml() const 
{ 
    std::ostringstream oss;
    to_yaml_stream(oss);
    return oss.str(); 
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::to_yaml_stream(std::ostream &os) const 
{ 
    // yep, its the same as to_json_stream ...
    to_json_stream(os);;
}

//---------------------------------------------------------------------------//
// DataArray::set() signed integers single element
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const int8 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::set(const  int16 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const int32 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const  int64 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
// DataArray::set() unsigned integers multi element
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const  uint8 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const  uint16 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const uint32 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const uint64 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
// DataArray::set() floating point multi element
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const float32 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
template <typename T> 
void            
DataArray<T>::set(const float64 *values, index_t num_elements) const
{
    detail::set_values_helper(*this, values, num_elements);
}

//---------------------------------------------------------------------------//
// Set from std::initializer_list 
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<int8> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<int16> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<int32> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<int64> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<uint8> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<uint16> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<uint32> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<uint64> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<float32> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::set(const std::initializer_list<float64> &values) const
{
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}


//---------------------------------------------------------------------------//
// Set from std::initializer_list  c native gap methods
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<char> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_CHAR
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<signed char> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<unsigned char> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_CHAR
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_SHORT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<short> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<unsigned short> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_SHORT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_INT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<int> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<unsigned int> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_INT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_LONG
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<long> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<unsigned long> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_LONG
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<long long> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<unsigned long long> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_LONG_LONG
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_FLOAT
//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<float> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_FLOAT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_DOUBLE
//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const std::initializer_list<double> &values) const
{ 
    // set up to the number of elements of this array
    const index_t num_elems = std::min((index_t)values.size(),
                                       dtype().number_of_elements());
    detail::set_values_helper(*this, values.begin(), num_elems);
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_DOUBLE
//---------------------------------------------------------------------------//



//-----------------------------------------------------------------------------
// fill
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// signed integer fill
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::fill(int8 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
template <typename T> 
void
DataArray<T>::fill(int16 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
template <typename T> 
void
DataArray<T>::fill(int32 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
template <typename T> 
void
DataArray<T>::fill(int64 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
/// unsigned integer fill
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::fill(uint8 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
template <typename T> 
void
DataArray<T>::fill(uint16 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
template <typename T> 
void
DataArray<T>::fill(uint32 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
template <typename T> 
void
DataArray<T>::fill(uint64 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
/// floating point fill
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::fill(float32 value)
{
    detail::fill_value_helper(*this, value);
}

//-----------------------------------------------------------------------------
template <typename T> 
void
DataArray<T>::fill(float64 value)
{
    detail::fill_value_helper(*this, value);
}

//---------------------------------------------------------------------------//
// assign operator overloads for initializer_list
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<int8> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<int16> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<int32> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<int64> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<uint8> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<uint16> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<uint32> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<uint64> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<float32> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<float64> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
// Set from std::initializer_list  c native gap methods
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<char> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_CHAR
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<signed char> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<unsigned char> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_CHAR
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_SHORT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<short> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<unsigned short> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_SHORT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_INT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<int> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<unsigned int> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_INT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_LONG
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<long> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<unsigned long> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_LONG
//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<long long> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<unsigned long long> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_LONG_LONG
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_FLOAT
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<float> &values)
{
    set(values);
    return *this;
}


//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_FLOAT
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
#ifndef CONDUIT_USE_DOUBLE
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
template <typename T>
DataArray<T> &
DataArray<T>::operator=(const std::initializer_list<double> &values)
{
    set(values);
    return *this;
}

//---------------------------------------------------------------------------//
#endif // CONDUIT_USE_DOUBLE
//---------------------------------------------------------------------------//

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
DataArray<T>::set(const DataArray<int8> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<int16> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<int32> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<int64> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
// Set from DataArray unsigned integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<uint8> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<uint16> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<uint32> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<uint64> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
// Set from DataArray floating point
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<float32> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataArray<float64> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
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
DataArray<T>::set(const DataAccessor<int8> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void            
DataArray<T>::set(const DataAccessor<int16> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void            
DataArray<T>::set(const DataAccessor<int32> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void            
DataArray<T>::set(const DataAccessor<int64> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
// Set from DataAccessor unsigned integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataAccessor<uint8> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataAccessor<uint16> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataAccessor<uint32> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataArray<T>::set(const DataAccessor<uint64> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
// Set from DataAccessor floating point
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::set(const DataAccessor<float32> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::set(const DataAccessor<float64> &values) const
{
    detail::set_values_helper(*this, values, dtype().number_of_elements());
}

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::compact_elements_to(uint8 *data) const
{
    // copy all elements 
    index_t ele_bytes = DataType::default_bytes(dtype().id());

    utils::conduit_memcpy_strided_elements(data,   // dest
                    (size_t)dtype().number_of_elements(), // num eles to copy
                    ele_bytes,      // bytes per element
                    ele_bytes,      // dest stride per ele
                    element_ptr(0),             // src
                    (size_t)dtype().stride());  // src stride per ele
}

//---------------------------------------------------------------------------//
template <typename T> 
std::string
DataArray<T>::to_summary_string_default() const
{ 
    return to_summary_string();
}

//---------------------------------------------------------------------------//
template <typename T> 
std::string
DataArray<T>::to_summary_string(index_t threshold) const
{ 
    std::ostringstream oss;
    to_summary_string_stream(oss, threshold);
    return oss.str();
}

//---------------------------------------------------------------------------//
template <typename T> 
void
DataArray<T>::to_summary_string_stream(std::ostream &os,
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

            switch(dtype().id())
            {
                // ints
                case DataType::INT8_ID:
                case DataType::INT16_ID:
                case DataType::INT32_ID:
                case DataType::INT64_ID:
                {
                     os << (int64) element(idx);
                     break;
                }
                // uints
                case DataType::UINT8_ID:
                case DataType::UINT16_ID:
                case DataType::UINT32_ID:
                case DataType::UINT64_ID:
                {
                    os << (uint64) element(idx);
                    break;
                }
                // floats
                case DataType::FLOAT32_ID:
                case DataType::FLOAT64_ID:
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
                    break;
                }
                default:
                {
                    CONDUIT_ERROR("Leaf type \""
                                  <<  dtype().name()
                                  << "\""
                                  << "is not supported in conduit::DataArray.")
                }
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
// -- conduit::DataArray explicit instantiations for supported array types --
//
//-----------------------------------------------------------------------------
template class DataArray<int8>;
template class DataArray<int16>;
template class DataArray<int32>;
template class DataArray<int64>;

template class DataArray<uint8>;
template class DataArray<uint16>;
template class DataArray<uint32>;
template class DataArray<uint64>;

template class DataArray<float32>;
template class DataArray<float64>;

// gap template instantiations for c-native types

// we never use 'char' directly as a type,
// so we always need to inst the char case
template class DataArray<char>;

#ifndef CONDUIT_USE_CHAR
template class DataArray<signed char>;
template class DataArray<unsigned char>;
#endif

#ifndef CONDUIT_USE_SHORT
template class DataArray<signed short>;
template class DataArray<unsigned short>;
#endif

#ifndef CONDUIT_USE_INT
template class DataArray<signed int>;
template class DataArray<unsigned int>;
#endif

#ifndef CONDUIT_USE_LONG
template class DataArray<signed long>;
template class DataArray<unsigned long>;
#endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
template class DataArray<signed long long>;
template class DataArray<unsigned long long>;
#endif

#ifndef CONDUIT_USE_FLOAT
template class DataArray<float>;
#endif

#ifndef CONDUIT_USE_DOUBLE
template class DataArray<double>;
#endif

#ifdef CONDUIT_USE_LONG_DOUBLE
template class DataArray<long double>;
#endif

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
