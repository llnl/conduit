# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

FROM ubuntu:18.04

# fetch build env
RUN apt-get update && apt-get install -y \
    git \
    curl \
    build-essential \
    gcc \
    g++ \
    gfortran \
    zlib1g-dev \
    python \
    python3 \
    tar \
    unzip \
    perl \
    libncurses-dev \
    libssl-dev \
    libopenblas-dev \
    zlib1g-dev \
    libgdbm-dev \
    libreadline-dev \
    libsqlite3-dev \
    libbz2-dev \
    mpich \
    libmpich-dev \
    vim \
 && rm -rf /var/lib/apt/lists/*


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
#  for this example we use mpich for MPI b/c openmpi's mpiexec
#  will not run for the root user
RUN cd conduit && python3 scripts/uberenv/uberenv.py \
    -k \
    --spec "+mpi%gcc" \
    --spack-env-file=scripts/uberenv_configs/spack_configs/envs/alpinedav-ubuntu_18.04_devel/spack.yaml

# configure a debug build with cmake
RUN cd conduit && \
    uberenv_libs/spack/opt/spack/*/*/cmake*/bin/cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX=/conduit/install-debug \
    -C /conduit/uberenv_libs/*-patch.cmake \
    -B build-debug -S src

# build, test, and install conduit
RUN cd conduit/build-debug && make
RUN cd conduit/build-debug && env CTEST_OUTPUT_ON_FAILURE=1 make test
RUN cd conduit/build-debug && make install

# open port 9000, for use by conduit relay
EXPOSE 9000

CMD ["/bin/bash"]
