# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.1.0] - 2026-11-13

### Added

- Qt 6 desktop application with single-count and sweep modes.
- Background benchmark worker with progress reporting and cancellation.
- Results table and interactive execution-time and speedup charts with a
  logarithmic time axis.
- CSV and JSON export, CSV session loading, and PNG chart export.
- Versioned CSV and JSON data formats, including import of legacy CSV files.
- Benchmark warmup, median-of-iterations timing, and CUDA event kernel timing.
- CUDA device information reporting.
- Core, I/O, CLI, and GUI smoke tests.
- GitHub Actions CI and CMake presets.
- Install rules, desktop entry, application icon, and CPack packaging.
- Requirements documentation.
