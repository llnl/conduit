// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_io_adios.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay.hpp"
#include "conduit_error.hpp"
#include <iostream>
#include <cmath>
#include "gtest/gtest.h"
#include <sys/stat.h>

using namespace conduit;

// Include some utility functions
#include "adios_test_utils.hpp"

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_adios, test_options_contain_adios)
{
    int has_adios_protocol = 0;
    int has_adios_options = 0;
    Node opts;
    relay::io::about(opts);
    if(opts.has_child("protocols"))
    {
        const Node &protocols = opts["protocols"];
        if(protocols.has_child("adios"))
        {
            if(protocols["adios"].as_string() == std::string("enabled"))
                has_adios_protocol = 1;
        }
    }

    if(opts.has_child("options"))
    {
        const Node &options = opts["options"];
        has_adios_options = options.has_child("adios") ? 1 : 0;
    }

    EXPECT_EQ(has_adios_protocol + has_adios_options, 2);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_adios, test_read_badfile)
{
    int caught_error = 0;
    std::string path("does_not_exist.bp");
    try
    {
        Node in;
        relay::io::load(path, in);
    }
    catch(conduit::Error)
    {
        caught_error = 1;
    }

    EXPECT_EQ(caught_error, 1);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_adios, test_scalar_types)
{
    const float64 pi = 3.141592653589793;

    int8    a(1);
    int16   b(2);
    int32   c(3);
    int64   d(4);
    float32 e(1.23456);
    float64 f(pi);
    uint8   g(5);
    uint16  h(6);
    uint32  i(7);
    uint64  j(8);
    char    k(9);
    short   l(10);
    int     m(11);
    long    n(12);
    float   o(1.23456);
    double  p(pi);
    unsigned char  q(13);
    unsigned short r(14);
    unsigned int   s(15);
    unsigned long  t(16);
    std::string    u("Hello from ADIOS");

    Node out;
    out["a"] = a;
    out["b"] = b;
    out["c"] = c;
    out["d"] = d;
    out["e"] = e;
    out["f"] = f;
    out["g"] = g;
    out["h"] = h;
    out["i"] = i;
    out["j"] = j;
    out["k"] = k;
    out["l"] = l;
    out["m"] = m;
    out["n"] = n;
    out["o"] = o;
    out["p"] = p;
    out["q"] = q;
    out["r"] = r;
    out["s"] = s;
    out["t"] = t;
    out["u"] = u;

    std::string path("test_scalar_types.bp");
    relay::io::save(out, path);
    //std::cout << "out=" << out.to_json() << std::endl;
    CONDUIT_INFO("Reading " << path);
    Node in;
    relay::io::load(path, in);
    
    Node n_info;
    EXPECT_FALSE(out.diff(in,n_info,0.0));
    //EXPECT_EQ(compare_nodes(out, in, out), true);
}

TEST(conduit_relay_io_adios, test_array_types)
{
    const float64 pi = 3.141592653589793;

    int8    a[] = {1,2,3,4,5};
    int16   b[] = {2,3,4};
    int32   c[] = {3,4,5,6,7,8,9};
    int64   d[] = {4,5,6,7,8};
    float32 e[] = {1.23456, 2.3456, 3.4567};
    float64 f[] = {pi, 2.*pi, 3.*pi};
    uint8   g[] = {5,6,7,8,9};
    uint16  h[] = {6,7,8,9};
    uint32  i[] = {7,8,9,10,11};
    uint64  j[] = {8,9,10,11,12,13,14,15};
    char    k[] = {'A','D','I','O','S'};
    short   l[] = {9,10,11,12,13,14,15,16};
    int     m[] = {10,11,12};
    long    n[] = {11,12,13};
    float   o[] = {1.23456, 2.3456, 3.4567};
    double  p[] = {pi, 2.*pi, 3.*pi};
    unsigned char  q[] = {0,1,2,3};
    unsigned short r[] = {1,2,3,4,5};
    unsigned int   s[] = {2,3,4,5,6,7,8};
    unsigned long  t[] = {3,4,5,6,7,8};

    int32 sequence[] = {-2, -1, 0, 1, 2, 3, 4, 5, 6, 7};

    Node out;
    out["a"].set_int8_ptr(a, sizeof(a)/sizeof(int8));
    out["b"].set_int16_ptr(b, sizeof(b) / sizeof(int16));
    out["c"].set_int32_ptr(c, sizeof(c) / sizeof(int32));
    out["d"].set_int64_ptr(d, sizeof(d) / sizeof(int64));
    out["e"].set_float32_ptr(e, sizeof(e) / sizeof(float32));
    out["f"].set_float64_ptr(f, sizeof(f) / sizeof(float64));
    out["g"].set_uint8_ptr(g, sizeof(g) / sizeof(uint8));
    out["h"].set_uint16_ptr(h, sizeof(h) / sizeof(uint16));
    out["i"].set_uint32_ptr(i, sizeof(i) / sizeof(uint32));
    out["j"].set_uint64_ptr(j, sizeof(j) / sizeof(uint64));
#ifndef CONDUIT_USE_CHAR
    out["k"].set(k, sizeof(k) / sizeof(char));
    out["q"].set(q, sizeof(q) / sizeof(unsigned char));
#endif
#ifndef CONDUIT_USE_SHORT
    out["l"].set(l, sizeof(l) / sizeof(short));
    out["r"].set(r, sizeof(r) / sizeof(unsigned short));
#endif
#ifndef CONDUIT_USE_INT
    out["m"].set(m, sizeof(m) / sizeof(int));
    out["s"].set(s, sizeof(s) / sizeof(unsigned int));
#endif
#ifndef CONDUIT_USE_LONG
    out["n"].set(n, sizeof(n) / sizeof(long));
    out["t"].set(t, sizeof(t) / sizeof(unsigned long));
#endif
    out["o"].set(o, sizeof(o) / sizeof(float));
    out["p"].set(p, sizeof(p) / sizeof(double));

    out["one_elem"].set_int32_ptr(c, 1);

    // Add some variables with stride/offset.
    out["evens"].set(sequence, 5, 0, 2 * sizeof(int32));
    out["odds"].set(sequence, 5, sizeof(int32), 2 * sizeof(int32));
    out["evens_positive"].set(sequence, 4, 2*sizeof(int32), 2 * sizeof(int32));

    //std::cout << out.to_json() << std::endl;
    std::string path("test_array_types.bp");
    relay::io::save(out, path);

    CONDUIT_INFO("Reading " << path);
    Node in;
    relay::io::load(path, in);

    //std::cout << in.to_json() << std::endl;
    Node n_info;
    EXPECT_FALSE(out.diff(in,n_info,0.0));
    //EXPECT_EQ(compare_nodes(out, in, out), true);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_adios, test_vector_types)
{
    std::vector<int8>    a;
    std::vector<int16>   b;
    std::vector<int32>   c;
    std::vector<int64>   d;
    std::vector<float32> e;
    std::vector<float64> f;
    for(int i = 0; i < 10; ++i)
    {
        a.push_back(static_cast<int8>(i));
        b.push_back(static_cast<int16>(i));
        c.push_back(static_cast<int32>(i));
        d.push_back(static_cast<int64>(i));
        e.push_back(static_cast<float32>(i) * 0.1);
        f.push_back(static_cast<float64>(i) * 0.2);
    }

    Node out;
    out["a"] = a;
    out["b"] = b;
    out["c"] = c;
    out["d"] = d;
    out["e"] = e;
    out["f"] = f;

    std::string path("test_vector_types.bp");
    relay::io::save(out, path);

    CONDUIT_INFO("Reading " << path);
    Node in;
    relay::io::load(path, in);

    //std::cout << in.to_json() << std::endl;
    Node n_info;
    EXPECT_FALSE(out.diff(in,n_info,0.0));
    //EXPECT_EQ(compare_nodes(out, in, out), true);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_adios, test_list_types)
{

    Node out;
    std::string key("path/to/a_list");
    out["path/to/a"] = 1;
    // Add an empty list to the node.
    out[key].set(DataType::list());
    out["path/to/b"] = 2;
    // Add some elements to the list node.
    out[key].append().set(1.1234);
    out[key].append().set(2.3456);
    out[key].append().set(3.4567);

    // Add another list node
    std::string key2("/path/to/list2");
    out[key2].set(DataType::list());
    // Add various data types to the 2nd list.
    out[key2].append().set(1);
    out[key2].append().set("Hi there");
    const float64 pi = 3.141592653589793;
    const float64 v[] = {pi, 2*pi, 4*pi, 6*pi, 8*pi};
    out[key2].append().set(v, 5);

    // Add another list node and let's put mesh objects in it.
    std::string key3("multiple_domains");
    out[key3].set(DataType::list());
    Node &domain0 = out[key3].append();
    domain0["domain_id"] = 0;
    float64 origin[3] = {0., 0., 0.};
    float64 size[3]   = {3., 4., 5.};
    int     dims[3]   = {4,5,6};
    add_rectilinear_mesh(domain0, origin, size, dims);
    Node &domain1 = out[key3].append();
    domain1["domain_id"] = 1;
    origin[0] += size[0];
    int     dims2[3]   = {7, 9, 11};
    add_rectilinear_mesh(domain1, origin, size, dims2);

    std::string path("test_list_types.bp");
    relay::io::save(out, path);

    CONDUIT_INFO("Reading " << path);
    Node in;
    relay::io::load(path, in);

    //std::cout << in.to_json() << std::endl;
    Node n_info;
    // TODO: THIS CASES FAILS STD DIFF, I think b/c list
    // eles come back as objects
    //EXPECT_FALSE(out.diff(in,n_info,0.0));
    // n_info.print();
    EXPECT_EQ(compare_nodes(out, in, out), true);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_adios, test_save_path)
{
    Node out;
    out["a"] = 1;
    out["b"] = 2;
    out["c/d"] = 4;
    out["c/e"] = 8;

    // Test writing/reading using a path.
    std::string key("powers/of/two");
    std::string path(std::string("test_save_path.bp") + std::string(":") + key);
    relay::io::save(out, path);
    //std::cout << out.to_json() << std::endl;

    CONDUIT_INFO("Reading " << path);
    Node in;
    relay::io::load(path, in);
    //std::cout << in.to_json() << std::endl;
    //std::cout << in[key].to_json() << std::endl;
    Node n_info;
    EXPECT_FALSE(out.diff(in[key],n_info,0.0));
    //EXPECT_EQ(compare_nodes(out, in[key], out), true);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_adios, test_opts_transforms)
{
    std::vector<float> a(1000), b(20000);
    for(size_t i = 0; i < a.size(); ++i)
    {
        float t = float(i) / float(a.size()-1);
        float angle = M_PI * 10.;
        a[i] = sin(angle);
    }
    for(size_t i = 0; i < b.size(); ++i)
    {
        float t = float(i) / float(b.size()-1);
        float angle = M_PI * 10.;
        b[i] = sin(angle);
    }

    Node out;
    out["sine/low"] = a;
    out["sine/high"] = b;
    out["sine/low.size"] = a.size();
    out["sine/high.size"] = b.size();

    std::string path("test_opts_transforms.bp");
    std::string protocol("adios");
    Node opts;
    opts["adios/write/transform"] = "zfp";
    opts["adios/write/transform_options"] = "rate=0.25";
    relay::io::save(out, path, protocol, opts);

    CONDUIT_INFO("Reading " << path);
    Node in;
    relay::io::load(path, in);

    // Compare floats with some tolerance.
    //bool exact = false;
    float tolerance = 0.0001;
    Node n_info;
    EXPECT_FALSE(out.diff(in,n_info,tolerance));
    //EXPECT_EQ(compare_nodes(out, in, out, exact, tolerance), true);

    // Check the file size and make sure it got compressed.
    size_t rough_uncompressed_size = (a.size() + b.size()) * sizeof(float);
    size_t compressed_size_guess = rough_uncompressed_size / 4;
    size_t compressed_file_size = rough_uncompressed_size;
    struct stat buf;
    if(stat(path.c_str(), &buf) == 0)
        compressed_file_size = static_cast<size_t>(buf.st_size);
    //std::cout << "compressed_file_size = " << compressed_file_size << std::endl;
    EXPECT_EQ(compressed_file_size < compressed_size_guess, true);
}

TEST(conduit_relay_io_adios, test_load_merged)
{
    // Write a file.
    Node out;
    out["a"] = 1;
    out["b"] = 2.3456;
    out["c"] = std::vector<int>(10, 0);

    std::string path("test_load_merged.bp");
    relay::io::save(out, path);

    Node in;
    std::string key("pre-existing"), value("This key already exists.");
    in[key] = value;
    relay::io::load_merged(path, in);

    // Now add the same key to out so we can compare.
    out[key] = value;
    Node n_info;
    EXPECT_FALSE(out.diff(in,n_info,0.0));
    //EXPECT_EQ(compare_nodes(out, in, out), true);
}

TEST(conduit_relay_io_adios, test_save_merged)
{
    // Write a file.
    Node out;
    out["a"] = 1;
    out["b"] = 2.3456;
    out["c"] = std::vector<int>(10, 0);

    // Write a file.
    std::string path("test_save_merged.bp");
    relay::io::save(out, path);

    Node extra;
    extra["d"] = 3.14159;
    extra["e"] = "This is an extra field.";
    extra["f"] = std::vector<double>(10, 1.2345678);

    // Append to the file.
    relay::io::save_merged(extra, path);

    // Load the file.
    Node in;
    relay::io::load(path, in);

    // Add the extra things to out so we can compare out,in.
    out["d"] = extra["d"];
    out["e"] = extra["e"];
    out["f"] = extra["f"];
#if 0
    std::cout << "extra=" << extra.to_json() << std::endl;
    std::cout << "out=" << out.to_json() << std::endl;
    std::cout << "in=" << in.to_json() << std::endl;
#endif

    Node n_info;
    EXPECT_FALSE(out.diff(in,n_info,0.0));
    //EXPECT_EQ(compare_nodes(out, in, out), true);
}

TEST(conduit_relay_io_adios, test_load_subtree)
{
    const char *abc_keys[] = {"a/b/c/d","a/b/c/dog"};
    const char *a_keys[] = {"a/b/cat",
                            "a/b/carnivores/cat",
                            "a/b/carnivores/dinosaur"};
    const char *aa_keys[] = {"a/a/bull"};
    const char *b_keys[] = {"b/c/d/e", "binary", "blue"};

    // Make a node we can save.
    int index = 0;
    Node out;
    add_keys_to_node(out, abc_keys, 2, index);
    add_keys_to_node(out, a_keys, 3, index);
    add_keys_to_node(out, aa_keys, 1, index);
    add_keys_to_node(out, b_keys, 3, index);
    std::string path("test_load_subtree.bp");
    //std::cout << "out=" << out.to_json() << std::endl;
    relay::io::save(out, path);

    // Try reading nodes with subpath a/b/c
    Node in1;
    std::string path1(path + ":a/b/c");
    relay::io::load(path1, in1);
    Node abc;
    copy_node_keys(abc, out, abc_keys, 2);
    //std::cout << "in1=" << in1.to_json() << std::endl;
    
    Node n_info;
    EXPECT_FALSE(abc.diff(in1,n_info,0.0));
    //EXPECT_EQ(compare_nodes(abc, in1, abc), true);

    // Try reading nodes with subpath a/b
    Node in2;
    std::string path2(path + ":a/b");
    relay::io::load(path2, in2);
    Node ab;
    copy_node_keys(ab, out, abc_keys, 2);
    copy_node_keys(ab, out, a_keys, 3);
    //std::cout << "in2=" << in2.to_json() << std::endl;
    EXPECT_FALSE(ab.diff(in2,n_info,0.0));
    //EXPECT_EQ(compare_nodes(ab, in2, ab), true);

    // Try reading nodes with subpath a
    Node in3;
    std::string path3(path + ":a");
    relay::io::load(path3, in3);
    Node a;
    copy_node_keys(a, out, abc_keys, 2);
    copy_node_keys(a, out, a_keys, 3);
    copy_node_keys(a, out, aa_keys, 1);
    //std::cout << "in3=" << in3.to_json() << std::endl;
    EXPECT_FALSE(a.diff(in3,n_info,0.0));
    //EXPECT_EQ(compare_nodes(a, in3, a), true);

    // Try reading nodes with subpath b
    Node in4;
    std::string path4(path + ":b");
    relay::io::load(path4, in4);
    Node b;
    copy_node_keys(b, out, b_keys, 1); //1 since binary and blue won't match.
    //std::cout << "in4=" << in4.to_json() << std::endl;
    EXPECT_FALSE(b.diff(in4,n_info,0.0));
    //EXPECT_EQ(compare_nodes(b, in4, b), true);

    // Try reading nodes with subpath a/b/c:b
    Node in5;
    std::string path5(path + ":a/b/c:b");
    relay::io::load(path5, in5);
    Node abcb;
    copy_node_keys(abcb, out, abc_keys, 2);
    copy_node_keys(abcb, out, b_keys, 1); //1 since binary and blue won't match.
    //std::cout << "in5=" << in5.to_json() << std::endl;
    EXPECT_FALSE(abcb.diff(in5,n_info,0.0));
    //EXPECT_EQ(compare_nodes(abcb, in5, abcb), true);
}

TEST(conduit_relay_io_adios, test_save_merged_series)
{
    int dims[] = {3, 4};
    double x[] = {0., 1., 2.};
    double y[] = {0., 1., 2., 3.};
    int dims2[] = {5, 7};
    double x2[] = {0.,0.5, 1., 1.5, 2.};
    double y2[] = {0.,0.5, 1., 1.5, 2., 2.5, 3.};

    // Write data sequentially to the same file.
    std::string path("test_save_merged_series.bp");
    Node out[4];
    for(int dom = 0; dom < 4; ++dom)
    {
        Node &n = out[dom];
        n["mesh/domain"] = dom;
        // Vary the size of the domains.
        if(dom % 2 == 0)
        {
            n["mesh/x"].set(x, dims[0]);
            n["mesh/y"].set(y, dims[1]);
        }
        else
        {
            n["mesh/x"].set(x2, dims2[0]);
            n["mesh/y"].set(y2, dims2[1]);
        }
        if(dom == 0)
            relay::io::save(n, path);
        else
            relay::io::save_merged(n, path);
    }

    // Read back one domain at a time.
    for(int dom = 0; dom < 4; ++dom)
    {
        std::ostringstream oss;
        oss << path << ":" << dom;
        Node in;
        std::cout << "Loading path " << oss.str() << std::endl;
        relay::io::load(oss.str(), in);
        //std::cout << "domain " << dom << " in=" << in.to_json() << std::endl;
        Node n_info;
        EXPECT_FALSE(in.diff(out[dom],n_info,0.0));
        //EXPECT_EQ(compare_nodes(in, out[dom], in), true);
    }

    // Test the number of domains.
    int ndoms = relay::io::query_number_of_domains(path);
    EXPECT_EQ(ndoms, 4);
}

TEST(conduit_relay_io_adios, test_time_series)
{
    std::string path("test_time_series.bp"), protocol("adios");

    // Remove the file if it exists.
    conduit::utils::remove_path_if_exists(path);

    // Write multiple time steps to the same file.
    int nts = 5;
    Node *out = new Node[nts];
    for(int ts = 0; ts < nts; ++ts)
    {
        Node &n = out[ts];
        int idx = ts*10;
        n["a"] = idx + 1;
        n["b"] = idx + 2;
        n["c/d"] = idx + 3;
        n["c/e"] = idx + 4;
        n["f"] = 3.14159f * float(ts);

        //std::cout << "time step " << ts << " out=" << n.to_json() << std::endl;
        relay::io::add_step(n, path);

        // Make sure the file has the new step.
        int qnts = relay::io::query_number_of_steps(path);
        EXPECT_EQ(qnts, ts+1);
    }

    // Read back data for each step.
    for(int ts = 0; ts < nts; ++ts)
    {
        Node in;
        int domain = 0;
        relay::io::load(path, protocol, ts, domain, in);
        Node n_info;
        EXPECT_FALSE(in.diff(out[ts],n_info,0.0));
        //EXPECT_EQ(compare_nodes(in, out[ts], in), true);
    }

    // Try reading back by encoding the step in a path.
    for(int ts = 0; ts < nts; ++ts)
    {
        std::ostringstream oss;
        int domain = 0;
        oss << path << ":" << ts << ":" << domain;
        std::string timestep_path(oss.str());

        Node in;
        relay::io::load(timestep_path, in);
        //std::cout << "path=" << timestep_path << " in=" << in.to_json() << std::endl;
        Node n_info;
        EXPECT_FALSE(in.diff(out[ts],n_info,0.0));
        //EXPECT_EQ(compare_nodes(in, out[ts], in), true);
    }

    delete [] out;
}

TEST(conduit_relay_io_adios, test_node_path)
{
    std::string path("test_node_path.bp");

    Node all;
    all["extra/level/data/a"] = int32(0);
    all["extra/level/data/b"] = std::vector<int>(10, 1);
    all["extra/level/data/c"] = std::vector<int>(20, 2);
    all["extra/levels"] = int32(1); // So we test s vs / in comparison
    //std::cout << "all=" << all.to_json() << std::endl;

    // Write out a subtree that has parent nodes.
    const Node &data = all["extra/level"];
    //std::cout << "data=" << data.to_json() << std::endl;
    relay::io::save(data, path);

    // Make sure that when we read it back in, we get the same thing and
    // not data that has the parent nodes prepended.
    Node in;
    relay::io::load(path, in);
    //std::cout << "in=" << in.to_json() << std::endl;
    Node n_info;
    EXPECT_FALSE(in.diff(data,n_info,0.0));
    // EXPECT_EQ(compare_nodes(in, data, in), true);
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    conduit::relay::io::initialize();
    result = RUN_ALL_TESTS();
    conduit::relay::io::finalize();

    return result;
}
