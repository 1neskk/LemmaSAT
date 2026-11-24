# AGENTS.md

Guidance for coding agents working in this repository.

## Project

LemmaSAT is a CDCL SAT solver for DIMACS CNF formulas, written in C++20. It is
a **personal project** with one author. Treat the owner as the decision-maker
on design, and treat this repo as a place to learn, not a product to ship.

Current state: the DIMACS parser, clause database, preprocessor and proof
emitter exist. The CDCL engine (`Solver` class) is an empty stub. `main.cpp`
parses, preprocesses and stops.

## Working agreement: explain first, then code

Before writing **non-trivial solver logic**, post a short outline and wait for
an explicit go-ahead. Non-trivial means anything that changes what the solver
computes: propagation, preprocessing techniques, CDCL (decisions, conflict
analysis, learning, backtracking, restarts), heuristics, proof emission, model
reconstruction, clause-database semantics. It also covers changes to the
build/test structure.

The outline should cover:

- the technique and a reference (paper, handbook chapter, or the matching
  Kissat/CaDiCaL code; see the README's References section);
- which invariants below it touches (lazy deletion, occurrence lists, proof
  lines, reconstruction stack);
- what it means for DRUP/DRAT/LRAT proofs;
- cost and limits (step budgets, occurrence caps);
- how it will be tested.

Changes you can make without asking first: typo and comment fixes, formatting,
renames, mechanical refactors that don't change behavior, one-line build fixes,
and new tests for existing behavior. Say what you changed when you finish.

When you find a bug outside your task, report it. Don't fix it silently.

## Layout

```
CMakeLists.txt          top level: C++20, adds spdlog, Core, Solver
Makefile                convenience wrapper (build / debug / relwithdebinfo / format / clean)
Core/src/               static lib "Core": Logger (spdlog wrapper, LOG_* macros)
Solver/src/             executable "Solver"
  main.cpp              CLI entry: parse -> ClauseDatabase -> Preprocessor
  parser.{h,cpp}        DIMACS reader
  clause_database.{h,cpp}  clause storage, occurrence lists, signatures
  preprocessor.{h,cpp}  BCP, pure literals, subsumption/SSR, BVE, probing, BCE
  proof_emitter.{h,cpp} DRUP/DRAT/LRAT text writer
  solver.{h,cpp}        CDCL engine (stub)
thirdparty/spdlog       git submodule
formula.cnf, unsat.cnf  tiny sample inputs (SAT / UNSAT)
```

## Build and run

```bash
git submodule update --init --recursive   # spdlog; required once

make debug              # build/ with -Wall -Wextra and _DEBUG defined
make build              # Release, -O3, NDEBUG
make relwithdebinfo     # -O3 -g
make clean

./build/Solver/Solver formula.cnf
./build/Solver/Solver unsat.cnf      # prints UNSATISFIABLE
```

If you call CMake directly, you must pass a build type. On Linux an empty or
unrecognized type stops configuration with `Invalid build type`:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug   # or Release, RelWithDebInfo
cmake --build build
```

Gotchas:

- The accepted types are `Debug`, `Release` and `RelWithDebInfo`, matched
  case-sensitively.
- Every Makefile target reuses `build/` and reconfigures it, so switching types
  triggers a rebuild. For side-by-side builds, use separate `-B` directories.
- The executable is `build/Solver/Solver`.
- Sources are picked up by `file(GLOB ... CONFIGURE_DEPENDS)`. New files in
  `Solver/src/` are added automatically. `Solver/CMakeLists.txt` also globs
  `Solver/*/*.cpp`, so **any** subdirectory of `Solver/` (including a future
  `tests/`) gets compiled into the solver executable unless the glob changes.
- Windows/MSVC is supported (`/MD`, `/utf-8`, `VS_STARTUP_PROJECT`). Keep code
  portable. The `#undef small` in `preprocessor.h` works around a Windows
  header macro.

## Formatting and style

- Run `make format` before committing. It runs clang-format, Google-based
  `.clang-format`, 2-space indent, 80 columns, includes regrouped and sorted.
  To check without rewriting:
  `clang-format --dry-run --Werror --style=file Core/src/*.{h,cpp} Solver/src/*.{h,cpp}`.
  If you add source directories, extend the `format` target to cover them.
- Naming: types and methods are `PascalCase` (`ClauseDatabase::AddClause`).
  Locals and parameters are `snake_case`. Data members are `snake_case_` with a
  trailing underscore, and static members also get an `s_` prefix
  (`s_core_logger_`). Enumerators are `kName` (`Value::kUndef`); `ProofFormat`
  is the one exception. Files are `snake_case.{h,cpp}`.
- Headers start with `#pragma once`. Include what you use; don't rely on
  `pch.h`. A `.cpp` includes its own header first.
- Mark getters and pure queries `[[nodiscard]]`. Use `std::span<const int32_t>`
  for read-only literal sequences.
- Use fixed-width types: `int32_t` for literals and variables, `uint32_t` for
  clause indices, `int64_t` for statistics and step counters.
- Log with `LOG_TRACE/INFO/WARN/ERROR/CRITICAL` (and `LOG_INFO_THROTTLED` in
  hot loops) from `logger.h`. Don't call spdlog or `std::cout` directly, except
  for the solver's final result output (see Output).

## Core data model and invariants

Read this before touching `clause_database.*` or `preprocessor.*`.

- **Literals** are DIMACS integers. Variables run from `1` to `num_vars`, a
  literal is `+v` or `-v`, and `0` is only a terminator, never stored.
  `ClauseDatabase::LitIndex` maps `+v` to `2(v-1)` and `-v` to `2(v-1)+1` for
  occurrence lists. Per-variable arrays (`assigns_`, `eliminated_`, models) are
  indexed by `v` and sized `num_vars + 1`. Models are `int8_t`: `1` true, `-1`
  false.
- **Clause indices are stable.** Clauses are never physically removed.
  `DeleteClause` only sets `deleted = true` (lazy deletion). Every loop over
  clauses or occurrence lists must skip `deleted` clauses. Call
  `CleanOccurrences(lit)` before counting occurrences.
- **Occurrence lists are unordered and change under you.** `StrengthenClause(ci, l)`
  removes `ci` from `occ[l]` by swapping in the last element and popping.
  When you strengthen while iterating that same list, iterate **backwards**, as
  `PropagateQueue` and SSR already do. A nested `Enqueue` + `PropagateQueue`
  can strengthen arbitrary clauses and shrink **any** list. Don't keep
  indices, iterators or `literals` references across such a call without
  re-checking bounds and `deleted`.
- `AddClause` can reallocate the clause vector and occurrence lists, which
  invalidates any `Clause&` or occurrence-list reference you hold.
- `Clause::signature` is a 64-bit literal bloom filter used for subsumption
  pre-checks. Any change to `literals` must recompute it; `StrengthenClause`
  does this.
- **Return convention:** preprocessing steps return `bool`. `true` means no
  conflict and `false` means the formula is UNSAT. Derive the empty clause only
  through `SignalEmptyClause` / `DetectEmptyClause`, which set `empty_clause_`
  and emit the empty proof line once. A conflict inside `ProbeLit` refutes only
  the probe assumption and must never be signalled as the empty clause.
- **Proof lines:** when `proof_` is set, every clause addition and deletion
  must be written in an order a checker accepts. For strengthening, emit the
  new clause first, then delete the old one. BVE resolvents are RUP.
  Additions that are RAT but not RUP, such as a pure-literal unit, are valid
  only in DRAT, and DRAT needs the pivot literal first. LRAT also needs clause IDs and hints, which the
  preprocessor doesn't track yet.
- **Model reconstruction:** a technique that removes clauses without
  preserving equivalence (BVE, BCE, and later others) must record what it
  removed so that `ExtendModel` can repair a model. BVE pushes an
  `EliminationRecord`; follow that pattern.
- `Preprocessor::Preprocess` runs rounds until `Stats` stops changing or
  `max_rounds` is reached. If you add a technique, add a counter to `Stats`;
  otherwise the fixpoint check can't see its progress.

## Testing (GoogleTest)

New or changed logic should come with GoogleTest tests under `Solver/tests/`.
The scaffolding is only half there, and it is **broken today**:

- `Solver/CMakeLists.txt` fetches GoogleTest and calls
  `add_subdirectory(tests)` only when `CMAKE_BUILD_TYPE=Test`. Both
  CMakeLists' build-type checks reject `Test` with a fatal error, and
  `Solver/tests/` doesn't exist.
- The `*/*.cpp` glob would compile test files into the solver executable,
  together with `main.cpp`.
- `make test` configures `Debug`, not `Test`, and then `cd`s into a directory
  that doesn't exist. `make help` lists `test`, but `.PHONY` doesn't.

Fixing this is a build-structure change, so outline it first. A likely shape:
move everything except `main.cpp` into a library target that both the
executable and a `tests` executable link, and make the test build type (or a
`LEMMASAT_BUILD_TESTS` option) pass the build-type checks.

What good tests look like here:

- Small hand-written CNFs, built in code or written to a temp file.
- Soundness checks against a brute-force reference for small formulas (about
  20 variables or fewer). Preprocessing must preserve SAT/UNSAT, and on SAT,
  `ExtendModel` applied to the reduced formula's model must satisfy the
  **original** clauses.
- Per-technique tests. `Preprocessor::Config` is private and has no setter
  today, so isolating techniques needs a small API change (outline it first).
- Regression tests for the occurrence-list and iterator hazards above.

No reference solver (kissat, cadical) or proof checker (drat-trim, lrat-check)
is installed. Don't assume one exists. If an external cross-check would help,
suggest it.

## Output (target: SAT-competition format)

Today all logs and the result go to stdout, UNSAT prints a bare
`UNSATISFIABLE`, and every path exits with code 0 except the missing-argument
error. The target, to be adopted as the engine lands, is:

- `s SATISFIABLE`, `s UNSATISFIABLE` or `s UNKNOWN` on stdout;
- on SAT, the model as `v ...` lines ending in `v ... 0`;
- every other stdout line starts with `c `, or diagnostics go to stderr;
- exit code `10` for SAT, `20` for UNSAT, `0` for unknown or error;
- proof file path as an optional second positional argument.

This changes CLI behavior, so outline the plan before you change `main.cpp` or
`Logger`.

## Known gaps (as of 2026-11-23)

Update this list when you close an item.

- `Solver` declares a constructor that is never defined and holds a
  `ProofEmitter& emitter_` reference member, which must be bound in the
  constructor's initializer list. `main.cpp` doesn't use `Solver` yet.
- `main.cpp` never creates a `ProofEmitter`, so no proof is written from the
  CLI.
- Parser: one clause per line only (TODO). A file that fails to open is logged,
  but the run continues as an empty formula and exits 0. The header counts
  aren't validated.
- Proofs: units from pure-literal elimination and probing aren't emitted, so
  the final empty clause isn't always checkable (see the NOTE in
  `SignalEmptyClause`).
- BCE deletes blocked clauses without recording them for reconstruction, and
  `ExtendModel` handles only BVE records. SAT models can be wrong once they are
  printed.

## Git

- Commit messages use conventional prefixes: `feat:`, `fix:`, `refactor:`,
  `docs:`, `chore:`, `test:`, followed by a lowercase summary.
- Commit only when the owner asks. Never commit `build/` or editor folders.
- Before calling a change done, check that `make format` produces no diff,
  `make debug` builds with no new warnings, both sample CNFs give the expected
  result, and tests pass (once they exist).
