from pathlib import Path
from typing import Iterable

import numpy as np
import pandas as pd

from .read_raw import EventType, RawFile, read_raw
from .metadata import RunMetadata, discover_runs

PERCENTILES = (50.0, 90.0, 99.0, 99.9)

EVENT_TYPE_NAMES = {
    EventType.ADD: "add",
    EventType.MODIFY: "modify",
    EventType.CANCEL: "cancel",
}


def _percentile_key(p: float) -> str:
    return f"p{p}".replace(".", "_")


def summarize(latencies: np.ndarray) -> dict[str, float]:
    if latencies.size == 0:
        return {
            "n": 0,
            "mean_ns": float("nan"),
            "max_ns": float("nan"),
            **{_percentile_key(p): float("nan") for p in PERCENTILES},
        }

    row: dict[str, float] = {
        "n": int(latencies.size),
        "max_ns": float(latencies.max()),
    }

    percentiles = np.percentile(latencies, PERCENTILES)
    for p, v in zip(PERCENTILES, percentiles):
        row[_percentile_key(p)] = float(v)

    return row


def _row_for(meta: RunMetadata, event_type: str, latencies: np.ndarray) -> dict:
    return {
        "run_id": meta.run.run_id,
        "impl": meta.run.impl,
        "scenario": meta.run.scenario,
        "git_commit": meta.build.git_commit,
        "event_type": event_type,
        **summarize(latencies),
    }


def summarize_run(meta: RunMetadata, raw: RawFile) -> pd.DataFrame:
    latencies = raw.records["latency_ns"]
    event_types = raw.records["event_type"]

    rows = [_row_for(meta, "all", latencies)]
    for raw_type, event_type in EVENT_TYPE_NAMES.items():
        mask = event_types == int(raw_type)
        rows.append(_row_for(meta, event_type, latencies[mask]))

    return pd.DataFrame(rows)


def summarize_runs(pairs: Iterable[tuple[RunMetadata, Path]]) -> pd.DataFrame:
    frames = []
    for meta, raw_path in pairs:
        raw = read_raw(raw_path)
        frames.append(summarize_run(meta, raw))

    if not frames:
        return pd.DataFrame()

    return pd.concat(frames, ignore_index=True)


def summarize_dir(results_dir: str | Path) -> pd.DataFrame:
    return summarize_runs(discover_runs(results_dir))
