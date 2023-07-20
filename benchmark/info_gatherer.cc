#include "benchmark/info_gatherer.hh"

namespace dreal {

InfoGatherer::InfoGatherer(const string& filename, const string& solver,
                           const string& precision)
    : _config{}, _filename{filename}, _solver{solver} {
  _precision = stod(precision);
  _config.mutable_lp_solver().set_from_command_line(GetLPSolver(_solver));
  _config.mutable_precision().set_from_command_line(_precision);
}

void InfoGatherer::SetDebugScanning(bool debugScanning) {
  _debug_scanning = debugScanning;
}

void InfoGatherer::SetDebugParsing(bool debugParsing) {
  _debug_parsing = debugParsing;
}

void InfoGatherer::Run() {
  Init();
  GatherInfo();
  DeInit();
}

const string& InfoGatherer::filename() const { return _filename; }
const string& InfoGatherer::solver() const { return _solver; }
double InfoGatherer::precision() const { return _precision; }
double InfoGatherer::actualPrecision() const { return _actualPrecision; }
size_t InfoGatherer::nAssertions() const { return _nAssertions; }
bool InfoGatherer::isSat() const { return _isSat; }

string InfoGatherer::GetSolverName(const Config::LPSolver solver) {
  switch (solver) {
    case Config::SOPLEX:
      return "soplex";
    case Config::QSOPTEX:
      return "qsoptex";
    default:
      throw DREAL_RUNTIME_ERROR("Unknown solver {}", solver);
  }
}

Config::LPSolver InfoGatherer::GetLPSolver(const string& solver) {
  if (solver == "soplex") {
    return Config::SOPLEX;
  } else if (solver == "qsoptex") {
    return Config::QSOPTEX;
  }
  throw DREAL_RUNTIME_ERROR("Unknown solver {}", solver);
}

void InfoGatherer::Init() {
  if (_config.lp_solver() == Config::QSOPTEX) {
    qsopt_ex::QSXStart();
    InftyStart(qsopt_ex::mpq_INFTY, qsopt_ex::mpq_NINFTY);
  } else if (_config.lp_solver() == Config::SOPLEX) {
    InftyStart(soplex::infinity);
  }
  Expression::InitConstants();
}

void InfoGatherer::DeInit() {
  Expression::DeInitConstants();
  InftyFinish();
  if (_config.lp_solver() == Config::QSOPTEX) {
    qsopt_ex::QSXFinish();
  }
}

void InfoGatherer::GatherInfo() {
  Smt2Driver smt2_driver{Context{Config{_config}}};
  smt2_driver.set_trace_scanning(_debug_scanning);
  smt2_driver.set_trace_parsing(_debug_parsing);
  smt2_driver.parse_file(_filename);
  _nAssertions = smt2_driver.context().assertions().size();
  _isSat = !smt2_driver.context().get_model().empty();
  _actualPrecision = smt2_driver.context().config().precision();
}

}  // namespace dreal
