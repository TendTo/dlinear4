#include "benchmark_program.hh"

using std::cerr;
using std::cout;
using std::string;

namespace dreal {

static void benchmark_dlinear(benchmark::State& state, const string& filename,
                              const string& solver, const string& precision) {
  const char* argv[DEFAULT_ARGC];
  int argc = BenchmarkProgram::InitArgv(argv, filename, solver, precision);

  for (auto _ : state) {
    MainProgram main_program{argc, argv};
    main_program.Run();
  }
}

BenchmarkProgram::BenchmarkProgram(int argc, char* argv[]) {
  this->_argc = argc;
  this->_argv = argv;
}

int BenchmarkProgram::Run() {
  ReadConfigurationFile();
  LoadSmt2Files();
  RegisterBenchmarks();
  StartBenchmarks();
  return 0;
}

void BenchmarkProgram::LoadSmt2Files(const string& directory,
                                     const string& fileExtension) {
  // If there are arguments, use them as files to benchmark
  if (this->_argc >= 2) {
    for (int i = 1; i < this->_argc; i++)
      this->smt2Files.push_back(string{this->_argv[i]});
    return;
  }

  // Otherwise, look for files with the fileExtension in the given directory
  DIR* dir;
  struct dirent* ent;
  if ((dir = opendir(directory.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (EndsWith(ent->d_name, fileExtension.c_str()))
        smt2Files.push_back(fmt::format("{}/{}", directory, ent->d_name));
    }
    closedir(dir);
  } else {
    throw DLINEAR_RUNTIME_ERROR(
        "Could not open directory '{}' looking for '{}' files", directory,
        fileExtension);
  }
}

void BenchmarkProgram::StartBenchmarks() {
  std::cout << "Starting benchmarks" << std::endl;
  std::cout << this->smt2Files[0] << " files to benchmark" << std::endl;
  benchmark::Initialize(&_argc, _argv);
  benchmark::SetDefaultTimeUnit(benchmark::kMillisecond);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}

void BenchmarkProgram::ReadConfigurationFile(const string& filename) {
  std::ifstream conf_file{filename};
  if (!conf_file.is_open())
    throw DLINEAR_RUNTIME_ERROR("File '{}' could not be opened", filename);

  const std::regex conf_regex("^(\\w+) *= *(.+?) *$");
  std::smatch conf_matches;

  string line;
  while (conf_file) {
    std::getline(conf_file, line);

    if (std::regex_match(line, conf_matches, conf_regex)) {
      if (conf_matches.size() != 3) continue;

      string parameter = conf_matches[1].str();
      std::stringstream values{conf_matches[2].str()};
      string value;
      while (values >> value) parameters[parameter].push_back(value);
    }
  }
}

void BenchmarkProgram::RegisterBenchmarks() {
  std::vector<string> solvers = GetParameterValues("solver", "qsoptex");
  std::vector<string> precisions =
      GetParameterValues("precision", "0.0009999999999999996");

  for (string& filename : smt2Files) {
    for (string& solver : solvers) {
      for (string& precision : precisions) {
        InfoGatherer info_gatherer{filename, solver, precision};
        if (info_gatherer.Run()) {
          benchmark::RegisterBenchmark(
              fmt::format("{},{},{},{},{},{}", filename, solver, precision,
                          info_gatherer.actualPrecision(),
                          info_gatherer.nAssertions(),
                          info_gatherer.isSat() ? SAT : UNSAT),
              &benchmark_dlinear, filename, solver, precision);
        }
      }
    }
  }
}

inline vector<string> BenchmarkProgram::GetParameterValues(
    const string& parameter, const string& defaultValue) {
  return parameters.count(parameter) == 1 ? parameters[parameter]
                                          : vector<string>{defaultValue};
}

inline int BenchmarkProgram::InitArgv(const char* argv[DEFAULT_ARGC],
                                      const string& filename,
                                      const string& solver,
                                      const string& precision) {
  int argc = DEFAULT_ARGC;
  argv[0] = "dlinear";
  argv[1] = filename.c_str();
  argv[2] = "--lp-solver";
  argv[3] = solver.c_str();
  if (stof(precision) == 0) {
    argv[4] = "--exhaustive";
    argc--;
  } else {
    argv[4] = "--precision";
    argv[5] = precision.c_str();
  }
  return argc;
}

}  // namespace dreal
