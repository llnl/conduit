// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_handle_sidre.inc.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// SidreIOHandle -- IO Handle implementation for Sidre Style Files
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class SidreIOHandle: public IOHandle::HandleInterface
{
public:
    SidreIOHandle(const std::string &path,
               const std::string &protocol,
               const Node &options);
    virtual ~SidreIOHandle();

    void open();

    bool is_open() const;

    // main interface methods
    void read(Node &node);
    void read(Node &node, const Node &opts);
    void read(const std::string &path,
              Node &node);
    void read(const std::string &path,
              Node &node,
              const Node &opts);

    void write(const Node &node);
    void write(const Node &node, const Node &opts);
    void write(const Node &node,
               const std::string &path);
    void write(const Node &node,
               const std::string &path,
               const Node &opts);

    void remove(const std::string &path);

    void list_child_names(std::vector<std::string> &res);
    void list_child_names(const std::string &path,
                          std::vector<std::string> &res);

    bool has_path(const std::string &path);

    void flush();
    void close();


private:

    std::string root_file_directory() const;
    std::string detect_root_protocol() const;

    std::string expand_pattern(const std::string pattern,
                               int idx) const;

    int         generate_file_id_for_tree(int tree_id) const;
    std::string generate_file_path(int tree_id) const;
    std::string generate_tree_path(int tree_id) const;
    void        generate_domain_to_file_map(int num_domains,
                                            int num_files,
                                            Node &out) const;


    // helpers to create sidre meta paths for a group or view
    static std::string generate_sidre_meta_group_path(const std::string &tree_path);
    static std::string generate_sidre_meta_view_path(const std::string &tree_path);

    static void read_from_sidre_tree(IOHandle &hnd,
                                     const std::string &tree_prefix,
                                     const std::string &path,
                                     Node &sidre_meta,
                                     Node &node);

    // basic sidre read logic that works at the handle level
    static void load_sidre_tree(Node &sidre_meta,
                                IOHandle &hnd,
                                const std::string &tree_prefix,
                                const std::string &tree_path,
                                const std::string &curr_path,
                                Node &out);

    static void load_sidre_group(Node &sidre_meta,
                                 IOHandle &hnd,
                                 const std::string &tree_prefix,
                                 const std::string &group_path,
                                 Node &out);

    static void load_sidre_view(Node &sidre_meta_view,
                                IOHandle &hnd,
                                const std::string &tree_prefix,
                                const std::string &view_path,
                                Node &out);

    bool sidre_meta_tree_has_path(const Node &sidre_meta,
                                  const std::string &path);

    void sidre_meta_tree_list_child_names(const Node &sidre_meta,
                                          const std::string &path,
                                          std::vector<std::string> &res);

    void read_from_root(const std::string &path,
                        Node &node);

    void read_from_sidre_tree(int tree_id,
                              const std::string &path,
                              Node &node);

    void prepare_file_handle(int tree_id);
    void prepare_sidre_meta_tree(int tree_id,
                                 const std::string &path);

    static void prepare_sidre_meta_tree(IOHandle &hnd,
                                       const std::string &tree_prefix,
                                       const std::string &path,
                                       Node &sidre_meta);
    bool sidre_meta_tree_has_path(int tree_id,
                                  const std::string &path);

    void sidre_meta_tree_list_child_names(int tree_id,
                                          const std::string &path,
                                          std::vector<std::string> &res);

    bool                     m_open;
    bool                     m_has_spio_index;

    int                      m_num_trees;
    int                      m_num_files;

    std::string              m_root_protocol;
    std::string              m_root_file;

    std::string              m_file_pattern;
    std::string              m_tree_pattern;
    std::string              m_file_protocol;

    // io handle used to interacte with the sidre root file
    IOHandle                 m_root_handle;

    // holds open I/O handles for each tree
    std::map<int,IOHandle>   m_file_handles;
    // holds cached sidre meta date for each tree
    std::map<int,Node>       m_sidre_meta;

};
