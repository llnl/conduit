// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: adjset_validate.cpp
///
//-----------------------------------------------------------------------------

#include <conduit.hpp>
#include <conduit_blueprint.hpp>
#include <conduit_relay.hpp>
#include <conduit_blueprint_mesh_topology_metadata.hpp>
#include <conduit_blueprint_mesh_utils.hpp>

#include <iostream>
#include <cstring>
#include <numeric>
#include <cstdio>

// NOTE: This is a serial prototype.

//---------------------------------------------------------------------------
std::vector<const conduit::Node *>
GetAdjsets(const conduit::Node &doms)
{
    std::vector<const conduit::Node *> adjsets;
    auto domains = conduit::blueprint::mesh::domains(doms);
    for(const auto &dom : domains)
    {
        if(dom->has_child("adjsets"))
        {
            const auto srcNode = dom->fetch_ptr("adjsets");
            for(conduit::index_t i = 0; i < srcNode->number_of_children(); i++)
                adjsets.push_back(srcNode->child_ptr(i));
            break;
        }
    }
    return adjsets;
}

//---------------------------------------------------------------------------
/**
 @brief If the error information included coordinates then we add a new
        coordset+topo that includes where the failures occurred.

 @param adjsetName The name of the current adjset.
 @param info A node that contains error messages about the adjset.
 @param[out] n A node that will contain the new point mesh.
 */
void
addPointMesh(const std::string &adjsetName, const conduit::Node &info, conduit::Node &n)
{
    std::vector<double> cx, cy, cz;
    std::vector<int> domain, vertex, neighbor;

    for(conduit::index_t domainId = 0; domainId < info.number_of_children(); domainId++)
    {
        const conduit::Node &dom = info[domainId];
        std::string domainName(dom.name());

        for(conduit::index_t adjsetId = 0; adjsetId < dom.number_of_children(); adjsetId++)
        {
            const conduit::Node &adjset = dom[adjsetId];

            for(conduit::index_t groupId = 0; groupId < adjset.number_of_children(); groupId++)
            {
                const conduit::Node &group = adjset[groupId];
                for(conduit::index_t errId = 0; errId < group.number_of_children(); errId++)
                {
                    const conduit::Node &err = group[errId];
                    if(err.has_child("coordinate") && err.has_child("neighbor"))
                    {
                        auto da = err["coordinate"].as_double_accessor();
                        cx.push_back(da[0]);
                        if(da.number_of_elements() > 1)
                            cy.push_back(da[1]);
                        if(da.number_of_elements() > 2)
                            cz.push_back(da[2]);

                        domain.push_back(static_cast<int>(domainId));
                        vertex.push_back(err["vertex"].to_int());
                        neighbor.push_back(err["neighbor"].to_int());
                    }
                }
            }
        }
    }

    // If we had some errors, we'll have coordinates. Add a new coordset+topo.
    if(!cx.empty())
    {
        // Add coordset.
        std::string coordsetName("coords_" + adjsetName);
        conduit::Node &coordset = n["coordsets/" + coordsetName];
        coordset["type"] = "explicit";
        coordset["values/x"].set(cx);
        if(!cy.empty())
            coordset["values/y"].set(cy);
        if(!cz.empty())
            coordset["values/z"].set(cz);

        // Add topo
        std::string topoName(adjsetName);
        conduit::Node &topo = n["topologies/" + topoName];
        topo["coordset"] = coordsetName;
        topo["type"] = "points";

        // Add fields.
        conduit::Node &fields = n["fields"];
        conduit::Node &f1 = fields[adjsetName + "_domain"];
        f1["topology"] = topoName;
        f1["association"] = "vertex";
        f1["values"].set(domain);

        conduit::Node &f2 = fields[adjsetName + "_neighbor"];
        f2["topology"] = topoName;
        f2["association"] = "vertex";
        f2["values"].set(neighbor);

        conduit::Node &f3 = fields[adjsetName + "_vertex"];
        f3["topology"] = topoName;
        f3["association"] = "vertex";
        f3["values"].set(vertex);

        n["state/domain_id"] = 0;        
    }
}

//---------------------------------------------------------------------------
void
printUsage(const char *program)
{
    std::cout << "Usage: " << program << " -input filename [-output filebase] [-protocol name] [-help]" << std::endl;
    std::cout << std::endl;
    std::cout << "Argument         Description" << std::endl;
    std::cout << "================ ============================================================" << std::endl;
    std::cout << "-input filename  Set the input filename." << std::endl;
    std::cout << "-output filebase The base filename to use when outputting point mesh files." << std::endl;
    std::cout << "-protocol name   The name of the Conduit protocol to use when writing point mesh files." << std::endl;
    std::cout << std::endl;
    std::cout << "-help            Print the usage and exit." << std::endl;
    std::cout << std::endl;
}

//---------------------------------------------------------------------------
static void conduit_debug_err_handler(const std::string &s1, const std::string &s2, int i1)
{
    std::cout << "s1=" << s1 << ", s2=" << s2 << ", i1=" << i1 << std::endl;
    // This is on purpose.
    while(1)
      ;
}

//---------------------------------------------------------------------------
/**
 * @brief Get the topology node for the specified adjset
 *
 * @param n_mesh The mesh node.
 * @param adjsetName The name of the adjset that identifies the topology.
 *
 * @return A reference to the topology. If it does not exist, Conduit will
 *         throw an exception.
 */
const conduit::Node &
adjset_topology(const conduit::Node &n_mesh, const std::string &adjsetName)
{
    const conduit::Node &n_adjset = n_mesh.fetch_existing("adjsets/" + adjsetName);
    const std::string topoName = n_adjset.fetch_existing("topology").as_string();
    return n_mesh.fetch_existing("topologies/" + topoName);
}

//---------------------------------------------------------------------------
/**
 * @brief Test whether an adjset's topology contains lines.
 *
 * @param n_mesh The mesh we're testing. It might have multiple domains!
 * @param adjsetName The name of the adjset that identifies the topology.
 *
 * @return True if the topology contains lines, false otherwise.
 */
bool is_line_topology(const conduit::Node &n_mesh, const std::string &adjsetName)
{
    bool is_line = false;
    auto domains = conduit::blueprint::mesh::domains(n_mesh);
    for(const auto &dom : domains)
    {
        const conduit::Node &n_topo = adjset_topology(*dom, adjsetName);
        const std::string type = n_topo.fetch_existing("type").as_string();
        if(type == "unstructured")
        {
            is_line = n_topo.fetch_existing("elements/shape").as_string() == "line";
            break;
        }
    }
    return is_line;
}

//---------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
    std::string input, output, protocol;
    double tolerance = conduit::blueprint::mesh::utils::query::PointQueryBase::DEFAULT_POINT_TOLERANCE;

    // Set default protocol. Use HDF5 if present.
    conduit::Node props;
    conduit::relay::io::about(props);
    if(props.has_path("protocols/hdf5"))
    {
        if(props["protocols/hdf5"].as_string() == "enabled")
            protocol = "hdf5";
    }
    else
    {
        protocol = "yaml";
    }

    // Handle command line args.
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printUsage(argv[0]);
            return -1;
        }
        else if(strcmp(argv[i], "-input") == 0 && (i+1) < argc)
        {
            input = argv[i+1];
            i++;
        }
        else if(strcmp(argv[i], "-output") == 0 && (i+1) < argc)
        {
            output = argv[i+1];
            i++;
        }
        else if(strcmp(argv[i], "-protocol") == 0 && (i+1) < argc)
        {
            protocol = argv[i+1];
            i++;
        }
        else if(strcmp(argv[i], "-tolerance") == 0 && (i+1) < argc)
        {
            tolerance = atof(argv[i+1]);
            i++;
        }
        else if(strcmp(argv[i], "-handler") == 0)
        {
            conduit::utils::set_error_handler(conduit_debug_err_handler);
        }
    }

    if(input.empty())
    {
        printUsage(argv[0]);
        return -1;
    }

    int retval = 0;
    try
    {
        // Load the mesh
        conduit::Node root;
        conduit::relay::io::blueprint::load_mesh(input, root);

        // Get adjsets.
        std::vector<const conduit::Node *> adjsets(GetAdjsets(root));

        // Look through the adjsets to see if the points are all good.
        bool err = false;
        conduit::Node pointMeshes;
        for(size_t i = 0; i < adjsets.size(); i++)
        {
            std::string adjsetName(adjsets[i]->name());

            // Get the adjset association.
            std::string association;
            if(adjsets[i]->has_path("association"))
                association = adjsets[i]->fetch_existing("association").as_string();

            conduit::Node info, opts;
            opts["tolerance"] = tolerance;

            bool res = conduit::blueprint::mesh::utils::adjset::validate(root, adjsetName, info, opts);
            if(res)
            {
                // If the adjset is vertex associated then compare the points in
                // it to make sure that they are the same on each side of the boundary.
                //
                // We can do this also for element associated adjsets that contain lines
                // since in that case compare_pointwise will make points at the line segment
                // midpoints. This allows us to check that the lines are in same order.
                //
                if(association == "vertex" ||
                   (association == "element" && is_line_topology(root, adjsetName)))
                {
                    res = conduit::blueprint::mesh::utils::adjset::compare_pointwise(root, adjsetName, info, opts);
                }

                if(res)
                {
                    std::cout << "Check " << association << " adjset " << adjsetName << "... PASS" << std::endl;
                }
                else
                {
                    std::cout << "Check " << association << " adjset " << adjsetName << "... FAIL: The adjset points have different orders" << std::endl;
                    info.print();
                    // If we're outputting, make a point mesh of the differences.
                    if(!output.empty())
                        addPointMesh(adjsetName, info, pointMeshes);
                    err = true;
                }
            }
            else
            {
                std::cout << "Check " << association << " adjset " << adjsetName << "... FAIL: The adjsets contain errors." << std::endl;
                info.print();
                // If we're outputting, make a point mesh of the differences.
                if(!output.empty())
                    addPointMesh(adjsetName, info, pointMeshes);
                err = true;
            }

            // If we're outputting. write the adjsets as point meshes that we can look at.
            if(!output.empty())
                conduit::blueprint::mesh::utils::adjset::to_topo(root, adjsetName, pointMeshes);
        }
        // Write any point meshes that were created.
        if(!output.empty() && pointMeshes.number_of_children() > 0)
        {
            conduit::relay::io::blueprint::save_mesh(pointMeshes, output, protocol);
        }
        if(err)
            return -2;
    }
    catch(std::exception &err)
    {
        std::cout << err.what() << std::endl;
        retval = -3;
    }

    return retval;
}
