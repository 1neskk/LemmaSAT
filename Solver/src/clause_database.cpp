#include "clause_database.h"

#include <algorithm>

ClauseDatabase::ClauseDatabase(int32_t num_vars)
    : num_vars_(num_vars), occ_(2 * static_cast<size_t>(num_vars)) {}

uint32_t ClauseDatabase::AddClause(std::vector<int32_t> literals) {
  uint32_t idx = static_cast<uint32_t>(clauses_.size());
  uint64_t sig = ComputeSignature(literals);

  for (int32_t lit : literals) {
    occ_[LitIndex(lit)].push_back(idx);
  }

  clauses_.push_back({std::move(literals), sig, false});
  return idx;
}

void ClauseDatabase::DeleteClause(uint32_t idx) {
  clauses_[idx].deleted = true;
}

void ClauseDatabase::StrengthenClause(uint32_t idx, int32_t remove_lit) {
  auto& clause = clauses_[idx];
  if (clause.deleted) return;

  auto& lits = clause.literals;
  lits.erase(std::remove(lits.begin(), lits.end(), remove_lit), lits.end());
  clause.signature = ComputeSignature(lits);

  auto& occ = occ_[LitIndex(remove_lit)];
  auto it = std::find(occ.begin(), occ.end(), idx);
  if (it != occ.end()) {
    *it = occ.back();
    occ.pop_back();
  }
}

const std::vector<uint32_t>& ClauseDatabase::Occurrences(int32_t lit) const {
  return occ_[LitIndex(lit)];
}

void ClauseDatabase::CleanOccurrences(int32_t lit) {
  auto& occ = occ_[LitIndex(lit)];
  occ.erase(std::remove_if(occ.begin(), occ.end(),
                           [this](uint32_t i) { return clauses_[i].deleted; }),
            occ.end());
}

uint64_t ClauseDatabase::ComputeSignature(std::span<const int32_t> literals) {
  uint64_t sig = 0;
  for (int32_t lit : literals) {
    sig |= 1ULL << (LitIndex(lit) % 64);
  }
  return sig;
}
