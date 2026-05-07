import json
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class RunInfo:
    run_id: str
    timestamp: str
    scenario: str
    impl: str


@dataclass(frozen=True)
class WorkloadInfo:
    event_count: int
    warmup_count: int
    seed: int
    add_ratio: float
    modify_ratio: float
    cancel_ratio: float
    mid_price: int
    max_price_offset: int
    min_qty: int
    max_qty: int


@dataclass(frozen=True)
class BuildInfo:
    compiler: str
    compiler_version: str
    build_type: str
    build_flags: str
    cxx_standard: str
    git_commit: str


@dataclass(frozen=True)
class SystemInfo:
    hostname: str
    cpu_model: str
    cpu_governor: str


@dataclass(frozen=True)
class RawInfo:
    file: str
    record_count: int
    record_size_bytes: int
    format: str


@dataclass(frozen=True)
class RunMetadata:
    path: Path
    run: RunInfo
    workload: WorkloadInfo
    build: BuildInfo
    system: SystemInfo
    raw: RawInfo


def read_metadata(path: str | Path) -> RunMetadata:
    path = Path(path)
    with path.open() as f:
        data = json.load(f)

    run = data["run"]
    workload = data["workload"]
    build = data["build"]
    system = data["system"]
    raw = data["raw"]

    return RunMetadata(
        path=path,
        run=RunInfo(
            run_id=run["runId"],
            timestamp=run["timestamp"],
            scenario=run["scenario"],
            impl=run["impl"],
        ),
        workload=WorkloadInfo(
            event_count=workload["eventCount"],
            warmup_count=workload["warmupCount"],
            seed=workload["seed"],
            add_ratio=workload["addRatio"],
            modify_ratio=workload["modifyRatio"],
            cancel_ratio=workload["cancelRatio"],
            mid_price=workload["midPrice"],
            max_price_offset=workload["maxPriceOffset"],
            min_qty=workload["minQty"],
            max_qty=workload["maxQty"],
        ),
        build=BuildInfo(
            compiler=build["compiler"],
            compiler_version=build["compilerVersion"],
            build_type=build["buildType"],
            build_flags=build["buildFlags"],
            cxx_standard=build["cxxStandard"],
            git_commit=build["gitCommit"],
        ),
        system=SystemInfo(
            hostname=system["hostname"],
            cpu_model=system["cpuModel"],
            cpu_governor=system["cpuGovernor"],
        ),
        raw=RawInfo(
            file=raw["file"],
            record_count=raw["recordCount"],
            record_size_bytes=raw["recordSizeBytes"],
            format=raw["format"],
        ),
    )


def discover_runs(results_dir: str | Path) -> list[tuple[RunMetadata, Path]]:
    results_dir = Path(results_dir)
    pairs: list[tuple[RunMetadata, Path]] = []

    for meta_path in sorted(results_dir.glob("meta_*.json")):
        meta = read_metadata(meta_path)
        raw_path = results_dir / meta.raw.file
        if not raw_path.exists():
            raise FileNotFoundError(
                f"raw file missing for {meta_path}: {raw_path}")
        pairs.append((meta, raw_path))

    return pairs
