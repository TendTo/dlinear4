#include <cstdio>
#include <iostream>
#include <regex>

#include "dreal/main_program.h"
#include "dreal/qsopt_ex.h"
#include "dreal/soplex.h"
#include "dreal/smt2/driver.h"
#include "dreal/smt2/run.h"
#include "dreal/solver/config.h"
#include "dreal/util/infty.h"

using dreal::util::InftyFinish;
using dreal::util::InftyStart;
using std::string;

namespace dreal {

class InfoGatherer {
 public:
  InfoGatherer() = delete;
  InfoGatherer(const string& filename, const string& solver,
               const string& precision);
  void SetDebugScanning(bool debugScanning);
  void SetDebugParsing(bool debugParsing);
  void Run();

  const string& filename() const;
  const string& solver() const;
  double precision() const;
  double actualPrecision() const;
  size_t nAssertions() const;
  bool isSat() const;

 private:
  Config _config;
  string _filename;
  bool _debug_scanning{false};
  bool _debug_parsing{false};
  string _solver;
  double _precision{0.0};
  double _actualPrecision{0.0};
  size_t _nAssertions{0};
  bool _isSat{false};

  string GetSolverName(const Config::LPSolver solver);
  Config::LPSolver GetLPSolver(const string& solver);
  void Init();
  void DeInit();
  void GatherInfo();
};

}  // namespace dreal