#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

struct Clause {
  std::vector<int32_t> literals;
  uint64_t signature = 0;
  bool deleted = false;

  [[nodiscard]] size_t Size() const { return literals.size(); }
  [[nodiscard]] bool Empty() const { return literals.empty(); }
  [[nodiscard]] bool IsUnit() const { return literals.size() == 1; }
};

class ClauseDatabase {
 public:
  explicit ClauseDatabase(int32_t num_vars);

  uint32_t AddClause(std::vector<int32_t> literals);

  // Lazy clause deletion!
  void DeleteClause(uint32_t idx);

  void StrengthenClause(uint32_t idx, int32_t remove_lit);

  // Access
  Clause& operator[](uint32_t idx) { return clauses_[idx]; }
  const Clause& operator[](uint32_t idx) const { return clauses_[idx]; }

  [[nodiscard]] uint32_t NumClauses() const {
    return static_cast<uint32_t>(clauses_.size());
  }

  [[nodiscard]] const std::vector<uint32_t>& Occurrences(int32_t lit) const;

  void CleanOccurrences(int32_t lit);

  [[nodiscard]] int32_t NumVars() const { return num_vars_; }

  // Map literal to occurrence list index: positive literals at even indices,
  // negative at odd.
  [[nodiscard]] static uint32_t LitIndex(int32_t lit) {
    return lit > 0 ? 2u * static_cast<uint32_t>(lit - 1)
                   : 2u * static_cast<uint32_t>(-lit - 1) + 1;
  }

  [[nodiscard]] static uint64_t ComputeSignature(
      std::span<const int32_t> literals);

 private:
  int32_t num_vars_;
  std::vector<Clause> clauses_;
  std::vector<std::vector<uint32_t>> occ_;  // indexed by LitIndex(lit)
};
