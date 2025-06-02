#!/bin/bash

##############################################################################
# Demonstrates how to manually build Conduit and its dependencies, including:
#
#  zlib, hdf5
#
# usage example:
#   env enable_mpi=ON enable_openmp=ON ./build_conduit.sh
#
#
# Assumes: 
#  - cmake is in your path
#  - selected compilers are in your path or set via env vars
#  - [when enabled] MPI and Python (+numpy and mpi4py), are in your path
#
##############################################################################
set -eu -o pipefail

##############################################################################
# Build Options
##############################################################################

# shared options
enable_fortran="${enable_fortran:=OFF}"
enable_python="${enable_python:=OFF}"
enable_openmp="${enable_openmp:=OFF}"
enable_mpi="${enable_mpi:=OFF}"
enable_find_mpi="${enable_find_mpi:=ON}"
enable_tests="${enable_tests:=OFF}"
enable_verbose="${enable_verbose:=ON}"
build_jobs="${build_jobs:=6}"
build_config="${build_config:=Release}"
build_shared_libs="${build_shared_libs:=ON}"

# tpl controls
build_zlib="${build_zlib:=true}"
build_hdf5="${build_hdf5:=true}"
build_cgns="${build_cgns:=true}"

# conduit options
build_conduit="${build_conduit:=true}"

# see if we are building on windows
build_windows="${build_windows:=OFF}"

# see if we are building on macOS
build_macos="${build_macos:=OFF}"

case "$OSTYPE" in
  win*)     build_windows="ON";;
  msys*)    build_windows="ON";;
  darwin*)  build_macos="ON";;
  *)        ;;
esac

if [[ "$build_windows" == "ON" ]]; then
  echo "*** configuring for windows"
fi

if [[ "$build_macos" == "ON" ]]; then
  echo "*** configuring for macos"
fi

################
# path helpers
################
function ospath()
{
  if [[ "$build_windows" == "ON" ]]; then
    echo `cygpath -m $1`
  else
    echo $1
  fi 
}

function abs_path()
{
  if [[ "$build_macos" == "ON" ]]; then
    echo "$(cd $(dirname "$1");pwd)/$(basename "$1")"
  else
    echo `realpath $1`
  fi
}

root_dir=$(pwd)
root_dir="${prefix:=${root_dir}}"
root_dir=$(ospath ${root_dir})
root_dir=$(abs_path ${root_dir})
script_dir=$(abs_path "$(dirname "${BASH_SOURCE[0]}")")

# root_dir is where we will build and install
# override with `prefix` env var
if [ ! -d ${root_dir} ]; then
  mkdir -p ${root_dir}
fi

cd ${root_dir}

# install_dir is where we will install
# override with `prefix` env var
install_dir="${install_dir:=$root_dir/install}"

echo "*** prefix:       ${root_dir}" 
echo "*** build root:   ${root_dir}/build"
echo "*** install root: ${install_dir}"
echo "*** script dir:   ${script_dir}"

################
# CMake Compiler Settings
################
cmake_compiler_settings=""

# capture compilers if they are provided via env vars
if [ ! -z ${CC+x} ]; then
  cmake_compiler_settings="-DCMAKE_C_COMPILER:PATH=${CC}"
fi

if [ ! -z ${CXX+x} ]; then
  cmake_compiler_settings="${cmake_compiler_settings} -DCMAKE_CXX_COMPILER:PATH=${CXX}"
fi

if [ ! -z ${FTN+x} ]; then
  cmake_compiler_settings="${cmake_compiler_settings} -DCMAKE_Fortran_COMPILER:PATH=${FTN}"
fi

################
# print all build_ZZZ and enable_ZZZ options
################
echo "*** cmake_compiler_settings: ${cmake_compiler_settings}"
echo "*** build_conduit `enable` settings:"
set | grep enable_
echo "*** build_conduit `build` settings:"
set | grep build_

################
# Zlib
################
zlib_version=1.3.1
zlib_src_dir=$(ospath ${root_dir}/zlib-${zlib_version})
zlib_build_dir=$(ospath ${root_dir}/build/zlib-${zlib_version}/)
zlib_install_dir=$(ospath ${install_dir}/zlib-${zlib_version}/)
zlib_tarball=zlib-${zlib_version}.tar.gz

# build only if install doesn't exist
if [ ! -d ${zlib_install_dir} ]; then
if ${build_zlib}; then
if [ ! -d ${zlib_src_dir} ]; then
  echo "**** Downloading ${zlib_tarball}"
  curl -L https://github.com/madler/zlib/releases/download/v${zlib_version}/zlib-${zlib_version}.tar.gz -o ${zlib_tarball}
  tar -xzf ${zlib_tarball}
fi

echo "**** Configuring Zlib ${zlib_version}"
cmake -S ${zlib_src_dir} -B ${zlib_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DCMAKE_INSTALL_PREFIX=${zlib_install_dir}

echo "**** Building Zlib ${zlib_version}"
cmake --build ${zlib_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing Zlib ${zlib_version}"
cmake --install ${zlib_build_dir} --config ${build_config}

fi
else
  echo "**** Skipping Zlib build, install found at: ${zlib_install_dir}"
fi # build_zlib


################
# HDF5
################
# release 1-2 GAH!
hdf5_version=1.14.1-2
hdf5_middle_version=1.14.1
hdf5_short_version=1.14
hdf5_src_dir=$(ospath ${root_dir}/hdf5-${hdf5_version})
hdf5_build_dir=$(ospath ${root_dir}/build/hdf5-${hdf5_version}/)
hdf5_install_dir=$(ospath ${install_dir}/hdf5-${hdf5_version}/)
hdf5_tarball=hdf5-${hdf5_version}.tar.gz

# build only if install doesn't exist
if [ ! -d ${hdf5_install_dir} ]; then
if ${build_hdf5}; then
if [ ! -d ${hdf5_src_dir} ]; then
  echo "**** Downloading ${hdf5_tarball}"
  curl -L https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-${hdf5_short_version}/hdf5-${hdf5_middle_version}/src/hdf5-${hdf5_version}.tar.gz -o ${hdf5_tarball}
  tar -xzf ${hdf5_tarball}
fi

#################
#
# hdf5 1.14.x CMake recipe for using zlib
#
# -DHDF5_ENABLE_Z_LIB_SUPPORT=ON
# Add zlib install dir to CMAKE_PREFIX_PATH
#
#################

echo "**** Configuring HDF5 ${hdf5_version}"
hdf_parallel_settings=""
# check if mpi is enabled
if [[ "${enable_mpi}" == "ON" ]]; then
  echo "**** Enabling MPI support for HDF5"
  hdf_parallel_settings=" -DHDF5_ENABLE_PARALLEL=ON"
else
  echo "**** Disabling MPI support for HDF5"
  hdf_parallel_settings=" -DHDF5_ENABLE_PARALLEL=OFF"
fi

cmake -S ${hdf5_src_dir} -B ${hdf5_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DHDF5_ENABLE_Z_LIB_SUPPORT=ON \
  -DCMAKE_PREFIX_PATH=${zlib_install_dir} \
  -DCMAKE_INSTALL_PREFIX=${hdf5_install_dir} \
  ${hdf_parallel_settings}

echo "**** Building HDF5 ${hdf5_version}"
cmake --build ${hdf5_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing HDF5 ${hdf5_version}"
cmake --install ${hdf5_build_dir} --config ${build_config}

fi
else
  echo "**** Skipping HDF5 build, install found at: ${hdf5_install_dir}"
fi # build_hdf5


################
# CGNS
################

cgns_version=4.4.0
cgns_tarball=cgns-${cgns_version}.tar.gz
cgns_install_dir=$(ospath ${install_dir}/cgns-${cgns_version}/)
cgns_src_dir=$(ospath ${root_dir}/CGNS-${cgns_version})
cgns_build_dir=$(ospath ${root_dir}/build/cgns-${cgns_version}/)

# build only if install doesn't exist
if [ ! -d ${cgns_install_dir} ]; then
if ${build_cgns}; then
if [ ! -d ${cgns_src_dir} ]; then
  echo "**** Downloading ${cgns_tarball}"
  curl -L https://github.com/CGNS/CGNS/archive/refs/tags/v${cgns_version}.tar.gz -o ${cgns_tarball}
  tar -xzf ${cgns_tarball}
fi


cgns_parallel_settings=""
# check if mpi is enabled
if [[ "${enable_mpi}" == "ON" ]]; then
  echo "**** Enabling MPI support for CGNS"
  cgns_parallel_settings=" -DCGNS_ENABLE_PARALLEL=ON -DHDF5_NEED_MPI=ON"
else
  echo "**** Disabling MPI support for CGNS"
  cgns_parallel_settings=" -DCGNS_ENABLE_PARALLEL=OFF -DHDF5_NEED_MPI=OFF"
fi


cmake -S ${cgns_src_dir} -B ${cgns_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DCGNS_BUILD_SHARED=ON \
  -DCMAKE_PREFIX_PATH=${hdf5_install_dir} \
  -DCMAKE_INSTALL_PREFIX=${cgns_install_dir} \
  -DCGNS_ENABLE_SCOPING=ON \
  -DCGNS_ENABLE_64BIT=ON \
  ${cgns_parallel_settings}

echo "**** Building CGNS ${cgns_version}"
cmake --build ${cgns_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing CGNS ${cgns_version}"
cmake --install ${cgns_build_dir} --config ${build_config}

fi # build_cgns
fi # if cgns install exists, skip build


################
# Conduit
################
conduit_version=develop
conduit_src_dir=$(ospath ${root_dir}/conduit/src)
conduit_build_dir=$(ospath ${root_dir}/build/conduit-${conduit_version}/)
conduit_install_dir=$(ospath ${install_dir}/conduit-${conduit_version}/)

echo "**** Creating Conduit host-config (conduit-config.cmake)"
#
echo '# host-config file generated by build_conduit.sh' > ${root_dir}/conduit-config.cmake

# capture compilers if they are provided via env vars
if [ ! -z ${CC+x} ]; then
    echo 'set(CMAKE_C_COMPILER ' ${CC} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
fi

if [ ! -z ${CXX+x} ]; then
    echo 'set(CMAKE_CXX_COMPILER ' ${CXX} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
fi

if [ ! -z ${FTN+x} ]; then
    echo 'set(CMAKE_Fortran_COMPILER ' ${FTN} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
fi

# capture compiler flags  if they are provided via env vars
if [ ! -z ${CFLAGS+x} ]; then
    echo 'set(CMAKE_C_FLAGS "' ${CFLAGS} '" CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
fi

if [ ! -z ${CXXFLAGS+x} ]; then
    echo 'set(CMAKE_CXX_FLAGS "' ${CXXFLAGS} '" CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
fi

if [ ! -z ${FFLAGS+x} ]; then
    echo 'set(CMAKE_F_FLAGS "' ${FFLAGS} '" CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
fi

echo 'set(CMAKE_VERBOSE_MAKEFILE ' ${enable_verbose} ' CACHE BOOL "")' >> ${root_dir}/conduit-config.cmake
echo 'set(CMAKE_BUILD_TYPE ' ${build_config} ' CACHE STRING "")' >> ${root_dir}/conduit-config.cmake
echo 'set(BUILD_SHARED_LIBS ' ${build_shared_libs} ' CACHE STRING "")' >> ${root_dir}/conduit-config.cmake
echo 'set(CMAKE_INSTALL_PREFIX ' ${conduit_install_dir} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
echo 'set(ENABLE_TESTS ' ${enable_tests} ' CACHE BOOL "")' >> ${root_dir}/conduit-config.cmake
echo 'set(ENABLE_MPI ' ${enable_mpi} ' CACHE BOOL "")' >> ${root_dir}/conduit-config.cmake
echo 'set(ENABLE_FIND_MPI ' ${enable_find_mpi} ' CACHE BOOL "")' >> ${root_dir}/conduit-config.cmake
echo 'set(ENABLE_FORTRAN ' ${enable_fortran} ' CACHE BOOL "")' >> ${root_dir}/conduit-config.cmake
echo 'set(ENABLE_PYTHON ' ${enable_python} ' CACHE BOOL "")' >> ${root_dir}/conduit-config.cmake
echo 'set(CONDUIT_DIR ' ${conduit_install_dir} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
echo 'set(HDF5_DIR ' ${hdf5_install_dir} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
echo 'set(CGNS_DIR ' ${cgns_install_dir} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake
echo 'set(ZLIB_DIR ' ${zlib_install_dir} ' CACHE PATH "")' >> ${root_dir}/conduit-config.cmake

# build only if install doesn't exist
if [ ! -d ${conduit_install_dir} ]; then
if ${build_conduit}; then
if [ ! -d ${conduit_src_dir} ]; then
    echo "**** Cloning Conduit"
    git clone --recursive https://github.com/LLNL/conduit.git
fi

echo "**** Configuring Conduit"
cmake -S ${conduit_src_dir} -B ${conduit_build_dir} -C ${root_dir}/conduit-config.cmake

echo "**** Building Conduit"
cmake --build ${conduit_build_dir} --config ${build_config} -j${build_jobs}

echo "**** Installing Conduit"
cmake --install ${conduit_build_dir}  --config ${build_config}

fi
else
  echo "**** Skipping Conduit build, install found at: ${conduit_install_dir}"
fi # build_conduit
