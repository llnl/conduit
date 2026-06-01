# Conduit repo notes

## Repo layout
- Source lives under `src/` (top-level `CMakeLists.txt` expects `-S src`).
- Libraries are implemented in `src/libs`.
- Tests are implemented in `src/tests`.
- Documentation is written using sphinx and is located at `src/docs/sphinx`.
- Conduit is developed on github and the repo location is https://github.com/llnl/conduit.


## Third-party Dependencies
- You can use `scripts/build_conduit/build_conduit.sh` to build the third party libraries that Conduit requires

## Build (CMake)
- Prefer out-of-source builds:
  - Configure: `cmake -S src -B build-debug -C host-config.cmake -DCMAKE_BUILD_TYPE=Debug`
  - Build: `cmake --build build-debug -j`
  - Test (ctest): `ctest --test-dir build-debug --output-on-failure`
  - You can run Conduit's python module from a build using `bin/run_python_with_conduit_build.sh`


## Tests
- To run a single test: `ctest --test-dir build-debug -R '^t_conduit_node$' --output-on-failure -V`
- Keep new tests small and deterministic; add them to the closest existing `src/tests/**/CMakeLists.txt`.


## Style / scope
- Prefer minimal, surgical changes; don’t reformat unrelated code.
- Avoid adding new dependencies or touching vendored third-party code unless required by the task.


## Conduit
- The conduit library/module provides the core Conduit API.
- Conduit provides C++, Python, C, and Fortran APIs.
- The main data structure in Conduit API is Node object, it is tree based data structure similar to a json object.
- Leaf nodes hold 1D arrays described using bitwidth-style types or c-style strings.
- Leaf data can be copied into the tree using set or the assignment operator.
- Leaf data can be described zero-copy using set_external.
- Conduit Nodes can be converted to and loaded from yaml or json strings using `to_yaml`, `to_json`, and `parse`.


## Blueprint
- The blueprint library/module outlines schemas and convention for sharing data using Conduit Nodes.
- Conduit provides conventions for sharing simulation mesh data that are called the Mesh Blueprint.
- You can use `conduit.blueprint.mesh.verify` to check if a Conduit Node confirms to these conventions.
- You can generate many example meshes using `conduit.blueprint.mesh.examples.generate`, you can review the mesh types and their options using `generate_default_options`.
- You can save meshes to hdf5 or yaml using `conduit.relay.io.blueprint.save_mesh`.
- You can load meshes from hdf5 or yaml using `conduit.relay.io.blueprint.load_mesh`.


## Relay
- The relay library/module provides I/O methods for using Conduit Nodes with hdf5, silo, and mpi.
