Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded HIR parameter-name carrier
route in `hir_ir.hpp`, `hir_functions.cpp`, `hir_build.cpp`, and
`hir_stmt.cpp`.

- Added parallel HIR `Param::name_text_id` storage so ordinary lowered
  functions, bodyless consteval functions, and synthetic method `this`
  parameters are no longer string-only for their stable HIR-owned spelling.
- Threaded HIR-owned text interning through each parameter construction site
  without collapsing `TextId`, `SymbolId`, or `LinkNameId`.
- Extended focused HIR proof so ordinary function params, consteval params,
  and method params all resolve their parallel `TextId`s through the module
  text table.

## Suggested Next

Keep Step 2 bounded: choose one nearby declaration-side HIR carrier that still
stores stable TU text as `std::string`, such as `HirTemplateDef.name`, instead
of widening into broader registry/index churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `Param::name_text_id` now needs to stay aligned anywhere new synthetic or
  bodyless parameter construction paths are introduced.
- Method-function lookup names are not a stable proof surface here; prefer
  matching the lowered parameter shape in tests instead of assuming one
  emitted constructor spelling.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
