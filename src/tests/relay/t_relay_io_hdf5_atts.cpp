// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_io_hdf5_atts.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay.hpp"
#include "conduit_relay_io_hdf5.hpp"
#include "hdf5.h"
#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::relay;

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_hdf5, hdf5_atts_roundtrip)
{
    Node n, nload, info;
    n["grp/dset/value"] = 3.1415;
    n["grp/dset/attributes/my_att_a"] = 42;
    n["grp/dset/attributes/my_att_b"] = "details";
    n["grp/attributes/my_att"] = 42;
    n["grp/my_list"].append() = 42;
    n["grp/my_list"].append() = 96;

    Node opts;
    opts["attributes/enabled"] = "false";
    io::hdf5_set_options(opts);

    CONDUIT_INFO("Example Tree")
    n.print();

    CONDUIT_INFO("Write without atts")
    // round trip both with and without atts support should be the same
    io::hdf5_write(n,"tout_hdf5_attrs_disabled.hdf5");

    CONDUIT_INFO("Read without atts");
    io::hdf5_read("tout_hdf5_attrs_disabled.hdf5",nload);
    nload.print();

    EXPECT_FALSE(n.diff(nload,info));

    opts["attributes/enabled"] = "true";
    io::hdf5_set_options(opts);

    CONDUIT_INFO("Write with atts")
    // round trip both with and without atts support should be the same
    io::hdf5_write(n,"tout_hdf5_attrs_enabled.hdf5");
    CONDUIT_INFO("Read with atts")
    nload.reset();
    io::hdf5_read("tout_hdf5_attrs_enabled.hdf5",nload);
    nload.print();
    EXPECT_FALSE(n.diff(nload,info));

}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_hdf5, hdf5_atts_custom_keys)
{
    // create a simple buffer of doubles
    Node n,nload,nsansattr,info;
    n["grp/dset/V"] = 3.1415;
    n["grp/dset/Atts/my_att"] = 42;
    n["grp/Atts/my_att_a"] = 42;
    n["grp/Atts/my_att_b"] = "details";
    
    nsansattr["grp/dset"] = 3.1415;
    
    Node opts;
    opts["attributes/enabled"] = "true";
    opts["attributes/attributes_key"] = "Atts";
    opts["attributes/value_key"] = "V";
    io::hdf5_set_options(opts);

    CONDUIT_INFO("Example Tree")
    n.print();
    CONDUIT_INFO("Write with atts")
    io::hdf5_write(n,"tout_hdf5_attrs_custom_keys.hdf5");
    CONDUIT_INFO("Read with atts")
    nload.reset();
    io::hdf5_read("tout_hdf5_attrs_custom_keys.hdf5",nload);
    nload.print();
    EXPECT_FALSE(n.diff(nload,info));


    opts["attributes/enabled"] = "false";
    io::hdf5_set_options(opts);
    nload.reset();
    io::hdf5_read("tout_hdf5_attrs_custom_keys.hdf5",nload);
    EXPECT_FALSE(nsansattr.diff(nload,info));

}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_hdf5, hdf5_read_existing_atts)
{
    // create hdf5 file that has a group and dataset with multiple attributes
    // TODO
}



