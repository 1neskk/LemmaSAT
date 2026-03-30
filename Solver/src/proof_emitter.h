#pragma once

#include <cstdint>
#include <fstream>
#include <span>
#include <string>

enum class ProofFormat { DRUP, DRAT, LRAT };

class ProofEmitter {
 public:
  explicit ProofEmitter(std::string filename, ProofFormat format);
  ~ProofEmitter() = default;

  void EmitClause(std::span<const int32_t> clause);
  void EmitDelete(std::span<const int32_t> clause);

  void EmitLRATClause(uint64_t id, std::span<const int32_t> clause,
                      std::span<const uint64_t> hints);
  void EmitLRATDelete(uint64_t id, std::span<const uint64_t> clause_ids);

  [[nodiscard]] ProofFormat GetFormat() const { return format_; }

  void Flush();

 private:
  ProofFormat format_;
  std::ofstream out_;
};