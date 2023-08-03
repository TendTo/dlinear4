#include "config.hh"

namespace dreal {

BenchConfig::BenchConfig() { ConfigureParser(); }

void BenchConfig::ConfigureParser() {
  opt_.overview = "dLinear benchmarks : benchmarking delta-complete SMT solver";
  opt_.syntax = "dreal-benchmarks [OPTIONS]";

  opt_.add("" /* Default */, false /* Required? */,
           0 /* Number of args expected. */,
           0 /* Delimiter if expecting multiple args. */,
           "Display usage instructions.", "-h", "-help", "--help", "--usage");

  opt_.add("" /* Default */, false /* Required? */,
           0 /* Number of args expected. */,
           0 /* Delimiter if expecting multiple args. */,
           "Dry run. No benchmarks are produced", "-d", "--dry-run");

  opt_.add("0" /* Default */, false /* Required? */,
           1 /* Number of args expected. */,
           0 /* Delimiter if expecting multiple args. */,
           "Max time in seconds allowed for info gathering for each problem. "
           "Only problems taking less than the timeout are benchmarked. If set "
           "to 0, it is disabled.",
           "-t", "--timeout");

  opt_.add(CONF_FILE /* Default */, false /* Required? */,
           1 /* Number of args expected. */,
           0 /* Delimiter if expecting multiple args. */,
           "Path to the configuration file.", "-c", "--config");

  opt_.add(SMT2_DIR /* Default */, false /* Required? */,
           1 /* Number of args expected. */,
           0 /* Delimiter if expecting multiple args. */,
           "Path to the directory containing the smt2 files.", "-p", "--path");

  opt_.add("" /* Default */, false /* Required? */,
           -1 /* Number of args expected. */,
           ',' /* Delimiter if expecting multiple args. */,
           "Comma separated list of .smt2 files containing the problems to benchmark. If set, "
           "--path will be ignored.",
           "-f", "--file");
}

void BenchConfig::StoreConfig() {
  isDryRun_ = opt_.get("-d")->isSet;
  opt_.get("-t")->getInt(timeout_);
  opt_.get("-c")->getString(config_file_);
  opt_.get("-p")->getString(path_);
  opt_.get("-f")->getStrings(files_);
}

void BenchConfig::Parse(int argc, const char* argv[]) {
  opt_.parse(argc, argv);
  if (opt_.isSet("-h")) {
    std::string usage;
    opt_.getUsage(usage);
    std::cout << usage;
    exit(0);
  }
  StoreConfig();
}

std::ostream& operator<<(std::ostream& os, const BenchConfig& config) {
  os << "BenchConfig(" << std::endl
     << "  config_file = " << config.config_file_ << std::endl
     << "  path = " << config.path_ << std::endl
     << "  files = [";
  for (const auto& file : config.files_) {
    os << file << ", ";
  }
  os << "]" << std::endl
     << "  isDryRun = " << config.isDryRun_ << std::endl
     << "  timeout = " << config.timeout_ << std::endl
     << ")";
  return os;
}

}  // namespace dreal