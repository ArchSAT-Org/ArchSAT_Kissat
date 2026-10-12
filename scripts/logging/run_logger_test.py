#!/usr/bin/env python3
"""Run the instrumented solver on the benchmarks you name.

The solver writes 'log/log-output.txt' relative to its working directory, so it
runs inside its own tree and every run is filed under '<tree>/log/<instance>/'.
Instances run one at a time because they share that one live log file.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

SOLVER_TREE = Path(__file__).resolve().parents[2]
DEFAULT_SOLVER = SOLVER_TREE / "build" / "kissat"
DEFAULT_INSTANCES = ("ezunsat8",)

# Keep in sync with 'LOG_PATH' in 'src/logfile.c'.
LIVE_LOG = Path("log") / "log-output.txt"


def solver_tree(solver: Path) -> Path:
    """The tree the solver was built in, 'ArchSAT_Kissat' for 'build/kissat'."""
    for parent in solver.resolve().parents:
        if (parent / "src").is_dir():
            return parent
    return solver.resolve().parent


def run(solver: Path, cnf: Path, root: Path, timeout: int, conflicts: int) -> None:
    run_dir = root / LIVE_LOG.parent / cnf.stem
    shutil.rmtree(run_dir, ignore_errors=True)
    run_dir.mkdir(parents=True)
    # The solver always writes to the same path, so drop any stale log first.
    live_log = root / LIVE_LOG
    live_log.unlink(missing_ok=True)

    # '--profile=4' is the level at which propagation is timed for 'BCP run time'.
    command = [str(solver), "--profile=4"]
    if conflicts:
        command.append(f"--conflicts={conflicts}")
    command.append(str(cnf.resolve()))
    with (run_dir / "solver.out").open("wb") as stdout:
        try:
            code = subprocess.run(
                command,
                cwd=root,
                stdout=stdout,
                stderr=subprocess.STDOUT,
                timeout=timeout,
            ).returncode
            if code == 10:
                status = "SATISFIABLE"
            elif code == 20:
                status = "UNSATISFIABLE"
            elif code == 0 and conflicts:
                status = f"stopped at {conflicts} conflicts"
            else:
                status = f"exit {code}"
        except subprocess.TimeoutExpired:
            status = f"timeout after {timeout}s"

    log_path = run_dir / live_log.name
    if live_log.exists():
        shutil.move(str(live_log), str(log_path))
    print(f"{cnf.name}: {status}, log in {log_path}")


def find_instance(benchmarks: Path, name: str) -> Path:
    """A path is used as given. A bare name is looked up in the directory."""
    path = Path(name)
    if path.is_file():
        return path
    for candidate in (benchmarks / name, benchmarks / f"{name}.cnf"):
        if candidate.is_file():
            return candidate
    sys.exit(f"error: no benchmark '{name}' in {benchmarks}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add = parser.add_argument
    add("benchmarks", help="directory holding the *.cnf files")
    add("instances", nargs="*",
        help="benchmark names to run, e.g. ezunsat8 ezunsat9 (default: ezunsat8)")
    add("--solver", default=str(DEFAULT_SOLVER), help="instrumented solver binary")
    add("--timeout", type=int, default=120, help="per-instance limit in seconds")
    add("--conflicts", type=int, default=100_000,
        help="stop the search after this many conflicts; 0 solves without a limit")
    args = parser.parse_args()

    solver = Path(args.solver)
    if not solver.is_file():
        sys.exit(f"error: no solver at {solver}, build it first")
    benchmarks = Path(args.benchmarks)
    if not benchmarks.is_dir():
        sys.exit(f"error: no benchmark directory at {benchmarks}")
    names = args.instances or list(DEFAULT_INSTANCES)
    instances = [find_instance(benchmarks, name) for name in names]

    root = solver_tree(solver)
    for cnf in instances:
        run(solver, cnf, root, args.timeout, args.conflicts)
    return 0


if __name__ == "__main__":
    sys.exit(main())
