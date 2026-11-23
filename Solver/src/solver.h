#pragma once

#include "clause_database.h"
#include "parser.h"
#include "proof_emitter.h"

// The CDCL engine
class Solver {
 public:
  explicit Solver(ClauseDatabase& db);
  ~Solver() = default;

 private:
  ProofFormat format_;
  ProofEmitter& emitter_;
};
