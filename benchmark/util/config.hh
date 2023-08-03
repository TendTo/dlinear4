#include <iostream>
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#endif
#include <ezoptionparser/ezOptionParser.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#pragma GCC diagnostic pop

#define CONF_FILE "benchmark/benchmark.conf"
#define SMT2_DIR "benchmark/smt2"

namespace dreal {

class BenchConfig {
 private:
  ez::ezOptionParser opt_;
  std::string config_file_;
  std::string path_;
  std::vector<std::string> files_;
  bool isDryRun_;
  int timeout_;
  void ConfigureParser();
  void StoreConfig();

 public:
  BenchConfig();

  void Parse(int argc, const char* argv[]);

  bool isDryRun() const { return isDryRun_; }
  bool filesProvided() const { return !files_.empty(); }
  const std::vector<std::string>& files() const { return files_; }
  std::vector<std::string> copyFiles() const { return files_; }
  int timeout() const { return timeout_; }
  const std::string& config_file() const { return config_file_; }
  const std::string& path() const { return path_; }
  friend std::ostream& operator<<(std::ostream& os, const BenchConfig& config);
};

std::ostream& operator<<(std::ostream& os, const BenchConfig& config);
}  // namespace dreal
