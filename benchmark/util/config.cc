#include "config.hh"

namespace dreal {

BenchConfig::BenchConfig(int argv, char* argc[]) {
  for (int i = 1; i < argv; i++) {
    std::string arg{argc[i]};
    if (arg == "--dry-run") {
      this->isDryRun_ = true;
      continue;
    } else if (arg.find("--timeout=") == 0) {
      timeout_ = std::stoi(arg.substr(10));
      continue;
    } else if (arg.find("--") == 0) {
      continue;
    }
    this->files_.push_back(std::string{argc[i]});
  }
}

}  // namespace dreal