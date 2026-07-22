# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

import argparse
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import thicket as th
from caliperreader.readererror import ReaderError
from matplotlib.ticker import FormatStrFormatter

COORDSET_CONVERSIONS = [
    "coordset_rectilinear_to_explicit",
    "coordset_uniform_to_explicit",
    "coordset_uniform_to_rectilinear",
]

TOPOLOGY_CONVERSIONS = [
    "topology_uniform_to_unstructured",
    "topology_rectilinear_to_unstructured",
    "topology_structured_to_unstructured",
]

CONVERSIONS = COORDSET_CONVERSIONS + TOPOLOGY_CONVERSIONS

CONVERSION_LABELS = {
    "coordset_rectilinear_to_explicit": "rect→explicit",
    "coordset_uniform_to_explicit": "uniform→explicit",
    "coordset_uniform_to_rectilinear": "uniform→rect",
    "topology_uniform_to_unstructured": "uniform→unstruct",
    "topology_rectilinear_to_unstructured": "rect→unstruct",
    "topology_structured_to_unstructured": "struct→unstruct",
}

CONVERSION_GROUPS = [
    ("coordset", COORDSET_CONVERSIONS),
    ("topology", TOPOLOGY_CONVERSIONS),
]

END_TO_END_GROUP_PANELS = [
    (group_name, [(CONVERSION_LABELS[c], c, "end_to_end") for c in group_conversions])
    for group_name, group_conversions in CONVERSION_GROUPS
]

COMPONENT_METRICS = [
    ("use_with", "use_with"),
    ("forall", "forall"),
    ("data movement", "data_movement"),
    ("other processing", "other_processing"),
]

CMAP = plt.get_cmap("tab10")
MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*"]

CFG_ABBREVIATIONS = {
    "openmp": "omp",
    "device": "dev",
}


def format_cfg_label(cfg):
    backend, src, exec_, out = cfg

    if backend is None:
        return "serial - before device support"

    backend = CFG_ABBREVIATIONS.get(backend, backend)
    src = CFG_ABBREVIATIONS.get(src, src)
    exec_ = CFG_ABBREVIATIONS.get(exec_, exec_)
    out = CFG_ABBREVIATIONS.get(out, out)
    return f"{backend}: {src}→{exec_}→{out}"


def format_range_label(prefix, values):
    values = sorted(values)
    if len(values) == 1:
        return f"{prefix}={values[0]}"
    return f"{prefix}={values[0]}-{values[-1]}"


def parse_config_name(name):
    conv_name = None
    for c in CONVERSIONS:
        if name.startswith(c + "_"):
            conv_name = c
            break
    if conv_name is None:
        return None

    rest = name[len(conv_name) + 1 :]
    parts = rest.split("_")

    if len(parts) not in (6, 7, 8):
        return None

    values = {}
    backend = None

    # If the first token has no dash, treat it as backend
    start_idx = 0
    if "-" not in parts[0]:
        backend = parts[0]
        start_idx = 1

    for field in parts[start_idx:]:
        if "-" not in field:
            return None
        key, value = field.split("-", 1)
        values[key] = value

    required = {"dim", "src", "exec", "out", "sync", "iter"}
    if set(values) not in (required, required | {"threads"}):
        return None
    if values["src"] not in ("host", "device"):
        return None
    if values["exec"] not in ("host", "device"):
        return None
    if values["out"] not in ("host", "device"):
        return None
    if values["sync"] not in ("sync", "assume"):
        return None
    if not values["dim"].isdigit() or not values["iter"].isdigit():
        return None
    threads = None
    if "threads" in values:
        if not values["threads"].isdigit():
            return None
        threads = int(values["threads"])

    cfg = (backend, values["src"], values["exec"], values["out"])
    return conv_name, cfg, int(values["dim"]), int(values["iter"]), threads, values["sync"]


def parse_timestamp(cali_file):
    stem = Path(cali_file).stem
    if len(stem) == 15 and stem[:8].isdigit() and stem[8] == "_" and stem[9:].isdigit():
        return f"{stem[0:4]}-{stem[4:6]}-{stem[6:8]} {stem[9:11]}:{stem[11:13]}:{stem[13:15]}"
    return None


def find_cali_file():
    all_cali = list(Path.cwd().glob("*.cali"))
    cali_files = []
    for f in all_cali:
        if parse_timestamp(f) is not None:
            cali_files.append(f)
    cali_files.sort(reverse=True)

    if not cali_files:
        sys.exit(f"no timestamped .cali files (YYYYMMDD_HHMMSS.cali) found in {Path.cwd()}")

    skipped = len(all_cali) - len(cali_files)
    print(
        f"found {len(cali_files)} timestamped .cali file(s) in {Path.cwd()}"
        + (f" ({skipped} other .cali file(s) skipped)" if skipped else "")
        + ":"
    )
    return cali_files[0]


def node_time(thicket, profile_id, node):
    try:
        return thicket.dataframe.loc[(node, profile_id), "time"]
    except KeyError:
        return float("nan")


def subtree_time(thicket, profile_id, node):
    # Caliper measures exclusive time, so a region's inclusive time is its
    # own exclusive time plus the summed exclusive time of its subtree.
    total = node_time(thicket, profile_id, node)
    total = 0.0 if np.isnan(total) else total
    for child in node.children:
        total += subtree_time(thicket, profile_id, child)
    return total

LEAF_METRIC_NAMES = ("use_with", "forall", "sync", "assume")

def accumulate_leaf_metrics(thicket, profile_id, node, totals):
    name = node.frame["name"]
    if name in LEAF_METRIC_NAMES:
        val = node_time(thicket, profile_id, node)
        totals[name] += 0.0 if np.isnan(val) else val
    for child in node.children:
        accumulate_leaf_metrics(thicket, profile_id, child, totals)


def collect_data(thicket):
    profile_id = thicket.dataframe.index.get_level_values("profile")[0]
    data = {}
    dims_seen = set()
    backends_seen = set()
    syncs_seen = set()

    root = list(thicket.graph.roots)[0]
    for cfg_node in root.children:
        parsed = parse_config_name(cfg_node.frame["name"])
        if parsed is None:
            continue
        conv_name, cfg, dim, iters, threads, sync_strategy = parsed
        backend, _, _, _ = cfg

        dims_seen.add(dim)
        syncs_seen.add(sync_strategy)
        if backend is not None:
            backends_seen.add(backend)

        totals = {name: 0.0 for name in LEAF_METRIC_NAMES}
        accumulate_leaf_metrics(thicket, profile_id, cfg_node, totals)

        end_to_end = subtree_time(thicket, profile_id, cfg_node) / iters
        use_with = totals["use_with"] / iters
        forall = totals["forall"] / iters
        data_movement = (totals["sync"] + totals["assume"]) / iters

        data[(cfg, conv_name, dim, sync_strategy)] = {
            "end_to_end": end_to_end,
            "use_with": use_with,
            "forall": forall,
            "data_movement": data_movement,
            "other_processing": max(0.0, end_to_end - (use_with + forall + data_movement)),
            "iters": iters,
            "threads": threads,
        }

    return data, sorted(dims_seen), backends_seen, syncs_seen


def plot_panels(data, dims, cfg_keys, panels, suptitle, path, ylabel, iters_label, per_element=False):
    n = len(panels)
    fig_width = 10

    # ms is the only unit
    scale = 1e3

    panel_data = []
    for _, conv_name, metric in panels:
        raw_ys_by_cfg = []
        panel_values = []
        for cfg in cfg_keys:
            ys = []
            for d in dims:
                v = data.get((cfg, conv_name, d), {}).get(metric, np.nan)
                if not np.isnan(v):
                    if per_element:
                        v = v / (d ** 3)  # dim is per axis, so the mesh has d**3 elements
                    v = v * scale
                ys.append(v)
            raw_ys_by_cfg.append(ys)
            panel_values.extend(v for v in ys if not np.isnan(v))
        panel_data.append((raw_ys_by_cfg, panel_values))

    fig, axes = plt.subplots(n, 1, figsize=(fig_width, 4.2 * n), squeeze=False)
    axes = axes[:, 0]
    suptitle_text = fig.suptitle(f"{suptitle} ({iters_label})", fontsize=20, fontweight="bold")

    for i in range(n):
        ax = axes[i]
        title, conv_name, metric = panels[i]
        raw_ys_by_cfg, panel_values = panel_data[i]

        for j, cfg in enumerate(cfg_keys):
            ys = raw_ys_by_cfg[j]
            ax.plot(
                dims, ys,
                label=format_cfg_label(cfg),
                color=CMAP(j % 10), marker=MARKERS[j % len(MARKERS)],
                linestyle="-" if cfg[2] == "device" else "--",
                markersize=7, linewidth=2.2, alpha=0.9,
            )

        ax.set_xscale("log", base=2)
        ax.set_xticks(dims)
        ax.set_xticklabels([str(d) for d in dims])
        ax.grid(True, which="both", linestyle=":", alpha=0.4)
        ax.set_title(title, fontsize=17)
        ax.tick_params(axis="both", which="major", labelsize=13)

        positive_values = [v for v in panel_values if v > 0]
        if per_element and positive_values:
            # Per-element cost spans many orders of magnitude, so use
            # a log-scale y-axis
            ax.set_yscale("log")
        else:
            # Fixed precision so tick labels are comparable across panels
            ax.yaxis.set_major_formatter(FormatStrFormatter("%.3f"))
            local_max = max(panel_values) if panel_values else 0.0
            scaled_max = local_max * 1.05 if panel_values else 1.0
            ax.set_ylim(bottom=-0.04 * scaled_max, top=scaled_max)

    # One shared x/y title instead of repeating it on every panel
    fig.supxlabel("dim size per axis (mesh is N³)", fontsize=15)
    fig.supylabel(f"{ylabel} (ms)", fontsize=15)

    fig.tight_layout(rect=(0, 0, 1, 0.96))

    # Legend to the right of the subplots, one entry per row.
    handles, labels = axes[0].get_legend_handles_labels()
    legend = fig.legend(handles, labels, loc="center left", bbox_to_anchor=(1.0, 0.5), fontsize=13)

    fig.canvas.draw()
    legend_bbox_fig = legend.get_window_extent().transformed(fig.transFigure.inverted())
    suptitle_text.set_x(legend_bbox_fig.x1 / 2)

    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"saved {path}")


def generate_plots(data, dims, cfg_keys, iters_label, out_dir, suffix="", include_components=True):
    ylabel = "avg time / iter"

    for group_name, group_panels in END_TO_END_GROUP_PANELS:
        plot_panels(
            data, dims, cfg_keys,
            group_panels,
            f"avg {group_name} conversion time vs data size",
            out_dir / f"end_to_end_combined_{group_name}{suffix}.png",
            ylabel,
            iters_label,
        )

    for conv in CONVERSIONS:
        plot_panels(
            data, dims, cfg_keys,
            [(CONVERSION_LABELS[conv], conv, "end_to_end")],
            f"{CONVERSION_LABELS[conv]} avg end-to-end conversion time vs data size",
            out_dir / f"end_to_end_{conv}{suffix}.png",
            ylabel,
            iters_label,
        )

        if include_components:
            component_panels = [(title, conv, metric) for title, metric in COMPONENT_METRICS]
            plot_panels(
                data, dims, cfg_keys,
                component_panels,
                f"{CONVERSION_LABELS[conv]} components vs data size",
                out_dir / f"components_{conv}{suffix}.png",
                ylabel,
                iters_label,
            )


def generate_per_element_plots(data, dims, cfg_keys, iters_label, out_dir, suffix=""):
    ylabel = "avg time / element"

    for group_name, group_panels in END_TO_END_GROUP_PANELS:
        plot_panels(
            data, dims, cfg_keys,
            group_panels,
            f"avg {group_name} time per element vs data size",
            out_dir / f"end_to_end_combined_{group_name}_per_element{suffix}.png",
            ylabel,
            iters_label,
            per_element=True,
        )

    for conv in CONVERSIONS:
        plot_panels(
            data, dims, cfg_keys,
            [(CONVERSION_LABELS[conv], conv, "end_to_end")],
            f"{CONVERSION_LABELS[conv]} avg end-to-end time per element vs data size",
            out_dir / f"end_to_end_{conv}_per_element{suffix}.png",
            ylabel,
            iters_label,
            per_element=True,
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "cali_file", nargs="?",
        help="path to a .cali file",
    )
    args = parser.parse_args()

    cali_file = Path(args.cali_file) if args.cali_file else find_cali_file()
    if not cali_file.exists():
        sys.exit(f"no such file: {cali_file}")

    try:
        thicket = th.Thicket.from_caliperreader(str(cali_file))
    except ReaderError as e:
        sys.exit(f"failed to read {cali_file}: {e}")

    data, dims, backends_seen, syncs_seen = collect_data(thicket)
    if not data:
        sys.exit(f"no benchmark config nodes found in {cali_file}")

    iters_label = format_range_label("n", {entry["iters"] for entry in data.values()})
    threads_seen = {entry["threads"] for entry in data.values() if entry["threads"] is not None}
    if threads_seen:
        iters_label = f"{iters_label}, {format_range_label('threads', threads_seen)}"

    out_dir = Path(cali_file.stem)
    out_dir.mkdir(exist_ok=True)

    serial_only = not backends_seen
    if serial_only:
        print("no backend field found, treating runs as serial-only")

    multi_sync = len(syncs_seen) > 1
    for sync_strategy in sorted(syncs_seen):
        sync_data = {
            (cfg, conv_name, dim): entry
            for (cfg, conv_name, dim, s), entry in data.items()
            if s == sync_strategy
        }
        cfg_keys = sorted({cfg for cfg, _, _ in sync_data})
        sync_suffix = f"_{sync_strategy}" if multi_sync else ""

        generate_plots(
            sync_data, dims, cfg_keys, iters_label, out_dir,
            suffix=sync_suffix,
            include_components=not serial_only,
        )
        generate_per_element_plots(
            sync_data, dims, cfg_keys, iters_label, out_dir,
            suffix=sync_suffix,
        )

        if not serial_only:
            for backend in sorted(backends_seen):
                backend_keys = [cfg for cfg in cfg_keys if cfg[0] == backend]
                if backend_keys:
                    generate_plots(
                        sync_data, dims, backend_keys, iters_label, out_dir,
                        suffix=f"{sync_suffix}_{backend}",
                        include_components=True,
                    )

    print(f"wrote plots to {out_dir}/")

if __name__ == "__main__":
    main()
