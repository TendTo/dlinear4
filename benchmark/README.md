# Benchmarks

## Setup and configuration

Fist of all, dLinear must be able to compile correctly. Follow the instruction in the main [Readme](../README.md).

The benchmark program will consider all the _.smt2_ files in the _smt2_ folder.

To configure the parameters used for the benchmarks, edit the _benchmark.conf_ file:

```conf
precision = 0.1 0.00001 0
solver = qsoptex
```

It is possible to list any number of value for each parameter, provided they make sense.
All values are separated by space.  
The benchmark program will run every possible combination of parameters on each input _.smt2_ file.

## Running the benchmarks

Bazel is used to run the benchmarks:

```shell
BENCHMARK_OUT="path/to/benchmark.json" BENCHMARK_OUT_FORMAT=json bazel run //benchmark
```

> TODO: add soplex support

## Enriching the benchmarks

After running the benchmarks, the python3 script _benchmark_parser.py_ must be used in order to create a _.csv_ file with all the useful information.

```shell
python3 benchmark_parser.py path/to/benchmark.json path/to/benchmark.csv
```

> The python script uses the Z3 library to parse the _.smt2_ files and enrich the output. If Z3 is not installed, the script will fail.

The headers of the _.csv_ file are:

- file: the name of the input _.smt2_ file
- solver: the name of the solver used (qsoptex or soplex)
- assertions: the number of assertions in the input file
- precision: the precision used for the benchmark
- realTime: the time taken by the solver to solve the problem
- cpuTime: the cpu time taken by the solver to solve the problem
- timeUnit: the unit of time used for the benchmark (ms, s, ...)
- iterations: the number of iterations performed by the solver to calculate the timing
- result: the result of the solver (sat, unsat)
- delta: the delta used to achieve the result, when sat

## Using the benchmarks in LaTex

It is possible to quickly create a table in a LaTex document with the results of the benchmarks.
To do so, include the package _csvsimple_ in the preamble of the document:

```latex
\usepackage{csvsimple}
```

This is an example of a table:

```latex
\begin{tabular}{|l|l|l|l|l|l|l|l|l|}
    \bfseries File & \bfseries Solver & \bfseries Assertions & \bfseries Result & \bfseries delta & \bfseries Precision & \bfseries Time (ms) & \bfseries Iterations
    \csvreader[head to column names]{path/to/benchmark.csv}{}
    { \\\hline \file & \solver & \assertions & \result & \delta & \precision & \cpuTime & \iterations }
\end{tabular}
```
