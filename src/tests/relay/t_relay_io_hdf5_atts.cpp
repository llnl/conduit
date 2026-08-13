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
    io::hdf5_reset_options();

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
    io::hdf5_reset_options();

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
    io::hdf5_reset_options();
    // create hdf5 file that has a group and dataset with multiple attributes

    std::string fname = "tout_hdf5_attrs_non_conduit_writer.hdf5";
    herr_t status = 0;
    hsize_t num_vals = 10;
    std::vector<double> vals(10,3.1415);

    // create a file
    hid_t file = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    ASSERT_GE(file, 0) << "Failed to create hdf5 file";

    // Create, init a dataspace for the dataset
    hid_t    dataset, dataspace;
    dataspace = H5Screate_simple(1, &num_vals, NULL);

    // Create, init the dataset.  Element type is double.
    dataset = H5Dcreate(file,
                        "my_data",
                        H5T_NATIVE_DOUBLE,
                        dataspace,
                        H5P_DEFAULT,
                        H5P_DEFAULT,
                        H5P_DEFAULT);
    ASSERT_GE(dataset, 0) << "Failed to create dataset";
    status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, vals.data());
    ASSERT_GE(status, 0) << "Failed to write to dataset";
    // add attributes to the dataset

    //
    // write integer attribute
    //t

    hid_t attspace  = H5Screate(H5S_SCALAR);
    ASSERT_GE(attspace, 0) << "Failed to create dataspace";
    hid_t att = H5Acreate2(dataset, "my_int_att", H5T_NATIVE_INT, attspace,
                            H5P_DEFAULT, H5P_DEFAULT);
    ASSERT_GE(att, 0) << "Failed to create to create attribute";

    int a_int_val = 42;
    status = H5Awrite(att, H5T_NATIVE_INT, &a_int_val);
    ASSERT_GE(status, 0) << "Failed to write attribute";
    status = H5Sclose(attspace);
    ASSERT_GE(status, 0) << "Failed to close space";
    status = H5Aclose(att);
    ASSERT_GE(status, 0) << "Failed to close attribute";

    //
    // write string attribute
    //
    const char *a_str_val = "xyz";
    attspace = H5Screate(H5S_SCALAR);
    ASSERT_GE(attspace, 0) << "Failed to create dataspace";
    hid_t atype = H5Tcopy(H5T_C_S1);
    // Note: we need to include null term
    H5Tset_size(atype, strlen(a_str_val)+1);
    H5Tset_strpad(atype,H5T_STR_NULLTERM);

    att = H5Acreate2(dataset, "my_str_att", atype, attspace, H5P_DEFAULT, H5P_DEFAULT);
    ASSERT_GE(att, 0) << "Failed to create to create attribute";

    status = H5Awrite(att, atype, a_str_val);
    ASSERT_GE(status, 0) << "Failed to write attribute";
    status = H5Sclose(attspace);
    ASSERT_GE(status, 0) << "Failed to close space";
    status = H5Aclose(att);
    ASSERT_GE(status, 0) << "Failed to close attribute";

    // close dataset
    status = H5Dclose(dataset);
    ASSERT_GE(status, 0) << "Failed to close to dataset";
    // close the dataspace
    status = H5Sclose(dataspace);
    ASSERT_GE(status, 0) << "Failed to close to dataspace";

    // create a group
    hid_t grp = H5Gcreate(file,
                          "my_group",
                           H5P_DEFAULT,
                           H5P_DEFAULT,
                           H5P_DEFAULT);
    ASSERT_GE(grp, 0) << "Failed to create group";

    // add attributes to the group

    //
    // write integer attribute
    //t

    attspace  = H5Screate(H5S_SCALAR);
    ASSERT_GE(attspace, 0) << "Failed to create dataspace";
    att = H5Acreate2(grp, "my_float_att", H5T_NATIVE_FLOAT, attspace,
                     H5P_DEFAULT, H5P_DEFAULT);
    ASSERT_GE(att, 0) << "Failed to create to create attribute";

    float a_float_val = 2.71828;
    status = H5Awrite(att, H5T_NATIVE_FLOAT, &a_float_val);
    ASSERT_GE(status, 0) << "Failed to write attribute";
    status = H5Sclose(attspace);
    ASSERT_GE(status, 0) << "Failed to close space";
    status = H5Aclose(att);
    ASSERT_GE(status, 0) << "Failed to close attribute";

    //
    // write string attribute
    //
    attspace = H5Screate(H5S_SCALAR);
    ASSERT_GE(attspace, 0) << "Failed to create dataspace";
    atype = H5Tcopy(H5T_C_S1);
    // Note: we need to include null term
    H5Tset_size(atype, strlen(a_str_val)+1);
    H5Tset_strpad(atype,H5T_STR_NULLTERM);

    att = H5Acreate2(grp, "my_str_att", atype, attspace, H5P_DEFAULT, H5P_DEFAULT);
    ASSERT_GE(att, 0) << "Failed to create to create attribute";


    status = H5Awrite(att, atype, a_str_val);
    ASSERT_GE(status, 0) << "Failed to write attribute";
    status = H5Sclose(attspace);
    ASSERT_GE(status, 0) << "Failed to close space";
    status = H5Aclose(att);
    ASSERT_GE(status, 0) << "Failed to close attribute";


    // close group
    status = H5Gclose(grp);
    ASSERT_GE(status, 0) << "Failed to close to group";

    // close file
    status = H5Fclose(file);
    ASSERT_GE(status, 0) << "Failed to close to file";

    // read the file using conduit's atts support

    Node opts;
    opts["attributes/enabled"] = "true";
    io::hdf5_set_options(opts);

    Node nload;
    io::hdf5_read(fname,nload);
    nload.print();

    // [Expected]
    // my_data:
    //   attributes:
    //     my_int_att: 42
    //     my_str_att: "xyz"
    //   value: [3.1415, 3.1415, 3.1415, ..., 3.1415, 3.1415]
    // my_group:
    //   attributes:
    //     my_float_att: 2.71828007698059
    //     my_str_att: "xyz"

    EXPECT_TRUE(nload.has_path("my_data/value"));
    EXPECT_TRUE(nload.has_path("my_data/attributes/my_int_att"));
    EXPECT_TRUE(nload.has_path("my_data/attributes/my_str_att"));

    EXPECT_TRUE(nload.has_path("my_group/attributes/my_float_att"));
    EXPECT_TRUE(nload.has_path("my_group/attributes/my_str_att"));

    EXPECT_EQ(nload["my_data/attributes/my_int_att"].to_int64(),42);
    EXPECT_EQ(nload["my_data/attributes/my_str_att"].as_string(),"xyz");

    float64_accessor val_acc = nload["my_data/value"].value();
    EXPECT_NEAR(val_acc[0],3.1415,0.00001);

    EXPECT_EQ(nload["my_group/attributes/my_str_att"].as_string(),"xyz");
    float fval = nload["my_group/attributes/my_float_att"].to_float32();
    EXPECT_NEAR(fval,2.7182,0.001);
}



