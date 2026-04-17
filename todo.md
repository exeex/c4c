Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded HIR consteval-call
metadata route in `hir_ir.hpp`, `compile_time_engine.cpp`, and
`frontend_hir_tests.cpp`.

- Added parallel HIR `ConstevalCallInfo::fn_name_text_id` storage so reduced
  consteval-call metadata is no longer string-only for preserved function
  names.
- Threaded consteval-call name interning through pending-consteval evaluation
  using `module.link_name_texts` without collapsing `TextId`, `SymbolId`, or
  `LinkNameId`.
- Extended focused HIR proof so consteval-call metadata resolves its parallel
  `TextId` through the HIR module text table.

## Suggested Next

Keep Step 2 bounded: choose one nearby persistent HIR carrier such as
`InitListItem.field_designator` or `MemberExpr.field` instead of widening into
broader registry/index churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `ConstevalCallInfo::fn_name_text_id` must stay aligned with the legacy
  `fn_name` string anywhere consteval metadata gets synthesized or rewritten.
- Compile-time registries still key consteval definitions by string for
  untouched consumers, so keep this slice limited to parallel storage rather
  than registry/index churn.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
