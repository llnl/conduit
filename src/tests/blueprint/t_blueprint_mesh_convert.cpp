// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_convert.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mesh_examples.hpp"
#include "conduit_relay_io.hpp"
#include "conduit_relay_io_blueprint.hpp"

#include <cstring>
#include <string>
#include <vector>
#include "gtest/gtest.h"

std::vector<std::string> targets_for_meshtype(const std::string &type, bool add_generate_targets)
{
    std::vector<std::string> uniform_targets{"uniform", "rectilinear", "structured", "unstructured", "polytopal"};
    std::vector<std::string> rectilinear_targets{"rectilinear", "structured", "unstructured", "polytopal"};
    std::vector<std::string> structured_targets{"structured", "unstructured", "polytopal"};
    std::vector<std::string> unstructured_targets{"unstructured", "polytopal"};
    std::vector<std::string> generated_targets{
                        "generate_points",
                        "generate_lines",
                        "generate_faces",
                        "generate_centroids",
                        "generate_sides",
                        "generate_corners"
                        };
    std::vector<std::string> targets;
    if(type == "uniform")
    {
      targets = uniform_targets;
    }
    else if(type == "rectilinear")
    {
      targets = rectilinear_targets;
    }
    else if(type == "structured")
    {
      targets = structured_targets;
    }
    else if(type == "unstructured")
    {
      targets = unstructured_targets;
    }
    if(add_generate_targets)
    {
        for(const auto &t : generated_targets)
        {
            targets.push_back(t);
        }
    }
    return targets;
}


void test_convert(const std::string &meshType,
                  int dimension,
                  bool add_generate_targets = true)
{
  int dims[] = {5, 5, 5};
  if(dimension == 2)
  {
      dims[2] = 0;
  }

  conduit::Node n_mesh;
  conduit::blueprint::mesh::examples::braid(meshType,
                                            dims[0],
                                            dims[1],
                                            dims[2],
                                            n_mesh);

  const std::string type = n_mesh["topologies/mesh/type"].as_string();
  const auto targets = targets_for_meshtype(type, add_generate_targets);
  for(const auto &target : targets)
  {
    conduit::Node n_converted, n_maps;
    conduit::Node n_options;
    n_options["topology"] = "mesh";
    n_options["degrade_polytopes"] = 1;
    n_options["copy"] = 1;
    n_options["target"] = target;

    std::cout << "  " << meshType << " to " << target << std::endl;
    conduit::blueprint::mesh::convert(n_mesh, n_options, n_converted, n_maps);

#if 0
    // For debugging.
    if(target == "polytopal")
    {
        n_converted.print();
        conduit::relay::io::save(n_converted, meshType + "_polytopal.yaml", "yaml");
        conduit::relay::io::blueprint::save_mesh(n_converted, meshType + "_polytopal", "hdf5");
    }
#endif

    EXPECT_TRUE(n_converted.has_path("coordsets/coords"));
    EXPECT_TRUE(n_converted.has_path("topologies/mesh"));
    // Test that polyhedral meshes for wedges and pyramids now make both triangle and quad faces.
    if(target == "polytopal" && (meshType == "wedges" || meshType == "pyramids" || meshType == "mixed"))
    {
        const auto subelements_sizes = n_converted["topologies/mesh/subelements/sizes"].as_index_t_accessor();
        EXPECT_EQ(subelements_sizes.min(), 3);
        EXPECT_EQ(subelements_sizes.max(), 4);
    }

    if(target.find("generate_") == std::string::npos)
    {
        // Check that the fields are in the converted mesh.
        const conduit::Node &n_fields = n_mesh["fields"];
        for(conduit::index_t i = 0; i < n_fields.number_of_children(); i++)
        {
            if(!n_converted.has_path(n_fields[i].path()))
            {
                std::cout << "Missing field " << n_fields[i].name() << std::endl;
            }
            EXPECT_TRUE(n_converted.has_path(n_fields[i].path()));
        }
    }
    else
    {
        EXPECT_TRUE(n_maps.has_path("s2dmap"));
        EXPECT_TRUE(n_maps.has_path("d2smap"));
    }
    EXPECT_TRUE(n_converted.has_path("state"));
  }
}

void test_convert_multi_domain(int dimension)
{
  int dims[] = {4, 4, 4};
  if(dimension == 2)
  {
      dims[2] = 0;
  }

  conduit::Node n_mesh, n_tile_options;
  n_tile_options["numDomains"] = 2;
  conduit::blueprint::mesh::examples::tiled(dims[0],
                                            dims[1],
                                            dims[2],
                                            n_mesh,
                                            n_tile_options);
  const auto domains = conduit::blueprint::mesh::domains(n_mesh);
  EXPECT_EQ(domains.size(), 2);

  const std::string type = domains[0]->fetch_existing("topologies/mesh/type").as_string();
  const auto targets = targets_for_meshtype(type, true);
  for(const auto &target : targets)
  {
    conduit::Node n_converted, n_maps;
    conduit::Node n_options;
    n_options["topology"] = "mesh";
    n_options["convert_polytopes"] = 1;
    n_options["copy"] = 1;
    n_options["target"] = target;

    std::cout << "  " << type << " to " << target << std::endl;
    conduit::blueprint::mesh::convert(n_mesh, n_options, n_converted, n_maps);

    const auto convertedDomains = conduit::blueprint::mesh::domains(n_converted);
    int index = 0;
    for(const auto domainPtr : convertedDomains)
    {
      EXPECT_TRUE(domainPtr->has_path("coordsets/coords"));
      EXPECT_TRUE(domainPtr->has_path("topologies/mesh"));
      if(target.find("generate_") == std::string::npos)
      {
        EXPECT_TRUE(domainPtr->has_path("adjsets/mesh_adjset"));
      }
      else
      {       
        EXPECT_TRUE(n_maps[index].has_path("s2dmap"));
        EXPECT_TRUE(n_maps[index].has_path("d2smap"));
      }
      EXPECT_TRUE(domainPtr->has_path("state"));
      index++;
    }
  }
}

TEST(convert, uniform2D)
{
    test_convert("uniform", 2);
}

TEST(convert, rectilinear2D)
{
    test_convert("rectilinear", 2);
}

TEST(convert, structured2D)
{
    test_convert("structured", 2);
}

TEST(convert, tris)
{
    test_convert("tris", 2);
}

TEST(convert, quads)
{
    test_convert("quads", 2);
}

TEST(convert, polygons2D)
{
    test_convert("quads_poly", 2);
}

TEST(convert, mixed2D)
{
    // NOTE: Fox mixed_2d, some of the generate targets do not like the mesh!
    const bool add_generate_targets = false;
    test_convert("mixed_2d", 2, add_generate_targets);
}

TEST(convert, uniform3D)
{
    test_convert("uniform", 3);
}

TEST(convert, rectilinear3D)
{
    test_convert("rectilinear", 3);
}

TEST(convert, structured3D)
{
    test_convert("structured", 3);
}

TEST(convert, tets)
{
    test_convert("tets", 3);
}

TEST(convert, wedges)
{
    test_convert("wedges", 3);
}

TEST(convert, pyramids)
{
    test_convert("pyramids", 3);
}

TEST(convert, hexs)
{
    test_convert("hexs", 3);
}

TEST(convert, hexs_poly)
{
    test_convert("hexs_poly", 3);
}

TEST(convert, mixed)
{
    // NOTE: Fox mixed, some of the generate targets do not like the mesh!
    const bool add_generate_targets = false;
    test_convert("mixed", 3, add_generate_targets);
}

TEST(convert, multi_domain_2d)
{
    test_convert_multi_domain(2);
}

TEST(convert, multi_domain_3d)
{
    test_convert_multi_domain(3);
}

/// Conduit error handler that blocks (helpful for getting a stack in a debugger)
static void conduit_debug_err_handler(const std::string &s1, const std::string &s2, int i1)
{
    std::cout << "s1=" << s1 << ", s2=" << s2 << ", i1=" << i1 << std::endl;
    // This is on purpose.
    while(1);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    bool handler = false;
    for(int i = 0; i < argc; i++)
    {
        if(strcmp(argv[i], "-handler") == 0)
        {
            handler = true;
        }
    }

    if(handler)
    {
        conduit::utils::set_error_handler(conduit_debug_err_handler);
    }

    // Run all the tests.
    int result = RUN_ALL_TESTS();
    return result;
}
