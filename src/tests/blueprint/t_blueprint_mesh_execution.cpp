// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: TODO.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_log.hpp"

#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, coordset_transforms)
{
    Node exec_opts;
    exec_opts["execution_policy"] = "host";
    exec_opts["output_allocator"] = "host";
    exec_opts["sync_strategy"] = "sync";
    execution::execution_set_options(exec_opts);

    Node mesh;
    blueprint::mesh::examples::braid("uniform", 100, 100, 100, mesh);

    Node cali_opts;
    cali_opts["config"] = "runtime-report";
    annotations::initialize(cali_opts);

    const Node &uniform_coords = mesh["coordsets"]["coords"];
    Node rect_coords;
    blueprint::mesh::coordset::uniform::to_rectilinear(uniform_coords, rect_coords);

    annotations::finalize();

    Node info;
    EXPECT_TRUE(blueprint::mesh::coordset::rectilinear::verify(rect_coords, info));
    
    // EXPECT_FALSE(rect_coords.diff(TODO, info));
}
