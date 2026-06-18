// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_execution.cpp
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
TEST(conduit_blueprint_mesh_transform, uniform_to_rect)
{
    // create example mesh
    Node mesh;
    blueprint::mesh::examples::braid("uniform", 100, 100, 100, mesh);
    const Node &uniform_coords = mesh["coordsets"]["coords"];

    // create baseline result for correctness checking
    Node baseline;
    blueprint::mesh::examples::braid("rectilinear", 100, 100, 100, baseline);
    
    Node exec_opts;
    // TODO for each choice in the matrix of choices...
    {
        // set execution options
        exec_opts["execution_location"] = "host";
        exec_opts["output_location"] = "host";
        exec_opts["sync_strategy"] = "sync";
        exec_opts["fallback_location"] = "host";
        execution::execution_set_options(exec_opts);

        // start timing
        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        // perform transform
        Node rect_coords;
        blueprint::mesh::coordset::uniform::to_rectilinear(uniform_coords, rect_coords);
    
        // end timing
        annotations::finalize();

        // verification check
        Node info;
        EXPECT_TRUE(blueprint::mesh::coordset::rectilinear::verify(rect_coords, info));
        
        // quick correctness check
        EXPECT_FALSE(rect_coords.diff(baseline["coordsets"]["coords"], info));
    }
}
