# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
"""
 file: t_python_blueprint_mesh.py
 description: Simple unit test for the conduit blueprint mesh python module.

"""

import sys
import unittest

import conduit.blueprint as blueprint
import conduit.blueprint.mpi

import conduit.blueprint.mpi.mesh
import conduit.blueprint.mpi.mesh.examples

import conduit.relay as relay
import conduit.relay.mpi

from conduit import Node

# TODO:
# from mpi4py import MPI

class Test_Blueprint_Mesh(unittest.TestCase):

    def has_empty_warning(self, info):
        res = False

        if info.dtype().is_object() and info.has_child("info"):
            iinfo = info.fetch('info')
            if iinfo.dtype().is_object() or iinfo.dtype().is_list():
                iitr = iinfo.children()
                for ival in iitr:
                    inode = ival.node()
                    res = res or (inode.dtype().is_string() and "is an empty mesh" in inode.to_string())

        return res

    def test_braid_uniform_multi_domain_and_verify(self):
        n = Node()
        info = Node()
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm_id = MPI.COMM_WORLD.py2f()
        self.assertTrue(blueprint.mpi.verify("mesh",n,info,comm_id))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mpi.mesh.verify(n,info,comm_id))
        self.assertTrue(self.has_empty_warning(info))
        blueprint.mpi.mesh.examples.braid_uniform_multi_domain(n,comm_id);
        self.assertTrue(blueprint.mpi.mesh.verify(n,info,comm_id))
        self.assertFalse(self.has_empty_warning(info))
        n_idx = Node()
        blueprint.mpi.mesh.generate_index(n,"",n_idx,comm_id)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))

    def test_spiral_round_robin_and_verify(self):
        n = Node()
        info = Node()
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm_id = MPI.COMM_WORLD.py2f()
        self.assertTrue(blueprint.mpi.verify("mesh",n,info,comm_id))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mpi.mesh.verify(n,info,comm_id))
        self.assertTrue(self.has_empty_warning(info))
        blueprint.mpi.mesh.examples.spiral_round_robin(4,n,comm_id);
        self.assertTrue(blueprint.mpi.mesh.verify(n,info,comm_id))
        self.assertFalse(self.has_empty_warning(info))
        n_idx = Node()
        blueprint.mpi.mesh.generate_index(n,"",n_idx,comm_id)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))

    def test_partition(self):
        def same(a, b):
            for i in range(len(a)):
                if a[i] != b[i]:
                    return False
            return True
        n = Node()
        # skip tests on windows until we work out proper
        # mpi4py install for our windows ci
        if sys.platform == "win32":
            return
        from mpi4py import MPI
        comm_id = MPI.COMM_WORLD.py2f()
        rank    = relay.mpi.rank(comm_id)
        blueprint.mpi.mesh.examples.spiral_round_robin(4, n, comm_id)
        output = Node()
        options = Node()
        options["target"] = 2
        options["mapping"] = 0
        conduit.blueprint.mpi.mesh.partition(n, options, output, comm_id)
        self.assertTrue(options.number_of_children() == 2)
        expected_x0 = (0.0, 1.0, 0.0, 1.0, 2.0, 2.0, -3.0, -2.0, -1.0, -3.0, -2.0, -1.0, -3.0, -2.0, -1.0, 0.0, -3.0, -2.0, -1.0, 0.0)
        expected_y0 = (0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0, 3.0, 3.0, 3.0, 3.0)
        expected_x1 = (0.0, 1.0, 2.0)
        expected_y1 = (1.0, 2.0, 3.0)
        if rank == 0:
            self.assertTrue(same(expected_x0, output["coordsets/coords/values/x"]))
            self.assertTrue(same(expected_y0, output["coordsets/coords/values/y"]))
        else:
            self.assertTrue(same(expected_x1, output["coordsets/coords/values/x"]))
            self.assertTrue(same(expected_y1, output["coordsets/coords/values/y"]))

if __name__ == '__main__':
    unittest.main()


