// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_utils.cpp
///
//-----------------------------------------------------------------------------


#include "conduit.hpp"

#include <iostream>
#include <limits>
#include "gtest/gtest.h"

#include "t_config.hpp"


using namespace conduit;

bool info_occured    = false;
bool warning_occured = false;
bool error_occured   = false;

bool other_info_occured    = false;
bool other_warning_occured = false;
bool other_error_occured   = false;

//-----------------------------------------------------------------------------
void
print_msg(const std::string &msg,
          const std::string &file,
          int line)
{
    std::cout << "File:"    << file << std::endl;
    std::cout << "Line:"    << line << std::endl;
    std::cout << "Message:" << msg  << std::endl;
}

//-----------------------------------------------------------------------------
void
my_info_handler(const std::string &msg,
                const std::string &file,
                int line)
{
    print_msg(msg,file,line);
    info_occured = true;
}


//-----------------------------------------------------------------------------
void
my_warning_handler(const std::string &msg,
                   const std::string &file,
                   int line)
{
    print_msg(msg,file,line);
    warning_occured = true;
}

//-----------------------------------------------------------------------------
void
my_error_handler(const std::string &msg,
                 const std::string &file,
                 int line)
{
    print_msg(msg,file,line);
    error_occured = true;
}

//-----------------------------------------------------------------------------
void
my_other_info_handler(const std::string &msg,
                      const std::string &file,
                      int line)
{
    print_msg(msg,file,line);
    other_info_occured = true;
}


//-----------------------------------------------------------------------------
void
my_other_warning_handler(const std::string &msg,
                   const std::string &file,
                   int line)
{
    print_msg(msg,file,line);
    other_warning_occured = true;
}

//-----------------------------------------------------------------------------
void
my_other_error_handler(const std::string &msg,
                 const std::string &file,
                 int line)
{
    print_msg(msg,file,line);
    other_error_occured = true;
}


//-----------------------------------------------------------------------------
TEST(conduit_utils, error_constructors)
{
    conduit::Error e("mymessage","myfile",10);
    CONDUIT_INFO(e.message());
    CONDUIT_INFO(e.what());

    try
    {
        utils::handle_warning("ERROR!",__FILE__,__LINE__);
    }
    catch(conduit::Error &e)
    {
        conduit::Error ecpy(e);
        CONDUIT_INFO(ecpy.message());
    }

    try
    {
        utils::handle_warning("ERROR!",__FILE__,__LINE__);
    }
    catch(conduit::Error &e)
    {
        conduit::Error ecpy;
        ecpy = e;
        CONDUIT_INFO(ecpy.message());
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_utils, override_info)
{
    utils::handle_info("INFO!",__FILE__,__LINE__);

    EXPECT_FALSE(info_occured);

    conduit::utils::set_info_handler(my_info_handler);
    utils::handle_info("INFO!",__FILE__,__LINE__);
    EXPECT_TRUE(info_occured);

    conduit::utils::set_info_handler(conduit::utils::default_info_handler);

    utils::handle_info("INFO!",__FILE__,__LINE__);
}


//-----------------------------------------------------------------------------
TEST(conduit_utils, override_warning)
{
    EXPECT_THROW(utils::handle_warning("WARNING!",__FILE__,__LINE__),
                 conduit::Error);

    EXPECT_FALSE(warning_occured);
    conduit::utils::set_warning_handler(my_warning_handler);
    utils::handle_warning("WARNING!",__FILE__,__LINE__);
    EXPECT_TRUE(warning_occured);

    conduit::utils::set_warning_handler(conduit::utils::default_warning_handler);

    EXPECT_THROW(utils::handle_warning("WARNING!",__FILE__,__LINE__),
                 conduit::Error);
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, override_error)
{
    EXPECT_THROW(utils::handle_warning("ERROR!",__FILE__,__LINE__),
                 conduit::Error);

    EXPECT_FALSE(error_occured);
    conduit::utils::set_error_handler(my_error_handler);
    utils::handle_error("ERROR!",__FILE__,__LINE__);
    EXPECT_TRUE(error_occured);

    conduit::utils::set_error_handler(conduit::utils::default_error_handler);

    EXPECT_THROW(utils::handle_warning("ERROR!",__FILE__,__LINE__),
                 conduit::Error);

}

//-----------------------------------------------------------------------------
TEST(conduit_utils, handler_defaults_are_the_defaults)
{
    conduit::utils::conduit_info_handler on_info = conduit::utils::info_handler();
    conduit::utils::conduit_info_handler on_info_default = conduit::utils::default_info_handler;
    EXPECT_EQ(on_info,on_info_default);

    conduit::utils::conduit_warning_handler on_warning = conduit::utils::warning_handler();
    conduit::utils::conduit_warning_handler on_warning_default = conduit::utils::default_warning_handler;
    EXPECT_EQ(on_warning,on_warning_default);

    conduit::utils::conduit_error_handler on_error = conduit::utils::error_handler();
    conduit::utils::conduit_error_handler on_error_default = conduit::utils::default_error_handler;
    EXPECT_EQ(on_error,on_error_default);


    EXPECT_NE(on_info,on_error);
    EXPECT_NE(on_info,on_warning);

    //
    // NOTE: (Excerpt from "Adventures in fun with compilers")
    // The default implementations of:
    //    conduit::utils::default_warning_handler
    //       and
    //    conduit::utils::default_error_handler
    // are identical at the source level.
    // On windows, we hit a case where the compiler understands this
    // and low and behold there is only one copy!
    //
    // B/c of this we can't expect the following to hold:
    // EXPECT_NE(on_error,on_warning);
    //

}

//-----------------------------------------------------------------------------
TEST(conduit_utils, handler_defaults_push_pop)
{
    // change to other handler
    conduit::utils::set_info_handler(my_other_info_handler);
    conduit::utils::set_warning_handler(my_other_warning_handler);
    conduit::utils::set_error_handler(my_other_error_handler);

    // ----------
    // demo push / pop like swing to default
    // ----------
    conduit::utils::conduit_info_handler curr_on_info = conduit::utils::info_handler();
    conduit::utils::conduit_warning_handler curr_on_warning = conduit::utils::warning_handler();
    conduit::utils::conduit_error_handler curr_on_error = conduit::utils::error_handler();

    // set to default
    conduit::utils::set_info_handler(conduit::utils::default_info_handler);
    conduit::utils::set_warning_handler(conduit::utils::default_warning_handler);
    conduit::utils::set_error_handler(conduit::utils::default_error_handler);

    EXPECT_THROW(utils::handle_warning("ERROR!",__FILE__,__LINE__),
                 conduit::Error);

    // set to curr
    conduit::utils::set_info_handler(curr_on_info);
    conduit::utils::set_warning_handler(curr_on_warning);
    conduit::utils::set_error_handler(curr_on_error);

    EXPECT_FALSE(other_info_occured);
    EXPECT_FALSE(other_warning_occured);
    EXPECT_FALSE(other_error_occured);

    utils::handle_info("INFO!",__FILE__,__LINE__);
    utils::handle_warning("WARNING!",__FILE__,__LINE__);
    utils::handle_error("ERROR!",__FILE__,__LINE__);

    EXPECT_TRUE(other_info_occured);
    EXPECT_TRUE(other_warning_occured);
    EXPECT_TRUE(other_error_occured);


    // back set to default (for other tests !!!!)
    conduit::utils::set_info_handler(conduit::utils::default_info_handler);
    conduit::utils::set_warning_handler(conduit::utils::default_warning_handler);
    conduit::utils::set_error_handler(conduit::utils::default_error_handler);

}




//-----------------------------------------------------------------------------
TEST(conduit_utils, escape_special_chars)
{
    std::string test = "\"myvalue\":10";
    std::string test_escaped   = utils::escape_special_chars(test);
    std::string test_unescaped = utils::unescape_special_chars(test_escaped);

    CONDUIT_INFO( test << " vs " << test_escaped);

    EXPECT_EQ(test_escaped, "\\\"myvalue\\\":10");
    EXPECT_EQ(test,test_unescaped);


    test = "\" \\ \n \t \b \f \r /";
    test_escaped   = utils::escape_special_chars(test);
    test_unescaped = utils::unescape_special_chars(test_escaped);

    CONDUIT_INFO( test << "\nvs\n" << test_escaped);

    EXPECT_EQ(test_escaped, "\\\" \\\\ \\n \\t \\b \\f \\r /");
    EXPECT_EQ(test,test_unescaped);
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, float64_to_string)
{
    float64 v = 10.0;

    EXPECT_EQ("10.0",utils::float64_to_string(v));

    v = 10000000000000000;
    EXPECT_EQ("1e+16",utils::float64_to_string(v));

    v = std::numeric_limits<float64>::infinity();
    CONDUIT_INFO(utils::float64_to_string(v));

    EXPECT_EQ("inf",utils::float64_to_string(v));

    v = -std::numeric_limits<float64>::infinity();
    CONDUIT_INFO(utils::float64_to_string(v));

    EXPECT_EQ("-inf",utils::float64_to_string(v));

    v = std::numeric_limits<float64>::quiet_NaN();
    CONDUIT_INFO(utils::float64_to_string(v));

    EXPECT_EQ("nan",utils::float64_to_string(v));
}



//-----------------------------------------------------------------------------
TEST(conduit_utils, is_dir)
{
    EXPECT_TRUE(utils::is_directory(CONDUIT_T_SRC_DIR));
    EXPECT_TRUE(utils::is_directory(CONDUIT_T_BIN_DIR));

    EXPECT_FALSE(utils::is_directory("asdasdasdasd"));
}


//-----------------------------------------------------------------------------
TEST(conduit_utils, is_file)
{

    std::string tf_path = utils::join_file_path(CONDUIT_T_SRC_DIR,
                                                "conduit");

    tf_path = utils::join_file_path(tf_path,"t_conduit_utils.cpp");

    EXPECT_TRUE(utils::is_file(tf_path));

    EXPECT_FALSE(utils::is_file(CONDUIT_T_SRC_DIR));
    EXPECT_FALSE(utils::is_file(CONDUIT_T_BIN_DIR));

    EXPECT_FALSE(utils::is_file("asdasdasdasd"));
}



//-----------------------------------------------------------------------------
TEST(conduit_utils, remove_file)
{
    std::ofstream ofs;

    ofs.open("t_remove_file.txt");
    ofs << "here" << std::endl;
    ofs.close();

    EXPECT_TRUE(utils::is_file("t_remove_file.txt"));

    utils::remove_file("t_remove_file.txt");

    EXPECT_FALSE(utils::is_file("t_remove_file.txt"));
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, system_exec)
{
#if !defined(CONDUIT_PLATFORM_WINDOWS)
    EXPECT_EQ(utils::system_execute("pwd"),0);
#else
    EXPECT_EQ(utils::system_execute("echo %cd%"), 0);
#endif
}



//-----------------------------------------------------------------------------
TEST(conduit_utils, base64_enc_dec)
{
    Node n_src;
    n_src["a"].set_int32(10);
    n_src["b"].set_int32(20);
    n_src["c"].set_int32(30);

    // we need compact data for base64
    Node n;
    n_src.compact_to(n);

    // use libb64 to encode the data
    index_t nbytes = n.schema().total_strided_bytes();
    Node bb64_data;
    index_t enc_buff_size = utils::base64_encode_buffer_size(nbytes);

    bb64_data.set(DataType::char8_str(enc_buff_size));

    const char *src_ptr = (const char*)n.data_ptr();
    char *dest_ptr      = (char*)bb64_data.data_ptr();
    memset(dest_ptr,0,(size_t)enc_buff_size);

    utils::base64_encode(src_ptr,nbytes,dest_ptr);

    index_t dec_buff_size = utils::base64_decode_buffer_size(enc_buff_size);

    // use libb64 to decode the data

    // decode buffer
    Node bb64_decode;
    bb64_decode.set(DataType::char8_str(dec_buff_size));
    char *b64_decode_ptr = (char*)bb64_decode.data_ptr();
    memset(b64_decode_ptr,0,(size_t)dec_buff_size);

    utils::base64_decode(bb64_data.as_char8_str(),
                         enc_buff_size,
                         b64_decode_ptr);

    // apply schema
    Node n_res(n.schema(),b64_decode_ptr,false);

    n.print();
    n_res.print();

    // check we have the same values
    EXPECT_EQ(n_src["a"].as_int32(), n_res["a"].as_int32());
    EXPECT_EQ(n_src["b"].as_int32(), n_res["b"].as_int32());
    EXPECT_EQ(n_src["c"].as_int32(), n_res["c"].as_int32());
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, dir_create_and_remove_tests)
{
    std::string test_dir = utils::join_file_path(CONDUIT_T_BIN_DIR,
                                                 "tout_dir_create_test");

    CONDUIT_INFO("test creating and removing dir: " << test_dir);

    EXPECT_FALSE(utils::is_directory(test_dir));

    EXPECT_TRUE(utils::create_directory(test_dir));

    EXPECT_TRUE(utils::is_directory(test_dir));

    EXPECT_TRUE(utils::remove_directory(test_dir));

    EXPECT_FALSE(utils::is_directory(test_dir));
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, file_path_split_tests)
{

    std::string sep = utils::file_path_separator();

#ifdef CONDUIT_PLATFORM_WINDOWS
    EXPECT_EQ(sep,"\\");
#else
    EXPECT_EQ(sep,"/");
#endif


    std::string my_path = "a" +  sep + "b" + sep + "c";

    std::string my_path_via_join = utils::join_file_path("a","b");
    my_path_via_join = utils::join_file_path(my_path_via_join,"c");

    EXPECT_EQ(my_path,my_path_via_join);

    std::string curr,next;
    utils::split_file_path(my_path,curr,next);
    EXPECT_EQ(curr,"a");
    EXPECT_EQ(next,"b" + sep + "c");

    my_path = next;
    utils::split_file_path(my_path,curr,next);
    EXPECT_EQ(curr,"b");
    EXPECT_EQ(next,"c");

    my_path = next;
    utils::split_file_path(my_path,curr,next);
    EXPECT_EQ(curr,"c");
    EXPECT_EQ(next,"");


    my_path = "a" +  sep + "b" + sep + "c";

    utils::rsplit_file_path(my_path,curr,next);
    EXPECT_EQ(curr,"c");
    EXPECT_EQ(next,"a" + sep + "b");

    my_path = next;
    utils::rsplit_file_path(my_path,curr,next);
    EXPECT_EQ(curr,"b");
    EXPECT_EQ(next,"a");

    my_path = next;
    utils::rsplit_file_path(my_path,curr,next);
    EXPECT_EQ(curr,"a");
    EXPECT_EQ(next,"");

}


//-----------------------------------------------------------------------------
TEST(conduit_utils, join_path_tests)
{
    // note: these test joining conduit paths, not file system paths


    std::string res = utils::join_path("","mypath");
    EXPECT_EQ(res,"mypath");

    res = utils::join_path("mypath","");
    EXPECT_EQ(res,"mypath");

    res = utils::join_path("","");
    EXPECT_EQ(res,"");

    res = utils::join_path("here/","mypath");
    EXPECT_EQ(res,"here/mypath");

    res = utils::join_path("/","mypath");
    EXPECT_EQ(res,"/mypath");

    res = utils::join_path("/here","mypath");
    EXPECT_EQ(res,"/here/mypath");

}


//-----------------------------------------------------------------------------
TEST(conduit_utils, split_windows_paths)
{
    std::string curr, next;
    utils::split_file_path("D:\\",
                           std::string(":"),
                           curr, next);

    EXPECT_EQ(curr,std::string("D:\\"));
    EXPECT_EQ(next,std::string(""));

    utils::split_file_path("D:\\test\\some\\path",
                           std::string(":"),
                           curr, next);

    EXPECT_EQ(curr,std::string("D:\\test\\some\\path"));
    EXPECT_EQ(next,std::string(""));

    utils::split_file_path("D:\\test\\some\\path:subpath",
                           std::string(":"),
                           curr, next);

    EXPECT_EQ(curr,std::string("D:\\test\\some\\path"));
    EXPECT_EQ(next,std::string("subpath"));

    utils::split_file_path(std::string(next),
                           std::string(":"),
                           curr, next);

    EXPECT_EQ(curr,std::string("subpath"));
    EXPECT_EQ(next,std::string(""));



    // drive letter needs '\\', so check corner case where its
    // not there, which should be treat as non-windows case
    utils::split_file_path("a:subpath",
                           std::string(":"),
                           curr, next);

    EXPECT_EQ(curr,std::string("a"));
    EXPECT_EQ(next,std::string("subpath"));

    utils::split_file_path(std::string(next),
                           std::string(":"),
                           curr, next);

    EXPECT_EQ(curr,std::string("subpath"));
    EXPECT_EQ(next,std::string(""));


    // rsplit tests
    //
    utils::rsplit_file_path("D:\\",
                            std::string(":"),
                            curr,next);

    EXPECT_EQ(curr,std::string("D:\\"));
    EXPECT_EQ(next,std::string(""));


    utils::rsplit_file_path("D:\\test\\some\\path",
                            std::string(":"),
                            curr,next);

    EXPECT_EQ(curr,std::string("D:\\test\\some\\path"));
    EXPECT_EQ(next,std::string(""));

    utils::rsplit_file_path("D:\\test\\some\\path:subpath",
                            std::string(":"),
                            curr,next);

    EXPECT_EQ(curr,std::string("subpath"));
    EXPECT_EQ(next,std::string("D:\\test\\some\\path"));


    // drive letter needs '\\', so check corner case where its
    // not there, which should be treat as non-windows case
    utils::rsplit_file_path("a:subpath",
                            std::string(":"),
                            curr,next);

    EXPECT_EQ(curr,std::string("subpath"));
    EXPECT_EQ(next,std::string("a"));

}

//-----------------------------------------------------------------------------
TEST(conduit_utils, split_tests)
{
    // forward split
    // expect a,b,c then empty
    std::string t("a b c");

    std::string p = t;

    std::string curr;
    std::string next;

    utils::split_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string("a"));
    p = next;

    utils::split_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string("b"));
    p = next;

    utils::split_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string("c"));
    p = next;

    utils::split_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string(""));
    p = next;

    // reverse split
    // expect c,b,a then empty
    p = t;

    curr = "";
    next = "";

    utils::rsplit_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string("c"));
    p = next;

    utils::rsplit_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string("b"));
    p = next;

    utils::rsplit_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string("a"));
    p = next;

    utils::rsplit_string(p," ",curr,next);
    EXPECT_EQ(curr,std::string(""));
    p = next;

}

//-----------------------------------------------------------------------------
TEST(conduit_utils, trim_string)
{
    const std::string ans = "trimmed";
    std::string leading = "  trimmed";
    std::string leading2 = "\t trimmed";
    std::string trailing = "trimmed   ";
    std::string trailing2 = "trimmed\r\n";
    std::string both =  "  trimmed ";
    std::string both2 = "\f\ttrimmed\v ";

    utils::trim_string(leading);
    EXPECT_EQ(ans, leading);
    utils::trim_string(leading2);
    EXPECT_EQ(ans, leading2);
    utils::trim_string(trailing);
    EXPECT_EQ(ans, trailing);
    utils::trim_string(trailing2);
    EXPECT_EQ(ans, trailing2);
    utils::trim_string(both);
    EXPECT_EQ(ans, both);
    utils::trim_string(both2);
    EXPECT_EQ(ans, both2);
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, format_args)
{
    Node args;

    // args not an obj or list, throw exception
    {
        args.reset();
        EXPECT_THROW(conduit::utils::format("{a} {b} {c} {d} {e} {f}",args),
                     conduit::Error);

        args = 32;
        EXPECT_THROW(conduit::utils::format("{a} {b} {c} {d} {e} {f}",args),
                     conduit::Error);
    }

    // named args test
    args["a"] = "something about";
    args["b"] = "and";
    args["c"] = 3.1415;
    args["d"] = 42;

    std::string t = conduit::utils::format("{a} {c:0.3} {b} {d:04}", args);
    std::cout << t << std::endl;

    EXPECT_EQ("something about 3.14 and 0042",t);

    // not enough args to exec format, throw exception
    EXPECT_THROW(conduit::utils::format("{a} {b} {c} {d} {e} {f}",args),
                 conduit::Error);

    // ordered args test
    args.reset();
    args.append() = "something about";
    args.append() = 3.1415;
    args.append() = "and";
    args.append() = 42;

    t = conduit::utils::format("{} {:0.3} {} {:04}", args);
    std::cout << t << std::endl;
    EXPECT_EQ("something about 3.14 and 0042",t);


    // mis match args to format req, throw exception
    args.reset();
    args.append() = "something about";
    args.append() = "and";
    args.append() = 3.1415;
    args.append() = 42;

    EXPECT_THROW(conduit::utils::format("{} {:0.3} {} {:04}", args),
                 conduit::Error);

    // not enough args to exec format, throw exception
    EXPECT_THROW(conduit::utils::format("{} {} {} {} {} {}",args),
                 conduit::Error);

}


// //-----------------------------------------------------------------------------
TEST(conduit_utils, format_maps)
{
    Node maps;

    // args not an obj or list, throw exception
    {
        maps.reset();
        EXPECT_THROW(conduit::utils::format("{a} {b} {c} {d} {e} {f}",
                                            maps,
                                            0),
                     conduit::Error);

        maps = 32;
        EXPECT_THROW(conduit::utils::format("{a} {b} {c} {d} {e} {f}",
                                            maps,
                                            0),
                     conduit::Error);
    }

    // named args test
    maps["a"] = {1.1415, 2.1415, 3.1415};
    maps["b"] = {0,0,42};

    std::string t = conduit::utils::format("something about {a:0.3} and {b:04}",
                                           maps,
                                           2);
    std::cout << t << std::endl;
    EXPECT_EQ("something about 3.14 and 0042",t);


    // bad map index, (too small)
    {
        EXPECT_THROW(conduit::utils::format("{a} {b} {c}",maps,-100),
                     conduit::Error);

        EXPECT_THROW(conduit::utils::format("{a} {b} {c}",maps,-1),
                     conduit::Error);
    }

    // bad map index, (too big)
    {
        EXPECT_THROW(conduit::utils::format("{a} {b} {c}",maps,4),
                     conduit::Error);

        EXPECT_THROW(conduit::utils::format("{a} {b} {c}",maps,100),
                     conduit::Error);
    }


    // not enough args to exec format, throw exception
    EXPECT_THROW(conduit::utils::format("{a} {b} {c}",maps,2),
                 conduit::Error);

    // // ordered args test
    maps.reset();
    maps.append() = {1.1415, 2.1415, 3.1415};
    maps.append() = {0,0,42};

    t = conduit::utils::format("something about {:0.3} and {:04}",
                               maps,
                               2);
    std::cout << t << std::endl;
    EXPECT_EQ("something about 3.14 and 0042",t);

    // string list case
    maps.reset();
    Node &slist = maps.append();

    slist.append() = "hi 0";
    slist.append() = "hi 1";
    slist.append() = "hi 2";

    t = conduit::utils::format("{} is it",maps,0);
    std::cout << t << std::endl;
    EXPECT_EQ("hi 0 is it",t);

    t = conduit::utils::format("{} is it",maps,1);
    std::cout << t << std::endl;
    EXPECT_EQ("hi 1 is it",t);

    t = conduit::utils::format("{} is it",maps,2);
    std::cout << t << std::endl;
    EXPECT_EQ("hi 2 is it",t);

    // not enough args to exec format, throw exception
    EXPECT_THROW(conduit::utils::format("{} {} {} {} {} {}",maps),
                 conduit::Error);

}

//-----------------------------------------------------------------------------
TEST(conduit_utils, value_fits)
{
    uint64 ui64v_max = std::numeric_limits<uint64>::max();
    uint64 ui64v_min = std::numeric_limits<uint64>::min();
    uint64 ui64v_low = std::numeric_limits<uint64>::lowest();

    int64 i64v_max = std::numeric_limits<int64>::max();
    int64 i64v_min = std::numeric_limits<int64>::min();
    int64 i64v_low = std::numeric_limits<int64>::lowest();


    int8 i8_val_pos  = 1;
    int8 i8_val_neg  = -1;
    int8 i8_val_zero = 0;


    uint8 ui8_val_pos  = 1;
    uint8 ui8_val_zero = 0;


    // signed too big into signed
    EXPECT_FALSE( (conduit::utils::value_fits<int64,int8>(i64v_max) ) );
    EXPECT_FALSE( (conduit::utils::value_fits<int64,int8>(i64v_min) ) );
    EXPECT_FALSE( (conduit::utils::value_fits<int64,int8>(i64v_low) ) );

    // unsigned into signed
    EXPECT_FALSE( (conduit::utils::value_fits<uint64,int8>(ui64v_max) ) );
    // zero into signed
    EXPECT_TRUE(  (conduit::utils::value_fits<uint64,int8>(ui64v_min) ) );
    EXPECT_TRUE( (conduit::utils::value_fits<uint64,int8>(ui64v_low) ) );

    // unsigned too big to unsigned
    EXPECT_FALSE( (conduit::utils::value_fits<uint64,uint8>(ui64v_max) ) );
    // unsigned min and lower into unsigned
    EXPECT_TRUE(  (conduit::utils::value_fits<uint64,uint8>(ui64v_min) ) );
    EXPECT_TRUE(  (conduit::utils::value_fits<uint64,uint8>(ui64v_low) ) );

    // signed small into unsigned big
    // pos number ok
    EXPECT_TRUE( (conduit::utils::value_fits<int8,uint64>(i8_val_pos) ) );
    // zero -
    EXPECT_TRUE( (conduit::utils::value_fits<int8,uint64>(ui8_val_zero) ) );
    // neg number won't work
    EXPECT_FALSE( (conduit::utils::value_fits<int8,uint64>(i8_val_neg) ) );

    EXPECT_TRUE( (conduit::utils::value_fits<uint8,int64>(ui8_val_pos) ) );
    EXPECT_TRUE( (conduit::utils::value_fits<uint8,int64>(ui8_val_zero) ) );

}

//-----------------------------------------------------------------------------
TEST(conduit_utils, to_and_from_hex_string)
{
    int64 val = 1024;
    std::string hstring = utils::to_hex_string(val);
    std::cout << hstring << std::endl;
    int64 val_check = conduit::utils::hex_string_to_value<int64>(hstring);
    EXPECT_EQ(val,val_check);
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, timer)
{
    conduit::utils::Timer t;
    // sleep 1/4 a second
    conduit::utils::sleep(250);
    // check that at least 1/4 a second has elapsed
    EXPECT_TRUE(t.elapsed() > .249);
}

//-----------------------------------------------------------------------------
TEST(conduit_utils, memory_usage)
{
    index_t prev_usage = conduit::utils::memory_usage();

    // alloc 5 mB
    Node n;
    n.set(DataType::uint8(5*1024*1024));
    //  touch memory to make sure it is actually used
    uint8_array vals = n.value();
    for(index_t i=0;i<vals.number_of_elements();i++)
    {
        vals[i] = i % 256;
    }

    index_t curr_usage = conduit::utils::memory_usage();

    std::cout << "memory usage compare - prev: " << prev_usage << std::endl;
    std::cout << "memory usage compare - curr: " << curr_usage << std::endl;

    EXPECT_TRUE(curr_usage >  prev_usage);
}

