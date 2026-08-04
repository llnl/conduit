.. # Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
.. # Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
.. # other details. No copyright assignment is required to contribute to Conduit.

.. _data_parallel_execution_model:

=============================
Data-Parallel Execution Model
=============================

.. note::
    The Data-Parallel Execution Model APIs and docs are under active development.

This page assumes familiarity with Conduit's core data model (see :doc:`conduit`) and with Conduit Blueprint (see :doc:`blueprint`, in particular the :ref:`Mesh Blueprint <mesh_blueprint>`).

The Conduit Blueprint mesh transform APIs, such as the coordset and topology conversions in ``conduit::blueprint::mesh``, previously only executed on the CPU. The Data-Parallel Execution Model is an API layer that provides tools for building implementations of these operations that are portable across host and device backends. It uses `RAJA <https://github.com/LLNL/RAJA>`_ for kernel dispatch and `Umpire <https://github.com/LLNL/Umpire>`_ for memory management when Conduit is built with them, and falls back to CPU-based implementations otherwise. Existing code keeps working as before, and the same APIs can now also run on the GPU.

The execution model lives in the ``conduit::execution`` namespace and is declared in ``conduit_execution.hpp`` and ``conduit_execution_policy.hpp``.

Goals
-----

Modern high-performance computers are heterogeneous, meaning they include multiple types of processors within a single node. The most common pairing is one or two CPUs alongside several GPUs. Simulation codes are increasingly GPU-accelerated, which means Conduit Blueprint data may originate in device memory. Previously, this data always had to be copied to host memory before the Blueprint mesh transform APIs in ``conduit::blueprint::mesh`` could operate on it. One example is ``mesh::convert`` and the underlying coordset and topology conversions it performs.

The goal of the execution model is to give users flexibility in where these Blueprint operations execute. Porting existing APIs to the execution model generally involves better parallelization, and in some cases the introduction of parallelization where none existed previously. This has led to performance improvements in both CPU- and GPU-based workloads.

Building with Support for Different Backends
--------------------------------------------

The execution model compiles into every Conduit build, but the available execution backends depend on build options and third-party libraries. For the best performance, we recommend building Conduit with RAJA, OpenMP, and a GPU backend (CUDA or HIP depending on architecture). The options below are environment variables understood by the ``scripts/build_conduit/build_conduit.sh`` helper script, which builds Conduit along with the needed third-party libraries. When configuring CMake directly, the corresponding options are ``RAJA_DIR``, ``ENABLE_OPENMP``, ``ENABLE_CUDA``, ``ENABLE_HIP``, and ``UMPIRE_DIR``.

* **RAJA** (``build_raja=true``) provides the kernel dispatch, sorting, and reduction backends used for device execution. Without RAJA, execution model operations use built-in serial and OpenMP implementations on the host.
* **OpenMP** (``enable_openmp=ON``) enables a multithreaded host backend. This is beneficial with or without RAJA, even if a GPU backend is enabled.
* **CUDA** (``enable_cuda=ON``) or **HIP** (``enable_hip=ON``) enable the GPU execution backends (only select one or the other, depending on architecture). Device execution and device memory allocation also require `Umpire <https://github.com/LLNL/Umpire>`_ (``build_umpire=true``).

With none of these, all execution model APIs still compile and run using a serial CPU-based fallback. At runtime, use the ``conduit::execution::ExecutionPolicy::is_*_enabled()`` static methods described below to query what a given build supports. For general build option details, see :doc:`building`.

Getting Started
---------------

Conduit Nodes can hold data that lives in host or device memory. The execution model provides the fundamental tools for writing transformations that operate on this data regardless of where it lives.

When Conduit is built with CUDA or HIP, call ``init_device_memory_handlers()`` once during setup. It initializes memory-space-aware copy and set handlers so that Node operations (``set()``, assignment, and copies between Nodes) work transparently across host and device memory:

.. code:: cpp

    conduit::execution::init_device_memory_handlers();

Below is a minimal example that doubles the values of an array on the device, starting from data that lives on the host. First we initialize the memory handlers, select device execution, and allocate the input and output leaf data with the host allocator, which is also the default:

.. code:: cpp

    #include "conduit.hpp"
    #include "conduit_execution.hpp"

    using namespace conduit;
    using conduit::execution::ExecutionPolicy;
    namespace execution = conduit::execution;

    // setup: install memory handlers and select device execution
    execution::init_device_memory_handlers();

    Node exec_opts;
    exec_opts["execution_location"] = "device";
    execution::execution_set_options(exec_opts);

    // this will be a device policy because we requested the
    // execution location to be the device
    ExecutionPolicy policy = execution::get_execution_policy();

    // allocate our node leaf data on the host
    const index_t n = 1024;
    const std::vector<float64> src_vals(n, 1.0);
    const std::vector<float64> des_vals(n, 0.0);

    Node node;
    node["src"].set_allocator(execution::get_host_allocator_id());
    node["des"].set_allocator(execution::get_host_allocator_id());
    node["src"].set(src_vals);
    node["des"].set(des_vals);

    // wrap the leaf data in accessors, execute with the selected policy,
    // and sync the results back to node["des"]
    run_data_accessor_policy_and_sync(node, policy);

The work itself happens in ``run_data_accessor_policy_and_sync()``, borrowed from Conduit's execution tests. It wraps the node leaf data in accessors, moves that data to the memory space of the policy, doubles each value, and copies the results back to where ``node["des"]`` started:

.. literalinclude:: ../../tests/conduit/execution_test_utils.hpp
   :start-after: _run_data_accessor_policy_and_sync_start
   :end-before:  _run_data_accessor_policy_and_sync_end
   :language: cpp

Because ``node["src"]`` and ``node["des"]`` start on the host and ``policy`` is a device policy, ``use_with(policy)`` moves the data: each accessor allocates a device-side working buffer and copies the leaf data into it. The kernel then reads and writes those device buffers, and ``acc_des.sync()`` copies the results back into the host buffer still owned by ``node["des"]``. If the node data had already been in the device memory space, both calls would have been no-ops. See `Moving Data Between Memory Spaces`_ for the full semantics.

Execution Options
-----------------

APIs that are aware of the execution model (for example, the ported :ref:`Mesh Blueprint <mesh_blueprint>` transforms) read a set of global execution options. Set them with ``execution_set_options()``, inspect them with ``execution_options()``, and restore the defaults with ``reset_execution_options()``:

.. code:: cpp

    Node exec_opts;
    exec_opts["execution_location"] = "device";
    exec_opts["output_location"]    = "device";
    exec_opts["sync_strategy"]      = "assume";
    conduit::execution::execution_set_options(exec_opts);

    Node cur_opts;
    conduit::execution::execution_options(cur_opts);
    cur_opts.print();

    conduit::execution::reset_execution_options();

The supported options are:

``execution_location``: ``"host"``, ``"device"``, or ``"input"`` (default: ``"input"``)
    Where operations that are aware of the execution model execute.
    ``"input"`` derives the location from the memory space of the input data.

``output_location``: ``"host"``, ``"device"``, ``"input"``, or an integer allocator id (default: ``"input"``)
    Which allocator is used for output data. ``"input"`` reuses the allocator
    of the input node; an integer value selects a specific Conduit allocator id.

``sync_strategy``: ``"sync"`` or ``"assume"`` (default: ``"assume"``)
    How results move back to the output node. ``"sync"`` copies the working
    buffer into the buffer the output node already owns; ``"assume"`` hands
    ownership of the working buffer to the output node.

``fallback_location``: ``"host"`` or ``"device"`` (default: ``"host"``)
    Where to execute and allocate when ``"input"`` is selected but there is
    no input node to reason about.

``execution_options()`` also reports three read-only fields: ``device_allocator`` and ``host_allocator`` hold the Conduit allocator ids used for device and host output, and ``user_provided_allocator`` holds the allocator id set through an integer ``output_location`` (``-1`` when unset).

The ``sync_strategy`` option controls the final step of a transform. Internally, transforms stage output through the ``use_with()`` / ``sync()`` / ``assume()`` methods described in `Moving Data Between Memory Spaces`_. With ``"sync"``, results are copied back into the output node's original memory space, which is convenient when a caller expects host-resident output. With ``"assume"`` (the default), the output node instead takes ownership of the working buffer in whatever memory space it occupies, which avoids a final copy but can change where the output node's data lives.

Helper methods resolve the current options, optionally against an input node:

.. code:: cpp

    namespace execution = conduit::execution;
    using execution::ExecutionPolicy;
    using execution::SyncStrategy;

    ExecutionPolicy policy     = execution::get_execution_policy(src_node);
    index_t         alloc_id   = execution::get_output_allocator_id(src_node);
    SyncStrategy    strategy   = execution::get_sync_strategy();
    index_t         dev_alloc  = execution::get_device_allocator_id();
    index_t         host_alloc = execution::get_host_allocator_id();

Execution Policies
------------------

A ``conduit::execution::ExecutionPolicy`` is a lightweight runtime value that selects where kernels run. The examples below assume ``using conduit::execution::ExecutionPolicy;``. Static factory methods create policies; the alias factories (``host()``, ``device()``, ``parallel()``) resolve to a concrete backend based on how Conduit was built:

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
     - No policy. Calls made with an empty policy raise an error.

Policies can also be constructed from a (string) name: ``ExecutionPolicy(<policy name>)`` accepts ``"empty"``, ``"serial"``, ``"cuda"``, ``"hip"``, ``"openmp"``, and the aliases ``"host"``, ``"device"``, and ``"parallel"``.

Query a policy with ``is_serial()``, ``is_cuda()``, ``is_hip()``, ``is_openmp()``, and ``is_empty()``, plus the aggregate checks ``is_host_policy()`` (serial or OpenMP), ``is_device_policy()`` (CUDA or HIP), and ``is_parallel_policy()``. ``policy_name()`` and ``policy_id()`` convert a policy to a string or enum value.

Query what a build supports with the static methods ``is_serial_enabled()``, ``is_cuda_enabled()``, ``is_hip_enabled()``, ``is_openmp_enabled()``, ``is_host_enabled()``, ``is_device_enabled()``, and ``is_parallel_enabled()``. The device backends are enabled only when Conduit is built with both CUDA/HIP and Umpire.

Requesting a backend that Conduit was not built with (for example, a CUDA policy without CUDA support) raises a ``conduit::Error``.

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
* ``CONDUIT_DEVICE_ERROR_CHECK(policy)`` reports CUDA/HIP errors from the last kernel launch by printing to ``stderr``. It does not raise a ``conduit::Error`` or abort. It is a no-op for host policies.

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

The available reducers are ``ReduceSum<T>`` (``+=``), ``ReduceMin<T>`` (``min()``), ``ReduceMax<T>`` (``max()``), and the value-plus-index variants ``ReduceMinLoc<T>`` and ``ReduceMaxLoc<T>`` (``minloc(value, index)`` / ``maxloc(value, index)``, with results from ``get()`` and ``getLoc()``).

Atomics
~~~~~~~

Atomic updates are safe to call from any kernel. Here ``vals_ptr`` points to memory that is valid in the policy's memory space:

.. code:: cpp

    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
    {
        conduit::execution::atomic_add(vals_ptr + i, i);
        conduit::execution::atomic_min(vals_ptr + i, static_cast<index_t>(-10));
        conduit::execution::atomic_max(vals_ptr + i, static_cast<index_t>(10));
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

Each atomic returns the value held at the given address before the update. For a complete example that also allocates the buffer and copies results back to the host, see ``run_test_atomics()`` in ``src/tests/conduit/t_conduit_execution.cpp``.

Sorting
~~~~~~~

``sort_ascending()`` and ``sort_descending()`` sort the range ``[begin, end)`` in place, dispatching to a parallel or device implementation based on the policy:

.. code:: cpp

    conduit::execution::sort_ascending(policy, begin, end);
    conduit::execution::sort_descending(policy, begin, end);

Moving Data Between Memory Spaces
---------------------------------

``DataAccessor`` (``float64_accessor``, ``index_t_accessor``, ...) and ``DataArray`` (``float64_array``, ``index_t_array``, ...) wrap node leaf data. Constructing one of these wrappers from a Node, as in ``float64_accessor acc_src(node["src"]);``, is what gives it a handle back to the owning Node. That handle is what enables the data movement methods below. A wrapper constructed from a bare pointer or from a temporary value has no owning Node, so these methods have nothing to move data to or from.

* ``use_with(policy)``: make the data accessible in the memory space of ``policy``. If the data already lives in that memory space, this is a no-op. Otherwise the wrapper allocates a *working buffer* in the policy's memory space, copies the node's elements into it (compacting stride and offset in the process), and points itself at that buffer. The Node's original buffer is untouched and still owned by the Node, so between ``use_with()`` and the next ``sync()`` or ``assume()`` the wrapper manages two memory spaces: the Node's original data on one side and the wrapper's working copy on the other. Calling ``use_with()`` again with a policy for the original memory space first syncs the working buffer back, then frees it and returns the wrapper to the Node's own buffer.
* ``sync()``: copy the contents of the working buffer back into the buffer the Node owns, reallocating the Node's data if the types or element counts no longer match. The Node keeps its original memory space, and the wrapper keeps its working buffer. A no-op if the data never moved.
* ``assume()``: hand the working buffer to the Node. The Node releases its original buffer and takes ownership of the working buffer where it currently lives, which means the Node's data may change memory spaces. This avoids the copy that ``sync()`` performs. A no-op if the data never moved.
* ``active_space()``: returns an ``ExecutionPolicy`` describing the memory space where the wrapper's data currently lives. This lets you execute wherever the input already resides:

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

Note that a policy passed explicitly to ``use_with()`` or ``forall()`` takes effect regardless of the global execution options; the options are only consulted by the ``get_*`` helpers and by the ported Blueprint transforms. The ``sync_strategy`` execution option described in `Execution Options`_ selects between ``sync()`` and ``assume()`` for the ported Blueprint transforms.

Host and Device Memory Managers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``conduit::execution`` namespace also provides low-level allocation interfaces:

* ``HostMemory``: host allocation and deallocation (Umpire-backed when Umpire is available, ``malloc``/``free`` otherwise).
* ``DeviceMemory``: device allocation and deallocation (requires Umpire), plus ``is_device_ptr()`` to test whether a pointer refers to device memory.
* ``MagicMemory``: ``copy()`` and ``set()`` methods that work across host and device memory spaces. These are backed by the handlers initialized by ``init_device_memory_handlers()``.

The allocator ids returned by ``get_host_allocator_id()`` and ``get_device_allocator_id()`` can be passed to ``Node::set_allocator()`` so that node leaf data is allocated directly in the desired memory space.

Blueprint Port Status
---------------------

The :ref:`Mesh Blueprint <mesh_blueprint>` ``mesh::convert`` API is in the process of being ported to the execution model. Currently, the mesh-to-mesh conversions (coordset and topology conversions) are supported. Additional transforms and internal operations are being ported over time. Until a transform is ported, it will continue to execute on the host exactly as before.
