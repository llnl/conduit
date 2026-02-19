# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

FROM ubuntu:24.04

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
             clang \
             llvm \
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
             nodejs \
             npm \
             vim

# manual mpich install
# https://github.com/actions/runner-images/issues/13204
RUN curl -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/28285882/+files/libmpich12_4.2.0-5.1_amd64.deb
RUN curl -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/28285882/+files/mpich_4.2.0-5.1_amd64.deb
RUN curl  -LO https://launchpad.net/ubuntu/+source/mpich/4.2.0-5.1/+build/28285882/+files/libmpich-dev_4.2.0-5.1_amd64.deb

RUN apt install  -y ./libmpich12_4.2.0-5.1_amd64.deb
RUN apt install  -y ./mpich_4.2.0-5.1_amd64.deb
RUN apt install  -y ./libmpich-dev_4.2.0-5.1_amd64.deb

RUN rm -rf /var/lib/apt/lists/*

# untar the current source  (created as part of example_build.sh)
COPY conduit.docker.src.tar /
RUN tar -xf conduit.docker.src.tar

# if you would like to clone conduit directly, you can use:
#RUN git clone --depth 1 https://github.com/LLNL/conduit.git

# at some sites ssl certs are intercepted, which cases issues fetching 
# tpl sources via https

# to resolve this, either:
# 1) pass the "-k" option to uberenv (recommended), 
# 2) install the proper certs into the image, or
# 3) use  the following commands to disable ssl for git and 
#    curl (both are used by spack):
#RUN git config --global http.sslVerify false
#RUN echo insecure >> ~/.curlrc

# bootstrap third party libs using spack and uberenv
RUN cd conduit && python3 scripts/uberenv/uberenv.py \
    -k \
    --spec "+mpi%gcc" \
    --spack-env-file=scripts/uberenv_configs/spack_configs/envs/ubuntu-24.04/spack.yaml

# configure a debug build with cmake
RUN cd conduit && \
    uberenv_libs/spack/opt/spack/*/cmake-*/bin/cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=/conduit/install-debug \
    -C /conduit/uberenv_libs/*-patch.cmake \
    -B build-debug -S src

# build, test, and install conduit
RUN cd conduit/build-debug && make
RUN cd conduit/build-debug && env make test
RUN cd conduit/build-debug && make install

# open port 9000, for use by conduit relay
EXPOSE 9000

CMD ["/bin/bash"]
