#include "proof_emitter.h"

#include <stdexcept>

ProofEmitter::ProofEmitter(std::string filename, ProofFormat format)
    : format_(format), out_(std::move(filename), std::ios::out) {
  if (!out_.is_open()) throw std::runtime_error("Failed to open proof file");
}

void ProofEmitter::EmitClause(std::span<const int32_t> clause) {
  for (int32_t literal : clause) {
    out_ << literal << " ";
  }
  out_ << "0\n";
}

void ProofEmitter::EmitDelete(std::span<const int32_t> clause) {
  out_ << "d ";
  for (int32_t literal : clause) {
    out_ << literal << " ";
  }
  out_ << "0\n";
}

void ProofEmitter::EmitLRATClause(uint64_t id, std::span<const int32_t> clause,
                                  std::span<const uint64_t> hints) {
  out_ << id << " ";

  for (int32_t literal : clause)
    out_ << literal << " ";

  out_ << "0 ";

  for (uint64_t hint : hints)
    out_ << hint << " ";

  out_ << "0\n";
}

void ProofEmitter::EmitLRATDelete(uint64_t id,
                                  std::span<const uint64_t> clause_ids) {
  out_ << id << " d ";

  for (uint64_t clause_id : clause_ids)
    out_ << clause_id << " ";

  out_ << "0\n";
}

void ProofEmitter::Flush() {
  out_.flush();
}
