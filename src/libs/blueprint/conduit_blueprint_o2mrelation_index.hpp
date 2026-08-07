// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_o2mrelation_index.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_O2MRELATION_INDEX_HPP
#define CONDUIT_BLUEPRINT_O2MRELATION_INDEX_HPP

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_exports.h"


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
// -- begin conduit::blueprint::o2mrelation::O2Mindex --
//-----------------------------------------------------------------------------
///
/// class: conduit::blueprint::o2mrelation::O2MIndex
///
/// description:
///  General purpose index for 'o2mrelation' Nodes.
///
//-----------------------------------------------------------------------------
class CONDUIT_BLUEPRINT_API O2MIndex
{
public:
//-----------------------------------------------------------------------------
//
// -- conduit::blueprint::o2mrelation::O2MIndex public members --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// O2MIndex Construction and Destruction
//-----------------------------------------------------------------------------
    /// Default constructor.
    O2MIndex();

    /// Copy constructor.
    O2MIndex(const O2MIndex &idx);

    /// Primary index constructor.
    O2MIndex(const Node *node);

    /// Primary index constructor.
    /// this will use the pointer to the passed Node ref.
    O2MIndex(const Node &node);

    /// Destructor
    ~O2MIndex() { };

    /// Assignment operator.
    O2MIndex &operator=(const O2MIndex &itr);

//-----------------------------------------------------------------------------
/// Retrieve a flat-index.
//-----------------------------------------------------------------------------
    index_t     index(index_t one_index, index_t many_index) const;

//-----------------------------------------------------------------------------
/// Get info on the "ones" of this index.
///
/// size() returns the number of "ones".  size(i) returns the number of "many"
/// associated with item i.  offset(i) returns the position in the data array
/// where the data for item i starts.
//-----------------------------------------------------------------------------
    index_t     size(index_t one_index = -1) const;
    index_t     offset(index_t one_index) const;

//-----------------------------------------------------------------------------
/// Human readable info about this iterator
//-----------------------------------------------------------------------------
    void        info(Node &res) const;

private:

//-----------------------------------------------------------------------------
//
// -- conduit::blueprint::o2mrelation::O2MIndex private members --
//
//-----------------------------------------------------------------------------

    // we precompute the number of ones so we can quickly return it later
    index_t m_num_ones;

//-----------------------------------------------------------------------------
/// Index state/fields.
//-----------------------------------------------------------------------------

    /// Accessors for sizes, indices, and offsets
    index_t_accessor m_sizes_acc;
    index_t_accessor m_indices_acc;
    index_t_accessor m_offsets_acc;
};
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::o2mrelation::O2MIndex --
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


#endif
