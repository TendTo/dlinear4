#!/usr/bin/env python3
# pylint: disable=missing-module-docstring,missing-function-docstring,missing-class-docstring,invalid-name
import json
import csv
import sys
import re
from typing import TYPE_CHECKING
import os

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

    class SloaneStufken(TypedDict):
        solver: str
        assertions: int
        precision: float
        actualPrecision: float
        realTime: float
        cpuTime: float
        timeUnit: str
        iterations: int
        result: str
        s1: int
        k1: int
        s2: int
        k2: int
        t: int

    class LPProblem(TypedDict):
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

    class SMTProblem(TypedDict):
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
        self.lp_problem_rows: "list[LPProblem]" = []
        self.slone_stufken_rows: "list[SloaneStufken]" = []
        self.smt_problem_rows: "list[SMTProblem]" = []

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
        self.write_cvs("lp")
        self.write_cvs("ss")
        self.write_cvs("smt")

    def write_cvs(self, row_type: "Literal['ss', 'smt', 'lp']"):
        rows = self.lp_problem_rows
        match row_type:
            case "ss":
                rows = self.slone_stufken_rows
            case "smt":
                rows = self.smt_problem_rows
            case "lp":
                rows = self.lp_problem_rows

        if len(rows) > 0:
            with open(self.get_output_with_prefix(row_type), "w", encoding="utf-8") as f:
                writer = csv.DictWriter(f, fieldnames=rows[0].keys())
                writer.writeheader()
                writer.writerows(rows)

    def parse_lp_problem(self, benchmark: "Benchmark"):
        file, solver, precision, actual_precision, assertions, result = benchmark[
            "name"
        ].split(",")

        file = file.split("/")[-1]
        file = file.removeprefix("LP_")
        file = file.removesuffix(".smt2")
        precision = float(precision)
        assertions = int(assertions)
        actual_precision = float(actual_precision)

        self.lp_problem_rows.append(
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

    def parse_sloane_stufken_problem(self, benchmark: "Benchmark"):
        file, solver, precision, actual_precision, assertions, result = benchmark[
            "name"
        ].split(",")

        file = file.split("/")[-1]
        file = file.removesuffix(".smt2")
        precision = float(precision)
        assertions = int(assertions)
        actual_precision = float(actual_precision)
        s1, k1, s2, k2, t = (int(val) for val in file.split("-"))

        self.slone_stufken_rows.append(
            {
                "solver": solver,
                "assertions": assertions,
                "precision": precision,
                "actualPrecision": actual_precision,
                "realTime": round(benchmark["real_time"], 3),
                "cpuTime": round(benchmark["cpu_time"], 3),
                "timeUnit": benchmark["time_unit"],
                "iterations": benchmark["iterations"],
                "result": result,
                "k1": k1,
                "s1": s1,
                "k2": k2,
                "s2": s2,
                "t": t,
            }
        )

    def parse_smt_problem(self, benchmark: "Benchmark"):
        file, solver, precision, actual_precision, assertions, result = benchmark[
            "name"
        ].split(",")

        file = file.split("/")[-1]
        file = file.removesuffix(".smt2")
        file = file.removeprefix("SMT_")
        precision = float(precision)
        assertions = int(assertions)
        actual_precision = float(actual_precision)

        self.smt_problem_rows.append(
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

    def parse_benchmarks(self, out_format: "BenchmarkFormat" = "csv"):
        benchmarks = self.get_benchmarks()
        self.lp_problem_rows: "list[LPProblem]" = []

        for benchmark in benchmarks:
            file = benchmark["name"].split(",")[0]
            file = file.split("/")[-1]
            if file.startswith("LP"):
                self.parse_lp_problem(benchmark)
            elif file.startswith("SMT"):
                self.parse_smt_problem(benchmark)
            elif re.match(r"^(\d+-){4}\d+.smt2$", file):
                self.parse_sloane_stufken_problem(benchmark)
            else:
                self.parse_smt_problem(benchmark)

        self.write_benchmarks(out_format)

    def get_output_with_prefix(self, prefix: "str") -> "str":
        filename = f"{prefix}{os.path.basename(self.output_file)}"
        return os.path.join(os.path.dirname(self.output_file), filename)


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

