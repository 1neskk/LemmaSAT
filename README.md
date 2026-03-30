# CDCL SAT Solver

This is a SAT solver that uses the CDCL algorithm to solve the Boolean Satisfiability Problem on CNF formulas.

## Table of Contents

- [To do](#to-do)
- [Building](#building)
- [Running](#running)
- [Contributing](#contributing)

## To do
- [x] DIMACS Parser
- [x] Clause Database
- [x] Preprocessing (BCP, Pure Literal Elimination, Subsumption & SSR, Strengthening, Blocked Clause Elimination, Variable Elimination, Probing)
- [ ] CDCL Engine (Decision Heuristics, Conflict Analysis, Learning Clauses, Backtracking)
- [x] Proof System (DRUP, LRAT and DRAT-proof)
- [ ] Parallelization / HPC [optional]

## Building

```bash
cmake -B build -S .
cmake --build build --config Release
```

## Running

```bash
./build/Solver/SAT-solver <path_to_dimacs_file>
```

## Contributing

Contributions are not welcome. This is a personal project.