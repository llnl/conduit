# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

FROM ubuntu:24.04

ARG TARGETARCH
ARG CMAKE_VERSION=3.31.12

#####################
# base deps
#####################
# add sudo to base ubuntu container
# so we can install additional packages as
# non-root, but admin default user on azure pipelines
RUN apt-get update && apt-get -y install sudo
# tzdata install wants to ask questions, so handled as sep case
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata
# install std packages
#
# Note: switched to openmpi due to issues with mpich in ubuntu 24.04
#  https://github.com/pmodels/mpich/issues/7064
#  https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1102612
#  https://bugs.launchpad.net/ubuntu/+source/mpich/+bug/2072338
#
RUN apt-get update && apt-get -y install \
             binutils \
             cmake \
             gcc \
             g++ \
             gfortran \
             python3 \
             python3-dev \
             python-is-python3 \
             python3.12-venv \
             perl \
             git \
             git-lfs \
             curl \
             wget \
             tar \
             unzip \
             file \
             build-essential \
             libncurses-dev \
             libssl-dev \
             libblas-dev \
             liblapack-dev \
             liblapacke-dev \
             zlib1g-dev \
             libgdbm-dev \
             libreadline-dev \
             libsqlite3-dev \
             libbz2-dev \
             vim

# manual mpich install
# https://github.com/actions/runner-images/issues/13204
#
# amd64: curl -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/28285882/
# arm64: curl -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/28285883/

RUN case "$TARGETARCH" in \
      "amd64") BUILD_NUM="28285882" ;; \
      "arm64") BUILD_NUM="28285883" ;; \
    esac; \
    curl -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/${BUILD_NUM}/+files/libmpich12_4.2.0-5.1_${TARGETARCH}.deb

RUN case "$TARGETARCH" in \
          "amd64") BUILD_NUM="28285882" ;; \
          "arm64") BUILD_NUM="28285883" ;; \
    esac; \
    curl -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/${BUILD_NUM}/+files/mpich_4.2.0-5.1_${TARGETARCH}.deb

RUN case "$TARGETARCH" in \
          "amd64") BUILD_NUM="28285882" ;; \
          "arm64") BUILD_NUM="28285883" ;; \
    esac; \
    curl -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/${BUILD_NUM}/+files/libmpich-dev_4.2.0-5.1_${TARGETARCH}.deb


RUN apt install  -y ./libmpich12_4.2.0-5.1_${TARGETARCH}.deb
RUN apt install  -y ./mpich_4.2.0-5.1_${TARGETARCH}.deb
RUN apt install  -y ./libmpich-dev_4.2.0-5.1_${TARGETARCH}.deb

RUN rm -rf /var/lib/apt/lists/*

# manual cmake install
RUN case "$TARGETARCH" in \
          "amd64") CMAKE_ARCH="linux-x86_64" ;; \
          "arm64") CMAKE_ARCH="aarch64" ;; \
    esac; \
    curl -L https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-${CMAKE_ARCH}.tar.gz -o cmake-${CMAKE_VERSION}-${CMAKE_ARCH}.tar.gz && \
    tar -xzf cmake-${CMAKE_VERSION}-${CMAKE_ARCH}.tar.gz && \
    cmake-${CMAKE_VERSION}-${CMAKE_ARCH}/bin/cmake --version && \
    mv cmake-${CMAKE_VERSION}-${CMAKE_ARCH} cmake
    
ENV PATH="/cmake/bin:${PATH}"

RUN cmake --version

# untar the current source  (created as part of build.sh)
COPY conduit.docker.src.tar /
RUN tar -xf conduit.docker.src.tar

# bootstrap third party libs
RUN chmod +x  conduit/scripts/build_conduit/build_conduit.sh
RUN cd conduit && env prefix=tpls enable_mpi=ON       \
                                  enable_fortran=ON   \
                                  enable_tests=OFF    \
                                  enable_verbose=OFF  \
                                  build_caliper=true  \
                                  build_conduit=false \
                                  build_jobs=2        \
            ./scripts/build_conduit/build_conduit.sh


# configure a debug build with cmake
RUN cd conduit && \
     cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/conduit/install \
    -C tpls/conduit-config.cmake \
    -B build -S src

# build, test, and install conduit
RUN cmake --build conduit/build --config Release
RUN ctest --test-dir conduit/build --output-on-failure -V
RUN cmake --install conduit/build --config Release

# open port 9000, for use by conduit relay
EXPOSE 9000

CMD ["/bin/bash"]
