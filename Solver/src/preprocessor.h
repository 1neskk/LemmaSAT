#pragma once

#include <cstdint>
#include <queue>
#include <span>
#include <vector>

#include "clause_database.h"
#include "proof_emitter.h"

#ifdef small
#undef small
#endif

class Preprocessor {
 public:
  struct Config {
    bool enable_subsumption = true;
    bool enable_bve = true;
    bool enable_probing = true;
    bool enable_bce = true;

    int32_t bve_max_growth = 0;
    int32_t bve_max_occurrences = 800;

    int64_t probe_max_steps = 100000;

    int32_t max_rounds = 10;
  };

  struct Stats {
    int64_t units_propagated = 0;
    int64_t pure_eliminated = 0;
    int64_t clauses_subsumed = 0;
    int64_t literals_strengthened = 0;
    int64_t vars_eliminated = 0;
    int64_t probing_units = 0;
    int64_t blocked_clauses = 0;

    bool operator==(const Stats&) const = default;
  };

  Preprocessor(ClauseDatabase& db, ProofEmitter* proof = nullptr);

  bool Preprocess();

  bool Propagate();
  bool EliminatePureLiterals();
  bool SubsumeAndStrengthen();
  bool EliminateVariables();
  bool Probe();
  void EliminateBlockedClauses();

  [[nodiscard]] const std::vector<int32_t>& Trail() const { return trail_; }
  [[nodiscard]] const Stats& GetStats() const { return stats_; }

  [[nodiscard]] bool HasEmptyClause() const { return empty_clause_; }

  void ExtendModel(std::vector<int8_t>& model) const;

 private:
  enum class Value : uint8_t { kUndef = 0, kTrue, kFalse };

  enum class ClauseStatus : uint8_t {
    kSatisfied,
    kFalsified,
    kUnit,
    kUnresolved
  };

  [[nodiscard]] Value LitValue(int32_t lit) const;
  void SetValue(int32_t var, Value val);

  [[nodiscard]] ClauseStatus Evaluate(const Clause& clause,
                                      int32_t* unit_lit = nullptr) const;

  bool DetectEmptyClause(uint32_t ci);
  bool ScanForEmptyClause();
  bool SignalEmptyClause(std::span<const int32_t> antecedent);

  bool Enqueue(int32_t lit);
  bool PropagateQueue();

  bool Subsume(uint32_t ci);

  [[nodiscard]] bool IsSubset(const Clause& small, const Clause& large) const;

  bool TryEliminate(int32_t var);

  // Resolve `pos` and `neg` on `var`, writing the resolvent.
  // Returns false if the resolvent is tautological.
  bool Resolve(const Clause& pos, const Clause& neg, int32_t var,
               std::vector<int32_t>& resolvent);

  struct ProbeResult {
    bool conflict = false;
    std::vector<int32_t> implied;
  };

  ProbeResult ProbeLit(int32_t lit);

  [[nodiscard]] bool IsBlocked(uint32_t clause_idx, int32_t blocking_lit) const;

  ClauseDatabase& db_;
  ProofEmitter* proof_;
  Config config_;
  Stats stats_;

  std::vector<Value> assigns_;
  std::vector<int32_t> trail_;
  std::queue<int32_t> unit_queue_;

  bool empty_clause_ = false;

  std::vector<bool> eliminated_;

  // Stack of clause sets removed by BVE, for model reconstruction.
  struct EliminationRecord {
    int32_t var;
    std::vector<std::vector<int32_t>> clauses;
  };
  std::vector<EliminationRecord> elim_stack_;
};
