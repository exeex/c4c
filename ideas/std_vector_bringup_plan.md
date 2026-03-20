# std::vector Bring-up Plan

## Goal

Push `tests/cpp/std/std_vector_simple.cpp` from current failure state to a stable,
regression-guarded passing state.

This plan is intentionally narrow:

- target one concrete standard-library bring-up path,
- use it as a forcing function for system-header compatibility,
- keep fixes minimal and layered,
- avoid broad "support all libstdc++" promises.


## Why This Case Matters

`tests/cpp/std/std_vector_simple.cpp` is a good early end-to-end target because it
exercises several layers at once:

- system header preprocessing
- GNU / libstdc++ internal dialect handling
- namespace and implementation-identifier lookup
- templates and inline namespaces
- codegen for a real C++ standard container usage pattern

It is also small enough to debug incrementally.


## Current Failure Shape

The current failure is not just "vector is unsupported".

It has already shown dependency on:

- `decltype`
- namespace attributes
- `extern "C++"` wrappers
- inline namespace handling
- implementation identifiers such as `__true_type`
- libstdc++ internal typedef / trait scaffolding

That makes this test a useful staged bring-up target after the codegen refactor
guardrails are in place.


## Scope

This plan covers only what is needed to get this case passing with good regression signal.

Do:

- fix parser / preprocessor / semantic gaps exposed directly by this case
- add targeted reduced repro tests as each blocker is found
- keep each compatibility fix attached to a concrete failing layer

Do not:

- add giant allowlists of random `__xxx` names
- promise broad STL completeness from one passing test
- mix unrelated frontend feature work into this bring-up


## Dependency On Main Refactor

This bring-up should start **after** the main codegen refactor has the following in place:

1. `--codegen=legacy|lir|compare`
2. compare mode that can emit two `.ll` files
3. first smoke tests for compare mode

Reason:

- we want new frontend fixes to be checked against both old and new codegen paths
- if `std::vector` starts lowering differently under the new path, we want instant
  visibility


## Work Phases

## Phase A: Lock In A Repro Harness

### Objective

Make the failing case cheap to rerun and easy to reduce.

### Tasks

1. add a dedicated quick-run target or documented command for

- `tests/cpp/std/std_vector_simple.cpp`

2. add compare-mode invocation for this case once compare mode exists

3. preserve reduced temporary repro files when the failing layer is unclear

### Exit Criteria

- one command reproduces current status
- one command compares legacy vs new codegen output once codegen succeeds


## Phase B: Reduce Frontend Header Blockers

### Objective

Systematically remove the parser / pp / sema blockers that stop libstdc++ headers
before `vector` semantics are even reached.

### Tasks

1. reduce each blocker into a tiny standalone test

Examples:

- `decltype(nullptr)`
- `namespace std __attribute__((...)) { ... }`
- `extern "C++" { ... }`
- `inline namespace __cxx11 { ... }`
- `typedef __true_type __type;`

2. fix the narrowest responsible layer

- lexer
- preprocessor
- parser
- scope/type lookup

3. add one regression test per reduced blocker

### Exit Criteria

- libstdc++ gets significantly farther than the current first-header failure
- each newly fixed blocker has a tiny test that does not depend on `<vector>`


## Phase C: Reach A Successful Parse / Lowering Boundary

### Objective

Get the `std_vector_simple.cpp` case past parsing and semantic setup into codegen.

### Tasks

1. continue reducing failures as they move deeper
2. distinguish header-compatibility failures from container semantics failures
3. keep fixes localized and documented

### Exit Criteria

- `std_vector_simple.cpp` no longer fails in the first wave of system headers
- the remaining failures, if any, are closer to actual `std::vector` usage


## Phase D: Validate Under Both Codegen Paths

### Objective

Use the main refactor's compare mode to ensure bring-up work does not silently
diverge new and old codegen.

### Tasks

1. run `--codegen=legacy`
2. run `--codegen=lir`
3. run `--codegen=compare`

4. treat mismatches as triage input

- frontend bug
- lowering difference
- printer difference

### Exit Criteria

- once the test compiles, both codegen paths are checked
- the case becomes part of compare-mode smoke coverage


## Phase E: Promote To Regression Guard

### Objective

Turn this from an ad hoc bring-up target into a maintained signal.

### Tasks

1. keep `tests/cpp/std/std_vector_simple.cpp` in the regular suite
2. add it to compare-mode smoke coverage
3. document known remaining STL limits nearby if needed

### Exit Criteria

- the case stays green
- future regressions are caught quickly


## Success Criteria

This plan is complete when:

- `tests/cpp/std/std_vector_simple.cpp` compiles successfully
- reduced blocker tests exist for the header-compatibility issues it exposed
- the case is validated under both legacy and new codegen paths
- the case participates in regression protection instead of remaining a one-off debug target
