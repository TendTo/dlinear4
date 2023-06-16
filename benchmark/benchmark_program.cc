#include "benchmark_program.hh"

using std::cerr;
using std::cout;
using std::string;

namespace dreal {

static void benchmark_dlinear(benchmark::State& state, const string& filename,
                              const string& solver, const string& precision) {
  const char* argv[6];
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
    for (string& solver : solvers)
      for (string& precision : precisions) {
        string isSatResult = IsSat(filename, solver, precision);
        cout << isSatResult << std::endl;
        benchmark::RegisterBenchmark(
            fmt::format("{},{},{},{}", filename, solver, precision,
                        isSatResult),
            &benchmark_dlinear, filename, solver, precision);
      }
  }
}
inline vector<string> BenchmarkProgram::GetParameterValues(
    const string& parameter, const string& defaultValue) {
  return parameters.count(parameter) == 1 ? parameters[parameter]
                                          : vector<string>{defaultValue};
}

inline string BenchmarkProgram::IsSat(const string& filename,
                                      const string& solver,
                                      const string& precision) {
  FILE* tempFile = fopen(TEMP_FILE, "w+");
  FILE originalStdout = *stdout;  // preserve the original stdout
  *stdout = *tempFile;            // redirect stdout to the temp file

  const char* argv[DEFAULT_ARGC];
  int argc = BenchmarkProgram::InitArgv(argv, filename, solver, precision);
  MainProgram main{argc, argv};
  main.Run();
  fflush(stdout);

  *stdout = originalStdout;  // restore stdout

  rewind(tempFile);
  char line[LINE_MAX];
  while (fgets(line, LINE_MAX, tempFile)) {
    if (strncmp(line, UNSAT, strlen(UNSAT)) ||
        strncmp(line, SAT, strlen(SAT))) {
      line[strlen(line) - 1] = '\0'; // Remove \n
      fclose(tempFile);
      return string{line};
    }
  }
  fclose(tempFile);
  throw DLINEAR_RUNTIME_ERROR(
      "Could not parse the output of the run.\nFile '{}', solver '{}', "
      "precision '{}'",
      filename, solver, precision);
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
