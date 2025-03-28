# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
"""
 file: python_blueprint_smoke.py
 description: Simple unit test for the conduit blueprint python module interface.

"""

import sys
import unittest

import conduit.blueprint as blueprint

class Test_Blueprint_Basic(unittest.TestCase):
    def test_about(self):
        print(blueprint.about())

if __name__ == '__main__':
    unittest.main()


