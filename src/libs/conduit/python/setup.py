# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
###############################################################################

###############################################################################
# file: setup.py
# Purpose: setuptools setup for conduit python module.
#
###############################################################################
import sys
import platform
from setuptools import setup

# path args fix helper for windows
def adjust_windows_args_paths():
    print("[windows detected: normalizing paths]")
    nargs =[]
    for v in sys.argv:
        nargs.append(v.replace("/","\\"))
    sys.argv = nargs

# if windows,  defend against path issue:
#  https://github.com/LLNL/conduit/issues/1017
if platform.system() == 'Windows':
    adjust_windows_args_paths()

CONDUIT_VERSION = '0.9.5'

setup (name = 'conduit',
       description = 'conduit',
       version = CONDUIT_VERSION,
       package_dir = {'conduit':'py_src'},
       zip_safe=False,
       packages=['conduit',
                 'conduit.utils',
                 'conduit.blueprint',
                 'conduit.blueprint.mcarray',
                 'conduit.blueprint.mcarray.examples',
                 'conduit.blueprint.mesh',
                 'conduit.blueprint.mesh.examples',
                 'conduit.blueprint.table',
                 'conduit.blueprint.table.examples',
                 'conduit.blueprint.mpi',
                 'conduit.blueprint.mpi.mesh',
                 'conduit.blueprint.mpi.mesh.examples',
                 'conduit.relay',
                 'conduit.relay.io',
                 'conduit.relay.io.blueprint',
                 'conduit.relay.io.silo',
                 'conduit.relay.mpi',
                 'conduit.relay.mpi.io',
                 'conduit.relay.mpi.io.blueprint',
                 'conduit.relay.web'])
