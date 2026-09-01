#include "preprocessor.h"

#include <algorithm>
#include <cstdlib>
#include <string>

#include "logger.h"

Preprocessor::Preprocessor(ClauseDatabase& db, ProofEmitter* proof)
    : db_(db),
      proof_(proof),
      assigns_(static_cast<size_t>(db.NumVars() + 1), Value::kUndef),
      eliminated_(static_cast<size_t>(db.NumVars() + 1), false) {}

bool Preprocessor::Preprocess() {
  for (int32_t round = 0; round < config_.max_rounds; ++round) {
    Stats before = stats_;

    if (HasEmptyClause()) return false;
    if (!Propagate()) return false;
    if (!EliminatePureLiterals()) return false;

    if (config_.enable_subsumption) {
      if (!SubsumeAndStrengthen()) return false;
    }
    if (config_.enable_bve) {
      if (!EliminateVariables()) return false;
    }
    if (config_.enable_probing) {
      if (!Probe()) return false;
    }
    if (config_.enable_bce) {
      EliminateBlockedClauses();
    }

    if (stats_ == before) break;

    LOG_INFO("Preprocessing round {} complete", round + 1);
  }

  LOG_INFO(
      "Preprocessing done: {} units, {} pure, {} subsumed, {} strengthened, "
      "{} vars eliminated, {} probing units, {} blocked",
      stats_.units_propagated, stats_.pure_eliminated, stats_.clauses_subsumed,
      stats_.literals_strengthened, stats_.vars_eliminated,
      stats_.probing_units, stats_.blocked_clauses);

  return true;
}

Preprocessor::Value Preprocessor::LitValue(int32_t lit) const {
  Value val = assigns_[std::abs(lit)];
  if (val == Value::kUndef) return Value::kUndef;
  if (lit > 0) return val;
  return val == Value::kTrue ? Value::kFalse : Value::kTrue;
}

void Preprocessor::SetValue(int32_t var, Value val) { assigns_[var] = val; }

bool Preprocessor::Enqueue(int32_t lit) {
  int32_t var = std::abs(lit);
  Value cur = LitValue(lit);

  if (cur == Value::kTrue) return true;

  if (cur == Value::kFalse) {
    const int32_t conflicting[] = {lit};
    return SignalEmptyClause(conflicting);
  }

  SetValue(var, lit > 0 ? Value::kTrue : Value::kFalse);
  trail_.push_back(lit);
  unit_queue_.push(lit);
  ++stats_.units_propagated;
  return true;
}

Preprocessor::ClauseStatus Preprocessor::Evaluate(const Clause& clause,
                                                  int32_t* unit_lit) const {
  int32_t unassigned = 0;
  int32_t last_free = 0;

  for (int32_t lit : clause.literals) {
    Value v = LitValue(lit);
    if (v == Value::kTrue) return ClauseStatus::kSatisfied;
    if (v == Value::kUndef) {
      ++unassigned;
      last_free = lit;
    }
  }

  if (unassigned == 0) return ClauseStatus::kFalsified;

  if (unassigned == 1) {
    if (unit_lit != nullptr) *unit_lit = last_free;
    return ClauseStatus::kUnit;
  }
  return ClauseStatus::kUnresolved;
}

bool Preprocessor::SignalEmptyClause(std::span<const int32_t> antecedent) {
  if (!empty_clause_) {
    empty_clause_ = true;

    // The empty clause closes a DRUP/DRAT proof.
    // NOTE: units derived by pure-literal elimination and probing are not
    // emitted yet, so this line is only RUP-checkable once they are.
    if (proof_) proof_->EmitClause(std::span<const int32_t>{});

    std::string lits;
    for (int32_t lit : antecedent) {
      if (!lits.empty()) lits += ' ';
      lits += std::to_string(lit);
    }
    LOG_INFO("Empty clause derived (falsified antecedent: [{}]) - UNSAT", lits);
  }
  return false;
}

bool Preprocessor::DetectEmptyClause(uint32_t ci) {
  const Clause& clause = db_[ci];
  if (clause.deleted) return false;
  if (Evaluate(clause) != ClauseStatus::kFalsified) return false;

  SignalEmptyClause(clause.literals);
  return true;
}

bool Preprocessor::ScanForEmptyClause() {
  for (uint32_t ci = 0; ci < db_.NumClauses(); ++ci) {
    if (DetectEmptyClause(ci)) return true;
  }
  return false;
}

bool Preprocessor::PropagateQueue() {
  while (!unit_queue_.empty()) {
    const int32_t lit = unit_queue_.front();
    unit_queue_.pop();

    // Clauses containing `lit` are satisfied — delete them.
    for (uint32_t ci : db_.Occurrences(lit)) {
      if (db_[ci].deleted) continue;
      if (proof_) proof_->EmitDelete(db_[ci].literals);
      db_.DeleteClause(ci);
    }

    // Clauses containing negated `lit` — strengthen by removing `-lit`.
    int32_t neg = -lit;
    const auto& occ = db_.Occurrences(neg);
    // Iterate backwards: StrengthenClause removes `ci` from this very list
    // by swapping in the back element, which we have already visited.
    for (size_t i = occ.size(); i > 0; --i) {
      uint32_t ci = occ[i - 1];
      if (db_[ci].deleted) continue;

      if (proof_) {
        std::vector<int32_t> strengthened;
        for (int32_t l : db_[ci].literals) {
          if (l != neg) strengthened.push_back(l);
        }
        proof_->EmitClause(strengthened);
        proof_->EmitDelete(db_[ci].literals);
      }

      db_.StrengthenClause(ci, neg);

      if (DetectEmptyClause(ci)) return false;

      if (db_[ci].IsUnit()) {
        if (!Enqueue(db_[ci].literals[0])) return false;
      }
    }
  }
  return true;
}

bool Preprocessor::Propagate() {
  if (ScanForEmptyClause()) return false;

  for (uint32_t i = 0; i < db_.NumClauses(); ++i) {
    if (db_[i].deleted) continue;
    if (db_[i].IsUnit()) {
      if (!Enqueue(db_[i].literals[0])) return false;
    }
  }
  return PropagateQueue();
}

bool Preprocessor::EliminatePureLiterals() {
  for (int32_t var = 1; var <= db_.NumVars(); ++var) {
    if (assigns_[var] != Value::kUndef || eliminated_[var]) continue;

    db_.CleanOccurrences(var);
    db_.CleanOccurrences(-var);

    bool has_pos = !db_.Occurrences(var).empty();
    bool has_neg = !db_.Occurrences(-var).empty();

    if (has_pos == has_neg) continue;  // not a pure literal - skip

    int32_t pure_lit = has_pos ? var : -var;
    if (!Enqueue(pure_lit)) return false;
    if (!PropagateQueue()) return false;
    ++stats_.pure_eliminated;
  }
  return true;
}

bool Preprocessor::IsSubset(const Clause& small, const Clause& large) const {
  if ((small.signature & ~large.signature) != 0) return false;
  if (small.Size() > large.Size()) return false;

  for (int32_t lit : small.literals) {
    bool found = false;
    for (int32_t other : large.literals) {
      if (lit == other) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return true;
}

bool Preprocessor::Subsume(uint32_t ci) {
  const auto& clause = db_[ci];
  if (clause.deleted) return true;

  // Pick the literal with the smallest occurrence list.
  int32_t best_lit = clause.literals[0];
  size_t best_count = db_.Occurrences(best_lit).size();
  for (size_t k = 1; k < clause.Size(); ++k) {
    size_t cnt = db_.Occurrences(clause.literals[k]).size();
    if (cnt < best_count) {
      best_count = cnt;
      best_lit = clause.literals[k];
    }
  }

  // Forward subsumption: C \subseteq D -> delete D
  for (uint32_t di : db_.Occurrences(best_lit)) {
    if (di == ci || db_[di].deleted) continue;
    if (IsSubset(clause, db_[di])) {
      if (proof_) proof_->EmitDelete(db_[di].literals);
      db_.DeleteClause(di);
      ++stats_.clauses_subsumed;
    }
  }

  // Self-subsuming resolution (SSR)
  for (int32_t lit : clause.literals) {
    int32_t neg = -lit;
    auto& occ = db_.Occurrences(neg);
    for (size_t i = occ.size(); i > 0; --i) {
      uint32_t di = occ[i - 1];
      if (di == ci || db_[di].deleted) continue;
      if (db_[di].Size() < clause.Size()) continue;

      // Check: every literal of C except `lit` appears in D.
      bool can_strengthen = true;
      for (int32_t l : clause.literals) {
        if (l == lit) continue;
        bool found = false;
        for (int32_t dl : db_[di].literals) {
          if (l == dl) {
            found = true;
            break;
          }
        }
        if (!found) {
          can_strengthen = false;
          break;
        }
      }

      if (can_strengthen) {
        if (proof_) {
          std::vector<int32_t> strengthened;
          for (int32_t dl : db_[di].literals) {
            if (dl != neg) strengthened.push_back(dl);
          }
          proof_->EmitClause(strengthened);
          proof_->EmitDelete(db_[di].literals);
        }

        db_.StrengthenClause(di, neg);
        ++stats_.literals_strengthened;

        if (DetectEmptyClause(di)) return false;
        if (db_[di].IsUnit()) {
          if (!Enqueue(db_[di].literals[0])) return false;
          if (!PropagateQueue()) return false;
        }
      }
    }
  }

  return true;
}

bool Preprocessor::SubsumeAndStrengthen() {
  // Process clauses smallest-first for more effective subsumption.
  std::vector<uint32_t> order(db_.NumClauses());
  for (uint32_t i = 0; i < db_.NumClauses(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [this](uint32_t a, uint32_t b) {
    return db_[a].Size() < db_[b].Size();
  });

  for (uint32_t ci : order) {
    if (!Subsume(ci)) return false;
  }
  return true;
}

bool Preprocessor::Resolve(const Clause& pos, const Clause& neg, int32_t var,
                           std::vector<int32_t>& resolvent) {
  resolvent.clear();

  for (int32_t lit : pos.literals) {
    if (lit == var) continue;
    resolvent.push_back(lit);
  }

  for (int32_t lit : neg.literals) {
    if (lit == -var) continue;

    // Tautology check
    for (int32_t r : resolvent) {
      if (r == -lit) return false;
    }
    // Duplicate check
    bool dup = false;
    for (int32_t r : resolvent) {
      if (r == lit) {
        dup = true;
        break;
      }
    }
    if (!dup) resolvent.push_back(lit);
  }
  return true;
}

bool Preprocessor::TryEliminate(int32_t var) {
  if (assigns_[var] != Value::kUndef || eliminated_[var]) return true;

  db_.CleanOccurrences(var);
  db_.CleanOccurrences(-var);

  const auto& pos_occs = db_.Occurrences(var);
  const auto& neg_occs = db_.Occurrences(-var);

  int32_t original_count =
      static_cast<int32_t>(pos_occs.size() + neg_occs.size());
  if (original_count == 0) return true;
  if (original_count > config_.bve_max_occurrences) return true;

  // Generate all non-tautological resolvents.
  std::vector<std::vector<int32_t>> resolvents;
  for (uint32_t pi : pos_occs) {
    if (db_[pi].deleted) continue;
    for (uint32_t ni : neg_occs) {
      if (db_[ni].deleted) continue;
      std::vector<int32_t> resolvent;
      if (Resolve(db_[pi], db_[ni], var, resolvent)) {
        resolvents.push_back(std::move(resolvent));
      }
    }
  }

  // Cost guard: reject if net clause count would grow.
  int32_t growth = static_cast<int32_t>(resolvents.size()) - original_count;
  if (growth > config_.bve_max_growth) return true;

  EliminationRecord rec;
  rec.var = var;
  for (uint32_t pi : pos_occs) {
    if (!db_[pi].deleted) rec.clauses.push_back(db_[pi].literals);
  }
  for (uint32_t ni : neg_occs) {
    if (!db_[ni].deleted) rec.clauses.push_back(db_[ni].literals);
  }
  elim_stack_.push_back(std::move(rec));

  for (uint32_t pi : pos_occs) {
    if (!db_[pi].deleted) {
      if (proof_) proof_->EmitDelete(db_[pi].literals);
      db_.DeleteClause(pi);
    }
  }
  for (uint32_t ni : neg_occs) {
    if (!db_[ni].deleted) {
      if (proof_) proof_->EmitDelete(db_[ni].literals);
      db_.DeleteClause(ni);
    }
  }

  for (auto& resolvent : resolvents) {
    if (proof_) proof_->EmitClause(resolvent);
    uint32_t new_idx = db_.AddClause(std::move(resolvent));
    if (DetectEmptyClause(new_idx)) return false;
    if (db_[new_idx].IsUnit()) {
      if (!Enqueue(db_[new_idx].literals[0])) return false;
      if (!PropagateQueue()) return false;
    }
  }

  eliminated_[var] = true;
  ++stats_.vars_eliminated;
  return true;
}

bool Preprocessor::EliminateVariables() {
  // Sort candidates by total occurrence count — cheap eliminations first.
  std::vector<std::pair<int32_t, int32_t>> candidates;
  for (int32_t var = 1; var <= db_.NumVars(); ++var) {
    if (assigns_[var] != Value::kUndef || eliminated_[var]) continue;
    int32_t cost = static_cast<int32_t>(db_.Occurrences(var).size() +
                                        db_.Occurrences(-var).size());
    candidates.emplace_back(cost, var);
  }
  std::sort(candidates.begin(), candidates.end());

  for (auto [cost, var] : candidates) {
    if (!TryEliminate(var)) return false;
  }
  return true;
}

Preprocessor::ProbeResult Preprocessor::ProbeLit(int32_t lit) {
  ProbeResult result;

  // Save state.
  size_t trail_save = trail_.size();
  auto assigns_save = assigns_;

  // Tentatively assign.
  SetValue(std::abs(lit), lit > 0 ? Value::kTrue : Value::kFalse);
  trail_.push_back(lit);

  // Read-only BCP — no clause modifications.
  std::queue<int32_t> q;
  q.push(lit);

  while (!q.empty()) {
    int32_t prop = q.front();
    q.pop();

    for (uint32_t ci : db_.Occurrences(-prop)) {
      if (db_[ci].deleted) continue;

      int32_t unit_lit = 0;
      // Same falsification test as DetectEmptyClause, but a falsified clause
      // here only refutes the assumption `lit`, not the formula - so it must
      // never be signalled as the empty clause.
      switch (Evaluate(db_[ci], &unit_lit)) {
        case ClauseStatus::kFalsified:
          result.conflict = true;
          trail_.resize(trail_save);
          assigns_ = assigns_save;
          return result;

        case ClauseStatus::kUnit:
          SetValue(std::abs(unit_lit),
                   unit_lit > 0 ? Value::kTrue : Value::kFalse);
          trail_.push_back(unit_lit);
          q.push(unit_lit);
          break;

        case ClauseStatus::kSatisfied:
        case ClauseStatus::kUnresolved:
          break;
      }
    }
  }

  // Record implications (excluding the assumption itself).
  for (size_t i = trail_save + 1; i < trail_.size(); ++i) {
    result.implied.push_back(trail_[i]);
  }

  // Restore state.
  trail_.resize(trail_save);
  assigns_ = assigns_save;
  return result;
}

bool Preprocessor::Probe() {
  int64_t steps = 0;

  for (int32_t var = 1; var <= db_.NumVars(); ++var) {
    if (assigns_[var] != Value::kUndef || eliminated_[var]) continue;
    if (steps >= config_.probe_max_steps) break;
    ++steps;

    auto pos = ProbeLit(var);
    auto neg = ProbeLit(-var);

    if (pos.conflict && neg.conflict) return false;  // UNSAT

    if (pos.conflict) {
      if (!Enqueue(-var)) return false;
      if (!PropagateQueue()) return false;
      ++stats_.probing_units;
      continue;
    }
    if (neg.conflict) {
      if (!Enqueue(var)) return false;
      if (!PropagateQueue()) return false;
      ++stats_.probing_units;
      continue;
    }

    // Common implications — forced under both polarities.
    std::sort(pos.implied.begin(), pos.implied.end());
    std::sort(neg.implied.begin(), neg.implied.end());

    size_t i = 0, j = 0;
    while (i < pos.implied.size() && j < neg.implied.size()) {
      if (pos.implied[i] == neg.implied[j]) {
        int32_t common = pos.implied[i];
        if (LitValue(common) == Value::kUndef) {
          if (!Enqueue(common)) return false;
          ++stats_.probing_units;
        }
        ++i;
        ++j;
      } else if (pos.implied[i] < neg.implied[j]) {
        ++i;
      } else {
        ++j;
      }
    }
    if (!PropagateQueue()) return false;
  }
  return true;
}

bool Preprocessor::IsBlocked(uint32_t clause_idx, int32_t blocking_lit) const {
  const auto& clause = db_[clause_idx];

  // A clause C is blocked on `blocking_lit` if every resolvent of C with
  // any clause containing `-blocking_lit` is tautological.
  for (uint32_t ri : db_.Occurrences(-blocking_lit)) {
    if (db_[ri].deleted) continue;

    bool tautology = false;
    for (int32_t lit : clause.literals) {
      if (lit == blocking_lit) continue;
      for (int32_t rlit : db_[ri].literals) {
        if (rlit == -lit) {
          tautology = true;
          break;
        }
      }
      if (tautology) break;
    }

    if (!tautology) return false;
  }
  return true;
}

void Preprocessor::EliminateBlockedClauses() {
  for (uint32_t i = 0; i < db_.NumClauses(); ++i) {
    if (db_[i].deleted) continue;

    for (int32_t lit : db_[i].literals) {
      if (IsBlocked(i, lit)) {
        if (proof_) proof_->EmitDelete(db_[i].literals);
        db_.DeleteClause(i);
        ++stats_.blocked_clauses;
        break;
      }
    }
  }
}

void Preprocessor::ExtendModel(std::vector<int8_t>& model) const {
  // Process BVE records in reverse elimination order.
  for (auto it = elim_stack_.rbegin(); it != elim_stack_.rend(); ++it) {
    int32_t var = it->var;

    // Set the variable to satisfy at least one original clause containing
    // +var. If all such clauses are already satisfied, default to false.
    bool needs_true = false;
    for (const auto& clause : it->clauses) {
      bool satisfied = false;
      bool contains_pos = false;
      for (int32_t lit : clause) {
        if (lit == var) {
          contains_pos = true;
          continue;
        }
        if (lit == -var) continue;
        int32_t v = std::abs(lit);
        if ((lit > 0 && model[v] == 1) || (lit < 0 && model[v] == -1)) {
          satisfied = true;
          break;
        }
      }
      if (!satisfied && contains_pos) {
        needs_true = true;
        break;
      }
    }

    model[var] = needs_true ? 1 : -1;
  }
}
