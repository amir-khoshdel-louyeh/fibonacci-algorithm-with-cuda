# Fibonacci Algorithm with CUDA

This project benchmarks a recursive Fibonacci implementation on the CPU and
GPU. The GPU version intentionally uses one thread so the example demonstrates
CUDA execution and transfer overhead for a sequential workload.

## Features

- Recursive Fibonacci on CPU and GPU
- Configurable number of terms
- Benchmark sweeps across multiple input sizes
- CPU/GPU timing and result validation
- CSV timing export for charting
- Clear CUDA error reporting

## Requirements

- CUDA Toolkit with `nvcc`
- A CUDA-capable GPU

## Build and run

The recommended build uses CMake:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/fibonacci_cuda
```

The direct `nvcc` command is also supported:

```bash
nvcc fibonacci-algorithm-with-cuda.cu -o fibonacci_cuda
./fibonacci_cuda
```

Run a larger benchmark and export timing data:

```bash
./fibonacci_cuda --count 30 --csv benchmark.csv

# Sweep from 5 through 30 terms and export chart-ready data
./fibonacci_cuda --sweep 5 30 --csv sweep.csv
```

Use `./fibonacci_cuda --help` to see all options. The program accepts 1 to 47
terms; the upper bound prevents signed 32-bit integer overflow.

## Example output

```text
Fibonacci benchmark (5 terms)
CPU time: 0.000001200 s
GPU time: 0.000423100 s
Results match: Yes
```

The optional CSV output contains one row per implementation:

```csv
implementation,count,duration_seconds,results_match
cpu,30,0.0000123,true
gpu,30,0.0004512,true
```

Sweep exports include a `results_match` column and one CPU and GPU row for
every input size, making them suitable for plotting directly.

## Notes

Recursive Fibonacci is intentionally inefficient. This project is educational:
the one-thread GPU implementation makes the sequential nature of the algorithm
explicit rather than claiming that this workload should benefit from GPU
parallelism.

## Author

Amir Khoshdel Louyeh
