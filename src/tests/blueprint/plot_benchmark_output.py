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

CONVERSIONS = [
    "rectilinear_to_explicit",
    "uniform_to_explicit",
    "uniform_to_rectilinear",
]

CONVERSION_LABELS = {
    "rectilinear_to_explicit": "rect→explicit",
    "uniform_to_explicit": "uniform→explicit",
    "uniform_to_rectilinear": "uniform→rect",
}

CMAP = plt.get_cmap("tab10")
MARKERS = ["o", "s", "^", "D", "v", "P", "X", "*"]


def parse_config_name(name):
    conv_name = None
    for c in CONVERSIONS:
        if name.startswith(c + "_"):
            conv_name = c
            break
    if conv_name is None:
        return None

    parts = name[len(conv_name) + 1 :].split("_")
    if len(parts) != 6:
        return None

    backend = parts[0]
    fields = parts[1:]
    values = {}
    for field in fields:
        if "-" not in field:
            return None
        key, value = field.split("-", 1)
        values[key] = value

    if set(values) != {"dim", "src", "exec", "out", "iter"}:
        return None
    if values["src"] not in ("host", "device"):
        return None
    if values["exec"] not in ("host", "device"):
        return None
    if values["out"] not in ("host", "device"):
        return None
    if not values["dim"].isdigit() or not values["iter"].isdigit():
        return None

    cfg = (backend, values["src"], values["exec"], values["out"])
    return conv_name, cfg, int(values["dim"]), int(values["iter"])


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
    for i, f in enumerate(cali_files):
        print(f"  {f.name}" + ("  <- using this one" if i == 0 else ""))
    return cali_files[0]


def node_time(thicket, profile_id, node):
    try:
        return thicket.dataframe.loc[(node, profile_id), "time"]
    except KeyError:
        return float("nan")


def subtree_time(thicket, profile_id, node):
    # Caliper measures exclusive time, so a region's inclusive time
    # is its own time plus the summed time of its subtree
    total = node_time(thicket, profile_id, node)
    total = 0.0 if np.isnan(total) else total
    for child in node.children:
        total += subtree_time(thicket, profile_id, child)
    return total


def collect_data(thicket):
    profile_id = thicket.dataframe.index.get_level_values("profile")[0]
    data = {}
    dims_seen = set()

    root = list(thicket.graph.roots)[0]
    for cfg_node in root.children:
        parsed = parse_config_name(cfg_node.frame["name"])
        if parsed is None:
            continue
        conv_name, cfg, dim, iters = parsed
        dims_seen.add(dim)

        forall_t = np.nan
        sync_acc = 0.0
        usewith_acc = 0.0
        for to_node in cfg_node.children:
            for coordset_node in to_node.children:
                for leaf in coordset_node.children:
                    val = node_time(thicket, profile_id, leaf)
                    val = 0.0 if np.isnan(val) else val
                    name = leaf.frame["name"]
                    if name == "forall":
                        forall_t = val
                    elif name == "sync":
                        sync_acc += val
                    elif name == "use_with":
                        usewith_acc += val

        data[(cfg, conv_name, dim)] = {
            "inclusive": subtree_time(thicket, profile_id, cfg_node) / iters,
            "forall": forall_t / iters,
            "sync": sync_acc / iters,
            "use_with": usewith_acc / iters,
        }

    return data, sorted(dims_seen)


def plot_panels(data, dims, cfg_keys, panels, suptitle, path, ymax):
    fig, axes = plt.subplots(1, len(panels), figsize=(6 * len(panels) + 1, 5), squeeze=False)
    axes = axes[0]
    fig.suptitle(suptitle, fontsize=14, fontweight="bold")

    for i in range(len(panels)):
        ax = axes[i]
        title, conv_name, metric = panels[i]

        for j, cfg in enumerate(cfg_keys):
            ys = []
            for d in dims:
                entry = data.get((cfg, conv_name, d), {})
                ys.append(entry.get(metric, np.nan))
            ax.plot(
                dims, ys,
                label=f"{cfg[0]}: {cfg[1]}→{cfg[2]}→{cfg[3]}",
                color=CMAP(j % 10), marker=MARKERS[j % len(MARKERS)],
                linestyle="-" if cfg[2] == "device" else "--",
                markersize=5, linewidth=1.6, alpha=0.9,
            )

        dim_labels = []
        for d in dims:
            dim_labels.append(str(d))

        ax.set_xscale("log", base=2)
        ax.set_xticks(dims)
        ax.set_xticklabels(dim_labels)
        ax.set_xlabel("dim size per axis (mesh is N³)", fontsize=9)
        ax.grid(True, which="both", linestyle=":", alpha=0.4)
        ax.set_title(title, fontsize=11)
        ax.set_ylabel("avg time / iter (s)", fontsize=10)
        ax.set_ylim(bottom=-0.04 * ymax, top=ymax)

    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=4, fontsize=9, bbox_to_anchor=(0.5, -0.12))
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"saved {path}")


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

    data, dims = collect_data(thicket)
    if not data:
        sys.exit(f"no benchmark config nodes found in {cali_file}")

    cfg_set = set()
    for cfg, conv_name, dim in data:
        cfg_set.add(cfg)
    cfg_keys = sorted(cfg_set)

    all_values = []
    for metrics in data.values():
        for v in metrics.values():
            if not np.isnan(v):
                all_values.append(v)
    ymax = max(all_values) * 1.05

    out_dir = Path(cali_file.stem)
    out_dir.mkdir(exist_ok=True)

    combined_panels = []
    for conv in CONVERSIONS:
        combined_panels.append((CONVERSION_LABELS[conv], conv, "inclusive"))

    plot_panels(
        data, dims, cfg_keys,
        combined_panels,
        "avg conversion time vs data size",
        out_dir / "inclusive_combined.png",
        ymax,
    )

    for conv in CONVERSIONS:
        plot_panels(
            data, dims, cfg_keys,
            [(CONVERSION_LABELS[conv], conv, "inclusive")],
            f"{CONVERSION_LABELS[conv]} avg conversion time vs data size",
            out_dir / f"inclusive_{conv}.png",
            ymax,
        )

        component_panels = []
        for metric in ("forall", "sync", "use_with"):
            component_panels.append((metric, conv, metric))

        plot_panels(
            data, dims, cfg_keys,
            component_panels,
            f"{CONVERSION_LABELS[conv]} components vs data size",
            out_dir / f"components_{conv}.png",
            ymax,
        )

    print(f"wrote plots to {out_dir}/")


if __name__ == "__main__":
    main()
