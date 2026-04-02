# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

set -ev

# remove old source tarball if it exists
echo "rm -f conduit.docker.src.tar.gz"
rm -f conduit.docker.src.tar.gz

# get current copy of the conduit source
echo "cd ../../../../ && python3 package.py src/examples/docker/ubuntu/conduit.docker.src.tar"
cd ../../../../ && python3 package.py src/examples/docker/ubuntu/conduit.docker.src.tar

# change back to the dir with our Dockerfile
echo "cd src/examples/docker/ubuntu/"
cd src/examples/docker/ubuntu/

export TAG_ARCH=`uname -m`
export TAG_NAME=conduit:ubuntu-24.04-${TAG_ARCH}

# exec docker build to create image
echo "docker build -t ${TAG_NAME} ."
docker build -t ${TAG_NAME} .


