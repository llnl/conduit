// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_handle_sidre.cpp
///
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    // FIXME:
    #include "conduit_relay_io_handle.hpp"
#else
    #include "conduit_relay_io_handle.hpp"
#endif

#include "conduit_relay_io.hpp"
#include "conduit_relay_io_handle.hpp"
#include "conduit_relay_io_identify_protocol.hpp"
#include "conduit_relay_io_handle_sidre.hpp"

#include "conduit_fmt/conduit_fmt.h"

//-----------------------------------------------------------------------------
// standard lib includes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay --
//-----------------------------------------------------------------------------
namespace relay
{

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
//-----------------------------------------------------------------------------
namespace mpi
{
#endif

//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::io --
//-----------------------------------------------------------------------------
namespace io
{

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// SidreIOHandle Implementation
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
SidreIOHandle::SidreIOHandle(const std::string &path,
                         const std::string &protocol,
                         const Node &options)
: HandleInterface(path,protocol,options)
{
    // empty
}

//-----------------------------------------------------------------------------
SidreIOHandle::~SidreIOHandle()
{
    close();
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::open()
{
    close();

    // call base class method, which does final sanity checks
    // and processes standard options (mode = "rw", etc)
    HandleInterface::open();

    if( open_mode_write_only() )
        CONDUIT_ERROR("SidreIOHandle does not support write mode "
                      "(open_mode = 'w')");

    // two cases,
    //  a standalone file with a sidre style hierarchy
    //  a collection of files with a spio generated root

    // the root file will be what is passed to path
    m_root_file = path();

    if( !utils::is_file( m_root_file ) )
        CONDUIT_ERROR("Invalid sidre file: " << m_root_file);

    m_root_protocol = detect_root_protocol();

    //
    m_root_handle.open(m_root_file, m_root_protocol);

    // check the simple case
    if(m_root_handle.has_path("sidre"))
    {
        // in this case, we only use the root handle
        // and sidre meta 0
        m_has_spio_index = false;
        m_num_trees = 0;
        m_num_files = 0;
        m_file_pattern = "";
        m_tree_pattern = "";
    }
    else
    {
        // check for standard sidre root file entries, we need:
        // file_pattern, tree_pattern, number_of_trees, number_of_files, protocol

        if(!m_root_handle.has_path("file_pattern"))
            CONDUIT_ERROR("Sidre root file missing entry: file_pattern")

        if(!m_root_handle.has_path("tree_pattern"))
            CONDUIT_ERROR("Sidre root file missing entry: tree_pattern")

        if(!m_root_handle.has_path("number_of_trees"))
            CONDUIT_ERROR("Sidre root file missing entry: number_of_trees")

        if(!m_root_handle.has_path("number_of_files"))
            CONDUIT_ERROR("Sidre root file missing entry: number_of_files")

        if(!m_root_handle.has_path("protocol"))
            CONDUIT_ERROR("Sidre root file missing entry: protocol")

        // read the standard entries
        Node root_info;
        m_root_handle.read("file_pattern",root_info["file_pattern"]);
        m_root_handle.read("tree_pattern",root_info["tree_pattern"]);
        m_root_handle.read("number_of_trees",root_info["number_of_trees"]);
        m_root_handle.read("number_of_files",root_info["number_of_files"]);
        m_root_handle.read("protocol", root_info["protocol"]);

        m_num_trees = root_info["number_of_trees"].to_int();
        m_num_files = root_info["number_of_files"].to_int();

        m_file_pattern = root_info["file_pattern"].as_string();
        m_tree_pattern = root_info["tree_pattern"].as_string();

        // read protocol/name to obtain correct protocol for data
        // we expect a string like the following:
        //    sidre_zzz
        // where zzz is that value we want as the file protocol

        m_file_protocol = root_info["protocol/name"].as_string();
        std::string curr,next;
        utils::split_string(m_file_protocol,"_",curr,next);
        m_file_protocol = next;

        m_has_spio_index = true;
    }
    m_open = true;
}


//-----------------------------------------------------------------------------
bool
SidreIOHandle::is_open() const
{
    return m_open;
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::read(Node &node)
{
    Node opts;
    read(node,opts);
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::read(Node &node,
                    const Node &opts)
{
    CONDUIT_UNUSED(opts);
    // note: wrong mode errors are handled before dispatch to interface

    std::vector<std::string> child_names;
    list_child_names(child_names);

    for(size_t i=0;i<child_names.size();i++)
    {
        read(child_names[i],node[child_names[i]]);
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::read(const std::string &path,
                    Node &node)
{
    Node opts;
    read(path,node,opts);
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::read(const std::string &path,
                    Node &node,
                    const Node &opts)
{
    CONDUIT_UNUSED(opts);
    // note: wrong mode errors are handled before dispatch to interface

    // if blank path or "/", use other method and early exist.
    if(path.empty() || path == "/")
    {
        read(node);
        return;
    }

    // split the path to find out what we need to read
    // "root" =>> read_from_root(path,node)
    // tree_id (number) ==> read_from_sidre_tree(tree_id,path)

    std::string p_first;
    std::string p_next;
    utils::split_path(path,p_first,p_next);

    if(m_has_spio_index)
    {
        if(p_first == "root")
        {
            read_from_root(p_next,node);
        }
        else
        {
            if(!utils::string_is_integer(p_first))
            {
                CONDUIT_ERROR("Cannot read Sidre path: '"
                              << p_first
                              << "'"
                              << std::endl
                              << "Expected 'root' or an integer "
                              << "tree id (ex: '0')");
            }

            int tree_id = utils::string_to_value<int>(p_first);

            // make sure we have a valid tree_id
            if(tree_id < 0 || tree_id > m_num_trees)
            {
                CONDUIT_ERROR("Cannot read from invalid Sidre tree id: "
                              << tree_id
                              << std::endl
                              << "Expected id in range [0,"
                              << m_num_trees << ")");
            }

            read_from_sidre_tree(tree_id,
                                 p_next,
                                 node);
        }
    }
    else //
    {
        // we need to prep sidre meta ...

        read_from_sidre_tree(m_root_handle,
                             "",
                             path,
                             m_sidre_meta[0],
                             node);
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::write(const Node & /*node*/) // node is unused
{
    // note: wrong mode errors are handled before dispatch to interface

    // not supported yet, so throw a fatal error
    CONDUIT_ERROR("IOHandle: sidre write support not implemented");
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::write(const Node & /*node*/, // node is unused
                     const Node & /*opts*/) // opts is unused
{
    // note: wrong mode errors are handled before dispatch to interface

    // not supported yet, so throw a fatal error
    CONDUIT_ERROR("IOHandle: sidre write support not implemented");
}


//-----------------------------------------------------------------------------
void
SidreIOHandle::write(const Node & /*node*/, // node is unused
                   const std::string & /*path*/ ) // path is unused
{
    // note: wrong mode errors are handled before dispatch to interface

    // not supported yet, so throw a fatal error
    CONDUIT_ERROR("IOHandle: sidre write support not implemented");

}

//-----------------------------------------------------------------------------
void
SidreIOHandle::write(const Node & /*node*/, // node is unused
                   const std::string & /*path*/, // path is unused
                   const Node & /*opts*/) // opts is unused
{
    // note: wrong mode errors are handled before dispatch to interface

    // not supported yet, so throw a fatal error
    CONDUIT_ERROR("IOHandle: sidre write support not implemented");

}

//-----------------------------------------------------------------------------
void
SidreIOHandle::list_child_names(std::vector<std::string> &res)
{
    // note: wrong mode errors are handled before dispatch to interface

    if(m_has_spio_index)
    {
        // root case of the file name populate with root + tree_ids
        res.clear();
        res.push_back("root");

        std::ostringstream oss;
        for(int i=0;i<m_num_trees;i++)
        {
            oss.str("");
            oss << i;
            res.push_back(oss.str());
        }
    }
    else
    {
        // we use tree id zero for non index case
        sidre_meta_tree_list_child_names(0,"",res);
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::list_child_names(const std::string &path,
                                std::vector<std::string> &res)
{
    // note: wrong mode errors are handled before dispatch to interface

    // note: if the path is bad, we return an empty list
    res.clear();

    if(m_has_spio_index)
    {
        std::string p_first;
        std::string p_next;
        utils::split_path(path,p_first,p_next);

        if(p_first == "root")
        {
            if(p_next.empty())
            {
                m_root_handle.list_child_names(res);
            }
            else
            {
                m_root_handle.list_child_names(p_next,res);
            }
        }
        else
        {
            if(utils::string_is_integer(p_first))
            {
                int tree_id = utils::string_to_value<int>(p_first);
                // make sure tree_id is valid
                if(tree_id >= 0 && tree_id < m_num_trees )
                {
                    sidre_meta_tree_list_child_names(tree_id,p_next,res);
                }
            }
        }
    }
    else
    {
        // we use tree id zero for non index case
        sidre_meta_tree_list_child_names(0,path,res);
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::remove(const std::string &/*path*/) // path is unused
{
    // note: wrong mode errors are handled before dispatch to interface

    // not supported yet, so throw a fatal error
    CONDUIT_ERROR("IOHandle: sidre write support not implemented");
}

//-----------------------------------------------------------------------------
bool
SidreIOHandle::has_path(const std::string &path)
{
    // note: wrong mode errors are handled before dispatch to interface

    bool res = false;

    if(m_has_spio_index)
    {
        std::string p_first;
        std::string p_next;
        utils::split_path(path,p_first,p_next);

        if(p_first == "root")
        {
            if(p_next.empty())
            {
                res = true;
            }
            else
            {
                res = m_root_handle.has_path(p_next);
            }
        }
        else
        {
            if(!utils::string_is_integer(p_first))
            {
                res = false;
            }
            else
            {
                int tree_id = utils::string_to_value<int>(p_first);
                // make sure tree_id is valid
                if(tree_id >= 0 && tree_id < m_num_trees )
                {
                    if(p_next.empty())
                    {
                        res = true;
                    }
                    else
                    {
                        res = sidre_meta_tree_has_path(tree_id,p_next);
                    }
                }
                else
                {
                    res = false;
                }
            }
        }
    }
    else
    {
        // we use tree id zero for non index case
        sidre_meta_tree_has_path(0,path);
    }

    return res;
}


//-----------------------------------------------------------------------------
void
SidreIOHandle::flush()
{
    // Sidre IO Handle currently only supports read only use cases
    // flush is a no-op
}


//-----------------------------------------------------------------------------
void
SidreIOHandle::close()
{
    m_open = false;
    m_root_handle.close();

    //
    // TODO: clear should call close when handles are destructed ...
    // double check

    m_file_handles.clear();
    m_sidre_meta.clear();
}

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
std::string
SidreIOHandle::root_file_directory() const
{
    std::string curr, next;
    utils::rsplit_file_path(m_root_file, curr, next);
    return next;
}


//-------------------------------------------------------------------//
std::string
SidreIOHandle::detect_root_protocol() const
{
    //
    // TODO: detect underlying root file type --
    //  could be hdf5, json, or yaml
    std::string ftype;
    identify_file_type(path(),ftype);
    return ftype;
}

//-------------------------------------------------------------------//
// adapted from VisIt, avtBlueprintTreeCache
//              ascent, hola, BlueprintTreePathGenerator
std::string
SidreIOHandle::expand_pattern(const std::string pattern,
                              int idx) const
{
    //
    // This currently handles format strings:
    // "%d" "%02d" "%03d" "%04d" "%05d" "%06d" "%07d" "%08d" "%09d"
    //

    std::size_t pattern_idx = pattern.find("%d");

    if(pattern_idx != std::string::npos)
    {
        std::string res = pattern;
        res.replace(pattern_idx,
                    4,
                    conduit_fmt::format("{:d}",idx));
        return res;
    }

    for(int i=2; i<10; i++)
    {
        std::string pat = "%0" + conduit_fmt::format("{:d}",i) + "d";
        pattern_idx = pattern.find(pat);

        if(pattern_idx != std::string::npos)
        {
            pat = "{:0" + conduit_fmt::format("{:d}",i) + "d}";

            std::string res = pattern;
            res.replace(pattern_idx,
                        4,
                        conduit_fmt::format(pat,idx));
            return res;
        }
    }

    return pattern;
}


//-------------------------------------------------------------------//
// adapted from VisIt, avtBlueprintTreeCache
int
SidreIOHandle::generate_file_id_for_tree(int tree_id) const
{
    int file_id = -1;

    if(m_num_trees == m_num_files)
    {
        file_id = tree_id;
    }
    else if(m_num_files == 1)
    {
        file_id = 0;
    }
    else
    {
        // TODO: Cache the d2f_map
        Node d2f_map;
        generate_domain_to_file_map(m_num_trees,
                                    m_num_files,
                                    d2f_map);
        int32_array v_domain_to_file = d2f_map["global_domain_to_file"].value();
        file_id = v_domain_to_file[tree_id];
    }

    return file_id;
}

//-------------------------------------------------------------------//
// adapted from VisIt, avtBlueprintTreeCache
std::string
SidreIOHandle::generate_file_path(int tree_id) const
{
    // TODO: Map for tree_ids to file_ids?
    int file_id = generate_file_id_for_tree(tree_id);
    return utils::join_file_path(root_file_directory(),
                                 expand_pattern(m_file_pattern,file_id));
}

//-------------------------------------------------------------------//
// adapted from VisIt, avtBlueprintTreeCache
std::string
SidreIOHandle::generate_tree_path(int tree_id) const
{
    // the tree path should always end in a /
    std::string res = expand_pattern(m_tree_pattern,tree_id);
    if( (res.size() > 0) && (res[res.size()-1] != '/') )
    {
        res += "/";
    }
    return res;
}

//-----------------------------------------------------------------------------
// This uses a mapping scheme created by ascent + conduit
// Note: We will support explicit maps from the bp index in the future.
// TODO: We should cache this, not regen every fetch
//-----------------------------------------------------------------------------
// adapted from VisIt, avtBlueprintTreeCache
//-----------------------------------------------------------------------------
void
SidreIOHandle::generate_domain_to_file_map(int num_domains,
                                         int num_files,
                                         Node &out) const
{
    int num_domains_per_file = num_domains / num_files;
    int left_overs = num_domains % num_files;

    out["global_domains_per_file"].set(DataType::int32(num_files));
    out["global_domain_offsets"].set(DataType::int32(num_files));
    out["global_domain_to_file"].set(DataType::int32(num_domains));

    int32_array v_domains_per_file = out["global_domains_per_file"].value();
    int32_array v_domains_offsets  = out["global_domain_offsets"].value();
    int32_array v_domain_to_file   = out["global_domain_to_file"].value();

    // setup domains per file
    for(int f=0; f < num_files; f++)
    {
        v_domains_per_file[f] = num_domains_per_file;
        if( f < left_overs)
            v_domains_per_file[f]+=1;
    }

    // prefix sum to calc offsets
    for(int f=0; f < num_files; f++)
    {
        v_domains_offsets[f] = v_domains_per_file[f];
        if(f > 0)
            v_domains_offsets[f] += v_domains_offsets[f-1];
    }

    // do assignment, create simple map
    int f_idx = 0;
    for(int d=0; d < num_domains; d++)
    {
        if(d >= v_domains_offsets[f_idx])
            f_idx++;
        v_domain_to_file[d] = f_idx;
    }
}

//---------------------------------------------------------------------------//
// Helper that expands a sidre groups's meta path to the expected file path
//---------------------------------------------------------------------------//
std::string
SidreIOHandle::generate_sidre_meta_group_path(const std::string &tree_path)
{
    // sidre/groups/ga/groups/gb/groups/gc/.../groups/gn

    std::ostringstream oss;

    std::string t_path  = tree_path;
    std::string t_curr;
    std::string t_next;

    while(t_path != "")
    {
        conduit::utils::split_path(t_path,
                                   t_curr,
                                   t_next);

        oss << "groups/" << t_curr;
        if( t_next != "")
        {
            oss << "/";
        }

        t_path = t_next;

    }

    return oss.str();

}

//---------------------------------------------------------------------------//
// Helper that expands a sidre view's meta path to the expected file path
//---------------------------------------------------------------------------//
std::string
SidreIOHandle::generate_sidre_meta_view_path(const std::string &tree_path)
{
    // sidre/groups/ga/groups/gb/groups/gc/.../views/gn

    std::ostringstream oss;

    std::string t_path  = tree_path;
    std::string t_curr;
    std::string t_next;

    while(t_path != "")
    {
        conduit::utils::split_path(t_path,
                                   t_curr,
                                   t_next);
        if(t_next == "")
        {
            oss << "views/" << t_curr;
        }
        else
        {
            oss << "groups/" << t_curr << "/";
        }
        t_path = t_next;
    }

    return oss.str();
}

//----------------------------------------------------------------------------/
bool
SidreIOHandle::sidre_meta_tree_has_path(int tree_id,
                                        const std::string &path)
{
    prepare_sidre_meta_tree(tree_id,path);
    Node &sidre_meta = m_sidre_meta[tree_id];
    return sidre_meta_tree_has_path(sidre_meta,path);
}


//----------------------------------------------------------------------------/
bool
SidreIOHandle::sidre_meta_tree_has_path(const Node &sidre_meta,
                                        const std::string &path)
{
    std::string sidre_mtree_group = generate_sidre_meta_group_path(path);
    std::string sidre_mtree_view  = generate_sidre_meta_view_path(path);

    return sidre_meta.has_path(sidre_mtree_group) ||
           sidre_meta.has_path(sidre_mtree_view);

}


//----------------------------------------------------------------------------/
void
SidreIOHandle::sidre_meta_tree_list_child_names(const Node &sidre_meta,
                                                const std::string &path,
                                                std::vector<std::string> &res)
{
    res.clear();

    std::string sidre_mtree_group = generate_sidre_meta_group_path(path);

    // empty path means we are checking for read at root of hierarchy
    if(sidre_mtree_group == "")
    {
        if(sidre_meta.has_child("groups"))
        {
            // group case, enum groups then views
            NodeConstIterator g_itr = sidre_meta["groups"].children();
            while(g_itr.has_next())
            {
                g_itr.next();
                res.push_back(g_itr.name());
            }
        }

        if(sidre_meta.has_child("views"))
        {
            NodeConstIterator v_itr = sidre_meta["views"].children();
            while(v_itr.has_next())
            {
                v_itr.next();
                res.push_back(v_itr.name());
            }
        }
    }
    else if(sidre_meta.has_path(sidre_mtree_group))
    {
        // group case, enum groups then views
        if(sidre_meta[sidre_mtree_group].has_child("groups"))
        {
            NodeConstIterator g_itr = sidre_meta[sidre_mtree_group]["groups"].children();
            while(g_itr.has_next())
            {
                g_itr.next();
                res.push_back(g_itr.name());
            }
        }

        if(sidre_meta[sidre_mtree_group].has_child("views"))
        {
            NodeConstIterator v_itr = sidre_meta[sidre_mtree_group]["views"].children();
            while(v_itr.has_next())
            {
                v_itr.next();
                res.push_back(v_itr.name());
            }
        }
    }
}



//----------------------------------------------------------------------------/
void
SidreIOHandle::sidre_meta_tree_list_child_names(int tree_id,
                                                const std::string &path,
                                                std::vector<std::string> &res)
{
    res.clear();
    // this will throw an error if we try to access a bad path
    prepare_sidre_meta_tree(tree_id,path);
    sidre_meta_tree_list_child_names(m_sidre_meta[tree_id],
                                     path,
                                     res);
}

//----------------------------------------------------------------------------/
void
SidreIOHandle::load_sidre_tree(Node &sidre_meta,
                               IOHandle &hnd,
                               const std::string &tree_prefix,
                               const std::string &tree_path,
                               const std::string &curr_path,
                               Node &out)
{
    // CONDUIT_INFO("load_sidre_tree w/ meta "
    //              << tree_prefix << " "
    //              << tree_path   << " "
    //              << curr_path);

    // we want to pull out a sub-tree of the sidre group hierarchy
    //
    // descend down to "tree_path" in sidre meta

    std::string tree_curr;
    std::string tree_next;
    conduit::utils::split_path(tree_path,tree_curr,tree_next);

    // root case for a data tree
    if( tree_curr.empty() )
    {
            // we have the correct sidre meta node, now read
            load_sidre_group(sidre_meta,
                             hnd,
                             tree_prefix,
                             "",
                             out);
    }
    else if( sidre_meta["groups"].has_path(tree_curr) )
    {
        // BP_PLUGIN_INFO(curr_path << tree_curr << " is a group");
        if(tree_next.size() == 0)
        {
            // we have the correct sidre meta node, now read
            load_sidre_group(sidre_meta["groups"][tree_curr],
                             hnd,
                             tree_prefix,
                             curr_path + tree_curr  + "/",
                             out);
        }
        else // keep descending
        {
            load_sidre_tree(sidre_meta["groups"][tree_curr],
                            hnd,
                            tree_prefix,
                            tree_next,
                            curr_path + tree_curr  + "/",
                            out);
        }
    }
    else if( sidre_meta["views"].has_path(tree_curr) )
    {
        // BP_PLUGIN_INFO(curr_path << tree_curr << " is a view");
        if(tree_next.size() != 0)
        {
            CONDUIT_ERROR("Sidre path extends beyond sidre view, "
                          "however Sidre views are leaves.");
        }
        else
        {
            load_sidre_view(sidre_meta["views"][tree_curr],
                            hnd,
                            tree_prefix,
                            curr_path + tree_curr  + "/",
                            out);
        }
    }
    else
    {
        CONDUIT_ERROR("sidre path " << curr_path
                                    << "/"
                                    << tree_curr
                                    << " does not exist");
    }
}

//----------------------------------------------------------------------------/
void
SidreIOHandle::load_sidre_group(Node &sidre_meta,
                                IOHandle &hnd,
                                const std::string &tree_prefix,
                                const std::string &group_path,
                                Node &out)
{
    // CONDUIT_INFO("load_sidre_group "
    //              << tree_prefix << " "
    //              << group_path);

    // load this group's children groups and views
    NodeIterator g_itr = sidre_meta["groups"].children();
    while(g_itr.has_next())
    {
        Node &g = g_itr.next();
        std::string g_name = g_itr.name();
        //BP_PLUGIN_INFO("loading " << group_path << g_name << " as group");
        std::string cld_path = group_path + g_name;
        load_sidre_group(g,
                         hnd,
                         tree_prefix,
                         cld_path + "/",
                         out[g_name]);
    }

    NodeIterator v_itr = sidre_meta["views"].children();
    while(v_itr.has_next())
    {
        Node &v = v_itr.next();
        std::string v_name = v_itr.name();
        // BP_PLUGIN_INFO("loading " << group_path << v_name << " as view");
        std::string cld_path = group_path + v_name;
        load_sidre_view(v,
                        hnd,
                        tree_prefix,
                        cld_path,
                        out[v_name]);
    }
}

//----------------------------------------------------------------------------/
void
SidreIOHandle::load_sidre_view(Node &sidre_meta_view,
                               IOHandle &hnd,
                               const std::string &tree_prefix,
                               const std::string &view_path,
                               Node &out)
{
    // CONDUIT_INFO("load_sidre_view " << view_path);

    // view load cases:
    //   the view is a scalar or string
    //     simply copy the "value" from the meta view
    //
    //   the view is attached to a buffer
    //     in this case we need to get the info about the buffer the view is
    //     attached to and read the proper slab of that buffer's hdf5 dataset
    //     into a new compact node.
    //
    //   the view is has external data
    //     for this case we can follow the "tree_path" in the sidre external
    //     data tree, and fetch the hdf5 dataset that was written there.
    //

    std::string view_state = sidre_meta_view["state"].as_string();

    if( view_state == "STRING")
    {
        //BP_PLUGIN_INFO("loading " << view_path << " as sidre string view");
        out.set(sidre_meta_view["value"]);
    }
    else if(view_state == "SCALAR")
    {
        // BP_PLUGIN_INFO("loading " << view_path << " as sidre scalar view");
        out.set(sidre_meta_view["value"]);
    }
    else if( view_state == "BUFFER" )
    {
        // BP_PLUGIN_INFO("loading " << view_path << " as sidre view linked to a buffer");
        // we need to fetch the buffer
        int buffer_id = sidre_meta_view["buffer_id"].to_int();

        std::ostringstream buffer_fetch_path_oss;
        buffer_fetch_path_oss << tree_prefix << "/sidre/buffers/buffer_id_" << buffer_id;

        // buffer data path
        std::string buffer_data_fetch_path   = buffer_fetch_path_oss.str() + "/data";

        // we also need the buffer's schema
        std::string buffer_schema_fetch_path = buffer_fetch_path_oss.str() + "/schema";

        Node n_buffer_schema_str;

        hnd.read(buffer_schema_fetch_path,n_buffer_schema_str);

        std::string buffer_schema_str = n_buffer_schema_str.as_string();
        Schema buffer_schema(buffer_schema_str);

        //BP_PLUGIN_INFO("sidre buffer schema: " << buffer_schema.to_json());
        //BP_PLUGIN_INFO("sidre buffer data path " << buffer_data_fetch_path);

        std::string view_schema_str = sidre_meta_view["schema"].as_string();
        // create the schema we want for this view
        // it describes how the view relates to the buffer in the hdf5 file

        Schema view_schema(view_schema_str);
        // BP_PLUGIN_INFO("sidre view schema: " << view_schema.to_json());

        // if the schema isn't compact, or if we are reading
        // less elements than the entire buffer,
        // we need to read a subset of the hdf5 dataset

        if(   !view_schema.is_compact() ||
            ( view_schema.dtype().number_of_elements() <
              buffer_schema.dtype().number_of_elements() )
          )
        {
            // BP_PLUGIN_INFO("Sidre View from Buffer Slab Fetch Case");
            //
            // Create a compact schema to describe our desired output data
            //
            Schema view_schema_compact;
            view_schema.compact_to(view_schema_compact);
            // setup and allocate the output node
            out.set(view_schema_compact);

            // TODO: Implement BUFFER-SLAB FETCH
            // ---------------------------------------------------------------
            // BUFFER-SLAB FETCH
            // ---------------------------------------------------------------
            //
            // we can use hdf5 slab fetch if the the dtype.id() of the buffer
            // and the view are the same.
            //
            //  otherwise, we will have to fetch the entire buffer since
            //  hdf5 doesn't support byte level striding.

            // if(
            //     tree_cache.Read(tree_id,
            //                     buffer_data_fetch_path,
            //                     view_schema.dtype(),
            //                     out)
            //     )
            // {
                // BP_PLUGIN_INFO("Sidre View from Buffer Slab Fetch Case Failed");
                // ---------------------------------------------------------------
                // Fall back to Non BUFFER-SLAB FETCH
                // ---------------------------------------------------------------
                // this reads the entire buffer to get the proper subset
                Node n_buff;
                Node n_view;

                hnd.read(buffer_data_fetch_path,n_buff);

                // create our view on the buffer
                n_view.set_external(view_schema,n_buff.data_ptr());
                // compact the view to our output
                n_view.compact_to(out);
            // }
            // else
            // {
            //     //BP_PLUGIN_INFO("Sidre View from Buffer Slab Fetch Case Successful");
            // }
        }
        else // compact, and compat, we can just read
        {
            hnd.read(buffer_data_fetch_path,out);
        }
    }
    else if( view_state == "EXTERNAL" )
    {
        //BP_PLUGIN_INFO("loading " << view_path << " as sidre external view");

        std::string fetch_path = tree_prefix + "sidre/external/" + view_path;

        // BP_PLUGIN_INFO("relay:io::hdf5_read "
        //                << "domain " << tree_id
        //                << " : "
        //                << fetch_path);

        hnd.read(fetch_path,out);;
    }
    else
    {
        // error:  "unsupported sidre view state: " << view_state );
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::read_from_root(const std::string &path, Node &node)
{
    // skip cache first
    if(!path.empty())
        m_root_handle.read(path,node);
    else
        m_root_handle.read(node);
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::prepare_file_handle(int tree_id)
{
    // find if the have the correct io handle open already
    int file_id = generate_file_id_for_tree(tree_id);
    if( m_file_handles.count(file_id) == 0 ||
        !m_file_handles[file_id].is_open() )
    {
        // CONDUIT_INFO("opening: " << generate_file_path(tree_id));
        // if not, open the handle
        m_file_handles[file_id].open(generate_file_path(tree_id));
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::prepare_sidre_meta_tree(IOHandle &hnd,
                                       const std::string &tree_prefix,
                                       const std::string &path,
                                       Node &sidre_meta)
{
    //CONDUIT_INFO("prepare_sidre_meta_tree "<< tree_prefix << " " << path);

    // check for read at root of the file, for this case we
    // need to read the entire sidre tree
    if(path.empty() || path == "/")
    {
        // only read the group + view structure, not buffers or external
        // since those aren't meta data (they are real data!)
        hnd.read( tree_prefix + "/sidre/groups",sidre_meta["groups"]);
        // TODO - are there ever views at the root, I don't recall?
    }
    else // subtree read
    {
        std::string sidre_mtree_view  = generate_sidre_meta_view_path(path);
        std::string sidre_mtree_group = generate_sidre_meta_group_path(path);

        // this path will either be a sidre group or a sidre view
        // check if either exists cached
        if( !sidre_meta.has_path(sidre_mtree_group) ||
            !sidre_meta.has_path(sidre_mtree_view) )
        {
            // CONDUIT_INFO("sidre meta not loaded yet "
            //             << sidre_mtree_group << " or " << sidre_mtree_view);
            // CONDUIT_INFO("check for " << tree_prefix + "sidre/" + sidre_mtree_group);
            // CONDUIT_INFO("check for " << tree_prefix + "sidre/" + sidre_mtree_view);

            // if not cached, we need to fetch
            // check to see if we have a group or view
            if( hnd.has_path(tree_prefix + "sidre/" + sidre_mtree_group) )
            {
                // we have a group, read the meta data
                hnd.read(tree_prefix + "sidre/" + sidre_mtree_group,
                         sidre_meta[sidre_mtree_group]);

            }
            else if( hnd.has_path(tree_prefix + "sidre/" + sidre_mtree_view) )
            {
                // we have a view, read the meta data
                 hnd.read(tree_prefix + "sidre/" + sidre_mtree_view,
                          sidre_meta[sidre_mtree_view]);
            }
            // the path is invalid, we don't throw an error here
            // because this method is also used to prepare sidre meta trees
            // for has_path and list_child_names cases, which don't throw
            // errors, but here are the details about what we would expect
            // for valid path:
            // {
            //     CONDUIT_ERROR("Failed to read tree path: " << std::endl
            //                 << "Expected to find Sidre Group: "
            //                 << tree_prefix << "sidre/" << sidre_mtree_group
            //                 << " or "
            //                 << "Sidre View: "
            //                 << tree_prefix << "sidre/" << sidre_mtree_view);
            // }
        }
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::prepare_sidre_meta_tree(int tree_id,
                                       const std::string &path)
{
    Node &sidre_meta = m_sidre_meta[tree_id];

    if(m_has_spio_index) // multi-tree with index case
    {
        prepare_file_handle(tree_id);
        int file_id = generate_file_id_for_tree(tree_id);
        // in this case, path needs to be augmented with the
        // tree prefix
        prepare_sidre_meta_tree(m_file_handles[file_id],
                                generate_tree_path(tree_id),
                                path,
                                sidre_meta);
    }
    else
    {
        prepare_sidre_meta_tree(m_root_handle,
                                "",
                                path,
                                sidre_meta);
    }
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::read_from_sidre_tree(IOHandle &hnd,
                                    const std::string &tree_prefix,
                                    const std::string &path,
                                    Node &sidre_meta,
                                    Node &out)
{
    // if we don't already have it cached, this will fetch
    // the proper sidre meta data
    prepare_sidre_meta_tree(hnd,
                            tree_prefix,
                            path,
                            sidre_meta);

    load_sidre_tree(sidre_meta,
                    hnd,
                    tree_prefix,
                    path,
                    "", // current path starts at root
                    out);
}

//-----------------------------------------------------------------------------
void
SidreIOHandle::read_from_sidre_tree(int tree_id,
                                    const std::string &path,
                                    Node &out)
{
    // if we don't already have it cached, this will fetch
    // the proper sidre meta data
    prepare_sidre_meta_tree(tree_id,path);

    if(m_has_spio_index)
    {

        // fetch the right file handle
        prepare_file_handle(tree_id);
        int file_id = generate_file_id_for_tree(tree_id);

        // CONDUIT_INFO("read_from_sidre_tree: "
        //               << " tree_id " << tree_id
        //               << " file_id " << file_id);

        // start a top level traversal
        // call load sidre variant that uses existing sidre meta tree
        Node &sidre_meta = m_sidre_meta[tree_id];
        load_sidre_tree(sidre_meta,
                        m_file_handles[file_id],
                        generate_tree_path(tree_id),
                        path,
                        "", // current path starts at root
                        out);
    }
    else
    {
        // start a top level traversal
        // call load sidre variant that uses existing sidre meta tree
        Node &sidre_meta = m_sidre_meta[tree_id];
        load_sidre_tree(sidre_meta,
                        m_root_handle,
                        generate_tree_path(tree_id),
                        path,
                        "", // current path starts at root
                        out);
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io --
//-----------------------------------------------------------------------------

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
}
//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
//-----------------------------------------------------------------------------
#endif

}
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
