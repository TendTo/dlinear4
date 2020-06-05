#pragma once

#include <set>
#include <vector>
#include <map>
#include <functional>
#include <utility>

#include "dreal/solver/config.h"
#include "dreal/symbolic/symbolic.h"
#include "dreal/util/box.h"
#include "dreal/util/literal.h"
#include "dreal/gmp.h"
#include "dreal/soplex.h"

#define QS_EXACT_UNKNOWN   0
#define QS_EXACT_UNSAT     1
#define QS_EXACT_SAT       2
#define QS_EXACT_DELTA_SAT 3

namespace dreal {

/// Theory solver for linear theory over the Reals.
class SoplexTheorySolver {
 public:
  SoplexTheorySolver() = delete;
  explicit SoplexTheorySolver(const Config& config);

  /// Checks consistency. Returns true if there is a satisfying
  /// assignment. Otherwise, return false.
  int CheckSat(const Box& box, const std::vector<Literal>& assertions,
               soplex::SoPlex& prob,
               const std::map<int, Variable>& var_map);

  /// Gets a satisfying Model.
  const Box& GetModel() const;

  /// Gets a list of used constraints.
  const LiteralSet& GetExplanation() const;

 private:
  const Config& config_;
  Box model_;
  LiteralSet explanation_;
  mpq_class precision_;
};

}  // namespace dreal
