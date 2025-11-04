# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
"""
 file: t_python_relay_io.py
 description: Unit tests for the conduit relay io python module interface.

"""

import sys
import os
import unittest

from numpy import *
from conduit import Node
from conduit import DataType

import conduit
import conduit.relay as relay
import conduit.relay.mpi
import conduit.relay.mpi.io

class Test_Relay_IO(unittest.TestCase):
    def test_load_save(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        comm_id = MPI.COMM_WORLD.py2f()
        a_val = uint32(10)
        b_val = uint32(20)
        c_val = float64(30.0)
        #
        n = Node()
        n['a'] = a_val
        n['b'] = b_val
        n['c'] = c_val
        self.assertTrue(n['a'] == a_val)
        self.assertTrue(n['b'] == b_val)
        self.assertTrue(n['c'] == c_val)
        relay.mpi.io.save(n,"tout_python_relay_io_save_load.conduit_bin",comm_id)
        #  now load the value
        comm.Barrier()
        n_load = Node()
        relay.mpi.io.load(n_load,"tout_python_relay_io_save_load.conduit_bin",comm_id)
        self.assertTrue(n_load['a'] == a_val)
        self.assertTrue(n_load['b'] == b_val)
        self.assertTrue(n_load['c'] == c_val)

    def test_load_save_protocols(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        comm_id = MPI.COMM_WORLD.py2f()
        a_val = int64(10)
        b_val = int64(20)
        c_val = float64(30.0)
        #
        n = Node()
        n['a'] = a_val
        n['b'] = b_val
        n['c'] = c_val
        self.assertTrue(n['a'] == a_val)
        self.assertTrue(n['b'] == b_val)
        self.assertTrue(n['c'] == c_val)

        protos = ["conduit_bin",
                  "json",
                  "conduit_json",
                  "conduit_base64_json",
                  "yaml"]
        
        # only test hdf5 if relay was built with hdf5 support
        if relay.mpi.io.about(comm_id)["protocols/hdf5"] == "enabled":
            protos.append("hdf5")
        
        for proto in protos:
            print("testing protocol: ", proto)
            ftest = "tout_python_relay_io_save_load_proto." + proto
            relay.mpi.io.save(n,ftest,comm_id)
            comm.Barrier()
            #  now load the value
            n_load = Node()
            relay.mpi.io.load(n_load,ftest,comm_id)
            self.assertTrue(n_load['a'] == a_val)
            self.assertTrue(n_load['b'] == b_val)
            self.assertTrue(n_load['c'] == c_val)
            comm.Barrier()
        # only test silo if relay was built with hdf5 support
        if relay.mpi.io.about(comm_id)["protocols/conduit_silo"] == "enabled":
            # silo needs a subpath
            print("testing protocol: silo")
            ftest = "tout_python_relay_io_save_load_proto.silo:obj"
            relay.mpi.io.save(n,ftest,comm_id)
            comm.Barrier()
            #  now load the value
            n_load = Node()
            relay.mpi.io.load(n_load,ftest,comm_id)
            self.assertTrue(n_load['a'] == a_val)
            self.assertTrue(n_load['b'] == b_val)
            self.assertTrue(n_load['c'] == c_val)
            comm.Barrier()

    def test_load_merged(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        comm_id = MPI.COMM_WORLD.py2f()
        rank    = relay.mpi.rank(comm_id)
        a_val = int64(10)
        b_val = int64(20)
        c_val = float64(30.0)
        d_val = a_val * 4
        #
        n = Node()
        n['a'] = a_val
        n['b'] = b_val
        n['c'] = c_val
        tout = "tout_python_relay_io_load_merged.conduit_bin"

        if rank == 0:
            if os.path.isfile(tout):
                os.remove(tout)
            if os.path.isfile(tout + "_json"):
                os.remove(tout + "_json")

        comm.Barrier()
        relay.mpi.io.save(n,tout,comm_id)
        comm.Barrier()
        self.assertTrue(os.path.isfile(tout))
        n_load = Node()
        n_load['d'] = d_val
        relay.mpi.io.load_merged(n_load,tout,comm_id)
        print(n_load)
        self.assertTrue(n_load.has_child("d"))
        self.assertTrue(n_load.has_child("a"))
        self.assertTrue(n_load['a'] == a_val)
        self.assertTrue(n_load['d'] == d_val)
   
    def test_hdf5_generic_save_opts(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm_id = MPI.COMM_WORLD.py2f()

        # only run if we have hdf5
        if not relay.mpi.io.about(comm_id)["protocols/hdf5"] == "enabled":
            return
        # 5k float64 zeros, will compress well, but below default 
        # thresh
        n = Node()
        opts = Node()
        opts["hdf5/chunking/threshold"]  = 2000
        opts["hdf5/chunking/chunk_size"] = 2000
        # hdf5 requires object at top level ... 
        n['value'].set(DataType.float64(5000))
        tout_std = "tout_python_relay_io_hdf5_generic_std.hdf5"
        tout_cmp = "tout_python_relay_io_hdf5_generic_cmp.hdf5"
        relay.mpi.io.save(n,tout_std,comm_id)
        relay.mpi.io.save(n,tout_cmp,options=opts,comm=comm_id)
        tout_std_fs  = os.path.getsize(tout_std)
        tout_std_cmp = os.path.getsize(tout_cmp)
        self.assertTrue(os.path.isfile(tout_std))
        self.assertTrue(os.path.isfile(tout_cmp))
        print("fs compare: std = ", tout_std_fs, " cmp = ", tout_std_cmp)
        self.assertTrue(tout_std_cmp < tout_std_fs)

    def test_io_errors(self):
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm_id = MPI.COMM_WORLD.py2f()

        n = Node()
        with self.assertRaises(IOError):
            relay.mpi.io.load(n,"pile_of_garbage.conduit_bin",comm_id)
        with self.assertRaises(IOError):
            relay.mpi.io.load_merged(n,"another_pile_of_garbage.conduit_bin",comm_id)
        with self.assertRaises(IOError):
            relay.mpi.io.save(n,"/bad/bad/bad/cant_write_here.conduit_bin",comm_id)
        with self.assertRaises(IOError):
            relay.mpi.io.save_merged(n,"/bad/bad/bad/cant_write_here_either.conduit_bin",comm_id)




if __name__ == '__main__':
    unittest.main()


