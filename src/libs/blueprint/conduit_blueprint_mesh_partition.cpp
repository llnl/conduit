// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_partition.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh_partition.hpp"

//-----------------------------------------------------------------------------
// std lib includes
//-----------------------------------------------------------------------------
#include <algorithm>
#include <array>
#include <deque>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <set>
#include <vector>
#include <cstddef>
#include <type_traits>
#include <map>
#include <unordered_set>
#include <numeric>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_node.hpp"
#include "conduit_data_accessor.hpp"
#include "conduit_geometry_vector.hpp"
#include "conduit_blueprint_mcarray.hpp"
#include "conduit_blueprint_o2mrelation.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_mesh_utils_iterate_elements.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_o2mrelation_iterator.hpp"
#include "conduit_log.hpp"

#include "conduit_fmt/conduit_fmt.h"

// Uncomment to enable some debugging output from partitioner.
// #define CONDUIT_DEBUG_PARTITIONER

// #define CONDUIT_DEBUG_PARTITIONER_MAPBACK
// #define CONDUIT_DEBUG_PARTITIONER_MAPBACK_VERBOSE

// Toggles debug prints for point merge
// #define DEBUG_POINT_MERGE
#ifndef DEBUG_POINT_MERGE
#define PM_DEBUG_PRINT(stream)
#else
#define PM_DEBUG_PRINT(stream) do { std::cerr << stream; } while(0)
#endif

// Toggles debug prints for structured combine
// #define DEBUG_STRUCTURED_COMBINE

using index_t=conduit::index_t;
using std::cout;
using std::endl;

extern void grid_ijk_to_id(const index_t *ijk, const index_t *dims, index_t &grid_id);
extern void grid_id_to_ijk(const index_t id, const index_t *dims, index_t *grid_ijk);

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint --
//-----------------------------------------------------------------------------
namespace blueprint
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh --
//-----------------------------------------------------------------------------
namespace mesh
{

//-------------------------------------------------------------------------
static index_t
get_index_t(const conduit::Node &n, bool &ok)
{
    ok = true;
    index_t retval = 0;
    if(n.dtype().is_int8())
        retval = static_cast<index_t>(n.as_int8());
    else if(n.dtype().is_int16())
        retval = static_cast<index_t>(n.as_int16());
    else if(n.dtype().is_int32())
        retval = static_cast<index_t>(n.as_int32());
    else if(n.dtype().is_int64())
        retval = static_cast<index_t>(n.as_int64());
    else if(n.dtype().is_uint8())
        retval = static_cast<index_t>(n.as_uint8());
    else if(n.dtype().is_uint16())
        retval = static_cast<index_t>(n.as_uint16());
    else if(n.dtype().is_uint32())
        retval = static_cast<index_t>(n.as_uint32());
    else if(n.dtype().is_uint64())
        retval = static_cast<index_t>(n.as_uint64());
    else
        ok = false;
    return retval;
}

//---------------------------------------------------------------------------
inline void
as_index_t(const conduit::Node &n, conduit::Node &out)
{
#ifdef CONDUIT_INDEX_32
    n.to_unsigned_int_array(out);
#else
    n.to_uint64_array(out);
#endif
}

//---------------------------------------------------------------------------
#ifdef CONDUIT_INDEX_32
inline conduit::unsigned_int_array
as_index_t_array(const conduit::Node &n)
{
    return n.as_unsigned_int_array();
}
#else
inline conduit::uint64_array
as_index_t_array(const conduit::Node &n)
{
    return n.as_uint64_array();
}
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
const std::string Selection::DOMAIN_KEY("domain_id");
const std::string Selection::TOPOLOGY_KEY("topology");

//---------------------------------------------------------------------------
Selection::Selection()
: whole(Selection::WHOLE_UNDETERMINED),
  domain(0),
  topology(),
  domain_any(false)
{
}

//---------------------------------------------------------------------------
Selection::Selection(const Selection &obj)
: whole(obj.whole),
  domain(obj.domain),
  topology(obj.topology),
  domain_any(obj.domain_any)
{
}

//---------------------------------------------------------------------------
Selection::~Selection()
{
}

//---------------------------------------------------------------------------
index_t
Selection::length(const conduit::Node &/*n_mesh*/) const
{
    return 0;
}

//---------------------------------------------------------------------------
bool
Selection::get_whole(const conduit::Node &n_mesh)
{
    // If we have not yet determined whether the selection appears whole,
    // do it now.
    if(whole == WHOLE_UNDETERMINED)
        set_whole(determine_is_whole(n_mesh));

    return whole == WHOLE_DETERMINED_TRUE;
}

//---------------------------------------------------------------------------
void
Selection::set_whole(bool value)
{
    whole = value ? WHOLE_DETERMINED_TRUE : WHOLE_DETERMINED_FALSE;
}

//---------------------------------------------------------------------------
bool
Selection::requires_initial_partition() const
{
    return false;
}

//---------------------------------------------------------------------------
index_t
Selection::get_domain() const
{
    return domain;
}

//---------------------------------------------------------------------------
void
Selection::set_domain(index_t value)
{
    domain = value;
}

//---------------------------------------------------------------------------
const std::string &
Selection::get_topology() const
{
    return topology;
}

//---------------------------------------------------------------------------
void
Selection::set_topology(const std::string &value)
{
    topology = value;
}

//---------------------------------------------------------------------------
int
Selection::get_destination_rank() const
{
    return FREE_RANK_ID;
}

//---------------------------------------------------------------------------
int
Selection::get_destination_domain() const
{
    return FREE_DOMAIN_ID;
}

//---------------------------------------------------------------------------
bool
Selection::init(const conduit::Node &n_options)
{
    bool retval = true;

    try
    {
        if(n_options.has_child(DOMAIN_KEY))
        {
            const conduit::Node &n_dk = n_options[DOMAIN_KEY];
            if(n_dk.dtype().is_string())
            {
                if(n_dk.as_string() == "any" && supports_domain_any())
                {
                    domain_any = true;
                    domain = 0;
                }
                else
                {
                    // domain is not allowed to be "any".
                    retval = false;
                }
            }
            else
            {
                bool ok = false;
                index_t tmp = get_index_t(n_dk, ok);
                if(ok)
                    domain = tmp;
            }
        }

        if(n_options.has_child(TOPOLOGY_KEY))
            topology = n_options[TOPOLOGY_KEY].as_string();
    }
    catch(const conduit::Error &/*e*/)
    {
        //e.print();
        retval = false;
    }

    return retval;
}

//---------------------------------------------------------------------------
const conduit::Node &
Selection::selected_topology(const conduit::Node &n_mesh) const
{
    if(n_mesh.has_child("topologies"))
    {
        const conduit::Node &n_topos = n_mesh["topologies"];
        if(topology.empty())
            return n_topos[0];
        else if(n_topos.has_child(topology))
            return n_topos[topology];
    }

    std::stringstream oss;
    oss << "The input mesh does not contain a topology with name " << topology;
    CONDUIT_ERROR(oss.str());
    throw conduit::Error(oss.str(), __FILE__, __LINE__);
}

//---------------------------------------------------------------------------
bool
Selection::get_domain_any() const
{
    return domain_any;
}

//---------------------------------------------------------------------------
bool
Selection::supports_domain_any() const
{
    return false;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/**
 @brief This class represents a logical IJK selection with start and end
        values. Start begins at 0 and End is the size of the mesh (in terms
        of cells) minus 1.

        A cell with 10x10x10 cells would have the following selection to
        select it all: start={0,0,0}, end={9,9,9}. To select a single cell,
        make start the same as end.
*/
class SelectionLogical : public Selection
{
public:
    SelectionLogical();
    SelectionLogical(const SelectionLogical &);
    virtual ~SelectionLogical();

    static std::string name() { return "logical"; }

    virtual std::shared_ptr<Selection> copy() const override;

    // Initializes the selection from a conduit::Node.
    virtual bool init(const conduit::Node &n_options) override;

    virtual bool applicable(const conduit::Node &n_mesh) override;

    // Computes the number of cells in the selection.
    virtual index_t length(const conduit::Node &/*n_mesh*/) const override
    {
        return cells_for_axis(0) *
               cells_for_axis(1) *
               cells_for_axis(2);
    }

    virtual std::vector<std::shared_ptr<Selection> > partition(const conduit::Node &n_mesh) const override;

    void get_start(index_t &s0, index_t &s1, index_t &s2) const
    {
        s0 = start[0];
        s1 = start[1];
        s2 = start[2];
    }

    void set_start(index_t s0, index_t s1, index_t s2)
    {
        start[0] = s0;
        start[1] = s1;
        start[2] = s2;
    }

    void get_end(index_t &e0, index_t &e1, index_t &e2) const
    {
        e0 = end[0];
        e1 = end[1];
        e2 = end[2];
    }

    void set_end(index_t e0, index_t e1, index_t e2)
    {
        end[0] = e0;
        end[1] = e1;
        end[2] = e2;
    }

    virtual void get_element_ids(const conduit::Node &n_mesh,
                                 std::vector<index_t> &element_ids) const override;

    void get_vertex_ids(const conduit::Node &n_mesh,
                        std::vector<index_t> &element_ids) const;

    virtual void print(std::ostream &os) const override;

protected:
    virtual bool determine_is_whole(const conduit::Node &n_mesh) const override;

    index_t cells_for_axis(int axis) const
    {
        index_t nc = static_cast<index_t>(end[axis] - start[axis] + 1);
        return (axis >= 0 && axis <= 2) ? nc : 0;
    }

    static const std::string START_KEY;
    static const std::string END_KEY;

    index_t start[3];
    index_t end[3];
};

const std::string SelectionLogical::START_KEY("start");
const std::string SelectionLogical::END_KEY("end");

//---------------------------------------------------------------------------
SelectionLogical::SelectionLogical() : Selection()
{
    start[0] = start[1] = start[2] = 0;
    end[0] = end[1] = end[2] = 0;
}

//---------------------------------------------------------------------------
SelectionLogical::SelectionLogical(const SelectionLogical &obj) : Selection(obj)
{
    for(int i = 0; i < 3; i++)
    {
        start[i] = obj.start[i];
        end[i] = obj.end[i];
    }
}

//---------------------------------------------------------------------------
SelectionLogical::~SelectionLogical()
{
}

//---------------------------------------------------------------------------
std::shared_ptr<Selection>
SelectionLogical::copy() const
{
    return std::make_shared<SelectionLogical>(*this);
}

//---------------------------------------------------------------------------
bool
SelectionLogical::init(const conduit::Node &n_options)
{
    bool retval = false;
    if(Selection::init(n_options))
    {
        if(n_options.has_child(START_KEY) && n_options.has_child(END_KEY))
        {
            conduit::Node n_s, n_e;
            n_options[START_KEY].to_uint64_array(n_s);
            n_options[END_KEY].to_uint64_array(n_e);

            auto s = n_s.as_uint64_array();
            auto e = n_e.as_uint64_array();
            if(s.number_of_elements() == 3 &&  e.number_of_elements() == 3)
            {
                for(int i = 0; i < 3; i++)
                {
                    start[i] = static_cast<index_t>(s[i]);
                    end[i] = static_cast<index_t>(e[i]);
                }
                retval = true;
            }
        }
    }
    return retval;
}

//---------------------------------------------------------------------------
/**
 @brief Returns whether the logical selection applies to the input mesh.
 @note The method is not const because it clamps the end values.
 */
bool
SelectionLogical::applicable(const conduit::Node &n_mesh)
{
    bool retval = false;
    try
    {
        // Get the selected topology and coordset.
        const conduit::Node &n_topo = selected_topology(n_mesh);
        std::string csname(n_topo["coordset"].as_string());
        const conduit::Node &n_coords = n_mesh["coordsets"][csname];

        bool is_uniform = n_coords["type"].as_string() == "uniform";
        bool is_rectilinear = n_coords["type"].as_string() == "rectilinear";
        bool is_structured = n_coords["type"].as_string() == "explicit" &&
                             n_topo["type"].as_string() == "structured";
        if(is_uniform || is_rectilinear || is_structured)
        {
            index_t dims[3] = {1,1,1};
            conduit::blueprint::mesh::utils::topology::logical_dims(n_topo, dims, 3);

            // See that the selection starts inside the dimensions.
            if(start[0] < dims[0] && start[1] < dims[1] && start[2] < dims[2])
            {
                // Clamp the selection to the dimensions of the mesh.
                end[0] = std::min(end[0], dims[0]-1);
                end[1] = std::min(end[1], dims[1]-1);
                end[2] = std::min(end[2], dims[2]-1);

                retval = true;
            }
        }
    }
    catch(conduit::Error &)
    {
        retval = false;
    }

    return retval;
}

//---------------------------------------------------------------------------
bool
SelectionLogical::determine_is_whole(const conduit::Node &n_mesh) const
{
    bool retval = false;
    try
    {
        // Get the selected topology and coordset.
        const conduit::Node &n_topo = selected_topology(n_mesh);
        index_t len = conduit::blueprint::mesh::utils::topology::length(n_topo);
        retval = len == length(n_mesh);
    }
    catch(conduit::Error &)
    {
        retval = false;
    }

    return retval;
}

//---------------------------------------------------------------------------
/**
 @brief Partitions along the longest axis and returns a vector containing 2
        logical selections.
 */
std::vector<std::shared_ptr<Selection> >
SelectionLogical::partition(const conduit::Node &n_mesh) const
{
    std::vector<std::shared_ptr<Selection> > parts;
    if(length(n_mesh) > 1)
    {
        int la = 0;
        if(cells_for_axis(1) > cells_for_axis(la))
            la = 1;
        if(cells_for_axis(2) > cells_for_axis(la))
            la = 2;
        auto n = cells_for_axis(la);

        auto p0 = std::make_shared<SelectionLogical>();
        auto p1 = std::make_shared<SelectionLogical>();
        p0->set_whole(false);
        p1->set_whole(false);
        p0->set_domain(domain);
        p1->set_domain(domain);
        p0->set_topology(topology);
        p1->set_topology(topology);

        if(la == 0)
        {
            p0->set_start(start[0],       start[1],       start[2]);
            p0->set_end(start[0]+n/2-1,   end[1],         end[2]);
            p1->set_start(start[0]+n/2,   start[1],       start[2]);
            p1->set_end(end[0],           end[1],         end[2]);
        }
        else if(la == 1)
        {
            p0->set_start(start[0],       start[1],       start[2]);
            p0->set_end(end[0],           start[1]+n/2-1, end[2]);
            p1->set_start(start[0],       start[1]+n/2,   start[2]);
            p1->set_end(end[0],           end[1],         end[2]);
        }
        else
        {
            p0->set_start(start[0],       start[1],       start[2]);
            p0->set_end(end[0],           end[1],         start[2]+n/2-1);
            p1->set_start(start[0],       start[1],       start[2]+n/2);
            p1->set_end(end[0],           end[1],         end[2]);
        }
#if 0
cout << "****\nSelectionLogical::partition: la=" << la << endl << "\t";
p0->print(cout);
cout << endl << "\t";
p1->print(cout);
cout << endl << "****" << endl;
#endif
        parts.push_back(p0);
        parts.push_back(p1);
    }
    return parts;
}

//---------------------------------------------------------------------------
void
SelectionLogical::get_vertex_ids(const conduit::Node &n_mesh,
    std::vector<index_t> &ids) const
{
    try
    {
        const conduit::Node &n_topo = selected_topology(n_mesh);
        index_t dims[3] = {1,1,1};
        conduit::blueprint::mesh::utils::topology::logical_dims(n_topo, dims, 3);
        index_t ndims = conduit::blueprint::mesh::utils::topology::dims(n_topo);
        dims[0]++;
        dims[1]++;
        if(ndims > 2)
            dims[2]++;

        ids.clear();
        ids.reserve(dims[0] * dims[1] * dims[2]);
        auto mesh_NXNY = dims[0] * dims[1];
        auto mesh_NX   = dims[0];
        index_t n_end[3];
        n_end[0] = end[0] + 1;
        n_end[1] = end[1] + 1;
        n_end[2] = (ndims > 2) ? (end[2] + 1) : start[2];

        for(index_t k = start[2]; k <= n_end[2]; k++)
        for(index_t j = start[1]; j <= n_end[1]; j++)
        for(index_t i = start[0]; i <= n_end[0]; i++)
        {
            index_t id = k*mesh_NXNY + j*mesh_NX + i;
            ids.push_back(id);
        }
    }
    catch(conduit::Error &)
    {
    }
}

//---------------------------------------------------------------------------
void
SelectionLogical::get_element_ids(const conduit::Node &n_mesh,
    std::vector<index_t> &element_ids) const
{
    try
    {
        const conduit::Node &n_topo = selected_topology(n_mesh);
        index_t dims[3] = {1,1,1};
        conduit::blueprint::mesh::utils::topology::logical_dims(n_topo, dims, 3);

        element_ids.clear();
        element_ids.reserve(length(n_mesh));
        auto mesh_CXCY = dims[0] * dims[1];
        auto mesh_CX   = dims[0];
        for(index_t k = start[2]; k <= end[2]; k++)
        for(index_t j = start[1]; j <= end[1]; j++)
        for(index_t i = start[0]; i <= end[0]; i++)
        {
            auto eid = k*mesh_CXCY + j*mesh_CX + i;
            element_ids.push_back(eid);
        }
    }
    catch(conduit::Error &)
    {
    }
}

//---------------------------------------------------------------------------
void
SelectionLogical::print(std::ostream &os) const
{
    os << "{"
       << "\"name\":\"" << name() << "\","
       << "\"domain\":" << get_domain() << ", "
       << "\"topology\":\"" << get_topology() << "\", "
       << "\"start\":[" << start[0] << ", " << start[1] << ", " << start[2] << "],"
       << "\"end\":[" << end[0] << ", " << end[1] << ", " << end[2] << "]"
       << "}";
}

//---------------------------------------------------------------------------
/**
   @brief This selection explicitly defines which cells we're pulling out from
          a mesh, and in which order.
 */
class SelectionExplicit : public Selection
{
public:
    SelectionExplicit();
    SelectionExplicit(const SelectionExplicit &obj);
    virtual ~SelectionExplicit();

    static std::string name() { return "explicit"; }

    virtual std::shared_ptr<Selection> copy() const override;

    virtual bool init(const conduit::Node &n_options) override;

    virtual bool applicable(const conduit::Node &n_mesh) override;

    // Computes the number of cells in the selection.
    virtual index_t length(const conduit::Node &/*n_mesh*/) const override
    {
        return ids_storage.dtype().number_of_elements();
    }

    virtual std::vector<std::shared_ptr<Selection> > partition(const conduit::Node &n_mesh) const override;

    const index_t *get_indices() const
    {
        // Access the converted data as index_t.
        return reinterpret_cast<const index_t *>(ids_storage.data_ptr());
    }

    void set_indices(const conduit::Node &value)
    {
        ids_storage.reset();
        as_index_t(value, ids_storage);
    }

    void set_indices(const std::vector<index_t> &value)
    {
        ids_storage.set(value);
    }

    index_t num_indices() const { return ids_storage.dtype().number_of_elements(); }

    virtual void get_element_ids(const conduit::Node &n_mesh,
                                 std::vector<index_t> &element_ids) const override;

    virtual void print(std::ostream &os) const override;

protected:
    virtual bool determine_is_whole(const conduit::Node &n_mesh) const override;

    static const std::string ELEMENTS_KEY;
    conduit::Node ids_storage;
};

const std::string SelectionExplicit::ELEMENTS_KEY("elements");

//---------------------------------------------------------------------------
SelectionExplicit::SelectionExplicit()
: Selection(),
  ids_storage()
{
}

//---------------------------------------------------------------------------
SelectionExplicit::SelectionExplicit(const SelectionExplicit &obj)
: Selection(obj),
  ids_storage(obj.ids_storage)
{
}

//---------------------------------------------------------------------------
SelectionExplicit::~SelectionExplicit()
{
}

//---------------------------------------------------------------------------
std::shared_ptr<Selection>
SelectionExplicit::copy() const
{
    return std::make_shared<SelectionExplicit>(*this);
}

//---------------------------------------------------------------------------
bool
SelectionExplicit::init(const conduit::Node &n_options)
{
    bool retval = false;
    if(Selection::init(n_options))
    {
        if(n_options.has_child(ELEMENTS_KEY))
        {
            const conduit::Node &n_elem = n_options[ELEMENTS_KEY];
            if(n_elem.dtype().is_number())
            {
                // Convert to the right type for index_t. This may copy
                // but that's ok since we access through ids_storage since
                // we may later partition that array.
                as_index_t(n_elem, ids_storage);
                retval = true;
            }
        }
    }
    return retval;
}

//---------------------------------------------------------------------------
/**
 @brief Returns whether the explicit selection applies to the input mesh.
 */
bool
SelectionExplicit::applicable(const conduit::Node &/*n_mesh*/)
{
    return true;
}

//---------------------------------------------------------------------------
bool
SelectionExplicit::determine_is_whole(const conduit::Node &n_mesh) const
{
    bool is_whole = false;
    try
    {
        // Get the selected topology and coordset.
        const conduit::Node &n_topo = selected_topology(n_mesh);
        auto num_elem_in_mesh = topology::length(n_topo);
        auto n = num_indices();
        if(n == num_elem_in_mesh)
        {
            auto indices = get_indices();
            std::set<index_t> unique;
            for(index_t i = 0; i < n; i++)
                unique.insert(indices[i]);
            is_whole = static_cast<index_t>(unique.size()) == num_elem_in_mesh;

            // If the mesh is whole, check that the indices are all in ascending
            // order. If not, we're reordering and we should not consider the
            // chunk available for passing through whole.
            if(n > 1)
            {
                for(index_t i = 1; i < n && is_whole; i++)
                {
                    is_whole &= (indices[i - 1] < indices[i]);
                }
            }
        }
    }
    catch(conduit::Error &)
    {
        is_whole = false;
    }

    return is_whole;
}

//---------------------------------------------------------------------------
std::vector<std::shared_ptr<Selection> >
SelectionExplicit::partition(const conduit::Node &n_mesh) const
{
    // Get the number of elements in the topology.
    index_t num_elem_in_topo = 0;
    try
    {
        // Get the selected topology and coordset.
        const conduit::Node &n_topo = selected_topology(n_mesh);
        num_elem_in_topo = topology::length(n_topo);
    }
    catch(conduit::Error &)
    {
    }

    // Divide the elements into 2 vectors.
    auto n = num_indices();
    auto n_2 = n/2;
    auto indices = get_indices();
    std::vector<index_t> ids0, ids1;
    ids0.reserve(n_2);
    ids1.reserve(n_2);
    for(index_t i = 0; i < n; i++)
    {
        if(indices[i] < num_elem_in_topo)
        {
            if(i < n_2)
                ids0.push_back(indices[i]);
            else
                ids1.push_back(indices[i]);
        }
    }

    // Make partitioned selections.
    auto p0 = std::make_shared<SelectionExplicit>();
    auto p1 = std::make_shared<SelectionExplicit>();
    p0->ids_storage.set(ids0);
    p1->ids_storage.set(ids1);
    p0->set_whole(false);
    p1->set_whole(false);
    p0->set_domain(domain);
    p1->set_domain(domain);
    p0->set_topology(topology);
    p1->set_topology(topology);

    std::vector<std::shared_ptr<Selection> > parts;
    parts.push_back(p0);
    parts.push_back(p1);

    return parts;
}

//---------------------------------------------------------------------------
void
SelectionExplicit::get_element_ids(const conduit::Node &n_mesh,
    std::vector<index_t> &element_ids) const
{
    try
    {
        const conduit::Node &n_topo = selected_topology(n_mesh);
        auto topolen = topology::length(n_topo);

        auto n = ids_storage.dtype().number_of_elements();
        auto indices = get_indices();
        element_ids.reserve(n);
        for(index_t i = 0; i < n; i++)
        {
            auto eid = indices[i];
            if(eid < topolen)
                element_ids.push_back(eid);
        }
    }
    catch(conduit::Error &)
    {
    }
}

//---------------------------------------------------------------------------
void
SelectionExplicit::print(std::ostream &os) const
{
    os << "{"
       << "\"name\":\"" << name() << "\","
       << "\"domain\":" << get_domain() << ", "
       << "\"topology\":\"" << get_topology() << "\", "
       << "\"elements\":[";
    auto n = num_indices();
    auto indices = get_indices();
    for(index_t i = 0; i < n; i++)
    {
        if(i > 0)
            os << ", ";
        os << indices[i];
    }
    os << "]}";
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class SelectionRanges : public Selection
{
public:
    SelectionRanges();
    SelectionRanges(const SelectionRanges &obj);
    virtual ~SelectionRanges();

    static std::string name() { return "ranges"; }

    virtual std::shared_ptr<Selection> copy() const override;

    virtual bool init(const conduit::Node &n_options) override;

    virtual bool applicable(const conduit::Node &n_mesh) override;

    // Computes the number of cells in the selection.
    virtual index_t length(const conduit::Node &/*n_mesh*/) const override;

    virtual std::vector<std::shared_ptr<Selection> > partition(const conduit::Node &n_mesh) const override;

    virtual void get_element_ids(const conduit::Node &n_mesh,
                                 std::vector<index_t> &element_ids) const override;

    void set_ranges(const conduit::Node &n)
    {
        ranges_storage.reset();
#ifdef CONDUIT_INDEX_32
        n.to_uint32_array(ranges_storage);
#else
        n.to_uint64_array(ranges_storage);
#endif
    }

    const index_t *get_ranges() const
    {
        // Access the converted data as index_t.
        return reinterpret_cast<const index_t *>(ranges_storage.data_ptr());
    }

    index_t num_ranges() const;

    virtual void print(std::ostream &os) const override;

protected:
    virtual bool determine_is_whole(const conduit::Node &n_mesh) const override;

    static const std::string RANGES_KEY;
    conduit::Node ranges_storage;
};

const std::string SelectionRanges::RANGES_KEY("ranges");

//---------------------------------------------------------------------------
SelectionRanges::SelectionRanges()
: Selection(),
  ranges_storage()
{
}

//---------------------------------------------------------------------------
SelectionRanges::SelectionRanges(const SelectionRanges &obj)
: Selection(obj),
  ranges_storage(obj.ranges_storage)
{
}

//---------------------------------------------------------------------------
SelectionRanges::~SelectionRanges()
{
}

//---------------------------------------------------------------------------
std::shared_ptr<Selection>
SelectionRanges::copy() const
{
    return std::make_shared<SelectionRanges>(*this);
}

//---------------------------------------------------------------------------
bool
SelectionRanges::init(const conduit::Node &n_options)
{
    bool retval = false;
    if(Selection::init(n_options))
    {
        if(n_options.has_child(RANGES_KEY))
        {
            const conduit::Node &n_ranges = n_options[RANGES_KEY];
            if(n_ranges.dtype().is_number())
            {
                // Convert to the right type for index_t
#ifdef CONDUIT_INDEX_32
                n_ranges.to_uint32_array(ranges_storage);
#else
                n_ranges.to_uint64_array(ranges_storage);
#endif
                retval = (ranges_storage.dtype().number_of_elements() % 2 == 0);
            }
        }
    }
    return retval;
}

//---------------------------------------------------------------------------
bool
SelectionRanges::applicable(const conduit::Node &/*n_mesh*/)
{
    return true;
}

//---------------------------------------------------------------------------
index_t
SelectionRanges::length(const conduit::Node &/*n_mesh*/) const
{
    index_t ncells = 0;
    const index_t *ranges = get_ranges();
    auto n = num_ranges();
    for(index_t i = 0; i < n; i++)
    {
        ncells += ranges[2*i+1] - ranges[2*i] + 1;
    }
    return ncells;
}

//---------------------------------------------------------------------------
index_t
SelectionRanges::num_ranges() const
{
    return ranges_storage.dtype().number_of_elements() / 2;
}

//---------------------------------------------------------------------------
bool
SelectionRanges::determine_is_whole(const conduit::Node &n_mesh) const
{
    bool is_whole = false;
    index_t num_elem_in_topo = 0;
    try
    {
        // Get the selected topology and coordset.
        const conduit::Node &n_topo = selected_topology(n_mesh);
        num_elem_in_topo = topology::length(n_topo);
    }
    catch(conduit::Error &)
    {
        num_elem_in_topo = 0;
    }

    auto n = num_ranges();
    if(n == 1)
    {
        auto upper_bound = std::min(get_ranges()[1], num_elem_in_topo-1);
        is_whole = get_ranges()[0] == 0 &&
                upper_bound == num_elem_in_topo-1;
    }
    else
    {
        auto indices = get_ranges();
        std::set<index_t> unique;
        for(index_t i = 0; i < n; i++)
        {
            index_t start = indices[2*i];
            index_t end = std::min(indices[2*i+1], num_elem_in_topo-1);
            for(index_t eid = start; eid <= end; eid++)
                unique.insert(eid);
        }
        is_whole = static_cast<index_t>(unique.size()) == num_elem_in_topo;
    }

    return is_whole;
}

//---------------------------------------------------------------------------
std::vector<std::shared_ptr<Selection> >
SelectionRanges::partition(const conduit::Node &n_mesh) const
{
    index_t ncells = length(n_mesh);
    auto ncells_2 = ncells / 2;
    auto n = num_ranges();
    auto ranges = get_ranges();
    index_t count = 0;
    index_t split_index = 0;
    for(index_t i = 0; i < n; i++)
    {
        auto rc = ranges[2*i+1] - ranges[2*i] + 1;
        if(count + rc > ncells_2)
        {
            split_index = i;
            break;
        }
        else
        {
            count += rc;
        }
    }

    std::vector<index_t> r0, r1;
    for(index_t i = 0; i < n; i++)
    {
        if(i < split_index)
        {
            r0.push_back(ranges[2*i+0]);
            r0.push_back(ranges[2*i+1]);
        }
        else if(i == split_index)
        {
            auto rc = (ranges[2*i+1] - ranges[2*i]) + 1;
            if(rc == 1)
            {
                r0.push_back(ranges[2*i+0]);
                r0.push_back(ranges[2*i+0]);
            }
            else if(rc == 2)
            {
                r0.push_back(ranges[2*i+0]);
                r0.push_back(ranges[2*i+0]);

                r1.push_back(ranges[2*i+1]);
                r1.push_back(ranges[2*i+1]);
            }
            else
            {
                auto rc_2 = rc / 2;
                r0.push_back(ranges[2*i+0]);
                r0.push_back(ranges[2*i+0] + rc_2);

                r1.push_back(ranges[2*i+0] + rc_2 + 1);
                r1.push_back(ranges[2*i+1]);
            }
        }
        else //if(i > split_index)
        {
            r1.push_back(ranges[2*i+0]);
            r1.push_back(ranges[2*i+1]);
        }
    }

    auto p0 = std::make_shared<SelectionRanges>();
    auto p1 = std::make_shared<SelectionRanges>();
    p0->ranges_storage.set(r0);
    p1->ranges_storage.set(r1);
    p0->set_whole(false);
    p1->set_whole(false);
    p0->set_domain(domain);
    p1->set_domain(domain);
    p0->set_topology(topology);
    p1->set_topology(topology);

    std::vector<std::shared_ptr<Selection> > parts;
    parts.push_back(p0);
    parts.push_back(p1);

    return parts;
}

//---------------------------------------------------------------------------
void
SelectionRanges::get_element_ids(const conduit::Node &n_mesh,
    std::vector<index_t> &element_ids) const
{
    try
    {
        const conduit::Node &n_topo = selected_topology(n_mesh);
        auto topolen = topology::length(n_topo);
        auto n = num_ranges();
        auto indices = get_ranges();
        for(index_t i = 0; i < n; i++)
        {
            index_t start = indices[2*i];
            index_t end = indices[2*i+1];
            for(index_t eid = start; eid <= end; eid++)
            {
                if(eid < topolen)
                    element_ids.push_back(eid);
            }
        }
    }
    catch(conduit::Error &)
    {
    }
}

//---------------------------------------------------------------------------
void
SelectionRanges::print(std::ostream &os) const
{
    os << "{"
       << "\"name\":\"" << name() << "\","
       << "\"domain\":" << get_domain() << ", "
       << "\"topology\":\"" << get_topology() << "\", "
       << "\"ranges\":[";
    auto n = num_ranges() * 2;
    auto indices = get_ranges();
    for(index_t i = 0; i < n; i++)
    {
        if(i > 0)
            os << ", ";
        os << indices[i];
    }
    os << "]}";
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/**
 @brief This class represents a field-based selection where the selection
        uses a field that contains domain numbers to split the mesh into
        chunks.
*/
class SelectionField : public Selection
{
public:
    SelectionField();
    SelectionField(const SelectionField &obj);
    virtual ~SelectionField();

    static std::string name() { return "field"; }

    virtual std::shared_ptr<Selection> copy() const override;

    // Initializes the selection from a conduit::Node.
    virtual bool init(const conduit::Node &n_options) override;

    virtual bool applicable(const conduit::Node &n_mesh) override;

    // Computes the number of cells in the selection.
    virtual index_t length(const conduit::Node &n_mesh) const override;

    virtual bool requires_initial_partition() const override { return true; }

    virtual std::vector<std::shared_ptr<Selection> > partition(const conduit::Node &n_mesh) const override;

    const std::string & get_field() const
    {
        return field;
    }

    void set_field(const std::string &value)
    {
        field = value;
    }

    void get_selected_value(index_t &value, bool &value_set) const
    {
        value = selected_value;
        value_set = selected_value_set;
    }

    void set_selected_value(index_t value)
    {
        selected_value = value;
        selected_value_set = true;
    }

    void set_destination_ranks(const conduit::Node &value)
    {
        destination_ranks.set_external(value);
    }

    /**
     @brief Returns the destination rank for this selection. The default
            version returns -1, indicating that we'll number the domain.
     @return The destination domain for this selection.
     */
    virtual int get_destination_domain() const override;

    /**
     @brief Returns the destination rank for this selection. The default
            version returns -1, indicating that we don't care where the
            domain is placed.
     @note  We override this method to allow the field selection to
            indicate a preference for the MPI rank where the data end up.
     @return The destination rank for this selection.
     */
    virtual int get_destination_rank() const override;

    virtual void get_element_ids(const conduit::Node &n_mesh,
                                 std::vector<index_t> &element_ids) const override;

    virtual void print(std::ostream &os) const override;

protected:
    virtual bool supports_domain_any() const override { return true; }

    virtual bool determine_is_whole(const conduit::Node &n_mesh) const override;
    bool const_applicable(const conduit::Node &n_mesh) const;

    static const std::string FIELD_KEY;
    static const std::string DESTINATION_RANKS_KEY;

    std::string   field;
    index_t       selected_value;
    bool          selected_value_set;
    conduit::Node destination_ranks;
};

const std::string SelectionField::FIELD_KEY("field");
const std::string SelectionField::DESTINATION_RANKS_KEY("destination_ranks");

//---------------------------------------------------------------------------
SelectionField::SelectionField()
: Selection(),
  field(),
  selected_value(0),
  selected_value_set(false),
  destination_ranks()
{
}

//---------------------------------------------------------------------------
SelectionField::SelectionField(const SelectionField &obj)
: Selection(obj),
  field(obj.field),
  selected_value(obj.selected_value),
  selected_value_set(obj.selected_value_set),
  destination_ranks(obj.destination_ranks)
{
}

//---------------------------------------------------------------------------
SelectionField::~SelectionField()
{
}

//---------------------------------------------------------------------------
std::shared_ptr<Selection>
SelectionField::copy() const
{
    return std::make_shared<SelectionField>(*this);
}

//---------------------------------------------------------------------------
bool
SelectionField::init(const conduit::Node &n_options)
{
    bool retval = false;
    if(Selection::init(n_options))
    {
        if(n_options.has_child(FIELD_KEY))
        {
            field = n_options[FIELD_KEY].as_string();
            retval = true;
        }
        if(n_options.has_child(DESTINATION_RANKS_KEY))
        {
            const conduit::Node &dr = n_options[DESTINATION_RANKS_KEY];

            if(dr.dtype().is_integer())
            {
                auto acc = dr.as_index_t_accessor();
                for(conduit::index_t i = 0; i < acc.number_of_elements(); i++)
                {
                    if(acc[i] < 0)
                    {
                        CONDUIT_ERROR(DESTINATION_RANKS_KEY << " may only contain non-negative values.");
                        retval = false;
                    }
                }
                if(retval)
                {
                    set_destination_ranks(dr);
                }
            }
        }
    }
    return retval;
}

//---------------------------------------------------------------------------
/**
 @brief Returns whether the field selection applies to the input mesh.
 */
bool
SelectionField::const_applicable(const conduit::Node &n_mesh) const
{
    bool retval = false;
    const conduit::Node &n_fields = n_mesh["fields"];
    if(n_fields.has_child(field))
    {
        const conduit::Node &n_field = n_fields[field];
        if(n_field.has_child("association") && n_field.has_child("topology"))
        {
            const conduit::Node &n_topo = selected_topology(n_mesh);
            if(n_topo.name() != n_field["topology"].as_string())
            {
                CONDUIT_INFO("Incompatible topology used for field selection.");
            }
            else if(n_field["association"].as_string() != "element")
            {
                CONDUIT_INFO("Field " << field
                             << " has incompatible association for field selection.");
            }
            else
            {
                retval = true;
            }
        }
    }

    return retval;
}

//---------------------------------------------------------------------------
bool
SelectionField::applicable(const conduit::Node &n_mesh)
{
    return const_applicable(n_mesh);
}

//---------------------------------------------------------------------------
index_t
SelectionField::length(const conduit::Node &n_mesh) const
{
    index_t len = 0;
    const conduit::Node &n_fields = n_mesh["fields"];
    if(const_applicable(n_mesh))
    {
        const conduit::Node &n_field = n_fields[field];
        if(selected_value_set)
        {
            // Count number of occurrances of the selected value.
            conduit::Node n_values;
            as_index_t(n_field["values"], n_values);
            auto iarr = as_index_t_array(n_values);
            index_t N = iarr.number_of_elements();
            for(index_t i = 0; i < N; i++)
            {
                if(static_cast<index_t>(iarr[i]) == selected_value)
                    len++;
            }
        }
        else
        {
            // No single value has been selected yet so use the field as the length.
            len = n_field["values"].dtype().number_of_elements();
        }
    }
    return len;
}

//---------------------------------------------------------------------------
bool
SelectionField::determine_is_whole(const conduit::Node &n_mesh) const
{
    bool retval = false;
    try
    {
        // Get the selected topology.
        const conduit::Node &n_topo = selected_topology(n_mesh);
        index_t len = conduit::blueprint::mesh::utils::topology::length(n_topo);
        retval = len == length(n_mesh);
    }
    catch(conduit::Error &)
    {
        retval = false;
    }

    return retval;
}

//---------------------------------------------------------------------------
/**
 @brief Partitions the mesh using the specified field.
 */
std::vector<std::shared_ptr<Selection> >
SelectionField::partition(const conduit::Node &n_mesh) const
{
    std::vector<std::shared_ptr<Selection> > parts;

    if(const_applicable(n_mesh))
    {
        if(!selected_value_set)
        {
            const conduit::Node &n_fields = n_mesh["fields"];
            const conduit::Node &n_field = n_fields[field];

            // Iterate through the field to find the unique values.
            conduit::Node n_values;
            as_index_t(n_field["values"], n_values);
            auto iarr = as_index_t_array(n_values);
            index_t N = iarr.number_of_elements();
            std::set<index_t> unique;
            for(index_t i = 0; i < N; i++)
            {
                unique.insert(iarr[i]);
            }

            // Now, make new SelectionField objects that reference only one value.
            for(auto it = unique.begin(); it != unique.end(); it++)
            {
                auto p0 = std::make_shared<SelectionField>();
                p0->set_whole(false);
                p0->set_domain(domain);
                p0->set_topology(topology);
                p0->set_field(field);
                p0->set_selected_value(*it);
                p0->set_destination_ranks(destination_ranks);
                parts.push_back(p0);
            }
        }
        else
        {
            // Normally a field selection would be telling us exactly which
            // domain we want to use for the selected cells. We may have set
            // a large target though that means we might still be splitting
            // larger field selections. In that case, make them into explicit
            // selections.
            std::vector<index_t> eids, ids0, ids1;
            get_element_ids(n_mesh, eids);
            size_t n2 = eids.size() / 2;
            for(size_t i = 0; i < eids.size(); i++)
            {
                if(i < n2)
                    ids0.push_back(eids[i]);
                else
                    ids1.push_back(eids[i]);
            }

            // Make partitioned selections.
            auto p0 = std::make_shared<SelectionExplicit>();
            auto p1 = std::make_shared<SelectionExplicit>();
            p0->set_indices(ids0);
            p1->set_indices(ids1);
            p0->set_whole(false);
            p1->set_whole(false);
            p0->set_domain(domain);
            p1->set_domain(domain);
            p0->set_topology(topology);
            p1->set_topology(topology);

            parts.push_back(p0);
            parts.push_back(p1);
        }
    }

    return parts;
}

//---------------------------------------------------------------------------
int
SelectionField::get_destination_domain() const
{
    return selected_value_set ? static_cast<int>(selected_value) : FREE_DOMAIN_ID;
}

//---------------------------------------------------------------------------
int
SelectionField::get_destination_rank() const
{
    // If destination_ranks was not provided, return FREE_RANK_ID so the
    // partitioner can place the data as needed.
    int dr = FREE_RANK_ID;

    // If destination_ranks was provided, consult it to see which rank
    // should get the selected domain value.
    if(selected_value_set &&
       destination_ranks.dtype().is_number() &&
       destination_ranks.dtype().number_of_elements() > 0)
    {
        auto acc = destination_ranks.as_int32_accessor();
        auto idx = selected_value % destination_ranks.dtype().number_of_elements();
        dr = static_cast<int>(acc[idx]);
    }

    return dr;
}

//---------------------------------------------------------------------------
void
SelectionField::get_element_ids(const conduit::Node &n_mesh,
    std::vector<index_t> &element_ids) const
{
    if(const_applicable(n_mesh))
    {
        const conduit::Node &n_fields = n_mesh["fields"];
        const conduit::Node &n_field = n_fields[field];

        // Iterate through the field to find the unique values.
        conduit::Node n_values;
        as_index_t(n_field["values"], n_values);
        auto iarr = as_index_t_array(n_values);
        index_t N = iarr.number_of_elements();
        for(index_t i = 0; i < N; i++)
        {
            if(static_cast<index_t>(iarr[i]) == selected_value)
                element_ids.push_back(i);
        }
    }
}

//---------------------------------------------------------------------------
void
SelectionField::print(std::ostream &os) const
{
    os << "{"
       << "\"name\":\"" << name() << "\","
       << "\"domain\":" << get_domain() << ", "
       << "\"topology\":\"" << get_topology() << "\", "
       << "\"field\": " << field << ","
       << "\"selected_value\": " << selected_value << ", "
       << "\"selected_value_set\": " << selected_value_set;

    if(destination_ranks.dtype().is_number())
    {
        os << ", "
           << "\"destination_ranks\": [";
        auto acc = destination_ranks.as_int32_accessor();
        for(conduit::index_t i = 0; i < acc.number_of_elements(); i++)
        {
            if(i > 0) os << ", ";
            os << acc[i];
        }
        os << "]";
    }

    os << "}";
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
Partitioner::Chunk::Chunk()
: mesh(nullptr),
  owns(false),
  destination_rank(Selection::FREE_RANK_ID),
  destination_domain(Selection::FREE_DOMAIN_ID)
{
}

//---------------------------------------------------------------------------
Partitioner::Chunk::Chunk(const Node *m, bool own)
: mesh(m),
  owns(own),
  destination_rank(Selection::FREE_RANK_ID),
  destination_domain(Selection::FREE_DOMAIN_ID)
{
}

//---------------------------------------------------------------------------
Partitioner::Chunk::Chunk(const Node *m, bool own, int dr, int dd)
: mesh(m),
  owns(own),
  destination_rank(dr),
  destination_domain(dd)
{
}

//---------------------------------------------------------------------------
void
Partitioner::Chunk::free()
{
    if(owns)
    {
        Node *m = const_cast<Node *>(mesh);
        delete m;
        mesh = nullptr;
        owns = false;
    }
}

//---------------------------------------------------------------------------
Partitioner::Partitioner()
: rank(0),
  size(1),
  target(1),
  meshes(),
  selections(),
  selected_fields(),
  mapping(true),
  build_adjsets(true),
  merge_tolerance(1.e-8),
  original_vertex_ids("original_vertex_ids"),
  original_element_ids("original_element_ids")
{
}

//---------------------------------------------------------------------------
Partitioner::~Partitioner()
{
}

//---------------------------------------------------------------------------
std::shared_ptr<Selection>
Partitioner::create_selection(const std::string &type) const
{
    std::shared_ptr<Selection> retval;

    if(type == SelectionLogical::name())
        retval = std::make_shared<SelectionLogical>();
    else if(type == SelectionExplicit::name())
        retval = std::make_shared<SelectionExplicit>();
    else if(type == SelectionRanges::name())
        retval = std::make_shared<SelectionRanges>();
    else if(type == SelectionField::name())
        retval = std::make_shared<SelectionField>();
    else
    {
        CONDUIT_ERROR("Unknown selection type: " << type);
    }
    return retval;
}

//---------------------------------------------------------------------------
std::shared_ptr<Selection>
Partitioner::create_selection_all_elements(const conduit::Node &n_mesh) const
{
    std::shared_ptr<Selection> retval;

    // We're making a selection that includes all elements for the mesh.
    // Take the first topology and its coordset.
    const conduit::Node &n_topo = n_mesh["topologies"][0];
    std::string csname(n_topo["coordset"].as_string());
    const conduit::Node &n_coords = n_mesh["coordsets"][csname];

    // Does the topo+coordset combo look structured?
    bool is_uniform = n_coords["type"].as_string() == "uniform";
    bool is_rectilinear = n_coords["type"].as_string() == "rectilinear";
    bool is_structured = n_coords["type"].as_string() == "explicit" &&
                         n_topo["type"].as_string() == "structured";

    if(is_uniform || is_rectilinear || is_structured)
    {
        index_t edims[3] = {1,1,1};
        conduit::blueprint::mesh::utils::topology::logical_dims(n_topo, edims, 3);
        retval = create_selection(SelectionLogical::name());
        retval->set_whole(true);
        retval->set_topology(n_topo.name());
        auto typed_sel = dynamic_cast<SelectionLogical *>(retval.get());
        if(typed_sel != nullptr)
        {
            typed_sel->set_end(edims[0] > 0 ? edims[0]-1 : 0,
                               edims[1] > 0 ? edims[1]-1 : 0,
                               edims[2] > 0 ? edims[2]-1 : 0);
        }
    }
    else
    {
        index_t nelem = topology::length(n_topo);

        // Create a range that selects the topology.
        retval = create_selection(SelectionRanges::name());
        retval->set_whole(true);
        retval->set_topology(n_topo.name());
        auto typed_sel = dynamic_cast<SelectionRanges *>(retval.get());
        if(typed_sel != nullptr)
        {
#ifdef CONDUIT_INDEX_32
            std::vector<uint32> range;
            range.push_back(0);
            range.push_back(static_cast<uint32>(nelem - 1));
            conduit::Node n_range;
            n_range.set(range);
            typed_sel->set_ranges(n_range);
#else
            std::vector<uint64> range;
            range.push_back(0);
            range.push_back(static_cast<uint64>(nelem - 1));
            conduit::Node n_range;
            n_range.set(range);
            typed_sel->set_ranges(n_range);
#endif
        }
    }
    return retval;
}

//---------------------------------------------------------------------------
bool
Partitioner::options_get_target(const conduit::Node &options,
                                unsigned int &value) const
{
    bool retval = false;
    value = 0;
    if(options.has_child("target"))
    {
        const conduit::Node &n_target = options["target"];
        if(n_target.dtype().is_number())
        {
            // For signed int types, make the min allowable value be 0.
            if(n_target.dtype().is_int8())
            {
                auto v = n_target.as_int8();
                value = static_cast<unsigned int>((v < 0) ? 0 : v);
            }
            else if(n_target.dtype().is_int16())
            {
                auto v = n_target.as_int16();
                value = static_cast<unsigned int>((v < 0) ? 0 : v);
            }
            else if(n_target.dtype().is_int32())
            {
                auto v = n_target.as_int32();
                value = static_cast<unsigned int>((v < 0) ? 0 : v);
            }
            else if(n_target.dtype().is_int64())
            {
                auto v = n_target.as_int64();
                value = static_cast<unsigned int>((v < 0) ? 0 : v);
            }
            else
            {
                value = n_target.to_unsigned_int();
            }
            retval = true;
        }
        else
        {
            CONDUIT_INFO("Nonnumber passed as selection target.");
        }
    }
#ifdef CONDUIT_DEBUG_PARTITIONER
    std::cout << "Partition called with target " << value << std::endl;
#endif
    return retval;
}

//---------------------------------------------------------------------------
bool
Partitioner::initialize(const conduit::Node &n_mesh,
                        const conduit::Node &options)
{
    auto global_domids = get_global_domids(n_mesh);

    auto doms = conduit::blueprint::mesh::domains(n_mesh);

    // Iterate over the selections in the options and check them against the
    // domains that were passed in to make a vector of meshes and selections
    // that can be used to partition the meshes.
    selections.clear();
    if(options.has_child("selections"))
    {
        const conduit::Node &n_selections = options["selections"];
        for(index_t i = 0; i < n_selections.number_of_children(); i++)
        {
            const conduit::Node &n_sel = n_selections[i];
            try
            {
                // It has to have type to be a valid selection.
                if(!n_sel.has_child("type"))
                    continue;

                std::string type(n_sel["type"].as_string());
                auto sel = create_selection(type);
                if(sel != nullptr && sel->init(n_sel))
                {
                    // The selection is good. See if it applies to the domains.
                    auto n = static_cast<index_t>(doms.size());
                    for(index_t di = 0; di < n; di++)
                    {
                        // Get the global id for this domain.
                        index_t domid = global_domids[di];

                        // NOTE: A selection is tied to a single domain at present.
                        //       The field selection could apply to multiple domains
                        //       if we wanted it to.
                        bool domain_compatible = (domid == sel->get_domain()) ||
                                                 sel->get_domain_any();
                        if(domain_compatible && sel->applicable(*doms[di]))
                        {
                            // We may split some selections upfront here so we
                            // can figure these values into the target.
                            if(sel->requires_initial_partition())
                            {
                                if(sel->get_domain_any())
                                    sel->set_domain(domid);
                                auto ps = sel->partition(*doms[di]);
                                for(size_t j = 0; j < ps.size(); j++)
                                {
                                    meshes.push_back(doms[di]);
                                    selections.push_back(ps[j]);
                                }
                            }
                            else
                            {
                                meshes.push_back(doms[di]);
                                if(sel->get_domain_any())
                                {
                                    auto newsel = sel->copy();
                                    newsel->set_domain(domid);
                                    selections.push_back(newsel);
                                }
                                else
                                    selections.push_back(sel);
                            }

                            // If domain_any is true then we want the selection to
                            // apply to the next domain too so do not break.
                            if(!sel->get_domain_any())
                                break;
                        }
                    }
                }
                else
                {
                    CONDUIT_INFO("Could not initialize selection " << i);
                    return false;
                }
            }
            catch(const conduit::Error &e)
            {
                CONDUIT_INFO("Exception thrown handling selection " << i
                     << ": " << e.message());
                return false;
            }
        }
    }
    else
    {
        // Add selections to indicate that we take the whole domain.
        for(size_t i = 0; i < doms.size(); i++)
        {
            auto sel = create_selection_all_elements(*doms[i]);
            index_t domid = global_domids[i];
            sel->set_domain(domid);
            selections.push_back(sel);
            meshes.push_back(doms[i]);
        }
    }

    // Get the number of target partitions that we're making. We determine
    // whether the options contain a target in a method so we can override
    // it in parallel.
    unsigned int targetval = 1;
    if(options_get_target(options, targetval))
    {
        target = targetval;
    }
    else
    {
        // We are likely using all domains on this rank and on all ranks,
        // so we need to sum the number of domains to arrive at the target.
        // Or, we did not pass target and had valid selections. Or, we
        // had an invalid target. In any case, we sum the number of valid
        // selections.
        target = count_targets();
    }

    // Get any fields that we're using to limit the selection.
    if(options.has_child("fields"))
    {
        selected_fields.clear();
        const conduit::Node &n_fields = options["fields"];
        for(index_t i = 0; i < n_fields.number_of_children(); i++)
            selected_fields.push_back(n_fields[i].name());
    }

    // Get whether we want to preserve old numbering of vertices, elements.
    if(options.has_child("mapping"))
        mapping = options["mapping"].to_unsigned_int() != 0;

    // Get the name of the original_vertex_ids field.
    if(options.has_child("original_vertex_ids"))
        original_vertex_ids = options["original_vertex_ids"].as_string();

    // Get the name of the original_element_ids field.
    if(options.has_child("original_element_ids"))
        original_element_ids = options["original_element_ids"].as_string();

    // Get whether we want to build adjsets if they are present. This option
    // lets us ignore them.
    if(options.has_child("build_adjsets"))
        build_adjsets = options["build_adjsets"].to_unsigned_int() != 0;

    // Get whether we want to preserve old numbering of vertices, elements.
    if(options.has_child("merge_tolerance"))
        merge_tolerance = options["merge_tolerance"].to_double();

#ifdef CONDUIT_DEBUG_PARTITIONER
    cout << rank << ": Partitioner::initialize" << endl;
    cout << "\ttarget=" << target << endl;
    for(size_t i = 0; i < selections.size(); i++)
    {
        cout << "\t";
        selections[i]->print(cout);
        cout << endl;
    }
#endif

    // If we made it to the end then we will have created any applicable
    // selections. Some ranks may have created no selections. That is ok.
    return true; //!selections.empty();
}

//---------------------------------------------------------------------------
std::vector<index_t>
Partitioner::get_global_domids(const conduit::Node& n_mesh)
{
    const auto doms = conduit::blueprint::mesh::domains(n_mesh);
    std::vector<index_t> domids(doms.size(), -1);
    for(size_t i = 0; i < doms.size(); i++)
    {
        domids[i] = i;
        // If the mesh has a domain_id then use that as the domain number.
        if(doms[i]->has_path("state/domain_id"))
        {
            bool ok = false;
            index_t tmp = get_index_t(doms[i]->operator[]("state/domain_id"), ok);
            if(ok)
                domids[i] = tmp;
        }
    }
    return domids;
}

//---------------------------------------------------------------------------
unsigned int
Partitioner::count_targets() const
{
    // We make a pass over the selections on this rank and determine the
    // number of selections that produce free domains. These are domains that
    // have FREE_DOMAIN_ID for their destination domain. We also figure out
    // the number of named domains with unique domain ids. These are not
    // double-counted. The number of actual targets is the sum.
    unsigned int free_domains = 0;
    std::set<int> named_domains;
    for(size_t i = 0; i < selections.size(); i++)
    {
        int dd = selections[i]->get_destination_domain();
        if(dd == Selection::FREE_DOMAIN_ID)
            free_domains++;
        else
            named_domains.insert(dd);
    }

    unsigned int n = free_domains + static_cast<unsigned int>(named_domains.size());
    return n;
}

//---------------------------------------------------------------------------
void
Partitioner::get_largest_selection(int &sel_rank, int &sel_index) const
{
    sel_rank = 0;
    sel_index = -1;
    long largest_selection_size = 0;
    for(size_t i = 0; i < selections.size(); i++)
    {
        long ssize = static_cast<long>(selections[i]->length(*meshes[i]));
        if(ssize > largest_selection_size)
        {
            largest_selection_size = ssize;
            sel_index = static_cast<int>(i);
        }
    }
}

//---------------------------------------------------------------------------
long
Partitioner::get_total_selections() const
{
    return static_cast<long>(selections.size());
}

//---------------------------------------------------------------------------
void
Partitioner::split_selections()
{
    // Splitting based on target.
#ifdef CONDUIT_DEBUG_PARTITIONER
    int iteration = 1;
#endif
    bool splitting = true;
    while(splitting)
    {
        // Get the total number of targets produced by the current selections.
        auto nt = count_targets();
        if(nt == 0)
            splitting = false;
        else
            splitting = target > nt;

        if(splitting)
        {
            // Get the rank with the largest selection and get that local
            // selection index.
            int sel_rank = -1, sel_index = -1;
            get_largest_selection(sel_rank, sel_index);

            if(rank == sel_rank)
            {
                auto ps = selections[sel_index]->partition(*meshes[sel_index]);

                if(!ps.empty())
                {
                    const conduit::Node *m = meshes[sel_index];
                    meshes.insert(meshes.begin()+sel_index, ps.size()-1, m);
                    selections.insert(selections.begin()+sel_index, ps.size()-1, nullptr);
                    for(size_t i = 0; i < ps.size(); i++)
                        selections[sel_index + i] = ps[i];

#ifdef CONDUIT_DEBUG_PARTITIONER
                    if(rank == 0)
                    {
                        cout << "Partitioner::split_selections target=" << target
                             << ", nt=" << nt << " (after split "
                             << iteration << ")" << endl;
                        for(size_t i = 0; i < selections.size(); i++)
                        {
                            cout << "\t";
                            selections[i]->print(cout);
                            cout << endl;
                        }
                    }
                    iteration++;
#endif
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
template<typename FloatType>
static void
copy_volume_fractions(
    const std::vector<index_t> &element_ids,
    const Node &in_data,
    Node &out_data)
{
    out_data.set_dtype(conduit::DataType(in_data.dtype().id(), element_ids.size()));

    const conduit::DataArray<FloatType> in_values(in_data.element_ptr(0), in_data.dtype());
    conduit::DataArray<FloatType> out_values(out_data.element_ptr(0), out_data.dtype());

    // Iterate over desired element ids
    index_t out_idx = 0;
    for(const auto elem_id : element_ids)
    {
        out_values[out_idx++] = in_values[elem_id];
    }
}

//---------------------------------------------------------------------------
template<typename FloatType>
static void
copy_volume_fractions_o2m(
    const std::vector<index_t> &element_ids,
    const Node &in_data,
    Node &out_data)
{
    // In data is o2m, need to extract "values" to get data
    conduit::blueprint::o2mrelation::O2MIterator indexer(in_data);
    const conduit::Node &n_in_values = in_data["values"];
    out_data.set_dtype(conduit::DataType(in_data["values"].dtype().id(), element_ids.size()));

    const conduit::DataArray<FloatType> in_values(n_in_values.element_ptr(0), n_in_values.dtype());
    conduit::DataArray<FloatType> out_values(out_data.element_ptr(0), out_data.dtype());

    // Iterate over desired element ids
    index_t out_idx = 0;
    for(const auto elem_id : element_ids)
    {
        indexer.to(elem_id);
        const auto index = indexer.index();
        out_values[out_idx++] = in_values[index];
    }
}

//---------------------------------------------------------------------------
template<typename FloatType>
static void
copy_uni_buffer_matset_impl(
    const std::vector<index_t> &element_ids,
    const conduit::Node &n_matset,
    conduit::Node &out_matset)
{
    const index_t num_elements = (index_t)element_ids.size();

    // Determine output size and sizes/offsets
    index_t out_size = 0;
    if(n_matset.has_child("sizes"))
    {
        // Element based matset will have an entry for each element, should be able to safely index by element id
        const conduit::DataAccessor<index_t> size_access = n_matset.fetch_existing("sizes").value();
        out_matset["sizes"].set_dtype(conduit::DataType::index_t(num_elements));
        out_matset["offsets"].set_dtype(conduit::DataType::index_t(num_elements));
        DataArray<index_t> out_sizes   = out_matset["sizes"].value();
        DataArray<index_t> out_offsets = out_matset["offsets"].value();

        // Build output sizes / offsets, while computing size of output data array
        bool need_offsets_sizes = false;
        for(index_t i = 0; i < num_elements; i++)
        {
            const index_t id = element_ids[i];
            const index_t s  = size_access[id];
            if(s != 1)
            {
                need_offsets_sizes = true;
            }
            out_offsets[i] = out_size;
            out_sizes[i]   = s;
            out_size      += s;
        }

        // If each entry in the O2M was 1 then we don't need the auxillary arrays
        if(!need_offsets_sizes)
        {
            out_matset.remove("sizes");
            out_matset.remove("offsets");
        }
    }
    else
    {
        out_size = num_elements;
    }

    // Allocate output arrays
    out_matset["material_ids"].set_dtype(conduit::DataType::index_t(out_size));
    out_matset["volume_fractions"].set_dtype(
        conduit::DataType(n_matset.fetch_existing("volume_fractions").dtype().id(), out_size));
    DataArray<index_t> out_material_ids = out_matset.fetch_existing("material_ids").value();
    DataArray<FloatType> out_volume_fractions = out_matset.fetch_existing("volume_fractions").value();

    const DataAccessor<index_t> in_material_ids = n_matset.fetch_existing("material_ids").value();
    const DataArray<FloatType> in_volume_fractions = n_matset.fetch_existing("volume_fractions").value();
    conduit::blueprint::o2mrelation::O2MIterator indexer(n_matset);
    index_t offset = 0;

    for(index_t i = 0; i < num_elements; i++)
    {
        // Point the o2m iterator at (element_id, 0)
        const auto element_id = element_ids[i];
        indexer.to(element_id, conduit::blueprint::o2mrelation::ONE);
        indexer.to(0, conduit::blueprint::o2mrelation::MANY);

        // Get the number of "many" elements for this element_id
        const index_t size = indexer.elements(conduit::blueprint::o2mrelation::MANY);
        index_t idx  = indexer.index(conduit::blueprint::o2mrelation::DATA);

#if 0
        std::cout << "element_id = " << element_id
            << "\nsize = " << size
            << "\nidx = " << idx << std::endl;
#endif

        // Copy each item in the O2M relation
        for(index_t j = 0; j < size; j++)
        {
            out_material_ids[offset]     = in_material_ids[idx];
            out_volume_fractions[offset] = in_volume_fractions[idx];
            idx++;
            offset++;
        }
    }
}

//---------------------------------------------------------------------------
static void
copy_uni_buffer_matset(
    const std::vector<index_t> &element_ids,
    const conduit::Node &n_matset,
    conduit::Node &out_matset)
{
    // Determine floating point type
    const auto &dtype = n_matset.fetch_existing("volume_fractions").dtype();
    if(dtype.is_float())
    {
        copy_uni_buffer_matset_impl<float>(element_ids, n_matset, out_matset);
    }
    else if(dtype.is_double())
    {
        copy_uni_buffer_matset_impl<double>(element_ids, n_matset, out_matset);
    }
#if CONDUIT_USE_LONG_DOUBLE
    else if(dtype.is_long_double())
    {
        copy_uni_buffer_matset_impl<long double>(element_ids, n_matset, out_matset);
    }
#endif

}

//---------------------------------------------------------------------------
static void
copy_multi_buffer_matset(
    const std::vector<index_t> &element_ids,
    const conduit::Node &n_matset,
    conduit::Node &out_matset)
{
    const conduit::Node &n_vfracts = n_matset["volume_fractions"];
    conduit::Node &out_vfracts = out_matset.add_child("volume_fractions");
    auto vfract_itr = n_vfracts.children();
    while(vfract_itr.has_next())
    {
        const conduit::Node &n_vfract = vfract_itr.next();
        conduit::Node &out_vfract = out_vfracts.add_child(n_vfract.name());
        bool is_o2m = n_vfract.dtype().is_object();
        if(is_o2m)
        {
            const auto data_paths = blueprint::o2mrelation::data_paths(n_vfract);
            if(data_paths.empty())
            {
                CONDUIT_ERROR("volume_fractions appears to be an o2m relation but has no data_paths.");
                return;
            }
            std::string data_name = data_paths[0];

            const auto &dtype = n_vfract[data_name].dtype();
            if(dtype.is_float())
            {
                copy_volume_fractions_o2m<float>(element_ids, n_vfract, out_vfract);
            }
            else if(dtype.is_double())
            {
                copy_volume_fractions_o2m<double>(element_ids, n_vfract, out_vfract);
            }
#if CONDUIT_USE_LONG_DOUBLE
            else if(dtype.is_long_double())
            {
                copy_volume_fractions_o2m<long double>(element_ids, n_vfract, out_vfract);
            }
#endif
        }
        else
        {
            const auto &dtype = n_vfract.dtype();
            if(dtype.is_float())
            {
                copy_volume_fractions<float>(element_ids, n_vfract, out_vfract);
            }
            else if(dtype.is_double())
            {
                copy_volume_fractions<double>(element_ids, n_vfract, out_vfract);
            }
#if CONDUIT_USE_LONG_DOUBLE
            else if(dtype.is_long_double())
            {
                copy_volume_fractions<long double>(element_ids, n_vfract, out_vfract);
            }
#endif
        }
    }
}

//---------------------------------------------------------------------------
/**
 @brief Populate unordered map of topological element_ids -> matset data indicies
*/
static void
create_element_to_material_map(const conduit::DataAccessor<index_t> &mat_element_ids,
                               std::unordered_map<index_t, index_t> &out)
{
    const auto mat_nelem = mat_element_ids.number_of_elements();
    out.clear();
    out.reserve(mat_nelem);
    for(index_t i = 0; i < mat_nelem; i++)
    {
        // Map the actual element id to an index
        out.insert({mat_element_ids[i], i});
    }
}

//---------------------------------------------------------------------------
template<typename FloatType>
static void
copy_material_based_volume_fractions(const std::vector<index_t> &element_ids,
                                     const conduit::Node &n_vfract,
                                     const std::unordered_map<index_t, index_t> &elem_map,
                                     conduit::Node &out_matset)
{
    // Figure out if vfract is o2m, create iterator if it is
    const bool is_o2m = n_vfract.dtype().is_object();

    std::string o2m_data_name;
    if(is_o2m)
    {
        auto paths = blueprint::o2mrelation::data_paths(n_vfract);
        if(paths.empty())
        {
            CONDUIT_ERROR("volume_fraction appears to be an o2m relation but it contains no data paths.");
            return;
        }
        o2m_data_name = paths[0];
    }
    const conduit::DataArray<FloatType> vfracts = (is_o2m) ? n_vfract[o2m_data_name].value() : n_vfract.value();

    const std::string &mat_name = n_vfract.name();
    conduit::Node &out_mat_vfract = out_matset["volume_fractions"].add_child(mat_name);
    conduit::Node &out_mat_elems  = out_matset["element_ids"].add_child(mat_name);

    std::vector<FloatType> out_vfracts;
    std::vector<index_t> out_elems;
    const index_t nelem = (index_t)element_ids.size();
    if(is_o2m)
    {
        conduit::blueprint::o2mrelation::O2MIterator indexer(n_vfract);
        for(index_t i = 0; i < nelem; i++)
        {
            const index_t id = element_ids[i];
            const auto itr = elem_map.find(id);
            if(itr != elem_map.end())
            {
                const index_t one_idx = itr->second;
                indexer.to(one_idx, conduit::blueprint::o2mrelation::ONE);
                indexer.to(0, conduit::blueprint::o2mrelation::MANY);
                const index_t data_idx = indexer.index();
                out_vfracts.push_back(vfracts[data_idx]);
                out_elems.push_back(i);
            }
        }
    }
    else
    {
        for(index_t i = 0; i < nelem; i++)
        {
            const index_t id = element_ids[i];
            const auto itr = elem_map.find(id);
            if(itr != elem_map.end())
            {
                const index_t data_idx = itr->second;
                out_vfracts.push_back(vfracts[data_idx]);
                out_elems.push_back(i);
            }
        }
    }

    // For baseline consistency
    if(out_vfracts.size() > 0)
    {
        out_mat_vfract.set(out_vfracts);
        out_mat_elems.set(out_elems);
    }
}

//---------------------------------------------------------------------------
template<typename FloatType>
static void
copy_material_based_uni_buffer(const std::vector<index_t> &element_ids,
                               const conduit::Node &n_matset,
                               const std::unordered_map<index_t, index_t> &elem_map,
                               conduit::Node &out_matset)
{
    const conduit::DataAccessor<index_t> in_material_ids     = n_matset["material_ids"].value();
    const conduit::DataArray<FloatType>  in_volume_fractions = n_matset["volume_fractions"].value();
    const index_t num_elements = (index_t)element_ids.size();

    // Make a first pass to determine sizes, offsets, and new element ids
    // Also build "one_idxs" so we don't have to query the map again on the second pass
    std::vector<index_t> one_idxs;
    one_idxs.reserve(num_elements);
    index_t out_size = 0;
    if(n_matset.has_child("sizes"))
    {
        // Element based matset will have an entry for each element, should be able to safely index by element id
        const conduit::DataAccessor<index_t> size_access = n_matset.fetch_existing("sizes").value();
        std::vector<index_t> out_sizes;
        std::vector<index_t> out_offsets;
        std::vector<index_t> out_elem_ids;
        out_sizes.reserve(num_elements);
        out_offsets.reserve(num_elements);
        out_elem_ids.reserve(num_elements);

        // Build output sizes / offsets, while computing size of output data array
        bool need_offsets_sizes = false;
        for(index_t i = 0; i < num_elements; i++)
        {
            const index_t id = element_ids[i];
            const auto itr   = elem_map.find(id);
            if(itr != elem_map.end())
            {
                const index_t idx = itr->second;
                one_idxs.push_back(idx);
                out_elem_ids.push_back(i);
                const index_t s  = size_access[idx];
                if(s != 1)
                {
                    need_offsets_sizes = true;
                }
                out_offsets.push_back(out_size);
                out_sizes.push_back(s);
                out_size += s;
            }
        }

        out_matset["element_ids"].set(out_elem_ids);
        // Only copy sizes/offsets if they are actually needed
        if(need_offsets_sizes)
        {
            out_matset["sizes"].set(out_sizes);
            out_matset["offsets"].set(out_offsets);
        }
    }
    else
    {
        std::vector<index_t> out_elem_ids;
        out_elem_ids.reserve(num_elements);
        for(index_t i = 0; i < num_elements; i++)
        {
            const index_t id = element_ids[i];
            const auto itr   = elem_map.find(id);
            if(itr != elem_map.end())
            {
                one_idxs.push_back(itr->second);
                out_elem_ids.push_back(i);
            }
        }
        out_size = (index_t)out_elem_ids.size();
        out_matset["element_ids"].set(out_elem_ids);
    }

    // Allocate output
    out_matset["material_ids"].set_dtype(conduit::DataType::index_t(out_size));
    out_matset["volume_fractions"].set_dtype(conduit::DataType(in_volume_fractions.dtype().id(), out_size));

    conduit::blueprint::o2mrelation::O2MIterator indexer(n_matset);
    conduit::DataArray<index_t>   out_mat_ids = out_matset.fetch_existing("material_ids").value();
    conduit::DataArray<FloatType> out_vfracts = out_matset.fetch_existing("volume_fractions").value();
    index_t offset = 0;
    for(const index_t one_idx : one_idxs)
    {
        // Point the o2m iterator at (element_id, 0)
        indexer.to(one_idx, conduit::blueprint::o2mrelation::ONE);
        indexer.to(0, conduit::blueprint::o2mrelation::MANY);

        // Get the number of "many" elements for this element_id
        const index_t size = indexer.elements(conduit::blueprint::o2mrelation::MANY);
        index_t idx  = indexer.index(conduit::blueprint::o2mrelation::DATA);

        // Copy each item in the O2M relation
        for(index_t j = 0; j < size; j++)
        {
            out_mat_ids[offset] = in_material_ids[idx];
            out_vfracts[offset] = in_volume_fractions[idx];
            idx++;
            offset++;
        }
    }
}

//---------------------------------------------------------------------------
static void
copy_material_based_matset(const std::vector<index_t> &element_ids,
                           const conduit::Node &n_matset,
                           conduit::Node &out_matset)
{
    const conduit::Node &n_elem_ids = n_matset["element_ids"];
    const conduit::Node &n_volume_fractions = n_matset["volume_fractions"];
    std::unordered_map<index_t, index_t> elem_map;
    // Material based matsets can also be multi or uni buffer
    if(blueprint::mesh::matset::is_multi_buffer(n_matset))
    {
        auto itr = n_volume_fractions.children();
        while(itr.has_next())
        {
            const conduit::Node &n_mat_vfract = itr.next();
            const conduit::Node &n_mat_elems = n_elem_ids[n_mat_vfract.name()];
            const conduit::DataAccessor<index_t> mat_elems = n_mat_elems.value();
            create_element_to_material_map(mat_elems, elem_map);

            const auto &dtype = n_mat_vfract.dtype();
            if(dtype.is_float())
            {
                copy_material_based_volume_fractions<float>(element_ids, n_mat_vfract,
                    elem_map, out_matset);
            }
            else if(dtype.is_double())
            {
                copy_material_based_volume_fractions<double>(element_ids, n_mat_vfract,
                    elem_map, out_matset);
            }
#if CONDUIT_USE_LONG_DOUBLE
            else if(dtype.is_long_double())
            {
                copy_material_based_volume_fractions<long double>(element_ids, n_mat_vfract,
                    elem_map, out_matset);
            }
#endif
        }
    }
    else
    {
        const conduit::DataAccessor<index_t> mat_elems = n_elem_ids.value();
        create_element_to_material_map(mat_elems, elem_map);

        const auto &dtype = n_volume_fractions.dtype();
        if(dtype.is_float())
        {
            copy_material_based_uni_buffer<float>(element_ids, n_matset,
                elem_map, out_matset);
        }
        else if(dtype.is_double())
        {
            copy_material_based_uni_buffer<double>(element_ids, n_matset,
                elem_map, out_matset);
        }
#if CONDUIT_USE_LONG_DOUBLE
        else if(dtype.is_long_double())
        {
            copy_material_based_volume_fractions<long double>(element_ids, n_mat_vfract,
                elem_map, out_matset);
        }
#endif
    }
}

//---------------------------------------------------------------------------
void
Partitioner::copy_matsets(const std::string &topology,
                          const std::vector<index_t> &element_ids,
                          const conduit::Node &n_mesh,
                          conduit::Node &n_output) const
{
    // If there are no matsets then there is nothing to do.
    if(!n_mesh.has_child("matsets"))
        return;

    // n_output.schema().print();
    const conduit::Node &n_matsets = n_mesh["matsets"];
    conduit::Node &out_matsets = n_output["matsets"];
    auto matset_itr = n_matsets.children();
    while(matset_itr.has_next())
    {
        // Only transfer matsets on the active topology
        const conduit::Node &n_matset = matset_itr.next();
        if(n_matset["topology"].as_string() != topology)
        {
            continue;
        }
        conduit::Node &out_matset = out_matsets.add_child(n_matset.name());
        out_matset["topology"].set(topology);

        // "material_map" is required by uni_buffer and optional for multi bugffer
        if(n_matset.has_child("material_map"))
        {
            out_matset["material_map"].set(n_matset["material_map"]);
        }

        // Check if material based
        if(n_matset.has_child("element_ids"))
        {
            copy_material_based_matset(element_ids, n_matset, out_matset);
        }
        else if(blueprint::mesh::matset::is_multi_buffer(n_matset))
        {
            // Element based
            copy_multi_buffer_matset(element_ids, n_matset, out_matset);
        }
        else
        {
            // Element based
            copy_uni_buffer_matset(element_ids, n_matset, out_matset);
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::copy_fields(index_t domain, const std::string &topology,
    const std::vector<index_t> &vertex_ids,
    const std::vector<index_t> &element_ids,
    const conduit::Node &n_mesh,
    conduit::Node &n_output) const
{
    if(n_mesh.has_child("fields"))
    {
        const conduit::Node &n_fields = n_mesh["fields"];
        if(!vertex_ids.empty())
        {
            conduit::Node &n_output_fields = n_output["fields"];
            for(index_t i = 0; i < n_fields.number_of_children(); i++)
            {
                const conduit::Node &n_field = n_fields[i];
                if(n_field.has_child("association"))
                {
                    auto association = n_field["association"].as_string();
                    auto assoc_topo = n_field["topology"].as_string();
                    if(association == "vertex" && topology == assoc_topo)
                    {
                        copy_field(n_field, vertex_ids, n_output_fields);
                    }
                }
            }

            if(mapping)
            {
                // Save the vertex_ids as a new MC field.
                conduit::Node &n_field = n_output_fields[original_vertex_ids_name()];
                n_field["association"] = "vertex";
                if(!topology.empty())
                    n_field["topology"] = topology;
                std::vector<index_t> domain_ids(vertex_ids.size(), domain);
                conduit::Node &n_values = n_field["values"];
                n_values["domains"].set(domain_ids);
                n_values["ids"].set(vertex_ids);
            }
        }

        if(!element_ids.empty())
        {
            conduit::Node &n_output_fields = n_output["fields"];
            for(index_t i = 0; i < n_fields.number_of_children(); i++)
            {
                const conduit::Node &n_field = n_fields[i];
                if(n_field.has_child("association"))
                {
                    auto association = n_field["association"].as_string();
                    auto assoc_topo = n_field["topology"].as_string();
                    if(association == "element" && topology == assoc_topo)
                    {
                        copy_field(n_field, element_ids, n_output_fields);
                    }
                }
            }

            if(mapping)
            {
                // Save the element_ids as a new MC field.
                conduit::Node &n_field = n_output_fields[original_element_ids_name()];
                n_field["association"] = "element";
                if(!topology.empty())
                    n_field["topology"] = topology;
                std::vector<index_t> domain_ids(element_ids.size(), domain);
                conduit::Node &n_values = n_field["values"];
                n_values["domains"].set(domain_ids);
                n_values["ids"].set(element_ids);
            }
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::copy_field(const conduit::Node &n_field,
    const std::vector<index_t> &ids, Node &n_output_fields) const
{
    static const std::vector<std::string> keys{"association", "grid_function",
        "volume_dependent", "topology"};

    // If we're subselecting the list of fields, check whether the field we
    // want to copy is in selected_fields.
    if(!selected_fields.empty() &&
       std::find(selected_fields.begin(),
                 selected_fields.end(),
                 n_field.name()) == selected_fields.end())
    {
        return;
    }

// TODO: What about matsets and mixed fields?...
// https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#fields

    // Copy common field attributes from the old field into the new one.
    conduit::Node &n_new_field = n_output_fields[n_field.name()];
    for(const auto &key : keys)
    {
        if(n_field.has_child(key))
            n_new_field[key] = n_field[key];
    }

    const conduit::Node &n_values = n_field["values"];
    conduit::Node &new_values = n_new_field["values"];
    conduit::blueprint::mesh::utils::slice_field(n_values, ids, new_values);
}

//---------------------------------------------------------------------------
/**
 @brief Iterates over the cells in the topo that are specified in element_ids
        and adds their vertex ids into vertex_ids so we can build up a set of
        vertices that will need to be pulled from the coordset.
 */
void
Partitioner::get_vertex_ids_for_element_ids(const conduit::Node &n_topo,
    const std::vector<index_t> &element_ids,
    std::set<index_t> &vertex_ids) const
{
    bool is_base_rectilinear = n_topo["type"].as_string() == "rectilinear";
    bool is_base_structured = n_topo["type"].as_string() == "structured";
    bool is_base_uniform = n_topo["type"].as_string() == "uniform";

    if(is_base_rectilinear || is_base_structured || is_base_uniform)
    {
        index_t edims[3] = {1,1,1}, dims[3] = {0,0,0};
        auto ndims = topology::dims(n_topo);
        conduit::blueprint::mesh::utils::topology::logical_dims(n_topo, edims, 3);
//cout << "get_vertex_ids_for_element_ids: edims=" << edims[0] << ", " << edims[1] << ", " << edims[2] << endl;
        dims[0] = edims[0] + 1;
        dims[1] = edims[1] + (ndims > 1 ? 1 : 0);
        dims[2] = edims[2] + (ndims > 2 ? 1 : 0);
//cout << "get_vertex_ids_for_element_ids: dims=" << dims[0] << ", " << dims[1] << ", " << dims[2] << endl;

        index_t cell_ijk[3]={0,0,0}, pt_ijk[3] = {0,0,0}, ptid = 0;
        static const index_t offsets[8][3] = {
            {0,0,0},
            {1,0,0},
            {0,1,0},
            {1,1,0},
            {0,0,1},
            {1,0,1},
            {0,1,1},
            {1,1,1}
        };
        int np = (ndims == 2) ? 4 : 8;
        auto n = element_ids.size();
        for(size_t j = 0; j < n; j++)
        {
            // Get the IJK coordinate of the element.
            grid_id_to_ijk(element_ids[j], edims, cell_ijk);
//cout << element_ids[i] << "-> " << cell_ijk[0] << "," << cell_ijk[1] << "," << cell_ijk[2] << endl;
            // Turn the IJK into vertex ids.
            for(int i = 0; i < np; i++)
            {
                pt_ijk[0] = cell_ijk[0] + offsets[i][0];
                pt_ijk[1] = cell_ijk[1] + offsets[i][1];
                pt_ijk[2] = cell_ijk[2] + offsets[i][2];
                grid_ijk_to_id(pt_ijk, dims, ptid);

//cout << "\t" << pt_ijk[0] << "," << pt_ijk[1] << "," << pt_ijk[2] << "-> " << ptid << endl;

                vertex_ids.insert(ptid);
            }
        }
    }
    else
    {
        conduit::blueprint::mesh::utils::ShapeType shape(n_topo);
        if(shape.is_polygonal())
        {
            conduit::Node n_indices;
            as_index_t(n_topo["elements/connectivity"], n_indices);
            auto iptr = as_index_t_array(n_indices);

            // Make sure we have offsets we can use.
            conduit::Node n_offsets, n_sizes;
#ifdef CONDUIT_INDEX_32
            if(n_topo.has_path("elements/offsets"))
                n_topo["elements/offsets"].to_unsigned_int_array(n_offsets);
            else
            {
                conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(n_topo, n_offsets);
            }
            auto offsets = n_offsets.as_unsigned_int_array();
#else
            if(n_topo.has_path("elements/offsets"))
                n_topo["elements/offsets"].to_unsigned_long_array(n_offsets);
            else
                conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(n_topo, n_offsets);
            auto offsets = n_offsets.as_unsigned_long_array();
#endif
            n_topo["elements/sizes"].to_unsigned_int_array(n_sizes);
            auto sizes = n_sizes.as_unsigned_int_array();
            for(size_t i = 0; i < element_ids.size(); i++)
            {
                auto offset = offsets[element_ids[i]];
                auto sz = sizes[element_ids[i]];
                for(unsigned int ptid = 0; ptid < sz; ptid++)
                    vertex_ids.insert(iptr[offset + ptid]);
            }
        }
        else if(shape.is_polyhedral())
        {
            conduit::Node n_indices;
            as_index_t(n_topo["elements/connectivity"], n_indices);
            auto iptr = as_index_t_array(n_indices);

            // NOTE: I'm assuming that offsets are built already.
            conduit::Node n_offsets, n_sizes;
            as_index_t(n_topo["elements/offsets"], n_offsets);
            as_index_t(n_topo["elements/sizes"], n_sizes);
            auto offsets = as_index_t_array(n_offsets);
            auto sizes = as_index_t_array(n_sizes);

            conduit::Node n_se_conn, n_se_offsets, n_se_sizes;
            as_index_t(n_topo["subelements/connectivity"], n_se_conn);
            as_index_t(n_topo["subelements/offsets"], n_se_offsets);
            as_index_t(n_topo["subelements/sizes"], n_se_sizes);
            auto se_conn = as_index_t_array(n_se_conn);
            auto se_offsets = as_index_t_array(n_se_offsets);
            auto se_sizes = as_index_t_array(n_se_sizes);

            for(auto eid : element_ids)
            {
                auto offset = static_cast<index_t>(offsets[eid]);
                auto nfaces = static_cast<index_t>(sizes[eid]);
                for(index_t fi = 0; fi < nfaces; fi++)
                {
                    // Now, look up the vertices in this face.
                    auto face_id = iptr[offset + fi];
                    auto face_offset = static_cast<index_t>(se_offsets[face_id]);
                    auto face_nfaces = static_cast<index_t>(se_sizes[face_id]);
                    for(index_t vi = 0; vi < face_nfaces; vi++)
                    {
                        vertex_ids.insert(se_conn[face_offset + vi]);
                    }
                }
            }
        }
        else if(n_topo.has_path("elements/element_types"))
        {
            // It looks like the topology is unstructured but contains multiple
            // element types.

            // Determine the number of points for each stream id and shape.
            std::map<int,int> stream_id_npts;
            const conduit::Node &n_element_types = n_topo["elements/element_types"];
            for(index_t i = 0; i < n_element_types.number_of_children(); i++)
            {
                const conduit::Node &n = n_element_types[i];
                auto stream_id = n["stream_id"].to_int();
                std::string shape_str(n["shape"].as_string());
                for(size_t j = 0; j < utils::TOPO_SHAPES.size(); j++)
                {
                    if(shape_str == utils::TOPO_SHAPES[j])
                    {
                        stream_id_npts[stream_id] = utils::TOPO_SHAPE_INDEX_COUNTS[j];
                        break;
                    }
                }
            }

            conduit::Node n_stream_ids, n_element_counts, n_stream;
            as_index_t(n_topo["elements/element_index/stream_ids"], n_stream_ids);
            as_index_t(n_topo["elements/element_index/element_counts"], n_element_counts);
            as_index_t(n_topo["elements/stream"], n_stream);
            auto stream_ids = as_index_t_array(n_stream_ids);
            auto element_counts = as_index_t_array(n_element_counts);
            auto stream = as_index_t_array(n_stream);

            // Compute some size,offsets into the stream to help
            std::vector<index_t> sizes, offsets;
            index_t offset = 0;
            for(index_t j = 0; j < stream_ids.number_of_elements(); j++)
            {
                auto n = static_cast<index_t>(element_counts[j]);
                auto npts = stream_id_npts[stream_ids[j]];
                for(index_t i = 0; i < n; i++)
                {
                    sizes.push_back(npts);
                    offsets.push_back(offset);
                    offset += npts;
                }
            }

            // Now, for each element, insert its vertices.
            for(size_t i = 0; i < element_ids.size(); i++)
            {
                auto npts = sizes[element_ids[i]];
                for(index_t j = 0; j < npts; j++)
                    vertex_ids.insert(stream[offsets[element_ids[i]] + j]);
            }
        }
        else if(n_topo.fetch_existing("elements/shape").as_string() == "mixed")
        {
            const auto conn = n_topo["elements/connectivity"].as_index_t_accessor();
            const auto shapes = n_topo.fetch_existing("elements/shapes").as_index_t_accessor();
            const Node &n_shape_map = n_topo.fetch_existing("elements/shape_map");

            if(n_topo.has_path("elements/sizes") && n_topo.has_path("elements/offsets"))
            {
                const auto sizes = n_topo.fetch_existing("elements/sizes").as_index_t_accessor();
                const auto offsets = n_topo.fetch_existing("elements/offsets").as_index_t_accessor();

                if(n_topo.has_path("subelements/connectivity") &&
                   n_topo.has_path("subelements/sizes") &&
                   n_topo.has_path("subelements/offsets"))
                {
                    // This input topology contains some polyhedra.
                    const auto se_conn = n_topo.fetch_existing("subelements/connectivity").as_index_t_accessor();
                    const auto se_sizes = n_topo.fetch_existing("subelements/sizes").as_index_t_accessor();
                    const auto se_offsets = n_topo.fetch_existing("subelements/offsets").as_index_t_accessor();

                    // Determine the value being used for polyhedra from the shape_map.
                    index_t phShape = -1;
                    for(index_t i = 0; i < n_shape_map.number_of_children(); i++)
                    {
                        const Node &n_shape = n_shape_map[i];
                        if(n_shape.name() == "polyhedral")
                        {
                            phShape = n_shape.to_int();
                            break;
                        }
                    }
 
                    for(size_t i = 0; i < element_ids.size(); i++)
                    {
                        const auto zsize = sizes[element_ids[i]];
                        const auto zoffset = offsets[element_ids[i]];
                        const auto zshape = shapes[element_ids[i]];
                        if(zshape == phShape)
                        {
                            // The connectivity for this zone contains face ids.
                            // Loop over face definitions and select face nodes.
                            for(index_t j = 0; j < zsize; j++)
                            {
                                const auto faceIndex = conn[zoffset + j];
                                const auto faceSize = se_sizes[faceIndex];
                                const auto faceOffset = se_offsets[faceIndex];
                                for(index_t k = 0; k < faceSize; k++)
                                {
                                    vertex_ids.insert(se_conn[faceOffset + k]);
                                }
                            }
                        }
                        else
                        {
                            // The connectivity contains nodes.
                            for(index_t j = 0; j < zsize; j++)
                            {
                                vertex_ids.insert(conn[zoffset + j]);
                            }
                        }
                    }
                }
                else
                {
                    // no subelements.
                    for(size_t i = 0; i < element_ids.size(); i++)
                    {
                        const auto zsize = sizes[element_ids[i]];
                        const auto zoffset = offsets[element_ids[i]];
                        for(index_t j = 0; j < zsize; j++)
                        {
                            vertex_ids.insert(conn[zoffset + j]);
                        }
                    }
                }
            }
            else
            {
                CONDUIT_ERROR("Unstructured topologies with mixed elements require sizes and offsets.");
            }
        }
        else
        {
            // Shapes are single types one after the next in the connectivity.
            const auto conn = n_topo["elements/connectivity"].as_index_t_accessor();
            auto nverts_in_shape = conduit::blueprint::mesh::utils::TOPO_SHAPE_INDEX_COUNTS[shape.id];
            for(size_t i = 0; i < element_ids.size(); i++)
            {
                auto offset = element_ids[i] * nverts_in_shape;
                for(index_t j = 0; j < nverts_in_shape; j++)
                    vertex_ids.insert(conn[offset + j]);
            }
        }
    }
}

//---------------------------------------------------------------------------
inline void
index_t_set_to_vector(const std::set<index_t> &src, std::vector<index_t> &dest)
{
    dest.reserve(src.size());
    for(auto it = src.begin(); it != src.end(); it++)
        dest.push_back(*it);
}

//---------------------------------------------------------------------------
conduit::Node *
Partitioner::extract(size_t idx, const conduit::Node &n_mesh, std::vector<index_t>& vertex_ids_out) const
{
    if(idx >= selections.size())
        return nullptr;

    conduit::Node *retval = nullptr;
    try
    {
        // Get the appropriate topology and coordset nodes.
        const conduit::Node &n_topo = selections[idx]->selected_topology(n_mesh);
        const conduit::Node &n_coordsets = n_mesh["coordsets"];
        std::string csname(n_topo["coordset"].as_string());
        const conduit::Node &n_coordset = n_coordsets[csname];

        // Make output.
        retval = new conduit::Node;
        conduit::Node &n_output = *retval;
        conduit::Node &n_new_coordsets = n_output["coordsets"];
        conduit::Node &n_new_topos = n_output["topologies"];

        // Copy state.
        if(n_mesh.has_child("state"))
        {
            const conduit::Node &n_state = n_mesh["state"];
            n_output["state"].set(n_state);
            // Override the dd if we have set it.
            int dd = selections[idx]->get_destination_domain();
            if(dd != Selection::FREE_DOMAIN_ID)
                n_output["state/domain_id"] = dd;
        }

        // Get the indices of the selected elements.
        std::vector<index_t> element_ids, vertex_ids;
        selections[idx]->get_element_ids(n_mesh, element_ids);

        // Try special case for logical selections. We do this so we can make
        // logically structured outputs.
        auto log_sel = dynamic_cast<SelectionLogical *>(selections[idx].get());
        if(log_sel != nullptr)
        {
            index_t start[3], end[3];
            log_sel->get_start(start[0], start[1], start[2]);
            log_sel->get_end(end[0], end[1], end[2]);

            // Get the vertex ids of the selected elements. We can do it like
            // this faster than using a set. (needed for field extraction)
            log_sel->get_vertex_ids(n_mesh, vertex_ids);

            if(n_coordset["type"].as_string() == "uniform")
                create_new_uniform_coordset(n_coordset, start, end, n_new_coordsets[csname]);
            else if(n_coordset["type"].as_string() == "rectilinear")
                create_new_rectilinear_coordset(n_coordset, start, end, n_new_coordsets[csname]);
            else
                create_new_explicit_coordset(n_coordset, vertex_ids, n_new_coordsets[csname]);

            // Now, create new topologies.
            if(n_topo["type"].as_string() == "uniform")
                create_new_uniform_topo(n_topo, csname, start, n_new_topos[n_topo.name()]);
            else if(n_topo["type"].as_string() == "rectilinear")
                create_new_rectilinear_topo(n_topo, csname, start, n_new_topos[n_topo.name()]);
            else if(n_topo["type"].as_string() == "structured")
                create_new_structured_topo(n_topo, csname, start, end, n_new_topos[n_topo.name()]);
        }
        else
        {
            // Get the unique set of vertex ids used by the elements.
            std::set<index_t> vertex_ids_set;
            get_vertex_ids_for_element_ids(n_topo, element_ids, vertex_ids_set);
            index_t_set_to_vector(vertex_ids_set, vertex_ids);

            // Create a new coordset consisting of the selected vertex ids.
            create_new_explicit_coordset(n_coordset, vertex_ids, n_new_coordsets[csname]);
            index_t originalCoordsetLength = conduit::blueprint::mesh::coordset::length(n_coordset);

            // Create a new topology consisting of the selected element ids.
            create_new_unstructured_topo(n_topo, csname, originalCoordsetLength,
                element_ids, vertex_ids, n_new_topos[n_topo.name()]);
        }

        // Create new matsets
        copy_matsets(n_topo.name(), element_ids, n_mesh, n_output);

        // Create new fields.
        copy_fields(selections[idx]->get_domain(), n_topo.name(),
            vertex_ids, element_ids, n_mesh, n_output);

        vertex_ids_out = std::move(vertex_ids);
    }
    catch(conduit::Error &e)
    {
        e.print();
        delete retval;
        retval = nullptr;
    }

    return retval;
}

//---------------------------------------------------------------------------
void
Partitioner::create_new_uniform_coordset(const conduit::Node &n_coordset,
    const index_t start[3], const index_t end[3], conduit::Node &n_new_coordset) const
{
    // Set coordset dimensions from element start/end.
    index_t ndims = conduit::blueprint::mesh::utils::coordset::dims(n_coordset);
    n_new_coordset["type"] = "uniform";
    n_new_coordset["dims/i"] = end[0] - start[0] + 2;
    n_new_coordset["dims/j"] = end[1] - start[1] + 2;
    if(ndims > 2)
        n_new_coordset["dims/k"] = end[2] - start[2] + 2;

    // Adjust the origins to the right start values.
    auto axes(conduit::blueprint::mesh::utils::coordset::axes(n_coordset));
    const conduit::Node &n_origin = n_coordset["origin"];
    const conduit::Node &n_spacing = n_coordset["spacing"];
    conduit::Node &n_new_origin = n_new_coordset["origin"];
    for(index_t i = 0; i < ndims; i++)
    {
        conduit::Node &origin_i = n_new_origin[n_origin[i].name()];
        double org = n_origin[i].to_double() + n_spacing[i].to_double() * start[i];
        origin_i.set(org);
    }
    // Copy all the spacings
    n_new_coordset["spacing"].set(n_coordset["spacing"]);
}

//---------------------------------------------------------------------------
void
Partitioner::create_new_rectilinear_coordset(const conduit::Node &n_coordset,
    const index_t start[3], const index_t end[3], conduit::Node &n_new_coordset) const
{
    const conduit::Node &n_values = n_coordset["values"];
    conduit::Node &n_new_values = n_new_coordset["values"];
    n_new_coordset["type"] = "rectilinear";
    // Slice each axis logically.
    index_t ndims = conduit::blueprint::mesh::utils::coordset::dims(n_coordset);
    for(index_t d = 0; d < ndims; d++)
    {
        std::vector<index_t> indices;
        auto nend = end[d] + 1;
        for(index_t i = start[d]; i <= nend; i++)
            indices.push_back(i);

        const conduit::Node &src = n_values[d];
        conduit::blueprint::mesh::utils::slice_array(src, indices, n_new_values[src.name()]);
    }
}

//---------------------------------------------------------------------------
void
Partitioner::create_new_explicit_coordset(const conduit::Node &n_coordset,
    const std::vector<index_t> &vertex_ids, conduit::Node &n_new_coordset) const
{
    conduit::Node n_explicit;
    n_new_coordset["type"] = "explicit";
    if(n_coordset["type"].as_string() == "uniform")
    {
        conduit::blueprint::mesh::coordset::uniform::to_explicit(n_coordset, n_explicit);

        auto axes = conduit::blueprint::mesh::utils::coordset::axes(n_explicit);
        const conduit::Node &n_values = n_explicit["values"];
        conduit::Node &n_new_values = n_new_coordset["values"];
        for(size_t i = 0; i < axes.size(); i++)
        {
            const conduit::Node &n_axis_values = n_values[axes[i]];
            conduit::Node &n_new_axis_values = n_new_values[axes[i]];
            conduit::blueprint::mesh::utils::slice_array(n_axis_values, vertex_ids, n_new_axis_values);
        }
    }
    else if(n_coordset["type"].as_string() == "rectilinear")
    {
        conduit::blueprint::mesh::coordset::rectilinear::to_explicit(n_coordset, n_explicit);

        auto axes = conduit::blueprint::mesh::utils::coordset::axes(n_explicit);
        const conduit::Node &n_values = n_explicit["values"];
        conduit::Node &n_new_values = n_new_coordset["values"];
        for(size_t i = 0; i < axes.size(); i++)
        {
            const conduit::Node &n_axis_values = n_values[axes[i]];
            conduit::Node &n_new_axis_values = n_new_values[axes[i]];
            conduit::blueprint::mesh::utils::slice_array(n_axis_values, vertex_ids, n_new_axis_values);
        }
    }
    else if(n_coordset["type"].as_string() == "explicit")
    {
        auto axes = conduit::blueprint::mesh::utils::coordset::axes(n_coordset);
        const conduit::Node &n_values = n_coordset["values"];
        conduit::Node &n_new_values = n_new_coordset["values"];
        for(size_t i = 0; i < axes.size(); i++)
        {
            const conduit::Node &n_axis_values = n_values[axes[i]];
            conduit::Node &n_new_axis_values = n_new_values[axes[i]];
            conduit::blueprint::mesh::utils::slice_array(n_axis_values, vertex_ids, n_new_axis_values);
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::create_new_uniform_topo(const conduit::Node &n_topo,
    const std::string &csname, const index_t start[3],
    conduit::Node &n_new_topo) const
{
    n_new_topo["type"] = "uniform";
    n_new_topo["coordset"] = csname;
    const char *keys[] = {"elements/origin/i",
        "elements/origin/j",
        "elements/origin/k"};
    for(int i = 0; i < 3; i++)
    {
        if(n_topo.has_path(keys[i]))
        {
            const conduit::Node &value = n_topo[keys[i]];
            n_new_topo[keys[i]].set(static_cast<conduit::int64>(value.to_uint64() + start[i]));
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::create_new_rectilinear_topo(const conduit::Node &n_topo,
    const std::string &csname, const index_t start[3],
    conduit::Node &n_new_topo) const
{
    n_new_topo["type"] = "rectilinear";
    n_new_topo["coordset"] = csname;
    const char *keys[] = {"elements/origin/i",
        "elements/origin/j",
        "elements/origin/k"};
    for(int i = 0; i < 3; i++)
    {
        if(n_topo.has_path(keys[i]))
        {
            const conduit::Node &value = n_topo[keys[i]];
            n_new_topo[keys[i]].set(static_cast<conduit::int64>(value.to_uint64() + start[i]));
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::create_new_structured_topo(const conduit::Node &n_topo,
    const std::string &csname, const index_t start[3], const index_t end[3],
    conduit::Node &n_new_topo) const
{
    n_new_topo["type"] = "structured";
    n_new_topo["coordset"] = csname;
    conduit::Node &n_dims = n_new_topo["elements/dims"];
    n_dims["i"].set(static_cast<conduit::int64>(end[0] - start[0] + 1));
    n_dims["j"].set(static_cast<conduit::int64>(end[1] - start[1] + 1));
    if(n_topo.has_path("elements/dims/k"))
        n_dims["k"].set(static_cast<conduit::int64>(end[2] - start[2] + 1));
    const char *keys[] = {"elements/origin/i0",
        "elements/origin/j0",
        "elements/origin/k0"};
    for(int i = 0; i < 3; i++)
    {
        if(n_topo.has_child(keys[i]))
        {
            const conduit::Node &value = n_topo[keys[i]];
            n_new_topo[keys[i]].set(static_cast<conduit::int64>(value.to_uint64() + start[i]));
        }
    }
}

//---------------------------------------------------------------------------
/**
 @note We pass in the coordset name because the conversion routines in here
       result in topologies that do not set a coordset name.
 */
void
Partitioner::create_new_unstructured_topo(const conduit::Node &n_topo,
    const std::string &csname,
    index_t originalCoordsetLength,
    const std::vector<index_t> &element_ids,
    const std::vector<index_t> &vertex_ids,
    conduit::Node &n_new_topo) const
{
    if(n_topo["type"].as_string() == "uniform")
    {
        conduit::Node n_uns, cdest; // what is cdest?
        conduit::blueprint::mesh::topology::uniform::to_unstructured(n_topo, n_uns, cdest);
        unstructured_topo_from_unstructured(n_uns, csname, originalCoordsetLength, element_ids, vertex_ids, n_new_topo);
    }
    else if(n_topo["type"].as_string() == "rectilinear")
    {
        conduit::Node n_uns, cdest; // what is cdest?
        conduit::blueprint::mesh::topology::rectilinear::to_unstructured(n_topo, n_uns, cdest);
        unstructured_topo_from_unstructured(n_uns, csname, originalCoordsetLength, element_ids, vertex_ids, n_new_topo);
    }
    else if(n_topo["type"].as_string() == "structured")
    {
        conduit::Node n_uns, cdest; // what is cdest?
        conduit::blueprint::mesh::topology::structured::to_unstructured(n_topo, n_uns, cdest);
        unstructured_topo_from_unstructured(n_uns, csname, originalCoordsetLength, element_ids, vertex_ids, n_new_topo);
    }
    else if(n_topo["type"].as_string() == "unstructured")
    {
        unstructured_topo_from_unstructured(n_topo, csname, originalCoordsetLength,
                                            element_ids, vertex_ids, n_new_topo);
    }
}

//---------------------------------------------------------------------------
void
Partitioner::unstructured_topo_from_unstructured(const conduit::Node &n_topo,
    const std::string &csname,
    index_t originalCoordsetLength,
    const std::vector<index_t> &element_ids,
    const std::vector<index_t> &vertex_ids,
    conduit::Node &n_new_topo) const
{
    n_new_topo["type"].set("unstructured");
    n_new_topo["coordset"].set(csname);

    // vertex_ids contains the list of old vertex ids that our selection uses
    // from the old coordset. It can serve as a new to old map.
    std::vector<index_t> old2new(originalCoordsetLength, INVALID_NODE);
    for(size_t i = 0; i < vertex_ids.size(); i++)
    {
        old2new[vertex_ids[i]] = static_cast<index_t>(i);
    }
#if 0
    // Print the old2new map.
    std::cout << "unstructured_topo_from_unstructured: old2new={";
    for(size_t i = 0; i < vertex_ids.size(); i++)
    {
        std::cout << old2new[i] << ", ";
    }
    std::cout << "}\n";
#endif
    conduit::blueprint::mesh::utils::ShapeType shape(n_topo);
    std::vector<index_t> new_conn;
    if(shape.is_polygonal())
    {
        unstructured_topo_from_polygonal(n_topo, element_ids, old2new, n_new_topo);
    }
    else if(shape.is_polyhedral())
    {
        unstructured_topo_from_polyhedral(n_topo, element_ids, old2new, n_new_topo);
    }
    else if(n_topo.has_path("elements/element_types"))
    {
        unstructured_topo_from_stream(n_topo, element_ids, old2new, n_new_topo);
    }
    else if(n_topo.fetch_existing("elements/shape").as_string() == "mixed")
    {
        unstructured_topo_from_mixed(n_topo, element_ids, old2new, n_new_topo);
    }
    else
    {
        // Shapes are single types one after the next in the connectivity.
        conduit::Node n_indices;
        as_index_t(n_topo["elements/connectivity"], n_indices);
        auto iptr = as_index_t_array(n_indices);

        auto nverts_in_shape = conduit::blueprint::mesh::utils::TOPO_SHAPE_INDEX_COUNTS[shape.id];
        for(size_t i = 0; i < element_ids.size(); i++)
        {
            auto offset = element_ids[i] * nverts_in_shape;
#if 0
            cout << "cell " << element_ids[i] << ":  old(";
            for(index_t j = 0; j < nverts_in_shape; j++)
                cout << iptr[offset + j] << ", ";
            cout << "), new(";
            for(index_t j = 0; j < nverts_in_shape; j++)
            {
                auto it = old2new.find(iptr[offset + j]);
                if(it == old2new.end())
                    cout << "ERROR" << ", ";
                else
                    cout << it->second << ", ";
            }
            cout << ")" << endl;
#endif
            for(index_t j = 0; j < nverts_in_shape; j++)
                new_conn.push_back(old2new[iptr[offset + j]]);
        }

        n_new_topo["elements/shape"].set(n_topo["elements/shape"]);
        n_new_topo["elements/connectivity"].set(new_conn);
    }
}

//---------------------------------------------------------------------------
void
Partitioner::unstructured_topo_from_polygonal(const conduit::Node &n_topo,
    const std::vector<index_t> &element_ids,
    const std::vector<index_t> &old2new,
    conduit::Node &n_new_topo) const
{
    conduit::Node n_indices;
    as_index_t(n_topo["elements/connectivity"], n_indices);
    auto iptr = as_index_t_array(n_indices);

    // Make sure we have offsets we can use.
    conduit::Node n_offsets, n_sizes;
#ifdef CONDUIT_INDEX_32
    if(n_topo.has_path("elements/offsets"))
        n_topo["elements/offsets"].to_unsigned_int_array(n_offsets);
    else
        conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(n_topo, n_offsets);
    auto offsets = n_offsets.as_unsigned_int_array();
#else
    if(n_topo.has_path("elements/offsets"))
        n_topo["elements/offsets"].to_unsigned_long_array(n_offsets);
    else
        conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(n_topo, n_offsets);
    auto offsets = n_offsets.as_unsigned_long_array();
#endif
    n_topo["elements/sizes"].to_unsigned_int_array(n_sizes);
    auto sizes = n_sizes.as_unsigned_int_array();

    std::vector<index_t> new_conn, new_offsets, new_sizes;
    index_t next_offset = 0;
    for(size_t i = 0; i < element_ids.size(); i++)
    {
        auto offset = offsets[element_ids[i]];
        auto sz = sizes[element_ids[i]];
        for(unsigned int ptid = 0; ptid < sz; ptid++)
            new_conn.push_back(old2new[iptr[offset + ptid]]);
        new_sizes.push_back(sz);
        new_offsets.push_back(next_offset);
        next_offset += sz;
    }
    n_new_topo["elements/shape"].set(n_topo["elements/shape"]);
    n_new_topo["elements/connectivity"].set(new_conn);
    n_new_topo["elements/sizes"] = new_sizes;
    n_new_topo["elements/offsets"] = new_offsets;
}

//---------------------------------------------------------------------------
void
Partitioner::unstructured_topo_from_polyhedral(const conduit::Node &n_topo,
    const std::vector<index_t> &element_ids,
    const std::vector<index_t> &old2new,
    conduit::Node &n_new_topo) const
{
    const auto conn = n_topo["elements/connectivity"].as_index_t_accessor();

    // NOTE: I'm assuming that offsets are built already.
    const auto offsets = n_topo["elements/offsets"].as_index_t_accessor();
    const auto sizes = n_topo["elements/sizes"].as_index_t_accessor();

    const auto se_conn = n_topo["subelements/connectivity"].as_index_t_accessor();
    const auto se_offsets = n_topo["subelements/offsets"].as_index_t_accessor();
    const auto se_sizes = n_topo["subelements/sizes"].as_index_t_accessor();

    // Figure out the index of each face being used. The first use of a face
    // will put 0 in faceOrientation, the second use will be 1.
    const auto maxFace = conn.max() + 1;
    const auto connlen = conn.number_of_elements();
    std::vector<uint8> faceCount(maxFace, 0);
    std::vector<uint8> faceOrientation(connlen, 0);
    for(index_t i = 0; i < connlen; i++)
    {
        const auto faceId = conn[i];
        faceOrientation[i] = faceCount[faceId]++;
    }

    std::map<index_t,index_t> old2new_faces;
    std::vector<index_t> new_conn, new_sizes, new_offsets, new_se_conn, new_se_sizes, new_se_offsets;

    const index_t old2new_size = static_cast<index_t>(old2new.size());
    index_t new_offset = 0, new_se_offset = 0;
    for(auto eid : element_ids)
    {
        const auto offset = offsets[eid];
        const auto nfaces = sizes[eid];
        for(index_t fi = 0; fi < nfaces; fi++)
        {
            const auto face_id = conn[offset + fi];
            const auto it = old2new_faces.find(face_id);
            if(it == old2new_faces.end())
            {
                // We have not seen the face before. Add it.
                const auto new_face_id = static_cast<index_t>(old2new_faces.size());
                old2new_faces[face_id] = new_face_id;
                new_conn.push_back(new_face_id);

                const auto face_offset = se_offsets[face_id];
                const auto face_nverts = se_sizes[face_id];

                // Get the orientation for the face. If it is 0 then the source
                // PH element was the first to define the face so we can add it in
                // the same order as the new PH mesh should be the first to see it.
                // If orientation is not zero then the source PH mesh was not the
                // first to define it. We assume the face was defined relative to
                // a different element and we must flip vertex ordering to make the
                // normals point the right way.
                const auto orientation = faceOrientation[offset + fi];
                if(orientation == 0)
                {
                    // Add vertices in normal order
                    for(index_t vi = 0; vi < face_nverts; vi++)
                    {
                        const auto vid = se_conn[face_offset + vi];

                        if(vid >= old2new_size || old2new[vid] == INVALID_NODE)
                        {
                            CONDUIT_ERROR("No vertex " << vid << " in old2new.");
                        }
                        const auto nvid = old2new[vid];
                        new_se_conn.push_back(nvid);
                    }
                }
                else
                {
                    // Add vertices in reverse order so the new face is
                    // defined relative to this element.
                    for(index_t vi = face_nverts-1; vi >= 0; vi--)
                    {
                        const auto vid = se_conn[face_offset + vi];

                        if(vid >= old2new_size || old2new[vid] == INVALID_NODE)
                        {
                            CONDUIT_ERROR("No vertex " << vid << " in old2new.");
                        }

                        const auto nvid = old2new[vid];
                        new_se_conn.push_back(nvid);
                    }
                }

                new_se_sizes.push_back(face_nverts);
                new_se_offsets.push_back(new_se_offset);
                new_se_offset += face_nverts;
            }
            else
            {
                // We've seen the face before. Reference it.
                new_conn.push_back(it->second);
            }
        }

        new_sizes.push_back(nfaces);
        new_offsets.push_back(new_offset);
        new_offset += nfaces;
    }

    n_new_topo["elements/sizes"].set(new_sizes);
    n_new_topo["elements/offsets"].set(new_offsets);
    n_new_topo["elements/shape"].set(n_topo["elements/shape"]);
    n_new_topo["elements/connectivity"].set(new_conn);
    n_new_topo["subelements/shape"].set(n_topo["subelements/shape"].as_string());
    n_new_topo["subelements/connectivity"].set(new_se_conn);
    n_new_topo["subelements/sizes"].set(new_se_sizes);
    n_new_topo["subelements/offsets"].set(new_se_offsets);
}

//---------------------------------------------------------------------------
void
Partitioner::unstructured_topo_from_stream(const conduit::Node &n_topo,
    const std::vector<index_t> &element_ids,
    const std::vector<index_t> &old2new,
    conduit::Node &n_new_topo) const
{
    const conduit::Node &n_element_types = n_topo["elements/element_types"];

    // Determine the number of points for each stream id and shape.
    std::map<int,int> stream_id_npts;
    std::set<int> unique_shape_types;
    std::map<std::string, int> shape_stream_id;
    std::map<int, std::string> stream_id_shape;

    for(index_t i = 0; i < n_element_types.number_of_children(); i++)
    {
        const conduit::Node &n = n_element_types[i];
        auto stream_id = n["stream_id"].to_int();
        std::string shape_str(n["shape"].as_string());
        for(size_t j = 0; j < utils::TOPO_SHAPES.size(); j++)
        {
            if(shape_str == utils::TOPO_SHAPES[j])
            {
                stream_id_npts[stream_id] = utils::TOPO_SHAPE_INDEX_COUNTS[j];
                unique_shape_types.insert(utils::TOPO_SHAPE_INDEX_COUNTS[j]);
                shape_stream_id[shape_str] = stream_id;
                stream_id_shape[stream_id] = shape_str;
                break;
            }
        }
    }

    if(unique_shape_types.size() > 1)
    {
        // Elements are different types.

        conduit::Node n_stream_ids, n_element_counts, n_stream;
        as_index_t(n_topo["elements/element_index/stream_ids"], n_stream_ids);
        as_index_t(n_topo["elements/element_index/element_counts"], n_element_counts);
        as_index_t(n_topo["elements/stream"], n_stream);
        auto stream_ids = as_index_t_array(n_stream_ids);
        auto element_counts = as_index_t_array(n_element_counts);
        auto stream = as_index_t_array(n_stream);

        // Compute some size,offsets into the stream to help
        std::vector<index_t> stream_ids_expanded, offsets;
        index_t offset = 0;
        for(index_t j = 0; j < stream_ids.number_of_elements(); j++)
        {
            auto n = static_cast<index_t>(element_counts[j]);
            auto npts = stream_id_npts[stream_ids[j]];
            for(index_t i = 0; i < n; i++)
            {
                stream_ids_expanded.push_back(stream_ids[j]);
                offsets.push_back(offset);
                offset += npts;
            }
        }

        // Now, for each element, add its topology to the new stream.
        int current_stream_id = -1;
        index_t current_stream_count = 0;
        std::vector<index_t> new_conn, new_stream_ids, new_element_counts;
        for(size_t i = 0; i < element_ids.size(); i++)
        {
            int sid = stream_ids_expanded[element_ids[i]];
            auto npts = stream_id_npts[sid];

            // Save the element's vertices into the new stream.
            for(index_t j = 0; j < npts; j++)
            {
                auto vid = stream[offsets[element_ids[i]] + j];
                new_conn.push_back(old2new[vid]);
            }

            if(current_stream_id == -1)
            {
                current_stream_id = sid;
                current_stream_count = 1;
            }
            else if(current_stream_id != sid)
            {
                new_stream_ids.push_back(current_stream_id);
                new_element_counts.push_back(current_stream_count);

                current_stream_id = sid;
                current_stream_count = 1;
            }
            else
            {
                current_stream_count++;
            }
        }
        if(current_stream_count > 0)
        {
            new_stream_ids.push_back(current_stream_id);
            new_element_counts.push_back(current_stream_count);
        }

        n_new_topo["elements/element_types"].set(n_topo["elements/element_types"]);
        n_new_topo["elements/element_index/stream_ids"].set(new_stream_ids);
        n_new_topo["elements/element_index/element_counts"].set(new_element_counts);
        n_new_topo["elements/stream"].set(new_conn);
    }
    else
    {
        // All elements are actually a single type.
        int nverts_in_shape = -1;
        const conduit::Node &n_et = n_topo["elements/element_types"][0];
        std::string shape_str(n_et["shape"].as_string());
        for(size_t j = 0; j < utils::TOPO_SHAPES.size(); j++)
        {
            if(shape_str == utils::TOPO_SHAPES[j])
            {
                nverts_in_shape = utils::TOPO_SHAPE_INDEX_COUNTS[j];
                break;
            }
        }
        if(nverts_in_shape == -1)
        {
            CONDUIT_ERROR("Invalid shape");
        }
        else
        {
            std::vector<index_t> new_conn;
            const auto conn = n_topo["elements/stream"].as_index_t_accessor();
            for(size_t i = 0; i < element_ids.size(); i++)
            {
                auto offset = element_ids[i] * nverts_in_shape;
                for(index_t j = 0; j < nverts_in_shape; j++)
                    new_conn.push_back(old2new[conn[offset + j]]);
            }

            n_new_topo["elements/shape"].set(n_topo["elements/shape"]);
            n_new_topo["elements/connectivity"].set(new_conn);
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::unstructured_topo_from_mixed(const conduit::Node &n_topo,
    const std::vector<index_t> &element_ids,
    const std::vector<index_t> &old2new,
    conduit::Node &n_new_topo) const
{
    // Make map of shape value to name from the shape map.
    const conduit::Node &n_shape_map = n_topo.fetch_existing("elements/shape_map");
    std::map<int, std::string> value2shape;
    bool hasPolyhedra = false;
    index_t phShape = -1;
    for(index_t i = 0; i < n_shape_map.number_of_children(); i++)
    {
        const conduit::Node &n_shape = n_shape_map[i];
        value2shape[n_shape.to_int()] = n_shape.name();
        if(n_shape.name() == "polyhedral")
        {
            hasPolyhedra = true;
            phShape = n_shape.to_int();
        }
    }

    // Get the topology data as accessors.
    const auto conn = n_topo["elements/connectivity"].as_index_t_accessor();
    const auto sizes = n_topo["elements/sizes"].as_index_t_accessor();
    const auto offsets = n_topo["elements/offsets"].as_index_t_accessor();
    const auto shapes = n_topo["elements/shapes"].as_index_t_accessor();

    const auto newNumZones = static_cast<index_t>(element_ids.size());
    std::set<int> selectedShapes;

    // Populate the new topology.
    if(hasPolyhedra)
    {
        const auto se_conn = n_topo["subelements/connectivity"].as_index_t_accessor();
        const auto se_sizes = n_topo["subelements/sizes"].as_index_t_accessor();
        const auto se_offsets = n_topo["subelements/offsets"].as_index_t_accessor();
        const auto se_shapes = n_topo["subelements/shapes"].as_index_t_accessor();

        std::vector<index_t> new_conn, new_sizes, new_offsets, new_shapes;
        std::vector<index_t> new_se_conn, new_se_sizes, new_se_offsets, new_se_shapes;
        std::map<index_t, index_t> oldFace2newFace;
        for(index_t i = 0; i < newNumZones; i++)
        {
            const auto elemId = element_ids[i];
            const auto zsize = sizes[elemId];
            const auto zoffset = offsets[elemId];
            const auto zshape = shapes[elemId];
            selectedShapes.insert(zshape);

            new_sizes.push_back(zsize);
            new_offsets.push_back(static_cast<index_t>(new_conn.size()));
            new_shapes.push_back(zshape);

            if(zshape == phShape)
            {
                for(index_t fi = 0; fi < zsize; fi++)
                {
                    const index_t oldFaceId = conn[zoffset + fi];
                    index_t newFaceId = -1;
                    const auto it = oldFace2newFace.find(oldFaceId);
                    if(it == oldFace2newFace.end())
                    {
                        // Make new face.
                        const auto faceOffset = se_offsets[oldFaceId];
                        const auto faceSize = se_sizes[oldFaceId];
                        const auto faceShape = se_shapes[oldFaceId];

                        newFaceId = static_cast<index_t>(new_se_sizes.size());
                        new_se_sizes.push_back(faceSize);
                        new_se_offsets.push_back(new_se_conn.size());
                        new_se_shapes.push_back(faceShape);

                        for(index_t index = 0; index < faceSize; index++)
                        {
                            new_se_conn.push_back(old2new[se_conn[faceOffset + index]]);
                        }
                    }
                    else
                    {
                        newFaceId = it->second;
                    }
                    new_conn.push_back(newFaceId);
                }
            }
            else
            {
                // Copy and map node ids
                for(index_t j = 0; j < zsize; j++)
                {
                   new_conn.push_back(old2new[conn[zoffset + j]]);
                }
            }
        }

        n_new_topo["elements/shape"] = "mixed";
        n_new_topo["elements/connectivity"].set(new_conn);
        n_new_topo["elements/sizes"].set(new_sizes);
        n_new_topo["elements/offsets"].set(new_offsets);
        n_new_topo["elements/shapes"].set(new_shapes);
        if(!new_se_sizes.empty())
        {
            n_new_topo["subelements/shape"] = "mixed";
            n_new_topo["subelements/connectivity"].set(new_se_conn);
            n_new_topo["subelements/sizes"].set(new_se_sizes);
            n_new_topo["subelements/offsets"].set(new_se_offsets);
            n_new_topo["subelements/shapes"].set(new_se_shapes);
            if(n_topo.has_path("subelements/shape_map"))
            {
                n_new_topo["subelements/shape_map"].set(n_topo["subelements/shape_map"]);
            }
        }
    }
    else
    {
        // Figure out new connectivity size.
        index_t newConnSize = 0;
        for(index_t i = 0; i < newNumZones; i++)
        {
          newConnSize += sizes[element_ids[i]];
        }

        // Set up new arrays.
        n_new_topo["elements/shape"] = "mixed";
        n_new_topo["elements/connectivity"].set(DataType::index_t(newConnSize));
        n_new_topo["elements/sizes"].set(DataType::index_t(newNumZones));
        n_new_topo["elements/offsets"].set(DataType::index_t(newNumZones));
        n_new_topo["elements/shapes"].set(DataType::index_t(newNumZones));
        index_t *new_conn = n_new_topo["elements/connectivity"].as_index_t_ptr();
        index_t *new_sizes = n_new_topo["elements/sizes"].as_index_t_ptr();
        index_t *new_offsets = n_new_topo["elements/offsets"].as_index_t_ptr();
        index_t *new_shapes = n_new_topo["elements/shapes"].as_index_t_ptr();

        index_t newOffset = 0;
        for(index_t i = 0; i < newNumZones; i++)
        {
            const auto elemId = element_ids[i];
            const auto zsize = sizes[elemId];
            const auto zoffset = offsets[elemId];
            const auto zshape = shapes[elemId];

            new_sizes[i] = zsize;
            new_offsets[i] = newOffset;
            new_shapes[i] = zshape;
            selectedShapes.insert(zshape);

            // Copy and map node ids
            for(index_t j = 0; j < zsize; j++)
            {
                new_conn[newOffset + j] = old2new[conn[zoffset + j]];
            }

            newOffset += zsize;
        }
    }

    // Build new shape map.
    for(auto s : selectedShapes)
    {
        n_new_topo["elements/shape_map/" + value2shape[s]] = s;
    }
}

//---------------------------------------------------------------------------
/**
 @brief This method wraps the input node and adds original elements/vertices
        to it since downstream it is best that these fields exist.
 */
conduit::Node *
Partitioner::wrap(size_t idx, const conduit::Node &n_mesh) const
{
    conduit::Node *n_wrap = new conduit::Node;

    // Add the members from n_mesh into n_wrap but treat fields specially.
    for(index_t i = 0; i < n_mesh.number_of_children(); i++)
    {
        const conduit::Node &f = n_mesh[i];
        if(f.name() != "fields")
            (*n_wrap)[f.name()].set_external_node(f);
    }
    conduit::Node &n_new_fields = (*n_wrap)["fields"];
    // Make references to any fields from the input into the n_wrap/fields node.
    if(n_mesh.has_path("fields"))
    {
        const conduit::Node &n_fields = n_mesh["fields"];
        for(index_t i = 0; i < n_fields.number_of_children(); i++)
        {
            const conduit::Node &f = n_fields[i];
            n_new_fields[f.name()].set_external_node(f);
        }
    }

    const conduit::Node &n_topo = selections[idx]->selected_topology(n_mesh);
    std::string toponame(n_topo.name());
    index_t nelements = conduit::blueprint::mesh::topology::length(n_topo);
    index_t domain = selections[idx]->get_domain();
    std::string csname(n_topo["coordset"].as_string());
    index_t nvertices = 0;
    if(n_mesh.has_child("coordsets"))
    {
        const conduit::Node &n_cs = n_mesh["coordsets"];
        nvertices = mesh::utils::coordset::length(n_cs[csname]);
    }

    // Save the vertex_ids as a new MC field.
    if(nvertices > 0)
    {
        conduit::Node &n_field = n_new_fields[original_vertex_ids_name()];
        n_field["association"] = "vertex";
        if(!toponame.empty())
            n_field["topology"] = toponame;
        std::vector<index_t> vertex_ids(nvertices);
        for(index_t i = 0; i < nvertices; i++)
            vertex_ids[i] = i;
        std::vector<index_t> domain_ids(nvertices, domain);
        n_field["values/domains"].set(domain_ids);
        n_field["values/ids"].set(vertex_ids);
    }

    // Save the element_ids as a new MC field.
    if(nelements > 0)
    {
        conduit::Node &n_field = n_new_fields[original_element_ids_name()];
        n_field["association"] = "element";
        if(!toponame.empty())
            n_field["topology"] = toponame;
        std::vector<index_t> elem_ids(nelements);
        for(index_t i = 0; i < nelements; i++)
            elem_ids[i] = i;
        std::vector<index_t> domain_ids(nelements, domain);
        n_field["values/domains"].set(domain_ids);
        n_field["values/ids"].set(elem_ids);
    }

    return n_wrap;
}

//---------------------------------------------------------------------------
static const conduit::Node* get_associated_topo_adjset(const conduit::Node& domain,
                                                       const std::string& topo)
{
    if (!domain.has_child("adjsets"))
    {
        return nullptr;
    }
    for (const conduit::Node& adjset : domain["adjsets"].children())
    {
        const std::string& adjset_topo = adjset["topology"].as_string();
        const std::string& adjset_assoc = adjset["association"].as_string();
        if (topo == adjset_topo && adjset_assoc == "vertex")
        {
            return &adjset;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------
void
Partitioner::build_intradomain_adjsets(const std::vector<int>& chunk_offsets,
                                       const DomainToChunkMap& dom_2_chunks,
                                       std::vector<conduit::Node*>& adjset_data)
{
    index_t chunk_offset = chunk_offsets[rank];
    for (const auto& it : dom_2_chunks)
    {
        // Get the chunk ids in the current domain.
        const auto& chunks_2_verts = it.second;
        std::vector<index_t> chunk_ids;
        for (const auto& kv : chunks_2_verts)
        {
            chunk_ids.push_back(kv.first);
        }

        // Loop over pairs of chunks.
        for (size_t ci = 0; ci < chunk_ids.size(); ci++)
        {
            for (size_t cj = ci+1; cj < chunk_ids.size(); cj++)
            {
                index_t i_chunk = chunk_ids[ci];
                index_t j_chunk = chunk_ids[cj];

                const std::vector<index_t>& i_verts = chunks_2_verts.find(i_chunk)->second;
                const std::vector<index_t>& j_verts = chunks_2_verts.find(j_chunk)->second;

                std::vector<index_t> i_shared, j_shared;

                size_t ividx = 0, jvidx = 0;

                // Perform the intersection between the two vertex sets.
                // NOTE: these arrays should be sorted since they're initialized
                // from std::sets - if they aren't, the below won't work
                while (ividx < i_verts.size() && jvidx < j_verts.size())
                {
                    if (i_verts[ividx] < j_verts[jvidx])
                    {
                        ividx++;
                    }
                    else if (i_verts[ividx] > j_verts[jvidx])
                    {
                        jvidx++;
                    }
                    else // i_verts[ividx] == j_verts[jvidx]
                    {
                        i_shared.push_back(ividx);
                        j_shared.push_back(jvidx);
                        ividx++;
                        jvidx++;
                    }
                }

                if (adjset_data[i_chunk] == nullptr || adjset_data[j_chunk] == nullptr)
                {
                    continue;
                }

                // Add the chunk intersections to their corresponding adjsets.
                for (Node& adjset_i : adjset_data[i_chunk]->children())
                {
                    Node& adjset_groups_i = adjset_i["groups"];
                    Node& new_set = adjset_groups_i.append();
                    new_set["neighbors"].set(chunk_offset + j_chunk);
                    new_set["values"].set(i_shared);
                }

                for (Node& adjset_j : adjset_data[j_chunk]->children())
                {
                    Node& adjset_groups_j = adjset_j["groups"];
                    Node& new_set = adjset_groups_j.append();
                    new_set["neighbors"].set(chunk_offset + i_chunk);
                    new_set["values"].set(j_shared);
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::init_chunk_adjsets(const std::vector<const Node*>& chunk_assoc_adjset,
                                std::vector<Node*>& adjset_data)
{
    for (size_t icnk = 0; icnk < chunk_assoc_adjset.size(); icnk++)
    {
        if (!chunk_assoc_adjset[icnk]) { continue; }
        const Node& adjset = *chunk_assoc_adjset[icnk];
        const std::string adjset_name = adjset.name();
        const std::string adjset_topo = adjset["topology"].as_string();
        const std::string adjset_assoc = adjset["association"].as_string();

        Node& new_adjset = adjset_data[icnk]->fetch(adjset_name);
        new_adjset["association"].set("vertex");
        new_adjset["topology"].set(adjset_topo);
    }
}


//---------------------------------------------------------------------------
void
Partitioner::get_prelb_adjset_maps(const std::vector<int>& chunk_offsets,
                                   const DomainToChunkMap& chunks,
                                   const std::map<index_t, const Node*>& domain_map,
                                   std::vector<Node>& adjset_chunk_maps)
{
    index_t chunk_offset = chunk_offsets[rank];

    // This might be alloced by the derived ParallelPartitioner calling into
    // this function; otherwise, we allocate based on the largest local domain
    // index
    if (domain_map.size() > 0)
    {
        if ( static_cast<index_t>(adjset_chunk_maps.size()) <
             static_cast<index_t>(domain_map.rbegin()->first + 1))
        {
            adjset_chunk_maps.resize(domain_map.rbegin()->first + 1);
        }
    }

    // Build adjset-to-chunk maps for all of our local domains.
    for (const auto& dom_it : domain_map)
    {
        const Node& domain = *(dom_it.second);
        const index_t dom_idx = dom_it.first;
        if (!domain.has_child("adjsets"))
        {
            continue;
        }
        const ChunkToVertsMap& domain_chunks = chunks.find(&domain)->second;
        Node& dom_out = adjset_chunk_maps[dom_idx];
        // Create map of old vertex ids to chunk ids
        std::vector<std::vector<index_t>> vtx_to_cnkid;
        size_t max_verts = 0;
        if (domain_chunks.size() != 1)
        {
            for (const auto& cnk : domain_chunks)
            {
                // Ensure enough space is in the map this chunks vertices
                max_verts = std::max<size_t>(max_verts, *(cnk.second.rbegin())+1);
                vtx_to_cnkid.resize(max_verts);
                for (index_t idx : cnk.second)
                {
                    vtx_to_cnkid[idx].push_back(cnk.first + chunk_offset);
                }
            }
        }

        // loop over all adjsets in the current domain
        for (const Node& adjset : domain["adjsets"].children())
        {
            // Make sure the input adjset is pairwise.
            std::string adjset_name(adjset.name());
            conduit::Node pwadjset;
            if(conduit::blueprint::mesh::adjset::is_pairwise(adjset))
            {
                pwadjset.set_external(adjset);
            }
            else
            {
                conduit::blueprint::mesh::adjset::to_pairwise(adjset, pwadjset);
            }

            const std::string adjset_topo = pwadjset["topology"].as_string();
            const std::string adjset_assoc = pwadjset["association"].as_string();
            // TODO: check adjset for vertex associativity.
            Node& out_adjset_map = dom_out[adjset_name];
            for (const Node& group : pwadjset["groups"].children())
            {
                index_t_accessor neighbor_vals = group["neighbors"].as_index_t_accessor();
                if (neighbor_vals.number_of_elements() != 1)
                {
                    CONDUIT_ERROR("Expected pairwise adjset: 1 neighbor per group, got "
                            << neighbor_vals.number_of_elements());
                }
                index_t dst_dom = neighbor_vals[0];
                index_t_accessor adj_vals = group["values"].as_index_t_accessor();
                size_t nvals = adj_vals.number_of_elements();

                std::vector<index_t> offsets;
                std::vector<index_t> csr_vtx_map;
                if (vtx_to_cnkid.size() == 0)
                {
                    // single chunk comprises the whole domain
                    index_t dst_cnk = domain_chunks.begin()->first + chunk_offset;
                    offsets.resize(nvals);
                    std::iota(offsets.begin(), offsets.end(), 0);
                    csr_vtx_map = std::vector<index_t>(nvals, dst_cnk);
                }
                else
                {
                    index_t curr_offset = 0;
                    for (size_t i = 0; i < nvals; i++)
                    {
                        index_t old_vtx = adj_vals[i];
                        offsets.push_back(curr_offset);
                        curr_offset += vtx_to_cnkid[old_vtx].size();
                        csr_vtx_map.insert(csr_vtx_map.end(),
                                           vtx_to_cnkid[old_vtx].begin(),
                                           vtx_to_cnkid[old_vtx].end());
                    }
                }

                // Index by destination domain to make it a little easier to find.
                const std::string dst_key = std::to_string(dst_dom);
                out_adjset_map["to_dom"][dst_key]["group_name"] = group.name();
                out_adjset_map["to_dom"][dst_key]["cnk_offsets"].set(offsets);
                out_adjset_map["to_dom"][dst_key]["vtx_map"].set(csr_vtx_map);
            }
        }
    }
}

//---------------------------------------------------------------------------
void
Partitioner::build_interdomain_adjsets(const std::vector<int>& chunk_offsets,
                                       const DomainToChunkMap& dom_2_chunks,
                                       const std::map<index_t, const Node*>& domain_map,
                                       std::vector<conduit::Node*>& adjset_data)
{
    index_t chunk_offset = chunk_offsets[rank];
    std::unordered_map<const conduit::Node*,
        std::unordered_map<index_t, std::vector<conduit::Node*>>> remap_to_chunk_vid;

    std::vector<Node> dom_adjset_maps;
    get_prelb_adjset_maps(chunk_offsets, dom_2_chunks, domain_map, dom_adjset_maps);

    // Iterate over all local domains to generate new chunk-based adjsets.
    for (const auto& dom_it : domain_map)
    {
        const Node& domain = *(dom_it.second);
        const index_t dom_idx = dom_it.first;
        if (!domain.has_child("adjsets"))
        {
            continue;
        }
        for (const Node& adjset : domain["adjsets"].children())
        {
            // Make sure the input adjset is pairwise.
            std::string adjset_name(adjset.name());
            conduit::Node pwadjset;
            if(conduit::blueprint::mesh::adjset::is_pairwise(adjset))
            {
                pwadjset.set_external(adjset);
            }
            else
            {
                conduit::blueprint::mesh::adjset::to_pairwise(adjset, pwadjset);
            }

            const std::string adjset_topo = pwadjset["topology"].as_string();
            const std::string adjset_assoc = pwadjset["association"].as_string();
            const Node& adjset_maps = dom_adjset_maps[dom_idx][adjset_name];
            // Maps (local_chunk, remote_chunk) -> shared vtx on local chunk
            std::map<std::pair<index_t, index_t>, std::vector<index_t>> new_adjsets;
            for (const Node& adj_group : pwadjset["groups"].children())
            {
                index_t_accessor neighbor_vals = adj_group["neighbors"].as_index_t_accessor();
                if (neighbor_vals.number_of_elements() != 1)
                {
                    CONDUIT_ERROR("Expected pairwise adjset: 1 neighbor per group, got "
                            << neighbor_vals.number_of_elements());
                }
                index_t nbr_idx = neighbor_vals[0];
                const std::string nbr_key = std::to_string(nbr_idx);

                // 1. Get map of points in our current adjset group to destination chunks.
                const Node& group_adj_map = adjset_maps["to_dom"][nbr_key];

                if (group_adj_map["group_name"].as_string() != adj_group.name())
                {
                    CONDUIT_ERROR("Group name does not match up with adjset map entry.");
                }

                index_t_array src_cnk_offsets = group_adj_map["cnk_offsets"].as_index_t_array();
                index_t_array src_cnk_map = group_adj_map["vtx_map"].as_index_t_array();

                // 2. Get map of points in corresponding adjset group in other domain.
                const std::string this_key = std::to_string(dom_idx);
                const Node& other_group_map
                    = dom_adjset_maps[nbr_idx][adjset_name]["to_dom"][this_key];

                index_t_array dst_cnk_offsets = other_group_map["cnk_offsets"].as_index_t_array();
                index_t_array dst_cnk_map = other_group_map["vtx_map"].as_index_t_array();

                // 3. Get our local shared vertex ids in the adjset group.
                index_t_accessor local_adj_vals = adj_group["values"].as_index_t_accessor();

                if (local_adj_vals.number_of_elements() != src_cnk_offsets.number_of_elements())
                {
                    CONDUIT_ERROR("Local vertex-to-chunk map does not contain the same number "
                                  "of vertices as the local adjset.");
                }

                if (local_adj_vals.number_of_elements() != src_cnk_offsets.number_of_elements())
                {
                    CONDUIT_ERROR("Local vertex-to-chunk map does not contain the same number "
                                  "of vertices as the local adjset.");
                }

                index_t nvals = local_adj_vals.number_of_elements();
                // 4. Iterate through values in the adjset group to create new groups
                // for each chunk.
                for (index_t ipair = 0; ipair < nvals; ipair++)
                {
                    index_t src_cnk_begin = src_cnk_offsets[ipair];
                    index_t dst_cnk_begin = dst_cnk_offsets[ipair];
                    index_t src_cnk_end, dst_cnk_end;
                    if (ipair+1 < nvals)
                    {
                        src_cnk_end = src_cnk_offsets[ipair+1];
                        dst_cnk_end = dst_cnk_offsets[ipair+1];
                    }
                    else
                    {
                        src_cnk_end = src_cnk_map.number_of_elements();
                        dst_cnk_end = dst_cnk_map.number_of_elements();
                    }

                    for (index_t srci = src_cnk_begin; srci < src_cnk_end; srci++)
                    {
                        for (index_t dsti = dst_cnk_begin; dsti < dst_cnk_end; dsti++)
                        {
                            index_t src_cnk = src_cnk_map[srci];
                            index_t dst_cnk = dst_cnk_map[dsti];
                            std::pair<index_t, index_t> src_2_dst {src_cnk, dst_cnk};
                            new_adjsets[src_2_dst].push_back(local_adj_vals[ipair]);
                        }
                    }

                }
            }
            // 5. We now have adjsets for each chunk. Insert them into the adjset
            //    nodes for each corresponding chunk.
            for (const auto& curAdjset : new_adjsets)
            {
                // Get local chunk id
                index_t chunk_id = curAdjset.first.first - chunk_offset;
                index_t chunk_nbr = curAdjset.first.second;
                if (!adjset_data[chunk_id] || !adjset_data[chunk_id]->has_child(adjset_name))
                {
                    continue;
                }
                Node& adjset_groups = adjset_data[chunk_id]->fetch(adjset_name + "/groups");
                Node& new_set = adjset_groups.append();
                new_set["neighbors"].set(chunk_nbr);
                new_set["values"].set(curAdjset.second);
                // If we are not the only chunk in the local adjset, the values
                // are relative to the original domain - flag for remap.
                if (dom_2_chunks.find(&domain)->second.size() > 1)
                {
                    remap_to_chunk_vid[&domain][chunk_id].push_back(&new_set);
                }
            }
        }
    }

    // 6. Map original domain vids to corresponding chunk vids for any chunks
    //    that aren't just the original domain.
    for (auto& domain_coll : remap_to_chunk_vid)
    {
        const ChunkToVertsMap& chunk_2_verts = dom_2_chunks.find(domain_coll.first)->second;
        // Iterate over all chunks in the domain with adjsets needing to be
        // remapped.
        for (auto& chunk_adjs : domain_coll.second)
        {
            index_t local_chunk_id = chunk_adjs.first;
            const std::vector<index_t>& vert_set = chunk_2_verts.find(local_chunk_id)->second;
            std::unordered_map<index_t, index_t> old_to_new;
            // Build a map of domain-relative vids to chunk vids
            for (size_t new_vid = 0; new_vid < vert_set.size(); new_vid++)
            {
                old_to_new[vert_set[new_vid]] = new_vid;
            }
            // Remap adjsets with chunk-specific vids
            for (conduit::Node* aset : chunk_adjs.second)
            {
                index_t_array aset_values = (*aset)["values"].value();
                for (index_t i = 0; i < aset_values.number_of_elements(); i++)
                {
                    aset_values[i] = old_to_new[aset_values[i]];
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
static void
attach_chunk_adjset_to_single_dom(conduit::Node& dom, index_t src_chunk, const conduit::Node* chunk_adjs = nullptr)
{
    Node tmp_node;
    if (chunk_adjs == nullptr)
    {
        if (!dom.has_child("adjsets"))
        {
            return;
        }
        tmp_node = dom.fetch("adjsets");
        dom.remove("adjsets");
        chunk_adjs = &tmp_node;
    }

    // TODO: should we create adjsets in the single-dom case? (ie no pre-existing adjset)
    for (const auto& adjsets : chunk_adjs->children())
    {
        if (!dom["adjsets"].has_child(adjsets.name()))
        {
            // Just take the entire first chunk adjset group, which should have
            // elem/vert association and topology set
            Node& output_adjset = dom["adjsets"][adjsets.name()];
            output_adjset.set(adjsets);
            for (auto& group : output_adjset["groups"].children())
            {
                group["src_chunk"].set(src_chunk);
            }
        }
        else
        {
            Node& output_adjset = dom["adjsets"][adjsets.name()];
            Node& output_adjset_groups = output_adjset["groups"];
            // TODO: check that we have same associativity/topology?
            for (const auto& group : adjsets["groups"].children())
            {
                // Add each group to the output adjset
                Node& new_grp = output_adjset_groups.append();
                new_grp.set(group);
                new_grp["src_chunk"].set(src_chunk);
            }
        }
    }
}

//---------------------------------------------------------------------------
static void
merge_chunked_adjsets(conduit::Node& dom_adjsets, const std::vector<int>& chunks_2_doms)
{
    // We'll use this to lexicographically sort our nodes for a given target domain
    // based on the pair (min(src_cnk, dst_cnk), max(src_cnk, dst_cnk))
    // This ensures that we get a consistent ordering of chunk-based adjset groups
    // on corresponding domains if they're on different ranks.
    using SortedGrpMap = std::map<std::pair<index_t, index_t>, const Node*>;
    index_t domain_id = -1;
    for (auto& adjset : dom_adjsets.children())
    {
        std::unordered_map<index_t, SortedGrpMap> new_adjset_doms;
        for (const auto& grp : adjset["groups"].children())
        {
            index_t src_cnk = grp["src_chunk"].to_index_t();
            index_t dst_cnk = grp["neighbors"].to_index_t();
            index_t src_dom = chunks_2_doms[src_cnk];
            index_t dst_dom = chunks_2_doms[dst_cnk];
            if (dst_dom == src_dom)
            {
                // Don't add self-referencing adjsets
                continue;
            }
            if (domain_id == -1)
            {
                // Save our current domain id
                domain_id = src_dom;
            }
            std::pair<index_t, index_t> adj_ord = { std::min(src_cnk, dst_cnk),
                                                    std::max(src_cnk, dst_cnk) };
            new_adjset_doms[dst_dom][adj_ord] = &grp;
        }
        Node& new_grps = adjset["new_groups"];
        for (const auto& domain : new_adjset_doms)
        {
            // Add a new group, with a defined name.
            std::string group_name = "group";
            {
                index_t min_dom = std::min(domain_id, domain.first);
                index_t max_dom = std::max(domain_id, domain.first);
                group_name += "_";
                group_name += std::to_string(min_dom);
                group_name += "_";
                group_name += std::to_string(max_dom);
            }
            Node& dom_grp = new_grps[group_name];
            // We want to add the neighbor as an array of index_t
            dom_grp["neighbors"].set({domain.first});
            std::vector<index_t> vals;
            std::unordered_set<index_t> unique_vals;
            bool first_set = true;
            // Iterate over the chunk-based adjsets, taking all unique shared vertices.
            // This should be equivalent on both domains since duplicated vertices on
            // this domain should correspond to duplicated vertices on the other domain
            // as well.
            for (const auto& grp_it : domain.second)
            {
                const Node chunk_grp = *grp_it.second;
                index_t_array cnk_vals = chunk_grp["values"].as_index_t_array();
                if (first_set)
                {
                    // We'll add the entire first group we encounter, since there
                    // shouldn't be any duplicates.
                    vals.resize(cnk_vals.number_of_elements());
                    for (int idx = 0; idx < cnk_vals.number_of_elements(); idx++)
                    {
                        vals[idx] = cnk_vals[idx];
                    }
                    unique_vals = std::unordered_set<index_t>(vals.begin(), vals.end());
                    first_set = false;
                }
                else
                {
                    for (int idx = 0; idx < cnk_vals.number_of_elements(); idx++)
                    {
                        // Filter out any vertices that were already present in previous chunks.
                        if (unique_vals.find(cnk_vals[idx]) == unique_vals.end())
                        {
                            vals.push_back(cnk_vals[idx]);
                            unique_vals.insert(cnk_vals[idx]);
                        }
                    }
                }
            }
            dom_grp["values"].set(vals);
        }
        // Drop the old adjset groups.
        adjset.remove_child("groups");
        adjset.rename_child("new_groups", "groups");
    }
}

//---------------------------------------------------------------------------
void
Partitioner::execute(conduit::Node &output)
{
    // By this stage, we will have at least target selections spread across
    // the participating ranks. Now, we need to process the selections to
    // make chunks.
    std::vector<Chunk> chunks;
    std::vector<conduit::Node*> adjset_data;
    std::vector<const conduit::Node*> chunk_assoc_aset;

    // Maps each pre-load balance mesh domain to a set of vertex lists for each chunk.
    // This is used in constructing the intermediate chunk adjsets within a domain.
    DomainToChunkMap domain_to_chunk_map;
    std::map<index_t, const conduit::Node*> domain_id_to_node;

    for(size_t i = 0; i < selections.size(); i++)
    {
        // Get destination rank, domain if the selection has any. If not, it
        // will return -1,-1 so we have some flexibility in how data are moved.
        int dr = selections[i]->get_destination_rank();
        int dd = selections[i]->get_destination_domain();

        index_t sr = selections[i]->get_domain();

        domain_id_to_node[sr] = meshes[i];
        if(selections[i]->get_whole(*meshes[i]))
        {
            // We had a selection that spanned the entire mesh so we'll take
            // the whole mesh rather than extracting. If we are using "mapping"
            // then we will be wrapping the mesh so we can add vertex and element
            // maps to it without changing the input mesh.
            const conduit::Node* assoc_aset = nullptr;
            conduit::Node* wrapped_adjset = nullptr;
            if(mapping || meshes[i]->has_child("adjsets"))
            {
                conduit::Node *c = wrap(i, *meshes[i]);
                chunks.push_back(Chunk(c, true, dr, dd));
                assoc_aset = get_associated_topo_adjset(*meshes[i], selections[i]->get_topology());
                if (assoc_aset)
                {
                    wrapped_adjset = c->fetch_ptr("adjsets");
                }
            }
            else
            {
                chunks.push_back(Chunk(meshes[i], false, dr, dd));
            }
            chunk_assoc_aset.push_back(assoc_aset);
            adjset_data.push_back(wrapped_adjset);
            domain_to_chunk_map[meshes[i]][i] = {};
        }
        else
        {
            std::vector<index_t> vert_ids;
            conduit::Node *c = extract(i, *meshes[i], vert_ids);
            chunks.push_back(Chunk(c, true, dr, dd));
            const conduit::Node* assoc_aset
                = get_associated_topo_adjset(*meshes[i], selections[i]->get_topology());
            chunk_assoc_aset.push_back(assoc_aset);
            if (assoc_aset)
            {
                adjset_data.push_back(c->fetch_ptr("adjsets"));
            }
            else
            {
                adjset_data.push_back(nullptr);
            }
            domain_to_chunk_map[meshes[i]][i] = std::move(vert_ids);
        }
    }

    // Compute the destination rank and destination domain of each input
    // chunk present on this rank.
    std::vector<int> dest_rank, dest_domain, offsets;
    map_chunks(chunks, dest_rank, dest_domain, offsets);

    // It is possible that this topology has no associated adjset. If that is true
    // then the adjset_data will all be nullptr. We skip adjset construction in
    // that case since some of the routines iterate over all adjsets, even when
    // they may not apply. We also skip adjset creation if the user turned it off
    // in the options.
    size_t nullCount = 0;
    for(const auto &value : adjset_data)
        nullCount += (value == nullptr) ? 1 : 0;
    if(build_adjsets && nullCount < adjset_data.size())
    {
        init_chunk_adjsets(chunk_assoc_aset, adjset_data);
        build_interdomain_adjsets(offsets, domain_to_chunk_map, domain_id_to_node, adjset_data);
        build_intradomain_adjsets(offsets, domain_to_chunk_map, adjset_data);
    }

    // Communicate chunks to the right destination ranks
    std::vector<Chunk> chunks_to_assemble;
    std::vector<int> chunks_to_assemble_domains;
    std::vector<int> chunks_to_assemble_gids;
    communicate_chunks(chunks, dest_rank, dest_domain, offsets,
        chunks_to_assemble,
        chunks_to_assemble_domains,
        chunks_to_assemble_gids);

    // Now that we have all the parts we need in chunks_to_assemble, combine
    // the chunks.
    std::set<int> unique_doms;
    for(size_t i = 0; i < chunks_to_assemble_domains.size(); i++)
    {
        unique_doms.insert(chunks_to_assemble_domains[i]);
    }

#ifdef CONDUIT_DEBUG_PARTITIONER
    std::cout << "unique_doms:\n";
    for(auto dom = unique_doms.begin(); dom != unique_doms.end(); dom++)
        std::cout << "  " << (int)*dom << "\n";
    std::cout << std::endl;
#endif

    if(!chunks_to_assemble.empty())
    {
        output.reset();
        for(auto dom = unique_doms.begin(); dom != unique_doms.end(); dom++)
        {
            // Get the chunks for this output domain.
            std::vector<const Node *> this_dom_chunks;
            std::vector<index_t> this_dom_cnkid;
            for(size_t i = 0; i < chunks_to_assemble_domains.size(); i++)
            {
                if(chunks_to_assemble_domains[i] == *dom)
                {
                    this_dom_chunks.push_back(chunks_to_assemble[i].mesh);
                    this_dom_cnkid.push_back(chunks_to_assemble_gids[i]);
                }
            }

            conduit::Node* new_dom;
            if (unique_doms.size() > 1)
            {
                new_dom = &(output.append());
            }
            else
            {
                new_dom = &output;
            }
            if(this_dom_chunks.size() == 1)
            {
                new_dom->set(*this_dom_chunks[0]); // Could we transfer ownership if we own the chunk?
                new_dom->set_path("state/domain_id", (int)*dom);

                attach_chunk_adjset_to_single_dom(*new_dom, this_dom_cnkid[0]);
            }
            else if(this_dom_chunks.size() > 1)
            {
                // Combine the chunks for this domain and add to a list in output.
                combine(*dom, this_dom_chunks, this_dom_cnkid, *new_dom);
            }

            if (new_dom->has_child("adjsets"))
            {
                merge_chunked_adjsets((*new_dom)["adjsets"], dest_domain);
            }
        }
    }

    // Clean up
    for(size_t i = 0; i < chunks.size(); i++)
        chunks[i].free();
    for(size_t i = 0; i < chunks_to_assemble.size(); i++)
        chunks_to_assemble[i].free();

#ifdef CONDUIT_DEBUG_PARTITIONER
    std::cout << "Partition output domains:\n";
    if(blueprint::mesh::is_multi_domain(output))
    {
        for(index_t i = 0; i < output.number_of_children(); i++)
        {
            std::cout << "  " << output[i]["state/domain_id"].to_int() << "\n";
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "  " << output["state/domain_id"].to_int() << "\n" << std::endl;
    }
#endif
}

//-------------------------------------------------------------------------
unsigned int
Partitioner::starting_index(const std::vector<Partitioner::Chunk> &/*chunks*/)
{
    return 0;
}

//-------------------------------------------------------------------------
void
Partitioner::map_chunks(const std::vector<Partitioner::Chunk> &chunks,
    std::vector<int> &dest_ranks,
    std::vector<int> &dest_domain,
    std::vector<int> &offsets)
{
    // All data for this rank begins at offset 0.
    offsets.push_back(0);

    // All data stays on this rank in serial.
    dest_ranks.resize(chunks.size());
    for(size_t i = 0; i < chunks.size(); i++)
        dest_ranks[i] = rank;
// #ifdef CONDUIT_DEBUG_PARTITIONER
//     cout << "map_chunks:" << endl;
//     for(size_t i = 0; i < chunks.size(); i++)
//         chunks[i].mesh->print();
// #endif

    // Determine average chunk size.
    std::vector<index_t> chunk_sizes;
    index_t total_len = 0;
    for(size_t i =0 ; i < chunks.size(); i++)
    {
        const conduit::Node &n_topos = chunks[i].mesh->operator[]("topologies");
        index_t len = 0;
        for(index_t j = 0; j < n_topos.number_of_children(); j++)
            len += conduit::blueprint::mesh::topology::length(n_topos[j]);
        total_len += len;
        chunk_sizes.push_back(len);
    }

#ifdef CONDUIT_DEBUG_PARTITIONER
    cout << "map_chunks: chunks.size=" << chunks.size()
         << ", total_len = " << total_len
         << ", target=" << target << endl;
    cout << "chunk_sizes={";
    for(const index_t s : chunk_sizes) cout << s << ", ";
    cout << "}" << endl;
#endif
    // Come up with a list of domain ids to avoid in our numbering.
    std::set<int> reserved_dd;
    for(size_t i = 0; i < chunks.size(); i++)
    {
        if(chunks[i].destination_domain != Selection::FREE_DOMAIN_ID)
            reserved_dd.insert(chunks[i].destination_domain);
    }
#ifdef CONDUIT_DEBUG_PARTITIONER
    cout << "map_chunks: reserved_dd={";
    for(auto dd : reserved_dd)
        cout << dd << ", ";
    cout << "}" << endl;
#endif
    int start_index = starting_index(chunks);
    // Get a domain id for the first free domain.
    int domid = start_index;
    while(reserved_dd.find(domid) != reserved_dd.end())
        domid++;
    reserved_dd.insert(domid);
#ifdef CONDUIT_DEBUG_PARTITIONER
    cout << "map_chunks: domid=" << domid << endl;
#endif

    // We have a certain number of chunks but determine how many targets
    // that makes. It ought to be equal to target.
    auto targets_from_chunks = static_cast<unsigned int>(count_targets());
#ifdef CONDUIT_DEBUG_PARTITIONER
    std::cout << "targets_from_chunks: " << targets_from_chunks << std::endl;
#endif
    if(targets_from_chunks == target)
    {
        // The number of targets we'd make from the chunks is the same
        // as the target.
        for(size_t i =0 ; i < chunks.size(); i++)
        {
            int dd = chunks[i].destination_domain;
            if(dd == Selection::FREE_DOMAIN_ID)
            {
                dest_domain.push_back(domid);

                // Get the next domain id.
                while(reserved_dd.find(domid) != reserved_dd.end())
                    domid++;
                reserved_dd.insert(domid);
            }
            else
            {
                // We know it goes in this domain.
                dest_domain.push_back(dd);
            }
        }
    }
    else if(targets_from_chunks > target)
    {
        // This may happen when we need to combine domains to a smaller
        // target count.

        // NOTE: For domains that do not already have a destination domain,
        //       we are just grouping adjacent chunks in the overall list
        //       while trying to target a certain number of cells per domain.
        //       We may someday also want to consider the bounding boxes so
        //       we group chunks that are close spatially.
        // NOTE: I updated the logic for grouping domains with a "free"
        //       destination domain. Same idea but now the "len_per_target"
        //       is recomputed based off how many cells are left divided by
        //       how many more domains need to be created.

        index_t remaining_len  = total_len;
        index_t len_per_target = total_len / std::max(target, 1u);
        index_t remaining_target = target - 1;
        index_t running_len = 0;
#ifdef CONDUIT_DEBUG_PARTITIONER
        std::cout << "len_per_target={" << len_per_target << ", ";
#endif
        for(size_t i = 0; i < chunks.size(); i++)
        {
            int dd = chunks[i].destination_domain;
            if(dd == Selection::FREE_DOMAIN_ID)
            {
                const index_t remaining_chunks = (chunks.size() - 1) - i;
                dest_domain.push_back(domid);
                running_len += chunk_sizes[i];
                remaining_len -= chunk_sizes[i];
                if(remaining_chunks != 0 &&
                    (running_len >= len_per_target
                        || remaining_chunks == remaining_target))
                {
                    // Update new len per target
                    running_len = 0;
                    len_per_target = remaining_len / std::max(remaining_target, (index_t)1);
                    remaining_target--;
#ifdef CONDUIT_DEBUG_PARTITIONER
                    std::cout << len_per_target << ", ";
#endif

                    // Get the next domain id.
                    while(reserved_dd.find(domid) != reserved_dd.end())
                        domid++;
                    reserved_dd.insert(domid);
                }
            }
            else
            {
                // We know it goes in this domain.
                dest_domain.push_back(dd);
            }
        }
#ifdef CONDUIT_DEBUG_PARTITIONER
        std::cout << "}" << std::endl;
#endif
    }
    else
    {
        // The number of chunks is less than the target. Something is wrong!
        CONDUIT_ERROR("The number of chunks (" << chunks.size()
                      << ") is smaller than requested (" << target << ").");
    }
#ifdef CONDUIT_DEBUG_PARTITIONER
    cout << "dest_ranks={";
    for(size_t i = 0; i < dest_ranks.size(); i++)
        cout << dest_ranks[i] << ", ";
    cout << "}" << endl;
    cout << "dest_domain={";
    for(size_t i = 0; i < dest_domain.size(); i++)
        cout << dest_domain[i] << ", ";
    cout << "}" << endl;
#endif
}

//-------------------------------------------------------------------------
void
Partitioner::communicate_chunks(const std::vector<Partitioner::Chunk> &chunks,
    const std::vector<int> &/*dest_rank*/,
    const std::vector<int> &dest_domain,
    const std::vector<int> &/*offsets*/,
    std::vector<Partitioner::Chunk> &chunks_to_assemble,
    std::vector<int> &chunks_to_assemble_domains,
    std::vector<int> &chunks_to_assemble_gids)
{
    // In serial, communicating the chunks among ranks means passing them
    // back in the output arguments. We mark them as not-owned so we do not
    // double-free.
    for(size_t i = 0; i < chunks.size(); i++)
    {
        chunks_to_assemble.push_back(Chunk(chunks[i].mesh, false));
        chunks_to_assemble_domains.push_back(dest_domain[i]);
        chunks_to_assemble_gids.push_back(static_cast<int>(i));
    }
}

//-------------------------------------------------------------------------
void
Partitioner::communicate_mapback(std::unordered_map<index_t, Node>& /*packed_fields*/)
{
    // implemented only in parallel case
}

//-------------------------------------------------------------------------
void Partitioner::synchronize_gvids(const std::vector<std::vector<index_t>>& /*remap_to_local_doms*/,
                                    std::map<index_t, std::vector<index_t>>& /*orig_dom_gvids*/)
{
    // implemented only in parallel case
}


//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::coordset --
//-----------------------------------------------------------------------------
namespace coordset
{

/**
 @brief The implmentation of conduit::blueprint::mesh::coordset::merge
*/
class point_merge
{
public:
    void execute(const std::vector<const conduit::Node *> &coordsets,
                 double tolerance,
                 Node &output);

private:
    enum class coord_system
    {
        cartesian,
        cylindrical,
        spherical,
        logical
    };

    int examine_extents(std::vector<std::vector<float64>> &extents) const;

    /**
    @brief Useful when none of the coordsets overlap. Combines all coordinates
        to one array.
    */
    void append_data(const std::vector<Node> &coordsets,
        const std::vector<coord_system> &systems, index_t dimension);

    /**
    @brief Used when coordsets overlap. Combines all coordinates into
        one array; merging points within tolerance.
    */
    void merge_data(const std::vector<Node> &coordsets,
        const std::vector<coord_system> &systems, index_t, double tolerance);

    void create_output(index_t dimension, Node &output) const;

    /**
    @brief Iterates the coordinates in the given coordinate set,
        invoking the given lambda function with the signature
        void (float64 *point, index_t dim)
    NOTE: It will always be valid to index the "point" as if its
        "dim" was 3. For example if the original data was only 2D then
        point[2] will be 0 and dim will be 2.
    */
    template<typename Func>
    static void iterate_coordinates(const Node &coordset, Func &&func);

    /**
    @brief Determines how many points there are and reserves space in member vectors
        new_points and old_to_new_ids
    @return npoints*dimension
    */
    index_t reserve_vectors(const std::vector<Node> &coordsets, index_t dimension);

    /**
    @brief The simple (slow) approach to merging the data based off distance.
    */
    void simple_merge_data(const std::vector<Node> &coordsets,
        const std::vector<coord_system> &systems, index_t dimension,
        double tolerance);

    void spatial_search_merge(const std::vector<Node> &coordsets,
        const std::vector<coord_system> &systems, index_t dimension,
        double tolerance);

    static void xyz_to_rtp(double x, double y, double z, double &out_r, double &out_t, double &out_p);
    // TODO
    static void xyz_to_rz (double x, double y, double z, double &out_r, double &out_z);

    // TODO
    static void rz_to_xyz (double r, double z, double &out_x, double &out_y, double &out_z);
    // TODO
    static void rz_to_rtp (double r, double z, double &out_r, double &out_t, double &out_p);

    static void rtp_to_xyz(double r, double t, double p, double &out_x, double &out_y, double &out_z);
    // TODO
    static void rtp_to_rz (double r, double t, double p, double &out_r, double &out_z);

    static void translate_system(coord_system in_system, coord_system out_system,
        float64 p0, float64 p1, float64 p2, float64 &out_p1, float64 &out_p2, float64 &out_p3);

    /**
    @brief Returns the axis names for the given coordinate system
    */
    static const std::vector<std::string> &get_axes_for_system(coord_system);

    coord_system out_system;

    // Outputs
    std::vector<std::vector<index_t>> old_to_new_ids;
    std::vector<float64> new_coords;
};

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::coordset::utils --
//-----------------------------------------------------------------------------
namespace utils
{

//-----------------------------------------------------------------------------
/**
 @brief A simple bounding box struct to be used by the kdtree
*/
template<typename VectorType>
struct bounding_box
{
    using value_type = typename VectorType::value_type;
    using data_type = typename VectorType::data_type;
    using T = value_type;
    VectorType min;
    VectorType max;

    bool contains(const VectorType &point) const
    {
        return (point >= min && point <= max);
    }

    bool contains(const VectorType &point, double tolerance) const
    {
        return (point >= (min - tolerance) && point <= (max + tolerance));
    }

    void expand(const VectorType &point)
    {
        for(size_t i = 0u; i < min.size(); i++)
        {
            min[i] = std::min(min[i], point[i]);
            max[i] = std::max(max[i], point[i]);
        }
    }
};

//-----------------------------------------------------------------------------
using vec2f = conduit::geometry::vector<float,2>;
using vec3f = conduit::geometry::vector<float,3>;
using vec2  = conduit::geometry::vector<double,2>;
using vec3  = conduit::geometry::vector<double,3>;

/**
 @brief A spatial search structure used to merge points within a given tolerance.

 @note This class differs from conduit::blueprint::mesh::utils::kdtree in that it
       is designed to be constructed on the fly as points are inserted into it
       and it contains the sorted data, which lends itself better to use with
       multiple coordsets. The two classes were introduced independently and in
       the future, it may be better to consolidate them.
*/
template<typename VectorType, typename DataType>
class kdtree
{
private:
    using Float = typename VectorType::value_type;
public:
    constexpr static auto dimension = std::tuple_size<typename VectorType::data_type>::value;
    using vector_type = VectorType;
    using data_type = DataType;
    using IndexType = size_t;

    template<typename Precision, size_t D>
    struct kdnode
    {
        using node = kdnode<Precision, D>;
        std::vector<VectorType> points;
        std::vector<DataType> data;
        bounding_box<VectorType> bb;
        node *left{nullptr};
        node *right{nullptr};
        Float split{0.0};
        unsigned int dim;
        bool has_split{false};
    };
    using node = kdnode<Float, dimension>;
    using pair = std::pair<std::pair<node*,DataType&>, bool>;

    kdtree() = default;
    ~kdtree()
    {
        const auto lambda = [](node *n, unsigned int)
        {
            delete n;
        };
        if(root) { traverse_lrn(lambda, root); }
    }

    /**
    @brief Searches the tree for the given point using
            tolerance as an acceptable distance to merge like-points.
    */
    DataType *find_point(const VectorType &point, Float tolerance)
    {
        DataType *retval = nullptr;
        if(!root)
        {
            retval = nullptr;
        }
        else if(root->bb.contains(point, tolerance))
        {
            retval = find_point(root, 0, point, tolerance);
        }
        return retval;
    }

    void insert(const VectorType &point, const DataType &r)
    {
        scratch.reserve(point_vec_size*2);
        if(!root)
        {
            root = create_node(point, r);
            npoints++;
        }
        else
        {
            insert(root, 0, point, r);
        }
    }

    IndexType size()  const { return npoints; }
    IndexType nodes() const { return nnodes; }
    IndexType depth() const { return tree_depth; }

    void set_bucket_size(IndexType n) { point_vec_size = n; }
    IndexType get_bucket_size() const { return point_vec_size; }

    template<typename Func>
    void iterate_points(Func &&func)
    {
        IndexType point_id = 0u;
        const auto lambda = [&](node *n, unsigned int) {
            for(IndexType i = 0u; i < n->points.size(); i++)
            {
                func(point_id, n->points[i], n->data[i]);
                point_id++;
            }
        };
        if(root) { traverse_lnr(lambda, root); }
    }

    template<typename Func>
    void traverse_nodes(Func &&func)
    {
        if(root) { traverse_lnr(func, root); }
    }

private:
    // Create an empty node
    node *create_node()
    {
        node *newnode = new node;
        newnode->points.reserve(point_vec_size);
        newnode->data.reserve(point_vec_size);
        for(size_t i = 0; i < newnode->bb.min.size(); i++)
            newnode->bb.min[i] = std::numeric_limits<Float>::max();
        for(size_t i = 0; i < newnode->bb.max.size(); i++)
            newnode->bb.max[i] = std::numeric_limits<Float>::lowest();
        newnode->left = nullptr;
        newnode->right = nullptr;
        newnode->split = 0;
        newnode->dim = 0;
        newnode->has_split = false;
        nnodes++;
        return newnode;
    }

    // Create a node with initial values inserted
    node *create_node(VectorType loc, const DataType &r)
    {
        node *newnode = create_node();
        node_add_data(newnode, loc, r);
        return newnode;
    }

    static void node_add_data(node *n, const VectorType &p, const DataType &d)
    {
        n->bb.expand(p);
        n->points.push_back(p);
        n->data.push_back(d);
    }

    /**
    @brief Splits the given node and inserts point/data into the proper child
    */
    void node_split(node *n, const VectorType &point, const DataType &data)
    {
        // Determine which dim to split on
        IndexType dim = 0;
        {
            Float longest_dim = std::numeric_limits<Float>::lowest();
            for(IndexType i = 0; i < n->bb.min.size(); i++)
            {
                const Float dim_len = n->bb.max[i] - n->bb.min[i];
                if(longest_dim < dim_len)
                {
                    dim = i;
                    longest_dim = dim_len;
                }
            }
            n->dim = dim;
        }

        // Determine what value on the dim to split on
        {
            scratch.clear();
            for(IndexType i = 0; i < point_vec_size; i++)
            {
                scratch.push_back(i);
            }
            std::sort(scratch.begin(), scratch.end(), [=](IndexType i0, IndexType i1) {
                return n->points[i0][dim] < n->points[i1][dim];
            });

            // If the index stored in scratch is point_vec_size
            const IndexType scratch_idx = scratch.size() / 2;
            const IndexType median_idx = scratch[scratch_idx];
            Float median = n->points[median_idx][dim];
            // Check if the new point is our actual median
            if(point[dim] > n->points[scratch[scratch_idx-1]][dim] && point[dim] < median)
            {
                median = point[dim];
            }

            n->split = median;
            n->left = create_node();
            n->right = create_node();
            n->has_split = true;

            for(IndexType i = 0; i < point_vec_size; i++)
            {
                const Float temp = n->points[i][dim];
                if(temp < median)
                {
                    node_add_data(n->left, n->points[i], n->data[i]);
                }
                else
                {
                    node_add_data(n->right, n->points[i], n->data[i]);
                }
            }

            if(point[dim] < median)
            {
                node_add_data(n->left, point, data);
            }
            else
            {
                node_add_data(n->right, point, data);
            }

            // Clear the data from the parent node
            std::vector<VectorType>{}.swap(n->points);
            std::vector<DataType>{}.swap(n->data);
        }
    }

    DataType *find_point(node *current, unsigned int depth, const VectorType &point, Float tolerance)
    {
        // If we got here we know that the point was in this node's bounding box
        DataType *retval = nullptr;

        // This node has children
        if(current->has_split)
        {
            const bool left_contains = current->left->bb.contains(point, tolerance);
            const bool right_contains = current->right->bb.contains(point, tolerance);
            if(left_contains && right_contains)
            {
                // Rare, but possible due to tolerance.
                // Check if the left side has the point without tolerance
                const bool pref_left = current->left->bb.contains(point);
                retval = (pref_left)
                    ? find_point(current->left, depth+1, point, tolerance)
                    : find_point(current->right, depth+1, point, tolerance);
                // We tried the preferred side but it didn't contain the point
                if(retval == nullptr)
                {
                    retval = (pref_left)
                        ? find_point(current->right, depth+1, point, tolerance)
                        : find_point(current->left, depth+1, point, tolerance);
                }
            }
            else if(left_contains)
            {
                // Traverse left
                retval = find_point(current->left, depth+1, point, tolerance);
            }
            else if(right_contains)
            {
                // Traverse right
                retval = find_point(current->right, depth+1, point, tolerance);
            }
            else //if(!left_contains && !right_contains)
            {
                // ERROR! This shouldn't happen, the tree must've been built improperly
                retval = nullptr;
            }
        }
        else
        {
            // This is a leaf node.
            const auto t2 = tolerance * tolerance;
            const IndexType N = current->points.size();
            IndexType idx = 0;
            for(idx = 0; idx < N; idx++)
            {
                const auto &p = current->points[idx];
                const auto dist2 = point.distance2(p);
                if(dist2 <= t2)
                {
                    break;
                }
            }

            // Did not find point
            if(idx == N)
            {
                retval = nullptr;
            }
            else
            {
                retval = &current->data[idx];
            }
        }
        return retval;
    }

    void insert(node *current, unsigned int depth, const VectorType &loc, const DataType &r)
    {
        // No matter what we need to add this point to the current bounding box
        current->bb.expand(loc);

        // This node has children
        if(current->has_split)
        {
            const auto dim = current->dim;
            if(loc[dim] < current->split)
            {
                // Go left
                insert(current->left, depth+1, loc, r);
            }
            else // (loc[dim] >= current->split)
            {
                // Go right
                insert(current->right, depth+1, loc, r);
            }
        }
        else
        {
            // This is a leaf node
            // Determine if the node needs to be split
            if((current->points.size()) == point_vec_size)
            {
                // This will add the point and data to the correct child
                node_split(current, loc, r);
                tree_depth = std::max(tree_depth, (IndexType)depth+1);
                npoints++;
            }
            else
            {
                // This node does not need to be split
                node_add_data(current, loc, r);
                npoints++;
            }
        }
    }

    template<typename Func>
    void traverse_lnr(Func &&func, node *n, unsigned int depth = 0)
    {
        if(n->left) { traverse_lnr(func, n->left, depth + 1); }
        func(n, depth);
        if(n->right) { traverse_lnr(func, n->right, depth + 1); }
    }

    template<typename Func>
    void traverse_lrn(Func &&func, node *n, unsigned int depth = 0)
    {
        if(n->left) { traverse_lrn(func, n->left, depth + 1); }
        if(n->right) { traverse_lrn(func, n->right, depth + 1); }
        func(n, depth);
    }

    // Keep track of tree performance
    IndexType npoints{0u};
    IndexType nnodes{0u};
    IndexType tree_depth{0u};

    node *root{nullptr};
    IndexType point_vec_size{32};
    std::vector<IndexType> scratch;
};

//-----------------------------------------------------------------------------
using combine_implicit_data_t = std::pair<const Node*, bounding_box<vec3>>;

template<typename InDataArray, typename OutDataArray>
static index_t
copy_node_data_impl2(const InDataArray &in, OutDataArray &out, index_t offset)
{
    index_t out_idx = offset;
    for(index_t i = 0; i < in.number_of_elements(); i++, out_idx++)
    {
        out[out_idx] = static_cast<typename OutDataArray::ElementType>(in[i]);
    }
    return out_idx;
}

//-----------------------------------------------------------------------------
template<typename OutDataArray>
static index_t
copy_node_data_impl(const Node &in, OutDataArray &out, index_t offset)
{
    const auto idt = in.dtype();
    index_t retval = offset;
    if (idt.is_unsigned_integer())
    {
        retval = copy_node_data_impl2(in.as_uint64_accessor(), out, offset);
    }
    else if (idt.is_signed_integer())
    {
        retval = copy_node_data_impl2(in.as_int64_accessor(), out, offset);
    }
    else if (idt.is_number())
    {
        retval = copy_node_data_impl2(in.as_float64_accessor(), out, offset);
    }
    else
    {
        CONDUIT_ERROR("Tried to iterate " << idt.name() << " as integer data!");
    }
    return retval;
}

//-----------------------------------------------------------------------------
static index_t
copy_node_data(const Node &in, Node &out, index_t offset = 0)
{
    const auto id = out.dtype().id();
    index_t retval = offset;
    switch(id)
    {
    case conduit::DataType::INT8_ID:
    {
        DataArray<int8> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::INT16_ID:
    {
        DataArray<int16> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::INT32_ID:
    {
        DataArray<int32> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::INT64_ID:
    {
        DataArray<int64> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::UINT8_ID:
    {
        DataArray<uint8> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::UINT16_ID:
    {
        DataArray<uint16> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::UINT32_ID:
    {
        DataArray<uint32> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::UINT64_ID:
    {
        DataArray<uint64> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::FLOAT32_ID:
    {
        DataArray<float32> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    case conduit::DataType::FLOAT64_ID:
    {
        DataArray<float64> da = out.value();
        retval = copy_node_data_impl(in, da, offset);
        break;
    }
    default:
        CONDUIT_ERROR("Tried to iterate " << conduit::DataType::id_to_name(id) << " as integer data!");
        break;
    }
    return retval;
}

//-----------------------------------------------------------------------------
static bool node_value_compare_int(const Node& lhs, const Node& rhs)
{
    int64_accessor lhs_data = lhs.as_int64_accessor();
    int64_accessor rhs_data = rhs.as_int64_accessor();
    const index_t nele = lhs_data.number_of_elements();
    if(nele != rhs_data.number_of_elements())
    {
        return false;
    }

    // If one of lhs/rhs is int64 and the other is uint64, we need to check that
    // neither of the pairwise-compared values have a sign bit set.
    auto lhs_typeid = lhs_data.dtype().id();
    auto rhs_typeid = rhs_data.dtype().id();

    bool diff64 = (lhs_typeid == conduit::DataType::INT64_ID
                   && rhs_typeid == conduit::DataType::UINT64_ID);

    diff64 = diff64
            || (lhs_typeid == conduit::DataType::UINT64_ID
                && rhs_typeid == conduit::DataType::INT64_ID);

    bool retval = true;
    for(index_t i = 0; i < nele; i++)
    {
        if (lhs_data[i] != rhs_data[i])
        {
            retval = false;
            break;
        }
        if (diff64 && lhs_data[i] < 0 && rhs_data[i] < 0)
        {
            retval = false;
            break;
        }
    }
    return retval;
}


//-----------------------------------------------------------------------------
static bool
node_value_compare_flt(const Node &lhs, const Node &rhs, double epsilon)
{
    float64_accessor lhs_data = lhs.as_float64_accessor();
    float64_accessor rhs_data = rhs.as_float64_accessor();
    const index_t nele = lhs_data.number_of_elements();
    if(nele != rhs_data.number_of_elements())
    {
        return false;
    }

    bool retval = true;
    for(index_t i = 0; i < nele; i++)
    {
        const double diff = std::abs(lhs_data[i] - rhs_data[i]);
        if(!(diff <= epsilon))
        {
            retval = false;
            break;
        }
    }
    return retval;
}

//-----------------------------------------------------------------------------
static bool
node_value_compare(const Node &lhs, const Node &rhs, double epsilon = CONDUIT_EPSILON)
{
    const auto rid = rhs.dtype();
    const auto lid = lhs.dtype();
    bool retval = true;

    if (rid.is_integer() && lid.is_integer())
    {
        // test for integer types by converting to signed int64
        retval = node_value_compare_int(lhs, rhs);
    }
    else if (rid.is_number() && lid.is_number())
    {
        // default numeric case - convert to double
        retval = node_value_compare_flt(lhs, rhs, epsilon);
    }
    else
    {
        if (!rid.is_number())
        {
            CONDUIT_ERROR("Tried to iterate " << rid.name() << " as integer data!");
        }
        else if (!lid.is_number())
        {
            CONDUIT_ERROR("Tried to iterate " << lid.name() << " as integer data!");
        }
    }
    return retval;
}

//-----------------------------------------------------------------------------
template<typename DataArray_t, typename T>
static index_t
find_rectilinear_offset(const DataArray_t &da, T val, double tolerance = CONDUIT_EPSILON)
{
    // TODO: Binary search? Rectilinear values should be sorted.
    index_t retval = -1;
    for(index_t i = 0; i < da.number_of_elements(); i++)
    {
        const auto diff = val - da[i];
        if(diff <= tolerance)
        {
            retval = i;
            break;
        }
    }
    return retval;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::coordset::utils --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void
point_merge::execute(const std::vector<const Node *> &coordsets,
                     double tolerance,
                     Node &output)
{
    if(coordsets.empty())
        return;

    if(coordsets.size() == 1)
    {
        if(coordsets[0] != nullptr)
        {
            output.reset();
            output["coordsets/coords"] = *coordsets[0];
        }
        return;
    }

    // What do we need to know about the coordsets before we execute the algorithm
    //  - Systems
    //  - Types
    //  - Dimension
    std::vector<Node> working_sets;
    std::vector<coord_system> systems;
    std::vector<std::vector<float64>> extents;
    index_t ncartesian = 0, ncylindrical = 0, nspherical = 0;
    index_t dimension = 0;
    for(size_t i = 0u; i < coordsets.size(); i++)
    {
        const Node *cset = coordsets[i];
        if(!cset)
        {
            // ERROR! You passed me a nullptr for a coordset!
            continue;
        }

        if(!cset->has_child("type"))
        {
            // Error! Invalid coordset!
            continue;
        }
        const std::string type = cset->child("type").as_string();

        // Track some information about each set
        dimension = std::max(dimension, mesh::utils::coordset::dims(*cset));
        extents.push_back(mesh::utils::coordset::extents(*cset));

        // Translate coordsystem string to enum
        std::string system = mesh::utils::coordset::coordsys(*cset);
        if(system == "cylindrical")
        {
            ncylindrical++;
            systems.push_back(coord_system::cylindrical);
        }
        else if(system == "spherical")
        {
            nspherical++;
            systems.push_back(coord_system::spherical);
        }
        else if(system == "logical")
        {
            systems.push_back(coord_system::logical);
        }
        else // system == cartesian
        {
            ncartesian++;
            systems.push_back(coord_system::cartesian);
        }

        // We only work on explicit sets
        working_sets.emplace_back();
        if(type == "uniform")
        {
            mesh::coordset::uniform::to_explicit(*cset, working_sets.back());
        }
        else if(type == "rectilinear")
        {
            mesh::coordset::rectilinear::to_explicit(*cset, working_sets.back());
        }
        else // type == "explicit"
        {
            working_sets.back() = *cset;
        }
    }

    // Determine best output coordinate system
    // Prefer cartesian, if they are all the same use that coordinate system
    if(ncartesian > 0 || (ncylindrical > 0 && nspherical > 0))
    {
        out_system = coord_system::cartesian;
    }
    else if(nspherical > 0)
    {
        out_system = coord_system::spherical;
    }
    else if(ncylindrical > 0)
    {
        out_system = coord_system::cylindrical;
    }
    else
    {
        // Error! Unhandled case!
        std::cerr << "UNHANDLED CASE " << ncartesian << " " << ncylindrical << " " << nspherical << std::endl;
        return;
    }

    int noverlapping_sets = examine_extents(extents);
    PM_DEBUG_PRINT("I was given " << coordsets.size() << " I am combining " << working_sets.size() << std::endl);
    PM_DEBUG_PRINT("noverlapping sets: " << noverlapping_sets << std::endl);
    if(noverlapping_sets == 0)
    {
        // We don't need to do any kind of merging
        append_data(working_sets, systems, dimension);
    }
    else
    {
        merge_data(working_sets, systems, dimension, tolerance);
    }

    create_output(dimension, output);
}

//-----------------------------------------------------------------------------
int
point_merge::examine_extents(std::vector<std::vector<float64>> &extents) const
{
    const auto overlap = [](const float64 box1[6], const float64 box2[6]) -> bool {
        bool retval = true;
        for(auto i = 0u; i < 3u; i++)
        {
            const auto idx  = i * 2;
            const auto min1 = box1[idx];
            const auto max1 = box1[idx+1];
            const auto min2 = box2[idx];
            const auto max2 = box2[idx+1];
            retval &= (max1 >= min2 && max2 >= min1);
        }
        return retval;
    };

    int retval = 0;
    for(auto i = 0u; i < extents.size(); i++)
    {
        float64 box1[6] = {0.,0.,0.,0.,0.,0.};
        const auto &ext1 = extents[i];
        for(auto j = 0u; j < ext1.size(); j++)
        {
            box1[j] = ext1[j];
        }

        for(auto j = 0u; j < extents.size(); j++)
        {
            if(i == j) { continue; }
            const auto ext2 = extents[j];
            float64 box2[6] = {0.,0.,0.,0.,0.,0.};
            for(auto k = 0u; k < ext2.size(); k++)
            {
                box2[k] = ext2[k];
            }

            retval += overlap(box1, box2);
        }
    }
    return retval;
}

//-----------------------------------------------------------------------------
void
point_merge::append_data(const std::vector<Node> &coordsets,
    const std::vector<coord_system> &systems, index_t dimension)
{
    reserve_vectors(coordsets, dimension);

    index_t newid = 0;
    for(size_t i = 0u; i < coordsets.size(); i++)
    {
        const auto append = [&](float64 *p, index_t)
        {
            old_to_new_ids[i].push_back(newid);
            for(auto j = 0; j < dimension; j++)
            {
                new_coords.push_back(p[j]);
            }
            newid++;
        };

        const auto translate_append = [&](float64 *p, index_t d) {
            translate_system(systems[i], out_system,
                p[0], p[1], p[2], p[0], p[1], p[2]);
            append(p, d);
        };


        // Invoke the proper lambda on each coordinate
        if(systems[i] != out_system
            && systems[i] != coord_system::logical)
        {
            iterate_coordinates(coordsets[i], translate_append);
        }
        else
        {
            iterate_coordinates(coordsets[i], append);
        }
    }
}

//-----------------------------------------------------------------------------
void
point_merge::merge_data(const std::vector<Node> &coordsets,
        const std::vector<coord_system> &systems, index_t dimension, double tolerance)
{
#define USE_SPATIAL_SEARCH_MERGE
#if defined(USE_SPATIAL_SEARCH_MERGE)
    spatial_search_merge(coordsets, systems, dimension, tolerance);
#else
    simple_merge_data(coordsets, systems, dimension, tolerance);
#endif
#undef USE_SPATIAL_SEARCH_MERGE
}

//-----------------------------------------------------------------------------
void
point_merge::create_output(index_t dimension, Node &output) const
{
    if(dimension < 0 || dimension > 3)
    {
        // ERROR! Invalid dimension!
        return;
    }

    output.reset();

    // Add the new coordset
    {
        output["type"] = "explicit";
        auto &values = output.add_child("values");

        // Define the node schema for coords
        Schema s;
        const auto npoints = new_coords.size() / dimension;
        const index_t stride = sizeof(float64) * dimension;
        const index_t size = sizeof(float64);
        const auto &axes = get_axes_for_system(out_system);
        for(auto i = 0; i < dimension; i++)
        {
            s[axes[i]].set(DataType::float64(npoints,i*size,stride));
        }

        // Copy out coordinate values
        values.set(s);
        float64_array coord_arrays[3];
        for(auto i = 0; i < dimension; i++)
        {
            coord_arrays[i] = values[axes[i]].value();
        }

        index_t point_id = 0;
        for(auto itr = new_coords.begin(); itr != new_coords.end();)
        {
            for(auto d = 0; d < dimension; d++)
            {
                coord_arrays[d][point_id] = *itr++;
            }
            point_id++;
        }
    }

    // Add the pointmaps
    {
    #ifdef DEBUG_POINT_MERGE
        std::cout << "ID MAP SIZES:" << std::endl;
        for(const auto &idmap : old_to_new_ids)
        {
            std::cout << "  " << idmap.size() << std::endl;
        }
    #endif
        auto &pointmaps = output["pointmaps"];
        for(const auto &idmap : old_to_new_ids)
        {
            const auto size = idmap.size();
            // Create the list entry
            auto &ids = pointmaps.append();
            ids.set(DataType::index_t(size));
            // Copy the contents into the node
            DataArray<index_t> ids_data = ids.value();
            for(size_t i = 0u; i < size; i++)
            {
                ids_data[i] = idmap[i];
            }
        }
    }
}

//-----------------------------------------------------------------------------
template<typename Func>
void
point_merge::iterate_coordinates(const Node &coordset, Func &&func)
{
    if(!coordset.has_child("type"))
    {
        CONDUIT_ERROR("Coordset does not have a type");
        return;
    }

    if(coordset["type"].as_string() != "explicit")
    {
        CONDUIT_ERROR("Coordset is not explicit");
        return;
    }

    if(!coordset.has_child("values"))
    {
        CONDUIT_ERROR("Coordset does not have values");
        return;
    }

    const Node &coords = coordset["values"];

    // Fetch the nodes for the coordinate values
    const Node *xnode = coords.fetch_ptr("x");
    const Node *ynode = nullptr, *znode = nullptr;
    if(xnode)
    {
        // Cartesian
        ynode = coords.fetch_ptr("y");
        znode = coords.fetch_ptr("z");
    }
    // Cylindrical
    else if((xnode = coords.fetch_ptr("z")))
    {
        ynode = coords.fetch_ptr("r");
    }
    // Spherical
    else if((xnode = coords.fetch_ptr("r")))
    {
        ynode = coords.fetch_ptr("theta");
        znode = coords.fetch_ptr("phi");
    }
        // Logical
    else if(((xnode = coords.fetch_ptr("i"))))
    {
        ynode = coords.fetch_ptr("j");
        znode = coords.fetch_ptr("k");
    }

    // Iterate accordingly
    float64 p[3] {0., 0., 0.};
    if(xnode && ynode && znode)
    {
        // 3D
        auto xarray = xnode->as_float64_accessor();
        auto yarray = ynode->as_float64_accessor();
        auto zarray = znode->as_float64_accessor();
        const index_t N = xarray.number_of_elements();
        for(index_t i = 0; i < N; i++)
        {
            p[0] = xarray[i]; p[1] = yarray[i]; p[2] = zarray[i];
            func(p, 3);
        }
    }
    else if(xnode && ynode)
    {
        // 2D
        auto xarray = xnode->as_float64_accessor();
        auto yarray = ynode->as_float64_accessor();
        const index_t N = xarray.number_of_elements();
        for(index_t i = 0; i < N; i++)
        {
            p[0] = xarray[i]; p[1] = yarray[i]; p[2] = 0.;
            func(p, 2);
        }
    }
    else if(xnode)
    {
        // 1D
        auto xarray = xnode->as_float64_accessor();
        const index_t N = xarray.number_of_elements();
        for(index_t i = 0; i < N; i++)
        {
            p[0] = xarray[i]; p[1] = 0.; p[2] = 0.;
            func(p, 1);
        }
    }
    else
    {
        CONDUIT_ERROR("No valid node values found.");
    }
}

//-----------------------------------------------------------------------------
index_t
point_merge::reserve_vectors(const std::vector<Node> &coordsets, index_t dimension)
{
    old_to_new_ids.reserve(coordsets.size());
    index_t new_size = 0;
    for(size_t i = 0u; i < coordsets.size(); i++)
    {
        const Node *values = coordsets[i].fetch_ptr("values");
        index_t npts = 0;
        if(values)
        {
            const Node *xnode = values->fetch_ptr("x");
            if(!xnode)
            {
                xnode = values->fetch_ptr("r");
            }
            if(!xnode)
            {
                xnode = values->fetch_ptr("i");
            }

            if(xnode)
            {
                npts = xnode->dtype().number_of_elements();
            }
        #ifdef DEBUG_POINT_MERGE
            std::cout << "coordset " << i << " ";
            std::cout << npts << std::endl;
            coordsets[i].print();
        #endif
        }

        old_to_new_ids.push_back({});
        old_to_new_ids.back().reserve(npts);
        new_size += npts*dimension;
    }
    new_coords.reserve(new_size);
    return new_size;
}

//-----------------------------------------------------------------------------
void
point_merge::simple_merge_data(const std::vector<Node> &coordsets,
        const std::vector<coord_system> &systems, index_t dimension, double tolerance)
{
    PM_DEBUG_PRINT("Simple merging!" << std::endl);
    reserve_vectors(coordsets, dimension);

    const auto t2 = (tolerance*tolerance);
    for(size_t i = 0u; i < coordsets.size(); i++)
    {
        const index_t end_check = (index_t)new_coords.size();
        const Node &coordset = coordsets[i];
        auto &idmap = old_to_new_ids[i];

        // To be invoked on each coordinate
        const auto merge = [&](float64 *p, index_t) {
            for(index_t idx = 0; idx < end_check; idx += dimension)
            {
                float64 dist2 = 0.;
                for(index_t d = 0; d < dimension; d++)
                {
                    const auto diff = p[d] - new_coords[idx+d];
                    dist2 += (diff*diff);
                }

                // Within tolerance!
                if(dist2 < t2)
                {
                    // idx / dim should truncate to the proper id
                    idmap.push_back(idx / dimension);
                    return;
                }
            }

            // Did not find another point within tolerance
            const auto newid = (index_t)(new_coords.size() / dimension);
            idmap.push_back(newid);
            for(index_t d = 0; d < dimension; d++)
            {
                new_coords.push_back(p[d]);
            }
        };

        const auto translate_merge = [&](float64 *p, index_t d) {
            translate_system(systems[i], coord_system::cartesian,
                p[0], p[1], p[2], p[0], p[1], p[2]);
            merge(p, d);
        };

        // Invoke the proper lambda on each coordinate
        if(systems[i] != coord_system::cartesian
            && systems[i] != coord_system::logical)
        {
            iterate_coordinates(coordset, translate_merge);
        }
        else
        {
            iterate_coordinates(coordset, merge);
        }
    }
}

//-----------------------------------------------------------------------------
void
point_merge::spatial_search_merge(const std::vector<Node> &coordsets,
        const std::vector<coord_system> &systems, index_t dimension,
        double tolerance)
{
    PM_DEBUG_PRINT("Spatial search merging!" << std::endl);
    reserve_vectors(coordsets, dimension);

    using namespace utils;
    kdtree<vec3, index_t> point_records;
    point_records.set_bucket_size(32);
    for(size_t i = 0u; i < coordsets.size(); i++)
    {
        const auto &coordset = coordsets[i];

        // To be invoked on every coordinate
        const auto merge = [&](const float64 *p, index_t) {
            vec3 key;
            key.v[0] = p[0]; key.v[1] = p[1]; key.v[2] = p[2];
            const auto potential_id = new_coords.size() / dimension;
            const index_t *existing_id = point_records.find_point(key, tolerance);
            // Point wasn't already in the tree, insert it
            if(!existing_id)
            {
                old_to_new_ids[i].push_back(potential_id);
                for(index_t d = 0; d < dimension; d++)
                {
                    new_coords.push_back(p[d]);
                }
                // Store the point in the tree so we can look it up later
                point_records.insert(key, potential_id);
            }
            else
            {
                // Point already existed in the tree, reference the known id
                old_to_new_ids[i].push_back(*existing_id);
            }
        };

        const auto translate_merge = [&](const float64 *p, index_t d) {
            float64 tp[3];
            translate_system(systems[i], coord_system::cartesian,
                p[0], p[1], p[2], tp[0], tp[1], tp[2]);
            merge(tp, d);
        };

        // Invoke the proper lambda on each coordinate
        if(systems[i] != coord_system::cartesian
            && systems[i] != coord_system::logical)
        {
            iterate_coordinates(coordset, translate_merge);
        }
        else
        {
            iterate_coordinates(coordset, merge);
        }
    }

    PM_DEBUG_PRINT("Number of points in tree " << point_records.size()
        << ", depth of tree " << point_records.depth()
        << ", nodes in tree " << point_records.nodes() << std::endl);
}

//-----------------------------------------------------------------------------
void
point_merge::xyz_to_rtp(double x, double y, double z, double &out_r, double &out_t, double &out_p)
{
    const auto r = std::sqrt(x*x + y*y + z*z);
    out_r = r;
    out_t = std::acos(r / z);
    out_p = std::atan(y / x);
}

//-----------------------------------------------------------------------------
void
point_merge::rtp_to_xyz(double r, double t, double p, double &out_x, double &out_y, double &out_z)
{
    out_x = r * std::cos(p) * std::sin(t);
    out_y = r * std::sin(p) * std::sin(t);
    out_z = r * std::cos(t);
}

//-----------------------------------------------------------------------------
void
point_merge::translate_system(coord_system in_system, coord_system out_system,
        float64 p0, float64 p1, float64 p2,
        float64 &out_p0, float64 &out_p1, float64 &out_p2)
{
    switch(out_system)
    {
    case coord_system::logical: // fallthrough, out_system is never logical.
    case coord_system::cartesian:
        switch(in_system)
        {
        case coord_system::spherical:
            rtp_to_xyz(p0,p1,p2, out_p0,out_p1,out_p2);
            break;
        // cartesian == cylindrical in this context
        default:
            out_p0 = p0;
            out_p1 = p1;
            out_p2 = p2;
            break;
        }
        break;
    case coord_system::cylindrical:
        switch(in_system)
        {
        case coord_system::spherical:
            rtp_to_xyz(p0,p1,p2, out_p0,out_p1,out_p2);
            break;
        default:
            out_p0 = p0;
            out_p1 = p1;
            out_p2 = p2;
            break;
        }
        break;
    case coord_system::spherical:
        switch(in_system)
        {
        case coord_system::cylindrical: // Fallthrough, same as cartesian in this context
        case coord_system::cartesian:
            xyz_to_rtp(p0,p1,p2, out_p0,out_p1,out_p2);
            break;
        default:
            out_p0 = p0;
            out_p1 = p1;
            out_p2 = p2;
            break;
        }
        break;
    default:
        out_p0 = p0;
        out_p1 = p1;
        out_p2 = p2;
        break;
    }
}

//-----------------------------------------------------------------------------
const std::vector<std::string> &
point_merge::get_axes_for_system(coord_system cs)
{
    const std::vector<std::string> *retval = &mesh::utils::CARTESIAN_AXES;
    switch(cs)
    {
    case coord_system::cylindrical:
        retval = &mesh::utils::CYLINDRICAL_AXES;
        break;
    case coord_system::spherical:
        retval = &mesh::utils::SPHERICAL_AXES;
        break;
    case coord_system::logical:
        retval = &mesh::utils::LOGICAL_AXES;
        break;
    case coord_system::cartesian: // Do nothing
        break;
    default:
        break;
    }
    return *retval;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::coordset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::topology --
//-----------------------------------------------------------------------------
namespace topology
{

//-----------------------------------------------------------------------------
static void
build_unstructured_output(const std::vector<const Node*> &topologies,
                          const Node &pointmaps,
                          const std::string &cset_name,
                          Node &output)
{
    // std::cout << "Building unstructured output!" << std::endl;
    using namespace utils::topology;
    output.reset();
    output["type"].set("unstructured");
    output["coordset"].set(cset_name);
    std::set<index_t> shape_types;
    std::vector<index_t> out_conn, out_shapes, out_sizes, out_offsets;
    std::vector<index_t> se_conn, se_shapes, se_sizes, se_offsets;
    std::vector<index_t> out_elem_map;
    const index_t ntopos = (index_t)topologies.size();
    index_t offset = 0;
    std::map<std::pair<index_t, index_t>, index_t> domFace2Face;
    const conduit::blueprint::mesh::utils::ShapeType triShape("tri");
    const conduit::blueprint::mesh::utils::ShapeType quadShape("quad");
    const conduit::blueprint::mesh::utils::ShapeType polygonShape("polygonal");
    index_t se_shape_bits = 0;

    for(index_t i = 0; i < ntopos; i++)
    {
        const Node *topo = topologies[i];
        const Node *elements = topo->fetch_ptr("elements");
        if(!elements)
        {
            // ERROR
            continue;
        }

        const Node *pointmap = pointmaps.child_ptr(i);
        if(!pointmap)
        {
            // ERROR
            continue;
        }
        DataArray<index_t> pmap_da = pointmap->value();

        iterate_elements(*topo, [&](const entity &e)
        {
            shape_types.insert(e.shape.id);
            out_shapes.push_back(e.shape.id);
            out_sizes.push_back(e.element_ids.size());
            out_offsets.push_back(offset);

            // Build element_map.
            out_elem_map.push_back(i);
            out_elem_map.push_back(e.entity_id);

            if(e.subelement_ids.empty())
            {
                // Non-PH elements.
                for(const index_t id : e.element_ids)
                {
                    out_conn.push_back(pmap_da[id]);
                }
            }
            else
            {
                // PH elements. e.element_ids stores face ids for domain i
                for(size_t fi = 0; fi < e.element_ids.size(); fi++)
                {
                    // Look for face.
                    const std::pair<index_t, index_t> domFace(i, e.element_ids[fi]);
                    auto it = domFace2Face.find(domFace);
                    index_t newFaceId = -1;

                    if(it == domFace2Face.end())
                    {
                        // Define new face.
                        newFaceId = domFace2Face.size();
                        domFace2Face[domFace] = newFaceId;
                        const auto &se_ids = e.subelement_ids[fi];

                        se_offsets.push_back(se_conn.size());
                        const index_t numSides = static_cast<index_t>(se_ids.size());
                        se_sizes.push_back(numSides);
                        const index_t faceShape = (numSides == 3) ? triShape.id :
                            ((numSides == 4) ? quadShape.id : polygonShape.id);
                        se_shapes.push_back(faceShape);
                        se_shape_bits |= (1 << faceShape);

                        for(auto id : se_ids)
                        {
                            se_conn.push_back(pmap_da[id]);
                        }
                    }
                    else
                    {
                        // Reuse face.
                        newFaceId = it->second;
                    }

                    out_conn.push_back(newFaceId);
                }
            }

            offset += e.element_ids.size();
        });
    }

    if(shape_types.size() == 1)
    {
        conduit::blueprint::mesh::utils::ShapeType shape(*shape_types.begin());
        output["elements/shape"] = shape.type();
        output["elements/connectivity"].set(out_conn);
        if(shape.is_polygonal())
        {
            output["elements/sizes"].set(out_sizes);
            output["elements/offsets"].set(out_offsets);
        }
        else if(shape.is_polyhedral() && !se_conn.empty())
        {
            output["elements/sizes"].set(out_sizes);
            output["elements/offsets"].set(out_offsets);

            output["subelements/shape"] = "polygonal";
            output["subelements/connectivity"].set(se_conn);
            output["subelements/sizes"].set(se_sizes);
            output["subelements/offsets"].set(se_offsets);
            output["subelements/shapes"].set(se_shapes);
        }
    }
    else if(shape_types.size() > 1)
    {
        // Build mixed output.
        output["elements/shape"] = "mixed";
        for(auto s : shape_types)
        {
            conduit::blueprint::mesh::utils::ShapeType shape(s);
            // Cast b/c VisIt requires int32 at the moment.
            output["elements/shape_map"][shape.type()] = static_cast<int>(s);
        }
        output["elements/connectivity"].set(out_conn);
        output["elements/sizes"].set(out_sizes);
        output["elements/offsets"].set(out_offsets);
        output["elements/shapes"].set(out_shapes);

        if(!se_conn.empty())
        {
            output["subelements/shape"] = "mixed";
            output["subelements/connectivity"].set(se_conn);
            output["subelements/sizes"].set(se_sizes);
            output["subelements/offsets"].set(se_offsets);
            output["subelements/shapes"].set(se_shapes);
            // Make shape map.
            // Cast b/c VisIt requires int32 at the moment.
            if(se_shape_bits & (1 << triShape.id))
                output["subelements/shape_map/tri"] = static_cast<int>(triShape.id);
            if(se_shape_bits & (1 << quadShape.id))
                output["subelements/shape_map/quad"] = static_cast<int>(quadShape.id);
            if(se_shape_bits & (1 << polygonShape.id))
                output["subelements/shape_map/polygonal"] = static_cast<int>(polygonShape.id);
        }
    }
    output["element_map"].set(out_elem_map);
}

//-----------------------------------------------------------------------------
static void
build_polygonal_output(const std::vector<const Node*> &topologies,
                       const Node &pointmaps,
                       const std::string &cset_name,
                       Node &output)
{
    // std::cout << "Building polygonal output!" << std::endl;
    using namespace utils::topology;
    output["type"].set("unstructured");
    output["coordset"].set(cset_name);
    output["elements/shape"].set("polygonal");

    std::vector<index_t> out_offsets;
    std::vector<index_t> out_conn;
    std::vector<index_t> out_sizes;
    std::vector<index_t> out_elem_map;
    const index_t ntopos = (index_t)topologies.size();
    for(index_t i = 0; i < ntopos; i++)
    {
        const Node &topo = *topologies[i];
        const Node *pointmap = pointmaps.child_ptr(i);
        if(!pointmap)
        {
            CONDUIT_WARN("Could not merge topology " << i <<  ", no associated pointmap.");
            continue;
        }
        DataArray<index_t> pmap_da = pointmap->value();

        iterate_elements(topo, [&out_offsets, &out_conn, &out_sizes, &pmap_da, &out_elem_map, i](const entity &e) {
            // If it is a polygon or 2D/1D the remapping is trivial
            if(e.shape.is_polygonal() || e.shape.dim == 2 || e.shape.dim == 1)
            {
                out_elem_map.push_back(i);
                out_elem_map.push_back(e.entity_id);
                const index_t sz = (index_t)e.element_ids.size();
                out_offsets.push_back(out_conn.size());
                out_sizes.push_back(sz);
                for(index_t j = 0; j < sz; j++)
                {
                    out_conn.push_back(pmap_da[e.element_ids[j]]);
                }
            }
            else if(e.shape.is_polyhedral())
            {
                // Q: Will this even be a case? I'd imagine if polyhedra are present then
                //  everything would need to be promoted to polyhedra
            }
            else if(e.shape.id == (index_t)ShapeId::Tet || e.shape.id == (index_t)ShapeId::Hex)
            {
                // Q: Will this even be a case? I'd imagine if 3D elements exist then the output
                //  would have to be a 3D topology
                const index_t embeded_sz = mesh::utils::TOPO_SHAPE_INDEX_COUNTS[e.shape.embed_id];
                index_t ei = 0;
                for(index_t j = 0; j < e.shape.embed_count; j++)
                {
                    out_elem_map.push_back(i);
                    out_elem_map.push_back(e.entity_id);
                    out_offsets.push_back(out_conn.size());
                    out_sizes.push_back(embeded_sz);
                    for(index_t k = 0; k < embeded_sz; k++)
                    {
                        // Use the embedding table to find the correct index into the element_ids
                        //  array. Then map the element_id through the pointmap to get the new id.
                        out_conn.push_back(pmap_da[e.element_ids[e.shape.embedding[ei]]]);
                        ei++;
                    }
                }
            }
            else // (e.shape.dim == 0 || !e.shape.is_valid()) Q: Any other cases caught in here?
            {
                CONDUIT_ERROR("Encountered invalid element! At element " << e.entity_id);
                return;
            }
        });
    }
    output["element_map"].set(out_elem_map);
    output["elements/sizes"].set(out_sizes);
    output["elements/offsets"].set(out_offsets);
    output["elements/connectivity"].set(out_conn);
}

//-----------------------------------------------------------------------------
static void
build_polyhedral_output(const std::vector<const Node*> &topologies,
                       const Node &pointmaps,
                       const std::string &cset_name,
                       Node &output)
{
    // std::cout << "Building polyhedral output!" << std::endl;
    using namespace utils::topology;
    output.reset();
    output["type"].set("unstructured");
    output["coordset"].set(cset_name);
    output["elements/shape"].set("polyhedral");
    output["subelements/shape"].set("polygonal");
    std::vector<index_t> out_conn;
    std::vector<index_t> out_sizes;
    std::vector<index_t> out_offsets;
    std::vector<index_t> out_subconn;
    std::vector<index_t> out_subsizes;
    std::vector<index_t> out_suboffsets;
    std::vector<index_t> out_elem_map;

    // Variables to help with face uniqueness.
    std::vector<index_t> tmp_subconn, new_subconn;
    std::map<conduit::uint64, index_t> faceHashToId;

    // Adds a face to faceHashToId and the subelement information if the face has
    // not been seen before. If the face has been seen before then the previous
    // face id is returned.
    auto addFace = [&](std::vector<index_t> &tmp_conn, const std::vector<index_t> &conn)
    {
        index_t nextFaceId = 0;

        // Make a hash that identifies the face.
        std::sort(tmp_conn.begin(), tmp_conn.end());
        auto faceHash = conduit::utils::hash(&tmp_conn[0], tmp_conn.size());

        // Insert the face if it does not exist.
        auto it = faceHashToId.find(faceHash);
        if(it == faceHashToId.end())
        {
            // Put the faceHash,nextFaceId in the map.
            nextFaceId = static_cast<index_t>(out_subsizes.size());
            faceHashToId[faceHash] = nextFaceId;

            // We need to insert the new face into the subconn info.
            out_suboffsets.push_back(static_cast<index_t>(out_subconn.size()));
            out_subsizes.push_back(static_cast<index_t>(conn.size()));
            for(const auto &ptid : conn)
                out_subconn.push_back(ptid);
        }
        else
        {
            nextFaceId = it->second;
        }

        return nextFaceId;
    };

    // Handle all of the topologies.
    const index_t ntopos = (index_t)topologies.size();
    for(index_t i = 0; i < ntopos; i++)
    {
        const Node &topo = *topologies[i];
        const Node *pointmap = pointmaps.child_ptr(i);
        if(!pointmap)
        {
            CONDUIT_WARN("Could not merge topology " << i <<  ", no associated pointmap.");
            continue;
        }
        const DataArray<index_t> pmap_da = pointmap->value();

        iterate_elements(topo, [&](const entity &e)
        {
            out_elem_map.push_back(i);
            out_elem_map.push_back(e.entity_id);
            if(e.shape.is_polyhedral())
            {
                // Copy the subelements to their new offsets
                const index_t sz = static_cast<index_t>(e.element_ids.size());
                out_offsets.push_back(static_cast<index_t>(out_conn.size()));
                out_sizes.push_back(sz);
                for(index_t ii = 0; ii < sz; ii++)
                {
                    const index_t subsz = static_cast<index_t>(e.subelement_ids[ii].size());

                    // Gather up the face point ids in the new coordset.
                    tmp_subconn.clear();
                    new_subconn.clear();
                    tmp_subconn.reserve(subsz);
                    new_subconn.reserve(subsz);
                    for(index_t j = 0; j < subsz; j++)
                    {
                        const auto new_ptid = pmap_da[e.subelement_ids[ii][j]];
                        tmp_subconn.push_back(new_ptid);
                        new_subconn.push_back(new_ptid);
                    }

                    // Get the id of this face (adding it if needed)
                    index_t nextFaceId = addFace(tmp_subconn, new_subconn);

                    // Store the face id in the connectivity.
                    out_conn.push_back(nextFaceId);
                }
            }
            else if(utils::TOPO_SHAPE_IDS[e.shape.id] == "f" || utils::TOPO_SHAPE_IDS[e.shape.id] == "l")
            {
                // 2D faces / 1D lines: Line, Tri, Quad
                // Add the shape to the sub elements
                const index_t subsz = static_cast<index_t>(e.element_ids.size());

                // Gather up the face point ids in the new coordset.
                tmp_subconn.clear();
                new_subconn.clear();
                tmp_subconn.reserve(subsz);
                new_subconn.reserve(subsz);
                for(index_t j = 0; j < subsz; j++)
                {
                    const auto new_ptid = pmap_da[e.element_ids[j]];
                    tmp_subconn.push_back(new_ptid);
                    new_subconn.push_back(new_ptid);
                }

                // Get the id of this face (adding it if needed)
                index_t nextFaceId = addFace(tmp_subconn, new_subconn);

                // Then create a polyhedron with only 1 subelement
                out_offsets.push_back(static_cast<index_t>(out_conn.size()));
                out_sizes.push_back(1);
                out_conn.push_back(nextFaceId);
            }
            else if(utils::TOPO_SHAPE_IDS[e.shape.id] == "c")
            {
                // 3D cells. We use ShapeType's face functions since we want to ensure
                // we do not end up splitting faces into triangles. That could make them
                // not match in this context.

                const index_t nfaces = e.shape.num_faces();
                out_offsets.push_back(static_cast<index_t>(out_conn.size()));
                out_sizes.push_back(nfaces);
                for(index_t j = 0; j < nfaces; j++)
                {
                    // Get the points for the face
                    index_t nids = 0;
                    const index_t *ids = e.shape.get_face(j, nids);

                    // Gather up the face point ids in the new coordset.
                    tmp_subconn.clear();
                    new_subconn.clear();
                    tmp_subconn.reserve(nids);
                    new_subconn.reserve(nids);
                    for(index_t k = 0; k < nids; k++)
                    {
                        const auto new_ptid = pmap_da[e.element_ids[ids[k]]];
                        tmp_subconn.push_back(new_ptid);
                        new_subconn.push_back(new_ptid);
                    }

                    // Get the id of this face (adding it if needed)
                    index_t nextFaceId = addFace(tmp_subconn, new_subconn);

                    // Store the face id in the connectivity.
                    out_conn.push_back(nextFaceId);
                }
            }
            else // (utils::TOPO_SHAPE_IDS[e.shape.id] == "p" || !e.shape.is_valid()) Q: Any other cases caught in here?
            {
                CONDUIT_ERROR("Encountered invalid element! At element " << e.entity_id);
                return;
            }
        });
    }
    output["element_map"].set(out_elem_map);
    output["elements/sizes"].set(out_sizes);
    output["elements/offsets"].set(out_offsets);
    output["elements/connectivity"].set(out_conn);
    output["subelements/sizes"].set(out_subsizes);
    output["subelements/offsets"].set(out_suboffsets);
    output["subelements/connectivity"].set(out_subconn);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::topology --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils --
//-----------------------------------------------------------------------------
namespace utils
{

class combine_implicit_topologies
{
public:
    combine_implicit_topologies() = default;
    ~combine_implicit_topologies() = default;

    //-------------------------------------------------------------------------
    bool execute(const std::string &topo_name,
        const std::string &rt,
        const std::vector<const Node *> &n_inputs,
        const std::string &original_element_ids,
        const std::string &original_vertex_ids,
        Node &output,
        double merge_tolerance = CONDUIT_EPSILON)
    {
        bool retval = false;

        // Establish the paths to the topo/cset we need from each input
        topo_path = "topologies/"+topo_name;
        cset_path = "coordsets/"+(*n_inputs[0])[topo_path]["coordset"].as_string();

        // Determine which labels to use for axes
        const std::vector<const Node*> first_two_csets{n_inputs[0]->fetch_ptr(cset_path), n_inputs[1]->fetch_ptr(cset_path)};
        axes = figure_out_implicit_axes(first_two_csets);

        // Determine the logical dims & dimension
        dimension = 0;
        for(const Node *in : n_inputs)
        {
            logical_dims.emplace_back();
            const Node &n_topo = in->fetch_existing(topo_path);
            utils::topology::logical_dims(n_topo, logical_dims.back().data(), MAXDIM);
            dimension = std::max(dimension, utils::topology::dims(n_topo));
        }

        // Need to check that uniform coordsets have compatible spacing before continuing
        std::string type = rt;
        if(type == "uniform")
        {
            // Inspect input[0] for baseline spacing/dimension
            const auto baseline_spacing = mesh::utils::coordset::uniform::spacing((*n_inputs[0])[cset_path]);

            for(const Node *n_in : n_inputs)
            {
                const auto this_spacing = mesh::utils::coordset::uniform::spacing((*n_in)[cset_path]);
                bool should_continue = true;
                for(size_t di = 0; di < baseline_spacing.size(); di++)
                {
                    if(this_spacing[di] != baseline_spacing[di])
                    {
                        type = "rectilinear";
                        should_continue = false;
                        break;
                    }
                }
                if(!should_continue)
                {
                    break;
                }
            }

            // We can now pre-populate the output with the correct spacing
            if(type == "uniform")
            {
                bool needs_spacing = false;
                Node &out_cset = output[cset_path];
                for(const auto s : baseline_spacing)
                {
                    if(s != 1.) needs_spacing = true;
                }
                if(needs_spacing)
                {
                    Schema s;
                    for(index_t d = 0; d < dimension; d++)
                    {
                        s["d"+axes[d]].set(DataType::c_double(1, d*sizeof(double), dimension*sizeof(double)));
                    }
                    out_cset["spacing"].set(s);
                    for(index_t d = 0; d < dimension; d++)
                    {
                        out_cset["spacing/d"+axes[d]].set(baseline_spacing[d]);
                    }
                }
            }
        }

        // We must convert all inputs to the recommended topology
        //  (if they aren't already)
        index_t id = 0;
        std::vector<Node> temp_meshes;
        temp_meshes.reserve(n_inputs.size());
        for(const Node *n_in : n_inputs)
        {
            const Node &n_topo = (*n_in)[topo_path];
            const Node &n_cset = (*n_in)[cset_path];
            const std::string topo_type = n_topo["type"].as_string();
            const std::string cset_type = n_cset["type"].as_string();
            temp_meshes.emplace_back();
            Node &n_mesh = temp_meshes.back();
            n_mesh["id"] = id;
            id++;

            if(type == "structured")
            {
                // Check the coordset
                if(cset_type != "explicit")
                {
                    if(cset_type == "uniform")
                    {
                        mesh::coordset::uniform::to_explicit(n_cset, n_mesh[cset_path]);
                    }
                    else // (cset_type == "rectilinear")
                    {
                        mesh::coordset::rectilinear::to_explicit(n_cset, n_mesh[cset_path]);
                    }
                }
                else
                {
                    n_mesh[cset_path].set_external((*n_in)[cset_path]);
                }

                // Check topology
                if(topo_type != "structured")
                {
                    Node &n_new_topo = n_mesh[topo_path];
                    n_new_topo["type"] = "structured";
                    n_new_topo["coordset"] = n_cset.name();

                    const index_t dim = utils::topology::dims(n_topo);
                    std::array<index_t, MAXDIM> logical_dims_vals;
                    utils::topology::logical_dims(n_topo, logical_dims_vals.data(), dim);
                    for(index_t ldi = 0; ldi < dim; ldi++)
                    {
                        n_new_topo["elements/dims/"+utils::LOGICAL_AXES[ldi]] = logical_dims_vals[ldi];
                    }

                    if(n_topo.has_path("elements/origin"))
                    {
                        n_new_topo["elements/origin"] = n_topo["elements/origin"];
                    }
                }
                else
                {
                    n_mesh[topo_path].set_external((*n_in)[topo_path]);
                }

            #ifdef DEBUG_STRUCTURED_COMBINE
                if(n_in->has_path("fields/dist"))
                {
                    n_mesh["fields/dist"].set_external((*n_in)["fields/dist"]);
                }
            #endif
            }
            else if(type == "rectilinear")
            {
                // Check the coordset
                if(cset_type != "rectilinear")
                {
                    mesh::coordset::uniform::to_rectilinear(n_cset, n_mesh[cset_path]);
                }
                else
                {
                    n_mesh[cset_path].set_external((*n_in)[cset_path]);
                }

                // Check topology
                if(topo_type != "rectilinear")
                {
                    Node &n_new_topo = n_mesh[topo_path];
                    n_new_topo = n_topo;
                    n_new_topo["type"] = "rectilinear";
                }
                else
                {
                    n_mesh[topo_path].set_external((*n_in)[topo_path]);
                }
            }
            else // if (rt == "uniform")
            {
                n_mesh[cset_path].set_external((*n_in)[cset_path]);
                n_mesh[topo_path].set_external((*n_in)[topo_path]);
            }
        }

        // Now time to start combining meshes that neighbor eachother
        mode = (type == "uniform" ? CombineImplicitMode::Uniform
            : (type == "rectilinear" ? CombineImplicitMode::Rectilinear : CombineImplicitMode::Structured));

        // Combine implicit expects const Node *
        std::vector<const Node*> n_meshes(temp_meshes.size(), nullptr);
        for(size_t i = 0; i < temp_meshes.size(); i++)
        {
            n_meshes[i] = &temp_meshes[i];
        }

        tolerance = merge_tolerance;

        if(mode == CombineImplicitMode::Structured)
        {
            retval = combine_structured_impl(n_meshes, original_element_ids, original_vertex_ids, output);
        }
        else
        {
            retval = combine_implicit_impl(n_meshes, output);
        }
        return retval;
    }

private:
    // Constants
    using vec = blueprint::mesh::coordset::utils::vec3;
    using combine_structured_data_t = std::pair<const Node *, std::vector<vec>>;
    static constexpr index_t MAXDIM = 3;
    enum class CombineImplicitMode : index_t
    {
        Uniform = 0,
        Rectilinear = 1,
        Structured = 2
    };
    static const std::array<std::string, 3> CI_CSET_TYPES;
    static const std::array<std::string, 3> CI_TOPO_TYPES;
    static const index_t dim_for_face[6];
    static const index_t dim_for_edge[4];

    //-------------------------------------------------------------------------
    /**
    @brief Helper function to case a node value at a given index to double.
    */
    static double as_double(const Node &n_vals, index_t idx)
    {
        return n_vals.as_float64_accessor()[idx];
    };

    //-------------------------------------------------------------------------
    /**
    @brief Returns a reference to the correct vector defined in conduit_blueprint_mesh_utils.hpp.
        Inspects n_inputs[0] and [1] to determine the proper axes.
    */
    static const std::vector<std::string> &
    figure_out_implicit_axes(const std::vector<const Node *> &n_inputs)
    {
        // n_inputs was already checked to be > 1
        // It's possible that n_inputs[0] claims to be logical
        //  but the rest of the inputs exist in a different coordsys.
        // For example if the first input has no explicit origin or spacing
        //  it will report as "logical" but the next guy may report an origin in xyz.
        const std::string &csys0 = mesh::utils::coordset::coordsys(*n_inputs[0]);
        const std::string &csys1 = mesh::utils::coordset::coordsys(*n_inputs[1]);
        return (csys0 == "logical"
            // if csys0 was logical, check csys1 for a non-logical coordsys
            ? (csys1 == "cartesian" ? mesh::utils::CARTESIAN_AXES
                : (csys1 == "cylindrical" ? mesh::utils::CYLINDRICAL_AXES
                    : csys1 == "spherical" ? mesh::utils::SPHERICAL_AXES : mesh::utils::LOGICAL_AXES))
            // if csys0 wasn't logical, lookup the proper axes
            : (csys0 == "cartesian" ? mesh::utils::CARTESIAN_AXES
                : (csys0 == "cylindrical" ? mesh::utils::CYLINDRICAL_AXES
                    : csys0 == "spherical" ? mesh::utils::SPHERICAL_AXES : mesh::utils::LOGICAL_AXES))
        );
    }

    //-------------------------------------------------------------------------
    std::vector<index_t>
    find_implicit_offsets(const Node &whole_mesh,
            const Node &sub_mesh) const
    {
        std::vector<index_t> offsets;
        const Node &n_whole_cset = whole_mesh[cset_path];
        const Node &n_sub_cset   = sub_mesh[cset_path];
        if(mode == CombineImplicitMode::Uniform)
        {
            const auto worigin = mesh::utils::coordset::uniform::origin(n_whole_cset);
            const auto sorigin = mesh::utils::coordset::uniform::origin(n_sub_cset);
            const auto spacing = mesh::utils::coordset::uniform::spacing(n_whole_cset);
            for(size_t i = 0; i < worigin.size(); i++)
            {
                const auto difference = sorigin[i] - worigin[i];
                offsets.push_back(difference / spacing[i]);
            }
        }
        else if(mode == CombineImplicitMode::Rectilinear)
        {
            const auto exts = mesh::utils::coordset::extents(n_sub_cset);
            const Node &n_values = n_whole_cset["values"];
            const auto cset_axes = mesh::utils::coordset::axes(n_whole_cset);
            for(size_t i = 0; i < cset_axes.size(); i++)
            {
                const Node &n_value = n_values[cset_axes[i]];
                if(n_value.dtype().is_floating_point())
                {
                    float64_accessor da = n_value.as_float64_accessor();
                    offsets.push_back(
                        mesh::coordset::utils::find_rectilinear_offset(da, exts[i*2], tolerance));
                }
                else
                {
                    CONDUIT_ERROR("Unknown value type for recilinear coordset. " << n_value.dtype().name());
                }
            }
        }
        else // if(mode == CombineImplicitMode::Structured)
        {
        #ifndef OLD_STRUCTURED_COMBINE
            // NOTE: Structured combine does all the offset calculations itself because it also
            //  has to rotate and reflect input domains.
            CONDUIT_ERROR("Internal error, structured meshes should not be combined in this way.")
        #else
            const Node &n_whole_topo = whole_mesh[topo_path];
            const Node &n_sub_topo   = sub_mesh[topo_path];
            std::array<index_t, MAXDIM> whole_dims;
            std::array<index_t, MAXDIM> sub_dims;
            mesh::utils::topology::logical_dims(n_whole_topo, whole_dims.data(), MAXDIM);
            mesh::utils::topology::logical_dims(n_sub_topo, sub_dims.data(), MAXDIM);

            // TODO: Handle potential "grain" issue
            std::array<double, MAXDIM> start_values;
            Node temp;
            for(index_t i = 0; i < dimension; i++)
            {
                const Node &n_dim_vals = n_sub_cset["values/"+axes[i]];
                temp.set_external(DataType(n_dim_vals.dtype().id(), 1),
                    const_cast<void*>(n_dim_vals.element_ptr(0)));
                start_values[i] = temp.to_double();
            }

            // Find this value in the whole mesh
            const Node &n_values = n_whole_cset["values"];
            for(index_t di = 0; di < dimension; di++)
            {
                std::array<index_t, MAXDIM> ijk{0, 0, 0};
                const float64_accessor n_value = n_values[axes[di]].as_float64_accessor();
                index_t local_id = 0;
                bool found = false;
                for(index_t vi = 0; vi < n_value.number_of_elements(); vi++)
                {
                    ijk[di] = vi;
                    grid_ijk_to_id(ijk.data(), sub_dims.data(), local_id);
                    if(std::abs(n_value[vi] - start_values[di]) <= tolerance)
                    {
                        offsets.push_back(vi);
                        found = true;
                        break;
                    }
                }

                if(!found)
                {
                    CONDUIT_ERROR("Unable to find offset for structured topology.");
                }
            }
        #endif
        }
        return offsets;
    }

#if 0
    //-------------------------------------------------------------------------
    // Not used, maybe useful.
    DataType
    find_best_coord_vals_dtype(const Node &n_vals1, const Node &n_vals2) const
    {
        bool floating_point = false;
        index_t max_bytes = 4;
        for(index_t i = 0; i < dimension; i++)
        {
            const auto &dt1 = n_vals1[axes[i]].dtype();
            const auto &dt2 = n_vals2[axes[i]].dtype();
            floating_point = (!floating_point ? dt1.is_floating_point() : floating_point);
            floating_point = (!floating_point ? dt2.is_floating_point() : floating_point);
            max_bytes = std::max(max_bytes, dt1.element_bytes());
            max_bytes = std::max(max_bytes, dt2.element_bytes());
        }
        return (floating_point
            ? (max_bytes == 4 ? DataType::float32() : DataType::float64())
            : (max_bytes == 4 ? DataType::int32()   : DataType::int64()));
    }
#endif

    //-------------------------------------------------------------------------
    /**
    @brief Given a face id returns the dimension that corresponds to it.
        For instance face_id 4 represents the i=0 plane (when dimension == 3),
        so 0 would be the return value.
    @param face_id The id of the face
    @param[out] other_dims Optional argument which the function will use to report out
        the face's uv dimensions. For example when face_id is 4 the face's uv dimenions
        correspond to jk, so [1,2] would be returned.
    @return The corresponding dimension.
    */
    index_t
    determine_matched_dim(const index_t face_id, index_t *other_dims = nullptr) const
    {
        const index_t matched_dim = (dimension == 3) ? dim_for_face[face_id] : dim_for_edge[face_id];
        if(other_dims)
        {
            if(dimension == 3)
            {
                if(face_id < 2)     // face_id == 0 or 1
                {
                    other_dims[0] = 0;
                    other_dims[1] = 1;
                }
                else if(face_id < 4) // face_id == 2 or 3
                {
                    other_dims[0] = 2;
                    other_dims[1] = 0;
                }
                else                // face_id == 4 or 5
                {
                    other_dims[0] = 1;
                    other_dims[1] = 2;
                }
            }
            else if(dimension == 2)
            {
                other_dims[0] = (matched_dim == 0) ? 1 : 0;
            }
        }
        return matched_dim;
    }

    //-------------------------------------------------------------------------
    /**
    @brief Face ids correspond with different extents of the structured grid.
        For example face_id = 5 is the face that has i = dims[0] - 1.
        This function looks up the face case and sets ijk accordingly.
    */
    void
    idx_for_matched_dim(const index_t face_id, const index_t *dims, index_t *ijk) const
    {
        index_t idx = 0;
        ijk[0] = 0; ijk[1] = 0; ijk[2] = 0;
        if(dimension == 3)
        {
            if(face_id == 1) idx = dims[2] - 1;
            else if(face_id == 3) idx = dims[1] - 1;
            else if(face_id == 5) idx = dims[0] - 1;
            ijk[dim_for_face[face_id]] = idx;
        }
        else if(dimension == 2)
        {
            if(face_id == 1) idx = dims[1] - 1;
            else if(face_id == 3) idx = dims[0] - 1;
            ijk[dim_for_edge[face_id]] = idx;
        }
        else
        {
            ijk[0] = (face_id == 0) ? 0 : dims[0]-1;
        }
    }

    //-------------------------------------------------------------------------
    /**
    @brief Read the type information in n_values to create an output schema for the
            given dimensions.
    @param n_values The values node of a coordset that exists in this->dimension
    @param dims The sizes of each dimension (indexed based off this->dimension)
    @param out_schema A schema to be used for an explicit coordset's "values"
    @return The number of verticies in the new schema
    */
    index_t
    build_output_schema(const Node &n_values, const index_t *dims, Schema &out_schema) const
    {
        std::array<index_t, MAXDIM> offsets{0,0,0};
        index_t stride = 0;
        index_t N = 1;
        for(index_t i = 0; i < dimension; i++)
        {
            const auto &in_dtype = n_values[i].dtype();
            offsets[i] = stride;
            stride += in_dtype.element_bytes();
            N *= dims[i];
        }
        for(index_t i = 0; i < dimension; i++)
        {
            const Node &in_node = n_values[i];
            const auto &in_dtype = in_node.dtype();
            out_schema[in_node.name()].set(DataType(in_dtype.id(),
                N, offsets[i], stride, in_dtype.element_bytes(),
                in_dtype.endianness()));
        }
        return N;
    }

    //-------------------------------------------------------------------------
    /**
    @brief Inspects the given face ids and permutation id to determine
      how the two meshes will need to be oriented in the resulting combination.

      The *order variables will hold a mapping from local_dims to global dims
      IE: (i,j,k) -> (k,j,i) would be represented as [2,1,0]

      The *reverse variables will hold a 1 if the dimension needs to be reversed and 0 otherwise.
      IE: (i,j,k) -> (-k,j,-i) would be represented as [1,0,1].
    */
    void
    lookup_case(const index_t lhs_face, const index_t rhs_face,
        const index_t rhs_permutation,
        index_t *lhs_order, int *lhs_reverse,
        index_t *rhs_order, int *rhs_reverse) const
    {
        if(dimension == 3)
        {
            std::array<index_t, 2> uv_lhs;
            std::array<index_t, 2> uv_rhs;
            const index_t dim_lhs = determine_matched_dim(lhs_face, uv_lhs.data());
            const index_t dim_rhs = determine_matched_dim(rhs_face, uv_rhs.data());
            static const int permutation_lookup[8][2] = {
                {0, 1}, {1, 0}, {0, 1}, {1, 0}, // (u,v), (-v,u), (-u,-v), (v,-u)
                {1, 0}, {0, 1}, {1, 0}, {0, 1}  // (v,u), (u,-v), (-v,-u), (-u,v)
            };
            static const int permutation_lookup_reverse[8][2] = {
                {false,false}, {true,false}, {true,true}, {false,true},
                {false,false}, {false,true}, {true,true}, {true,false}
            };
            const int *perm = permutation_lookup[rhs_permutation];
            const int *rev  = permutation_lookup_reverse[rhs_permutation];
            std::array<index_t, 2> temp;
            for(int  i = 0; i < 2; i++)
            {
                temp[i] = uv_rhs[perm[i]];
            }

#ifdef DEBUG_STRUCTURED_COMBINE
            std::cout << "PERMUTATION " << rhs_permutation << std::endl;
            std::cout << "RHS UV:\n  ";
            for(int  i = 0; i < 2; i++)
            {
                std::cout << temp[i] << " ";
            }
            std::cout << std::endl;
            std::cout << "RHS REV:\n  ";
            for(int  i = 0; i < 2; i++)
            {
                std::cout <<  rev[perm[i]] << " ";
            }
            std::cout << std::endl;
#endif

            rhs_order[dim_lhs]   = dim_rhs;
            rhs_order[uv_lhs[0]] = temp[0];
            rhs_order[uv_lhs[1]] = temp[1];
            rhs_reverse[0] = 0; rhs_reverse[1] = 0; rhs_reverse[2] = 0;
            rhs_reverse[uv_lhs[0]] = rev[perm[0]];
            rhs_reverse[uv_lhs[1]] = rev[perm[1]];
            lhs_order[0] = 0;
            lhs_order[1] = 1;
            lhs_order[2] = 2;
            lhs_reverse[0] = false;
            lhs_reverse[1] = false;
            lhs_reverse[2] = false;

            // 0 means matched_dim[0] 1 means matched_dim[max]
            static const int matched_face_value[6] = {0,1,0,1,0,1};
            const int lhs_matched_dim_value = matched_face_value[lhs_face];
            const int rhs_matched_dim_value = matched_face_value[rhs_face];

            // LHS can't start on the plane dividing the left from the right
            if(lhs_matched_dim_value == 0)
            {
                lhs_reverse[dim_lhs] = true;
            }

            // Opposite applies for RHS
            if(rhs_matched_dim_value == 1)
            {
                rhs_reverse[dim_rhs] = true;
            }
        }
        else if(dimension == 2)
        {
            const index_t dim_lhs = determine_matched_dim(lhs_face);
            const index_t dim_rhs = determine_matched_dim(rhs_face);
            const index_t other_dim_lhs = (dim_lhs == 0) ? 1 : 0;
            const index_t other_dim_rhs = (dim_rhs == 0) ? 1 : 0;

            rhs_order[dim_lhs] = dim_rhs;
            rhs_order[other_dim_lhs] = other_dim_rhs;
            rhs_reverse[dim_lhs] = false;
            if(rhs_permutation != 0)
            {
                rhs_reverse[other_dim_lhs] = true;
            }
            lhs_order[0] = 0;
            lhs_order[1] = 1;
            lhs_reverse[0] = false;
            lhs_reverse[1] = false;

            static const int matched_face_value[4] = {0,1,0,1};
            const int lhs_matched_dim_value = matched_face_value[lhs_face];
            const int rhs_matched_dim_value = matched_face_value[rhs_face];
            // LHS can't start on the edge dividing the left from the right
            if(lhs_matched_dim_value == 0)
            {
                lhs_reverse[dim_lhs] = true;
            }

            // Opposite applies for RHS
            if(rhs_matched_dim_value == 1)
            {
                rhs_reverse[dim_rhs] = true;
            }
        }
    }

    //-------------------------------------------------------------------------
    /**
    @brief Inspects n_mesh to create a bounding box for the mesh extents.
      For 2D out_bb.size() == 4, for 3D out_bb.size() == 6.
    */
    void create_structured_bounding_box(const Node &n_mesh,
        std::vector<vec> &out_bb) const
    {
        std::array<index_t, MAXDIM> dims;
        mesh::utils::topology::logical_dims(n_mesh.fetch_existing(topo_path),
            dims.data(), MAXDIM);

        // Want to work in vertex dims here
        for(index_t &d : dims) d += 1;

        const Node &n_cset_values = n_mesh.fetch_existing(cset_path+"/values");
        std::array<const Node*, MAXDIM> xyz{
            n_cset_values.fetch_ptr(axes[0]),
            n_cset_values.fetch_ptr(axes[1]),
            (axes.size() > 2 ? n_cset_values.fetch_ptr(axes[2]) : nullptr)
        };

        out_bb.clear();
        if(dimension == 3)
        {
            out_bb.reserve(8);
            std::array<index_t, MAXDIM> ijk{0, 0, 0};
            for(ijk[2] = 0; ijk[2] < dims[2]; ijk[2] += dims[2]-1)
            {
                for(ijk[1] = 0; ijk[1] < dims[1]; ijk[1] += dims[1]-1)
                {
                    for(ijk[0] = 0; ijk[0] < dims[0]; ijk[0] += dims[0]-1)
                    {
                        vec temp;
                        index_t id;
                        grid_ijk_to_id(ijk.data(), dims.data(), id);
                        for(index_t i = 0; i < dimension; i++)
                        {
                            temp[i] = as_double(*xyz[i], id);
                        }
                        out_bb.push_back(temp);
                    }
                }
            }
        }
        else if(dimension == 2)
        {
            out_bb.reserve(4);
            std::array<index_t, MAXDIM> ijk{0, 0, 0};
            for(ijk[1] = 0; ijk[1] < dims[1]; ijk[1] += dims[1]-1)
            {
                for(ijk[0] = 0; ijk[0] < dims[0]; ijk[0] += dims[0]-1)
                {
                    vec temp;
                    index_t id;
                    grid_ijk_to_id(ijk.data(), dims.data(), id);
                    for(index_t i = 0; i < dimension; i++)
                    {
                        temp[i] = as_double(*xyz[i], id);
                    }
                    temp[2] = 0;
                    out_bb.push_back(temp);
                }
            }
        }
        else // if(dimension == 1)
        {
            out_bb.reserve(2);
            std::array<index_t, MAXDIM> ijk{0, 0, 0};
            for(ijk[0] = 0; ijk[0] < dims[0]; ijk[0] += dims[0]-1)
            {
                vec temp;
                index_t id;
                grid_ijk_to_id(ijk.data(), dims.data(), id);
                for(index_t i = 0; i < dimension; i++)
                {
                    temp[i] = as_double(*xyz[i], id);
                }
                temp[1] = 0;
                temp[2] = 0;
                out_bb.push_back(temp);
            }
        }
    }

    //-------------------------------------------------------------------------
    /**
    @brief Using the bounding box, try to match a face in lhs to a face in rhs;
        permuting the faces of rhs if necessary.
    */
    bool match_structured_faces(
                const combine_structured_data_t &lhs,
                const combine_structured_data_t &rhs,
                index_t &lhs_face_id, index_t &rhs_face_id, index_t &perm) const
    {
        const double t2    = tolerance*tolerance;
        const auto &lhs_bb = lhs.second;
        const auto &rhs_bb = rhs.second;

        // TODO: Static class variable?
        // The order of these face definitions matter because they
        //  all create planes with the same uv orientation.
        static const index_t faces3d[6][4] = {
            {0, 2, 3, 1}, {4, 6, 7, 5},  // Back  / Front (k0, kmax)
            {0, 1, 5, 4}, {2, 3, 7, 6},  // Bottom/ Top   (j0, jmax)
            {0, 4, 6, 2}, {1, 5, 7, 3}   // Left  / Right (i0, imax)
        };

        // TODO: Static class variable?
        // The order of these edge definitions matter because they
        //  all create edges with the same uv orientation.
        static const index_t edges2d[4][2] = {
            {0, 1}, {2, 3}, // Bottom/ Top   (j0, jmax)
            {0, 2}, {1, 3}  // Left  / Right (i0, imax)
        };

        if(dimension == 3)
        {
            for(std::size_t i = 0; i < 6; i++)
            {
                // Grab a face definition in lhs
                std::array<vec, 4> lhs_face;
                for(std::size_t fi = 0; fi < 4; fi++)
                {
                    lhs_face[fi] = lhs_bb[faces3d[i][fi]];
                }

                bool match_found = false;
                for(std::size_t j = 0; j < 6; j++)
                {
                    std::array<vec, 4> rhs_face;
                    for(std::size_t fi = 0; fi < 4; fi++)
                    {
                        rhs_face[fi] = rhs_bb[faces3d[j][fi]];
                    }

                    // TODO: Static class variable?
                    static const index_t perms[8][4] = {            // Inside our matched plane
                        {0,1,2,3}, {3,0,1,2}, {2,3,0,1}, {1,2,3,0}, // (u,v), (-v,u), (-u,-v), (v,-u)
                        {0,3,2,1}, {1,0,3,2}, {2,1,0,3}, {3,2,1,0}  // (v,u), (u,-v), (-v,-u), (-u,v)
                    };

                    for(index_t pi = 0; pi < 8; pi++)
                    {
                        match_found = true;
                        for(index_t pj = 0; pj < 4; pj++)
                        {
                            const double d2 = lhs_face[pj].distance2(rhs_face[perms[pi][pj]]);
                            if(d2 > t2)
                            {
                                match_found = false;
                                break;
                            }
                        }

                        if(match_found)
                        {
                            lhs_face_id = i;
                            rhs_face_id = j;
                            perm = pi;
                            return true;
                        }
                    }
                }
            }
        }
        else if(dimension == 2)
        {
            for(std::size_t i = 0; i < 4; i++)
            {
                // Grab a face definition in lhs
                std::array<vec, 2> lhs_edge;
                for(std::size_t fi = 0; fi < 2; fi++)
                {
                    lhs_edge[fi] = lhs_bb[edges2d[i][fi]];
                }
                lhs_edge[0][2] = 0;
                lhs_edge[1][2] = 0;

                bool match_found = false;
                for(std::size_t j = 0; j < 4; j++)
                {
                    std::array<vec, 2> rhs_edge;
                    for(std::size_t fi = 0; fi < 2; fi++)
                    {
                        rhs_edge[fi] = rhs_bb[edges2d[j][fi]];
                    }
                    rhs_edge[0][2] = 0;
                    rhs_edge[1][2] = 0;

                    // TODO: Static class variable?
                    static const index_t perms[2][2] = {
                        {0,1}, {1,0}
                    };

                    for(index_t pi = 0; pi < 2; pi++)
                    {
                        match_found = true;
                        for(index_t pj = 0; pj < 2; pj++)
                        {
                            const double d2 = lhs_edge[pj].distance2(rhs_edge[perms[pi][pj]]);
                            if(d2 > t2)
                            {
                                match_found = false;
                                break;
                            }
                        }

                        if(match_found)
                        {
                            lhs_face_id = i;
                            rhs_face_id = j;
                            perm = pi;
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------
    /**
    @brief Iterates over the given domain using l_dims, l_reorder, l_reverse
      to translate it from its local ijk to the new global ijk. Results are
      stored in orig_vert_domains, orig_vert_ids, elem_map.
    @param domain_id This domain's id, will be used to fill orig_vert_domains.
    @param g_dims The vertex dim lengths of the new domain. "Global dims"
    @param l_dims The vertex dim lengths of this domain.
        NOTE: This should be the domain's native dims, NOT reordered in any way.
        This function will internally handle any changes in orientation using the given
        information.
    @param l_reorder A mapping of local dimensions to global dimensions.
        For example {i,j,k} -> {j,i,k} would be [1,0,2].
    @param l_reverse Indicates whether a given dimension needs to be reversed.
        For example {i,j,k} -> {-j,i,k} would be [1,0,0]
    @param g_offsets The offsets into the new domain's ijk space that this domain will start.
    @param orig_vert_domains Output data array for source vertex domain ids.
    @param orig_vert_ids Output data array for source vertex ids.
    @param elem_map Output data array for source element domain ids and vertex ids.
    */
    void map_structured_verts_elems(
            const index_t domain_id,
            const index_t *g_dims, const index_t *l_dims, const index_t *l_reorder,
            const int *l_reverse, const index_t *g_offsets,
            DataArray<index_t> &orig_vert_domains, DataArray<index_t> &orig_vert_ids,
            DataArray<index_t> &elem_map) const
    {
        const std::array<index_t, MAXDIM> reordered_dims{
            l_dims[l_reorder[0]],
            (dimension > 1) ? l_dims[l_reorder[1]] : 0,
            (dimension > 2) ? l_dims[l_reorder[2]] : 0
        };
        if(dimension == 3)
        {
            const index_t d2m1 = reordered_dims[2] - 1;
            const index_t d1m1 = reordered_dims[1] - 1;
            const index_t d0m1 = reordered_dims[0] - 1;
            // Map verticies to new global ijk
            std::array<index_t, 3> ijk;
            for(index_t k = 0; k < reordered_dims[2]; k++)
            {
                ijk[2] = l_reverse[2] ? (d2m1-k) : k;
                const index_t koffset = g_offsets[2] + k;
                for(index_t j = 0; j < reordered_dims[1]; j++)
                {
                    ijk[1] = l_reverse[1] ? (d1m1-j) : j;
                    const index_t joffset = g_offsets[1] + j;
                    for(index_t i = 0; i < reordered_dims[0]; i++)
                    {
                        ijk[0] = l_reverse[0] ? (d0m1-i) : i;
                        const std::array<index_t, 3> ijk_local = {
                            ijk[l_reorder[0]],
                            ijk[l_reorder[1]],
                            ijk[l_reorder[2]]
                        };
                        const std::array<index_t, 3> ijk_global = {
                            g_offsets[0] + i,
                            joffset,
                            koffset};
                        index_t local_id, global_id;
                        grid_ijk_to_id(ijk_local.data(), l_dims, local_id);
                        grid_ijk_to_id(ijk_global.data(), g_dims, global_id);
                        orig_vert_domains[global_id] = domain_id;
                        orig_vert_ids[global_id]     = local_id;
                    }
                }
            }
            // Map elements to new global ijk
            const index_t d2m2 = d2m1 - 1;
            const index_t d1m2 = d1m1 - 1;
            const index_t d0m2 = d0m1 - 1;
            const std::array<index_t, 3> g_elem_dims{g_dims[0]-1, g_dims[1]-1, g_dims[2]-1};
            const std::array<index_t, 3> l_elem_dims{l_dims[0]-1, l_dims[1]-1, l_dims[2]-1};
            for(index_t k = 0; k < d2m1; k++)
            {
                ijk[2] = l_reverse[2] ? (d2m2-k) : k;
                const index_t koffset = g_offsets[2] + k;
                for(index_t j = 0; j < d1m1; j++)
                {
                    ijk[1] = l_reverse[1] ? (d1m2-j) : j;
                    const index_t joffset = g_offsets[1] + j;
                    for(index_t i = 0; i < d0m1; i++)
                    {
                        ijk[0] = l_reverse[0] ? (d0m2-i) : i;
                        const std::array<index_t, 3> ijk_local = {
                            ijk[l_reorder[0]],
                            ijk[l_reorder[1]],
                            ijk[l_reorder[2]]
                        };
                        const std::array<index_t, 3> ijk_global = {
                            g_offsets[0] + i,
                            joffset,
                            koffset};
                        index_t local_id, global_id;
                        grid_ijk_to_id(ijk_local.data(), l_elem_dims.data(), local_id);
                        grid_ijk_to_id(ijk_global.data(), g_elem_dims.data(), global_id);
                        const index_t idx = global_id * 2;
                        elem_map[idx]   = domain_id;
                        elem_map[idx+1] = local_id;
                    }
                }
            }
        }
        else if(dimension == 2)
        {
            const index_t d1m1 = reordered_dims[1] - 1;
            const index_t d0m1 = reordered_dims[0] - 1;
            // Map verticies to new global ijk
            std::array<index_t, 3> ijk;
            ijk[2] = 0;
            for(index_t j = 0; j < reordered_dims[1]; j++)
            {
                ijk[1] = l_reverse[1] ? (d1m1-j) : j;
                const index_t joffset = g_offsets[1] + j;
                for(index_t i = 0; i < reordered_dims[0]; i++)
                {
                    ijk[0] = l_reverse[0] ? (d0m1-i) : i;
                    const std::array<index_t, 3> ijk_local = {
                        ijk[l_reorder[0]],
                        ijk[l_reorder[1]],
                        0
                    };
                    const std::array<index_t, 3> ijk_global = {
                        g_offsets[0] + i,
                        joffset,
                        0};
                    index_t local_id, global_id;
                    grid_ijk_to_id(ijk_local.data(), l_dims, local_id);
                    grid_ijk_to_id(ijk_global.data(), g_dims, global_id);
                    orig_vert_domains[global_id] = domain_id;
                    orig_vert_ids[global_id]     = local_id;
                }
            }
            // Map elements to new global ijk
            const index_t d1m2 = d1m1 - 1;
            const index_t d0m2 = d0m1 - 1;
            const std::array<index_t, 3> g_elem_dims{g_dims[0]-1, g_dims[1]-1, 0};
            const std::array<index_t, 3> l_elem_dims{l_dims[0]-1, l_dims[1]-1, 0};
            ijk[2] = 0;
            for(index_t j = 0; j < d1m1; j++)
            {
                ijk[1] = l_reverse[1] ? (d1m2-j) : j;
                const index_t joffset = g_offsets[1] + j;
                for(index_t i = 0; i < d0m1; i++)
                {
                    ijk[0] = l_reverse[0] ? (d0m2-i) : i;
                    const std::array<index_t, 3> ijk_local = {
                        ijk[l_reorder[0]],
                        ijk[l_reorder[1]],
                        0
                    };
                    const std::array<index_t, 3> ijk_global = {
                        g_offsets[0] + i,
                        joffset,
                        0};
                    index_t local_id, global_id;
                    grid_ijk_to_id(ijk_local.data(), l_elem_dims.data(), local_id);
                    grid_ijk_to_id(ijk_global.data(), g_elem_dims.data(), global_id);
                    const index_t idx = global_id * 2;
                    elem_map[idx]   = domain_id;
                    elem_map[idx+1] = local_id;
                }
            }
        }
        else // if(dimension == 1)
        {
            CONDUIT_ERROR("TODO: SUPPORT 1D");
        }
    }

    //-------------------------------------------------------------------------
    bool combine_implicit_impl(const std::vector<const Node *> &n_meshes,
            Node &output) const
    {
        using bounding_box = blueprint::mesh::coordset::utils::bounding_box<vec>;
        using combine_implicit_data_t = std::pair<const Node *, bounding_box>;

        // Make sure extents matchup in the correct way
        std::vector<combine_implicit_data_t> meshes_and_bbs;
        for(const Node *n_mesh : n_meshes)
        {
            const Node *n_cset = n_mesh->fetch_ptr(cset_path);
            auto extents = mesh::utils::coordset::extents(*n_cset);
            bounding_box bb;
            for(index_t d = 0; d < dimension; d++)
            {
                const index_t ext_idx = d*2;
                bb.min[d] = extents[ext_idx];
                bb.max[d] = extents[ext_idx+1];
            }
            meshes_and_bbs.push_back({{n_mesh}, {bb}});
        }

        // Match and combine edges/planes on coordest boundaries until we have 1 left
        Node n_temporary_meshes;
        index_t iteration = 0;
        while(meshes_and_bbs.size() > 1)
        {
            // Print the work in progress
            // std::cout << "iteration " << iteration << "\n";
        #if 0
            for(size_t ei = 0; ei < csets_and_bbs.size(); ei++)
            {
                // const Node *n = csets_and_bbs[ei].first;
                const auto &bb = csets_and_bbs[ei].second;
                std::cout << "  " << ei << ": min[";
                for(index_t d = 0; d < dimension; d++)
                {
                    std::cout << bb.min[d] << (d == (dimension - 1) ? "] " : ", ");
                }
                std::cout << "  " << ei << ": max[";
                for(index_t d = 0; d < dimension; d++)
                {
                    std::cout << bb.max[d] << (d == (dimension - 1) ? "] " : ", ");
                }
                std::cout << "\n";
            }
            std::cout << std::endl;
        #elif 0
            output.print();
        #elif 0
            for(size_t ei = 0; ei < csets_and_bbs.size(); ei++)
            {
                std::cout << "[" << ei << "]" << std::endl;
                csets_and_bbs[ei].first->print();
            }
            std::cout << std::endl;
        #else
            (void)iteration;
        #endif
            iteration++;

            // Get the first extents
            bool any_matches = false;
            for(size_t ei = 0; ei < meshes_and_bbs.size(); ei++)
            {
                const Node *n_meshi = meshes_and_bbs[ei].first;
                auto &exti = meshes_and_bbs[ei].second;

                // Find a match
                const index_t NOT_FOUND = meshes_and_bbs.size();
                index_t matched_extents = NOT_FOUND;
                for(size_t ej = ei+1; ej < meshes_and_bbs.size(); ej++)
                {
                    const Node *n_meshj = meshes_and_bbs[ej].first;
                    const auto &extj = meshes_and_bbs[ej].second;

                    for(index_t di = 0; di < dimension; di++)
                    {
                        // First check if the end of one domain touches the start of another
                        const bool check1 = std::abs(exti.max[di] - extj.min[di]) <= tolerance;
                        const bool check2 = std::abs(exti.min[di] - extj.max[di]) <= tolerance;
                        if(!check1 && !check2)
                        {
                            continue;
                        }
                        // Now check that the extents of the touching domains match in the other dimensions
                        bool corners_match = true;
                        for(index_t dj = 0; dj < dimension; dj++)
                        {
                            if(dj == di) { continue; }
                            // All other axis extents should be equal
                            const bool check3 = std::abs(exti.min[dj] - extj.min[dj]) <= tolerance;
                            const bool check4 = std::abs(exti.max[dj] - extj.max[dj]) <= tolerance;
                            if(!check3 || !check4)
                            {
                                corners_match = false;
                                break;
                            }
                        }

                        // If the corners match combine them
                        if(corners_match)
                        {
                            Node &new_mesh = n_temporary_meshes.append();
                            Node &new_cset = new_mesh[cset_path];
                            Node &new_topo = new_mesh[topo_path];

                            new_cset["type"] = CI_CSET_TYPES[(index_t)mode];
                            new_topo["type"] = CI_TOPO_TYPES[(index_t)mode];
                            new_topo["coordset"] = new_cset.name();

                            const Node &n_lhs = (check1 ? *n_meshi : *n_meshj);
                            // const Node &n_rhs = (check1 ? *n_meshj : *n_meshi);
                            // const double matched_val = (check1 ? exti.max[di] : exti.min[di]);
                            // If there is an origin in the lhs domain, copy it to the new mesh
                            {
                                const Node *origin = n_lhs.fetch_ptr(topo_path+"/elements/origin");
                                if(origin)
                                {
                                    new_topo["elements/origin"] = *origin;
                                }
                            }

                            if(mode == CombineImplicitMode::Uniform)
                            {
                                // std::cout << "Handling uniform combine" << std::endl;

                                std::vector<double> spacing{1.,1.,1.};
                                if(output.has_path(cset_path + "/spacing"))
                                {
                                    const Node &out_spacing = output.fetch_existing(cset_path + "/spacing");
                                    new_cset["spacing"] = out_spacing;
                                    spacing = utils::coordset::uniform::spacing(new_cset);
                                }

                                Schema s_origin;
                                Schema s_dims;
                                for(index_t dj = 0; dj < dimension; dj++)
                                {
                                    s_origin[axes[dj]].set(DataType::c_double(1, dj*sizeof(double), dimension*sizeof(double)));
                                    s_dims[mesh::utils::LOGICAL_AXES[dj]].set(DataType::index_t(1, dj*sizeof(index_t), dimension*sizeof(index_t)));
                                }
                                new_cset["origin"].set(s_origin);
                                new_cset["dims"].set(s_dims);

                                // Update the extents
                                exti.min[di] = std::min(exti.min[di], extj.min[di]);
                                exti.max[di] = std::max(exti.max[di], extj.max[di]);

                                for(index_t dj = 0; dj < dimension; dj++)
                                {
                                    const std::string dims_path = "dims/"+mesh::utils::LOGICAL_AXES[dj];
                                    new_cset["origin/"+axes[dj]] = exti.min[dj];
                                    // Extents are inclusive so we need to add 1 to the difference
                                    new_cset[dims_path] = (index_t)(((exti.max[dj] - exti.min[dj]) / spacing[dj]) + 1.5);
                                }
                                meshes_and_bbs[ei].first = &new_mesh;
                                meshes_and_bbs.erase(meshes_and_bbs.begin() + ej);
                                matched_extents = ej;
                                break;
                            }
                            else if(mode == CombineImplicitMode::Rectilinear)
                            {
                                // std::cout << "Handling rectilinear combine" << std::endl;
                                // We need to further check that the spacing along the matched edge/plane is okay
                                const Node &n_cseti = n_meshi->fetch_existing(cset_path);
                                const Node &n_csetj = n_meshj->fetch_existing(cset_path);
                                bool ok = true;
                                index_t dim_sizes[3];
                                index_t max_bytes = 0;
                                for(index_t dj = 0; dj < dimension; dj++)
                                {
                                    const Node &n_vals0 = n_cseti["values/"+axes[dj]];
                                    const Node &n_vals1 = n_csetj["values/"+axes[dj]];
                                    max_bytes = std::max(max_bytes, n_vals0.dtype().element_bytes());
                                    max_bytes = std::max(max_bytes, n_vals1.dtype().element_bytes());
                                    if(di == dj)
                                    {
                                        dim_sizes[dj] = n_vals0.dtype().number_of_elements() + n_vals1.dtype().number_of_elements() - 1;
                                    }
                                    else
                                    {
                                        dim_sizes[dj] = n_vals0.dtype().number_of_elements();
                                        ok = mesh::coordset::utils::node_value_compare(n_vals0, n_vals1);
                                        if(!ok)
                                        {
                                            // std::cout << "Incompatible rectilinear domains" << std::endl;
                                            break;
                                        }
                                    }
                                }

                                // std::cout << "ok? " << ok << std::endl;

                                if(!ok)
                                {
                                    n_temporary_meshes.remove(n_temporary_meshes.number_of_children() - 1);
                                    matched_extents = NOT_FOUND;
                                    break;
                                }

                                // std::cout << "Made it to the heavy lifting!" << std::endl;

                                // Update the extents
                                exti.min[di] = std::min(exti.min[di], extj.min[di]);
                                exti.max[di] = std::max(exti.max[di], extj.max[di]);

                                // Allocate the output arrays
                                const DataType out_dtype((max_bytes < 8) ? DataType::c_float() : DataType::c_double());
                                index_t offset = 0;
                                Schema s;
                                for(index_t dj = 0; dj < dimension; dj++)
                                {
                                    s[axes[dj]].set(DataType(out_dtype.id(), dim_sizes[dj], offset,
                                        out_dtype.element_bytes(), out_dtype.element_bytes(), out_dtype.endianness()));
                                    offset += out_dtype.element_bytes() * dim_sizes[dj];
                                }
                                new_cset["values"].set(s);
                                std::array<const Node*, 2> n_in_di{
                                    ((check1) ? n_cseti.fetch_ptr("values/"+axes[di]) : n_csetj.fetch_ptr("values/"+axes[di])),
                                    ((check1) ? n_csetj.fetch_ptr("values/"+axes[di]) : n_cseti.fetch_ptr("values/"+axes[di]))
                                };
                                for(index_t dj = 0; dj < dimension; dj++)
                                {
                                    Node &n_out_values = new_cset["values/"+axes[dj]];
                                    if(di == dj)
                                    {
                                        index_t out_idx = 0;
                                        for(size_t i = 0; i < n_in_di.size(); i++, out_idx--)
                                        {
                                            const Node &n_in_values = *n_in_di[i];
                                            out_idx = mesh::coordset::utils::copy_node_data(n_in_values, n_out_values, out_idx);
                                        }
                                    }
                                    else
                                    {
                                        const Node &n_vals0 = n_cseti["values/"+axes[dj]];
                                        mesh::coordset::utils::copy_node_data(n_vals0, n_out_values);
                                    }
                                }

                                meshes_and_bbs[ei].first = &new_mesh;
                                meshes_and_bbs.erase(meshes_and_bbs.begin() + ej);
                                matched_extents = ej;
                                break;
                            }
                            else // if(mode == CombineImplicitMode::Structured)
                            {
                                CONDUIT_ERROR("Internal error when combining implicit topology, structured combinations should follow a different path.");
                                return false;
                            }
                        }
                    }

                    if(matched_extents != NOT_FOUND)
                    {
                        any_matches = true;
                        break;
                    }
                }
            }

            if(any_matches == false)
            {
                break;
            }
        }
        // std::cout << "REMAINING DOMAINS " << meshes_and_bbs.size() << std::endl;
        bool retval = false;
        if(meshes_and_bbs.size() == 1)
        {
            // TODO: Figure out how to move the data out of the temporary node
            //  instead of deep copying.
            output[cset_path] = (*meshes_and_bbs[0].first)[cset_path];
            output[topo_path] = (*meshes_and_bbs[0].first)[topo_path];
            build_implicit_maps(n_meshes, output, output[cset_path+"/pointmaps"], output[topo_path+"/element_map"]);
            retval = true;
        }
        return retval;
    }

    //-------------------------------------------------------------------------
    bool
    combine_structured_impl(
            const std::vector<const Node *> &n_meshes,
            const std::string &/*original_element_ids*/,
            const std::string &/*original_vertex_ids*/,
            Node &output) const
    {
        // Cannot use axis-aligned bounding boxes for structured grids
        std::vector<combine_structured_data_t> meshes_and_bbs;
        for(const Node *n_mesh : n_meshes)
        {
            meshes_and_bbs.push_back({{n_mesh}, {}});
            create_structured_bounding_box(*n_mesh, meshes_and_bbs.back().second);
        }

        // Match and combine edges/planes on coordest boundaries until we have 1 left
        Node n_temporary_meshes;
        index_t iteration = 0;
        while(meshes_and_bbs.size() > 1)
        {
        #ifdef DEBUG_STRUCTURED_COMBINE
            std::cout << "Iteration " << iteration << std::endl;
            if(iteration > 0)
            {
                Node temp;
                for(size_t i = 0; i < meshes_and_bbs.size(); i++)
                {
                    const Node &mesh = *meshes_and_bbs[i].first;
                    Node &out_mesh = temp[(i < 10) ? "domain_0000" + std::to_string(i) : "domain_000" + std::to_string(i)];
                    out_mesh.set_external(mesh);

                    Node &oeid = out_mesh["fields/" + original_element_ids];
                    oeid["association"] = "element";
                    oeid["topology"] = "mesh";
                    Schema s;
                    const auto N = mesh["topologies/mesh/element_map"].dtype().number_of_elements() / 2;
                    const void *ptr = mesh["topologies/mesh/element_map"].element_ptr(0);
                    s["domains"].set(DataType::index_t(N, 0, 2*sizeof(index_t)));
                    s["ids"].set(DataType::index_t(N, sizeof(index_t), 2*sizeof(index_t)));
                    oeid["values"].set_external(s, const_cast<void*>(ptr));

                    Node &ovid = out_mesh["fields/" + original_vertex_ids];
                    ovid["association"] = "vertex";
                    ovid["topology"] = "mesh";
                    ovid["values"].set_external(mesh["coordsets/coords/pointmaps"]);
                }
                std::ofstream f_out("combine_structured_iteration." +
                    ((iteration < 10) ? ("0000" + std::to_string(iteration)) : ("000" + std::to_string(iteration)))
                    + ".yaml");
                temp.to_string_stream(f_out);
            }
        #else
            // Silence iteration warning
            (void)iteration;
        #endif
            iteration++;

            // Get the first extents
            bool any_matches = false;
            for(size_t ei = 0; ei < meshes_and_bbs.size(); ei++)
            {
                const Node *n_meshi = meshes_and_bbs[ei].first;

                // Find a match
                bool ei_matched = false;
                for(size_t ej = ei+1; ej < meshes_and_bbs.size(); ej++)
                {
                    const Node *n_meshj = meshes_and_bbs[ej].first;

                    index_t ei_face = 0;
                    index_t ej_face = 0;
                    index_t ej_permutation = 0;
                    bool match_found = match_structured_faces(
                        meshes_and_bbs[ei], meshes_and_bbs[ej],
                        ei_face, ej_face, ej_permutation);

                    if(match_found)
                    {
                        // TODO: Create a function to allocate a new temp mesh
                        //  this can be the same for structured and implicit combines
                        Node &new_mesh = n_temporary_meshes.append();
                        Node &new_cset = new_mesh[cset_path];
                        Node &new_topo = new_mesh[topo_path];
                        new_cset["type"] = "explicit";
                        new_topo["type"] = "structured";
                        new_topo["coordset"] = new_cset.name();

                        // If there is an origin in the lhs domain, copy it to the new mesh
                        {
                            const Node *origin = n_meshi->fetch_ptr(topo_path+"/elements/origin");
                            if(origin)
                            {
                                new_topo["elements/origin"] = *origin;
                            }
                        }

                    #ifdef DEBUG_STRUCTURED_COMBINE
                        std::cout << "Matched faces " << n_meshi->fetch_existing("id").to_index_t() << ":" << ei_face
                            << " to " << n_meshj->fetch_existing("id").to_index_t() << ":" << ej_face << ":"
                            << ej_permutation << std::endl;
                    #endif

                        // Determine who is the lhs and who is the rhs
                        const Node *n_lhs = nullptr;
                        const Node *n_rhs = nullptr;
                        index_t lhs_face = 0;
                        index_t rhs_face = 0;
                        if((ej_permutation == 0) &&
                            ((ei_face == 1 && ej_face == 0)
                            || (ei_face == 3 && ej_face == 2)
                            || (ei_face == 5 && ej_face == 4)))
                        {
                            n_lhs = n_meshi;
                            n_rhs = n_meshj;
                            lhs_face = ei_face;
                            rhs_face = ej_face;
                        }
                        else if((ej_permutation == 0) &&
                            ((ei_face == 0 && ej_face == 1)
                            || (ei_face == 2 && ej_face == 3)
                            || (ei_face == 4 && ej_face == 5)))
                        {
                            n_lhs = n_meshj;
                            n_rhs = n_meshi;
                            lhs_face = ej_face;
                            rhs_face = ei_face;
                        }
                        else // Every other odd case, ei is lhs. This is the only way the permutation case is accurate.
                        {
                            n_lhs = n_meshi;
                            n_rhs = n_meshj;
                            lhs_face = ei_face;
                            rhs_face = ej_face;
                        }

                        // Convenience
                        using mesh::coordset::utils::kdtree;
                        using mesh::coordset::utils::vec3;
                        using mesh::coordset::utils::vec2;
                        const Node &n_cset_lhs = n_lhs->fetch_existing(cset_path);
                        const Node &n_topo_lhs = n_lhs->fetch_existing(topo_path);
                        const Node &n_cset_rhs = n_rhs->fetch_existing(cset_path);
                        const Node &n_topo_rhs = n_rhs->fetch_existing(topo_path);
                        std::array<const Node *, MAXDIM> vals_lhs{
                            n_cset_lhs.fetch_ptr("values/"+axes[0]),
                            n_cset_lhs.fetch_ptr("values/"+axes[1]),
                            (axes.size() > 2) ? n_cset_lhs.fetch_ptr("values/"+axes[2]) : nullptr
                        };
                        std::array<DataType, MAXDIM> dts_lhs{
                            (vals_lhs[0]) ? DataType(vals_lhs[0]->dtype().id(), 1) : DataType(),
                            (vals_lhs[1]) ? DataType(vals_lhs[1]->dtype().id(), 1) : DataType(),
                            (vals_lhs[2]) ? DataType(vals_lhs[2]->dtype().id(), 1) : DataType(),
                        };
                        std::array<const Node *, MAXDIM> vals_rhs{
                            n_cset_rhs.fetch_ptr("values/"+axes[0]),
                            n_cset_rhs.fetch_ptr("values/"+axes[1]),
                            (axes.size() > 2) ? n_cset_rhs.fetch_ptr("values/"+axes[2]) : nullptr
                        };
                        std::array<DataType, MAXDIM> dts_rhs{
                            (vals_rhs[0]) ? DataType(vals_rhs[0]->dtype().id(), 1) : DataType(),
                            (vals_rhs[1]) ? DataType(vals_rhs[1]->dtype().id(), 1) : DataType(),
                            (vals_rhs[2]) ? DataType(vals_rhs[2]->dtype().id(), 1) : DataType(),
                        };

                        // Collect required info
                        std::array<index_t, MAXDIM> dims_lhs{0,0,0};
                        std::array<index_t, MAXDIM> dims_rhs{0,0,0};
                        topology::logical_dims(n_topo_lhs, dims_lhs.data(), dimension);
                        topology::logical_dims(n_topo_rhs, dims_rhs.data(), dimension);
                        // Make this the dimensions for the verts (instead of elements)
                        for(auto &val : dims_lhs) val += 1;
                        for(auto &val : dims_rhs) val += 1;

                        // Now we must validate that this plane/edge matches
                        std::array<index_t, MAXDIM> ijk{0,0,0};
                        std::array<index_t, MAXDIM-1> other_dims{0,0};
                        Node n_temp;
                        if(dimension == 3)
                        {
                            // Build the tree with all the points from the lhs face
                            kdtree<mesh::coordset::utils::vec3, index_t> verts;
                            vec3 temp;
                            index_t idx;

                            // TODO: Sort "other_dims" in ascending order for better memory traversal
                            determine_matched_dim(lhs_face, other_dims.data());
                            idx_for_matched_dim(lhs_face, dims_lhs.data(), ijk.data());
                            for(index_t j = 0; j < dims_lhs[other_dims[1]]; j++)
                            {
                                ijk[other_dims[1]] = j;
                                for(index_t i = 0; i < dims_lhs[other_dims[0]]; i++)
                                {
                                    ijk[other_dims[0]] = i;
                                    grid_ijk_to_id(ijk.data(), dims_lhs.data(), idx);
                                    for(index_t di = 0; di < dimension; di++)
                                    {
                                        n_temp.set_external(dts_lhs[di], const_cast<void*>(vals_lhs[di]->element_ptr(idx)));
                                        temp[di] = n_temp.to_double();
                                    }
                                    verts.insert(temp, idx);
                                }
                            }

                            // Check all the points in the tree against the points from the rhs face
                            // TODO: Sort "other_dims" in ascending order for better memory traversal
                            determine_matched_dim(rhs_face, other_dims.data());
                            idx_for_matched_dim(rhs_face, dims_rhs.data(), ijk.data());
                            index_t *existing_idx = nullptr;
                            for(index_t j = 0; j < dims_rhs[other_dims[1]]; j++)
                            {
                                ijk[other_dims[1]] = j;
                                for(index_t i = 0; i < dims_rhs[other_dims[0]]; i++)
                                {
                                    ijk[other_dims[0]] = i;
                                    grid_ijk_to_id(ijk.data(), dims_rhs.data(), idx);
                                    for(index_t di = 0; di < dimension; di++)
                                    {
                                        n_temp.set_external(dts_rhs[di], const_cast<void*>(vals_rhs[di]->element_ptr(idx)));
                                        temp[di] = n_temp.to_double();
                                    }
                                    existing_idx = verts.find_point(temp, tolerance);
                                    if(!existing_idx)
                                    {
                                        // If we couldn't match this face then we can't combine as structured.
                                        CONDUIT_INFO("Face definition does not match.");
                                        return false;
                                    }
                                }
                            }
                        }
                        else if(dimension == 2)
                        {
                            // Build the tree with all the points from the lhs face
                            kdtree<mesh::coordset::utils::vec2, index_t> verts;
                            vec2 temp;
                            index_t idx;

                            // This is the lhs so our plane is on the end of matched dim
                            determine_matched_dim(lhs_face, other_dims.data());
                            idx_for_matched_dim(lhs_face, dims_lhs.data(), ijk.data());
                            for(index_t i = 0; i < dims_lhs[other_dims[0]]; i++)
                            {
                                ijk[other_dims[0]] = i;
                                grid_ijk_to_id(ijk.data(), dims_lhs.data(), idx);
                                for(index_t di = 0; di < dimension; di++)
                                {
                                    n_temp.set_external(dts_lhs[di], const_cast<void*>(vals_lhs[di]->element_ptr(idx)));
                                    temp[di] = n_temp.to_double();
                                }
                                verts.insert(temp, idx);
                            }

                            // Check all the points in the tree against the points from the rhs face
                            determine_matched_dim(rhs_face, other_dims.data());
                            idx_for_matched_dim(rhs_face, dims_rhs.data(), ijk.data());
                            index_t *existing_idx = nullptr;
                            for(index_t i = 0; i < dims_rhs[other_dims[0]]; i++)
                            {
                                ijk[other_dims[0]] = i;
                                grid_ijk_to_id(ijk.data(), dims_rhs.data(), idx);
                                for(index_t di = 0; di < dimension; di++)
                                {
                                    n_temp.set_external(dts_rhs[di], const_cast<void*>(vals_rhs[di]->element_ptr(idx)));
                                    temp[di] = n_temp.to_double();
                                }
                                existing_idx = verts.find_point(temp, tolerance);
                                if(!existing_idx)
                                {
                                    // If we couldn't match this face then we can't combine as structured.
                                    CONDUIT_INFO("Face definition does not match.");
                                    return false;
                                }
                            }
                        }
                        // Nothing needs to be checked for dimension 1
                        // NOTE: We would have returned from the function if the check failed

                        // Creates the "new_mesh" and handling orientation cases
                        combine_structured_submeshes(
                            *n_lhs, *n_rhs, dims_lhs.data(),
                            dims_rhs.data(), lhs_face, rhs_face, ej_permutation,
                            new_mesh);

                        meshes_and_bbs[ei].first = &new_mesh;
                        create_structured_bounding_box(new_mesh, meshes_and_bbs[ei].second);
                        // TODO: Destroy n_lhs and n_rhs if they are temp nodes
                        meshes_and_bbs.erase(meshes_and_bbs.begin() + ej);
                        ei_matched = true;
                        break;
                    }
                }
                if(ei_matched)
                {
                    any_matches = true;
                    continue;
                }
            }

            if(any_matches == false)
            {
                break;
            }
        }
        // std::cout << "REMAINING DOMAINS " << meshes_and_bbs.size() << std::endl;
        bool retval = false;
        if(meshes_and_bbs.size() == 1)
        {
            // TODO: Figure out how to move the data out of the temporary node
            //  instead of deep copying.
            output[cset_path] = (*meshes_and_bbs[0].first)[cset_path];
            output[topo_path] = (*meshes_and_bbs[0].first)[topo_path];

        #ifdef DEBUG_STRUCTURED_COMBINE
            std::ofstream f_out("combine_structured_final.yaml");
            if((*meshes_and_bbs[0].first).has_child("fields"))
            {
                output["fields"].set_external((*meshes_and_bbs[0].first)["fields"]);
            }
            output.to_string_stream(f_out);
            if(output.has_child("fields")) output.remove_child("fields");
        #endif
            retval = true;
        }
        return retval;
    }

    //-------------------------------------------------------------------------
    /**
    @brief Creates a new domain in n_new_mesh that is the combination
      of n_lhs and n_rhs. This new domain will start in n_lhs and append
      n_rhs in the appropriate direction. This handles any remapping that
      needs to happen as a result of transforming the input domains.
    */
    void combine_structured_submeshes(
        const Node &n_lhs, const Node &n_rhs,
        const index_t *dims_lhs, const index_t *dims_rhs,
        const index_t face_lhs, const index_t face_rhs, const index_t perm_rhs,
        Node &n_new_mesh) const
    {
        const std::string orig_verts_path = cset_path + "/pointmaps";
        const std::string orig_elements_path = topo_path + "/element_map";
        const Node &n_cset_lhs = n_lhs[cset_path];
        Node &n_new_topo = n_new_mesh[topo_path];
        Node &n_new_cset = n_new_mesh[cset_path];

        // Determine final mesh size
        const index_t matched_dim = determine_matched_dim(face_lhs);
        std::array<index_t, MAXDIM> order_lhs{0,1,2};
        std::array<index_t, MAXDIM> order_rhs{0,1,2};
        std::array<int, MAXDIM>     reverse_lhs{0,0,0};
        std::array<int, MAXDIM>     reverse_rhs{0,0,0};
        lookup_case(face_lhs, face_rhs, perm_rhs,
            order_lhs.data(), reverse_lhs.data(),
            order_rhs.data(), reverse_rhs.data());
        std::array<index_t, MAXDIM> new_dims{1, 1, 1};
        for(index_t i = 0; i < dimension; i++)
        {
            new_dims[i] = (i == matched_dim)
                ? dims_lhs[i] + dims_rhs[order_rhs[i]] - 1
                : dims_lhs[i];
        }

        // Set the dims in our output topology
        n_new_topo["elements/dims/i"] = new_dims[0]-1;
        if(dimension > 1) n_new_topo["elements/dims/j"] = new_dims[1]-1;
        if(dimension > 2) n_new_topo["elements/dims/k"] = new_dims[2]-1;

        // Allocate output cset
        Schema out_schema;
        const index_t N = build_output_schema(n_cset_lhs["values"], new_dims.data(), out_schema);
        Node &out_vals = n_new_cset["values"];
        out_vals.set(out_schema);

        Schema s_pointmap;
        s_pointmap["domains"].set(DataType::index_t(N, 0, 2*sizeof(index_t)));
        s_pointmap["ids"].set(DataType::index_t(N, sizeof(index_t), 2*sizeof(index_t)));
        Node &n_pointmap = n_new_mesh[orig_verts_path];
        n_pointmap.set(s_pointmap);
        DataArray<index_t> orig_vert_domains = n_pointmap["domains"].value();
        DataArray<index_t> orig_vert_ids     = n_pointmap["ids"].value();

    #ifdef DEBUG_STRUCTURED_COMBINE
        std::cout << "Dims LHS:";
        for(int i = 0; i < 3; i++)
        {
            std::cout << " " << dims_lhs[i];
        }
        std::cout << std::endl;
        std::cout << "Dims RHS:";
        for(int i = 0; i < 3; i++)
        {
            std::cout << " " << dims_rhs[i];
        }
        std::cout << std::endl;
        std::cout << "New dims:";
        for(int i = 0; i < 3; i++)
        {
            std::cout << " " << new_dims[i];
        }
        std::cout << std::endl;
    #endif

        index_t Nele = 1;
        for(index_t d = 0; d < dimension; d++) Nele *= (new_dims[d]-1);
        // The fields functions later in combine expect this to be 1 array, not a schema
        Node &n_elem_map = n_new_mesh[orig_elements_path];
        n_elem_map.set(DataType::index_t(Nele*2));
        DataArray<index_t> elem_map = n_elem_map.value();

        const std::array<index_t, 2> domain_lookup{
            n_lhs["id"].to_index_t(),
            n_rhs["id"].to_index_t()
        };
        n_new_mesh["id"] = n_lhs["id"];

        {
            // LHS shouldn't have offsets but it may need to reverse a dim
            const std::array<index_t, 3> no_offsets{0, 0, 0};
            const std::array<index_t, 3> no_reorder{0, 1, 2};
            map_structured_verts_elems(domain_lookup[0], new_dims.data(),
                dims_lhs, no_reorder.data(), reverse_lhs.data(), no_offsets.data(),
                orig_vert_domains, orig_vert_ids, elem_map);
        }

        {
            // RHS will be tranformed to exist in LHS' ijk space
            std::array<index_t, MAXDIM> offsets{0,0,0};
            offsets[matched_dim] = dims_lhs[matched_dim] - 1;
            map_structured_verts_elems(domain_lookup[1], new_dims.data(),
                dims_rhs, order_rhs.data(), reverse_rhs.data(), offsets.data(),
                orig_vert_domains, orig_vert_ids, elem_map);
        }

        // Move the coordinates
        const Node &n_vals_lhs = n_cset_lhs["values"];
        const Node &n_vals_rhs = n_rhs[cset_path + "/values"];
        Node temp1, temp2;
        for(index_t i = 0; i < orig_vert_domains.number_of_elements(); i++)
        {
            const index_t domain = orig_vert_domains[i];
            const index_t id     = orig_vert_ids[i];
            if(domain == domain_lookup[0])
            {
            #ifdef DEBUG_STRUCTURED_COMBINE
                if(!(id < n_vals_lhs[0].dtype().number_of_elements()))
                {
                    CONDUIT_ERROR("INDEX OUT OF BOUNDS, INVALID VALUE IN POINTMAP");
                }
            #endif

                for(index_t d = 0; d < dimension; d++)
                {
                    temp1.set_external(
                        DataType(n_vals_lhs[d].dtype().id(),1),
                        const_cast<void*>(n_vals_lhs[d].element_ptr(id)));
                    temp1.to_data_type(out_vals[d].dtype().id(), temp2);
                    std::memcpy(out_vals[d].element_ptr(i),
                        temp2.element_ptr(0),
                        temp2.dtype().element_bytes());
                }
            }
            else if(domain == domain_lookup[1])
            {
            #ifdef DEBUG_STRUCTURED_COMBINE
                if(!(id < n_vals_rhs[0].dtype().number_of_elements()))
                {
                    CONDUIT_ERROR("INDEX OUT OF BOUNDS, INVALID VALUE IN POINTMAP");
                }
            #endif

                for(index_t d = 0; d < dimension; d++)
                {
                    temp1.set_external(
                        DataType(n_vals_rhs[d].dtype().id(),1),
                        const_cast<void*>(n_vals_rhs[d].element_ptr(id)));
                    temp1.to_data_type(out_vals[d].dtype().id(), temp2);
                    std::memcpy(out_vals[d].element_ptr(i),
                        temp2.element_ptr(0),
                        temp2.dtype().element_bytes());
                }
            }
        }

    #if DEBUG_STRUCTURED_COMBINE
        // The mesh used in the unit test has a "dist" field which is useful to plot
        //  in visit to see if the vertex mappings got scrambled.
        if(n_lhs.has_path("fields/dist/values") && n_rhs.has_path("fields/dist/values"))
        {
            n_new_mesh["fields/dist/association"] = "vertex";
            n_new_mesh["fields/dist/topology"] = "mesh";
            n_new_mesh["fields/dist/values"].set(DataType::c_float(N));
            float *out_dist = (float*)n_new_mesh["fields/dist/values"].element_ptr(0);
            const float *dist_lhs = (const float*)n_lhs["fields/dist/values"].element_ptr(0);
            const float *dist_rhs = (const float*)n_rhs["fields/dist/values"].element_ptr(0);
            for(index_t i = 0; i < orig_vert_domains.number_of_elements(); i++)
            {
                const index_t domain = orig_vert_domains[i];
                const index_t id     = orig_vert_ids[i];
                if(domain == domain_lookup[0])
                {
                    out_dist[i] = dist_lhs[id];
                }
                else if(domain == domain_lookup[1])
                {
                    out_dist[i] = dist_rhs[id];
                }
            }
        }
    #endif

        // If the domains already had pointmaps/element maps get the original values
        bool lhs_has_maps = n_lhs.has_path(orig_verts_path);
        bool rhs_has_maps = n_rhs.has_path(orig_verts_path);
        if(lhs_has_maps || rhs_has_maps)
        {
            const DataType dt = DataType::index_t(1);
            const std::array<const Node*, 2> vert_maps{
                n_lhs.fetch_ptr(orig_verts_path),
                n_rhs.fetch_ptr(orig_verts_path)
            };
            const std::array<const Node*, 2> elem_maps{
                n_lhs.fetch_ptr(orig_elements_path),
                n_rhs.fetch_ptr(orig_elements_path)
            };
            Node temp_domain;
            Node temp_id;
            // Do verticies
            for(index_t i = 0; i < orig_vert_domains.number_of_elements(); i++)
            {
                if(orig_vert_domains[i] == domain_lookup[0] && vert_maps[0])
                {
                    const index_t id = orig_vert_ids[i];
                    temp_domain.set_external(dt, const_cast<void*>(vert_maps[0]->fetch("domains").element_ptr(id)));
                    temp_id.set_external(dt, const_cast<void*>(vert_maps[0]->fetch("ids").element_ptr(id)));
                    orig_vert_domains[i] = temp_domain.to_index_t();
                    orig_vert_ids[i] = temp_id.to_index_t();
                }
                else if(orig_vert_domains[i] == domain_lookup[1] && vert_maps[1])
                {
                    const index_t id = orig_vert_ids[i];
                    temp_domain.set_external(dt, const_cast<void*>(vert_maps[1]->fetch("domains").element_ptr(id)));
                    temp_id.set_external(dt, const_cast<void*>(vert_maps[1]->fetch("ids").element_ptr(id)));
                    orig_vert_domains[i] = temp_domain.to_index_t();
                    orig_vert_ids[i] = temp_id.to_index_t();
                }
            }
            // Do elements
            for(index_t i = 0; i < Nele; i++)
            {
                const index_t idx = i*2;
                if(elem_map[idx] == domain_lookup[0] && elem_maps[0])
                {
                    const index_t id = elem_map[idx+1];
                    const index_t l_idx = id*2;
                #ifdef DEBUG_STRUCTURED_COMBINE
                    if(l_idx >= elem_maps[0]->dtype().number_of_elements())
                    {
                        CONDUIT_ERROR("INDEX OUT OF BOUNDS " << l_idx);
                    }
                #endif
                    temp_domain.set_external(dt, const_cast<void*>(elem_maps[0]->element_ptr(l_idx)));
                    temp_id.set_external(dt, const_cast<void*>(elem_maps[0]->element_ptr(l_idx+1)));
                    elem_map[idx]   = temp_domain.to_index_t();
                    elem_map[idx+1] = temp_id.to_index_t();
                }
                else if(elem_map[idx] == domain_lookup[1] && elem_maps[1])
                {
                    const index_t id = elem_map[idx+1];
                    const index_t l_idx = id*2;
                #ifdef DEBUG_STRUCTURED_COMBINE
                    if(l_idx >= elem_maps[1]->dtype().number_of_elements())
                    {
                        CONDUIT_ERROR("INDEX OUT OF BOUNDS " << l_idx);
                    }
                #endif
                    temp_domain.set_external(dt, const_cast<void*>(elem_maps[1]->element_ptr(l_idx)));
                    temp_id.set_external(dt, const_cast<void*>(elem_maps[1]->element_ptr(l_idx+1)));
                    elem_map[idx]   = temp_domain.to_index_t();
                    elem_map[idx+1] = temp_id.to_index_t();
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    /**
    @brief Creates pointmaps and element_maps for rectilinear and uniform combinations
        to be used by to map fields later in the combine process.
        CANNOT BE USED FOR STRUCTURED.
    */
    void build_implicit_maps(const std::vector<const Node *> n_meshes,
            const Node &final_mesh,
            Node &out_pointmaps,
            Node &out_element_map) const
    {
        std::vector<index_t> elem_dims(dimension);
        mesh::utils::topology::logical_dims(final_mesh[topo_path], elem_dims.data(), dimension);
        auto final_dim_lengths = elem_dims;
        index_t Nelem = 1;
        for(auto &final_dim_length : final_dim_lengths)
        {
            // final_dim_length is the number of elements, then we add 1 to get points
            Nelem = Nelem * final_dim_length;
            final_dim_length = final_dim_length + 1;
        }

        // Allocate the output_elem_map
        out_element_map.set(DataType::index_t(Nelem*2));
        DataArray<index_t> emap_da = out_element_map.value();

        index_t dom_idx = 0;
        for(const Node *n_mesh : n_meshes)
        {
            // Get the number of elements in this submesh
            const auto &this_elem_dims = logical_dims[dom_idx];

            // Allocate output pointmap
            const Node *n_cset = n_mesh->fetch_ptr(cset_path);
            const index_t N = mesh::utils::coordset::length(*n_cset);
            const DataType dt(DataType::index_t(N));
            Node &pointmap = out_pointmaps.append();
            pointmap.set(dt);
            DataArray<index_t> pmap_da = pointmap.value();

            // Determine where this submesh exists in the global space
            const auto offsets = find_implicit_offsets(final_mesh, *n_mesh);
            if(final_dim_lengths.size() == 3)
            {
                // Do pointmap
                {
                    const index_t nx   = final_dim_lengths[0];
                    const index_t nxny = nx * final_dim_lengths[1];
                    const index_t ioff = offsets[0];
                    const index_t joff = offsets[1] * nx;
                    const index_t koff = offsets[2] * nxny;
                    index_t idx = 0;
                    for(index_t k = 0; k < this_elem_dims[2]+1; k++)
                    {
                        const index_t knxny = koff + k * nxny;
                        for(index_t j = 0; j < this_elem_dims[1]+1; j++)
                        {
                            const index_t jnx = joff + j * nx;
                            for(index_t i = 0; i < this_elem_dims[0]+1; i++, idx++)
                            {
                                pmap_da[idx] = knxny + jnx + ioff + i;
                            }
                        }
                    }
                }

                // Do element_map
                {
                    const index_t nx   = elem_dims[0];
                    const index_t nxny = nx*elem_dims[1];
                    const index_t ioff = offsets[0];
                    const index_t joff = offsets[1] * nx;
                    const index_t koff = offsets[2] * nxny;
                    const index_t this_nx = this_elem_dims[0];
                    const index_t this_nxny = this_nx * this_elem_dims[1];
                    for(index_t k = 0; k < this_elem_dims[2]; k++)
                    {
                        const index_t this_knxny = k * this_nxny;
                        const index_t knxny = koff + k * nxny;
                        for(index_t j = 0; j < this_elem_dims[1]; j++)
                        {
                            const index_t this_jnx = this_knxny + j * this_nx;
                            const index_t jnx = joff + j * nx;
                            for(index_t i = 0; i < this_elem_dims[0]; i++)
                            {
                                const index_t id  = knxny + jnx + ioff + i;
                                const index_t idx = id * 2;
                                emap_da[idx]   = dom_idx;
                                emap_da[idx+1] = this_jnx + i;
                            }
                        }
                    }
                }
            }
            else if(final_dim_lengths.size() == 2)
            {
                // Do pointmap
                {
                    const index_t ioff = offsets[0];
                    const index_t joff = offsets[1] * final_dim_lengths[0];
                    index_t idx = 0;
                    for(index_t j = 0; j < this_elem_dims[1]+1; j++)
                    {
                        const auto jnx = joff + j * final_dim_lengths[0];
                        for(index_t i = 0; i < this_elem_dims[0]+1; i++, idx++)
                        {
                            pmap_da[idx] = jnx + ioff + i;
                        }
                    }
                }

                // Do element_map
                {
                    const index_t nx   = elem_dims[0];
                    const index_t ioff = offsets[0];
                    const index_t joff = offsets[1] * nx;
                    const index_t this_nx = this_elem_dims[0];
                    for(index_t j = 0; j < this_elem_dims[1]; j++)
                    {
                        const index_t this_jnx = j * this_nx;
                        const index_t jnx = joff + j * nx;
                        for(index_t i = 0; i < this_elem_dims[0]; i++)
                        {
                            const index_t id  = jnx + ioff + i;
                            const index_t idx = id * 2;
                            emap_da[idx]   = dom_idx;
                            emap_da[idx+1] = this_jnx + i;
                        }
                    }
                }
            }
            else // if(dim_lengths.size() == 1)
            {
                // Do pointmap
                {
                    const index_t ioff = offsets[0];
                    for(index_t i = 0; i < this_elem_dims[0]+1; i++)
                    {
                        pmap_da[i] = ioff + i;
                    }
                }

                // Do element_map
                {
                    const index_t ioff = offsets[0];
                    for(index_t i = 0; i < this_elem_dims[0]; i++)
                    {
                        const index_t id  = ioff + i;
                        const index_t idx = id * 2;
                        emap_da[idx] = dom_idx;
                        emap_da[idx] = i;
                    }
                }
            }

            dom_idx++;
        }
    }

    std::string cset_path;
    std::string topo_path;
    std::vector<std::string> axes;
    std::vector<std::array<index_t, MAXDIM>> logical_dims;
    double tolerance;
    index_t dimension;
    CombineImplicitMode mode;
};

const std::array<std::string, 3> combine_implicit_topologies::CI_CSET_TYPES{
    "uniform",
    "rectilinear",
    "explicit",
};

const std::array<std::string, 3> combine_implicit_topologies::CI_TOPO_TYPES{
    "uniform",
    "rectilinear",
    "structured"
};

const index_t combine_implicit_topologies::dim_for_face[6] = {2, 2, 1, 1, 0, 0};
const index_t combine_implicit_topologies::dim_for_edge[4] = {1, 1, 0, 0};


}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::fields --
//-----------------------------------------------------------------------------
namespace fields
{

//-------------------------------------------------------------------------
/**
 @brief Determine an appropriate schema to hold data based on the input.

 @param in The input node, which should be field "values" node.
 @param ntuples The number of tuples in the input node.
 @param[out] out_ncomps The number of components detected in the input field.
 @param[out] out_schema The schema to use when creating the output node, if
                        that needs to be done.

 @note The out_ncomps parameter is 0 for scalars and 1 or more for vectors.
       This way we can tell scalars from vectors down the line.
 */
static void
determine_schema(const Node &in,
        const index_t ntuples, index_t &out_ncomps,
        Schema &out_schema)
{
    out_ncomps = 0;
    out_schema.reset();

    const index_t num_children = in.number_of_children();
    if(num_children)
    {
        out_ncomps = num_children;

#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK) && defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK_VERBOSE)
        std::cout << "========================================================" << std::endl;
        std::cout << "determine_schema:" << std::endl;
        for(index_t i = 0; i < num_children; i++)
        {
            std::cout << "  -" << std::endl;
            std::cout << "    name: " << in[i].name() << std::endl;
            in[i].dtype().to_string_stream(std::cout, "yaml", 4, 1);
        }
        std::cout << "========================================================" << std::endl;
#endif

        // Check that all children are the same type.
        bool same_types = true;
        for(index_t i = 1; i < num_children; i++)
            same_types = (in[i].dtype().id() == in[0].dtype().id());

        if(same_types && conduit::blueprint::mcarray::is_interleaved(in))
        {
            // NOTE: In practice, we don't seem to get here yet from partition_map_back.
            //       The reason is that the mapback routine ends up calling copy_field
            //       to make an extracted field with certain ids. That method calls
            //       slice_field and that function does not preserve interleaving at
            //       this time.

            // Make interleaved data.
            for(index_t i = 0; i < num_children; i++)
            {
                auto offset = i * in[i].dtype().element_bytes();
                auto stride = num_children * in[i].dtype().element_bytes();
                const DataType dt(in[i].dtype().id(),
                                  ntuples,
                                  offset,
                                  stride,
                                  in[i].dtype().element_bytes(),
                                  in[i].dtype().endianness());
                out_schema[in[i].name()].set(dt);
            }
        }
        else
        {
            // All component 0, All component 1, ...
            index_t offset = 0;
            for(index_t i = 0; i < num_children; i++)
            {
                const auto stride = in[i].dtype().element_bytes();
                const DataType dt(in[i].dtype().id(),
                                  ntuples,
                                  offset,
                                  stride,
                                  in[i].dtype().element_bytes(),
                                  in[i].dtype().endianness());
                out_schema[in[i].name()].set(dt);
                offset += dt.number_of_elements() * dt.element_bytes();
            }
        }
    }
    else
    {
        out_ncomps = 0;
        out_schema.set(DataType(in.dtype().id(), ntuples));
    }
}

//-------------------------------------------------------------------------
static index_t
total_storage_size(const Node &in)
{
    index_t size = 0;
    const index_t num_children = in.number_of_children();
    if(num_children > 0)
    {
        for(index_t i = 0; i < num_children; i++)
            size += in[i].dtype().number_of_elements();
    }
    else
    {
        size = in.dtype().number_of_elements();
    }
    return size;
}

//-------------------------------------------------------------------------
static void
map_vertex_field(const std::vector<const Node*> &in_nodes,
        const std::vector<DataArray<index_t>> &pointmaps,
        const index_t num_vertices,
        Node &out_node)
{
    if(in_nodes.empty() || pointmaps.empty())
    {
        return;
    }

    if(in_nodes.size() != pointmaps.size())
    {
        CONDUIT_WARN("Number of input fields and number of pointmaps should be equal!");
    }

    // Figure out num components and out dtype
    index_t ncomps = 0;
    Schema out_schema;
    determine_schema((*in_nodes[0])["values"], num_vertices, ncomps, out_schema);

#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK) && defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK_VERBOSE)
    conduit::Node top;
    conduit::Node &p = top["map_vertex_field"];
    for(auto &val : in_nodes)
        p["in_nodes"].append().set_external_node(*val);
    for(auto &val : pointmaps)
        p["pointmaps"].append().set_external((index_t*)val.data_ptr(), val.number_of_elements());
    p["num_vertices"] = num_vertices;
    p["ncomps"] = ncomps;
    p["out_node"].set_external_node(out_node);
    top.print();
#endif

    // Only reset the node if its current storage cannot contain the new field.
    // Most likely, if we're mapping back, the original mesh contained the
    // field we're mapping in the first place. We don't want to totally reset in
    // case that field points to external data.
    const index_t detected_size = total_storage_size(out_node);
    const index_t actual_ncomps = std::max(ncomps, static_cast<index_t>(1));
    const index_t expected_size = num_vertices * actual_ncomps;
    if(detected_size != expected_size)
    {
        out_node.reset();
        out_node.set(out_schema);

#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK)
        std::cout << out_node.path() << ": resetting to hold " << num_vertices << "*" << actual_ncomps << " values." << std::endl;
        //out_schema.print();
        //out_node.print();
#endif
    }
#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK)
    else
    {
        std::cout << out_node.path() << ": use existing field memory." << std::endl;
    }
#endif

    const index_t npmaps = (index_t)pointmaps.size();
    if(ncomps > 0)
    {
        for(index_t fi = 0; fi < npmaps; fi++)
        {
            const auto &pmap = pointmaps[fi];
            const Node &in_values = in_nodes[fi]->child("values");
            for(index_t idx = 0; idx < pmap.number_of_elements(); idx++)
            {
                const index_t out_idx = pmap[idx];
                for(index_t ci = 0; ci < ncomps; ci++)
                {
                    const auto bytes = out_node[ci].dtype().element_bytes();
                    void *out_data = out_node[ci].element_ptr(out_idx);
                    const void *in_data = in_values[ci].element_ptr(idx);
                    memcpy(out_data, in_data, bytes);
                }
            }
        }
    }
    else
    {
        const auto bytes = out_node.dtype().element_bytes();
        for(index_t fi = 0; fi < npmaps; fi++)
        {
            const auto &pmap = pointmaps[fi];
            const Node &in_values = in_nodes[fi]->child("values");
            for(index_t idx = 0; idx < pmap.number_of_elements(); idx++)
            {
                const index_t out_idx = pmap[idx];
                void *out_data = out_node.element_ptr(out_idx);
                const void *in_data = in_values.element_ptr(idx);
                memcpy(out_data, in_data, bytes);
            }
        }
    }
}

//-------------------------------------------------------------------------
/**
 @brief Overload for the second flavor of pointmap
*/
static void
map_vertex_field(const std::vector<const Node*> &in_nodes,
        const DataArray<index_t> &orig_domains,
        const DataArray<index_t> &orig_ids,
        Node &out_node)
{
    if(in_nodes.empty())
    {
        return;
    }

    // Figure out num components and out dtype
    const index_t num_vertices = orig_domains.number_of_elements();
    index_t ncomps = 0;
    Schema out_schema;
    determine_schema((*in_nodes[0])["values"], num_vertices, ncomps, out_schema);

#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK) && defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK_VERBOSE)
    conduit::Node top;
    conduit::Node &p = top["map_vertex_field"];
    for(auto &val : in_nodes)
        p["in_nodes"].append().set_external_node(*val);
    p["orig_domains"].append().set_external((index_t*)orig_domains.data_ptr(), orig_domains.number_of_elements());
    p["orig_ids"].append().set_external((index_t*)orig_ids.data_ptr(), orig_ids.number_of_elements());
    p["num_vertices"] = num_vertices;
    p["ncomps"] = ncomps;
    p["out_node"].set_external_node(out_node);
    top.print();
#endif

    // Only reset the node if its current storage cannot contain the new field.
    // Most likely, if we're mapping back, the original mesh contained the
    // field we're mapping in the first place. We don't want to totally reset in
    // case that field points to external data.
    const index_t detected_size = total_storage_size(out_node);
    const index_t actual_ncomps = std::max(ncomps, static_cast<index_t>(1));
    const index_t expected_size = num_vertices * actual_ncomps;
    if(detected_size != expected_size)
    {
        out_node.reset();
        out_node.set(out_schema);

#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK)
        std::cout << out_node.path() << ": resetting to hold " << num_vertices << "*" << actual_ncomps << " values." << std::endl;
        //out_schema.print();
        //out_node.print();
#endif
    }
#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK)
    else
    {
        std::cout << out_node.path() << ": use existing field memory." << std::endl;
    }
#endif

    if(ncomps > 0)
    {
        for(index_t i = 0; i < num_vertices; i++)
        {
            const index_t orig_dom = orig_domains[i];
            const index_t orig_id  = orig_ids[i];
            const Node &in_values  = in_nodes[orig_dom]->child("values");
            for(index_t ci = 0; ci < ncomps; ci++)
            {
                const auto bytes = out_node[ci].dtype().element_bytes();
                void *out_data = out_node[ci].element_ptr(i);
                const void *in_data = in_values[ci].element_ptr(orig_id);
                memcpy(out_data, in_data, bytes);
            }
        }
    }
    else
    {
        const auto bytes = out_node.dtype().element_bytes();
        for(index_t i = 0; i < num_vertices; i++)
        {
            const index_t orig_dom = orig_domains[i];
            const index_t orig_id  = orig_ids[i];
            void *out_data = out_node.element_ptr(i);
            const void *in_data = in_nodes[orig_dom]->child("values").element_ptr(orig_id);
            memcpy(out_data, in_data, bytes);
        }
    }
}

//-------------------------------------------------------------------------
static void
map_element_field(const std::vector<const Node*> &in_nodes,
        const DataArray<index_t> &elemmap,
        Node &out_node)
{
    if(in_nodes.empty())
    {
        return;
    }

    // Determine the number of elements and components needed to store the field
    const index_t nelements = elemmap.number_of_elements() / 2;
    index_t ncomps = 0;
    Schema out_schema;
    determine_schema(in_nodes[0]->child("values"), nelements, ncomps, out_schema);

#if defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK) && defined(CONDUIT_DEBUG_PARTITIONER_MAPBACK_VERBOSE)
    conduit::Node top;
    conduit::Node &p = top["map_element_field"];
    for(auto &val : in_nodes)
        p["in_nodes"].append().set_external_node(*val);
    p["elemmap"].set_external((index_t*)elemmap.data_ptr(), elemmap.number_of_elements());
    p["nelements"] = nelements;
    p["ncomps"] = ncomps;
    p["out_node"].set_external_node(out_node);
    conduit::Node &oni = p["out_node_info"];
    oni["number_of_children"] = out_node.number_of_children();
    for(int i = 0; i < out_node.number_of_children(); i++)
    {
        const conduit::Node &src = out_node[i];
        conduit::Node &n = oni[src.name()];
        n["type"] = src.dtype().name();
        n["number_of_elements"] = src.dtype().number_of_elements();
    }
    oni["storage_size"] = total_storage_size(out_node);
    top.print();
#endif

    // Only reset the node if its current storage cannot contain the new field.
    // Most likely, if we're mapping back, the original mesh contained the
    // field we're mapping in the first place. We don't want to totally reset in
    // case that field points to external data.
    const index_t detected_size = total_storage_size(out_node);
    const index_t actual_ncomps = std::max(ncomps, static_cast<index_t>(1));
    const index_t expected_size = nelements * actual_ncomps;
    if(detected_size != expected_size)
    {
        out_node.reset();
        out_node.set(out_schema);

#ifdef CONDUIT_DEBUG_PARTITIONER_MAPBACK
        std::cout << out_node.path() << ": resetting to hold " << nelements << "*" << actual_ncomps << " values." << std::endl;
        //out_schema.print();
        //out_node.print();
#endif
    }
#ifdef CONDUIT_DEBUG_PARTITIONER_MAPBACK
    else
    {
        std::cout << out_node.path() << ": use existing field memory." << std::endl;
    }
#endif

    if(ncomps > 0)
    {
        for(index_t out_idx = 0; out_idx < nelements; out_idx++)
        {
            const index_t idx      = out_idx * 2;
            const index_t dom_idx  = elemmap[idx];
            const index_t elem_idx = elemmap[idx+1];
            const Node &data = in_nodes[dom_idx]->child("values");
            for(index_t ci = 0; ci < ncomps; ci++)
            {
                const auto bytes = out_node[ci].dtype().element_bytes();
                void *out_data = out_node[ci].element_ptr(out_idx);
                const void *in_data = data[ci].element_ptr(elem_idx);
                memcpy(out_data, in_data, bytes);
            }
        }
    }
    else
    {
        const auto bytes = out_node.dtype().element_bytes();
        for(index_t out_idx = 0; out_idx < nelements; out_idx++)
        {
            const index_t idx      = out_idx * 2;
            const index_t dom_idx  = elemmap[idx];
            const index_t elem_idx = elemmap[idx+1];
            const Node &data = in_nodes[dom_idx]->child("values");
            void *out_data = out_node.element_ptr(out_idx);
            const void *in_data = data.element_ptr(elem_idx);
            memcpy(out_data, in_data, bytes);
        }
    }
}

//-----------------------------------------------------------------------------
static void
combine(const std::vector<const Node*> &in_fields,
        const Node &assoc_topology, const Node &assoc_coordset, Node &output)
{
    const std::string &assoc = in_fields[0]->child("association").as_string();
    const std::string &assoc_topo = in_fields[0]->child("topology").as_string();
    output["topology"] = assoc_topo;
    output["association"] = assoc;
    // Determine if we are vertex or element associated
    if(assoc == utils::ASSOCIATIONS[0])
    {
        // Vertex association
        // Need to use pointmaps to map this field
        // Get the point map
        const Node *pointmaps = assoc_coordset.fetch_ptr("pointmaps");
        if(!pointmaps) { CONDUIT_ERROR("No pointmap for coordset"); return; }

        if(!pointmaps->dtype().is_object())
        {
            std::vector<DataArray<index_t>> pmaps;
            if(!pointmaps) { CONDUIT_ERROR("No pointmap for coordset"); return; }
            for(index_t pi = 0; pi < pointmaps->number_of_children(); pi++)
            {
                pmaps.emplace_back(pointmaps->child(pi).value());
            }
            const index_t nt = coordset::length(assoc_coordset);
            fields::map_vertex_field(in_fields, pmaps, nt, output["values"]);
        }
        else
        {
            // Structured combine produces a pointmap with 2 components
            DataArray<index_t> orig_domains = pointmaps->child("domains").value();
            DataArray<index_t> orig_ids     = pointmaps->child("ids").value();
            fields::map_vertex_field(in_fields, orig_domains, orig_ids, output["values"]);
        }
    }
    else if(assoc == utils::ASSOCIATIONS[1])
    {
        // Element association
        // Need to use element maps to map this field
        const Node &out_topo_map = assoc_topology["element_map"];
        const DataArray<index_t> tmap = out_topo_map.value();
        fields::map_element_field(in_fields, tmap, output["values"]);
    }
    else
    {
        CONDUIT_WARN("Unsupported association for field " << assoc);
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::fields --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::adjset --
//-----------------------------------------------------------------------------
namespace adjset
{

static void
map_adjset_group(const Node& adjset,
                 const std::unordered_map<index_t, index_t>& local_cnk_map,
                 const std::vector<DataArray<index_t>>& pointmaps,
                 Node& out_adjset)
{
    // Remap the vertex ids of each intermediate chunk group
    for (const auto& group : adjset["groups"].children())
    {
        Node& tgt_group = out_adjset["groups"].append();
        index_t src_chunk = group["src_chunk"].to_index_t();
        tgt_group["src_chunk"].set(src_chunk);
        tgt_group["neighbors"].set(group["neighbors"].to_index_t());

        // Get the chunk-to-domain vertex id map for the given chunk gid
        auto local_cnk_id = local_cnk_map.find(src_chunk);
        if (local_cnk_id == local_cnk_map.end())
        {
            CONDUIT_ERROR("Could not map source adjset chunk id "
                         << src_chunk << " to a chunk in this domain.");
        }
        const index_t_array& pmap = pointmaps[local_cnk_id->second];

        index_t_array orig_shared_nodes = group["values"].as_index_t_array();
        std::vector<index_t> new_shared_nodes(orig_shared_nodes.number_of_elements());
        for (index_t inode = 0; inode < orig_shared_nodes.number_of_elements(); inode++)
        {
            index_t new_vert_idx = pmap[orig_shared_nodes[inode]];
            new_shared_nodes[inode] = new_vert_idx;
        }
        tgt_group["values"].set(new_shared_nodes);
    }
}

//-----------------------------------------------------------------------------
static void
combine(const std::vector<const Node*>& in_adjsets,
        const std::vector<index_t>& in_chunk_ids,
        const Node& topologies,
        const Node& coordsets,
        Node& out_adjsets)
{
    // Map of global chunk id to their local index on this domain
    std::unordered_map<index_t, index_t> local_cnk_idx;
    Node tmp_dom;
    for(size_t iadj = 0; iadj < in_adjsets.size(); iadj++)
    {
        const Node& adjset = *in_adjsets[iadj];
        local_cnk_idx[in_chunk_ids[iadj]] = iadj;
        attach_chunk_adjset_to_single_dom(tmp_dom, in_chunk_ids[iadj], &adjset);
    }

    for (const auto& adjset : tmp_dom["adjsets"].children())
    {
        const std::string adjset_name = adjset.name();
        const std::string assoc_topo = adjset["topology"].as_string();
        const std::string set_assoc = adjset["association"].as_string();
        if (set_assoc != utils::ASSOCIATIONS[0])
        {
            CONDUIT_WARN("Only vertex-associated adjsets are supported");
            continue;
        }

        // Make sure we have an output topology for the given adjset
        if (!topologies.has_child(assoc_topo))
        {
            CONDUIT_ERROR("Adjset " << adjset_name << " references topology "
                         << assoc_topo << " which doesn't exist.");
        }
        const Node& topo_for_aset = topologies[assoc_topo];

        // Look up the associated coordset for the topology
        const std::string assoc_cset = topo_for_aset["coordset"].as_string();
        if (!coordsets.has_child(assoc_cset))
        {
            CONDUIT_ERROR("Topology " << assoc_topo << " for adjset "
                          << adjset_name << " references coordset "
                          << assoc_cset << " which doesn't exist.");
        }
        const Node& coordset_for_aset = coordsets[assoc_cset];

        // Vertex association
        // Need to use pointmaps to map this field
        // Get the point map
        const Node *pointmaps = coordset_for_aset.fetch_ptr("pointmaps");
        if(!pointmaps) { CONDUIT_ERROR("No pointmap for coordset"); return; }

        if(!pointmaps->dtype().is_object())
        {
            std::vector<DataArray<index_t>> pmaps;
            if(!pointmaps) { CONDUIT_ERROR("No pointmap for coordset"); return; }
            for(index_t pi = 0; pi < pointmaps->number_of_children(); pi++)
            {
                pmaps.emplace_back(pointmaps->child(pi).value());
            }
            map_adjset_group(adjset, local_cnk_idx, pmaps, out_adjsets[adjset_name]);
        }
        else
        {
            // Structured combine produces a pointmap with 2 components
            DataArray<index_t> orig_domains = pointmaps->child("domains").value();
            DataArray<index_t> orig_ids     = pointmaps->child("ids").value();
            CONDUIT_ERROR("Adjset merge unimplemented for structured combine case");
        }
        out_adjsets[adjset_name]["topology"] = assoc_topo;
        out_adjsets[adjset_name]["association"] = set_assoc;
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::adjset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::matset --
//-----------------------------------------------------------------------------
namespace matset
{

typedef std::pair<std::vector<double>, std::vector<index_t>> vf_id_pair_t;

//-----------------------------------------------------------------------------
template<bool is_elem_based>
static void
handle_uni_buffer(const Node &n_matset,
                  const std::vector<index_t> &elem_map,
                  std::map<index_t, std::string> material_map,
                  std::map<std::string, vf_id_pair_t> &out_vfracts_eids)
{
    // If this is material based then we must read element_ids
    std::unique_ptr<DataAccessor<index_t>> element_ids{nullptr};
    if(!is_elem_based)
    {
        element_ids.reset(new DataAccessor<index_t>(n_matset["element_ids"].value()));
    }

    const DataAccessor<index_t> material_ids = n_matset["material_ids"].value();
    const DataAccessor<double> volume_fractions = n_matset["volume_fractions"].value();
    index_t local_elem_id = 0;
    blueprint::o2mrelation::O2MIterator o2m_itr(n_matset);
    // Question: Why does o2m_itr.has_next(o2mrelaton::ONE) contain different logic
    //  than the implementation of o2m_itr.next(o2mrelation::ONE)?
    //  For instance, if we have an o2m relation with 1 item has_next(ONE) will always
    //  return false - even if we called o2m_itr.to_front() first.
    while(o2m_itr.has_next())
    {
        o2m_itr.next(blueprint::o2mrelation::ONE);
        o2m_itr.to_front(blueprint::o2mrelation::MANY);

        while(o2m_itr.has_next(blueprint::o2mrelation::MANY))
        {
            o2m_itr.next(blueprint::o2mrelation::MANY);
            const index_t o2m_idx = o2m_itr.index();
            const std::string &mat_name = material_map[material_ids[o2m_idx]];
            auto &pair = out_vfracts_eids[mat_name];
            pair.first.push_back(volume_fractions[o2m_idx]);
            if(!is_elem_based)
            {
                pair.second.push_back(elem_map[element_ids->element(local_elem_id)]);
            }
            else // if(is_elem_based)
            {
                pair.second.push_back(elem_map[local_elem_id]);
            }
        }
        local_elem_id++;
    }
}

//-----------------------------------------------------------------------------
template<bool is_elem_based>
static void
handle_multi_buffer(const Node &n_matset,
                    const std::vector<index_t> &elem_map,
                    std::map<std::string, vf_id_pair_t> &out_vfracts_eids)
{
    auto vfract_itr = n_matset["volume_fractions"].children();
    while(vfract_itr.has_next())
    {
        const Node &n_volume_fractions = vfract_itr.next();
        const std::string mat_name = n_volume_fractions.name();

        if(n_volume_fractions.dtype().is_empty())
        {
            continue;
        }

        // If this is material based then we must read element_ids
        std::unique_ptr<DataAccessor<index_t>> element_ids{nullptr};
        if(!is_elem_based)
        {
            const Node &n_element_ids = n_matset["element_ids"];
            if(n_element_ids.has_child(mat_name))
            {
                element_ids.reset(new DataAccessor<index_t>(n_element_ids[mat_name].value()));
            }
        }

        // NOTE: According the spec, the volume_fractions could be an o2m relation
        Node temp_vfracts;
        if(n_volume_fractions.dtype().is_object())
        {
            temp_vfracts.set_external(n_volume_fractions);
        }
        else // if(!n_volume_fractions.dtype().is_object())
        {
            temp_vfracts["values"].set_external(n_volume_fractions);
        }

        // Temporary node that points to volume fraction data array.
        Node vfract_data;
        {
            const std::string data_path =
                blueprint::o2mrelation::data_paths(temp_vfracts).front();
            vfract_data.set_external(temp_vfracts[data_path]);
        }
        const DataAccessor<double> volume_fractions = vfract_data.value();

        if(!is_elem_based && (element_ids.get() != nullptr)
            && volume_fractions.number_of_elements() != element_ids->number_of_elements())
        {
            CONDUIT_ERROR("element_ids and volume_fractions to not contain the same "
                << "number of elements for matset " << conduit::utils::log::quote(mat_name));
            continue;
        }

        auto &out_pair = out_vfracts_eids[mat_name];
        blueprint::o2mrelation::O2MIterator itr(temp_vfracts);
        itr.to_front();
        // Iterate the volume fraction data, still need i for indexing into element ids
        for(index_t i = 0; itr.has_next(); i++)
        {
            itr.next();
            const index_t idx = itr.index();
            out_pair.first.push_back(volume_fractions[idx]);
            // Material based will use element_ids array
            if(!is_elem_based && (element_ids.get() != nullptr))
            {
                out_pair.second.push_back(elem_map[element_ids->element(i)]);
            }
            else
            {
                out_pair.second.push_back(elem_map[i]);
            }
        }
    }
}

//-----------------------------------------------------------------------------
/**
 @brief Combine input matsets using n_topo as a reference topology.
        Each matset in the inputs vector must have an additional "domain_id" field.
*/
static void
combine(const std::vector<Node> &inputs,
        const std::vector<const Node*> &in_topos,
        const Node &out_topo,
        Node &out_matset)
{
    const index_t nin = (index_t)inputs.size();
    // n_topo.print();

    // Elem map is an array of index_t [domain0 ele0 domain0 ele1 ...]
    // The structure of this data needs to be changed
    std::vector<std::vector<index_t>> elem_map;
    elem_map.resize(in_topos.size());
    {
        // Resize vectors to the correct size for each domain
        for(index_t i = 0; i < (index_t)in_topos.size(); i++)
        {
            elem_map[i].resize(blueprint::mesh::topology::length(*in_topos[i]));
        }

        const DataArray<index_t> orig_elem_map = out_topo["element_map"].value();
        const index_t N = orig_elem_map.number_of_elements();
        for(index_t i = 0; i < N; i += 2)
        {
            elem_map[orig_elem_map[i]][orig_elem_map[i+1]] = i / 2;
        }
    }

    // Output will always be multi-buffer, material dominant
    std::map<std::string, vf_id_pair_t> out_vfracts_eids;
    std::map<index_t, std::string> material_map;
    for(index_t in_idx = 0; in_idx < nin; in_idx++)
    {
        const Node &n_matset = inputs[in_idx];
        const std::string name = n_matset.name();
        const bool is_uni_buffer = blueprint::mesh::matset::is_uni_buffer(n_matset);
        const bool is_elem_based = blueprint::mesh::matset::is_element_dominant(n_matset);
        const index_t dom_idx = n_matset["domain_id"].to_index_t();

        // Build up the material map
        if(n_matset.has_child("material_map"))
        {
            auto itr = n_matset["material_map"].children();
            while(itr.has_next())
            {
                const Node &n = itr.next();
                material_map[n.to_index_t()] = n.name();
            }
        }

        if(is_uni_buffer)
        {
            if(is_elem_based)
            {
                handle_uni_buffer<true>(n_matset, elem_map[dom_idx], material_map, out_vfracts_eids);
            }
            else
            {
                handle_uni_buffer<false>(n_matset, elem_map[dom_idx], material_map, out_vfracts_eids);
            }
        }
        else // if(!is_uni_buffer)
        {
            if(is_elem_based)
            {
                handle_multi_buffer<true>(n_matset, elem_map[dom_idx], out_vfracts_eids);
            }
            else
            {
                handle_multi_buffer<false>(n_matset, elem_map[dom_idx], out_vfracts_eids);
            }
        }
    }

    out_matset["topology"].set(out_topo.name());
    for(const auto &pair : out_vfracts_eids)
    {
        const std::string &name = pair.first;
        const std::vector<double> &volume_fractions = pair.second.first;
        const std::vector<index_t> &element_ids = pair.second.second;

        out_matset["volume_fractions"][name].set(volume_fractions);
        out_matset["element_ids"][name].set(element_ids);
    }

    // If the material_map does not exist, add it since it supplies the entire
    // list of materials, which can be important when a domain does not have
    // all materials.
    if(!out_matset.has_child("material_map") && !material_map.empty())
    {
        Node &mm = out_matset["material_map"];
        for(auto it = material_map.begin(); it != material_map.end(); it++)
            mm[it->second] = it->first;
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::matset --
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------
std::string
Partitioner::recommended_topology(const std::vector<const Node *> &inputs,
        const std::string &topo_name) const
{
    // Meshes:
    //   Uniform
    //   Rectilinear
    //   Structured
    //   Unstructured
    //     - Single Shape
    //     - Multiple Shape
    //     - Poly

    // Coordsets:
    //   Uniform
    //   Rectilinear
    //   Explicit

    // Topologies:
    //   Points
    //   Uniform
    //   Rectilinear
    //   Structured
    //   Unstructured

    // Redefine these here because the order matters
    static const std::array<std::string, 3> coordset_types = {
        "uniform",
        "rectilinear",
        "explicit"
    };

    static const std::array<std::string, 5> topology_types = {
        "points",
        "uniform",
        "rectilinear",
        "structured",
        "unstructured"
    };

    index_t worst_coordset = 0;
    index_t worst_topology = 0;
    for(const Node *input : inputs)
    {
        const Node *n_topo = input->fetch_ptr("topologies/"+topo_name);
        if(!n_topo)
        {
            CONDUIT_ERROR("Unable to combine inputs, topology \"" << topo_name
                << "\" is not present in all inputs");
        }

        const std::string cset_name = n_topo->child("coordset").as_string();
        const Node *n_cset = input->fetch_ptr("coordsets/"+cset_name);
        if(!n_cset)
        {
            CONDUIT_ERROR("Unable to combine inputs, coordset \"" << cset_name <<
                " is not present in all inputs.");
        }

        // Get coordset type index
        {
            const std::string cset_type = n_cset->child("type").as_string();
            const index_t idx = std::find(coordset_types.begin(),
                coordset_types.end(), cset_type) - coordset_types.begin();
            worst_coordset = std::max(worst_coordset, idx);
        }
        // Get topology type index
        {
            const std::string topo_type = n_topo->child("type").as_string();
            const index_t idx = std::find(topology_types.begin(),
                topology_types.end(), topo_type) - topology_types.begin();
            worst_topology = std::max(worst_topology, idx);
        }
    }

    std::string retval;
    if(worst_topology < 2 && worst_coordset < 1)
    {
        retval = "uniform";
    }
    else if(worst_topology < 3 && worst_coordset < 2)
    {
        retval = "rectilinear";
    }
    else if(worst_topology < 4)
    {
        retval = "structured";
    }
    else
    {
        retval = "unstructured";
    }
    return retval;
}

//-------------------------------------------------------------------------
static std::vector<std::pair<std::string, std::vector<const Node *>>>
group_coordsets(const std::vector<const Node *> inputs)
{
    // Group all the like-named coordsets
    using cset_group_t = std::pair<std::string, std::vector<const Node*>>;
    std::vector<cset_group_t> coordset_groups;
    const index_t ninputs = (index_t)inputs.size();
    for(index_t i = 0; i < ninputs; i++)
    {
        const Node *input = inputs[i];
        if(!input) { continue; }

        const Node *csets = input->fetch_ptr("coordsets");
        if(!csets) { continue; }

        const auto &cset_names = csets->child_names();
        const index_t ncset_names = (index_t)cset_names.size();
        for(index_t j = 0; j < ncset_names; j++)
        {
            const auto &cset_name = cset_names[j];
            const Node *cset = csets->fetch_ptr(cset_name);
            if(!cset) { continue; }

            auto itr = std::find_if(coordset_groups.begin(), coordset_groups.end(),
                [&cset_name](const cset_group_t &a)
            {
                return a.first == cset_name;
            });
            if(itr != coordset_groups.end())
            {
                const auto idx = itr - coordset_groups.begin();
                coordset_groups[idx].second.push_back(cset);
            }
            else // (itr == names.end())
            {
                coordset_groups.emplace_back();
                coordset_groups.back().first = cset_name;
                coordset_groups.back().second.push_back(cset);
            }
        }
    }
    return coordset_groups;
}

//-------------------------------------------------------------------------
static std::vector<std::pair<std::string, std::vector<const Node *>>>
group_topologies(const std::vector<const Node *> &inputs)
{
    using topo_group_t = std::pair<std::string, std::vector<const Node*>>;
    std::vector<topo_group_t> topo_groups;

    // Group all the like-named toplogies
    const index_t ninputs = (index_t)inputs.size();
    for(index_t i = 0; i < ninputs; i++)
    {
        const Node *input = inputs[i];
        if(!input) { continue; }

        const Node *topos = input->fetch_ptr("topologies");
        if(!topos) { continue; }

        const auto &topo_names = topos->child_names();
        const index_t ntopo_names = (index_t)topo_names.size();
        for(index_t j = 0; j < ntopo_names; j++)
        {
            const auto &topo_name = topo_names[j];
            const Node *topo = topos->fetch_ptr(topo_name);
            if(!topo) { continue; }

            auto itr = std::find_if(topo_groups.begin(), topo_groups.end(), [&](const topo_group_t &g) {
                return g.first == topo_name;
            });
            if(itr != topo_groups.end())
            {
                const auto idx = itr - topo_groups.begin();
                // Need to check if the topologies in this group have the same coordset
                const Node *group_cset = topo_groups[idx].second[0]->fetch_ptr("coordset");
                if(!group_cset) { continue; }
                const Node *this_cset = topo->fetch_ptr("coordset");
                if(!this_cset) { continue; }
                if(group_cset->as_string() != this_cset->as_string())
                {
                    // Error! Cannot merge two topologies that reference different named coordsets
                    continue;
                }
                topo_groups[idx].second.push_back(topo);
            }
            else // (itr == names.end())
            {
                const auto idx = topo_groups.size();
                topo_groups.emplace_back();
                topo_groups[idx].first = topo_name;
                topo_groups[idx].second.push_back(topo);
            }
        }
    }
    return topo_groups;
}

//-------------------------------------------------------------------------
void
Partitioner::combine(int domain,
    const std::vector<const Node *> &inputs,
    const std::vector<index_t> &chunk_ids,
    Node &output)
{
    // NOTE: Some decisions upstream, for the time being, make all the chunks
    //       unstructured. We will try to relax that so we might end up
    //       trying to combine multiple uniform,rectilinear,structured
    //       topologies.
#ifdef CONDUIT_DEBUG_PARTITIONER
    std::cout << "Combining domain " << domain << " size " << inputs.size() << std::endl;
    // std::cout << "INPUTS:";
    // for(const Node *in : inputs)
    // {
    //     in->schema().print();
    // }
#endif
    // Handle trivial cases
    output.reset();
    const auto sz = inputs.size();
    if(sz == 0)
    {
        CONDUIT_ERROR("Partitioner::combine called with 0 inputs, cannot combine 0 inputs.")
        return;
    }
    else if(sz == 1)
    {
        output = *inputs[0];
        output["state/domain_id"] = domain;
        return;
    }

    // Combine state - take state from inputs[0], overwrite domain_id
    // Q: Should this be more involved?
    if(inputs[0]->has_child("state"))
    {
        output["state"] = inputs[0]->child("state");
    }
    output["state/domain_id"] = domain;

    // Need to know which topology we are combining
    std::string main_topo_name = "";
    if(main_topo_name.empty())
    {
        main_topo_name = (*inputs[0])["topologies"][0].name();
    }

    // Determine the combine approach
    std::string rt(recommended_topology(inputs, main_topo_name));

    // Main mesh combine, topology + coordset
    using topo_group_t = std::pair<std::string, std::vector<const Node *>>;
    const auto topo_groups = group_topologies(inputs);
    const auto cset_groups = group_coordsets(inputs);
    {

        bool combined = false;
        if(rt != "unstructured")
        {
            combined = combine_as_structured(main_topo_name,
                rt, inputs, output);

            if(!combined)
            {
                CONDUIT_INFO("Unable to combine domains as " << rt << ", using unstructured.");
            }
        }

        if(!combined)
        {
            // Clear any data from a failed structured merge (if it happened)
            if(output.has_child("topologies"))
            {
                output.remove_child("topologies");
            }
            if(output.has_child("coordsets"))
            {
                output.remove_child("coordsets");
            }

            combine_as_unstructured(main_topo_name, topo_groups,
                cset_groups, output);
            combined = true;
        }
    }
    // std::cout << "Current output mesh:" << std::endl;
    // output.print();

    Node &output_topologies = output["topologies"];
    Node &output_coordsets  = output["coordsets"];

    // Handle material sets
    bool have_matsets = false;
    for(const Node *n : inputs)
    {
        if(n->has_child("matsets"))
        {
            have_matsets = true;
            break;
        }
    }
    if(have_matsets)
    {
        std::map<std::string, std::vector<Node>> matsets_to_combine;
        for(index_t i = 0; i < (index_t)inputs.size(); i++)
        {
            const Node *n = inputs[i];
            const Node *n_matsets = n->fetch_ptr("matsets");
            if(n_matsets)
            {
                auto itr = n_matsets->children();
                while(itr.has_next())
                {
                    const Node &n_matset = itr.next();
                    auto &matset_vec = matsets_to_combine[n_matset.name()];
                    matset_vec.emplace_back();
                    matset_vec.back().set_external(n_matset);
                    // Add domain id along to be used for mapping
                    matset_vec.back()["domain_id"].set((index_t)i);
                }
            }
        }

        Node &out_matsets = output["matsets"];
        for(const auto &pair : matsets_to_combine)
        {
            bool ok = !pair.second.empty();
            // Should never happen
            if(!ok)
            {
                CONDUIT_ERROR("matsets_to_combine conatins an empty entry!");
                continue;
            }

            // Verify that all the matsets reference the same topology
            const std::string mset_topo_name = pair.second[0].fetch("topology").as_string();
            for(const Node &n_matset : pair.second)
            {
                if(mset_topo_name != n_matset.fetch("topology").as_string())
                {
                    ok = false;
                    break;
                }
            }
            if(!ok)
            {
                CONDUIT_ERROR("Unable to combine matset " << conduit::utils::log::quote(pair.first)
                    << " because the input domains do not agree upon a topology.");
                continue;
            }

            // Fetch the referenced topology
            ok = output_topologies.has_child(mset_topo_name);
            if(!ok)
            {
                CONDUIT_ERROR("Unable to combine matset " << conduit::utils::log::quote(pair.first)
                    << " because it references an unknown topology " << conduit::utils::log::quote(mset_topo_name));
                continue;
            }

            // We will also need the input topo group to help map original elements
            auto topo_itr = std::find_if(topo_groups.begin(), topo_groups.end(),
                    [&mset_topo_name](const topo_group_t &g) {
                return g.first == mset_topo_name;
            });
            if(topo_itr == topo_groups.end())
            {
                CONDUIT_ERROR("Unable to combine matset " << conduit::utils::log::quote(pair.first)
                    << " because it references an unknown topology " << conduit::utils::log::quote(mset_topo_name));
                continue;
            }
            matset::combine(pair.second, topo_itr->second, output_topologies[mset_topo_name], out_matsets[pair.first]);
        }
    }


    // All inputs must have a fields object to merge fields
    bool have_fields = true;
    for(const Node *n : inputs)
    {
        if(!n->has_child("fields"))
        {
            have_fields = false;
            break;
        }
    }

    Node &output_fields = output["fields"];
    if(have_fields)
    {
        // Note: It should already be verified that they have a "fields" child
        using field_group_t = std::pair<std::string, std::vector<const Node*>>;
        std::vector<field_group_t> field_groups;
        for(const Node *n : inputs)
        {
            const Node &fields = n->child("fields");
            auto itr = fields.children();
            while(itr.has_next())
            {
                const Node &n_child = itr.next();
                auto field_groups_itr_pos = std::find_if(field_groups.begin(), field_groups.end(), [&](const field_group_t &g) {
                    return g.first == n_child.name();
                });
                if(field_groups_itr_pos != field_groups.end())
                {
                    field_groups_itr_pos->second.push_back(&n_child);
                }
                else
                {
                    field_groups.emplace_back();
                    field_groups.back().first = n_child.name();
                    field_groups.back().second.push_back(&n_child);
                }
            }
        }

        for(size_t fgi = 0; fgi < field_groups.size(); fgi++)
        {
            const auto &field_name = field_groups[fgi].first;
            const auto &field_group = field_groups[fgi].second;
            // Figure out field association and topology
            if(!field_group[0]->has_child("topology") || !field_group[0]->has_child("values"))
            {
                CONDUIT_WARN("Field " << field_name << " is not topology based, TODO: Implement material based field combinations.");
                continue;
            }
            const std::string &assoc_topo_name = field_group[0]->child("topology").as_string();

            // Make sure we have an output toplogy for the given name
            if(!output_topologies.has_child(assoc_topo_name))
            {
                CONDUIT_ERROR("Field " << field_name << " references " << assoc_topo_name
                    << " which doesn't exist.");
                continue;
            }
            Node &out_topo = output_topologies[assoc_topo_name];

            // Make sure there were as many input topologies as there are fields
            //  for this topology name
            auto topo_itr = std::find_if(topo_groups.begin(), topo_groups.end(),
                    [&assoc_topo_name](const topo_group_t &g) {
                return g.first == assoc_topo_name;
            });

            if(topo_itr->second.size() != field_group.size())
            {
                CONDUIT_INFO("Field " << field_name << " is not present on all input domains, skipping...");
                continue;
            }

            // Make sure we have an output coordset for the given name
            const std::string &assoc_cset_name = out_topo["coordset"].as_string();
            if(!output_coordsets.has_child(assoc_cset_name))
            {
                CONDUIT_ERROR("Topology " << assoc_topo_name << " references coordset "
                    << assoc_cset_name << " which doesn't exist. This error was found when building output fields.");
                continue;
            }
            Node &out_coordset = output_coordsets[assoc_cset_name];
            fields::combine(field_group, out_topo, out_coordset, output_fields[field_name]);
        }
    }


    bool have_adjsets = false;
    std::vector<const Node*> adjsets;
    for(const Node *n : inputs)
    {
        if (n->has_child("adjsets"))
        {
            adjsets.push_back(n->fetch_ptr("adjsets"));
            have_adjsets = true;
        }
        else
        {
            adjsets.push_back(nullptr);
        }
    }
    if (have_adjsets)
    {
        adjset::combine(adjsets, chunk_ids, output_topologies, output_coordsets, output["adjsets"]);
    }

    // Cleanup the output node, add original cells/verticies in needed
    if(!output_fields.has_child(original_element_ids_name()))
    {
        // Q: What happens in the case of multiple topologies?
        const Node &n_elem_map = output_topologies[0]["element_map"];
        Node &out_field = output_fields[original_element_ids_name()];
        out_field["topology"].set(output_topologies[0].name());
        // utils::ASSOCIATIONS[1]
        out_field["association"].set("element");
        Schema s;
        const DataType &dt = n_elem_map.dtype();
        const index_t half_sz = dt.number_of_elements() / 2;
        s["domains"].set(DataType(dt.id(), half_sz, 0,
            2*dt.element_bytes(), dt.element_bytes(), dt.endianness()));
        s["ids"].set(DataType(dt.id(), half_sz, 1*dt.element_bytes(),
            2*dt.element_bytes(), dt.element_bytes(), dt.endianness()));
        out_field["values"].set(s);

        std::memcpy(
            out_field["values/domains"].element_ptr(0),
            n_elem_map.element_ptr(0),
            dt.element_bytes()*dt.number_of_elements());

        std::vector<index_t> domain_map;
        bool has_domain_ids = false;
        for(index_t i = 0; i < (index_t)inputs.size(); i++)
        {
            if(inputs[i]->has_path("state/domain_id"))
            {
                const index_t did = (*inputs[i])["state/domain_id"].to_index_t();
                if(did != i)
                {
                    domain_map.push_back(did);
                    has_domain_ids = true;
                    continue;
                }
            }
            domain_map.push_back(i);
        }

        if(has_domain_ids)
        {
            DataArray<index_t> out_domains = out_field["values/domains"].value();
            for(index_t i = 0; i < out_domains.number_of_elements(); i++)
            {
                out_domains[i] = domain_map[out_domains[i]];
            }
        }

    }
    // Remove the element_maps from the output
    for(index_t i = 0; i < output_topologies.number_of_children(); i++)
    {
        output_topologies[i].remove("element_map");
    }

    if(!output_fields.has_child(original_vertex_ids_name()))
    {
        // Get the pointmaps
        const std::string &coordset_name = output_topologies[0]["coordset"].as_string();
        if(!output_coordsets.has_child(coordset_name))
        {
            CONDUIT_ERROR("Output topology 0 references coordset " << coordset_name << " which doesn't exist.");
            return;
        }
        const Node &n_pointmaps = output_coordsets[coordset_name]["pointmaps"];
        std::vector<DataArray<index_t>> pointmaps;
        for(index_t i = 0; i < n_pointmaps.number_of_children(); i++)
        {
            pointmaps.emplace_back(n_pointmaps[i].value());
        }

        Node &out_field = output_fields[original_vertex_ids_name()];
        out_field["topology"].set(output_topologies[0].name());
        // utils::ASSOCIATIONS[0]
        out_field["association"].set("vertex");

        if(!n_pointmaps.dtype().is_object())
        {
            const index_t coordset_sz = mesh::coordset::length(output_coordsets[coordset_name]);
            const DataType dt(pointmaps[0].dtype().id(), coordset_sz);
            out_field["values"]["domains"].set(dt);
            out_field["values"]["ids"].set(dt);
            DataArray<index_t> out_domains = out_field["values/domains"].value();
            DataArray<index_t> out_ids     = out_field["values/ids"].value();

            for(index_t pi = 0; pi < (index_t)pointmaps.size(); pi++)
            {
                index_t dom_id = pi;
                if(inputs[pi]->has_path("state/domain_id"))
                {
                    dom_id = (*inputs[pi])["state/domain_id"].to_index_t();
                }
                const auto &pmap = pointmaps[pi];
                for(index_t vi = 0; vi < pmap.number_of_elements(); vi++)
                {
                    const auto out_idx = pmap[vi];
                    out_domains[out_idx] = dom_id;
                    out_ids[out_idx]     = vi;
                }
            }
        }
        else
        {
            // Structured combine creates the pointmaps in a compatible format to be directly
            //  copied into the output field.
            if(!n_pointmaps.has_child("domains") || !n_pointmaps.has_child("ids"))
            {
                // So this should never happen
                CONDUIT_INFO("Unable to generate pointmap, the \"pointmaps\" child exists " <<
                    "on the output coordset but is not in the proper format to be export as a field.");
            }
            else
            {
                out_field["values"] = n_pointmaps;
            }
        }
    }
    // Remove the pointmaps from the output
    for(index_t i = 0; i < output_coordsets.number_of_children(); i++)
    {
        output_coordsets[i].remove("pointmaps");
    }
}

//-------------------------------------------------------------------------
void
Partitioner::map_back_fields(const conduit::Node& repart_mesh,
                             const conduit::Node& options,
                             Node& orig_mesh)
{
    using namespace std;
    auto repart_doms = mesh::domains(repart_mesh);
    auto orig_doms = mesh::domains(orig_mesh);
    // the below should also init dom_to_rank_map in mpi partitioner
    auto orig_dom_ids = get_global_domids(orig_mesh);

    // Get the name of the original_vertex_ids field.
    if(options.has_child("original_vertex_ids"))
        original_vertex_ids = options["original_vertex_ids"].as_string();

    // Get the name of the original_element_ids field.
    if(options.has_child("original_element_ids"))
        original_element_ids = options["original_element_ids"].as_string();

    unordered_map<index_t, Node*> gid_to_orig_dom;
    for (size_t idom = 0; idom < orig_doms.size(); idom++)
    {
        gid_to_orig_dom[orig_dom_ids[idom]] = orig_doms[idom];
    }

    vector<string> field_names;
    for (const Node& field : options["fields"].children())
    {
        field_names.emplace_back(field.as_string());
    }
    bool has_vert_fields = false;
    if (!repart_doms.empty())
    {
        const conduit::Node& dom = *repart_doms[0];
        // check domain fields from the first domain we encounter
        for (const Node& field : dom["fields"].children())
        {
            if (std::find(field_names.begin(),
                          field_names.end(),
                          field.name()) == field_names.end())
            {
                continue;
            }
            if (field["association"].as_string() == "vertex")
            {
                has_vert_fields = true;
            }
        }
    }

    // Get a field prefix. Some of the mapping fields used here may have
    // used a field_prefix when they were generated.
    std::string field_prefix;
    if(options.has_child("field_prefix"))
       field_prefix = options.fetch_existing("field_prefix").as_string();

    // map repart domid -> orig domids
    vector<vector<index_t>> map_tgt_domains(repart_doms.size());
    // map repart domid -> orig domid -> original elem/vertex ids
    vector<unordered_map<index_t, vector<index_t>>>
        map_tgt_elems(repart_doms.size()), map_tgt_verts(repart_doms.size());
    // map repart domid -> orig domid -> repart elem/vertex ids
    vector<unordered_map<index_t, vector<index_t>>>
        map_sel_elems(repart_doms.size()), map_sel_verts(repart_doms.size());
    for (size_t idom = 0; idom < repart_doms.size(); idom++)
    {
        const conduit::Node& dom = *repart_doms[idom];

        if (dom["fields"].has_child(original_element_ids_name()))
        {
            const Node& orig_v_node = dom["fields/" + original_element_ids_name() + "/values"];
            const index_t_accessor orig_elems_vals = orig_v_node["ids"].value();
            const index_t_accessor orig_doms_vals = orig_v_node["domains"].value();
            for (index_t ielem = 0; ielem < orig_elems_vals.number_of_elements(); ielem++)
            {
                map_tgt_elems[idom][orig_doms_vals[ielem]].push_back(orig_elems_vals[ielem]);
                map_sel_elems[idom][orig_doms_vals[ielem]].push_back(ielem);
            }
            for (const auto& slice_map : map_tgt_elems[idom])
            {
                // Keep a set of all source domains
                map_tgt_domains[idom].emplace_back(slice_map.first);
            }
        }
        else
        {
            CONDUIT_ERROR(
                "Map-back requires that mesh repartitioning is performed with "
                "the mapping option enabled. (missing " << original_element_ids_name() << ")");
        }
    }

    // TODO: this is a lot of bookkeeping to do if we aren't mapping back a
    // vertex-centered field. we should probably check if we have vertex fields
    // first.
    if (has_vert_fields)
    {
        std::string global_vertex_ids(field_prefix + "global_vertex_ids");

        // map of orig domid -> global vert ids
        map<index_t, vector<index_t>> orig_dom_gvids;
        for (const auto& dom_ent : gid_to_orig_dom)
        {
            index_t orig_idx = dom_ent.first;
            const Node& orig_dom = *dom_ent.second;
            const Node& orig_dom_fields = orig_dom.fetch_existing("fields");
            vector<index_t>& orig_gvids = orig_dom_gvids[orig_idx];
            if (orig_dom_fields.has_child(global_vertex_ids))
            {
                const index_t_accessor gvids = orig_dom_fields[global_vertex_ids + "/values"].value();
                orig_gvids.resize(gvids.number_of_elements());
                for (index_t ivert = 0; ivert < gvids.number_of_elements(); ivert++)
                {
                    orig_gvids[ivert] = gvids[ivert];
                }
            }
            else
            {
                CONDUIT_ERROR(
                    "Map-back of a vertex-centered field requires that a global "
                    "vertex numbering is generated. (missing global_vertex_ids)");
            }
        }

        // communicate domain gvids to ranks that need them
        synchronize_gvids(map_tgt_domains, orig_dom_gvids);

        for (index_t repart_idx = 0; repart_idx < static_cast<index_t>(repart_doms.size()); repart_idx++)
        {
            const Node& dom = *repart_doms[repart_idx];
            const Node& dom_fields = dom.fetch_existing("fields");

            std::unordered_map<index_t, index_t> gvid_to_repart_vid;
            if (dom_fields.has_child(global_vertex_ids))
            {
                const Node& global_vid_node = dom_fields[global_vertex_ids + "/values"];
                const index_t_accessor gvids = global_vid_node.value();
                for (index_t ivert = 0; ivert < gvids.number_of_elements(); ivert++)
                {
                    gvid_to_repart_vid[gvids[ivert]] = ivert;
                }
            }
            for (const index_t orig_idx : map_tgt_domains[repart_idx])
            {
                if (orig_dom_gvids.count(orig_idx) == 0)
                {
                    CONDUIT_ERROR(
                        conduit_fmt::format("No vertex gid data (rank: {}, domid: {})",
                                            rank, orig_idx));
                }
                // Get current slice of orig dom
                auto& orig_gvids = orig_dom_gvids[orig_idx];
                // Loop over all vertices in the original domain
                for (size_t ivert = 0; ivert < orig_gvids.size(); ivert++)
                {
                    index_t global_vid = orig_gvids[ivert];
                    if (gvid_to_repart_vid.count(global_vid) != 0)
                    {
                        index_t repart_vid = gvid_to_repart_vid.find(global_vid)->second;
                        map_tgt_verts[repart_idx][orig_idx].push_back(ivert);
                        map_sel_verts[repart_idx][orig_idx].push_back(repart_vid);
                    }
                }
            }
        }
    }

    // map of original domid -> sliced data
    unordered_map<index_t, Node> packed_fields;
    // schema of each node
    // [remap_dom0]:
    //  - vert_map: [orig_vert_ids0]
    //  - elem_map: [orig_elem_ids0]
    //  - fields:
    //    - field0: [sliced field]
    //    - field1: [sliced field]
    //    ...
    // [remap_dom1]:
    //  - vert_map: [orig_vert_ids1]
    //  - elem_map: [orig_elem_ids1]
    //  - fields:
    //     - field0: [sliced field]
    //     - field1: [sliced field]
    //     ...
    for (size_t idom = 0; idom < repart_doms.size(); idom++)
    {
        const Node& dom = *repart_doms[idom];
        for (const index_t tgt_dom : map_tgt_domains[idom])
        {
            Node& remap_dom_ent = packed_fields[tgt_dom].append();
            // Copy field with source element indices
            const vector<index_t>& sel_elems = map_sel_elems[idom][tgt_dom];
            const vector<index_t>& sel_verts = map_sel_verts[idom][tgt_dom];
            for (const std::string& field_name : field_names)
            {
                const Node& field = dom["fields"][field_name];
                const std::string& assoc = field["association"].as_string();
                if (assoc == "vertex")
                {
                    copy_field(field, sel_verts, remap_dom_ent["fields"]);
                }
                else if (assoc == "element")
                {
                    copy_field(field, sel_elems, remap_dom_ent["fields"]);
                }
                else
                {
                    CONDUIT_ERROR(
                        conduit_fmt::format("Encountered field with unsupported association."
                                            " (field: {} association: {}", field_name, assoc));
                }
            }
            // Add target element indices to the node
            remap_dom_ent["elem_map"].set(map_tgt_elems[idom][tgt_dom]);
            remap_dom_ent["vert_map"].set(map_tgt_verts[idom][tgt_dom]);
        }
    }

    // If we're multi-process, redistribute target field chunks to original
    // domain homes
    communicate_mapback(packed_fields);
    for (const auto& orig_dom : packed_fields)
    {
        // Precompute final element count
        index_t nelems = 0;
        for (const Node& src_chunk : orig_dom.second.children())
        {
            DataArray<index_t> cnk_map = src_chunk["elem_map"].value();
            nelems += cnk_map.number_of_elements();
        }

        int cnk_id = 0;
        vector<index_t> tgt_elem_map(nelems * 2);
        vector<DataArray<index_t>> tgt_vert_maps(orig_dom.second.number_of_children());
        unordered_map<string, vector<const Node*>> field_groups;
        for (const Node& src_chunk : orig_dom.second.children())
        {
            // Group common field nodes together by name
            for (const Node& field : src_chunk["fields"].children())
            {
                field_groups[field.name()].push_back(&field);
            }
            // Build element map
            DataArray<index_t> cnk_map = src_chunk["elem_map"].value();
            for (index_t i = 0; i < cnk_map.number_of_elements(); i++)
            {
                index_t out_idx = cnk_map[i];
                tgt_elem_map[2*out_idx] = cnk_id;
                tgt_elem_map[2*out_idx + 1] = i;
            }
            // map_vertex_field expects a collection of vertex maps for each
            // chunk, instead of a single unified map
            tgt_vert_maps[cnk_id] = src_chunk["vert_map"].value();
            cnk_id++;
        }
        // map_element_field expects a DataArray
        DataArray<index_t> elem_map_arr;
        {
            Node tmp;
            tmp.set_external(tgt_elem_map);
            elem_map_arr = tmp.value();
        }
        Node& tgt_dom = *gid_to_orig_dom[orig_dom.first];
        for (const auto& fg : field_groups)
        {
            const vector<const Node*>& src_fields = fg.second;
            const string& field_name = fg.first;

            const string& assoc = src_fields[0]->child("association").as_string();
            const string& assoc_topo = src_fields[0]->child("topology").as_string();

            Node& output = tgt_dom["fields"][field_name];
            output["association"] = assoc;
            output["topology"] = assoc_topo;
            if (assoc == "vertex")
            {
                const std::string& assoc_cset = tgt_dom["topologies"][assoc_topo]["coordset"].as_string();
                const Node& assoc_coordset = tgt_dom["coordsets"][assoc_cset];
                index_t nverts = coordset::length(assoc_coordset);
                fields::map_vertex_field(src_fields, tgt_vert_maps, nverts, output["values"]);
            }
            else if (assoc == "element")
            {
                fields::map_element_field(src_fields, elem_map_arr, output["values"]);
            }
        }
    }
}

//-------------------------------------------------------------------------
bool
Partitioner::combine_as_structured(const std::string &topo_name,
    const std::string &rt,
    const std::vector<const Node *> &n_inputs,
    Node &output)
{
    mesh::utils::combine_implicit_topologies combine;
    return combine.execute(topo_name, rt, n_inputs, original_element_ids_name(), original_vertex_ids_name(), output);
}

//-------------------------------------------------------------------------
void
Partitioner::combine_as_unstructured(const std::string &topo_name,
    const std::vector<std::pair<std::string, std::vector<const Node*>>> &topo_groups,
    const std::vector<std::pair<std::string, std::vector<const Node*>>> &coordset_groups,
    Node &output)
{
    using pair_t = std::pair<std::string, std::vector<const Node*>>;
    const auto tg_itr = std::find_if(topo_groups.begin(), topo_groups.end(),
        [&topo_name](const pair_t &p)
        {
            return p.first == topo_name;
        });

    if(tg_itr == topo_groups.end())
    {
        CONDUIT_ERROR("Invalid topo_name");
        return;
    }

    const std::string cset_name = tg_itr->second[0]->fetch_existing("coordset").as_string();
    const auto cg_itr = std::find_if(coordset_groups.begin(), coordset_groups.end(),
        [&cset_name](const pair_t &p)
        {
            return p.first == cset_name;
        });

    if(cg_itr == coordset_groups.end())
    {
        CONDUIT_ERROR("Unable to find coordset associated with given topology");
        return;
    }

    // Determine names of all coordsets
    Node &output_coordsets = output.add_child("coordsets");
    {
        coordset::point_merge pm;
        pm.execute(cg_itr->second, merge_tolerance, output_coordsets.add_child(cset_name));
    }
    const Node &pointmaps = output_coordsets[cset_name]["pointmaps"];

    // Iterate over all topology names and combine like-named topologies
    // as new unstructured topology.
    Node &output_topologies = output.add_child("topologies");
    {
        const auto &topologies = tg_itr->second;
        std::vector<const Node*> working_topologies;
        std::vector<Node> temporary_nodes;
        temporary_nodes.reserve(topologies.size());

        // Validate / translate inputs
        const index_t ntopos = topologies.size();
        for(index_t i = 0; i < ntopos; i++)
        {
            const Node *topo = topologies[i];
            Node temp;
            if(!topo) { continue; }

            const Node *type = topo->fetch_ptr("type");
            if(!type) { continue; }

            const Node *cset = topo->fetch_ptr("coordset");
            if(!cset) { continue; }

            const std::string &t = type->as_string();
            if(t == "points")
            {
                temporary_nodes.emplace_back();
                // topology::points::to_explicit(topo, temporary_nodes.back());
                working_topologies.push_back(&temporary_nodes.back());
            }
            else if(t == "uniform")
            {
                temporary_nodes.emplace_back();
                topology::uniform::to_unstructured(*topo, temporary_nodes.back(), temp);
                working_topologies.push_back(&temporary_nodes.back());
            }
            else if(t == "rectilinear")
            {
                temporary_nodes.emplace_back();
                topology::rectilinear::to_unstructured(*topo, temporary_nodes.back(), temp);
                working_topologies.push_back(&temporary_nodes.back());
            }
            else if(t == "structured")
            {
                temporary_nodes.emplace_back();
                topology::structured::to_unstructured(*topo, temporary_nodes.back(), temp);
                working_topologies.push_back(&temporary_nodes.back());
            }
            else if(t == "unstructured")
            {
                working_topologies.push_back(topo);
            }
            else
            {
                //  ERROR!
                continue;
            }
        }

        // Make sure we have the correct number of point maps
        if(pointmaps.number_of_children() != (index_t)working_topologies.size())
        {
            CONDUIT_ERROR("Number of input pointmaps and number of input topologies do not match! "
                << pointmaps.number_of_children() << " != " << working_topologies.size());
            return;
        }

        // Start building the output
        Node &out_topo = output_topologies.add_child(topo_name);
        if(working_topologies.size())
        {
            bool do_polyhedral = false;
            bool do_polygonal  = false;
            index_t dim = 0;
            for(const Node *topo : topologies)
            {
                // TODO: Write a dim function that knows about all the different topologies
                const Node *shape = topo->fetch_ptr("elements/shape");
                if(!shape) { continue; }
                const utils::ShapeType s(shape->as_string());
                if(s.is_polyhedral())
                {
                    do_polyhedral = true;
                }
                else if(s.is_polygonal())
                {
                    do_polygonal = true;
                }
                dim = std::max(dim, s.dim);
                if(do_polyhedral) break;
            }

            if(do_polyhedral || (do_polygonal && dim > 2))
            {
                mesh::topology::build_polyhedral_output(working_topologies, pointmaps, cset_name, out_topo);
            }
            else if(do_polygonal)
            {
                mesh::topology::build_polygonal_output(working_topologies, pointmaps, cset_name, out_topo);
            }
            else // Single shape & multi shape topologies
            {
                mesh::topology::build_unstructured_output(working_topologies, pointmaps, cset_name, out_topo);
            }
        }
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
