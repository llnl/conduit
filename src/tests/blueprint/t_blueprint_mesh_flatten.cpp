// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_flatten.cpp
///
//-----------------------------------------------------------------------------
#include <array>
#include <iostream>
#include <string>

#include <conduit.hpp>
#include <conduit_blueprint.hpp>
#include <conduit_relay.hpp>

#include <gtest/gtest.h>

#include "blueprint_test_helpers.hpp"

using namespace conduit;

// #define GENERATE_BASELINES

//-----------------------------------------------------------------------------
#ifdef _WIN32
const std::string sep("\\");
#else
const std::string sep("/");
#endif

//-----------------------------------------------------------------------------
std::string
baseline_dir()
{
    std::string path(__FILE__);
    auto idx = path.rfind(sep);
    if(idx != std::string::npos)
        path = path.substr(0, idx);
    path = path + sep + std::string("baselines");
    return path;
}

//-----------------------------------------------------------------------------
std::string test_name() { return std::string("t_blueprint_mesh_flatten"); }

//-----------------------------------------------------------------------------
int get_rank() { return 0; }

//-----------------------------------------------------------------------------
void barrier() { }

//-----------------------------------------------------------------------------
// Include some helper function definitions
#include "blueprint_baseline_helpers.hpp"

//-----------------------------------------------------------------------------
/**
@brief flattens a single domain mesh (with default options)
    and checks that all of the vertex/elements fields have been
    flattened properly. Also checks for domain info and coordset
    values. Does NOT check cell centers. Does NOT test multi-domain meshes.
*/
static void
test_mesh_single_domain(const Node &mesh, const std::string &case_name)
{
    ASSERT_FALSE(blueprint::mesh::is_multi_domain(mesh)) << case_name;
    std::cout << "-- Testing " << case_name << " --" << std::endl;

    // Use default options
    Node table, opts;
    blueprint::mesh::flatten(mesh, opts, table);

    Node info;
    ASSERT_TRUE(blueprint::table::verify(table, info)) << info.to_json();

    // Grab the coordset
    const Node &n_topo = mesh["topologies"][0];
    const std::string cset_name = n_topo["coordset"].as_string();
    const Node &n_cset = mesh["coordsets"][cset_name];
    const Node &n_table_cset = table["vertex_data/values"][cset_name];
    const std::string cset_type = n_cset["type"].as_string();
    if(cset_type == "uniform")
    {
        Node temp;
        blueprint::mesh::coordset::uniform::to_explicit(n_cset, temp);
        EXPECT_FALSE(temp["values"].diff(n_table_cset, info)) << info.to_json();
    }
    else if(cset_type == "rectilinear")
    {
        Node temp;
        blueprint::mesh::coordset::rectilinear::to_explicit(n_cset, temp);
        EXPECT_FALSE(temp["values"].diff(n_table_cset, info)) << info.to_json();
    }
    else if(cset_type == "explicit")
    {
        EXPECT_FALSE(n_cset["values"].diff(n_table_cset, info)) << info.to_json();
    }
    else
    {
        EXPECT_FALSE(true) << "Unknown cset type " << cset_type;
    }

    // Check fields
    const Node &n_fields = mesh["fields"];
    for(index_t i = 0; i < n_fields.number_of_children(); i++)
    {
        const Node &n_field = n_fields[i];
        if(!n_field.has_child("topology"))
        {
            EXPECT_FALSE(true) << "Material based fields are not implemented.";
            continue;
        }

        const std::string topo_name = n_field["topology"].as_string();
        if(topo_name != n_topo.name())
        {
            continue;
        }

        const std::string values_path = n_field["association"].as_string() + "_data/values";
        EXPECT_FALSE(n_field["values"].diff(table[values_path][n_field.name()], info)) << info.to_json();
    }

    // Check the domain info
    {
        index_t domid = 0;
        if(mesh.has_path("state/domain_id") && mesh["state/domain_id"].dtype().is_integer())
        {
            domid = mesh["state/domain_id"].to_index_t();
        }

        {
            const index_t nverts = blueprint::mesh::coordset::length(n_cset);
            std::vector<index_t> domain_ids(nverts, domid);
            std::vector<index_t> vert_ids(nverts);
            for(index_t i = 0; i < nverts; i++) vert_ids[i] = i;

            {
                Node temp;
                temp.set_external(vert_ids.data(), nverts);
                EXPECT_FALSE(temp.diff(table["vertex_data/values/vertex_id"], info)) << info.to_json();
            }

            {
                Node temp;
                temp.set_external(domain_ids.data(), nverts);
                EXPECT_FALSE(temp.diff(table["vertex_data/values/domain_id"], info)) << info.to_json();
            }
        }

        {
            const index_t nelems = blueprint::mesh::topology::length(n_topo);
            std::vector<index_t> domain_ids(nelems, domid);
            std::vector<index_t> elem_ids(nelems);
            for(index_t i = 0; i < nelems; i++) elem_ids[i] = i;

            {
                Node temp;
                temp.set_external(elem_ids.data(), nelems);
                EXPECT_FALSE(temp.diff(table["element_data/values/element_id"], info)) << info.to_json();
            }

            {
                Node temp;
                temp.set_external(domain_ids.data(), nelems);
                EXPECT_FALSE(temp.diff(table["element_data/values/domain_id"], info)) << info.to_json();
            }
        }
    }
    std::cout << "-- End testing " << case_name << " --" << std::endl;
}

//-----------------------------------------------------------------------------
/**
@brief Creates a multi-domain (4 domains) mesh using the spiral example then
    adds a "constant_element_field" and "constant_vertex_field" using the
    provided not_the_fill_value argument. Removes the "dist" field from
    domain 2.
*/
template<typename FieldType = index_t>
static void
create_mesh_mutli_domain(Node &out_mesh, FieldType not_the_fill_value = 1337)
{
    out_mesh.reset();
    const index_t ndom = 4;
    blueprint::mesh::examples::spiral(ndom, out_mesh);

    const std::string topo_name = out_mesh[0]["topologies"][0].name();
    for(index_t i = 0; i < ndom; i++)
    {
        Node &domain = out_mesh[i];
        Node &topo = domain["topologies"][topo_name];
        Node &cset = domain["coordsets"][topo["coordset"].as_string()];
        Node &fields = domain["fields"];

        // Add an element field only to domain 0
        if(i == 0)
        {
            const index_t nelems = blueprint::mesh::topology::length(topo);
            std::vector<FieldType> new_field(nelems, not_the_fill_value);
            fields["constant_element_field/topology"] = topo_name;
            fields["constant_element_field/association"] = "element";
            fields["constant_element_field/volume_dependnt"] = "false";
            fields["constant_element_field/values"].set(new_field);
        }

        // Remove the dist field from i == 2
        if(i == 2)
        {
            fields.remove_child("dist");
        }

        // Add a vertex field to every domain but the first
        if(i != 0)
        {
            const index_t nverts = blueprint::mesh::coordset::length(cset);
            std::vector<FieldType> new_field(nverts, not_the_fill_value);
            fields["constant_vertex_field/topology"] = topo_name;
            fields["constant_vertex_field/association"] = "vertex";
            fields["constant_vertex_field/volume_dependnt"] = "false";
            fields["constant_vertex_field/values"].set(new_field);
        }
    }
}

TEST(blueprint_mesh_flatten, basic)
{
    const std::string mesh_types[] = {
        "uniform", "rectilinear", "structured",
        "tris", "quads", "polygons",
        "tets", "hexs", "wedges", 
        "pyramids", "polyhedra"
    };
    for(const auto &mesh_type : mesh_types)
    {
        if(mesh_type == "tris" || mesh_type == "quads"
            || mesh_type == "polygons")
        {
            // Field on basic mesh is too large if you give it 3D dims
            //  and a 2D shape.
            Node mesh;
            blueprint::mesh::examples::basic(mesh_type, 4, 3, 0, mesh);
            test_mesh_single_domain(mesh, "basic_" + mesh_type);
        }
        else if(mesh_type == "uniform" || mesh_type == "rectilinear"
            || mesh_type == "structured")
        {
            // Do 2D and 3D
            {
                // 2D
                Node mesh;
                blueprint::mesh::examples::basic(mesh_type, 4, 3, 0, mesh);
                test_mesh_single_domain(mesh, "basic_" + mesh_type + "_2D");
            }
            {
                // 3D
                Node mesh;
                blueprint::mesh::examples::basic(mesh_type, 4, 3, 3, mesh);
                test_mesh_single_domain(mesh, "basic_" + mesh_type + "_3D");
            }
        }
        else
        {
            // 3D shapes
            Node mesh;
            blueprint::mesh::examples::basic(mesh_type, 4, 3, 3, mesh);
            test_mesh_single_domain(mesh, "basic_" + mesh_type);
        }
    }
}

TEST(blueprint_mesh_flatten, braid)
{
    const std::string mesh_types[] = {
        "points_implicit", "uniform", "rectilinear", "structured",
        "points", "lines", "tris", "quads", "quads_poly",
        "tets", "hexs", "hexs_poly",
        "wedges", "pyramids"
    };
    const index_t x = 4;
    const index_t y = 3;
    const index_t z = 3;
    for(const auto &mesh_type : mesh_types)
    {
        if(mesh_type == "points_implicit" || mesh_type == "points")
        {
            // points_implicit can be 1D, 2D or 3D
            {
                // 1D
                Node mesh;
                blueprint::mesh::examples::braid(mesh_type, x, 1, 1, mesh);
                test_mesh_single_domain(mesh, "braid_" + mesh_type + "_1D");
            }
            {
                // 2D
                Node mesh;
                blueprint::mesh::examples::braid(mesh_type, x, y, 1, mesh);
                test_mesh_single_domain(mesh, "braid_" + mesh_type + "_2D");
            }
            {
                // 3D
                Node mesh;
                blueprint::mesh::examples::braid(mesh_type, x, y, z, mesh);
                test_mesh_single_domain(mesh, "braid_" + mesh_type + "_3D");
            }
        }
        else if(mesh_type == "uniform"
            || mesh_type == "rectilinear"
            || mesh_type == "structured"
            || mesh_type == "lines")
        {
            // mesh_types that support 2D or 3D
            {
                // 2D
                Node mesh;
                blueprint::mesh::examples::braid(mesh_type, x, y, 0, mesh);
                test_mesh_single_domain(mesh, "braid_" + mesh_type + "_2D");
            }
            {
                // 3D
                Node mesh;
                blueprint::mesh::examples::braid(mesh_type, x, y, z, mesh);
                test_mesh_single_domain(mesh, "braid_" + mesh_type + "_3D");
            }
        }
        else if(mesh_type == "tris"
            || mesh_type == "quads"
            || mesh_type == "quads_poly")
        {
            // 2D celltypes
            Node mesh;
            blueprint::mesh::examples::braid(mesh_type, x, y, 0, mesh);
            test_mesh_single_domain(mesh, "braid_" + mesh_type);
        }
        else
        {
            // 3D celltypes
            Node mesh;
            blueprint::mesh::examples::braid(mesh_type, x, y, z, mesh);
            test_mesh_single_domain(mesh, "braid_" + mesh_type);
        }
    }
}

TEST(blueprint_mesh_flatten, polytess)
{
    Node mesh;
    blueprint::mesh::examples::polytess(5, 1, mesh);
    test_mesh_single_domain(mesh, "polytess");
}

TEST(blueprint_mesh_flatten, spiral)
{
    // Because this example is multi-domain we will write a baseline
    //  and compare to that.
    // This means that cell_centers and domain info will be properly tested.
    const index_t ndoms = 3;
    Node mesh;
    blueprint::mesh::examples::spiral(ndoms, mesh);

    // First test each individual domain, because why not
    for(index_t i = 0; i < ndoms; i++)
    {
        test_mesh_single_domain(mesh[i], "spiral_" + std::to_string(i));
    }

    const std::string case_name = "spiral_mulidomain";
    std::cout << "-- Testing " << case_name << " --" << std::endl;

    // Use default options
    Node table, opts;
    blueprint::mesh::flatten(mesh, opts, table);

    const std::string filename = baseline_file(case_name);
    Node baseline;
#ifdef GENERATE_BASELINES
    make_baseline(filename, table);
#endif
    load_baseline(filename, baseline);
    table::compare_to_baseline(table, baseline);

    std::cout << "-- End testing " << case_name << " --" << std::endl;
}

TEST(blueprint_mesh_flatten, domains_missing_fields)
{
    Node mesh;
    create_mesh_mutli_domain(mesh);

    Node table, opts;
    blueprint::mesh::flatten(mesh, opts, table);

    Node info;
    ASSERT_TRUE(blueprint::table::verify(table, info)) << info.to_json();
    ASSERT_TRUE(table.has_child("vertex_data"));
    ASSERT_TRUE(table.has_child("element_data"));

    const std::array<std::string, 5> expected_vertex_columns{
        "coords", "domain_id", "vertex_id", "dist",
        "constant_vertex_field"
    };
    const std::array<std::string, 4> expected_element_columns{
        "element_centers", "domain_id", "element_id",
        "constant_element_field"
    };

    const Node &vertex_data_values = table["vertex_data/values"];
    bool ok = true;
    for(const std::string &name : expected_vertex_columns)
    {
        bool has_child = vertex_data_values.has_child(name);
        EXPECT_TRUE(has_child) << name << " not found in vertex_table.";
        ok &= has_child;
    }

    const Node &element_data_values = table["element_data/values"];
    for(const std::string &name : expected_element_columns)
    {
        bool has_child = element_data_values.has_child(name);
        EXPECT_TRUE(has_child) << name << " not found in vertex_table.";
        ok &= has_child;
    }

    if(ok)
    {
        const std::string filename = baseline_file("domains_missing_fields");
#ifdef GENERATE_BASELINES
        make_baseline(filename, table);
#endif
        Node baseline;
        load_baseline(filename, baseline);
        table::compare_to_baseline(table, baseline);
    }
}
