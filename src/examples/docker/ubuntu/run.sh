#!/bin/bash
###############################################################################
# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

# exec docker run to create a container from our image
echo "docker run -p 9000:9000 -t -i conduit-ubuntu:current"
docker run -p 9000:9000 -t -i conduit-ubuntu:current


