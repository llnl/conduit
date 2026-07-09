// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_geometry_vector.hpp
///
//-----------------------------------------------------------------------------
#ifndef CONDUIT_GEOMETRY_VECTOR_HPP
#define CONDUIT_GEOMETRY_VECTOR_HPP

#include <array>
#include <cmath>

#include "conduit_execution_macros.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::geometry --
//-----------------------------------------------------------------------------
namespace geometry
{

/*!
 * @brief A simple math vector.
 */
template<typename T, size_t Size>
struct vector
{
    using this_type = vector<T, Size>;
    using data_type = std::array<T, Size>;
    using value_type = T;
private:
    // Used to alias vector data
    template<size_t Index>
    struct accessor
    {
        data_type data;

        constexpr CONDUIT_EXEC operator T() const
        {
            static_assert(Index < Size, "Invalid access into data.");
            return data[Index];
        }

        CONDUIT_EXEC T operator=(T v)
        {
            static_assert(Index < Size, "Invalid access into data.");
            return data[Index] = v;
        }

        CONDUIT_EXEC T operator +=(T v)
        {
            static_assert(Index < Size, "Invalid access into data.");
            data[Index] += v;
            return data[Index];
        }

        CONDUIT_EXEC T operator -=(T v)
        {
            static_assert(Index < Size, "Invalid access into data.");
            data[Index] -= v;
            return data[Index];
        }

        CONDUIT_EXEC T operator *=(T v)
        {
            static_assert(Index < Size, "Invalid access into data.");
            data[Index] *= v;
            return data[Index];
        }

        CONDUIT_EXEC T operator /=(T v)
        {
            static_assert(Index < Size, "Invalid access into data.");
            data[Index] /= v;
            return data[Index];
        }
    };

public:
    // Possible to access vector data with x/y/z
    union
    {
        data_type    v;
        accessor<0>  x;
        accessor<1>  y;
        accessor<2>  z;
    };

    CONDUIT_EXEC vector()
    {
        v = data_type{};
    }

    template <typename... S>
    CONDUIT_EXEC vector(S... args)
        : v{static_cast<T>(args)...}
    {
        static_assert(sizeof...(S) == Size, "Incorrect number of arguments for vector");
    }

    constexpr CONDUIT_EXEC size_t size() const
    {
        return Size;
    }

    CONDUIT_EXEC const T &operator[](size_t index) const
    {
        return v[index];
    }

    CONDUIT_EXEC T &operator[](size_t index)
    {
        return v[index];
    }

    CONDUIT_EXEC void zero()
    {
        set_all(0);
    }

    CONDUIT_EXEC void set_all(T val)
    {
        for(size_t i = 0u; i < size(); i++)
        {
            v[i] = val;
        }
    }

    CONDUIT_EXEC void copy(const this_type &other)
    {
        for(auto i = 0u; i < size(); i++)
            other.v[i] = v[i];
    }

    CONDUIT_EXEC bool operator <= (const this_type &other) const
    {
        bool retval = true;
        for(size_t i = 0u; i < size(); i++)
            retval &= v[i] <= other[i];
        return retval;
    }

    CONDUIT_EXEC bool operator >= (const this_type &other) const
    {
        bool retval = true;
        for(size_t i = 0u; i < size(); i++)
            retval &= v[i] >= other[i];
        return retval;
    }

    CONDUIT_EXEC this_type operator - () const
    {
        this_type retval;
        for(size_t i = 0u; i < size(); i++)
        {
            retval[i] = -v[i];
        }
        return retval;
    }   

    CONDUIT_EXEC this_type operator + (const this_type &other) const
    {
        this_type retval;
        for(size_t i = 0u; i < size(); i++)
        {
            retval[i] = v[i] + other.v[i];
        }
        return retval;
    }

    CONDUIT_EXEC this_type operator - (const this_type &other) const
    {
        this_type retval;
        for(size_t i = 0u; i < size(); i++)
        {
            retval[i] = v[i] - other.v[i];
        }
        return retval;
    }   

    CONDUIT_EXEC this_type operator + (T scalar) const
    {
        this_type retval;
        for(size_t i = 0u; i < size(); i++)
        {
            retval[i] = v[i] + scalar;
        }
        return retval;
    }

    CONDUIT_EXEC this_type operator - (T scalar) const
    {
        this_type retval;
        for(size_t i = 0u; i < size(); i++)
        {
            retval[i] = v[i] - scalar;
        }
        return retval;
    }

    CONDUIT_EXEC this_type operator * (T scalar) const
    {
        this_type retval;
        for(size_t i = 0u; i < size(); i++)
        {
            retval[i] = v[i] * scalar;
        }
        return retval;
    }

    CONDUIT_EXEC this_type operator / (T scalar) const
    {
        this_type retval;
        for(size_t i = 0u; i < size(); i++)
        {
            retval[i] = v[i] / scalar;
        }
        return retval;
    }

    CONDUIT_EXEC void operator += (const this_type &other)
    {
        for(size_t i = 0u; i < size(); i++)
        {
            v[i] += other.v[i];
        }
    }

    CONDUIT_EXEC void operator -= (const this_type &other)
    {
        for(size_t i = 0u; i < size(); i++)
        {
            v[i] -= other.v[i];
        }
    }

    CONDUIT_EXEC void operator *= (T scalar)
    {
        for(size_t i = 0u; i < size(); i++)
        {
            v[i] *= scalar;
        }
    }

    CONDUIT_EXEC void operator /= (T scalar)
    {
        for(size_t i = 0u; i < size(); i++)
        {
            v[i] /= scalar;
        }
    }

    CONDUIT_EXEC T norm() const
    {
        return static_cast<T>(sqrt(dot(*this)));
    }

    CONDUIT_EXEC this_type normalize()
    {
        auto n = norm();
        n = (n == 0.) ? T(1) : n;
        for(size_t i = 0u; i < size(); i++)
        {
            v[i] /= n;
        }
        return *this;
    }

    CONDUIT_EXEC T dot(const this_type &other) const
    {
        T sum{};
        for(size_t i = 0u; i < size(); i++)
        {
            sum += other[i] * v[i];
        }
        return sum;
    }

    // Treat the vectors as points and compute distance squared.
    CONDUIT_EXEC T distance2(const this_type &other) const
    {
        T d2{};
        for(size_t i = 0u; i < size(); i++)
        {
            const auto diff = other[i] - v[i];
            d2 += (diff * diff);
        }
        return d2;
    }

    // Treat the vectors as points and compute distance.
    CONDUIT_EXEC T distance(const this_type &other) const
    {
        return static_cast<T>(std::sqrt(distance2(other)));
    }

    template <int DIM = Size>
    CONDUIT_EXEC typename std::enable_if<DIM == 2, T>::type cross(const this_type &other) const
    {
        return x * other.y - y * other.x;
    }

    template <int DIM = Size>
    CONDUIT_EXEC typename std::enable_if<DIM == 3, this_type>::type cross(const this_type &other) const
    {
        return this_type(y * other.z - z * other.y,
                         z * other.x - x * other.z,
                         x * other.y - y * other.x);
    }
};

}
//-----------------------------------------------------------------------------
// -- end conduit::geometry --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
