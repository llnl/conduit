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
enable_cuda="${enable_cuda:=OFF}"
enable_hip="${enable_hip:=OFF}"
# TODO: sycl
enable_fortran="${enable_fortran:=OFF}"
enable_python="${enable_python:=OFF}"
enable_openmp="${enable_openmp:=OFF}"
enable_mpi="${enable_mpi:=OFF}"
enable_mpicc="${enable_mpicc:=OFF}"
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
build_pyvenv="${build_pyvenv:=false}"
build_caliper="${build_caliper:=false}"
build_camp="${build_camp:=true}"
build_raja="${build_raja:=true}"
build_umpire="${build_umpire:=true}"
build_silo="${build_silo:=true}"
build_zfp="${build_zfp:=true}"

# conduit options
build_conduit="${build_conduit:=true}"

# see if we are building on windows
build_windows="${build_windows:=OFF}"

# see if we are building on macOS
build_macos="${build_macos:=OFF}"

if [[ "$enable_cuda" == "ON" ]]; then
    echo "*** configuring with CUDA support"

    CC="${CC:=gcc}"
    CXX="${CXX:=g++}"
    FTN="${FTN:=gfortran}"

    CUDA_ARCH="${CUDA_ARCH:=80}"
fi

if [[ "$enable_hip" == "ON" ]]; then
    echo "*** configuring with HIP support"

    CC="${CC:=/opt/rocm/llvm/bin/amdclang}"
    CXX="${CXX:=/opt/rocm/llvm/bin/amdclang++}"
    # FTN?
    ROCM_ARCH="${ROCM_ARCH:=gfx90a}"
    ROCM_PATH="${ROCM_PATH:=/opt/rocm/}"
fi

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
build_dir=$(ospath ${root_dir}/build)
source_dir=$(ospath ${root_dir}/source)

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
echo "*** build root:   ${build_dir}"
echo "*** sources root: ${source_dir}"
echo "*** install root: ${install_dir}"
echo "*** script dir:   ${script_dir}"

################
# tar options
################
tar_extra_args=""
if [[ "$build_windows" == "ON" ]]; then
  tar_extra_args="--force-local"
fi

# make sure sources dir exists
if [ ! -d ${source_dir} ]; then
  mkdir -p ${source_dir}
fi
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

############################
# mpi related vars
############################
mpicc_exe="${mpicc_exe:=mpicc}"
mpicxx_exe="${mpicxx_exe:=mpic++}"

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
zlib_src_dir=$(ospath ${source_dir}/zlib-${zlib_version})
zlib_build_dir=$(ospath ${build_dir}/zlib-${zlib_version}/)
zlib_install_dir=$(ospath ${install_dir}/zlib-${zlib_version}/)
zlib_tarball=$(ospath ${source_dir}/zlib-${zlib_version}.tar.gz)

# build only if install doesn't exist
if [ ! -d ${zlib_install_dir} ]; then
if ${build_zlib}; then
if [ ! -f ${zlib_tarball} ]; then
  echo "**** Downloading ${zlib_tarball}"
  curl -L https://github.com/madler/zlib/releases/download/v${zlib_version}/zlib-${zlib_version}.tar.gz -o ${zlib_tarball}
fi
if [ ! -d ${zlib_src_dir} ]; then
  echo "**** Extracting ${zlib_tarball}"
  tar  ${tar_extra_args} -xzf ${zlib_tarball} -C ${source_dir}
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
hdf5_version=2.0.0
hdf5_middle_version=2_0_0
hdf5_short_version=2_0
hdf5_src_dir=$(ospath ${source_dir}/hdf5-${hdf5_version})
hdf5_build_dir=$(ospath ${build_dir}/hdf5-${hdf5_version}/)
hdf5_install_dir=$(ospath ${install_dir}/hdf5-${hdf5_version}/)
hdf5_tarball=$(ospath ${source_dir}/hdf5-${hdf5_version}.tar.gz)

# build only if install doesn't exist
if [ ! -d ${hdf5_install_dir} ]; then
if ${build_hdf5}; then
if [ ! -f ${hdf5_tarball} ]; then
  echo "**** Downloading ${hdf5_tarball}"
  curl -L https://support.hdfgroup.org/releases/hdf5/v${hdf5_short_version}/v${hdf5_middle_version}/downloads/hdf5-${hdf5_version}.tar.gz -o ${hdf5_tarball}
fi
if [ ! -d ${hdf5_src_dir} ]; then
  echo "**** Extracting ${hdf5_tarball}"
  tar ${tar_extra_args} -xzf ${hdf5_tarball} -C ${source_dir}
fi

#################
#
# hdf5 CMake recipe for using zlib
#
# -DHDF5_ENABLE_ZLIB_SUPPORT=ON
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
  -DHDF5_ENABLE_ZLIB_SUPPORT:BOOL=ON \
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

cgns_version=4.5.0
cgns_src_dir=$(ospath ${source_dir}/CGNS-${cgns_version})
cgns_build_dir=$(ospath ${build_dir}/cgns-${cgns_version}/)
cgns_install_dir=$(ospath ${install_dir}/cgns-${cgns_version}/)
cgns_tarball=$(ospath ${source_dir}/cgns-${cgns_version}.tar.gz)

# build only if install doesn't exist
if [ ! -d ${cgns_install_dir} ]; then
if ${build_cgns}; then
if [ ! -f ${cgns_tarball} ]; then
  echo "**** Downloading ${cgns_tarball}"
  curl -L https://github.com/CGNS/CGNS/archive/refs/tags/v${cgns_version}.tar.gz -o ${cgns_tarball}
fi
if [ ! -d ${cgns_src_dir} ]; then
  echo "**** Extracting ${cgns_tarball}"
  tar ${tar_extra_args} -xzf ${cgns_tarball} -C ${source_dir}

  # hdf5 2.0 patch
  cd ${cgns_src_dir}
  patch -p1 < ${script_dir}/2026_01_06_cgns_hdf5_2.patch
  cd ${root_dir}
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
# Silo
################
silo_version=4.12.0
silo_src_dir=$(ospath ${source_dir}/Silo-${silo_version})
silo_build_dir=$(ospath ${build_dir}/silo-${silo_version}/)
silo_install_dir=$(ospath ${install_dir}/silo-${silo_version}/)
silo_tarball=$(ospath ${source_dir}/silo-${silo_version}.tar.gz)

# build only if install doesn't exist
if [ ! -d ${silo_install_dir} ]; then
if ${build_silo}; then
if [ ! -f ${silo_tarball} ]; then
  echo "**** Downloading ${silo_tarball}"
  curl -L https://github.com/LLNL/Silo/archive/refs/tags/${silo_version}.tar.gz -o ${silo_tarball}
fi
if [ ! -d ${silo_src_dir} ]; then
  echo "**** Extracting ${silo_tarball}"
  # untar and avoid symlinks (which windows despises)
  tar ${tar_extra_args} -xzf ${silo_tarball} -C ${source_dir} \
      --exclude="Silo-${silo_version}/config-site/*" \
      --exclude="Silo-${silo_version}/LICENSE.md" \
      --exclude="Silo-${silo_version}/silo_objects.png"
fi

echo "**** Configuring Silo ${silo_version}"
cmake -S ${silo_src_dir} -B ${silo_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DCMAKE_INSTALL_PREFIX=${silo_install_dir} \
  -DSILO_ENABLE_SHARED=${build_shared_libs} \
  -DCMAKE_C_FLAGS=-Doff64_t=off_t \
  -DSILO_ENABLE_HDF5=ON \
  -DSILO_ENABLE_TESTS=OFF \
  -DSILO_BUILD_FOR_BSD_LICENSE=ON \
  -DSILO_ENABLE_FORTRAN=OFF \
  -DSILO_HDF5_DIR=${hdf5_install_dir}/cmake/ \
  -DCMAKE_PREFIX_PATH=${zlib_install_dir}


echo "**** Building Silo ${silo_version}"
cmake --build ${silo_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing Silo ${silo_version}"
cmake --install ${silo_build_dir} --config ${build_config}

fi
else
  echo "**** Skipping Silo build, install found at: ${silo_install_dir}"
fi # build_silo

############################
# Python Virtual Env
############################
python_exe="${python_exe:=python3}"
venv_install_dir=$(ospath ${install_dir}/python-venv/)
venv_python_exe=$(ospath ${venv_install_dir}/bin/python3)
venv_sphinx_exe=$(ospath ${venv_install_dir}/bin/sphinx-build)

# build only if install doesn't exist
if [ ! -d ${venv_install_dir} ]; then
if ${build_pyvenv}; then
    echo "**** Creating Python Virtual Env"
    cd ${install_dir} && ${python_exe} -m venv python-venv
    ${venv_python_exe} -m pip install --upgrade pip
    ${venv_python_exe} -m pip install wheel numpy sphinx sphinx_rtd_theme
    if ${build_zfp}; then
        ${venv_python_exe} -m pip install cython setuptools
    fi
    if [[ "$enable_mpi" == "ON" ]]; then
        ${venv_python_exe} -m pip install mpi4py
    fi
fi
else
  echo "**** Skipping Python venv build, install found at: ${venv_install_dir}"
fi # build_pyvenv

if ${build_pyvenv}; then
    venv_python_ver=`${venv_python_exe} -c "import sys;print('{0}.{1}'.format(sys.version_info.major, sys.version_info.minor))"`
    venv_python_site_pkgs_dir=${venv_install_dir}/lib/python${venv_python_ver}/site-packages
fi

################
# Caliper
################
caliper_version=2.11.0
caliper_src_dir=$(ospath ${source_dir}/Caliper-${caliper_version})
caliper_build_dir=$(ospath ${build_dir}/caliper-${caliper_version}/)
caliper_install_dir=$(ospath ${install_dir}/caliper-${caliper_version}/)
caliper_tarball=$(ospath ${source_dir}/caliper-${caliper_version}-src-with-blt.tar.gz)

# build only if install doesn't exist
if [ ! -d ${caliper_install_dir} ]; then
if ${build_caliper}; then
if [ ! -f ${caliper_tarball} ]; then
  echo "**** Downloading ${caliper_tarball}"
  curl -L https://github.com/LLNL/Caliper/archive/refs/tags/v${caliper_version}.tar.gz -o ${caliper_tarball}
fi
if [ ! -d ${caliper_src_dir} ]; then
  echo "**** Extracting ${caliper_tarball}"
  tar ${tar_extra_args} -xzf ${caliper_tarball} -C ${source_dir}
  # windows specific patch
  cd  ${caliper_src_dir}
  if [[ "$build_windows" == "ON" ]]; then
    patch -p1 < ${script_dir}/2024_08_01_caliper-win-smaller-opts.patch
  fi
  cd ${root_dir}
fi
#
# Note: Caliper has optional Umpire support,
# if we want to support in the future, we will need to build umpire first
#

# -DWITH_CUPTI=ON -DWITH_NVTX=ON -DCUDA_TOOLKIT_ROOT_DIR={path} -DCUPTI_PREFIX={path}
# -DWITH_ROCTRACER=ON -DWITH_ROCTX=ON -DROCM_PREFIX={path}

caliper_windows_cmake_flags="-DCMAKE_CXX_STANDARD=17 -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON -DWITH_TOOLS=OFF"

caliper_extra_cmake_args=""
if [[ "$build_windows" == "ON" ]]; then
  caliper_extra_cmake_args="${caliper_windows_cmake_flags}"
fi

# TODO enable_cuda
#
# if [[ "$enable_hip" == "ON" ]]; then
#   caliper_extra_cmake_args="${caliper_extra_cmake_args} -DWITH_ROCTRACER=ON"
#   caliper_extra_cmake_args="${caliper_extra_cmake_args} -DWITH_ROCTX=ON"
#   caliper_extra_cmake_args="${caliper_extra_cmake_args} -DROCM_PREFIX:PATH=${ROCM_PATH}"
#   caliper_extra_cmake_args="${caliper_extra_cmake_args} -DROCM_ROOT_DIR:PATH=${ROCM_PATH}"
# fi

if [[ "$enable_mpicc" == "ON" ]]; then
  caliper_extra_cmake_args="${caliper_extra_cmake_args} -DMPI_C_COMPILER=${mpicc_exe}"
  caliper_extra_cmake_args="${caliper_extra_cmake_args} -DMPI_CXX_COMPILER=${mpicxx_exe}"
fi


echo "**** Configuring Caliper ${caliper_version}"
cmake -S ${caliper_src_dir} -B ${caliper_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DBUILD_SHARED_LIBS=${build_shared_libs} \
  -DCMAKE_INSTALL_PREFIX=${caliper_install_dir} \
  -DWITH_MPI=${enable_mpi} ${caliper_extra_cmake_args}

echo "**** Building Caliper ${caliper_version}"
cmake --build ${caliper_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing Caliper ${caliper_version}"
cmake --install ${caliper_build_dir} --config ${build_config}

fi
else
  echo "**** Skipping Caliper build, install found at: ${caliper_install_dir}"
fi # build_caliper

################
# Camp
################
camp_version=v2025.09.2
camp_src_dir=$(ospath ${source_dir}/camp-${camp_version})
camp_build_dir=$(ospath ${build_dir}/camp-${camp_version})
camp_install_dir=$(ospath ${install_dir}/camp-${camp_version}/)
camp_tarball=$(ospath ${source_dir}/camp-${camp_version}.tar.gz)


# build only if install doesn't exist
if [ ! -d ${camp_install_dir} ]; then
if ${build_camp}; then
if [ ! -f ${camp_tarball} ]; then
  echo "**** Downloading ${camp_tarball}"
  curl -L https://github.com/LLNL/camp/releases/download/${camp_version}/camp-${camp_version}.tar.gz -o ${camp_tarball}
fi
if [ ! -d ${camp_src_dir} ]; then
  echo "**** Extracting ${camp_tarball}"
  tar ${tar_extra_args} -xzf ${camp_tarball} -C ${source_dir}
fi

camp_extra_cmake_args=""
if [[ "$enable_cuda" == "ON" ]]; then
  camp_extra_cmake_args="-DENABLE_CUDA=ON"
  camp_extra_cmake_args="${camp_extra_cmake_args} -DCMAKE_CUDA_ARCHITECTURES=${CUDA_ARCH}"
fi

if [[ "$enable_hip" == "ON" ]]; then
    camp_extra_cmake_args="-DENABLE_HIP=ON"
    camp_extra_cmake_args="${camp_extra_cmake_args} -DCMAKE_HIP_COMPILER=${CXX}"
    camp_extra_cmake_args="${camp_extra_cmake_args} -DCMAKE_HIP_ARCHITECTURES=${ROCM_ARCH}"
    camp_extra_cmake_args="${camp_extra_cmake_args} -DROCM_PATH=${ROCM_PATH}"
fi

echo "**** Configuring Camp ${camp_version}"
cmake -S ${camp_src_dir} -B ${camp_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose}\
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DBUILD_SHARED_LIBS=${build_shared_libs} \
  -DENABLE_TESTS=OFF \
  -DENABLE_EXAMPLES=OFF ${camp_extra_cmake_args} \
  -DCMAKE_INSTALL_PREFIX=${camp_install_dir}

echo "**** Building Camp ${camp_version}"
cmake --build ${camp_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing Camp ${camp_version}"
cmake --install ${camp_build_dir}  --config ${build_config}

fi
else
  echo "**** Skipping Camp build, install found at: ${camp_install_dir}"
fi # build_camp


################
# RAJA
################
raja_version=v2025.09.0
raja_src_dir=$(ospath ${source_dir}/RAJA-${raja_version})
raja_build_dir=$(ospath ${build_dir}/raja-${raja_version})
raja_install_dir=$(ospath ${install_dir}/raja-${raja_version}/)
raja_tarball=$(ospath ${source_dir}/RAJA-${raja_version}.tar.gz)
raja_enable_vectorization="${raja_enable_vectorization:=ON}"

# build only if install doesn't exist
if [ ! -d ${raja_install_dir} ]; then
if ${build_raja}; then
if [ ! -f ${raja_tarball} ]; then
  echo "**** Downloading ${raja_tarball}"
  curl -L https://github.com/LLNL/RAJA/releases/download/${raja_version}/RAJA-${raja_version}.tar.gz -o ${raja_tarball}
fi
if [ ! -d ${raja_src_dir} ]; then
  echo "**** Extracting ${raja_tarball}"
  tar ${tar_extra_args} -xzf ${raja_tarball} -C ${source_dir}
fi

raja_extra_cmake_args=""
if [[ "$enable_cuda" == "ON" ]]; then
  raja_extra_cmake_args="-DENABLE_CUDA=ON"
  raja_extra_cmake_args="${raja_extra_cmake_args} -DCMAKE_CUDA_ARCHITECTURES=${CUDA_ARCH}"
fi

if [[ "$enable_hip" == "ON" ]]; then
  raja_extra_cmake_args="-DENABLE_HIP=ON"
  raja_extra_cmake_args="${raja_extra_cmake_args} -DCMAKE_HIP_COMPILER=${CXX}"
  raja_extra_cmake_args="${raja_extra_cmake_args} -DCMAKE_HIP_ARCHITECTURES=${ROCM_ARCH}"
  raja_extra_cmake_args="${raja_extra_cmake_args} -DROCM_PATH=${ROCM_PATH}"
fi

echo "**** Configuring RAJA ${raja_version}"
cmake -S ${raja_src_dir} -B ${raja_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose}\
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DBUILD_SHARED_LIBS=${build_shared_libs} \
  -Dcamp_DIR=${camp_install_dir} \
  -DENABLE_OPENMP=${enable_openmp} \
  -DENABLE_TESTS=OFF \
  -DRAJA_ENABLE_TESTS=OFF \
  -DENABLE_EXAMPLES=OFF \
  -DENABLE_EXERCISES=OFF ${raja_extra_cmake_args} \
  -DCMAKE_INSTALL_PREFIX=${raja_install_dir} \
  -DRAJA_ENABLE_VECTORIZATION=${raja_enable_vectorization}

echo "**** Building RAJA ${raja_version}"
cmake --build ${raja_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing RAJA ${raja_version}"
cmake --install ${raja_build_dir}  --config ${build_config}

fi
else
  echo "**** Skipping RAJA build, install found at: ${raja_install_dir}"
fi # build_raja

################
# Umpire
################
# note: the release tarball naming scheme for Umpire is different vs RAJA + Camp
umpire_version=2025.09.0
umpire_src_dir=$(ospath ${source_dir}/umpire-${umpire_version})
umpire_build_dir=$(ospath ${build_dir}/umpire-${umpire_version})
umpire_install_dir=$(ospath ${install_dir}/umpire-${umpire_version}/)
umpire_tarball=$(ospath ${source_dir}/umpire-${umpire_version}.tar.gz)
umpire_windows_cmake_flags="-DBLT_CXX_STD=c++17 -DCMAKE_CXX_STANDARD=17 -DUMPIRE_ENABLE_FILESYSTEM=On -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=On"

umpire_extra_cmake_args=""
if [[ "$build_windows" == "ON" ]]; then
  umpire_extra_cmake_args="${umpire_windows_cmake_flags}"
fi

if [[ "$enable_cuda" == "ON" ]]; then
  umpire_extra_cmake_args="${umpire_extra_cmake_args} -DENABLE_CUDA=ON"
  umpire_extra_cmake_args="${umpire_extra_cmake_args} -DCMAKE_CUDA_ARCHITECTURES=${CUDA_ARCH}"
fi

if [[ "$enable_hip" == "ON" ]]; then
  umpire_extra_cmake_args="${umpire_extra_cmake_args} -DENABLE_HIP=ON"
  umpire_extra_cmake_args="${umpire_extra_cmake_args} -DCMAKE_HIP_COMPILER=${CXX}"
  umpire_extra_cmake_args="${umpire_extra_cmake_args} -DCMAKE_HIP_ARCHITECTURES=${ROCM_ARCH}"
  umpire_extra_cmake_args="${umpire_extra_cmake_args} -DROCM_PATH=${ROCM_PATH}"
fi

# build only if install doesn't exist
if [ ! -d ${umpire_install_dir} ]; then
if ${build_umpire}; then
if [ ! -f ${umpire_tarball} ]; then
  echo "**** Downloading ${umpire_tarball}"
  curl -L https://github.com/LLNL/Umpire/releases/download/v${umpire_version}/umpire-${umpire_version}.tar.gz -o ${umpire_tarball}
fi
if [ ! -d ${umpire_src_dir} ]; then
  echo "**** Extracting ${umpire_tarball}"
  tar ${tar_extra_args} -xzf ${umpire_tarball} -C ${source_dir}
fi

echo "**** Configuring Umpire ${umpire_version}"
cmake -S ${umpire_src_dir} -B ${umpire_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DBUILD_SHARED_LIBS=${build_shared_libs} \
  -Dcamp_DIR=${camp_install_dir} \
  -DENABLE_OPENMP=${enable_openmp} \
  -DENABLE_TESTS=OFF \
  -DUMPIRE_ENABLE_TOOLS=Off \
  -DUMPIRE_ENABLE_BENCHMARKS=OFF ${umpire_extra_cmake_args} \
  -DCMAKE_INSTALL_PREFIX=${umpire_install_dir}

echo "**** Building Umpire ${umpire_version}"
cmake --build ${umpire_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing Umpire ${umpire_version}"
cmake --install ${umpire_build_dir}  --config ${build_config}

fi
else
  echo "**** Skipping Umpire build, install found at: ${umpire_install_dir}"
fi # build_umpire


################
# ZFP
################
zfp_version=1.0.1
zfp_src_dir=$(ospath ${source_dir}/zfp-${zfp_version})
zfp_build_dir=$(ospath ${build_dir}/zfp-${zfp_version}/)
zfp_install_dir=$(ospath ${install_dir}/zfp-${zfp_version}/)
zfp_tarball=$(ospath ${source_dir}/zfp-${zfp_version}.tar.gz)

# build only if install doesn't exist
if [ ! -d ${zfp_install_dir} ]; then
if ${build_zfp}; then
if [ ! -f ${zfp_tarball} ]; then
  echo "**** Downloading ${zfp_tarball}"
  curl -L https://github.com/LLNL/zfp/releases/download/1.0.1/zfp-${zfp_version}.tar.gz -o ${zfp_tarball}
fi
if [ ! -d ${zfp_src_dir} ]; then
  echo "**** Extracting ${zfp_tarball}"
  tar ${tar_extra_args} -xzf ${zfp_tarball} -C ${source_dir}

  # apply patches
  cd ${zfp_src_dir}
  patch -p1 < ${script_dir}/2025_02_14_zfp_python_build_hardening.patch
  cd ${root_dir}
fi

#
# extra cmake args
#
zfp_extra_cmake_opts="-DBUILD_ZFPY=${enable_python}"
if ${build_pyvenv}; then
  zfp_extra_cmake_opts="${zfp_extra_cmake_opts} -DPYTHON_EXECUTABLE=${venv_python_exe}"
  zfp_extra_cmake_opts="${zfp_extra_cmake_opts} -Dpython_install_lib_dir=${venv_python_site_pkgs_dir}"
fi

echo "**** Configuring ZFP ${zfp_version}"
cmake -S ${zfp_src_dir} -B ${zfp_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DBUILD_SHARED_LIBS=${build_shared_libs} \
  -DZFP_BIT_STREAM_WORD_SIZE=8 \
  -DCMAKE_INSTALL_PREFIX=${zfp_install_dir} ${zfp_extra_cmake_opts}

echo "**** Building ZFP ${zfp_version}"
cmake --build ${zfp_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing ZFP ${zfp_version}"
cmake --install ${zfp_build_dir} --config ${build_config}

fi
else
  echo "**** Skipping ZFP build, install found at: ${zfp_install_dir}"
fi # build_zfp


################
# H5Z-ZFP
################
h5zzfp_version=1.1.1
h5zzfp_src_dir=$(ospath ${source_dir}/H5Z-ZFP-${h5zzfp_version})
h5zzfp_build_dir=$(ospath ${build_dir}/h5zzfp-${h5zzfp_version}/)
h5zzfp_install_dir=$(ospath ${install_dir}/h5zzfp-${h5zzfp_version}/)
h5zzfp_tarball=$(ospath ${source_dir}/h5zzfp-${h5zzfp_version}.tar.gz)

# build only if install doesn't exist
if [ ! -d ${h5zzfp_install_dir} ]; then
# also enabled via `build_zfp` instead of sep option
if ${build_zfp}; then
if [ ! -f ${h5zzfp_tarball} ]; then
  echo "**** Downloading ${h5zzfp_tarball}"
  curl -L "https://github.com/LLNL/H5Z-ZFP/archive/refs/tags/v${h5zzfp_version}.tar.gz"  -o ${h5zzfp_tarball}
fi
if [ ! -d ${h5zzfp_src_dir} ]; then
  echo "**** Extracting ${h5zzfp_tarball}"
  tar ${tar_extra_args} -xzf ${h5zzfp_tarball} -C ${source_dir}

  # apply patches
  cd ${h5zzfp_src_dir}
  patch -p1 < ${script_dir}/2025_12_08_h5zzfp-hdf5-cmake-fix.patch
  cd ${root_dir}
fi

echo "**** Configuring H5Z-ZFP ${h5zzfp_version}"

HDF5_DIR=${hdf5_install_dir}  \
ZFP_DIR=${zfp_install_dir}/lib/cmake/zfp/ \
cmake -S ${h5zzfp_src_dir} -B ${h5zzfp_build_dir} ${cmake_compiler_settings} \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=${enable_verbose} \
  -DCMAKE_BUILD_TYPE=${build_config} \
  -DFORTRAN_INTERFACE=OFF \
  -DBUILD_SHARED_LIBS=${build_shared_libs} \
  -DCMAKE_INSTALL_PREFIX=${h5zzfp_install_dir}

echo "**** Building H5Z-ZFP ${h5zzfp_version}"
cmake --build ${h5zzfp_build_dir} --config ${build_config} -j${build_jobs}
echo "**** Installing H5Z-ZFP ${h5zzfp_version}"
cmake --install ${h5zzfp_build_dir} --config ${build_config}

fi
else
  echo "**** Skipping H5Z-ZFP build, install found at: ${h5zzfp_install_dir}"
fi # build_h5zzfp


################
# Conduit
################
# if we are in an checkout, use existing source
conduit_checkout_dir=$(ospath ${script_dir}/../../src)
conduit_checkout_dir=$(abs_path ${conduit_checkout_dir})
echo ${conduit_checkout_dir}
if [ -d ${conduit_checkout_dir} ]; then
    conduit_version=checkout
    conduit_src_dir=$(abs_path ${conduit_checkout_dir})
    echo "**** Using existing Conduit source repo checkout: ${conduit_src_dir}"
else
    # otherwise use develop
    conduit_version=develop
    conduit_src_dir=$(ospath ${source_dir}/conduit/src)
fi

conduit_build_dir=$(ospath ${build_dir}/conduit-${conduit_version}/)
conduit_install_dir=$(ospath ${install_dir}//conduit-${conduit_version}/)

cmake_host_config=${root_dir}/conduit-config.cmake

echo "**** Creating Conduit host-config (conduit-config.cmake)"
#
echo '# host-config file generated by build_conduit.sh' > ${cmake_host_config}

# capture compilers if they are provided via env vars
if [ ! -z ${CC+x} ]; then
    echo 'set(CMAKE_C_COMPILER ' ${CC} ' CACHE PATH "")' >>  ${cmake_host_config}
fi

if [ ! -z ${CXX+x} ]; then
    echo 'set(CMAKE_CXX_COMPILER ' ${CXX} ' CACHE PATH "")' >>  ${cmake_host_config}
fi

if [ ! -z ${FTN+x} ]; then
    echo 'set(CMAKE_Fortran_COMPILER ' ${FTN} ' CACHE PATH "")' >>  ${cmake_host_config}
fi

# capture compiler flags  if they are provided via env vars
if [ ! -z ${CFLAGS+x} ]; then
    echo 'set(CMAKE_C_FLAGS "' ${CFLAGS} '" CACHE PATH "")' >>  ${cmake_host_config}
fi

if [ ! -z ${CXXFLAGS+x} ]; then
    echo 'set(CMAKE_CXX_FLAGS "' ${CXXFLAGS} '" CACHE PATH "")' >>  ${cmake_host_config}
fi

if [ ! -z ${FFLAGS+x} ]; then
    echo 'set(CMAKE_F_FLAGS "' ${FFLAGS} '" CACHE PATH "")' >>  ${cmake_host_config}
fi

if [[ "$enable_mpicc" == "ON" ]]; then
  echo 'set(MPI_C_COMPILER '  ${mpicc_exe}  ' CACHE PATH "")' >> ${cmake_host_config}
  echo 'set(MPI_CXX_COMPILER ' ${mpicxx_exe}  ' CACHE PATH "")' >> ${cmake_host_config}
fi

echo 'set(CMAKE_VERBOSE_MAKEFILE ' ${enable_verbose} ' CACHE BOOL "")' >> ${cmake_host_config}
echo 'set(CMAKE_BUILD_TYPE ' ${build_config} ' CACHE STRING "")' >> ${cmake_host_config}
echo 'set(BUILD_SHARED_LIBS ' ${build_shared_libs} ' CACHE STRING "")' >> ${cmake_host_config}
echo 'set(CMAKE_INSTALL_PREFIX ' ${conduit_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
echo 'set(ENABLE_TESTS ' ${enable_tests} ' CACHE BOOL "")' >> ${cmake_host_config}
echo 'set(ENABLE_MPI ' ${enable_mpi} ' CACHE BOOL "")' >> ${cmake_host_config}
echo 'set(ENABLE_FIND_MPI ' ${enable_find_mpi} ' CACHE BOOL "")' >> ${cmake_host_config}
echo 'set(ENABLE_FORTRAN ' ${enable_fortran} ' CACHE BOOL "")' >> ${cmake_host_config}
echo 'set(ENABLE_PYTHON ' ${enable_python} ' CACHE BOOL "")' >> ${cmake_host_config}
if ${build_pyvenv}; then
echo 'set(PYTHON_EXECUTABLE ' ${venv_python_exe} ' CACHE PATH "")' >> ${cmake_host_config}
echo 'set(PYTHON_MODULE_INSTALL_PREFIX ' ${venv_python_site_pkgs_dir} ' CACHE PATH "")' >> ${cmake_host_config}
echo 'set(ENABLE_DOCS ON CACHE BOOL "")' >> ${cmake_host_config}
echo 'set(SPHINX_EXECUTABLE ' ${venv_sphinx_exe} ' CACHE PATH "")' >> ${cmake_host_config}
fi
if ${build_caliper}; then
  echo 'set(CALIPER_DIR ' ${caliper_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
fi
if ${build_camp}; then
  echo 'set(CAMP_DIR ' ${camp_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
fi
if ${build_raja}; then
  echo 'set(RAJA_DIR ' ${raja_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
fi
if ${build_umpire}; then
  echo 'set(UMPIRE_DIR ' ${umpire_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
fi


echo 'set(HDF5_DIR ' ${hdf5_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
echo 'set(CGNS_DIR ' ${cgns_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
echo 'set(ZLIB_DIR ' ${zlib_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
if ${build_zfp}; then
  echo 'set(ZFP_DIR ' ${zfp_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
  echo 'set(H5ZZFP_DIR ' ${h5zzfp_install_dir} ' CACHE PATH "")' >> ${cmake_host_config}
fi

if [[ "$enable_cuda" == "ON" ]]; then
    echo 'set(ENABLE_CUDA ON CACHE BOOL "")' >> ${cmake_host_config}
    echo 'set(CMAKE_CUDA_ARCHITECTURES ' ${CUDA_ARCH} ' CACHE PATH "")' >> ${cmake_host_config}
fi

if [[ "$enable_hip" == "ON" ]]; then
    echo 'set(ENABLE_HIP ON CACHE BOOL "")' >> ${cmake_host_config}
    echo 'set(CMAKE_HIP_COMPILER ' ${CXX} ' CACHE STRING "")' >> ${cmake_host_config}
    echo 'set(CMAKE_HIP_ARCHITECTURES ' ${ROCM_ARCH} ' CACHE STRING "")' >> ${cmake_host_config}
    echo 'set(ROCM_PATH ' ${ROCM_PATH} ' CACHE PATH "")' >> ${cmake_host_config}
fi

# build only if install doesn't exist
if [ ! -d ${conduit_install_dir} ]; then
if ${build_conduit}; then
if [ ! -d ${conduit_src_dir} ]; then
    echo "**** Cloning Conduit"
    git clone --recursive https://github.com/LLNL/conduit.git
fi

echo "**** Configuring Conduit"
cmake -S ${conduit_src_dir} -B ${conduit_build_dir} -C ${cmake_host_config}

echo "**** Building Conduit"
cmake --build ${conduit_build_dir} --config ${build_config} -j${build_jobs}

echo "**** Installing Conduit"
cmake --install ${conduit_build_dir}  --config ${build_config}

fi
else
  echo "**** Skipping Conduit build, install found at: ${conduit_install_dir}"
fi # build_conduit
