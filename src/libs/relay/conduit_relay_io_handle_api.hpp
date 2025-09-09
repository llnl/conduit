// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_handle_api.hpp
///
//-----------------------------------------------------------------------------


#ifndef CONDUIT_RELAY_IO_HANDLE_API_HPP
#define CONDUIT_RELAY_IO_HANDLE_API_HPP


//-----------------------------------------------------------------------------
///
/// class: conduit::relay::{mpi}::io::IOHandle
///
/// Contract: Changes to backing (file on disk, etc) aren't guaranteed to
//  be reflected until a call to close
//-----------------------------------------------------------------------------
class CONDUIT_RELAY_API IOHandle
{

public:
     IOHandle();
    ~IOHandle();

    /// establish a handle
    void open(const std::string &path);

    void open(const std::string &path,
              const std::string &protocol);

    void open(const std::string &path,
              const Node &options);

    void open(const std::string &path,
              const std::string &protocol,
              const Node &options);

    /// check if the handle is currently open

    bool is_open() const;

    /// read contents starting at the root of the handle
    void read(Node &node);
    void read(Node &node, const Node &options);
    /// read contents starting at given subpath
    void read(const std::string &path,
              Node &node);
    void read(const std::string &path,
              Node &node,
              const Node &options);

    /// write contents of passed node to the root of the handle
    void write(const Node &node);
    void write(const Node &node, const Node &options);
    /// write contents of passed node to given subpath
    void write(const Node &node,
               const std::string &path);
    void write(const Node &node,
               const std::string &path,
               const Node &options);

    /// list child names at root of handle
    void list_child_names(std::vector<std::string> &res);
    /// list child names at subpath
    void list_child_names(const std::string &path,
                          std::vector<std::string> &res);

    /// flush i/o handle, semantics vary between concrete implementations
    void flush();

    // TODO: options variants for read and write above? with update of
    // above options with passed?

    /// remove contents at given path
    void remove(const std::string &path);

    /// check if given path exists
    bool has_path(const std::string &path);

    // FUTURE: also provide access to read schema
    // void read_schema(Schema &schema);
    // void read_schema(const std::string &path,
    //                  Schema &schema);

    /// close the handle
    void close();

    //-----------------------------------------------------------------------------
    // HandleInterface -- base class for all concrete IO Handle Implementations
    //-----------------------------------------------------------------------------
    class HandleInterface
    {
    public:

        HandleInterface(const std::string &path,
                        const std::string &protocol,
                        const Node &options);
        virtual ~HandleInterface();

        // main interface methods

        // note: make sure to call HandleInterface::open in derived class
        //       open() overrides
        virtual void open();
        virtual bool is_open() const = 0;
        virtual void read(Node &node) = 0;
        virtual void read(Node &node, const Node &options) = 0;
        virtual void read(const std::string &path,
                          Node &node) = 0;
        virtual void read(const std::string &path,
                          Node &node,
                          const Node &options) = 0;
        virtual void write(const Node &node) = 0;
        virtual void write(const Node &node, const Node &options) = 0;
        virtual void write(const Node &node,
                   const std::string &path) = 0;
        virtual void write(const Node &node,
                   const std::string &path,
                   const Node &options) = 0;
        virtual void list_child_names(std::vector<std::string> &res) = 0;
        virtual void list_child_names(const std::string &path,
                              std::vector<std::string> &res) = 0;
        virtual void flush() = 0;
        virtual void remove(const std::string &path) = 0;
        virtual bool has_path(const std::string &path) = 0;
        virtual void close() = 0;

        // access to common state
        const std::string &path()      const;
        const std::string &protocol()  const;
        const Node        &options()   const;
        const std::string &open_mode() const;

        bool              open_mode_append() const
                            { return m_open_mode_append;}
        bool              open_mode_truncate() const
                            { return m_open_mode_truncate;}

        bool              open_mode_read() const
                            { return m_open_mode_read;}
        bool              open_mode_write() const
                            { return m_open_mode_write;}

        bool              open_mode_read_only() const
                            { return m_open_mode_read && ! m_open_mode_write;}
        bool              open_mode_write_only() const
                            { return m_open_mode_write && ! m_open_mode_read;}

        // factory helper methods used by interface class
        static HandleInterface *create(const std::string &path);

        static HandleInterface *create(const std::string &path,
                                       const std::string &protocol);

        static HandleInterface *create(const std::string &path,
                                       const Node &options);

        static HandleInterface *create(const std::string &path,
                                       const std::string &protocol,
                                       const Node &options);

    private:
        std::string m_path;
        std::string m_protocol;
        std::string m_open_mode;
        Node        m_options;

        bool        m_open_mode_read;
        bool        m_open_mode_write;
        bool        m_open_mode_append;
        bool        m_open_mode_truncate;
    };

private:
    HandleInterface *m_handle;

};


#endif
