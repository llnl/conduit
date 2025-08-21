// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_kdtree.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_KDTREE_HPP
#define CONDUIT_BLUEPRINT_MESH_KDTREE_HPP

//-----------------------------------------------------------------------------
// std includes
//-----------------------------------------------------------------------------
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"

// This macro enables some kdtree debugging information.
// #define CONDUIT_DEBUG_KDTREE

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

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils --
//-----------------------------------------------------------------------------
namespace utils
{

//---------------------------------------------------------------------------
/**
 @brief This class lets us look up points in a kdtree to accelerate lookups.

 @tparam Indexable The container that contains elements of type T. This can be
                   raw pointers, a vector, a data_array, data_accessor, etc.
 @tparam CoordinateType The storage type of the coordinates.
 @tparam NDIMS The number of dimensions in a coordinate.
 */
template <typename Indexable, typename CoordinateType, int NDIMS>
class kdtree
{
public:
    static const conduit::index_t NoChild;
    static const conduit::index_t NotFound;
    static const CoordinateType   DEFAULT_POINT_TOLERANCE;

    using IndexableType = Indexable;
    using BoxType = CoordinateType[NDIMS][2];
    using PointType = CoordinateType[NDIMS];

    /// Constructor
    kdtree();

    /**
     @brief Set the point tolerance, a distance under which points are the same.
     @param tolerance The point tolerance distance.
     */
    void setPointTolerance(CoordinateType tolerance);

    /**
     @brief Initialize the tree so it can look up points.
     @param c An array of indexable objects that contain coordinate scalars (arrays, etc.)
     @param len The number of coordinates in each array.
     */
    void initialize(Indexable c[NDIMS], size_t len);

    /**
     @brief Find a point in the tree if it exists.

     @param pt The point to locate.

     @return The point id of the point or NotFound if the point was not found.
     */
    conduit::index_t findPoint(const PointType &pt) const;

    /**
     @brief Return the number of dimensions.
     @return The number of dimensions.
     */
    inline int dims() const { return NDIMS; }

    /**
     @brief Return the sorted index order.
     @return A vector sorted spatially. The elements contain the original indices.
     */
    std::vector<conduit::index_t> &getIndices() { return index; }

    /**
     @brief Return the sorted index order.
     @return A vector sorted spatially. The elements contain the original indices.
     */
    const std::vector<conduit::index_t> &getIndices() const { return index; }

    /**
     @brief Print the tree to a stream.
     @param os A stream to use for printing.
     */
    void print(std::ostream &os) const;
private:
    /// Represents ranges of the index vector.
    struct RangeType
    {
        conduit::index_t offset;
        conduit::index_t size;
    };

    /// Represents a box in the tree hierarchy.
    struct BoxInfo
    {
        conduit::index_t childOffset; // Offset into boxes array where this box's 2 children start.
                                      // -1 if there are no children.
        int splitDimension; // The dimension along which this box is split.
        RangeType range;    // The slice of the index array that belongs to this box.
#ifdef CONDUIT_DEBUG_KDTREE
        BoxType   box;
#endif
    };

    // Internal helpers.
    void cutBox(const BoxType &input, int dimension, BoxType &A, BoxType &B) const;
    void cutRange(const RangeType &input, int dimension, CoordinateType maxValue, RangeType &A, RangeType &B) const;
    void calculateExtents();
    void construct();
    void constructBox(conduit::index_t bp, const RangeType &range, const BoxType &b, int level, int maxlevel);

    int longest(const BoxType &b) const;
    void sortIndexRange(const RangeType &range, int dimension);
    bool pointInBox(const PointType &pt, const BoxType &b) const;
    bool pointEqual(const PointType &pt, conduit::index_t index) const;
    void printBox(std::ostream &os, const BoxType &b, const std::string &indent) const;
private:
    std::vector<BoxInfo> boxes;          //!< Describes boxes
    std::vector<conduit::index_t> index; //!< Contains sorted coordinate indices
    BoxType box;                         //!< Overall bounding box
    Indexable coords[NDIMS];             //!< Coordinate arrays
    size_t coordlen;                     //!< Length of a coordinate array.
    CoordinateType pointTolerance;       //!< Distance to a point before it is same.
};

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
std::ostream &
operator << (std::ostream &os, const kdtree<Indexable, CoordinateType, NDIMS> &obj)
{
    obj.print(os);
    return os;
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
const conduit::index_t kdtree<Indexable, CoordinateType, NDIMS>::NoChild = -1;

template <typename Indexable, typename CoordinateType, int NDIMS>
const conduit::index_t kdtree<Indexable, CoordinateType, NDIMS>::NotFound = -1;

template <typename Indexable, typename CoordinateType, int NDIMS>
const CoordinateType kdtree<Indexable, CoordinateType, NDIMS>::DEFAULT_POINT_TOLERANCE = static_cast<CoordinateType>(1.e-9);

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
kdtree<Indexable, CoordinateType, NDIMS>::kdtree() : boxes(), index()
{
    for(int i = 0; i < dims(); i++)
    {
        box[i][0] = CoordinateType{};
        box[i][1] = CoordinateType{};
        coords[i] = Indexable{};
    }
    coordlen = 0;
    setPointTolerance(DEFAULT_POINT_TOLERANCE);
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::setPointTolerance(CoordinateType tolerance)
{
    // Prevent use of tolerance values that are below the type's epsilon
    pointTolerance = std::max(tolerance, std::numeric_limits<CoordinateType>::epsilon());
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::initialize(Indexable c[NDIMS], size_t len)
{
    boxes.clear();
    index.clear();

    for(int i = 0; i < dims(); i++)
    {
        coords[i] = c[i];
    }
    coordlen = len;
    calculateExtents();
    construct();
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::printBox(std::ostream &os,
    const kdtree<Indexable, CoordinateType, NDIMS>::BoxType &b, const std::string &indent) const
{
    os << indent << "box: [";
    for(int i = 0; i < dims(); i++)
    {
        if(i > 0) os << ", ";
        os << b[i][0] << ", " << b[i][1];
    }
    os << "]";
}

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::utils::detail --
//-----------------------------------------------------------------------------
namespace detail
{
// Helpers for printing.
template <typename T>
const void *get_pointer(const T *obj) { return static_cast<const void *>(obj); }

template <typename T>
const void *get_pointer(const conduit::DataArray<T> &obj) { return static_cast<const void *>(obj.element_ptr(0)); }

template <typename T>
const void *get_pointer(const conduit::DataAccessor<T> &obj) { return static_cast<const void *>(obj.element_ptr(0)); }
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils::detail --
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::print(std::ostream &os) const
{
    os << "boxesSize: " << boxes.size() << std::endl;
    os << "boxes:" << std::endl;
    for(size_t i = 0; i < boxes.size(); i++)
    {
        os << "  -" << std::endl;
        os << "    boxIndex: " << i << std::endl;
        os << "    childOffset: " << boxes[i].childOffset << std::endl;
        os << "    splitDimension: " << boxes[i].splitDimension << std::endl;
        os << "    range:" << std::endl;
        os << "      offset: " << boxes[i].range.offset << std::endl;
        os << "      size: " << boxes[i].range.size << std::endl;
#ifdef CONDUIT_DEBUG_KDTREE
        printBox(os, boxes[i].box, "    ");
        os << std::endl;
#endif
    }
    os << std::endl;

    os << "indexSize: " << index.size() << std::endl;
    os << "index: [";
    for(size_t i = 0; i < index.size(); i++)
    {
        if(i > 0) os << ", ";
        os << index[i];
    }
    os << "]" << std::endl;

    printBox(os, box, "");
    os << std::endl;

    os << "coords: [";
    for(int i = 0; i < dims(); i++)
    {
        if(i > 0) os << ", ";
        os << detail::get_pointer(coords[i]);
    }
    os << "]" << std::endl;

    os << "coordlen: " << coordlen << std::endl;
    os << "pointTolerance: " << pointTolerance << std::endl;
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::cutBox(const BoxType &input, int dimension,
    BoxType &A, BoxType &B) const
{
    // NOTE: This just does uniform splitting for now. It could sample the
    //       coordinates to get an idea of their center and store a t value
    //       to lerp the dimension min/max. This would help with long dimensions
    //       where points are clustered unevenly.
    constexpr CoordinateType two = 2.;
    memcpy(A, input, sizeof(BoxType));
    memcpy(B, input, sizeof(BoxType));
    CoordinateType mid = (input[dimension][0] + input[dimension][1]) / two;
    A[dimension][1] = mid;
    B[dimension][0] = mid;
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::cutRange(const RangeType &input, int dimension,
    CoordinateType maxValue, RangeType &A, RangeType &B) const
{
    // We can't just partition the range in half because it might mean leaving
    // coordinate data with the same value on either side of the split.
    // That doesn't work spatially. Instead, partition the range such that A
    // contains all indices less than maxValue and B contains the rest.
    conduit::index_t sizeA = 0;
    for(conduit::index_t idx = 0; idx < input.size; idx++)
    {
        if(coords[dimension][index[input.offset + idx]] < maxValue)
        {
            sizeA++;
        }
        else
        {
            break;
        }
    }
    conduit::index_t sizeB = input.size - sizeA;
    A.offset = input.offset;
    A.size = sizeA;

    B.offset = input.offset + sizeA;
    B.size = sizeB;

#ifdef CONDUIT_DEBUG_KDTREE
    // Print the data in each range.
    std::cout << "cutRange: input=(" << input.offset
              << ", " << input.size
              << "), dimension=" << dimension
              << ", maxValue=" << maxValue
              << std::endl;
    std::cout << "\tA={";
    for(conduit::index_t idx = 0; idx < A.size; idx++)
    {
        if(idx > 0) std::cout << ", ";
        std::cout << coords[dimension][index[A.offset + idx]];
    }
    std::cout << "}" << std::endl;
    std::cout << "\tB={";
    for(conduit::index_t idx = 0; idx < B.size; idx++)
    {
        if(idx > 0) std::cout << ", ";
        std::cout << coords[dimension][index[B.offset + idx]];
    }
    std::cout << "}" << std::endl;
#endif
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
conduit::index_t
kdtree<Indexable, CoordinateType, NDIMS>::findPoint(const kdtree<Indexable, CoordinateType, NDIMS>::PointType &pt) const
{
    conduit::index_t foundIndex = NotFound;
#ifdef CONDUIT_DEBUG_KDTREE
    std::cout << "findPoint(";
    for(int i = 0; i < dims(); i++)
        std::cout << pt[i] << ", ";
    std::cout << ")" << std::endl;
    printBox(std::cout, box, "    ");
    std::cout << std::endl;
#endif
    // Check whether the point is in the global bounding box.
    if(pointInBox(pt, box))
    {
        // Iterate until we find a box that contains the point.
        BoxType currentBox;
        memcpy(currentBox, box, sizeof(BoxType));
        conduit::index_t bp = 0, prevbp = 0;
        while(boxes[bp].childOffset != NoChild)
        {
            // Cut the box to make 2 child boxes.
            BoxType A, B;
            cutBox(currentBox, boxes[bp].splitDimension, A, B);

            // Determine which child box contains the point and set bp to that box.
            prevbp = bp;
            if(pointInBox(pt, A))
            {
#ifdef CONDUIT_DEBUG_KDTREE
                std::cout << "    Point in A "; printBox(std::cout, A, "");
                std::cout << " goes to box " << boxes[bp].childOffset << std::endl;
#endif
                memcpy(currentBox, A, sizeof(BoxType));
                bp = boxes[bp].childOffset;
            }
            else
            {
#ifdef CONDUIT_DEBUG_KDTREE
                std::cout << "    Point in B "; printBox(std::cout, B, "");
                std::cout << " goes to box " << (boxes[bp].childOffset+1) << std::endl;
#endif
                memcpy(currentBox, B, sizeof(BoxType));
                bp = boxes[bp].childOffset + 1;
            }
        }
#ifdef CONDUIT_DEBUG_KDTREE
        std::cout << "    Search box: bp=" << prevbp << std::endl;
#endif
        // Check prevbp box for the points of interest.
        conduit::index_t offset = boxes[prevbp].range.offset;
        conduit::index_t size = boxes[prevbp].range.size;
        for(conduit::index_t i = 0; i < size && (foundIndex == NotFound); i++)
        {
            // This the index of the point we want to look at.
            conduit::index_t ptIdx = index[offset + i];

            // See if the point is "equal".
            if(pointEqual(pt, ptIdx))
            {
                foundIndex = ptIdx;
            }
        }
#ifdef CONDUIT_DEBUG_KDTREE
        std::cout << "    Point index: " << foundIndex << std::endl;
#endif

    }
#ifdef CONDUIT_DEBUG_KDTREE
    else
    {
        std::cout << "Point not in box!" << std::endl;
    }
#endif
    return foundIndex;
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
bool
kdtree<Indexable, CoordinateType, NDIMS>::pointEqual(const kdtree<Indexable, CoordinateType, NDIMS>::PointType &pt, conduit::index_t ptIdx) const
{
    bool eq = true;
    for(int i = 0; i < dims(); i++)
    {
        CoordinateType delta = pt[i] - coords[i][ptIdx];
        eq &= (std::fabs(delta) < pointTolerance);
    }
    return eq;
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::calculateExtents()
{
    for(int i = 0; i < dims(); i++)
    {
        box[i][0] = std::numeric_limits<CoordinateType>::max();
        box[i][1] = std::numeric_limits<CoordinateType>::lowest();
        for(size_t j = 0; j < coordlen; j++)
        {
            box[i][0] = std::min(box[i][0], coords[i][j]);
            box[i][1] = std::max(box[i][1], coords[i][j]);
        }
    }

    // Expand the box a little. Note that we have a minimum expansion to account
    // for when a dimension has all the same values. Expand the box with a little
    // asymmetrically so if we're given data that is symmetric, we do not end up
    // splitting right at 0.
    constexpr CoordinateType s = static_cast<CoordinateType>(200.);
    const CoordinateType minExpansion = DEFAULT_POINT_TOLERANCE * s;
    for(int i = 0; i < dims(); i++)
    {
        CoordinateType side = std::max(box[i][1] - box[i][0], minExpansion);
        CoordinateType d = side / s;
        box[i][0] -= d;
        box[i][1] += static_cast<CoordinateType>(1.1) * d;
    }
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::construct()
{
    // Fill the index with 0,1,2,3,...
    index.resize(coordlen);
    std::iota(index.begin(), index.end(), 0);

    // Figure out a number of levels that we want to use. We take the floor so
    // the leaf nodes may have multiple elements.
    int maxLevels = static_cast<int>(std::floor(std::log((double)coordlen) / std::log((double)2.)));
    // This is the summation of all of the levels
    int nBoxes = static_cast<int>(std::pow(2., maxLevels + 1) - 1);

    boxes.resize(nBoxes);

    // Fill in the childOffset values. Make them all be odd numbers for starters.
    // When we get down to the point that there is 1 point in a box, or max level
    // has been reached, we'll later add NoChild. Probably the second half of boxes
    // will contain NoChild.
    for(int i = 0; i < nBoxes; i++)
        boxes[i].childOffset = 2 * i + 1;

    // Build the box for level 0.
    RangeType range;
    range.offset = 0;
    range.size = static_cast<conduit::index_t>(coordlen);
    constructBox(0, range, box, 0, maxLevels);
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::constructBox(conduit::index_t bp,
    const kdtree<Indexable, CoordinateType, NDIMS>::RangeType &range,
    const kdtree<Indexable, CoordinateType, NDIMS>::BoxType &b,
    int level,
    int maxLevels)
{
#ifdef CONDUIT_DEBUG_KDTREE
    memcpy(boxes[bp].box, b, sizeof(BoxType));
#endif
    if(range.size > 0)
    {
        boxes[bp].splitDimension = longest(b);
        boxes[bp].range = range;
        sortIndexRange(boxes[bp].range, boxes[bp].splitDimension);

        BoxType A, B;
        cutBox(b, boxes[bp].splitDimension, A, B);
        CoordinateType maxValue = A[boxes[bp].splitDimension][1];
        RangeType rangeA, rangeB;
        cutRange(range, boxes[bp].splitDimension, maxValue, rangeA, rangeB);

        if(level < maxLevels)
        {
            conduit::index_t child0 = boxes[bp].childOffset;
            conduit::index_t child1 = boxes[bp].childOffset + 1;
            constructBox(child0, rangeA, A, level + 1, maxLevels);
            constructBox(child1, rangeB, B, level + 1, maxLevels);
        }
        else
        {
            boxes[bp].childOffset = NoChild;
        }
    }
    else
    {
        boxes[bp].childOffset = NoChild;
    }
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
int kdtree<Indexable, CoordinateType, NDIMS>::longest(const kdtree<Indexable, CoordinateType, NDIMS>::BoxType &b) const
{
    int rv = 0;
    CoordinateType len = b[0][1] - b[0][0];
    for(int i = 1; i < dims(); i++)
    {
        CoordinateType newlen = b[i][1] - b[i][0];
        if(newlen > len)
            rv = i;
    } 
    return rv;
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
void kdtree<Indexable, CoordinateType, NDIMS>::sortIndexRange(
    const kdtree<Indexable, CoordinateType, NDIMS>::RangeType &range, int dimension)
{
    if(range.size > 1)
    {
        // Sort index elements within the range according to the coordinate dimension.
        const Indexable &data = coords[dimension];
        std::sort(index.begin() + range.offset,
                  index.begin() + range.offset + range.size,
                  [&data](conduit::index_t idx1, conduit::index_t idx2)
                  {
                      return data[idx1] < data[idx2];
                  });
    }
}

//---------------------------------------------------------------------------
template <typename Indexable, typename CoordinateType, int NDIMS>
bool kdtree<Indexable, CoordinateType, NDIMS>::pointInBox(
    const kdtree<Indexable, CoordinateType, NDIMS>::PointType &pt, const BoxType &b) const
{
    bool retval = true;
    for(int i = 0; i < dims(); i++)
        retval &= (pt[i] >= b[i][0] && pt[i] < b[i][1]);
    return retval;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::utils --
//-----------------------------------------------------------------------------

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

#endif
