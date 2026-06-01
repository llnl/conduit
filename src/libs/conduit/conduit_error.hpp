// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_error.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_ERROR_HPP
#define CONDUIT_ERROR_HPP

//-----------------------------------------------------------------------------
// -- standard lib includes -- 
//-----------------------------------------------------------------------------
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------
// -- conduit includes -- 
//-----------------------------------------------------------------------------
#include "conduit_core.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::Error --
//-----------------------------------------------------------------------------
///
/// class: conduit::Error
///
/// description:
///  Class used to record a runtime error in conduit.
///
//-----------------------------------------------------------------------------
class CONDUIT_API Error : public std::exception
{
public:    
//-----------------------------------------------------------------------------
//
// -- conduit::Error public members --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Construction and Destruction
//-----------------------------------------------------------------------------
    /// Default constructor
    Error();
    /// Copy constructor
    Error(const Error &err);
    /// Copy assignment operator
    Error &operator=(const Error &);
    /// Main constructor, used by CONDUIT_ERROR macro
    Error(const std::string &msg, 
          const std::string &file,
          index_t line);
    /// Destructor
    ~Error() throw();
    
//-----------------------------------------------------------------------------
// Methods used to access and display Error as a human readable string. 
//-----------------------------------------------------------------------------
    /// Print info about this error to stdout.
    void                print()   const;
    /// Return a human readable string that describes this error.
    std::string         message() const;
    /// Writes a human readable string that describes this error to the 
    /// passed output stream.
    void                message(std::ostringstream &oss) const;
    /// part of std::exception interface
    virtual const char* what() const throw();

private:
//-----------------------------------------------------------------------------
//
// -- conduit::Error private data members --
//
//-----------------------------------------------------------------------------
    /// holds the error message
    std::string m_msg;
    /// holds the source file name where the error was raised
    std::string m_file;
    /// holds the line number in the source file where the error was raised.
    index_t     m_line;
    /// holds the formatted error message for std::exception interface
    std::string m_what;
    
};
//-----------------------------------------------------------------------------
// -- end conduit::Error --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif
