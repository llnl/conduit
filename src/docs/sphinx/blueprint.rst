.. # Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
.. # Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
.. # other details. No copyright assignment is required to contribute to Conduit.

===================
Blueprint
===================

.. .. note::
..     The **blueprint** API and docs are work in progress.


The flexibility of the Conduit Node allows it to be used to represent a wide range of scientific data. Unconstrained, this flexibly can lead to many application specific choices for common types of data that could potentially be shared between applications.

The goal of Blueprint is to help facilite a set of shared higher-level conventions for using Conduit Nodes to hold common simulation data structures. The Blueprint library in Conduit provides methods to verify if a Conduit Node instance conforms to known conventions, which we call **protocols**. It also provides property and transform methods that can be used on conforming Nodes. 

For now, Blueprint is focused on conventions for two important types of data:

*  Computational Meshes (protocol: ``mesh``)

    Many taxonomies and concrete mesh data models have been developed to allow computational meshes to be used in software. Blueprint's conventions for representing mesh data were formed by negotiating with simulation application teams at LLNL and from a survey of existing projects that provide scientific mesh-related APIs including: ADIOS,  Damaris, EAVL, MFEM, Silo, VTK, VTKm, and Xdmf. Blueprint's mesh conventions are not a replacement for existing mesh data models or APIs. Our explicit goal is to outline a comprehensive, but small set of options for describing meshes in-core that simplifies the process of adapting data to several existing mesh-aware APIs.

*  One-to-Many Relations (protocol: ``o2mrelation``)

    A one-to-many relation is a collection of arbitrarily grouped values that encode element associations from a source ("one"s) to a destination ("many"s) space.
    These constructs are used in computational meshes to represent sparse material data, polygonal/polyhedral topologies, and other non-uniform mappings.

*  Multi-Component Arrays (protocol: ``mcarray``)

    A multi-component array is a collection of fixed-sized numeric tuples.
    They are used in the context computational meshes to represent coordinate data or field data, such as the three directional components of a 3D velocity field. There are a few common in-core data layouts used by several APIs to accept multi-component array data, these include:  row-major vs column-major layouts, or the use of arrays of struct vs struct of arrays in C-style languages. Blueprint provides transforms that convert any multi-component array to these common data layouts.

*  Tabular Data (protocol: ``table``)

    A collection of data represented as columns with the same number of rows.
    Generally used to serialize data in a flattened form, specifically to and from CSV files.

Blueprint mesh transforms are being ported to Conduit's :ref:`Data-Parallel Execution Model <data_parallel_execution_model>`, which allows them to execute on both host and device. The documentation contains information about which APIs have been ported or are in the process of being ported. 

.. toctree::
    blueprint_mesh
    blueprint_o2mrelation
    blueprint_mcarray
    blueprint_table
    blueprint_mesh_partition

Top Level Blueprint Interface
-------------------------------

Blueprint provides a generic top level ``verify()`` method, which exposes the verify checks for all supported protocols. 

.. code:: cpp

    bool conduit::blueprint::verify(const std::string &protocol,
                                    const Node &node,
                                    Node &info);

``verify()`` returns true if the passed Node *node* conforms to the named protocol. It also provides details about the verification, including specific errors in the passed *info* Node.

.. literalinclude:: ../../tests/docs/t_conduit_docs_blueprint_examples.cpp
   :start-after: BEGIN_EXAMPLE("blueprint_example_1")
   :end-before:  END_EXAMPLE("blueprint_example_1")
   :language: cpp
   :dedent: 4


.. literalinclude:: t_conduit_docs_blueprint_examples_out.txt
   :start-after: BEGIN_EXAMPLE("blueprint_example_1")
   :end-before:  END_EXAMPLE("blueprint_example_1")



Methods for specific protocols are grouped in namespaces:


.. literalinclude:: ../../tests/docs/t_conduit_docs_blueprint_examples.cpp
   :start-after: BEGIN_EXAMPLE("blueprint_example_2")
   :end-before:  END_EXAMPLE("blueprint_example_2")
   :language: cpp
   :dedent: 4

.. literalinclude:: t_conduit_docs_blueprint_examples_out.txt
   :start-after: BEGIN_EXAMPLE("blueprint_example_2")
   :end-before:  END_EXAMPLE("blueprint_example_2")




