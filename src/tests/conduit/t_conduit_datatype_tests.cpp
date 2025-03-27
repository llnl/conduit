// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: type_tests.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;


//-----------------------------------------------------------------------------
typedef bool (*DTypeCompareFun)(const DataType&, const DataType&);
bool dtypes_convertible(const DataType &base, const DataType &ref) { return false; /*base.convertible(ref);*/ }
bool dtypes_compatible(const DataType &base, const DataType &ref) { return base.compatible(ref); }
bool dtypes_equal(const DataType &base, const DataType &ref) { return base.equals(ref); }

//-----------------------------------------------------------------------------
void print_dt(const DataType &dtype)
{
    std::cout << dtype.to_string() << std::endl;
}

//-----------------------------------------------------------------------------
void print_dt_bells_and_whistles(const DataType &dtype)
{
    std::cout << "to_json()" << std::endl;
    std::cout << dtype.to_json() << std::endl;
    std::cout << std::endl;

    std::cout << "to_yaml()" << std::endl;
    std::cout << dtype.to_yaml() << std::endl;
    std::cout << std::endl;

    std::cout << "to_json_stream()" << std::endl;
    dtype.to_json_stream(std::cout);
    std::cout << std::endl;

    std::cout << "to_yaml_stream()" << std::endl;
    dtype.to_yaml_stream(std::cout,10,1);
    std::cout << std::endl;

    std::cout << "to_yaml_default()" << std::endl;
    std::cout << dtype.to_yaml_default();
    std::cout << std::endl;

}

//-----------------------------------------------------------------------------
TEST(dtype_tests, value_print)
{
    EXPECT_EQ(DataType::EMPTY_ID,0);
    EXPECT_EQ(DataType::id_to_name(DataType::EMPTY_ID),"empty");
    EXPECT_EQ(DataType::name_to_id("empty"),DataType::EMPTY_ID);
    EXPECT_TRUE( (DataType::EMPTY_ID != DataType::OBJECT_ID) );

    DataType dt;

    // empty leaf
    dt = DataType::empty();
    EXPECT_TRUE(dt.is_empty());
    print_dt(dt);

    // generic type
    dt = DataType::object();
    EXPECT_TRUE(dt.is_object());
    print_dt(dt);

    dt = DataType::list();
    EXPECT_TRUE(dt.is_list());
    print_dt(dt);

    // signed ints
    dt = DataType::int8();
    EXPECT_TRUE(dt.is_int8());
    print_dt(dt);

    dt = DataType::int16();
    EXPECT_TRUE(dt.is_int16());
    print_dt(dt);
    
    dt = DataType::int32();
    EXPECT_TRUE(dt.is_int32());
    print_dt(dt);

    dt = DataType::int64();
    EXPECT_TRUE(dt.is_int64());
    print_dt(dt);

    // unsigned ints
    dt = DataType::uint8();
    EXPECT_TRUE(dt.is_uint8());
    print_dt(dt);

    dt = DataType::uint16();
    EXPECT_TRUE(dt.is_uint16());
    print_dt(dt);
    
    dt = DataType::uint32();
    EXPECT_TRUE(dt.is_uint32());
    print_dt(dt);

    dt = DataType::uint64();
    EXPECT_TRUE(dt.is_uint64());
    print_dt(dt);
    
    // floating point
    dt = DataType::float32();
    EXPECT_TRUE(dt.is_float32());
    print_dt(dt);

    dt = DataType::float64();
    EXPECT_TRUE(dt.is_float64());
    print_dt(dt);
}

//-----------------------------------------------------------------------------
TEST(dtype_tests, value_print_bells_and_whistles)
{
    EXPECT_EQ(DataType::EMPTY_ID,0);
    EXPECT_EQ(DataType::id_to_name(DataType::EMPTY_ID),"empty");
    EXPECT_EQ(DataType::name_to_id("empty"),DataType::EMPTY_ID);
    EXPECT_TRUE( (DataType::EMPTY_ID != DataType::OBJECT_ID) );

    DataType dt;

    // empty leaf
    dt = DataType::empty();
    EXPECT_TRUE(dt.is_empty());
    print_dt_bells_and_whistles(dt);

    // generic type
    dt = DataType::object();
    EXPECT_TRUE(dt.is_object());
    print_dt_bells_and_whistles(dt);

    dt = DataType::list();
    EXPECT_TRUE(dt.is_list());
    print_dt_bells_and_whistles(dt);

    // signed ints
    dt = DataType::int8();
    EXPECT_TRUE(dt.is_int8());
    print_dt_bells_and_whistles(dt);

    dt = DataType::int16();
    EXPECT_TRUE(dt.is_int16());
    print_dt_bells_and_whistles(dt);
    
    dt = DataType::int32();
    EXPECT_TRUE(dt.is_int32());
    print_dt_bells_and_whistles(dt);

    dt = DataType::int64();
    EXPECT_TRUE(dt.is_int64());
    print_dt_bells_and_whistles(dt);

    // unsigned ints
    dt = DataType::uint8();
    EXPECT_TRUE(dt.is_uint8());
    print_dt_bells_and_whistles(dt);

    dt = DataType::uint16();
    EXPECT_TRUE(dt.is_uint16());
    print_dt_bells_and_whistles(dt);
    
    dt = DataType::uint32();
    EXPECT_TRUE(dt.is_uint32());
    print_dt_bells_and_whistles(dt);

    dt = DataType::uint64();
    EXPECT_TRUE(dt.is_uint64());
    print_dt_bells_and_whistles(dt);
    
    // floating point
    dt = DataType::float32();
    EXPECT_TRUE(dt.is_float32());
    print_dt_bells_and_whistles(dt);

    dt = DataType::float64();
    EXPECT_TRUE(dt.is_float64());
    print_dt_bells_and_whistles(dt);
}

//-----------------------------------------------------------------------------
TEST(dtype_tests, c_types_value_print)
{
    
    DataType dt;
    
    // ints
    dt = DataType::c_char();
    EXPECT_TRUE(dt.is_char());
    print_dt(dt);

    dt = DataType::c_short();
    EXPECT_TRUE(dt.is_short());
    print_dt(dt);

    dt = DataType::c_int();
    EXPECT_TRUE(dt.is_int());
    print_dt(dt);

    dt = DataType::c_long();
    EXPECT_TRUE(dt.is_long());
    print_dt(dt);

#ifdef CONDUIT_HAS_LONG_LONG
    dt = DataType::c_long_long();
    EXPECT_TRUE(dt.is_long_long());
    print_dt(dt);
#else
    EXPECT_FALSE(dt.is_long_long());
#endif

    
    // signed ints
    dt = DataType::c_signed_char();
    EXPECT_TRUE(dt.is_signed_char());
    print_dt(dt);
    
    dt = DataType::c_signed_short();
    EXPECT_TRUE(dt.is_signed_short());
    print_dt(dt);
    
    dt = DataType::c_signed_int();
    EXPECT_TRUE(dt.is_signed_int());
    print_dt(dt);

    dt = DataType::c_signed_long();
    EXPECT_TRUE(dt.is_signed_long());
    print_dt(dt);

#ifdef CONDUIT_HAS_LONG_LONG
    dt = DataType::c_signed_long_long();
    EXPECT_TRUE(dt.is_signed_long_long());
    print_dt(dt);
#else
    EXPECT_FALSE(dt.is_signed_long_long());
#endif

    // unsigned ints
    dt = DataType::c_unsigned_char();
    EXPECT_TRUE(dt.is_unsigned_char());
    print_dt(dt);
    
    dt = DataType::c_unsigned_short();
    EXPECT_TRUE(dt.is_unsigned_short());
    print_dt(dt);
    
    dt = DataType::c_unsigned_int();
    EXPECT_TRUE(dt.is_unsigned_int());
    print_dt(dt);

    dt = DataType::c_unsigned_long();
    EXPECT_TRUE(dt.is_unsigned_long());
    print_dt(dt);

#ifdef CONDUIT_HAS_LONG_LONG
    dt = DataType::c_unsigned_long_long();
    EXPECT_TRUE(dt.is_unsigned_long_long());
    print_dt(dt);
#else
    EXPECT_FALSE(dt.is_unsigned_long_long());
#endif

    // floats
    dt = DataType::c_float();
    EXPECT_TRUE(dt.is_float());
    print_dt(dt);

    dt = DataType::c_double();
    EXPECT_TRUE(dt.is_double());
    print_dt(dt);
    
#ifdef CONDUIT_USE_LONG_DOUBLE
        dt = DataType::c_long_double();
        EXPECT_TRUE(dt.is_long_double());
        print_dt(dt);
#else
        // if we aren't using long double, this will always return false
        EXPECT_FALSE(dt.is_long_double());
#endif
}


//-----------------------------------------------------------------------------
TEST(dtype_tests, dtype_construct_from_string)
{
    print_dt(DataType("int8",
                      1,
                      0,
                      sizeof(int8),
                      sizeof(int8),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("int16",
                      1,
                      0,
                      sizeof(int16),
                      sizeof(int16),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("int32",
                      1,
                      0,
                      sizeof(int32),
                      sizeof(int32),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("int64",
                      1,
                      0,
                      sizeof(int64),
                      sizeof(int64),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("uint8",
                      1,
                      0,
                      sizeof(uint8),
                      sizeof(uint8),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("uint16",
                      1,
                      0,
                      sizeof(uint16),
                      sizeof(uint16),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("uint32",
                      1,
                      0,
                      sizeof(uint32),
                      sizeof(uint32),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("uint64",
                      1,
                      0,
                      sizeof(uint64),
                      sizeof(uint64),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("float32",
                      1,
                      0,
                      sizeof(float32),
                      sizeof(float32),
                      Endianness::DEFAULT_ID));

    print_dt(DataType("float64",
                      1,
                      0,
                      sizeof(float64),
                      sizeof(float64),
                      Endianness::DEFAULT_ID));

}

//-----------------------------------------------------------------------------
TEST(dtype_tests, dtype_index_t)
{
    EXPECT_FALSE(DataType::index_t().is_empty());
    EXPECT_TRUE(DataType::index_t().is_index_t());
#ifdef CONDUIT_INDEX_32
    EXPECT_TRUE(DataType::index_t().is_int32());
#else
    EXPECT_TRUE(DataType::index_t().is_int64());
#endif 

}

//-----------------------------------------------------------------------------
TEST(dtype_tests, dtype_set_using_string)
{
    DataType dt;
    
    EXPECT_EQ(DataType::EMPTY_ID,dt.id());
    
    dt.set("int8",
           1,
           0,
           sizeof(int8),
           sizeof(int8),
           Endianness::DEFAULT_ID);
    print_dt(dt);
    EXPECT_EQ(DataType::INT8_ID,dt.id());
    
    dt.set("int16",
           1,
           0,
           sizeof(int16),
           sizeof(int16),
           Endianness::DEFAULT_ID);
    print_dt(dt);
    EXPECT_EQ(DataType::INT16_ID,dt.id());
    
    dt.set("int32",
           1,
           0,
           sizeof(int32),
           sizeof(int32),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::INT32_ID,dt.id());
        
    dt.set("int64",
           1,
           0,
           sizeof(int64),
           sizeof(int64),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::INT64_ID,dt.id());
    
    
    dt.set("uint8",
           1,
           0,
           sizeof(uint8),
           sizeof(uint8),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::UINT8_ID,dt.id());
    
    dt.set("uint16",
           1,
           0,
           sizeof(uint16),
           sizeof(uint16),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::UINT16_ID,dt.id());
    
    dt.set("uint32",
           1,
           0,
           sizeof(uint32),
           sizeof(uint32),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::UINT32_ID,dt.id());
        
    dt.set("uint64",
           1,
           0,
           sizeof(uint64),
           sizeof(uint64),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::UINT64_ID,dt.id());
    
    dt.set("float32",
           1,
           0,
           sizeof(float32),
           sizeof(float32),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::FLOAT32_ID,dt.id());
        
    dt.set("float64",
           1,
           0,
           sizeof(float64),
           sizeof(float64),
           Endianness::DEFAULT_ID);
    
    print_dt(dt);
    EXPECT_EQ(DataType::FLOAT64_ID,dt.id());
    
    dt.set("char8_str",
           1,
           0,
           1,
           1,
           Endianness::DEFAULT_ID);

    print_dt(dt);
    EXPECT_EQ(DataType::CHAR8_STR_ID,dt.id());
    EXPECT_TRUE(dt.is_string());
    EXPECT_TRUE(dt.is_char8_str());
    
}


//-----------------------------------------------------------------------------
TEST(dtype_tests, default_bytes_from_string)
{

    EXPECT_EQ(0,DataType::default_bytes("[empty]"));
    EXPECT_EQ(0,DataType::default_bytes("object"));
    EXPECT_EQ(0,DataType::default_bytes("list"));

    
    EXPECT_EQ(sizeof(int8),DataType::default_bytes("int8"));
    EXPECT_EQ(sizeof(int16),DataType::default_bytes("int16"));
    EXPECT_EQ(sizeof(int32),DataType::default_bytes("int32"));
    EXPECT_EQ(sizeof(int64),DataType::default_bytes("int64"));
    
    
    EXPECT_EQ(sizeof(uint8),DataType::default_bytes("uint8"));
    EXPECT_EQ(sizeof(uint16),DataType::default_bytes("uint16"));
    EXPECT_EQ(sizeof(uint32),DataType::default_bytes("uint32"));
    EXPECT_EQ(sizeof(uint64),DataType::default_bytes("uint64"));

    EXPECT_EQ(sizeof(float32),DataType::default_bytes("float32"));
    EXPECT_EQ(sizeof(float64),DataType::default_bytes("float64"));

}



//-----------------------------------------------------------------------------
TEST(dtype_tests, default_dtype_from_string)
{

    EXPECT_TRUE(DataType::default_dtype("[empty]").is_empty());
    EXPECT_TRUE(DataType::default_dtype("object").is_object());
    EXPECT_TRUE(DataType::default_dtype("list").is_list());

    EXPECT_TRUE(DataType::default_dtype("int8").is_int8());
    EXPECT_TRUE(DataType::default_dtype("int16").is_int16());
    EXPECT_TRUE(DataType::default_dtype("int32").is_int32());
    EXPECT_TRUE(DataType::default_dtype("int64").is_int64());

    EXPECT_TRUE(DataType::default_dtype("uint8").is_uint8());
    EXPECT_TRUE(DataType::default_dtype("uint16").is_uint16());
    EXPECT_TRUE(DataType::default_dtype("uint32").is_uint32());
    EXPECT_TRUE(DataType::default_dtype("uint64").is_uint64());
    
    EXPECT_TRUE(DataType::default_dtype("float32").is_float32());
    EXPECT_TRUE(DataType::default_dtype("float64").is_float64());

}


//-----------------------------------------------------------------------------
TEST(dtype_tests,dtype_id_from_c_type_names)
{

    EXPECT_EQ(CONDUIT_NATIVE_CHAR_ID,  DataType::c_type_name_to_id("char"));
    EXPECT_EQ(CONDUIT_NATIVE_SHORT_ID, DataType::c_type_name_to_id("short"));
    EXPECT_EQ(CONDUIT_NATIVE_INT_ID,   DataType::c_type_name_to_id("int"));
    EXPECT_EQ(CONDUIT_NATIVE_LONG_ID,  DataType::c_type_name_to_id("long"));

#if CONDUIT_HAS_LONG_LONG
    EXPECT_EQ(CONDUIT_NATIVE_LONG_LONG_ID,
              DataType::c_type_name_to_id("long long"));
#endif


    EXPECT_EQ(CONDUIT_NATIVE_SIGNED_CHAR_ID,
              DataType::c_type_name_to_id("signed char"));

    EXPECT_EQ(CONDUIT_NATIVE_SIGNED_SHORT_ID,
              DataType::c_type_name_to_id("signed short"));

    EXPECT_EQ(CONDUIT_NATIVE_SIGNED_INT_ID,
              DataType::c_type_name_to_id("signed int"));

    EXPECT_EQ(CONDUIT_NATIVE_SIGNED_LONG_ID,
              DataType::c_type_name_to_id("signed long"));

#if CONDUIT_HAS_LONG_LONG
    EXPECT_EQ(CONDUIT_NATIVE_SIGNED_LONG_LONG_ID,
              DataType::c_type_name_to_id("signed long long"));
#endif


    EXPECT_EQ(CONDUIT_NATIVE_UNSIGNED_CHAR_ID,
              DataType::c_type_name_to_id("unsigned char"));

    EXPECT_EQ(CONDUIT_NATIVE_UNSIGNED_SHORT_ID,
              DataType::c_type_name_to_id("unsigned short"));

    EXPECT_EQ(CONDUIT_NATIVE_UNSIGNED_INT_ID,
              DataType::c_type_name_to_id("unsigned int"));

    EXPECT_EQ(CONDUIT_NATIVE_UNSIGNED_LONG_ID,
              DataType::c_type_name_to_id("unsigned long"));

#if CONDUIT_HAS_LONG_LONG
    EXPECT_EQ(CONDUIT_NATIVE_UNSIGNED_LONG_LONG_ID,
              DataType::c_type_name_to_id("unsigned long long"));
#endif

    EXPECT_EQ(CONDUIT_NATIVE_FLOAT_ID, DataType::c_type_name_to_id("float"));
    EXPECT_EQ(CONDUIT_NATIVE_DOUBLE_ID,DataType::c_type_name_to_id("double"));

    EXPECT_EQ(CONDUIT_CHAR8_STR_ID,DataType::c_type_name_to_id("char8_str"));

}


//-----------------------------------------------------------------------------
TEST(dtype_tests,dtype_endianness_checks)
{

    DataType dt;
    dt.set(DataType::UINT64_ID,
           1,
           0,
           sizeof(uint64),
           sizeof(uint64),
           Endianness::DEFAULT_ID);

    dt.set(DataType::UINT64_ID,
           1,
           0,
           sizeof(uint64),
           sizeof(uint64),
           Endianness::BIG_ID);

    EXPECT_TRUE(dt.is_big_endian());

    dt.set(DataType::UINT64_ID,
           1,
           0,
           sizeof(uint64),
           sizeof(uint64),
           Endianness::LITTLE_ID);

    EXPECT_TRUE(dt.is_little_endian());
    
    dt.set_endianness(Endianness::machine_default());
    EXPECT_TRUE(dt.endianness_matches_machine());
    
    if(Endianness::machine_is_little_endian())
    {
        dt.set_endianness(Endianness::BIG_ID);
    }
    else
    {
        dt.set_endianness(Endianness::LITTLE_ID);
    }
    
    EXPECT_FALSE(dt.endianness_matches_machine());
}

//-----------------------------------------------------------------------------
TEST(dtype_tests,dtype_stride_checks)
{
    // Test for element stride calculation and alignment checks
    DataType dt;
    
    // Standard case: stride equals element_bytes (compact data)
    dt.set(DataType::FLOAT64_ID,
           10,                    // 10 elements
           0,                     // 0 offset
           sizeof(float64),       // stride = 8 bytes
           sizeof(float64),       // element_bytes = 8 bytes
           Endianness::DEFAULT_ID);
    
    EXPECT_EQ(dt.element_stride(), 1);
    EXPECT_TRUE(dt.is_stride_element_aligned());
    EXPECT_TRUE(dt.is_stride_aligned(4));
    EXPECT_TRUE(dt.is_stride_aligned(2));
    EXPECT_TRUE(dt.is_stride_aligned(1));
    
    // Strided case: stride is multiple of element_bytes
    dt.set(DataType::FLOAT64_ID,
           10,                    // 10 elements
           0,                     // 0 offset
           sizeof(float64)*2,     // stride = 16 bytes (2 elements)
           sizeof(float64),       // element_bytes = 8 bytes
           Endianness::DEFAULT_ID);
    
    EXPECT_EQ(dt.element_stride(), 2);
    EXPECT_TRUE(dt.is_stride_element_aligned());
    EXPECT_TRUE(dt.is_stride_aligned(8));
    EXPECT_TRUE(dt.is_stride_aligned(4));
    EXPECT_TRUE(dt.is_stride_aligned(2));
    
    // Unaligned case: stride is not a multiple of element_bytes
    dt.set(DataType::FLOAT64_ID,
           10,                    // 10 elements
           0,                     // 0 offset
           sizeof(float64) + 1,   // stride = 9 bytes (8 + 1)
           sizeof(float64),       // element_bytes = 8 bytes
           Endianness::DEFAULT_ID);
    
    // Since 9/8 = 1.125, the element_stride will be 1 (integer division)
    // but it's not properly aligned
    EXPECT_EQ(dt.element_stride(), 1);
    EXPECT_FALSE(dt.is_stride_element_aligned());
    EXPECT_FALSE(dt.is_stride_aligned(8));
    EXPECT_FALSE(dt.is_stride_aligned(4));
    EXPECT_FALSE(dt.is_stride_aligned(2));
    EXPECT_TRUE(dt.is_stride_aligned(1));
    
    // Zero element_bytes case (should handle gracefully)
    dt.set(DataType::EMPTY_ID,
           0,                     // 0 elements
           0,                     // 0 offset
           0,                     // stride = 0 bytes
           0,                     // element_bytes = 0 bytes
           Endianness::DEFAULT_ID);
    
    EXPECT_EQ(dt.element_stride(), 0);
    EXPECT_FALSE(dt.is_stride_element_aligned());
    EXPECT_TRUE(dt.is_stride_aligned(8));
}


//-----------------------------------------------------------------------------
TEST(dtype_tests,dtype_to_string_simple_checks)
{
    std::string r_json = DataType::float64().to_json();
    std::string r_yaml = DataType::float64().to_yaml();
    EXPECT_TRUE(r_json.find("float64") != std::string::npos);
    EXPECT_TRUE(r_json.find("\"") != std::string::npos);
    EXPECT_TRUE(r_json.find("{")  != std::string::npos);
    EXPECT_TRUE(r_json.find("}")  != std::string::npos);

    EXPECT_TRUE(r_yaml.find("float64") != std::string::npos);
    EXPECT_TRUE(r_yaml.find(":") != std::string::npos);
    EXPECT_TRUE(r_yaml.find("{") == std::string::npos);
    EXPECT_TRUE(r_yaml.find("}") == std::string::npos);

}


//-----------------------------------------------------------------------------
TEST(dtype_tests,dtype_comparison_checks)
{
    const DTypeCompareFun COMPARE_FUNS[3] = {
        dtypes_convertible, dtypes_compatible, dtypes_equal};

    const int INT_TYPE_IDS[4] = {
        DataType::INT8_ID, DataType::INT16_ID,
        DataType::INT32_ID, DataType::INT64_ID};
    const int UINT_TYPE_IDS[4] = {
        DataType::UINT8_ID, DataType::UINT16_ID,
        DataType::UINT32_ID, DataType::UINT64_ID};
    const int FLOAT_TYPE_IDS[4] = {
        DataType::FLOAT32_ID, DataType::FLOAT32_ID, // Ignored; for testing purposes
        DataType::FLOAT32_ID, DataType::FLOAT64_ID};

    // TODO(JRC): Either revert the start index back to 0 here once the
    // "convertible" concept is modified and adopted or update this file
    // to remove traces of this concept should it be rejected.
    for(index_t fi = 1; fi < 3; fi++)
    {
        DTypeCompareFun dtype_compare = COMPARE_FUNS[fi];
        bool is_expand_compatible = fi == 0;
        bool is_list_compatible = fi != 2;

        for(index_t gi = 0; gi < 3; gi++)
        {
            const int* XINT_TYPE_IDS =
                (gi == 0) ? &INT_TYPE_IDS[0] : (
                (gi == 1) ? &UINT_TYPE_IDS[0] :
                &FLOAT_TYPE_IDS[0]);
            const int* XINT_OTHER_TYPE_IDS =
                (gi == 0) ? &UINT_TYPE_IDS[0] :
                &INT_TYPE_IDS[0];

            for(index_t ti = (gi != 2) ? 0 : 2; ti < 4; ti++)
            {
                DataType itype_base(XINT_TYPE_IDS[ti], 1);
                EXPECT_EQ(dtype_compare(itype_base, itype_base), true);

                DataType itype_list(XINT_TYPE_IDS[ti], 5);
                EXPECT_EQ(dtype_compare(itype_list, itype_base), is_list_compatible);
                EXPECT_EQ(dtype_compare(itype_base, itype_list), false);

                DataType itype_long(XINT_TYPE_IDS[ti], 1, 0,
                    2*itype_base.element_bytes(), 2*itype_base.element_bytes(),
                    itype_base.endianness());
                EXPECT_EQ(dtype_compare(itype_long, itype_base), is_expand_compatible);
                EXPECT_EQ(dtype_compare(itype_base, itype_long), false);

                for(index_t tj = ti + 1; tj < 4; tj++)
                {
                    DataType jtype_base(XINT_TYPE_IDS[tj], 1);
                    EXPECT_EQ(dtype_compare(jtype_base, itype_base), is_expand_compatible);
                    EXPECT_EQ(dtype_compare(itype_base, jtype_base), false);
                }

                DataType itype_other(XINT_OTHER_TYPE_IDS[ti], 1);
                EXPECT_EQ(dtype_compare(itype_base, itype_other), false);
                EXPECT_EQ(dtype_compare(itype_other, itype_base), false);
            }
        }
    }
}


//-----------------------------------------------------------------------------
TEST(dtype_tests,sizeof_index_t)
{
    int sz_it = DataType::sizeof_index_t();

#ifdef CONDUIT_INDEX_32
    EXPECT_EQ(sz_it,4);
#else
    EXPECT_EQ(sz_it,8);
#endif

    EXPECT_EQ(sz_it,CONDUIT_SIZEOF_INDEX_T);

    // also check about entries
    Node n_about;
    conduit::about(n_about);


    Node &n_it_info = n_about["index_t_typemap"];
    EXPECT_EQ(sz_it,n_it_info["sizeof_index_t"].to_int());

    if(sz_it == 4)
    {
        EXPECT_EQ(n_it_info["index_t"].as_string(),"int32");
    }
    else // if(sz_it == 8)
    {
        EXPECT_EQ(n_it_info["index_t"].as_string(),"int64");
    }

}
