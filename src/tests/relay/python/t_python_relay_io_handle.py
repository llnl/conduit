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
import conduit.relay.io

class Test_Relay_IO_Handle(unittest.TestCase):
    def test_io_handle(self):
        tbase = "tout_python_relay_io_handle."
        protos = ["conduit_bin",
                  "json",
                  "conduit_json",
                  "conduit_base64_json",
                  "yaml",
                  "conduit_base64_yaml"]
        
        # only test hdf5 if relay was built with hdf5 support
        if relay.io.about()["protocols/hdf5"] == "enabled":
            protos.append("hdf5")
        for proto in protos:
            test_file = tbase + proto
            if os.path.isfile(test_file):
                os.remove(test_file)
            
            n = conduit.Node();
            n["a"] = int64(20);
            n["b"] = int64(8);
            n["c"] = int64(12);
            n["d/here"] = int64(10);

            h = conduit.relay.io.IOHandle();
            h.open(test_file)
            h.write(n)
            self.assertTrue(h.has_path("d/here"))
            cnames = h.list_child_names()
            self.assertTrue(cnames[0] == "a")
            self.assertTrue(cnames[1] == "b")
            self.assertTrue(cnames[2] == "c")
            self.assertTrue(cnames[3] == "d")
            cnames = h.list_child_names("d")
            self.assertTrue(cnames[0] == "here")
            h.remove("d");
            self.assertFalse(h.has_path("d"))
            self.assertFalse(h.has_path("d/here"))
            h.close();
            n2 = conduit.Node()
            h2 = conduit.relay.io.IOHandle();
            h2.open(test_file);
            cnames = h2.list_child_names();
            self.assertTrue(cnames[0] == "a")
            self.assertTrue(cnames[1] == "b")
            self.assertTrue(cnames[2] == "c")
            n_val = conduit.Node()
            n_val.set(int64(10))
            h2.write(n_val,"d/here")
            h2.read(n2);
            info = conduit.Node()
            self.assertFalse(n.diff(n2,info,0.0))
            n_val.reset()
            h2.read(n_val,"c");
            self.assertTrue(n_val.value() == 12)
            h2.close()

    def test_io_handle_flush(self):
        tbase = "tout_python_relay_io_handle_flush."
        protos = ["conduit_bin",
                  "json",
                  "conduit_json",
                  "conduit_base64_json",
                  "yaml",
                  "conduit_base64_yaml"]
 
        # only test hdf5 if relay was built with hdf5 support
        if relay.io.about()["protocols/hdf5"] == "enabled":
            protos.append("hdf5")
        for proto in protos:
            test_file = tbase + proto
            if os.path.isfile(test_file):
                os.remove(test_file)
            
            n = conduit.Node();
            n["a"] = int64(20);
            n["b"] = int64(8);
            n["c"] = int64(12);
            n["d/here"] = int64(10);

            h = conduit.relay.io.IOHandle();
            h.open(test_file)
            h.write(n)
            h.flush()

            n2 = conduit.Node()
            h2 = conduit.relay.io.IOHandle();
            ronly_opts = conduit.Node()
            ronly_opts["mode"] = "r"
            h2.open(test_file,options=ronly_opts);
            h2.read(n2);
            self.assertTrue(n2["a"] == 20)
            self.assertTrue(n2["b"] == 8)
            self.assertTrue(n2["c"] == 12)
            self.assertTrue(n2["d/here"] == 10)
            h2.close()
            h.close();

    def test_io_handle_exceptions(self):
            h = conduit.relay.io.IOHandle()
            n = conduit.Node()
            # call to un-opend
            with self.assertRaises(IOError):
                h.read(n);
            with self.assertRaises(IOError):
                h.write(n);
            with self.assertRaises(IOError):
                h.has_path("here");
            with self.assertRaises(IOError):
                h.remove("here");
            with self.assertRaises(IOError):
                h.list_child_names();

            # bad path
            with self.assertRaises(IOError):
                h.open("here/is/a/garbage/file/path.json");

    def test_io_handle_is_open(self):
            h = conduit.relay.io.IOHandle()
            self.assertFalse(h.is_open())
            # bad path
            with self.assertRaises(IOError):
                h.open("here/is/a/garbage/file/path.json");
            # still not open
            self.assertFalse(h.is_open())
            # open valid path
            h.open("tout_python_relay_io_handle.conduit_json")
            # better be open
            self.assertTrue(h.is_open())
            # close it
            h.close()
            # better be closed
            self.assertFalse(h.is_open())

    def test_io_handle_offset_stride_size(self):
            # only test hdf5 if relay was built with hdf5 support
            if relay.io.about()["protocols/hdf5"] != "enabled":
                return

            # remove files if they already exist
            tfile_out = "tout_hdf5_io_handle_with_offset.hdf5";
            if os.path.isfile(tfile_out):
                os.remove(tfile_out)

            n = conduit.Node()
            n_read = conduit.Node()
            n_check = conduit.Node()
            opts = conduit.Node()
            info = conduit.Node()

            n["data"]= [0,1,2,3,4,5,6,7,8,9];

            h = conduit.relay.io.IOHandle()
            h.open(tfile_out)
            h.write(n)

            h.read(n_read)
            print(n_read)

            n_check = n
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # strided read
            n_read.reset()
            opts.reset()
            opts["stride"] = 2
            h.read(n_read,options=opts)
            print(n_read)

            n_check.reset();
            n_check["data"] = [0,2,4,6,8];
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # offset write
            n.set([-1,-1,-1,-1,-1])
            opts.reset()
            opts["offset"] = 5
            h.write(node=n,path="data",options=opts)

            n_read.reset()
            h.read(n_read)
            print(n_read)

            n_check.reset()
            n_check["data"] = [0,1,2,3,4,-1,-1,-1,-1,-1]
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # read the  first part of the seq
            opts.reset()
            opts["size"] = 5
            n_read.reset()
            h.read(node=n_read, path="data", options=opts)
            print(n_read)

            n_check.reset()
            n_check.set([0,1,2,3,4])
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # read the second part of the seq (-1's)
            opts.reset();
            opts["offset"] = 5
            n_read.reset()
            h.read(path="data",node=n_read,options=opts)
            print(n_read)

            n_check.reset()
            n_check.set([-1,-1,-1,-1,-1])
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # strided write
            n.set([1,1,1,1,1])
            opts.reset()
            opts["stride"] = 2
            h.write(node=n,path="data",options=opts)

            # strided +offset write
            n.set([2,2,2,2,2])
            opts.reset()
            opts["offset"] = 1
            opts["stride"] = 2
            h.write(n,"data",opts)

            n_read.reset()
            h.read(n_read)
            print(n_read)

            n_check.reset()
            n_check["data"] = [1, 2, 1, 2, 1, 2,  1, 2, 1, 2];
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))


            # read the 1's
            opts.reset();
            opts["offset"] = 0
            opts["stride"] = 2
            n_read.reset()
            h.read(path="data",node=n_read,options=opts)
            print(n_read)

            n_check.reset()
            n_check.set([1, 1, 1, 1, 1])
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # read the 2's
            opts.reset()
            opts["offset"] = 1
            opts["stride"] = 2
            n_read.reset()
            h.read(path="data",node=n_read,options=opts)
            print(n_read)

            n_check.reset()
            n_check.set([2, 2, 2, 2, 2])
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # read subset of the 2's
            opts.reset()
            opts["offset"] = 1
            opts["stride"] = 2
            opts["size"] = 2
            n_read.reset()
            h.read(path="data",node=n_read,options=opts)
            print(n_read)

            n_check.reset();
            n_check.set([2, 2]);
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # huge stride, this will only read the first entry
            n_read.reset()
            opts.reset()
            opts["stride"] = 1000
            h.read(node=n_read,options=opts)
            print(n_read)

            n_check.reset()
            n_check["data"] = [1]
            # expect no diff
            self.assertFalse(n_read.diff(n_check,info))

            # huge size
            n_read.reset()
            opts.reset()
            opts["size"] = 1000
            with self.assertRaises(IOError):
                h.read(node=n_read,options=opts)

            # huge offset
            n_read.reset()
            opts.reset()
            opts["offset"] = 1000
            with self.assertRaises(IOError):
                h.read(node=n_read,options=opts)



if __name__ == '__main__':
    unittest.main()


