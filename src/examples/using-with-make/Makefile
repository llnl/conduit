# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
###############################################################################
#
# Example that shows how to use an installed instance of Conduit in Makefile
# based build system.
#
# To build:
#  make CONDUIT_DIR={conduit install path} 
#  ./conduit_example
#
# From within a conduit install:
#  make 
#  ./conduit_example
#
# Which corresponds to:
#
#  make CONDUIT_DIR=../../../ 
#  ./conduit_example
#
###############################################################################

CONDUIT_DIR ?= ../../..

# See $(CONDUIT_DIR)/share/conduit/conduit_config.mk for detailed linking info
include $(CONDUIT_DIR)/share/conduit/conduit_config.mk

# Conduit requires c++17 support
CXX_FLAGS = -std=c++17
INC_FLAGS = $(CONDUIT_INCLUDE_FLAGS)
LNK_FLAGS = $(CONDUIT_LINK_RPATH) $(CONDUIT_LIB_FLAGS)

main:
	$(CXX) $(CXX_FLAGS) $(INC_FLAGS) conduit_example.cpp $(LNK_FLAGS) -o conduit_example

clean:
	rm -f conduit_example

