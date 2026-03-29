# std::vector Bring-up Plan

Status: Parked
Last Updated: 2026-03-29

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

After the recent frontend refactors, the direct repro no longer dies in the
older unmatched-brace / parse-state-loss shape that previously ended as
`expected RBRACE`.

The current frontier is earlier and more structured:

- typedef / alias setup inside libstdc++ iterator and allocator scaffolding
- function-style declarator parsing with template and reference-heavy signatures
- expression parsing inside libstdc++ helper layers such as `predefined_ops`
  and `stl_algobase`

That is a meaningful improvement: the parser is now getting through many earlier
header hazards and exposing narrower, more actionable blockers.

## Current Status

As of 2026-03-29, this bring-up has moved past the earlier libstdc++ parser
frontline blockers that were showing up in `<type_traits>` and
`ext/numeric_traits.h`.

The following header-compatibility gaps were reduced and fixed:

- template-template parameters in template parameter lists
  - example shape: `template<typename...> class Op`
- qualified typedef non-type template parameters
  - example shape: `template<typename T, std::size_t N>`
- dependent template-template application in type context
  - example shape: `using type = Op<Args...>;`
- dependent enum initializers that use `sizeof(type)` and ternary expressions
  - example shape: `enum { width = __value ? sizeof(T) * 8 : 0 };`

Reduced regression tests added during this slice:

- `tests/cpp/internal/postive_case/template_template_param_parse.cpp`
- `tests/cpp/internal/postive_case/template_qualified_nttp_parse.cpp`
- `tests/cpp/internal/postive_case/template_dependent_enum_sizeof_parse.cpp`
- `tests/cpp/internal/postive_case/access_labels_parse.cpp`
- `tests/cpp/internal/postive_case/access_labels_treated_public_runtime.cpp`
- `tests/cpp/internal/postive_case/friend_access_parse.cpp`
- `tests/cpp/internal/postive_case/friend_inline_operator_parse.cpp`

These reduced tests are now registered in `ctest` and passing.

Current direct repro status for `tests/cpp/std/std_vector_simple.cpp`:

- preprocessing succeeds
- the previous `ext/numeric_traits.h:57` dependent-enum failure is gone
- the previous late `expected RBRACE` failure is no longer the lead blocker
- the current parse-only repro now fails earlier in libstdc++ with errors such as:
  - `/usr/include/c++/14/bits/stl_iterator.h:1494:15: error: typedef uses unknown base type: _Iterator`
  - `/usr/include/c++/14/bits/predefined_ops.h:150:32: error: expected RPAREN but got '__comp'`
  - `/usr/include/c++/14/bits/stl_algobase.h:971:11: error: unexpected token in expression: const`
  - `/usr/include/c++/14/bits/new_allocator.h:70:15: error: typedef uses unknown base type: _Tp`
  - `/usr/include/c++/14/bits/stl_uninitialized.h:179:61: error: unexpected token in expression: ,`

Interpretation:

- the bring-up has advanced through the first wave of system-header
  compatibility failures
- the recent refactor appears to have closed many of the earlier
  parse-state-loss holes
- the next task is to reduce and localize the new libstdc++ frontier around
  typedef-base resolution, declarator parsing, and expression parsing in the
  iterator / allocator / algorithm helper stack
- the work is still in Phase B moving into Phase C; it has not yet reached
  `std::vector` semantics or codegen validation

### Adjacent Work Parked Separately

During reduction of the current libstdc++ parser failures, it became clear that
the error surface from messages such as:

- `expected RPAREN but got '&'`
- `expected RPAREN but got '&&'`
- `unexpected token in expression: const`

was too flat to cleanly expose the real parser entry point that failed.

An instrumentation experiment was started on the branch to add parser-side
failure tracking, a `--parser-debug` flag, and `ParseContextGuard`-based parse
function tracing. That work has been split into the separate open idea:

- `ideas/open/parser_error_diagnostics_plan.md`

Reason for the split:

- it is adjacent and useful, but not strictly required to keep the
  `std::vector` bring-up scoped to the current header blocker
- it changes parser observability rather than the language support behavior
- it should proceed as its own focused diagnostic-improvement initiative

Update from the completed diagnostics plan on 2026-03-29:

- the parser-debug runbook is now complete and archived separately
- the remaining
  `/usr/include/c++/14/bits/stl_bvector.h:663:34` `got='&&'` top-level wrapper
  family was rechecked against its reduced repro and does not currently need
  additional committed-failure vs no-match bookkeeping
- that means the surviving `std::vector` work stays on language-support
  bring-up, not further diagnostic ranking changes, until a new libstdc++
  header family proves otherwise


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

### Current Manual Workflow

There is currently a standalone CMake workflow target for the EASTL type-traits
repro case:

- target: `eastl_type_traits_simple_workflow`
- source: `tests/cpp/eastl/eastl_type_traits_simple.cpp`
- script: `tests/cpp/eastl/run_eastl_type_traits_simple_workflow.cmake`

Run it with:

```bash
cmake --build build --target eastl_type_traits_simple_workflow
```

This workflow is intentionally **not** part of the default `ctest` set.
It exists to make the current EASTL bring-up path easy to reproduce without
promoting an ad hoc workflow into the regular suite.

The workflow performs:

1. host compile of `tests/cpp/eastl/eastl_type_traits_simple.cpp` with EASTL/EABase include paths
2. host execution of that reference binary
3. `c4cll` compile of the same source with matching include paths, emitting LLVM IR
4. host `clang` compile of the emitted IR
5. execution of the resulting c4c-produced binary

Artifacts are written under:

- `build/eastl_type_traits_simple/eastl_type_traits_simple.ll`
- `build/eastl_type_traits_simple/eastl_type_traits_simple.host.bin`
- `build/eastl_type_traits_simple/eastl_type_traits_simple.c4c.bin`

The current EASTL parser/sema regression slices are registered in `ctest`.
To inspect their up-to-date names from the configured build tree, use:

```bash
ctest --test-dir build -N | rg 'cpp_positive_sema_eastl'
```

### Exit Criteria

- one command reproduces current status
- one command compares legacy vs new codegen output once codegen succeeds

### Current Repro Command

At the moment the cheapest direct repro command is:

```bash
./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp
```

Useful companion commands:

```bash
./build/c4cll --pp-only tests/cpp/std/std_vector_simple.cpp > /tmp/std_vector_simple.pp
./build/c4cll --parser-debug --parse-only tests/cpp/std/std_vector_simple.cpp
./build/c4cll tests/cpp/std/std_vector_simple.cpp -o /tmp/std_vector_simple.ll
```

Use the parser-debug path when the default parse error is too flat to expose the
real failing parser entry point, especially for messages such as:

- `expected RPAREN but got ...`

## Parking Update

This idea was deactivated on 2026-03-29 without being completed.

### Latest Completed Slices

- Added `tests/cpp/internal/postive_case/if_condition_decl_parse.cpp` and implemented C++ `if` declaration-condition parsing in `src/frontend/parser/statements.cpp`.
- Added `tests/cpp/internal/postive_case/free_function_record_ref_param_parse.cpp` and fixed C++ record-tag injection / namespace-local class-name lookup so free-function `E&` / `E&&` parameters parse.
- Added `tests/cpp/internal/postive_case/cpp20_feature_macro_branch_parse.cpp` and updated predefined C++ macros in `src/frontend/preprocessor/preprocessor.cpp` to advertise `__cplusplus=202002L`, `__cpp_constexpr=201811L`, and `__cpp_concepts=201907L`.
- Added `tests/cpp/internal/postive_case/cpp20_requires_clause_parse.cpp` and updated `src/frontend/parser/declarations.cpp` to skip an optional requires-clause between `template<...>` and the following declaration.
- Added `tests/cpp/internal/postive_case/cpp20_constrained_template_param_parse.cpp` for the reduced constrained-template-parameter frontier in `bits/iterator_concepts.h`.
- Added `tests/cpp/internal/postive_case/qualified_record_partial_specialization_parse.cpp` for the reduced qualified record partial-specialization frontier in `bits/iterator_concepts.h`.
- Added `tests/cpp/internal/postive_case/record_final_specifier_parse.cpp` for the reduced record-`final` frontier in `bits/iterator_concepts.h`.
- Added `tests/cpp/internal/postive_case/cpp20_requires_expression_if_constexpr_parse.cpp` for the reduced `if constexpr (requires { ... })` frontier from `bits/stl_iterator.h:2292`.

### Current Blocker

`tests/cpp/std/std_vector_simple.cpp` no longer fails at the old `stl_iterator.h:2292` requires-expression site. The current remaining frontier is a parameter-list / incomplete-type cluster that starts with:

- `/usr/include/c++/14/bits/exception.h:65:30` `expected=RPAREN got='&'`
- `/usr/include/c++/14/bits/exception.h:67:24` `expected=RPAREN got='&&'`
- `/usr/include/c++/14/bits/stl_iterator.h:1011`
- `/usr/include/c++/14/bits/stl_iterator.h:1572`
- `/usr/include/c++/14/bits/stl_iterator.h:1954`
- `/usr/include/c++/14/bits/stl_iterator.h:1978`

The remaining diagnostics appear to depend on surrounding parser state or recovery, so the next useful step is to reduce that contextual failure into a committed standalone repro before attempting another parser fix.

### Regression Status At Parking Time

- A clean full-suite `ctest` run no longer failed on `verify_tests_verify_top_level_recovery`.
- The known remaining suite failures at parking time were seven backend LIR variadic ABI tests:
  - `backend_lir_aarch64_variadic_dpair_ir`
  - `backend_lir_aarch64_variadic_float_array_ir`
  - `backend_lir_aarch64_variadic_nested_float_array_ir`
  - `backend_lir_aarch64_variadic_float4_ir`
  - `backend_lir_aarch64_variadic_double4_ir`
  - `backend_lir_aarch64_variadic_single_double_ir`
  - `backend_lir_aarch64_variadic_single_float_ir`

### Resume Point

If this idea is re-activated, resume from:

```bash
./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp
```

Then reduce the first still-contextual `exception.h` / `stl_iterator.h` parameter-list failure into the smallest committed repro, land the narrowest parser fix, and re-run targeted C++ parser coverage before the next full-suite regression pass.
- `unexpected token in expression: ...`

In those cases, inspect the `--parser-debug` output first to see which parse
function stack led to the failure before reducing or changing parser behavior.


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
- `template<typename...> class Op`
- `template<typename T, std::size_t N>`
- `enum { width = __value ? sizeof(T) * 8 : 0 };`

2. fix the narrowest responsible layer

- lexer
- preprocessor
- parser
- scope/type lookup

3. add one regression test per reduced blocker

### Exit Criteria

- libstdc++ gets significantly farther than the current first-header failure
- each newly fixed blocker has a tiny test that does not depend on `<vector>`

### Phase B Progress

Completed in this slice:

- template-template parameter parsing
- qualified typedef NTTP parsing
- dependent enum initializer parsing for `sizeof(type)` plus ternary

Reduced tests now covering this progress:

- `template_template_param_parse`
- `template_qualified_nttp_parse`
- `template_dependent_enum_sizeof_parse`

Remaining immediate blocker after these fixes:

- reduce the new libstdc++ failures currently exposed by `std_vector_simple.cpp`
- prioritize the first structured blockers in:
  - `bits/stl_iterator.h`
  - `bits/predefined_ops.h`
  - `bits/stl_algobase.h`
  - `bits/new_allocator.h`
  - `bits/stl_uninitialized.h`

Additional parser coverage now in place around class-body recovery:

- `public:` / `protected:` / `private:` labels are tolerated as structural
  markers in class bodies
- `friend` declarations and inline `friend` operator definitions no longer
  terminate the enclosing class parse at the function-body `}`

Important limitation:

- this work does **not** implement real C++ access-control semantics yet
- for now these access labels are only covered as parser/front-end structure;
  the regular host-clang-backed runtime suite cannot validate a
  "treat private/protected as public" approximation


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

### Current Assessment

This phase is not complete yet, but the case is meaningfully closer:

- it no longer stops on the earlier `<type_traits>` template-parameter failures
- it no longer stops on `ext/numeric_traits.h:57`
- it no longer leads with the older unmatched-brace / `expected RBRACE` failure
- it now fails deeper in libstdc++ with narrower parser/sema issues around
  iterator, allocator, and helper-expression scaffolding
- it still does not reach `std::vector` semantics or lowering


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
