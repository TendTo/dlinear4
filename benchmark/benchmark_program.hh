#include <dirent.h>

#include <cstdio>
#include <iostream>
#include <regex>

#include <benchmark/benchmark.h>

#include "benchmark/util/config.hh"
#include "benchmark/util/info_gatherer.hh"
#include "benchmark/util/utils.hh"

#include "dreal/main_program.h"

#define SMT2_EXT ".smt2"
#define DEFAULT_ARGC 6
#define TEMP_FILE "/tmp/dLinear-benchmark"
#define SAT "delta-sat"
#define UNSAT "unsat"

using std::string;
using std::vector;

namespace dreal {
class BenchmarkProgram {
 public:
  /**
   * @brief Create an instance of a BenchmarkProgram.
   * The object can than be used to run the benchmarks.
   * @param argc number of command line arguments. Passed by the main
   * function
   * @param argv array of command line arguments. Passed by the main
   * function
   */
  BenchmarkProgram(int argc, const char* argv[]);
  /**
   * @brief Run the benchmarks.
   * @return exit status
   */
  int Run();
  /**
   * @brief Initialize the array @p argv with the correct values, making it
   * ready to be used when starting a benchmark. The @p argv must have a size
   * greater or equal to 6. The value returned is intended to be userd as the
   * argc parameter, and corresponds to the number of args in @p argv .
   * @param argv array to be initialized by the function
   * @param filename name of the file used in the benchmark
   * @param solver solver used in the benchmark
   * @param precision precision to use in the benchmark. If set to 0, enables
   * the --exhaustive flag
   * @return the argc parameter, meaning the size of usable arguments in @p argv
   */
  static inline int InitArgv(const char* argv[DEFAULT_ARGC],
                             const string& filename, const string& solver,
                             const string& precision);

 private:
  int argc_;
  const char** argv_;
  BenchConfig config;
  /**
   * @brief Map containing all the configuration loaded from the
   * configuration file.
   */
  std::map<string, vector<string>> parameters;
  /**
   * @brief Vector of files the program will run.
   */
  vector<string> smt2Files;

  /**
   * @brief Parse the configuration file @p filename .
   * All line are expected in the format
   * parameter = value1 [value2 ...]
   * @param filename configuration file
   */
  void ReadConfigurationFile();
  /**
   * @brief Load all the files found in the @p directory matching the provided
   * @p fileExtension and store their name in the @ref
   * BenchmarkProgram::smt2Files vector.
   * @param directory directory containing the files to load
   * @param fileExtension extension of the files to load
   */
  void LoadSmt2Files(const string& fileExtension = SMT2_EXT);

  /**
   * @brief Register all the benchmarks that will be run.
   */
  void RegisterBenchmarks();
  /**
   * @brief Start the benchmarking framework, making it run all registered
   * benchmarks
   * @param argc number of command line arguments
   * @param argv array of command line arguments
   */
  void StartBenchmarks();
  /**
   * @brief Return the list of values associated with the provided parameter.
   * The list should have been already loaded from the config file.
   * If the requested parameter has not been specified in the configuration, a
   * vector containing the default value is returned instead
   * @param parameter parameter to know the values of
   * @param defaultValue default value to return if the requested parameter is
   * not present
   * @return list of values associated with the parameter
   */
  inline vector<string> GetParameterValues(const string& parameter,
                                           const string& defaultValue);
};

}  // namespace dreal