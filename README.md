# LemmaSAT

This is a SAT solver that uses the CDCL algorithm to solve the Boolean Satisfiability Problem on CNF formulas.

## Table of Contents

- [To do](#to-do)
- [Building](#building)
- [Running](#running)
- [References](#references)
- [LICENSE](#license)
- [Contributing](#contributing)

## To do
- [x] DIMACS Parser
- [x] Clause Database
- [x] Preprocessing (BCP, Pure Literal Elimination, Subsumption & SSR, Strengthening, Blocked Clause Elimination, Variable Elimination, Probing)
- [ ] CDCL Engine (Decision Heuristics, Conflict Analysis, Learning Clauses, Backtracking)
- [x] Proof System (DRUP, LRAT and DRAT-proof)
- [ ] Policy Layer ??
- [ ] Parallelization / HPC [optional]

## Building

```bash
make build
```

## Running

```bash
./build/Solver/Solver <path_to_dimacs_file>
```

## References

- [Unit Propagation](https://en.wikipedia.org/wiki/Unit_propagation)
- [CDCL](https://en.wikipedia.org/wiki/Conflict-driven_clause_learning)
- [Impact of BVE on Solving Pigeonhole Formulas](https://www.cs.cmu.edu/~mheule/publications/bve-paper.pdf)
- [Preprocessing in SAT Solving](https://cca.informatik.uni-freiburg.de/papers/BiereJarvisaloKiesl-SAT-Handbook-2021-Preprocessing-Chapter-Manuscript.pdf) (if link is unreachable, DOI: 10.3233/FAIA200992)
- [Kissat](https://github.com/arminbiere/kissat) and [CaDiCaL](https://github.com/arminbiere/cadical) source codes

## LICENSE

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are not welcome. This is a personal project.
