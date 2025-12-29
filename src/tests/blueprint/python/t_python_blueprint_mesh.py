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

import conduit.blueprint.mesh
import conduit.blueprint.mesh.examples

from conduit import Node

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

    def test_basic_and_verify(self):
        n = Node()
        info = Node()
        self.assertTrue(blueprint.verify("mesh",n,info))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertTrue(self.has_empty_warning(info))
        blueprint.mesh.examples.basic("hexs",2,2,2,n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))
        n_idx = Node()
        blueprint.mesh.generate_index(n,"",1,n_idx)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))

    def test_braid_and_verify(self):
        n = Node()
        info = Node()
        self.assertTrue(blueprint.verify("mesh",n,info))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertTrue(self.has_empty_warning(info))
        blueprint.mesh.examples.braid("hexs",2,2,2,n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))
        n_idx = Node()
        blueprint.mesh.generate_index(n,"",1,n_idx)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))

    def test_julia_and_verify(self):
        n = Node()
        info = Node()
        self.assertTrue(blueprint.verify("mesh",n,info))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertTrue(self.has_empty_warning(info))
        blueprint.mesh.examples.julia(200,200,
                                      -2.0, 2.0,
                                      -2.0, 2.0,
                                      0.285, 0.01,
                                      n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))
        n_idx = Node()
        blueprint.mesh.generate_index(n,"",1,n_idx)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))

    def test_spiral_and_verify(self):
        n = Node()
        info = Node()
        self.assertTrue(blueprint.verify("mesh",n,info))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertTrue(self.has_empty_warning(info))
        blueprint.mesh.examples.spiral(4,n);
        self.assertTrue(blueprint.mesh.verify(n["domain_000000"],info))
        self.assertFalse(self.has_empty_warning(info))
        n_idx = Node()
        blueprint.mesh.generate_index(n["domain_000000"],"",4,n_idx)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))

    def test_julia_nestsets(self):
        n = Node()
        info = Node()
        self.assertTrue(blueprint.verify("mesh",n,info))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertTrue(self.has_empty_warning(info))
        # simple case
        blueprint.mesh.examples.julia_nestsets_simple(-2.0, 2.0,
                                                      -2.0, 2.0,
                                                      0.285, 0.01,
                                                      n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))
        n_idx = Node()
        blueprint.mesh.generate_index(n["domain_000000"],"",1,n_idx)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))
        # complex case
        blueprint.mesh.examples.julia_nestsets_complex(50,50,
                                                       -2.0, 2.0,
                                                       -2.0, 2.0,
                                                       0.285, 0.01,
                                                       3,
                                                       n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        n_idx = Node()
        blueprint.mesh.generate_index(n["domain_000000"],"",1,n_idx)
        self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
        self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))

    def test_venn(self):
        n = Node()
        info = Node()
        self.assertTrue(blueprint.verify("mesh",n,info))
        self.assertTrue(self.has_empty_warning(info))
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertTrue(self.has_empty_warning(info))
        for matset_type in ['full', 
                            'sparse_by_material',
                            'sparse_by_element' ]:
            blueprint.mesh.examples.venn(matset_type,
                                         10,10,
                                         .5,
                                         n);
            self.assertTrue(blueprint.mesh.verify(n,info))
            self.assertFalse(self.has_empty_warning(info))
            n_idx = Node()
            blueprint.mesh.generate_index(n,"",1,n_idx)
            self.assertTrue(blueprint.verify("mesh/index",n_idx,info))
            print(info)
            self.assertTrue(blueprint.mesh.verify(protocol="index",node=n_idx,info=info))
            print(info)

    def test_polytess(self):
        n = Node()
        info = Node()

        # 2D case
        blueprint.mesh.examples.polytess(3,1,n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))

        # 3D case
        blueprint.mesh.examples.polytess(3,3,n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))

    def test_polychain(self):
        n = Node()
        info = Node()

        # simple case
        blueprint.mesh.examples.polychain(5,n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))

    def test_polystar(self):
        n = Node()
        info = Node()
        blueprint.mesh.examples.polystar(n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))

    def test_strided_structured(self):
        n = Node()
        info = Node()
        opts = Node()
        blueprint.mesh.examples.strided_structured(opts,5,5,5,n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))

        # test w/ options
        opts["vertex_data/shape"]   = [7,8,0]
        opts["vertex_data/origin"]  = [2,1,0]
        opts["element_data/shape"]  = [6,4,0]
        opts["element_data/origin"] = [1,1,0]

        blueprint.mesh.examples.strided_structured(opts,4, 3, 0,n);
        self.assertTrue(blueprint.mesh.verify(n,info))
        self.assertFalse(self.has_empty_warning(info))

    def test_examples_generate_default_opts(self):
        opts = conduit.Node()
        blueprint.mesh.examples.generate_default_options(opts)
        print(opts)
        enames = ["braid",
                  "basic",
                  "strided_structured",
                  "grid",
                  "spiral",
                  "polytess",
                  "polychain",
                  "misc",
                  "adjset_uniform",
                  "gyre",
                  "julia",
                  "julia_nestsets_simple",
                  "julia_nestsets_complex",
                  "polystar",
                  "related_boundary",
                  "rz_cylinder",
                  "tiled",
                  "venn"]
    
        for ename in enames:
            opts = conduit.Node()
            blueprint.mesh.examples.generate_default_options(opts,ename)
            print(ename)
            print(opts)

    def test_examples_generate(self):
        enames = ["braid",
                  "basic",
                  "strided_structured",
                  "grid",
                  "spiral",
                  "polytess",
                  "polychain",
                  "misc",
                  "adjset_uniform",
                  "gyre",
                  "julia",
                  "julia_nestsets_simple",
                  "julia_nestsets_complex",
                  "polystar",
                  "related_boundary",
                  "rz_cylinder",
                  "tiled",
                  "venn"]
        for ename in enames:
            n    = conduit.Node()
            info = conduit.Node()
            blueprint.mesh.examples.generate(ename,n)
            self.assertTrue(blueprint.mesh.verify(n,info))

    def test_partition(self):
        def same(a, b):
            for i in range(len(a)):
                if a[i] != b[i]:
                    return False
            return True
        n = Node()
        blueprint.mesh.examples.spiral(4, n)
        output = Node()
        options = Node()
        options["target"] = 2
        options["mapping"] = 0
        conduit.blueprint.mesh.partition(n, options, output)
        self.assertTrue(options.number_of_children() == 2)
        expected_x0 = (0., 1., 2.)
        expected_y0 = (0., 1., 2., 3.)
        expected_x1 = (-3.0, -2.0, -1.0, 0.0)
        expected_y1 = (0.0, 1.0, 2.0, 3.0)
        self.assertTrue(same(expected_x0, output[0]["coordsets/coords/values/x"]))
        self.assertTrue(same(expected_y0, output[0]["coordsets/coords/values/y"]))
        self.assertTrue(same(expected_x1, output[1]["coordsets/coords/values/x"]))
        self.assertTrue(same(expected_y1, output[1]["coordsets/coords/values/y"]))

    def test_convert(self):
        tgts = { 
        "uniform":      [ "uniform", "rectilinear", "structured", "unstructured", "polytopal"],
        "rectilinear":  [ "rectilinear", "structured", "unstructured", "polytopal"],
        "structured":   [ "structured", "unstructured", "polytopal"],
        "unstructured": [ "unstructured", "polytopal"],
        "generate":     [ "generate_points",
                          "generate_lines",
                          "generate_faces",
                          "generate_centroids",
                          "generate_sides",
                          "generate_corners"]
        }

        braid_types = { "uniform" : [ { "mesh_type": "uniform", "dims": [5, 5, 0]} , 
                                      { "mesh_type": "uniform", "dims": [5, 5, 5]} ],
                        "rectilinear" : [ { "mesh_type": "rectilinear", "dims": [5, 5, 0]} , 
                                          { "mesh_type": "rectilinear", "dims": [5, 5, 5]} ],
                        "structured" : [ { "mesh_type": "structured", "dims": [5, 5, 0]} , 
                                         { "mesh_type": "structured", "dims": [5, 5, 5]} ],
                        "unstructured" : [ { "mesh_type": "tris", "dims": [5, 5, 0]} , 
                                           { "mesh_type": "hexs", "dims": [5, 5, 5]},
                                           { "mesh_type": "quads_poly", "dims": [5, 5, 0]},
                                           { "mesh_type": "hexs_poly", "dims": [5, 5, 5]},
                                           ]}

        for mesh_cat, test_meshes in braid_types.items():
            for test_mesh in test_meshes:
                n = Node()
                output = Node()
                options = Node()
                maps = Node()
                conduit.blueprint.mesh.examples.braid(test_mesh["mesh_type"],
                                                      test_mesh["dims"][0],
                                                      test_mesh["dims"][1],
                                                      test_mesh["dims"][2],
                                                      n)
                for target in tgts[mesh_cat]:
                    print("Testing",test_mesh, "to", target)
                    options["target"] = target 
                    conduit.blueprint.mesh.convert(n, options, output)
                    conduit.blueprint.mesh.convert(n, options, output, maps)


if __name__ == '__main__':
    unittest.main()


