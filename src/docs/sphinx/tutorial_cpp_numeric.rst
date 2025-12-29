.. # Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
.. # Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
.. # other details. No copyright assignment is required to contribute to Conduit.

=======================
Accessing Numeric Data
=======================


You can access leaf types (numeric scalars or arrays) using Node's *as_{type}*
methods.  You can let Conduit handle striding with the DataArray class, and
additionally type conversion with the DataAccessor class.

Accessing Scalars
-----------------

The call to *Node::as_int64()* gives you the numeric value.  Make sure the
Node does actually contain an int64, or Conduit will throw an error.

.. # from t_conduit_docs_tutorial_numeric: numeric_as_dtype

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: BEGIN_EXAMPLE("numeric_as_dtype")
   :end-before:  END_EXAMPLE("numeric_as_dtype")
   :language: cpp
   :dedent: 4

.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_as_dtype")
   :end-before:  END_EXAMPLE("numeric_as_dtype")
   
Or you can use *Node::value()*, which can infer the correct return type via a
cast.  As with the *as_{type}* methods, make sure to ask for the type of data
that the Node actually contains.

.. # from t_conduit_docs_tutorial_numeric: numeric_via_value

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: BEGIN_EXAMPLE("numeric_via_value")
   :end-before:  END_EXAMPLE("numeric_via_value")
   :language: cpp
   :dedent: 4

.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_via_value")
   :end-before:  END_EXAMPLE("numeric_via_value")


Accessing Arrays
----------------

You can get numeric array data out of Conduit as a pointer; as a DataArray,
which handles striding; or as a DataAccessor, which handles striding and
type conversion.

Access array data via pointers using Node's *as_{type}* methods.

.. # from t_conduit_docs_tutorial_numeric: numeric_ptr_as_dtype

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: BEGIN_EXAMPLE("numeric_ptr_as_dtype")
   :end-before:  END_EXAMPLE("numeric_ptr_as_dtype")
   :language: cpp
   :dedent: 4

.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_ptr_as_dtype")
   :end-before:  END_EXAMPLE("numeric_ptr_as_dtype")


Or using Node::value():

.. # from t_conduit_docs_tutorial_numeric: numeric_ptr_via_value

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: BEGIN_EXAMPLE("numeric_ptr_via_value")
   :end-before:  END_EXAMPLE("numeric_ptr_via_value")
   :language: cpp
   :dedent: 4

.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_ptr_via_value")
   :end-before:  END_EXAMPLE("numeric_ptr_via_value")



For non-contiguous arrays, direct pointer access is complex due to the indexing
required. Conduit provides a simple DataArray class that handles per-element 
indexing for all types of arrays.

.. # from t_conduit_docs_tutorial_numeric: numeric_strided_data_array

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: BEGIN_EXAMPLE("numeric_strided_data_array")
   :end-before:  END_EXAMPLE("numeric_strided_data_array")
   :language: cpp
   :dedent: 4

.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_strided_data_array")
   :end-before:  END_EXAMPLE("numeric_strided_data_array")


The DataAccessor class lets you write code to gracefully handle data
of varying type, such as user input data.  See the next section for
an example of DataAccessor use.


Using Introspection and Conversion
-----------------------------------

In this example, we have an array in a node that we are interested in
processing using an existing function that only handles doubles. We ensure
the node is compatible with the function, or transform it to a contiguous
double array. 


.. # from t_conduit_docs_tutorial_numeric: numeric_double_conversion

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: _conduit_tutorial_cpp_numeric_introspection_start
   :end-before:  _conduit_tutorial_cpp_numeric_introspection_end
   :language: cpp


.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_double_conversion")
   :end-before:  END_EXAMPLE("numeric_double_conversion")

Alternately, we can use a DataAccessor to do the conversion as needed.

.. # from t_conduit_docs_tutorial_numeric: numeric_data_accessor

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: BEGIN_EXAMPLE("numeric_data_accessor")
   :end-before:  END_EXAMPLE("numeric_data_accessor")
   :language: cpp

.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_data_accessor")
   :end-before:  END_EXAMPLE("numeric_data_accessor")

The first version examines the data type of the Node passed to
*process_doubles()* and converts the entire array if needed.  The worker
function *must_have_doubles_function()* works with a pointer, as is the case
with much existing code, giving speed of access with no guard-rails.  The second
version constructs a float64_accessor from its Node argument and passes that to
the worker function *takes_float64_accessor()*.  That worker function shows how
to use the summary methods of DataAccessor and then prints the values in the
array passed to it.  The DataAccessor casts each value as needed on access, thus
incurring a small cost at each access.  The DataAccessor is also safer and
simpler to use.

It is also possible to convert between DataAccessors and DataArrays using their
set methods. Below is an example of conversion from a DataArray to a DataAccessor.
It is possible to convert from a DataAccessor to a DataArray as well.

.. code-block:: c++

    int64 i_vals[4] = {100,200,300,400};

    Node n;
    n["ints"].set(i_vals,
                  2, // # of elements
                  0, // offset in bytes
                  sizeof(int64)*2); // stride in bytes

    int64_array vals_arr = n["ints"].value();
    n["floats"].set(DataType::float64(2));
    float64_accessor vals_acc = n["floats"].as_float64_accessor();
    vals_acc.set(vals_arr);
    
    std::cout << vals_acc[0] << std::endl;
    std::cout << vals_acc[1] << std::endl;

::

   100.0
   300.0


C++11 Initializer Lists 
-----------------------------------

You can set Node values using C++11 style initializer lists of numeric literals. 

.. literalinclude:: ../../tests/docs/t_conduit_docs_tutorial_numeric.cpp
   :start-after: BEGIN_EXAMPLE("numeric_cxx11_init")
   :end-before:  END_EXAMPLE("numeric_cxx11_init")
   :language: cpp
   :dedent: 4


.. literalinclude:: t_conduit_docs_tutorial_numeric_out.txt
   :start-after: BEGIN_EXAMPLE("numeric_cxx11_init")
   :end-before:  END_EXAMPLE("numeric_cxx11_init")

