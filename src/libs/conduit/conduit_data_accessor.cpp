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
#include "conduit_data_array.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
//
// -- conduit::DataAccessor public methods --
//
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor()
: m_data(NULL),
  m_dtype()
{}

//---------------------------------------------------------------------------//
template <typename T>
DataAccessor<T>::DataAccessor(const DataAccessor<T> &accessor)
: m_data(accessor.m_data),
  m_dtype(accessor.m_dtype)
{}


//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(void *data, const DataType &dtype)
: m_data(data),
  m_dtype(dtype)
{}


//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::DataAccessor(const void *data, const DataType &dtype)
: m_data(const_cast<void*>(data)),
  m_dtype(dtype)
{}

//---------------------------------------------------------------------------//
template <typename T> 
DataAccessor<T>::~DataAccessor()
{} // all data is external


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
DataAccessor<T> &
DataAccessor<T>::operator=(const DataAccessor<T> &accessor)
{
    if(this != &accessor)
    {
        m_data  = accessor.m_data;
        m_dtype = accessor.m_dtype;
    }
    return *this;
}

//---------------------------------------------------------------------------//
template <typename T> 
T
DataAccessor<T>::element(index_t idx) const
{
    switch(m_dtype.id())
    {
        // ints
        case DataType::INT8_ID:
            return (T)(*(int8*)(element_ptr(idx)));
        case DataType::INT16_ID: 
            return (T)(*(int16*)(element_ptr(idx)));
        case DataType::INT32_ID:
            return (T)(*(int32*)(element_ptr(idx)));
        case DataType::INT64_ID:
            return (T)(*(int64*)(element_ptr(idx)));
        // uints
        case DataType::UINT8_ID:
            return (T)(*(uint8*)(element_ptr(idx)));
        case DataType::UINT16_ID:
            return (T)(*(uint16*)(element_ptr(idx)));
        case DataType::UINT32_ID:
            return (T)(*(uint32*)(element_ptr(idx)));
        case DataType::UINT64_ID:
            return (T)(*(uint64*)(element_ptr(idx)));
        // floats 
        case DataType::FLOAT32_ID:
            return (T)(*(float32*)(element_ptr(idx)));
        case DataType::FLOAT64_ID:
            return (T)(*(float64*)(element_ptr(idx)));
    }

    // error
    CONDUIT_ERROR("DataAccessor does not support dtype: "
                  << m_dtype.name());
    return (T)0;
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(index_t idx, T value)
{
    switch(m_dtype.id())
    {
        // ints
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
        // uints
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
        // floats
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
            // error
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << m_dtype.name());
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::fill(T value)
{
    switch(m_dtype.id())
    {
        // ints
        case DataType::INT8_ID:
        {
            int8 v = static_cast<int8>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(int8*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::INT16_ID:
        {
            int16 v = static_cast<int16>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(int16*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::INT32_ID:
        {
            int32 v = static_cast<int32>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(int32*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::INT64_ID:
        {
            int64 v = static_cast<int64>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(int64*)(element_ptr(i))) = v;
            }
            break;
        }
        // uints
        case DataType::UINT8_ID:
        {
            uint8 v = static_cast<uint8>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(uint8*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::UINT16_ID:
        {
            uint16 v = static_cast<uint16>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(uint16*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::UINT32_ID:
        {
            uint32 v = static_cast<uint32>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(uint32*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::UINT64_ID:
        {
            uint64 v = static_cast<uint64>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(uint64*)(element_ptr(i))) = v;
            }
            break;
        }
        // floats
        case DataType::FLOAT32_ID:
        {
            float32 v = static_cast<float32>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(float32*)(element_ptr(i))) = v;
            }
            break;
        }
        case DataType::FLOAT64_ID:
        {
            float64 v = static_cast<float64>(value);
            for(index_t i=0;i < m_dtype.number_of_elements(); i++)
            {
                 (*(float64*)(element_ptr(i))) = v;
            }
            break;
        }
        default:
            // error
            CONDUIT_ERROR("DataAccessor does not support dtype: "
                          << m_dtype.name());
    }
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
DataAccessor<T>::set(const DataAccessor<int8> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int16> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int32> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<int64> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
// Set from DataAccessor unsigned integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint8> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint16> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint32> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<uint64> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
// Set from DataAccessor floating point
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<float32> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataAccessor<float64> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
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
DataAccessor<T>::set(const DataArray<int8> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int16> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int32> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<int64> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
// Set from DataArray unsigned integers
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint8> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint16> &values)
{ 
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint32> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<uint64> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
// Set from DataArray floating point
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<float32> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
DataAccessor<T>::set(const DataArray<float64> &values)
{
    index_t num_elems = dtype().number_of_elements();
    for(index_t i=0; i <num_elems; i++)
    {
        this->set(i, (T)values[i]);
    }
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

