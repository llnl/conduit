# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
"""
 file: t_python_blueprint_mpi.py
 description: Unit tests for blueprint mpi

"""

import sys
import unittest

import conduit
import conduit.blueprint as blueprint
import conduit.blueprint.mpi

# TODO:
# from mpi4py import MPI

class Test_Relay_MPI_Module(unittest.TestCase):

    def test_about(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        print(conduit.blueprint.mpi.about())
        self.assertTrue(True)


if __name__ == '__main__':
    unittest.main()



