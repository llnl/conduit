// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_benchmark.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BENCHMARK_HPP
#define CONDUIT_BENCHMARK_HPP

#include <limits>
#include <random>
#include <vector>

#include "conduit_annotations.hpp"
#include "conduit_data_type.hpp"
#include "conduit_execution_policy.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin benchmark --
//-----------------------------------------------------------------------------
namespace benchmark
{

using EP = execution::ExecutionPolicy;

//-----------------------------------------------------------------------------
enum class FillMode
{
    Sequential, // Fill with sequential values
    Random,     // Fill with random values
    None        // No-op for benchmarks that do not fill
};

//-----------------------------------------------------------------------------
// -- begin detail
//-----------------------------------------------------------------------------
namespace detail
{

//-----------------------------------------------------------------------------
template <typename T>
void
fill_sequential(void* ptr,
                index_t num_elements)
{
    T* typed = static_cast<T*>(ptr);
    for (index_t i = 0; i < num_elements; ++i)
    {
        typed[i] = static_cast<T>(i);
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void
fill_random(void* ptr,
            index_t num_elements)
{
    // Seed rng
    static std::mt19937_64 rng(std::random_device{}());
    T* typed = static_cast<T*>(ptr);

    if (std::is_floating_point<T>::value)
    {
        // Using double for floating-point types
        std::uniform_real_distribution<double> dist(
            static_cast<double>(std::numeric_limits<T>::lowest()),
            static_cast<double>(std::numeric_limits<T>::max()));
        for (index_t i = 0; i < num_elements; ++i)
        {
            typed[i] = static_cast<T>(dist(rng));
        }
    }
    else if (std::is_signed<T>::value)
    {
        // Using int64_t for signed integer types
        std::uniform_int_distribution<int64_t> dist(
            static_cast<int64_t>(std::numeric_limits<T>::min()),
            static_cast<int64_t>(std::numeric_limits<T>::max()));
        for (index_t i = 0; i < num_elements; ++i)
        {
            typed[i] = static_cast<T>(dist(rng));
        }
    }
    else // if (std::is_unsigned<T>::value)
    {
        // Using uint64_t for unsigned integer types
        std::uniform_int_distribution<uint64_t> dist(
            static_cast<uint64_t>(std::numeric_limits<T>::min()),
            static_cast<uint64_t>(std::numeric_limits<T>::max()));
        for (index_t i = 0; i < num_elements; ++i)
        {
            typed[i] = static_cast<T>(dist(rng));
        }
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void
fill(void* ptr,
     index_t num_elements,
     FillMode mode = FillMode::Sequential)
{
    if (FillMode::Sequential == mode)
    {
        fill_sequential<T>(ptr, num_elements);
    }
    else // if (FillMode::Random == mode)
    {
        fill_random<T>(ptr, num_elements);
    }
}

//-----------------------------------------------------------------------------
template <typename PolicyFn>
inline void
add_if_enabled(std::vector<EP>& policies,
               bool enabled,
               PolicyFn&& get_policy)
{
    // Don't try to add policies that are compile-time disabled
    if (!enabled)
    {
        return;
    }

    EP policy = get_policy();
    const auto id = policy.policy_id();

    // Check if this policy is already in the vector
    auto it = std::find_if(
        policies.begin(),
        policies.end(),
        [id](const EP& p)
        {
            return p.policy_id() == id;
        });

    // If the policy is not already in the vector, add it
    if (it == policies.end())
    {
        policies.push_back(policy);
    }
}
}
//-----------------------------------------------------------------------------
// -- end detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Maps a compile-time type T to its corresponding conduit::DataType
/// TODO: I'd like to find a nicer way of doing this
template <typename T> inline DataType dtype_of();

template <> inline DataType dtype_of<int8>()    { return DataType::int8(); }
template <> inline DataType dtype_of<int16>()   { return DataType::int16(); }
template <> inline DataType dtype_of<int32>()   { return DataType::int32(); }
template <> inline DataType dtype_of<int64>()   { return DataType::int64(); }
template <> inline DataType dtype_of<uint8>()   { return DataType::uint8(); }
template <> inline DataType dtype_of<uint16>()  { return DataType::uint16(); }
template <> inline DataType dtype_of<uint32>()  { return DataType::uint32(); }
template <> inline DataType dtype_of<uint64>()  { return DataType::uint64(); }
template <> inline DataType dtype_of<float32>() { return DataType::float32(); }
template <> inline DataType dtype_of<float64>() { return DataType::float64(); }

//-----------------------------------------------------------------------------
template <typename T>
inline std::string
get_annotated_name(const std::string& name, FillMode mode)
{
    if (FillMode::Sequential == mode)
    {
        return name + "_" + dtype_of<T>().name() + "_sequential";
    }
    else // if (FillMode::Random == mode)
    {
        return name + "_" + dtype_of<T>().name() + "_random";
    }
}

//-----------------------------------------------------------------------------
inline void
make_data(const DataType& dtype,
          index_t num_elements,
          std::vector<uint8_t>& out,
          FillMode mode = FillMode::Sequential)
{
    out.resize(static_cast<size_t>(num_elements * dtype.element_bytes()));
    void* ptr = out.data();

    // Dispatch on DataType
    switch (dtype.id())
    {
        case DataType::INT8_ID:
            detail::fill<int8>(ptr, num_elements, mode);
            break;
        case DataType::INT16_ID:
            detail::fill<int16>(ptr, num_elements, mode);
            break;
        case DataType::INT32_ID:
            detail::fill<int32>(ptr, num_elements, mode);
            break;
        case DataType::INT64_ID:
            detail::fill<int64>(ptr, num_elements, mode);
            break;
        case DataType::UINT8_ID:
            detail::fill<uint8>(ptr, num_elements, mode);
            break;
        case DataType::UINT16_ID:
            detail::fill<uint16>(ptr, num_elements, mode);
            break;
        case DataType::UINT32_ID:
            detail::fill<uint32>(ptr, num_elements, mode);
            break;
        case DataType::UINT64_ID:
            detail::fill<uint64>(ptr, num_elements, mode);
            break;
        case DataType::FLOAT32_ID:
            detail::fill<float32>(ptr, num_elements, mode);
            break;
        case DataType::FLOAT64_ID:
            detail::fill<float64>(ptr, num_elements, mode);
            break;
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
// TODO: This might be nice as a public helper in the execution:: API
inline std::vector<EP>
get_enabled_policies()
{
    std::vector<EP> policies;
    policies.reserve(7);

    // Since asking for compile-time disabled policies will throw an error
    // (and subsequently cause our benchmark "tests" to fail), we have to
    // do extra work to avoid asking for disabled policies
    detail::add_if_enabled(policies, EP::is_serial_enabled(),   [] { return EP::serial(); });
    detail::add_if_enabled(policies, EP::is_host_enabled(),     [] { return EP::host(); });
    detail::add_if_enabled(policies, EP::is_openmp_enabled(),   [] { return EP::openmp(); });
    detail::add_if_enabled(policies, EP::is_parallel_enabled(), [] { return EP::parallel(); });
    detail::add_if_enabled(policies, EP::is_device_enabled(),   [] { return EP::device(); });
    detail::add_if_enabled(policies, EP::is_cuda_enabled(),     [] { return EP::cuda(); });
    detail::add_if_enabled(policies, EP::is_hip_enabled(),      [] { return EP::hip(); });

    return policies;
}

// TODO: I think the current semantics are okay, but I want to investigate if
// something like conduit::benchmark::(func) is possible/would be nicer
template <typename BenchmarkFn>
void
exec(BenchmarkFn&& fn,
     const index_t warmup,
     const index_t iterations,
     std::initializer_list<FillMode> modes = {FillMode::Sequential, FillMode::Random})
{
    // Setup
    execution::init_device_memory_handlers();
    auto policies = conduit::benchmark::get_enabled_policies();

    // TODO: Loop over host vs device
    
    for (const auto& policy : policies)
    {
        for (const FillMode mode : modes)
        {
            // Warm-up
            for (index_t i = 0; i < warmup; i++)
            {
                fn(policy, mode);
            }

            std::cout << "\n" << policy.policy_name() << "," << warmup << "," << iterations << std::endl;
            
            // TODO: Do we care to have bespoke timing in the case that conduit was built
            // without caliper?
            Node cali_opts;
            cali_opts["config"] = "runtime-report";
            annotations::initialize(cali_opts);

            // Benchmark
            for (index_t i = 0; i < iterations; i++)
            {
                fn(policy, mode);
            }

            annotations::finalize();
        }
    }
}

}
//-----------------------------------------------------------------------------
// -- end benchmark:: --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif // CONDUIT_BENCHMARK_HPP
