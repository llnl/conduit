// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: silo_test_utils.hpp
///
//-----------------------------------------------------------------------------

#ifndef SILO_TEST_UTILS_HPP
#define SILO_TEST_UTILS_HPP

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "t_config.hpp"

using namespace conduit;
using namespace conduit::utils;

// Some utility functions for SILO tests.

//-----------------------------------------------------------------------------
std::string
relay_test_silo_data_path(const std::string &test_fname)
{
    std::string res = utils::join_file_path(CONDUIT_T_SRC_DIR, "relay");
    res = utils::join_file_path(res, "data");
    res = utils::join_file_path(res, "silo");
    return utils::join_file_path(res, test_fname);
}

//-----------------------------------------------------------------------------
// The Blueprint to Silo to Blueprint round trip will
// transform uniform to rectilinear so we will do the same
// to allow diffs to succeed
void
silo_uniform_to_rect_conversion(const std::string &coordset_name,
                                const std::string &topo_name,
                                Node &save_mesh)
{
    Node save_mesh_rect;
    Node &save_mesh_rect_coords = save_mesh_rect["coordsets"][coordset_name];
    Node &save_mesh_rect_topo = save_mesh_rect["topologies"][topo_name];
    blueprint::mesh::topology::uniform::to_rectilinear(
        save_mesh["topologies"][topo_name],
        save_mesh_rect_topo, save_mesh_rect_coords);
    save_mesh["topologies"][topo_name].set(save_mesh_rect_topo);
    save_mesh["coordsets"][coordset_name].set(save_mesh_rect_coords);
}

//-----------------------------------------------------------------------------
// The Blueprint to Silo transformation changes several names
// and some information is lost. We manually make changes so
// that the diff will pass.
void
silo_name_changer(const std::string &mmesh_name,
                  Node &save_mesh)
{
    std::map<std::string, std::string> old_to_new_names;

    if (! save_mesh.has_path("state/domain_id"))
    {
        save_mesh["state"]["domain_id"] = 0;
    }
    if (! save_mesh.has_path("state/cycle"))
    {
        // this is to pass the diff, as silo will add cycle in if it is not there
        save_mesh["state/cycle"] = 0;
    }

    if (save_mesh.has_child("topologies"))
    {
        auto topo_itr = save_mesh["topologies"].children();
        while(topo_itr.has_next())
        {
            Node &n_topo = topo_itr.next();
            const std::string topo_name = topo_itr.name();
            const std::string new_topo_name = mmesh_name + "_" + topo_name;

            old_to_new_names[topo_name] = new_topo_name;

            const std::string coordset_name = n_topo["coordset"].as_string();
            const std::string new_coordset_name = mmesh_name + "_" + topo_name;

            // change the coordset this topo refers to
            save_mesh["topologies"][topo_name]["coordset"].reset();
            save_mesh["topologies"][topo_name]["coordset"] = new_coordset_name;

            // change the name of the topo
            save_mesh["topologies"].rename_child(topo_name, new_topo_name);

            // change the name of the coordset
            if (save_mesh.has_path("coordsets/" + coordset_name))
            {
                // add default labels if they don't exist already
                if (! save_mesh["coordsets"][coordset_name].has_child("labels") &&
                    save_mesh["topologies"][new_topo_name]["type"].as_string() != "points")
                {
                    const int ndims = save_mesh["coordsets"][coordset_name]["values"].number_of_children();

                    const std::string &x_axis_label = save_mesh["coordsets"][coordset_name]["values"][0].name();
                    save_mesh["coordsets"][coordset_name]["labels"][x_axis_label] = "X Axis";
                    if (ndims > 1)
                    {
                        const std::string &y_axis_label = save_mesh["coordsets"][coordset_name]["values"][1].name();
                        save_mesh["coordsets"][coordset_name]["labels"][y_axis_label] = "Y Axis";
                    }
                    if (ndims > 2)
                    {
                        const std::string &z_axis_label = save_mesh["coordsets"][coordset_name]["values"][2].name();
                        save_mesh["coordsets"][coordset_name]["labels"][z_axis_label] = "Z Axis";
                    }
                }

                save_mesh["coordsets"].rename_child(coordset_name, new_coordset_name);
            }
        }
    }

    if (save_mesh.has_child("matsets"))
    {
        auto matset_itr = save_mesh["matsets"].children();
        while (matset_itr.has_next())
        {
            Node &n_matset = matset_itr.next();
            const std::string matset_name = matset_itr.name();

            const std::string old_topo_name = n_matset["topology"].as_string();

            if (old_to_new_names.find(old_topo_name) == old_to_new_names.end())
            {
                continue;
                // If this is the case, we probably need to delete this matset.
                // But our job in this function is just to rename things, so we
                // will just skip.
            }
            const std::string new_topo_name = old_to_new_names[old_topo_name];

            // use new topo name
            n_matset["topology"].reset();
            n_matset["topology"] = new_topo_name;

            // come up with new matset name
            const std::string new_matset_name = mmesh_name + "_" + matset_name;

            old_to_new_names[matset_name] = new_matset_name;

            // rename the matset
            save_mesh["matsets"].rename_child(matset_name, new_matset_name);
        }
    }

    if (save_mesh.has_child("specsets"))
    {
        auto specset_itr = save_mesh["specsets"].children();
        while (specset_itr.has_next())
        {
            Node &n_specset = specset_itr.next();
            const std::string specset_name = specset_itr.name();

            const std::string old_matset_name = n_specset["matset"].as_string();

            if (old_to_new_names.find(old_matset_name) == old_to_new_names.end())
            {
                continue;
                // If this is the case, we probably need to delete this specset.
                // But our job in this function is just to rename things, so we
                // will just skip.
            }

            // TODO I assume the specset is multi_buffer full
            // we have no way to check for now
            // and multi_buffer full is the only allowed specset variety
            // later, we will need to add functionality here

            const std::string &new_matset_name = old_to_new_names[old_matset_name];
            const Node &n_matset = save_mesh["matsets"][new_matset_name];
            const int num_zones = blueprint::mesh::matset::count_zones_from_matset(n_matset);
            Node &matset_values = n_specset["matset_values"];

            // we must modify the specset; see note below
            for (int zone_id = 0; zone_id < num_zones; zone_id ++)
            {
                auto mat_itr = matset_values.children();
                while (mat_itr.has_next())
                {
                    Node &mat = mat_itr.next();
                    const std::string matname = mat_itr.name();
                    if (! blueprint::mesh::matset::is_material_in_zone(n_matset, matname, zone_id))
                    {
                        // if this material is not present in the zone, then we need
                        // to change the species mass fractions to all zero to pass
                        // the diff, since Silo read will set everything to zero
                        // for multi_buffer full.

                        auto spec_itr = mat.children();
                        while (spec_itr.has_next())
                        {
                            Node &spec = spec_itr.next();
                            const std::string specname = spec_itr.name();
                            float64_accessor mass_fractions = spec.value();
                            mass_fractions.set(zone_id, 0.0);
                        }
                    }
                }
            }

            // use new matset name
            n_specset["matset"].reset();
            n_specset["matset"] = new_matset_name;

            // come up with new specset name
            const std::string new_specset_name = mmesh_name + "_" + specset_name;

            old_to_new_names[specset_name] = new_specset_name;

            // rename the specset
            save_mesh["specsets"].rename_child(specset_name, new_specset_name);
        }
    }

    if (save_mesh.has_child("fields"))
    {
        auto field_itr = save_mesh["fields"].children();
        while (field_itr.has_next())
        {
            Node &n_field = field_itr.next();
            const std::string field_name = field_itr.name();

            const std::string old_topo_name = n_field["topology"].as_string();
            if (old_to_new_names.find(old_topo_name) == old_to_new_names.end())
            {
                continue;
                // If this is the case, we probably need to delete this field.
                // But our job in this function is just to rename things, so we
                // will just skip.
            }
            const std::string new_topo_name = old_to_new_names[old_topo_name];
            // use new topo name
            n_field["topology"].reset();
            n_field["topology"] = new_topo_name;

            if (n_field.has_child("matset"))
            {
                const std::string old_matset_name = n_field["matset"].as_string();
                if (old_to_new_names.find(old_matset_name) == old_to_new_names.end())
                {
                    continue;
                    // If this is the case, we probably need to delete this field.
                    // But our job in this function is just to rename things, so we
                    // will just skip.
                }
                const std::string new_matset_name = old_to_new_names[old_matset_name];
                // use new matset name
                n_field["matset"].reset();
                n_field["matset"] = new_matset_name;
            }

            // remove vol dep
            if (n_field.has_child("volume_dependent"))
            {
                n_field.remove_child("volume_dependent");
            }

            // we need to rename vector components
            if (n_field["values"].dtype().is_object())
            {
                if (n_field["values"].number_of_children() > 0)
                {
                    int child_index = 0;
                    auto val_itr = n_field["values"].children();
                    while (val_itr.has_next())
                    {
                        val_itr.next();
                        const std::string comp_name = val_itr.name();

                        // rename vector components
                        n_field["values"].rename_child(comp_name, std::to_string(child_index));

                        child_index ++;
                    }
                }
            }

            // come up with new field name
            std::string new_field_name = mmesh_name + "_" + field_name;

            // rename the field
            save_mesh["fields"].rename_child(field_name, new_field_name);
        }
    }
}

//-----------------------------------------------------------------------------
// The Blueprint to Overlink transformation changes several names
// and some information is lost. We manually make changes so
// that the diff will pass.
void
overlink_name_changer(conduit::Node &save_mesh)
{
    // handle state
    if (! save_mesh.has_path("state/domain_id"))
    {
        save_mesh["state"]["domain_id"] = 0;
    }
    if (! save_mesh.has_path("state/cycle"))
    {
        save_mesh["state/cycle"] = 0;
    }

    // we assume 1 coordset and 1 topo
    Node &coordsets = save_mesh["coordsets"];
    Node &topologies = save_mesh["topologies"];

    // we assume only 1 child for each
    const std::string coordset_name = coordsets.children().next().name();
    const std::string topo_name = topologies.children().next().name();

    // add default labels if they don't exist already
    if (! save_mesh["coordsets"][coordset_name].has_child("labels") &&
        save_mesh["topologies"][topo_name]["type"].as_string() != "points")
    {
        const int ndims = save_mesh["coordsets"][coordset_name]["values"].number_of_children();

        const std::string &x_axis_label = save_mesh["coordsets"][coordset_name]["values"][0].name();
        save_mesh["coordsets"][coordset_name]["labels"][x_axis_label] = "X Axis";
        if (ndims > 1)
        {
            const std::string &y_axis_label = save_mesh["coordsets"][coordset_name]["values"][1].name();
            save_mesh["coordsets"][coordset_name]["labels"][y_axis_label] = "Y Axis";
        }
        if (ndims > 2)
        {
            const std::string &z_axis_label = save_mesh["coordsets"][coordset_name]["values"][2].name();
            save_mesh["coordsets"][coordset_name]["labels"][z_axis_label] = "Z Axis";
        }
    }

    // rename the coordset and references to it
    coordsets.rename_child(coordset_name, "MMESH");
    topologies[topo_name]["coordset"].reset();
    topologies[topo_name]["coordset"] = "MMESH";

    // rename the topo
    topologies.rename_child(topo_name, "MMESH");

    if (save_mesh.has_child("adjsets"))
    {
        // we assume 1 adjset
        Node &n_adjset = save_mesh["adjsets"].children().next();
        const std::string adjset_name = n_adjset.name();

        // use new topo name
        n_adjset["topology"].reset();
        n_adjset["topology"] = "MMESH";


        if (adjset_name != "adjset")
        {
            // rename the adjset
            save_mesh["adjsets"].rename_child(adjset_name, "adjset");
        }
    }

    if (save_mesh.has_child("matsets"))
    {
        // you can have multiple matsets when saving, provided you only have
        // one per topo. But if you want the diff to pass for tests, you
        // really can only have one. So we assume one.
        Node &n_matset = save_mesh["matsets"].children().next();
        const std::string matset_name = n_matset.name();

        // use new topo name
        n_matset["topology"].reset();
        n_matset["topology"] = "MMESH";

        // rename the matset
        save_mesh["matsets"].rename_child(matset_name, "MMATERIAL");
    }

    if (save_mesh.has_child("specsets"))
    {
        int species_id = 0;

        auto specset_itr = save_mesh["specsets"].children();
        while (specset_itr.has_next())
        {
            Node &n_specset = specset_itr.next();
            const std::string specset_name = specset_itr.name();

            // TODO I assume the specset is multi_buffer full
            // we have no way to check for now
            // and multi_buffer full is the only allowed specset variety
            // later, we will need to add functionality here

            const Node &n_matset = save_mesh["matsets"]["MMATERIAL"];
            const int num_zones = blueprint::mesh::matset::count_zones_from_matset(n_matset);
            Node &matset_values = n_specset["matset_values"];

            // we must modify the specset; see note below
            for (int zone_id = 0; zone_id < num_zones; zone_id ++)
            {
                auto mat_itr = matset_values.children();
                while (mat_itr.has_next())
                {
                    Node &mat = mat_itr.next();
                    const std::string matname = mat_itr.name();
                    if (! blueprint::mesh::matset::is_material_in_zone(n_matset, matname, zone_id))
                    {
                        // if this material is not present in the zone, then we need
                        // to change the species mass fractions to all zero to pass
                        // the diff, since Silo read will set everything to zero
                        // for multi_buffer full.

                        auto spec_itr = mat.children();
                        while (spec_itr.has_next())
                        {
                            Node &spec = spec_itr.next();
                            const std::string specname = spec_itr.name();
                            float64_accessor mass_fractions = spec.value();
                            mass_fractions.set(zone_id, 0.0);
                        }
                    }
                }
            }

            // use new matset name
            n_specset["matset"].reset();
            n_specset["matset"] = "MMATERIAL";

            // rename the specset
            if (species_id == 0)
            {
                save_mesh["specsets"].rename_child(specset_name, "MSPECIES");
            }
            else
            {
                save_mesh["specsets"].rename_child(specset_name, "MSPECIES" + std::to_string(species_id));
            }
            species_id ++;
        }
    }

    if (save_mesh.has_child("fields"))
    {
        auto field_itr = save_mesh["fields"].children();
        while (field_itr.has_next())
        {
            Node &n_field = field_itr.next();

            // use new topo name
            n_field["topology"].reset();
            n_field["topology"] = "MMESH";

            if (n_field.has_child("matset"))
            {
                // use new matset name
                n_field["matset"].reset();
                n_field["matset"] = "MMATERIAL";
            }

            // overlink tracks volume dependence so we do not have to remove it
            // but we should add it in if it is not present
            // remove vol dep
            if (! n_field.has_child("volume_dependent"))
            {
                n_field["volume_dependent"] = "false";
            }

            // there are only scalar variables for overlink so we do not
            // need to worry about renaming vector components.
            if (n_field["values"].dtype().is_object())
            {
                CONDUIT_ERROR("Overlink only allows scalar variables. You are doing this wrong.");
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
add_matset_to_spiral(Node &n_mesh, const int ndomains)
{
    // Add a matset to each domain
    for (index_t domain_id = 0; domain_id < n_mesh.number_of_children(); domain_id ++)
    {
        Node &domain = n_mesh[domain_id];
        const auto num_elements = blueprint::mesh::topology::length(domain["topologies/topo"]);
        Node &matset = domain["matsets/matset"];
        // add a matset to it
        matset["topology"].set("topo");

        // Uni buffer requires material map
        for(index_t i = 0; i < ndomains; i ++)
        {
            const std::string mat_name("mat" + std::to_string(i));
            matset["material_map"][mat_name].set((int32) i);
        }

        Node &mat_ids = matset["material_ids"];
        mat_ids.set_dtype(DataType::index_t(num_elements));
        index_t_array ids = mat_ids.value();
        for (index_t i = 0; i < ids.number_of_elements(); i++)
        {
            ids[i] = domain_id;
        }

        Node &mat_vfs = matset["volume_fractions"];
        mat_vfs.set_dtype(DataType::c_float(num_elements));
        float_array data = mat_vfs.value();
        for (index_t i = 0; i < data.number_of_elements(); i++)
        {
            data[i] = 1.f;
        }
    }
}

//-----------------------------------------------------------------------------
void
vector_field_to_scalars_braid(Node &n_mesh, const std::string &dim)
// TODO generalize this approach so that it works for all fields as part of overlink_name_changer
{
    Node &field_vel = n_mesh["fields"]["vel"];

    Node &field_vel_u = n_mesh["fields"]["vel_u"];
    field_vel_u["topology"].set(field_vel["topology"]);
    field_vel_u["association"].set(field_vel["association"]);
    field_vel_u["values"].set(field_vel["values/u"]);
    if (field_vel.has_child("units"))
    {
        field_vel_u["units"].set(field_vel["units"]);
    }
    if (field_vel.has_child("label"))
    {
        field_vel_u["label"].set(field_vel["label"]);
    }

    Node &field_vel_v = n_mesh["fields"]["vel_v"];
    field_vel_v["topology"].set(field_vel["topology"]);
    field_vel_v["association"].set(field_vel["association"]);
    field_vel_v["values"].set(field_vel["values/v"]);
    if (field_vel.has_child("units"))
    {
        field_vel_v["units"].set(field_vel["units"]);
    }
    if (field_vel.has_child("label"))
    {
        field_vel_v["label"].set(field_vel["label"]);
    }

    if (dim == "3")
    {
        Node &field_vel_w = n_mesh["fields"]["vel_w"];
        field_vel_w["topology"].set(field_vel["topology"]);
        field_vel_w["association"].set(field_vel["association"]);
        field_vel_w["values"].set(field_vel["values/w"]);
        if (field_vel.has_child("units"))
        {
            field_vel_w["units"].set(field_vel["units"]);
        }
        if (field_vel.has_child("label"))
        {
            field_vel_w["label"].set(field_vel["label"]);
        }
    }

    n_mesh["fields"].remove_child("vel");
}

//-----------------------------------------------------------------------------
void
add_multi_buffer_full_matset(Node &n_mesh,
                             const int num_elements,
                             const std::string &topo_name)
{
    Node &n_matset = n_mesh["matsets"]["matset"];
    n_matset["topology"] = topo_name;
    n_matset["volume_fractions"]["mat_a"].set(DataType::float64(num_elements));
    n_matset["volume_fractions"]["mat_b"].set(DataType::float64(num_elements));

    double_array a_vfs = n_matset["volume_fractions"]["mat_a"].value();
    double_array b_vfs = n_matset["volume_fractions"]["mat_b"].value();

    for (int i = 0; i < num_elements; i ++)
    {
        a_vfs[i] = (i % 2 ? 1.0 : 0.0);
        b_vfs[i] = (i % 2 ? 0.0 : 1.0);
    }
}

//---------------------------------------------------------------------------//
// (mostly) copied from conduit_blueprint_mesh_examples.hpp
void braid_init_example_matset(index_t nele_x,
                               index_t nele_y,
                               index_t nele_z,
                               Node &res)
{
    index_t nele = nele_x * nele_y * ((nele_z > 0) ? nele_z : 1);

    res["topology"] = "mesh";

    Node &vfs = res["volume_fractions"];
    vfs["mat1"].set(DataType::float64(nele));
    vfs["mat2"].set(DataType::float64(nele));

    float64_array mat1_vals = vfs["mat1"].value();
    float64_array mat2_vals = vfs["mat2"].value();

    for(index_t k = 0, idx = 0; (idx == 0 || k < nele_z); k++)
    {
        for(index_t j = 0; (idx == 0 || j < nele_y) ; j++)
        {
            for(index_t i = 0; (idx == 0 || i < nele_x) ; i++, idx++)
            {
                float64 mv = (nele_x == 1) ? 0.5 : i / (nele_x - 1.0);

                mat1_vals[idx] = mv;
                mat2_vals[idx] = 1.0 - mv;
            }
        }
    }
}

#endif
