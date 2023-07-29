#include <iostream>
#include <string>
#include <vector>

namespace dreal {

class BenchConfig {
 private:
  bool isDryRun_{false};
  std::vector<std::string> files_;
  int timeout_{0};

 public:
  BenchConfig(int argc, char* argv[]);

  bool isDryRun() const { return isDryRun_; }
  bool filesProvided() const { return !files_.empty(); }
  const std::vector<std::string>& files() const { return files_; }
  std::vector<std::string> copyFiles() const { return files_; }
  int timeout() const { return timeout_; }
};

}  // namespace dreal
