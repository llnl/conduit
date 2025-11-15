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

#include <string>
#include <vector>
#include "gtest/gtest.h"

std::vector<std::string> targets_for_meshtype(const std::string &type)
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
    for(const auto &t : generated_targets)
    {
        targets.push_back(t);
    }
    return targets;
}


void test_convert(const std::string &meshType,
                  int dimension)
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
  const auto targets = targets_for_meshtype(type);
  for(const auto &target : targets)
  {
    conduit::Node n_converted, n_maps;
    conduit::Node n_options;
    n_options["topology"] = "mesh";
    n_options["convert_polytopes"] = 1;
    n_options["copy"] = true;
    n_options["target"] = target;

    std::cout << "  " << meshType << " to " << target << std::endl;
    conduit::blueprint::mesh::convert(n_mesh, n_options, n_converted, n_maps);
    //n_converted.print();

    EXPECT_TRUE(n_converted.has_path("coordsets/coords"));
    EXPECT_TRUE(n_converted.has_path("topologies/mesh"));
    if(target.find("generate_") == std::string::npos)
    {
        EXPECT_TRUE(n_converted.has_path("fields/braid"));
        EXPECT_TRUE(n_converted.has_path("fields/radial"));
        EXPECT_TRUE(n_converted.has_path("fields/vel"));
    }
    EXPECT_TRUE(n_converted.has_path("state"));
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

TEST(convert, hexs)
{
    test_convert("hexs", 3);
}

TEST(convert, hexs_poly)
{
    test_convert("hexs_poly", 3);
}
