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

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "conduit_annotations.hpp"
#include "conduit_data_type.hpp"

// This is not needed after device support is added back
#if defined(CONDUIT_USE_OPENMP)
#include <omp.h>
#endif

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

//-----------------------------------------------------------------------------
inline
std::string
get_timestamp()
{
    std::time_t t = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S");
    return oss.str();
}

//-----------------------------------------------------------------------------
struct ExecConfig
{
    std::string src_location;
    std::string exec_location;
    std::string output_location;
};

//-----------------------------------------------------------------------------
inline
std::vector<ExecConfig>
get_exec_configs()
{
    // In the pre-device execution model world, we don't have the concept of
    // host vs device execution.
    std::vector<ExecConfig> configs{
        //source location, execution location, output location
        {"host",           "host",             "host"},
    };
    return configs;
}

//-----------------------------------------------------------------------------
template <typename RunFn, typename SizeFn>
void
exec(const std::string &name,
     const Node &input,
     Node &output,
     RunFn &&run,
     SizeFn &&size_info,
     const ExecConfig &config,
     const index_t npts,
     const index_t warmup,
     const index_t iterations)
{
    // Capture input/output data sizes to include in the scope name below.
    // This is a function of (name, npts) and never changes between
    // iterations.
    run(input, output);
    const std::string sizes = size_info(input, output);
    output.reset();

    // Execute `run` `warmup` times
    {
        // Scope this separately to make it easier to disregard warmup
        // iterations in the timing output.
        CONDUIT_ANNOTATE_MARK_SCOPE("warmup");
        for (index_t i = 0; i < warmup; i++)
        {
            run(input, output);
            output.reset();
        }
    }

    // Execute `run` `iterations` times
    {
        // This scope name is used to identify specific benchmarks in the
        // Caliper output and identify their attributes.
        const std::string scope_name = name
            + "_dim-"  + std::to_string(npts)
            + "_" + sizes
            + "_src-"  + config.src_location
            + "_exec-" + config.exec_location
            + "_out-"  + config.output_location
#if defined(CONDUIT_USE_OPENMP)
            + "_threads-" + std::to_string(omp_get_max_threads())
#endif
            + "_iter-" + std::to_string(iterations);

        for (index_t i = 0; i < iterations; i++)
        {
            CONDUIT_ANNOTATE_MARK_BEGIN(scope_name.c_str());
            run(input, output);
            CONDUIT_ANNOTATE_MARK_END(scope_name.c_str());
            output.reset();
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
