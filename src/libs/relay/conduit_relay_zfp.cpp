// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_zfp.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay_zfp.hpp"
#include "zfp/factory.hpp"

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


//-----------------------------------------------------------------------------
// -- begin conduit::relay::io --
//-----------------------------------------------------------------------------
namespace io
{

//-----------------------------------------------------------------------------
zfp::array *
unwrap_zfparray(const Node &node)
{
    const Node &zfp_hdr  = node.fetch_existing(ZFP_HEADER_FIELD);
    const Node &zfp_data = node.fetch_existing(ZFP_COMPRESSED_DATA_FIELD);

    uint zfp_dim   = node.fetch_existing(ZFP_HEADER_DIM_FIELD).to_value();
    uint zfp_stype = node.fetch_existing(ZFP_HEADER_SCALAR_TYPE_FIELD).to_value();

    if( zfp_stype == zfp_type_none )
    {
         CONDUIT_ERROR("unwrapping zfp array with scalar type zfp_type_none ("
                       << zfp_stype << " is not supported") ;
    }
    else if( zfp_stype == zfp_type_int32 )
    {
         CONDUIT_ERROR("unwrapping zfp array with scalar type zfp_type_int32 ("
                       << zfp_stype << " is not supported") ;
    }
    else  if( zfp_stype == zfp_type_int64 )
    {
         CONDUIT_ERROR("unwrapping zfp array with scalar type zfp_type_int64 ("
                       << zfp_stype << " is not supported") ;
    }

    if(zfp_dim == 1)
    {
        if(zfp_stype == zfp_type_float)
        {
            zfp::array1f::header h(zfp_hdr.data_ptr(), zfp_hdr.allocated_bytes());
            zfp::array *res = zfp::array::construct(h);
            // copy data into the array instance
            std::memcpy(res->compressed_data(), 
                        zfp_data.data_ptr(),zfp_data.allocated_bytes());
            return res;
        }
        else if(zfp_stype == zfp_type_double)
        {
            zfp::array1d::header h(zfp_hdr.data_ptr(), zfp_hdr.allocated_bytes());
            zfp::array *res = zfp::array::construct(h);
            // copy data into the array instance
            std::memcpy(res->compressed_data(), 
                        zfp_data.data_ptr(),zfp_data.allocated_bytes());
            return res;
        }
    }
    else if(zfp_dim == 2)
    {
        if(zfp_stype == zfp_type_float)
        {
            zfp::array2f::header h(zfp_hdr.data_ptr(), zfp_hdr.allocated_bytes());
            zfp::array *res = zfp::array::construct(h);
            // copy data into the array instance
            std::memcpy(res->compressed_data(), 
                        zfp_data.data_ptr(),zfp_data.allocated_bytes());
            return res;
        }
        else if(zfp_stype == zfp_type_double)
        {
            zfp::array2d::header h(zfp_hdr.data_ptr(), zfp_hdr.allocated_bytes());
            zfp::array *res = zfp::array::construct(h);
            // copy data into the array instance
            std::memcpy(res->compressed_data(), 
                        zfp_data.data_ptr(),zfp_data.allocated_bytes());
            return res;
        }
    }
    else if(zfp_dim == 3)
    {
        if(zfp_stype == zfp_type_float)
        {
            zfp::array3f::header h(zfp_hdr.data_ptr(), zfp_hdr.allocated_bytes());
            zfp::array *res = zfp::array::construct(h);
            // copy data into the array instance
            std::memcpy(res->compressed_data(), 
                        zfp_data.data_ptr(),zfp_data.allocated_bytes());
            return res;
        }
        else if(zfp_stype == zfp_type_double)
        {
            zfp::array3d::header h(zfp_hdr.data_ptr(), zfp_hdr.allocated_bytes());
            zfp::array *res = zfp::array::construct(h);
            // copy data into the array instance
            std::memcpy(res->compressed_data(), 
                        zfp_data.data_ptr(),zfp_data.allocated_bytes());
            return res;
        }
    }
    // note: 4d is not supported
    else
    {
        // error, unsupported dim
        CONDUIT_ERROR("unwrapping zfp array with dimension " << zfp_dim << " is not supported") ;
    }

    return NULL;
}


//-----------------------------------------------------------------------------
void
wrap_zfparray(const zfp::array *arr,
              Node &dest)
{
    wrap_zfparray(*arr,dest);
}

//-----------------------------------------------------------------------------
void
wrap_zfparray(const zfp::array &arr,
              Node &dest)
{
    dest.reset();

    // store header
    uint zfp_dim   = arr.dimensionality();
    uint zfp_stype = arr.scalar_type();

    // array::scalar_type()
    // supported types:
    //  zfp_type_float  = 3, // single precision floating point
    //  zfp_type_double = 4  // double precision floating point

    // unsupported:
    //  zfp_type_none   = 0, // unspecified type
    //  zfp_type_int32  = 1, // 32-bit signed integer
    //  zfp_type_int64  = 2, // 64-bit signed integer

    if( zfp_stype == zfp_type_none )
    {
         CONDUIT_ERROR("wrapping zfp array with scalar type zfp_type_none ("
                       << zfp_stype << " is not supported") ;
    }
    else if( zfp_stype == zfp_type_int32 )
    {
         CONDUIT_ERROR("wrapping zfp array with scalar type zfp_type_int32 ("
                       << zfp_stype << " is not supported") ;
    }
    else  if( zfp_stype == zfp_type_int64 )
    {
         CONDUIT_ERROR("wrapping zfp array with scalar type zfp_type_int64 ("
                       << zfp_stype << " is not supported") ;
    }

    dest[ZFP_HEADER_DIM_FIELD] = zfp_dim;
    dest[ZFP_HEADER_SCALAR_TYPE_FIELD] = zfp_stype;

    if(zfp_dim == 1)
    {
        if(zfp_stype == zfp_type_float)
        {
            zfp::array1f::header h(arr);
            dest[ZFP_HEADER_FIELD].set(static_cast<const uint8*>(h.data()),
                                       h.size_bytes());
        }
        else if(zfp_stype == zfp_type_double)
        {
            zfp::array1d::header h(arr);
            dest[ZFP_HEADER_FIELD].set(static_cast<const uint8*>(h.data()),
                                       h.size_bytes());
        }
    }
    else if(zfp_dim == 2)
    {
        if(zfp_stype == zfp_type_float)
        {
            zfp::array2f::header h(arr);
            dest[ZFP_HEADER_FIELD].set(static_cast<const uint8*>(h.data()),
                                       h.size_bytes());
        }
        else if(zfp_stype == zfp_type_double)
        {
            zfp::array2d::header h(arr);
            dest[ZFP_HEADER_FIELD].set(static_cast<const uint8*>(h.data()),
                                       h.size_bytes());
        }
    }
    else if(zfp_dim == 3)
    {
        if(zfp_stype == zfp_type_float)
        {
            zfp::array3f::header h(arr);
            dest[ZFP_HEADER_FIELD].set(static_cast<const uint8*>(h.data()),
                                       h.size_bytes());
        }
        else if(zfp_stype == zfp_type_double)
        {
            zfp::array3d::header h(arr);
            dest[ZFP_HEADER_FIELD].set(static_cast<const uint8*>(h.data()),
                                       h.size_bytes());
        }
    } // note: 4d not supported
    else
    {
        // error, unsupported dim
        CONDUIT_ERROR("wrapping zfp array with dimension " << zfp_dim << " is not supported") ;
    }

    // store data
    dest[ZFP_COMPRESSED_DATA_FIELD].set(static_cast<const uint8*>(arr.compressed_data()),
                                        arr.compressed_size());
}

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::io --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
