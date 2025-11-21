// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh.cpp
///
//-----------------------------------------------------------------------------

#if defined(CONDUIT_PLATFORM_WINDOWS)
#define NOMINMAX
#undef min
#undef max
#include "windows.h"
#endif

//-----------------------------------------------------------------------------
// std lib includes
//-----------------------------------------------------------------------------
#include <algorithm>
#include <deque>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <iterator>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_fmt/conduit_fmt.h"
#include "conduit_execution.hpp"
#include "conduit_fixed_size_map.hpp"
#include "conduit_fixed_size_vector.hpp"
#include "conduit_geometry_vector.hpp"
#include "conduit_blueprint_mcarray.hpp"
#include "conduit_blueprint_o2mrelation.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_mesh_partition.hpp"
#include "conduit_blueprint_mesh_flatten.hpp"
#include "conduit_blueprint_mesh_topology_metadata.hpp"
#include "conduit_blueprint_mesh_utils_iterate_elements.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_log.hpp"
#include "conduit_annotations.hpp"
#include "conduit_utils.hpp"

using namespace conduit;
// Easier access to the Conduit logging functions
using namespace conduit::utils;
// access conduit path helper
using ::conduit::utils::join_path;
// access conduit blueprint mesh utilities
namespace bputils = conduit::blueprint::mesh::utils;
typedef bputils::ShapeType ShapeType;
typedef bputils::ShapeCascade ShapeCascade;
// Use the old version.
//typedef bputils::reference::TopologyMetadata TopologyMetadata;
// Use the new version.
typedef bputils::TopologyMetadata TopologyMetadata;

//-----------------------------------------------------------------------------
// -- begin internal helpers --
//-----------------------------------------------------------------------------

// access conduit blueprint mesh utilities
namespace bputils = conduit::blueprint::mesh::utils;
// access one-to-many index types
namespace o2mrelation = conduit::blueprint::o2mrelation;

// typedefs for verbose but commonly used types
typedef std::tuple<conduit::Node*, conduit::Node*, conduit::Node*> DomMapsTuple;

// typedefs to enable passing around function pointers
typedef void (*GenDerivedFun)(const conduit::Node&, conduit::Node&, conduit::Node&, conduit::Node&);
typedef void (*GenDecomposedFun)(const conduit::Node&, conduit::Node&, conduit::Node&, conduit::Node&, conduit::Node&);
typedef conduit::index_t (*IdDecomposedFun)(const bputils::TopologyMetadata&, const conduit::index_t /*ei*/, const conduit::index_t /*di*/);
typedef std::vector<conduit::index_t> (*CalcDimDecomposedFun)(const bputils::ShapeType&);

//-----------------------------------------------------------------------------
// - begin internal potpourri functions -
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void grid_ijk_to_id(const index_t *ijk,
                    const index_t *dims,
                    index_t &grid_id)
{
    grid_id = 0;
    for(index_t d = 0; d < 3; d++)
    {
        index_t doffset = ijk[d];
        for(index_t dd = 0; dd < d; dd++)
        {
            doffset *= dims[dd];
        }

        grid_id += doffset;
    }
}

//-----------------------------------------------------------------------------
void grid_id_to_ijk(const index_t id,
                    const index_t *dims,
                    index_t *grid_ijk)
{
    index_t dremain = id;
    for(index_t d = 3; d-- > 0;)
    {
        index_t dstride = 1;
        for(index_t dd = 0; dd < d; dd++)
        {
            dstride *= dims[dd];
        }

        grid_ijk[d] = dremain / dstride;
        dremain = dremain % dstride;
    }
}

//-----------------------------------------------------------------------------
template <typename Container1, typename Container2>
std::vector<index_t> intersect_sets(const Container1 &v1,
                                    const Container2 &v2)
{
    std::vector<index_t> res;
    for(const auto elem1 : v1)
    {
        for(const auto elem2 : v2)
        {
            if(elem1 == elem2)
            {
                res.push_back(elem1);
            }
        }
    }

    return res;
}

//-----------------------------------------------------------------------------
template <>
std::vector<index_t> intersect_sets(const std::set<index_t> &s1,
                                    const std::set<index_t> &s2)
{
    std::vector<index_t> si(std::max(s1.size(), s2.size()));
    std::vector<index_t>::iterator si_end = std::set_intersection(
        s1.begin(), s1.end(), s2.begin(), s2.end(), si.begin());
    return std::vector<index_t>(si.begin(), si_end);
}

//-----------------------------------------------------------------------------
std::vector<index_t> subtract_sets(const std::vector<index_t> &v1,
                                   const std::vector<index_t> &v2)
{
    std::vector<index_t> res;
    for(index_t i1 = 0; i1 < (index_t)v1.size(); i1++)
    {
        bool vi1_found = false;
        for(index_t i2 = 0; i2 < (index_t)v2.size() && !vi1_found; i2++)
        {
            vi1_found |= v1[i1] == v2[i2];
        }

        if(!vi1_found)
        {
            res.push_back(v1[i1]);
        }
    }
    return res;
}

//-----------------------------------------------------------------------------
// - end internal potpourri functions -
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// - begin internal helper functions -
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool verify_field_exists(const std::string &protocol,
                         const conduit::Node &node,
                         conduit::Node &info,
                         const std::string &field_name = "")
{
    bool res = true;

    if(field_name != "")
    {
        if(!node.has_child(field_name))
        {
            log::error(info, protocol, "missing child" + log::quote(field_name, 1));
            res = false;
        }

        log::validation(info[field_name], res);
    }

    return res;
}

//-----------------------------------------------------------------------------
bool verify_integer_field(const std::string &protocol,
                          const conduit::Node &node,
                          conduit::Node &info,
                          const std::string &field_name = "")
{
    Node &field_info = (field_name != "") ? info[field_name] : info;

    bool res = verify_field_exists(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = (field_name != "") ? node[field_name] : node;

        if(!field_node.dtype().is_integer())
        {
            log::error(info, protocol, log::quote(field_name) + "is not an integer (array)");
            res = false;
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_number_field(const std::string &protocol,
                         const conduit::Node &node,
                         conduit::Node &info,
                         const std::string &field_name = "")
{
    Node &field_info = (field_name != "") ? info[field_name] : info;

    bool res = verify_field_exists(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = (field_name != "") ? node[field_name] : node;

        if(!field_node.dtype().is_number())
        {
            log::error(info, protocol, log::quote(field_name) + "is not a number");
            res = false;
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_string_field(const std::string &protocol,
                         const conduit::Node &node,
                         conduit::Node &info,
                         const std::string &field_name = "")
{
    Node &field_info = (field_name != "") ? info[field_name] : info;

    bool res = verify_field_exists(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = (field_name != "") ? node[field_name] : node;

        if(!field_node.dtype().is_string())
        {
            log::error(info, protocol, log::quote(field_name) + "is not a string");
            res = false;
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_object_field(const std::string &protocol,
                         const conduit::Node &node,
                         conduit::Node &info,
                         const std::string &field_name = "",
                         const bool allow_list = false,
                         const bool allow_empty = false,
                         const index_t num_children = 0)
{
    Node &field_info = (field_name != "") ? info[field_name] : info;

    bool res = verify_field_exists(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = (field_name != "") ? node[field_name] : node;

        if(!(field_node.dtype().is_object() ||
            (allow_list && field_node.dtype().is_list())))
        {
            log::error(info, protocol, log::quote(field_name) + "is not an object" +
                                       (allow_list ? " or a list" : ""));
            res = false;
        }
        else if(!allow_empty && field_node.number_of_children() == 0)
        {
            log::error(info,protocol, "has no children");
            res = false;
        }
        else if(num_children && field_node.number_of_children() != num_children)
        {
            std::ostringstream oss;
            oss << "has incorrect number of children ("
                << field_node.number_of_children()
                << " vs "
                << num_children
                << ")";
            log::error(info,protocol, oss.str());
            res = false;
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_mcarray_field(const std::string &protocol,
                          const conduit::Node &node,
                          conduit::Node &info,
                          const std::string &field_name)
{
    Node &field_info = info[field_name];

    bool res = verify_field_exists(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = node[field_name];
        res = blueprint::mcarray::verify(field_node,field_info);
        if(res)
        {
            log::info(info, protocol, log::quote(field_name) + "is an mcarray");
        }
        else
        {
            log::error(info, protocol, log::quote(field_name) + "is not an mcarray");
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_mlarray_field(const std::string &protocol,
                          const conduit::Node &node,
                          conduit::Node &info,
                          const std::string &field_name,
                          const index_t min_depth,
                          const index_t max_depth,
                          const bool leaf_uniformity)
{
    Node &field_info = info[field_name];

    bool res = verify_field_exists(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = node[field_name];
        res = blueprint::mlarray::verify(field_node,field_info,min_depth,max_depth,leaf_uniformity);
        if(res)
        {
            log::info(info, protocol, log::quote(field_name) + "is an mlarray");
        }
        else
        {
            log::error(info, protocol, log::quote(field_name) + "is not an mlarray");
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_o2mrelation_field(const std::string &protocol,
                              const conduit::Node &node,
                              conduit::Node &info,
                              const std::string &field_name)
{
    Node &field_info = info[field_name];

    bool res = verify_field_exists(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = node[field_name];
        res = blueprint::o2mrelation::verify(field_node,field_info);
        if(res)
        {
            log::info(info, protocol, log::quote(field_name) + "describes a one-to-many relation");
        }
        else
        {
            log::error(info, protocol, log::quote(field_name) + "doesn't describe a one-to-many relation");
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_enum_field(const std::string &protocol,
                       const conduit::Node &node,
                       conduit::Node &info,
                       const std::string &field_name,
                       const std::vector<std::string> &enum_values )
{
    Node &field_info = (field_name != "") ? info[field_name] : info;

    bool res = verify_string_field(protocol, node, info, field_name);
    if(res)
    {
        const Node &field_node = (field_name != "") ? node[field_name] : node;

        const std::string field_value = field_node.as_string();
        bool is_field_enum = false;
        for(size_t i=0; i < enum_values.size(); i++)
        {
            is_field_enum |= (field_value == enum_values[i]);
        }

        if(is_field_enum)
        {
            log::info(info, protocol, log::quote(field_name) +
                                      "has valid value" +
                                      log::quote(field_value, 1));
        }
        else
        {
            log::error(info, protocol, log::quote(field_name) +
                                       "has invalid value" +
                                       log::quote(field_value, 1));
            res = false;
        }
    }

    log::validation(field_info, res);

    return res;
}


//-----------------------------------------------------------------------------
bool verify_reference_field(const std::string &protocol,
                            const conduit::Node &node_tree,
                            conduit::Node &info_tree,
                            const conduit::Node &node,
                            conduit::Node &info,
                            const std::string &field_name,
                            const std::string &ref_path)
{
    bool res = verify_string_field(protocol, node, info, field_name);
    if(res)
    {
        const std::string ref_name = node[field_name].as_string();

        if(!node_tree.has_child(ref_path) || !node_tree[ref_path].has_child(ref_name))
        {
            log::error(info, protocol, "reference to non-existent " + field_name +
                                        log::quote(ref_name, 1));
            res = false;
        }
        else if(info_tree[ref_path][ref_name]["valid"].as_string() != "true")
        {
            log::error(info, protocol, "reference to invalid " + field_name +
                                       log::quote(ref_name, 1));
            res = false;
        }
    }

    log::validation(info[field_name], res);
    log::validation(info, res);

    return res;
}

bool verify_shapes_node(const Node &node, const Node &shape_map, Node &info)
{
  const std::string protocol = "mesh::topology::unstructured";
  bool res = true;
  res &= verify_integer_field(protocol, node, info);

  const int32_accessor shapes = node.value();

  std::vector<int32> range;
  for(auto &child : shape_map.children())
  {
    range.emplace_back(child.to_int32());
  }

  for(index_t i = 0; i < shapes.number_of_elements(); ++i)
  {
    const int32 shape = shapes[i];
    const bool expectedShape = std::find(range.begin(), range.end(), shape) != range.end();
    if (!expectedShape)
    {
      log::error(info, protocol, "shape not in shape_map");
    }
    res &= expectedShape;
  }

  if (res)
  {
    for(auto &child : shape_map.children())
    {
      log::info(info, protocol, "cells found for shape "
        + child.name() + " ("
        + std::to_string(child.to_int32())
        + ").");
    }
  }

  return res;
}

bool verify_mixed_elements_node(const Node& topo_elems, Node &info_elems, bool& elems_res)
{
  const std::string protocol = "mesh::topology::unstructured";
  elems_res &= verify_field_exists(protocol, topo_elems, info_elems, "shape") &&
      conduit::blueprint::mesh::topology::shape::verify(topo_elems["shape"], info_elems["shape"]);

  elems_res &= verify_field_exists(protocol, topo_elems, info_elems, "shape_map") &&
      conduit::blueprint::mesh::topology::shape_map::verify(topo_elems["shape_map"], info_elems["shape_map"]);

  elems_res &= verify_field_exists(protocol, topo_elems, info_elems, "shapes") &&
      verify_shapes_node(topo_elems["shapes"], topo_elems["shape_map"], info_elems["shapes"]);

  return elems_res;
}

bool verify_mixed_node(const Node &topo, Node &info, bool& elems_res, bool& subelems_res)
{
    const std::string protocol = "mesh::topology::unstructured";
    const Node& topo_elems = topo["elements"];
    Node& info_elems = info["elements"];

    elems_res &= verify_mixed_elements_node(topo_elems, info_elems, elems_res);
    elems_res &= verify_o2mrelation_field(protocol, topo, info, "elements");

    // if polyhedra in mixed definition, the polyhedra have their faces in subelements
    if (topo.has_child("subelements"))
    {
        const Node& topo_subelems = topo["subelements"];
        Node& info_subelems = info["subelements"];

        subelems_res &= verify_mixed_elements_node(topo_subelems, info_subelems, subelems_res);
        subelems_res &= verify_o2mrelation_field(protocol, topo, info, "subelements");
    }
    return elems_res && subelems_res;
}

//-----------------------------------------------------------------------------
bool verify_poly_node(bool is_mixed_topo,
                      std::string name,
                      const conduit::Node &node,
                      conduit::Node &node_info,
                      const conduit::Node &topo,
                      conduit::Node &info,
                      bool &elems_res)
{
    const std::string protocol = "mesh::topology::unstructured";
    bool node_res = true;

    // Polygonal & Polyhedral shape
    if(node.has_child("shape") &&
       node["shape"].dtype().is_string() &&
       (node["shape"].as_string() == "polygonal" ||
       node["shape"].as_string() == "polyhedral"))
    {
        node_res &= blueprint::o2mrelation::verify(node, node_info);

        // Polyhedral - Check for subelements
        if (node["shape"].as_string() == "polyhedral")
        {
            bool subnode_res = true;
            if(!verify_object_field(protocol, topo, info, "subelements"))
            {
                subnode_res = false;
            }
            else
            {
                const Node &topo_subelems = topo["subelements"];
                Node &info_subelems = info["subelements"];
                bool has_subnames = topo_subelems.dtype().is_object();

                // Look for child "name" if mixed topology case,
                // otherwise look for "shape" variable.
                name = is_mixed_topo ? name : "shape";
                if(!topo_subelems.has_child(name))
                {
                    subnode_res = false;
                }
                // Checks for topo["subelements"]["name"]["shape"] with mixed topology,
                // or topo["subelements"]["shape"] with single topology,
                else
                {
                    const Node &sub_node  = is_mixed_topo ? topo_subelems[name] : topo_subelems;
                    Node &subnode_info =
                        !is_mixed_topo ? info_subelems :
                        has_subnames ? info["subelements"][name] :
                        info["subelements"].append();

                    if(sub_node.has_child("shape"))
                    {
                        subnode_res &= verify_field_exists(protocol, sub_node, subnode_info, "shape") &&
                        blueprint::mesh::topology::shape::verify(sub_node["shape"], subnode_info["shape"]);
                        subnode_res &= verify_integer_field(protocol, sub_node, subnode_info, "connectivity");
                        subnode_res &= sub_node["shape"].as_string() == "polygonal";
                        subnode_res &= blueprint::o2mrelation::verify(sub_node, subnode_info);
                    }
                    else
                    {
                        subnode_res = false;
                    }

                    log::validation(subnode_info,subnode_res);
                }
                log::validation(info_subelems, subnode_res);
            }
            elems_res &= subnode_res;
        }
    }
    node_res &= elems_res;
    return node_res;
}


//-----------------------------------------------------------------------------
bool
verify_single_domain(const Node &n,
                     Node &info)
{
    const std::string protocol = "mesh";
    bool res = true;
    info.reset();

    if(!verify_object_field(protocol, n, info, "coordsets"))
    {
        res = false;
    }
    else
    {
        bool cset_res = true;
        NodeConstIterator itr = n["coordsets"].children();
        while(itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();

            cset_res &= blueprint::mesh::coordset::verify(chld, info["coordsets"][chld_name]);
        }

        log::validation(info["coordsets"],cset_res);
        res &= cset_res;
    }

    if(!verify_object_field(protocol, n, info, "topologies"))
    {
        res = false;
    }
    else
    {
        bool topo_res = true;
        NodeConstIterator itr = n["topologies"].children();
        while(itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();
            Node &chld_info = info["topologies"][chld_name];

            topo_res &= blueprint::mesh::topology::verify(chld, chld_info);
            topo_res &= verify_reference_field(protocol, n, info,
                chld, chld_info, "coordset", "coordsets");
        }

        log::validation(info["topologies"],topo_res);
        res &= topo_res;
    }

    // optional: "matsets", each child must conform to "mesh::matset"
    if(n.has_path("matsets"))
    {
        if(!verify_object_field(protocol, n, info, "matsets"))
        {
            res = false;
        }
        else
        {
            bool mset_res = true;
            NodeConstIterator itr = n["matsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["matsets"][chld_name];

                mset_res &= blueprint::mesh::matset::verify(chld, chld_info);
                mset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "topology", "topologies");
            }

            log::validation(info["matsets"],mset_res);
            res &= mset_res;
        }
    }

    // optional: "specsets", each child must conform to "mesh::specset"
    if(n.has_path("specsets"))
    {
        if(!verify_object_field(protocol, n, info, "specsets"))
        {
            res = false;
        }
        else
        {
            bool sset_res = true;
            NodeConstIterator itr = n["specsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["specsets"][chld_name];

                sset_res &= blueprint::mesh::specset::verify(chld, chld_info);
                sset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "matset", "matsets");
            }

            log::validation(info["specsets"],sset_res);
            res &= sset_res;
        }
    }

    // optional: "fields", each child must conform to "mesh::field"
    if(n.has_path("fields"))
    {
        if(!verify_object_field(protocol + "::fields", n, info, "fields"))
        {
            res = false;
        }
        else
        {
            bool field_res = true;
            NodeConstIterator itr = n["fields"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["fields"][chld_name];

                field_res &= blueprint::mesh::field::verify(chld, chld_info);
                if(chld.has_child("topology"))
                {
                    field_res &= verify_reference_field(protocol + "::field", n, info,
                        chld, chld_info, "topology", "topologies");
                }
                if(chld.has_child("matset"))
                {
                    field_res &= verify_reference_field(protocol + "::field", n, info,
                        chld, chld_info, "matset", "matsets");
                }
            }

            log::validation(info["fields"],field_res);
            res &= field_res;
        }
    }

    // optional: "adjsets", each child must conform to "mesh::adjset"
    if(n.has_path("adjsets"))
    {
        if(!verify_object_field(protocol, n, info, "adjsets"))
        {
            res = false;
        }
        else
        {
            bool aset_res = true;
            NodeConstIterator itr = n["adjsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["adjsets"][chld_name];

                aset_res &= blueprint::mesh::adjset::verify(chld, chld_info);
                aset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "topology", "topologies");
            }

            log::validation(info["adjsets"],aset_res);
            res &= aset_res;
        }
    }

    // optional: "nestsets", each child must conform to "mesh::nestset"
    if(n.has_path("nestsets"))
    {
        if(!verify_object_field(protocol, n, info, "nestsets"))
        {
            res = false;
        }
        else
        {
            bool nset_res = true;
            NodeConstIterator itr = n["nestsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["nestsets"][chld_name];

                nset_res &= blueprint::mesh::nestset::verify(chld, chld_info);
                nset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "topology", "topologies");
            }

            log::validation(info["nestsets"],nset_res);
            res &= nset_res;
        }
    }


    // one last pass to make sure if a grid_function was specified by a topo,
    // it is valid
    if (n.has_child("topologies"))
    {
        bool topo_res = true;
        NodeConstIterator itr = n["topologies"].children();
        while (itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();
            Node &chld_info = info["topologies"][chld_name];

            if(chld.has_child("grid_function"))
            {
                topo_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "grid_function", "fields");
            }
        }

        log::validation(info["topologies"],topo_res);
        res &= topo_res;
    }

    log::validation(info,res);

    return res;
}


//-------------------------------------------------------------------------
bool
verify_multi_domain(const Node &n,
                    Node &info)
{
    const std::string protocol = "mesh";
    bool res = true;
    info.reset();

    if(!n.dtype().is_object() && !n.dtype().is_list() && !n.dtype().is_empty())
    {
        log::error(info, protocol, "not an object, a list, or empty");
        res = false;
    }
    else
    {
        if(n.dtype().is_empty() || n.number_of_children() == 0)
        {
            log::info(info, protocol, "is an empty mesh");
        }
        else
        {
            NodeConstIterator itr = n.children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                res &= verify_single_domain(chld, info[chld_name]);
            }
        }

        log::info(info, protocol, "is a multi domain mesh");
    }

    log::validation(info,res);

    return res;
}

//-----------------------------------------------------------------------------
// - end internal data function helpers -
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// - start internal topology helpers -
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------
void
convert_coordset_to_rectilinear(const std::string &/*base_type*/,
                                const conduit::Node &coordset,
                                conduit::Node &dest)
{ 
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    // bool is_base_uniform = true;

    dest.reset();
    dest["type"].set("rectilinear");

    DataType float_dtype = bputils::find_widest_dtype(coordset, bputils::DEFAULT_FLOAT_DTYPE);

    const std::vector<std::string> csys_axes = bputils::coordset::axes(coordset);
    const std::vector<std::string> &logical_axes = bputils::LOGICAL_AXES;
    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        const std::string& csys_axis = csys_axes[i];
        const std::string& logical_axis = logical_axes[i];

        float64 dim_origin = coordset.has_child("origin") ?
            coordset["origin"][csys_axis].to_float64() : 0.0;
        float64 dim_spacing = coordset.has_child("spacing") ?
            coordset["spacing"]["d"+csys_axis].to_float64() : 1.0;
        index_t dim_len = coordset["dims"][logical_axis].to_int64();

        Node &dst_cvals_node = dest["values"][csys_axis];
        dst_cvals_node.set(DataType(float_dtype.id(), dim_len));

        Node src_cval_node, dst_cval_node;
        for(index_t d = 0; d < dim_len; d++)
        {
            // TODO: USE ACCESSORS
            src_cval_node.set(dim_origin + d * dim_spacing);
            dst_cval_node.set_external(float_dtype, dst_cvals_node.element_ptr(d));
            src_cval_node.to_data_type(float_dtype.id(), dst_cval_node);
        }
    }
}

//-------------------------------------------------------------------------
void
convert_coordset_to_explicit(const std::string &base_type,
                             const conduit::Node &coordset,
                             conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    bool is_base_rectilinear = base_type == "rectilinear";
    bool is_base_uniform = base_type == "uniform";

    dest.reset();
    dest["type"].set("explicit");

    DataType float_dtype = bputils::find_widest_dtype(coordset, bputils::DEFAULT_FLOAT_DTYPE);

    const std::vector<std::string> csys_axes = bputils::coordset::axes(coordset);
    const std::vector<std::string> &logical_axes = bputils::LOGICAL_AXES;

    index_t dim_lens[3] = {0, 0, 0}, coords_len = 1;
    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        dim_lens[i] = is_base_rectilinear ?
            coordset["values"][csys_axes[i]].dtype().number_of_elements() :
            coordset["dims"][logical_axes[i]].to_int64();
        coords_len *= dim_lens[i];
    }

    Node info;
    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        const std::string& csys_axis = csys_axes[i];

        // NOTE: The following values are specific to the
        // rectilinear transform case.
        const Node &src_cvals_node = coordset.has_child("values") ?
            coordset["values"][csys_axis] : info;
        float64_accessor src_cvals_acc = src_cvals_node.value();
        // NOTE: The following values are specific to the
        // uniform transform case.
        float64 dim_origin = coordset.has_child("origin") ?
            coordset["origin"][csys_axis].to_float64() : 0.0;
        float64 dim_spacing = coordset.has_child("spacing") ?
            coordset["spacing"]["d"+csys_axis].to_float64() : 1.0;

        index_t dim_block_size = 1, dim_block_count = 1;
        for(index_t j = 0; j < (index_t)csys_axes.size(); j++)
        {
            dim_block_size *= (j < i) ? dim_lens[j] : 1;
            dim_block_count *= (i < j) ? dim_lens[j] : 1;
        }

        Node &dst_cvals_node = dest["values"][csys_axis];
        dst_cvals_node.set(DataType(float_dtype.id(), coords_len));

        float64_accessor dst_cvals_acc = dst_cvals_node.value();

        for(index_t d = 0; d < dim_lens[i]; d++)
        {
            index_t doffset = d * dim_block_size;
            for(index_t b = 0; b < dim_block_count; b++)
            {
                index_t boffset = b * dim_block_size * dim_lens[i];
                for(index_t bi = 0; bi < dim_block_size; bi++)
                {
                    index_t ioffset = doffset + boffset + bi;
                    if(is_base_rectilinear)
                    {
                        dst_cvals_acc.set(ioffset, src_cvals_acc[d]);
                    }
                    else if(is_base_uniform)
                    {
                        dst_cvals_acc.set(ioffset, dim_origin + d * dim_spacing);
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------------------
void
convert_oneD_coordset_to_strip(const conduit::Node &coordset,
                               conduit::Node &dest)
{
    dest.reset();
    std::string coord_type = coordset["type"].as_string();
    dest["type"].set(coord_type);

    if (coord_type == "uniform")
    {
        dest["dims/i"] = 1;
        dest["dims/j"] = coordset["dims/i"];

        if (coordset.has_child("origin"))
        {
            dest["origin/x"] = 0.;
            dest["origin/y"] = coordset["origin/x"];
        }

        if (coordset.has_child("spacing"))
        {
            dest["spacing/dx"] = 1.;
            dest["spacing/dy"] = coordset["spacing/dx"];
        }
    }
    else
    {
        dest["values/x"].set(DataType::float64(2));
        double *x_vals = dest["values/x"].value();
        x_vals[0] = 0.;
        x_vals[1] = 1.;
        coordset["values/x"].to_float64_array(dest["values/y"]);
    }
}


// TODO(JRC): For all of the following topology conversion functions, it's
// possible if the user validates the topology in isolation that it can be
// good and yet the conversion will fail due to an invalid reference coordset.
// In order to eliminate this concern, it may be better to update the mesh
// verify code so that "topology::verify" verifies reference fields, which
// would enable more assurances.

//-------------------------------------------------------------------------
void
convert_topology_to_rectilinear(const std::string &/*base_type*/,
                                const conduit::Node &topo,
                                conduit::Node &dest,
                                conduit::Node &cdest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    // bool is_base_uniform = true;

    dest.reset();
    cdest.reset();

    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    blueprint::mesh::coordset::uniform::to_rectilinear(*coordset, cdest);

    dest.set(topo);
    dest["type"].set("rectilinear");
    dest["coordset"].set(cdest.name());
}

//-------------------------------------------------------------------------
void
convert_topology_to_structured(const std::string &base_type,
                               const conduit::Node &topo,
                               conduit::Node &dest,
                               conduit::Node &cdest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    bool is_base_rectilinear = base_type == "rectilinear";
    bool is_base_uniform = base_type == "uniform";

    dest.reset();
    cdest.reset();

    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    if(is_base_rectilinear)
    {
        blueprint::mesh::coordset::rectilinear::to_explicit(*coordset, cdest);
    }
    else if(is_base_uniform)
    {
        blueprint::mesh::coordset::uniform::to_explicit(*coordset, cdest);
    }

    dest["type"].set("structured");
    dest["coordset"].set(cdest.name());
    if(topo.has_child("origin"))
    {
        dest["origin"].set(topo["origin"]);
    }

    // TODO(JRC): In this case, should we reach back into the coordset
    // and use its types to inform those of the topology?
    DataType int_dtype = bputils::find_widest_dtype(topo, bputils::DEFAULT_INT_DTYPES);

    const std::vector<std::string> csys_axes = bputils::coordset::axes(*coordset);
    const std::vector<std::string> &logical_axes = bputils::LOGICAL_AXES;
    for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
    {
        Node src_dlen_node;
        src_dlen_node.set(is_base_uniform ?
            (*coordset)["dims"][logical_axes[i]].to_int64() :
            (*coordset)["values"][csys_axes[i]].dtype().number_of_elements());
        // NOTE: The number of elements in the topology is one less
        // than the number of points along each dimension.
        src_dlen_node.set(src_dlen_node.to_int64() - 1);

        Node &dst_dlen_node = dest["elements/dims"][logical_axes[i]];
        src_dlen_node.to_data_type(int_dtype.id(), dst_dlen_node);
    }
}

//-------------------------------------------------------------------------
void
convert_topology_to_unstructured(const std::string &base_type,
                                 const conduit::Node &topo,
                                 conduit::Node &dest,
                                 conduit::Node &cdest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    bool is_base_structured = base_type == "structured";
    bool is_base_rectilinear = base_type == "rectilinear";
    bool is_base_uniform = base_type == "uniform";

    dest.reset();
    cdest.reset();

    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    if(is_base_structured)
    {
        cdest.set(*coordset);
    }
    else if(is_base_rectilinear)
    {
        blueprint::mesh::coordset::rectilinear::to_explicit(*coordset, cdest);
    }
    else if(is_base_uniform)
    {
        blueprint::mesh::coordset::uniform::to_explicit(*coordset, cdest);
    }

    dest["type"].set("unstructured");
    dest["coordset"].set(cdest.name());
    if(topo.has_child("origin"))
    {
        dest["origin"].set(topo["origin"]);
    }

    // TODO(JRC): In this case, should we reach back into the coordset
    // and use its types to inform those of the topology?
    DataType int_dtype = bputils::find_widest_dtype(topo, bputils::DEFAULT_INT_DTYPES);

    const std::vector<std::string> csys_axes = bputils::coordset::axes(*coordset);
    dest["elements/shape"].set(
        (csys_axes.size() == 1) ? "line" : (
        (csys_axes.size() == 2) ? "quad" : (
        (csys_axes.size() == 3) ? "hex"  : "")));
    const std::vector<std::string> &logical_axes = bputils::LOGICAL_AXES;

    index_t edims_axes[3] = {1, 1, 1};
    if(is_base_structured)
    {
        const conduit::Node &dim_node = topo["elements/dims"];
        for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
        {
            edims_axes[i] = dim_node[logical_axes[i]].to_int();
        }
    }
    else if(is_base_rectilinear)
    {
        const conduit::Node &dim_node = (*coordset)["values"];
        for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
        {
            edims_axes[i] =
                dim_node[csys_axes[i]].dtype().number_of_elements() - 1;
        }
    }
    else if(is_base_uniform)
    {
        const conduit::Node &dim_node = (*coordset)["dims"];
        for(index_t i = 0; i < (index_t)csys_axes.size(); i++)
        {
            edims_axes[i] = dim_node[logical_axes[i]].to_int() - 1;
        }
    }

    index_t vdims_axes[3] = {1, 1, 1}, num_elems = 1;
    for(index_t d = 0; d < 3; d++)
    {
        num_elems *= edims_axes[d];
        vdims_axes[d] = edims_axes[d] + 1;
    }
    index_t indices_per_elem = (index_t) pow(2, csys_axes.size());

    CONDUIT_ANNOTATE_MARK_BEGIN("to_unstructured_index_gen");
    conduit::Node &conn_node = dest["elements/connectivity"];
    conn_node.set(DataType(int_dtype.id(), num_elems * indices_per_elem));

    int64_accessor conn_node_vals = conn_node.value();
    Node src_idx_node, dst_idx_node;
    index_t curr_elem[3], curr_vert[3];
    index_t idx=0;
    for(index_t e = 0; e < num_elems; e++)
    {
        grid_id_to_ijk(e, &edims_axes[0], &curr_elem[0]);

        // NOTE(JRC): In order to get all adjacent vertices for the
        // element, we use the bitwise interpretation of each index
        // per element to inform the direction (e.g. 5, which is
        // 101 bitwise, means (z+1, y+0, x+1)).
        for(index_t i = 0, v = 0; i < indices_per_elem; i++)
        {
            memcpy(&curr_vert[0], &curr_elem[0], 3 * sizeof(index_t));
            for(index_t d = 0; d < (index_t)csys_axes.size(); d++)
            {
                curr_vert[d] += (i & (index_t)pow(2, d)) >> d;
            }
            grid_ijk_to_id(&curr_vert[0], &vdims_axes[0], v);

            conn_node_vals.set(idx,v);
            idx++;
        }

        // TODO(JRC): This loop inverts quads/hexes to conform to
        // the default Blueprint ordering. Once the ordering transforms
        // are introduced, this code should be removed and replaced
        // with initializing the ordering label value.
        for(index_t p = 2; p < indices_per_elem; p += 4)
        {
            index_t p1 = e * indices_per_elem + p;
            index_t p2 = e * indices_per_elem + p + 1;

            int64 value_swap = conn_node_vals[p1];
            conn_node_vals.set(p1,conn_node_vals[p2]);
            conn_node_vals.set(p2,value_swap);
        }
    }

    CONDUIT_ANNOTATE_MARK_END("to_unstructured_index_gen");
}

//-------------------------------------------------------------------------
/**
 @brief This function scans a list of values and stores a 1 for the first
        occurance of each unique value. Subsequent occurances of repeated
        values get a 0.

 @param values The sequence of values to be searched.
 @param offset An offset from the start of values.
 @param n The number of values in the sequence.
 @param mask A buffer in which to store the mask. It must have at least n
             elements.
 
 @return True if there were dups; False otherwise.

 @note This function could be useful in a few places. I might move it later.
*/
template <typename Container>
bool
unique_mask(const Container &values, index_t offset, index_t n, int *mask)
{
#define LUT
#ifdef LUT
    // Look up tables for the comparisons we make for n<=8.
    static const int ncaseslut[] = {0,0,1,3,6,10,15,21,28};
    static const int offsets[]   = {0,0,0,1,4,10,20,35,56};
    static const int leftlut[] = {
        // 2 values
        0,
        // 3 values
        0,0,
        1,
        // 4 values
        0,0,0,
        1,1,
        2,
        // 5 values
        0,0,0,0,
        1,1,1,
        2,2,
        3,
        // 6 values
        0,0,0,0,0,
        1,1,1,1,
        2,2,2,
        3,3,
        4,
        // 7 values
        0,0,0,0,0,0,
        1,1,1,1,1,
        2,2,2,2,
        3,3,3,
        4,4,
        5,
        // 8 values
        0,0,0,0,0,0,0,
        1,1,1,1,1,1,
        2,2,2,2,2,
        3,3,3,3,
        4,4,4,
        5,5,
        6
      };
    static const int rightlut[] = {
        // 2 values
        1,
        // 3 values
        1,2,
        2,
        // 4 values
        1,2,3,
        2,3,
        3,
        // 5 values
        1,2,3,4,
        2,3,4,
        3,4,
        4,
        // 6 values
        1,2,3,4,5,
        2,3,4,5,
        3,4,5,
        4,5,
        5,
        // 7 values
        1,2,3,4,5,6,
        2,3,4,5,6,
        3,4,5,6,
        4,5,6,
        5,6,
        6,
        // 8 values
        1,2,3,4,5,6,7,
        2,3,4,5,6,7,
        3,4,5,6,7,
        4,5,6,7,
        5,6,7,
        6,7,
        7
    };
#endif
    // The mask is the same size as the values vector.
    constexpr index_t onemask = 1;
    for(index_t i = 0; i < n; i++)
        mask[i] = onemask;
    bool needmask = false;

    // LUTs faster are a bit faster than loops.
#ifdef LUT
    if(n <= 8)
    {
        // Make the mask using the LUT values.
        int ncases = ncaseslut[n];
        const int *left = &leftlut[offsets[n]];
        const int *right = &rightlut[offsets[n]];
        if(offset == 0)
        {
            for(int i = 0; i < ncases; i++)
            {
                if(values[left[i]] == values[right[i]])
                {
                    mask[right[i]]--;
                    needmask = true;
                }
            }
        }
        else
        {
            for(int i = 0; i < ncases; i++)
            {
                if(values[offset + left[i]] == values[offset + right[i]])
                {
                    mask[right[i]]--;
                    needmask = true;
                }
            }
        }
    }
    else
    {
#endif
        // Make the mask using loops
        if(offset == 0)
        {
            for(int row = 0; row < n; row++)
            {
                for(int col = row + 1; col <= n; col++)
                {
                    if(values[row] == values[col])
                    {
                        mask[col]--;
                        needmask = true;
                    }
                }
            }
        }
        else
        {
            for(int row = 0; row < n; row++)
            {
                for(int col = row + 1; col <= n; col++)
                {
                    if(values[offset + row] == values[offset + col])
                    {
                        mask[col]--;
                        needmask = true;
                    }
                }
            }
        }
#ifdef LUT
    }
#endif
    return needmask;
}

//-------------------------------------------------------------------------
// NOTE: This function is templated to support passing raw pointers (as
//       well as accessors) and for passing in a custom function to store
//       the data.
template <typename IndexType, typename CoordType, typename StorageFunc>
void
unstructured_centroid(const ShapeType &topo_shape,
                      const IndexType &topo_conn,
                      const IndexType &topo_offsets,
                      const IndexType &topo_sizes,
                      index_t topo_num_elems,
                      const CoordType &coords,
                      index_t ncoord_dims,
                      StorageFunc &&store)
{
    constexpr size_t max_size_guess = 12;
    std::vector<index_t> eids;
    std::vector<int> mask;
    eids.reserve(max_size_guess);
    mask.reserve(max_size_guess);

    bool is_polygonal = topo_shape.is_polygonal();
    for(index_t ei = 0; ei < topo_num_elems; ei++)
    {
        const index_t eoffset = topo_offsets[ei];
        const index_t npts = is_polygonal
            ? topo_sizes[ei] : topo_shape.indices;

        // Just in case for larger polygons
        mask.resize(npts);
        eids.resize(npts);

        // Assuming topo_conn is an index_t accessor. We copy the data
        // out to an actual index_t buffer first.
        for(index_t ci = 0; ci < npts; ci++)
            eids[ci] = topo_conn[eoffset + ci];

        // Compute a mask that identifies the unique points. No sorting.
        bool needmask = unique_mask(&eids[0], 0, npts, &mask[0]);

        // Accumulate unique node values for centroid, using mask.
        float64 centroid[3]={0.,0.,0.};
        index_t npts_used = 0;
        if(needmask)
        {
            for(index_t ci = 0; ci < npts; ci++)
            {
                if(mask[ci])
                {
                    auto id = eids[ci];
                    for(index_t ai = 0; ai < ncoord_dims; ai++)
                        centroid[ai] += coords[ai][id];
                    npts_used++;
                }
            }
        }
        else
        {
            // We don't need to use the mask - save some branches.
            npts_used = npts;
            for(index_t ci = 0; ci < npts; ci++)
            {
                auto id = eids[ci];
                for(index_t ai = 0; ai < ncoord_dims; ai++)
                    centroid[ai] += coords[ai][id];
            }
        }
        // Average the values.
        float64 one_over_npts = 1. / static_cast<float64>(npts_used);
        for(index_t ai = 0; ai < ncoord_dims; ai++)
            centroid[ai] *= one_over_npts;

        // Store the centroid.
        store(ei, centroid);
    }
}

//-------------------------------------------------------------------------
// NOTE: This function is templated to support passing raw pointers (as
//       well as accessors) and for passing in a custom function to store
//       the data.
template <typename IndexType, typename CoordType, typename StorageFunc>
void
unstructured_centroid_polyhedral(const ShapeType &/*topo_shape*/,
                                 const IndexType &topo_conn,
                                 const IndexType &topo_offsets,
                                 const IndexType &topo_sizes,
                                 const IndexType &topo_subconn,
                                 const IndexType &topo_suboffsets,
                                 const IndexType &topo_subsizes,
                                 index_t topo_num_elems,
                                 const CoordType &coords,
                                 index_t ncoord_dims,
                                 StorageFunc &&store)
{
    std::vector<index_t> elem_coord_indices;
    elem_coord_indices.reserve(12);

    for(index_t ei = 0; ei < topo_num_elems; ei++)
    {
        const index_t eoffset = topo_offsets[ei];

        // Determine the unique points in the element.
        elem_coord_indices.clear();
        const index_t elem_num_faces = topo_sizes[ei];
        for(index_t fi = 0, foffset = eoffset; fi < elem_num_faces; fi++)
        {
            index_t subelem_index = topo_conn[foffset];
            index_t subelem_offset = topo_suboffsets[subelem_index];
            index_t subelem_size = topo_subsizes[subelem_index];

            const index_t face_num_coords = subelem_size;
            for(index_t ci = 0; ci < face_num_coords; ci++)
            {
                index_t id = topo_subconn[subelem_offset + ci];
                if(std::find(elem_coord_indices.cbegin(),
                             elem_coord_indices.cend(), id)
                   == elem_coord_indices.cend())
                {
                    elem_coord_indices.push_back(id);
                }
            }
            foffset++;
        }

        // Compute the centroid.
        float64 centroid[3] = {0., 0., 0.};
        float64 one_over_npts = 1. / static_cast<float64>(elem_coord_indices.size());
        for(index_t ai = 0; ai < ncoord_dims; ai++)
        {
            for(const auto ci : elem_coord_indices)
                centroid[ai] += coords[ai][ci];
            centroid[ai] *= one_over_npts;
        }

        // Store the centroid.
        store(ei, centroid);
    }
}

// NOTE(JRC): The following two functions need to be passed the coordinate set
// and can't use 'find_reference_node' because these internal functions aren't
// guaranteed to be passed nodes that exist in the context of an existing mesh
// tree ('generate_corners' has a good example wherein an in-situ edge topology
// is used to contruct an in-situ centroid topology).

//-------------------------------------------------------------------------
void
calculate_unstructured_centroids(const conduit::Node &topo,
                                 const conduit::Node &coordset,
                                 conduit::Node &dest,
                                 conduit::Node &cdest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // NOTE(JRC): This is a stand-in implementation for the method
    // 'mesh::topology::unstructured::generate_centroids' that exists because there
    // is currently no good way in Blueprint to create mappings with sparse data.
    const std::vector<std::string> csys_axes = bputils::coordset::axes(coordset);

    const ShapeCascade topo_cascade(topo);
    const ShapeType &topo_shape = topo_cascade.get_shape();

    Node topo_offsets, topo_suboffsets;

    if (topo_shape.is_polyhedral())
    {
        bputils::topology::unstructured::generate_offsets(topo,
                                                          topo_offsets,
                                                          topo_suboffsets);
    }
    else
    {
        bputils::topology::unstructured::generate_offsets(topo, topo_offsets);
    }

    const index_t topo_num_elems = topo_offsets.dtype().number_of_elements();

    Node topo_sizes;
    if (topo_shape.is_poly())
    {
      topo_sizes = topo["elements/sizes"];
    }

    Node topo_subconn;
    Node topo_subsizes;
    if (topo_shape.is_polyhedral())
    {
        topo_subconn.set_external(topo["subelements/connectivity"]);
        topo_subsizes.set_external(topo["subelements/sizes"]);
    }

    // Discover Data Types //

    DataType int_dtype, float_dtype;
    {
        conduit::Node src_node;
        src_node["topology"].set_external(topo);
        src_node["coordset"].set_external(coordset);
        int_dtype = bputils::find_widest_dtype(src_node, bputils::DEFAULT_INT_DTYPES);
        float_dtype = bputils::find_widest_dtype(src_node, bputils::DEFAULT_FLOAT_DTYPE);
    }

    const Node &topo_conn_const = topo["elements/connectivity"];
    Node topo_conn; topo_conn.set_external(topo_conn_const);
    const DataType conn_dtype(topo_conn.dtype().id(), 1);
    const DataType offset_dtype(topo_offsets.dtype().id(), 1);
    const DataType size_dtype(topo_sizes.dtype().id(), 1);

    const DataType subconn_dtype(topo_subconn.dtype().id(), 1);
    const DataType suboffset_dtype(topo_suboffsets.dtype().id(), 1);
    const DataType subsize_dtype(topo_subsizes.dtype().id(), 1);

    // Allocate Data Templates for Outputs //

    dest.reset();
    dest["type"].set("unstructured");
    dest["coordset"].set(cdest.name());
    dest["elements/shape"].set(topo_cascade.get_shape(0).type());
    dest["elements/connectivity"].set(DataType(int_dtype.id(), topo_num_elems));

    cdest.reset();
    cdest["type"].set("explicit");
    for(index_t ai = 0; ai < (index_t)csys_axes.size(); ai++)
    {
        cdest["values"][csys_axes[ai]].set(DataType(float_dtype.id(), topo_num_elems));
    }

    // Compute Data for Centroid Topology //

    // Store the element ids into the connectivity - with some fast paths.
    // Should we make a Node::iota() function?
    Node &dest_elem_conn = dest["elements/connectivity"];
    if(dest_elem_conn.dtype().is_int64())
    {
        auto conn = dest_elem_conn.as_int64_ptr();
        auto n = static_cast<int64>(topo_num_elems);
        for(int64 i = 0; i < n; i++)
            conn[i] = i;
    }
    else if(dest_elem_conn.dtype().is_int32())
    {
        auto conn = dest_elem_conn.as_int32_ptr();
        auto n = static_cast<int32>(topo_num_elems);
        for(int32 i = 0; i < n; i++)
            conn[i] = i;
    }
    else
    {
        // Store generally - but SLOW.
        int64 ei_value;
        Node data_node;
        Node ei_data(DataType::int64(1), &ei_value, true);
        auto n = static_cast<int64>(topo_num_elems);
        for(ei_value = 0; ei_value < n; ei_value++)
        {
            // TODO: USE ACCESSORS
            // Use data_node to wrap connectivity[ei].
            data_node.set_external(int_dtype, dest_elem_conn.element_ptr(ei_value));
            // Convert ei_data to int, store in data_node.
            ei_data.to_data_type(int_dtype.id(), data_node);
        }
    }

    // Create some accessors to access the data.
    index_t csys_axes_size = csys_axes.size();
    std::vector<float64_accessor> axis_data_access;
    for(index_t ai = 0; ai < csys_axes_size; ai++)
    {
        axis_data_access.push_back(coordset["values"][csys_axes[ai]].as_float64_accessor());
    }

    const auto topo_sizes_access = topo_sizes.as_index_t_accessor();
    const auto topo_offsets_access = topo_offsets.as_index_t_accessor();
    const auto topo_conn_access = topo_conn.as_index_t_accessor();

    // Get the dest nodes to save on lookups later.
    std::vector<Node *> dest_centroid;
    for(index_t ai = 0; ai < csys_axes_size; ai++)
        dest_centroid.push_back(cdest["values"].fetch_ptr(csys_axes[ai]));

    // NOTE: We're primarily dispatching to different template functions that
    //       let us pick how to store the centroid results. This was the latest
    //       slow part. The functions are templated though in case we wanted to
    //       pass raw index_t pointers (when appropriate) instead of using
    //       accessors.
    if(topo_shape.is_polyhedral())
    {
        const auto topo_subconn_access = topo_subconn.as_index_t_accessor();
        const auto topo_suboffsets_access = topo_suboffsets.as_index_t_accessor();
        const auto topo_subsizes_access = topo_subsizes.as_index_t_accessor();

        if(float_dtype.is_float64())
        {
            // Get pointers to where we'll write the centroid data directly.
            float64 *typed_dest_centroid[3] = {nullptr, nullptr, nullptr};
            for(index_t ai = 0; ai < csys_axes_size; ai++)
                typed_dest_centroid[ai] = reinterpret_cast<float64 *>(dest_centroid[ai]->element_ptr(0));
            
            unstructured_centroid_polyhedral(topo_shape,
                topo_conn_access, topo_offsets_access, topo_sizes_access, 
                topo_subconn_access, topo_suboffsets_access, topo_subsizes_access,
                topo_num_elems,
                axis_data_access, csys_axes_size,
                [&](index_t ei, const float64 centroid[3])
            {
                for(index_t ai = 0; ai < csys_axes_size; ai++)
                    typed_dest_centroid[ai][ei] = centroid[ai];
            });
        }
        else //if(float_dtype.is_float32())
        {
            // Get pointers to where we'll write the centroid data directly.
            float32 *typed_dest_centroid[3] = {nullptr, nullptr, nullptr};
            for(index_t ai = 0; ai < csys_axes_size; ai++)
                typed_dest_centroid[ai] = reinterpret_cast<float32 *>(dest_centroid[ai]->element_ptr(0));

            unstructured_centroid_polyhedral(topo_shape,
                topo_conn_access, topo_offsets_access, topo_sizes_access,
                topo_subconn_access, topo_suboffsets_access, topo_subsizes_access,
                topo_num_elems,
                axis_data_access, csys_axes_size,
                [&](index_t ei, const float64 centroid[3])
            {
                for(index_t ai = 0; ai < csys_axes_size; ai++)
                    typed_dest_centroid[ai][ei] = static_cast<float32>(centroid[ai]);
            });
        }
    }
    else // polygonal, other
    {
        if(float_dtype.is_float64())
        {
            // Get pointers to where we'll write the centroid data directly.
            float64 *typed_dest_centroid[3] = {nullptr, nullptr, nullptr};
            for(index_t ai = 0; ai < csys_axes_size; ai++)
                typed_dest_centroid[ai] = reinterpret_cast<float64 *>(dest_centroid[ai]->element_ptr(0));
            
            unstructured_centroid(topo_shape,
                topo_conn_access, topo_offsets_access, topo_sizes_access, topo_num_elems,
                axis_data_access, csys_axes_size,
                [&](index_t ei, const float64 centroid[3])
            {
                for(index_t ai = 0; ai < csys_axes_size; ai++)
                    typed_dest_centroid[ai][ei] = centroid[ai];
            });
        }
        else //if(float_dtype.is_float32())
        {
            // Get pointers to where we'll write the centroid data directly.
            float32 *typed_dest_centroid[3] = {nullptr, nullptr, nullptr};
            for(index_t ai = 0; ai < csys_axes_size; ai++)
                typed_dest_centroid[ai] = reinterpret_cast<float32 *>(dest_centroid[ai]->element_ptr(0));

            unstructured_centroid(topo_shape,
                topo_conn_access, topo_offsets_access, topo_sizes_access, topo_num_elems,
                axis_data_access, csys_axes_size,
                [&](index_t ei, const float64 centroid[3])
            {
                for(index_t ai = 0; ai < csys_axes_size; ai++)
                    typed_dest_centroid[ai][ei] = static_cast<float32>(centroid[ai]);
            });
        }
    }
}

//-----------------------------------------------------------------------------
// - end internal topology helpers -
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- end internal helper functions --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{


//-----------------------------------------------------------------------------
// -- begin conduit::blueprint --
//-----------------------------------------------------------------------------
namespace blueprint
{

//-----------------------------------------------------------------------------
bool
mesh::verify(const std::string &protocol,
             const Node &n,
             Node &info)
{
    bool res = false;
    info.reset();

    if(protocol == "coordset")
    {
        res = coordset::verify(n,info);
    }
    else if(protocol == "topology")
    {
        res = topology::verify(n,info);
    }
    else if(protocol == "matset")
    {
        res = matset::verify(n,info);
    }
    else if(protocol == "specset")
    {
        res = specset::verify(n,info);
    }
    else if(protocol == "field")
    {
        res = field::verify(n,info);
    }
    else if(protocol == "adjset")
    {
        res = adjset::verify(n,info);
    }
    else if(protocol == "nestset")
    {
        res = nestset::verify(n,info);
    }
    else if(protocol == "index")
    {
        res = index::verify(n,info);
    }
    else if(protocol == "coordset/index")
    {
        res = coordset::index::verify(n,info);
    }
    else if(protocol == "topology/index")
    {
        res = topology::index::verify(n,info);
    }
    else if(protocol == "matset/index")
    {
        res = matset::index::verify(n,info);
    }
    else if(protocol == "specset/index")
    {
        res = specset::index::verify(n,info);
    }
    else if(protocol == "field/index")
    {
        res = field::index::verify(n,info);
    }
    else if(protocol == "adjset/index")
    {
        res = adjset::index::verify(n,info);
    }
    else if(protocol == "nestset/index")
    {
        res = nestset::index::verify(n,info);
    }

    return res;
}


//-----------------------------------------------------------------------------
bool
mesh::verify(const Node &mesh,
             Node &info)
{
    bool res = true;
    info.reset();

    // if n has the child "coordsets", we assume it is a single domain
    // mesh
    if(mesh.has_child("coordsets"))
    {
        res = verify_single_domain(mesh, info);
    }
    else
    {
       res = verify_multi_domain(mesh, info);
    }
    return res;
}


//-------------------------------------------------------------------------
bool mesh::is_multi_domain(const conduit::Node &mesh)
{
    // this is a blueprint property, we can assume it will be called
    // only when mesh verify is true. Given that - the only check
    // we need to make is the minimal check to distinguish between
    // a single domain and a multi domain tree structure.
    // checking for a child named "coordsets" mirrors the
    // top level verify check

    return !mesh.has_child("coordsets");
}


//-------------------------------------------------------------------------
index_t
mesh::number_of_domains(const conduit::Node &mesh)
{
    // this is a blueprint property, we can assume it will be called
    // only when mesh verify is true. Given that - it is easy to
    // answer the number of domains

    if(!is_multi_domain(mesh))
    {
        return 1;
    }
    else
    {
        return mesh.number_of_children();
    }
}

//-------------------------------------------------------------------------
void
mesh::state(const conduit::Node &mesh, conduit::Node &state)
{
    state.reset();
    if (! is_multi_domain(mesh))
    {
        if (mesh.has_child("state"))
        {
            state.set(mesh["state"]);
        }
    }
    else
    {
        auto doms_itr = mesh.children();
        while (doms_itr.has_next())
        {
            const Node &dom = doms_itr.next();
            if (dom.has_child("state"))
            {
                state.set(dom["state"]);
                break;
            }
        }
    }
}

//-------------------------------------------------------------------------
index_t
mesh::cycle(const conduit::Node &mesh)
{
    Node state;
    ::conduit::blueprint::mesh::state(mesh, state);
    if (state.has_child("cycle"))
    {
        return state["cycle"].to_index_t();
    }
    return std::numeric_limits<int>::max();
}

//-------------------------------------------------------------------------
float64
mesh::time(const conduit::Node &mesh)
{
    Node state;
    ::conduit::blueprint::mesh::state(mesh, state);
    if (state.has_child("time"))
    {
        return state["time"].to_float64();
    }
    return std::numeric_limits<int>::max();
}

//-------------------------------------------------------------------------
std::vector<conduit::Node *>
mesh::domains(conduit::Node &n)
{
    // this is a blueprint property, we can assume it will be called
    // only when mesh verify is true. Given that - it is easy to
    // aggregate all of the domains into a list

    std::vector<conduit::Node *> doms;

    if(!mesh::is_multi_domain(n))
    {
        doms.push_back(&n);
    }
    else if(!n.dtype().is_empty())
    {
        NodeIterator nitr = n.children();
        while(nitr.has_next())
        {
            doms.push_back(&nitr.next());
        }
    }

    return doms;
}


//-------------------------------------------------------------------------
std::vector<const conduit::Node *>
mesh::domains(const conduit::Node &mesh)
{
    // this is a blueprint property, we can assume it will be called
    // only when mesh verify is true. Given that - it is easy to
    // aggregate all of the domains into a list

    std::vector<const conduit::Node *> doms;

    if(!mesh::is_multi_domain(mesh))
    {
        doms.push_back(&mesh);
    }
    else if(!mesh.dtype().is_empty())
    {
        NodeConstIterator nitr = mesh.children();
        while(nitr.has_next())
        {
            doms.push_back(&nitr.next());
        }
    }

    return doms;
}

//-------------------------------------------------------------------------
void
mesh::domains(conduit::Node &mesh,
              std::vector<conduit::Node *> &res)
{
    // this is a blueprint property, we can assume it will be called
    // only when mesh verify is true. Given that - it is easy to
    // aggregate all of the domains into a list

    res.clear();

    if(!mesh::is_multi_domain(mesh))
    {
        res.push_back(&mesh);
    }
    else if(!mesh.dtype().is_empty())
    {
        NodeIterator nitr = mesh.children();
        while(nitr.has_next())
        {
            res.push_back(&nitr.next());
        }
    }
}

//-------------------------------------------------------------------------
void
mesh::domains(const conduit::Node &mesh,
              std::vector<const conduit::Node *> &res)
{
    // this is a blueprint property, we can assume it will be called
    // only when mesh verify is true. Given that - it is easy to
    // aggregate all of the domains into a list

    res.clear();

    if(!mesh::is_multi_domain(mesh))
    {
        res.push_back(&mesh);
    }
    else if(!mesh.dtype().is_empty())
    {
        NodeConstIterator nitr = mesh.children();
        while(nitr.has_next())
        {
            res.push_back(&nitr.next());
        }
    }
}



//-------------------------------------------------------------------------
void mesh::to_multi_domain(const conduit::Node &mesh,
                           conduit::Node &dest)
{
    dest.reset();

    if(mesh::is_multi_domain(mesh))
    {
        dest.set_external(mesh);
    }
    else
    {
        conduit::Node &dest_dom = dest.append();
        dest_dom.set_external(mesh);
    }
}

//-------------------------------------------------------------------------
void
mesh::generate_index(const conduit::Node &mesh,
                     const std::string &ref_path,
                     index_t number_of_domains,
                     Node &index_out)
{
    // domains can have different fields, etc
    // so we need the union of the index entries
    index_out.reset();

    if(mesh.dtype().is_empty())
    {
        CONDUIT_ERROR("Cannot generate mesh blueprint index for empty mesh.");
    }
    else if(blueprint::mesh::is_multi_domain(mesh))
    {
        NodeConstIterator itr = mesh.children();

        while(itr.has_next())
        {
            Node curr_idx;
            const Node &cld = itr.next();
            generate_index_for_single_domain(cld,
                                             ref_path,
                                             curr_idx);
            // add any new entries to the running index
            index_out.update(curr_idx);
        }
    }
    else
    {
        generate_index_for_single_domain(mesh,
                                         ref_path,
                                         index_out);
    }

    index_out["state/number_of_domains"] = number_of_domains;
}


//-----------------------------------------------------------------------------
void
mesh::generate_index_for_single_domain(const Node &mesh,
                                       const std::string &ref_path,
                                       Node &index_out)
{
    index_out.reset();
    if(!mesh.has_child("coordsets"))
    {
        CONDUIT_ERROR("Cannot generate mesh blueprint index for empty mesh."
                      " (input mesh missing 'coordsets')");
    }

    if(mesh.has_child("state"))
    {
        // check if the input mesh has state/cycle state/time
        // if so, add those to the index
        if(mesh.has_path("state/cycle"))
        {
            index_out["state/cycle"].set(mesh["state/cycle"]);
        }

        if(mesh.has_path("state/time"))
        {
            index_out["state/time"].set(mesh["state/time"]);
        }
        // state may contain other important stuff, like
        // the domain_id, so we need a way to read it
        // from the index
        index_out["state/path"] = join_path(ref_path, "state");
    }

    // an empty node is a valid blueprint mesh
    // so we nede to check for coordsets, can't assume they exist

    if(mesh.has_child("coordsets"))
    {
        NodeConstIterator itr = mesh["coordsets"].children();
        while(itr.has_next())
        {
            const Node &coordset = itr.next();
            std::string coordset_name = itr.name();
            Node &idx_coordset = index_out["coordsets"][coordset_name];

            std::string coordset_type =   coordset["type"].as_string();
            idx_coordset["type"] = coordset_type;
            if(coordset_type == "uniform")
            {
                // default to cartesian, but check if origin or spacing exist
                // b/c they may name axes from cyln or sph
                if(coordset.has_child("origin"))
                {
                    NodeConstIterator origin_itr = coordset["origin"].children();
                    while(origin_itr.has_next())
                    {
                        origin_itr.next();
                        idx_coordset["coord_system/axes"][origin_itr.name()];
                    }
                }
                else if(coordset.has_child("spacing"))
                {
                    NodeConstIterator spacing_itr = coordset["spacing"].children();
                    while(spacing_itr.has_next())
                    {
                        spacing_itr.next();
                        std::string axis_name = spacing_itr.name();

                        // if spacing names start with "d", use substr
                        // to determine axis name

                        // otherwise use spacing name directly, to avoid empty
                        // path fetch if just 'x', etc are passed
                        if(axis_name[0] == 'd' && axis_name.size() > 1)
                        {
                            axis_name = axis_name.substr(1);
                        }
                        idx_coordset["coord_system/axes"][axis_name];
                    }
                }
                else
                {
                    // assume cartesian
                    index_t num_comps = coordset["dims"].number_of_children();

                    if(num_comps > 0)
                    {
                        idx_coordset["coord_system/axes/x"];
                    }

                    if(num_comps > 1)
                    {
                        idx_coordset["coord_system/axes/y"];
                    }

                    if(num_comps > 2)
                    {
                        idx_coordset["coord_system/axes/z"];
                    }
                }
            }
            else
            {
                // use child names as axes
                NodeConstIterator values_itr = coordset["values"].children();
                while(values_itr.has_next())
                {
                    values_itr.next();
                    idx_coordset["coord_system/axes"][values_itr.name()];
                }
            }

            std::string coord_sys_type = bputils::coordset::coordsys(coordset);
            // logical is not supported in the blueprint index.
            if(coord_sys_type == "logical")
            {
                coord_sys_type = "cartesian";
            }
            idx_coordset["coord_system/type"] = coord_sys_type;

            //idx_coordset["coord_system/type"] = bputils::coordset::coordsys(coordset);

            std::string cs_ref_path = join_path(ref_path, "coordsets");
            cs_ref_path = join_path(cs_ref_path, coordset_name);
            idx_coordset["path"] = cs_ref_path;
        }
    }

    // an empty node is a valid blueprint mesh
    // so we nede to check for topologies, can't assume they exist
    if(mesh.has_child("topologies"))
    {
        NodeConstIterator itr = mesh["topologies"].children();
        while(itr.has_next())
        {
            const Node &topo = itr.next();
            std::string topo_name = itr.name();
            Node &idx_topo = index_out["topologies"][topo_name];
            idx_topo["type"] = topo["type"].as_string();
            idx_topo["coordset"] = topo["coordset"].as_string();

            std::string tp_ref_path = join_path(ref_path,"topologies");
            tp_ref_path = join_path(tp_ref_path,topo_name);
            idx_topo["path"] = tp_ref_path;

            // a topology may also specify a grid_function
            if(topo.has_child("grid_function"))
            {
                idx_topo["grid_function"] = topo["grid_function"].as_string();
            }
        }
    }

    if(mesh.has_child("matsets"))
    {
        NodeConstIterator itr = mesh["matsets"].children();
        while(itr.has_next())
        {
            const Node &matset = itr.next();
            const std::string matset_name = itr.name();
            Node &idx_matset = index_out["matsets"][matset_name];

            idx_matset["topology"] = matset["topology"].as_string();

            // support different flavors of valid matset protos
            //
            // if we have material_map (node with names to ids)
            // use it in the index
            if(matset.has_child("material_map"))
            {
                idx_matset["material_map"] = matset["material_map"];
            }
            else if(matset.has_child("materials"))
            {
                // NOTE: I believe path is deprecated ...
                NodeConstIterator mats_itr = matset["materials"].children();
                while(mats_itr.has_next())
                {
                    mats_itr.next();
                    idx_matset["materials"][mats_itr.name()];
                }
            }
            else if(matset.has_child("volume_fractions"))
            {
                // we don't have material_map (node with names to ids)
                // so mapping is implied from node order, construct
                // an actual map that follows the implicit order
                NodeConstIterator mats_itr = matset["volume_fractions"].children();
                while(mats_itr.has_next())
                {
                    mats_itr.next();
                    idx_matset["material_map"][mats_itr.name()] = mats_itr.index();
                }
            }
            else // surprise!
            {
                CONDUIT_ERROR("blueprint::mesh::generate_index: "
                              "Invalid matset flavor."
                              "Input node does not conform to mesh blueprint.");
            }

            std::string ms_ref_path = join_path(ref_path, "matsets");
            ms_ref_path = join_path(ms_ref_path, matset_name);
            idx_matset["path"] = ms_ref_path;
        }
    }

    if(mesh.has_child("specsets"))
    {
        NodeConstIterator itr = mesh["specsets"].children();
        while(itr.has_next())
        {
            const Node &specset = itr.next();
            const std::string specset_name = itr.name();
            Node &idx_specset = index_out["specsets"][specset_name];

            idx_specset["matset"] = specset["matset"].as_string();

            if (specset.has_child("species_names"))
            {
                idx_specset["species"] = specset["species_names"];
            }
            else if (specset.has_child("matset_values"))
            {
                NodeConstIterator matset_vals_itr = specset["matset_values"].children();
                while(matset_vals_itr.has_next())
                {
                    const Node &matset_val = matset_vals_itr.next();
                    const std::string matname = matset_vals_itr.name();

                    NodeConstIterator specs_itr = matset_val.children();
                    while(specs_itr.has_next())
                    {
                        specs_itr.next();
                        const std::string specname = specs_itr.name();
                        idx_specset["species"][matname][specname];
                    }
                }
            }
            else // surprise!
            {
                CONDUIT_ERROR("blueprint::mesh::generate_index: "
                              "Invalid specset flavor."
                              "Input node does not conform to mesh blueprint.");
            }

            std::string ms_ref_path = join_path(ref_path, "specsets");
            ms_ref_path = join_path(ms_ref_path, specset_name);
            idx_specset["path"] = ms_ref_path;
        }
    }

    if(mesh.has_child("fields"))
    {
        NodeConstIterator itr = mesh["fields"].children();
        while(itr.has_next())
        {
            const Node &fld = itr.next();
            std::string fld_name = itr.name();
            Node &idx_fld = index_out["fields"][fld_name];

            index_t ncomps = 1;
            if(fld.has_child("values"))
            {
                if(fld["values"].dtype().is_object())
                {
                    ncomps = fld["values"].number_of_children();
                }
            }
            else
            {
                if(fld["matset_values"].child(0).dtype().is_object())
                {
                    ncomps = fld["matset_values"].child(0).number_of_children();
                }
            }
            idx_fld["number_of_components"] = ncomps;

            if(fld.has_child("topology"))
            {
                idx_fld["topology"] = fld["topology"].as_string();
            }
            if(fld.has_child("matset"))
            {
                idx_fld["matset"] = fld["matset"].as_string();
            }
            if(fld.has_child("volume_dependent"))
            {
                idx_fld["volume_dependent"] = fld["volume_dependent"].as_string();
            }

            if(fld.has_child("association"))
            {
                idx_fld["association"] = fld["association"];
            }
            else
            {
                idx_fld["basis"] = fld["basis"];
            }

            std::string fld_ref_path = join_path(ref_path,"fields");
            fld_ref_path = join_path(fld_ref_path, fld_name);
            idx_fld["path"] = fld_ref_path;
        }
    }

    if(mesh.has_child("adjsets"))
    {
        NodeConstIterator itr = mesh["adjsets"].children();
        while(itr.has_next())
        {
            const Node &adjset = itr.next();
            const std::string adj_name = itr.name();
            Node &idx_adjset = index_out["adjsets"][adj_name];

            // TODO(JRC): Determine whether or not any information from the
            // "neighbors" and "values" sections need to be included in the index.
            idx_adjset["association"] = adjset["association"].as_string();
            idx_adjset["topology"] = adjset["topology"].as_string();

            std::string adj_ref_path = join_path(ref_path,"adjsets");
            adj_ref_path = join_path(adj_ref_path, adj_name);
            idx_adjset["path"] = adj_ref_path;
        }
    }

    if(mesh.has_child("nestsets"))
    {
        NodeConstIterator itr = mesh["nestsets"].children();
        while(itr.has_next())
        {
            const Node &nestset = itr.next();
            const std::string nest_name = itr.name();
            Node &idx_nestset = index_out["nestsets"][nest_name];

            // TODO(JRC): Determine whether or not any information from the
            // "domain_id" or "ratio" sections need to be included in the index.
            idx_nestset["association"] = nestset["association"].as_string();
            idx_nestset["topology"] = nestset["topology"].as_string();

            std::string adj_ref_path = join_path(ref_path,"nestsets");
            adj_ref_path = join_path(adj_ref_path, nest_name);
            idx_nestset["path"] = adj_ref_path;
        }
    }
}



//-------------------------------------------------------------------------
// blueprint tests and converters for one-dimensional meshes
//-------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::can_generate_strip(const Node &mesh,
                         const std::string & topo_name,
                         Node &info)
{
    const std::string protocol = "can_generate_strip";
    bool res = true;
    info.reset();

    // The topology must be 1D and cannot be points
    const Node& topo = mesh["topologies"][topo_name];
    const Node& coordset = mesh["coordsets"][topo["coordset"].as_string()];
    index_t cs_dim = mesh::utils::coordset::dims(coordset);
    std::string topo_type = topo["type"].as_string();
    if (!(cs_dim == 1 && topo_type != "points"))
    {
        log::error(info, protocol, "coordset dimension != 1, or topology type is points");
        res = false;
    }

    // Only element-associated fields allowed (no vertex-fields)
    // TODO Relax this requirement?
    NodeConstIterator fitr = mesh["fields"].children();
    while(fitr.has_next())
    {
        const Node &f = fitr.next();
        // This method requires fields to have "association" == "element".
        if (!f.has_child("association") || f["association"].as_string() != "element")
        {
            log::error(info,
                       protocol,
                       "fields[" + log::quote(fitr.name()) + "/association] != element");
            res = false;
        }
    }

    return res;
}

//-------------------------------------------------------------------------
void
mesh::generate_strip(conduit::Node &mesh,
                     std::string src_topo_name,
                     std::string dst_topo_name)
{

    const Node& src_topo = mesh["topologies"][src_topo_name];
    const Node& src_coordset = mesh["coordsets"][src_topo["coordset"].as_string()];
    Node dst_topo, dst_coordset;
    std::map<std::string, std::string> matset_names;

    mesh::coordset::generate_strip(src_coordset, dst_coordset);
    mesh::topology::generate_strip(src_topo, dst_topo_name, dst_topo);
    mesh::field::generate_strip(mesh["fields"], src_topo_name, dst_topo_name, matset_names);
    if (matset_names.size() > 0)
    {
        Node& matsets = mesh["matsets"];

        for (std::pair<std::string, std::string> msnames : matset_names)
        {
           matsets[msnames.second] = matsets[msnames.first];
           matsets[msnames.second]["topology"] = dst_topo_name;
        }
    }

    mesh["topologies"][dst_topo_name] = dst_topo;
    mesh["coordsets"][dst_topo_name] = dst_coordset;
}


void 
mesh::generate_strip(const conduit::Node& topo,
                     conduit::Node& topo_dest,
                     conduit::Node& coords_dest,
                     conduit::Node& fields_dest,
                     const conduit::Node& options)
{
    const std::string topo_name = topo.name();
    const std::string topo_dest_name = topo_dest.name();
    std::string field_prefix = "", matset_prefix = "";
    std::vector<std::string> field_names;
    std::map<std::string, std::string> matset_names;
    const Node& fields_src = (*(topo.parent()->parent()))["fields"];
    const Node& coordset_src = (*(topo.parent()->parent()))["coordsets/" + topo["coordset"].as_string()];

    // check for existence of field prefix
    if (options.has_child("field_prefix"))
    {
        if (options["field_prefix"].dtype().is_string())
        {
            field_prefix = options["field_prefix"].as_string();
            matset_prefix = field_prefix;
        }
        else
        {
            CONDUIT_ERROR("field_prefix must be a string.");
        }
    }

    // check for target field names
    if (options.has_child("field_names"))
    {
        if (options["field_names"].dtype().is_string())
        {
            field_names.push_back(options["field_names"].as_string());
        }
        else if (options["field_names"].dtype().is_list())
        {
            NodeConstIterator itr = options["field_names"].children();
            while (itr.has_next())
            {
                const Node& cld = itr.next();
                if (cld.dtype().is_string())
                {
                    field_names.push_back(cld.as_string());
                }
                else
                {
                    CONDUIT_ERROR("field_names must be a string or a list of strings.");
                }
            }
        }
        else
        {
            CONDUIT_ERROR("field_names must be a string or a list of strings.");
        }
    }
    else
    {
        // fill field_names with all current fields' names
        NodeConstIterator itr = fields_src.children();
        while (itr.has_next())
        {
            const Node& cld = itr.next();
            if (cld["topology"].as_string() == topo_name)
            {
                field_names.push_back(itr.name());
            }
        }
    }

    // check that the discovered field names exist in the target fields
    for (uint64 i = 0; i < field_names.size(); i++)
    {
        if (!fields_src.has_child(field_names[i]))
        {
            CONDUIT_ERROR("field " + field_names[i] + " not found in target.");
        }
    }

    // check for existence of matset prefix
    if (options.has_child("matset_prefix"))
    {
        if (options["matset_prefix"].dtype().is_string())
        {
            matset_prefix = options["matset_prefix"].as_string();
        }
        else
        {
            CONDUIT_ERROR("matset_prefix must be a string.");
        }
    }

    // generate new topology
    mesh::coordset::generate_strip(coordset_src, coords_dest);
    mesh::topology::generate_strip(topo, coords_dest.name(), topo_dest);

    // generate new fields
    for (const std::string& field_name : field_names)
    {
        // TODO Something useful with grid functions, when needed by users.
        if (fields_src[field_name]["association"].as_string() == "element")
        {
            const std::string field_dest_name = field_prefix + field_name;

            if (fields_dest.has_child(field_dest_name))
            {
               CONDUIT_ERROR("Attempted to generate a field with an existing name " + field_dest_name);
            }

            fields_dest[field_dest_name] = fields_src[field_name];
            fields_dest[field_dest_name]["topology"] = topo_dest_name;

            // If a field refers to a matset, fix up the reference and convert the matset later.
            if (fields_dest[field_dest_name].has_child("matset"))
            {
                const std::string matset_name = fields_dest[field_dest_name]["matset"].as_string();
                const std::string matset_dest_name = matset_prefix + matset_name;
                matset_names[matset_name] = matset_dest_name;
                fields_dest[field_dest_name]["matset"] = matset_dest_name;
            }
        }
        else
        {
            // For now, do nothing.
            // TODO Something useful with vertex fields.  Confer with users.
        }
    }

    // for each referenced matset, generate a matset
    if (fields_dest.parent()->has_child("matsets"))
    {
        const Node& matsets_src = (*(topo.parent()->parent()))["matsets"];
        Node& matsets_dest = (*(fields_dest.parent()))["matsets"];

        for (std::pair<std::string, std::string> matset_name : matset_names)
        {
            if (matsets_dest.has_child(matset_name.second))
            {
               CONDUIT_ERROR("Attempted to generate a matset with an existing name " + matset_name.second);
            }

            matsets_dest[matset_name.second] = matsets_src[matset_name.first];
            matsets_dest[matset_name.second]["topology"] = topo_dest_name;
        }
    }
}


//
// This function is responsible for collecting the domains within the given mesh
// with subnodes of the given maps based on the domain's path, e.g.:
//
// input:
//   mesh: {"domain0": {/*1*/}, "domain1": {/*2*/}}
//   s2dmap: {}
//   d2smap: {}
//
// output:
//   mesh: {"domain0": {/*1*/}, "domain1": {/*2*/}}
//   s2dmap: {"domain0": {/*A*/}, "domain1": {/*B*/}}
//   d2smap: {"domain0": {/*a*/}, "domain1": {/*b*/}}
//   return: [<&1, &A, &a>, <&2, &B, &b>]
//
std::vector<DomMapsTuple>
group_domains_and_maps(conduit::Node &mesh, conduit::Node &s2dmap, conduit::Node &d2smap)
{
    std::vector<DomMapsTuple> doms_and_maps;

    s2dmap.reset();
    d2smap.reset();

    if(!conduit::blueprint::mesh::is_multi_domain(mesh))
    {
        doms_and_maps.emplace_back(&mesh, &s2dmap, &d2smap);
    }
    else
    {
        NodeIterator domains_it = mesh.children();
        while(domains_it.has_next())
        {
            conduit::Node& domain = domains_it.next();
            if(mesh.dtype().is_object())
            {
                doms_and_maps.emplace_back(&domain,
                                           &s2dmap[domain.name()],
                                           &d2smap[domain.name()]);
            }
            else
            {
                doms_and_maps.emplace_back(&domain,
                                           &s2dmap.append(),
                                           &d2smap.append());
            }
        }
    }

    return doms_and_maps;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// -- Top level generate methods that support multi domain meshes -- 
// These were moved from conduit::blueprint::mpi, b/c the do not depend on mpi
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/**
 * \brief Average a set of coordinates in a coordset.
 *
 * \param cset The input coordset, which needs to be explicit.
 * \param coordIds A container with the coord ids that will be averaged.
 *
 * \return A vector containing the averaged coordinate.
 */
template <typename CoordIdContainer>
std::vector<float64> average_coords(const Node &cset, const CoordIdContainer &coordIds)
{
    std::vector<float64> avg_coords;
    const float64 w = coordIds.empty() ? 1. : (1. / static_cast<float64>(coordIds.size()));
    for(const index_t &id : coordIds)
    {
        // Get the points for entity_pidx.
        const std::vector<float64> point_coords = bputils::coordset::_explicit::coords(cset, id);

        if(avg_coords.empty())
        {
            avg_coords.resize(point_coords.size(), 0.);
        }

        for(size_t comp = 0; comp < point_coords.size(); comp++)
        {
            avg_coords[comp] += w * point_coords[comp];
        }
    }
    return avg_coords;
}

//-----------------------------------------------------------------------------
template <typename ContainerType>
inline void printContainer(std::ostream &os, const ContainerType &container)
{
    os << "{";
    int i = 0;
    for(auto it = container.begin(); it != container.end(); i++, it++)
    {
        if(i > 0)
           os << ", ";
        os << *it;
    }
    os << "}";
}

//-----------------------------------------------------------------------------
/**
 @brief Used to generate derived entities from the source mesh. The main work
        is done by the \a generate_derived function. The bulk of this function
        is dedicated to constructing a new adjset from the source mesh's adjset.

        In practice, this mostly does smoothly. However, there can be corner
        cases, that would erroneously add elements into the derived adjset.
        This happens depending on the domain decomposition. To avoid introducing
        unwanted entities, a MatchQuery is used. The MatchQuery keeps track of
        all of the local entities that were requested and then checks to make
        sure matching entities were also requested in the adjacent domain. Note
        that this step involves coordinate comparisons (because they can have
        epsilon deltas across domain boundaries) and exchanging entity query
        topologies.

        
        The query is passed in so parallel functions can pass in a parallel
        version of the MatchQuery.

  @param mesh The input mesh, which may contain multiple domains.
  @param src_adjset_name The source adjset name that will be used to guide
                         construction of the new adjset.
  @param dst_adjset_name The name of the new adjset.
  @param dst_topo_name   The name of the new topology to contain the derived
                         entities.
  @param s2dmap          A source to dest map for source elements to derived elements.
  @param d2smap          A dest to source map for derived elements to source elements.
  @param generate_derived A function that will be applied to the source domains
                          to generate derived domains.
  @param Q                A MatchQuery instance that helps in constructing the
                          derived adjset.
  &param mic              A MeshInfoCollection object use in quantizing nodes.
 */
static void
generate_derived_entities(conduit::Node &mesh,
                          const std::string &src_adjset_name,
                          const std::string &dst_adjset_name,
                          const std::string &dst_topo_name,
                          conduit::Node &s2dmap,
                          conduit::Node &d2smap,
                          GenDerivedFun generate_derived,
                          conduit::blueprint::mesh::utils::query::MatchQuery &Q,
                          conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    namespace topoutils = conduit::blueprint::mesh::utils::topology;
    using Entity = std::tuple<topoutils::Quantizer::QuantizedIndex, index_t>;
    using EntityVector = std::vector<Entity>;

    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // loop over all domains and call generate_derived on each domain
    CONDUIT_ANNOTATE_MARK_BEGIN("Setup");
    const std::vector<DomMapsTuple> doms_and_maps = group_domains_and_maps(mesh, s2dmap, d2smap);
    for(index_t di = 0; di < (index_t)doms_and_maps.size(); di++)
    {
        conduit::Node &domain = *std::get<0>(doms_and_maps[di]);
        conduit::Node &domain_s2dmap = *std::get<1>(doms_and_maps[di]);
        conduit::Node &domain_d2smap = *std::get<2>(doms_and_maps[di]);

        const conduit::Node &src_adjset = domain["adjsets"][src_adjset_name];
        const Node *src_topo_ptr = bputils::find_reference_node(src_adjset, "topology");
        const Node &src_topo = *src_topo_ptr;

        conduit::Node &dst_topo = domain["topologies"][dst_topo_name];
        generate_derived(src_topo, dst_topo, domain_s2dmap, domain_d2smap);

        conduit::Node &dst_adjset = domain["adjsets"][dst_adjset_name];
        dst_adjset.reset();
        dst_adjset["association"].set("element");
        dst_adjset["topology"].set(dst_topo_name);
    }
    CONDUIT_ANNOTATE_MARK_END("Setup");

    CONDUIT_ANNOTATE_MARK_BEGIN("dom_entity_neighbor_map.start");
    Node src_data, dst_data;
    std::vector<std::tuple<int,int,index_t,size_t>> query_guide;
    std::map<int,std::map<index_t, std::set<index_t>>> dom_entity_neighbor_map;
    for(index_t di = 0; di < (index_t)doms_and_maps.size(); di++)
    {
        conduit::Node &domain = *std::get<0>(doms_and_maps[di]);
        const index_t domain_id = domain["state/domain_id"].to_index_t();

        const Node *src_topo_ptr = bputils::find_reference_node(domain["adjsets"][src_adjset_name], "topology");
        const Node &src_topo = *src_topo_ptr;

        const Node &dst_topo = domain["topologies"][dst_topo_name];
        const index_t dst_topo_len = bputils::topology::length(dst_topo);

        const conduit::Node &src_adjset_groups = domain["adjsets"][src_adjset_name]["groups"];

        // Tell the query which topology to use.
        Q.selectTopology(src_topo.name());

        // Organize Adjset Points into Interfaces (Pair-Wise Groups) //
        //
        // for all neighbors:
        // construct a per-neighbor list of all unique points across all groups
        // result is a map that contains:
        //
        // {(neighbor domain id): <(participating points for domain interface)>}
        std::map<index_t, std::set<index_t>> neighbor_pidxs_map;
        for(const std::string &group_name : src_adjset_groups.child_names())
        {
            const conduit::Node &src_group = src_adjset_groups[group_name];
            index_t_accessor src_neighbors = src_group["neighbors"].value();
            index_t_accessor src_values    = src_group["values"].value();

            for(index_t ni = 0; ni < src_neighbors.dtype().number_of_elements(); ni++)
            {
                std::set<index_t> &neighbor_pidxs = neighbor_pidxs_map[src_neighbors[ni]];
                for(index_t pi = 0; pi < src_values.dtype().number_of_elements(); pi++)
                {
                    neighbor_pidxs.insert(src_values[pi]);
                }
            }
        }

        // Collect Viable Entities for All Interfaces //
        conduit::Node &domain_d2smap = *std::get<2>(doms_and_maps[di]);
        index_t_accessor d2s_sizes = domain_d2smap["sizes"].value();

        // {(entity id in topology): <(neighbor domain ids that contain this entity)>}
        for(index_t ei = 0; ei < dst_topo_len; ei++)
        {
            // if we are dealing with anything but points
            // we don't want to include duplicated entities 
            // (that means they are internal to the domain)
            std::vector<index_t> entity_pidxs = bputils::topology::unstructured::points(dst_topo, ei);
            if(d2s_sizes[ei] < 2 || entity_pidxs.size() == 1)
            {
                for(const auto &neighbor_pair : neighbor_pidxs_map)
                {
                    const index_t &ni = neighbor_pair.first;
                    const std::set<index_t> &neighbor_pidxs = neighbor_pair.second;

                    // check if the new element has all of its points
                    // contained inside of the adjset

                    bool entity_in_neighbor = true;
                    for(index_t pi = 0; pi < (index_t)entity_pidxs.size() && entity_in_neighbor; pi++)
                    {
                        entity_in_neighbor &= neighbor_pidxs.find(entity_pidxs[pi]) != neighbor_pidxs.end();
                    }

                    // if the element is fully in the adjset, and it is not an internal element
                    // (count of d2s > 1), include it in the map

                    if(entity_in_neighbor)
                    {
                        // Add the entity to the query for consideration.
                        const auto eid = Q.add(domain_id, ni, entity_pidxs);

                        // Add the candidate entity to the match query, which
                        // will help resolve things across domains.
                        query_guide.push_back(std::make_tuple(static_cast<int>(domain_id), static_cast<int>(ni), ei, eid));
                    }
                }
            }
        }
    }
    CONDUIT_ANNOTATE_MARK_END("dom_entity_neighbor_map.start");

    // Execute the queries.
    CONDUIT_ANNOTATE_MARK_BEGIN("query");
    Q.execute();
    CONDUIT_ANNOTATE_MARK_END("query");

    // Use query results to finish building entity_neighbor_map.
    CONDUIT_ANNOTATE_MARK_BEGIN("dom_entity_neighbor_map.finish");
    for(const auto &obj : query_guide)
    {
        const auto domain_id = std::get<0>(obj);
        const auto ni = std::get<1>(obj);
        const auto ei = std::get<2>(obj);
        const auto eid = std::get<3>(obj);
        if(Q.exists(domain_id, ni, eid))
        {
            auto &entity_neighbor_map = dom_entity_neighbor_map[domain_id];
            entity_neighbor_map[ei].insert(ni);
        }
#ifdef DEBUG_PRINT
        else
        {
            std::cout << "!!!! NOT adding domain " << domain_id << " entity " << ei << " to entity_neighbor_map for neighbor " << ni << std::endl;
        }
#endif
    }
    CONDUIT_ANNOTATE_MARK_END("dom_entity_neighbor_map.finish");

    CONDUIT_ANNOTATE_MARK_BEGIN("mesh_info");
    const index_t num_domains = static_cast<index_t>(doms_and_maps.size());
    mic.begin();
    for(index_t di = 0; di < num_domains; di++)
    {
        Node &domain = *std::get<0>(doms_and_maps[di]);
        const index_t domain_id = domain["state/domain_id"].to_index_t();
        const Node &dst_topo = domain["topologies"][dst_topo_name];

        // Compute information about the dst_topo, store in mic.
        topoutils::MeshInfo dst_info;
        topoutils::compute_mesh_info(dst_topo, dst_info);
        mic.add(domain_id, dst_info);
    }
    mic.end();
    topoutils::MeshInfo gMeshInfo(mic.getMergedMeshInfo());
    // Since we're averaging entity coordinates below for spatial sorting,
    // reduce the distances by half. This makes more bins in the quantizer
    // to reduce the likelihood that averaged points map the same bin.
    gMeshInfo.minEdgeLength /= 2.;
    gMeshInfo.minDiagonalLength /= 2.;
    CONDUIT_ANNOTATE_MARK_END("mesh_info");

    // Finish building the adjset.
    CONDUIT_ANNOTATE_MARK_BEGIN("build");
    for(index_t di = 0; di < num_domains; di++)
    {
        conduit::Node &domain = *std::get<0>(doms_and_maps[di]);
        const index_t domain_id = domain["state/domain_id"].to_index_t();

        const Node *src_topo_ptr = bputils::find_reference_node(domain["adjsets"][src_adjset_name], "topology");
        const Node &src_topo = *src_topo_ptr;
        const Node *src_cset_ptr = bputils::find_reference_node(src_topo, "coordset");
        const Node &src_cset = *src_cset_ptr;
        const conduit::Node &src_adjset_groups = domain["adjsets"][src_adjset_name]["groups"];

        const Node &dst_topo = domain["topologies"][dst_topo_name];

        // Make Quantizer from merged mesh info.
        topoutils::Quantizer quantizer(gMeshInfo);

        conduit::Node &dst_adjset_groups = domain["adjsets"][dst_adjset_name]["groups"];

        // Uncomment the following line to generate files for debugging.
//#define CONDUIT_DEBUG_DERIVED_ENTITIES
#ifdef CONDUIT_DEBUG_DERIVED_ENTITIES
        std::stringstream ss;
        ss << dst_adjset_name << "." << domain_id << ".txt";
        std::string filename(ss.str());
        std::ofstream ofs;
        ofs.open(filename.c_str());
        ofs << mic << std::endl;
        ofs << "Quantizer: " << quantizer << std::endl;
#endif
        // Use Entity Interfaces to Construct Group Entity Lists //
        std::map<std::set<index_t>, EntityVector> group_entity_map;
        const auto &entity_neighbor_map = dom_entity_neighbor_map[domain_id];
        for(const auto &entity_neighbor_pair : entity_neighbor_map)
        {
            const index_t &ei = entity_neighbor_pair.first;
            const std::set<index_t> &entity_neighbors = entity_neighbor_pair.second;

            // Get point ids used in entity ei in dst_topo
            const std::vector<index_t> entity_pidxs = bputils::topology::unstructured::points(dst_topo, ei);

            // Average the entity points to form a point for sorting.
            const auto avg_coords = average_coords(src_cset, entity_pidxs);
            const auto q = quantizer.quantize(avg_coords);

            // Make entity
            Entity entity;
            std::get<0>(entity) = q;
            std::get<1>(entity) = ei;

            auto &group_entities = group_entity_map[entity_neighbors];

#ifdef CONDUIT_DEBUG_DERIVED_ENTITIES
            // Print entity information to the log.
            ofs << "Entity " << ei << ", neighbors=";
            printContainer(ofs, entity_neighbors);
            ofs << ", pidxs=";
            printContainer(ofs, entity_pidxs);
            ofs << ", avg_coords=";
            printContainer(ofs, avg_coords);
            ofs << ", q=" << q << "\n";

            // See whether the entity already exists in the group_entities. If
            // so, this is BAD as it means that more than one entity had the
            // same quantized index.
            for(size_t i = 0; i < group_entities.size(); i++)
            {
              if(std::get<0>(group_entities[i]) == q)
              {
                ofs << "ERROR: entity " << std::get<1>(group_entities[i])
                    << " with q=" << q << " already found at index " << i
                    << std::endl;
                break;
              }
            }
#endif
            // NOTE(JRC): Inserting with this method allows this algorithm to sort new
            // elements as they're generated, rather than as a separate process at the
            // end (slight optimization overall).
            auto entity_itr = std::upper_bound(group_entities.begin(), group_entities.end(), entity);
            group_entities.insert(entity_itr, entity);
        }

        for(const auto &group_pair : group_entity_map)
        {
            const std::set<index_t> &group_nidxs = group_pair.first;
            const EntityVector &group_entities = group_pair.second;

            // NOTE(JRC): It's possible for the 'src_adjset_groups' node to be empty,
            // so we only want to query child data types if we know there is at least
            // 1 non-empty group.
            const conduit::DataType src_neighbors_dtype = src_adjset_groups.child(0)["neighbors"].dtype();
            const conduit::DataType src_values_dtype = src_adjset_groups.child(0)["values"].dtype();

            std::string group_name;
            {
                // NOTE(JRC): The current domain is included in the domain name so that
                // it matches across all domains and processors (also, using std::set
                // ensures that items are sorted and the order is the same across ranks).
                std::set<index_t> group_all_nidxs = group_nidxs;
                group_all_nidxs.insert(domain_id);

                std::ostringstream oss;
                oss << "group";
                for(const index_t &group_nidx : group_all_nidxs)
                {
                    oss << "_" << group_nidx;
                }
                group_name = oss.str();
            }

            conduit::Node &dst_group = dst_adjset_groups[group_name];
            conduit::Node &dst_neighbors = dst_group["neighbors"];
            conduit::Node &dst_values = dst_group["values"];

            dst_neighbors.set(DataType(src_neighbors_dtype.id(), group_nidxs.size()));
            index_t ni = 0;
            auto neighbors = dst_neighbors.as_index_t_accessor();
            for(auto nitr = group_nidxs.begin(); nitr != group_nidxs.end(); ++nitr)
            {
                neighbors.set(ni++, *nitr);
            }
            dst_values.set(DataType(src_values_dtype.id(), group_entities.size()));
            auto values = dst_values.as_index_t_accessor();
            for(index_t ei = 0; ei < (index_t)group_entities.size(); ei++)
            {
                const index_t nodeid = std::get<1>(group_entities[ei]);
                values.set(ei, nodeid);
            }
        }
#ifdef CONDUIT_DEBUG_DERIVED_ENTITIES
        ofs.close();
#endif
    }
    CONDUIT_ANNOTATE_MARK_END("build");
}


//-----------------------------------------------------------------------------
std::vector<index_t>
calculate_decomposed_dims(const conduit::Node &mesh, const std::string &adjset_name, CalcDimDecomposedFun calc_dims)
{
    // NOTE(JRC): This strategy works even if some ranks have empty meshes because
    // the empty ranks won't use the resulting 'dims' array to compute indices and
    // thus it doesn't matter if the variable has the technically incorrect value.
    const std::vector<const Node *> domains = ::conduit::blueprint::mesh::domains(mesh);
    if(domains.empty())
    {
        return std::vector<index_t>();
    }
    else // if(!domains.empty())
    {
        const Node &domain = *domains.front();

        const Node &adjset = domain["adjsets"][adjset_name];
        const Node *topo_ptr = bputils::find_reference_node(adjset, "topology");
        const Node &topo = *topo_ptr;

        const bputils::ShapeType shape(topo);
        return calc_dims(shape);
    }
}

//-----------------------------------------------------------------------------
void
generate_decomposed_entities(conduit::Node &mesh,
                             const std::string &src_adjset_name,
                             const std::string &dst_adjset_name,
                             const std::string &dst_topo_name,
                             const std::string &dst_cset_name,
                             conduit::Node &s2dmap,
                             conduit::Node &d2smap,
                             GenDecomposedFun generate_decomposed,
                             IdDecomposedFun  identify_decomposed,
                             const std::vector<index_t> &decomposed_centroid_dims,
                             conduit::blueprint::mesh::utils::query::PointQueryBase &query,
                             conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    namespace topoutils = conduit::blueprint::mesh::utils::topology;
    using Entity = std::tuple<topoutils::Quantizer::QuantizedIndex, index_t>;
    using EntityVector = std::vector<Entity>;

    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // Iterate over the domains and produce a "decomposed" mesh for each domain.
    // The new mesh will live in the domain's topologies next to the topology for
    // the adjset identified by src_adjset_name.
    CONDUIT_ANNOTATE_MARK_BEGIN("Setup");
    const std::vector<DomMapsTuple> doms_and_maps = group_domains_and_maps(mesh, s2dmap, d2smap);
    for(index_t di = 0; di < (index_t)doms_and_maps.size(); di++)
    {
        Node &domain = *std::get<0>(doms_and_maps[di]);
        Node &domain_s2dmap = *std::get<1>(doms_and_maps[di]);
        Node &domain_d2smap = *std::get<2>(doms_and_maps[di]);

        const Node &src_adjset = domain["adjsets"][src_adjset_name];
        const Node *src_topo_ptr = bputils::find_reference_node(src_adjset, "topology");
        const Node &src_topo = *src_topo_ptr;

        // NOTE(JRC): Diff- new code below
        Node &dst_topo = domain["topologies"][dst_topo_name];
        Node &dst_cset = domain["coordsets"][dst_cset_name];
        generate_decomposed(src_topo, dst_topo, dst_cset, domain_s2dmap, domain_d2smap);

        Node &dst_adjset = domain["adjsets"][dst_adjset_name];
        dst_adjset.reset();
        // NOTE(JRC): Diff- different association (decomposed entity -> assoc: vertex)
        dst_adjset["association"].set("vertex");
        dst_adjset["topology"].set(dst_topo_name);
    }
    CONDUIT_ANNOTATE_MARK_END("Setup");

    // Iterate over each domain and build up dom_entity_neighbor_map as much as
    // we can. For some entities, we need to ask neighboring domains whether the
    // entity-produced corner coordset point is valid there. For those, we add
    // points to the input query and build up query_guide.
    CONDUIT_ANNOTATE_MARK_BEGIN("dom_entity_neighbor_map.start");
    Node src_data, dst_data;
    src_data.reset();
    dst_data.reset();
    index_t num_domains = static_cast<index_t>(doms_and_maps.size());
    std::map<index_t, std::map<index_t, std::set<index_t>>> dom_entity_neighbor_map;
    std::vector<std::tuple<index_t, index_t, index_t, index_t>> query_guide;
    for(index_t di = 0; di < num_domains; di++)
    {
        Node &domain = *std::get<0>(doms_and_maps[di]);
        const index_t domain_id = domain["state/domain_id"].to_index_t();

        const Node *src_topo_ptr = bputils::find_reference_node(domain["adjsets"][src_adjset_name], "topology");
        const Node &src_topo = *src_topo_ptr;
        const Node *src_cset_ptr = bputils::find_reference_node(src_topo, "coordset");
        const Node &src_cset = *src_cset_ptr;
        // NOTE(JRC): Diff- generate topology metadata for source topology to find
        // centroids that may exist within an adjset group.
        bputils::TopologyMetadata src_topo_data(src_topo, src_cset);

        // const Node &dst_topo = domain["topologies"][dst_topo_name];
        const Node &dst_cset = domain["coordsets"][dst_cset_name];

        const Node &src_adjset_groups = domain["adjsets"][src_adjset_name]["groups"];

        // Organize Adjset Points into Interfaces (Pair-Wise Groups) //
        //
        //   Iterate over all adjset groups and build a list of vertex ids for 
        //   each neighbor (neighbor_pidxs). Each list of vertex ids contains
        //   vertices in the main topology's coordset. The vertex ids will
        //   probably also all live on the external surfaces of the domain.
        //
        // {(neighbor domain id): <(participating points for domain interface)>}
        std::map<index_t, std::set<index_t>> neighbor_pidxs_map;
        for(const std::string &group_name : src_adjset_groups.child_names())
        {
            const Node &src_group = src_adjset_groups[group_name];

            index_t_accessor src_neighbors=src_group["neighbors"].value();
            index_t_accessor src_values=src_group["values"].value();

            for(index_t ni = 0; ni < src_neighbors.dtype().number_of_elements(); ni++)
            {
                std::set<index_t> &neighbor_pidxs = neighbor_pidxs_map[src_neighbors[ni]];
                for(index_t pi = 0; pi < src_values.dtype().number_of_elements(); pi++)
                {
                    neighbor_pidxs.insert(src_values[pi]);
                }
            }
        }

        // Collect Viable Entities for All Interfaces //

        // {(entity centroid id): <(neighbor domain ids that contain this entity)>}
        auto &entity_neighbor_map = dom_entity_neighbor_map[domain_id];
        // NOTE(JRC): Diff, entirely different iteration strategy for finding entities
        // to consider on individual adjset interfaces.

        // Iterate over the entity dimensions that can be produced by cascading
        // the main topology. From there, iterate over all of the entities in
        // each cascade dimension to get its points and see if that entity's
        // points are contained by neighbor_pidxs. The goal is to find out which
        // entities are connected to which neighbors.
        for(const index_t &dentry : decomposed_centroid_dims)
        {
            index_t dentry_length = src_topo_data.get_length(dentry);
            for(index_t ei = 0; ei < dentry_length; ei++)
            {
                // Get the point ids for the current entity ei.
                auto entity_pidxs = src_topo_data.get_global_association(ei, dentry, 0);
                for(const auto &neighbor_pair : neighbor_pidxs_map)
                {
                    const index_t &ni = neighbor_pair.first;
                    const std::set<index_t> &neighbor_pidxs = neighbor_pair.second;

                    // Check the points in entity_pidxs to see if they are ALL
                    // in the neighbor_pidxs set. If so then the points in this entity
                    // touch the neighbor.
                    bool need_to_add = true;
                    for(index_t pi = 0; pi < (index_t)entity_pidxs.size() && need_to_add; pi++)
                    {
                        need_to_add &= neighbor_pidxs.find(entity_pidxs[pi]) != neighbor_pidxs.end();
                    }

                    if(need_to_add)
                    {
                        // Compute a new index for the entity. This is later used
                        // as an index into the corner mesh's coordset.

                        // dentry 0: entity_cidx = ei
                        // dentry 1: entity_cidx = npts + ei
                        // dentry 2: entity_cidx = npts + nfaces + ei
                        const index_t entity_cidx = identify_decomposed(src_topo_data, ei, dentry);

                        // For edges and faces, add the entity to a point query
                        // that will be used to filter invalid points in the local
                        // domain's corner mesh that may not exist in the neighbor.
                        // For vertices, we can add those to entity_neighbor_map
                        // without checking.
                        if(dentry > 0)
                        {
                            // Get the point (it might not be 3D)
                            auto pt = bputils::coordset::_explicit::coords(dst_cset, entity_cidx);
                            double pt3[3];
                            pt3[0] = pt[0];
                            pt3[1] = (pt.size() > 1) ? pt[1] : 0.;
                            pt3[2] = (pt.size() > 2) ? pt[2] : 0.;

                            // Add the point to the query and store some information
                            // help finish up later.
                            auto idx = query.add(ni, pt3);
                            query_guide.emplace_back(domain_id, entity_cidx, ni, idx);
                        }
                        else
                        {
                            // dentry is 0 so it is a point that carries over from
                            // the source topo. We can just add it.
                            entity_neighbor_map[entity_cidx].insert(ni);
                        }
                    }
                }
            } // for ei
        } // for dentry
    }
    CONDUIT_ANNOTATE_MARK_END("dom_entity_neighbor_map.start");

    // Perform the queries for the points that we need to know about.
    CONDUIT_ANNOTATE_MARK_BEGIN("query");
    query.execute(dst_cset_name);
    CONDUIT_ANNOTATE_MARK_END("query");

    // Use the point query results to finish building entity_neighbor_map.
    CONDUIT_ANNOTATE_MARK_BEGIN("dom_entity_neighbor_map.finish");
    for(const auto &obj : query_guide)
    {
        index_t domain_id = std::get<0>(obj);
        index_t entity_cidx = std::get<1>(obj);
        index_t ni = std::get<2>(obj);
        index_t idx = std::get<3>(obj);

        auto &entity_neighbor_map = dom_entity_neighbor_map[domain_id];
        // Add the entity to entity_neighbor_map if its point existed on
        // the remote domain.
        const auto &results = query.results(ni);
        if(results[idx] > query.NotFound)
        {
            entity_neighbor_map[entity_cidx].insert(ni);
        }
    }
    CONDUIT_ANNOTATE_MARK_END("dom_entity_neighbor_map.finish");

    CONDUIT_ANNOTATE_MARK_BEGIN("mesh_info");
    mic.begin();
    for(index_t di = 0; di < num_domains; di++)
    {
        Node &domain = *std::get<0>(doms_and_maps[di]);
        const index_t domain_id = domain["state/domain_id"].to_index_t();
        const Node &dst_topo = domain["topologies"][dst_topo_name];

        // Compute information about the dst_topo, store in mic.
        topoutils::MeshInfo dst_info;
        topoutils::compute_mesh_info(dst_topo, dst_info);
        mic.add(domain_id, dst_info);
    }
    mic.end();
    CONDUIT_ANNOTATE_MARK_END("mesh_info");

    // Finish building the corner mesh adjset.
    CONDUIT_ANNOTATE_MARK_BEGIN("build");
    for(index_t di = 0; di < num_domains; di++)
    {
        Node &domain = *std::get<0>(doms_and_maps[di]);
        const index_t domain_id = domain["state/domain_id"].to_index_t();
        const auto &entity_neighbor_map = dom_entity_neighbor_map[domain_id];

        const Node &dst_cset = domain["coordsets"][dst_cset_name];

        const Node &src_adjset_groups = domain["adjsets"][src_adjset_name]["groups"];
        Node &dst_adjset_groups = domain["adjsets"][dst_adjset_name]["groups"];

        // Make Quantizer from merged mesh info.
        topoutils::Quantizer quantizer(mic.getMergedMeshInfo());

        // Use Entity Interfaces to Construct Group Entity Lists //

        // Iterate over the entity_neighbor_map and build up group_entity_map.
        // entity_neighbor_map is indexed by an entity id and its associated data
        // contains a set of neighbor domains for that entity. Those entity ids
        // are for the source topology's points, edges, faces in that order.
        // They are used to index into the corner topology's coordset, assuming
        // that the coordinates were added in that same order.
        //
        // This assumption must be made to avoid a costly coordinate->id lookup.

        std::map<std::set<index_t>, EntityVector> group_entity_map;
        for(const auto &entity_neighbor_pair : entity_neighbor_map)
        {
            const index_t &entity_cidx = entity_neighbor_pair.first;
            const std::set<index_t> &entity_neighbors = entity_neighbor_pair.second;

            // NOTE(JRC): Diff: Substitute entity for centroid point at the end here.

            // Use the entity_cidx (a source topo index) to get the coordinate
            // in the corner coordset.
            const std::vector<float64> point_coords = bputils::coordset::_explicit::coords(
                dst_cset, entity_cidx);

            // Make entity
            Entity entity;
            std::get<0>(entity) = quantizer.quantize(point_coords);
            std::get<1>(entity) = entity_cidx;

            // NOTE(JRC): Inserting with this method allows this algorithm to sort new
            // elements as they're generated, rather than as a separate process at the
            // end (slight optimization overall).
            auto &group_entities = group_entity_map[entity_neighbors];
            auto entity_itr = std::upper_bound(group_entities.begin(), group_entities.end(), entity);
            group_entities.insert(entity_itr, entity);
        }

        // Build the output adjsets from data in group_entity_map
        for(const auto &group_pair : group_entity_map)
        {
            // NOTE(JRC): It's possible for the 'src_adjset_groups' node to be empty,
            // so we only want to query child data types if we know there is at least
            // 1 non-empty group.
            const DataType src_neighbors_dtype = src_adjset_groups.child(0)["neighbors"].dtype();
            const DataType src_values_dtype = src_adjset_groups.child(0)["values"].dtype();

            const auto &group_nidxs = group_pair.first;
            const auto &group_entities = group_pair.second;
            std::string group_name;
            {
                // NOTE(JRC): The current domain is included in the domain name so that
                // it matches across all domains and processors (also, using std::set
                // ensures that items are sorted and the order is the same across ranks).
                std::set<index_t> group_all_nidxs = group_nidxs;
                group_all_nidxs.insert(domain_id);

                std::ostringstream oss;
                oss << "group";
                for(const index_t &group_nidx : group_all_nidxs)
                {
                    oss << "_" << group_nidx;
                }
                group_name = oss.str();
            }

            Node &dst_group = dst_adjset_groups[group_name];
            Node &dst_neighbors = dst_group["neighbors"];
            Node &dst_values = dst_group["values"];

            dst_neighbors.set(DataType(src_neighbors_dtype.id(), group_nidxs.size()));
            index_t ni = 0;
            auto neighbors = dst_neighbors.as_index_t_accessor();
            for(auto nitr = group_nidxs.begin(); nitr != group_nidxs.end(); ++nitr)
            {
                neighbors.set(ni++, *nitr);
            }

            dst_values.set(DataType(src_values_dtype.id(), group_entities.size()));
            auto values = dst_values.as_index_t_accessor();
            for(index_t ei = 0; ei < (index_t)group_entities.size(); ei++)
            {
                const index_t nodeid = std::get<1>(group_entities[ei]);
                values.set(ei, nodeid);
            }
        }
    }
    CONDUIT_ANNOTATE_MARK_END("build");
}


//-----------------------------------------------------------------------------
static void
verify_generate_mesh(const conduit::Node &mesh,
                     const std::string &adjset_name)
{
    const std::vector<const Node *> domains = blueprint::mesh::domains(mesh);
    for(index_t di = 0; di < (index_t)domains.size(); di++)
    {
        const Node &domain = *domains[di];
        Node info;

        if(!domain.has_path("adjsets"))
        {
            CONDUIT_ERROR("<blueprint::mpi::mesh::generate_*> " <<
                          "Domain '" << domain.name() << "' lacks adjacency sets.");
        }

        const Node &n_adjsets = domain.fetch_existing("adjsets");

        if(!n_adjsets.has_child(adjset_name))
        {
            CONDUIT_ERROR("<blueprint::mpi::mesh::generate_*> " <<
                          "Requested source adjacency set '" << adjset_name << "' " <<
                          "doesn't exist on domain '" << domain.name() << ".'");
        }

        if(n_adjsets[adjset_name]["association"].as_string() != "vertex")
        {
            CONDUIT_ERROR("<blueprint::mpi::mesh::generate_*> " <<
                          "Given adjacency set has an unsupported association type 'element.'\n" <<
                          "Supported associations:\n" <<
                          "  'vertex'");
        }

        const Node &adjset = n_adjsets[adjset_name];
        const Node *topo_ptr = bputils::find_reference_node(adjset, "topology");
        const Node &topo = *topo_ptr;
        if(!conduit::blueprint::mesh::topology::unstructured::verify(topo, info))
        {
            CONDUIT_ERROR("<blueprint::mpi::mesh::generate_*> " <<
                          "Requested source topology '" << topo.name() << "' " <<
                          "is of unsupported type '" << topo["type"].as_string() << ".'\n" <<
                          "Supported types:\n" <<
                          "  'unstructured'");
        }
    }
}

//-----------------------------------------------------------------------------
void
mesh::generate_points(conduit::Node &mesh,
                      const std::string &src_adjset_name,
                      const std::string &dst_adjset_name,
                      const std::string &dst_topo_name,
                      conduit::Node &s2dmap,
                      conduit::Node &d2smap,
                      conduit::blueprint::mesh::utils::query::MatchQuery &query,
                      conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    verify_generate_mesh(mesh, src_adjset_name);
    generate_derived_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_points, query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_points(conduit::Node &mesh,
                      const std::string &src_adjset_name,
                      const std::string &dst_adjset_name,
                      const std::string &dst_topo_name,
                      conduit::Node &s2dmap,
                      conduit::Node &d2smap)
{
    verify_generate_mesh(mesh, src_adjset_name);
    conduit::blueprint::mesh::utils::query::MatchQuery query(mesh);
    conduit::blueprint::mesh::utils::topology::MeshInfoCollection mic;
    generate_derived_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_points, query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_lines(conduit::Node &mesh,
                     const std::string &src_adjset_name,
                     const std::string &dst_adjset_name,
                     const std::string &dst_topo_name,
                     conduit::Node &s2dmap,
                     conduit::Node &d2smap,
                     conduit::blueprint::mesh::utils::query::MatchQuery &query,
                     conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    verify_generate_mesh(mesh, src_adjset_name);
    generate_derived_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_lines, query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_lines(conduit::Node &mesh,
                     const std::string &src_adjset_name,
                     const std::string &dst_adjset_name,
                     const std::string &dst_topo_name,
                     conduit::Node &s2dmap,
                     conduit::Node &d2smap)
{
    verify_generate_mesh(mesh, src_adjset_name);
    conduit::blueprint::mesh::utils::query::MatchQuery query(mesh);
    conduit::blueprint::mesh::utils::topology::MeshInfoCollection mic;

    generate_derived_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_lines, query, mic);
}


//-----------------------------------------------------------------------------
void
mesh::generate_faces(conduit::Node &mesh,
                     const std::string& src_adjset_name,
                     const std::string& dst_adjset_name,
                     const std::string& dst_topo_name,
                     conduit::Node& s2dmap,
                     conduit::Node& d2smap,
                     conduit::blueprint::mesh::utils::query::MatchQuery &query,
                     conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    verify_generate_mesh(mesh, src_adjset_name);
    generate_derived_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_faces, query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_faces(conduit::Node &mesh,
                     const std::string& src_adjset_name,
                     const std::string& dst_adjset_name,
                     const std::string& dst_topo_name,
                     conduit::Node& s2dmap,
                     conduit::Node& d2smap)
{
    verify_generate_mesh(mesh, src_adjset_name);
    conduit::blueprint::mesh::utils::query::MatchQuery query(mesh);
    conduit::blueprint::mesh::utils::topology::MeshInfoCollection mic;

    generate_derived_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_faces, query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_centroids(conduit::Node& mesh,
                         const std::string& src_adjset_name,
                         const std::string& dst_adjset_name,
                         const std::string& dst_topo_name,
                         const std::string& dst_cset_name,
                         conduit::Node& s2dmap,
                         conduit::Node& d2smap,
                         conduit::blueprint::mesh::utils::query::PointQueryBase &query,
                         conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    const static auto identify_centroid = []
        (const bputils::TopologyMetadata &/*topo_data*/, const index_t ei, const index_t /*di*/)
    {
        return ei;
    };

    const static auto calculate_centroid_dims = [] (const bputils::ShapeType &topo_shape)
    {
        return std::vector<index_t>(1, topo_shape.dim);
    };

    verify_generate_mesh(mesh, src_adjset_name);

    const std::vector<index_t> centroid_dims = calculate_decomposed_dims(
        mesh, src_adjset_name, calculate_centroid_dims);

    generate_decomposed_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, dst_cset_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_centroids, identify_centroid, centroid_dims,
        query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_centroids(conduit::Node& mesh,
                         const std::string& src_adjset_name,
                         const std::string& dst_adjset_name,
                         const std::string& dst_topo_name,
                         const std::string& dst_cset_name,
                         conduit::Node& s2dmap,
                         conduit::Node& d2smap)
{
    conduit::blueprint::mesh::utils::query::PointQueryBase query(mesh);
    conduit::blueprint::mesh::utils::topology::MeshInfoCollection mic;

    generate_centroids(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, dst_cset_name, s2dmap, d2smap,
        query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_sides(conduit::Node& mesh,
                     const std::string& src_adjset_name,
                     const std::string& dst_adjset_name,
                     const std::string& dst_topo_name,
                     const std::string& dst_cset_name,
                     conduit::Node& s2dmap,
                     conduit::Node& d2smap,
                     conduit::blueprint::mesh::utils::query::PointQueryBase &query,
                     conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    const static auto identify_side = []
        (const bputils::TopologyMetadata &topo_data, const index_t ei, const index_t di)
    {
        index_t doffset = 0;
        for(index_t dii = 0; dii < di; dii++)
        {
            if(dii != 1)
            {
                doffset += topo_data.get_length(dii);
            }
        }

        return doffset + ei;
    };

    const static auto calculate_side_dims = [] (const bputils::ShapeType &topo_shape)
    {
        std::vector<index_t> side_dims;

        side_dims.push_back(index_t{0});
        if(topo_shape.dim == 3)
        {
            side_dims.push_back(index_t{2});
        }

        return side_dims;
    };

    verify_generate_mesh(mesh, src_adjset_name);

    const std::vector<index_t> side_dims = calculate_decomposed_dims(
        mesh, src_adjset_name, calculate_side_dims);

    generate_decomposed_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, dst_cset_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_sides, identify_side, side_dims,
        query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_sides(conduit::Node& mesh,
                     const std::string& src_adjset_name,
                     const std::string& dst_adjset_name,
                     const std::string& dst_topo_name,
                     const std::string& dst_cset_name,
                     conduit::Node& s2dmap,
                     conduit::Node& d2smap)
{
    conduit::blueprint::mesh::utils::query::PointQueryBase query(mesh);
    conduit::blueprint::mesh::utils::topology::MeshInfoCollection mic;

    generate_sides(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, dst_cset_name, s2dmap, d2smap,
        query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_corners(conduit::Node& mesh,
                       const std::string& src_adjset_name,
                       const std::string& dst_adjset_name,
                       const std::string& dst_topo_name,
                       const std::string& dst_cset_name,
                       conduit::Node& s2dmap,
                       conduit::Node& d2smap,
                       conduit::blueprint::mesh::utils::query::PointQueryBase &query,
                       conduit::blueprint::mesh::utils::topology::MeshInfoCollection &mic)
{
    const static auto identify_corner = []
        (const bputils::TopologyMetadata &topo_data, const index_t ei, const index_t di)
    {
        index_t doffset = 0;
        for(index_t dii = 0; dii < di; dii++)
        {
            doffset += topo_data.get_length(dii);
        }

        return doffset + ei;
    };

    const static auto calculate_corner_dims = [] (const bputils::ShapeType &topo_shape)
    {
        std::vector<index_t> corner_dims;

        for(index_t di = 0; di < topo_shape.dim; di++)
        {
            corner_dims.push_back(di);
        }

        return corner_dims;
    };

    verify_generate_mesh(mesh, src_adjset_name);

    const std::vector<index_t> corner_dims = calculate_decomposed_dims(
        mesh, src_adjset_name, calculate_corner_dims);

    generate_decomposed_entities(
        mesh, src_adjset_name, dst_adjset_name, dst_topo_name, dst_cset_name, s2dmap, d2smap,
        conduit::blueprint::mesh::topology::unstructured::generate_corners, identify_corner, corner_dims,
        query, mic);
}

//-----------------------------------------------------------------------------
void
mesh::generate_corners(conduit::Node& mesh,
                       const std::string& src_adjset_name,
                       const std::string& dst_adjset_name,
                       const std::string& dst_topo_name,
                       const std::string& dst_cset_name,
                       conduit::Node& s2dmap,
                       conduit::Node& d2smap)
{
    // Q: Should we use PointQuery instead so it actually checks for non-existent points?
    //    The PointQueryBase will preserve prior behavior.
    conduit::blueprint::mesh::utils::query::PointQueryBase query(mesh);
    conduit::blueprint::mesh::utils::topology::MeshInfoCollection mic;

    generate_corners(mesh, src_adjset_name, dst_adjset_name, dst_topo_name, dst_cset_name,
                     s2dmap, d2smap, query, mic);
}

//-----------------------------------------------------------------------------
// blueprint::mesh::logical_dims protocol interface
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
bool
mesh::logical_dims::verify(const Node &dims,
                           Node &info)
{
    const std::string protocol = "mesh::logical_dims";
    bool res = true;
    info.reset();

    res &= verify_integer_field(protocol, dims, info, "i");
    if(dims.has_child("j"))
    {
        res &= verify_integer_field(protocol, dims, info, "j");
    }
    if(dims.has_child("k"))
    {
        res &= verify_integer_field(protocol, dims, info, "k");
    }

    log::validation(info, res);

    return res;
}


//-----------------------------------------------------------------------------
// blueprint::mesh::association protocol interface
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
bool
mesh::association::verify(const Node &assoc,
                          Node &info)
{
    const std::string protocol = "mesh::association";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, assoc, info, "", bputils::ASSOCIATIONS);

    log::validation(info, res);

    return res;
}


//-----------------------------------------------------------------------------
// blueprint::mesh::coordset protocol interface
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// blueprint::mesh::coordset::verify protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::coordset::uniform::origin::verify(const Node &origin,
                                        Node &info)
{
    const std::string protocol = "mesh::coordset::uniform::origin";
    bool res = true;
    info.reset();

    for(size_t i = 0; i < bputils::COORDINATE_AXES.size(); i++)
    {
        const std::string &coord_axis = bputils::COORDINATE_AXES[i];
        if(origin.has_child(coord_axis))
        {
            res &= verify_number_field(protocol, origin, info, coord_axis);
        }
    }

    log::validation(info, res);

    return res;
}



//-----------------------------------------------------------------------------
bool
mesh::coordset::uniform::spacing::verify(const Node &spacing,
                                         Node &info)
{
    const std::string protocol = "mesh::coordset::uniform::spacing";
    bool res = true;
    info.reset();

    for(size_t i = 0; i < bputils::COORDINATE_AXES.size(); i++)
    {
        const std::string &coord_axis = bputils::COORDINATE_AXES[i];
        const std::string coord_axis_spacing = "d" + coord_axis;
        if(spacing.has_child(coord_axis_spacing))
        {
            res &= verify_number_field(protocol, spacing, info, coord_axis_spacing);
        }
    }

    log::validation(info,res);

    return res;
}

//-----------------------------------------------------------------------------
bool
mesh::coordset::uniform::verify(const Node &coordset,
                                Node &info)
{
    const std::string protocol = "mesh::coordset::uniform";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, coordset, info, "type",
        std::vector<std::string>(1, "uniform"));

    res &= verify_object_field(protocol, coordset, info, "dims") &&
           mesh::logical_dims::verify(coordset["dims"], info["dims"]);

    if(coordset.has_child("origin"))
    {
        log::optional(info, protocol, "has origin");
        res &= mesh::coordset::uniform::origin::verify(coordset["origin"],
                                                       info["origin"]);
    }

    if(coordset.has_child("spacing"))
    {
        log::optional(info,protocol, "has spacing");
        res &= mesh::coordset::uniform::spacing::verify(coordset["spacing"],
                                                        info["spacing"]);
    }

    log::validation(info,res);

    return res;
}

//-----------------------------------------------------------------------------
bool
mesh::coordset::rectilinear::verify(const Node &coordset,
                                    Node &info)
{
    const std::string protocol = "mesh::coordset::rectilinear";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, coordset, info, "type",
        std::vector<std::string>(1, "rectilinear"));

    if(!verify_object_field(protocol, coordset, info, "values", true))
    {
        res = false;
    }
    else
    {
        NodeConstIterator itr = coordset["values"].children();
        while(itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();
            if(!chld.dtype().is_number())
            {
                log::error(info, protocol, "value child " + log::quote(chld_name) +
                                           " is not a number array");
                res = false;
            }
        }
    }

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
bool
mesh::coordset::_explicit::verify(const Node &coordset,
                                  Node &info)
{
    const std::string protocol = "mesh::coordset::explicit";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, coordset, info, "type",
        std::vector<std::string>(1, "explicit"));

    res &= verify_mcarray_field(protocol, coordset, info, "values");

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
bool
mesh::coordset::verify(const Node &coordset,
                       Node &info)
{
    const std::string protocol = "mesh::coordset";
    bool res = true;
    info.reset();

    res &= verify_field_exists(protocol, coordset, info, "type") &&
           mesh::coordset::type::verify(coordset["type"], info["type"]);

    if(res)
    {
        const std::string type_name = coordset["type"].as_string();

        if(type_name == "uniform")
        {
            res = mesh::coordset::uniform::verify(coordset,info);
        }
        else if(type_name == "rectilinear")
        {
            res = mesh::coordset::rectilinear::verify(coordset,info);
        }
        else if(type_name == "explicit")
        {
            res = mesh::coordset::_explicit::verify(coordset,info);
        }
    }

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
index_t
mesh::coordset::dims(const Node &coordset)
{
    return bputils::coordset::dims(coordset);
}


//-----------------------------------------------------------------------------
index_t
mesh::coordset::length(const Node &coordset)
{
    return bputils::coordset::length(coordset);
}


//-----------------------------------------------------------------------------
void
mesh::coordset::generate_strip(const Node& coordset,
                               conduit::Node& coordset_dest)
{
    coordset_dest.reset();
    std::string coord_type = coordset["type"].as_string();
    coordset_dest["type"].set(coord_type);

    if (coord_type == "uniform")
    {
        coordset_dest["dims/i"] = 1;
        coordset_dest["dims/j"] = coordset["dims/i"];

        if (coordset.has_child("origin"))
        {
            coordset_dest["origin/x"] = 0.;
            coordset_dest["origin/y"] = coordset["origin/x"];
        }

        if (coordset.has_child("spacing"))
        {
            coordset_dest["spacing/dx"] = 1.;
            coordset_dest["spacing/dy"] = coordset["spacing/dx"];
        }
    }
    else
    {
        coordset_dest["values/x"].set(DataType::float64(2));
        double* x_vals = coordset_dest["values/x"].value();
        x_vals[0] = 0.;
        x_vals[1] = 1.;
        coordset["values/x"].to_float64_array(coordset_dest["values/y"]);
    }
}

//-----------------------------------------------------------------------------
void
mesh::coordset::to_explicit(const conduit::Node& coordset,
                            conduit::Node& coordset_dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    std::string type = coordset.fetch_existing("type").as_string();

    if(type == "uniform")
        mesh::coordset::uniform::to_explicit(coordset, coordset_dest);
    else if(type == "rectilinear")
        mesh::coordset::rectilinear::to_explicit(coordset, coordset_dest);
    else if(type == "explicit")
        coordset_dest.set_external(coordset);
}

//-----------------------------------------------------------------------------
void
mesh::coordset::uniform::to_rectilinear(const conduit::Node &coordset,
                                        conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    convert_coordset_to_rectilinear("uniform", coordset, dest);
}


//-----------------------------------------------------------------------------
void
mesh::coordset::uniform::to_explicit(const conduit::Node &coordset,
                                     conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    convert_coordset_to_explicit("uniform", coordset, dest);
}


//-----------------------------------------------------------------------------
void
mesh::coordset::rectilinear::to_explicit(const conduit::Node &coordset,
                                         conduit::Node &dest)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    convert_coordset_to_explicit("rectilinear", coordset, dest);
}


//-----------------------------------------------------------------------------
// blueprint::mesh::coordset::type protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::coordset::type::verify(const Node &type,
                             Node &info)
{
    const std::string protocol = "mesh::coordset::type";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, type, info, "", bputils::COORD_TYPES);

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
// blueprint::mesh::coordset::coord_system protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::coordset::coord_system::verify(const Node &coord_sys,
                                     Node &info)
{
    const std::string protocol = "mesh::coordset::coord_system";
    bool res = true;
    info.reset();

    std::string coord_sys_str = "unknown";
    if(!verify_enum_field(protocol, coord_sys, info, "type", bputils::COORD_SYSTEMS))
    {
        res = false;
    }
    else
    {
        coord_sys_str = coord_sys["type"].as_string();
    }

    if(!verify_object_field(protocol, coord_sys, info, "axes"))
    {
        res = false;
    }
    else if(coord_sys_str != "unknown")
    {
        NodeConstIterator itr = coord_sys["axes"].children();
        while(itr.has_next())
        {
            itr.next();
            const std::string axis_name = itr.name();

            bool axis_name_ok = true;
            if(coord_sys_str == "cartesian")
            {
                axis_name_ok = axis_name == "x" || axis_name == "y" ||
                               axis_name == "z";
            }
            else if(coord_sys_str == "cylindrical")
            {
                axis_name_ok = axis_name == "r" || axis_name == "z";
            }
            else if(coord_sys_str == "spherical")
            {
                axis_name_ok = axis_name == "r" || axis_name == "theta" ||
                               axis_name == "phi";
            }

            if(!axis_name_ok)
            {
                log::error(info, protocol, "unsupported " + coord_sys_str +
                                           " axis name: " + axis_name);
                res = false;
            }
        }
    }

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
// blueprint::mesh::coordset::index protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::coordset::index::verify(const Node &coordset_idx,
                              Node &info)
{
    const std::string protocol = "mesh::coordset::index";
    bool res = true;
    info.reset();

    res &= verify_field_exists(protocol, coordset_idx, info, "type") &&
           mesh::coordset::type::verify(coordset_idx["type"], info["type"]);
    res &= verify_string_field(protocol, coordset_idx, info, "path");
    res &= verify_object_field(protocol, coordset_idx, info, "coord_system") &&
           coordset::coord_system::verify(coordset_idx["coord_system"], info["coord_system"]);

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
// blueprint::mesh::topology protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::verify(const Node &topo,
                       Node &info)
{
    const std::string protocol = "mesh::topology";
    bool res = true;
    info.reset();

    if(!(verify_field_exists(protocol, topo, info, "type") &&
         mesh::topology::type::verify(topo["type"], info["type"])))
    {
        res = false;
    }
    else
    {
        const std::string topo_type = topo["type"].as_string();

        if(topo_type == "points")
        {
            res &= mesh::topology::points::verify(topo,info);
        }
        else if(topo_type == "uniform")
        {
            res &= mesh::topology::uniform::verify(topo,info);
        }
        else if(topo_type == "rectilinear")
        {
            res &= mesh::topology::rectilinear::verify(topo,info);
        }
        else if(topo_type == "structured")
        {
            res &= mesh::topology::structured::verify(topo,info);
        }
        else if(topo_type == "unstructured")
        {
            res &= mesh::topology::unstructured::verify(topo,info);
        }
    }

    if(topo.has_child("grid_function"))
    {
        log::optional(info, protocol, "includes grid_function");
        res &= verify_string_field(protocol, topo, info, "grid_function");
    }

    log::validation(info,res);

    return res;

}


//-----------------------------------------------------------------------------
index_t
mesh::topology::dims(const Node &topology)
{
    return bputils::topology::dims(topology);
}


//-----------------------------------------------------------------------------
index_t
mesh::topology::length(const Node &topology)
{
    return bputils::topology::length(topology);
}

//-----------------------------------------------------------------------------
void
mesh::topology::generate_strip(const Node& topo,
                               const std::string & csname,
                               conduit::Node& topo_dest)
{
    topo_dest.reset();
    topo_dest["type"] = topo["type"].as_string();
    topo_dest["coordset"] = csname;

    if (topo.has_child("elements"))
    {
        const Node& topoelts = topo["elements"];
        Node& destelts = topo_dest["elements"];

        if (topoelts.has_child("origin"))
        {
            destelts["origin/i"] = 0.;
            destelts["origin/j"] = topoelts["origin/i"];
        }

        if (topoelts.has_child("dims"))
        {
            destelts["dims/i"] = 1;
            destelts["dims/j"] = topoelts["dims/i"];
        }
    }
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::points protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::points::verify(const Node &topo,
                               Node &info)
{
    const std::string protocol = "mesh::topology::points";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, topo, info, "coordset");

    res &= verify_enum_field(protocol, topo, info, "type",
        std::vector<std::string>(1, "points"));

    // if needed in the future, can be used to verify optional info for
    // implicit 'points' topology

    log::validation(info,res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::uniform protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::uniform::verify(const Node &topo,
                                Node &info)
{
    const std::string protocol = "mesh::topology::uniform";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, topo, info, "coordset");

    res &= verify_enum_field(protocol, topo, info, "type",
        std::vector<std::string>(1, "uniform"));

    // future: will be used to verify optional info from "elements"
    // child of a uniform topology

    log::validation(info,res);

    return res;
}


//-------------------------------------------------------------------------
void
mesh::topology::uniform::to_rectilinear(const conduit::Node &topo,
                                        conduit::Node &topo_dest,
                                        conduit::Node &coords_dest)
{
    convert_topology_to_rectilinear("uniform", topo, topo_dest, coords_dest);
}


//-------------------------------------------------------------------------
void
mesh::topology::uniform::to_structured(const conduit::Node &topo,
                                       conduit::Node &topo_dest,
                                       conduit::Node &coords_dest)
{
    convert_topology_to_structured("uniform", topo, topo_dest, coords_dest);
}


//-------------------------------------------------------------------------
void
mesh::topology::uniform::to_unstructured(const conduit::Node &topo,
                                         conduit::Node &topo_dest,
                                         conduit::Node &coords_dest)
{
    convert_topology_to_unstructured("uniform", topo, topo_dest, coords_dest);
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::rectilinear protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::rectilinear::verify(const Node &topo,
                                    Node &info)
{
    const std::string protocol = "mesh::topology::rectilinear";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, topo, info, "coordset");

    res &= verify_enum_field(protocol, topo, info, "type",
        std::vector<std::string>(1, "rectilinear"));

    // future: will be used to verify optional info from "elements"
    // child of a rectilinear topology

    log::validation(info,res);

    return res;
}


//-------------------------------------------------------------------------
void
mesh::topology::rectilinear::to_structured(const conduit::Node &topo,
                                           conduit::Node &topo_dest,
                                           conduit::Node &coords_dest)
{
    convert_topology_to_structured("rectilinear", topo, topo_dest, coords_dest);
}


//-------------------------------------------------------------------------
void
mesh::topology::rectilinear::to_unstructured(const conduit::Node &topo,
                                             conduit::Node &topo_dest,
                                             conduit::Node &coords_dest)
{
    convert_topology_to_unstructured("rectilinear", topo, topo_dest, coords_dest);
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::structured protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::structured::verify(const Node &topo,
                                   Node &info)
{
    const std::string protocol = "mesh::topology::structured";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, topo, info, "coordset");

    res &= verify_enum_field(protocol, topo, info, "type",
        std::vector<std::string>(1, "structured"));

    if(!verify_object_field(protocol, topo, info, "elements"))
    {
        res = false;
    }
    else
    {
        const Node &topo_elements = topo["elements"];
        Node &info_elements = info["elements"];

        bool elements_res =
            verify_object_field(protocol, topo_elements, info_elements, "dims") &&
            mesh::logical_dims::verify(topo_elements["dims"], info_elements["dims"]);

        log::validation(info_elements,elements_res);
        res &= elements_res;
    }

    // FIXME: Add some verification code here for the optional origin in the
    // structured topology.

    log::validation(info,res);

    return res;
}


//-------------------------------------------------------------------------
void
mesh::topology::structured::to_unstructured(const conduit::Node &topo,
                                            conduit::Node &topo_dest,
                                            conduit::Node &coords_dest)
{
    convert_topology_to_unstructured("structured", topo, topo_dest, coords_dest);
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::unstructured protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::unstructured::verify(const Node &topo,
                                     Node &info)
{
    const std::string protocol = "mesh::topology::unstructured";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, topo, info, "coordset");

    res &= verify_enum_field(protocol, topo, info, "type",
        std::vector<std::string>(1, "unstructured"));

    if(!verify_object_field(protocol, topo, info, "elements"))
    {
        res = false;
    }
    else
    {
        const Node &topo_elems = topo["elements"];
        Node &info_elems = info["elements"];

        bool elems_res = true;
        bool subelems_res = true;

        // single shape or mixed definition case
        if(topo_elems.has_child("shape"))
        {
            elems_res &= verify_field_exists(protocol, topo_elems, info_elems, "shape") &&
                   mesh::topology::shape::verify(topo_elems["shape"], info_elems["shape"]);
            elems_res &= verify_integer_field(protocol, topo_elems, info_elems, "connectivity");

            if (topo_elems["shape"].dtype().is_string() &&
                topo_elems["shape"].as_string() == "mixed")
            {
              elems_res &= verify_mixed_node(topo, info, elems_res, subelems_res);
            }
            else
            {
              // Verify if node is polygonal or polyhedral
              elems_res &= verify_poly_node(false, "", topo_elems, info_elems, topo, info, elems_res);
            }
        }
        // shape stream case
        else if(topo_elems.has_child("element_types"))
        {
            // TODO
        }
        // mixed shape case
        else if(topo_elems.number_of_children() != 0)
        {
            bool has_names = topo_elems.dtype().is_object();

            NodeConstIterator itr = topo_elems.children();
            while(itr.has_next())
            {
                const Node &chld  = itr.next();
                std::string name = itr.name();
                Node &chld_info = has_names ? info["elements"][name] :
                    info["elements"].append();

                bool chld_res = true;
                chld_res &= verify_field_exists(protocol, chld, chld_info, "shape") &&
                       mesh::topology::shape::verify(chld["shape"], chld_info["shape"]);
                chld_res &= verify_integer_field(protocol, chld, chld_info, "connectivity");

                // Verify if child is polygonal or polyhedral
                chld_res &= verify_poly_node (true, name, chld, chld_info, topo, info, elems_res);

                log::validation(chld_info,chld_res);
                elems_res &= chld_res;
            }
        }
        else
        {
            log::error(info,protocol,"invalid child 'elements'");
            res = false;
        }

        log::validation(info_elems,elems_res);
        res &= elems_res;
        res &= subelems_res;
    }

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::to_polytopal(const Node &topo,
                                           Node &dest)
{
    to_polygonal(topo,dest);
}

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::detail --
//-----------------------------------------------------------------------------
namespace detail
{

template <typename IndexAccessor>
struct accessor_traits
{
  using value_type = typename IndexAccessor::value_type;
  static bool is_empty(const IndexAccessor obj) { return obj.number_of_elements() == 0; }
};

template <>
struct accessor_traits<const int64 *>
{
  using value_type = int64;
  static bool is_empty(const int64 *obj) { return obj == nullptr; }
};
template <>
struct accessor_traits<const int32 *>
{
  using value_type = int32;
  static bool is_empty(const int32 *obj) { return obj == nullptr; }
};

/*!
 * @brief Get the unique face id for the given face (provided as indices). If the
 *        face is not defined, it gets defined.
 *
 * @param face_indices The vertex ids that make up the face.
 * @param face_size The number of vertices in the face.
 * @param[inout] faceHashToFaceId A map to contain face hashes to face ids. It is used to make unique faces.
 * @param[out] subelements_connectivity A vector containing subelement connectivity (face definitions).
 * @param[out] subelements_sizes A vector containing subelement sizes.
 *
 * @return The face id.
 */
template <typename ConnectivityType>
ConnectivityType get_unique_face(const ConnectivityType *face_indices,
                                 index_t face_size,
                                 std::map<uint64, ConnectivityType> &faceHashToFaceId,
                                 std::vector<ConnectivityType> &subelements_connectivity,
                                 std::vector<ConnectivityType> &subelements_sizes)
{
    constexpr int MAX_VERTICES_PER_FACE = 4;
    index_t sorted_face_indices[MAX_VERTICES_PER_FACE];
    for (index_t vi = 0; vi < face_size; vi++)
    {
        sorted_face_indices[vi] = static_cast<index_t>(face_indices[vi]);
    }
    // Sort the face's vertices so we can make a faceId for it.
    std::sort(sorted_face_indices, sorted_face_indices + face_size);
    const auto faceHash = conduit::utils::hash(sorted_face_indices, static_cast<unsigned int>(face_size));

    auto it = faceHashToFaceId.find(faceHash);
    ConnectivityType faceId{};
    if(it == faceHashToFaceId.end())
    {
        // Define the face.
        faceId = static_cast<ConnectivityType>(subelements_sizes.size());
        faceHashToFaceId[faceHash] = faceId;

        subelements_connectivity.insert(subelements_connectivity.end(),
            face_indices, face_indices + face_size);
        subelements_sizes.push_back(static_cast<ConnectivityType>(face_size));
    }
    else
    {
        faceId = it->second;
    }
    return faceId;
}

/*!
 * @brief Given an element shape and connectivity, iterate over the faces for the
 *        element, make unique faces, and assemble a polyhedral element that references
 *        those faces.
 *
 * @param shape The element shape.
 * @param src_elements_connectivity An accessor for the element connectivity.
 * @param src_element_offset An offset into the connectivity array.
 * @param[inout] faceHashToFaceId A map to contain face hashes to face ids. It is used to make unique faces.
 * @param[out] elements_connectivity A vector containing element connectivity.
 * @param[out] elements_sizes A vector containing element sizes.
 * @param[out] subelements_connectivity A vector containing subelement connectivity (face definitions).
 * @param[out] subelements_sizes A vector containing subelement sizes.
 */
template <typename ConnectivityType, typename IndexAccessor>
void define_faces_and_element(const ShapeType &shape,
                              IndexAccessor src_elements_connectivity,
                              index_t src_element_offset,
                              std::map<uint64, ConnectivityType> &faceHashToFaceId,
                              std::vector<ConnectivityType> &elements_connectivity,
                              std::vector<ConnectivityType> &elements_sizes,
                              std::vector<ConnectivityType> &subelements_connectivity,
                              std::vector<ConnectivityType> &subelements_sizes)
{
    constexpr int MAX_VERTICES_PER_FACE = 4;
    ConnectivityType face_indices[MAX_VERTICES_PER_FACE];

    const auto src_element_num_faces = shape.num_faces();
    for (index_t fi = 0; fi < src_element_num_faces; fi++)
    {
        // Get the current face's vertices (numbered starting at 0).
        index_t faceVertexSize = 0;
        const index_t *faceVertexIds = shape.get_face(fi, faceVertexSize);

        // Translate the faceVertexIds to the current element's actual vertices.
        for (index_t vi = 0; vi < faceVertexSize; vi++)
        {
            face_indices[vi] = src_elements_connectivity[src_element_offset + faceVertexIds[vi]];
        }
        const auto faceId = get_unique_face(face_indices,
                                            faceVertexSize,
                                            faceHashToFaceId,
                                            subelements_connectivity,
                                            subelements_sizes);

        // Reference the face in the element connectivity
        elements_connectivity.push_back(faceId);
    }
    elements_sizes.push_back(static_cast<ConnectivityType>(src_element_num_faces));
}

/*!
 * @brief Convert unstructured topology (zoo elements all the same shape) to polyhedral topology.
 *
 * @tparam IndexAccessor The container type that contains the connectivity and related arrays.
 *                       This can be pointers or DataAccessors.
 *
 * @param topo The Node that contains the input unstructured topology.
 * @param[out] dest The Node that will contain the output polyhedral topology.
 *
 */
template <typename IndexAccessor>
void unstructured_to_polyhedral(const conduit::Node &topo, conduit::Node &dest)
{
    using ConnectivityType = typename accessor_traits<IndexAccessor>::value_type;

    const ShapeCascade topo_cascade(topo);
    const ShapeType topo_shape(topo_cascade.get_shape());

    // Access the connectivity and offsets (if present).
    const conduit::Node &n_src_elements_connectivity = topo["elements/connectivity"];
    const index_t numElements = n_src_elements_connectivity.dtype().number_of_elements() / topo_shape.indices;
    const IndexAccessor src_elements_connectivity = n_src_elements_connectivity.value();
    IndexAccessor src_elements_offsets {};
    if(topo.has_path("elements/offsets"))
    {
        src_elements_offsets = topo["elements/offsets"].value();
    }

    // Generate each polyhedral element by generating its constituent
    // polygonal faces. Also, make sure that faces connecting the same
    // set of vertices aren't duplicated;

    // NOTE: This algorithm does not use Shape embedding because for shapes
    //       such as pyramids and wedges, it breaks faces into triangles.
    std::map<uint64, ConnectivityType> faceHashToFaceId;
    std::vector<ConnectivityType> elements_connectivity;
    std::vector<ConnectivityType> elements_sizes;
    elements_sizes.reserve(numElements);
    std::vector<ConnectivityType> subelements_connectivity;
    std::vector<ConnectivityType> subelements_sizes;

    for (index_t ei = 0; ei < numElements; ei++)
    {
        const auto src_element_size = topo_shape.indices;
        // Support optional offsets
        const auto src_element_offset = accessor_traits<IndexAccessor>::is_empty(src_elements_offsets) ? (src_element_size * ei) : src_elements_offsets[ei];
        define_faces_and_element(topo_shape,
                                 src_elements_connectivity,
                                 src_element_offset,
                                 faceHashToFaceId,
                                 elements_connectivity,
                                 elements_sizes,
                                 subelements_connectivity,
                                 subelements_sizes);
    }

    dest["type"] = "unstructured";
    dest["coordset"].set(topo["coordset"]);
    dest["elements/shape"] = "polyhedral";
    dest["elements/connectivity"].set(elements_connectivity);
    dest["elements/sizes"].set(elements_sizes);
    dest["subelements/shape"] = "polygonal";
    dest["subelements/connectivity"].set(subelements_connectivity);
    dest["subelements/sizes"].set(subelements_sizes);
    conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets_inline(dest);
}

/*!
 * @brief Convert mixed unstructured topology to polyhedral topology.
 *
 * @tparam IndexAccessor The container type that contains the connectivity and related arrays.
 *                       This can be pointers or DataAccessors.
 *
 * @param topo The Node that contains the input unstructured topology.
 * @param[out] dest The Node that will contain the output polyhedral topology.
 *
 */
template <typename IndexAccessor>
void unstructured_mixed_to_polyhedral(const conduit::Node &topo, conduit::Node &dest)
{
    // Use index_t because the element traversal API uses it.
    using ConnectivityType = index_t;

    std::map<uint64, ConnectivityType> faceHashToFaceId;
    std::vector<ConnectivityType> elements_connectivity;
    std::vector<ConnectivityType> elements_sizes;
    std::vector<ConnectivityType> subelements_connectivity;
    std::vector<ConnectivityType> subelements_sizes;

    // Iterate over each mixed element and define it as a polyhedron.
    namespace bpiter = conduit::blueprint::mesh::utils::topology;
    bpiter::impl::traverse_mixed_elements([&](const bpiter::entity &e) {
        if(!e.subelement_ids.empty())
        {
            // We got a polyhedral element
            for(size_t fi = 0; fi < e.subelement_ids.size(); fi++)
            {
                const auto faceId = get_unique_face(e.subelement_ids[fi].data(),
                                                    e.subelement_ids[fi].size(),
                                                    faceHashToFaceId,
                                                    subelements_connectivity,
                                                    subelements_sizes);
                elements_connectivity.push_back(faceId);
            }
            elements_sizes.push_back(e.subelement_ids.size());
        }
        else
        {
            // The element is NOT polyhedral
            define_faces_and_element(e.shape,
                                     e.element_ids,
                                     0,
                                     faceHashToFaceId,
                                     elements_connectivity,
                                     elements_sizes,
                                     subelements_connectivity,
                                     subelements_sizes);
        }
    }, topo);

    dest["type"] = "unstructured";
    dest["coordset"].set(topo["coordset"]);
    dest["elements/shape"] = "polyhedral";
    dest["elements/connectivity"].set(elements_connectivity);
    dest["elements/sizes"].set(elements_sizes);
    dest["subelements/shape"] = "polygonal";
    dest["subelements/connectivity"].set(subelements_connectivity);
    dest["subelements/sizes"].set(subelements_sizes);
    conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets_inline(dest);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::detail --
//-----------------------------------------------------------------------------

void
mesh::topology::unstructured::to_polygonal(const Node &topo,
                                           Node &dest)
{
    dest.reset();

    const ShapeType topo_shape(topo);
    if(topo_shape.is_poly())
    {
        dest.set(topo);
    }
    else // if(!topo_shape.is_poly())
    {
        const bool is_topo_3d = topo_shape.dim == 3;

        if (!is_topo_3d) // polygonal
        {
            const DataType int_dtype = bputils::find_widest_dtype(topo, bputils::DEFAULT_INT_DTYPES);
            const Node &topo_conn_const = topo["elements/connectivity"];
            Node topo_conn; topo_conn.set_external(topo_conn_const);
            const DataType topo_dtype(topo_conn.dtype().id(), 1);

            Node topo_templ;
            topo_templ.set_external(topo);
            topo_templ.remove("elements");
            dest.set(topo_templ);
            dest["elements/shape"].set("polygonal");

            // NOTE(JRC): The derived polygonal topology simply inherits the
            // original implicit connectivity and adds sizes/offsets, which
            // means that it inherits the orientation/winding of the source as well.
            Node temp;
            temp.set_external(topo_conn);
            temp.to_data_type(int_dtype.id(), dest["elements/connectivity"]);

            if(topo.has_path("elements/sizes"))
            {
                // If the topo has sizes, copy them.
                const conduit::Node &elements_sizes = topo["elements/sizes"];
                elements_sizes.to_data_type(int_dtype.id(), dest["elements/sizes"]);
            }
            else
            {
                const index_t topo_indices = topo_conn.dtype().number_of_elements();
                const index_t topo_elems = topo_indices / topo_shape.indices;
                std::vector<int64> poly_size_data(topo_elems, topo_shape.indices);
                temp.set_external(poly_size_data);
                temp.to_data_type(int_dtype.id(), dest["elements/sizes"]);
            }
            utils::topology::unstructured::generate_offsets_inline(dest);
        }
        else // if(is_topo_3d) // polyhedral
        {
            const conduit::Node &n_conn = topo["elements/connectivity"];
            if(n_conn.dtype().is_compact() && n_conn.dtype().is_int64())
            {
               if(topo_shape.is_mixed())
                   detail::unstructured_mixed_to_polyhedral<const int64 *>(topo, dest);
               else
                   detail::unstructured_to_polyhedral<const int64 *>(topo, dest);
            }
            else if(n_conn.dtype().is_compact() && n_conn.dtype().is_int32())
            {
               if(topo_shape.is_mixed())
                   detail::unstructured_mixed_to_polyhedral<const int32 *>(topo, dest);
               else
                   detail::unstructured_to_polyhedral<const int32 *>(topo, dest);
            }
            else
            {
               if(topo_shape.is_mixed())
                   detail::unstructured_mixed_to_polyhedral<DataAccessor<index_t>>(topo, dest);
               else
                   detail::unstructured_to_polyhedral<DataAccessor<index_t>>(topo, dest);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_points(const Node &topo,
                                              Node &dest,
                                              Node &s2dmap,
                                              Node &d2smap)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // TODO(JRC): Revise this function so that it works on every base topology
    // type and then move it to "mesh::topology::{uniform|...}::generate_points".
    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    index_t src_dim = blueprint::mesh::topology::dims(topo);
    index_t dst_dim = 0;
    // Request only these maps.
    std::vector<std::pair<size_t, size_t>> desired;
    desired.push_back(std::make_pair(static_cast<size_t>(src_dim),
                                     static_cast<size_t>(dst_dim)));
    desired.push_back(std::make_pair(static_cast<size_t>(dst_dim),
                                     static_cast<size_t>(src_dim)));
    TopologyMetadata topo_data(topo, *coordset, dst_dim, desired);
    dest.reset();
    topo_data.get_topology(dst_dim, dest);

    topo_data.get_dim_map(TopologyMetadata::GLOBAL, src_dim, dst_dim, s2dmap);
    topo_data.get_dim_map(TopologyMetadata::GLOBAL, dst_dim, src_dim, d2smap);
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_lines(const Node &topo,
                                             Node &dest,
                                             Node &s2dmap,
                                             Node &d2smap)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if(topo.has_path("type") && topo["type"].as_string() != "unstructured")
    {
        CONDUIT_ERROR("The topology was not unstructured.");
    }

    // TODO(JRC): Revise this function so that it works on every base topology
    // type and then move it to "mesh::topology::{uniform|...}::generate_lines".
    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    index_t src_dim = blueprint::mesh::topology::dims(topo);
    index_t dst_dim = 1;
    // Request only these maps.
    std::vector<std::pair<size_t, size_t>> desired;
    desired.push_back(std::make_pair(static_cast<size_t>(src_dim),
                                     static_cast<size_t>(dst_dim)));
    desired.push_back(std::make_pair(static_cast<size_t>(dst_dim),
                                     static_cast<size_t>(src_dim)));
    TopologyMetadata topo_data(topo, *coordset, dst_dim, desired);
    dest.reset();
    topo_data.get_topology(dst_dim, dest);

    topo_data.get_dim_map(TopologyMetadata::GLOBAL, src_dim, dst_dim, s2dmap);
    topo_data.get_dim_map(TopologyMetadata::GLOBAL, dst_dim, src_dim, d2smap);
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_faces(const Node &topo,
                                             Node &dest,
                                             Node &s2dmap,
                                             Node &d2smap)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if(topo.has_path("type") && topo["type"].as_string() != "unstructured")
    {
        CONDUIT_ERROR("The topology was not unstructured.");
    }

    // TODO(JRC): Revise this function so that it works on every base topology
    // type and then move it to "mesh::topology::{uniform|...}::generate_faces".
    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    index_t src_dim = blueprint::mesh::topology::dims(topo);
    index_t dst_dim = 2;
    // Request only these maps.
    std::vector<std::pair<size_t, size_t>> desired;
    desired.push_back(std::make_pair(static_cast<size_t>(src_dim),
                                     static_cast<size_t>(dst_dim)));
    desired.push_back(std::make_pair(static_cast<size_t>(dst_dim),
                                     static_cast<size_t>(src_dim)));
    TopologyMetadata topo_data(topo, *coordset, dst_dim, desired);
    dest.reset();
    topo_data.get_topology(dst_dim, dest);

    topo_data.get_dim_map(TopologyMetadata::GLOBAL, src_dim, dst_dim, s2dmap);
    topo_data.get_dim_map(TopologyMetadata::GLOBAL, dst_dim, src_dim, d2smap);
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_centroids(const Node &topo,
                                                 Node &topo_dest,
                                                 Node &coords_dest,
                                                 Node &s2dmap,
                                                 Node &d2smap)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    // TODO(JRC): Revise this function so that it works on every base topology
    // type and then move it to "mesh::topology::{uniform|...}::generate_centroids".
    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    calculate_unstructured_centroids(topo, *coordset, topo_dest, coords_dest);

    Node map_node;
    std::vector<index_t> map_vec;
    index_t n = bputils::topology::length(topo);
    for(index_t ei = 0; ei < n; ei++)
    {
        map_vec.push_back(1);
        map_vec.push_back(ei);
    }
    map_node.set(map_vec);

    DataType int_dtype = bputils::find_widest_dtype(bputils::link_nodes(topo, *coordset), bputils::DEFAULT_INT_DTYPES);
    s2dmap.reset();
    d2smap.reset();
    map_node.to_data_type(int_dtype.id(), s2dmap);
    map_node.to_data_type(int_dtype.id(), d2smap);
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_sides(const Node &topo,
                                             Node &topo_dest,
                                             Node &coords_dest,
                                             Node &s2dmap,
                                             Node &d2smap)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if(topo.has_path("type") && topo["type"].as_string() != "unstructured")
    {
        CONDUIT_ERROR("The topology was not unstructured.");
    }

    // Retrieve Relevant Coordinate/Topology Metadata //

    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    const std::vector<std::string> csys_axes = bputils::coordset::axes(*coordset);

    const ShapeCascade topo_cascade(topo);
    const ShapeType topo_shape = topo_cascade.get_shape();
    const ShapeType line_shape = topo_cascade.get_shape(1);
    const ShapeType side_shape(topo_shape.dim == 3 ? "tet" : "tri");
    if(topo_shape.dim < 2)
    {
        CONDUIT_ERROR("Failed to generate side mesh for input; " <<
            "input tology must be topologically 2D or 3D.");
    }

    // Extract Derived Coordinate/Topology Data //

    TopologyMetadata topo_data(topo, *coordset);
    const DataType &int_dtype = topo_data.get_int_dtype();
    const DataType &float_dtype = topo_data.get_float_dtype();

    std::vector<conduit::Node> dim_cent_topos(topo_shape.dim + 1);
    std::vector<conduit::Node> dim_cent_coords(topo_shape.dim + 1);

    for(index_t di = 0; di <= topo_shape.dim; di++)
    {
        // NOTE: No centroids are generate for the lines of the geometry
        // because they aren't included in the final sides topology.
        if(di == line_shape.dim) { continue; }

        calculate_unstructured_centroids(
            topo_data.get_topology(di), *coordset,
            dim_cent_topos[di], dim_cent_coords[di]);
    }

    // Allocate Data Templates for Outputs //

    const index_t topo_num_elems = topo_data.get_length(topo_shape.dim);
    const index_t sides_num_coords =
        topo_data.get_length() - topo_data.get_length(line_shape.dim);
    const index_t sides_num_elems =
        topo_data.get_embed_length(topo_shape.dim, line_shape.dim);
    const index_t sides_elem_degree = (topo_shape.dim - line_shape.dim) + 2;

    topo_dest.reset();
    topo_dest["type"].set("unstructured");
    topo_dest["coordset"].set(coords_dest.name());
    topo_dest["elements/shape"].set(side_shape.type());
    topo_dest["elements/connectivity"].set(DataType(int_dtype.id(),
        side_shape.indices * sides_num_elems));

    coords_dest.reset();
    coords_dest["type"].set("explicit");
    for(index_t ai = 0; ai < (index_t)csys_axes.size(); ai++)
    {
        coords_dest["values"][csys_axes[ai]].set(DataType(float_dtype.id(),
            sides_num_coords));
    }

    // Populate Data Arrays w/ Calculated Coordinates //

    std::vector<index_t> dim_coord_offsets(topo_shape.dim + 1);
    for(index_t ai = 0; ai < (index_t)csys_axes.size(); ai++)
    {
        Node dst_data;
        Node &dst_axis = coords_dest["values"][csys_axes[ai]];

        for(index_t di = 0, doffset = 0; di <= topo_shape.dim; di++)
        {
            dim_coord_offsets[di] = doffset;

            // TODO(JRC): This comment may be important for parallel processing;
            // there are a lot of assumptions in that code on how ordering is
            // presented via 'TopologyMataData'.
            //
            // NOTE: The centroid ordering for the positions is different
            // from the base ordering, which messes up all subsequent indexing.
            // We must use the coordinate set associated with the base topology.
            const Node &cset = (di != 0) ? dim_cent_coords[di] : *coordset;
            if(!cset.dtype().is_empty())
            {
                const Node &cset_axis = cset["values"][csys_axes[ai]];
                index_t cset_length = cset_axis.dtype().number_of_elements();
                // TODO: USE ACCESSORS
                dst_data.set_external(DataType(float_dtype.id(), cset_length),
                    dst_axis.element_ptr(doffset));
                cset_axis.to_data_type(float_dtype.id(), dst_data);
                doffset += cset_length;
            }
        }
    }

    // Compute New Elements/Fields for Side Topology //

    int64 elem_index = 0, side_index = 0;
    int64 s2d_val_index = 0, d2s_val_index = 0;
    int64 s2d_elem_index = 0, d2s_elem_index = 0;

    std::vector<int64> side_data_raw(sides_elem_degree);

    Node misc_data;
    Node raw_data(DataType::int64(1));
    Node elem_index_data(DataType::int64(1), &elem_index, true);
    Node side_index_data(DataType::int64(1), &side_index, true);
    Node side_data(DataType::int64(sides_elem_degree), &side_data_raw[0], true);

    s2dmap.reset();
    s2dmap["values"].set(DataType(int_dtype.id(), sides_num_elems));
    s2dmap["sizes"].set(DataType(int_dtype.id(), topo_num_elems));
    s2dmap["offsets"].set(DataType(int_dtype.id(), topo_num_elems));

    d2smap.reset();
    d2smap["values"].set(DataType(int_dtype.id(), sides_num_elems));
    d2smap["sizes"].set(DataType(int_dtype.id(), sides_num_elems));
    d2smap["offsets"].set(DataType(int_dtype.id(), sides_num_elems));

    Node &dest_conn = topo_dest["elements/connectivity"];
    for(; elem_index < (int64)topo_num_elems; elem_index++)
    {
        std::deque< index_t > elem_embed_stack(1, elem_index);
        std::deque< index_t > elem_edim_stack(1, topo_shape.dim);
        std::deque< std::vector<index_t> > elem_eparent_stack(1);

        int64 s2d_start_index = s2d_val_index;

        while(!elem_embed_stack.empty())
        {
            index_t embed_index = elem_embed_stack.front();
            elem_embed_stack.pop_front();
            index_t embed_dim = elem_edim_stack.front();
            elem_edim_stack.pop_front();
            std::vector<index_t> embed_parents = elem_eparent_stack.front();
            elem_eparent_stack.pop_front();

            // NOTE(JRC): We iterate using local index values so that we
            // get the correct orientations for per-element lines.
            const auto embed_ids = topo_data.get_local_association(
                embed_index, embed_dim, embed_dim - 1);
            if(embed_dim > line_shape.dim)
            {
                embed_parents.push_back(embed_index);
                for(index_t ei = 0; ei < (index_t)embed_ids.size(); ei++)
                {
                    elem_embed_stack.push_back(embed_ids[ei]);
                    elem_edim_stack.push_back(embed_dim - 1);
                    elem_eparent_stack.push_back(embed_parents);
                }
            }
            else // if(embed_dim == line_shape.dim)
            {
                // NOTE(JRC): Side ordering retains original element orientation
                // by creating elements as follows:
                // - 2D: Face-Line Start => Face-Line End => Face Center
                // - 3D: Cell-Face-Line Start => Cell-Face-Line End => Cell-Face Center => Cell Center
                const auto local_to_global_lower = topo_data.get_local_to_global_map(embed_dim - 1);
                const auto num_embed_ids = static_cast<index_t>(embed_ids.size());
                for(index_t ei = 0; ei < num_embed_ids; ei++)
                {
                    index_t point_id = local_to_global_lower[embed_ids[ei]];
                    side_data_raw[ei] = point_id;
                }
                const auto num_embed_parents = static_cast<index_t>(embed_parents.size());
                for(index_t pi = 0; pi < num_embed_parents; pi++)
                {
                    index_t parent_index = embed_parents[num_embed_parents - pi - 1];
                    index_t parent_dim = embed_dim + pi + 1;
                    const auto local_to_global_parent = topo_data.get_local_to_global_map(parent_dim);
                    index_t parent_id = local_to_global_parent[parent_index];
                    side_data_raw[2 + pi] = dim_coord_offsets[parent_dim] + parent_id;
                }

                // TODO: USE ACCESSORS?
                misc_data.set_external(DataType(int_dtype.id(), sides_elem_degree),
                    dest_conn.element_ptr(sides_elem_degree * side_index));
                side_data.to_data_type(int_dtype.id(), misc_data);

                misc_data.set_external(DataType(int_dtype.id(), 1),
                    s2dmap["values"].element_ptr(s2d_val_index++));
                side_index_data.to_data_type(int_dtype.id(), misc_data);

                misc_data.set_external(DataType(int_dtype.id(), 1),
                    d2smap["values"].element_ptr(d2s_val_index++));
                elem_index_data.to_data_type(int_dtype.id(), misc_data);

                int64 side_num_elems = 1;
                raw_data.set(side_num_elems);

                misc_data.set_external(DataType(int_dtype.id(), 1),
                    d2smap["sizes"].element_ptr(d2s_elem_index++));
                raw_data.to_data_type(int_dtype.id(), misc_data);

                side_index++;
            }
        }

        int64 elem_num_sides = s2d_val_index - s2d_start_index;
        raw_data.set(elem_num_sides);
        // TODO: USE ACCESSORS
        misc_data.set_external(DataType(int_dtype.id(), 1),
            s2dmap["sizes"].element_ptr(s2d_elem_index++));
        raw_data.to_data_type(int_dtype.id(), misc_data);
    }

    // TODO(JRC): Implement these counts in-line instead of being lazy and
    // taking care of it at the end of the function w/ a helper.
    Node info;
    blueprint::o2mrelation::generate_offsets(s2dmap, info);
    blueprint::o2mrelation::generate_offsets(d2smap, info);
}

//-----------------------------------------------------------------------------
namespace detail
{
    class vec3
    {
    public:
        float64 x, y, z;
        vec3(float64 i, float64 j, float64 k) : x(i), y(j), z(k) {}

        vec3 operator+(const vec3 &v) const
        {
            return vec3(x + v.x, y + v.y, z + v.z);
        }

        vec3 operator-(const vec3 &v) const
        {
            return vec3(x - v.x, y - v.y, z - v.z);
        }

        float64 dot(const vec3 &v) const
        {
            return x * v.x + y * v.y + z * v.z;
        }

        vec3 cross(const vec3 &v) const
        {
            float64 cx, cy, cz;
            cx = this->y * v.z - this->z * v.y;
            cy = this->z * v.x - this->x * v.z;
            cz = this->x * v.y - this->y * v.x;
            return vec3(cx, cy, cz);
        }
    };

    // given three points in 2D, calculates the area of the triangle formed by those points
    float64 triangle_area(float64 x1, float64 y1,
                          float64 x2, float64 y2,
                          float64 x3, float64 y3)
    {
        return 0.5f * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    }

    float64 tetrahedron_volume(vec3 &a, vec3 &b, vec3 &c, vec3 &d)
    {
        return fabs((a - d).dot((b - d).cross(c - d))) / 6.0f;
    }

    // determines the type of the coordinate values and calls
    // we want access to the new topology so we can calculate the areas
    // of the new triangles/volumes of the new tetrahedra
    void
    volume_dependent(const Node &topo_dest,
                     const Node &coordset_dest,
                     const int dimensions,
                     const int new_num_shapes, // number of new triangles or tetrahedrons
                     const int num_orig_shapes, // number of original polygons or polyhedra
                     int64_accessor tri_to_poly,
                     Node &volumes_info,
                     Node &volumes_field_values)
    {
        // first we calculate the volume of each triangle
        volumes_field_values.set(conduit::DataType::float64(new_num_shapes));
        float64_array tri_volumes = volumes_field_values.value();

        int64_accessor connec = topo_dest["elements/connectivity"].value();
        float64_accessor coords_x = coordset_dest["values/x"].value();
        float64_accessor coords_y = coordset_dest["values/y"].value();

        if (dimensions == 2)
        {
            for (int i = 0; i < new_num_shapes; i ++)
            {
                float64 x1 = static_cast<float64>(coords_x[connec[i * 3 + 0]]);
                float64 y1 = static_cast<float64>(coords_y[connec[i * 3 + 0]]);
                float64 x2 = static_cast<float64>(coords_x[connec[i * 3 + 1]]);
                float64 y2 = static_cast<float64>(coords_y[connec[i * 3 + 1]]);
                float64 x3 = static_cast<float64>(coords_x[connec[i * 3 + 2]]);
                float64 y3 = static_cast<float64>(coords_y[connec[i * 3 + 2]]);

                tri_volumes[i] = triangle_area(x1, y1, x2, y2, x3, y3);
            }
        }
        else if (dimensions == 3)
        {
            float64_accessor coords_z = coordset_dest["values/z"].value();

            for (int i = 0; i < new_num_shapes; i ++)
            {
                vec3 a = vec3(static_cast<float64>(coords_x[connec[i * 4 + 0]]),
                              static_cast<float64>(coords_y[connec[i * 4 + 0]]),
                              static_cast<float64>(coords_z[connec[i * 4 + 0]]));
                vec3 b = vec3(static_cast<float64>(coords_x[connec[i * 4 + 1]]),
                              static_cast<float64>(coords_y[connec[i * 4 + 1]]),
                              static_cast<float64>(coords_z[connec[i * 4 + 1]]));
                vec3 c = vec3(static_cast<float64>(coords_x[connec[i * 4 + 2]]),
                              static_cast<float64>(coords_y[connec[i * 4 + 2]]),
                              static_cast<float64>(coords_z[connec[i * 4 + 2]]));
                vec3 d = vec3(static_cast<float64>(coords_x[connec[i * 4 + 3]]),
                              static_cast<float64>(coords_y[connec[i * 4 + 3]]),
                              static_cast<float64>(coords_z[connec[i * 4 + 3]]));
                
                tri_volumes[i] = tetrahedron_volume(a,b,c,d);
            }
        }
        else
        {
            CONDUIT_ERROR("Higher dimensions are not supported.");
        }

        // next we calculate the volume of each polygon
        volumes_info["poly"].set(conduit::DataType::float64(num_orig_shapes));
        float64_array poly_volumes = volumes_info["poly"].value();

        for (int i = 0; i < num_orig_shapes; i ++)
        {
            poly_volumes[i] = 0;
        }
        for (int i = 0; i < new_num_shapes; i ++)
        {
            poly_volumes[tri_to_poly[i]] += tri_volumes[i];
        }

        // finally we calculate the volume ratio
        volumes_info["ratio"].set(conduit::DataType::float64(new_num_shapes));
        float64_array ratio = volumes_info["ratio"].value();

        for (int i = 0; i < new_num_shapes; i ++)
        {
            ratio[i] = tri_volumes[i] / poly_volumes[tri_to_poly[i]];
        }
    }

    void
    vertex_associated_field(const Node &topo_dest,
                            float64_accessor poly_field_data,
                            int orig_num_points,
                            int new_num_points,
                            int dimensions,
                            float64_accessor field_values)
    {
        // copy field values from the original field over to the
        // points that are in both the old and new topologies
        for (int i = 0; i < orig_num_points; i ++)
        {
            field_values.set(i, poly_field_data[i]);
        }

        // this map will record for each new point (represented by
        // an integer that indexes into the points array) the list
        // of other points that it is connected to (a set of integers)
        std::map<int, std::set<int>> info;

        int iter = dimensions == 2 ? 3 : 4;
        int64_accessor new_connec = topo_dest["elements/connectivity"].value();
        int length_of_connec = topo_dest["elements/connectivity"].dtype().number_of_elements();

        // iterate thru the connectivity array, going in groups of 3 or 4,
        // depending on the dimension
        for (int i = 0; i < length_of_connec; i += iter)
        {
            // iterate through the points in the current shape
            for (int j = i; j < i + iter; j ++)
            {
                // if we run into a new point
                if (new_connec[j] >= orig_num_points)
                {
                    // then we iterate through the same set of points again,
                    // recording the points it is connected to
                    for (int k = i; k < i + iter; k ++)
                    {
                        // make sure we do not mark down that our point is
                        // connected to itself
                        if (k != j)
                        {
                            // then add or modify an entry in the map to reflect
                            // the new information
                            auto ikey = static_cast<int>(new_connec[j]);
                            auto ivalue = static_cast<int>(new_connec[k]);
                            info[ikey].insert(ivalue);
                        }
                    }
                }
            }
        }

        // now we iterate through the new points
        for (int i = orig_num_points; i < new_num_points; i ++)
        {
            // if they have an entry in the map (i.e. they are connected
            // to another point)
            if (info.find(i) != info.end())
            {
                float64 sum = 0.0;
                float64 num_neighbors = 0.0;
                std::set<int>::iterator it;
                // we iterate through the set and sum the field values
                // of the points we are connected to that are also
                // original points
                for (it = info[i].begin(); it != info[i].end(); it ++)
                {
                    if (*it < orig_num_points)
                    {
                        sum += field_values[*it];
                        num_neighbors += 1.0;
                    }
                }
                // then we divide by the number of incident points,
                // giving us an average. We do not want to divide by
                // the size of the set, since there are neighbors which
                // may go unused, since they are not from the original
                // coordset
                field_values.set(i, sum / num_neighbors);
            }
            // if the points go unused in the topology, we assign them 0
            else
            {
                field_values.set(i, 0.0);
            }
        }
    }

    void
    map_field_to_generated_sides(Node &field_out,
                                 const Node &field_src,
                                 int new_num_shapes,
                                 int64_accessor tri_to_poly,
                                 float64_accessor volume_ratio,
                                 bool vol_dep,
                                 bool vert_assoc,
                                 int orig_num_points,
                                 int new_num_points,
                                 int dimensions,
                                 const Node &topo_dest)
    {
        // a pointer to the destination for field values
        float64_accessor field_values = field_out["values"].value();

        // a pointer to the original field values
        float64_accessor poly_field_data = field_src["values"].value();

        // if our field is vertex associated
        if (vert_assoc)
        {
            vertex_associated_field(topo_dest,
                                    poly_field_data,
                                    orig_num_points,
                                    new_num_points,
                                    dimensions,
                                    field_values);
        }
        else
        {
            for (int i = 0; i < new_num_shapes; i ++)
            {
                // tri_to_poly[i] is the index of the original polygon
                // that triangle 'i' is associated with.
                // If we use that to index into poly_field_data we
                // get the field value of the original polygon,
                // which we then assign to the destination field values.

                // if our field is volume dependent
                if (vol_dep)
                {
                    field_values.set(i, poly_field_data[tri_to_poly[i]] * volume_ratio[i]);
                }
                else
                {
                    field_values.set(i, poly_field_data[tri_to_poly[i]]);
                }
            }
        }
    }

    void
    map_fields_to_generated_sides(const Node &topo_src,
                                  const Node &coordset_src,
                                  const Node &fields_src,
                                  const Node &d2smap,
                                  const Node &topo_dest,
                                  const Node &coordset_dest,
                                  Node &fields_dest,
                                  const std::vector<std::string> &field_names,
                                  const std::string &field_prefix)
    {
        NodeConstIterator fields_itr = fields_src.children(); // to iterate through the fields
        std::string topo_name = topo_src.name(); // the name of the topology we are working with
        bool no_field_names = field_names.empty(); // true if the user has specified no fields to be copied, meaning all should be copied
        bool vol_dep = false; // true if the current field is volume dependent
        bool vert_assoc = false; // true if the current field is vertex associated
        int dimensions = 0; // are we in 2D or 3D?
        int new_num_shapes = 0; // the number of new triangles or tetrahedrons
        int num_orig_shapes = topo_src["elements/sizes"].dtype().number_of_elements(); // the number of original polygons or polyhedra
        Node volumes_info; // a container for the volumes of old shapes and the ratio between new and old volumes for each new shape
        bool volumes_calculated = false; // so we only calculate the volumes once as we go through the while loop
        float64_accessor volume_ratio; // a float64 accessor to the ratio between new and old volumes for each new shape

        if (topo_dest["elements/shape"].as_string() == "tet")
        {
            new_num_shapes = topo_dest["elements/connectivity"].dtype().number_of_elements() / 4;
            dimensions = 3;
        }
        else if (topo_dest["elements/shape"].as_string() == "tri")
        {
            new_num_shapes = topo_dest["elements/connectivity"].dtype().number_of_elements() / 3;
            dimensions = 2;
        }
        else
        {
            CONDUIT_ERROR(((std::string) "Bad shape in ").append(topo_dest["elements/shape"].as_string()));
        }

        int64_accessor tri_to_poly = d2smap["values"].value();

        // set up original elements id field
        Node &original_elements = fields_dest[field_prefix + "original_element_ids"];
        original_elements["topology"] = topo_name;
        original_elements["association"] = "element";
        original_elements["volume_dependent"] = "false";
        d2smap["values"].to_int32_array(original_elements["values"]);

        // set up original vertex id field
        // we assume that new points are added to the end of the list of points
        Node &original_vertices = fields_dest[field_prefix + "original_vertex_ids"];
        original_vertices["topology"] = topo_name;
        original_vertices["association"] = "vertex";
        original_vertices["volume_dependent"] = "false";
        index_t orig_num_points = coordset_src["values/x"].dtype().number_of_elements();
        index_t new_num_points = coordset_dest["values/x"].dtype().number_of_elements();
        original_vertices["values"].set(conduit::DataType::int32(new_num_points));
        int32_array orig_vert_ids = original_vertices["values"].value();
        for (index_t i = 0; i < new_num_points; i ++)
        {
            if (i < orig_num_points)
            {
                orig_vert_ids[i] = static_cast<int32>(i);
            }
            else
            {
                orig_vert_ids[i] = static_cast<int32>(-1);
            }
        }

        while(fields_itr.has_next())
        {
            const Node &field = fields_itr.next();
            std::string field_name = fields_itr.name();

            // check that the field is one of the selected fields specified in the options node
            bool found = false;
            if (no_field_names)
            {
                // we want to copy all fields if no field names were provided
                found = true;
            }
            else
            {
                for (size_t i = 0; i < field_names.size(); i ++)
                {
                    if (field_names[i] == field_name)
                    {
                        found = true;
                        break;
                    }
                }
            }

            // check that the current field uses the chosen topology
            if (found && field["topology"].as_string() == topo_name)
            {
                Node &field_out = fields_dest[field_prefix + field_name];

                if (field.has_child("association"))
                {
                    if (field["association"].as_string() != "element")
                    {
                        if (field["association"].as_string() == "vertex")
                        {
                            vert_assoc = true;
                        }
                        else
                        {
                            CONDUIT_ERROR("Unsupported association option in " + field["association"].as_string() + ".");
                        }
                    }
                }

                if (field.has_child("volume_dependent"))
                {
                    if (field["volume_dependent"].as_string() == "true")
                    {
                        vol_dep = true;
                        if (vert_assoc)
                        {
                            CONDUIT_ERROR("Volume-dependent vertex-associated fields are not supported.");
                        }

                    }
                }

                // copy all information from the old field except for the values
                NodeConstIterator itr = field.children();
                while (itr.has_next())
                {
                    const Node &cld = itr.next();
                    std::string cld_name = itr.name();

                    if (cld_name != "values")
                    {
                        field_out[cld_name] = cld;
                    }
                }

                // handle volume dependent fields
                // if the field is volume dependent and we have not already calculated the volumes
                if (vol_dep && !volumes_calculated)
                {
                    volumes_calculated = true;

                    // make volume into a field
                    Node &volumes_field = fields_dest[field_prefix + "volume"];
                    volumes_field["topology"] = topo_name;
                    volumes_field["association"] = "element";
                    volumes_field["volume_dependent"] = "true";

                    // get the volumes and ratio
                    volume_dependent(topo_dest,
                                     coordset_dest,
                                     dimensions,
                                     new_num_shapes,
                                     num_orig_shapes,
                                     tri_to_poly,
                                     volumes_info,
                                     volumes_field["values"]);
                    volume_ratio = volumes_info["ratio"].value();
                }

                const auto field_out_size = vert_assoc ? new_num_points : new_num_shapes;

                // If we are volume dependent or vertex associated, we do not care what 
                // the original type of the fields was. We are going to make brand new 
                // fields that have type float64.
                if (vol_dep || vert_assoc)
                {
                    field_out["values"].set(conduit::DataType::float64(field_out_size));
                }
                // If we are neither volume dependent nor vertex associated, we can go
                // ahead and honor the original field type.
                else
                {
                    if (field["values"].dtype().is_uint64())
                    {
                        field_out["values"].set(conduit::DataType::uint64(field_out_size));
                    }
                    else if (field["values"].dtype().is_uint32())
                    {
                        field_out["values"].set(conduit::DataType::uint32(field_out_size));
                    }
                    else if (field["values"].dtype().is_int64())
                    {
                        field_out["values"].set(conduit::DataType::int64(field_out_size));
                    }
                    else if (field["values"].dtype().is_int32())
                    {
                        field_out["values"].set(conduit::DataType::int32(field_out_size));
                    }
                    else if (field["values"].dtype().is_float64())
                    {
                        field_out["values"].set(conduit::DataType::float64(field_out_size));
                    }
                    else if (field["values"].dtype().is_float32())
                    {
                        field_out["values"].set(conduit::DataType::float32(field_out_size));
                    }
                    else
                    {
                        CONDUIT_ERROR("Unsupported field type in " << field["values"].dtype().to_yaml());
                    }
                }

                map_field_to_generated_sides(field_out,
                                             field,
                                             new_num_shapes,
                                             tri_to_poly,
                                             volume_ratio,
                                             vol_dep,
                                             vert_assoc,
                                             orig_num_points,
                                             new_num_points,
                                             dimensions,
                                             topo_dest);

                if (vol_dep)
                {
                    vol_dep = false;
                }
                if (vert_assoc)
                {
                    vert_assoc = false;
                }
            }
            else
            {
                // if we couldn't find the field in the specified field_names, then we don't care;
                // but if it was found, and we are here, then that means that the field we want
                // uses the wrong topology
                if (! no_field_names && found)
                {
                    CONDUIT_ERROR("field " + field_name + " does not use " + topo_name + ".");
                }
            }
        }
    }
} // end namespace detail

void
mesh::topology::unstructured::generate_sides(const conduit::Node &topo_src,
                                             conduit::Node &topo_dest,
                                             conduit::Node &coordset_dest,
                                             conduit::Node &fields_dest,
                                             conduit::Node &s2dmap,
                                             conduit::Node &d2smap,
                                             const conduit::Node &options)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if(topo_src.has_path("type") && topo_src["type"].as_string() != "unstructured")
    {
        CONDUIT_ERROR("The topology was not unstructured.");
    }

    std::string field_prefix = "";
    std::vector<std::string> field_names;

    const Node &coordset_src = (*(topo_src.parent()->parent()))["coordsets/" + topo_src["coordset"].as_string()];

    // generate sides as usual
    generate_sides(topo_src, topo_dest, coordset_dest, s2dmap, d2smap);

    if (topo_src.parent() &&
        topo_src.parent()->parent() &&
        (*(topo_src.parent()->parent())).has_child("fields"))
    {
        const Node &fields_src = (*(topo_src.parent()->parent()))["fields"];

        // check for existence of field prefix
        if (options.has_child("field_prefix"))
        {
            if (options["field_prefix"].dtype().is_string())
            {
                field_prefix = options["field_prefix"].as_string();
            }
            else
            {
                CONDUIT_ERROR("field_prefix must be a string.");
            }
        }

        // check for target field names
        if (options.has_child("field_names"))
        {
            if (options["field_names"].dtype().is_string())
            {
                field_names.push_back(options["field_names"].as_string());
            }
            else if (options["field_names"].dtype().is_list())
            {
                NodeConstIterator itr = options["field_names"].children();
                while (itr.has_next())
                {
                    const Node &cld = itr.next();
                    if (cld.dtype().is_string())
                    {
                        field_names.push_back(cld.as_string());
                    }
                    else
                    {
                        CONDUIT_ERROR("field_names must be a string or a list of strings.");
                    }
                }
            }
            else
            {
                CONDUIT_ERROR("field_names must be a string or a list of strings.");
            }
        }

        // check that the discovered field names exist in the target fields
        for (uint64 i = 0; i < field_names.size(); i ++)
        {
            if (! fields_src.has_child(field_names[i]))
            {
                CONDUIT_ERROR("field " + field_names[i] + " not found in target.");
            }
        }

        // now map fields
        detail::map_fields_to_generated_sides(topo_src,
                                              coordset_src,
                                              fields_src,
                                              d2smap,
                                              topo_dest,
                                              coordset_dest,
                                              fields_dest,
                                              field_names,
                                              field_prefix);
    }
}

// this variant of the function same as generate sides and map fields
// with empty options
//----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_sides(const conduit::Node &topo,
                                             conduit::Node &topo_dest,
                                             conduit::Node &coords_dest,
                                             conduit::Node &fields_dest,
                                             conduit::Node &s2dmap,
                                             conduit::Node &d2smap)
{
    Node opts;
    mesh::topology::unstructured::generate_sides(topo,
                                                 topo_dest,
                                                 coords_dest,
                                                 fields_dest,
                                                 s2dmap,
                                                 d2smap,
                                                 opts);
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_corners(const Node &topo,
                                               Node &topo_dest,
                                               Node &coords_dest,
                                               Node &s2dmap,
                                               Node &d2smap)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if(topo.has_path("type") && topo["type"].as_string() != "unstructured")
    {
        CONDUIT_ERROR("The topology was not unstructured.");
    }

    // Retrieve Relevent Coordinate/Topology Metadata //

    const Node *coordset = bputils::find_reference_node(topo, "coordset");
    const std::vector<std::string> csys_axes = bputils::coordset::axes(*coordset);

    const ShapeCascade topo_cascade(topo);
    const ShapeType topo_shape = topo_cascade.get_shape();
    const bool is_topo_3d = topo_shape.dim == 3;
    const ShapeType point_shape = topo_cascade.get_shape(0);
    const ShapeType line_shape = topo_cascade.get_shape(1);
    const ShapeType face_shape = topo_cascade.get_shape(2);
    const ShapeType corner_shape(topo_shape.dim == 3 ? "polyhedral" : "polygonal");
    if(topo_shape.dim < 2)
    {
        CONDUIT_ERROR("Failed to generate corner mesh for input; " <<
            "input tology must be topologically 2D or 3D.");
    }

    // Extract Derived Coordinate/Topology Data //

    TopologyMetadata topo_data(topo, *coordset);
    const index_t topo_num_elems = topo_data.get_length(topo_shape.dim);
    const DataType &int_dtype = topo_data.get_int_dtype();
    const DataType &float_dtype = topo_data.get_float_dtype();

    std::vector<conduit::Node> dim_cent_topos(topo_shape.dim + 1);
    std::vector<conduit::Node> dim_cent_coords(topo_shape.dim + 1);
    for(index_t di = 0; di <= topo_shape.dim; di++)
    {
        calculate_unstructured_centroids(
            topo_data.get_topology(di), *coordset,
            dim_cent_topos[di], dim_cent_coords[di]);
    }

    // Allocate Data Templates for Outputs //

    const index_t corners_num_coords = topo_data.get_length();
    const index_t corners_face_degree = 4;

    topo_dest.reset();
    topo_dest["type"].set("unstructured");
    topo_dest["coordset"].set(coords_dest.name());
    topo_dest["elements/shape"].set(corner_shape.type());
    if (is_topo_3d)
    {
        topo_dest["subelements/shape"].set("polygonal");
    }
    // TODO(JRC): I wasn't able to find a good way to compute the connectivity
    // length a priori because of the possibility of polygonal 3D inputs, but
    // having this information would improve the performance of the method.
    // dest["elements/connectivity"].set(DataType(int_dtype.id(), ???);

    coords_dest.reset();
    coords_dest["type"].set("explicit");
    for(index_t ai = 0; ai < (index_t)csys_axes.size(); ai++)
    {
        coords_dest["values"][csys_axes[ai]].set(DataType(float_dtype.id(),
            corners_num_coords));
    }

    s2dmap.reset();
    d2smap.reset();

    // Populate Data Arrays w/ Calculated Coordinates //

    std::vector<index_t> dim_coord_offsets(topo_shape.dim + 1);
    for(index_t ai = 0; ai < (index_t)csys_axes.size(); ai++)
    {
        Node dst_data;
        Node &dst_axis = coords_dest["values"][csys_axes[ai]];

        // TODO(JRC): This is how centroids offsets are generated in the
        // final topology!
        for(index_t di = 0, doffset = 0; di <= topo_shape.dim; di++)
        {
            dim_coord_offsets[di] = doffset;

            // NOTE: The centroid ordering for the positions is different
            // from the base ordering, which messes up all subsequent indexing.
            // We must use the coordinate set associated with the base topology.
            const Node &cset = (di != 0) ? dim_cent_coords[di] : *coordset;
            const Node &cset_axis = cset["values"][csys_axes[ai]];
            index_t cset_length = cset_axis.dtype().number_of_elements();
            // TODO: USE ACCESSORS?
            dst_data.set_external(DataType(float_dtype.id(), cset_length),
                dst_axis.element_ptr(doffset));
            cset_axis.to_data_type(float_dtype.id(), dst_data);
            doffset += cset_length;
        }
    }

    // Compute New Elements/Fields for Corner Topology //

    std::vector<int64> conn_data_raw, size_data_raw;
    std::vector<int64> subconn_data_raw, subsize_data_raw;
    std::vector<int64> s2d_idx_data_raw, s2d_size_data_raw;
    std::vector<int64> d2s_idx_data_raw, d2s_size_data_raw;
    std::map< std::set<index_t>, index_t > subconn_topo_set;

    const auto face_local_to_global_map = topo_data.get_local_to_global_map(face_shape.dim);
    const auto line_local_to_global_map = topo_data.get_local_to_global_map(line_shape.dim);
    const auto point_local_to_global_map = topo_data.get_local_to_global_map(point_shape.dim);

    for(index_t elem_index = 0, corner_index = 0; elem_index < topo_num_elems; elem_index++)
    {
        // per-face, per-line orientations for this element, i.e. {(f_gi, l_gj) => (v_gk, v_gl)}
        std::map< std::pair<index_t, index_t>, std::pair<index_t, index_t> > elem_orient;
        { // establish the element's internal line constraints
            const auto elem_faces = topo_data.get_local_association(
                elem_index, topo_shape.dim, face_shape.dim);
            for(index_t fi = 0; fi < (index_t)elem_faces.size(); fi++)
            {
                const index_t face_lid = elem_faces[fi];
                const index_t face_gid = face_local_to_global_map[face_lid];

                const auto face_lines = topo_data.get_local_association(
                    face_lid, face_shape.dim, line_shape.dim);
                for(index_t li = 0; li < (index_t)face_lines.size(); li++)
                {
                    const index_t line_lid = face_lines[li];
                    const index_t line_gid = line_local_to_global_map[line_lid];

                    const auto line_points = topo_data.get_local_association(
                        line_lid, line_shape.dim, point_shape.dim);
                    const index_t start_gid = point_local_to_global_map[line_points[0]];
                    const index_t end_gid = point_local_to_global_map[line_points[1]];

                    elem_orient[std::make_pair(face_gid, line_gid)] =
                        std::make_pair(start_gid, end_gid);
                }
            }
        }

        const auto elem_lines = topo_data.get_global_association(
            elem_index, topo_shape.dim, line_shape.dim);
        const auto elem_faces = topo_data.get_global_association(
            elem_index, topo_shape.dim, face_shape.dim);

        // NOTE(JRC): Corner ordering retains original element orientation
        // by creating elements as follows:
        //
        // - for a given element, determine how its co-faces and co-lines are
        //   oriented, and set these as constraints
        // - based on these constraints, create the co-line/co-face centroid
        //   corner lines, which add a new set of contraints
        // - finally, if the topology is 3D, create the co-face/cell centroid
        //   corner lines based on all previous constraints, and then collect
        //   these final lines into corner faces
        //
        // To better demonstrate this algorithm, here's a simple 2D example:
        //
        // - Top-Level Element/Constraints (See Arrows)
        //
        //   p2      l2      p3
        //   +<---------------+
        //   |                ^
        //   |                |
        //   |                |
        // l3|       f0       |l1
        //   |                |
        //   |                |
        //   v                |
        //   +--------------->+
        //   p0      l0      p1
        //
        // - Consider Corner f0/p0 and Centroids; Impose Top-Level Constraints
        //
        //   p2      l2      p3
        //   +----------------+
        //   |                |
        //   |                |
        //   |       f0       |
        // l3+       +        |l1
        //   |                |
        //   |                |
        //   v                |
        //   +------>+--------+
        //   p0      l0      p1
        //
        // - Create Face/Line Connections Based on Top-Level Constraints
        //
        //   p2      l2      p3
        //   +----------------+
        //   |                |
        //   |                |
        //   |       f0       |
        // l3+<------+        |l1
        //   |       ^        |
        //   |       |        |
        //   v       |        |
        //   +------>+--------+
        //   p0      l0      p1
        //

        // per-elem, per-point corners, informed by cell-face-line orientation constraints
        const auto elem_points = topo_data.get_global_association(
            elem_index, topo_shape.dim, point_shape.dim);
        for(index_t pi = 0; pi < (index_t)elem_points.size(); pi++, corner_index++)
        {
            const index_t point_index = elem_points[pi];

            const auto point_faces = topo_data.get_global_association(
                point_index, point_shape.dim, face_shape.dim);
            const auto point_lines = topo_data.get_global_association(
                point_index, point_shape.dim, line_shape.dim);
            const std::vector<index_t> elem_point_faces = intersect_sets(
                elem_faces, point_faces);
            const std::vector<index_t> elem_point_lines = intersect_sets(
                elem_lines, point_lines);

            // per-corner face vertex orderings, informed by 'corner_orient'
            std::vector< std::vector<index_t> > corner_faces(
                // # of faces per corner: len(v.faces & c.faces) * (2 if is_3d else 1)
                elem_point_faces.size() * (is_topo_3d ? 2 : 1),
                // # of vertices per face: 4 (all faces are quads in corner topology)
                std::vector<index_t>(corners_face_degree, 0));
            // per-face, per-line orientations for this corner, i.e. {(f_gi, l_gj) => bool}
            std::map< std::pair<index_t, index_t>, bool > corner_orient;
            // flags for the 'corner_orient' map; if TO_FACE, line is (l_gj, f_gi);
            // if FROM_FACE, line is (f_gi, l_gj)
            const static bool TO_FACE = true, FROM_FACE = false;

            // generate oriented corner-to-face faces using internal line constraints
            for(index_t fi = 0; fi < (index_t)elem_point_faces.size(); fi++)
            {
                const index_t face_index = elem_point_faces[fi];

                const auto elem_face_lines = topo_data.get_global_association(
                    face_index, face_shape.dim, line_shape.dim);
                const std::vector<index_t> corner_face_lines = intersect_sets(
                    elem_face_lines, point_lines);

                std::vector<index_t> &corner_face = corner_faces[fi];
                {
                    corner_face[0] = point_index;
                    corner_face[2] = face_index;

                    const index_t first_line_index = corner_face_lines.front();
                    const index_t second_line_index = corner_face_lines.back();
                    const auto first_line_pair = std::make_pair(face_index, first_line_index);
                    const auto second_line_pair = std::make_pair(face_index, second_line_index);

                    const bool is_first_forward = elem_orient[first_line_pair].first == point_index;
                    corner_face[1] = is_first_forward ? first_line_index : second_line_index;
                    corner_face[3] = is_first_forward ? second_line_index : first_line_index;
                    corner_orient[first_line_pair] = is_first_forward ? TO_FACE : FROM_FACE;
                    corner_orient[second_line_pair] = is_first_forward ? FROM_FACE : TO_FACE;

                    // NOTE(JRC): The non-corner points are centroids and thus
                    // need to be offset relative to their dimensional position.
                    corner_face[0] += dim_coord_offsets[point_shape.dim];
                    corner_face[1] += dim_coord_offsets[line_shape.dim];
                    corner_face[3] += dim_coord_offsets[line_shape.dim];
                    corner_face[2] += dim_coord_offsets[face_shape.dim];
                }
            }
            // generate oriented line-to-cell faces using corner-to-face constraints from above
            for(index_t li = 0; li < (index_t)elem_point_lines.size() && is_topo_3d; li++)
            {
                const index_t line_index = elem_point_lines[li];

                const auto line_faces = topo_data.get_global_association(
                    line_index, line_shape.dim, face_shape.dim);
                const std::vector<index_t> corner_line_faces = intersect_sets(
                    elem_faces, line_faces);

                std::vector<index_t> &corner_face = corner_faces[elem_point_faces.size() + li];
                {
                    corner_face[0] = line_index;
                    corner_face[2] = elem_index;

                    const index_t first_face_index = corner_line_faces.front();
                    const index_t second_face_index = corner_line_faces.back();
                    const auto first_face_pair = std::make_pair(first_face_index, line_index);
                    // const auto second_face_pair = std::make_pair(second_face_index, line_index);

                    // NOTE(JRC): The current corner face will use the co-edge of the existing
                    // edge in 'corner_orient', so we flip the orientation for the local use.
                    const bool is_first_forward = !corner_orient[first_face_pair];
                    corner_face[1] = is_first_forward ? first_face_index : second_face_index;
                    corner_face[3] = is_first_forward ? second_face_index : first_face_index;

                    // NOTE(JRC): The non-corner points are centroids and thus
                    // need to be offset relative to their dimensional position.
                    corner_face[0] += dim_coord_offsets[line_shape.dim];
                    corner_face[1] += dim_coord_offsets[face_shape.dim];
                    corner_face[3] += dim_coord_offsets[face_shape.dim];
                    corner_face[2] += dim_coord_offsets[topo_shape.dim];
                }
            }

            if(!is_topo_3d)
            {
                const std::vector<index_t> &corner_face = corner_faces.front();
                size_data_raw.push_back(corner_face.size());
                conn_data_raw.insert(conn_data_raw.end(),
                    corner_face.begin(), corner_face.end());
            }
            else // if(is_topo_3d)
            {
                size_data_raw.push_back(corner_faces.size());
                for(index_t fi = 0; fi < (index_t)corner_faces.size(); fi++)
                {
                    const std::vector<index_t> &corner_face = corner_faces[fi];
                    // TODO(JRC): For now, we retain the behavior of storing only
                    // unique faces in the subconnectivity for 3D corners, but
                    // this can be easily changed by modifying the logic below.
                    const std::set<index_t> corner_face_set(corner_face.begin(), corner_face.end());
                    auto sts_it = subconn_topo_set.find(corner_face_set);
                    index_t face_index;
                    if(sts_it == subconn_topo_set.end())
                    {
                        const index_t next_face_index = subconn_topo_set.size();
                        face_index = next_face_index;
                        subconn_topo_set[corner_face_set] = next_face_index;
                        subsize_data_raw.push_back(corner_face_set.size());
                        subconn_data_raw.insert(subconn_data_raw.end(),
                            corner_face.begin(), corner_face.end());
                    }
                    else
                    {
                        face_index = sts_it->second;
                    }

                    conn_data_raw.push_back(face_index);
                }
            }

            s2d_idx_data_raw.push_back(corner_index);
            d2s_size_data_raw.push_back(1);
            d2s_idx_data_raw.push_back(elem_index);
        }

        s2d_size_data_raw.push_back(elem_points.size());
    }

    Node raw_data, info;
    {
        // TODO: USE ACCESSORS?
        raw_data.set_external(
            DataType::int64(conn_data_raw.size()),
            conn_data_raw.data());
        raw_data.to_data_type(int_dtype.id(),
            topo_dest["elements/connectivity"]);
        raw_data.set_external(
            DataType::int64(size_data_raw.size()),
            size_data_raw.data());
        raw_data.to_data_type(int_dtype.id(),
            topo_dest["elements/sizes"]);

        if (is_topo_3d)
        {
            raw_data.set_external(
                DataType::int64(subconn_data_raw.size()),
                subconn_data_raw.data());
            raw_data.to_data_type(int_dtype.id(),
                topo_dest["subelements/connectivity"]);
            raw_data.set_external(
                DataType::int64(subsize_data_raw.size()),
                subsize_data_raw.data());
            raw_data.to_data_type(int_dtype.id(),
                topo_dest["subelements/sizes"]);
        }

        raw_data.set_external(
            DataType::int64(s2d_idx_data_raw.size()),
            s2d_idx_data_raw.data());
        raw_data.to_data_type(int_dtype.id(), s2dmap["values"]);
        raw_data.set_external(
            DataType::int64(s2d_size_data_raw.size()),
            s2d_size_data_raw.data());
        raw_data.to_data_type(int_dtype.id(), s2dmap["sizes"]);

        raw_data.set_external(
            DataType::int64(d2s_idx_data_raw.size()),
            d2s_idx_data_raw.data());
        raw_data.to_data_type(int_dtype.id(), d2smap["values"]);
        raw_data.set_external(
            DataType::int64(d2s_size_data_raw.size()),
            d2s_size_data_raw.data());
        raw_data.to_data_type(int_dtype.id(), d2smap["sizes"]);

        // TODO(JRC): Implement these counts in-line instead of being lazy and
        // taking care of it at the end of the function w/ a helper.
        generate_offsets_inline(topo_dest);
        
        blueprint::o2mrelation::generate_offsets(s2dmap, info);
        blueprint::o2mrelation::generate_offsets(d2smap, info);
    }
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_offsets(const Node &topo,
                                               Node &dest)
{
    return bputils::topology::unstructured::generate_offsets(topo, dest);
}

//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_offsets(const Node &topo,
                                               Node &dest_eleoffsets,
                                               Node &dest_subeleoffsets)
{
    return bputils::topology::unstructured::generate_offsets(topo,
                                                             dest_eleoffsets,
                                                             dest_subeleoffsets);
}


//-----------------------------------------------------------------------------
void
mesh::topology::unstructured::generate_offsets_inline(Node &topo)
{
    return bputils::topology::unstructured::generate_offsets_inline(topo);
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::index protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::index::verify(const Node &topo_idx,
                              Node &info)
{
    const std::string protocol = "mesh::topology::index";
    bool res = true;
    info.reset();

    res &= verify_field_exists(protocol, topo_idx, info, "type") &&
           mesh::topology::type::verify(topo_idx["type"], info["type"]);
    res &= verify_string_field(protocol, topo_idx, info, "coordset");
    res &= verify_string_field(protocol, topo_idx, info, "path");

    if (topo_idx.has_child("grid_function"))
    {
        log::optional(info, protocol, "includes grid_function");
        res &= verify_string_field(protocol, topo_idx, info, "grid_function");
    }

    log::validation(info,res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::type protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::type::verify(const Node &type,
                             Node &info)
{
    const std::string protocol = "mesh::topology::type";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, type, info, "", bputils::TOPO_TYPES);

    log::validation(info,res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::shape protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::shape::verify(const Node &shape,
                              Node &info)
{
    const std::string protocol = "mesh::topology::shape";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, shape, info, "", bputils::TOPO_SHAPES);

    log::validation(info,res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::shape_map protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::topology::shape_map::verify(const Node& shape_map,
  Node& info)
{
  const std::string protocol = "mesh::topology::shape_map";
  bool res = true;
  info.reset();

  res &= verify_object_field(protocol, shape_map, info);

  for (const Node &child : shape_map.children())
  {
    bool isEnum = false;
    for(const auto &enum_value : bputils::TOPO_SHAPES)
    {
      isEnum |= (enum_value == child.name());
    }
    if (isEnum)
    {
      log::info(info, protocol, "has valid value "+ log::quote(child.name()) +
      "(" + std::to_string(child.to_int32()) + ")");
    }
    else
    {
      log::error(info, protocol, "has invalid value " + log::quote(child.name()) +
        "(" + std::to_string(child.to_int32()) + ")");
    }
    res &= isEnum;
  }

  log::validation(info, res);

  return res;
}


//-----------------------------------------------------------------------------
// blueprint::mesh::matset protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// helper to verify a matset material_map
//-----------------------------------------------------------------------------
bool verify_matset_material_map(const std::string &protocol,
                                const conduit::Node &matset,
                                conduit::Node &info)
{
    bool res = verify_object_field(protocol, matset, info, "material_map");

    if(res)
    {
        // we already know we have an object, children should be
        // integer scalars
        NodeConstIterator itr = matset["material_map"].children();
        while(itr.has_next())
        {
            const Node &curr_child = itr.next();
            if(!curr_child.dtype().is_integer())
            {
                log::error(info,
                           protocol,
                           log::quote("material_map") +
                           "child " +
                           log::quote(itr.name()) +
                           " is not an integer leaf.");
                res = false;
            }
        }
    }

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
bool
mesh::matset::verify(const Node &matset,
                     Node &info)
{
    const std::string protocol = "mesh::matset";
    bool res = true, vfs_res = true;
    bool mat_map_is_optional = true;
    info.reset();

    res &= verify_string_field(protocol, matset, info, "topology");
    res &= vfs_res &= verify_field_exists(protocol, matset, info, "volume_fractions");

    if(vfs_res)
    {
        if(!matset["volume_fractions"].dtype().is_number() &&
            !matset["volume_fractions"].dtype().is_object())
        {
            log::error(info, protocol, "'volume_fractions' isn't the correct type");
            res &= vfs_res &= false;
        }
        else if(matset["volume_fractions"].dtype().is_number() &&
            verify_number_field(protocol, matset, info, "volume_fractions"))
        {
            log::info(info, protocol, "detected uni-buffer matset");
            // materials_map is not optional in this case, signal
            // for opt check down the line
            mat_map_is_optional = false;

            vfs_res &= verify_integer_field(protocol, matset, info, "material_ids");
            vfs_res &= blueprint::o2mrelation::verify(matset, info);

            res &= vfs_res;
        }
        else if(matset["volume_fractions"].dtype().is_object() &&
            verify_object_field(protocol, matset, info, "volume_fractions"))
        {
            log::info(info, protocol, "detected multi-buffer matset");

            const Node &vfs = matset["volume_fractions"];
            Node &vfs_info = info["volume_fractions"];

            NodeConstIterator mat_it = vfs.children();
            while(mat_it.has_next())
            {
                const Node &mat = mat_it.next();
                const std::string &mat_name = mat_it.name();

                if(mat.dtype().is_object())
                {
                    vfs_res &= verify_o2mrelation_field(protocol, vfs, vfs_info, mat_name);
                }
                else
                {
                    vfs_res &= verify_number_field(protocol, vfs, vfs_info, mat_name);
                }
            }

            res &= vfs_res;
            log::validation(vfs_info, vfs_res);
        }
    }

    if(!mat_map_is_optional && !matset.has_child("material_map"))
    {
        log::error(info, protocol,
            "'material_map' is missing (required for uni-buffer matsets) ");
        res &= false;
    }

    if(matset.has_child("material_map"))
    {
        if(mat_map_is_optional)
        {
            log::optional(info, protocol, "includes material_map");
        }

        res &= verify_matset_material_map(protocol,matset,info);

        // for cases where vfs are an object, we expect the material_map child
        // names to be a subset of the volume_fractions child names
        if(matset.has_child("volume_fractions") &&
           matset["volume_fractions"].dtype().is_object())
        {
            NodeConstIterator itr =  matset["material_map"].children();
            while(itr.has_next())
            {
                itr.next();
                std::string curr_name = itr.name();
                if(!matset["volume_fractions"].has_child(curr_name))
                {
                    std::ostringstream oss;
                    oss << "'material_map' hierarchy must be a subset of "
                           "'volume_fractions'. "
                           " 'volume_fractions' is missing child '"
                           << curr_name
                           <<"' which exists in 'material_map`" ;
                    log::error(info, protocol,oss.str());
                    res &= false;
                }
            }
        }
    }

    if(matset.has_child("element_ids"))
    {
        bool eids_res = true;

        if(vfs_res)
        {
            if(!matset["element_ids"].dtype().is_integer() &&
                !matset["element_ids"].dtype().is_object())
            {
                log::error(info, protocol, "'element_ids' isn't the correct type");
                res &= eids_res &= false;
            }
            else if(matset["element_ids"].dtype().is_object() &&
                matset["volume_fractions"].dtype().is_object())
            {
                const std::vector<std::string> &vf_mats = matset["volume_fractions"].child_names();
                const std::vector<std::string> &eid_mats = matset["element_ids"].child_names();
                const std::set<std::string> vf_matset(vf_mats.begin(), vf_mats.end());
                const std::set<std::string> eid_matset(eid_mats.begin(), eid_mats.end());
                if(vf_matset != eid_matset)
                {
                    log::error(info, protocol, "'element_ids' hierarchy must match 'volume_fractions'");
                    eids_res &= false;
                }

                const Node &eids = matset["element_ids"];
                Node &eids_info = info["element_ids"];

                NodeConstIterator mat_it = eids.children();
                while(mat_it.has_next())
                {
                    const std::string &mat_name = mat_it.next().name();
                    eids_res &= verify_integer_field(protocol, eids, eids_info, mat_name);
                }

                res &= eids_res;
                log::validation(eids_info, eids_res);
            }
            else if(matset["element_ids"].dtype().is_integer() &&
                matset["volume_fractions"].dtype().is_number())
            {
                res &= eids_res &= verify_integer_field(protocol, matset, info, "element_ids");
            }
            else
            {
                log::error(info, protocol, "'element_ids' hierarchy must match 'volume_fractions'");
                res &= eids_res &= false;
            }
        }
    }

    log::validation(info, res);

    return res;
}

//-------------------------------------------------------------------------
bool
mesh::matset::is_multi_buffer(const Node &matset)
{
    return matset.child("volume_fractions").dtype().is_object();
}

//-------------------------------------------------------------------------
bool
mesh::matset::is_uni_buffer(const Node &matset)
{
    return matset.child("volume_fractions").dtype().is_number();
}

//-------------------------------------------------------------------------
bool
mesh::matset::is_element_dominant(const Node &matset)
{
    return !matset.has_child("element_ids");
}

//-------------------------------------------------------------------------
bool
mesh::matset::is_material_dominant(const Node &matset)
{
    return matset.has_child("element_ids");
}

//-----------------------------------------------------------------------------
// blueprint::mesh::matset::index protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::matset::index::verify(const Node &matset_idx,
                            Node &info)
{
    const std::string protocol = "mesh::matset::index";
    bool res = true;
    info.reset();

    // TODO(JRC): Determine whether or not extra verification needs to be
    // performed on the "materials" field.

    res &= verify_string_field(protocol, matset_idx, info, "topology");

    // 2021-1-29 cyrush:
    // prefer new "material_map" index spec, vs old "materials"
    if(matset_idx.has_child("material_map"))
    {
        res &= verify_matset_material_map(protocol,matset_idx,info);
    }
    else
    {
        res &= verify_object_field(protocol, matset_idx, info, "materials");
    }

    res &= verify_string_field(protocol, matset_idx, info, "path");

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::field protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::field::verify(const Node &field,
                    Node &info)
{
    const std::string protocol = "mesh::field";
    bool res = true;
    info.reset();

    bool has_assoc = field.has_child("association");
    bool has_basis = field.has_child("basis");
    if(!has_assoc && !has_basis)
    {
        log::error(info, protocol, "missing child 'association' or 'basis'");
        res = false;
    }
    if(has_assoc)
    {
        res &= mesh::association::verify(field["association"], info["association"]);
    }
    if(has_basis)
    {
        res &= mesh::field::basis::verify(field["basis"], info["basis"]);
    }

    bool has_topo = field.has_child("topology");
    bool has_matset = field.has_child("matset");
    bool has_topo_values = field.has_child("values");
    bool has_matset_values = field.has_child("matset_values");
    if(!has_topo && !has_matset)
    {
        log::error(info, protocol, "missing child 'topology' or 'matset'");
        res = false;
    }

    if(has_topo ^ has_topo_values)
    {
        std::ostringstream oss;
        oss << "'" << (has_topo ? "topology" : "values") <<"'"
            << " is present, but its companion "
            << "'" << (has_topo ? "values" : "topology") << "'"
            << " is missing";
        log::error(info, protocol, oss.str());
        res = false;
    }
    else if(has_topo && has_topo_values)
    {
        res &= verify_string_field(protocol, field, info, "topology");
        res &= verify_mlarray_field(protocol, field, info, "values", 0, 1, false);
    }

    if(has_matset ^ has_matset_values)
    {
        std::ostringstream oss;
        oss << "'" << (has_matset ? "matset" : "matset_values") <<"'"
            << " is present, but its companion "
            << "'" << (has_matset ? "matset_values" : "matset") << "'"
            << " is missing";
        log::error(info, protocol, oss.str());
        res = false;
    }
    else if(has_matset && has_matset_values)
    {
        res &= verify_string_field(protocol, field, info, "matset");
        res &= verify_mlarray_field(protocol, field, info, "matset_values", 0, 2, false);
    }

    // TODO(JRC): Enable 'volume_dependent' once it's confirmed to be a required
    // entry for fields.
    // res &= verify_enum_field(protocol, field, info, "volume_dependent", bputils::BOOLEANS);

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
void
mesh::field::generate_strip(Node& fields,
                            const std::string& toponame,
                            const std::string& dest_toponame,
                            std::map<std::string, std::string> & matset_names)
{
    Node newfields;

    NodeConstIterator fields_it = fields.children();
    while (fields_it.has_next())
    {
        const Node& field = fields_it.next();

        if (field["topology"].as_string() == toponame)
        {
            // TODO Something useful with grid functions, when needed by users.
            if (field.has_child("association"))
            {
                if (field["association"].as_string() == "element")
                {
                    const std::string newfieldname = dest_toponame + "_" + fields_it.name();
                    newfields[newfieldname] = field;
                    newfields[newfieldname]["topology"] = dest_toponame;
                    if (newfields[newfieldname].has_child("matset"))
                    {
                        const std::string matset_name = newfields[newfieldname]["matset"].as_string();
                        const std::string newmatsetname = dest_toponame + "_" + matset_name;
                        matset_names[matset_name] = newmatsetname;
                    }
                }
                else
                {
                    // For now, do nothing.
                    // TODO Something useful with vertex fields.  Confer with users.
                }
            }
        }
    }

    NodeConstIterator newfields_it = newfields.children();
    while (newfields_it.has_next())
    {
        const Node& newfield = newfields_it.next();
        fields[newfields_it.name()] = newfield;
    }
}

//-----------------------------------------------------------------------------
// blueprint::mesh::field::basis protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::field::basis::verify(const Node &basis,
                           Node &info)
{
    const std::string protocol = "mesh::field::basis";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, basis, info);

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::field::index protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::field::index::verify(const Node &field_idx,
                           Node &info)
{
    const std::string protocol = "mesh::field::index";
    bool res = true;
    info.reset();

    bool has_assoc = field_idx.has_child("association");
    bool has_basis = field_idx.has_child("basis");
    if(!has_assoc && !has_basis)
    {
        log::error(info, protocol, "missing child 'association' or 'basis'");
        res = false;
    }
    if(has_assoc)
    {
        res &= mesh::association::verify(field_idx["association"], info["association"]);
    }
    if(has_basis)
    {
        res &= mesh::field::basis::verify(field_idx["basis"], info["basis"]);
    }

    bool has_topo = field_idx.has_child("topology");
    bool has_matset = field_idx.has_child("matset");
    if(!has_topo && !has_matset)
    {
        log::error(info, protocol, "missing child 'topology' or 'matset'");
        res = false;
    }
    if(has_topo)
    {
        res &= verify_string_field(protocol, field_idx, info, "topology");
    }
    if(has_matset)
    {
        res &= verify_string_field(protocol, field_idx, info, "matset");
    }

    res &= verify_integer_field(protocol, field_idx, info, "number_of_components");
    res &= verify_string_field(protocol, field_idx, info, "path");

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::specset protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// helper to verify a specset species_names
//-----------------------------------------------------------------------------
bool verify_specset_species_names(const std::string &protocol,
                                  const conduit::Node &specset,
                                  conduit::Node &info)
{
    bool res = verify_object_field(protocol, specset, info, "species_names");

    if (res)
    {
        // we already know we have an object, children should be
        // objects
        NodeConstIterator itr = specset["species_names"].children();
        while (itr.has_next())
        {
            const Node &curr_child = itr.next();
            if (!curr_child.dtype().is_object())
            {
                log::error(info,
                           protocol,
                           log::quote("species_names") +
                           "child " +
                           log::quote(itr.name()) +
                           " is not an object.");
                res = false;
            }
        }
    }

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
bool
mesh::specset::verify(const Node &specset,
                      Node &info)
{
    const std::string protocol = "mesh::specset";
    bool res = true, mvs_res = true;
    bool specnames_are_optional = true;
    info.reset();

    res &= verify_string_field(protocol, specset, info, "matset");
    res &= mvs_res &= verify_field_exists(protocol, specset, info, "matset_values");

    if (mvs_res)
    {
        if (!specset["matset_values"].dtype().is_number() &&
            !specset["matset_values"].dtype().is_object())
        {
            log::error(info, protocol, "'matset_values' isn't the correct type");
            res &= mvs_res &= false;
        }
        else if (specset["matset_values"].dtype().is_number() &&
                 verify_number_field(protocol, specset, info, "matset_values"))
        {
            log::info(info, protocol, "detected uni-buffer specset");
            // species_names is not optional in this case, signal
            // for opt check down the line
            specnames_are_optional = false;

            mvs_res &= blueprint::o2mrelation::verify(specset, info);

            res &= mvs_res;
        }
        else if (specset["matset_values"].dtype().is_object() &&
                 verify_object_field(protocol, specset, info, "matset_values"))
        {
            log::info(info, protocol, "detected multi-buffer specset");

            const Node &mfs = specset["matset_values"];
            Node &mfs_info = info["matset_values"];

            NodeConstIterator mat_it = mfs.children();
            while (mat_it.has_next())
            {
                const Node &mat = mat_it.next();
                const std::string &mat_name = mat_it.name();

                if (!mat.dtype().is_object())
                {
                    log::error(info, protocol,
                               "each material name must be the parent of species names (required for multi-buffer specsets) ");
                    res &= mvs_res &= false;
                }
                else
                {
                    Node &mat_info = mfs_info[mat_name];
                    NodeConstIterator spec_it = mat.children();
                    while (spec_it.has_next())
                    {
                        const Node &spec = spec_it.next();
                        const std::string &spec_name = spec_it.name();

                        if (spec.dtype().is_object())
                        {
                            mvs_res &= verify_o2mrelation_field(protocol, mat, mat_info, spec_name);
                        }
                        else
                        {
                            mvs_res &= verify_number_field(protocol, mat, mat_info, spec_name);
                        }
                    }

                    res &= mvs_res;
                    log::validation(mat_info, mvs_res);
                }
            }

            res &= mvs_res;
            log::validation(mfs_info, mvs_res);
        }
    }

    if (!specnames_are_optional && !specset.has_child("species_names"))
    {
        log::error(info, protocol,
            "'species_names' is missing (required for uni-buffer specsets) ");
        res &= false;
    }

    if (specset.has_child("species_names"))
    {
        if (specnames_are_optional)
        {
            log::optional(info, protocol, "includes species_names");
        }

        res &= verify_specset_species_names(protocol,specset,info);

        // for cases where matset_values are an object, we expect the species_names child
        // names to be a subset of the matset_values child names
        if (specset.has_child("matset_values") &&
            specset["matset_values"].dtype().is_object())
        {
            NodeConstIterator specnames_mat_itr = specset["species_names"].children();
            while (specnames_mat_itr.has_next())
            {
                const Node &specnames_mat = specnames_mat_itr.next();
                const std::string mat_name = specnames_mat_itr.name();

                NodeConstIterator specnames_mat_specnames_itr = specnames_mat.children();
                while (specnames_mat_specnames_itr.has_next())
                {
                    specnames_mat_specnames_itr.next();
                    const std::string spec_name = specnames_mat_specnames_itr.name();

                    if (! specset["matset_values"].has_path(mat_name + "/" + spec_name))
                    {
                        std::ostringstream oss;
                        oss << "'species_names' hierarchy must be a subset of "
                               "'matset_values'. "
                               " 'matset_values' is missing child '"
                               << mat_name << "/" << spec_name
                               <<"' which exists in 'species_names`" ;
                        log::error(info, protocol,oss.str());
                        res &= false;
                    }
                }
            }
        }
    }

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::specset::index::verify protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::specset::index::verify(const Node &specset_idx,
                             Node &info)
{
    const std::string protocol = "mesh::specset::index";
    bool res = true;
    info.reset();

    // TODO(JRC): Determine whether or not extra verification needs to be
    // performed on the "species" field.

    res &= verify_string_field(protocol, specset_idx, info, "matset");
    res &= verify_object_field(protocol, specset_idx, info, "species");
    res &= verify_string_field(protocol, specset_idx, info, "path");

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::adjset protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::adjset::verify(const Node &adjset,
                     Node &info)
{
    const std::string protocol = "mesh::adjset";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, adjset, info, "topology");
    res &= verify_field_exists(protocol, adjset, info, "association") &&
           mesh::association::verify(adjset["association"], info["association"]);

    if(!verify_object_field(protocol, adjset, info, "groups", false, true))
    {
        res = false;
    }
    else
    {
        bool groups_res = true;
        NodeConstIterator itr = adjset["groups"].children();
        while(itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();
            Node &chld_info = info["groups"][chld_name];

            bool group_res = true;
            group_res &= verify_integer_field(protocol, chld, chld_info, "neighbors");
            if(chld.has_child("values"))
            {
                group_res &= verify_integer_field(protocol, chld,
                    chld_info, "values");
            }
            else if(chld.has_child("windows"))
            {
                group_res &= verify_object_field(protocol, chld,
                    chld_info, "windows");

                bool windows_res = true;
                NodeConstIterator witr = chld["windows"].children();
                while(witr.has_next())
                {
                    const Node &wndw = witr.next();
                    const std::string wndw_name = witr.name();
                    Node &wndw_info = chld_info["windows"][wndw_name];

                    bool window_res = true;
                    window_res &= verify_field_exists(protocol, wndw,
                        wndw_info, "origin") &&
                        mesh::logical_dims::verify(wndw["origin"],
                            wndw_info["origin"]);
                    window_res &= verify_field_exists(protocol, wndw,
                        wndw_info, "dims") &&
                        mesh::logical_dims::verify(wndw["dims"],
                            wndw_info["dims"]);
                    window_res &= verify_field_exists(protocol, wndw,
                        wndw_info, "ratio") &&
                        mesh::logical_dims::verify(wndw["ratio"],
                            wndw_info["ratio"]);

                    // verify that dimensions for "origin" and
                    // "dims" and "ratio" are the same
                    if(window_res)
                    {
                        index_t window_dim = wndw["origin"].number_of_children();
                        window_res &= !wndw.has_child("dims") ||
                            verify_object_field(protocol, wndw,
                                wndw_info, "dims", false, window_dim);
                        window_res &= !wndw.has_child("ratio") ||
                            verify_object_field(protocol, wndw,
                                wndw_info, "ratio", false, window_dim);
                    }

                    log::validation(wndw_info,window_res);
                    windows_res &= window_res;
                }

                log::validation(chld_info["windows"],windows_res);
                res &= windows_res;

                if(chld.has_child("orientation"))
                {
                    group_res &= verify_integer_field(protocol, chld,
                        chld_info, "orientation");
                }
            }

            log::validation(chld_info,group_res);
            groups_res &= group_res;
        }

        log::validation(info["groups"],groups_res);
        res &= groups_res;
    }

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
bool
mesh::adjset::is_pairwise(const Node &adjset)
{
    bool res = true;

    NodeConstIterator group_itr = adjset["groups"].children();
    while(group_itr.has_next() && res)
    {
        const Node &group = group_itr.next();
        res &= group["neighbors"].dtype().number_of_elements() == 1;
    }

    return res;
}

//-----------------------------------------------------------------------------
bool
mesh::adjset::is_maxshare(const Node &adjset)
{
    bool res = true;

    std::set<index_t> ids;

    NodeConstIterator group_itr = adjset["groups"].children();
    while(group_itr.has_next() && res)
    {
        const Node &group = group_itr.next();
        const auto values = group["values"].as_index_t_accessor();

        for(index_t vi = 0; vi < values.number_of_elements(); vi++)
        {
            const index_t next_id = values[vi];

            res &= ids.find(next_id) == ids.end();
            ids.insert(next_id);
        }
    }

    return res;
}

//-----------------------------------------------------------------------------
void
mesh::adjset::to_pairwise(const Node &adjset,
                          Node &dest)
{
    dest.reset();

    const DataType int_dtype = bputils::find_widest_dtype(adjset, bputils::DEFAULT_INT_DTYPES);

    // NOTE(JRC): We assume that group names are shared across ranks, but
    // make no assumptions on the uniqueness of a set of neighbors for a group
    // (i.e. the same set of neighbors can be used in >1 groups).
    std::vector<std::string> adjset_group_names = adjset["groups"].child_names();
    std::sort(adjset_group_names.begin(), adjset_group_names.end());

    // Compile ordered lists for each neighbor containing their unique lists
    // of 'adjset' entity indices, as compiled from all groups in the source 'adjset'.
    std::map<index_t, std::vector<index_t>> pair_values_map;
    for(const std::string &group_name : adjset_group_names)
    {
        const Node &group_node = adjset["groups"][group_name];

        std::vector<index_t> group_neighbors;
        {
            const auto neighbors = group_node["neighbors"].as_index_t_accessor();
            group_neighbors.reserve(neighbors.number_of_elements());
            for(index_t ni = 0; ni < neighbors.number_of_elements(); ++ni)
            {
                group_neighbors.push_back(neighbors[ni]);
            }
        }

        std::vector<index_t> group_values;
        {
            const auto values = group_node["values"].as_index_t_accessor();
            const index_t numValues = values.number_of_elements();
            group_values.reserve(numValues);
            for(index_t vi = 0; vi < numValues; ++vi)
            {
                group_values.push_back(values[vi]);
            }
        }

        for(const index_t &neighbor_id : group_neighbors)
        {
            std::vector<index_t> &neighbor_values = pair_values_map[neighbor_id];
            neighbor_values.insert(neighbor_values.end(),
                group_values.begin(), group_values.end());
        }
    }

    // Given ordered lists of adjset values per neighbor, generate the destination
    // adjset hierarchy.
    Node adjset_template;
    adjset_template.set_external(adjset);
    adjset_template.remove("groups");

    dest.set(adjset_template);
    dest["groups"].set(DataType::object());

    for(const auto &pair_values_pair : pair_values_map)
    {
        const index_t &neighbor_id = pair_values_pair.first;
        const std::vector<index_t> &neighbor_values = pair_values_pair.second;

        Node &group_node = dest["groups"][std::to_string(dest["groups"].number_of_children())];
        group_node["neighbors"].set(DataType(int_dtype.id(), 1));
        {
            Node temp(DataType::index_t(1), (void*)&neighbor_id, true);
            temp.to_data_type(int_dtype.id(), group_node["neighbors"]);
        }
        group_node["values"].set(DataType(int_dtype.id(), neighbor_values.size()));
        {
            Node temp(DataType::index_t(neighbor_values.size()),
                (void*)neighbor_values.data(), true);
            temp.to_data_type(int_dtype.id(), group_node["values"]);
        }
    }
    bputils::adjset::canonicalize(dest);
}

//-----------------------------------------------------------------------------
void
mesh::adjset::to_maxshare(const Node &adjset,
                          Node &dest)
{
    dest.reset();

    const DataType int_dtype = bputils::find_widest_dtype(adjset, bputils::DEFAULT_INT_DTYPES);

    // NOTE(JRC): We assume that group names are shared across ranks, but
    // make no assumptions on the uniqueness of a set of neighbors for a group
    // (i.e. the same set of neighbors can be used in >1 groups).
    std::vector<std::string> adjset_group_names = adjset["groups"].child_names();
    std::sort(adjset_group_names.begin(), adjset_group_names.end());

    std::map<index_t, std::set<index_t>> entity_groupset_map;
    for(const std::string &group_name : adjset_group_names)
    {
        const Node &group_node = adjset["groups"][group_name];

        std::vector<index_t> group_neighbors;
        {
            const auto neighbors = group_node["neighbors"].as_index_t_accessor();
            group_neighbors.reserve(neighbors.number_of_elements());
            for(index_t ni = 0; ni < neighbors.number_of_elements(); ++ni)
            {
                group_neighbors.push_back(neighbors[ni]);
            }
        }

        std::vector<index_t> group_values;
        {
            const auto values = group_node["values"].as_index_t_accessor();
            for(index_t vi = 0; vi < values.number_of_elements(); ++vi)
            {
                group_values.push_back(values[vi]);
            }
        }

        for(const index_t &entity_id : group_values)
        {
            std::set<index_t> &entity_groupset = entity_groupset_map[entity_id];
            entity_groupset.insert(group_neighbors.begin(), group_neighbors.end());
        }
    }

    // Given ordered lists of adjset values per neighbor, generate the destination
    // adjset hierarchy.
    Node adjset_template;
    adjset_template.set_external(adjset);
    adjset_template.remove("groups");

    dest.set(adjset_template);
    dest["groups"].set(DataType::object());

    std::map<std::set<index_t>, Node *> groupset_groupnode_map;
    for(const auto &entity_groupset_pair : entity_groupset_map)
    {
        const std::set<index_t> &groupset = entity_groupset_pair.second;
        if(groupset_groupnode_map.find(groupset) == groupset_groupnode_map.end())
        {
            Node &group_node = dest["groups"][std::to_string(dest["groups"].number_of_children())];
            group_node["neighbors"].set(DataType(int_dtype.id(), groupset.size()));
            {
                const std::vector<index_t> grouplist(groupset.begin(), groupset.end());
                Node temp(DataType::index_t(grouplist.size()), (void*)grouplist.data(), true);
                temp.to_data_type(int_dtype.id(), group_node["neighbors"]);
            }

            groupset_groupnode_map[groupset] = &group_node;
        }
    }

    // Now that the groundwork for each unique max-share group has been set,
    // we populate the 'values' content of each group in order based on
    // lexicographically sorted group names
    std::map<std::set<index_t>, std::pair<std::vector<index_t>, std::set<index_t>>> groupset_values_map;
    for(const std::string &group_name : adjset_group_names)
    {
        const Node &group_node = adjset["groups"][group_name];
        const auto values = group_node["values"].as_index_t_accessor();
        for(index_t vi = 0; vi < values.number_of_elements(); ++vi)
        {
            const index_t group_entity = values[vi];

            auto &groupset_pair = groupset_values_map[entity_groupset_map[group_entity]];
            std::vector<index_t> &groupset_valuelist = groupset_pair.first;
            std::set<index_t> &groupset_valueset = groupset_pair.second;
            if(groupset_valueset.find(group_entity) == groupset_valueset.end())
            {
                groupset_valuelist.push_back(group_entity);
                groupset_valueset.insert(group_entity);
            }
        }
    }

    for(const auto &groupset_values_pair : groupset_values_map)
    {
        const std::set<index_t> &groupset = groupset_values_pair.first;
        const std::vector<index_t> &groupset_values = groupset_values_pair.second.first;

        Node &group_node = *groupset_groupnode_map[groupset];
        group_node["values"].set(DataType(int_dtype.id(), groupset_values.size()));
        {
            Node temp(DataType::index_t(groupset_values.size()),
                (void*)groupset_values.data(), true);
            temp.to_data_type(int_dtype.id(), group_node["values"]);
        }
    }

    bputils::adjset::canonicalize(dest);
}

//-----------------------------------------------------------------------------
std::string
mesh::adjset::group_prefix()
{
    return "group";
}

//-----------------------------------------------------------------------------
// blueprint::mesh::adjset::index protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::adjset::index::verify(const Node &adj_idx,
                            Node &info)
{
    const std::string protocol = "mesh::adjset::index";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, adj_idx, info, "topology");
    res &= verify_field_exists(protocol, adj_idx, info, "association") &&
           mesh::association::verify(adj_idx["association"], info["association"]);
    res &= verify_string_field(protocol, adj_idx, info, "path");

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::nestset protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::nestset::verify(const Node &nestset,
                      Node &info)
{
    const std::string protocol = "mesh::nestset";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, nestset, info, "topology");
    res &= verify_field_exists(protocol, nestset, info, "association") &&
           mesh::association::verify(nestset["association"], info["association"]);

    if(!verify_object_field(protocol, nestset, info, "windows"))
    {
        res = false;
    }
    else
    {
        bool windows_res = true;
        NodeConstIterator itr = nestset["windows"].children();
        while(itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();
            Node &chld_info = info["windows"][chld_name];

            bool window_res = true;
            window_res &= verify_integer_field(protocol, chld, chld_info, "domain_id");
            window_res &= verify_field_exists(protocol, chld, chld_info, "domain_type") &&
                mesh::nestset::type::verify(chld["domain_type"], chld_info["domain_type"]);

            window_res &= verify_field_exists(protocol, chld, chld_info, "ratio") &&
                mesh::logical_dims::verify(chld["ratio"], chld_info["ratio"]);
            window_res &= !chld.has_child("origin") ||
                mesh::logical_dims::verify(chld["origin"], chld_info["origin"]);
            window_res &= !chld.has_child("dims") ||
                mesh::logical_dims::verify(chld["dims"], chld_info["dims"]);

            // one last pass: verify that dimensions for "ratio", "origin", and
            // "dims" are all the same
            if(window_res)
            {
                index_t window_dim = chld["ratio"].number_of_children();
                window_res &= !chld.has_child("origin") ||
                    verify_object_field(protocol, chld, chld_info, "origin", false, false, window_dim);
                window_res &= !chld.has_child("dims") ||
                    verify_object_field(protocol, chld, chld_info, "dims", false, false, window_dim);
            }

            log::validation(chld_info,window_res);
            windows_res &= window_res;
        }

        log::validation(info["windows"],windows_res);
        res &= windows_res;
    }

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::nestset::index protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::nestset::index::verify(const Node &nest_idx,
                            Node &info)
{
    const std::string protocol = "mesh::nestset::index";
    bool res = true;
    info.reset();

    res &= verify_string_field(protocol, nest_idx, info, "topology");
    res &= verify_field_exists(protocol, nest_idx, info, "association") &&
           mesh::association::verify(nest_idx["association"], info["association"]);
    res &= verify_string_field(protocol, nest_idx, info, "path");

    log::validation(info, res);

    return res;
}

//-----------------------------------------------------------------------------
// blueprint::mesh::topology::type protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::nestset::type::verify(const Node &type,
                            Node &info)
{
    const std::string protocol = "mesh::nestset::type";
    bool res = true;
    info.reset();

    res &= verify_enum_field(protocol, type, info, "", bputils::NESTSET_TYPES);

    log::validation(info,res);

    return res;
}


//-----------------------------------------------------------------------------
// blueprint::mesh::index protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool
mesh::index::verify(const Node &n,
                    Node &info)
{
    const std::string protocol = "mesh::index";
    bool res = true;
    info.reset();

    if(!verify_object_field(protocol, n, info, "coordsets"))
    {
        res = false;
    }
    else
    {
        bool cset_res = true;
        NodeConstIterator itr = n["coordsets"].children();
        while(itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();
            cset_res &= coordset::index::verify(chld, info["coordsets"][chld_name]);
        }

        log::validation(info["coordsets"],cset_res);
        res &= cset_res;
    }

    if(!verify_object_field(protocol, n, info, "topologies"))
    {
        res = false;
    }
    else
    {
        bool topo_res = true;
        NodeConstIterator itr = n["topologies"].children();
        while(itr.has_next())
        {
            const Node &chld = itr.next();
            const std::string chld_name = itr.name();
            Node &chld_info = info["topologies"][chld_name];

            topo_res &= topology::index::verify(chld, chld_info);
            topo_res &= verify_reference_field(protocol, n, info,
                chld, chld_info, "coordset", "coordsets");
        }

        log::validation(info["topologies"],topo_res);
        res &= topo_res;
    }

    // optional: "matsets", each child must conform to
    // "mesh::index::matset"
    if(n.has_path("matsets"))
    {
        if(!verify_object_field(protocol, n, info, "matsets"))
        {
            res = false;
        }
        else
        {
            bool mset_res = true;
            NodeConstIterator itr = n["matsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["matsets"][chld_name];

                mset_res &= matset::index::verify(chld, chld_info);
                mset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "topology", "topologies");
            }

            log::validation(info["matsets"],mset_res);
            res &= mset_res;
        }
    }

    // optional: "specsets", each child must conform to
    // "mesh::index::specset"
    if(n.has_path("specsets"))
    {
        if(!verify_object_field(protocol, n, info, "specsets"))
        {
            res = false;
        }
        else
        {
            bool sset_res = true;
            NodeConstIterator itr = n["specsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["specsets"][chld_name];

                sset_res &= specset::index::verify(chld, chld_info);
                sset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "matset", "matsets");
            }

            log::validation(info["specsets"],sset_res);
            res &= sset_res;
        }
    }

    // optional: "fields", each child must conform to
    // "mesh::index::field"
    if(n.has_path("fields"))
    {
        if(!verify_object_field(protocol, n, info, "fields"))
        {
            res = false;
        }
        else
        {
            bool field_res = true;
            NodeConstIterator itr = n["fields"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["fields"][chld_name];

                field_res &= field::index::verify(chld, chld_info);
                if(chld.has_child("topology"))
                {
                    field_res &= verify_reference_field(protocol, n, info,
                        chld, chld_info, "topology", "topologies");
                }
                if(chld.has_child("matset"))
                {
                    field_res &= verify_reference_field(protocol, n, info,
                        chld, chld_info, "matset", "matsets");
                }
            }

            log::validation(info["fields"],field_res);
            res &= field_res;
        }
    }

    // optional: "adjsets", each child must conform to
    // "mesh::index::adjsets"
    if(n.has_path("adjsets"))
    {
        if(!verify_object_field(protocol, n, info, "adjsets"))
        {
            res = false;
        }
        else
        {
            bool aset_res = true;
            NodeConstIterator itr = n["adjsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["adjsets"][chld_name];

                aset_res &= adjset::index::verify(chld, chld_info);
                aset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "topology", "topologies");
            }

            log::validation(info["adjsets"],aset_res);
            res &= aset_res;
        }
    }

    // optional: "nestsets", each child must conform to
    // "mesh::index::nestsets"
    if(n.has_path("nestsets"))
    {
        if(!verify_object_field(protocol, n, info, "nestsets"))
        {
            res = false;
        }
        else
        {
            bool nset_res = true;
            NodeConstIterator itr = n["nestsets"].children();
            while(itr.has_next())
            {
                const Node &chld = itr.next();
                const std::string chld_name = itr.name();
                Node &chld_info = info["nestsets"][chld_name];

                nset_res &= nestset::index::verify(chld, chld_info);
                nset_res &= verify_reference_field(protocol, n, info,
                    chld, chld_info, "topology", "topologies");
            }

            log::validation(info["nestsets"],nset_res);
            res &= nset_res;
        }
    }

    log::validation(info, res);

    return res;
}


//-------------------------------------------------------------------------
void
mesh::paint_adjset(const std::string &adjset_name,
                   const std::string &field_prefix,
                   conduit::Node &mesh)
{
    // {field_prefix}_group_count -- total number of groups the vertex or element is in
    // {field_prefix}_order_{group_name} -- the vertex or element's order in group {group_name}

    // recipe adapted from Max Yang's checks in t_blueprint_mpi_mesh_parmetis

    auto mesh_doms = conduit::blueprint::mesh::domains(mesh);

    for(Node *dom : mesh_doms)
    {
        // if we don't have an adjset, skip this domain
        if(!dom->has_path("adjsets/" + adjset_name))
        {
            continue;
        }

        // check adjset assoc
        const Node &adjset = dom->fetch_existing("adjsets/" + adjset_name);
        const std::string assoc = adjset["association"].as_string();

        // get the coordset for our topo, so we can find out the
        // number of verts
        std::string topo_name = adjset["topology"].as_string();
        const Node &topo = dom->fetch_existing("topologies/" + topo_name);
        const Node &coordset = bputils::topology::coordset(topo);

        index_t num_verts = mesh::coordset::length(coordset);
        index_t num_eles  = mesh::topology::length(topo);
        index_t num_entries = num_verts;

        if(assoc == "element")
            num_entries = num_eles;

        // we want a field that counts the number of groups a vertex is in
        Node &res_cnt_field = dom->fetch("fields/" + field_prefix + "_group_count");
        res_cnt_field["association"] = assoc;
        res_cnt_field["topology"] = topo_name;
        res_cnt_field["values"].set(DataType::int64(num_entries));
        int64_array res_cnt_vals = res_cnt_field["values"].value();

        // loop over groups
        for (const Node& group : adjset["groups"].children())
        {
            // we also want a field that shows the order for each group
            // init this field
            Node &res_order_field = dom->fetch("fields/" + field_prefix + "_order_" + group.name());
            res_order_field["association"] = assoc;
            res_order_field["topology"] = topo_name;
            res_order_field["values"].set(DataType::int64(num_entries));
            int64_array res_order_vals = res_order_field["values"].value();
            res_order_vals.fill(-1);
            // get entires
            int64_accessor grp_vals = group["values"].as_int64_accessor();
            for (index_t iv = 0; iv < grp_vals.number_of_elements(); iv++)
            {
                // update count
                res_cnt_vals[grp_vals[iv]] += 1;
                // record order
                res_order_vals[grp_vals[iv]] = iv;
            }
        }
    }
}


//-------------------------------------------------------------------------
void
mesh::partition(const conduit::Node &n_mesh,
                const conduit::Node &options,
                conduit::Node &output)
{
    mesh::Partitioner p;
    if(p.initialize(n_mesh, options))
    {
        p.split_selections();
        output.reset();
        p.execute(output);
    }
}

//-------------------------------------------------------------------------
void mesh::partition_map_back(const Node& repart_mesh,
                              const Node& options,
                              Node& orig_mesh)
{
    mesh::Partitioner p;
    p.map_back_fields(repart_mesh, options, orig_mesh);
}

//-------------------------------------------------------------------------
void mesh::generate_boundary_partition_field(const conduit::Node &topo,
                                             const conduit::Node &partField,
                                             const conduit::Node &btopo,
                                             conduit::Node &bpartField)
{
    // Basic checks.
    if(topo.has_child("coordset") && btopo.has_child("coordset"))
    {
        if(topo.fetch_existing("coordset").as_string() != btopo.fetch_existing("coordset").as_string())
        {
            CONDUIT_ERROR("Input topologies must use the same coordset.");
        }
    }
    if(partField.has_child("topology") && (partField.fetch_existing("topology").as_string() != topo.name()))
    {
        CONDUIT_ERROR("The partition field must be associated with the " << topo.name() << " topology.");
    }

    const Node *coordset = bputils::find_reference_node(topo, "coordset");

    // Produce the external "faces" of the domain and for each "face", hash
    // its node ids and associate that hash with the parent zone for the face.
    auto d = static_cast<size_t>(conduit::blueprint::mesh::utils::topology::dims(btopo));
    std::vector<std::pair<size_t, size_t>> desired_maps{{d, d + 1}};
    bputils::TopologyMetadata md(topo, *coordset, d, desired_maps);
    const conduit::Node &dtopo = md.get_topology(d);
    auto nent = md.get_topology_length(d);
    std::map<conduit::uint64, int> hashToZone;
    for (conduit::index_t ei = 0; ei < nent; ei++)
    {
        // The global associatino for this "face" tells us how many elements
        // it belongs to. If there is just one parent, it is external and can
        // belong to the boundary.
        const auto vv = md.get_global_association(ei, d, d + 1);
        if (vv.size() == 1)
        {
            // Get the ids that make up the entity and hash them.
            auto ids = conduit::blueprint::mesh::utils::topology::unstructured::points(dtopo, ei);
            std::sort(ids.begin(), ids.end());
            conduit::uint64 h = conduit::utils::hash(&ids[0], static_cast<unsigned int>(ids.size()));

            // Save hash to parent zone.
            hashToZone[h] = vv[0];
        }
    }

    // Get the partition field.
    const auto f = partField.fetch_existing("values").as_int32_accessor();

    // Now, iterate through the boundary topology, hash each entity's ids
    // and try to look up the parent zone. The hashToZone map should contain
    // all possible external faces for the domain so the boundary should be
    // a subset of that.
    auto blen = conduit::blueprint::mesh::topology::length(btopo);
    bpartField.reset();
    bpartField["association"] = "element";
    bpartField["topology"] = btopo.name();
    bpartField["values"].set(conduit::DataType::int32(blen));
    auto bndPartition = bpartField["values"].as_int32_ptr();
    for (conduit::index_t ei = 0; ei < blen; ei++)
    {
        // Get the ids that make up the entity and hash them.
        auto ids = conduit::blueprint::mesh::utils::topology::unstructured::points(btopo, ei);
        std::sort(ids.begin(), ids.end());
        conduit::uint64 h = conduit::utils::hash(&ids[0], static_cast<unsigned int>(ids.size()));

        auto it = hashToZone.find(h);
        bndPartition[ei] = (it != hashToZone.end()) ? f[it->second] : 0;
    }
}

//-------------------------------------------------------------------------
void
mesh::flatten(const conduit::Node &mesh,
              const conduit::Node &options,
              conduit::Node &output)
{
    output.reset();

    MeshFlattener do_flatten;
    do_flatten.set_options(options);
    do_flatten.execute(mesh, output);
}

//-------------------------------------------------------------------------
void mesh::generate_domain_ids(conduit::Node &domains)
{
  int num_domains = (int)domains.number_of_children();

  int domain_offset = 0;

  for(int i = 0; i < num_domains; ++i)
  {
    conduit::Node &dom = domains.child(i);
    dom["state/domain_id"] = domain_offset + i;
  }
}

//
// NOTE: Begin some namespaces to enclose some internal functions.
//

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh --
//-----------------------------------------------------------------------------
namespace mesh
{

/*!
 * @brief Copy all nodes under the root that have the specified topology name.
 *
 * @param n_mesh The input mesh.
 * @param topoName The name of the topology of interest.
 * @param n_output The node that will contain the output mesh.
 * @param rootName The root name (e.g. topologies, fields, matsets, ...)
 * @param copy If true, copy the nodes from n_mesh to n_output. If false, we will
 *             use set_external instead.
 * @param selection An optional set of object names to copy.
 */
static void
copy_nodes_with_topology(const conduit::Node &n_mesh,
                         const std::string &topoName,
                         conduit::Node &n_output,
                         const std::string &rootName,
                         bool copy,
                         const std::set<std::string> &selection = std::set<std::string>())
{
    if(n_mesh.has_path(rootName))
    {
        const conduit::Node &n_objs = n_mesh.fetch_existing(rootName);
        conduit::Node &n_output_objs = n_output[rootName];
        for(conduit::index_t i = 0; i < n_objs.number_of_children(); i++)
        {
            const conduit::Node &n_obj = n_objs[i];

            // If we passed a selection, admit only those names.
            if(!selection.empty())
            {
                if(selection.find(n_obj.name()) == selection.end())
                {
                    // n_obj not found in selection. Skip it.
                    continue;
                }
            }

            if(n_obj["topology"].as_string() == topoName)
            {
                if(copy)
                {
                    n_output_objs[n_obj.name()].set(n_obj);
                }
                else
                {
                    n_output_objs[n_obj.name()].set_external(n_obj);
                }
            }
        }
        if(n_output_objs.number_of_children() == 0)
        {
            n_output.remove(rootName);
        }
    }
}

//-------------------------------------------------------------------------
/*!
 * @brief Copy a source node to a destination node.
 *
 * @param n_src The source node.
 * @param n_dest The destination node.
 * @param copy If true, deep copy the data; otherwise set_external the data.
 */
static void copy_node(const conduit::Node &n_src, conduit::Node &n_dest, bool copy)
{
    if(copy)
    {
        n_dest.set(n_src);
    }
    else
    {
        n_dest.set_external(n_src);
    }
}

//-------------------------------------------------------------------------
/*!
 * @brief Given an options node that might contain a "fields" node, return
 *        the names of the child nodes under the fields node.
 *
 * @param n_options The options node that contains the fields data.
 *
 * @return A set containing the selected fields. The set will be empty if the
 *         "fields" node was not present in the options.
 */
static std::set<std::string>
get_selected_fields(const conduit::Node &n_options)
{
    std::set<std::string> selection;
    if(n_options.has_path("fields"))
    {
        const conduit::Node &n_fields = n_options["fields"];
        for(conduit::index_t i = 0; i < n_fields.number_of_children(); i++)
        {
            selection.insert(n_fields[i].name());
        }
    }
    return selection;
}
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::topology --
//-----------------------------------------------------------------------------
namespace topology
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::topology::unstructured --
//-----------------------------------------------------------------------------
namespace unstructured
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::topology::unstructured::polytopal --
//-----------------------------------------------------------------------------
namespace polytopal
{

using Vector = conduit::geometry::vector<double, 3>;

/*!
 * @brief Compute face centers and face normals for polyhedral zones.
 *
 * @tparam ExecPolicy The execution policy to use for the loop.
 * @tparam IndexAccessor The container type for the subelement connectivity.
 * @tparam CoordAccessor The container type for the coordinates.
 *
 * @param subelements_connectivity An accessor used for subelements_connectivity
 * @param subelements_sizes An accessor used for subelements_sizes
 * @param subelements_offsets An accessor used for subelements_offsets
 * @param x An accessor (or pointer, etc) to access X coordinate data.
 * @param y An accessor (or pointer, etc) to access Y coordinate data.
 * @param z An accessor (or pointer, etc) to access Z coordinate data.
 * @param[out] allFaceCenters The output vector for all of the face centers.
 * @param[out] allFaceNormals The output vector for all of the face normals.
 *
 * @note Many of these arguments would normally be references but the loop captures
 *       by value so it is best to not use references.
 */
template <typename ExecPolicy, typename IndexAccessor, typename CoordAccessor>
void polyhedral_face_centers_normals(const IndexAccessor subelements_connectivity,
                                     const IndexAccessor subelements_sizes,
                                     const IndexAccessor subelements_offsets,
                                     const CoordAccessor x,
                                     const CoordAccessor y,
                                     const CoordAccessor z,
                                     std::vector<Vector> &allFaceCenters,
                                     std::vector<Vector> &allFaceNormals)
{
    using value_type = typename Vector::value_type;

    // Allocate output vectors.
    const auto totalNumFaces = subelements_sizes.number_of_elements();
    allFaceCenters.resize(totalNumFaces);
    allFaceNormals.resize(totalNumFaces);
    Vector *allFaceCentersPtr = allFaceCenters.data();
    Vector *allFaceNormalsPtr = allFaceNormals.data();

    // Compute face centers and normals.
    conduit::execution::for_all<ExecPolicy>(0, totalNumFaces, [=](conduit::index_t f) {
        const int NUM_VERTS = 4;
        const auto size = subelements_sizes[f];
        const auto offset = subelements_offsets[f];
        Vector center {};
        Vector pts[NUM_VERTS];
        for(conduit::index_t vi = 0; vi < size; vi++)
        {
            const auto ptId = subelements_connectivity[offset + vi];
            pts[vi] = Vector(static_cast<value_type>(x[ptId]),
                             static_cast<value_type>(y[ptId]),
                             static_cast<value_type>(z[ptId]));
            center += pts[vi];
        }

        center /= static_cast<value_type>(size);
        allFaceCentersPtr[f] = center;

        // Compute apparent face normal.
        allFaceNormalsPtr[f] = (pts[2] - pts[1]).cross(pts[0] - pts[1]).normalize();
    });
}

/*!
 * @brief Compute face centers and face normals for polyhedral zones.
 *
 * @tparam ExecPolicy The execution policy to use for the loop.
 * @tparam IndexAccessor The container type for the subelement connectivity.
 *
 * @param n_coordset The node that contains the coordset. It must be explicit.
 * @param subelements_connectivity An accessor used for subelements_connectivity
 * @param subelements_sizes An accessor used for subelements_sizes
 * @param subelements_offsets An accessor used for subelements_offsets
 * @param[out] allFaceCenters The output vector for all of the face centers.
 * @param[out] allFaceNormals The output vector for all of the face normals.
 */
template <typename ExecPolicy, typename IndexAccessor>
void polyhedral_face_centers_normals(const conduit::Node &n_coordset,
                                     const IndexAccessor &subelements_connectivity,
                                     const IndexAccessor &subelements_sizes,
                                     const IndexAccessor &subelements_offsets,
                                     std::vector<Vector> &allFaceCenters,
                                     std::vector<Vector> &allFaceNormals)
{
    // TODO: Call to_explicit() on the coordset if needed.

    const conduit::Node &n_x = n_coordset["values/x"];
    const conduit::Node &n_y = n_coordset["values/y"];
    const conduit::Node &n_z = n_coordset["values/z"];
    bool handled = false;
    // Dispatch to different instantiations of the function.
    if(n_x.dtype().is_compact() && n_y.dtype().is_compact() && n_z.dtype().is_compact())
    {
        // Handle contiguous float64, float32. (fast paths)
        if(n_x.dtype().is_float64() && n_y.dtype().is_float64() && n_z.dtype().is_float64())
        {
            polyhedral_face_centers_normals<ExecPolicy>(subelements_connectivity,
                                                        subelements_sizes,
                                                        subelements_offsets,
                                                        n_x.as_float64_ptr(),
                                                        n_y.as_float64_ptr(),
                                                        n_z.as_float64_ptr(),
                                                        allFaceCenters,
                                                        allFaceNormals);
            handled = true;
        }
        if(n_x.dtype().is_float32() && n_y.dtype().is_float32() && n_z.dtype().is_float32())
        {
            polyhedral_face_centers_normals<ExecPolicy>(subelements_connectivity,
                                                        subelements_sizes,
                                                        subelements_offsets,
                                                        n_x.as_float32_ptr(),
                                                        n_y.as_float32_ptr(),
                                                        n_z.as_float32_ptr(),
                                                        allFaceCenters,
                                                        allFaceNormals);
            handled = true;
        }
    }
    if(!handled)
    {
        // The coordinates were not supported in the more direct modes above so
        // use accessors instead.
        polyhedral_face_centers_normals<ExecPolicy>(subelements_connectivity,
                                                    subelements_sizes,
                                                    subelements_offsets,
                                                    n_x.as_double_accessor(),
                                                    n_y.as_double_accessor(),
                                                    n_z.as_double_accessor(),
                                                    allFaceCenters,
                                                    allFaceNormals);
    }
}

/*!
 * @brief Average a PH element's face centers to make its element center (good enough).
 *
 * @tparam ExecPolicy The execution policy to use for the loop.
 * @tparam IndexAccessor The container type for the element connectivity.
 *
 * @param elements_connectivity An accessor used for elements_connectivity
 * @param elements_sizes An accessor used for elements_sizes
 * @param elements_offsets An accessor used for elements_offsets
 * @param allFaceCenters The vector for all of the face centers.
 * @param[out] allElemCenters The output vector for the element centers.
 */
template <typename ExecPolicy, typename IndexAccessor>
void polyhedral_elem_centers(const IndexAccessor elements_connectivity,
                             const IndexAccessor elements_sizes,
                             const IndexAccessor elements_offsets,
                             const std::vector<Vector> &allFaceCenters,
                             std::vector<Vector> &allElemCenters)
{
    const auto totalNumElems = elements_sizes.number_of_elements();
    allElemCenters.resize(totalNumElems);
    Vector *allElemCentersPtr = allElemCenters.data();
    const Vector *allFaceCentersPtr = allFaceCenters.data();
    conduit::execution::for_all<ExecPolicy>(0, totalNumElems, [=](conduit::index_t i) {
        const auto size = elements_sizes[i];
        const auto offset = elements_offsets[i];
        Vector center {};
        for(conduit::index_t f = 0; f < size; f++)
        {
            const auto faceId = elements_connectivity[offset + f];
            center += allFaceCentersPtr[faceId];
        }
        center /= static_cast<double>(size);
        allElemCentersPtr[i] = center;
    });
}

/*!
 * @brief Convert a polyhedral topology to an unstructured topology of hexes. If the
 *        input topology does not contain exclusively hexes then it errors out.
 *
 * @tparam ExecPolicy The execution policy to use for the loop.
 *
 * @param n_topo The source unstructured polyhedral topology.
 * @param n_output_topo The output unstructured hex topology.
 */
template <typename ExecPolicy>
static void polyhedral_to_hexes(const conduit::Node &n_topo, conduit::Node &n_output_topo)
{
#if defined(_WIN32)
    // Use macros on Windows to work around an issue with lambda capture.
    #define CONDUIT_NUM_FACES 6
    #define CONDUIT_NUM_VERTS 4
    #define CONDUIT_VERTS_PER_HEX 8
#else
    constexpr conduit::index_t CONDUIT_NUM_FACES = 6;
    constexpr conduit::index_t CONDUIT_NUM_VERTS = 4;
    constexpr conduit::index_t CONDUIT_VERTS_PER_HEX = 8;
#endif

    // Checks for unstructured + polyhedral.
    if(n_topo["type"].as_string() != "unstructured")
    {
        CONDUIT_ERROR("The input topology is not unstructured.");
    }
    if(n_topo["elements/shape"].as_string() != "polyhedral")
    {
        CONDUIT_ERROR("The input topology is not polyhedral.");
    }

    // Make accessors for the basic polyhedral topology data.
    const auto elements_connectivity =
        n_topo.fetch_existing("elements/connectivity").as_index_t_accessor();
    const auto elements_sizes = n_topo.fetch_existing("elements/sizes").as_index_t_accessor();
    const auto elements_offsets = n_topo.fetch_existing("elements/offsets").as_index_t_accessor();
    const auto subelements_connectivity =
        n_topo.fetch_existing("subelements/connectivity").as_index_t_accessor();
    const auto subelements_sizes = n_topo.fetch_existing("subelements/sizes").as_index_t_accessor();
    const auto subelements_offsets =
        n_topo.fetch_existing("subelements/offsets").as_index_t_accessor();
    if(elements_sizes.max() != elements_sizes.min() || elements_sizes.max() != CONDUIT_NUM_FACES)
    {
        CONDUIT_ERROR("Polyhedral mesh elements/sizes indicate unsupported element type.");
    }
    if(subelements_sizes.max() != subelements_sizes.min() || subelements_sizes.max() != CONDUIT_NUM_VERTS)
    {
        CONDUIT_ERROR("Polyhedral mesh subelements/sizes indicate unsupported element type.");
    }

    const conduit::Node *n_coordset =
        conduit::blueprint::mesh::utils::find_reference_node(n_topo, "coordset");
    if(n_coordset == nullptr)
    {
        CONDUIT_ERROR("The topology's coordset could not be located.");
    }

    // Set up the output topology and allocate storage for the connectivity, sizes, offsets.
    const auto nElem = elements_sizes.number_of_elements();
    n_output_topo["type"] = "unstructured";
    n_output_topo["coordset"] = n_coordset->name();
    n_output_topo["elements/shape"] = std::string("hex");
    n_output_topo["elements/connectivity"].set(
        conduit::DataType(n_topo.fetch_existing("elements/connectivity").dtype().id(),
                          nElem * CONDUIT_VERTS_PER_HEX));
    n_output_topo["elements/sizes"].set(
        conduit::DataType(n_topo.fetch_existing("elements/sizes").dtype().id(), nElem));
    n_output_topo["elements/offsets"].set(
        conduit::DataType(n_topo.fetch_existing("elements/offsets").dtype().id(), nElem));
    auto out_connectivity = n_output_topo["elements/connectivity"].as_index_t_accessor();
    auto out_sizes = n_output_topo["elements/sizes"].as_index_t_accessor();
    auto out_offsets = n_output_topo["elements/offsets"].as_index_t_accessor();

    using IndexContainer = conduit::fixed_size_vector<conduit::index_t, 3>;

    /// Add a value to a vector if the value does not exist in the vector.
    auto appendIndex = [](IndexContainer &vec, conduit::index_t value) {
        if(std::find(vec.begin(), vec.end(), value) == vec.end())
        {
            vec.push_back(value);
        }
    };

    // Compute the face centers for all faces.
    std::vector<Vector> allFaceCenters, allFaceNormals;
    polyhedral_face_centers_normals<ExecPolicy>(*n_coordset,
                                                subelements_connectivity,
                                                subelements_sizes,
                                                subelements_offsets,
                                                allFaceCenters,
                                                allFaceNormals);

    // Compute all elem centers for all elems.
    std::vector<Vector> allElemCenters;
    polyhedral_elem_centers<ExecPolicy>(elements_connectivity,
                                        elements_sizes,
                                        elements_offsets,
                                        allFaceCenters,
                                        allElemCenters);

    // Fill in the output hex connectivity
    // NOTE: We're using value capture [=], mainly to avoid issues on Windows. This
    //       made us unable to use the accessors' set() method because it is not const.
    //       To compensate, we make a copy of the accessors for the time being.
    const Vector *allFaceCentersPtr = allFaceCenters.data();
    const Vector *allFaceNormalsPtr = allFaceNormals.data();
    const Vector *allElemCentersPtr = allElemCenters.data();
    conduit::execution::for_all<ExecPolicy>(0, nElem, [=](conduit::index_t i) {
        constexpr int FORWARD = 1;
        constexpr int BACKWARD = -1;
        // Determine face orientations with respect to this element.
        int orientation[] = {FORWARD, FORWARD, FORWARD, FORWARD, FORWARD, FORWARD};
        for(conduit::index_t f = 0; f < CONDUIT_NUM_FACES; f++)
        {
            const auto faceId = elements_connectivity[elements_offsets[i] + f];
            const Vector &faceNormal = allFaceNormalsPtr[faceId];
            const Vector &faceCenter = allFaceCentersPtr[faceId];
            const Vector outwardNormal = (faceCenter - allElemCentersPtr[i]).normalize();
            orientation[f] = (outwardNormal.dot(faceNormal) < 0.) ? BACKWARD : FORWARD;
        }

        // Order the points in the first face reversed with respect to orientation.
        // This is because we want the hex face ordering to be counter-clockwise when
        // viewed from the inside.
        conduit::index_t conn[CONDUIT_VERTS_PER_HEX];
        const auto firstFaceId = elements_connectivity[elements_offsets[i]];
        const auto firstFaceOffset = subelements_offsets[firstFaceId];
        if(orientation[0] == BACKWARD)
        {
            conn[0] = subelements_connectivity[firstFaceOffset];
            conn[1] = subelements_connectivity[firstFaceOffset + 1];
            conn[2] = subelements_connectivity[firstFaceOffset + 2];
            conn[3] = subelements_connectivity[firstFaceOffset + 3];
        }
        else
        {
            conn[0] = subelements_connectivity[firstFaceOffset + 3];
            conn[1] = subelements_connectivity[firstFaceOffset + 2];
            conn[2] = subelements_connectivity[firstFaceOffset + 1];
            conn[3] = subelements_connectivity[firstFaceOffset];
        }
        // Traverse the points in face 0 and add to the graph. These points that
        // make up face 0 will have 2 outbound graph edges. When we add more later
        // on another pass, the 3rd edge will be the one that connects the hex to
        // its other face.
        using MapType = conduit::fixed_size_map<conduit::index_t, IndexContainer, 8>;
        MapType graph;
        for(conduit::index_t vi = 0; vi < CONDUIT_NUM_VERTS; vi++)
        {
            conduit::index_t prevI = (vi == 0) ? 3 : (vi - 1);
            conduit::index_t nextI = (vi == CONDUIT_NUM_VERTS - 1) ? 0 : (vi + 1);
            auto &current = graph[conn[vi]];
            current.push_back(conn[prevI]);
            current.push_back(conn[nextI]);
        }
        // Traverse the rest of the faces and add to the graph.
        for(conduit::index_t f = 1; f < CONDUIT_NUM_FACES; f++)
        {
            const auto faceId = elements_connectivity[elements_offsets[i] + f];
            conduit::index_t faceVertexIds[CONDUIT_NUM_VERTS];
            const auto se_offset = (orientation[f] == FORWARD)
                ? subelements_offsets[faceId]
                : (subelements_offsets[faceId] + CONDUIT_NUM_VERTS - 1);

            for(conduit::index_t vi = 0; vi < CONDUIT_NUM_VERTS; vi++)
            {
                faceVertexIds[vi] = subelements_connectivity[se_offset + vi * orientation[f]];
            }

            for(conduit::index_t vi = 0; vi < CONDUIT_NUM_VERTS; vi++)
            {
                conduit::index_t prevI = (vi == 0) ? 3 : (vi - 1);
                conduit::index_t nextI = (vi == CONDUIT_NUM_VERTS - 1) ? 0 : (vi + 1);
                const auto currentId = faceVertexIds[vi];
                const auto prevId = faceVertexIds[prevI];
                const auto nextId = faceVertexIds[nextI];

                auto &current = graph[currentId];
                auto &prev = graph[prevId];
                auto &next = graph[nextId];

                appendIndex(current, prevId);
                appendIndex(current, nextId);
                appendIndex(prev, currentId);
                appendIndex(next, currentId);
            }
        }
        // Fill in the rest of the hex connectivity.
        conn[4] = graph.get(0)[2];
        conn[5] = graph.get(1)[2];
        conn[6] = graph.get(2)[2];
        conn[7] = graph.get(3)[2];

        // HACK: Make copies of the accessors so they are not const and we can use the set() method.
        auto out_connectivity_nc = out_connectivity;
        auto out_sizes_nc = out_sizes;
        auto out_offsets_nc = out_offsets;

        // Store it into the output arrays.
        const auto offset = i * CONDUIT_VERTS_PER_HEX;
        out_sizes_nc.set(i, CONDUIT_VERTS_PER_HEX);
        out_offsets_nc.set(i, offset);
        for(conduit::index_t vi = 0; vi < CONDUIT_VERTS_PER_HEX; vi++)
        {
            out_connectivity_nc.set(offset + vi, conn[vi]);
        }
    });
#if defined(_WIN32)
    #undef CONDUIT_NUM_FACES
    #undef CONDUIT_NUM_VERTS
    #undef CONDUIT_VERTS_PER_HEX
#endif
}

/*!
 * @brief Converts an input mesh from polygonal/polyhedral to quad/hex and also
 *        brings associated fields, matsets, adjsets, state to the output mesh.
 *
 * @param n_mesh The input mesh
 * @param n_options The options node that indicates which "topology" will be
 *                  operated on. If not specified, the first topology is used.
 *                  The "copy" attribute indicates whether to copy or set_external
 *                  data in the output mesh. The "policy" can be "omp" or "seq".
 * @param[out] n_output The node that will contain the output mesh.
 */
static void to_unstructured(const conduit::Node &n_mesh,
                            const conduit::Node &n_options,
                            conduit::Node &n_output)
{
    // Get a node designated by the options key, or the first one in the mesh if there are no options given.
    auto getNode = [](const conduit::Node &n_mesh2,
                      const conduit::Node &n_options2,
                      const std::string &rootName,
                      const std::string &key) {
        if(n_options2.has_path(key))
        {
            const std::string name = n_options2[key].as_string();
            return &n_mesh2[rootName + "/" + name];
        }
        return &(n_mesh2[rootName][0]);
    };

    // Determine whether we're copying or doing set_external (if possible).
    bool copy = true;
    if(n_options.has_path("copy"))
    {
        copy = (n_options["copy"].to_int() != 0);
    }
    // Determine whether input/output nodes are different. If they are different then
    // we need to copy input over to output. Otherwise, the inputs are already in the
    // output for most things.
    bool input_output_different = (&n_mesh != &n_output);

    // Get the topology and coordset
    const conduit::Node *n_topo = getNode(n_mesh, n_options, "topologies", "topology");
    if(n_topo == nullptr)
    {
        CONDUIT_ERROR("Could not find topology.");
    }
    const conduit::Node *n_coordset =
        bputils::find_reference_node(*n_topo, "coordset");
    if(n_coordset == nullptr)
    {
        CONDUIT_ERROR("Could not find coordset.");
    }

    // Check the topo type.
    const auto type = n_topo->fetch_existing("type").as_string();
    if(type != "unstructured")
    {
        CONDUIT_ERROR("to_unstructured called without unstructured mesh.");
    }
    const auto shape = n_topo->fetch_existing("elements/shape").as_string();
    if(shape == "polygonal")
    {
        const auto elements_sizes = n_topo->fetch_existing("elements/sizes").as_int_accessor();
        // Support all elements being either tri or quad.
        if(elements_sizes.max() != elements_sizes.min() ||
           (elements_sizes.max() != 4 && elements_sizes.max() != 3))
        {
            CONDUIT_ERROR("Polygonal mesh elements/sizes indicate unsupported element type.");
        }
        conduit::Node &n_output_topo = n_output["topologies/" + n_topo->name()];
        // If n_mesh and n_output are different nodes, copy coordset and topology to n_output.
        if(input_output_different)
        {
            n_output_topo.set(*n_topo);
        }
        // Force to quad from polygonal
        n_output_topo["elements/shape"] = (elements_sizes.max() == 4) ? "quad" : "tri";
    }
    else if(shape == "polyhedral")
    {
        conduit::Node &n_output_topo = n_output["topologies/" + n_topo->name()];

#if defined(CONDUIT_USE_OPENMP)
        // Let the user select an execution policy via options.
        std::string policy("seq");
        if(n_options.has_path("policy"))
        {
            const std::string p = n_options["policy"].as_string();
            if(p == "omp" || p == "seq") policy = p;
        }

        if(policy == "omp")
        {
            polyhedral_to_hexes<conduit::execution::OpenMPExec>(*n_topo, n_output_topo);
        }
        else
        {
            polyhedral_to_hexes<conduit::execution::SerialExec>(*n_topo, n_output_topo);
        }
#else
        polyhedral_to_hexes<conduit::execution::SerialExec>(*n_topo, n_output_topo);
#endif
    }
    else
    {
        if(input_output_different)
        {
            // It's just some other unstructured mesh. Copy to output node.
            copy_node(*n_topo, n_output["topologies/" + n_topo->name()], copy);
        }
    }

    // The input and output meshes are in different nodes so we must copy relevant
    // data over to the output node.
    if(input_output_different)
    {
        copy_node(*n_coordset, n_output["coordsets/" + n_coordset->name()], copy);

        copy_nodes_with_topology(n_mesh,
                              n_topo->name(),
                              n_output,
                              "fields",
                              copy,
                              get_selected_fields(n_options));
        copy_nodes_with_topology(n_mesh, n_topo->name(), n_output, "matsets", copy);
        copy_nodes_with_topology(n_mesh, n_topo->name(), n_output, "adjsets", copy);
        if(n_mesh.has_path("state"))
        {
            copy_node(n_mesh["state"], n_output["state"], copy);
        }
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::topology::unstructured::polytopal --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::topology::unstructured --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::topology --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh --
//-----------------------------------------------------------------------------

void mesh::convert(const conduit::Node &n_mesh,
                   const conduit::Node &n_options,
                   conduit::Node &n_output,
                   conduit::Node &n_maps)
{
    const auto domains = conduit::blueprint::mesh::domains(n_mesh);

    // Select the topology on which to operate.
    std::string topologyName;
    if(n_options.has_path("topology"))
    {
        topologyName = n_options["topology"].as_string();
    }
    else
    {
        const conduit::Node &n_topologies = domains[0]->fetch_existing("topologies");
        topologyName = n_topologies[0].name();
        if(n_topologies.number_of_children() > 1)
        {
            CONDUIT_INFO(conduit_fmt::format(
                "There is more than one possible topology in the mesh. The first one, \"{}\", will "
                "be used. Consider passing a \"topology\" name in the options.",
                topologyName));
        }
    }

    // Determine whether we're copying or doing set_external (if possible).
    bool copy = true;
    if(n_options.has_path("copy"))
    {
        copy = (n_options["copy"].to_int() != 0);
    }

    // Get the target
    std::string target("unstructured");
    if(n_options.has_path("target"))
    {
        target = n_options["target"].as_string();
    }

    const int FIELDS_MASK = 1;
    const int MATSETS_MASK = 2;
    const int ADJSETS_MASK = 4;
    int copyMask = 0;

    if(domains.size() > 1)
    {
        // Iterate over the domains and convert each one.
        for(size_t i = 0; i < domains.size(); i++)
        {
            convert(*domains[i], n_options, n_output.append(), n_maps.append());
        }
    }
    else
    {
        const conduit::Node &n_topo = n_mesh["topologies/" + topologyName];
        const conduit::Node *n_coordset =
            bputils::find_reference_node(n_topo, "coordset");
        if(n_coordset == nullptr)
        {
            CONDUIT_ERROR("Could not locate coordset.");
        }

        const std::string type = n_topo["type"].as_string();
        if(target == "unstructured")
        {
            // We want to convert the mesh to unstructured.
            if(type == "unstructured")
            {
                // We have an unstructured mesh.
                const std::string shape = n_topo["elements/shape"].as_string();
                const bool degrade_polytopes = n_options.has_path("degrade_polytopes")
                    ? (n_options["degrade_polytopes"].to_int() > 0)
                    : false;
                const bool isPolygonal = shape == "polygonal" && degrade_polytopes;
                const bool isPolyhedral = shape == "polyhedral" && degrade_polytopes;
                if(isPolygonal || isPolyhedral)
                {
                    // Convert polytopal BACK to unstructured.
                    conduit::blueprint::mesh::topology::unstructured::polytopal::to_unstructured(
                        n_mesh,
                        n_options,
                        n_output);
                }
                else
                {
                    copy_node(n_topo, n_output["topologies/" + topologyName], copy);
                    copy_node(*n_coordset, n_output["coordsets/" + n_coordset->name()], copy);
                    copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
                }
            }
            else
            {
                conduit::Node &topo_dest = n_output["topologies/" + topologyName];
                conduit::Node &coords_dest = n_output["coordsets/" + n_coordset->name()];
                if(type == "uniform")
                {
                    conduit::blueprint::mesh::topology::uniform::to_unstructured(n_topo,
                                                                                 topo_dest,
                                                                                 coords_dest);
                    copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
                }
                else if(type == "rectilinear")
                {
                    conduit::blueprint::mesh::topology::rectilinear::to_unstructured(n_topo,
                                                                                     topo_dest,
                                                                                     coords_dest);
                    copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
                }
                else if(type == "structured")
                {
                    conduit::blueprint::mesh::topology::structured::to_unstructured(n_topo,
                                                                                    topo_dest,
                                                                                    coords_dest);
                    copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
                }
                else
                {
                    CONDUIT_ERROR(conduit_fmt::format("No conversion for {} to {}.", type, target));
                }
            }
        }
        else if(target == "uniform")
        {
            if(type == "uniform")
            {
                copy_node(n_topo, n_output["topologies/" + topologyName], copy);
                copy_node(*n_coordset, n_output["coordsets/" + n_coordset->name()], copy);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
            else
            {
                CONDUIT_ERROR(conduit_fmt::format("No conversion for {} to {}.", type, target));
            }
        }
        else if(target == "rectilinear")
        {
            if(type == "uniform")
            {
                conduit::Node &topo_dest = n_output["topologies/" + topologyName];
                conduit::Node &coords_dest = n_output["coordsets/" + n_coordset->name()];
                conduit::blueprint::mesh::topology::uniform::to_rectilinear(n_topo,
                                                                            topo_dest,
                                                                            coords_dest);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
            else if(type == "rectilinear")
            {
                copy_node(n_topo, n_output["topologies/" + topologyName], copy);
                copy_node(*n_coordset, n_output["coordsets/" + n_coordset->name()], copy);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
            else
            {
                CONDUIT_ERROR(conduit_fmt::format("No conversion for {} to {}.", type, target));
            }
        }
        else if(target == "structured")
        {
            if(type == "uniform")
            {
                conduit::Node &topo_dest = n_output["topologies/" + topologyName];
                conduit::Node &coords_dest = n_output["coordsets/" + n_coordset->name()];
                conduit::blueprint::mesh::topology::uniform::to_structured(n_topo,
                                                                           topo_dest,
                                                                           coords_dest);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
            else if(type == "rectilinear")
            {
                conduit::Node &topo_dest = n_output["topologies/" + topologyName];
                conduit::Node &coords_dest = n_output["coordsets/" + n_coordset->name()];
                conduit::blueprint::mesh::topology::rectilinear::to_structured(n_topo,
                                                                               topo_dest,
                                                                               coords_dest);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
            else if(type == "structured")
            {
                copy_node(n_topo, n_output["topologies/" + topologyName], copy);
                copy_node(*n_coordset, n_output["coordsets/" + n_coordset->name()], copy);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
            else
            {
                CONDUIT_ERROR(conduit_fmt::format("to_structured not implemented for {}.", type));
            }
        }
        else if(target == "polytopal")
        {
            if(type == "unstructured")
            {
                conduit::Node &topo_dest = n_output["topologies/" + topologyName];
                conduit::blueprint::mesh::topology::unstructured::to_polytopal(n_topo, topo_dest);
                copy_node(*n_coordset, n_output["coordsets/" + n_coordset->name()], copy);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
            else
            {
                // Convert to unstructured first.
                conduit::Node options_copy(n_options);
                options_copy["target"] = "unstructured";
                options_copy["copy"] = 0;  // Use set_external when possible
                conduit::Node n_mesh_uns, tmp;
                convert(n_mesh, options_copy, n_mesh_uns, tmp);

                const conduit::Node &n_topo_uns = n_mesh_uns["topologies/" + topologyName];
                const conduit::Node &n_coordset_uns = n_mesh_uns["coordsets/" + n_coordset->name()];

                conduit::Node &topo_dest = n_output["topologies/" + topologyName];
                conduit::blueprint::mesh::topology::unstructured::to_polytopal(n_topo_uns, topo_dest);
                copy_node(n_coordset_uns, n_output["coordsets/" + n_coordset->name()], true);
                copyMask = FIELDS_MASK | MATSETS_MASK | ADJSETS_MASK;
            }
        }
        else if(target.find("generate_") == 0)
        {
            const conduit::Node *n_input = &n_mesh;
            // If the input mesh to any of these "generate_" targets is not unstructured,
            // recurse and make it unstructured.
            conduit::Node n_mesh_uns, tmp;
            if(type != "unstructured")
            {
                conduit::Node options_copy(n_options);
                options_copy["target"] = "unstructured";
                options_copy["copy"] = 0;  // Use set_external when possible
                convert(n_mesh, options_copy, n_mesh_uns, tmp);
                n_input = &n_mesh_uns;
            }

            const conduit::Node &n_input_topo = n_input->fetch_existing("topologies/" + topologyName);
            conduit::Node &n_output_topo = n_output["topologies/" + topologyName];
            if(target == "generate_points")
            {
                conduit::blueprint::mesh::topology::unstructured::generate_points(n_input_topo,
                                                                                  n_output_topo,
                                                                                  n_maps["s2dmap"],
                                                                                  n_maps["d2smap"]);
                const conduit::Node *n_coordset_uns =
                    bputils::find_reference_node(n_input_topo, "coordset");
                copy_node(*n_coordset_uns, n_output["coordsets/" + n_coordset_uns->name()], copy);
            }
            else if(target == "generate_lines")
            {
                conduit::blueprint::mesh::topology::unstructured::generate_lines(n_input_topo,
                                                                                 n_output_topo,
                                                                                 n_maps["s2dmap"],
                                                                                 n_maps["d2smap"]);
                const conduit::Node *n_coordset_uns =
                    bputils::find_reference_node(n_input_topo, "coordset");
                copy_node(*n_coordset_uns, n_output["coordsets/" + n_coordset_uns->name()], copy);
            }
            else if(target == "generate_faces")
            {
                conduit::blueprint::mesh::topology::unstructured::generate_faces(n_input_topo,
                                                                                 n_output_topo,
                                                                                 n_maps["s2dmap"],
                                                                                 n_maps["d2smap"]);
                const conduit::Node *n_coordset_uns =
                    bputils::find_reference_node(n_input_topo, "coordset");
                copy_node(*n_coordset_uns, n_output["coordsets/" + n_coordset_uns->name()], copy);
            }
            else if(target == "generate_centroids")
            {
                // NOTE: Use the same coordset name as the original mesh.
                conduit::blueprint::mesh::topology::unstructured::generate_centroids(
                    n_input_topo,
                    n_output["topologies/" + topologyName],
                    n_output["coordsets/" + n_coordset->name()],
                    n_maps["s2dmap"],
                    n_maps["d2smap"]);
            }
            else if(target == "generate_sides")
            {
                // NOTE: Use the same coordset name as the original mesh.
                conduit::blueprint::mesh::topology::unstructured::generate_sides(
                    n_input_topo,
                    n_output["topologies/" + topologyName],
                    n_output["coordsets/" + n_coordset->name()],
                    n_maps["s2dmap"],
                    n_maps["d2smap"]);
            }
            else if(target == "generate_corners")
            {
                // NOTE: Use the same coordset name as the original mesh.
                conduit::blueprint::mesh::topology::unstructured::generate_corners(
                    n_input_topo,
                    n_output["topologies/" + topologyName],
                    n_output["coordsets/" + n_coordset->name()],
                    n_maps["s2dmap"],
                    n_maps["d2smap"]);
            }
            else
            {
                CONDUIT_ERROR(conduit_fmt::format("Unsupported target {}.", target));
            }
        }
        else
        {
            CONDUIT_ERROR(conduit_fmt::format("Unsupported target {}.", target));
        }

        // Copy some objects from the input mesh to the output mesh.
        if(copyMask & FIELDS_MASK)
        {
            copy_nodes_with_topology(n_mesh,
                                  topologyName,
                                  n_output,
                                  "fields",
                                  copy,
                                  get_selected_fields(n_options));
        }
        if(copyMask & MATSETS_MASK)
        {
            copy_nodes_with_topology(n_mesh, topologyName, n_output, "matsets", copy);
        }
        if(copyMask & ADJSETS_MASK)
        {
            copy_nodes_with_topology(n_mesh, topologyName, n_output, "adjsets", copy);
        }
        if(n_mesh.has_path("state"))
        {
            copy_node(n_mesh["state"], n_output["state"], copy);
        }
    }
}

void mesh::convert(const conduit::Node &n_mesh, const conduit::Node &n_options, conduit::Node &n_output)
{
    conduit::Node n_maps;
    mesh::convert(n_mesh, n_options, n_output, n_maps);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

