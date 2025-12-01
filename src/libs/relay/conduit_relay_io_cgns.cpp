#include "conduit_relay_io_cgns.hpp"

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#ifdef CONDUIT_RELAY_IO_CGNS_ENABLED
#include <pcgnslib.h>
#endif

namespace
{

std::string meshType(const conduit::Node& node)
{
  return node["/topologies/mesh/elements/shape"].as_string();
}

int physicalDimension(const conduit::Node& node)
{
  return node["/coordsets/coords/values"].number_of_children();
}

int cellDimension(const conduit::Node& node)
{
  const auto type = meshType(node);
  if (type == "tet")
  {
    return 3;
  }
  else if (type == "hex")
  {
    return 3;
  }
  else if (type == "tri")
  {
    return 2;
  }
  else if (type == "quad")
  {
    return 2;
  }
  else if (type == "mixed")
  {
    return 3; // todo this could be 2d
  }
  else
  {
    CONDUIT_ERROR("cellDimension: unknown type " << type);
  }
  return 0;
}

std::int64_t nVerts(const conduit::Node& node)
{
  return node["/coordsets/coords/values/x"].dtype().is_float64()
    ? node["/coordsets/coords/values/x"].as_float64_array().number_of_elements()
    : node["/coordsets/coords/values/x"].as_float32_array().number_of_elements();
}

std::int64_t nConn(const conduit::Node& node)
{
  return node["/topologies/mesh/elements/connectivity"].dtype().is_int64()
    ? node["/topologies/mesh/elements/connectivity"].as_int64_array().number_of_elements()
    : node["/topologies/mesh/elements/connectivity"].as_int32_array().number_of_elements();
}

std::int64_t nCells(const conduit::Node& node)
{
  const std::int64_t nconn = nConn(node);
  const auto type = meshType(node);
  if (type == "tet")
  {
    return nconn / 4;
  }
  else if (type == "hex")
  {
    return nconn / 8;
  }
  else if (type == "tri")
  {
    return nconn / 3;
  }
  else if (type == "quad")
  {
    return nconn / 4;
  }
  else if (type == "mixed")
  {
    std::cout << "need to work on ncells for mixed" << std::endl;
  }
  else
  {
    CONDUIT_ERROR("ncells: unknown type " << type);
  }
  return 0;
}

} // namespace

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay --
//-----------------------------------------------------------------------------
namespace relay
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::io --
//-----------------------------------------------------------------------------
namespace io
{


void cgns_write(const conduit::Node& mesh, const std::string& path)
{
  std::cout << "path: " << path << std::endl;
  std::cout << "mesh: " << std::endl;
  mesh.print();

  // std::cout << mesh.has_path("/[0]/coordsets") << " has coordsets" << std::endl;
  // for (const auto& child : mesh.child_names()){
  //   std::cout << "child: " << child << std::endl;
  // }

  // std::cout << mesh.has_path(mesh.children()[0]) << "mesh.has_path(mesh.children()[0]) " << std::endl;


  const Node& mesh_node = mesh.child(0);


  int domain_id = 0;
  if (mesh_node.has_path("state/domain_id"))
  {
    domain_id = mesh_node["state/domain_id"].as_int32();
  }
  std::ostringstream domain_id_ss;
  domain_id_ss << std::setw(6) << std::setfill('0') << domain_id;
  const std::string full_path = path + "." + domain_id_ss.str() + ".cgns";

  const Node& mesh_topo = mesh_node["topologies/mesh"];
  std::cout << "mesh_topo: " << std::endl;

  mesh_topo.print();
  if (mesh_topo["type"].as_string() != "unstructured")
  {
    CONDUIT_ERROR("cgns::save_mesh only supports saving 'unstructured' type");
  }
  if (mesh_topo["elements/shape"].as_string() != "mixed")
  {
    CONDUIT_ERROR("cgns::save_mesh only supports saving 'mixed' shape");
  }
  std::cout << "elements/shapes.print()" << std::endl;
  // mesh_topo["elements/shapes"].print();
  const auto& mesh_shapes = mesh_topo["elements/shapes"];
  if (mesh_shapes.dtype().is_empty())
  {
    CONDUIT_ERROR("cgns::save_mesh requires mesh_topo['elements/shapes'] to be non-empty");
  }
  std::map<int, std::string> shape_map;
  const auto& mesh_shape_map = mesh_topo["elements/shape_map"];
  // std::cout << "iterating through shape map children" << std::endl;
  for (const auto& child : mesh_shape_map.children())
  {
    shape_map[child.as_int32()] = child.name();
  }
  std::cout << "shape_map: " << std::endl;
  for (const auto& pair : shape_map)
  {
    std::cout << "shape_map[" << pair.first << "] = " << pair.second << std::endl;
  }

  const std::map<std::string, int> shape_counts = [&]()
  {
    std::map<std::string, int> counts;
    const auto shapes = mesh_shapes.as_int32_array();
    for (int64_t i = 0; i < shapes.number_of_elements(); ++i)
    {
      counts[shape_map[shapes[i]]]++;
    }
    return counts;
  }();

  std::cout << "shape_counts: " << std::endl;
  for (const auto& pair : shape_counts)
  {
    std::cout << "shape_counts[" << pair.first << "] = " << pair.second << std::endl;
  }

  struct CGNSElementSection
  {
    std::string section_name;
    CG_ElementType_t element_type;
    std::vector<cgsize_t> connectivity;
    cgsize_t n_cells;
  };

  const std::map<std::string, CGNSElementSection> element_sections =
    [](const conduit::Node& elem_node)
  {
    std::map<int, std::string> shape_map;
    const auto& mesh_shape_map = elem_node["shape_map"];
    // std::cout << "iterating through shape map children" << std::endl;
    for (const auto& child : mesh_shape_map.children())
    {
      shape_map[child.as_int32()] = child.name();
    }

    std::map<std::string, CGNSElementSection> sections;
    const auto offsets = elem_node["offsets"].as_int32_array();
    const auto connectivity = elem_node["connectivity"].as_int32_array();
    const auto shapes = elem_node["shapes"].as_int32_array();
    const auto sizes = elem_node["sizes"].as_int32_array();

    for (int64_t i = 0; i < shapes.number_of_elements(); ++i)
    {
      const int shape_id = shapes[i];
      const std::string shape_name = shape_map[shape_id];
      // CGNSElementSection section;
      const int nconn = sizes[i];
      auto& conn = sections[shape_name].connectivity;
      for (int64_t j = offsets[i]; j < offsets[i] + nconn; ++j)
      {
        conn.push_back(connectivity[j] + 1); // CGNS is 1-based indexing
      }
      // sections.at(shape_name).
      // section.section_name = shape_name;
    }

    for (auto& section : sections)
    {
      const auto& cell_type = section.first;
      if (cell_type == "hex")
      {
        section.second.element_type = CG_HEXA_8;
        section.second.n_cells = section.second.connectivity.size() / 8;
        section.second.section_name = "Elem_HEXA_8";
      }
      else if (cell_type == "tet")
      {
        section.second.element_type = CG_TETRA_4;
        section.second.n_cells = section.second.connectivity.size() / 4;
        section.second.section_name = "Elem_TETRA_4";
      }
      else if (cell_type == "tri")
      {
        section.second.element_type = CG_TRI_3;
        section.second.n_cells = section.second.connectivity.size() / 3;
        section.second.section_name = "Elem_TRI_3";
      }
      else if (cell_type == "quad")
      {
        section.second.element_type = CG_QUAD_4;
        section.second.n_cells = section.second.connectivity.size() / 4;
        section.second.section_name = "Elem_QUAD_4";
      }
      else if (cell_type == "wedge")
      {
        section.second.element_type = CG_PENTA_6;
        section.second.n_cells = section.second.connectivity.size() / 6;
        section.second.section_name = "Elem_PENTA_6";
      }
      else if (cell_type == "pyramid")
      {
        section.second.element_type = CG_PYRA_5;
        section.second.n_cells = section.second.connectivity.size() / 5;
        section.second.section_name = "Elem_PYRA_5";
      }
      else
      {
        CONDUIT_ERROR("cgns::save_mesh: unknown cell type: " << cell_type);
      }
      // std::cout << "section: " << section.section_name << std::endl;
    }



    return sections;
  }(mesh_topo["elements"]);

  int file_index;
  std::cout << "opening file: " << full_path << std::endl;
  if (cg_open(full_path.c_str(), CG_MODE_WRITE, &file_index))
  {
    cg_error_exit();
  }
  // const int cell_dim = cellDimension(mesh_node);

  const int cell_dim = [&]()
  {
    // if shape_counts contains the key "tet", "hex", "pyramid", or "wedge", return 3;
    if (shape_counts.count("tet") || shape_counts.count("hex") || shape_counts.count("pyramid") ||
        shape_counts.count("wedge"))
    {
      return 3;
    }
    // if shape_counts contains the key "tri", "quad", return 2;
    if (shape_counts.count("tri") || shape_counts.count("quad"))
    {
      return 2;
    }
    CONDUIT_ERROR("cgns::save_mesh: unsupported cell dimension for mesh");
    return 0;
  }();

  const int phys_dim = physicalDimension(mesh_node);

  int base_index;
  std::cout << "writing base" << std::endl;
  if (cg_base_write(file_index, "Base", cell_dim, phys_dim, &base_index))
  {
    cg_error_exit();
  }
  std::cout << "base written" << std::endl;

  mesh_topo["elements/connectivity"].print();

  const int64_t n_total_cells = [&]()
  {
    int64_t ncells = 0;
    for (const auto& kv : shape_counts)
    {
      ncells += kv.second;
    }
    return ncells;
  }();

  cgsize_t isize[3][1];
  isize[0][0] = nVerts(mesh_node);
  // isize[1][0]           = nCells(mesh_node);
  isize[1][0] = n_total_cells;
  isize[2][0] = 0;
  int zone_index;
  std::cout << "writing zone" << std::endl;
  if (cg_zone_write(file_index, base_index, "Zone_Domain", isize[0], CG_Unstructured, &zone_index))
  {
    cg_error_exit();
  }
  std::cout << "zone written" << std::endl;

  auto write_coord = [&](const conduit::Node& coords, const std::string& coord_name)
  {
    // todo need to be 1 based
    int coord_index;
    if (coords.dtype().is_float64())
    {
      if (cg_coord_write(file_index,
                         base_index,
                         zone_index,
                         CG_RealDouble,
                         coord_name.c_str(),
                         coords.as_float64_array().data_ptr(),
                         &coord_index))
      {
        cg_error_exit();
      }
    }
    else if (coords.dtype().is_float32())
    {
      if (cg_coord_write(file_index,
                         base_index,
                         zone_index,
                         CG_RealSingle,
                         coord_name.c_str(),
                         coords.as_float32_array().data_ptr(),
                         &coord_index))
      {
        cg_error_exit();
      }
    }
    else
    {
      CONDUIT_ERROR("CGNS coordinate only supports float32 and float64.");
    }
  };


  std::cout << "writing coords" << std::endl;
  write_coord(mesh_node["/coordsets/coords/values/x"], "CoordinateX");
  if (phys_dim > 1)
  {
    write_coord(mesh_node["/coordsets/coords/values/y"], "CoordinateY");
  }
  if (phys_dim > 2)
  {
    write_coord(mesh_node["/coordsets/coords/values/z"], "CoordinateZ");
  }
  std::cout << "coords written" << std::endl;


  std::cout << "writing element sections" << std::endl;

  cgsize_t element_start_offset = 1;
  for (const auto& kv : element_sections)
  {
    std::cout << kv.first << ": " << kv.second.section_name << std::endl;
    std::cout << "  n_cells: " << kv.second.n_cells << std::endl;

    int section_index;
    if (cg_section_write(file_index,
                         base_index,
                         zone_index,
                         kv.second.section_name.c_str(),
                         kv.second.element_type,
                         element_start_offset,
                         element_start_offset + kv.second.n_cells - 1,
                         0,
                         kv.second.connectivity.data(),
                         &section_index))
    {
      cg_error_exit();
    }
    element_start_offset += kv.second.n_cells;
  }


  // mesh_node["/fields"]


  if (mesh_node.has_path("/fields"))
  {
    const bool has_vertex_fields = [&]()
    {
      for (const auto& field : mesh_node["/fields"].children())
      {
        if (field["association"].as_string() == "vertex")
        {
          return true;
        }
      }
      return false;
    }();

    const bool has_cell_fields = [&]()
    {
      for (const auto& field : mesh_node["/fields"].children())
      {
        if (field["association"].as_string() == "element")
        {
          return true;
        }
      }
      return false;
    }();

    int centroid_solution;
    int vertex_solution;
    if (has_cell_fields)
    {
      if (cg_sol_write(file_index, base_index, zone_index, "CellCenterSolution", CG_CellCenter, &centroid_solution))
      {
        cg_error_exit();
      }
    }
    if (has_vertex_fields)
    {
      if (cg_sol_write(file_index, base_index, zone_index, "VertexSolution", CG_Vertex, &vertex_solution))
      {
        cg_error_exit();
      }
    }

    for (const auto& field : mesh_node["/fields"].children())
    {
      const std::string field_name = field.name();
      std::cout << "writing field: " << field_name << std::endl;
      field.print();

      const CG_DataType_t datatype =
        field["values"].dtype().is_float64() ? CG_RealDouble : CG_RealSingle;
      const int isol = 
        field["association"].as_string() == "vertex" ? vertex_solution : centroid_solution;

      int ifield;
      cg_field_write(file_index,
                         base_index,
                         zone_index,
                         isol,
                         datatype,
                         field_name.c_str(),
                         field["values"].data_ptr(),
                         &ifield);
    }
  }

  if (cg_close(file_index))
  {
    cg_error_exit();
  }
}

//-----------------------------------------------------------------------------
// -- begin conduit::relay::io::cgns --
//-----------------------------------------------------------------------------
namespace cgns
{


void write_mesh(const conduit::Node& mesh, const std::string& path, const conduit::Node&)
{
  save_mesh(mesh, path);
}

void save_mesh(const conduit::Node& mesh, const std::string& path)
{

  CONDUIT_INFO("conduit::relay::io::cgns__save_mesh(const Node &node, const "
               "std::string &path)\n");
  if (mesh["/coordsets/coords/type"].as_string() != "explicit")
  {
    CONDUIT_ERROR("cgns::save_mesh requires explicit coordsets type");
  }

  mesh.print();

  // for multi-file, maybe ".root.cgns"
  const std::string filename = path + ".cgns";

  int file_index;
  if (cg_open(filename.c_str(), CG_MODE_WRITE, &file_index))
  {
    cg_error_exit();
  }
  const int cell_dim = cellDimension(mesh);
  const int phys_dim = physicalDimension(mesh);

  int base_index;
  if (cg_base_write(file_index, "Base", cell_dim, phys_dim, &base_index))
  {
    cg_error_exit();
  }


  cgsize_t isize[3][1];
  isize[0][0] = nVerts(mesh);
  isize[1][0] = nCells(mesh);
  isize[2][0] = 0;
  int zone_index;
  if (cg_zone_write(file_index, base_index, "Zone", isize[0], CG_Unstructured, &zone_index))
  {
    cg_error_exit();
  }


  auto write_coord = [&](const std::string& coord_path, const std::string& coord_name)
  {
    int coord_index;
    if (mesh[coord_path].dtype().is_float64())
    {
      if (cg_coord_write(file_index,
                         base_index,
                         zone_index,
                         CG_RealDouble,
                         coord_name.c_str(),
                         mesh[coord_path].as_float64_array().data_ptr(),
                         &coord_index))
      {
        cg_error_exit();
      }
    }
    else if (mesh[coord_path].dtype().is_float32())
    {
      if (cg_coord_write(file_index,
                         base_index,
                         zone_index,
                         CG_RealSingle,
                         coord_name.c_str(),
                         mesh[coord_path].as_float32_array().data_ptr(),
                         &coord_index))
      {
        cg_error_exit();
      }
    }
    else
    {
      CONDUIT_ERROR("CGNSHandle coordinate only supports float32 and float64.");
    }
  };

  write_coord("/coordsets/coords/values/x", "CoordinateX");
  if (phys_dim > 1)
  {
    write_coord("/coordsets/coords/values/y", "CoordinateY");
  }
  if (phys_dim > 2)
  {
    write_coord("/coordsets/coords/values/z", "CoordinateZ");
  }

  if (cg_close(file_index))
  {
    cg_error_exit();
  }
}

void load_mesh(const std::string& root_file_path, conduit::Node& mesh) {}

} // namespace cgns
//-----------------------------------------------------------------------------
// -- end conduit::relay::io::cgns --
//-----------------------------------------------------------------------------

} // namespace io
//-----------------------------------------------------------------------------
// -- end conduit::relay::io --
//-----------------------------------------------------------------------------

} // namespace relay
// namespace relay
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------

} // namespace conduit
// namespace conduit
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#ifdef CONDUIT_RELAY_IO_MPI_ENABLED
namespace conduit {
namespace relay {
namespace mpi {
namespace io {
namespace cgns {

void write_mesh(const conduit::Node& mesh, const std::string& path, const conduit::Node& opts, MPI_Comm comm)
{
  std::cout << "reached conduit::relay::mpi::io::cgns::write_mesh" << std::endl;abort();
  // save_mesh(mesh, path);
}

}
}
}
}
}
#endif