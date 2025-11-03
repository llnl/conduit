// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_utils.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_UTILS_HPP
#define CONDUIT_UTILS_HPP

//-----------------------------------------------------------------------------
// -- standard lib includes --
//-----------------------------------------------------------------------------
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include <functional>

//-----------------------------------------------------------------------------
// -- conduit includes --
//-----------------------------------------------------------------------------
#include "conduit_core.hpp"

//-----------------------------------------------------------------------------
//
/// The CONDUIT_INFO macro is the primary mechanism used to log basic messages.
/// It calls conduit::utils::handle_info() which invokes
///
/// The default info handler prints the message to std::out.
/// You can change the info handler via conduit::utils::set_info_handler().
///
//-----------------------------------------------------------------------------
#define CONDUIT_INFO( msg )                                         \
{                                                                   \
    std::ostringstream conduit_oss_info;                            \
    conduit_oss_info << msg;                                        \
    conduit::utils::handle_info( conduit_oss_info.str(),            \
                                  std::string(__FILE__),            \
                                  __LINE__);                        \
}                                                                   \

//-----------------------------------------------------------------------------
//
/// The CONDUIT_WARN macro is the primary mechanism used to capture warnings
/// in conduit. It calls conduit::utils::handle_warning() which invokes
/// the currently configured warning handler.
///
/// The default warning handler throws a c++ exception, in the form of a
/// conduit::Error instance. You can change the error handler via
/// conduit::utils::set_warning_handler().
//
//-----------------------------------------------------------------------------
#define CONDUIT_WARN( msg )                                          \
{                                                                    \
    std::ostringstream conduit_oss_warn;                             \
    conduit_oss_warn << msg;                                         \
    conduit::utils::handle_warning( conduit_oss_warn.str(),          \
                                    std::string(__FILE__),           \
                                    __LINE__);                       \
}                                                                    \

//-----------------------------------------------------------------------------
//
/// The CONDUIT_ERROR macro is the primary mechanism used to capture errors
/// in conduit. It calls conduit::utils::handle_error() which invokes
/// the currently configured error handler.
///
/// The default error handler throws a c++ exception, in the form of a
/// conduit::Error instance. You can change the error handler via
/// conduit::utils::set_error_handler().
//
//-----------------------------------------------------------------------------
#define CONDUIT_ERROR( msg )                                        \
{                                                                   \
    std::ostringstream conduit_oss_error;                           \
    conduit_oss_error << msg;                                       \
    conduit::utils::handle_error( conduit_oss_error.str(),          \
                                  std::string(__FILE__),            \
                                  __LINE__);                        \
}                                                                   \

//-----------------------------------------------------------------------------
//
/// The CONDUIT_ASSERT macro is the primary mechanism used to for asserts
/// in conduit. It calls conduit::utils::handle_error() which invokes
/// the currently configured error handler.
///
/// The default error handler throws a c++ exception, in the form of a
/// conduit::Error instance. You can change the error handler via
/// conduit::utils::set_error_handler().
//
//-----------------------------------------------------------------------------
#define CONDUIT_ASSERT( cond, msg )                                  \
{                                                                    \
    if(!(cond))                                                      \
    {                                                                \
        std::ostringstream conduit_oss_assert;                       \
        conduit_oss_assert << msg;                                   \
        conduit::utils::handle_error( conduit_oss_assert.str(),      \
                                      std::string(__FILE__),         \
                                      __LINE__);                     \
    }                                                                \
}                                                                    \

//-----------------------------------------------------------------------------
//
/// The CONDUIT_CHECK macro is the mechanism used for checks in conduit.
/// It calls conduit::utils::handle_warning() which invokes
/// the currently configured warning handler.
///
/// The default warning handler throws a c++ exception, in the form of a
/// conduit::Error instance. You can change the error handler via
/// conduit::utils::set_warning_handler().
//
//-----------------------------------------------------------------------------
#define CONDUIT_CHECK( cond, msg )                                   \
{                                                                    \
    if(!(cond))                                                      \
    {                                                                \
        std::ostringstream conduit_oss_check;                        \
        conduit_oss_check << msg;                                    \
        conduit::utils::handle_warning( conduit_oss_check.str(),     \
                                        std::string(__FILE__),       \
                                        __LINE__);                   \
    }                                                                \
}                                                                    \

//-----------------------------------------------------------------------------
//
/// The CONDUIT_EPSILON macro defines the default machine epsilon
/// value used when comparing floating-point values. This value is used
/// by default in all of the Conduit comparison operations (e.g.
/// 'conduit::Node::diff' and 'conduit::Node::diff_compatible').
//
//-----------------------------------------------------------------------------
#define CONDUIT_EPSILON 1e-12


//-----------------------------------------------------------------------------
//
/// The CONDUIT_UNUSED macro is used to identify unused variables
/// in cases where it is difficult to avoid defining in the method signature
/// for methods that use optional features.
///
//-----------------------------------------------------------------------------
#define CONDUIT_UNUSED( var ) (void)(var)

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

// fwd declare Node
class Node;

//-----------------------------------------------------------------------------
// -- begin conduit::utils --
//-----------------------------------------------------------------------------
namespace utils
{
//-----------------------------------------------------------------------------
/// Primary interface used by the conduit API when an info message is issued
/// This simply dispatches the message to the currently configured info handler.
/// The default info handler prints a the message to std::cout;
//-----------------------------------------------------------------------------
    void CONDUIT_API handle_info(const std::string &msg,
                                 const std::string &file,
                                 int line);

//-----------------------------------------------------------------------------
/// Info handler callback function type
//-----------------------------------------------------------------------------
    typedef void(*conduit_info_handler)(const std::string &,
                                        const std::string &,
                                        int);

//-----------------------------------------------------------------------------
/// Default info message handler, which prints the message to std::cout;
//-----------------------------------------------------------------------------
   void CONDUIT_API default_info_handler(const std::string &msg,
                                         const std::string &file,
                                         int line);

//-----------------------------------------------------------------------------
/// Allows other libraries to provide an alternate info message handler.
//-----------------------------------------------------------------------------
    void CONDUIT_API set_info_handler(conduit_info_handler on_info);

//-----------------------------------------------------------------------------
/// Returns the active info message handler.
//-----------------------------------------------------------------------------
    conduit_info_handler CONDUIT_API info_handler();

//-----------------------------------------------------------------------------
/// Primary interface used by the conduit API when a warning is issued.
/// This simply dispatches the warning to the currently configured warning handler.
/// The default warning handler throws a conduit::Error exception.
//-----------------------------------------------------------------------------
    void CONDUIT_API handle_warning(const std::string &msg,
                                    const std::string &file,
                                    int line);

//-----------------------------------------------------------------------------
/// Warning handler callback function type
//-----------------------------------------------------------------------------
    typedef void(*conduit_warning_handler)(const std::string &,
                                           const std::string &,
                                           int);

//-----------------------------------------------------------------------------
/// Default warning handler, which throws a conduit::Error exception.
//-----------------------------------------------------------------------------
   void CONDUIT_API default_warning_handler(const std::string &msg,
                                            const std::string &file,
                                            int line);

//-----------------------------------------------------------------------------
/// Allows other libraries to provide an alternate warning handler.
//-----------------------------------------------------------------------------
    void CONDUIT_API set_warning_handler(conduit_warning_handler on_warning);

//-----------------------------------------------------------------------------
/// Returns the active warning message handler.
//-----------------------------------------------------------------------------
    conduit_warning_handler CONDUIT_API warning_handler();

//-----------------------------------------------------------------------------
/// Primary interface used by the conduit API when an error occurs.
/// This simply dispatches the error to the currently configured error handler.
/// The default error handler throws a conduit::Error exception.
//-----------------------------------------------------------------------------
    void CONDUIT_API handle_error(const std::string &msg,
                                  const std::string &file,
                                  int line);

//-----------------------------------------------------------------------------
/// Default error handler, which throws a conduit::Error exception.
//-----------------------------------------------------------------------------
   void CONDUIT_API default_error_handler(const std::string &msg,
                                          const std::string &file,
                                          int line);
//-----------------------------------------------------------------------------
/// Warning handler callback function type
//-----------------------------------------------------------------------------
    typedef void(*conduit_error_handler)(const std::string &,
                                         const std::string &,
                                         int);

//-----------------------------------------------------------------------------
/// Allows other libraries to provide an alternate error handler.
//-----------------------------------------------------------------------------
    void CONDUIT_API set_error_handler(conduit_error_handler on_error);

//-----------------------------------------------------------------------------
/// Returns the active warning message handler.
//-----------------------------------------------------------------------------
    conduit_error_handler CONDUIT_API error_handler();



//-----------------------------------------------------------------------------
/// Primary interface used by the conduit API to move memory.
//-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    /// Function types for memory handlers
    //-----------------------------------------------------------------------------
    typedef std::function<void(void*, const void*, size_t)> handle_memcpy_type;
    typedef std::function<void(void*, int, size_t)> handle_memset_type;
    typedef std::function<void*(size_t, size_t)> handle_alloc_type;
    typedef std::function<void(void*)> handle_free_type;

    // conduit uses a single pair of memset and memcpy functions to
    // manage data movement.

    // this strategy allows downstream users to support complex cases
    // like moving between memory spaces not accessible on the host.
    //
    // These methods aren't bound to allocators b/c allocators
    // won't be tied into all of the places where source and dest pointers
    // need to be located.
    //
    void CONDUIT_API set_memcpy_handler(handle_memcpy_type);
    void CONDUIT_API set_memset_handler(handle_memset_type);

    void CONDUIT_API default_memset_handler(void *ptr,
                                            int value,
                                            size_t num);

    void CONDUIT_API default_memcpy_handler(void *destination,
                                            const void *source,
                                            size_t num);

    // general memcpy interface used by conduit
    void CONDUIT_API conduit_memcpy(void *destination,
                                    const void *source,
                                    size_t num);

    void CONDUIT_API conduit_memcpy_strided_elements(void *dest,
                                                     size_t num_elements,
                                                     size_t ele_bytes,
                                                     size_t dest_stride,
                                                     const void *src,
                                                     size_t src_stride);

    // general memset interface used by conduit
    // NOTE (cyrush): The default memset returns the orig pointer, but
    // other allocators like cuda do not.
    //    TODO: GIVEN THIS DO WE NEED TO PASS AN ALLOC_ID?
    void CONDUIT_API conduit_memset(void * ptr,
                                    int value,
                                    size_t num);

//-----------------------------------------------------------------------------
/// Primary interface used by the conduit API to allocate memory.
//-----------------------------------------------------------------------------

    // register a custom allocator
    index_t CONDUIT_API register_allocator(handle_alloc_type,
                                           handle_free_type);

    // generic allocate interface
    // allocator_id 0 is the default
    void CONDUIT_API * conduit_allocate(size_t num_items,
                                        size_t item_size,
                                        index_t allocator_id = 0);

    // generic free interface
    void CONDUIT_API conduit_free(void *data_ptr,
                                  index_t allocator_id = 0);



//-----------------------------------------------------------------------------
/// Helpers for common string splitting operations.
//-----------------------------------------------------------------------------
    void CONDUIT_API split_string(const std::string &str,
                                  const std::string &sep,
                                  std::string &curr,
                                  std::string &next);

    void CONDUIT_API split_string(const std::string &str,
                                  char sep,
                                  std::vector<std::string> &sv);

    void CONDUIT_API rsplit_string(const std::string &str,
                                   const std::string &sep,
                                   std::string &curr,
                                   std::string &next);

    //------------------------------------------------------------------------
    /// Removes the the leading and trailing whitespace from the string 'str'.
    /// 'chars_to_trim' can be overriden to trim a different set of characters
    /// from 'str'.
    //-------------------------------------------------------------------------
    void CONDUIT_API trim_string(std::string &str,
                                 const char *chars_to_trim = " \t\n\r\f\v");

//-----------------------------------------------------------------------------
/// Helpers for splitting and joining conduit paths (which always use "/")
//-----------------------------------------------------------------------------
    void  CONDUIT_API split_path(const std::string &path,
                                 std::string &curr,
                                 std::string &next);

    void  CONDUIT_API rsplit_path(const std::string &path,
                                  std::string &curr,
                                  std::string &next);

    std::string CONDUIT_API join_path(const std::string &left,
                                      const std::string &right);

//-----------------------------------------------------------------------------
/// Helpers for splitting and joining file system paths.
/// These use the proper platform specific separator (/ or \).
//-----------------------------------------------------------------------------
    std::string CONDUIT_API file_path_separator();

    void CONDUIT_API        split_file_path(const std::string &path,
                                            std::string &curr,
                                            std::string &next);

    void CONDUIT_API        rsplit_file_path(const std::string &path,
                                             std::string &curr,
                                             std::string &next);

     //------------------------------------------------------------------------
    /// `split_file_path` and `rsplit_file_path` are helpers that allows us to
    ///  use  ":" for subpaths even on Windows when a drive letter including
    ///  ":" is in the path.
    //-------------------------------------------------------------------------
    void CONDUIT_API split_file_path(const std::string &str,
                                     const std::string &sep,
                                     std::string &curr,
                                     std::string &next);

    void CONDUIT_API rsplit_file_path(const std::string &str,
                                      const std::string &sep,
                                      std::string &curr,
                                      std::string &next);


    std::string CONDUIT_API join_file_path(const std::string &left,
                                           const std::string &right);




//-----------------------------------------------------------------------------
     bool CONDUIT_API is_file(const std::string &path);

//-----------------------------------------------------------------------------
     bool CONDUIT_API is_directory(const std::string &path);

//-----------------------------------------------------------------------------
/// Lists the items contained in the given directory.
/// Each entry returned in "contents" will have the directory
/// included in the path.
///
/// Queries the system for the contents of the given directory.
/// Does not perform any checks on the returned filenames, could be
/// directories or files (anything a directory may contain).
///
/// Param "directory" must be the path to a directory.
/// Param "ignore_dot" ignores any items starting with '.'
///   (ie: "." or ".." or ".dotfile"), defaulted to true.
/// returns false if there was an error opening the provided directory,
/// true otherwise.
//-----------------------------------------------------------------------------
     bool CONDUIT_API list_directory_contents(const std::string &directory,
                                              std::vector<std::string> &contents,
                                              bool ignore_dot = true);

//-----------------------------------------------------------------------------
     index_t CONDUIT_API file_size(const std::string &path);

//-----------------------------------------------------------------------------
/// Creates a new directory.
///
/// Does not recursively create parent directories if they do not already
/// exist.
//-----------------------------------------------------------------------------
     bool CONDUIT_API create_directory(const std::string &path);

//-----------------------------------------------------------------------------
/// Remove files, or empty directories
//-----------------------------------------------------------------------------
     bool CONDUIT_API remove_file(const std::string &path);

     bool CONDUIT_API remove_directory(const std::string &path);

     bool CONDUIT_API remove_path_if_exists(const std::string &path);

//-----------------------------------------------------------------------------
     int  CONDUIT_API system_execute(const std::string &cmd);


//-----------------------------------------------------------------------------
/// Helpers for escaping / unescaping special characters in strings.
///
/// Our main use case for escaping is json, so we support the escape rules
/// outlined by the json standard (see: http://www.json.org/).
///
/// List of supported special characters.
///    " (quote)
///    \ (backward slash)
///    \n (newline)
///    \t (tab)
///    \b (backspace)
///    \f (form feed)
///    \r (carriage return)
///
/// Special chars that are not escaped, but are unescaped:
///    / (forward slash)
///
/// Special chars that are not escaped or unescaped:
///    \u (for hex escapes: \uFFFF)
///
//-----------------------------------------------------------------------------
    std::string CONDUIT_API escape_special_chars(const std::string &input);
    std::string CONDUIT_API unescape_special_chars(const std::string &input);

//-----------------------------------------------------------------------------
     std::string CONDUIT_API strip_quoted_strings(const std::string &input,
                                                  const std::string &quote_char);


//-----------------------------------------------------------------------------
/// fmt style string formatting helpers
//-----------------------------------------------------------------------------

    std::string CONDUIT_API format(const std::string &s,
                                   const conduit::Node &args);

    std::string CONDUIT_API format(const std::string &s,
                                   const conduit::Node &maps,
                                   index_t map_index);

//-----------------------------------------------------------------------------
/// Base64 Encoding of Buffers
//-----------------------------------------------------------------------------
    void CONDUIT_API base64_encode(const void *src,
                                   index_t src_nbytes,
                                   void *dest);

    index_t CONDUIT_API base64_encode_buffer_size(index_t src_nbytes);

    index_t CONDUIT_API base64_decode_buffer_size(index_t encoded_nbytes);


    void CONDUIT_API base64_decode(const void *src,
                                   index_t src_nbytes,
                                   void *dest);

//-----------------------------------------------------------------------------
     std::string CONDUIT_API json_sanitize(const std::string &json);

//-----------------------------------------------------------------------------
     // declare then define to avoid icc warnings
     template< typename T >
     std::string to_hex_string(T value);

     template< typename T >
     std::string to_hex_string(T value)
     {
        std::stringstream oss;
        oss << std::hex << value;
        return oss.str();
     }

//-----------------------------------------------------------------------------
// Helpers to identify value cast consequences
//-----------------------------------------------------------------------------
     // adapted from: https://stackoverflow.com/a/17225324/203071
    template< typename T_SRC, typename T_DEST>
    bool value_fits(T_SRC value)
    {
        if(std::is_same<T_SRC,T_DEST>())
        {
            return true;
        }

        return ( (value > static_cast<T_SRC>(0) ) ==
                 (static_cast<T_DEST>(value) > static_cast<T_DEST>(0))
               ) && static_cast<T_SRC>(static_cast<T_DEST>(value)) == value;

    }




//-----------------------------------------------------------------------------
// Helpers to identify if a string contains an integer.
//-----------------------------------------------------------------------------
    bool CONDUIT_API string_is_integer(const std::string &s);

//-----------------------------------------------------------------------------
// Helper that wraps parsing a string value into another type.
//-----------------------------------------------------------------------------
    // declare then define to avoid icc warnings
    template< typename T >
    T string_to_value(const std::string &s);

    template< typename T >
    T string_to_value(const std::string &s)
    {
        T res;
        std::istringstream iss(s);
        iss >> res;
        return res;
    }

    // declare then define to avoid icc warnings
    template< typename T >
    T hex_string_to_value(const std::string &s);

    template< typename T >
    T hex_string_to_value(const std::string &s)
    {
        T res;
        std::istringstream iss(s);
        iss >> std::hex >> res;
        return res;
    }


//-----------------------------------------------------------------------------
// floating point to string helper, strikes a balance of what we want
// for format-wise for debug printing and json + yaml.
//-----------------------------------------------------------------------------
    std::string CONDUIT_API float64_to_string(float64 value);

//-----------------------------------------------------------------------------
    void CONDUIT_API indent(std::ostream &os,
                            index_t indent,
                            index_t depth,
                            const std::string &pad);

//-----------------------------------------------------------------------------
    void CONDUIT_API sleep(index_t milliseconds);

//-----------------------------------------------------------------------------
/// Simple timer class
//-----------------------------------------------------------------------------
    class CONDUIT_API Timer
    {
        typedef std::chrono::high_resolution_clock high_resolution_clock;
        typedef std::chrono::duration<float> fsec;

        public:
            explicit Timer();
            void     reset();
            float    elapsed() const;

        private:
            std::chrono::high_resolution_clock::time_point m_start;
    };

//-----------------------------------------------------------------------------
// String hash functions
//-----------------------------------------------------------------------------
     unsigned int CONDUIT_API hash(const char *k,
                                   unsigned int length,
                                   unsigned int initval = 0);
     unsigned int CONDUIT_API hash(const char *k,
                                   unsigned int initval = 0);
     unsigned int CONDUIT_API hash(const std::string &k,
                                   unsigned int initval = 0);

//-----------------------------------------------------------------------------
// Other hash functions
//-----------------------------------------------------------------------------
     uint64       CONDUIT_API hash(const index_t *k,
                                   unsigned int length);
     uint64       CONDUIT_API hash(const float64 *k,
                                   unsigned int length);
     uint64       CONDUIT_API hash(const uint64 *k,
                                   unsigned int length);

//-----------------------------------------------------------------------------
// Math utility functions
//-----------------------------------------------------------------------------
    /**
     \brief Factor an integer and return a vector of its factors. If the number
            is not prime, we omit 1, num from the vector.

     \param num The number to factor.

     \return A vector containing the factors of the number. The product of these
             values will equal the original number.
     */
    std::vector<conduit::index_t> CONDUIT_API factor(conduit::index_t num);

//-----------------------------------------------------------------------------
// Memory usage helper
//-----------------------------------------------------------------------------
    /// returns process memory usage in kB, or zero if unable to read usage
    uint64 CONDUIT_API memory_usage();

}
//-----------------------------------------------------------------------------
// -- end conduit::utils --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
