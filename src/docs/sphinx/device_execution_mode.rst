Device Execution Model
Conduit is a library that provides an intuitive model for describing hierarchical scientific data. Conduit Blueprint is a set of methods for building, verifying, and transforming scientific datasets according to known conventions called protocols. In practice, this frequently involves representing 2D and 3D meshes along with corresponding topologies, coordsets, and field data.
Previously, these APIs were only CPU-accelerated. The Device Execution Model (DEM) is a new API that provides an abstraction layer (leveraging RAJA) for providing accelerator-agnostic implementations of these APIs. This allows Conduit to preserve existing CPU-accelerated behavior while enabling new GPU-accelerated workflows.
Goals
Modern high-performance computers are heterogenous, meaning they include multiple types of accelerators within a single node. The most common pairing is 1-2 CPUs to 4 GPUs. Simulation codes are increasingly GPU-accelerated, meaning that Conduit Blueprint data may originate in device memory. Previously, this data would always have to be copied to host memory before Blueprint APIs could operate on it. The goal of the DEM is to provide users with flexibility in where these Blueprint operations can be executed. Porting existing APIs to the DEM generally involves better parallelization, and in some cases, the introduction of parallelization where none existed previously. This has led to performance improvements in both CPU and GPU-accelerated workloads.
Getting Started
Conduit Nodes can hold data that ultimately lives in host or device memory. The DEM API provides the fundamental tools and operations for writing accelerator-agnostic transformations on this data. A minimal example might look like:
execution::init_device_memory_handlers();
const index_t n = 1024;
    const std::vector<float64> src_vals(n, 1.0);
    const std::vector<float64> des_vals(n, 0.0);
Node node;
        node["src"].set_allocator(node_alloc_id);
        node["des"].set_allocator(node_alloc_id);
        node["src"].set(src_vals);
        node["des"].set(des_vals);
float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);
acc_src.use_with(policy);
    acc_des.use_with(policy);
    index_t size = acc_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [acc_src, acc_des] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
Execution Options
Node exec_opts;
exec_opts["execution_location"].set(“device”);
exec_opts["output_location"].set(“device”);
exec_opts["sync_strategy"].set(“assume”);
execution::execution_set_options(exec_opts);
Execution Policies
ExecutionPolicy::device()
ExecutionPolicy::parallel()
ExecutionPolicy::host()
ExecutionPolicy::openmp()
ExecutionPolicy::hip()
ExecutionPolicy::cuda()
ExecutionPolicy::serial()
DEM APIs
forall(ExecutionPolicy &policy,
       const int& begin,
       const int& end,
       Kernel&& kernel)
sort_ascending(SerialExec,
               Iterator begin,
               Iterator end)
sort_descending(SerialExec,
                Iterator begin,
                Iterator end)
atomic_add(T *acc, T value)
atomic_min(T *acc, T value)
atomic_max(T *acc, T value)

Sync Strategies
Sync: working buffer is copied to a final destination, which can live in a different memory space.
Assume: the receiving node takes ownership of the working buffer, in whatever memory space was used for it.

API Port Status
Matrix of topology and coordset conversions. None of the generate transforms.
