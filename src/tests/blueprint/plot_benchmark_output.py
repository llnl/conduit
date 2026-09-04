# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

# Plots .cali output from t_blueprint_mesh_transform_benchmark.
#
# Usage:
#     python3 plot_benchmark_output.py [path/to/file.cali]
#
#     With no arguments, the most recently modified .cali file in the current
#     working directory is used. With one argument, a specific .cali file is
#     used. There are no flags or other options.
#
# Examples:
#     # Run the benchmark, which writes <YYYYmmdd_HHMMSS>.cali
#     # into its own working directory, so run the plotting script from that
#     # same directory.
#     ./t_blueprint_mesh_transform_benchmark
#     python3 plot_benchmark_output.py
#
#     # Plot a specific run, from anywhere.
#     python3 plot_benchmark_output.py /path/to/20260803_120000.cali
#
# Input:
#     A Caliper .cali file produced by t_blueprint_mesh_transform_benchmark.
#     Note that the benchmark only writes a .cali file if Conduit was built
#     with Caliper support; otherwise it prints
#     a warning and produces no timing output.
#
# Output:
#     A directory next to the .cali file, named after it with the extension
#     stripped (e.g. 20260803_120000.cali -> 20260803_120000/), containing:
#       - thicket_mesh_heatmap.png, thicket_generate_heatmap.png,
#         thicket_boxplot.png (skipped if seaborn is not installed)
#       - one subdirectory per execution backend found in the file (plus a
#         "combined" subdirectory when more than one backend is present),
#         each containing:
#           - mesh_scaling.png, mesh_scaling_per_element.png
#           - generate_scaling.png, generate_scaling_per_element.png
#           - generate_scaling_by_config.png and
#             generate_scaling_per_element_by_config.png (only when more
#             than one src/exec/out/sync configuration is present)
#           - mesh_conversion_heatmap_dim-<N>.png (and _per_element variant)
#           - generate_heatmap_dim-<N>.png (and _per_element variant)
#           - generate_growth_heatmap_dim-<N>.png
#           - the heatmap names above gain a _<src>-<exec>-<out>-<sync>
#             suffix when both the all-host and all-device configurations
#             are present
#           - individual/<figure name>/<panel>.png, one single-panel image
#             per panel of each multi-panel figure above
#
# Requirements:
#     pip install thicket caliper-reader matplotlib numpy seaborn

import math
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import thicket as th
from caliperreader.readererror import ReaderError

REQUIRED_NUMERIC_FIELDS = ("dim", "inverts", "inelems", "outverts", "outelems", "iter")
LOCATION_FIELDS = ("src", "exec", "out")
SYNC_STRATEGIES = ("sync", "assume")


def parse_scope_name(raw):
    tokens = raw.split("_")

    i = 0
    while i < len(tokens) and "-" not in tokens[i]:
        i += 1
    name = "_".join(tokens[:i])
    is_generate = any(name.startswith(op + "_") for op in OPERATIONS)
    if not (name.startswith("mesh_") or is_generate):
        return None

    fields = {}
    for token in tokens[i:]:
        if "-" not in token:
            return None
        key, value = token.split("-", 1)
        fields[key] = value

    if not all(fields.get(key, "").isdigit() for key in REQUIRED_NUMERIC_FIELDS):
        return None
    if any(fields.get(key) not in ("host", "device") for key in LOCATION_FIELDS):
        return None

    if fields.get("sync") not in SYNC_STRATEGIES:
        return None

    cfg = tuple(fields[key] for key in LOCATION_FIELDS) + (fields["sync"],)
    backend = fields.get("backend")
    if fields.get("mem") == "unified" and backend is not None:
        backend += "-unified"
    parsed = {"name": name, "cfg": cfg, "backend": backend}
    parsed.update((key, int(fields[key])) for key in REQUIRED_NUMERIC_FIELDS)
    threads = fields.get("threads")
    parsed["threads"] = int(threads) if threads is not None and threads.isdigit() else None
    return parsed


def split_mesh_convert(name):
    tokens = name.split("_")
    if len(tokens) == 4 and tokens[0] == "mesh" and tokens[2] == "to":
        return tokens[1], tokens[3]
    return None

OPERATIONS = [
    "generate_centroids",
    "generate_points",
    "generate_faces",
    "generate_lines",
    "generate_corners",
    "generate_sides",
    "to_polytopal",
]
MESH_SRC_ORDER = ["structured", "rectilinear", "uniform"]
SHAPE_ORDER = ["quads", "hexs", "pyramids"]

BASELINE_CFG = ("host", "host", "host", "sync")
DEVICE_BASELINE_CFG = ("device", "device", "device", "sync")

CFG_ORDER = [
    ("host", "host", "host", "sync"),
    ("host", "host", "device", "sync"),
    ("host", "host", "device", "assume"),
    ("host", "device", "host", "sync"),
    ("host", "device", "host", "assume"),
    ("host", "device", "device", "sync"),
    ("device", "host", "host", "sync"),
    ("device", "host", "device", "sync"),
    ("device", "host", "device", "assume"),
    ("device", "device", "host", "sync"),
    ("device", "device", "host", "assume"),
    ("device", "device", "device", "sync"),
]
CFG_ABBREVIATIONS = {"device": "dev"}

COMBINED_LABEL = "combined"

INDIVIDUAL_FIGSIZE = (6.4, 3.6)
MAX_STACKED_ROWS = 4
MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*"]


def format_cfg_label(cfg):
    src, exec_, out, sync = cfg
    path = "→".join(CFG_ABBREVIATIONS.get(part, part) for part in (src, exec_, out))
    return path if exec_ == out else f"{path} [{sync}]"


def ordered(values, preferred_order):
    values = set(values)
    return [v for v in preferred_order if v in values] + \
        sorted(values - set(preferred_order))


def split_generate_name(name):
    for operation in OPERATIONS:
        if name.startswith(operation + "_"):
            return operation, name[len(operation) + 1:]
    return name, ""

OPERATIONS = [
    "generate_centroids",
    "generate_points",
    "generate_faces",
    "generate_lines",
    "generate_corners",
    "generate_sides",
    "to_polytopal",
]
MESH_SRC_ORDER = ["structured", "rectilinear", "uniform"]
SHAPE_ORDER = ["quads", "hexs", "pyramids"]
MESH_SERIES_LABEL = "serial - before device support"

INDIVIDUAL_FIGSIZE = (6.4, 3.6)
MAX_STACKED_ROWS = 4


def ordered(values, preferred_order):
    values = set(values)
    return [v for v in preferred_order if v in values] + \
        sorted(values - set(preferred_order))


def split_generate_name(name):
    for operation in OPERATIONS:
        if name.startswith(operation + "_"):
            return operation, name[len(operation) + 1:]
    return name, ""


def find_cali_file():
    files = list(Path.cwd().glob("*.cali"))
    if not files:
        sys.exit("no .cali files found in current directory")
    return max(files, key=lambda path: path.stat().st_mtime)


def find_time_column(dataframe):
    for name in ("time", "sum#sum#time.duration", "sum#time.duration"):
        if name in dataframe.columns:
            return name
    for column in dataframe.columns:
        if str(column).endswith("time.duration"):
            return column
    sys.exit(f"no time column found; columns were {list(dataframe.columns)}")


INCLUSIVE_COLUMN = "inclusive#time.duration"


def add_inclusive_column(thicket):
    profile_id = thicket.dataframe.index.get_level_values("profile")[0]
    time_column = find_time_column(thicket.dataframe)

    def own_time(node):
        try:
            return float(thicket.dataframe.loc[(node, profile_id), time_column])
        except (KeyError, TypeError):
            return 0.0

    def node_time(node):
        return own_time(node) + sum(node_time(child) for child in node.children)

    def all_nodes(node):
        yield node
        for child in node.children:
            yield from all_nodes(child)

    totals = {node: node_time(node)
              for root in thicket.graph.roots for node in all_nodes(root)}
    thicket.dataframe[INCLUSIVE_COLUMN] = [
        totals.get(index[0], float("nan")) for index in thicket.dataframe.index
    ]
    return INCLUSIVE_COLUMN


def collect(thicket):
    profile_id = thicket.dataframe.index.get_level_values("profile")[0]
    time_column = find_time_column(thicket.dataframe)

    def node_time(node):
        try:
            total = float(thicket.dataframe.loc[(node, profile_id), time_column])
        except (KeyError, TypeError):
            total = 0.0
        for child in node.children:
            total += node_time(child)
        return total

    def all_nodes(node):
        yield node
        for child in node.children:
            yield from all_nodes(child)

    # A region's reported time is the total spent inside it: its own time plus
    # every nested region beneath it. Caliper stores only the own time, so the
    # children have to be added back in.
    records = []
    for root in thicket.graph.roots:
        for node in all_nodes(root):
            parsed = parse_scope_name(str(node.frame.get("name", "")))
            if parsed is None:
                continue
            parsed["avg_time"] = node_time(node) / parsed["iter"]
            parsed["node"] = node
            records.append(parsed)
    return records


def per_element_value(record):
    if record["outelems"] <= 0:
        return None
    return record["avg_time"] / record["outelems"]


def growth_value(record):
    if record["inelems"] <= 0:
        return None
    return record["outelems"] / record["inelems"]


def build_line_panels(records, group_series_fn, value_fn):
    panels = {}
    for record in records:
        grouped = group_series_fn(record)
        if grouped is None:
            continue
        value = value_fn(record)
        if value is None:
            continue
        panel_title, series_label = grouped
        panels.setdefault(panel_title, {}).setdefault(series_label, []) \
              .append((record["dim"], value))
    return panels


def render_series(ax, series, title, y_scale, y_log, series_order=None,
                  show_legend=True):
    labels = ordered(series, series_order) if series_order else sorted(series)
    for idx, label in enumerate(labels):
        points = sorted(series[label])
        dims = [str(dim) for dim, _ in points]
        ys = [value * y_scale for _, value in points]
        ax.plot(dims, ys, marker=MARKERS[idx % len(MARKERS)], label=label)

    ax.set_title(title.replace("_", " "), fontsize=10)
    ax.tick_params(labelsize=8)
    if y_log:
        ax.set_yscale("log")
    else:
        ax.ticklabel_format(axis="y", style="plain", useOffset=False)
    if show_legend:
        ax.legend(fontsize=7)


def plot_group(panels, path, suptitle, ylabel, y_scale=1.0, y_log=False,
              series_order=None, stack_vertical=False, legend_label=None):
    titles = sorted(panels)
    n = len(titles)

    if stack_vertical:
        rows = min(MAX_STACKED_ROWS, n)
        columns = math.ceil(n / rows)
        panel_width = 6.5
    else:
        columns = min(4, n)
        rows = math.ceil(n / columns)
        panel_width = 4.2

    fig, axes = plt.subplots(
        rows, columns, figsize=(panel_width * columns, 3.2 * rows), squeeze=False
    )
    if stack_vertical:
        axes = [axes[row, col] for col in range(columns) for row in range(rows)]
    else:
        axes = list(axes.flatten())

    show_axis_legend = legend_label is None
    for ax, title in zip(axes, titles):
        render_series(ax, panels[title], title, y_scale, y_log, series_order,
                      show_legend=show_axis_legend)
    for ax in axes[n:]:
        ax.set_visible(False)

    fig.suptitle(suptitle, fontsize=13, fontweight="bold", wrap=True)
    fig.supxlabel("dim (points per axis)")
    fig.supylabel(ylabel)
    fig.tight_layout(rect=(0, 0, 1, 0.95))

    if legend_label is not None:
        handles, labels = axes[0].get_legend_handles_labels()
        fig.legend(handles, labels, loc="lower left",
                  bbox_to_anchor=(1.0, 0.0), fontsize=9)
        fig.savefig(path, dpi=150, bbox_inches="tight")
    else:
        fig.savefig(path, dpi=150)
    plt.close(fig)
    print(f"saved {path}")

    individual_dir = path.parent / "individual" / path.stem
    individual_dir.mkdir(parents=True, exist_ok=True)
    for title in titles:
        fig, ax = plt.subplots(figsize=INDIVIDUAL_FIGSIZE)
        render_series(ax, panels[title], title, y_scale, y_log, series_order)
        ax.set_title(f"{suptitle}\n{title.replace('_', ' ')}", fontsize=11)
        ax.set_xlabel("dim (points per axis)")
        ax.set_ylabel(ylabel)
        fig.tight_layout()
        fig.savefig(individual_dir / f"{title}.png", dpi=150)
        plt.close(fig)
    print(f"saved {len(titles)} individual plot(s) to {individual_dir}/")


def format_plain(value, sig_figs=3):
    if value == 0:
        return f"{0:.{sig_figs - 1}f}"
    magnitude = math.floor(math.log10(abs(value)))
    decimals = max(0, sig_figs - magnitude - 1)
    return f"{value:.{decimals}f}"


def format_ms_cell(value):
    return format_plain(value, sig_figs=3)


def format_ns_cell(value):
    if value >= 1:
        return f"{value:,.0f}"
    return f"{value:.2f}"


def format_growth_cell(value):
    return f"{format_plain(value, sig_figs=2)}x"


def render_heatmap(ax, matrix, rows, cols, title, cell_fmt=format_ms_cell):
    im = ax.imshow(matrix, cmap="viridis", aspect="auto")
    ax.set_xticks(range(len(cols)))
    ax.set_xticklabels(cols, rotation=45, ha="right", fontsize=8)
    ax.set_yticks(range(len(rows)))
    ax.set_yticklabels(rows, fontsize=8)
    ax.set_title(title, fontsize=10)

    midpoint = np.nanmin(matrix) + (np.nanmax(matrix) - np.nanmin(matrix)) / 2 \
        if np.any(~np.isnan(matrix)) else 0.0
    for i in range(matrix.shape[0]):
        for j in range(matrix.shape[1]):
            value = matrix[i, j]
            if np.isnan(value):
                continue
            color = "white" if value > midpoint else "black"
            ax.text(j, i, cell_fmt(value), ha="center", va="center",
                    fontsize=7, color=color)
    return im


def plot_heatmaps(lookup, rows, cols, dims, value_fn, out_dir, filename_prefix,
                  suptitle, row_label, col_label, cbar_label,
                  cell_fmt=format_ms_cell):
    if not rows or not cols or not dims:
        return

    width = 4.0 + 0.9 * len(cols)
    height = 2.0 + 0.3 * len(rows)

    for dim in dims:
        matrix = np.full((len(rows), len(cols)), np.nan)
        for i, row in enumerate(rows):
            for j, col in enumerate(cols):
                record = lookup.get((row, col, dim))
                value = value_fn(record) if record is not None else None
                if value is not None:
                    matrix[i, j] = value

        fig, ax = plt.subplots(figsize=(width, height))
        im = render_heatmap(ax, matrix, rows, cols, f"{suptitle}\ndim={dim}",
                            cell_fmt=cell_fmt)
        cbar = fig.colorbar(im, ax=ax, label=cbar_label)
        cbar.ax.ticklabel_format(style="plain", useOffset=False)
        ax.set_xlabel(col_label)
        ax.set_ylabel(row_label)
        fig.tight_layout()
        path = out_dir / f"{filename_prefix}_dim-{dim}.png"
        fig.savefig(path, dpi=150)
        plt.close(fig)
        print(f"saved {path}")


def plot_thicket_views(thicket, records, time_column, out_dir):
    # time_column must be the inclusive (subtree-total) column so these
    # views agree with every other figure this script produces.
    if not hasattr(th.stats, "display_heatmap"):
        print("seaborn not installed; skipping thicket plots")
        return

    mean_column = th.stats.mean(thicket, columns=[time_column])[0]
    full_statsframe = thicket.statsframe.dataframe

    for group, nodes in (
        ("mesh", [r["node"] for r in records if r["name"].startswith("mesh_")]),
        ("generate", [r["node"] for r in records if not r["name"].startswith("mesh_")]),
    ):
        if not nodes:
            continue
        thicket.statsframe.dataframe = full_statsframe.loc[nodes]
        plt.figure(figsize=(8, 1.5 + 0.25 * len(nodes)))
        ax = th.stats.display_heatmap(thicket, columns=[mean_column], annot=True, fmt=".6f")
        ax.tick_params(axis="y", labelsize=7)
        if ax.collections and ax.collections[0].colorbar:
            ax.collections[0].colorbar.ax.ticklabel_format(style="plain", useOffset=False)
        path = out_dir / f"thicket_{group}_heatmap.png"
        ax.get_figure().savefig(path, dpi=150, bbox_inches="tight")
        plt.close(ax.get_figure())
        print(f"saved {path}")
    thicket.statsframe.dataframe = full_statsframe

    sample_nodes = [r["node"] for r in records if r["name"].startswith("mesh_")]
    if sample_nodes:
        ax = th.stats.display_boxplot(thicket, nodes=sample_nodes, columns=[time_column])
        ax.tick_params(axis="x", rotation=90, labelsize=6)
        ax.ticklabel_format(axis="y", style="plain", useOffset=False)
        path = out_dir / "thicket_boxplot.png"
        ax.get_figure().savefig(path, dpi=150, bbox_inches="tight")
        plt.close(ax.get_figure())
        print(f"saved {path}")


def render_plot_set(records, out_dir, iters_label):
    out_dir.mkdir(parents=True, exist_ok=True)

    cfgs_seen = {r["cfg"] for r in records}
    multi_cfg = len(cfgs_seen) > 1
    if multi_cfg:
        cfg_list = ", ".join(format_cfg_label(c) for c in ordered(cfgs_seen, CFG_ORDER))
        print(f"  {len(cfgs_seen)} execution configs: {cfg_list}")
    else:
        print(f"  single execution config ({format_cfg_label(next(iter(cfgs_seen)))}); "
              "config-comparison plots skipped")

    baseline_cfg = BASELINE_CFG if BASELINE_CFG in cfgs_seen \
        else ordered(cfgs_seen, CFG_ORDER)[0]
    baseline_records = [r for r in records if r["cfg"] == baseline_cfg]
    mesh_records = [r for r in records if r["name"].startswith("mesh_")]
    generate_records = [r for r in records if not r["name"].startswith("mesh_")]
    baseline_generate_records = [r for r in baseline_records if not r["name"].startswith("mesh_")]

    def generate_group(record):
        operation, shape = split_generate_name(record["name"])
        return (operation, shape) if shape else None

    def generate_config_group(record):
        operation, shape = split_generate_name(record["name"])
        if not shape:
            return None
        return (f"{operation}_{shape}", format_cfg_label(record["cfg"]))

    mesh_time_panels = build_line_panels(
        mesh_records, lambda r: (r["name"], format_cfg_label(r["cfg"])), lambda r: r["avg_time"]
    )
    generate_time_panels = build_line_panels(
        baseline_generate_records, generate_group, lambda r: r["avg_time"]
    )
    if mesh_time_panels:
        plot_group(
            mesh_time_panels, out_dir / "mesh_scaling.png",
            suptitle=f"Mesh conversion benchmark: avg time per iteration ({iters_label})",
            ylabel="avg time / iteration (ms)", y_scale=1e3,
            stack_vertical=True, legend_label="config",
        )
    if generate_time_panels:
        baseline_note = f" ({format_cfg_label(baseline_cfg)})" if multi_cfg else ""
        plot_group(
            generate_time_panels, out_dir / "generate_scaling.png",
            suptitle=f"Generate-function benchmark: avg time per iteration{baseline_note} ({iters_label})",
            ylabel="avg time / iteration (ms)", y_scale=1e3,
            series_order=SHAPE_ORDER, stack_vertical=True,
        )

    mesh_per_elem_panels = build_line_panels(
        mesh_records, lambda r: (r["name"], format_cfg_label(r["cfg"])), per_element_value
    )
    generate_per_elem_panels = build_line_panels(
        baseline_generate_records, generate_group, per_element_value
    )
    if mesh_per_elem_panels:
        plot_group(
            mesh_per_elem_panels, out_dir / "mesh_scaling_per_element.png",
            suptitle=f"Mesh conversion benchmark: avg time per element ({iters_label})",
            ylabel="avg time / element (ns)", y_scale=1e9, y_log=True,
            stack_vertical=True, legend_label="config",
        )
    if generate_per_elem_panels:
        baseline_note = f" ({format_cfg_label(baseline_cfg)})" if multi_cfg else ""
        plot_group(
            generate_per_elem_panels, out_dir / "generate_scaling_per_element.png",
            suptitle=f"Generate-function benchmark: avg time per element{baseline_note} ({iters_label})",
            ylabel="avg time / element (ns)", y_scale=1e9, y_log=True,
            series_order=SHAPE_ORDER, stack_vertical=True,
        )

    if multi_cfg:
        generate_config_time_panels = build_line_panels(
            generate_records, generate_config_group, lambda r: r["avg_time"]
        )
        generate_config_per_elem_panels = build_line_panels(
            generate_records, generate_config_group, per_element_value
        )
        cfg_order_labels = [format_cfg_label(c) for c in CFG_ORDER]
        if generate_config_time_panels:
            plot_group(
                generate_config_time_panels, out_dir / "generate_scaling_by_config.png",
                suptitle=f"Generate-function benchmark: avg time per iteration across execution configs ({iters_label})",
                ylabel="avg time / iteration (ms)", y_scale=1e3,
                series_order=cfg_order_labels, stack_vertical=True, legend_label="config",
            )
        if generate_config_per_elem_panels:
            plot_group(
                generate_config_per_elem_panels, out_dir / "generate_scaling_per_element_by_config.png",
                suptitle=f"Generate-function benchmark: avg time per element across execution configs ({iters_label})",
                ylabel="avg time / element (ns)", y_scale=1e9, y_log=True,
                series_order=cfg_order_labels, stack_vertical=True, legend_label="config",
            )

    heatmap_cfgs = [c for c in (BASELINE_CFG, DEVICE_BASELINE_CFG) if c in cfgs_seen]
    if not heatmap_cfgs:
        heatmap_cfgs = ordered(cfgs_seen, CFG_ORDER)[:1]
    suffix_heatmaps = len(heatmap_cfgs) > 1

    for cfg in heatmap_cfgs:
        suffix = "_" + "-".join(cfg) if suffix_heatmaps else ""
        cfg_note = f" [{format_cfg_label(cfg)}]" if suffix_heatmaps else ""
        cfg_mesh_records = [r for r in mesh_records if r["cfg"] == cfg]
        cfg_generate_records = [r for r in generate_records if r["cfg"] == cfg]

        mesh_lookup = {}
        for r in cfg_mesh_records:
            convert = split_mesh_convert(r["name"])
            if convert:
                mesh_lookup[(convert[0], convert[1], r["dim"])] = r
        if mesh_lookup:
            srcs = ordered({k[0] for k in mesh_lookup}, MESH_SRC_ORDER)
            dsts = sorted({k[1] for k in mesh_lookup})
            dims = sorted({k[2] for k in mesh_lookup})
            plot_heatmaps(
                mesh_lookup, srcs, dsts, dims,
                value_fn=lambda r: r["avg_time"] * 1e3,
                out_dir=out_dir, filename_prefix=f"mesh_conversion_heatmap{suffix}",
                suptitle=f"Mesh conversion avg time per iteration, ms{cfg_note} ({iters_label})",
                row_label="source representation", col_label="target representation",
                cbar_label="avg time / iteration (ms)",
            )
            plot_heatmaps(
                mesh_lookup, srcs, dsts, dims,
                value_fn=lambda r: per_element_value(r) * 1e9 if per_element_value(r) else None,
                out_dir=out_dir, filename_prefix=f"mesh_conversion_heatmap_per_element{suffix}",
                suptitle=f"Mesh conversion avg time per element, ns{cfg_note} ({iters_label})",
                row_label="source representation", col_label="target representation",
                cbar_label="avg time / element (ns)", cell_fmt=format_ns_cell,
            )

        shape_lookup = {}
        for r in cfg_generate_records:
            operation, shape = split_generate_name(r["name"])
            if shape:
                shape_lookup[(operation, shape, r["dim"])] = r
        if shape_lookup:
            operations = ordered({k[0] for k in shape_lookup}, OPERATIONS)
            shapes = ordered({k[1] for k in shape_lookup}, SHAPE_ORDER)
            dims = sorted({k[2] for k in shape_lookup})
            plot_heatmaps(
                shape_lookup, operations, shapes, dims,
                value_fn=lambda r: r["avg_time"] * 1e3,
                out_dir=out_dir, filename_prefix=f"generate_heatmap{suffix}",
                suptitle=f"Generate-function avg time per iteration, ms{cfg_note} ({iters_label})",
                row_label="operation", col_label="element shape",
                cbar_label="avg time / iteration (ms)",
            )
            plot_heatmaps(
                shape_lookup, operations, shapes, dims,
                value_fn=lambda r: per_element_value(r) * 1e9 if per_element_value(r) else None,
                out_dir=out_dir, filename_prefix=f"generate_heatmap_per_element{suffix}",
                suptitle=f"Generate-function avg time per element, ns{cfg_note} ({iters_label})",
                row_label="operation", col_label="element shape",
                cbar_label="avg time / element (ns)", cell_fmt=format_ns_cell,
            )

            if cfg == heatmap_cfgs[0]:
                plot_heatmaps(
                    shape_lookup, operations, shapes, dims,
                    value_fn=growth_value,
                    out_dir=out_dir, filename_prefix="generate_growth_heatmap",
                    suptitle=f"Generate-function output/input element ratio ({iters_label})",
                    row_label="operation", col_label="element shape",
                    cbar_label="output elements / input elements", cell_fmt=format_growth_cell,
                )

    return baseline_records


def main():
    cali_file = Path(sys.argv[1]) if len(sys.argv) > 1 else find_cali_file()
    if not cali_file.exists():
        sys.exit(f"no such file: {cali_file}")

    try:
        thicket = th.Thicket.from_caliperreader(str(cali_file))
    except ReaderError as error:
        sys.exit(f"failed to read {cali_file}: {error}")

    records = collect(thicket)
    if not records:
        sys.exit(f"no benchmark regions found in {cali_file}")

    iters = sorted({r["iter"] for r in records})
    iters_label = f"n={iters[0]}" if len(iters) == 1 else f"n={iters[0]}-{iters[-1]}"
    threads_seen = {r["threads"] for r in records if r["threads"] is not None}
    if threads_seen:
        threads_label = f"threads={min(threads_seen)}" if len(threads_seen) == 1 \
            else f"threads={min(threads_seen)}-{max(threads_seen)}"
        iters_label = f"{iters_label}, {threads_label}"

    out_dir = cali_file.with_suffix("")
    out_dir.mkdir(exist_ok=True)

    backends = sorted({r["backend"] for r in records if r["backend"]})
    if len(backends) > 1:
        groups = [(b, [r for r in records if r["backend"] == b]) for b in backends]
        groups.append((COMBINED_LABEL, records))
    elif backends:
        groups = [(backends[0], records)]
    else:
        print("no backend field found; plotting all records together")
        groups = [(COMBINED_LABEL, records)]

    baseline_records = records
    for label, group_records in groups:
        print(f"{label}: {len(group_records)} records")
        group_baseline = render_plot_set(
            group_records, out_dir / label, f"{iters_label}, {label}"
        )
        if label == COMBINED_LABEL or len(groups) == 1:
            baseline_records = group_baseline

    plot_thicket_views(thicket, baseline_records, add_inclusive_column(thicket), out_dir)

    print(f"wrote plots to {out_dir}/")


if __name__ == "__main__":
    main()
