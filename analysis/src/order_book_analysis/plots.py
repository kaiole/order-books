from pathlib import Path
from typing import Iterable

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.figure import Figure

from .metadata import RunMetadata, discover_runs, filter_runs
from .read_raw import RawFile, read_raw
from .stats import EVENT_TYPE_NAMES, PERCENTILES, _percentile_key, summarize_runs

IMPL_COLORS: dict[str, str] = {
    "Deque": "#1f77b4",
    "List": "#ff7f0e",
    "DequeIter": "#2ca02c",
    "ListIter": "#d62728",
    "ListPtr": "#8c564b",
    "DequeFat": "#9467bd",
}

EVENT_TYPE_ORDER = ("all", "add", "modify", "cancel")
HIST_BINS = 200
HIST_ALPHA = 0.5
HIST_XLIMIT_PERCENTILE = 99.0


def _color_for(impl: str) -> str:
    return IMPL_COLORS.get(impl, "#7f7f7f")


def _filter_event(records: np.ndarray, event_type: str) -> np.ndarray:
    if event_type == "all":
        return records["latency_ns"]

    name_to_raw = {v: int(k) for k, v in EVENT_TYPE_NAMES.items()}
    if event_type not in name_to_raw:
        raise ValueError(f"unknown event_type: {event_type}")

    mask = records["event_type"] == name_to_raw[event_type]
    return records["latency_ns"][mask]


def _latest_run_per_impl(
    pairs: list[tuple[RunMetadata, Path]],
) -> dict[str, tuple[RunMetadata, RawFile]]:
    by_impl: dict[str, tuple[RunMetadata, Path]] = {}
    for meta, raw_path in pairs:
        impl = meta.run.impl
        if impl not in by_impl or meta.run.run_id > by_impl[impl][0].run.run_id:
            by_impl[impl] = (meta, raw_path)

    return {impl: (meta, read_raw(raw_path))
            for impl, (meta, raw_path) in by_impl.items()}


def _format_mix_suffix(
    latest: dict[str, tuple[RunMetadata, RawFile]],
) -> str:
    mixes = {
        impl: (meta.workload.add_ratio,
               meta.workload.modify_ratio,
               meta.workload.cancel_ratio)
        for impl, (meta, _) in latest.items()
    }
    unique = set(mixes.values())
    if len(unique) > 1:
        raise ValueError(f"workload mix differs across impls: {mixes}")

    add, mod, cancel = next(iter(unique))
    return f"(add {add:.0%} / modify {mod:.0%} / cancel {cancel:.0%})"


def plot_histogram(
    samples_by_impl: dict[str, np.ndarray],
    event_type: str,
    xlimit: tuple[float, float] | None = None,
    title_suffix: str | None = None,
) -> Figure:
    fig, axis = plt.subplots(figsize=(9, 6))

    if xlimit is None and samples_by_impl:
        upper = max(np.percentile(s, HIST_XLIMIT_PERCENTILE)
                    for s in samples_by_impl.values() if s.size)
        xlimit = (0.0, float(upper))

    for impl, samples in samples_by_impl.items():
        if samples.size == 0:
            continue
        color = _color_for(impl)
        median = float(np.median(samples))
        axis.hist(samples, bins=HIST_BINS, range=xlimit, alpha=HIST_ALPHA,
                  color=color, label=impl)
        axis.axvline(median, color=color, linestyle="--",
                     label=f"{impl} median: {median:.0f} ns")

    title = f"OrderBook Latency Distribution — {event_type}"
    if title_suffix:
        title += f"\n{title_suffix}"
    axis.set_title(title)
    axis.set_xlabel("Latency (ns)")
    axis.set_ylabel("Frequency")
    if xlimit is not None:
        axis.set_xlim(xlimit)
    axis.legend()
    fig.tight_layout()
    return fig


def plot_cdf(
    samples_by_impl: dict[str, np.ndarray],
    event_type: str,
) -> Figure:
    fig, ax = plt.subplots(figsize=(9, 6))

    for impl, samples in samples_by_impl.items():
        if samples.size == 0:
            continue
        sorted_s = np.sort(samples)
        ranks = np.arange(1, sorted_s.size + 1) / sorted_s.size
        survival = 1.0 - ranks
        survival = np.clip(survival, 1.0 / sorted_s.size, None)
        ax.plot(sorted_s, survival, color=_color_for(impl), label=impl)

    ax.set_yscale("log")
    ax.set_xscale("log")
    ax.set_title(f"OrderBook Latency Tail (1 - CDF) — {event_type}")
    ax.set_xlabel("Latency (ns)")
    ax.set_ylabel("P(latency > x)")
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    return fig


def plot_percentiles(df: pd.DataFrame) -> Figure:
    plot_df = df[df["event_type"] != "all"].copy()
    event_types = [
        e for e in EVENT_TYPE_ORDER if e in plot_df["event_type"].unique()]
    impls = sorted(plot_df["impl"].unique())

    fig, axes = plt.subplots(1, len(PERCENTILES), figsize=(4 * len(PERCENTILES), 5),
                             sharey=False)
    if len(PERCENTILES) == 1:
        axes = [axes]

    bar_width = 0.8 / max(len(impls), 1)
    x = np.arange(len(event_types))

    for ax, p in zip(axes, PERCENTILES):
        key = _percentile_key(p)
        for i, impl in enumerate(impls):
            values = [
                float(plot_df[(plot_df["impl"] == impl) &
                              (plot_df["event_type"] == ev)][key].iloc[0])
                if not plot_df[(plot_df["impl"] == impl) &
                               (plot_df["event_type"] == ev)].empty
                else 0.0
                for ev in event_types
            ]
            offset = (i - (len(impls) - 1) / 2) * bar_width
            ax.bar(x + offset, values, width=bar_width,
                   color=_color_for(impl), label=impl)
        ax.set_title(f"p{p}")
        ax.set_xticks(x)
        ax.set_xticklabels(event_types)
        ax.set_ylabel("Latency (ns)")

    axes[-1].legend()
    fig.suptitle("OrderBook Latency Percentiles by Event Type")
    fig.tight_layout()
    return fig


def _selected_runs(
    pairs: list[tuple[RunMetadata, Path]],
) -> dict[str, tuple[RunMetadata, RawFile]]:
    by_impl: dict[str, tuple[RunMetadata, Path]] = {}
    for meta, raw_path in pairs:
        existing = by_impl.get(meta.run.impl)
        if existing is not None and existing[0].run.run_id != meta.run.run_id:
            raise ValueError(
                f"multiple runs for impl {meta.run.impl!r} in selection "
                f"({existing[0].run.run_id}, {meta.run.run_id}); "
                f"narrow with --run-id")
        by_impl[meta.run.impl] = (meta, raw_path)

    return {impl: (meta, read_raw(raw_path))
            for impl, (meta, raw_path) in by_impl.items()}


def plot_all(results_dir: str | Path, out_dir: str | Path,
             xlimit: tuple[float, float] | None = None,
             impls: Iterable[str] | None = None,
             run_ids: Iterable[str] | None = None) -> list[Path]:
    results_dir = Path(results_dir)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    pairs = discover_runs(results_dir)
    if not pairs:
        raise FileNotFoundError(f"no runs in {results_dir}")

    has_filter = any(x is not None for x in (impls, run_ids))
    if has_filter:
        pairs = filter_runs(pairs, impls=impls, run_ids=run_ids)
        if not pairs:
            raise FileNotFoundError(
                f"no runs in {results_dir} matched filters "
                f"(impls={impls}, run_ids={run_ids})")
        latest = _selected_runs(pairs)
    else:
        latest = _latest_run_per_impl(pairs)
    summary_df = summarize_runs(
        [(meta, results_dir / meta.raw.file) for meta, _ in latest.values()])

    mix_suffix = _format_mix_suffix(latest)

    written: list[Path] = []

    for event_type in EVENT_TYPE_ORDER:
        samples_by_impl = {
            impl: _filter_event(raw.records, event_type)
            for impl, (_, raw) in latest.items()
        }

        if not any(s.size for s in samples_by_impl.values()):
            continue

        hist_suffix = mix_suffix if event_type == "all" else None
        hist_fig = plot_histogram(samples_by_impl, event_type, xlimit=xlimit,
                                  title_suffix=hist_suffix)
        hist_path = out_dir / f"hist_{event_type}.png"
        hist_fig.savefig(hist_path, dpi=150)
        plt.close(hist_fig)
        written.append(hist_path)

        cdf_fig = plot_cdf(samples_by_impl, event_type)
        cdf_path = out_dir / f"cdf_{event_type}.png"
        cdf_fig.savefig(cdf_path, dpi=150)
        plt.close(cdf_fig)
        written.append(cdf_path)

    pct_fig = plot_percentiles(summary_df)
    pct_path = out_dir / "percentiles.png"
    pct_fig.savefig(pct_path, dpi=150)
    plt.close(pct_fig)
    written.append(pct_path)

    return written
