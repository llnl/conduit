// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_fixed_size_vector.hpp
///
//-----------------------------------------------------------------------------
#ifndef CONDUIT_FIXED_SIZE_VECTOR_HPP
#define CONDUIT_FIXED_SIZE_VECTOR_HPP

#include <algorithm>
#include <array>
#include <vector>

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

/*!
 * @brief This class represents a fixed-sized vector with no dynamically allocated memory.
 *
 * @tparam T The type to store in the vector.
 * @tparam N The max number of elements in the vector.
 */
template <typename T, int N>
class fixed_size_vector
{
public:
    int size() const { return m_length; }
    void clear() { m_length = 0; }
    void resize(int sz) { m_length = std::min(sz, N); }
    void push_back(const T &value)
    {
        if(m_length < N)
        {
            m_data[m_length++] = value;
        }
    }
    T &operator[](int index) { return m_data[index]; }
    const T &operator[](int index) const { return m_data[index]; }
    T *begin() { return m_data.data(); }
    const T *cbegin() const { return m_data.data(); }
    T *end() { return begin() + N; }
    const T *cend() const { return cbegin() + N; }

private:
    std::array<T, N> m_data {};
    int m_length {0};
};

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
