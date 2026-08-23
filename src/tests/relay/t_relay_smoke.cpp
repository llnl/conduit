// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_smoke.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay.hpp"
#include "conduit_relay_io.hpp"
#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;

TEST(conduit_relay_smoke, about)
{
    std::cout << relay::about() << std::endl;
    Node n;
    
    std::cout << n.to_json() << std::endl;
    
    std::cout << n.dtype().to_string() << std::endl;
    std::cout << n.number_of_children() << std::endl;
    n.set(DataType::object());

    std::cout << n.dtype().to_string() << std::endl;
    std::cout << n.number_of_children() << std::endl;
    
    
    n["here"].set(DataType::empty());
    conduit::relay::io::save(n,"here.hdf5");
    
    Node n2;

    conduit::relay::io::load("here.hdf5",n2);
    std::cout << n2.to_json() << std::endl;

}
