#include "clause_database.h"
#include "logger.h"
#include "parser.h"
#include "preprocessor.h"

int main(int argc, char** argv) {
  Logger::Init();

  std::string filepath;
  if (argc > 1) {
    filepath = argv[1];
  } else {
    LOG_ERROR("No filepath provided");
    return 1;
  }

  Parser parser(filepath);
  parser.Parse();

#ifdef _DEBUG
  LOG_INFO("Printing clauses");
  parser.PrintClauses();
#endif

  ClauseDatabase db(static_cast<int32_t>(parser.GetNumVariables()));
  for (const auto& clause : parser.GetClauses()) {
    db.AddClause(clause);
  }

  Preprocessor preprocessor(db);
  if (!preprocessor.Preprocess()) {
    LOG_INFO("UNSATISFIABLE (detected during preprocessing)");
    std::cout << "UNSATISFIABLE\n";  // print to stdout for external tools
    return 0;
  }

  LOG_INFO("Preprocessing complete — {} units on trail",
           preprocessor.Trail().size());

  // TODO: pass simplified ClauseDatabase to the solver.

  return 0;
}
