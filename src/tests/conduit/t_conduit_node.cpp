// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_node.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"
using namespace conduit;

//-----------------------------------------------------------------------------
TEST(conduit_node, simple)
{

    uint32   a_val  = 10;
    uint32   b_val  = 20;
    float64  c_val  = 30.0;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;

    EXPECT_EQ(n["a"].as_uint32(),a_val);
    EXPECT_EQ(n["b"].as_uint32(),b_val);
    EXPECT_EQ(n["c"].as_float64(),c_val);
    // data should be owned by the conduit Node
    EXPECT_FALSE(n["a"].is_data_external());

}

TEST(conduit_node, nested)
{

    uint32   val  = 10;

    Node n;
    n["a"]["b"] = val;
    EXPECT_EQ(n["a"]["b"].as_uint32(),val);
}

TEST(conduit_node, pathlike_child_name)
{

    uint32   path_val  = 10;
    uint32   direct_val  = 20;

    Node n;
    n["a/b"] = path_val;
    n.add_child("a/b") = direct_val;

    EXPECT_EQ(n["a/b"].as_uint32(),path_val);
    EXPECT_EQ(n.child("a/b").as_uint32(),direct_val);

    uint32   deletion_test_val = 35;
    n["c/d"] = deletion_test_val;
    n.add_child("c/d") = direct_val;
    n.remove("a/b");
    n.remove_child("c/d");

    EXPECT_EQ(n.child("a/b").as_uint32(),direct_val);
    EXPECT_EQ(n["c/d"].as_uint32(),deletion_test_val);
}

//-----------------------------------------------------------------------------
TEST(conduit_node, vector)
{

    std::vector<uint32> vec;
    for(int i=0;i<100;i++)
        vec.push_back(i);

    Node n;
    n["a"]= vec;
    EXPECT_EQ(n["a"].as_uint32_ptr()[99],99);
}

//-----------------------------------------------------------------------------
TEST(conduit_node, list)
{

    std::vector<uint32> vec;
    for(int i=0;i<100;i++)
        vec.push_back(i);

    Node n;
    Node& list = n["mylist"];
    uint32   a_val  = 10;
    uint32   b_val  = 20;
    float64  c_val  = 30.0;
    list.append().set(a_val);
    list.append().set(b_val);
    list.append().set(c_val);
    list.append().set(vec);
    EXPECT_EQ(list[0].as_uint32(),a_val);
    EXPECT_EQ(list[1].as_uint32(),b_val);
    EXPECT_EQ(list[2].as_float64(),c_val);
    EXPECT_EQ(list[3].as_uint32_ptr()[99],99);

    EXPECT_EQ(n["mylist"][0].as_uint32(),a_val);
    EXPECT_EQ(n["mylist"][1].as_uint32(),b_val);
    EXPECT_EQ(n["mylist"][2].as_float64(),c_val);
    EXPECT_EQ(n["mylist"][3].as_uint32_ptr()[99],99);

}

//-----------------------------------------------------------------------------
TEST(conduit_node, simple_schema_gen )
{
    uint32   a_val  = 10;
    uint32   b_val  = 20;
    float64  c_val  = 30.0;

    char *data = new char[16];
    memcpy(&data[0],&a_val,4);
    memcpy(&data[4],&b_val,4);
    memcpy(&data[8],&c_val,8);

    Schema schema("{\"a\":\"uint32\",\"b\":\"uint32\",\"c\":\"float64\"}");
    Node n(schema,data,true);

    EXPECT_EQ(n["a"].as_uint32(),a_val);
    EXPECT_EQ(n["b"].as_uint32(),b_val);
    EXPECT_EQ(n["c"].as_float64(),c_val);

    std::string s2_str = "{\"g\": {\"a\":\"uint32\",\"b\":\"uint32\",\"c\":\"float64\"}}";
    std::cout << s2_str << std::endl;
    Schema schema2(s2_str);

    Node n2(schema2,data,true);
    EXPECT_EQ(n2["g"]["a"].as_uint32(),a_val);
    EXPECT_EQ(n2["g"]["b"].as_uint32(),b_val);
    EXPECT_EQ(n2["g"]["c"].as_float64(),c_val);

    Schema schema3("{\"dtype\":\"uint32\",\"length\": 5}");
    uint32 *data2 = new uint32[5];
    for (int i = 0; i < 5; i++) {
       data2[i] = i * 5;
    }
    Node n3(schema3,data2,true);
    for (int i = 0; i < 5; i++) {
       EXPECT_EQ(n3.as_uint32_ptr()[i], i*5);
    }
    Schema schema4("[\"uint32\", \"float64\", \"uint32\"]");
    char* data3 = new char[16];
    memcpy(&data3[0],&a_val,4);
    memcpy(&data3[4],&c_val,8);
    memcpy(&data3[12],&b_val,4);
    Node n4(schema4,data3,true);
    EXPECT_EQ(n4[0].as_uint32(),a_val);
    EXPECT_EQ(n4[1].as_float64(),c_val);
    EXPECT_EQ(n4[2].as_uint32(),b_val);

    Schema schema5("{\"top\":[{\"int1\":\"uint32\", \"int2\":\"uint32\"}, \"float64\", \"uint32\"], \"other\":\"float64\"}");
    char* data4 = new char[28];
    uint32   d_val  = 40;
    float64  e_val  = 50.0;
    memcpy(&data4[0],&a_val,4);
    memcpy(&data4[4],&b_val,4);
    memcpy(&data4[8],&c_val,8);
    memcpy(&data4[16],&d_val,4);
    memcpy(&data4[20],&e_val,8);
    Node n5(schema5,data4,true);

    std::cout << n5.schema().to_json() << std::endl;
    EXPECT_EQ(n5["top"][0]["int1"].as_uint32(),a_val);
    EXPECT_EQ(n5["top"][0]["int2"].as_uint32(),b_val);
    EXPECT_EQ(n5["top"][1].as_float64(),c_val);
    EXPECT_EQ(n5["top"][2].as_uint32(),d_val);
    EXPECT_EQ(n5["other"].as_float64(),e_val);

    delete [] data;
    delete [] data2;
    delete [] data3;
    delete [] data4;
}


//-----------------------------------------------------------------------------
TEST(conduit_node, simple_schema)
{
    uint32   a_val  = 10;
    uint32   b_val  = 20;
    float64  c_val  = 30.0;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;
    n["here"]["there"] = c_val;

    std::string res = n.schema().to_json();
    std::cout << res;
    // parse schema output to check for children
    Node n_load;
    n_load.parse(res,"json");
    
    EXPECT_TRUE(n_load.has_child("a"));
    EXPECT_TRUE(n_load.has_child("b"));
    EXPECT_TRUE(n_load.has_child("c"));
}

//-----------------------------------------------------------------------------
TEST(conduit_node, simple_schema_parent)
{
    uint32   a_val  = 10;
    uint32   b_val  = 20;
    float64  c_val  = 30.0;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;
    n["here"]["there"] = c_val;

    EXPECT_TRUE(n.schema().is_root());
    Node & na = n["a"];
    const Schema &na_schema =na.schema();
    EXPECT_FALSE(na_schema.is_root());

}


//-----------------------------------------------------------------------------
TEST(conduit_node, in_place)
{
    uint32   a_val  = 10;
    uint32   b_val  = 20;
    float64  c_val  = 30.0;
    float64  d_val  = 40.0;

    char *data = new char[16];
    memcpy(&data[0],&a_val,4);
    memcpy(&data[4],&b_val,4);
    memcpy(&data[8],&c_val,8);
    EXPECT_EQ(*(float64*)(&data[8]), c_val);

    Schema schema("{\"a\":\"uint32\",\"b\":\"uint32\",\"c\":\"float64\"}");
    Node n(schema,data,true);
    n["a"] = b_val;
    n["b"] = a_val;
    n["c"] = d_val;

    // data shouldn't be owned by the conduit Node
    EXPECT_TRUE(n["a"].is_data_external());

    EXPECT_EQ(n["a"].as_uint32(), b_val);
    EXPECT_EQ(n["b"].as_uint32(), a_val);
    EXPECT_EQ(n["c"].as_float64(), d_val);

    EXPECT_EQ(*(uint32*)(&data[0]), b_val);
    EXPECT_EQ(*(uint32*)(&data[4]), a_val);
    EXPECT_EQ(*(float64*)(&data[8]), d_val);

    delete [] data;
}

//-----------------------------------------------------------------------------
TEST(conduit_node, remove_by_name)
{
    auto do_remove_by_name_test = [](Node &n)
    {
        n.print();
        EXPECT_TRUE(n.has_path("a"));
        EXPECT_TRUE(n.has_path("b"));
        EXPECT_TRUE(n.has_path("c"));
        n.remove("a");
        n.print();
        EXPECT_FALSE(n.has_path("a"));
        EXPECT_TRUE(n.has_path("b"));
        EXPECT_TRUE(n.has_path("c"));
        n.remove("c");
        n.print();
        EXPECT_FALSE(n.has_path("a"));
        EXPECT_TRUE(n.has_path("b"));
        EXPECT_FALSE(n.has_path("c"));
        n.remove("b");
        n.print();
        EXPECT_FALSE(n.has_path("a"));
        EXPECT_FALSE(n.has_path("b"));
        EXPECT_FALSE(n.has_path("c"));
    };
    // JSON
    {
        conduit::Generator g("{a:1,b:2,c:3}", "json");
        conduit::Node n(g,true);
        do_remove_by_name_test(n);
    }
    // YAML
    {
        conduit::Generator g("a: 1\nb: 2\nc: 3", "yaml");
        conduit::Node n(g,true);
        do_remove_by_name_test(n);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_node, remove_by_index)
{
    auto do_remove_by_index_test1 = [](Node &n)
    {
        n.print();
        EXPECT_TRUE(n.has_path("a"));
        EXPECT_TRUE(n.has_path("b"));
        EXPECT_TRUE(n.has_path("c"));
        n.remove(0);
        n.print();
        EXPECT_FALSE(n.has_path("a"));
        EXPECT_TRUE(n.has_path("b"));
        EXPECT_TRUE(n.has_path("c"));
        n.remove(1);
        n.print();
        EXPECT_FALSE(n.has_path("a"));
        EXPECT_TRUE(n.has_path("b"));
        EXPECT_FALSE(n.has_path("c"));
        n.remove(0);
        n.print();
        EXPECT_FALSE(n.has_path("a"));
        EXPECT_FALSE(n.has_path("b"));
        EXPECT_FALSE(n.has_path("c"));
    };
    auto do_remove_by_index_test2 = [](Node &n2)
    {
        n2.print();
        n2.remove(1);
        n2.print();
        EXPECT_EQ(n2[0].to_uint64(), 10);
        EXPECT_EQ(n2[1].to_uint64(), 30);
        n2.remove(0);
        n2.print();
        EXPECT_EQ(n2[0].to_uint64(), 30);
    };
    // JSON
    {
        conduit::Generator g("{a:1,b:2,c:3}", "json");
        conduit::Node n(g,true);
        do_remove_by_index_test1(n);
        conduit::Generator g2("[{dtype:int64, value: 10},{dtype:int64, value: 20},{dtype:int64, value: 30}]");
        conduit::Node n2(g2,true);
        do_remove_by_index_test2(n2);        
    }
    // YAML
    {
        conduit::Generator g("a: 1\nb: 2\nc: 3", "yaml");
        conduit::Node n(g,true);
        do_remove_by_index_test1(n);
        conduit::Generator g2("- \n  dtype: \"int64\"\n  value: 10\n- \n  dtype: \"int64\"\n  value: 20\n- \n  dtype: \"int64\"\n  value: 30", "conduit_yaml");
        conduit::Node n2(g2,true);
        do_remove_by_index_test2(n2);        
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_node, check_leaf_assert)
{

    conduit::Node n;
    int16 v = 64;
    n["v"] = v;
    n.print();
    EXPECT_THROW(n["v"].as_int8(),conduit::Error);
}

//-----------------------------------------------------------------------------
TEST(conduit_node, check_value_implict_c_type_cast)
{
    conduit::Node n;

    char  cv = 1;
    short sv = 2;
    int   iv = 3;
    long  lv = 4;

    signed char  scv = -1;
    signed short ssv = -2;
    signed int   siv = -3;
    signed long  slv = -4;

    unsigned char  ucv = 1;
    unsigned short usv = 2;
    unsigned int   uiv = 3;
    unsigned long  ulv = 4;

    float  fv = 1.2f;
    double dv = 2.4;

    n["cv"] = cv;
    n["sv"] = sv;
    n["iv"] = iv;
    n["lv"] = lv;

    n["scv"] = scv;
    n["ssv"] = ssv;
    n["siv"] = siv;
    n["slv"] = slv;

    n["ucv"] = ucv;
    n["usv"] = usv;
    n["uiv"] = uiv;
    n["ulv"] = ulv;

    n["fv"] = fv;
    n["dv"] = dv;


    n.print();

    char  cv_r = n["cv"].value();
    short sv_r = n["sv"].value();
    int   iv_r = n["iv"].value();
    long  lv_r = n["lv"].value();


    EXPECT_EQ(cv,cv_r);
    EXPECT_EQ(sv,sv_r);
    EXPECT_EQ(iv,iv_r);
    EXPECT_EQ(lv,lv_r);

    signed char  scv_r = n["scv"].value();
    signed short ssv_r = n["ssv"].value();
    signed int   siv_r = n["siv"].value();
    signed long  slv_r = n["slv"].value();

    EXPECT_EQ(scv,scv_r);
    EXPECT_EQ(ssv,ssv_r);
    EXPECT_EQ(siv,siv_r);
    EXPECT_EQ(slv,slv_r);

    unsigned char  ucv_r = n["ucv"].value();
    unsigned short usv_r = n["usv"].value();
    unsigned int   uiv_r = n["uiv"].value();
    unsigned long  ulv_r = n["ulv"].value();

    EXPECT_EQ(ucv,ucv_r);
    EXPECT_EQ(usv,usv_r);
    EXPECT_EQ(uiv,uiv_r);
    EXPECT_EQ(ulv,ulv_r);


    float  fv_r = n["fv"].value();
    double dv_r = n["dv"].value();

    EXPECT_NEAR(fv,fv_r,0.001);
    EXPECT_NEAR(dv,dv_r,0.001);


    char  *cv_p_r = n["cv"].value();
    short *sv_p_r = n["sv"].value();
    int   *iv_p_r = n["iv"].value();
    long  *lv_p_r = n["lv"].value();

    EXPECT_EQ(cv,cv_p_r[0]);
    EXPECT_EQ(sv,sv_p_r[0]);
    EXPECT_EQ(iv,iv_p_r[0]);
    EXPECT_EQ(lv,lv_p_r[0]);

    signed char  *scv_p_r = n["scv"].value();
    signed short *ssv_p_r = n["ssv"].value();
    signed int   *siv_p_r = n["siv"].value();
    signed long  *slv_p_r = n["slv"].value();

    EXPECT_EQ(scv,scv_p_r[0]);
    EXPECT_EQ(ssv,ssv_p_r[0]);
    EXPECT_EQ(siv,siv_p_r[0]);
    EXPECT_EQ(slv,slv_p_r[0]);


    unsigned char  *ucv_p_r = n["ucv"].value();
    unsigned short *usv_p_r = n["usv"].value();
    unsigned int   *uiv_p_r = n["uiv"].value();
    unsigned long  *ulv_p_r = n["ulv"].value();

    EXPECT_EQ(ucv,ucv_p_r[0]);
    EXPECT_EQ(usv,usv_p_r[0]);
    EXPECT_EQ(uiv,uiv_p_r[0]);
    EXPECT_EQ(ulv,ulv_p_r[0]);


    float  *fv_p_r = n["fv"].value();
    double *dv_p_r = n["dv"].value();

    EXPECT_NEAR(fv,fv_p_r[0],0.001);
    EXPECT_NEAR(dv,dv_p_r[0],0.001);


    char_array  cv_a_r = n["cv"].value();
    short_array sv_a_r = n["sv"].value();
    int_array   iv_a_r = n["iv"].value();
    long_array  lv_a_r = n["lv"].value();

    EXPECT_EQ(cv_a_r.number_of_elements(),1);
    EXPECT_EQ(sv_a_r.number_of_elements(),1);
    EXPECT_EQ(iv_a_r.number_of_elements(),1);
    EXPECT_EQ(lv_a_r.number_of_elements(),1);

    EXPECT_EQ(cv,cv_a_r[0]);
    EXPECT_EQ(sv,sv_a_r[0]);
    EXPECT_EQ(iv,iv_a_r[0]);
    EXPECT_EQ(lv,lv_a_r[0]);

    signed_char_array  scv_a_r = n["scv"].value();
    signed_short_array ssv_a_r = n["ssv"].value();
    signed_int_array   siv_a_r = n["siv"].value();
    signed_long_array  slv_a_r = n["slv"].value();

    EXPECT_EQ(scv_a_r.number_of_elements(),1);
    EXPECT_EQ(ssv_a_r.number_of_elements(),1);
    EXPECT_EQ(siv_a_r.number_of_elements(),1);
    EXPECT_EQ(slv_a_r.number_of_elements(),1);

    EXPECT_EQ(scv,scv_a_r[0]);
    EXPECT_EQ(ssv,ssv_a_r[0]);
    EXPECT_EQ(siv,siv_a_r[0]);
    EXPECT_EQ(slv,slv_a_r[0]);

    unsigned_char_array  ucv_a_r = n["ucv"].value();
    unsigned_short_array usv_a_r = n["usv"].value();
    unsigned_int_array   uiv_a_r = n["uiv"].value();
    unsigned_long_array  ulv_a_r = n["ulv"].value();

    EXPECT_EQ(ucv_a_r.number_of_elements(),1);
    EXPECT_EQ(usv_a_r.number_of_elements(),1);
    EXPECT_EQ(uiv_a_r.number_of_elements(),1);
    EXPECT_EQ(ulv_a_r.number_of_elements(),1);

    EXPECT_EQ(ucv,ucv_a_r[0]);
    EXPECT_EQ(usv,usv_a_r[0]);
    EXPECT_EQ(uiv,uiv_a_r[0]);
    EXPECT_EQ(ulv,ulv_a_r[0]);


    float_array  fv_a_r = n["fv"].value();
    double_array dv_a_r = n["dv"].value();

    EXPECT_EQ(fv_a_r.number_of_elements(),1);
    EXPECT_EQ(dv_a_r.number_of_elements(),1);


    EXPECT_NEAR(fv,fv_a_r[0],0.001);
    EXPECT_NEAR(dv,dv_a_r[0],0.001);

}


//-----------------------------------------------------------------------------
TEST(conduit_node, check_value_implict_bitwidth_type_cast)
{
    conduit::Node n;

    int8  i8v  = -1;
    int16 i16v = -2;
    int32 i32v = -3;
    int64 i64v = -4;

    uint8  ui8v  = 1;
    uint16 ui16v = 2;
    uint32 ui32v = 3;
    uint64 ui64v = 4;

    float32 f32v = 1.2f;
    float64 f64v = 2.4;

    n["i8v"]  = i8v;
    n["i16v"] = i16v;
    n["i32v"] = i32v;
    n["i64v"] = i64v;

    n["ui8v"]  = ui8v;
    n["ui16v"] = ui16v;
    n["ui32v"] = ui32v;
    n["ui64v"] = ui64v;

    n["f32v"] = f32v;
    n["f64v"] = f64v;


    n.print();

    int8  i8v_r  = n["i8v"].value();
    int16 i16v_r = n["i16v"].value();
    int32 i32v_r = n["i32v"].value();
    int64 i64v_r = n["i64v"].value();

    EXPECT_EQ(i8v,i8v_r);
    EXPECT_EQ(i16v,i16v_r);
    EXPECT_EQ(i32v,i32v_r);
    EXPECT_EQ(i64v,i64v_r);

    uint8  ui8v_r  = n["ui8v"].value();
    uint16 ui16v_r = n["ui16v"].value();
    uint32 ui32v_r = n["ui32v"].value();
    uint64 ui64v_r = n["ui64v"].value();

    EXPECT_EQ(ui8v,ui8v_r);
    EXPECT_EQ(ui16v,ui16v_r);
    EXPECT_EQ(ui32v,ui32v_r);
    EXPECT_EQ(ui64v,ui64v_r);


    float32 f32v_r = n["f32v"].value();
    float64 f64v_r = n["f64v"].value();

    EXPECT_NEAR(f32v,f32v_r,0.001);
    EXPECT_NEAR(f64v,f64v_r,0.001);


    int8  *i8v_p_r  = n["i8v"].value();
    int16 *i16v_p_r = n["i16v"].value();
    int32 *i32v_p_r = n["i32v"].value();
    int64 *i64v_p_r = n["i64v"].value();

    EXPECT_EQ(i8v,i8v_p_r[0]);
    EXPECT_EQ(i16v,i16v_p_r[0]);
    EXPECT_EQ(i32v,i32v_p_r[0]);
    EXPECT_EQ(i64v,i64v_p_r[0]);

    uint8  *ui8v_p_r  = n["ui8v"].value();
    uint16 *ui16v_p_r = n["ui16v"].value();
    uint32 *ui32v_p_r = n["ui32v"].value();
    uint64 *ui64v_p_r = n["ui64v"].value();

    EXPECT_EQ(ui8v,ui8v_p_r[0]);
    EXPECT_EQ(ui16v,ui16v_p_r[0]);
    EXPECT_EQ(ui32v,ui32v_p_r[0]);
    EXPECT_EQ(ui64v,ui64v_p_r[0]);


    float32 *f32v_p_r = n["f32v"].value();
    float64 *f64v_p_r = n["f64v"].value();

    EXPECT_NEAR(f32v,f32v_p_r[0],0.001);
    EXPECT_NEAR(f64v,f64v_p_r[0],0.001);

    int8_array  i8v_a_r  = n["i8v"].value();
    int16_array i16v_a_r = n["i16v"].value();
    int32_array i32v_a_r = n["i32v"].value();
    int64_array i64v_a_r = n["i64v"].value();

    EXPECT_EQ(i8v_a_r.number_of_elements(),1);
    EXPECT_EQ(i16v_a_r.number_of_elements(),1);
    EXPECT_EQ(i32v_a_r.number_of_elements(),1);
    EXPECT_EQ(i64v_a_r.number_of_elements(),1);

    EXPECT_EQ(i8v,i8v_a_r[0]);
    EXPECT_EQ(i16v,i16v_a_r[0]);
    EXPECT_EQ(i32v,i32v_a_r[0]);
    EXPECT_EQ(i64v,i64v_a_r[0]);

    uint8_array  ui8v_a_r  = n["ui8v"].value();
    uint16_array ui16v_a_r = n["ui16v"].value();
    uint32_array ui32v_a_r = n["ui32v"].value();
    uint64_array ui64v_a_r = n["ui64v"].value();

    EXPECT_EQ(ui8v_a_r.number_of_elements(),1);
    EXPECT_EQ(ui16v_a_r.number_of_elements(),1);
    EXPECT_EQ(ui32v_a_r.number_of_elements(),1);
    EXPECT_EQ(ui64v_a_r.number_of_elements(),1);

    EXPECT_EQ(ui8v,ui8v_a_r[0]);
    EXPECT_EQ(ui16v,ui16v_a_r[0]);
    EXPECT_EQ(ui32v,ui32v_a_r[0]);
    EXPECT_EQ(ui64v,ui64v_a_r[0]);


    float32_array f32v_a_r = n["f32v"].value();
    float64_array f64v_a_r = n["f64v"].value();

    EXPECT_EQ(f32v_a_r.number_of_elements(),1);
    EXPECT_EQ(f64v_a_r.number_of_elements(),1);

    EXPECT_NEAR(f32v,f32v_a_r[0],0.001);
    EXPECT_NEAR(f64v,f64v_a_r[0],0.001);


}


//-----------------------------------------------------------------------------
void
print_warning(const std::string &msg,
              const std::string &file,
              int line)
{
    std::cout << "File:"  << file << std::endl;
    std::cout << "Line:"  << line << std::endl;
    std::cout << "Error:" << msg  << std::endl;
}


//-----------------------------------------------------------------------------
TEST(conduit_node, check_as_value_default_after_warning)
{
    conduit::utils::set_warning_handler(print_warning);
    uint64 val = 10;
    Node n;
    n.set(val);

    EXPECT_EQ(n.as_uint8(),0);
    EXPECT_EQ(n.as_uint8_ptr(),(void*)NULL);

    float32_array arr = n.value();

    EXPECT_TRUE(arr.dtype().is_empty());
    EXPECT_EQ(arr.data_ptr(),(void*)NULL);

    conduit::utils::set_warning_handler(conduit::utils::default_warning_handler);
}

//-----------------------------------------------------------------------------
TEST(conduit_node, check_contiguous)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};

    uint8_array  u8av_a(u8av,DataType::uint8(6));
    uint16_array u16av_a(u16av,DataType::uint16(6));
    uint32_array u32av_a(u32av,DataType::uint32(6));
    uint64_array u64av_a(u64av,DataType::uint64(6));

    Node n;
    n["a"] = u8av_a;
    n["b"] = u16av_a;
    n["c"] = u32av_a;
    n["d"] = u64av_a;

    // compact
    EXPECT_TRUE(n.is_compact());
    // but not contig
    EXPECT_FALSE(n.is_contiguous());
    // contig dptr should be null if not contig
    EXPECT_FALSE(n.contiguous_data_ptr() != NULL);

    // compact to create compact + contig
    Node n2;
    n.compact_to(n2);
    EXPECT_TRUE(n2.is_compact());
    EXPECT_TRUE(n2.is_contiguous());
    // contig dptr should not be null if contig
    EXPECT_TRUE(n2.contiguous_data_ptr() != NULL);

    // no longer contig
    n2["e"] = 10;
    EXPECT_FALSE(n2.is_contiguous());
    // contig dptr should be null if not contig
    EXPECT_FALSE(n2.contiguous_data_ptr() != NULL);
    // still compact
    EXPECT_TRUE(n2.is_compact());

    // contig & compact external
    Node n3;
    n3["a"].set_external(u64av,2);
    n3["b"].set_external(u64av,4,sizeof(uint64)*2);
    EXPECT_TRUE(n3.is_contiguous());
    // contig dptr should not be null if contig
    EXPECT_TRUE(n3.contiguous_data_ptr() != NULL);


    // make non contig
    n3["c"].set_external(u64av,3,sizeof(uint64)*3);
    EXPECT_FALSE(n3.is_contiguous());
    // contig dptr should be null if not contig
    EXPECT_TRUE(n3.contiguous_data_ptr() == NULL);


    // contig but not compact
    Node n4;
    n4["a"].set_external(u64av,2);
    n4["b"].set_external(u64av,2,sizeof(uint64)*2,sizeof(uint64)*2);
    EXPECT_FALSE(n4.is_compact());
    EXPECT_TRUE(n4.is_contiguous());


    // nested contig and compact
    Node n5;
    n5["a/b/c/d/e/f"].set_int64(10);

    EXPECT_TRUE(n5.is_compact());
    EXPECT_TRUE(n5.is_contiguous());

    void *n5_contg_ptr = n5.contiguous_data_ptr();

    // contig dptr should be null if not contig
    EXPECT_TRUE(n5_contg_ptr != NULL);

    // check loc and value of contig dptr

    EXPECT_EQ(n5_contg_ptr,
              n5["a/b/c/d/e/f"].data_ptr());

    int64 *n5_v_ptr = (int64*)n5_contg_ptr;
    EXPECT_EQ(n5_v_ptr[0],
              n5["a/b/c/d/e/f"].as_int64());


}



//-----------------------------------------------------------------------------
TEST(conduit_node, check_contiguous_with)
{
    uint64  u64av[10] = {2,4,8,16,32,64,128,256,512,1024};

    Node n1;
    n1["a"].set_external(u64av,5);
    n1["b"].set_external(u64av,5,5 * sizeof(uint64));

    n1.print();

    // compact
    EXPECT_TRUE(n1.is_compact());
    // and contig
    EXPECT_TRUE(n1.is_contiguous());

    // we don't expect things to be contig with NULL
    EXPECT_FALSE(n1["a"].contiguous_with(NULL));
    EXPECT_FALSE(n1["b"].contiguous_with(NULL));

    // b should be contig with a
    EXPECT_TRUE(n1["b"].contiguous_with(n1["a"]));

    // but the reverse is not the case (b comes after a ...)
    EXPECT_FALSE(n1["a"].contiguous_with(n1["b"]));

    // b it should be contig with address at the end of a
    // a.ele_ptr(5) should land us right at start of b
    EXPECT_EQ(n1["b"].element_ptr(0),n1["a"].element_ptr(5));

    // b it should be contig with address at the end of a
    EXPECT_TRUE(n1["b"].contiguous_with(n1["a"].element_ptr(5)));


    Node n2;
    n2["a"].set_external(u64av,5);
    n2["b"].set_external(DataType::uint8(),NULL);
    n2["c"].set_external(u64av,5,5 * sizeof(uint64));

    // we expect c to be contig with a
    EXPECT_TRUE(n1["c"].contiguous_with(n1["a"]));

    // null leaf type in middle should break contig
    EXPECT_FALSE(n2.is_contiguous());

    // should be contig if we removed the null leaf
    n2.remove("b");
    EXPECT_TRUE(n2.is_contiguous());


    // but an empy leaf type in middle shouldn't break contig
    n2["a"].set_external(u64av,5);
    n2["b"].set(DataType::empty());
    n2["c"].set_external(u64av,5,5 * sizeof(uint64));

    EXPECT_TRUE(n2.is_contiguous());

}


//-----------------------------------------------------------------------------
TEST(conduit_node, check_path)
{

    Node n;

    n["a/b/c/d/e/f"] = 10;

    EXPECT_EQ(n.path(),"");
    EXPECT_EQ(n["a/b/c/d/e/f"].path(), "a/b/c/d/e/f");

    // check roundtrip -- using path() to fetch from root node
    EXPECT_EQ(n.fetch_ptr(n["a/b/c/d/e/f"].path()),&n["a/b/c/d/e/f"]);

    // list cases
    EXPECT_EQ(n["a/b/c/list"].append().path(), "a/b/c/list/[0]");
    EXPECT_EQ(n["a/b/c/list"].append().path(), "a/b/c/list/[1]");
    EXPECT_EQ(n["a/b/c/list"].append().path(), "a/b/c/list/[2]");

    n.print();


}

//-----------------------------------------------------------------------------
TEST(conduit_node, check_path_in_bad_access)
{
    // test if the path string appears in the exception thrown
    // for a bad dtype access

    Node n;

    n["a/b/c/d/e/f"] = 10;

    try
    {
        std::string s = n["a/b/c/d/e/f"].as_string();
    }
    catch(Error &e)
    {
        EXPECT_TRUE(e.message().find("a/b/c/d/e/f") != std::string::npos);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_node, check_const_access)
{
    Node n;

    n["a/b"].set_int32(10);
    n["a/c"].set(DataType::int64(2));

    int64_array c_vals = n["a/c"].value();
    c_vals[0]= 1;
    c_vals[1]= 2;

    // Note: this won't throw b/c n is not const, so the const fetch
    // will not bind
    //const Node &n_bad = n["bad"];

    const Node &n_a = n["a"];
    const int64 *c_vals_const = n_a["c"].as_int64_ptr();

    EXPECT_THROW(n_a.fetch_existing("bad");,conduit::Error);
    EXPECT_THROW(const Node &n_a_bad = n_a.fetch_existing("bad");,conduit::Error);

    EXPECT_THROW(const Node &n_a_bad = n_a["bad"];,conduit::Error);

    EXPECT_EQ(n_a["b"].as_int32(),10);
    EXPECT_EQ(c_vals_const[0],1);
    EXPECT_EQ(c_vals_const[1],2);

}


//-----------------------------------------------------------------------------
TEST(conduit_node, node_more_set_cases)
{
    // construct from data type
    Node n(DataType::object());

    // create complex tree
    n["a/b/c/d"].append().set(DataType::int64(2));

    int64_array cvals = n["a/b/c/d"][0].value();
    cvals[0]= 1;
    cvals[1]= 2;

    // set from constructor
    Node n2(n);

    // set using node
    Node n3;
    n3.set(n2);


    const int64 *n2_vals_ptr= n2["a/b/c/d"][0].as_int64_ptr();
    const int64 *n3_vals_ptr= n3["a/b/c/d"][0].as_int64_ptr();

    EXPECT_EQ(n2_vals_ptr[0],1);
    EXPECT_EQ(n2_vals_ptr[1],2);

    EXPECT_EQ(n3_vals_ptr[0],1);
    EXPECT_EQ(n3_vals_ptr[1],2);


    float64 fval[1] = { 3.1415 };

    Node n4;
    n4["a"].set(DataType::float64(),fval);
    n4.set_path("b",DataType::float64(),fval);

    EXPECT_EQ(n4["a"].as_float64(),fval[0]);
    EXPECT_EQ(n4["b"].as_float64(),fval[0]);

    n4.print();

    Node n5;
    n5["a"].set(Schema(DataType::float64()),fval);
    n5.set_path("b",Schema(DataType::float64()),fval);

    EXPECT_EQ(n5["a"].as_float64(),fval[0]);
    EXPECT_EQ(n5["b"].as_float64(),fval[0]);

    n5.print();


}


//-----------------------------------------------------------------------------
TEST(conduit_node, check_invalid_index_error)
{
    Node n;

    EXPECT_THROW(n.child(0),conduit::Error);
    EXPECT_THROW(n[0],conduit::Error);
    EXPECT_THROW(n.child(1),conduit::Error);
    EXPECT_THROW(n[1],conduit::Error);

    n.append();

    Node &n_0 = n[0];


}


//-----------------------------------------------------------------------------
TEST(conduit_node, check_empty_path_fetch_error)
{
    conduit::Node n;
    EXPECT_THROW(n[""],conduit::Error);
}

//-----------------------------------------------------------------------------
TEST(conduit_node, rename_child)
{
    Node n;

    // error, can't rename non object
    EXPECT_THROW(n.rename_child("a","b"),conduit::Error);

    n["a"].set(DataType::int64());
    n["b"].set(DataType::float64());
    n["c"].set(DataType::float32(10));

    float32_array c_vals = n["c"].value();
    for(index_t i=0;i<10;i++)
    {
        c_vals[i] = i;
    }

    n.print();

    // error, can't rename to existing child name
    EXPECT_THROW(n.rename_child("a","b"),conduit::Error);

    // error, can't rename non existing child
    EXPECT_THROW(n.rename_child("bad","d"),conduit::Error);

    std::vector<std::string> cnames = n.child_names();
    EXPECT_EQ(cnames[2],"c");
    EXPECT_TRUE(n.has_child("c"));
    EXPECT_FALSE(n.has_child("d"));

    n.rename_child("c","d");

    n.print();

    cnames = n.child_names();
    EXPECT_TRUE(n.has_child("d"));
    EXPECT_FALSE(n.has_child("c"));
    EXPECT_EQ(cnames[2],"d");

    // or old c_vals ptr should now be wired to d,
    // give the name change
    EXPECT_EQ(c_vals.data_ptr(),n["d"].data_ptr());
}

//-----------------------------------------------------------------------------
TEST(conduit_node, list_to_obj_cleanup)
{
    // before resolved,
    // this case triggered a mem-access after release case in:
    // https://github.com/LLNL/conduit/issues/428

    // create list
    Node n;
    n.append();
    n.print();
    // switch to object
    Node &n_cld = n["cld"];
    EXPECT_TRUE(n.has_child("cld"));
    n_cld.print();
}



//-----------------------------------------------------------------------------
TEST(conduit_node, test_parse_all_protos)
{
    Node n;

    n["a/b/c"] = (int64) 10;
    n["a/b/d"] = (float64) 42.2;
    n["a/b/e"] = " string !";

    std::vector<std::pair<std::string, std::string>> txt_cases = {
        std::make_pair("json",                  n.to_json()),
        std::make_pair("json",                  n.to_json("json")),
        std::make_pair("conduit_json",          n.to_json("conduit_json")),
        std::make_pair("conduit_json_external", n.to_json("conduit_json_external")),
        std::make_pair("conduit_base64_json",   n.to_json("conduit_base64_json")),
        std::make_pair("yaml",                  n.to_yaml()),
        std::make_pair("yaml",                  n.to_yaml("yaml")),
        std::make_pair("conduit_yaml",          n.to_yaml("conduit_yaml")),
        std::make_pair("conduit_yaml_external", n.to_yaml("conduit_yaml_external")),
        std::make_pair("conduit_base64_yaml",   n.to_yaml("conduit_base64_yaml")),
    };

    for (const auto &parse_me : txt_cases)
    {
        Node n2, info;
        n2.parse(parse_me.second,parse_me.first);
        EXPECT_FALSE(n.diff(n2,info));
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_node, to_string_and_parse_all_protos)
{
    Node n;

    n["a/b/c"] = (int64) 10;
    n["a/b/d"] = (float64) 42.2;
    n["a/b/e"] = " string !";

    std::ostringstream yaml_oss, json_oss;
    n.to_string_stream(yaml_oss);
    n.to_string_stream(json_oss,"json");

    std::vector<std::pair<std::string, std::string>> txt_cases = {
        std::make_pair("yaml",                n.to_string()),
        std::make_pair("yaml",                n.to_string_default()),
        std::make_pair("yaml",                n.to_string("yaml")),
        std::make_pair("yaml",                yaml_oss.str()),
        std::make_pair("conduit_yaml",        n.to_string("conduit_yaml")),
        std::make_pair("conduit_base64_yaml", n.to_string("conduit_base64_yaml")),
        std::make_pair("json",                n.to_string("json")),
        std::make_pair("json",                json_oss.str()),
        std::make_pair("conduit_json",        n.to_string("conduit_json")),
        std::make_pair("conduit_base64_json", n.to_string("conduit_base64_json")),
    };

    for (const auto &parse_me : txt_cases)
    {
        Node n2, info;
        n2.parse(parse_me.second,parse_me.first);
        EXPECT_FALSE(n.diff(n2,info));
        info.print();
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_node, to_string_and_indent_check_all_protos)
{
    Node n;

    n["a/b/c"] = (int64) 10;
    n["a/b/d"] = (float64) 42.2;
    n["a/b/e"] = " string !";

    const std::map<std::string, index_t> schema_key_depths =
        {{"a", 0}, {"b", 1}, {"c", 2}, {"d", 2}, {"e", 2}};

    std::ostringstream yaml_oss, json_oss;
    n.to_string_stream(yaml_oss);
    n.to_string_stream(json_oss, "json");

    std::vector<std::pair<std::string, std::string>> txt_cases = {
        std::make_pair("yaml", n.to_string()),
        std::make_pair("yaml", yaml_oss.str()),
        std::make_pair("yaml", n.to_string_default()),
        std::make_pair("yaml", n.to_string("conduit_yaml")),
        std::make_pair("yaml", n.to_string("yaml")),
        std::make_pair("json", json_oss.str()),
        std::make_pair("json", n.to_string("json")),
        std::make_pair("json", n.to_string("conduit_json")),
    // TODO: Eventually should test these cases, but they're too different at present.
        // std::make_pair("yaml", n.to_string("conduit_base64_yaml")),
        // std::make_pair("json", n.to_string("conduit_base64_json")),
    };

    for(index_t ti = 0; ti < (index_t)txt_cases.size(); ti++)
    {
        const std::string& txt_case = txt_cases[ti].second;
        const std::string& txt_type = txt_cases[ti].first;
        std::vector<std::string> txt_lines;
        conduit::utils::split_string(txt_case, '\n', txt_lines);

        for(const auto& key_pair : schema_key_depths)
        {
            const std::string& key_string = key_pair.first;
            const index_t key_depth = key_pair.second + ((txt_type == "json") ? 1 : 0);

            std::string key_line;
            {
                std::ostringstream oss;
                oss << std::string(2 * key_depth, ' ');
                if(txt_type == "json")
                {
                    oss << "\"";
                }
                oss << key_string;
                if(txt_type == "json")
                {
                    oss << "\"";
                }
                oss << ":";
                key_line = oss.str();
            }

            bool key_found = false;
            for(index_t li = 0; li < (index_t)txt_lines.size() && !key_found; li++)
            {
                key_found |= txt_lines[li].rfind(key_line) == 0;
            }
            ASSERT_TRUE(key_found);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_node, add_child)
{
    Node n;
    int64 val = 42;
    Node &n_a = n.add_child("a");
    n_a.set(val);
    EXPECT_EQ(n.child("a").as_int64(),val);
    std::cout << n.to_yaml() << std::endl;
    Node *n_ptr = &n.add_child("a");
    // if you add again, you should get the same ref bac.
    EXPECT_EQ(&n_a,n_ptr);
}



//-----------------------------------------------------------------------------
TEST(conduit_node, describe)
{
    Node n;

    n["a"] = {1,2,3,4,5};
    n["b"] = {1,2,3};
    n["c"] = {1,2,3,4,5,6};
    n["d"] = {1,2,3,4,5,6,7};
    n["e"] = {1,2,3,4,5,6,7,8,9,10,11,12};
    n["f"] = {1.0,2.0,3.0,4.0,5.0,6.0,7.0};
    n["g"] = {2.0,4.0};

    Node d;
    n.describe(d);
    d.print();

    EXPECT_EQ(d["a/count"].to_int(),5);
    EXPECT_EQ(d["b/count"].to_int(),3);
    EXPECT_EQ(d["c/count"].to_int(),6);
    EXPECT_EQ(d["d/count"].to_int(),7);
    EXPECT_EQ(d["e/count"].to_int(),12);
    EXPECT_EQ(d["f/count"].to_int(),7);

    EXPECT_EQ(d["a/min"].to_int(),1);
    EXPECT_EQ(d["b/min"].to_int(),1);
    EXPECT_EQ(d["c/min"].to_int(),1);
    EXPECT_EQ(d["d/min"].to_int(),1);
    EXPECT_EQ(d["e/min"].to_int(),1);
    EXPECT_EQ(d["f/min"].to_float(),1.0);

    EXPECT_EQ(d["a/max"].to_int(),5);
    EXPECT_EQ(d["b/max"].to_int(),3);
    EXPECT_EQ(d["c/max"].to_int(),6);
    EXPECT_EQ(d["d/max"].to_int(),7);
    EXPECT_EQ(d["e/max"].to_int(),12);
    EXPECT_EQ(d["f/max"].to_float(),7.0);

    EXPECT_EQ(d["a/mean"].to_float(),3.0);
    EXPECT_EQ(d["b/mean"].to_float(),2.0);
    EXPECT_EQ(d["c/mean"].to_float(),3.5);
    EXPECT_EQ(d["d/mean"].to_float(),4.0);
    EXPECT_EQ(d["e/mean"].to_float(),6.5);
    EXPECT_EQ(d["f/mean"].to_float(),4.0);
    EXPECT_EQ(d["g/mean"].to_float(),3.0);

    n["a"] = {1,2,3,4,5};
    n["b"] = {1,2,3};
    n["c"] = {1,2,3,4,5,6};
    n["d"] = {1,2,3,4,5,6,7};
    n["e"] = {1,2,3,4,5,6,7,8,9,10,11,12};
    n["f"] = {1.0,2.0,3.0,4.0,5.0,6.0,7.0};

    Node opts;
    opts["threshold"] = 10;
    n.describe(opts,d);
    d.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node, avoid_crazy_town)
{
    Node n;
    n.append() = "here";
    n.append() = "there";
    n.print();
    n["crazy_town"] = "not here";
    std::string res = n.to_string();
    std::cout << "res = " << res << std::endl;
    EXPECT_EQ(res,"\ncrazy_town: \"not here\"\n");
}

//-----------------------------------------------------------------------------
TEST(conduit_node, swap_schema_parent)
{
    Node root, subtree;
    subtree["to/data"] = 5;
    root["path"]["to"]["data"] = 6;

    std::string path_name = root["path"].name();
    std::string path_schema_parent_existence;
    if (root["path"].schema().parent() != NULL)
    {
        path_schema_parent_existence = "path schema parent exists";
    }
    else
    {
        path_schema_parent_existence = "path schema parent does not exist";
    }
    EXPECT_EQ(path_name, "path");
    EXPECT_EQ(path_schema_parent_existence, "path schema parent exists");

    // swap should preserve the schema parent, allowing name() to work
    root["path"].swap(subtree);

    path_name = root["path"].name();
    if (root["path"].schema().parent() != NULL)
    {
        path_schema_parent_existence = "path schema parent exists";
    }
    else
    {
        path_schema_parent_existence = "path schema parent does not exist";
    }
    EXPECT_EQ(path_name, "path");
    EXPECT_EQ(path_schema_parent_existence, "path schema parent exists");
}

//-----------------------------------------------------------------------------
TEST(conduit_node, move_schema_parent)
{
    Node root, subtree;
    subtree["to/data"] = 5;
    root["path"]["to"]["data"] = 6;

    std::string path_name = root["path"].name();
    std::string path_schema_parent_existence;
    if (root["path"].schema().parent() != NULL)
    {
        path_schema_parent_existence = "path schema parent exists";
    }
    else
    {
        path_schema_parent_existence = "path schema parent does not exist";
    }
    EXPECT_EQ(path_name, "path");
    EXPECT_EQ(path_schema_parent_existence, "path schema parent exists");

    // move should preserve the schema parent, allowing name() to work
    root["path"].move(subtree);

    path_name = root["path"].name();
    if (root["path"].schema().parent() != NULL)
    {
        path_schema_parent_existence = "path schema parent exists";
    }
    else
    {
        path_schema_parent_existence = "path schema parent does not exist";
    }
    EXPECT_EQ(path_name, "path");
    EXPECT_EQ(path_schema_parent_existence, "path schema parent exists");
}
