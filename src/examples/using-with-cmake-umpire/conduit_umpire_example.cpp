// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_umpire_example.cpp
///
//-----------------------------------------------------------------------------

#include <iostream>

#include "conduit.hpp"
#include "conduit_relay.hpp"
#include "conduit_blueprint.hpp"

#include "umpire/ResourceManager.hpp"
#include "umpire/Allocator.hpp"

int main(int argc, char **argv)
{
    // Hello from Conduit
    conduit::Node about;
    conduit::about(about["conduit"]);
    conduit::relay::about(about["conduit/relay"]);
    conduit::relay::io::about(about["conduit/relay/io"]);
    conduit::blueprint::about(about["conduit/blueprint"]);

    std::cout << about.to_yaml() << std::endl;

    // Hello from Umpire
    auto& rm = umpire::ResourceManager::getInstance();
    umpire::Allocator alloc = rm.getAllocator("HOST");

    std::cout << "Got allocator: " << alloc.getName() << std::endl;

    std::cout << "Available allocators: ";
    for (auto s : rm.getAllocatorNames()){
        std::cout << s << "  ";
    }
    std::cout << std::endl;

    return 0;
}


