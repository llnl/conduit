#include "conduit_relay_io_cgns.hpp"

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#ifdef CONDUIT_RELAY_IO_CGNS_ENABLED
#include <pcgnslib.h>
#endif

namespace {

std::string meshType(const conduit::Node &node) {
  return node["/topologies/mesh/elements/shape"].as_string();
}

int physicalDimension(const conduit::Node &node) {
  return node["/coordsets/coords/values"].number_of_children();
}

int cellDimension(const conduit::Node &node) {
  const auto type = meshType(node);
  if (type == "tet") {
    return 3;
  } else if (type == "hex") {
    return 3;
  } else if (type == "tri") {
    return 2;
  } else if (type == "quad") {
    return 2;
  } else {
    CONDUIT_ERROR("cellDimension: unknown type " << type);
  }
  return 0;
}

std::int64_t nVerts(const conduit::Node &node) {
  return node["/coordsets/coords/values/x"].dtype().is_float64()
             ? node["/coordsets/coords/values/x"].as_float64_array()
                   .number_of_elements()
             : node["/coordsets/coords/values/x"].as_float32_array()
                   .number_of_elements();
}

std::int64_t nConn(const conduit::Node &node) {
  return node["/topologies/mesh/elements/connectivity"]
             .dtype()
             .is_int64()
         ? node["/topologies/mesh/elements/connectivity"]
               .as_int64_array()
               .number_of_elements()
         : node["/topologies/mesh/elements/connectivity"]
               .as_int32_array()
               .number_of_elements();
}

std::int64_t nCells(const conduit::Node &node) {
  const std::int64_t nconn = nConn(node);
  const auto type = meshType(node);
  if (type == "tet"){
    return nconn/4;
  } else if (type == "hex"){
    return nconn/8;
  } else if (type == "tri"){
    return nconn/3;
  } else if (type == "quad"){
    return nconn/4;
  } else {
    CONDUIT_ERROR("ncells: unknown type " << type);
  }
  return 0;
}

} // namespace

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit {

//-----------------------------------------------------------------------------
// -- begin conduit::relay --
//-----------------------------------------------------------------------------
namespace relay {

//-----------------------------------------------------------------------------
// -- begin conduit::relay::io --
//-----------------------------------------------------------------------------
namespace io {

//-----------------------------------------------------------------------------
// -- begin conduit::relay::io::cgns --
//-----------------------------------------------------------------------------
namespace cgns {

void save_mesh(const conduit::Node &mesh, const std::string &path) {

  CONDUIT_INFO("conduit::relay::io::cgns__save_mesh(const Node &node, const "
               "std::string &path)\n");
  if (mesh["/coordsets/coords/type"].as_string() != "explicit") {
    CONDUIT_ERROR(
        "cgns::save_mesh only supports reading 'explicit' coordinate sets "
        "at this time.");
  }

  mesh.print();

  // for multi-file, maybe ".root.cgns"
  const std::string filename = path + ".cgns";

  int file_index;
  if (cg_open(filename.c_str(), CG_MODE_WRITE, &file_index)) {
    cg_error_exit();
  }
  const int cell_dim = cellDimension(mesh);
  const int phys_dim = physicalDimension(mesh);

  int base_index;
  if (cg_base_write(file_index, "Base", cell_dim, phys_dim, &base_index)) {
    cg_error_exit();
  }


  cgsize_t isize[3][1];
  isize[0][0]           = nVerts(mesh);
  isize[1][0]           = nCells(mesh);
  isize[2][0]           = 0;
  int zone_index;
  if (cg_zone_write(file_index, base_index, "Zone", isize[0], CG_Unstructured,
                    &zone_index)) {
    cg_error_exit();
  }


  auto write_coord = [&](const std::string &coord_path,
                         const std::string &coord_name) {
    int coord_index;
    if (mesh[coord_path].dtype().is_float64()) {
      if (cg_coord_write(file_index, base_index, zone_index, CG_RealDouble,
                         coord_name.c_str(),
                         mesh[coord_path].as_float64_array().data_ptr(),
                         &coord_index)) {
        cg_error_exit();
      }
    } else if (mesh[coord_path].dtype().is_float32()) {
      if (cg_coord_write(file_index, base_index, zone_index, CG_RealSingle,
                         coord_name.c_str(),
                         mesh[coord_path].as_float32_array().data_ptr(),
                         &coord_index)) {
        cg_error_exit();
      }
    } else {
      CONDUIT_ERROR("CGNSHandle coordinate only supports float32 and float64.");
    }
  };

  write_coord("/coordsets/coords/values/x", "CoordinateX");
  if (phys_dim > 1){
    write_coord("/coordsets/coords/values/y", "CoordinateY");
  }
  if (phys_dim > 2){
    write_coord("/coordsets/coords/values/z", "CoordinateZ");
  }

  if (cg_close(file_index)) {
    cg_error_exit();
  }
}

void load_mesh(const std::string &root_file_path, conduit::Node &mesh) {}

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
