.. # Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
.. # Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
.. # other details. No copyright assignment is required to contribute to Conduit.

.. _device_execution_model:

======================
Device Execution Model
======================

.. note::
    The Device Execution Model APIs and docs are under active development.

Conduit is a library that provides an intuitive model for describing hierarchical scientific data. Conduit Blueprint is a set of methods for building, verifying, and transforming scientific datasets according to known conventions called protocols. In practice, this frequently involves representing 2D and 3D meshes along with corresponding topologies, coordsets, and field data.

Previously, these APIs only executed on the CPU. The Device Execution Model (DEM) is an API layer that provides accelerator-agnostic implementations of these operations. It leverages `RAJA <https://github.com/LLNL/RAJA>`_ when Conduit is built with RAJA support and provides built-in host fallbacks otherwise. This allows Conduit to preserve existing CPU behavior while enabling new GPU-accelerated workflows through the same code paths.

The DEM lives in the ``conduit::execution`` namespace and is declared in ``conduit_execution.hpp`` and ``conduit_execution_policy.hpp``.

Goals
-----

Modern high-performance computers are heterogeneous, meaning they include multiple types of processors within a single node. The most common pairing is one or two CPUs alongside several GPUs. Simulation codes are increasingly GPU-accelerated, which means Conduit Blueprint data may originate in device memory. Previously, this data always had to be copied to host memory before Blueprint APIs could operate on it. The goal of the DEM is to give users flexibility in where these Blueprint operations execute. Porting existing APIs to the DEM generally involves better parallelization, and in some cases the introduction of parallelization where none existed previously. This has led to performance improvements in both CPU- and GPU-accelerated workloads.

Building with Device Support
----------------------------

The DEM compiles into every Conduit build, but the available execution backends depend on build options and third-party libraries:

* **CUDA** (``ENABLE_CUDA=ON``) or **HIP** (``ENABLE_HIP=ON``) enable the GPU execution backends. Device execution and device memory allocation also require `Umpire <https://github.com/LLNL/Umpire>`_ (``UMPIRE_DIR``).
* **RAJA** (``RAJA_DIR``) provides the kernel dispatch, sorting, and reduction backends used for device execution. Without RAJA, DEM operations use built-in serial and OpenMP implementations on the host.
* **OpenMP** (``ENABLE_OPENMP=ON``) enables a multithreaded host backend.

With none of these, all DEM APIs still compile and run using a serial host fallback. At runtime, use the ``ExecutionPolicy::is_*_enabled()`` static methods described below to query what a given build supports. For general build option details, see :doc:`building`.

Getting Started
---------------

Conduit Nodes can hold data that lives in host or device memory. The DEM provides the fundamental tools for writing accelerator-agnostic transformations on this data.

When Conduit is built with CUDA or HIP, call ``init_device_memory_handlers()`` once during setup. It installs memory-space-aware copy and set handlers so that Node operations (``set()``, assignment, and copies between Nodes) work transparently across host and device memory:

.. code:: cpp

    conduit::execution::init_device_memory_handlers();

A minimal example that doubles the values of an array using device execution:

.. code:: cpp

    #include "conduit.hpp"
    #include "conduit_execution.hpp"

    using namespace conduit;
    using conduit::execution::ExecutionPolicy;

    // setup: install memory handlers and select device execution
    execution::init_device_memory_handlers();

    Node exec_opts;
    exec_opts["execution_location"] = "device";
    exec_opts["output_location"]    = "device";
    execution::execution_set_options(exec_opts);

    ExecutionPolicy policy   = execution::get_execution_policy();
    const index_t   alloc_id = execution::get_output_allocator_id();

    // allocate node leaf data in the output memory space
    const index_t n = 1024;
    const std::vector<float64> src_vals(n, 1.0);
    const std::vector<float64> des_vals(n, 0.0);

    Node node;
    node["src"].set_allocator(alloc_id);
    node["des"].set_allocator(alloc_id);
    node["src"].set(src_vals);
    node["des"].set(des_vals);

    // accessors move data to the policy's memory space if needed
    float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);
    acc_src.use_with(policy);
    acc_des.use_with(policy);

    // launch the kernel; CONDUIT_EXEC marks it for host and device
    index_t size = acc_src.number_of_elements();
    conduit::execution::forall(policy, 0, size,
        [acc_src, acc_des] CONDUIT_EXEC(index_t idx)
        {
            const float64 val = 2.0 * acc_src[idx];
            acc_des.set(idx, val);
        });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // copy the results back to node["des"]'s original memory space
    // if different than the execution memory space
    acc_des.sync();

Execution Options
-----------------

DEM-aware APIs (for example, the ported :ref:`Mesh Blueprint <mesh_blueprint>` transforms) read a set of global execution options. Set them with ``execution_set_options()``, inspect them with ``execution_options()``, and restore the defaults with ``reset_execution_options()``:

.. code:: cpp

    Node exec_opts;
    exec_opts["execution_location"] = "device";
    exec_opts["output_location"]    = "device";
    exec_opts["sync_strategy"]      = "assume";
    conduit::execution::execution_set_options(exec_opts);

    Node cur_opts;
    conduit::execution::execution_options(cur_opts);

    conduit::execution::reset_execution_options();

The supported options are:

.. list-table::
   :header-rows: 1

   * - Option
     - Values
     - Default
     - Description

   * - ``execution_location``
     - ``"host"``, ``"device"``, ``"input"``
     - ``"input"``
     - Where DEM-aware operations execute. ``"input"`` derives the location from the memory space of the input data.

   * - ``output_location``
     - ``"host"``, ``"device"``, ``"input"``, or an integer allocator id
     - ``"input"``
     - Which allocator is used for output data. ``"input"`` reuses the allocator of the input node; an integer value selects a specific Conduit allocator id.

   * - ``sync_strategy``
     - ``"sync"``, ``"assume"``
     - ``"assume"``
     - How results move back to the output node. See `Sync Strategies`_.

   * - ``fallback_location``
     - ``"host"``, ``"device"``
     - ``"host"``
     - Where to execute and allocate when ``"input"`` is selected but there is no input node to reason about.

Helper methods resolve the current options, optionally against an input node:

.. code:: cpp

    namespace execution = conduit::execution;

    ExecutionPolicy         policy     = execution::get_execution_policy(src_node);
    index_t                 alloc_id   = execution::get_output_allocator_id(src_node);
    execution::SyncStrategy strategy   = execution::get_sync_strategy();
    index_t                 dev_alloc  = execution::get_device_allocator_id();
    index_t                 host_alloc = execution::get_host_allocator_id();

Execution Policies
------------------

An ``ExecutionPolicy`` is a lightweight runtime value that selects where kernels run. Static factory methods create policies; the alias factories (``host()``, ``device()``, ``parallel()``) resolve to a concrete backend based on how Conduit was built:

.. list-table::
   :header-rows: 1

   * - Factory
     - Resolves to

   * - ``ExecutionPolicy::serial()``
     - Serial CPU execution (always available).

   * - ``ExecutionPolicy::openmp()``
     - OpenMP CPU execution. Errors if Conduit was built without OpenMP.

   * - ``ExecutionPolicy::cuda()``
     - CUDA GPU execution. Errors if Conduit was built without CUDA.

   * - ``ExecutionPolicy::hip()``
     - HIP GPU execution. Errors if Conduit was built without HIP.

   * - ``ExecutionPolicy::host()``
     - ``openmp()`` when available, otherwise ``serial()``.

   * - ``ExecutionPolicy::device()``
     - ``cuda()`` or ``hip()``, depending on the build. Errors with neither.

   * - ``ExecutionPolicy::parallel()``
     - First available of ``cuda()``, ``hip()``, ``openmp()``, then ``serial()``.

   * - ``ExecutionPolicy::empty()``
     - No policy. DEM calls made with an empty policy raise an error.

Policies can also be constructed from a name: ``ExecutionPolicy("device")`` accepts ``"empty"``, ``"serial"``, ``"cuda"``, ``"hip"``, ``"openmp"``, and the aliases ``"host"``, ``"device"``, and ``"parallel"``.

Query a policy with ``is_serial()``, ``is_cuda()``, ``is_hip()``, ``is_openmp()``, and ``is_empty()``, plus the aggregate checks ``is_host_policy()`` (serial or OpenMP), ``is_device_policy()`` (CUDA or HIP), and ``is_parallel_policy()``. ``policy_name()`` and ``policy_id()`` convert a policy to a string or enum value.

Query what a build supports with the static methods ``is_serial_enabled()``, ``is_cuda_enabled()``, ``is_hip_enabled()``, ``is_openmp_enabled()``, ``is_host_enabled()``, ``is_device_enabled()``, and ``is_parallel_enabled()``. The device backends are enabled only when Conduit is built with both CUDA/HIP and Umpire.

Launching Kernels
-----------------

``forall()`` runs a kernel over the index range ``[begin, end)`` using a runtime policy:

.. code:: cpp

    conduit::execution::forall(policy, 0, size,
        [=] CONDUIT_EXEC(index_t i)
        {
            // kernel body
        });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

Two macros make kernels portable:

* ``CONDUIT_EXEC`` marks a function or lambda so it compiles for both host and device in CUDA/HIP translation units. In host-only builds it expands to nothing.
* ``CONDUIT_DEVICE_ERROR_CHECK(policy)`` reports CUDA/HIP errors from the last kernel launch. It is a no-op for host policies.

Requesting a backend that Conduit was not built with (for example, a CUDA policy without CUDA support) raises a ``conduit::Error``.

Reductions
~~~~~~~~~~

Reducers accumulate values across kernel iterations and work with any policy:

.. code:: cpp

    conduit::execution::ReduceSum<index_t>    sum(0);
    conduit::execution::ReduceMinLoc<float64> minloc(std::numeric_limits<float64>::max(), -1);

    conduit::execution::forall(policy, 0, size,
        [=] CONDUIT_EXEC(index_t i)
        {
            sum += vals[i];
            minloc.minloc(vals[i], i);
        });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    index_t total   = sum.get();
    float64 min_val = minloc.get();
    index_t min_idx = minloc.getLoc();

The available reducers are ``ReduceSum<T>`` (``+=`` or ``sum()``), ``ReduceMin<T>`` (``min()``), ``ReduceMax<T>`` (``max()``), and the value-plus-index variants ``ReduceMinLoc<T>`` and ``ReduceMaxLoc<T>`` (``minloc(value, index)`` / ``maxloc(value, index)``, with results from ``get()`` and ``getLoc()``).

Atomics
~~~~~~~

Atomic updates are safe to call from any kernel:

.. code:: cpp

    conduit::execution::atomic_add(&acc, value);
    conduit::execution::atomic_min(&acc, value);
    conduit::execution::atomic_max(&acc, value);

Each returns the value held at ``acc`` before the update.

Sorting
~~~~~~~

``sort_ascending()`` and ``sort_descending()`` sort the range ``[begin, end)`` in place, dispatching to a parallel or device implementation based on the policy:

.. code:: cpp

    conduit::execution::sort_ascending(policy, begin, end);
    conduit::execution::sort_descending(policy, begin, end);

Moving Data Between Memory Spaces
---------------------------------

``DataAccessor`` (``float64_accessor``, ``index_t_accessor``, ...) and ``DataArray`` (``float64_array``, ``index_t_array``, ...) wrappers expose the data movement methods used to make node leaf data available to kernels:

* ``use_with(policy)``: make the data accessible in the memory space of ``policy``, moving it if it is not already there.
* ``sync()``: copy the working buffer back to the node's original memory space. A no-op if the data never moved.
* ``assume()``: the node takes ownership of the working buffer in whatever memory space it currently occupies.
* ``active_space()``: returns an ``ExecutionPolicy`` describing the memory space where the data currently lives. This lets you execute wherever the input already resides:

.. code:: cpp

    float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);

    // execute where node["src"] lives
    ExecutionPolicy policy = acc_src.active_space();
    acc_src.use_with(policy);
    acc_des.use_with(policy);

    conduit::execution::forall(policy, 0, acc_src.number_of_elements(),
        [acc_src, acc_des] CONDUIT_EXEC(index_t idx)
        {
            acc_des.set(idx, 2.0 * acc_src[idx]);
        });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    acc_des.sync();

Sync Strategies
~~~~~~~~~~~~~~~

DEM-aware transforms stage output in a working buffer sized for the execution memory space, then reconcile it with the output node according to the global ``sync_strategy`` option:

* ``"sync"``: the working buffer is copied to the final destination, which can live in a different memory space (for example, back to the host).
* ``"assume"`` (default): the output node takes ownership of the working buffer in whatever memory space it occupies, avoiding a final copy.

Host and Device Memory Managers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``conduit::execution`` namespace also provides low-level allocation interfaces:

* ``HostMemory``: host allocation and deallocation (Umpire-backed when Umpire is available, ``malloc``/``free`` otherwise).
* ``DeviceMemory``: device allocation and deallocation (requires Umpire), plus ``is_device_ptr()`` to test whether a pointer refers to device memory.
* ``MagicMemory``: ``copy()`` and ``set()`` methods that work across host and device memory spaces; these back the handlers installed by ``init_device_memory_handlers()``.

The allocator ids returned by ``get_host_allocator_id()`` and ``get_device_allocator_id()`` can be passed to ``Node::set_allocator()`` so that node leaf data is allocated directly in the desired memory space.

Blueprint Port Status
---------------------

The DEM execution options are honored by the :ref:`Mesh Blueprint <mesh_blueprint>` coordset conversions (``coordset::uniform::to_rectilinear``, ``coordset::uniform::to_explicit``, and ``coordset::rectilinear::to_explicit``) and by the topology conversions that build on them (for example, ``topology::uniform::to_rectilinear`` and ``topology::rectilinear::to_structured``). Other Blueprint components use DEM kernels internally on the host, and additional transforms are being ported over time. Until a transform is ported, it continues to execute on the host exactly as before.
