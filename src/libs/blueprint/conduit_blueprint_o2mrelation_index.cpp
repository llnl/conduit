// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_o2mrelation_index.cpp
///
//-----------------------------------------------------------------------------
#include <sstream>
#include <set>

#include "conduit_blueprint_o2mrelation.hpp"
#include "conduit_blueprint_o2mrelation_index.hpp"

#include "conduit_error.hpp"
#include "conduit_utils.hpp"

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
// -- begin conduit::blueprint::o2mrelation --
//-----------------------------------------------------------------------------
namespace o2mrelation
{

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// O2MIndex
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// O2MIndex Construction and Destruction
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
O2MIndex::O2MIndex()
{
// empty //
}

//---------------------------------------------------------------------------//
O2MIndex::O2MIndex(const Node *node)
{
    if (! node->has_child("sizes") &&
        ! node->has_child("offsets") &&
        ! node->has_child("indices"))
    {
        std::set<index_t> candidate_sizes;
        auto nodeIter = node->children();
        while (nodeIter.has_next())
        {
            const Node &child = nodeIter.next();
            if (child.dtype().is_number())
            {
                candidate_sizes.insert(child.dtype().number_of_elements());
            }
        }
        if (candidate_sizes.size() == 1)
        {
            num_ones = *candidate_sizes.begin();
        }
        else
        {
            // we couldn't compute a size
            CONDUIT_ERROR("O2MIndex::O2MIndex(): failed to compute the number "
                          "of ones in the o2m relation. o2m relation is ambiguous "
                          "or poorly specified.");
        }
    }
    else
    {
        if (node->has_child("sizes"))
        {
            m_sizes_acc = (*node)["sizes"].as_index_t_accessor();
        }
        if(node->has_child("indices"))
        {
            m_indices_acc = (*node)["indices"].as_index_t_accessor();
        }
        if(node->has_child("offsets"))
        {
            m_offsets_acc = (*node)["offsets"].as_index_t_accessor();
        }

        if(m_offsets_acc.number_of_elements() > 0)
        {
            num_ones = m_offsets_acc.number_of_elements();
        }
        else if (m_indices_acc.number_of_elements() > 0)
        {
            num_ones = m_indices_acc.number_of_elements();
        }
    }
}

//---------------------------------------------------------------------------//
O2MIndex::O2MIndex(const Node &node)
: O2MIndex(&node)
{ }

//---------------------------------------------------------------------------//
O2MIndex::O2MIndex(const O2MIndex &itr)
: m_sizes_acc(itr.m_sizes_acc),
  m_indices_acc(itr.m_indices_acc),
  m_offsets_acc(itr.m_offsets_acc)
{ }

//---------------------------------------------------------------------------//
O2MIndex &
O2MIndex::operator=(const O2MIndex &itr)
{
    if(this != &itr)
    {
        m_sizes_acc = itr.m_sizes_acc;
        m_indices_acc = itr.m_indices_acc;
        m_offsets_acc = itr.m_offsets_acc;
    }
    return *this;
}

//-----------------------------------------------------------------------------
/// Human readable info about this iterator
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
void
copy_index_t_acc_to_node(const index_t_accessor& acc, Node& res, const char * label)
{
    if (acc.number_of_elements() > 0)
    {
        index_t count = acc.number_of_elements();
        res[label].set(DataType::index_t(count));
        index_t* p_output = res[label].as_index_t_ptr();
        for (index_t i = 0; i < count; ++i)
        {
            p_output[i] = acc[i];
        }
    }
}
void
O2MIndex::info(Node &res) const
{
    res.reset();

    copy_index_t_acc_to_node(m_sizes_acc, res, "sizes");
    copy_index_t_acc_to_node(m_indices_acc, res, "indices");
    copy_index_t_acc_to_node(m_offsets_acc, res, "offsets");
}

//-----------------------------------------------------------------------------
/// Get info on the "ones" of this index.
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
index_t
O2MIndex::index(index_t one_index, index_t many_index) const
{
    index_t offset = one_index;
    if(m_offsets_acc.number_of_elements() > 0)
    {
        offset = m_offsets_acc[one_index];
    }

    index_t index = offset;
    if(m_indices_acc.number_of_elements() > 0)
    {
        index = m_indices_acc[offset + many_index];
    }
    else
    {
        index += many_index;
    }

    return index;
}


//---------------------------------------------------------------------------//
index_t
O2MIndex::size(index_t one_index) const
{
    index_t nelements = 0;

    if(one_index == -1)
    {
        return num_ones;
    }
    else
    {
        if(m_sizes_acc.number_of_elements() > 0)
        {
            nelements = m_sizes_acc[one_index];
        }
        else
        {
            nelements = 1;
        }
    }

    return nelements;
}

//---------------------------------------------------------------------------//
index_t
O2MIndex::offset(index_t one_index) const
{
    index_t offset = one_index;
    if (m_offsets_acc.number_of_elements() > 0)
    {
        offset = m_offsets_acc[one_index];
    }
    return offset;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// End O2MIndex
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::o2mrelation --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


