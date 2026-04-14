# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
"""
 file: python_conduit_datatype.py
 description: Unit tests for conduit::DataType python module interface.

"""

import sys
import unittest

from conduit import Node
from conduit import DataType

from numpy import *


class Test_Conduit_Node(unittest.TestCase):
    def test_simple(self):
        a_val = uint32(10)
        b_val = uint32(20)
        c_val = float64(30.0)

        n = Node()
        n['a'] = a_val
        n['b'] = b_val
        n['c'] = c_val
        print(n)
        d = n.fetch('a').dtype()
        self.assertEqual(d.id(),DataType.name_to_id("uint32"))
        print(d)

    def test_id_to_name(self):
        names = [DataType.id_to_name(i) for i in range(14)]
        ids   = [DataType.name_to_id(n) for n in names]
        self.assertEqual(ids,list(range(14)))

    def test_explicit_set(self):
        d = DataType()
        d.set(dtype_id = DataType.name_to_id("uint32"),
              num_elements = 1,
              offset = 0,
              stride = 4,
              element_bytes = 4)
        print(d)
        self.assertEqual(d.id(),DataType.name_to_id("uint32"))
        self.assertEqual(d.number_of_elements(),1)
        self.assertEqual(d.offset(),0)
        self.assertEqual(d.stride(),4)
        self.assertEqual(d.element_bytes(),4)
        self.assertEqual(d.endianness(),0)

    def test_construction(self):
        dt = DataType();
        dt.set_id(DataType.name_to_id("uint32"))
        dt.set_number_of_elements(10);
        dt.set_offset(0);
        dt.set_stride(4);
        dt.set_element_bytes(4);

        dt2 = DataType(dt)
        self.assertEqual(dt.id(),dt2.id())
        self.assertEqual(dt.number_of_elements(),dt2.number_of_elements())
        self.assertEqual(dt.offset(),dt2.offset())
        self.assertEqual(dt.stride(),dt2.stride())
        self.assertEqual(dt.element_bytes(),dt2.element_bytes())
        self.assertEqual(dt.endianness(),dt2.endianness())

        dt3 = DataType()
        dt3.set(dtype_name="uint32",
                num_elements=10,
                offset=0,
                stride=4,
                element_bytes=4)
        self.assertEqual(dt2.id(),dt3.id())
        self.assertEqual(dt2.number_of_elements(),dt3.number_of_elements())
        self.assertEqual(dt2.offset(),dt3.offset())
        self.assertEqual(dt2.stride(),dt3.stride())
        self.assertEqual(dt2.element_bytes(),dt3.element_bytes())
        self.assertEqual(dt2.endianness(),dt3.endianness())

        print(dt)
        print(dt2)
        print(dt3)

    def test_constructor_helpers(self):
        # objs
        print(DataType.empty());
        print(DataType.object());
        print(DataType.list());
        # signed integers
        print(DataType.int8());
        print(DataType.int16());
        print(DataType.int32());
        print(DataType.int64());
        # unsigned integers
        print(DataType.uint8());
        print(DataType.uint16());
        print(DataType.uint32());
        print(DataType.uint64());
        # floating point
        print(DataType.float32());
        print(DataType.float64());
        # signed integers
        print(DataType.c_char());
        print(DataType.c_short());
        print(DataType.c_int());
        print(DataType.c_long());
        # unsigned integers
        print(DataType.c_unsigned_char());
        print(DataType.c_unsigned_short());
        print(DataType.c_unsigned_int());
        print(DataType.c_unsigned_long());
        # floating point
        print(DataType.c_float());
        print(DataType.c_double());

    def test_is_checks(self):
        # objs
        edt = DataType.empty()
        self.assertEqual(edt.name(),"empty")
        self.assertTrue(edt.is_empty())
        # lots of false checks anchored from empty
        self.assertFalse(edt.is_object())
        self.assertFalse(edt.is_list())
        #
        self.assertFalse(edt.is_number())
        self.assertFalse(edt.is_floating_point())
        self.assertFalse(edt.is_integer())
        self.assertFalse(edt.is_signed_integer())
        self.assertFalse(edt.is_unsigned_integer())
        #
        self.assertFalse(edt.is_int8())
        self.assertFalse(edt.is_int16())
        self.assertFalse(edt.is_int32())
        self.assertFalse(edt.is_int64())
        #
        self.assertFalse(edt.is_uint8())
        self.assertFalse(edt.is_uint16())
        self.assertFalse(edt.is_uint32())
        self.assertFalse(edt.is_uint64())
        #
        self.assertFalse(edt.is_float32())
        self.assertFalse(edt.is_float64())
        #
        self.assertFalse(edt.is_char())
        self.assertFalse(edt.is_short())
        self.assertFalse(edt.is_int())
        self.assertFalse(edt.is_long())
        self.assertFalse(edt.is_long_long())
        #
        self.assertFalse(edt.is_signed_char())
        self.assertFalse(edt.is_signed_short())
        self.assertFalse(edt.is_signed_int())
        self.assertFalse(edt.is_signed_long())
        self.assertFalse(edt.is_signed_long_long())
        #
        self.assertFalse(edt.is_unsigned_char())
        self.assertFalse(edt.is_unsigned_short())
        self.assertFalse(edt.is_unsigned_int())
        self.assertFalse(edt.is_unsigned_long())
        self.assertFalse(edt.is_unsigned_long_long())
        #
        self.assertFalse(edt.is_float())
        self.assertFalse(edt.is_double())
        self.assertFalse(edt.is_long_double())
        #
        self.assertFalse(edt.is_string())
        self.assertFalse(edt.is_char8_str())
        #
        self.assertFalse(edt.is_index_t())
        #
        self.assertFalse(DataType.object().is_empty())
        #############
        #
        self.assertTrue(DataType.object().is_object())
        self.assertTrue(DataType.list().is_list())
        # signed integers
        self.assertTrue(DataType.int8().is_number())
        self.assertTrue(DataType.int8().is_integer())
        self.assertTrue(DataType.int8().is_signed_integer())
        self.assertFalse(DataType.int8().is_unsigned_integer())
        self.assertFalse(DataType.int8().is_floating_point())
        self.assertTrue(DataType.int8().is_int8())
        self.assertTrue(DataType.int16().is_int16())
        self.assertTrue(DataType.int32().is_int32())
        self.assertTrue(DataType.int64().is_int64())
        # unsigned integers
        self.assertTrue(DataType.uint8().is_uint8())
        self.assertTrue(DataType.uint8().is_number())
        self.assertTrue(DataType.uint8().is_integer())
        self.assertFalse(DataType.uint8().is_signed_integer())
        self.assertTrue(DataType.uint8().is_unsigned_integer())
        self.assertFalse(DataType.uint8().is_floating_point())
        #
        self.assertTrue(DataType.uint16().is_uint16())
        self.assertTrue(DataType.uint32().is_uint32())
        self.assertTrue(DataType.uint64().is_uint64())
        # floating point
        self.assertTrue(DataType.float32().is_number())
        self.assertTrue(DataType.float32().is_floating_point())
        self.assertTrue(DataType.float32().is_float32())
        self.assertTrue(DataType.float64().is_float64())
        # c style integers
        self.assertTrue(DataType.c_char().is_char())
        self.assertTrue(DataType.c_short().is_short())
        self.assertTrue(DataType.c_int().is_int())
        self.assertTrue(DataType.c_long().is_long())
        self.assertFalse(DataType.c_char().is_long_long())
        # c style unsigned integers
        self.assertTrue(DataType.c_unsigned_char().is_unsigned_char());
        self.assertTrue(DataType.c_unsigned_short().is_unsigned_short());
        self.assertTrue(DataType.c_unsigned_int().is_unsigned_int());
        self.assertTrue(DataType.c_unsigned_long().is_unsigned_long());
        # floating point
        self.assertTrue(DataType.c_float().is_float());
        self.assertTrue(DataType.c_double().is_double());
        # string
        self.assertTrue(DataType.char8_str().is_string());
        self.assertTrue(DataType.char8_str().is_char8_str());
        # index_t
        self.assertTrue(DataType.index_t().is_index_t());

    def test_enum_ids(self):
        # objs
        self.assertEqual(DataType.empty().id(),  DataType.empty_id())
        self.assertEqual(DataType.object().id(), DataType.object_id())
        self.assertEqual(DataType.list().id(),   DataType.list_id())
        # signed integers
        self.assertEqual(DataType.int8().id(),  DataType.int8_id())
        self.assertEqual(DataType.int16().id(), DataType.int16_id())
        self.assertEqual(DataType.int32().id(), DataType.int32_id())
        self.assertEqual(DataType.int64().id(), DataType.int64_id())
        # unsigned integers
        self.assertEqual(DataType.uint8().id(),  DataType.uint8_id())
        self.assertEqual(DataType.uint16().id(), DataType.uint16_id())
        self.assertEqual(DataType.uint32().id(), DataType.uint32_id())
        self.assertEqual(DataType.uint64().id(), DataType.uint64_id())
        # floating point
        self.assertEqual(DataType.float32().id(), DataType.float32_id())
        self.assertEqual(DataType.float64().id(), DataType.float64_id())

    def test_construct_from_args(self):
        # string case
        dt = DataType("int64",
                      7, # num ele
                      10, # offset
                      16, # stride
                      8, # ele bytes
                      0) # endianness id
        self.assertTrue(dt.is_int64())
        self.assertEqual(dt.number_of_elements(),7)
        self.assertEqual(dt.offset(),10)
        self.assertEqual(dt.stride(),16)
        self.assertEqual(dt.element_bytes(),8)
        self.assertEqual(dt.endianness(),0)
        # id case
        dt = DataType(DataType.name_to_id("int64"),
                      7, # num ele
                      10, # offset
                      16, # stride
                      8, # ele bytes
                      0) # endianness id
        dt = DataType.int64(7,10,16,8)
        self.assertTrue(dt.is_int64())
        self.assertEqual(dt.number_of_elements(),7)
        self.assertEqual(dt.offset(),10)
        self.assertEqual(dt.stride(),16)
        self.assertEqual(dt.element_bytes(),8)
        self.assertEqual(dt.endianness(),0)

        # helper func case
        dt = DataType.int64(7,10,16,8,0)
        self.assertTrue(dt.is_int64())
        self.assertEqual(dt.number_of_elements(),7)
        self.assertEqual(dt.offset(),10)
        self.assertEqual(dt.stride(),16)
        self.assertEqual(dt.element_bytes(),8)
        self.assertEqual(dt.endianness(),0)

    def test_to_string_and_friends(self):
        dtypes = [ DataType.float64(),
                   DataType.object(),
                   DataType.list(),
                   DataType.empty()]

        for d in dtypes:
            print("yaml rep")
            print(d.to_string("yaml"))

            print("json rep")
            print(d.to_string("json"))

            self.assertEqual(d.to_string("yaml"),d.to_yaml())
            self.assertEqual(d.to_string("json"),d.to_json())




if __name__ == '__main__':
    unittest.main()


