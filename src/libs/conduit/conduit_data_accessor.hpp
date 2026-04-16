// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_accessor.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_DATA_ACCESSOR_HPP
#define CONDUIT_DATA_ACCESSOR_HPP


//-----------------------------------------------------------------------------
// -- conduit  includes -- 
//-----------------------------------------------------------------------------
#include "conduit_execution_decorators.hpp"
#include "conduit_core.hpp"
#include "conduit_data_type.hpp"
#include "conduit_memory_manager.hpp"
#include "conduit_utils.hpp"

#include <type_traits>


//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- forward declarations required for conduit::DataAccessor --
//-----------------------------------------------------------------------------
class Node;
template <typename T>
class DataArray;
namespace execution
{
    class ExecutionPolicy;
}

//-----------------------------------------------------------------------------
// -- begin conduit::DataArray --
//-----------------------------------------------------------------------------
///
/// class: conduit::DataAccessor
///
/// description:
///  Helps consume array data as desired type with on the fly conversion and 
///  supports memory movement between host and device.
///
//-----------------------------------------------------------------------------
template <typename T> 
class CONDUIT_API DataAccessor
{
public:
    using value_type = T;
//-----------------------------------------------------------------------------
//
// -- conduit::DataAccessor public methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Construction and Destruction
//-----------------------------------------------------------------------------
        /// Default constructor
        DataAccessor();
        ///
        /// This copy constructor must remain inline in the header because a
        /// DataAccessor is commonly captured by value into device lambdas.
        /// Device compilation needs to see the copy operation.
        ///
        /// Copy constructor
        CONDUIT_EXEC_HOST_DEVICE DataAccessor(const DataAccessor<T> &accessor)
        : m_data(accessor.m_data),
          m_orig_data_ptr(accessor.m_orig_data_ptr),
          m_dtype(accessor.m_dtype),
          m_node_ptr(accessor.m_node_ptr),
          m_other_ptr(accessor.m_other_ptr),
          m_other_dtype(accessor.m_other_dtype),
          m_do_i_own_it(false),
          m_offset(accessor.m_offset),
          m_stride(accessor.m_stride)
        {}
        /// Access a pointer to raw data according to dtype description.
        DataAccessor(void *data, const DataType &dtype);
        /// Access a const pointer to raw data according to dtype description.
        DataAccessor(const void *data, const DataType &dtype);
        /// Access a pointer to node data according to node dtype description.
        DataAccessor(Node &node);
        // /// Access a const pointer to node data according to node dtype description.
        DataAccessor(const Node &node);
        /// Access a pointer to node data according to node dtype description.
        DataAccessor(Node *node);
        /// Access a const pointer to node data according to node dtype description.
        DataAccessor(const Node *node);
        ///
        /// This destructor must remain inline in the header because accessors
        /// may be materialized during device compilation. The device path is a
        /// no-op while the host path preserves ownership cleanup.
        ///
        /// Destructor.
        CONDUIT_EXEC_HOST_DEVICE ~DataAccessor()
        {
#if !defined(CONDUIT_EXEC_DEVICE_COMPILE)
            if (m_do_i_own_it)
            {
                if (execution::DeviceMemory::is_device_ptr(m_other_ptr))
                {
                    execution::DeviceMemory::deallocate(m_other_ptr);
                }
                else
                {
                    execution::HostMemory::deallocate(m_other_ptr);
                }
            }
#endif
        }

    ///
    /// Summary Stats Helpers
    ///
    T               min()  const;
    T               max()  const;
    T               sum()  const;
    float64         mean() const;
    
    /// counts number of occurrences of given value
    index_t         count(T value) const;

    ///
    /// This assignment operator must remain inline in the header because
    /// accessors may be copied and assigned while preparing captures for
    /// device lambdas.
    ///
    /// Assignment operator
    CONDUIT_EXEC_HOST_DEVICE DataAccessor<T> &operator=(const DataAccessor<T> &accessor)
    {
        if(this != &accessor)
        {
            m_data  = accessor.m_data;
            m_orig_data_ptr = accessor.m_orig_data_ptr;
            m_dtype = accessor.m_dtype;
            m_node_ptr = accessor.m_node_ptr;
            m_other_ptr = accessor.m_other_ptr;
            m_other_dtype = accessor.m_other_dtype;
            m_do_i_own_it = false;
            m_offset = accessor.m_offset;
            m_stride = accessor.m_stride;
        }
        return *this;
    }

//-----------------------------------------------------------------------------
// Data and Info Access
//-----------------------------------------------------------------------------
    ///
    /// These inline methods form the minimal device-usable slice of
    /// DataAccessor. Kernels use them to read values, write values, and walk
    /// array layout, so device compilation must see the definitions here in
    /// the header.
    ///
    CONDUIT_EXEC_HOST_DEVICE T operator[](index_t idx) const
                    {return element(idx);}

    CONDUIT_EXEC_HOST_DEVICE T element(index_t idx) const
    {
        switch(dtype().id())
        {
            case DataType::INT8_ID:
                return (T)(*(int8*)(element_ptr(idx)));
            case DataType::INT16_ID:
                return (T)(*(int16*)(element_ptr(idx)));
            case DataType::INT32_ID:
                return (T)(*(int32*)(element_ptr(idx)));
            case DataType::INT64_ID:
                return (T)(*(int64*)(element_ptr(idx)));
            case DataType::UINT8_ID:
                return (T)(*(uint8*)(element_ptr(idx)));
            case DataType::UINT16_ID:
                return (T)(*(uint16*)(element_ptr(idx)));
            case DataType::UINT32_ID:
                return (T)(*(uint32*)(element_ptr(idx)));
            case DataType::UINT64_ID:
                return (T)(*(uint64*)(element_ptr(idx)));
            case DataType::FLOAT32_ID:
                return (T)(*(float32*)(element_ptr(idx)));
            case DataType::FLOAT64_ID:
                return (T)(*(float64*)(element_ptr(idx)));
            default:
            {
#if !defined(CONDUIT_EXEC_DEVICE_COMPILE)
                CONDUIT_ERROR("DataAccessor does not support dtype: "
                              << dtype().name());
#endif
                return (T)0;
            }
        }
    }

    // Restrict the scalar setter to non-pointer value types so calls like
    // set(0, value) do not collide with the bulk pointer setter below.
    template <typename U = T>
    CONDUIT_EXEC_HOST_DEVICE
    typename std::enable_if<!std::is_pointer<U>::value, void>::type
                    set(index_t idx, T value) const
    {
        switch(dtype().id())
        {
            case DataType::INT8_ID:
            {
                (*(int8*)(element_ptr(idx))) = static_cast<int8>(value);
                break;
            }
            case DataType::INT16_ID:
            {
                (*(int16*)(element_ptr(idx))) = static_cast<int16>(value);
                break;
            }
            case DataType::INT32_ID:
            {
                (*(int32*)(element_ptr(idx))) = static_cast<int32>(value);
                break;
            }
            case DataType::INT64_ID:
            {
                (*(int64*)(element_ptr(idx))) = static_cast<int64>(value);
                break;
            }
            case DataType::UINT8_ID:
            {
                (*(uint8*)(element_ptr(idx))) = static_cast<uint8>(value);
                break;
            }
            case DataType::UINT16_ID:
            {
                (*(uint16*)(element_ptr(idx))) = static_cast<uint16>(value);
                break;
            }
            case DataType::UINT32_ID:
            {
                (*(uint32*)(element_ptr(idx))) = static_cast<uint32>(value);
                break;
            }
            case DataType::UINT64_ID:
            {
                (*(uint64*)(element_ptr(idx))) = static_cast<uint64>(value);
                break;
            }
            case DataType::FLOAT32_ID:
            {
                (*(float32*)(element_ptr(idx))) = static_cast<float32>(value);
                break;
            }
            case DataType::FLOAT64_ID:
            {
                (*(float64*)(element_ptr(idx))) = static_cast<float64>(value);
                break;
            }
            default:
            {
#if !defined(CONDUIT_EXEC_DEVICE_COMPILE)
                CONDUIT_ERROR("DataAccessor does not support dtype: "
                              << dtype().name());
#endif
            }
        }
    }

    // Restrict the bulk setter to pointer-valued accessors.
    template <typename U = T>
    typename std::enable_if<std::is_pointer<U>::value, void>::type
                    set(const T* values, index_t num_elements) const;

    // void            set(const std::vector<typename std::remove_pointer<T>::type> &values)
    //                     { set(values.data(), values.size()); }


    void            fill(T value);

    CONDUIT_EXEC_HOST_DEVICE const void *element_ptr(index_t idx) const
                    {
                         return static_cast<const char*>(m_data) +
                                  dtype().element_index(idx);
                    }

    CONDUIT_EXEC_HOST_DEVICE index_t number_of_elements() const
                        {return dtype().number_of_elements();}

    ///
    /// dtype metadata is cached in the accessor so device code can choose
    /// between the original and migrated layout without dereferencing Node.
    /// This logic must stay inline in the header for device compilation.
    ///
    CONDUIT_EXEC_HOST_DEVICE const DataType &dtype() const
    {
        if (nullptr != m_node_ptr)
        {
            return (m_data == m_orig_data_ptr)
                   ? orig_dtype()
                   : other_dtype();
        }
        else
        {
            return m_dtype;
        }
    }

    ///
    /// These methods are part of the cached dtype metadata used by device
    /// code, so they must remain inline in the header alongside dtype().
    ///
    CONDUIT_EXEC_HOST_DEVICE const DataType &orig_dtype() const
                    { return m_dtype; }

    CONDUIT_EXEC_HOST_DEVICE const DataType &other_dtype() const
                    { return nullptr != m_node_ptr ? m_other_dtype : m_dtype; }

//-----------------------------------------------------------------------------
// Data movement
//-----------------------------------------------------------------------------
    void                                use_with(conduit::execution::ExecutionPolicy policy);

    void                                sync();

    void                                assume();

    conduit::execution::ExecutionPolicy active_space();

//-----------------------------------------------------------------------------
// Setters
//-----------------------------------------------------------------------------
    /// signed integer arrays via DataArray
    void            set(const DataArray<int8>    &values);
    void            set(const DataArray<int16>   &values);
    void            set(const DataArray<int32>   &values);
    void            set(const DataArray<int64>   &values);

    /// unsigned integer arrays via DataArray
    void            set(const DataArray<uint8>   &values);
    void            set(const DataArray<uint16>  &values);
    void            set(const DataArray<uint32>  &values);
    void            set(const DataArray<uint64>  &values);
    
    /// floating point arrays via DataArray
    void            set(const DataArray<float32>  &values);
    void            set(const DataArray<float64>  &values);

    /// signed integer arrays via DataAccessor
    void            set(const DataAccessor<int8>    &values);
    void            set(const DataAccessor<int16>   &values);
    void            set(const DataAccessor<int32>   &values);
    void            set(const DataAccessor<int64>   &values);

    /// unsigned integer arrays via DataAccessor
    void            set(const DataAccessor<uint8>   &values);
    void            set(const DataAccessor<uint16>  &values);
    void            set(const DataAccessor<uint32>  &values);
    void            set(const DataAccessor<uint64>  &values);
    
    /// floating point arrays via DataAccessor
    void            set(const DataAccessor<float32>  &values);
    void            set(const DataAccessor<float64>  &values);

//-----------------------------------------------------------------------------
// Transforms
//-----------------------------------------------------------------------------
    std::string     to_string(const std::string &protocol="json") const;
    void            to_string_stream(std::ostream &os,
                                     const std::string &protocol="json") const;

    // NOTE(cyrush): The primary reason this function exists is to enable
    // easier compatibility with debugging tools (e.g. totalview, gdb) that
    // have difficulty allocating default string parameters.
    std::string     to_string_default() const;

    std::string     to_json() const;
    void            to_json_stream(std::ostream &os) const;

    std::string     to_yaml() const;
    void            to_yaml_stream(std::ostream &os) const;

    /// Creates a string repression for printing that limits
    /// the number of elements shown to a max number
    std::string     to_summary_string_default() const;
    std::string     to_summary_string(index_t threshold=5) const;
    void            to_summary_string_stream(std::ostream &os,
                                             index_t threshold=5) const;

//-----------------------------------------------------------------------------
// -- stdout print methods ---
//-----------------------------------------------------------------------------
    /// print a simplified json representation of the this node to std out
    void            print() const
                      {std::cout << to_summary_string() << std::endl;}

private:

//-----------------------------------------------------------------------------
//
// -- conduit::DataAccessor private data members --
//
//-----------------------------------------------------------------------------
    /// holds data (always external, never allocated)
    void           *m_data;
    /// holds original wrapped data pointer
    void           *m_orig_data_ptr;
    /// holds data description
    DataType        m_dtype;

    Node           *m_node_ptr;

    /// holds data
    void           *m_other_ptr;
    /// holds data description
    DataType        m_other_dtype;
    
    bool            m_do_i_own_it;

    index_t         m_offset;
    index_t         m_stride;
    
};
//-----------------------------------------------------------------------------
// -- end conduit::DataAccessor --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- conduit::DataAccessor typedefs for supported types --
//
//-----------------------------------------------------------------------------

/// Note: these are also the types we explicitly instantiate.

/// signed integer arrays
typedef DataAccessor<int8>     int8_accessor;
typedef DataAccessor<int16>    int16_accessor;
typedef DataAccessor<int32>    int32_accessor;
typedef DataAccessor<int64>    int64_accessor;

/// unsigned integer arrays
typedef DataAccessor<uint8>    uint8_accessor;
typedef DataAccessor<uint16>   uint16_accessor;
typedef DataAccessor<uint32>   uint32_accessor;
typedef DataAccessor<uint64>   uint64_accessor;

/// floating point arrays
typedef DataAccessor<float32>  float32_accessor;
typedef DataAccessor<float64>  float64_accessor;

/// index type arrays
typedef DataAccessor<index_t>  index_t_accessor;

/// native c types arrays
typedef DataAccessor<char>       char_accessor;
typedef DataAccessor<short>      short_accessor;
typedef DataAccessor<int>        int_accessor;
typedef DataAccessor<long>       long_accessor;
#ifdef CONDUIT_HAS_LONG_LONG
typedef DataAccessor<long long>  long_long_accessor;
#endif


/// signed integer arrays
typedef DataAccessor<signed char>       signed_char_accessor;
typedef DataAccessor<signed short>      signed_short_accessor;
typedef DataAccessor<signed int>        signed_int_accessor;
typedef DataAccessor<signed long>       signed_long_accessor;
#ifdef CONDUIT_HAS_LONG_LONG
typedef DataAccessor<signed long long>  signed_long_long_accessor;
#endif


/// unsigned integer arrays
typedef DataAccessor<unsigned char>   unsigned_char_accessor;
typedef DataAccessor<unsigned short>  unsigned_short_accessor;
typedef DataAccessor<unsigned int>    unsigned_int_accessor;
typedef DataAccessor<unsigned long>   unsigned_long_accessor;
#ifdef CONDUIT_HAS_LONG_LONG
typedef DataAccessor<unsigned long long>  unsigned_long_long_accessor;
#endif


/// floating point arrays
typedef DataAccessor<float>   float_accessor;
typedef DataAccessor<double>  double_accessor;
#ifdef CONDUIT_USE_LONG_DOUBLE
typedef DataAccessor<long double>  long_double_accessor;
#endif

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
