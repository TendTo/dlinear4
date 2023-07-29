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
        assertions: int
        precision: float
        timeUnit: str
        iterations: int
        s1: int
        k1: int
        s2: int
        k2: int
        t: int
        actualPrecisionS: float
        actualPrecisionQ: float
        timeS: float
        timeQ: float
        resultS: str
        resultQ: str

    class LPProblem(TypedDict):
        file: str
        assertions: int
        precision: float
        timeUnit: str
        iterations: int
        actualPrecisionS: float
        actualPrecisionQ: float
        timeS: float
        timeQ: float
        resultS: str
        resultQ: str

    class SMTProblem(TypedDict):
        file: str
        assertions: int
        precision: float
        timeUnit: str
        iterations: int
        actualPrecisionS: float
        actualPrecisionQ: float
        timeS: float
        timeQ: float
        resultS: str
        resultQ: str


class BenchmarkParser:
    def __init__(self, input_file, output_file, smt2_folder: "str" = "") -> None:
        self.input_file: "str" = input_file
        self.output_file: "str" = output_file
        self.smt2_folder: "str" = smt2_folder
        self.benchmarks: "list[Benchmark]" = []
        self.lp_problem_rows: "dict[str, LPProblem]" = {}
        self.slone_stufken_rows: "dict[str, SloaneStufken]" = {}
        self.smt_problem_rows: "dict[str, SMTProblem]" = {}

    def load_benchmarks(self):
        with open(self.input_file, "r", encoding="utf-8") as f:
            benchmark_json: "dict" = json.load(f)
            self.benchmarks = benchmark_json.get("benchmarks", [])

    def write_benchmarks_csv(self):
        self.write_cvs("lp")
        self.write_cvs("ss")
        self.write_cvs("smt")

    def write_cvs(self, row_type: "Literal['ss', 'smt', 'lp']"):
        file_row = self.lp_problem_rows
        match row_type:
            case "ss":
                file_row = self.slone_stufken_rows
            case "smt":
                file_row = self.smt_problem_rows
            case "lp":
                file_row = self.lp_problem_rows

        if len(file_row) > 0:
            rows = [file_row[key] for key in file_row]
            with open(
                self.get_output_file_with_prefix(row_type), "w", encoding="utf-8"
            ) as f:
                writer = csv.DictWriter(f, fieldnames=rows[0].keys())
                writer.writeheader()
                writer.writerows(rows)

    def row_to_csv(self, row: "dict[str, str]"):
        pass

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

        key = f"{file}/{precision}"
        row: "LPProblem" = self.lp_problem_rows.get(
            key,
            {
                "file": file,
                "assertions": assertions,
                "precision": precision,
                "timeUnit": benchmark["time_unit"],
                "iterations": benchmark["iterations"],
                "actualPrecisionS": -1,
                "actualPrecisionQ": -1,
                "timeS": -1,
                "timeQ": -1,
                "resultS": "/",
                "resultQ": "/",
            },
        )
        if solver == "soplex":
            row["actualPrecisionS"] = actual_precision
            row["timeS"] = round(benchmark["cpu_time"], 3)
            row["resultS"] = result
        elif solver == "qsoptex":
            row["actualPrecisionQ"] = actual_precision
            row["timeQ"] = round(benchmark["cpu_time"], 3)
            row["resultQ"] = result
        self.lp_problem_rows[key] = row

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

        key = f"{file}/{precision}"
        row: "SloaneStufken" = self.slone_stufken_rows.get(
            key,
            {
                "assertions": assertions,
                "precision": precision,
                "timeUnit": benchmark["time_unit"],
                "iterations": benchmark["iterations"],
                "s1": s1,
                "k1": k1,
                "s2": s2,
                "k2": k2,
                "t": t,
                "actualPrecisionS": -1,
                "actualPrecisionQ": -1,
                "timeS": -1,
                "timeQ": -1,
                "resultS": "/",
                "resultQ": "/",
            },
        )
        if solver == "soplex":
            row["actualPrecisionS"] = actual_precision
            row["timeS"] = round(benchmark["cpu_time"], 3)
            row["resultS"] = result
        elif solver == "qsoptex":
            row["actualPrecisionQ"] = actual_precision
            row["timeQ"] = round(benchmark["cpu_time"], 3)
            row["resultQ"] = result
        self.slone_stufken_rows[key] = row

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

        key = f"{file}/{precision}"
        row: "SMTProblem" = self.smt_problem_rows.get(
            key,
            {
                "file": file,
                "assertions": assertions,
                "precision": precision,
                "timeUnit": benchmark["time_unit"],
                "iterations": benchmark["iterations"],
                "actualPrecisionS": -1,
                "actualPrecisionQ": -1,
                "timeS": -1,
                "timeQ": -1,
                "resultS": "/",
                "resultQ": "/",
            },
        )
        if solver == "soplex":
            row["actualPrecisionS"] = actual_precision
            row["timeS"] = round(benchmark["cpu_time"], 3)
            row["resultS"] = result
        elif solver == "qsoptex":
            row["actualPrecisionQ"] = actual_precision
            row["timeQ"] = round(benchmark["cpu_time"], 3)
            row["resultQ"] = result
        self.smt_problem_rows[key] = row

    def parse_benchmarks(self):
        self.load_benchmarks()

        for benchmark in self.benchmarks:
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

        self.write_benchmarks_csv()

    def get_output_file_with_prefix(self, prefix: "str") -> "str":
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
