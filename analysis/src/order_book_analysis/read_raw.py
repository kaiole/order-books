from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path

import numpy as np

MAGIC = b"ORDBOOK\x00"
VERSION = 1
RECORD_SIZE = 16
HEADER_SIZE = 24

HEADER_DTYPE = np.dtype(
    [
        ("magic", "V8"),
        ("version", "<u4"),
        ("record_size", "<u4"),
        ("record_count", "<u8"),
    ]
)

RECORD_DTYPE = np.dtype(
    [
        ("latency_ns", "<i8"),
        ("event_type", "u1"),
        ("pad", "7u1"),
    ]
)


class EventType(IntEnum):
    ADD = 0
    MODIFY = 1
    CANCEL = 2


@dataclass(frozen=True)
class RawFile:
    path: Path
    version: int
    record_count: int
    records: np.ndarray


def read_raw(path: str | Path) -> RawFile:
    path = Path(path)
    raw = np.fromfile(path, dtype=np.uint8)

    if raw.size < HEADER_SIZE:
        raise ValueError(
            f"{path}: file smaller than header ({raw.size} bytes)")

    header = raw[:HEADER_SIZE].view(HEADER_DTYPE)[0]

    if bytes(header["magic"]) != MAGIC:
        raise ValueError(f"{path}: bad magic {bytes(header['magic'])!r}")
    if int(header["version"]) != VERSION:
        raise ValueError(
            f"{path}: unsupported version {int(header['version'])}")
    if int(header["record_size"]) != RECORD_SIZE:
        raise ValueError(
            f"{path}: record_size {int(header['record_size'])} != {RECORD_SIZE}"
        )

    count = int(header["record_count"])
    expected = HEADER_SIZE + count * RECORD_SIZE
    if raw.size != expected:
        raise ValueError(
            f"{path}: size {raw.size} != expected {expected} for {count} records"
        )

    records = raw[HEADER_SIZE:].view(RECORD_DTYPE)

    return RawFile(
        path=path,
        version=int(header["version"]),
        record_count=count,
        records=records,
    )
