// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_error.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_error.hpp"

//-----------------------------------------------------------------------------
// -- conduit includes -- 
//-----------------------------------------------------------------------------
#include "conduit_node.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
//
// -- conduit::Error public members --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Error Construction and Destruction
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
Error::Error()
:m_msg(""),
 m_file(""),
 m_line(0),
 m_what("")
{
    m_what = message();
}

//---------------------------------------------------------------------------//
Error::Error(const Error &err)
:m_msg(err.m_msg),
 m_file(err.m_file),
 m_line(err.m_line),
 m_what("")
{
    m_what = message();
}

//---------------------------------------------------------------------------//
Error &
Error::operator=(const Error &err)
{
    if(this != &err)
    {
        m_msg  = err.m_msg;
        m_file = err.m_file;
        m_line = err.m_line;
        m_what = err.m_what;
    }
    return *this;
}

//---------------------------------------------------------------------------//
Error::Error(const std::string &msg,
             const std::string &file,
             index_t line)
:m_msg(msg),
 m_file(file),
 m_line(line),
 m_what("")
{
    m_what = message();
}

//---------------------------------------------------------------------------//
Error::~Error() throw()
{
    //empty
}

//-----------------------------------------------------------------------------
// Methods used to access and display Error as a human readable string. 
//-----------------------------------------------------------------------------


//---------------------------------------------------------------------------//
void
Error::print() const
{
    std::cout << message() << std::endl;
}


//---------------------------------------------------------------------------//
std::string
Error::message() const
{
    std::ostringstream oss;
    message(oss);
    return oss.str();
}

//---------------------------------------------------------------------------//
void
Error::message(std::ostringstream &oss) const
{
    oss << std::endl;
    oss << "file: " << m_file << std::endl;
    oss << "line: " << m_line << std::endl;
    oss << "message: " << std::endl;
    oss << m_msg << std::endl;
}

//---------------------------------------------------------------------------//
const char* 
Error::what() const throw()
{
    return m_what.c_str(); 
}


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

