# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

###############################################################################
# file: __init__.py
# Purpose: Main init for the conduit blueprint module.
###############################################################################

###############################################################################
# NOTE: windows python 3.8 and newer ignore PATH env var when loading DLLs
#   https://docs.python.org/3/whatsnew/3.8.html#bpo-36085-whatsnew
#   https://docs.python.org/3/library/os.html#os.add_dll_directory
#
# Allow CONDUIT_DLL_DIR to be used so windows can pick up dlls
# for python 3.8 and newer
import os
import sys
import platform
if "CONDUIT_DLL_DIR" in os.environ:
    # os.add_dll_directory should exist in Python >= 3.8 on windows
    if platform.system() == 'Windows':
        if sys.version_info.major >= 3 and sys.version_info.minor >= 8:
            for dll_path in os.environ["CONDUIT_DLL_DIR"].split(";"):
                os.add_dll_directory(dll_path)
###############################################################################


from .conduit_blueprint_python import *

from . import mcarray
from . import mesh
from . import table

# mpi support is optional, so drive on if we can't import
try:
    from . import mpi
except:
    pass

