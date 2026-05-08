import argparse
import sys
from pathlib import Path

from .plots import plot_all
from .stats import summarize_dir


def _summarize(args: argparse.Namespace) -> int:
    df = summarize_dir(args.results_dir)
    if df.empty:
        print(f"no runs found in {args.results_dir}", file=sys.stderr)
        return 1

    if args.out is None:
        print(df.to_string(index=False))
    else:
        out = Path(args.out)
        out.parent.mkdir(parents=True, exist_ok=True)
        df.to_csv(out, index=False)
        print(f"wrote {out}")
    return 0


def _plot(args: argparse.Namespace) -> int:
    xlimit: tuple[float, float] | None = None
    if args.xlimit is not None:
        lo, hi = args.xlimit.split(",")
        xlimit = (float(lo), float(hi))

    written = plot_all(args.results_dir, args.out, xlimit=xlimit)
    for path in written:
        print(f"wrote {path}")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="order-book-analyze")
    sub = parser.add_subparsers(dest="cmd", required=True)

    summarize = sub.add_parser(
        "summarize", help="compute percentile table for runs in a results dir")
    summarize.add_argument("results_dir", type=Path)
    summarize.add_argument("--out", type=Path, default=None,
                             help="write CSV to this path (default: stdout)")
    summarize.set_defaults(func=_summarize)

    plot = sub.add_parser(
        "plot", help="generate latency plots for runs in a results dir")
    plot.add_argument("results_dir", type=Path)
    plot.add_argument("--out", type=Path, required=True,
                        help="output directory for plot images")
    plot.add_argument("--xlimit", type=str, default=None,
                        help="histogram xlimit as 'lo,hi' in ns (default: auto p99)")
    plot.set_defaults(func=_plot)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
