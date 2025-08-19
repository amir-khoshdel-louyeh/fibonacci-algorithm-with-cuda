# fibonacci-algorithm-with-cuda

This is a small example of implementing the Fibonacci sequence using both
a **CPU (recursive function)** and a **GPU (CUDA kernel)** to compare execution time.

Although the GPU version here uses only **one thread** (to emulate sequential execution),
this example is useful for understanding CUDA basics and verifying correctness.

---

## Features

- Recursive Fibonacci on CPU
- Recursive Fibonacci on GPU (using a CUDA kernel)
- Measures execution time of CPU vs GPU
- Checks if both implementations return the same results

---

## How to Run

> Make sure CUDA Toolkit and a compatible GPU are installed on your machine.

```bash
# Compile
nvcc app.cu -o fibonacci_cuda

# Run
./fibonacci_cuda
```

---

## Example Output

```
CPU Fibonacci time: 0.00003 s
GPU Fibonacci time: 0.00045 s
Fibonacci results match? Yes
```

---

## Notes

- Recursive Fibonacci is **not efficient**, which is why `N` is set to a small value.
- The purpose of this example is educational, showing how to launch a kernel and transfer data.
- You can later extend this by running multiple threads or generating each number in parallel.

---

## Personal Insight

In this part of the project, we used a recursive method to calculate Fibonacci numbers, which is a good example of a sequential task. Even though this program can be written in a parallel way, we chose to keep it recursive to show how GPUs handle tasks that must be done step by step.

We started from Fibonacci(5) and went up to Fibonacci(35). As shown in the following table, the CPU is always faster than the GPU. When we tried to calculate numbers bigger than 45, the GPU often gave an error or wrong results (considering the CPU is always correct).

In the first two examples (matrix multiplication and sorting), the GPU did better as the size of the problem increased. This might make it seem like GPUs are always faster — but that’s not true. The Fibonacci example shows that GPUs are not good at running tasks that depend on previous steps, like recursive functions.

While GPUs are great for parallel tasks, CPUs are better for sequential tasks like this. This raised a question for me: **Why not run everything in parallel?**
The answer is simple: while parallel computing offers great speedups for many problems, not every task can be done in parallel. Some tasks depend on the results of previous steps and must be performed one after another — these are called **sequential tasks**.

---

## Author

Amir Khoshdel Louyeh
# fibonacci-algorithm-with-cuda
