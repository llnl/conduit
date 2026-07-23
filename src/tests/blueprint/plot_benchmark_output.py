# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

import argparse
import csv
import math
import re
import sys
from copy import copy
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import thicket as th
from caliperreader.readererror import ReaderError
from matplotlib.colors import LogNorm
from matplotlib.ticker import FuncFormatter


MESH_BENCHMARKS = [
    ("mesh_uniform_to_rectilinear", "uniform → rectilinear"),
    ("mesh_uniform_to_structured", "uniform → structured"),
    ("mesh_uniform_to_unstructured", "uniform → unstructured"),
    ("mesh_rectilinear_to_structured", "rectilinear → structured"),
    ("mesh_rectilinear_to_unstructured", "rectilinear → unstructured"),
    ("mesh_structured_to_unstructured", "structured → unstructured"),
]

MESH_LABELS = dict(MESH_BENCHMARKS)

UNSTRUCTURED_OPERATIONS = [
    ("to_polytopal", "to polytopal"),
    ("generate_points", "generate points"),
    ("generate_lines", "generate lines"),
    ("generate_faces", "generate faces"),
    ("generate_centroids", "generate centroids"),
    ("generate_sides", "generate sides"),
    ("generate_corners", "generate corners"),
    ("generate_offsets", "generate offsets"),
    ("generate_offsets_inline", "generate offsets inline"),
]

OPERATION_LABELS = dict(UNSTRUCTURED_OPERATIONS)
OPERATION_ORDER = {
    operation: index
    for index, (operation, _) in enumerate(UNSTRUCTURED_OPERATIONS)
}

PREFERRED_ELEMENT_TYPE_ORDER = [
    "tris",
    "quads",
    "tets",
    "hexs",
    "wedges",
    "pyramids",
    "mixed_2d",
    "mixed",
]

ELEMENT_TYPE_NDIMS = {
    "tris": 2,
    "quads": 2,
    "polygonal": 2,
    "mixed_2d": 2,
    "tets": 3,
    "hexs": 3,
    "wedges": 3,
    "pyramids": 3,
    "polyhedral": 3,
    "mixed": 3,
}

MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*"]

CONFIG_NAME_RE = re.compile(
    r"^(?P<benchmark>(?:mesh|unstructured)_.+?)"
    r"_dim-(?P<dim>\d+)"
    r"_(?P<metadata>.+)$"
)

TIMESTAMP_RE = re.compile(r"^\d{8}_\d{6}$")

REQUIRED_METADATA_FIELDS = {"src", "exec", "out", "iter"}
OPTIONAL_METADATA_FIELDS = {"backend", "sync", "threads"}
ALLOWED_METADATA_FIELDS = (
    REQUIRED_METADATA_FIELDS | OPTIONAL_METADATA_FIELDS
)


@dataclass(frozen=True)
class Config:
    backend: Optional[str]
    src: str
    execution: str
    out: str
    sync: Optional[str]
    threads: Optional[int]


@dataclass(frozen=True)
class BenchmarkInfo:
    name: str
    family: str
    label: str
    operation: Optional[str]
    element_type: Optional[str]
    ndims: Optional[int]


@dataclass(frozen=True)
class BenchmarkResult:
    benchmark: str
    config: Config
    dim: int
    iterations: int
    total_seconds: float
    seconds_per_iteration: float


def parse_config_name(name):
    match = CONFIG_NAME_RE.match(name)
    if match is None:
        return None

    benchmark = match.group("benchmark")
    dim = int(match.group("dim"))

    values = {}
    for field in match.group("metadata").split("_"):
        if "-" not in field:
            return None

        key, value = field.split("-", 1)
        if not key or not value or key in values:
            return None

        values[key] = value

    if not REQUIRED_METADATA_FIELDS <= values.keys():
        return None

    if values.keys() - ALLOWED_METADATA_FIELDS:
        return None

    if values["src"] not in {"host", "device"}:
        return None

    if values["exec"] not in {"host", "device"}:
        return None

    if values["out"] not in {"host", "device"}:
        return None

    if not values["iter"].isdigit():
        return None

    iterations = int(values["iter"])
    if dim <= 1 or iterations <= 0:
        return None

    threads = None
    if "threads" in values:
        if not values["threads"].isdigit():
            return None

        threads = int(values["threads"])
        if threads <= 0:
            return None

    config = Config(
        backend=values.get("backend"),
        src=values["src"],
        execution=values["exec"],
        out=values["out"],
        sync=values.get("sync"),
        threads=threads,
    )

    return benchmark, config, dim, iterations


def parse_unstructured_benchmark(name):
    # Longer operation names must be checked first so generate_offsets_inline
    # is not interpreted as generate_offsets plus an inline element type.
    operations = sorted(
        OPERATION_LABELS,
        key=len,
        reverse=True,
    )

    for operation in operations:
        prefix = f"unstructured_{operation}_"
        if name.startswith(prefix):
            element_type = name[len(prefix):]
            if element_type:
                return operation, element_type

    return None


def classify_benchmark(name):
    if name.startswith("mesh_"):
        return BenchmarkInfo(
            name=name,
            family="mesh",
            label=MESH_LABELS.get(name, name.replace("_", " ")),
            operation=name.removeprefix("mesh_"),
            element_type=None,
            ndims=3,
        )

    parsed = parse_unstructured_benchmark(name)
    if parsed is not None:
        operation, element_type = parsed
        return BenchmarkInfo(
            name=name,
            family="unstructured",
            label=OPERATION_LABELS[operation],
            operation=operation,
            element_type=element_type,
            ndims=ELEMENT_TYPE_NDIMS.get(element_type),
        )

    return BenchmarkInfo(
        name=name,
        family="other",
        label=name.replace("_", " "),
        operation=None,
        element_type=None,
        ndims=None,
    )


def config_sort_key(config):
    return (
        config.backend or "",
        config.src,
        config.execution,
        config.out,
        config.sync or "",
        config.threads if config.threads is not None else -1,
    )


def format_config(config):
    parts = []

    if config.backend:
        parts.append(config.backend)

    parts.append(
        f"{config.src} → {config.execution} → {config.out}"
    )

    if config.sync:
        parts.append(f"sync={config.sync}")

    if config.threads is not None:
        parts.append(f"threads={config.threads}")

    return ", ".join(parts)


def config_slug(config):
    parts = []

    if config.backend:
        parts.append(config.backend)

    parts.extend([
        config.src,
        config.execution,
        config.out,
    ])

    if config.sync:
        parts.append(config.sync)

    if config.threads is not None:
        parts.append(f"threads-{config.threads}")

    return safe_file_part("_".join(parts))


def safe_file_part(value):
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", value)


def ordered_element_types(infos):
    discovered = {
        info.element_type
        for info in infos.values()
        if info.element_type is not None
    }

    ordered = [
        element_type
        for element_type in PREFERRED_ELEMENT_TYPE_ORDER
        if element_type in discovered
    ]

    ordered.extend(
        sorted(discovered - set(PREFERRED_ELEMENT_TYPE_ORDER))
    )

    return ordered


def find_cali_file():
    candidates = [
        path
        for path in Path.cwd().glob("*.cali")
        if TIMESTAMP_RE.match(path.stem)
    ]
    candidates.sort(reverse=True)

    if not candidates:
        sys.exit(
            "no timestamped .cali files "
            f"(YYYYMMDD_HHMMSS.cali) found in {Path.cwd()}"
        )

    print(f"using most recent Caliper file: {candidates[0]}")
    return candidates[0]


def find_time_column(dataframe):
    preferred = (
        "time",
        "sum#sum#time.duration",
        "sum#time.duration",
        "time.duration",
    )

    for candidate in preferred:
        if candidate in dataframe.columns:
            return candidate

    for column in dataframe.columns:
        if str(column).endswith("time.duration"):
            return column

    raise RuntimeError(
        "unable to find a Caliper time column; available columns are: "
        + ", ".join(str(column) for column in dataframe.columns)
    )


def iter_graph_nodes(graph):
    stack = list(graph.roots)
    visited = set()

    while stack:
        node = stack.pop()
        node_id = id(node)

        if node_id in visited:
            continue

        visited.add(node_id)
        yield node
        stack.extend(node.children)


def node_time(thicket, profile_id, node, time_column):
    try:
        value = thicket.dataframe.loc[
            (node, profile_id),
            time_column,
        ]
    except KeyError:
        return float("nan")

    try:
        return float(np.asarray(value).reshape(-1)[0])
    except (TypeError, ValueError, IndexError):
        return float("nan")


def inclusive_time(
    thicket,
    profile_id,
    node,
    time_column,
    cache,
):
    node_id = id(node)
    if node_id in cache:
        return cache[node_id]

    value = node_time(
        thicket,
        profile_id,
        node,
        time_column,
    )
    total = 0.0 if np.isnan(value) else value

    for child in node.children:
        total += inclusive_time(
            thicket,
            profile_id,
            child,
            time_column,
            cache,
        )

    cache[node_id] = total
    return total


def collect_results(thicket):
    profile_ids = list(
        thicket.dataframe.index
        .get_level_values("profile")
        .unique()
    )

    if len(profile_ids) != 1:
        raise RuntimeError(
            "expected exactly one profile in the Caliper file, "
            f"found {len(profile_ids)}"
        )

    profile_id = profile_ids[0]
    time_column = find_time_column(thicket.dataframe)
    time_cache = {}

    results = []
    seen = set()
    malformed_names = []

    for node in iter_graph_nodes(thicket.graph):
        name = node.frame.get("name", "")
        parsed = parse_config_name(name)

        if parsed is None:
            if (
                isinstance(name, str)
                and name.startswith(("mesh_", "unstructured_"))
                and "_dim-" in name
            ):
                malformed_names.append(name)
            continue

        benchmark, config, dim, iterations = parsed
        key = (benchmark, config, dim)

        if key in seen:
            raise RuntimeError(
                "duplicate benchmark configuration encountered: "
                f"{name}"
            )

        seen.add(key)

        total = inclusive_time(
            thicket,
            profile_id,
            node,
            time_column,
            time_cache,
        )

        results.append(
            BenchmarkResult(
                benchmark=benchmark,
                config=config,
                dim=dim,
                iterations=iterations,
                total_seconds=total,
                seconds_per_iteration=total / iterations,
            )
        )

    if malformed_names:
        print(
            "warning: ignored benchmark-like regions with unsupported "
            "names:"
        )
        for name in sorted(set(malformed_names)):
            print(f"  {name}")

    return results, time_column


def base_grid_points(info, dim):
    if info.ndims is None:
        return None
    return dim ** info.ndims


def base_grid_cells(info, dim):
    if info.ndims is None:
        return None
    return (dim - 1) ** info.ndims


def format_duration(seconds):
    if not np.isfinite(seconds):
        return "n/a"

    seconds = abs(seconds)

    if seconds == 0:
        return "0"

    if seconds < 1.0e-6:
        return f"{seconds * 1.0e9:.3g} ns"

    if seconds < 1.0e-3:
        return f"{seconds * 1.0e6:.3g} µs"

    if seconds < 1.0:
        return f"{seconds * 1.0e3:.3g} ms"

    return f"{seconds:.3g} s"


def format_rate(rate):
    if not np.isfinite(rate):
        return "n/a"
    return f"{rate:.3g}"


def iteration_summary(results):
    values = sorted({result.iterations for result in results})

    if len(values) == 1:
        return f"{values[0]} measured iterations"

    return (
        f"{values[0]}-{values[-1]} measured iterations"
    )


def setup_x_axis(ax, values):
    values = sorted(set(values))
    if not values:
        return

    if (
        len(values) > 1
        and values[0] > 0
        and values[-1] / values[0] >= 4
    ):
        ax.set_xscale("log", base=2)

    ax.set_xticks(values)
    ax.set_xticklabels([f"{value:,}" for value in values])


def make_styles(items, cmap_name):
    cmap = plt.get_cmap(cmap_name)

    return {
        item: (
            cmap(index % cmap.N),
            MARKERS[index % len(MARKERS)],
        )
        for index, item in enumerate(items)
    }


def save_figure(fig, path, dpi):
    fig.savefig(
        path,
        dpi=dpi,
        bbox_inches="tight",
    )
    plt.close(fig)
    print(f"saved {path}")


def plot_benchmark_grid(
    results,
    infos,
    benchmark_names,
    configs,
    config_styles,
    title,
    path,
    dpi,
    x_mode,
    log_y=False,
    throughput=False,
):
    if not benchmark_names:
        return

    columns = min(3, len(benchmark_names))
    rows = math.ceil(len(benchmark_names) / columns)

    fig, axes = plt.subplots(
        rows,
        columns,
        figsize=(5.2 * columns, 3.8 * rows),
        squeeze=False,
    )
    axes = axes.flatten()

    legend_entries = {}

    for axis_index, benchmark in enumerate(benchmark_names):
        ax = axes[axis_index]
        info = infos[benchmark]

        panel_results = [
            result
            for result in results
            if result.benchmark == benchmark
        ]

        all_x_values = []
        all_y_values = []

        for config in configs:
            config_results = sorted(
                (
                    result
                    for result in panel_results
                    if result.config == config
                ),
                key=lambda result: result.dim,
            )

            if not config_results:
                continue

            x_values = []
            y_values = []

            for result in config_results:
                if x_mode == "points":
                    x_value = base_grid_points(info, result.dim)
                elif x_mode == "cells":
                    x_value = base_grid_cells(info, result.dim)
                else:
                    x_value = result.dim

                if x_value is None:
                    continue

                if throughput:
                    cell_count = base_grid_cells(info, result.dim)
                    if (
                        cell_count is None
                        or result.seconds_per_iteration <= 0
                    ):
                        continue

                    y_value = (
                        cell_count
                        / result.seconds_per_iteration
                        / 1.0e6
                    )
                else:
                    y_value = result.seconds_per_iteration

                x_values.append(x_value)
                y_values.append(y_value)

            if not x_values:
                continue

            color, marker = config_styles[config]
            label = format_config(config)

            line, = ax.plot(
                x_values,
                y_values,
                color=color,
                marker=marker,
                linewidth=2,
                markersize=6,
                label=label,
            )

            legend_entries.setdefault(label, line)
            all_x_values.extend(x_values)
            all_y_values.extend(y_values)

        setup_x_axis(ax, all_x_values)

        if log_y and all(value > 0 for value in all_y_values):
            ax.set_yscale("log")
        else:
            ax.set_ylim(bottom=0)

        if throughput:
            ax.yaxis.set_major_formatter(
                FuncFormatter(
                    lambda value, _: format_rate(value)
                )
            )
        else:
            ax.yaxis.set_major_formatter(
                FuncFormatter(
                    lambda value, _: format_duration(value)
                )
            )

        ax.set_title(info.label, fontsize=12)
        ax.grid(True, which="both", linestyle=":", alpha=0.45)

    for axis_index in range(len(benchmark_names), len(axes)):
        axes[axis_index].set_visible(False)

    if x_mode == "points":
        x_label = "input grid points"
    elif x_mode == "cells":
        x_label = "base-grid logical cells"
    else:
        x_label = "points per active axis"

    y_label = (
        "million base-grid logical cells / second"
        if throughput
        else "average time / iteration"
    )

    fig.suptitle(
        f"{title} ({iteration_summary(results)})",
        fontsize=17,
        fontweight="bold",
    )
    fig.supxlabel(x_label, fontsize=13)
    fig.supylabel(y_label, fontsize=13)

    if legend_entries:
        fig.legend(
            legend_entries.values(),
            legend_entries.keys(),
            loc="center left",
            bbox_to_anchor=(0.84, 0.5),
            fontsize=10,
        )
        right = 0.82
    else:
        right = 0.97

    fig.tight_layout(rect=(0.03, 0.03, right, 0.93))
    save_figure(fig, path, dpi)


def plot_cross_type_comparison(
    results,
    infos,
    config,
    element_types,
    element_styles,
    out_dir,
    dpi,
    log_y,
):
    operations = [
        operation
        for operation, _ in UNSTRUCTURED_OPERATIONS
        if any(
            info.operation == operation
            and info.element_type in element_types
            and info.ndims is not None
            for info in infos.values()
        )
    ]

    if not operations:
        return

    columns = 3
    rows = math.ceil(len(operations) / columns)

    fig, axes = plt.subplots(
        rows,
        columns,
        figsize=(5.2 * columns, 3.8 * rows),
        squeeze=False,
    )
    axes = axes.flatten()

    legend_entries = {}

    for axis_index, operation in enumerate(operations):
        ax = axes[axis_index]
        all_x_values = []
        all_y_values = []

        for element_type in element_types:
            matching_infos = [
                info
                for info in infos.values()
                if info.operation == operation
                and info.element_type == element_type
                and info.ndims is not None
            ]

            if not matching_infos:
                continue

            benchmark = matching_infos[0].name
            info = matching_infos[0]

            matching_results = sorted(
                (
                    result
                    for result in results
                    if result.benchmark == benchmark
                    and result.config == config
                ),
                key=lambda result: result.dim,
            )

            if not matching_results:
                continue

            x_values = [
                base_grid_cells(info, result.dim)
                for result in matching_results
            ]
            y_values = [
                result.seconds_per_iteration
                for result in matching_results
            ]

            color, marker = element_styles[element_type]

            line, = ax.plot(
                x_values,
                y_values,
                color=color,
                marker=marker,
                linewidth=2,
                markersize=6,
                label=element_type,
            )

            legend_entries.setdefault(element_type, line)
            all_x_values.extend(x_values)
            all_y_values.extend(y_values)

        setup_x_axis(ax, all_x_values)

        if log_y and all(value > 0 for value in all_y_values):
            ax.set_yscale("log")
        else:
            ax.set_ylim(bottom=0)

        ax.yaxis.set_major_formatter(
            FuncFormatter(
                lambda value, _: format_duration(value)
            )
        )
        ax.set_title(OPERATION_LABELS[operation], fontsize=12)
        ax.grid(True, which="both", linestyle=":", alpha=0.45)

    for axis_index in range(len(operations), len(axes)):
        axes[axis_index].set_visible(False)

    fig.suptitle(
        "Unstructured element-type comparison\n"
        f"{format_config(config)}",
        fontsize=17,
        fontweight="bold",
    )
    fig.supxlabel("base-grid logical cells", fontsize=13)
    fig.supylabel("average time / iteration", fontsize=13)

    if legend_entries:
        fig.legend(
            legend_entries.values(),
            legend_entries.keys(),
            loc="center left",
            bbox_to_anchor=(0.84, 0.5),
            fontsize=10,
        )

    fig.tight_layout(rect=(0.03, 0.03, 0.82, 0.91))

    path = (
        out_dir
        / f"unstructured_cross_type_{config_slug(config)}.png"
    )
    save_figure(fig, path, dpi)


def plot_heatmaps(
    results,
    infos,
    configs,
    element_types,
    out_dir,
    dpi,
    all_dimensions,
):
    benchmark_by_operation_and_type = {
        (info.operation, info.element_type): info.name
        for info in infos.values()
        if info.family == "unstructured"
    }

    operations = [
        operation
        for operation, _ in UNSTRUCTURED_OPERATIONS
        if any(
            (operation, element_type)
            in benchmark_by_operation_and_type
            for element_type in element_types
        )
    ]

    lookup = {
        (result.benchmark, result.config, result.dim): result
        for result in results
    }

    for config in configs:
        dimensions = sorted({
            result.dim
            for result in results
            if result.config == config
            and infos[result.benchmark].family == "unstructured"
        })

        if not dimensions:
            continue

        if not all_dimensions:
            dimensions = [dimensions[-1]]

        for dim in dimensions:
            matrix = np.full(
                (len(operations), len(element_types)),
                np.nan,
            )

            for row, operation in enumerate(operations):
                for column, element_type in enumerate(element_types):
                    benchmark = benchmark_by_operation_and_type.get(
                        (operation, element_type)
                    )

                    if benchmark is None:
                        continue

                    result = lookup.get(
                        (benchmark, config, dim)
                    )

                    if result is not None:
                        matrix[row, column] = (
                            result.seconds_per_iteration
                        )

            positive_values = matrix[
                np.isfinite(matrix) & (matrix > 0)
            ]

            if positive_values.size == 0:
                continue

            minimum = float(np.min(positive_values))
            maximum = float(np.max(positive_values))

            if minimum == maximum:
                minimum *= 0.5
                maximum *= 2.0

            norm = LogNorm(vmin=minimum, vmax=maximum)
            masked = np.ma.masked_invalid(matrix)
            masked = np.ma.masked_less_equal(masked, 0)

            cmap = copy(plt.get_cmap("viridis"))
            cmap.set_bad("#dddddd")

            fig, ax = plt.subplots(
                figsize=(
                    max(9, 1.4 * len(element_types)),
                    max(6, 0.75 * len(operations)),
                )
            )

            image = ax.imshow(
                masked,
                cmap=cmap,
                norm=norm,
                aspect="auto",
            )

            ax.set_xticks(range(len(element_types)))
            ax.set_xticklabels(
                element_types,
                rotation=35,
                ha="right",
            )
            ax.set_yticks(range(len(operations)))
            ax.set_yticklabels([
                OPERATION_LABELS[operation]
                for operation in operations
            ])

            for row in range(len(operations)):
                for column in range(len(element_types)):
                    value = matrix[row, column]

                    if not np.isfinite(value) or value <= 0:
                        text = "n/a"
                        color = "#666666"
                    else:
                        text = format_duration(value)
                        color = (
                            "white"
                            if norm(value) > 0.55
                            else "black"
                        )

                    ax.text(
                        column,
                        row,
                        text,
                        ha="center",
                        va="center",
                        fontsize=8,
                        color=color,
                    )

            colorbar = fig.colorbar(image, ax=ax)
            colorbar.set_label("average time / iteration")
            colorbar.ax.yaxis.set_major_formatter(
                FuncFormatter(
                    lambda value, _: format_duration(value)
                )
            )

            ax.set_title(
                "Unstructured benchmark summary\n"
                f"N={dim}, {format_config(config)}",
                fontsize=15,
                fontweight="bold",
            )

            fig.tight_layout()

            path = (
                out_dir
                / (
                    "unstructured_heatmap_"
                    f"{config_slug(config)}_dim-{dim}.png"
                )
            )
            save_figure(fig, path, dpi)


def write_csv(results, infos, path):
    fields = [
        "benchmark",
        "family",
        "operation",
        "element_type",
        "topological_dimensions",
        "dim",
        "grid_points",
        "base_grid_logical_cells",
        "backend",
        "source_location",
        "execution_location",
        "output_location",
        "sync_strategy",
        "threads",
        "iterations",
        "total_measured_seconds",
        "seconds_per_iteration",
        "base_grid_logical_cells_per_second",
    ]

    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()

        for result in sorted(
            results,
            key=lambda item: (
                item.benchmark,
                config_sort_key(item.config),
                item.dim,
            ),
        ):
            info = infos[result.benchmark]
            points = base_grid_points(info, result.dim)
            cells = base_grid_cells(info, result.dim)

            throughput = ""
            if (
                cells is not None
                and result.seconds_per_iteration > 0
            ):
                throughput = (
                    cells / result.seconds_per_iteration
                )

            writer.writerow({
                "benchmark": result.benchmark,
                "family": info.family,
                "operation": info.operation or "",
                "element_type": info.element_type or "",
                "topological_dimensions": (
                    info.ndims if info.ndims is not None else ""
                ),
                "dim": result.dim,
                "grid_points": points if points is not None else "",
                "base_grid_logical_cells": (
                    cells if cells is not None else ""
                ),
                "backend": result.config.backend or "",
                "source_location": result.config.src,
                "execution_location": result.config.execution,
                "output_location": result.config.out,
                "sync_strategy": result.config.sync or "",
                "threads": (
                    result.config.threads
                    if result.config.threads is not None
                    else ""
                ),
                "iterations": result.iterations,
                "total_measured_seconds": result.total_seconds,
                "seconds_per_iteration": (
                    result.seconds_per_iteration
                ),
                "base_grid_logical_cells_per_second": throughput,
            })

    print(f"saved {path}")


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Plot current Conduit mesh transform benchmark results."
        )
    )
    parser.add_argument(
        "cali_file",
        nargs="?",
        help="path to a current benchmark .cali file",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="output directory, defaults to the Caliper filename stem",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=160,
        help="PNG resolution, default: 160",
    )
    parser.add_argument(
        "--log-y",
        action="store_true",
        help="use logarithmic scaling for runtime plots",
    )
    parser.add_argument(
        "--throughput",
        action="store_true",
        help=(
            "also generate base-grid logical-cell throughput plots"
        ),
    )
    parser.add_argument(
        "--no-cross-type",
        action="store_true",
        help="skip cross-element-type comparison figures",
    )
    parser.add_argument(
        "--no-heatmaps",
        action="store_true",
        help="skip unstructured summary heatmaps",
    )
    parser.add_argument(
        "--all-heatmaps",
        action="store_true",
        help=(
            "generate heatmaps for every dimension instead of only "
            "the largest"
        ),
    )

    args = parser.parse_args()

    cali_file = (
        Path(args.cali_file)
        if args.cali_file
        else find_cali_file()
    )

    if not cali_file.exists():
        sys.exit(f"no such file: {cali_file}")

    try:
        thicket = th.Thicket.from_caliperreader(
            str(cali_file)
        )
        results, time_column = collect_results(thicket)
    except ReaderError as error:
        sys.exit(f"failed to read {cali_file}: {error}")
    except RuntimeError as error:
        sys.exit(str(error))

    if not results:
        sys.exit(
            f"no current mesh benchmark regions found in {cali_file}"
        )

    benchmark_names = sorted({
        result.benchmark for result in results
    })
    infos = {
        name: classify_benchmark(name)
        for name in benchmark_names
    }

    configs = sorted(
        {result.config for result in results},
        key=config_sort_key,
    )
    dimensions = sorted({result.dim for result in results})
    element_types = ordered_element_types(infos)

    out_dir = (
        args.output_dir
        if args.output_dir is not None
        else cali_file.with_suffix("")
    )
    out_dir.mkdir(parents=True, exist_ok=True)

    print(f"time column: {time_column}")
    print(f"benchmark names: {len(benchmark_names)}")
    print(f"configurations: {len(configs)}")
    print(f"dimension sizes: {dimensions}")
    print(
        "element types: "
        + (", ".join(element_types) if element_types else "none")
    )

    if max(dimensions) <= 4:
        print(
            "warning: all dimensions are <= 4; these runs are useful "
            "for CI validation but are likely dominated by fixed overhead"
        )

    write_csv(
        results,
        infos,
        out_dir / "benchmark_results.csv",
    )

    config_styles = make_styles(configs, "tab20")
    element_styles = make_styles(element_types, "tab10")

    plotted = set()

    mesh_names = [
        name
        for name, _ in MESH_BENCHMARKS
        if name in infos
    ]
    mesh_names.extend(sorted(
        name
        for name, info in infos.items()
        if info.family == "mesh"
        and name not in mesh_names
    ))

    if mesh_names:
        plot_benchmark_grid(
            results,
            infos,
            mesh_names,
            configs,
            config_styles,
            "Whole-mesh conversions",
            out_dir / "mesh_scaling.png",
            args.dpi,
            x_mode="points",
            log_y=args.log_y,
        )
        plotted.update(mesh_names)

    for element_type in element_types:
        type_names = [
            info.name
            for info in infos.values()
            if info.family == "unstructured"
            and info.element_type == element_type
        ]
        type_names.sort(
            key=lambda name: (
                OPERATION_ORDER.get(
                    infos[name].operation,
                    len(OPERATION_ORDER),
                ),
                name,
            )
        )

        if not type_names:
            continue

        use_logical_cells = all(
            infos[name].ndims is not None
            for name in type_names
        )
        x_mode = "cells" if use_logical_cells else "dim"

        plot_benchmark_grid(
            results,
            infos,
            type_names,
            configs,
            config_styles,
            f"Unstructured {element_type} transforms",
            out_dir
            / f"unstructured_{safe_file_part(element_type)}_scaling.png",
            args.dpi,
            x_mode=x_mode,
            log_y=args.log_y,
        )

        if args.throughput and use_logical_cells:
            plot_benchmark_grid(
                results,
                infos,
                type_names,
                configs,
                config_styles,
                f"Unstructured {element_type} throughput",
                out_dir
                / (
                    "unstructured_"
                    f"{safe_file_part(element_type)}_throughput.png"
                ),
                args.dpi,
                x_mode="cells",
                log_y=args.log_y,
                throughput=True,
            )

        plotted.update(type_names)

    other_names = sorted(set(benchmark_names) - plotted)
    if other_names:
        print(
            "warning: plotting unclassified benchmarks in "
            "other_scaling.png:"
        )
        for name in other_names:
            print(f"  {name}")

        plot_benchmark_grid(
            results,
            infos,
            other_names,
            configs,
            config_styles,
            "Other discovered benchmarks",
            out_dir / "other_scaling.png",
            args.dpi,
            x_mode="dim",
            log_y=args.log_y,
        )
        plotted.update(other_names)

    if set(benchmark_names) != plotted:
        missing = sorted(set(benchmark_names) - plotted)
        sys.exit(
            "internal error: benchmarks were not plotted: "
            + ", ".join(missing)
        )

    if not args.no_cross_type:
        for config in configs:
            plot_cross_type_comparison(
                results,
                infos,
                config,
                element_types,
                element_styles,
                out_dir,
                args.dpi,
                args.log_y,
            )

    if not args.no_heatmaps:
        plot_heatmaps(
            results,
            infos,
            configs,
            element_types,
            out_dir,
            args.dpi,
            args.all_heatmaps,
        )

    print(f"wrote results to {out_dir}/")


if __name__ == "__main__":
    main()