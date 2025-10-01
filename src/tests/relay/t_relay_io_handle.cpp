// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_io_handle.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay.hpp"
#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::relay;


//-----------------------------------------------------------------------------
std::vector<std::string>
basic_handle_io_protocols()
{
    return std::vector<std::string> {"conduit_bin",
                                     "json",
                                     "conduit_json",
                                     "conduit_base64_json",
                                     "yaml",
                                     "conduit_base64_yaml"};
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_active_protos)
{
    std::string tfile_base = "tout_conduit_relay_io_handle.";
    std::vector<std::string> protocols = basic_handle_io_protocols();

    Node n_about;
    io::about(n_about);

    if(n_about["protocols/hdf5"].as_string() == "enabled")
        protocols.push_back("hdf5");

    for (std::vector<std::string>::const_iterator itr = protocols.begin();
             itr < protocols.end(); ++itr)
    {
        std::string protocol = *itr;
        CONDUIT_INFO("Testing Relay IO Handle with protocol: "
                     << protocol );
        std::string test_file_name = tfile_base  + protocol;

        utils::remove_path_if_exists(test_file_name);

        int64 a_val = 20;
        int64 b_val = 8;
        int64 c_val = 13;
        int64 here_val = 10;

        Node n;
        n["a"] = a_val;
        n["b"] = b_val;
        n["c"] = c_val;
        n["d/here"] = here_val;

        std::vector<std::string> cnames;

        io::IOHandle h;
        h.open(test_file_name);
        h.write(n);
        // test flush (not always needed in practice, but we want to test all cases )
        h.flush();

        EXPECT_TRUE(h.has_path("d/here"));
        h.list_child_names(cnames);

        EXPECT_EQ(cnames[0],"a");
        EXPECT_EQ(cnames[1],"b");
        EXPECT_EQ(cnames[2],"c");
        EXPECT_EQ(cnames[3],"d");

        h.list_child_names("d",cnames);
        EXPECT_EQ(cnames[0],"here");

        h.remove("d");
        EXPECT_FALSE(h.has_path("d"));
        EXPECT_FALSE(h.has_path("d/here"));
        h.close();

        Node n2;
        io::IOHandle h2;
        h2.open(test_file_name);
        h2.list_child_names(cnames);
        EXPECT_EQ(cnames[0],"a");
        EXPECT_EQ(cnames[1],"b");
        EXPECT_EQ(cnames[2],"c");

        Node n_val;
        n_val = here_val;
        // write with path
        h2.write(n_val,"d/here");

        h2.read(n2);
        Node info;
        EXPECT_FALSE(n.diff(n2, info, 0.0));
        info.print();

        // read with path
        n_val.reset();
        h2.read("c",n_val);

        EXPECT_EQ(n_val.as_int64(),c_val);

        h2.close();
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_is_open)
{
    // remove files if they exist
    utils::remove_path_if_exists("tout_conduit_relay_io_handle_is_open.hdf5");
    utils::remove_path_if_exists("tout_conduit_relay_io_handle_is_open.conduit_json");

    io::IOHandle h;
    EXPECT_FALSE(h.is_open());
    EXPECT_THROW(h.open("here/is/a/garbage/file/path.json"),
                 conduit::Error);

    EXPECT_FALSE(h.is_open());

    // if hdf5 is enabled, check bad path open for that imp
    Node n_about;
    io::about(n_about);
    if(n_about["protocols/hdf5"].as_string() == "enabled")
    {
        EXPECT_THROW(h.open("here/is/a/garbage/file/path.hdf5"),
                     conduit::Error);

        EXPECT_FALSE(h.is_open());

        h.open("tout_conduit_relay_io_handle_is_open.hdf5");

        EXPECT_TRUE(h.is_open());

        h.close();

        EXPECT_FALSE(h.is_open());

    }

    // test subpath for exceptions
    EXPECT_THROW(h.open("file.json:here/is/a/subpath/to/data"),
                 conduit::Error);

    EXPECT_FALSE(h.is_open());

    h.open("tout_conduit_relay_io_handle_is_open.conduit_json");

    EXPECT_TRUE(h.is_open());

    h.close();

    EXPECT_FALSE(h.is_open());
}


//-----------------------------------------------------------------------------
// NOTE: If we add support for opening subpaths, here is a start at testing
// //-----------------------------------------------------------------------------
// TEST(conduit_relay_io_handle, test_active_protos_subpath)
// {
//
//     std::string tfile_base = "tout_conduit_relay_io_handle_subpath.";
//     std::vector<std::string> protocols = basic_handle_io_protocols();
//
//     Node n_about;
//     io::about(n_about);
//
//     if(n_about["protocols/hdf5"].as_string() == "enabled")
//         protocols.push_back("hdf5");
//
//     for (std::vector<std::string>::const_iterator itr = protocols.begin();
//              itr < protocols.end(); ++itr)
//     {
//         std::string protocol = *itr;
//         CONDUIT_INFO("Testing Relay IO Handle with protocol: "
//                      << protocol );
//         std::string test_file_name = tfile_base  + protocol;
//
//         if(utils::is_file(test_file_name))
//         {
//             utils::remove_file(test_file_name);
//         }
//
//         Node n_seed;
//
//
//         int64 a_val = 20;
//         int64 b_val = 8;
//         int64 c_val = 13;
//         int64 here_val = 10;
//
//         n_seed["here/is/my/data/a"] = a_val;
//         n_seed["here/is/my/data/b"] = b_val;
//         n_seed["here/is/my/data/c"] = c_val;
//
//         n_seed["here/is/other/data/here"] = here_val;
//
//         io::IOHandle h;
//         h.open(test_file_name);
//         h.write(n_seed);
//         h.close();
//
//         io::IOHandle h2;
//         Node n;
//         h2.open(test_file_name + ":here/is/my" );
//         h2.read(n);
//         n.print();
//     }
// }

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_exceptions)
{
    io::IOHandle h;

    // check throw on methods called on not open handle
    Node n;

    EXPECT_THROW(h.write(n), conduit::Error);
    EXPECT_THROW(h.read(n), conduit::Error);
    EXPECT_THROW(h.has_path("here"), conduit::Error);
    EXPECT_THROW(h.remove("here"), conduit::Error);

    std::vector<std::string> cld_names;
    EXPECT_THROW(h.list_child_names(cld_names), conduit::Error);

    EXPECT_THROW(h.open("here/is/a/garbage/file/path.json"),
                 conduit::Error);

    // if hdf5 is enabled, check bad path open for that imp
    Node n_about;
    io::about(n_about);
    if(n_about["protocols/hdf5"].as_string() == "enabled")
    {
        EXPECT_THROW(h.open("here/is/a/garbage/file/path.hdf5"),
                     conduit::Error);
    }

    // test subpath for exceptions
    EXPECT_THROW(h.open("file.json:here/is/a/subpath/to/data"),
                 conduit::Error);
}




//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, leading_slash_reads)
{
    std::vector<std::string> protocols  = basic_handle_io_protocols();

    Node n_about;
    io::about(n_about);

    if(n_about["protocols/hdf5"].as_string() == "enabled")
        protocols.push_back("hdf5");

    Node n;
    n["path/to/some/data"] = "data!";


    for (std::vector<std::string>::const_iterator itr = protocols.begin();
             itr < protocols.end(); ++itr)
    {

        std::string protocol = *itr;
        CONDUIT_INFO("Testing Relay IO Handle read path including leading '/' with protocol: "
                     << protocol );
        std::string test_file_name = "tout_conduit_relay_handle_read_leading_slash."
                                     + protocol;
        // seed the data
        conduit::relay::io::save(n,test_file_name,protocol);

        // get a handle
        io::IOHandle h;
        h.open(test_file_name,protocol);


        Node n_read, info;
        h.read("path",n_read);
        std::cout << "read: path" << std::endl;
        n_read.print();
        EXPECT_FALSE(n["path"].diff(n_read,info));
        std::cout << "diff:";
        info.print();

        n_read.reset();
        std::cout << "read: /path" << std::endl;
        h.read("/path",n_read);
        n_read.print();

        EXPECT_FALSE(n["path"].diff(n_read,info));
        std::cout << "diff:";
        info.print();
    }

}




//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_mode)
{
    // default mode is rw, this test checks the "only" modes
    // read only (mode="r") and write only (mode="w")
    int64 a_val = 20;
    int64 b_val = 8;
    int64 c_val = 13;
    int64 here_val = 10;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;
    n["d/here"] = here_val;

    std::vector<std::string> protocols = basic_handle_io_protocols();

    Node n_about;
    io::about(n_about);

    if(n_about["protocols/hdf5"].as_string() == "enabled")
        protocols.push_back("hdf5");

    for (std::vector<std::string>::const_iterator itr = protocols.begin();
             itr < protocols.end(); ++itr)
    {

        std::string protocol = *itr;
        CONDUIT_INFO("Testing Relay IO Handle Open Mode 'r' with protocol: "
                     << protocol );
        std::string test_file_name = "tout_conduit_relay_io_handle_mode_ro."
                                     + protocol;

        utils::remove_path_if_exists(test_file_name);

        /// read only
        Node opts_ronly;
        opts_ronly["mode"] = "r";

        io::IOHandle h_ro;
        // if read only, it will fail to open a file that doesn't exist
        EXPECT_THROW(h_ro.open(test_file_name,
                     opts_ronly),
                     conduit::Error);

        relay::io::save(n,test_file_name);
        // try again now that the file exists
        h_ro.open(test_file_name,
                  opts_ronly);

        // read only, fail to write:
        EXPECT_THROW(h_ro.write(n), conduit::Error);
        // read only, fail to write w/ path:
        EXPECT_THROW(h_ro.write(n,"super"), conduit::Error);
        // read only, fail to remove:
        EXPECT_THROW(h_ro.remove("super"), conduit::Error);

        // make sure we can read something
        Node n_read;
        h_ro.read("d/here",n_read);
        EXPECT_EQ(n["d/here"].as_int64(),n_read.as_int64());

        CONDUIT_INFO("Testing Relay IO Handle Open Mode 'w' with protocol: "
                     << protocol );
        test_file_name = "tout_conduit_relay_io_handle_mode_wo."
                         + protocol;

        /// write only
        Node opts_wonly;
        opts_wonly["mode"] = "w";

        io::IOHandle h_wo;
        h_wo.open(test_file_name,opts_wonly);

        // write only, fail to read
        EXPECT_THROW(h_wo.read(n), conduit::Error);
        EXPECT_THROW(h_wo.read("super",n), conduit::Error);
        // write only, fail to has_path
        EXPECT_THROW(h_wo.has_path("super"), conduit::Error);
        // write only, fail to list_child_names
        std::vector<std::string> cld_names;
        EXPECT_THROW(h_wo.list_child_names(cld_names), conduit::Error);
        EXPECT_THROW(h_wo.list_child_names("super",cld_names), conduit::Error);

        h_wo.write(n);
        h_wo.close();

        // lets read what wrote to make sure the handled actually wrote
        // something
        n_read.reset();
        relay::io::load(test_file_name,n_read);
        EXPECT_EQ(n["d/here"].as_int64(),n_read["d/here"].to_int64());
    }

    //
    // io::IOHandle h(opts);
    // // if read only, it will fail to open a file that doesn't exist
    // EXPECT_THROW(h.open("tout_conduit_relay_io_handle_mode_wo.conduit_bin"),
    //                     conduit::Error);
    // n.save("tout_conduit_relay_io_handle_mode_wo.conduit_bin");
    // // try again.
    // h.open("tout_conduit_relay_io_handle_mode_ok.conduit_bin")
    //
    // // read only, fail to write:
    // EXPECT_THROW(h.write(n), conduit::Error);



}



//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_reuse_handle)
{
    int64 a_val = 20;
    int64 b_val = 8;
    int64 c_val = 13;
    int64 here_val = 10;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;
    n["d/here"] = here_val;

    // remove files if they already exist
    utils::remove_path_if_exists("tout_conduit_relay_io_handle_reopen_1.conduit_bin");
    utils::remove_path_if_exists("tout_conduit_relay_io_handle_reopen_1.conduit_json");

    utils::remove_path_if_exists("tout_conduit_relay_io_handle_reopen_2.conduit_bin");
    utils::remove_path_if_exists("tout_conduit_relay_io_handle_reopen_2.conduit_json");


    io::IOHandle h;
    h.open("tout_conduit_relay_io_handle_reopen_1.conduit_bin");
    h.write(n);
    h.close();

    h.open("tout_conduit_relay_io_handle_reopen_2.conduit_bin");
    h.write(n);
    h.close();

    Node nread;
    h.open("tout_conduit_relay_io_handle_reopen_1.conduit_bin");
    h.read(nread);

    Node info;
    EXPECT_FALSE(n.diff(nread, info, 0.0));

    nread.reset();
    // check open w/o close
    h.open("tout_conduit_relay_io_handle_reopen_2.conduit_bin");
    h.read(nread);

    EXPECT_FALSE(n.diff(nread, info, 0.0));

}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_handle_trunc)
{
    int64 a_val = 20;
    int64 b_val = 8;
    int64 c_val = 13;
    int64 here_val = 10;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;
    n["d/here"] = here_val;

    // remove files if they already exist
    utils::remove_path_if_exists("tout_conduit_relay_io_handle_trunc_1.conduit_bin");
    utils::remove_path_if_exists("tout_conduit_relay_io_handle_trunc_1.conduit_json");

    io::IOHandle h;
    h.open("tout_conduit_relay_io_handle_trunc_1.conduit_bin");
    h.write(n);
    h.close();

    // test truncate
    n.reset();
    n["a"] = a_val;

    Node opts;
    opts["mode"] = "wt";
    h.open("tout_conduit_relay_io_handle_trunc_1.conduit_bin",opts);
    h.write(n);
    h.close();

    Node nread;
    opts["mode"] = "r";
    h.open("tout_conduit_relay_io_handle_trunc_1.conduit_bin",opts);
    h.read(nread);

    Node info;
    EXPECT_FALSE(n.diff(nread, info, 0.0));

    nread.print();
    info.print();
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_hdf5_trunc)
{
    std::string tfile_out = "tout_conduit_relay_io_handle_hdf5_trunc.hdf5";
    Node n_about;
    io::about(n_about);

    // skip test if hdf5 isn't enabled
    if(n_about["protocols/hdf5"].as_string() != "enabled")
        return;

    int64 a_val = 20;
    int64 b_val = 8;
    int64 c_val = 13;
    int64 here_val = 10;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;
    n["d/here"] = here_val;

    // remove files if they already exist
    utils::remove_path_if_exists(tfile_out);

    io::IOHandle h;
    h.open(tfile_out);
    h.write(n);
    h.close();

    // test truncate
    n.reset();
    n["a"] = a_val;

    Node opts;
    opts["mode"] = "wt";
    h.open(tfile_out,opts);
    h.write(n);
    h.close();

    Node nread;
    opts["mode"] = "r";
    h.open(tfile_out,opts);
    h.read(nread);

    Node info;
    EXPECT_FALSE(n.diff(nread, info, 0.0));

    nread.print();
    info.print();

    h.open(tfile_out);
    h.write(n,"b/c/d/e/f");
    h.close();

    n["b/c/d/e/f/a"] = a_val;

    opts["mode"] = "r";
    h.open(tfile_out,opts);
    h.read(nread);

    EXPECT_FALSE(n.diff(nread, info, 0.0));

    nread.print();
    info.print();
}


//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_offset_and_stride)
{
    Node n_about;
    io::about(n_about);

    // skip test if hdf5 isn't enabled
    if(n_about["protocols/hdf5"].as_string() != "enabled")
        return;

    std::string tfile_out = "tout_hdf5_io_handle_with_offset.hdf5";
    // remove files if they already exist
    utils::remove_path_if_exists(tfile_out);

    Node n, n_read, n_check, opts, info;
    n["data"]= { 0,1,2,3,4,5,6,7,8,9};

    io::IOHandle h;
    h.open(tfile_out);
    h.write(n);

    h.read(n_read);
    n_read.print();

    n_check = n;
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));

    // strided read
    n_read.reset();
    opts.reset();
    opts["stride"] = 2;
    h.read(n_read,opts);
    n_read.print();

    n_check.reset();
    n_check["data"] = {0,2,4,6,8};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));

    // offset write
    n = {-1,-1,-1,-1,-1};
    opts.reset();
    opts["offset"] = 5;
    h.write(n,"data",opts);

    n_read.reset();
    h.read(n_read);
    n_read.print();

    n_check.reset();
    n_check["data"] = {0,1,2,3,4,-1,-1,-1,-1,-1};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));


    // read the  first part of the seq
    opts.reset();
    opts["size"] = 5;
    n_read.reset();
    h.read("data",n_read,opts);
    n_read.print();

    n_check.reset();
    n_check = {0,1,2,3,4};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));

    // read the second part of the seq (-1's)
    opts.reset();
    opts["offset"] = 5;
    n_read.reset();
    h.read("data",n_read,opts);
    n_read.print();

    n_check.reset();
    n_check = {-1,-1,-1,-1,-1};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));


    // strided write
    n = {1,1,1,1,1};
    opts.reset();
    opts["stride"] = 2;
    h.write(n,"data",opts);

    // strided +offset write
    n = {2,2,2,2,2};
    opts.reset();
    opts["offset"] = 1;
    opts["stride"] = 2;
    h.write(n,"data",opts);

    n_read.reset();
    h.read(n_read);
    n_read.print();

    n_check.reset();
    n_check["data"] = {1, 2, 1, 2, 1, 2,  1, 2, 1, 2};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));


    // read the 1's
    opts.reset();
    opts["offset"] = 0;
    opts["stride"] = 2;
    n_read.reset();
    h.read("data",n_read,opts);
    n_read.print();

    n_check.reset();
    n_check = {1, 1, 1, 1, 1};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));


    // read the 2's
    opts.reset();
    opts["offset"] = 1;
    opts["stride"] = 2;
    n_read.reset();
    h.read("data",n_read,opts);
    n_read.print();


    n_check.reset();
    n_check = {2, 2, 2, 2, 2};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));


    // read subset of the 2's
    opts.reset();
    opts["offset"] = 1;
    opts["stride"] = 2;
    opts["size"] = 2;
    n_read.reset();
    h.read("data",n_read,opts);
    n_read.print();

    n_check.reset();
    n_check = {2, 2};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));

    // huge stride, this will only read the first entry
    n_read.reset();
    opts.reset();
    opts["stride"] = 1000;
    h.read(n_read,opts);
    n_read.print();

    n_check.reset();
    n_check["data"] = {1};
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));


    // now some error conditions:

    // neg size
    n_read.reset();
    opts.reset();
    opts["size"] = -100;
    EXPECT_THROW(h.read(n_read,opts),conduit::Error);

    // zero stride
    n_read.reset();
    opts.reset();
    opts["stride"] = 0;
    EXPECT_THROW(h.read(n_read,opts),conduit::Error);

    // // huge size
    n_read.reset();
    opts.reset();
    opts["size"] = 1000;
    EXPECT_THROW(h.read(n_read,opts),conduit::Error);

    // huge offset
    n_read.reset();
    opts.reset();
    opts["offset"] = 1000;
    EXPECT_THROW(h.read(n_read,opts),conduit::Error);

}




//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_empty_path_as_root)
{
    int64 a_val = 20;
    int64 b_val = 8;
    int64 c_val = 13;
    int64 here_val = 10;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;
    n["d/here"] = here_val;


    std::string ofname = "tout_conduit_relay_io_empty_path_as_root.conduit_bin";

    // remove files if they already exist
    utils::remove_path_if_exists("tout_conduit_relay_io_empty_path_as_root.conduit_bin");
    utils::remove_path_if_exists("tout_conduit_relay_io_empty_path_as_root.conduit_json");

    Node n_read_1, n_read_2, n_read_3, info;

    io::IOHandle h;
    h.open(ofname);
    h.write(n);
    h.close();

    // all of these 3 cases should be equiv
    h.open(ofname);
    h.read(n_read_1);
    h.close();

    h.open(ofname);
    h.read("",n_read_2);
    h.close();

    h.open(ofname);
    h.read("/",n_read_3);
    h.close();

    EXPECT_FALSE(n.diff(n_read_1, info, 0.0));
    EXPECT_FALSE(n.diff(n_read_2, info, 0.0));
    EXPECT_FALSE(n.diff(n_read_2, info, 0.0));
}


//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_ref_path_error_msg)
{
    // check that ref  path only appears in the error message string once

    Node n_about;
    io::about(n_about);

    // skip test if hdf5 isn't enabled
    if(n_about["protocols/hdf5"].as_string() != "enabled")
        return;

    std::string tfile_out = "tout_hdf5_io_handle_for_ref_path_error_msg.hdf5";
    // remove files if they already exist
    utils::remove_path_if_exists(tfile_out);

    Node n, n_read, n_check, opts, info;
    n["my/path/to/some/data"]= { 0,1,2,3,4,5,6,7,8,9};

    io::IOHandle h;
    h.open(tfile_out);
    h.write(n);

    h.read(n_read);
    n_read.print();

    n_check = n;
    // expect no diff
    EXPECT_FALSE(n_read.diff(n_check,info));

    // huge offset
    n_read.reset();
    opts.reset();
    opts["offset"] = 1000;
    try
    {
        h.read(n_read,opts);
    }
    catch(conduit::Error &e)
    {
        std::string msg = e.message();
        std::cout << "error message:"
                  <<  msg << std::endl;
        int count = 0;

        std::string::size_type pos = 0;
        std::string path = "my/path/to/some/data";

        while ((pos = msg.find(path, pos )) != std::string::npos)
        {
            count++;
            pos += path.length();
        }

        std::cout << "# of occurrences of path: " << count << std::endl;
        // the path should only appear in the error message string once
        EXPECT_EQ(count,1);
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_flush_basic)
{
    std::string tfile_base = "tout_conduit_relay_io_handle_flush.";
    std::vector<std::string> protocols = basic_handle_io_protocols();

    for (std::vector<std::string>::const_iterator itr = protocols.begin();
             itr < protocols.end(); ++itr)
    {
        std::string protocol = *itr;
        CONDUIT_INFO("Testing Relay IO Handle flush with protocol: "
                     << protocol );
        std::string test_file_name = tfile_base + protocol;

        utils::remove_path_if_exists(test_file_name);

        int64 a_val = 20;
        int64 b_val = 8;
        int64 c_val = 13;
        int64 here_val = 10;

        Node n;
        n["a"] = a_val;
        n["b"] = b_val;
        n["c"] = c_val;
        n["d/here"] = here_val;

        io::IOHandle h;
        h.open(test_file_name);
        h.write(n);

        // this will flush to file system
        h.flush();

        Node n2;
        io::IOHandle h2;

        // use read only b/c of the nature of this test
        Node opts_ronly;
        opts_ronly["mode"] = "r";
        h2.open(test_file_name,opts_ronly);
        h2.read(n2);
        EXPECT_EQ(n2["a"].as_int64(),a_val);
        EXPECT_EQ(n2["b"].as_int64(),b_val);
        EXPECT_EQ(n2["c"].as_int64(),c_val);
        EXPECT_EQ(n2["d/here"].as_int64(),here_val);
        h2.close();
        // close orig
        h.close();
    }
}
