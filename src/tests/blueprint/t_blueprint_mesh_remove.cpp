// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.
//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_remove.cpp
///
//-----------------------------------------------------------------------------


#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"

#include <string>
#include "gtest/gtest.h"


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_remove, bad_options)
{
    conduit::Node n_mesh;
    conduit::Node n_opts;
    // entries need to be a name (string) or list of name (list of strings)
    n_opts["topologies"] = 1;
    EXPECT_THROW(conduit::blueprint::mesh::remove(n_opts,n_mesh),conduit::Error);
    // lists needs to be all strings
    n_opts["topologies"].append() = "name";
    n_opts["topologies"].append() = 1;
    n_opts["topologies"].append() = 2;
    EXPECT_THROW(conduit::blueprint::mesh::remove(n_opts,n_mesh),conduit::Error);
    n_opts.reset();
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_remove, remove_simple)
{
    conduit::Node n_mesh, n_mesh_opts, n_opts;
    n_mesh_opts["mesh_type"] = "uniform";
    n_mesh_opts["nx"] = 3;
    n_mesh_opts["ny"] = 3;

    {
        conduit::blueprint::mesh::examples::generate("braid",n_mesh_opts,n_mesh);
        n_opts.reset();
        n_opts["topologies"] = "mesh";

        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/braid"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_TRUE(n_mesh.has_path("fields/vel"));

        std::cout << "[before]" << std::endl;
        std::cout << n_mesh.to_yaml() << std::endl;
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        std::cout << "[after]" << std::endl;
        std::cout << n_mesh.to_yaml() << std::endl;

        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
        EXPECT_FALSE(n_mesh.has_path("topologies/mesh"));
        EXPECT_FALSE(n_mesh.has_path("fields/braid"));
        EXPECT_FALSE(n_mesh.has_path("fields/radial"));
        EXPECT_FALSE(n_mesh.has_path("fields/vel"));
    }

    {
        conduit::blueprint::mesh::examples::generate("braid",n_mesh_opts,n_mesh);
        n_opts.reset();
        n_opts["coordsets"] = "coords";

        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/braid"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_TRUE(n_mesh.has_path("fields/vel"));

        std::cout << "[before]" << std::endl;
        std::cout << n_mesh.to_yaml() << std::endl;
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        std::cout << "[after]" << std::endl;
        std::cout << n_mesh.to_yaml() << std::endl;

        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_FALSE(n_mesh.has_path("coordsets/coords"));
        EXPECT_FALSE(n_mesh.has_path("topologies/mesh"));
        EXPECT_FALSE(n_mesh.has_path("fields/braid"));
        EXPECT_FALSE(n_mesh.has_path("fields/radial"));
        EXPECT_FALSE(n_mesh.has_path("fields/vel"));
    }

    {
        // reset mesh
        conduit::blueprint::mesh::examples::generate("braid",n_mesh_opts,n_mesh);
        n_opts.reset();
        n_opts["fields"].append() = "braid";
        n_opts["fields"].append() = "vel";

        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/braid"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_TRUE(n_mesh.has_path("fields/vel"));

        std::cout << "[before]" << std::endl;
        std::cout << n_mesh.to_yaml() << std::endl;
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        std::cout << "[after]" << std::endl;
        std::cout << n_mesh.to_yaml() << std::endl;

        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_FALSE(n_mesh.has_path("fields/braid"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_FALSE(n_mesh.has_path("fields/vel"));
    }



}