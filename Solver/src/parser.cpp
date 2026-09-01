#include "parser.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "logger.h"

Parser::Parser(std::string filename)
    : filename_(std::move(filename)), num_variables_(0), num_clauses_(0) {}

// TODO: Support to multiple clauses per line

void Parser::Parse() {
  std::ifstream file(filename_);
  std::string line;

  if (!file.is_open()) {
    LOG_ERROR("Failed to open file: {}", filename_);
    return;
  }

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == 'c') continue;

    std::istringstream iss(line);
    char cmd;
    iss >> cmd;

    if (cmd == 'p') {
      std::string problem_type;
      iss >> problem_type >> num_variables_ >> num_clauses_;
      if (problem_type != "cnf") {
        LOG_ERROR("Invalid problem type: {}", problem_type);
        LOG_INFO("Expected problem type: cnf");
        return;
      }
    } else {
      iss.putback(cmd);
      int32_t literal;
      std::vector<int32_t> current_clause;
      while (iss >> literal && literal != 0) {
        current_clause.push_back(literal);
      }
      clauses_.push_back(current_clause);
    }
  }

  LOG_INFO("Parsed {} variables and {} clauses", num_variables_,
           clauses_.size());
}

void Parser::PrintClauses() const {
  for (auto& clause : clauses_) {
    for (const auto& literal : clause) {
      std::cout << literal << " ";
    }
    std::cout << "\n";
  }
}
