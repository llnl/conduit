// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.
//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_rename.cpp
///
//-----------------------------------------------------------------------------


#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"

#include <string>
#include "gtest/gtest.h"

void echo_title(const std::string &title)
{
    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << title << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;
}

void echo_node(const std::string tag, const conduit::Node &node, bool full = false)
{
    std::cout << tag << std::endl;
    if(full)
    {
        std::cout << node.to_yaml() << std::endl;
    }
    else
    {
        std::cout << node.to_summary_string() << std::endl;
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, bad_options)
{
    echo_title("remove bad options");
    conduit::Node n_mesh;
    conduit::Node n_opts;
    n_opts["topologies"] = 1;
    EXPECT_THROW(conduit::blueprint::mesh::rename(n_opts,n_mesh),conduit::Error);
    // entries are current name to desired naem
    n_opts["topologies/bonkers"] = 1;
    EXPECT_THROW(conduit::blueprint::mesh::rename(n_opts,n_mesh),conduit::Error);
    n_opts.reset();
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, rename_coords)
{
    // coordset change impacts refs in:
    //  topos

    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
    n_mesh_opts["mesh_type"] = "uniform";
    n_mesh_opts["nx"] = 3;
    n_mesh_opts["ny"] = 3;

    echo_title("rename coords");

    conduit::blueprint::mesh::examples::generate("braid",n_mesh_opts,n_mesh);
    EXPECT_TRUE(n_mesh.has_path("coordsets/coords"));
    EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
    EXPECT_EQ(n_mesh["topologies/mesh/coordset"].as_string(),"coords");

    n_opts["coordsets/coords"] = "mycoords";

    echo_node("[before]",n_mesh);
    echo_node("[options]",n_opts);
    conduit::blueprint::mesh::rename(n_opts,n_mesh);
    EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
    echo_node("[after]",n_mesh);
    echo_node("info",n_info);

    EXPECT_FALSE(n_mesh.has_path("coordsets/coords"));
    EXPECT_TRUE(n_mesh.has_path("coordsets/mycoords"));
    EXPECT_EQ(n_mesh["topologies/mesh/coordset"].as_string(),"mycoords");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, rename_topo)
{
    // topo change impacts refs in:
    //  fields, matsets, nestsets, adjsets

    // check fields updates
    {
        echo_title("rename topo check fields");
        conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
        n_mesh_opts["mesh_type"] = "uniform";
        n_mesh_opts["nx"] = 3;
        n_mesh_opts["ny"] = 3;

        conduit::blueprint::mesh::examples::generate("braid",n_mesh_opts,n_mesh);
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("fields/braid"));
        EXPECT_EQ(n_mesh["fields/braid/topology"].as_string(),"mesh");

        n_opts["topologies/mesh"] = "topo";

        echo_node("[before]",n_mesh);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::rename(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("info",n_info);

        EXPECT_FALSE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("topologies/topo"));
        EXPECT_EQ(n_mesh["fields/braid/topology"].as_string(),"topo");
        EXPECT_EQ(n_mesh["fields/radial/topology"].as_string(),"topo");
        EXPECT_EQ(n_mesh["fields/vel/topology"].as_string(),"topo");
    }

    // check matsets updates
    {
        echo_title("rename topo check matsets");
        conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
        n_mesh_opts["matset_type"] = "full";
        n_mesh_opts["nx"] = 4;
        n_mesh_opts["ny"] = 4;
        n_mesh_opts["radius"] = 1;

        conduit::blueprint::mesh::examples::generate("venn",n_mesh_opts,n_mesh);
        EXPECT_TRUE(n_mesh.has_path("topologies/topo"));
        EXPECT_EQ(n_mesh["fields/importance/topology"].as_string(),"topo");
        EXPECT_EQ(n_mesh["matsets/matset/topology"].as_string(),"topo");

        n_opts["topologies/topo"] = "mytopo";

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::rename(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        EXPECT_TRUE(n_mesh.has_path("topologies/mytopo"));
        EXPECT_EQ(n_mesh["fields/importance/topology"].as_string(),"mytopo");
        EXPECT_EQ(n_mesh["matsets/matset/topology"].as_string(),"mytopo");
    }

    // check adjsets updates
    {
        echo_title("rename topo check adjsets");
        // adjsets do not have dependent refs
        conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
        conduit::blueprint::mesh::examples::generate("adjset_uniform",n_mesh);
        size_t number_of_doms = 8;
        EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
        for(size_t i=0; i<number_of_doms; i++)
        {
            EXPECT_TRUE(n_mesh[i].has_path("topologies/topo"));
            EXPECT_TRUE(n_mesh[i].has_path("adjsets/adjset"));
            EXPECT_EQ(n_mesh[i]["adjsets/adjset/topology"].as_string(),"topo");
        }

        n_opts["topologies/topo"] = "mytopo";

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::rename(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);

        EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
        for(size_t i=0; i<number_of_doms; i++)
        {
            EXPECT_TRUE(n_mesh[i].has_path("topologies/mytopo"));
            EXPECT_EQ(n_mesh[i]["adjsets/adjset/topology"].as_string(),"mytopo");
        }
    }

    // check nestsets updates
    {
        echo_title("rename topo check nestsets");
        // nestsets do not have dependent refs
        conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
        conduit::blueprint::mesh::examples::generate("julia_nestsets_simple",n_mesh);

        EXPECT_EQ(n_mesh.number_of_children(),2);
        EXPECT_EQ(n_mesh[0]["nestsets/nest/topology"].as_string(),"topo");
        EXPECT_EQ(n_mesh[1]["nestsets/nest/topology"].as_string(),"topo");

        n_opts["topologies/topo"] = "mytopo";

        echo_node("[before]",n_mesh);
        conduit::blueprint::mesh::rename(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh);
        echo_node("[info]",n_info);

        EXPECT_EQ(n_mesh.number_of_children(),2);
        EXPECT_EQ(n_mesh[0]["nestsets/nest/topology"].as_string(),"mytopo");
        EXPECT_EQ(n_mesh[1]["nestsets/nest/topology"].as_string(),"mytopo");
    }
}



//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, rename_fields)
{
    echo_title("rename fields");
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
    n_mesh_opts["mesh_type"] = "uniform";
    n_mesh_opts["nx"] = 3;
    n_mesh_opts["ny"] = 3;

    conduit::blueprint::mesh::examples::generate("braid",n_mesh_opts,n_mesh);
    EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
    EXPECT_TRUE(n_mesh.has_path("fields/braid"));
    EXPECT_EQ(n_mesh["fields/braid/topology"].as_string(),"mesh");

    n_opts["fields/braid"] = "awe";
    n_opts["fields/radial"] = "som";
    n_opts["fields/vel"] = "e";

    echo_node("[before]",n_mesh);
    echo_node("[options]",n_opts);
    conduit::blueprint::mesh::rename(n_opts,n_mesh);
    EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
    echo_node("[after]",n_mesh);
    echo_node("info",n_info);

    EXPECT_FALSE(n_mesh.has_path("fields/braid"));
    EXPECT_FALSE(n_mesh.has_path("fields/radial"));
    EXPECT_FALSE(n_mesh.has_path("fields/vel"));

    EXPECT_TRUE(n_mesh.has_path("fields/awe"));
    EXPECT_TRUE(n_mesh.has_path("fields/som"));
    EXPECT_TRUE(n_mesh.has_path("fields/e"));

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, rename_matsets)
{
    // matset change impacts refs in:
    //  fields, specsets
    {
        echo_title("rename matset");
        conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
        n_mesh_opts["matset_type"] = "full";
        n_mesh_opts["nx"] = 4;
        n_mesh_opts["ny"] = 4;
        n_mesh_opts["radius"] = 1;

        conduit::blueprint::mesh::examples::generate("venn",n_mesh_opts,n_mesh);
        EXPECT_TRUE(n_mesh.has_path("matsets/matset"));
        EXPECT_TRUE(n_mesh.has_path("fields/importance"));
        EXPECT_TRUE(n_mesh.has_path("fields/importance/matset"));
        EXPECT_EQ(n_mesh["fields/importance/matset"].as_string(),"matset");

        n_opts.reset();
        n_opts["matsets/matset"] = "mymatset";

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::rename(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        EXPECT_TRUE(n_mesh.has_path("topologies/topo"));
        EXPECT_TRUE(n_mesh.has_path("matsets/mymatset"));
        EXPECT_TRUE(n_mesh.has_path("fields/importance/matset"));
        EXPECT_EQ(n_mesh["fields/importance/matset"].as_string(),"mymatset");
    }
    // also check that spec ref to matset is updated
    {
        echo_title("rename matset check specsets");
        conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
        n_mesh_opts["mesh_type"] = "specsets";
        n_mesh_opts["nx"] = 5;
        n_mesh_opts["ny"] = 4;
        n_mesh_opts["ny"] = 3;

        conduit::blueprint::mesh::examples::generate("misc",n_mesh);
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("matsets/mesh"));
        EXPECT_TRUE(n_mesh.has_path("specsets/mesh"));

        n_opts.reset();
        n_opts["matsets/mesh"] = "matset";

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::rename(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        EXPECT_TRUE(n_mesh.has_path("matsets/matset"));
        EXPECT_EQ(n_mesh["specsets/mesh/matset"].as_string(),"matset");

    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, rename_specsets)
{
    // specsets do not have dependent refs
    {
        echo_title("rename specset check");
        conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
        n_mesh_opts["mesh_type"] = "specsets";
        n_mesh_opts["nx"] = 5;
        n_mesh_opts["ny"] = 4;
        n_mesh_opts["ny"] = 3;
        conduit::blueprint::mesh::examples::generate("misc",n_mesh);
        EXPECT_TRUE(n_mesh.has_path("topologies/mesh"));
        EXPECT_TRUE(n_mesh.has_path("matsets/mesh"));
        EXPECT_TRUE(n_mesh.has_path("specsets/mesh"));

        n_opts.reset();
        n_opts["specsets/mesh"] = "specset";

        echo_node("[before]",n_mesh,false);
        echo_node("[options]",n_opts);
        conduit::blueprint::mesh::rename(n_opts,n_mesh);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
        echo_node("[after]",n_mesh,false);
        echo_node("info",n_info);

        EXPECT_FALSE(n_mesh.has_path("specsets/mesh"));
        EXPECT_TRUE(n_mesh.has_path("specsets/specset"));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, rename_adjsets)
{
    echo_title("rename adjset");
    // adjsets do not have dependent refs
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
    conduit::blueprint::mesh::examples::generate("adjset_uniform",n_mesh);

    size_t number_of_doms = 8;
    EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
    for(size_t i=0; i<number_of_doms; i++)
    {
        EXPECT_TRUE(n_mesh[i].has_path("adjsets/adjset"));
    }

    n_opts.reset();
    n_opts["adjsets/adjset"] = "myadjset";

    echo_node("[before]",n_mesh);
    conduit::blueprint::mesh::rename(n_opts,n_mesh);
    EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
    echo_node("[after]",n_mesh);
    echo_node("[info]",n_info);

    EXPECT_EQ(n_mesh.number_of_children(),number_of_doms);
    for(size_t i=0; i<number_of_doms; i++)
    {
        EXPECT_FALSE(n_mesh[i].has_path("adjsets/adjset"));
        EXPECT_TRUE(n_mesh[i].has_path("adjsets/myadjset"));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_rename, remove_nestsets)
{
    echo_title("rename nestset");
    // nestsets do not have dependent refs
    conduit::Node n_mesh, n_mesh_opts, n_opts, n_info;
    conduit::blueprint::mesh::examples::generate("julia_nestsets_simple",n_mesh);

    EXPECT_EQ(n_mesh.number_of_children(),2);
    EXPECT_TRUE(n_mesh[0].has_path("nestsets/nest"));
    EXPECT_TRUE(n_mesh[1].has_path("nestsets/nest"));

    n_opts.reset();
    n_opts["nestsets/nest"] = "mynest";

    echo_node("[before]",n_mesh);
    conduit::blueprint::mesh::rename(n_opts,n_mesh);
    EXPECT_TRUE(conduit::blueprint::mesh::verify(n_mesh, n_info));
    echo_node("[after]",n_mesh);
    echo_node("[info]",n_info);

    EXPECT_EQ(n_mesh.number_of_children(),2);
    EXPECT_TRUE(n_mesh[0].has_path("nestsets/mynest"));
    EXPECT_TRUE(n_mesh[1].has_path("nestsets/mynest"));

}
