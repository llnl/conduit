# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

"""
 file: t_python_relay_io_blueprint.py
 description: Unit tests for the conduit relay io python module interface.

"""

import sys
import os
import unittest

from numpy import *
from conduit import Node


import conduit
import conduit.relay as relay
import conduit.blueprint as blueprint
import conduit.relay.mpi.io
import conduit.relay.mpi.io.blueprint


class Test_Relay_IO_Blueprint(unittest.TestCase):
    def test_relay_io_blueprint_write_read_mesh(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm_id = MPI.COMM_WORLD.py2f()

        # only run if we have hdf5
        if not relay.mpi.io.about(comm_id)["protocols/hdf5"] == "enabled":
            return
        data = Node()
        blueprint.mesh.examples.julia(200,200,
                                      -2.0, 2.0,
                                      -2.0, 2.0,
                                      0.285, 0.01,
                                      data);
        data["state/cycle"] =0
        tbase = "tout_python_relay_io_blueprint_mesh_t1"
        tout = tbase + ".cycle_000000.root"
        if os.path.isfile(tout):
            os.remove(tout)
        print("saving to {0}".format(tout))
        relay.mpi.io.blueprint.write_mesh(data, tbase, "hdf5", comm_id)
        self.assertTrue(os.path.isfile(tout))

        n_load = Node()
        info = Node()
        relay.mpi.io.blueprint.read_mesh(n_load, tout, comm_id)
        print(n_load)
        data.diff(n_load,info)
        print(info)

    def test_relay_io_blueprint_save_load_mesh(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm_id = MPI.COMM_WORLD.py2f()

        # only run if we have hdf5
        if not relay.mpi.io.about(comm_id)["protocols/hdf5"] == "enabled":
            return
        data = Node()
        blueprint.mesh.examples.julia(200,200,
                                      -2.0, 2.0,
                                      -2.0, 2.0,
                                      0.285, 0.01,
                                      data);
        data["state/cycle"] =0
        tbase = "tout_python_relay_io_blueprint_mesh_t1"
        tout = tbase + ".cycle_000000.root"
        if os.path.isfile(tout):
            os.remove(tout)
        print("saving to {0}".format(tout))
        relay.mpi.io.blueprint.save_mesh(data, tbase, "hdf5", comm_id)
        self.assertTrue(os.path.isfile(tout))

        n_load = Node()
        info = Node()
        n_load["HERE_IS_SOMETHING_BEFORE_WE_LOAD"] = 1
        relay.mpi.io.blueprint.load_mesh(n_load, tout, comm_id)
        # with load sematics, the input node is cleared, so this 
        # child should no longer exist
        self.assertFalse(n_load.has_child("HERE_IS_SOMETHING_BEFORE_WE_LOAD"))
        print(n_load)
        data.diff(n_load,info)
        print(info)

if __name__ == '__main__':
    unittest.main()


