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

void echo_node(const std::string tag, const conduit::Node &node, bool full = true)
{
    std::cout << tag << std::endl;
    if(full)
    {
        std::cout << node.to_yaml() << std::endl;
    }
    else
    {
        std::cout << node.to_string() << std::endl;
    }
}



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
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
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

        echo_node("[before]",n_mesh);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        // NOTE: this will be false b/c we lack a topology
        EXPECT_FALSE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("info",n_info);


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
        n_opts["state"] = "*";

        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/braid"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_TRUE(n_mesh.has_path("fields/vel"));

        echo_node("[before]",n_mesh);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        // NOTE: this will be true because an empty mesh is valid
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);

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

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);


        EXPECT_TRUE(n_mesh.has_path("state"));
        EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_FALSE(n_mesh.has_path("fields/braid"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_FALSE(n_mesh.has_path("fields/vel"));
    }



}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_remove, remove_matsets)
{
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
    n_mesh_opts["matset_type"] = "full";
    n_mesh_opts["nx"] = 4;
    n_mesh_opts["ny"] = 4;
    n_mesh_opts["radius"] = 1;

    {
        conduit::blueprint::mesh::examples::generate("venn",n_mesh_opts,n_mesh);
        n_opts.reset();
        n_opts["matsets"] = "matset";

        EXPECT_TRUE(n_mesh.has_path("topologies/topo"));
        EXPECT_TRUE(n_mesh.has_path("fields/importance"));

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        EXPECT_FALSE(n_mesh.has_path("matsets/matset"));
        EXPECT_TRUE(n_mesh.has_path("fields/importance"));
        EXPECT_TRUE(n_mesh.has_path("fields/importance/values"));
        EXPECT_FALSE(n_mesh.has_path("fields/importance/matset"));
        EXPECT_FALSE(n_mesh.has_path("fields/importance/matset_values"));
    }

    {
        conduit::blueprint::mesh::examples::generate("venn",n_mesh_opts,n_mesh);
        n_opts.reset();
        n_opts["topologies"] = "topo";

        EXPECT_TRUE(n_mesh.has_path("topologies/topo"));
        EXPECT_TRUE(n_mesh.has_path("fields/importance"));

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        // will be false b/c no valid topo
        EXPECT_FALSE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        // removing the topo should remove the matsets as well as fields
        EXPECT_FALSE(n_mesh.has_path("matsets/matset"));
        EXPECT_FALSE(n_mesh.has_path("fields/importance"));
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_remove, remove_specsets)
{
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
    n_mesh_opts["mesh_type"] = "specsets";
    n_mesh_opts["nx"] = 5;
    n_mesh_opts["ny"] = 4;
    n_mesh_opts["ny"] = 3;

    {
        conduit::blueprint::mesh::examples::generate("misc",n_mesh);
        n_opts.reset();
        n_opts["specsets"] = "mesh";

        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_TRUE(n_mesh.has_path("matsets/mesh"));
        EXPECT_TRUE(n_mesh.has_path("specsets/mesh"));

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        EXPECT_TRUE(n_mesh.has_path("matsets/mesh"));
        EXPECT_FALSE(n_mesh.has_path("specsets/mesh"));
        return;
    }

    {
        conduit::blueprint::mesh::examples::generate("misc",n_mesh);
        n_opts.reset();
        n_opts["matsets"] = "matset";

        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_TRUE(n_mesh.has_path("matsets/mesh"));
        EXPECT_TRUE(n_mesh.has_path("specsets/mesh"));

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        // removing the matset should remove the specset as well
        EXPECT_FALSE(n_mesh.has_path("matsets/mesh"));
        EXPECT_FALSE(n_mesh.has_path("specsets/mesh"));
    }

    {
        conduit::blueprint::mesh::examples::generate("misc",n_mesh);
        n_opts.reset();
        n_opts["topologies"] = "mesh";

        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/radial"));
        EXPECT_TRUE(n_mesh.has_path("matsets/mesh"));
        EXPECT_TRUE(n_mesh.has_path("specsets/mesh"));

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        // will be false b/c no valid topo
        EXPECT_FALSE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        // removing the topo should remove the matsets as well as fields
        EXPECT_FALSE(n_mesh.has_path("matsets/mesh"));
        EXPECT_FALSE(n_mesh.has_path("specsets/mesh"));
        EXPECT_FALSE(n_mesh.has_path("fields/radial"));
    }

}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_remove, remove_adj_sets)
{
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
  {
        conduit::blueprint::mesh::examples::generate("adjset_uniform",n_mesh);
        n_opts.reset();
        n_opts["adjsets"] = "adjset";

        size_t number_of_doms = 8;
        EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
        for(size_t i=0; i<number_of_doms; i++)
        {
            EXPECT_TRUE(n_mesh[i].has_path("adjsets/adjset"));
        }

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);

        EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
        for(size_t i=0; i<number_of_doms; i++)
        {
            EXPECT_FALSE(n_mesh[i].has_path("adjsets"));
            EXPECT_FALSE(n_mesh[i].has_path("adjsets/adjset"));
        }

    }

    // also remove mask
    {
        conduit::blueprint::mesh::examples::generate("adjset_uniform",n_mesh);
        n_opts.reset();
        n_opts["adjsets"] = "adjset";
        n_opts["fields"] = "id";

        size_t number_of_doms = 8;
        EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
    
        for(size_t i=0; i<number_of_doms; i++)
        {
            EXPECT_TRUE(n_mesh[i].has_path("adjsets/adjset"));
            EXPECT_TRUE(n_mesh[i].has_path("fields/id"));
        }

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);

        EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
        for(size_t i=0; i<number_of_doms; i++)
        {
            EXPECT_FALSE(n_mesh[i].has_path("adjsets/adjset"));
            EXPECT_FALSE(n_mesh[i].has_path("adjsets"));
            EXPECT_FALSE(n_mesh[i].has_path("fields/id"));
            EXPECT_FALSE(n_mesh[i].has_path("fields"));
        }
    }


    // remove all via coordset
    {
        conduit::blueprint::mesh::examples::generate("adjset_uniform",n_mesh);
        n_opts.reset();
        n_opts["coordsets"] = "coords";
        n_opts["state"] = "*";

        size_t number_of_doms = 8;
        EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
        for(size_t i=0; i<number_of_doms; i++)
        {
            EXPECT_TRUE(n_mesh[i].has_path("adjsets/adjset"));
        }

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        // this mesh will be empty post removal
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);

        EXPECT_EQ(n_mesh.number_of_children(),0);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_remove, remove_nest_sets)
{
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
    {
        conduit::blueprint::mesh::examples::generate("julia_nestsets_simple",n_mesh);
        n_opts.reset();
        n_opts["nestsets"] = "nest";

        EXPECT_EQ(n_mesh.number_of_children(),2);
        EXPECT_TRUE(n_mesh[0].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[0].has_path("fields/mask"));

        EXPECT_TRUE(n_mesh[1].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[1].has_path("fields/mask"));

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);
        
        EXPECT_EQ(n_mesh.number_of_children(),2);
        EXPECT_FALSE(n_mesh[0].has_path("nestsets"));
        EXPECT_FALSE(n_mesh[0].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[0].has_path("fields/mask"));

        EXPECT_FALSE(n_mesh[1].has_path("nestsets"));
        EXPECT_FALSE(n_mesh[1].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[1].has_path("fields/mask"));
    }

    // also remove mask
    {
        conduit::blueprint::mesh::examples::generate("julia_nestsets_simple",n_mesh);
        n_opts.reset();
        n_opts["nestsets"] = "nest";
        n_opts["fields"] = "mask";

        EXPECT_EQ(n_mesh.number_of_children(),2);
        EXPECT_TRUE(n_mesh[0].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[0].has_path("fields/mask"));

        EXPECT_TRUE(n_mesh[1].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[1].has_path("fields/mask"));


        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);

        EXPECT_EQ(n_mesh.number_of_children(),2);
        EXPECT_FALSE(n_mesh[0].has_path("nestsets"));
        EXPECT_FALSE(n_mesh[0].has_path("nestsets/nest"));
        EXPECT_FALSE(n_mesh[0].has_path("fields/mask"));

        EXPECT_FALSE(n_mesh[1].has_path("nestsets"));
        EXPECT_FALSE(n_mesh[1].has_path("nestsets/nest"));
        EXPECT_FALSE(n_mesh[1].has_path("fields/mask"));
    }

    // remove all via coordset
    {
        conduit::blueprint::mesh::examples::generate("julia_nestsets_simple",n_mesh);
        n_opts.reset();
        n_opts["coordsets"] = "coords";

        EXPECT_EQ(n_mesh.number_of_children(),2);
        EXPECT_TRUE(n_mesh[0].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[0].has_path("fields/mask"));

        EXPECT_TRUE(n_mesh[1].has_path("nestsets/nest"));
        EXPECT_TRUE(n_mesh[1].has_path("fields/mask"));

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::remove(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);


        EXPECT_EQ(n_mesh.number_of_children(),0);
    }
}