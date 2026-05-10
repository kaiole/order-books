import argparse
import sys
from pathlib import Path

from .plots import plot_all
from .stats import summarize_dir


def _summarize(args: argparse.Namespace) -> int:
    df = summarize_dir(args.results_dir, impls=args.impl,
                       run_ids=args.run_id)
    if df.empty:
        print(f"no runs found in {args.results_dir} "
              f"(impls={args.impl}, run_ids={args.run_id})", file=sys.stderr)
        return 1

    df = df[df["n"] > 0].reset_index(drop=True)

    if args.out is None:
        print(df.to_string(index=False))
        return 0

    out = Path(args.out)
    if out.is_dir():
        out = out / "summary.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(out, index=False)
    print(f"wrote {out}")
    return 0


def _plot(args: argparse.Namespace) -> int:
    xlimit: tuple[float, float] | None = None
    if args.xlimit is not None:
        parts = args.xlimit.split(",")
        if len(parts) != 2:
            print(f"--xlimit must be 'lo,hi' (got {args.xlimit!r})",
                  file=sys.stderr)
            return 2
        try:
            xlimit = (float(parts[0]), float(parts[1]))
        except ValueError:
            print(f"--xlimit values must be numeric (got {args.xlimit!r})",
                  file=sys.stderr)
            return 2

    out = Path(args.out)
    if out.exists() and not out.is_dir():
        print(f"--out must be a directory (exists as file: {out})",
              file=sys.stderr)
        return 2

    written = plot_all(args.results_dir, out, xlimit=xlimit,
                       impls=args.impl, run_ids=args.run_id)
    for path in written:
        print(f"wrote {path}")
    return 0


def _add_filter_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--impl", action="append", default=None,
                        help="filter by impl (repeatable; OR within flag)")
    parser.add_argument("--run-id", action="append", default=None,
                        help="filter by run_id (repeatable; OR within flag)")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="order-book-analyze")
    sub = parser.add_subparsers(dest="cmd", required=True)

    summarize = sub.add_parser(
        "summarize", help="compute percentile table for runs in a results dir")
    summarize.add_argument("results_dir", type=Path)
    summarize.add_argument("--out", type=Path, default=None,
                             help="write CSV to this path (default: stdout)")
    _add_filter_args(summarize)
    summarize.set_defaults(func=_summarize)

    plot = sub.add_parser(
        "plot", help="generate latency plots for runs in a results dir")
    plot.add_argument("results_dir", type=Path)
    plot.add_argument("--out", type=Path, required=True,
                        help="output directory for plot images")
    plot.add_argument("--xlimit", type=str, default=None,
                        help="histogram xlimit as 'lo,hi' in ns (default: auto p99)")
    _add_filter_args(plot)
    plot.set_defaults(func=_plot)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
