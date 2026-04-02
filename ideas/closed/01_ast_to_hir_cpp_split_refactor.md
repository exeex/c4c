# `ast_to_hir.cpp` Split Refactor

Status: Complete
Last Updated: 2026-04-02

## Goal

Split `src/frontend/hir/ast_to_hir.cpp` (10,210 lines) into a declaration-oriented header plus multiple focused implementation files so the full lowering surface is visible from one place and individual edit sessions do not require loading the whole translation unit.

## Why this plan

`src/frontend/hir/ast_to_hir.cpp` is now the largest file in the tree. It currently mixes:

- public entrypoints
- model/helper type definitions
- the full `Lowerer` class definition
- function prototypes and inline method declarations
- lowering implementations across unrelated domains

This makes the file expensive to navigate for both humans and AI agents. A contributor cannot inspect one header and understand the available lowering APIs, helper types, and ownership boundaries the way they can with [`src/frontend/parser/parser.hpp`](/workspaces/c4c/src/frontend/parser/parser.hpp).

## Refactor Objective

Adopt the same shape used by the parser:

- move class definitions, nested structs, enums, and function declarations into `src/frontend/hir/ast_to_hir.hpp`
- keep `ast_to_hir.hpp` as the single place that describes the HIR lowering surface
- split implementations into multiple `hir_*.cpp` files grouped by responsibility
- leave behavior unchanged; this is a structural refactor only

## Proposed Header Surface

`src/frontend/hir/ast_to_hir.hpp` should grow from a thin entrypoint header into the authoritative declaration file for HIR lowering, including:

- `InitialHirBuildResult`
- helper structs currently defined near the top of `ast_to_hir.cpp`
- `LayoutQueries`
- `Lowerer`
- method declarations for `Lowerer`
- free-function declarations such as `build_initial_hir(...)`

The goal is that an agent can open one header and immediately see:

- which types exist
- which lowering phases and helper families exist
- which methods belong to which responsibility cluster

## Proposed Split

| New File | Content | Estimated Lines |
|----------|---------|-----------------|
| `hir_build.cpp` | `build_initial_hir`, top-level orchestration, module setup, initial program lowering coordination | ~1200 |
| `hir_templates.cpp` | template arg helpers, deferred NTTP parser, template struct resolution, instantiation helpers | ~2600 |
| `hir_functions.cpp` | function lowering, method lowering, local function context setup, parameter/result binding | ~1800 |
| `hir_expr.cpp` | expression lowering, operator handling, overload/member operator resolution | ~2600 |
| `hir_types.cpp` | type reconstruction, layout queries, struct field/type helpers, destructor-related type helpers | ~1400 |
| `hir_stmt.cpp` | statement lowering, control flow, declaration statements, range-for / cleanup emission | ~1800 |

Exact boundaries can change during execution, but the split should follow semantic ownership rather than arbitrary line counts.

## Constraints

- Pure structural refactor: no intended logic changes
- `src/frontend/hir/ast_to_hir.hpp` becomes the canonical declaration surface
- Add corresponding entries to the frontend HIR build rules (`CMakeLists.txt` or equivalent)
- Keep internal helpers private to the implementation files unless cross-file sharing requires declaration in the header or a dedicated private helper header
- Prefer a small number of cohesive files over many tiny fragments
- Existing tests must pass unchanged after the split

## Execution Notes

Recommended order:

1. Expand `ast_to_hir.hpp` first so it mirrors the parser-style "single header shows the whole class" layout.
2. Make `ast_to_hir.cpp` compile as a temporary coordinator file with moved definitions.
3. Peel off one responsibility cluster at a time into `hir_*.cpp`.
4. Remove the monolithic implementation file once all methods have moved.

If cross-file private declarations become noisy, introduce one internal helper header such as `hir_lowerer_internal.hpp`, but keep the public/agent-facing surface in `ast_to_hir.hpp`.

## Acceptance Criteria

- [x] `src/frontend/hir/ast_to_hir.hpp` contains the full declaration surface for HIR lowering
- [x] `Lowerer` is no longer defined in `src/frontend/hir/ast_to_hir.cpp`
- [x] `src/frontend/hir/ast_to_hir.cpp` is removed or reduced to a thin forwarding/coordinator file
- [x] HIR lowering implementation is split across multiple focused `hir_*.cpp` files
- [x] `cmake --build build -j8` succeeds
- [x] `ctest --test-dir build -j 8` shows no regressions

## Completion Notes

- `ast_to_hir.hpp` now serves as the single declaration-oriented entrypoint for the lowering surface.
- HIR lowering ownership is split across focused `hir_build.cpp`, `hir_templates.cpp`, `hir_functions.cpp`, `hir_expr.cpp`, `hir_types.cpp`, and `hir_stmt.cpp` units that are compiled by the main build.
- `src/frontend/hir/ast_to_hir.cpp` is now reduced to shared coordinator/layout utilities and no longer carries the monolithic `Lowerer` implementation or dead `#if 0` staging copies.
- Validation on 2026-04-02: `cmake --build build -j8` succeeded; `ctest --test-dir build -j8 --output-on-failure > test_fail_after.log` finished at 2671 total tests, 2668 passing, with the same 3 historical failing tests (`positive_sema_linux_stage2_repro_03_asm_volatile_c`, `backend_lir_adapter_aarch64_tests`, and `llvm_gcc_c_torture_src_20080502_1_c`); `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed` passed with zero new failing tests.

## Non-Goals

- changing lowering order
- fixing semantic bugs discovered during the refactor unless required to restore previous behavior
- redesigning `Lowerer` ownership or public API beyond what is needed for the file split
