// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_blueprint.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay_io.hpp"
#include "conduit_relay_io_handle.hpp"
#include "conduit_blueprint.hpp"

#include "conduit_fmt/conduit_fmt.h"

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    #include "conduit_relay_mpi.hpp"
    #include "conduit_relay_mpi_io_blueprint.hpp"
    #include "conduit_blueprint_mpi.hpp"
    #ifdef CONDUIT_RELAY_IO_SILO_ENABLED
        #include "conduit_relay_mpi_io_silo.hpp"
    #endif
#else
    #include "conduit_relay_io_blueprint.hpp"
#endif

#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
    #include "conduit_relay_io_silo.hpp"
#endif

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
// Define an argument macro that adds the communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG) ,ARG
#else
// Define an argument macro that does not add the communicator argument.
#define CONDUIT_RELAY_COMMUNICATOR_ARG(ARG)
#endif

// std includes
#include <limits>
#include <set>

//-----------------------------------------------------------------------------
// standard lib includes
//-----------------------------------------------------------------------------
#include <iostream>


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
// -- begin conduit::relay::io
//-----------------------------------------------------------------------------
namespace io
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::io::<mpi>::blueprint
//-----------------------------------------------------------------------------
namespace blueprint
{

//-----------------------------------------------------------------------------
void gen_domain_to_file_map(index_t num_domains,
                            index_t num_files,
                            Node &out)
{
    index_t num_domains_per_file = num_domains / num_files;
    index_t left_overs = num_domains % num_files;

    out["global_domains_per_file"].set(DataType::index_t(num_files));
    out["global_domain_offsets"].set(DataType::index_t(num_files));
    out["global_domain_to_file"].set(DataType::index_t(num_domains));

    index_t_array v_domains_per_file = out["global_domains_per_file"].value();
    index_t_array v_domains_offsets  = out["global_domain_offsets"].value();
    index_t_array v_domain_to_file   = out["global_domain_to_file"].value();

    // setup domains per file
    for(index_t f=0; f < num_files; f++)
    {
        v_domains_per_file[f] = num_domains_per_file;
        if( f < left_overs)
            v_domains_per_file[f]+=1;
    }

    if(num_files > 1)
    {
        // prefix sum to calc offsets
        for(index_t f=1; f < num_files; f++)
        {
            v_domains_offsets[f] += (v_domains_per_file[f-1] +
                                     v_domains_offsets[f-1]);
        }
    }

    // do assignment, create simple map
    index_t f_idx = 0;
    for(index_t d=0; d < num_domains; d++)
    {
        // see if we are at the offset for the next file
        if(f_idx + 1 < num_files)
        {
            if(d >= v_domains_offsets[f_idx+1])
                f_idx++;
        }
        v_domain_to_file[d] = f_idx;
    }
}

//-----------------------------------------------------------------------------
// -- begin conduit::relay::<mpi>::blueprint::detail --
//-----------------------------------------------------------------------------


//
// Lots of helpers pulled in from Ascent for dealing with
// with mesh blueprint writing and reading.
// TODO: Could use cleanup.
//
namespace detail
{

class BlueprintPathGeneratorImpl
{
public:
    BlueprintPathGeneratorImpl()
    {}

    virtual ~BlueprintPathGeneratorImpl()
    {}

    virtual std::string GenerateFilePath(index_t tree_id) const =0;
    virtual std::string GenerateTreePath(index_t tree_id) const =0;
};


class BlueprintLegacyPathGenerator: public BlueprintPathGeneratorImpl
{
public:
    //-------------------------------------------------------------------//
    BlueprintLegacyPathGenerator(const std::string &file_pattern,
                                 const std::string &tree_pattern,
                                 index_t num_files,
                                 index_t num_trees,
                                 const std::string &protocol)
    : m_file_pattern(file_pattern),
      m_tree_pattern(tree_pattern),
      m_num_files(num_files),
      m_num_trees(num_trees),
      m_protocol(protocol)
    {
        // if we need domain to file map, gen it
        if( m_num_files > 1 && (m_num_trees != m_num_files) )
        {
            gen_domain_to_file_map(m_num_trees,
                                   m_num_files,
                                   m_d2f_map);
        }
    }

    //-------------------------------------------------------------------//
    virtual ~BlueprintLegacyPathGenerator()
    {

    }

    //-------------------------------------------------------------------//
    std::string Expand(const std::string pattern,
                       index_t idx) const
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
    virtual std::string GenerateFilePath(index_t tree_id) const
    {
        index_t file_id = -1;

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
            index_t_accessor v_d2f = m_d2f_map["global_domain_to_file"].value();
            file_id = v_d2f[tree_id];
        }

        return Expand(m_file_pattern,file_id);
    }

    //-------------------------------------------------------------------//
    virtual std::string GenerateTreePath(index_t tree_id) const
    {
        // the tree path should always end in a /
        std::string res = Expand(m_tree_pattern,tree_id);
        if( (res.size() > 0) && (res[res.size()-1] != '/') )
        {
            res += "/";
        }
        return res;
    }

private:
    std::string m_file_pattern;
    std::string m_tree_pattern;
    index_t     m_num_files;
    index_t     m_num_trees;
    std::string m_protocol;
    Node        m_d2f_map;
};

class BlueprintPartitonMapPathGenerator: public BlueprintPathGeneratorImpl
{
public:
    //-------------------------------------------------------------------//
    BlueprintPartitonMapPathGenerator(const std::string &part_pattern,
                                      const conduit::Node &part_map)
    : m_part_pattern(part_pattern),
      m_part_map(part_map),
      m_dom_to_tree()
    {
        // init the map that takes us from domain_id (what we want)
        // to part map entry

        index_t_accessor doms = part_map["domain"].value();
        index_t num_domains = doms.max() + 1;
        // NOTE: Most cases will be compact, but what about
        // cases that are not?
        m_dom_to_tree.set(DataType::index_t(num_domains));
        index_t_array dom_to_tree_vals = m_dom_to_tree.value();
        for(index_t i=0;i<num_domains;i++)
        {
            dom_to_tree_vals[doms[i]] = i;
        }
    }

    //-------------------------------------------------------------------//
    BlueprintPartitonMapPathGenerator(const std::string &part_pattern)
    : m_part_pattern(part_pattern),
      m_part_map(),
      m_dom_to_tree()
    {
        // empty
    }

    //-------------------------------------------------------------------//
    virtual ~BlueprintPartitonMapPathGenerator()
    {
        // empty
    }

    //-------------------------------------------------------------------//
    virtual std::string GenerateFullPath(index_t tree_id) const
    {
        if( m_part_map.number_of_children() == 0 )
        {
            // special case, not format sub needed
            return m_part_pattern;
        }
        else
        {
            index_t_array dom_to_tree_vals = m_dom_to_tree.value();
            index_t dom_lookup = dom_to_tree_vals[tree_id];
            return conduit::utils::format(m_part_pattern,
                                          m_part_map,
                                          dom_lookup);
        }
    }

    //-------------------------------------------------------------------//
    virtual std::string GenerateFilePath(index_t tree_id) const
    {
        std::string res,tmp;
        utils::split_string(GenerateFullPath(tree_id),
                            ":/",
                            res,  // result is before sep
                            tmp);
        return res;
    }

    //-------------------------------------------------------------------//
    virtual std::string GenerateTreePath(index_t tree_id) const
    {
        std::string res,tmp;
        utils::split_string(GenerateFullPath(tree_id),
                            ":/",
                            tmp,
                            res); // result is after sep
        // tree path should always end with "/"
        if( (res.size() > 0) && (res[res.size()-1] != '/') )
        {
            res += "/";
        }
        return res;

    }

private:
    std::string m_part_pattern;
    Node        m_part_map;
    // extra map for non trivial domain
    Node        m_dom_to_tree;
};


class BlueprintTreePathGenerator
{
public:

    BlueprintTreePathGenerator()
    : m_impl(nullptr)
    {

    }

    void Cleanup()
    {
        if(m_impl != nullptr)
        {
            delete m_impl;
            m_impl = nullptr;
        }
    }

    //-------------------------------------------------------------------//
    void Init(const std::string &file_pattern,
              const std::string &tree_pattern,
              index_t num_files,
              index_t num_trees,
              const std::string &protocol)
    {
        Cleanup();
        m_impl = new BlueprintLegacyPathGenerator(file_pattern,
                                                  tree_pattern,
                                                  num_files,
                                                  num_trees,
                                                  protocol);
    }

    //-------------------------------------------------------------------//
    void Init(const std::string   &part_pattern,
             const conduit::Node &part_map)
    {
        Cleanup();
        m_impl = new BlueprintPartitonMapPathGenerator(part_pattern,
                                                       part_map);
    }

    //-------------------------------------------------------------------//
    void Init(const std::string   &part_pattern)
    {
        Cleanup();
        m_impl = new BlueprintPartitonMapPathGenerator(part_pattern);
    }

    ~BlueprintTreePathGenerator()
    {
        Cleanup();
    }

    std::string GenerateFilePath(index_t tree_id) const
    {
        return m_impl->GenerateFilePath(tree_id);
    }

    std::string GenerateTreePath(index_t tree_id) const
    {
        return m_impl->GenerateTreePath(tree_id);
    }

private:
    BlueprintPathGeneratorImpl *m_impl;

};




bool global_someone_agrees(bool vote
                           CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
  bool agreement = vote;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
  int local_boolean = vote ? 1 : 0;
  int global_boolean;

  MPI_Allreduce((void *)(&local_boolean),
                (void *)(&global_boolean),
                1,
                MPI_INT,
                MPI_SUM,
                mpi_comm);

  if(global_boolean > 0)
  {
    agreement = true;
  }
  else
  {
    agreement = false;
  }
#endif
  return agreement;
}

bool quick_mesh_check(const conduit::Node &n)
{
    return n.has_child("topologies") &&
           n["topologies"].number_of_children() > 0;
}

// mfem needs these special fields so look for them
void check_for_attributes(const conduit::Node &input,
                          std::vector<std::string> &list)
{
  const index_t num_doms = input.number_of_children();
  std::set<std::string> specials;
  for(index_t d = 0; d < num_doms; ++d)
  {
    const conduit::Node &dom = input.child(d);
    if(dom.has_path("fields"))
    {
      const conduit::Node &fields = dom["fields"];
      std::vector<std::string> fnames = fields.child_names();
      for(size_t i = 0; i < fnames.size(); ++i)
      {
        if(fnames[i].find("_attribute") != std::string::npos)
        {
          specials.insert(fnames[i]);
        }
      }
    }
  }

  for(auto it = specials.begin(); it != specials.end(); ++it)
  {
    list.push_back(*it);
  }
}

void filter_fields(const conduit::Node &input,
                   conduit::Node &output,
                   std::vector<std::string> fields
                   CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
  // assume this is multi-domain
  //
  check_for_attributes(input, fields);

  const index_t num_doms = input.number_of_children();
  for(index_t d = 0; d < num_doms; ++d)
  {
    const conduit::Node &dom = input.child(d);
    conduit::Node &out_dom = output.append();
    for(size_t f = 0; f < fields.size(); ++f)
    {
      const std::string fname = fields[f];
      if(dom.has_path("fields/" + fname))
      {
        const std::string fpath = "fields/" + fname;
        out_dom[fpath].set_external(dom[fpath]);
        // check for topologies
        const std::string topo = dom[fpath + "/topology"].as_string();
        const std::string tpath = "topologies/" + topo;
        if(!out_dom.has_path(tpath))
        {
          out_dom[tpath].set_external(dom[tpath]);
          if(dom.has_path(tpath + "/grid_function"))
          {
            const std::string gf_name = dom[tpath + "/grid_function"].as_string();
            const std::string gf_path = "fields/" + gf_name;
            out_dom[gf_path].set_external(dom[gf_path]);
          }
          if(dom.has_path(tpath + "/boundary_topology"))
          {
            const std::string bname = dom[tpath + "/boundary_topology"].as_string();
            const std::string bpath = "topologies/" + bname;
            out_dom[bpath].set_external(dom[bpath]);
          }
        }
        // check for coord sets
        const std::string coords = dom[tpath + "/coordset"].as_string();
        const std::string cpath = "coordsets/" + coords;
        if(!out_dom.has_path(cpath))
        {
          out_dom[cpath].set_external(dom[cpath]);
        }
      }
    }
    if(dom.has_path("state"))
    {
      out_dom["state"].set_external(dom["state"]);
    }
  }

  const index_t num_out_doms = output.number_of_children();
  bool has_data = false;
  // check to see if this resulted in any data
  for(index_t d = 0; d < num_out_doms; ++d)
  {
    const conduit::Node &dom = output.child(d);
    if(dom.has_path("fields"))
    {
      index_t fsize = dom["fields"].number_of_children();
      if(fsize != 0)
      {
        has_data = true;
        break;
      }
    }
  }


#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    has_data = detail::global_someone_agrees(has_data,mpi_comm);
#else
    has_data = detail::global_someone_agrees(has_data);
#endif

  if(!has_data)
  {
    CONDUIT_ERROR("Relay: field selection resulted in no data."
                  "This can occur if the fields did not exist "
                  "in the simulation data or if the fields were "
                  "created as a result of a pipeline, but the "
                  "relay extract did not receive the result of "
                  "a pipeline");
  }

}

//---------------------------------------------------------------------------//
std::string
identify_protocol(const std::string &path)
{
    std::string file_path, obj_base;
    conduit::utils::split_file_path(path,
                                    std::string(":"),
                                    file_path,
                                    obj_base);

    std::string file_name_base, file_name_ext;
    conduit::utils::rsplit_string(file_path,
                                  std::string("."),
                                  file_name_ext,
                                  file_name_base);

    // default
    std::string io_type = "bin";

    // if hdf5 is supported, it should be the default for mesh i/o
    Node io_protos;
    relay::io::about(io_protos["io"]);
    if( io_protos["io/protocols/hdf5"].as_string() == "enabled" )
    {
      io_type = "hdf5";
    }

    if(file_name_ext.find("blueprint_root") == 0)
    {
        std::string file_name_true_ext = file_name_ext.substr(
            std::string("blueprint_root").length(), file_name_ext.length());

        // TODO: Add support for yaml protocol
        if(file_name_true_ext == "")
        {
            io_type = "json";
        }
        if(file_name_true_ext == "yaml")
        {
            io_type = "yaml";
        }
        else if(file_name_true_ext == "_hdf5" || file_name_true_ext == "_h5")
        {
            io_type = "hdf5";
        }
        else if(file_name_true_ext == "_silo")
        {
            io_type = "silo";
        }
    }

    return io_type;
}


//-----------------------------------------------------------------------------
// -- end conduit::relay::<mpi>::io_blueprint::detail --
//-----------------------------------------------------------------------------
};

//
// This expects a single or multi_domain blueprint mesh and will iterate
// through all domains to see if they are valid. Returns true
// if it contains valid data and false if there is no valid
// data.
//
// This is needed because after pipelines, it is possible to
// have no data left in a domain because of something like a
// clip
//
bool clean_mesh(const conduit::Node &data,
                conduit::Node &output
                CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    output.reset();
    const index_t potential_doms = data.number_of_children();
    bool maybe_multi_dom = true;

    if(!data.dtype().is_object() && !data.dtype().is_list())
    {
        maybe_multi_dom = false;
    }

    if(maybe_multi_dom)
    {
        // check all the children for valid domains
        for(int i = 0; i < potential_doms; ++i)
        {
            // we expect folks to use their best behaivor
            // (mesh bp verify is true before passing data)
            // so we can assume we have valid mesh bp.
            // if a child looks like a mesh, we have one
            conduit::Node info;
            const conduit::Node &child = data.child(i);

            bool is_valid = detail::quick_mesh_check(child);
            if(is_valid)
            {
                conduit::Node &dest_dom = output.append();
                dest_dom.set_external(child);
                // note: this algo may re-write domain ids
                // so make sure this isn't set as external
                if(dest_dom.has_path("state/domain_id"))
                {
                    index_t dom_id = dest_dom["state/domain_id"].to_index_t();
                    dest_dom["state/domain_id"].reset();
                    dest_dom["state/domain_id"] = dom_id;
                }
            }
        }
    }
    // if there is nothing in the output, lets see if it is a
    // valid single domain
    if(output.number_of_children() == 0)
    {
        // check to see if this is a single valid domain
        conduit::Node info;
        bool is_valid = detail::quick_mesh_check(data);
        if(is_valid)
        {
            conduit::Node &dest_dom = output.append();
            dest_dom.set_external(data);
        }
    }

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    conduit::blueprint::mpi::mesh::generate_domain_ids(output, mpi_comm);
#else
    conduit::blueprint::mesh::generate_domain_ids(output);
#endif

    const bool local_is_valid = output.number_of_children() > 0;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    return detail::global_someone_agrees(local_is_valid, mpi_comm);
#else
    return local_is_valid;
#endif
}


//-----------------------------------------------------------------------------
// Sig variants of save_mesh
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void save_mesh(const Node &mesh,
                const std::string &path
                CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    save_mesh(mesh,
              path,
              detail::identify_protocol(path),
              opts,
              mpi_comm);
#else
    save_mesh(mesh,
              path,
              detail::identify_protocol(path),
              opts);
#endif
}

//-----------------------------------------------------------------------------
void save_mesh(const Node &mesh,
                const std::string &path,
                const std::string &protocol
                CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    save_mesh(mesh,
              path,
              protocol,
              opts,
              mpi_comm);
#else
    save_mesh(mesh,
              path,
              protocol,
              opts);
#endif
}

//-----------------------------------------------------------------------------
// Main Mesh Blueprint Save, taken from Ascent
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      file_style: "default", "root_only", "multi_file"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      number_of_files:  {# of files}
///            when "multi_file":
///                 <= 0, use # of files == # of domains
///                  > 0, # of files == number_of_files
///
//-----------------------------------------------------------------------------
void save_mesh(const Node &mesh,
               const std::string &path,
               const std::string &protocol,
               const Node &opts
               CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // we force overwrite to true, so we need a copy of the const opts passed.
    Node save_opts;
    save_opts.set(opts);
    save_opts["truncate"] = "true";

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    write_mesh(mesh,
               path,
               protocol,
               save_opts,
               mpi_comm);
#else
    write_mesh(mesh,
               path,
               protocol,
               save_opts);
#endif
}

//-----------------------------------------------------------------------------
// Sig variants of write_mesh
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void write_mesh(const Node &mesh,
                const std::string &path
                CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    write_mesh(mesh,
               path,
               detail::identify_protocol(path),
               opts,
               mpi_comm);
#else
    write_mesh(mesh,
               path,
               detail::identify_protocol(path),
               opts);
#endif
}

//-----------------------------------------------------------------------------
void write_mesh(const Node &mesh,
                const std::string &path,
                const std::string &protocol
                CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    write_mesh(mesh,
               path,
               protocol,
               opts,
               mpi_comm);
#else
    write_mesh(mesh,
               path,
               protocol,
               opts);
#endif
}


//-----------------------------------------------------------------------------
// Main Mesh Blueprint Save, taken from Ascent
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      file_style: "default", "root_only", "multi_file"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      number_of_files:  {# of files}
///            when "multi_file":
///                 <= 0, use # of files == # of domains
///                  > 0, # of files == number_of_files
///
//-----------------------------------------------------------------------------
void write_mesh(const Node &mesh,
                const std::string &path,
                const std::string &file_protocol,
                const Node &opts
                CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{

    if(file_protocol == "silo")
    {
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
    #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        return conduit::relay::mpi::io::silo::write_mesh(mesh,
                                                         path,
                                                         opts,
                                                         mpi_comm);
    #else
        return conduit::relay::io::silo::write_mesh(mesh,
                                                    path,
                                                    opts);
    #endif
#else // conduit lacks silo support
    CONDUIT_ERROR("write_mesh invalid protocol option: `silo`"
                  << "conduit build lacks silo support\n");
#endif
    }

    // The assumption here is that everything is multi domain

    std::string opts_file_style = "default";
    std::string opts_suffix     = "default";
    std::string opts_mesh_name  = "mesh";
    int         opts_num_files  = -1;
    bool        opts_truncate   = false;

    // check for + validate file_style option
    if(opts.has_child("file_style") && opts["file_style"].dtype().is_string())
    {
        opts_file_style = opts["file_style"].as_string();

        if(opts_file_style != "default" &&
           opts_file_style != "root_only" &&
           opts_file_style != "multi_file" )
        {
            CONDUIT_ERROR("write_mesh invalid file_style option: \""
                          << opts_file_style << "\"\n"
                          " expected: \"default\", \"root_only\", "
                          "or \"multi_file\"");
        }

    }

    // check for + validate suffix option
    if(opts.has_child("suffix") && opts["suffix"].dtype().is_string())
    {
        opts_suffix = opts["suffix"].as_string();

        if(opts_suffix != "default" &&
           opts_suffix != "cycle" &&
           opts_suffix != "none" )
        {
            CONDUIT_ERROR("write_mesh invalid suffix option: \""
                          << opts_suffix << "\"\n"
                          " expected: \"default\", \"cycle\", or \"none\"");
        }
    }

    // check for + validate mesh_name option
    if(opts.has_child("mesh_name") && opts["mesh_name"].dtype().is_string())
    {
        opts_mesh_name = opts["mesh_name"].as_string();
    }


    // check for number_of_files, 0 or -1 implies #files => # domains
    if(opts.has_child("number_of_files") && opts["number_of_files"].dtype().is_integer())
    {
        opts_num_files = (int) opts["number_of_files"].to_int();
    }

    // check for truncate (overwrite)
    if(opts.has_child("truncate") && opts["truncate"].dtype().is_string())
    {
        const std::string ow_string = opts["truncate"].as_string();
        if(ow_string == "true")
            opts_truncate = true;
    }

    int num_files = opts_num_files;

    //-----------------------------------------------------------------------------
    // make sure some MPI task has data
    //-----------------------------------------------------------------------------
    Node multi_dom;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    if (! conduit::relay::mpi::io::blueprint::clean_mesh(mesh, multi_dom, mpi_comm))
#else
    if (! conduit::relay::io::blueprint::clean_mesh(mesh, multi_dom))
#endif
    {
        CONDUIT_INFO("Blueprint save: no valid data exists. Skipping save");
        return;
    }

    // -----------------------------------------------------------
    // get the cycle info
    // -----------------------------------------------------------
    index_t cycle;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    determine_cycle_and_resolve_suffix(multi_dom,
                                       path,
                                       cycle,
                                       opts_suffix,
                                       false, // we are not just generating the root file name
                                       mpi_comm);
#else
    determine_cycle_and_resolve_suffix(multi_dom,
                                       path,
                                       cycle,
                                       opts_suffix,
                                       false); // we are not just generating the root file name
#endif

    // -----------------------------------------------------------
    // par_rank and par_size
    // -----------------------------------------------------------
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    const int par_rank = relay::mpi::rank(mpi_comm);
    const int par_size = relay::mpi::size(mpi_comm);
#else
    const int par_rank = 0;
    const int par_size = 1;
#endif

    // -----------------------------------------------------------
    // find the local and global # of domains
    // -----------------------------------------------------------
    const int local_num_domains = conduit::blueprint::mesh::number_of_domains(multi_dom);
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    const int global_num_domains = conduit::blueprint::mpi::mesh::number_of_domains(multi_dom, mpi_comm);
#else
    const int global_num_domains = local_num_domains;
#endif

    if (global_num_domains == 0)
    {
        if (par_rank == 0)
        {
            CONDUIT_WARN("There is no data to save. Doing nothing.");
        }
        return;
    }

    std::string output_dir = "";

    // resolve file_style == default
    //
    // default implies multi_file if more than one domain
    if(opts_file_style == "default")
    {
        if( global_num_domains > 1)
        {
            opts_file_style = "multi_file";
        }
        else // other wise, use root only
        {
            opts_file_style = "root_only";
        }
    }

    // ----------------------------------------------------
    // if using multi_file, create output dir
    // ----------------------------------------------------
    if(opts_file_style == "multi_file")
    {
        // setup the directory
        output_dir = path;
        // at this point for suffix, we should only see
        // cycle or none -- default has been resolved
        if(opts_suffix == "cycle")
        {
            output_dir += conduit_fmt::format(".cycle_{:06d}",cycle);
        }

        bool dir_ok = false;

        // let rank zero handle dir creation
        if(par_rank == 0)
        {
            // check of the dir exists
            dir_ok = utils::is_directory(output_dir);
            if(!dir_ok)
            {
                // if not try to let rank zero create it
                dir_ok = utils::create_directory(output_dir);
            }
        }

        // make sure everyone knows if dir creation was successful

        #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        Node n_local, n_reduced;
        // use an mpi sum to check if the dir exists
        n_local = dir_ok ? 1 : 0;

        relay::mpi::sum_all_reduce(n_local,
                                   n_reduced,
                                   mpi_comm);

        dir_ok = (n_reduced.as_int() == 1);
        #endif

        if(!dir_ok)
        {
            CONDUIT_ERROR("Error: failed to create directory " << output_dir);
        }
    }

    // ----------------------------------------------------
    // setup root file name
    // ----------------------------------------------------
    std::string root_filename = path;

    // at this point for suffix, we should only see
    // cycle or none -- default has been resolved
    if(opts_suffix == "cycle")
    {
        root_filename += conduit_fmt::format(".cycle_{:06d}",cycle);
    }

    root_filename += ".root";
    // oss << ".root";

    // std::string root_filename = oss.str();

    // zero or negative (default cases), use one file per domain
    if(num_files <= 0)
    {
        num_files = global_num_domains;
    }

    // if global domains > num_files, warn and use one file per domain
    if(global_num_domains < num_files)
    {
        CONDUIT_INFO("Requested more files than actual domains, "
                     "writing one file per domain");
        num_files = global_num_domains;
    }

    // new style bp index partition_map
    // NOTE: the part_map is inited during write process for N domains
    // to M files case.
    // Other cases are simpler and are created when root file is written
    conduit::Node output_partition_map;

    // at this point for file_style,
    // default has been resolved, we need to just handle:
    //   root_only, multi_file
    if(opts_file_style == "root_only")
    {
        // if truncate, first touch needs to open the file with
        //          open_opts["mode"] = "wt";

        // write out local domains, since all tasks will
        // write to single file in this case, we need baton.
        // the outer loop + par_rank == current_writer implements
        // the baton.

        Node local_root_file_created;
        Node global_root_file_created;
        local_root_file_created.set((int)0);
        global_root_file_created.set((int)0);

        for(int current_writer=0; current_writer < par_size; current_writer++)
        {
            if(par_rank == current_writer)
            {
                relay::io::IOHandle hnd;

                for(int i = 0; i < local_num_domains; ++i)
                {
                    // if truncate, first rank to touch the file needs
                    // to open at
                    if( !hnd.is_open()
                        && (global_root_file_created.as_int() == 0)
                        && opts_truncate)
                    {
                        Node open_opts;
                        open_opts["mode"] = "wt";
                        hnd.open(root_filename,file_protocol,open_opts);
                        local_root_file_created.set((int)1);
                    }

                    if(!hnd.is_open())
                    {
                        hnd.open(root_filename,file_protocol);
                    }

                    const Node &dom = multi_dom.child(i);
                    // figure out the proper mesh path the file
                    std::string mesh_path = "";

                    if(global_num_domains == 1)
                    {
                        // no domain prefix, write to mesh name
                        mesh_path = opts_mesh_name;
                    }
                    else
                    {
                        // multiple domains, we need to use a domain prefix
                        uint64 domain = dom["state/domain_id"].to_uint64();
                        mesh_path = conduit_fmt::format("domain_{:06d}/{}",
                                                        domain,
                                                        opts_mesh_name);
                    }
                    hnd.write(dom,mesh_path);
                }

                // NOTE: local file handle goes out of scope here
                // and data is committed to file for handles that write
                // on close
            }

        // Reduce to sync up (like a barrier) and solve first writer need
        #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
            mpi::max_all_reduce(local_root_file_created,
                                global_root_file_created,
                                mpi_comm);
        #else
            global_root_file_created.set(local_root_file_created);
        #endif
        }
    }
    else if(global_num_domains == num_files)
    {
        // write out each domain
        // writes are independent, so no baton here
        for(int i = 0; i < local_num_domains; ++i)
        {
            const Node &dom = multi_dom.child(i);
            uint64 domain = dom["state/domain_id"].to_uint64();

            std::string output_file  = conduit::utils::join_file_path(output_dir,
                                                conduit_fmt::format("domain_{:06d}.{}",
                                                                    domain,
                                                                    file_protocol));
            // properly support truncate vs non truncate

            relay::io::IOHandle hnd;
            Node open_opts;
            open_opts["mode"] = "w";
            if(opts_truncate)
            {
               open_opts["mode"] = "wt";
            }

            // open our handle
            hnd.open(output_file, open_opts);
            // write to  mesh name subpath
            hnd.write(dom, opts_mesh_name);
        }
    }
    else // more complex case, N domains to M files
    {
        //
        // recall: we have re-labeled domain ids from 0 - > N-1, however
        // some mpi tasks may have no data.
        //

        // books we keep:
        Node books;
        books["local_domain_to_file"].set(DataType::index_t(local_num_domains));
        books["local_domain_status"].set(DataType::index_t(local_num_domains));

        // batons
        books["local_file_batons"].set(DataType::index_t(num_files));
        books["global_file_batons"].set(DataType::index_t(num_files));

        // used to track first touch
        books["local_file_created"].set(DataType::index_t(num_files));
        books["global_file_created"].set(DataType::index_t(num_files));

        // size local # of domains
        index_t_array local_domain_to_file = books["local_domain_to_file"].value();
        index_t_array local_domain_status  = books["local_domain_status"].value();

        // size num total files
        /// batons
        index_t_array local_file_batons    = books["local_file_batons"].value();
        index_t_array global_file_batons   = books["global_file_batons"].value();
        /// file created flags
        index_t_array local_file_created    = books["local_file_created"].value();
        index_t_array global_file_created   = books["global_file_created"].value();


        Node d2f_map;
        gen_domain_to_file_map(global_num_domains,
                               num_files,
                               books);

        //generate part map
        // use global_d2f is what we need for "file" part of part_map
        output_partition_map["file"] = books["global_domain_to_file"];
        output_partition_map["domain"].set(DataType::index_t(global_num_domains));
        index_t_array part_map_domain_vals = output_partition_map["domain"].value();
        for(index_t i=0; i < global_num_domains; i++)
        {
            part_map_domain_vals[i] = i;
        }

        index_t_accessor global_d2f = books["global_domain_to_file"].value();

        // init our local map and status array
        for(int d = 0; d < local_num_domains; ++d)
        {
            const Node &dom = multi_dom.child(d);
            uint64 domain = dom["state/domain_id"].to_uint64();
            // local domain index to file map
            local_domain_to_file[d] = global_d2f[domain];
            local_domain_status[d] = 1; // pending (1), vs done (0)
        }

        //
        // Round and round we go, will we deadlock I believe no :-)
        //
        // Here is how this works:
        //  At each round, if a rank has domains pending to write to a file,
        //  we put the rank id in the local file_batons vec.
        //  This vec is then mpi max'ed, and the highest rank
        //  that needs access to each file will write this round.
        //
        //  When a rank does not need to write to a file, we
        //  put -1 for this rank.
        //
        //  During each round, max of # files writers are participating
        //
        //  We are done when the mpi max of the batons is -1 for all files.
        //

        bool another_twirl = true;
        int twirls = 0;

        int local_all_is_good  = 1;
        int global_all_is_good = 1;

        books["local_all_is_good"].set_external(&local_all_is_good,1);
        books["global_all_is_good"].set_external(&global_all_is_good,1);

        std::string local_io_exception_msg = "";

        while(another_twirl)
        {
            // update baton requests
            for(int f = 0; f < num_files; ++f)
            {
                // reset file baton logic, look
                // to see if any local domains are
                // destined for this file
                local_file_batons[f] = -1;
                for(int d = 0; d < local_num_domains; ++d)
                {
                    // do we need to write this domain,
                    // and if so is it going to the file
                    // f
                    if(local_domain_status[d] == 1 &&
                       local_domain_to_file[d] == f)
                    {
                        local_file_batons[f] = par_rank;
                    }
                }
            }

            // mpi max file batons array
            #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                mpi::max_all_reduce(books["local_file_batons"],
                                    books["global_file_batons"],
                                    mpi_comm);
            #else
                global_file_batons.set(local_file_batons);
            #endif

            // mpi max file created array
            #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                mpi::max_all_reduce(books["local_file_created"],
                                    books["global_file_created"],
                                    mpi_comm);
            #else
                global_file_created.set(local_file_created);
            #endif


            // we now have valid batons (global_file_batons)
            for(int f = 0; f < num_files && local_all_is_good == 1 ; ++f)
            {
                // check if this rank has the global baton for this file
                if( global_file_batons[f] == par_rank )
                {
                    // check the domains this rank has pending
                    for(int d = 0; d < local_num_domains && local_all_is_good == 1; ++d)
                    {
                        // reuse this handle for all domains in the file
                        relay::io::IOHandle hnd;
                        if(local_domain_status[d] == 1 &&  // pending
                           local_domain_to_file[d] == f) // destined for this file
                        {
                            // now is the time to write!
                            // pattern is:
                            //  file_%06llu.{protocol}:/domain_%06llu/...
                            const Node &dom = multi_dom.child(d);
                            uint64 domain_id = dom["state/domain_id"].to_uint64();

                            // construct file name
                            std::string file_name = conduit_fmt::format(
                                                        "file_{:06d}.{}",
                                                        f,
                                                        file_protocol);

                            std::string output_file = conduit::utils::join_file_path(output_dir,
                                                                                     file_name);

                            // now the path in the file, and domain id
                            std::string curr_path = conduit_fmt::format(
                                                            "domain_{:06d}/{}",
                                                             domain_id,
                                                             opts_mesh_name);

                            try
                            {
                                // if truncate == true check if this is the first time we are
                                // touching file, and use wt
                                Node open_opts;
                                if(opts_truncate && global_file_created[f] == 0)
                                {
                                   open_opts["mode"] = "wt";

                                   local_file_created[f]  = 1;
                                   global_file_created[f] = 1;
                                }

                                if(!hnd.is_open())
                                {
                                    hnd.open(output_file, open_opts);
                                }

                                // CONDUIT_INFO("rank " << par_rank << " output_file"
                                //              << output_file << " path " << path);

                                hnd.write(dom, curr_path);

                                // update status, we are done with this doman
                                local_domain_status[d] = 0;
                            }
                            catch(conduit::Error &e)
                            {
                                local_all_is_good = 0;
                                local_io_exception_msg = e.message();
                            }
                        }
                    }
                }
            }

            // if any I/O errors happened stop and have all
            // tasks bail out with an exception (to avoid hangs)
            #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
                mpi::min_all_reduce(books["local_all_is_good"],
                                    books["global_all_is_good"],
                                    mpi_comm);
            #else
                global_all_is_good = local_all_is_good;
            #endif

            if(global_all_is_good == 0)
            {
                std::string emsg = "Failed to write mesh data on one more more ranks.";

                if(!local_io_exception_msg.empty())
                {
                     emsg += conduit_fmt::format("Exception details from rank {}: {}.",
                                                 par_rank, local_io_exception_msg);
                }
                CONDUIT_ERROR(emsg);
            }

            // // If you need to debug the baton algorithm,
            // // uncomment to examine the books:
            // if(par_rank == 0)
            // {
            //    std::cout << "[" << par_rank << "] "
            //              << " twirls: " << twirls
            //              << " details\n"
            //              << books.to_yaml();
            // }
            // void twirls warning for general case
            (void)twirls;

            // check if we have another round
            // stop when all batons are -1
            another_twirl = false;

            for(int f = 0; f < num_files && !another_twirl; ++f)
            {
                // if any entry is not -1, we still have more work to do
                if(global_file_batons[f] != -1)
                {
                    another_twirl = true;
                    twirls++;
                }
            }
        }
    }

    int root_file_writer = 0;
    if(local_num_domains == 0)
    {
        root_file_writer = -1;
    }
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    // Rank 0 could have an empty domain, so we have to check
    // to find someone with a data set to write out the root file.
    Node out;
    out = local_num_domains;
    Node rcv;

    mpi::all_gather_using_schema(out, rcv, mpi_comm);
    root_file_writer = -1;
    int* res_ptr = (int*)rcv.data_ptr();
    for(int i = 0; i < par_size; ++i)
    {
        if(res_ptr[i] != 0)
        {
            root_file_writer = i;
            break;
        }
    }

    MPI_Barrier(mpi_comm);
#endif

    if(root_file_writer == -1)
    {
        // this should not happen. global doms is already 0
        CONDUIT_WARN("Relay: there are no domains to write out");
    }

    // generate the bp index
    Node local_bp_idx, bp_idx;
    if(local_num_domains > 0)
    {
        ::conduit::blueprint::mesh::generate_index(multi_dom,
                                                   opts_mesh_name,
                                                   global_num_domains,
                                                   local_bp_idx);
    }
    // handle mpi case.
    // this logic is from the mpi ver of mesh index gen
    // it is duplicated here b/c we dont want a circular dep
    // between conduit_blueprint_mpi and conduit_relay_io_mpi
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    // NOTE: do to save vs write cases, these updates should be
    // single mesh only
    Node gather_bp_idx;
    relay::mpi::all_gather_using_schema(local_bp_idx,
                                        gather_bp_idx,
                                        mpi_comm);

    // union all entries into final index that reps
    // all domains
    NodeConstIterator itr = gather_bp_idx.children();
    while(itr.has_next())
    {
        const Node &curr = itr.next();
        bp_idx[opts_mesh_name].update(curr);
    }
#else
    // NOTE: do to save vs write cases, these updates should be
    // single mesh only
    bp_idx[opts_mesh_name] = local_bp_idx;
#endif

    // root_file_writer will now write out the root file
    if(par_rank == root_file_writer)
    {
        std::string output_dir_base, output_dir_path;
        conduit::utils::rsplit_file_path(output_dir,
                                         output_dir_base,
                                         output_dir_path);

        std::string output_tree_pattern;
        std::string output_file_pattern;
        // new style bp index partition spec
        std::string output_partition_pattern;

        // NOTE:
        // The file pattern needs to be relative to
        // the root file.
        // reverse split the path

        if(opts_file_style == "root_only")
        {
            // make sure this is relative to output dir
            std::string tmp;
            utils::rsplit_file_path(root_filename,
                                    output_file_pattern,
                                    tmp);

            if(global_num_domains == 1)
            {
                output_tree_pattern = "/";
                output_partition_pattern = output_file_pattern + ":/";
                // NOTE: we don't need the part map entries for this case
            }
            else
            {
                output_tree_pattern = "/domain_%06d/";
                output_partition_pattern = root_filename + ":/domain_{domain:06d}";

                //generate part map (we only need domain for this case)
                output_partition_map["domain"].set(DataType::index_t(global_num_domains));
                index_t_array part_map_domain_vals = output_partition_map["domain"].value();
                for(index_t i=0; i < global_num_domains; i++)
                {
                    part_map_domain_vals[i] = i;
                }
            }
        }
        else if(global_num_domains == num_files)
        {
            //generate part map
            output_partition_map["file"].set(DataType::index_t(global_num_domains));
            output_partition_map["domain"].set(DataType::index_t(global_num_domains));
            index_t_array part_map_file_vals   = output_partition_map["file"].value();
            index_t_array part_map_domain_vals = output_partition_map["domain"].value();

            for(index_t i=0; i < global_num_domains; i++)
            {
                // file id == domain id
                part_map_file_vals[i]   = i;
                part_map_domain_vals[i] = i;
            }

            std::string tmp;
            utils::rsplit_file_path(output_dir_base,
                                    output_file_pattern,
                                    tmp);

            output_partition_pattern = conduit::utils::join_file_path(
                                                output_file_pattern,
                                                "domain_{domain:06d}." +
                                                file_protocol +
                                                ":/");

            output_file_pattern = conduit::utils::join_file_path(
                                                output_file_pattern,
                                                "domain_%06d." + file_protocol);
            output_tree_pattern = "/";
        }
        else
        {
            std::string tmp;
            utils::rsplit_file_path(output_dir_base,
                                    output_file_pattern,
                                    tmp);

            output_partition_pattern = conduit::utils::join_file_path(
                                                output_file_pattern,
                                                "file_{file:06d}." +
                                                file_protocol +
                                                ":/domain_{domain:06d}");

            output_file_pattern = conduit::utils::join_file_path(
                                                output_file_pattern,
                                                "file_%06d." + file_protocol);
            output_tree_pattern = "/domain_%06d";
        }

        /////////////////////////////
        // mesh partition map
        /////////////////////////////
        // example of cases:
        // root only, single domain
        // partition_pattern: "out.root"
        //
        // root only, multi domain
        // partition_pattern: "out.root:domain_{domain:06d}"
        // partition_map:
        //   domain: [0, 1, 2, 3, 4 ]
        //
        // # domains == # files:
        // partition_pattern: "out/domain_{domain:06d}.hdf5"
        // partition_map:
        //   file:  [ 0, 1, 2, 3, 4 ]
        //   domain: [ 0, 1, 2, 3, 4 ]
        //
        // N domains to M files:
        // partition_pattern: "out/file_{file:06d}.hdf5:domain_{domain:06d}"
        // partition_map:
        //   file:  [ 0, 0, 1, 2, 2 ]
        //   domain: [ 0, 1, 2, 3, 4 ]
        //
        // N domains to M files (non trivial domain order):
        // partition_pattern: "out/file_{file:06d}.hdf5:domain_{domain:06d}"
        // partition_map:
        //    file:  [ 0, 0, 1, 2, 2 ]
        //    domain: [ 4, 0, 3, 2, 1 ]
        //
        // NOTE: do to save vs write cases, these updates should be
        // single mesh only
        bp_idx[opts_mesh_name]["state/partition_pattern"] = output_partition_pattern;

        if (output_partition_map.number_of_children() > 0 )
        {
            bp_idx[opts_mesh_name]["state/partition_map"] = output_partition_map;
        }

        Node root;
        root["blueprint_index"].set(bp_idx);

        root["protocol/name"]    = file_protocol;
        root["protocol/version"] = CONDUIT_VERSION;

        root["number_of_files"]  = num_files;
        root["number_of_trees"]  = global_num_domains;

        root["file_pattern"] = output_file_pattern;
        root["tree_pattern"] = output_tree_pattern;

        relay::io::IOHandle hnd;

        // if not root only, this is the first time we are writing
        // to the root file -- make sure to properly support truncate
        Node open_opts;
        if(opts_file_style != "root_only" && opts_truncate)
        {
            open_opts["mode"] = "wt";
        }

        hnd.open(root_filename, file_protocol, open_opts);
        hnd.write(root);
        hnd.close();
    }

    // barrier at end of work to avoid file system race
    // (non root task could write the root file in write_mesh,
    // but root task is always the one to read the root file
    // in read_mesh.

    #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        MPI_Barrier(mpi_comm);
    #endif
}

// -----------------------------------------------------------
// get the cycle info
// -----------------------------------------------------------
void determine_cycle_and_resolve_suffix(const Node &multi_dom,
                                        const std::string &path,
                                        index_t &cycle,
                                        std::string &opts_suffix,
                                        bool gen_name
                                        CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    cycle = conduit::blueprint::mpi::mesh::cycle(multi_dom, mpi_comm);
#else
    cycle = conduit::blueprint::mesh::cycle(multi_dom);
#endif
    // int max is the default cycle value if none exists
    const bool cycle_exists = (std::numeric_limits<int>::max() != cycle);

    if (! cycle_exists)
    {
        if ("cycle" == opts_suffix)
        {
            CONDUIT_INFO("determine_cycle_and_resolve_suffix: no 'state/cycle' present."
                         " Defaulting to counter");
            static std::map<std::string, int> counters;
            cycle = counters[path];

            // If we're just using this function to ask what the filename could be,
            // then we don't want to mess with the static counter. If we are
            // in the process of writing files, then we can make up a new cycle.
            if (! gen_name)
            {
                counters[path] ++;
            }
        }
        else
        {
            opts_suffix = "none";
        }
    }
    // if the cycle exists and we are in the default suffix case
    else if ("default" == opts_suffix)
    {
        opts_suffix = "cycle";
    }
    // if the cycle exists and we are in the cycle suffix case - cycle
    // has already been set appropriately.
    // if the cycle exists and we are in the no suffix case - it
    // doesn't matter what we have set cycle to.
}

//-----------------------------------------------------------------------------
// Generate root file name given parameters
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
std::string generate_root_filename(const conduit::Node &mesh,
                                   const std::string &path
                                   CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    return generate_root_filename(mesh,
                                  path,
                                  detail::identify_protocol(path),
                                  opts,
                                  mpi_comm);
#else
    return generate_root_filename(mesh,
                                  path,
                                  detail::identify_protocol(path),
                                  opts);
#endif
}

//-----------------------------------------------------------------------------
std::string generate_root_filename(const conduit::Node &mesh,
                                   const std::string &path,
                                   const std::string &file_protocol
                                   CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // empty opts
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    return generate_root_filename(mesh,
                                  path,
                                  file_protocol,
                                  opts,
                                  mpi_comm);
#else
    return generate_root_filename(mesh,
                                  path,
                                  file_protocol,
                                  opts);
#endif
}


//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      file_style: "default", "root_only", "multi_file", "overlink"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      root_file_ext: "default", "root", "silo"
///            "default"   ==> "root"
///            if overlink, this parameter is unused.
///
//-----------------------------------------------------------------------------
std::string generate_root_filename(const conduit::Node &mesh,
                                   const std::string &path,
                                   const std::string &file_protocol,
                                   const conduit::Node &opts
                                   CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    // The assumption here is that everything is multi domain

    std::string opts_file_style    = "default";
    std::string opts_suffix        = "default";
    std::string opts_root_file_ext = "default";
    bool write_overlink            = false;

    // We will use opts_root_file_ext so that we can share logic between
    // Silo and Blueprint in this function. We hardcode non-Silo (just
    // Blueprint) to use "root".
    if (file_protocol != "silo")
    {
        opts_root_file_ext = "root";
    }

    // check for + validate suffix option
    if (opts.has_child("suffix") && opts["suffix"].dtype().is_string())
    {
        opts_suffix = opts["suffix"].as_string();

        if (opts_suffix != "default" &&
            opts_suffix != "cycle" &&
            opts_suffix != "none" )
        {
            CONDUIT_ERROR("write_mesh invalid suffix option: \""
                          << opts_suffix << "\"\n"
                          " expected: \"default\", \"cycle\", or \"none\"");
        }
    }

    //-----------------------------------------------------------------------------
    // make sure some MPI task has data
    //-----------------------------------------------------------------------------
    Node multi_dom;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    if (! conduit::relay::mpi::io::blueprint::clean_mesh(mesh, multi_dom, mpi_comm))
#else
    if (! conduit::relay::io::blueprint::clean_mesh(mesh, multi_dom))
#endif
    {
        CONDUIT_INFO("generate_root_filename: no valid data exists. Filename could not be generated.");
        return "";
    }

    //
    // extra silo logic
    //
    if (file_protocol == "silo")
    {
        // check for + validate file_style option
        if (opts.has_child("file_style") && opts["file_style"].dtype().is_string())
        {
            opts_file_style = opts["file_style"].as_string();

            if (opts_file_style != "default" &&
                opts_file_style != "root_only" &&
                opts_file_style != "multi_file" &&
                opts_file_style != "overlink")
            {
                CONDUIT_ERROR("write_mesh invalid file_style option: \""
                              << opts_file_style << "\"\n"
                              " expected: \"default\", \"root_only\", "
                              "\"multi_file\", or \"overlink\"");
            }
        }

        // this is the earliest place we know for sure if we are writing overlink or not
        // this is set in stone.
        write_overlink = opts_file_style == "overlink";

        // check for + validate root_file_ext option
        if (opts.has_child("root_file_ext") && opts["root_file_ext"].dtype().is_string())
        {
            opts_root_file_ext = opts["root_file_ext"].as_string();

            if (opts_root_file_ext != "default" &&
                opts_root_file_ext != "root" &&
                opts_root_file_ext != "silo" )
            {
                CONDUIT_ERROR("write_mesh invalid root_file_ext option: \""
                              << opts_root_file_ext << "\"\n"
                              " expected: \"default\", \"root\", or \"silo\"");
            }
        }

        if (opts_root_file_ext == "default")
        {
            opts_root_file_ext = "root";
        }

        if (write_overlink)
        {
            opts_suffix = "none"; // force no suffix for overlink case
            opts_root_file_ext = "silo"; // force .silo file extension for root file
        }
    }

    // -----------------------------------------------------------
    // get the cycle info
    // -----------------------------------------------------------
    index_t cycle;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    determine_cycle_and_resolve_suffix(multi_dom,
                                       path,
                                       cycle,
                                       opts_suffix,
                                       true, // we are just generating the root file name
                                       mpi_comm);
#else
    determine_cycle_and_resolve_suffix(multi_dom,
                                       path,
                                       cycle,
                                       opts_suffix,
                                       true); // we are just generating the root file name
#endif

    // ----------------------------------------------------
    // setup root file name
    // ----------------------------------------------------
    std::string root_filename;
    if (file_protocol == "silo" && write_overlink)
    {
        std::string output_dir = path;

        // at this point for suffix, we should only see
        // cycle or none -- default has been resolved
        if (opts_suffix == "cycle")
        {
            output_dir += conduit_fmt::format(".cycle_{:06d}", cycle);
        }

        root_filename = utils::join_file_path(output_dir, "OvlTop." + opts_root_file_ext);
    }
    else
    {
        root_filename = path;

        // at this point for suffix, we should only see
        // cycle or none -- default has been resolved
        if (opts_suffix == "cycle")
        {
            root_filename += conduit_fmt::format(".cycle_{:06d}", cycle);
        }

        root_filename += "." + opts_root_file_ext;
    }

    return root_filename;
}

//-----------------------------------------------------------------------------
// The load semantics, the mesh node is reset before reading.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void load_mesh(const std::string &root_file_path,
               conduit::Node &mesh
               CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    mesh.reset();

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    read_mesh(root_file_path,
              mesh,
              mpi_comm);
#else
    read_mesh(root_file_path,
              mesh);
#endif
}

//-----------------------------------------------------------------------------
void load_mesh(const std::string &root_file_path,
               const conduit::Node &opts,
               conduit::Node &mesh
               CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    mesh.reset();

#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    read_mesh(root_file_path,
              opts,
              mesh,
              mpi_comm);
#else
    read_mesh(root_file_path,
              opts,
              mesh);
#endif
}

//-----------------------------------------------------------------------------
void read_mesh(const std::string &root_file_path,
               Node &mesh
               CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    Node opts;
#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
    read_mesh(root_file_path,
              opts,
              mesh,
              mpi_comm);
#else
    read_mesh(root_file_path,
              opts,
              mesh);
#endif
}


// read bp index from root file,
// returns false if there is an error
bool
read_root_blueprint_index(const std::string &root_file_path,
                          const Node &opts,
                          Node &root_node, // output
                          std::string &mesh_name, // output
                          std::ostringstream &error_oss) // output
{
    // clear output vars
    root_node.reset();
    mesh_name = "";
    error_oss.str("");

    // first, make sure we can open the root file
    std::ifstream ifs;
    ifs.open(root_file_path.c_str());
    if(!ifs.is_open())
    {
        error_oss << "failed to open root file: " << root_file_path;
        return false;
    }
    ifs.close();

    // check root file protocol using heuristic search
    std::string root_protocol;
    conduit::relay::io::identify_file_type(root_file_path,root_protocol);

    if(root_protocol == "unknown")
    {
        error_oss << "failed to detect file protocol (protocol ='"
                  << root_protocol
                  << "') of root file: "
                  << root_file_path;
        return false;
    }

    // Read root file info
    // We don't want to always read everything in the file
    // b/c the root index node is broadcasted to all ranks
    // (think of cases where root file also contains meshes)
    // so we still filter what is pulled out here

    // list of names we want to read from the root file
    conduit::Node index_names;
    index_names.append() = "blueprint_index";
    index_names.append() = "file_pattern";
    index_names.append() = "tree_pattern";
    index_names.append() = "number_of_trees";
    index_names.append() = "number_of_files";
    index_names.append() = "protocol";


    relay::io::IOHandle root_hnd;
    Node open_opts;
    open_opts["mode"] = "r";

    try
    {
        root_hnd.open(root_file_path, root_protocol, open_opts);

        // loop over all names and copy them to the output node
        NodeConstIterator itr = index_names.children();
        while(itr.has_next())
        {
            std::string curr_idx_name = itr.next().as_string();
            if(root_hnd.has_path(curr_idx_name))
            {
                root_hnd.read(curr_idx_name,
                              root_node[curr_idx_name]);
            }
        }
        root_hnd.close();
    }
    catch(const conduit::Error &err)
    {
        error_oss << err.message();
        return false;
    }

    if(!root_node.has_child("blueprint_index"))
    {
        error_oss << "Root file ("
                  << root_file_path
                  << " ) missing 'blueprint_index'";
        return false;
    }

    if(opts.has_child("mesh_name") && opts["mesh_name"].dtype().is_string())
    {
        mesh_name = opts["mesh_name"].as_string();
    }

    if(mesh_name.empty())
    {
        NodeConstIterator itr = root_node["blueprint_index"].children();
        itr.next();
        mesh_name = itr.name();
    }

    if(!root_node["blueprint_index"].has_child(mesh_name))
    {
        // bad name, construct an error message that
        // displays the valid options
        error_oss << "Mesh named '" << mesh_name << "' "
            << " not found in "
            << root_file_path
            << std::endl
            << " Mesh names found blueprint index: "
            << std::endl;
        NodeConstIterator itr = root_node["blueprint_index"].children();
        while(itr.has_next())
        {
            itr.next();
            error_oss << " " << itr.name();
            error_oss << std::endl;
        }
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
void read_mesh(const std::string &root_file_path,
               const Node &opts,
               Node &mesh
               CONDUIT_RELAY_COMMUNICATOR_ARG(MPI_Comm mpi_comm))
{
    int par_rank = 0;
#if CONDUIT_RELAY_IO_MPI_ENABLED
    par_rank = relay::mpi::rank(mpi_comm);
    int par_size = relay::mpi::size(mpi_comm);
#endif

    // check root file on rank 0, if silo call into silo mesh write logic
    std::string root_protocol;
    if(par_rank == 0)
    {
        // check root file protocol using heuristic search
        conduit::relay::io::identify_file_type(root_file_path,root_protocol);
    }

#if CONDUIT_RELAY_IO_MPI_ENABLED
    Node n_root_proto;
    n_root_proto.set(root_protocol);
    conduit::relay::mpi::broadcast_using_schema(n_root_proto,
                                                0,
                                                mpi_comm);
    root_protocol = n_root_proto.as_string();
#endif

    if(root_protocol == "silo")
    {
#ifdef CONDUIT_RELAY_IO_SILO_ENABLED
    #ifdef CONDUIT_RELAY_IO_MPI_ENABLED
        return conduit::relay::mpi::io::silo::read_mesh(root_file_path,
                                                        opts,
                                                        mesh,
                                                        mpi_comm);
    #else
        return conduit::relay::io::silo::read_mesh(root_file_path,
                                                   opts,
                                                   mesh);
    #endif
#else // conduit lacks silo support
    CONDUIT_ERROR("read_mesh invalid protocol option: `silo`"
                  << "conduit build lacks silo support\n");
#endif
    }

    int error = 0;
    std::ostringstream error_oss;
    Node root_node;
    std::string mesh_name;

    // only read bp index on rank 0
    if(par_rank == 0)
    {
        if(!read_root_blueprint_index(root_file_path,
                                      opts,
                                      root_node,
                                      mesh_name,
                                      error_oss))
        {
            error = 1;
        }
    }

#if CONDUIT_RELAY_IO_MPI_ENABLED
    Node n_local, n_global;
    n_local.set((int)error);
    relay::mpi::sum_all_reduce(n_local,
                               n_global,
                               mpi_comm);

    error = n_global.as_int();

    if(error == 1)
    {
        // we have a problem, broadcast string message
        // from rank 0 all ranks can throw an error
        n_global.set(error_oss.str());
        conduit::relay::mpi::broadcast_using_schema(n_global,
                                                    0,
                                                    mpi_comm);

        CONDUIT_ERROR(n_global.as_string());
    }
    else
    {
        // broadcast the mesh name and the bp index
        // from rank 0 to all ranks
        n_global.set(mesh_name);
        conduit::relay::mpi::broadcast_using_schema(n_global,
                                                    0,
                                                    mpi_comm);
        mesh_name = n_global.as_string();
        conduit::relay::mpi::broadcast_using_schema(root_node,
                                                    0,
                                                    mpi_comm);
    }
#else
    // non MPI case, throw error
    if(error == 1)
    {
        CONDUIT_ERROR(error_oss.str());
    }
#endif

    // make sure we have a valid bp index
    Node verify_info;
    const Node &mesh_index = root_node["blueprint_index"][mesh_name];
    if( !::conduit::blueprint::mesh::index::verify(mesh_index,
                                                   verify_info[mesh_name]))
    {
        CONDUIT_ERROR("Mesh Blueprint index verify failed" << std::endl
                      << verify_info.to_json());
    }

    bool has_part_pattern = mesh_index.has_path("state/partition_pattern");
    // We need either a state/partition_pattern, or root level file_pattern

    if(!has_part_pattern && !root_node.has_child("file_pattern"))
    {
        CONDUIT_ERROR("Root file missing 'file_pattern' or mesh specific"
            " partition_pattern (" << mesh_name << "/state/partition_pattern)");
    }

    //(per mesh part maps case doesn't need these, but older style does)
    if(!has_part_pattern && !root_node.has_child("number_of_trees"))
    {
        CONDUIT_ERROR("Root missing `number_of_trees`");
    }

    if(!has_part_pattern && !root_node.has_child("number_of_files"))
    {
        CONDUIT_ERROR("Root missing `number_of_files`");
    }

    std::string data_protocol = "hdf5";

    if(root_node.has_child("protocol"))
    {
        data_protocol = root_node["protocol/name"].as_string();
    }

    // read all domains for given mesh
    int num_domains = -1;
    if(has_part_pattern)
    {
        if(mesh_index.has_path("state/partition_map/domain"))
        {
            index_t_accessor doms = mesh_index["state/partition_map/domain"].value();
            num_domains = static_cast<int>(doms.max()) + 1;
        }
        else
        {
            // special case, one 1 domain
            num_domains = 1;
        }
    }
    else
    {
        num_domains = root_node["number_of_trees"].to_int();
    }
    detail::BlueprintTreePathGenerator gen;

    // three cases:
    //  legacy case that uses file_pattern and tree_pattern
    //  case that uses a mesh specific partition pattern and partition map
    //  case that uses a mesh specific partition pattern
    if(mesh_index["state"].has_child("partition_pattern"))
    {
        if(mesh_index["state"].has_child("partition_map"))
        {
            gen.Init(mesh_index["state/partition_pattern"].as_string(),
                     mesh_index["state/partition_map"]);
        }
        else
        {
            // this will only occur for single domain root file case
            gen.Init(mesh_index["state/partition_pattern"].as_string());
        }
    }
    else
    {
        int num_files   = root_node["number_of_files"].to_int();
        gen.Init(root_node["file_pattern"].as_string(),
                 root_node["tree_pattern"].as_string(),
                 num_files,
                 num_domains,
                 data_protocol);
    }

    std::ostringstream oss;
    int domain_start = 0;
    int domain_end = num_domains;

#if CONDUIT_RELAY_IO_MPI_ENABLED

    int read_size = num_domains / par_size;
    int rem = num_domains % par_size;
    if(par_rank < rem)
    {
        read_size++;
    }

    conduit::Node n_read_size;
    conduit::Node n_doms_per_rank;

    n_read_size.set_int32(read_size);

    relay::mpi::all_gather_using_schema(n_read_size,
                                        n_doms_per_rank,
                                        mpi_comm);
    int *counts = (int*)n_doms_per_rank.data_ptr();

    int rank_offset = 0;
    for(int i = 0; i < par_rank; ++i)
    {
        rank_offset += counts[i];
    }

    domain_start = rank_offset;
    domain_end = rank_offset + read_size;
#endif

    if(data_protocol == "sidre_hdf5")
    {
        relay::io::IOHandle hnd;
        Node open_opts;
        open_opts["mode"] = "r";
        hnd.open(root_file_path, "sidre_hdf5", open_opts);
        for(int i = domain_start ; i < domain_end; i++)
        {
            oss.str("");
            oss << i << "/" << mesh_name;
            hnd.read(oss.str(),mesh);
        }
    }
    else
    {
        relay::io::IOHandle hnd;
        Node open_opts;
        open_opts["mode"] = "r";
        for(int i = domain_start ; i < domain_end; i++)
        {
            std::string current, next;
            utils::rsplit_file_path(root_file_path, current, next);
            std::string domain_file = utils::join_file_path(next, gen.GenerateFilePath(i));

            hnd.open(domain_file, data_protocol, open_opts);

            // also need the tree path
            std::string tree_path = gen.GenerateTreePath(i);

            std::string mesh_path = conduit_fmt::format("domain_{:06d}",i);

            Node &mesh_out = mesh[mesh_path];

            // read components of the mesh according to the mesh index
            // for each child in the index
            NodeConstIterator outer_itr = mesh_index.children();

            while(outer_itr.has_next())
            {
                const Node &outer = outer_itr.next();
                std::string outer_name = outer_itr.name();

                // special logic for state, since it was not included in the index
                if(outer_name == "state" )
                {
                    // we do need to read the state!
                    if(outer.has_child("path"))
                    {
                        hnd.read(utils::join_path(tree_path,outer["path"].as_string()),
                                 mesh_out[outer_name]);
                    }
                    else
                    {
                        if(outer.has_child("cycle"))
                        {
                             mesh_out[outer_name]["cycle"] = outer["cycle"];
                        }

                        if(outer.has_child("time"))
                        {
                            mesh_out[outer_name]["time"] = outer["time"];
                        }
                     }
                }

                NodeConstIterator itr = outer.children();
                while(itr.has_next())
                {
                    const Node &entry = itr.next();
                    // check if it has a path
                    if(entry.has_child("path"))
                    {
                        std::string entry_name = itr.name();
                        std::string entry_path = entry["path"].as_string();
                        std::string fetch_path = utils::join_path(tree_path,
                                                                  entry_path);
                        // some parts may not exist in all domains
                        // only read if they are there
                        if(hnd.has_path(fetch_path))
                        {
                            hnd.read(fetch_path,
                                     mesh_out[outer_name][entry_name]);
                        }
                    }
                }
            }
        }
    }

}


//-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit::relay::io::<mpi>::blueprint --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::relay::io --
//-----------------------------------------------------------------------------


#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi --
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
