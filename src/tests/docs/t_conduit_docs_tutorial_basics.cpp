// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_docs_tutorial_examples.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_relay.hpp"
#include "t_conduit_docs_tutorial_helpers.hpp"

#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_very_basic)
{
    BEGIN_EXAMPLE("basics_very_basic");
    Node n;
    n["my"] = "data";
    n.print(); 
    END_EXAMPLE("basics_very_basic");
}

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_hierarchial)
{
    BEGIN_EXAMPLE("basics_hierarchial");
    Node n;
    n["my"] = "data";
    n["a/b/c"] = "d";
    n["a"]["b"]["e"] = 64.0;
    n.print();

    std::cout << "total bytes: " << n.total_strided_bytes() << std::endl;
    END_EXAMPLE("basics_hierarchial");
}

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_object_and_list)
{
    BEGIN_EXAMPLE("basics_object_and_list");
    Node n;
    n["object_example/val1"] = "data";
    n["object_example/val2"] = 10u;
    n["object_example/val3"] = 3.1415;
    
    for(int i = 0; i < 5 ; i++ )
    {
        Node &list_entry = n["list_example"].append();
        list_entry.set(i);
    }
    
    n.print();
    END_EXAMPLE("basics_object_and_list");
}

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_object_and_list_itr)
{
    BEGIN_EXAMPLE("basics_object_and_list_itr");
    Node n;
    n["object_example/val1"] = "data";
    n["object_example/val2"] = 10u;
    n["object_example/val3"] = 3.1415;
    
    for(int i = 0; i < 5 ; i++ )
    {
        Node &list_entry = n["list_example"].append();
        list_entry.set(i);
    }

    n.print();

    NodeIterator itr = n["object_example"].children();
    while(itr.has_next())
    {
        Node &cld = itr.next();
        std::string cld_name = itr.name();
        std::cout << cld_name << ": " << cld.to_string() << std::endl;
    }

    std::cout << std::endl;

    itr = n["list_example"].children();
    while(itr.has_next())
    {
        Node &cld = itr.next();
        std::cout << cld.to_string() << std::endl;
    }
    END_EXAMPLE("basics_object_and_list_itr");
}


//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_mem_spaces)
{
    BEGIN_EXAMPLE("basics_mem_spaces");
    Node n;
    n["my"] = "data";
    n["a/b/c"] = "d";
    n["a"]["b"]["e"] = 64.0;

    Node ninfo;
    n.info(ninfo);
    ninfo.print();
    END_EXAMPLE("basics_mem_spaces");
}

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_bw_style)
{
    BEGIN_EXAMPLE("basics_bw_style");
    Node n;
    uint32 val = 100;
    n["test"] = val;
    n.print();
    n.print_detailed();
    END_EXAMPLE("basics_bw_style");
}

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_bw_style_from_native)
{
    BEGIN_EXAMPLE("basics_bw_style_from_native");
    Node n;
    int val = 100;
    n["test"] = val;
    n.print_detailed();
    END_EXAMPLE("basics_bw_style_from_native");
}

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_node_refs)
{
    BEGIN_EXAMPLE("basics_node_refs");

    ////////////////////////////////////////////////////////////
    // In C++, use Node references!
    ////////////////////////////////////////////////////////////
    // Using Node references is common (good) pattern!

    // setup a node
    Node root;
    // set data in hierarchy 
    root["my/nested/path"] = 0.0;
    // display the contents
    root.print();

    // Get a ref to the node in the tree
    Node &data = root["my/nested/path"];
    // change the value
    data = 42.0;
    // display the contents
    root.print();

    END_EXAMPLE("basics_node_refs");
}

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_node_refs_bad)
{
    BEGIN_EXAMPLE("basics_node_refs_bad");
    ////////////////////////////////////////////////////////////
    // C++ anti-pattern to avoid: copy instead of reference
    ////////////////////////////////////////////////////////////

    // setup a node
    Node root;
    // set data in hierarchy 
    root["my/nested/path"] = 0.0;

    // display the contents
    root.print();

    // In this case, notice we aren't using a reference.
    // This creates a copy, disconnected from the orignal tree!
    // This is probably not what you are looking for ...
    Node data = root["my/nested/path"];
    // change the value
    data = 42.0;

    // display the contents
    root.print();

    END_EXAMPLE("basics_node_refs_bad");
}

//-----------------------------------------------------------------------------
// helpers for basics_const_vs_non_const
//-----------------------------------------------------------------------------

// BEGIN_BLOCK("basics_const_example")

// with non-const references, you can modify the node, 
// leading to surprises in cases were read-only 
// validation and processing is intended
void important_suprise(Node &data)
{
    // if this doesn't exist, we will get a new empty node
    // Note: we could also ask if the path exists via Node:has_path()
    int val = data["my/important/data"].to_int();
    std::cout << "\n==> important: " << val << std::endl;
}

// with const references,  the api provides checks
// that help
void important(const Node &data)
{
    // if this doesn't exist, const access will trigger exception here
    // Note: we could also ask if the path exists via Node:has_path()
    int val = data["my/important/data"].to_int();
    std::cout << "\n==> important: " << val << std::endl;
}

// END_BLOCK("basics_const_example")

//-----------------------------------------------------------------------------
TEST(conduit_tutorial, basics_const_vs_non_const)
{
    BEGIN_EXAMPLE("basics_const_vs_non_const");
    ////////////////////////////////////////////////////////////
    // In C++, leverage const refs for processing existing nodes
    ////////////////////////////////////////////////////////////

    // setup a node
    Node n1;
    n1["my/important/but/mistyped/path/to/data"] = 42.0;

    std::cout << "== n1 == " << std::endl;
    n1.print();

    // method with non-const arg drives on ...
    try
    {
        important_suprise(n1);
    }
    catch(conduit::Error &e)
    {
        e.print();
    }

    // check n1, was it was modified ( yes ... )
    std::cout << "n1 after calling `important_suprise`" << std::endl;
    n1.print();

    Node n2;
    n2["my/important/but/mistyped/path/to/data"] = 42.0;

    std::cout << "== n2 == " << std::endl;
    n2.print();

    // method with const arg lets us know, and also makes sure 
    // the node structure isn't modified
    try
    {
        important(n2);
    }
    catch(conduit::Error &e)
    {
        e.print();
    }

    // check n2, was it was modified ( no ... )
    std::cout << "n2 after calling `important`" << std::endl;
    n2.print();


    END_EXAMPLE("basics_const_vs_non_const");
}

