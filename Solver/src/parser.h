#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Parser {
 public:
  explicit Parser(std::string filename);
  ~Parser() = default;

  void Parse();

  void PrintClauses() const;

  [[nodiscard]] uint32_t GetNumVariables() const { return num_variables_; }
  [[nodiscard]] uint32_t GetNumClauses() const { return num_clauses_; }
  [[nodiscard]] const std::vector<std::vector<int32_t>>& GetClauses() const {
    return clauses_;
  }

 private:
  std::string filename_;
  std::vector<std::vector<int32_t>> clauses_;
  uint32_t num_variables_;
  uint32_t num_clauses_;
};
