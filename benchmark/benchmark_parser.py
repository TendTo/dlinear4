#!/usr/bin/env python3
# pylint: disable=missing-module-docstring,missing-function-docstring,missing-class-docstring,invalid-name
import json
import csv
import sys
import re
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from typing import TypedDict, Literal

    class Benchmark(TypedDict):
        name: str
        family_index: int
        per_family_instance_index: int
        run_name: str
        run_type: str
        repetitions: int
        repetition_index: int
        threads: int
        iterations: int
        real_time: float
        cpu_time: float
        time_unit: str

    class BenchmarkRow(TypedDict):
        file: str
        solver: str
        assertions: int
        precision: float
        actualPrecision: float
        realTime: float
        cpuTime: float
        timeUnit: str
        iterations: int
        result: str

    BenchmarkFormat = Literal["csv", "json"]


class BenchmarkParser:
    def __init__(self, input_file, output_file, smt2_folder: "str" = "") -> None:
        self.input_file: "str" = input_file
        self.output_file: "str" = output_file
        self.smt2_folder: "str" = smt2_folder
        self.benchmark_rows: "list[BenchmarkRow]" = []

    def get_benchmarks(self) -> "list[Benchmark]":
        with open(self.input_file, "r", encoding="utf-8") as f:
            benchmark_json: "dict" = json.load(f)
            return benchmark_json.get("benchmarks", [])

    def write_benchmarks(self, out_format: "BenchmarkFormat" = "csv"):
        if out_format == "csv":
            self.write_benchmarks_csv()
        else:
            raise ValueError(f"Invalid output format: {out_format}")

    def write_benchmarks_csv(self):
        if len(self.benchmark_rows) == 0:
            return
        with open(self.output_file, "w", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=self.benchmark_rows[0].keys())
            writer.writeheader()
            writer.writerows(self.benchmark_rows)

    def parse_benchmarks(self, out_format: "BenchmarkFormat" = "csv"):
        benchmarks = self.get_benchmarks()
        self.benchmark_rows: "list[BenchmarkRow]" = []

        for benchmark in benchmarks:
            file, solver, precision, actual_precision, assertions, result = benchmark[
                "name"
            ].split(",")

            file = file.split("/")[-1]
            precision = float(precision)
            assertions = int(assertions)
            actual_precision = float(actual_precision)

            self.benchmark_rows.append(
                {
                    "file": file,
                    "solver": solver,
                    "assertions": assertions,
                    "precision": precision,
                    "actualPrecision": actual_precision,
                    "realTime": round(benchmark["real_time"], 3),
                    "cpuTime": round(benchmark["cpu_time"], 3),
                    "timeUnit": benchmark["time_unit"],
                    "iterations": benchmark["iterations"],
                    "result": result,
                }
            )

        self.write_benchmarks(out_format)


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_file> <output_file>")
        sys.exit(1)

    sys.argv.pop(0)
    input_file = sys.argv.pop(0)
    output_file = sys.argv.pop(0)

    benchmark_parser = BenchmarkParser(input_file, output_file, "smt2")
    benchmark_parser.parse_benchmarks()


if __name__ == "__main__":
    main()
