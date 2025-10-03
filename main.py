"""Primary Python entry point for the Fibonacci CUDA GUI."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


def _find_native_executable() -> str | None:
    """Find the compiled C++/CUDA GUI used by this Python entry point."""
    configured_path = os.environ.get("FIBONACCI_GUI_BINARY")
    if configured_path:
        return configured_path

    project_root = Path(__file__).resolve().parent
    for candidate in (
        project_root / "build" / "fibonacci_gui",
    ):
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return str(candidate)

    return shutil.which("fibonacci_gui")


def main() -> int:
    """Start the native Qt GUI."""
    executable = _find_native_executable()
    if executable is None:
        print(
            "Error: fibonacci_gui was not found. Build the GUI first with "
            "'cmake --preset release && cmake --build --preset release', or "
            "set FIBONACCI_GUI_BINARY.",
            file=sys.stderr,
        )
        return 1

    try:
        completed = subprocess.run([executable], check=False)
    except OSError as error:
        print(f"Error: unable to run {executable}: {error}", file=sys.stderr)
        return 1
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
