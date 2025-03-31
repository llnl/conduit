// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_utils.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_utils.hpp"
#include "conduit_error.hpp"

//-----------------------------------------------------------------------------
// -- standard lib includes --
//-----------------------------------------------------------------------------

// for sleep and mem usage funcs
#if defined(CONDUIT_PLATFORM_WINDOWS)
#define NOMINMAX
#include <windows.h>
#include <direct.h>
#if (_MSC_VER && _MSC_VER < 1900)
    #define snprintf _snprintf
#endif
#undef min
#undef max
// mem usage api
#include <psapi.h>

#else // not windows
#include <dirent.h>
#include <time.h>
#endif

// for macos memory usage functions
#if defined(CONDUIT_PLATFORM_APPLE) && defined(__MACH__)
#include <mach/mach.h>
#endif

// file system funcs
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <limits>
#include <fstream>
#include <map>


// define proper path sep
#if defined(CONDUIT_PLATFORM_WINDOWS)
#define CONDUIT_UTILS_FILE_PATH_SEPARATOR "\\"
#else
#define CONDUIT_UTILS_FILE_PATH_SEPARATOR "/"
#endif

static const std::string file_path_sep_string(CONDUIT_UTILS_FILE_PATH_SEPARATOR);

#include "conduit.hpp"
#include "conduit_fmt/conduit_fmt.h"


//-----------------------------------------------------------------------------
// -- libb64 includes --
//-----------------------------------------------------------------------------
#define BUFFERSIZE 65536
#include "b64/encode.h"
#include "b64/decode.h"
using namespace base64;


//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::utils --
//-----------------------------------------------------------------------------
namespace utils
{

//-----------------------------------------------------------------------------
void *
default_alloc_handler(size_t items, size_t item_size)
{
  return calloc(items, item_size);
}

//-----------------------------------------------------------------------------
void
default_free_handler(void *data_ptr)
{
  free(data_ptr);
}

//-----------------------------------------------------------------------------
void
default_memcpy_handler(void * destination, const void * source, size_t num)
{
  memcpy(destination,source,num);
}

//-----------------------------------------------------------------------------
void
default_memset_handler(void * ptr, int value, size_t num )
{
  memset(ptr,value,num);
}


namespace detail
{
    //
    // AllocManager: A singleton that holds our alloc and free function maps
    //
    //
    // NOTE: THE SINGLETON INSTANCE IS INTENTIONALLY LEAKED!
    //
    // These maps are used by Node instances to alloc and free
    // memory, they need to exist as long as any Node object exists!
    //
    // We are doing static init correctly here, so we
    // avoid the C++ static obj init fiasco:
    // https://isocpp.org/wiki/faq/ctors#static-init-order
    //
    // However, we still can't apply fine grained control
    // to when these objects are cleaned up. This means
    // if you use a statically initialized Node object
    // you could get a crash on exit
    //  (unless we leak the singleton)
    //
    // One strategy to avoid this is to move everything
    // into the same compilation unit. We tried this
    // but we still hit an on exit cleanup where
    // the alloc maps were destructed, but a static Node
    // object still needed to cleanup.
    //
    // If leaking this singleton offends your sensibilities,
    // I am sorry. Ideally, these would be cleaned up on
    // exit absolutely last - meaning you could never use
    // those few precious bytes for anything else.
    //
    class AllocManager {

     public:
          static AllocManager& instance()
          {
            //
            // NOTE: THIS IS INTENTIONALLY LEAKED
            // See note above.
            //
            static AllocManager *inst = new AllocManager();
            return *inst;
          }

          // reg interface
          index_t register_allocator(handle_alloc_type conduit_hnd_allocate,
                                     handle_free_type conduit_hnd_free)
          {
              m_allocator_map[m_allocator_id] = conduit_hnd_allocate;
              m_free_map[m_allocator_id]      = conduit_hnd_free;
              return m_allocator_id++;
          }

          // alloc interface
          void *allocate(size_t n_items,
                         size_t item_size,
                         index_t allocator_id)
          {
              return m_allocator_map[allocator_id](n_items, item_size);
          }

          // free interface
          void free(void *ptr,
                   index_t allocator_id)
          {
              m_free_map[allocator_id](ptr);
          }

          // set memcpy and memset handlers
          void set_memcpy_handler(handle_memcpy_type conduit_hnd_memcpy)
          {
              m_handle_memcpy = conduit_hnd_memcpy;
          }

          void set_memset_handler(handle_memset_type conduit_hnd_memset)
          {
              m_handle_memset = conduit_hnd_memset;
          }

          void memcpy(void *destination,
                      const void *source,
                      size_t num)
          {
              m_handle_memcpy(destination, source, num);
          }

          void memset(void *ptr,
                      int value,
                      size_t num)
          {
              m_handle_memset(ptr, value, num);
          }

     private:
          // constructor
          AllocManager()
          : m_allocator_map(),
            m_free_map(),
            m_handle_memcpy(default_memcpy_handler),
            m_handle_memset(default_memset_handler)
          {
              // register default handlers
              m_allocator_map[0] = &default_alloc_handler;
              m_free_map[0]      = &default_free_handler;

              m_allocator_id = 1;

          }

          // destructor
          ~AllocManager()
          {

          }

          // vars
          index_t                                m_allocator_id;
          std::map<index_t,handle_alloc_type>    m_allocator_map;
          std::map<index_t,handle_free_type>     m_free_map;

          handle_memcpy_type                     m_handle_memcpy;
          handle_memset_type                     m_handle_memset;
    };
}

//-----------------------------------------------------------------------------
void
set_memcpy_handler(handle_memcpy_type conduit_hnd_memcpy)
{
    detail::AllocManager::instance().set_memcpy_handler(conduit_hnd_memcpy);
}

//-----------------------------------------------------------------------------
void
set_memset_handler(handle_memset_type conduit_hnd_memset)
{
    detail::AllocManager::instance().set_memset_handler(conduit_hnd_memset);
}

//-----------------------------------------------------------------------------
index_t
register_allocator(handle_alloc_type conduit_hnd_allocate,
                   handle_free_type conduit_hnd_free)
{
    return detail::AllocManager::instance().register_allocator(conduit_hnd_allocate,
                                                               conduit_hnd_free);
}


//-----------------------------------------------------------------------------
void *
conduit_allocate(size_t n_items,
                 size_t item_size,
                 index_t allocator_id)
{
    return detail::AllocManager::instance().allocate(n_items,
                                                     item_size,
                                                     allocator_id);
}

//-----------------------------------------------------------------------------
void
conduit_free(void *ptr,
             index_t allocator_id)
{
    detail::AllocManager::instance().free(ptr,allocator_id);
}

//-----------------------------------------------------------------------------
void
conduit_memcpy(void *destination,
               const void *source,
               size_t num)
{
    detail::AllocManager::instance().memcpy(destination,source,num);
}


//-----------------------------------------------------------------------------
void conduit_memset(void *ptr,
                    int value,
                    size_t num)
{
    detail::AllocManager::instance().memset(ptr,value,num);
}
//-----------------------------------------------------------------------------
void
conduit_memcpy_strided_elements(void *dest,
                                size_t num_elements,
                                size_t ele_bytes,
                                size_t dest_stride,
                                const void *src,
                                size_t src_stride)
{
    // source and dest are compact
    if( dest_stride == ele_bytes && src_stride == ele_bytes)
    {
        detail::AllocManager::instance().memcpy(dest,
                                                src,
                                                ele_bytes * num_elements);
    }
    else // the source or dest are strided in a non compact way
    {
        char *src_data_ptr  = (char*) src;
        char *dest_data_ptr = (char*) dest;
        for(size_t i=0; i< num_elements; i++)
        {
            // copy next strided element
            detail::AllocManager::instance().memcpy(dest_data_ptr,
                                                    src_data_ptr,
                                                    ele_bytes);
            // move by src stride
            src_data_ptr  += src_stride;
            // move by dest stride
            dest_data_ptr += dest_stride;
        }
    }
}

//-----------------------------------------------------------------------------
// default info message handler callback, simply prints to std::cout.
void
default_info_handler(const std::string &msg,
                     const std::string &file,
                     int line)
{
    std::cout << "[" << file
              << " : " << line  << "]"
              << "\n " << msg << std::endl;
}

//-----------------------------------------------------------------------------
// Private namespace member that holds our info message handler callback.
void (*conduit_on_info)(const std::string &,
                        const std::string &,
                        int)= default_info_handler;

//-----------------------------------------------------------------------------
// Allows other libraries to provide an alternate error handler.
void
set_info_handler(void(*on_info)
                 (const std::string&,
                  const std::string&,
                  int))
{
    conduit_on_info = on_info;
}

//-----------------------------------------------------------------------------
conduit_info_handler
info_handler()
{
    return conduit_on_info;
}

//-----------------------------------------------------------------------------
void
handle_info(const std::string &msg,
            const std::string &file,
            int line)
{
    conduit_on_info(msg,file,line);
}

//-----------------------------------------------------------------------------
// default warning handler callback, simply throws a conduit::Error exception.
void
default_warning_handler(const std::string &msg,
                        const std::string &file,
                        int line)
{
    throw conduit::Error( msg, file, line);
}

//-----------------------------------------------------------------------------
// Private namespace member that holds our info message handler callback.
void (*conduit_on_warning)(const std::string &,
                           const std::string &,
                           int)= default_warning_handler;

//-----------------------------------------------------------------------------
// Allows other libraries to provide an alternate warning handler.
void
set_warning_handler(void(*on_warning)
                    (const std::string&,
                     const std::string&,
                     int))
{
    conduit_on_warning = on_warning;
}

//-----------------------------------------------------------------------------
conduit_warning_handler
warning_handler()
{
    return conduit_on_warning;
}

//-----------------------------------------------------------------------------
void
handle_warning(const std::string &msg,
               const std::string &file,
               int line)
{
    conduit_on_warning(msg,file,line);
}


//-----------------------------------------------------------------------------
// default error handler callback, simply throws a conduit::Error exception.
void
default_error_handler(const std::string &msg,
                      const std::string &file,
                      int line)
{
    throw conduit::Error( msg, file, line);
}

//-----------------------------------------------------------------------------
// Private namespace member that holds our error handler callback.
void (*conduit_on_error)(const std::string &,
                         const std::string &,
                         int)= default_error_handler;

//-----------------------------------------------------------------------------
// Allows other libraries to provide an alternate error handler.
void
set_error_handler(void(*on_error)
                  (const std::string&,
                  const std::string&,
                  int))
{
    conduit_on_error = on_error;
}

//-----------------------------------------------------------------------------
conduit_error_handler
error_handler()
{
    return conduit_on_error;
}

//-----------------------------------------------------------------------------
void
handle_error(const std::string &msg,
             const std::string &file,
             int line)
{
    conduit_on_error(msg,file,line);
}


//-----------------------------------------------------------------------------
void
split_string(const std::string &str,
             const std::string &sep,
             std::string &curr,
             std::string &next)
{
    curr.clear();
    next.clear();

    std::size_t found = str.find(sep);
    if (found != std::string::npos)
    {
        curr = str.substr(0,found);
        if(found != str.size()-1)
            next = str.substr(found+1,str.size()-(found-1));
    }
    else
    {
        curr = str;
    }
}

//-----------------------------------------------------------------------------
void
split_string(const std::string &str, char sep, std::vector<std::string> &sv)
{
    if(!str.empty())
    {
        const char *start = str.c_str();
        const char *c     = str.c_str();
        while(*c != '\0')
        {
            if(*c == sep)
            {
                size_t len = c - start;
                if(len > 0)
                    sv.push_back(std::string(start, len));
                c++;
                start = c;
            }
            else
                c++;
        }
        if(*start != '\0')
        {
            size_t len = c - start;
            if(len > 0)
                sv.push_back(std::string(start, len));
        }
    }
}

//-----------------------------------------------------------------------------
void
rsplit_string(const std::string &str,
              const std::string &sep,
              std::string &curr,
              std::string &next)
{
    curr.clear();
    next.clear();

    std::size_t found = str.rfind(sep);
    if (found != std::string::npos)
    {
        next = str.substr(0,found);
        if(found != str.size()-1)
             curr = str.substr(found+1,str.size()-(found-1));
    }
    else
    {
        curr = str;
    }
}

//-----------------------------------------------------------------------------
void
trim_string(std::string &str,
            const char *chars_to_trim)
{
    // ltrim, rtrim
    str.erase(0, str.find_first_not_of(chars_to_trim));
    str.erase(str.find_last_not_of(chars_to_trim) + 1);
}

//-----------------------------------------------------------------------------
void
split_path(const std::string &path,
           std::string &curr,
           std::string &next)
{
    split_string(path,
                 std::string("/"),
                 curr,
                 next);
}

//-----------------------------------------------------------------------------
void
rsplit_path(const std::string &path,
            std::string &curr,
            std::string &next)
{
    rsplit_string(path,
                  std::string("/"),
                  curr,
                  next);
}

//-----------------------------------------------------------------------------
std::string
join_path(const std::string &left,
          const std::string &right)
{
    std::string res = left;
    if(res.size() > 0 &&
       res[res.size()-1] != '/' &&
       right.size() > 0 )
    {
        res += "/";
    }
    res += right;
    return res;
}

//-----------------------------------------------------------------------------
std::string
file_path_separator()
{
    return file_path_sep_string;
}


//-----------------------------------------------------------------------------
void
split_file_path(const std::string &path,
                std::string &curr,
                std::string &next)
{
    split_string(path,
                 file_path_sep_string,
                 curr,
                 next);
}

//-----------------------------------------------------------------------------
void
rsplit_file_path(const std::string &path,
                 std::string &curr,
                 std::string &next)
{
    rsplit_string(path,
                  file_path_sep_string,
                  curr,
                  next);
}

//---------------------------------------------------------------------------//
void
split_file_path(const std::string &path,
                const std::string &sep,
                std::string &curr,
                std::string &next)
{
    // if we are splitting by ":", we need to be careful on windows
    // since drive letters include ":"
    //
    // NOTE: We could if-def for windows, but its nice to be able
    // to run unit tests on other platforms.
    if( sep == std::string(":") &&
        path.size() > 2 &&
        path[1] == ':' &&
        path[2] == '\\')
    {
        // eval w/o drive letter
        if(path.size() > 3)
        {
            std::string check_path = path.substr(3);
            conduit::utils::split_string(check_path,
                                         sep,
                                         curr,
                                         next);
            // add drive letter back
            curr = path.substr(0,3) + curr;
        }
        else
        {
            // degen case, we we only have the drive letter
            curr = path;
            next = "";
        }
    }
    else
    {
        // normal case
        conduit::utils::split_string(path,
                                     sep,
                                     curr,
                                     next);

    }
}


//---------------------------------------------------------------------------//
void
rsplit_file_path(const std::string &path,
                 const std::string &sep,
                 std::string &curr,
                 std::string &next)
{
    // if we are splitting by ":", we need to be careful on windows
    // since drive letters include ":"
    //
    // NOTE: We could if-def for windows, but its nice to be able
    // to run unit tests on other platforms.
    if( sep == std::string(":") &&
        path.size() > 2 &&
        path[1] == ':' &&
        path[2] == '\\')
    {
        // eval w/o drive letter
        if(path.size() > 3)
        {
            std::string check_path = path.substr(3);
            conduit::utils::rsplit_string(check_path,
                                          sep,
                                          curr,
                                          next);
            // add drive letter back
            if(next == "")
            {
                // there was no split
                curr = path.substr(0,3) + curr;
            }
            else
            {
                // there was a split
                next = path.substr(0,3) + next;
            }
        }
        else
        {
            // degen case, we we only have the drive letter
            curr = path;
            next = "";
        }
    }
    else
    {
        // normal case
        conduit::utils::rsplit_string(path,
                                      sep,
                                      curr,
                                      next);

    }
}



//-----------------------------------------------------------------------------
std::string
join_file_path(const std::string &left,
               const std::string &right)
{
    std::string res = left;
    if(res.size() > 0 && res[res.size()-1] != file_path_sep_string[0])
    {
        res += file_path_sep_string;
    }
    res += right;
    return res;
}


//-----------------------------------------------------------------------------
bool
is_file(const std::string &path)
{
    bool res = false;
    struct stat path_stat;
    if(stat(path.c_str(), &path_stat) == 0)
    {
        if(path_stat.st_mode & S_IFREG)
            res = true;
    }
    return res;
}

//-----------------------------------------------------------------------------
int64
file_size(const std::string &path)
{
    std::ifstream ifs(path, std::ifstream::ate | std::ifstream::binary);
    return (int64) ifs.tellg();
}

//-----------------------------------------------------------------------------
bool
is_directory(const std::string &path)
{
    bool res = false;
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) == 0)
    {
        if (path_stat.st_mode & S_IFDIR)
            res = true;
    }
    return res;
}

//-----------------------------------------------------------------------------
bool
list_directory_contents(const std::string &path,
                        std::vector<std::string> &contents,
                        bool ignore_dot)
{
    contents.clear();
    if(path.empty())
    {
        return false;
    }

    // If the given path doesn't end with a path sep, add it to the end
    const std::string directory =
        (path.substr(path.size()-1) != file_path_sep_string)
            ? (path + file_path_sep_string)
            : path;

#if defined(CONDUIT_PLATFORM_WINDOWS)
    // NOTE: Adapted from VisIt
    if(path == "My Computer")
    {
        // Add the drives to the list.
        char buf[200];
        // DWORD bufLen = 200; unused
        DWORD slen = GetLogicalDriveStrings(200, buf);

        if(slen > 0)
        {
            char *ptr = buf;
            while(*ptr != 0)
            {
                contents.push_back(ptr);
                ptr += (contents.back().size() + 1);
            }
        }
    }
    else
    {
        // Search all files in the given directory
        std::string search_path(directory + "*");
        WIN32_FIND_DATA fd;
        HANDLE dirHandle = FindFirstFile(search_path.c_str(), &fd);
        if(dirHandle != INVALID_HANDLE_VALUE)
        {
            // Iterate the contents of the directory using FindNextFile
            do
            {
                const std::string name(fd.cFileName);
                if(name.empty())
                {
                    continue;
                }

                if(ignore_dot && name[0] == '.')
                {
                    continue;
                }
                contents.push_back(directory + name);
            } while(FindNextFile(dirHandle, &fd));
            FindClose(dirHandle);
        }
    }
#else
    DIR *dir;
    dirent *ent;
    // Open the directory
    dir = opendir(path.c_str());
    if(dir)
    {
        // Iterate the contents of the directory using ent
        while((ent = readdir(dir)) != NULL)
        {
            const std::string name(ent->d_name);
            if(name.empty())
            {
                continue;
            }

            if(ignore_dot && name[0] == '.')
            {
                continue;
            }

            contents.push_back(directory + ent->d_name);
        }
        closedir(dir);
    }
#endif
    return !contents.empty();
}

//-----------------------------------------------------------------------------
bool
create_directory(const std::string &path)
{

#if defined(CONDUIT_PLATFORM_WINDOWS)
    return (_mkdir(path.c_str()) == 0);
#else
    return (mkdir(path.c_str(),S_IRWXU | S_IRWXG) == 0);
#endif
}


//-----------------------------------------------------------------------------
bool
remove_file(const std::string &path)
{
    return ( remove(path.c_str()) == 0 );
}

//-----------------------------------------------------------------------------
bool
remove_directory(const std::string &path)
{
#if defined(CONDUIT_PLATFORM_WINDOWS)
    return ( _rmdir(path.c_str()) == 0 );
#else
    return ( remove(path.c_str()) == 0 );
#endif
}


//-----------------------------------------------------------------------------
bool
remove_path_if_exists(const std::string &path)
{
    if(utils::is_file(path))
    {
        return utils::remove_file(path);
    }
    else if(utils::is_directory(path))
    {
        return utils::remove_directory(path);
    }
    else
    {
        // nothing to do, report we didn't remove anything
        return false;
    }
}


//-----------------------------------------------------------------------------
int
system_execute(const std::string &cmd)
{
    return system(cmd.c_str());
}


//-----------------------------------------------------------------------------
bool
check_word_char(const char v)
{
    bool res = ( ( 'A' <= v) &&
                 (  v  <= 'Z') );
    res = res || ( ( 'a' <= v) &&
                 (  v  <= 'z') );
    res = res || v == '_';
    return res;
}

//-----------------------------------------------------------------------------
bool
check_num_char(const char v)
{
    bool res = ( ( '0' <= v) &&
                 (  v  <= '9') );
    return res;
}

//---------------------------------------------------------------------------//
std::string
strip_quoted_strings(const std::string &input, const std::string &quote_char)
{
    std::string res;
    bool in_string = false;
    bool emit = false;

    for(size_t i = 0; i < input.size(); ++i)
    {
        emit = true;
        // emit a character when not in a string, including the opening and
        // closing quote.
        //
        // check for a quote + start & end of a string
        // the check for `\\` ignores escaped quotes inside a string
        if(input[i] == quote_char[0] && ( i > 0 && ( input[i-1] != '\\' )))
        {
            if(in_string)
            {
                // we are in a string & have encountered an ending quote
                // so we are no longer in a string but  still don't want to
                // emit the (current) quote char.
                in_string = false;
                emit = false;
            }
            else
            {
                // we aren't in a string & have encountered a starting quote,
                // we are starting a string
                in_string = true;
            }
        }

        // never emit when in a string
        if(in_string)
            emit = false;

        if(emit)
            res += input[i];
    }

    return res;
}


//-----------------------------------------------------------------------------
std::string
json_sanitize(const std::string &json)
{
    ///
    /// Really wanted to use regexs to solve this
    /// but posix regs are greedy & it was hard for me to construct
    /// a viable regex, vs those that support non-greedy (Python + Perl style regex)
    ///
    /// Here are regexs I was able to use in python:
    //  *comments*
    //     Remove '//' to end of line
    //     regex: \/\/.*\?n
    //  *limited quoteless*
    //    find words not surrounded by quotes
    //    regex: (?<!"|\w)(\w+)(?!"|\w)
    //    and add quotes

    //
    // for now, we use a simple char by char parser
    //

    std::string res;
    bool        in_comment=false;
    bool        in_string=false;
    bool        in_id =false;
    std::string cur_id = "";

    for(size_t i = 0; i < json.size(); ++i)
    {
        bool emit = true;
        // check for start & end of a string
        if(json[i] == '\"' &&  ( i > 0 && ( json[i-1] != '\\' )))
        {
            if(in_string)
                in_string = false;
            else
                in_string = true;
        }

        // handle two cases were we want to sanitize:
        // comments '//' to end of line & unquoted ids
        if(!in_string)
        {
            if(!in_comment)
            {
                if( json[i] == '/'  &&
                    i < (json.size()-1) &&
                    json[i+1] == '/')
                {
                    in_comment = true;
                    emit = false;
                }
            }

            if(!in_comment)
            {

                if( !in_id && check_word_char(json[i]))
                {
                    // ids can't start with numbers ,
                    // check the prior char if it exists
                    if(i > 0 &&
                       !check_num_char(json[i-1]) &&
                       json[i-1] != '.')
                    {
                        in_id = true;
                        // accum id chars
                        cur_id += json[i];
                        emit = false;
                    }
                }
                else if(in_id) // finish the id
                {
                    if(check_word_char(json[i]) || check_num_char(json[i]))
                    {
                        in_id = true;
                        // accum id chars
                        cur_id += json[i];
                        emit = false;
                    }
                    else
                    {
                        in_id = false;
                        /// check for true, false, and null
                        /// which we need to support in json
                        if( !(cur_id == "true"  ||
                              cur_id == "false" ||
                              cur_id == "null" ))
                        {
                            /// emit cur_id
                            res += "\"" + cur_id + "\"";
                        }
                        else
                        {
                            /// don't escape true or false
                            res +=  cur_id;
                        }

                        cur_id = "";
                    }
                    // we will also emit this char
                }
            }

            if(in_comment)
            {
                emit = false;
                if(json[i] == '\n')
                {
                    in_comment = false;
                }
            }
        }

        if(emit)
            res += json[i];
    }

    return res;
}

//-----------------------------------------------------------------------------
void
indent(std::ostream &os,
       index_t indent,
       index_t depth,
       const std::string &pad)
{
    for(index_t i=0;i<depth;i++)
    {
        for(index_t j=0;j<indent;j++)
        {
            os << pad;
        }
    }
}

//-----------------------------------------------------------------------------
void
sleep(index_t milliseconds)
{

#if defined(CONDUIT_PLATFORM_WINDOWS)
    Sleep((DWORD)milliseconds);
#else // unix, etc
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// Timer class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Timer::Timer()
{
    reset();
}

//-----------------------------------------------------------------------------
void
Timer::reset()
{
    m_start = high_resolution_clock::now();
}

//-----------------------------------------------------------------------------
float
Timer::elapsed() const
{
    auto ftime  = std::chrono::duration_cast<fsec>(high_resolution_clock::now() - m_start);
    return ftime.count();
}


//-----------------------------------------------------------------------------
std::string
escape_special_chars(const std::string &input)
{
    std::string res;
    for(size_t i = 0; i < input.size(); ++i)
    {
        char val = input[i];
        // supported special chars
        switch(val)
        {
            // quotes and slashes
            case '\"':
            case '\\':
            {
                res += '\\';
                res += val;
                break;
            }
            // newline
            case '\n':
            {
                res += "\\n";
                break;
            }
            // tab
            case '\t':
            {
                res += "\\t";
                break;
            }
            // backspace
            case '\b':
            {
                res += "\\b";
                break;
            }
            // formfeed
            case '\f':
            {
                res += "\\f";
                break;
            }
            // carriage return
            case '\r':
            {
                res += "\\r";
                break;
            }

            default:
            {
                res += val;
            }
        }
    }

    return res;
}

//-----------------------------------------------------------------------------
std::string
unescape_special_chars(const std::string &input)
{
    std::string res;
    size_t input_size = input.size();
    for(size_t i = 0; i < input_size; ++i)
    {
        // check for escape char
        if( input[i] == '\\' &&
            i < (input_size -1))
        {
            char val = input[i+1];
            switch(val)
            {
                // quotes and slashes
                case '\"':
                case '\\':
                // even though we don't escape forward slashes
                // we support unescaping them.
                case '/':
                {
                    res += val;
                    // skip escape char
                    i++;
                    break;
                }
                // newline
                case 'n':
                {
                    res += "\n";
                    // skip escape char
                    i++;
                    break;
                }
                // tab
                case 't':
                {
                    res += "\t";
                    // skip escape char
                    i++;
                    break;
                }
                // backspace
                case 'b':
                {
                    res += "\b";
                    // skip escape char
                    i++;
                    break;
                }
                // formfeed
                case 'f':
                {
                    res += "\f";
                    // skip escape char
                    i++;
                    break;
                }
                // carriage return
                case 'r':
                {
                    res += "\r";
                    // skip escape char
                    i++;
                    break;
                }
                // \uFFFF & unknown escape strings
                default:
                {
                    // simply emit
                    res += val;
                    break;
                }
            }
        }
        else
        {
          res += input[i];
        }
    }

    return res;
}


//-----------------------------------------------------------------------------
void
base64_encode(const void *src,
              const index_t src_nbytes,
              void *dest)
{
    int nbytes = (int)src_nbytes;
    base64_encodestate enc_state;
    base64_init_encodestate(&enc_state);
    const char *src_ptr = (const char*)src;
    char *des_ptr       = (char*)dest;
    memset(des_ptr,0,(size_t)base64_encode_buffer_size(src_nbytes));

    int code_len = base64_encode_block(src_ptr,
                                       nbytes,
                                       des_ptr,
                                       &enc_state);
    des_ptr += code_len;
    code_len = base64_encode_blockend(des_ptr, &enc_state);
    des_ptr += code_len;

    // for some reason libb64 adds a newline
    des_ptr[-1] = 0;
}

//-----------------------------------------------------------------------------
index_t
base64_encode_buffer_size(const index_t src_nbytes)
{
     return  (4*src_nbytes) / 3 + 4 + 1;
}

//-----------------------------------------------------------------------------
index_t
base64_decode_buffer_size(const index_t encoded_nbytes)
{
    return (encoded_nbytes / 4) * 3 + 1;
}


//-----------------------------------------------------------------------------
void
base64_decode(const void *src,
              index_t src_nbytes,
              void *dest)
{
    base64_decodestate dec_state;
    int src_len = (int)src_nbytes;
    base64_init_decodestate(&dec_state);
    const char *src_ptr = (const char*)src;
    char *des_ptr = (char*)dest;
    base64_decode_block(src_ptr,
                        src_len,
                        des_ptr,
                        &dec_state);
}

//-----------------------------------------------------------------------------
bool
string_is_integer(const std::string &s)
{
    int v = -1;
    std::istringstream iss(s);
    iss >> v;

    return !iss.fail();
}


//-----------------------------------------------------------------------------
std::string
float64_to_string(float64 value)
{
    char buffer[64] = {0};
    snprintf(buffer,64,"%.15g",value);

    std::string res(buffer);

    // we check for inf or nan in string form.
    // std::isnan, isn't portable until c++11
    // http://stackoverflow.com/questions/570669/checking-if-a-double-or-float-is-nan-in-c

    // searching for 'n' covers inf and nan
    if(res.find('n') == std::string::npos &&
       res.find('.') == std::string::npos &&
       res.find('e') == std::string::npos )
    {
        res += ".0";
    }

    return res;
}

//-----------------------------------------------------------------------------
/// fmt style string formatting helpers
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
std::string
format(const std::string &pattern,
       const conduit::Node &args)
{
    if( !args.dtype().is_object() &&
        !args.dtype().is_list())
    {
        CONDUIT_ERROR("conduit::utils::format args Node must be "
                      << " an `object`, or `list`.\n"
                      << "Passed node type: "
                      << "`" << args.dtype().name() << "`.");
    }

    // if we have an object, we used named args for fmt
    // if we have an list, we don't use named args for fmt
    bool is_obj = args.dtype().is_object();

    conduit_fmt::dynamic_format_arg_store<conduit_fmt::format_context> store;
    conduit::NodeConstIterator itr  = args.children();

    while(itr.has_next())
    {
        const conduit::Node &curr = itr.next();
        switch(curr.dtype().id())
        {
            /* ints */
            case conduit::DataType::INT8_ID:
            {
                int8 val = curr.as_int8();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::INT16_ID:
            {
                int16 val = curr.as_int16();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::INT32_ID:
            {
                int32 val = curr.as_int32();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::INT64_ID:
            {
                int64 val = curr.as_int64();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            /* uints */
            case conduit::DataType::UINT8_ID:
            {
                uint8 val = curr.as_uint8();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::UINT16_ID:
            {
                uint16 val = curr.as_uint16();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::UINT32_ID:
            {
                uint32 val = curr.as_uint32();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::UINT64_ID:
            {
                uint64 val = curr.as_uint64();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            /* floats */
            case conduit::DataType::FLOAT32_ID:
            {
                float32 val = curr.as_float32();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::FLOAT64_ID:
            {
                float64 val = curr.as_float64();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            // string case
            case conduit::DataType::CHAR8_STR_ID:
            {
                std::string val = curr.as_string();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }

                break;
            }
            default:
            {
                // ERROR -- list, object, or empty
                CONDUIT_ERROR("conduit::utils::format does not support"
                              << " `object`, `list`, or `empty` Nodes"
                              << " as arguments.\n"
                              << "'" << itr.name() << "' type: "
                              << "`" << curr.dtype().name() << "`.");
            }
        }
    }

    std::string res = "";

    try
    {
        res = conduit_fmt::vformat(pattern,store);
    }
    catch(const std::runtime_error& re)
    {
        CONDUIT_ERROR("conduit::utils::format error: "
                      << "fmt error message:\n"
                      << re.what());
    }

    return res;
}

//-----------------------------------------------------------------------------
std::string
format(const std::string &pattern,
       const conduit::Node &maps,
       index_t map_index)
{
    // neg bounds check
    if(map_index < 0)
    {
         CONDUIT_ERROR("conduit::utils::format map_index must be positive "
                   << " (map_index = " << map_index <<  ")");
    }

    if( !maps.dtype().is_object() &&
        !maps.dtype().is_list())
    {
        CONDUIT_ERROR("conduit::utils::format maps Node must be "
                      << " an `object`, or `list`\n."
                      << " Passed node type: "
                      << "`" << maps.dtype().name() << "`.");
    }

    // if we have an object, we used named args for fmt
    // if we have an list, we don't use named args for fmt
    bool is_obj = maps.dtype().is_object();

    conduit_fmt::dynamic_format_arg_store<conduit_fmt::format_context> store;
    conduit::NodeConstIterator itr  = maps.children();
    while(itr.has_next())
    {
        const conduit::Node &curr = itr.next();
        // map bounds checks
        if(curr.dtype().is_list())
        {
            if(map_index >= curr.number_of_children())
            {
                 CONDUIT_ERROR("conduit::utils::format map_index "
                               << "(value = " << map_index  << ")"
                               << " for '" << itr.name() << "'"
                               << " list map entry "
                               << " is out of bounds."
                               << " Number of children = "
                               << curr.number_of_children()
                               << ". Valid range is [0,"
                               << curr.number_of_children() << ").");
            }
        }
        else if(curr.dtype().is_number())
        {
            if(map_index >= curr.dtype().number_of_elements())
            {
                 CONDUIT_ERROR("conduit::utils::format map_index "
                               << "(value = " << map_index  << ")"
                               << " for '" << itr.name() << "'"
                               << " array map entry "
                               << " is out of bounds."
                               << " Number of elements = "
                               << curr.dtype().number_of_elements()
                               << ". Valid range is [0,"
                               << curr.dtype().number_of_elements() << ").");
            }
        }

        switch(curr.dtype().id())
        {
            /* ints */
            case conduit::DataType::INT8_ID:
            {
                int8 val = curr.as_int8_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::INT16_ID:
            {
                int16 val = curr.as_int16_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::INT32_ID:
            {
                int32 val = curr.as_int32_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::INT64_ID:
            {
                int64 val = curr.as_int64_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            /* uints */
            case conduit::DataType::UINT8_ID:
            {
                uint8 val = curr.as_uint8_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::UINT16_ID:
            {
                uint16 val = curr.as_uint16_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::UINT32_ID:
            {
                uint32 val = curr.as_uint32_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::UINT64_ID:
            {
                uint64 val = curr.as_uint64_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            /* floats */
            case conduit::DataType::FLOAT32_ID:
            {
                float32 val = curr.as_float32_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            case conduit::DataType::FLOAT64_ID:
            {
                float64 val = curr.as_float64_ptr()[map_index];
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }
                break;
            }
            // support lists of strings ONLY ...
            case conduit::DataType::LIST_ID:
            {
                const Node &lst_ent = curr[map_index];
                if(!lst_ent.dtype().is_string())
                {
                    CONDUIT_ERROR("conduit::utils::format (maps) only supports "
                                  << " the list maps case for strings."
                                  << "'" << itr.name() << "' entry at index "
                                  << map_index << " type: "
                                   << "`" << lst_ent.dtype().name() << "`.");
                }

                std::string val = lst_ent.as_string();
                if(is_obj)
                {
                    store.push_back(conduit_fmt::arg(itr.name().c_str(),
                                                     val));
                }
                else
                {
                    store.push_back(val);
                }

                break;
            }
            default:
            {
                // ERROR --  object, string, or empty
                CONDUIT_ERROR("conduit::utils::format (maps) does not support"
                              << " `object`, `string, or `empty` Nodes"
                              << " as arguments."
                              << "'" << itr.name() << "' type: "
                              << "`" << curr.dtype().name() << "`.");
            }
        }
    }

    std::string res = "";

    try
    {
        res = conduit_fmt::vformat(pattern,store);
    }
    catch(const std::runtime_error& re)
    {
        CONDUIT_ERROR("conduit::utils::format error: "
                      << "fmt error message:\n"
                      << re.what());
    }

    return res;
}

//-----------------------------------------------------------------------------
uint64
memory_usage()
{

#if defined(CONDUIT_PLATFORM_WINDOWS)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        return static_cast<uint64>(pmc.WorkingSetSize / 1024);
    }
#elif defined(CONDUIT_PLATFORM_APPLE)
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    kern_return_t ret = task_info(mach_task_self(),
                                  MACH_TASK_BASIC_INFO,
                                  (task_info_t)&info, &count);

    if (ret == KERN_SUCCESS)
    {
        return static_cast<uint64>(info.resident_size / 1024);
    }

#else //linux, unix, etc
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line))
    {
        if (line.find("VmRSS") != std::string::npos)
        {
            size_t pos = line.find(":");
            return static_cast<uint64>(std::stoll(line.substr(pos + 1)));
        }
    }
#endif

    return 0;
}

//-----------------------------------------------------------------------------
// String hash functions
//-----------------------------------------------------------------------------
namespace hashing
{
// NOTE: Borrowed from VisIt.

// ****************************************************************************
//  Function: Hash
//
//  Purpose:
//      Hash a variable length stream of bytes into a 32-bit value.
//
//      Can also be used effectively as a checksum.
//
//      The best hash table sizes are powers of 2.  There is no need to do
//      mod a prime (mod is sooo slow!).  If you need less than 32 bits,
//      use a bitmask.  For example, if you need only 10 bits, do
//        h = (h & BJHashmask(10));
//        In which case, the hash table should have hashsize(10) elements.
//
//        If you are hashing n strings (unsigned char **)k, do it like this:
//          for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
//
//  Arguments:
//    k:       the key ((the unaligned variable-length array of bytes)
//    length:  the length of the key, in bytes
//    initval: can be any 4-byte value
//
//  Returns:  A 32-bit value.  Every bit of the key affects every bit of
//  the return value.  Every 1-bit and 2-bit delta achieves avalanche.
//
//  Programmer: By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.
//
//  You may use this code any way you wish, private, educational, or
//  commercial.  It's free. However, do NOT use for cryptographic purposes.
//
//  See http://burtleburtle.net/bob/hash/evahash.html
// ****************************************************************************

#define bjhash_mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

inline unsigned int Hash(const unsigned char *k, unsigned int length, unsigned int initval)
{
   unsigned int a,b,c,len;

   len = length;
   a = b = 0x9e3779b9;
   c = initval;

   while (len >= 12)
   {
      a += (k[0] +((unsigned int)k[1]<<8) +((unsigned int)k[2]<<16) +((unsigned int)k[3]<<24));
      b += (k[4] +((unsigned int)k[5]<<8) +((unsigned int)k[6]<<16) +((unsigned int)k[7]<<24));
      c += (k[8] +((unsigned int)k[9]<<8) +((unsigned int)k[10]<<16)+((unsigned int)k[11]<<24));
      bjhash_mix(a,b,c);
      k += 12; len -= 12;
   }

   c += length;

   switch(len)
   {
      case 11: c+=((unsigned int)k[10]<<24); /* FALLTHROUGH */
      case 10: c+=((unsigned int)k[9]<<16); /* FALLTHROUGH */
      case 9 : c+=((unsigned int)k[8]<<8); /* FALLTHROUGH */
      case 8 : b+=((unsigned int)k[7]<<24); /* FALLTHROUGH */
      case 7 : b+=((unsigned int)k[6]<<16); /* FALLTHROUGH */
      case 6 : b+=((unsigned int)k[5]<<8); /* FALLTHROUGH */
      case 5 : b+=k[4]; /* FALLTHROUGH */
      case 4 : a+=((unsigned int)k[3]<<24); /* FALLTHROUGH */
      case 3 : a+=((unsigned int)k[2]<<16); /* FALLTHROUGH */
      case 2 : a+=((unsigned int)k[1]<<8); /* FALLTHROUGH */
      case 1 : a+=k[0];
   }

   bjhash_mix(a,b,c);

   return c;
}

// Just to keep this macro from leaking out and polluting the global namespace
#undef bjhash_mix

/**
 Take the uint8 stream and hash it forwards and backwards and OR the results
 together into an int64.

 @note If the above Hash function was adapted to go forwards or backwards then
       we could use it instead.
 */
inline uint64
hash_uint8(const uint8 *data, index_t n)
{
    uint32 hashF = 0;

    // Build the length into the hash so {1} and {0,1} hash to different values.
    const auto ldata = reinterpret_cast<const uint8 *>(&n);
    for(size_t e = 0; e < sizeof(n); e++)
    {
      hashF += ldata[e];
      hashF += hashF << 10;
      hashF ^= hashF >> 6;
    }
    // hash the data forward and backwards.
    uint32 hashB = hashF;
    for(index_t i = 0; i < n; i++)
    {
        hashF += data[i];
        hashF += hashF << 10;
        hashF ^= hashF >> 6;

        hashB += data[n - 1 - i];
        hashB += hashB << 10;
        hashB ^= hashB >> 6;
    }
    hashF += hashF << 3;
    hashF ^= hashF >> 11;
    hashF += hashF << 15;

    hashB += hashB << 3;
    hashB ^= hashB >> 11;
    hashB += hashB << 15;

    // Combine the forward, backward into a uint64.
    return (static_cast<uint64>(hashF) << 32) | static_cast<uint64>(hashB);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::utils::hashing --
//-----------------------------------------------------------------------------

unsigned int
hash(const char *k, unsigned int length, unsigned int initval)
{
    return hashing::Hash((unsigned char const*)k, length, initval);
}

unsigned int
hash(const char *k, unsigned int initval)
{
    return hashing::Hash((unsigned char const*)k,
                         (unsigned int)strlen(k), initval);
}

unsigned int
hash(const std::string &k, unsigned int initval)
{
    return hashing::Hash((unsigned char const*)k.c_str(),
                         (unsigned int)k.size(), initval);
}

uint64
hash(const index_t *k, unsigned int length)
{
    return hashing::hash_uint8(reinterpret_cast<const uint8 *>(k),
                               length * sizeof(index_t));
}

uint64
hash(const float64 *k, unsigned int length)
{
    return hashing::hash_uint8(reinterpret_cast<const uint8 *>(k),
                               length * sizeof(float64));
}

uint64
hash(const uint64 *k, unsigned int length)
{
    return hashing::hash_uint8(reinterpret_cast<const uint8 *>(k),
                               length * sizeof(uint64));
}

//---------------------------------------------------------------------------
std::vector<conduit::index_t> factor(conduit::index_t num)
{
   std::vector<conduit::index_t> factors;
   conduit::index_t prod = 1;
   conduit::index_t n = num;
   for (conduit::index_t f = 2; f <= num && prod < num; f++)
   {
       while (n % f == 0)
       {
           factors.push_back(f);
           n /= f;
           prod *= f;
       }
   }
   if (factors.empty())
   {
       factors.push_back(1);
       if (num > 1)
           factors.push_back(num);
   }
   return factors;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::utils --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


