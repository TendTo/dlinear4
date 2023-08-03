#include <sys/mman.h>
#include <sys/wait.h>

#include <cstdio>
#include <iostream>
#include <regex>

#include "dreal/main_program.h"
#include "dreal/qsopt_ex.h"
#include "dreal/smt2/driver.h"
#include "dreal/smt2/run.h"
#include "dreal/solver/config.h"
#include "dreal/soplex.h"
#include "dreal/util/infty.h"

using dreal::util::InftyFinish;
using dreal::util::InftyStart;
using std::string;

namespace dreal {

struct shared_results {
  int _nAssertions;
  bool _isSat;
  double _actualPrecision;
};

class InfoGatherer {
 public:
  InfoGatherer() = delete;
  InfoGatherer(const string& filename, const string& solver,
               const string& precision);
  InfoGatherer(const string& filename, const string& solver,
               const string& precision, int timeout);
  void SetDebugScanning(bool debugScanning);
  void SetDebugParsing(bool debugParsing);
  bool Run();

  const string& filename() const;
  const string& solver() const;
  double precision() const;
  double actualPrecision() const;
  size_t nAssertions() const;
  bool isSat() const;

 private:
  Config config_;
  string filename_;
  bool debug_scanning_{false};
  bool debug_parsing_{false};
  string solver_;
  double precision_{0.0};
  double actualPrecision_{0.0};
  size_t nAssertions_{0};
  bool isSat_{false};
  pid_t intermediate_pid_{-1};
  int timeout_{0};

  string GetSolverName(const Config::LPSolver solver);
  Config::LPSolver GetLPSolver(const string& solver);
  void Init();
  void DeInit();
  void GatherInfo(shared_results* results);
  void StartIntermediateProcess(shared_results* results);
  bool WaitChild();
  void ParseResults(shared_results* results);
};

}  // namespace dreal