#include "dreal/solver/sat_solver.h"

#include <ostream>
#include <utility>
#include <cmath>

#include "dreal/util/assert.h"
#include "dreal/util/exception.h"
#include "dreal/util/logging.h"
#include "dreal/util/stat.h"
#include "dreal/util/timer.h"

namespace dreal {

using std::cout;
using std::set;
using std::vector;
using std::pair;
using std::make_pair;
using std::abs;
using qsopt_ex::mpq_QScreate_prob;
using qsopt_ex::mpq_QSset_param;
using qsopt_ex::mpq_QSfree_prob;
using qsopt_ex::mpq_QSchange_coef;
using qsopt_ex::mpq_QSget_rowcount;
using qsopt_ex::mpq_QSnew_row;
using qsopt_ex::mpq_QSget_colcount;
using qsopt_ex::mpq_QSnew_col;
using qsopt_ex::mpq_ILL_MINDOUBLE;  // mpq_NINFTY
using qsopt_ex::mpq_ILL_MAXDOUBLE;  // mpq_INFTY
using qsopt_ex::mpq_ninfty;  // mpq_class versions
using qsopt_ex::mpq_infty;
using qsopt_ex::__zeroLpNum_mpq__;  // mpq_zeroLpNum

SatSolver::SatSolver(const Config& config) : sat_{picosat_init()},
    cur_clause_start_{0} {
  // Enable partial checks via picosat_deref_partial. See the call-site in
  // SatSolver::CheckSat().
  picosat_save_original_clauses(sat_);
  if (config.random_seed() != 0) {
    picosat_set_seed(sat_, config.random_seed());
    DREAL_LOG_DEBUG("SatSolver::Set Random Seed {}", config.random_seed());
  }
  picosat_set_global_default_phase(
      sat_, static_cast<int>(config.sat_default_phase()));
  DREAL_LOG_DEBUG("SatSolver::Set Default Phase {}",
                  config.sat_default_phase());
  qsx_prob_ = mpq_QScreate_prob(NULL, QS_MIN);
  DREAL_ASSERT(qsx_prob_);
  if (log()->should_log(spdlog::level::trace)) {
    mpq_QSset_param(qsx_prob_, QS_PARAM_SIMPLEX_DISPLAY, 1);
  }
}

SatSolver::SatSolver(const Config& config, const vector<Formula>& clauses)
    : SatSolver{config} {
  AddClauses(clauses);
}

SatSolver::~SatSolver() {
  picosat_reset(sat_);
  mpq_QSfree_prob(qsx_prob_);
}

void SatSolver::AddFormula(const Formula& f) {
  DREAL_LOG_DEBUG("SatSolver::AddFormula({})", f);
  vector<Formula> clauses{cnfizer_.Convert(f)};
  // Collect CNF variables.
  for (const auto& p : cnfizer_.vars()) {
    cnf_variables_.insert(p.get_id());
  }
  for (Formula& clause : clauses) {
    clause = predicate_abstractor_.Convert(clause);
  }
  AddClauses(clauses);
}

void SatSolver::AddFormulas(const vector<Formula>& formulas) {
  for (const Formula& f : formulas) {
    AddFormula(f);
  }
}

void SatSolver::AddLearnedClause(const set<Literal>& literals) {
  for (const Literal& l : literals) {
      AddLiteral(make_pair(l.first, !(l.second)), true);
  }
  picosat_add(sat_, 0);
}

void SatSolver::AddClauses(const vector<Formula>& formulas) {
  for (const Formula& f : formulas) {
    AddClause(f);
  }
}

void SatSolver::AddClause(const Formula& f) {
  DREAL_LOG_DEBUG("SatSolver::AddClause({})", f);
  // Set up Variable ⇔ Literal (in SAT) map.
  for (const Variable& var : f.GetFreeVariables()) {
    MakeSatVar(var);
  }
  // Add clauses to SAT solver.
  DoAddClause(f);
}

namespace {
class SatSolverStat : public Stat {
 public:
  explicit SatSolverStat(const bool enabled) : Stat{enabled} {};
  SatSolverStat(const SatSolverStat&) = default;
  SatSolverStat(SatSolverStat&&) = default;
  SatSolverStat& operator=(const SatSolverStat&) = delete;
  SatSolverStat& operator=(SatSolverStat&&) = delete;
  ~SatSolverStat() override {
    if (enabled()) {
      using fmt::print;
      print(cout, "{:<45} @ {:<20} = {:>15}\n", "Total # of CheckSat",
            "SAT level", num_check_sat_);
      print(cout, "{:<45} @ {:<20} = {:>15f} sec\n",
            "Total time spent in SAT checks", "SAT level",
            timer_check_sat_.seconds());
    }
  }

  int num_check_sat_{0};
  Timer timer_check_sat_;
};
}  // namespace

// Collect active literals, removing those that are only required by learned
// clauses.
set<int> SatSolver::GetMainActiveLiterals() const {
    set<int> lits;
    for (int i = 1; i <= picosat_variables(sat_); ++i) {
      const int model_i{has_picosat_pop_used_ ? picosat_deref(sat_, i)
                                              : picosat_deref_partial(sat_, i)};
      if (model_i == 0) {
        continue;
      }
      lits.insert(model_i * i);
    }
    for (auto it = lits.begin(); it != lits.end(); ) {
      int i = *it;
      int required = false;
      // Determine whether literal `i' is required
      auto c_it = main_clause_lookup_.find(i);
      if (c_it != main_clause_lookup_.end()) {
        for (int c : c_it->second) {
          int count = 0;
          size_t j;
          for (j = c; j < main_clauses_copy_.size() &&
                          main_clauses_copy_[j]; ++j) {
            int k{main_clauses_copy_[j]};
            if (lits.find(k) != lits.end()) {
              ++count;
            }
          }
          DREAL_ASSERT(j < main_clauses_copy_.size());  // Detect buffer overrun
          DREAL_ASSERT(count > 0);  // Should contain at least `i'
          if (count == 1) {
            // `i' is the only active literal in clause `c'; hence, required.
            required = true;
            break;
          }
        }
      }
      if (!required) {
        // There is more than one literal in every main (non-learned) clause
        // containing literal `i'.  Hence, it is not required.
        it = lits.erase(it);
      } else {
        ++it;
      }
    }
    return lits;
}

optional<SatSolver::Model> SatSolver::CheckSat() {
  static SatSolverStat stat{DREAL_LOG_INFO_ENABLED};
  DREAL_LOG_DEBUG("SatSolver::CheckSat(#vars = {}, #clauses = {})",
                  picosat_variables(sat_),
                  picosat_added_original_clauses(sat_));
  stat.num_check_sat_++;
  // Call SAT solver.
  TimerGuard check_sat_timer_guard(&stat.timer_check_sat_,
                                   DREAL_LOG_INFO_ENABLED);
  const int ret{picosat_sat(sat_, -1)};
  check_sat_timer_guard.pause();

  Model model;
  if (ret == PICOSAT_SATISFIABLE) {
    // SAT Case.
    set<int> lits{GetMainActiveLiterals()};
    ResetLinearProblem();
    const auto& var_to_formula_map = predicate_abstractor_.var_to_formula_map();
    for (int i : lits) {
      const auto it_var = to_sym_var_.find(abs(i));
      if (it_var == to_sym_var_.end()) {
        // There is no symbolic::Variable corresponding to this
        // picosat variable (int). This could be because of
        // picosat_push/pop.
        continue;
      }
      const Variable& var{it_var->second};
      const auto it = var_to_formula_map.find(var);
      if (it != var_to_formula_map.end()) {
        DREAL_LOG_TRACE("SatSolver::CheckSat: Add theory literal {}{} to Model",
                        i > 0 ? "" : "¬", var);
        auto& theory_model = model.second;
        theory_model.emplace_back(var, i > 0);
        EnableLinearLiteral(var, i > 0);
      } else if (cnf_variables_.count(var.get_id()) == 0) {
        DREAL_LOG_TRACE(
            "SatSolver::CheckSat: Add Boolean literal {}{} to Model ",
            i > 0 ? "" : "¬", var);
        auto& boolean_model = model.first;
        boolean_model.emplace_back(var, i > 0);
      } else {
        DREAL_LOG_TRACE(
            "SatSolver::CheckSat: Skip {}{} which is a temporary variable.",
            i > 0 ? "" : "¬", var);
      }
    }
    DREAL_LOG_DEBUG("SatSolver::CheckSat() Found a model.");
    return model;
  } else if (ret == PICOSAT_UNSATISFIABLE) {
    DREAL_LOG_DEBUG("SatSolver::CheckSat() No solution.");
    // UNSAT Case.
    return {};
  } else {
    DREAL_ASSERT(ret == PICOSAT_UNKNOWN);
    DREAL_LOG_CRITICAL("PICOSAT returns PICOSAT_UNKNOWN.");
    throw DREAL_RUNTIME_ERROR("PICOSAT returns PICOSAT_UNKNOWN.");
  }
}

void SatSolver::Pop() {
  // FIXME: disabled for QSopt_ex changes
  throw DREAL_RUNTIME_ERROR("SatSolver::Pop() currently unsupported");
  DREAL_LOG_DEBUG("SatSolver::Pop()");
  cnf_variables_.pop();
  to_sym_var_.pop();
  to_sat_var_.pop();
  picosat_pop(sat_);
  has_picosat_pop_used_ = true;
}

void SatSolver::Push() {
  // FIXME: disabled for QSopt_ex changes
  throw DREAL_RUNTIME_ERROR("SatSolver::Push() currently unsupported");
  DREAL_LOG_DEBUG("SatSolver::Push()");
  picosat_push(sat_);
  to_sat_var_.push();
  to_sym_var_.push();
  cnf_variables_.push();
}

void SatSolver::SetQSXVarCoef(int qsx_row, const Variable& var,
                              const mpq_class& value) {
  const auto it = to_qsx_col_.find(var.get_id());
  if (it == to_qsx_col_.end()) {
    throw DREAL_RUNTIME_ERROR("Variable undefined: {}", var);
  }
  if (value <= mpq_ninfty() || value >= mpq_infty()) {
    throw DREAL_RUNTIME_ERROR("LP coefficient too large: {}", value);
  }
  mpq_t c_value;
  mpq_init(c_value);
  mpq_set(c_value, value.get_mpq_t());
  mpq_QSchange_coef(qsx_prob_, qsx_row, it->second, c_value);
  mpq_clear(c_value);
}

void SatSolver::ResetLinearProblem() {
  DREAL_LOG_TRACE("SatSolver::ResetLinearProblem()");
  const int qsx_rows{mpq_QSget_rowcount(qsx_prob_)};
  DREAL_ASSERT(static_cast<size_t>(qsx_rows) == from_qsx_row_.size());
  for (int i = 0; i < qsx_rows; i++) {
    mpq_QSchange_sense(qsx_prob_, i, 'G');
    mpq_QSchange_rhscoef(qsx_prob_, i, mpq_NINFTY);
  }
}

void SatSolver::EnableLinearLiteral(const Variable& var, bool truth) {
    const auto it = to_qsx_row_.find(make_pair(var.get_id(), truth));
    if (it == to_qsx_row_.end()) {
      // Either a learned literal, or a not-equal literal from the input
      // problem.
      DREAL_LOG_TRACE("SatSolver::EnableLinearLiteral: ignoring ({}, {})",
                      var, truth);
      return;
    }
    const int qsx_row = it->second;
    mpq_QSchange_sense(qsx_prob_, qsx_row, qsx_sense_[qsx_row]);
    mpq_QSchange_rhscoef(qsx_prob_, qsx_row, qsx_rhs_[qsx_row].get_mpq_t());
    DREAL_LOG_TRACE("SatSolver::EnableLinearLiteral({})", qsx_row);
}

// Because the input precision > 0, and we have reduced this by a small amount,
// we can replace any strict inequalities with the equivalent non-strict
// inequalities, and ignore not-equal constraints altogether.

static bool is_equal_or_whatever(const Formula& formula, bool truth) {
  if (truth) {
    return is_equal_to(formula);
  } else {
    return is_not_equal_to(formula);
  }
}

static bool is_not_equal_or_whatever(const Formula& formula, bool truth) {
  return is_equal_or_whatever(formula, !truth);
}

static bool is_greater_or_whatever(const Formula& formula, bool truth) {
  if (truth) {
    return is_greater_than(formula) || is_greater_than_or_equal_to(formula);
  } else {
    return is_less_than(formula) || is_less_than_or_equal_to(formula);
  }
}

static bool is_less_or_whatever(const Formula& formula, bool truth) {
  return is_greater_or_whatever(formula, !truth);
}

void SatSolver::AddLinearLiteral(const Variable& formulaVar, bool truth) {
    const auto& var_to_formula_map = predicate_abstractor_.var_to_formula_map();
    const auto it = var_to_formula_map.find(formulaVar);
    if (it == var_to_formula_map.end()) {
      // Boolean variable - no need to involve theory solver
      return;
    }
    const auto it2 = to_qsx_row_.find(make_pair(formulaVar.get_id(), truth));
    if (it2 != to_qsx_row_.end()) {
      // Found.
      return;
    }
    // Theory formula
    Formula formula = it->second;
    if (is_equal_or_whatever(formula, truth)) {
      qsx_sense_.push_back('E');
    } else if (is_greater_or_whatever(formula, truth)) {
      qsx_sense_.push_back('G');
    } else if (is_less_or_whatever(formula, truth)) {
      qsx_sense_.push_back('L');
    } else if (is_not_equal_or_whatever(formula, truth)) {
      // Nothing to do, because this constraint is always delta-sat for
      // delta > 0.
      return;
    } else {
      throw DREAL_RUNTIME_ERROR("Formula {} not supported", formula);
    }
    Expression expr;
    expr = (get_lhs_expression(formula) - get_rhs_expression(formula)).Expand();
    // Before calling SetQSXVarCoef(), we must create the LP solver variables
    // using AddLinearVariable().
    for (const Variable& var : formula.GetFreeVariables()) {
      AddLinearVariable(var);
    }
    const int qsx_row{mpq_QSget_rowcount(qsx_prob_)};
    mpq_QSnew_row(qsx_prob_, mpq_NINFTY, 'G', NULL);  // Inactive
    DREAL_ASSERT(static_cast<size_t>(qsx_row) == qsx_sense_.size() - 1);
    DREAL_ASSERT(static_cast<size_t>(qsx_row) == qsx_rhs_.size());
    qsx_rhs_.push_back(0);
    if (is_constant(expr)) {
      qsx_rhs_.back() = -get_constant_value(expr);
    } else if (is_variable(expr)) {
      SetQSXVarCoef(qsx_row, get_variable(expr), 1);
    } else if (is_multiplication(expr)) {
      std::map<Expression,Expression> map = get_base_to_exponent_map_in_multiplication(expr);
      if (map.size() != 1
       || !is_variable(map.begin()->first)
       || !is_constant(map.begin()->second)
       || get_constant_value(map.begin()->second) != 1) {
        throw DREAL_RUNTIME_ERROR("Expression {} not supported", expr);
      }
      SetQSXVarCoef(qsx_row,
                    get_variable(map.begin()->first),
                    get_constant_in_multiplication(expr));
    } else if (is_addition(expr)) {
      const std::map<Expression,mpq_class>& map = get_expr_to_coeff_map_in_addition(expr);
      for (const pair<Expression,mpq_class>& pair : map) {
        if (!is_variable(pair.first)) {
          throw DREAL_RUNTIME_ERROR("Expression {} not supported", expr);
        }
        SetQSXVarCoef(qsx_row, get_variable(pair.first), pair.second);
      }
      qsx_rhs_.back() = -get_constant_in_addition(expr);
    } else {
        throw DREAL_RUNTIME_ERROR("Expression {} not supported", expr);
    }
    if (qsx_rhs_.back() <= mpq_ninfty() || qsx_rhs_.back() >= mpq_infty()) {
      throw DREAL_RUNTIME_ERROR("LP RHS value too large: {}", qsx_rhs_.back());
    }
    to_qsx_row_.emplace(make_pair(make_pair(formulaVar.get_id(), truth), qsx_row));
    DREAL_ASSERT(static_cast<size_t>(qsx_row) == from_qsx_row_.size());
    from_qsx_row_.push_back(make_pair(formulaVar, truth));
    DREAL_LOG_DEBUG("SatSolver::AddLinearLiteral({}{} ↦ {})",
                    truth ? "" : "¬", it->second, qsx_row);
}

void SatSolver::UpdateLookup(int lit, int learned) {
  if (learned) {
    learned_clause_lits_.insert(lit);
  } else {
    main_clauses_copy_.push_back(lit);
    main_clause_lookup_[lit].insert(cur_clause_start_);
  }
}

void SatSolver::AddLiteral(const Literal& l, bool learned) {
  if (l.second) {
    // f = b
    const Variable& var{l.first};
    DREAL_ASSERT(var.get_type() == Variable::Type::BOOLEAN);
    // Add l = b
    int lit{to_sat_var_[var.get_id()]};
    picosat_add(sat_, lit);
    UpdateLookup(lit, learned);
    if (!learned) {
      AddLinearLiteral(var, true);
    }
  } else {
    // f = ¬b
    const Variable& var{l.first};
    DREAL_ASSERT(var.get_type() == Variable::Type::BOOLEAN);
    // Add l = ¬b
    int lit{-to_sat_var_[var.get_id()]};
    picosat_add(sat_, lit);
    UpdateLookup(lit, learned);
    if (!learned) {
      AddLinearLiteral(var, false);
    }
  }
}

void SatSolver::AddLiteral(const Formula& f) {
  DREAL_ASSERT(is_variable(f) ||
               (is_negation(f) && is_variable(get_operand(f))));
  if (is_variable(f)) {
    AddLiteral(make_pair(get_variable(f), true), false);
  } else {
    AddLiteral(make_pair(get_variable(get_operand(f)), false), false);
  }
}

void SatSolver::DoAddClause(const Formula& f) {
  cur_clause_start_ = main_clauses_copy_.size();
  if (is_disjunction(f)) {
    // f = l₁ ∨ ... ∨ lₙ
    for (const Formula& l : get_operands(f)) {
      AddLiteral(l);
    }
  } else {
    // f = b or f = ¬b.
    AddLiteral(f);
  }
  picosat_add(sat_, 0);
  main_clauses_copy_.push_back(0);
}

void SatSolver::MakeSatVar(const Variable& var) {
  auto it = to_sat_var_.find(var.get_id());
  if (it != to_sat_var_.end()) {
    // Found.
    return;
  }
  // It's not in the maps, let's make one and add it.
  const int sat_var{picosat_inc_max_var(sat_)};
  to_sat_var_.insert(var.get_id(), sat_var);
  to_sym_var_.insert(sat_var, var);
  DREAL_LOG_DEBUG("SatSolver::MakeSatVar({} ↦ {})", var, sat_var);
}

void SatSolver::AddLinearVariable(const Variable& var) {
  auto it = to_qsx_col_.find(var.get_id());
  if (it != to_qsx_col_.end()) {
    // Found.
    return;
  }
  const int qsx_col{mpq_QSget_colcount(qsx_prob_)};
  int status = mpq_QSnew_col(qsx_prob_, mpq_zeroLpNum, mpq_NINFTY, mpq_INFTY,
                             var.get_name().c_str());
  DREAL_ASSERT(!status);
  to_qsx_col_.emplace(make_pair(var.get_id(), qsx_col));
  DREAL_ASSERT(static_cast<size_t>(qsx_col) == from_qsx_col_.size());
  from_qsx_col_.push_back(var);
  DREAL_LOG_DEBUG("SatSolver::AddLinearVariable({} ↦ {})", var, qsx_col);
}

const vector<Variable>& SatSolver::GetLinearVarMap() const {
  DREAL_LOG_TRACE("SatSolver::GetLinearVarMap(): from_qsx_col_ =");
  if (log()->should_log(spdlog::level::trace)) {
    for (size_t i = 0; i < from_qsx_col_.size(); i++) {
      const Variable& var = from_qsx_col_[i];
      std::cerr << i << ": " << var << "\n";
    }
  }
  return from_qsx_col_;
}
}  // namespace dreal
