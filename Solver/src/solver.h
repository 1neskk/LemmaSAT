#pragma once

#include "clause_database.h"
#include "parser.h"
#include "proof_emitter.h"

// The CDCL engine
class Solver {
 public:
  explicit Solver(ClauseDatabase& db, ProofEmitter& emitter);
  ~Solver() = default;

 private:
  Parser& parser_;
  ProofEmitter& emitter_;
  ProofFormat format_;
};