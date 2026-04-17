Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded HIR `MemberExpr.field`
route in `hir_ir.hpp`, lowering sites that construct `MemberExpr`, and
`frontend_hir_tests.cpp`.

- Added parallel HIR `MemberExpr::field_text_id` storage so lowered field
  accesses are no longer string-only for preserved member names.
- Threaded member-name interning through the bounded `MemberExpr` construction
  paths using `module.link_name_texts` without collapsing `TextId`,
  `SymbolId`, or `LinkNameId`.
- Extended focused HIR proof so a lowered member access resolves its parallel
  `TextId` through the HIR module text table.

## Suggested Next

Keep Step 2 bounded: choose one nearby persistent HIR carrier such as
`InitListItem.field_designator` or `DeclStmt.symbol` instead of widening into
broader registry/index churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `MemberExpr::field_text_id` must stay aligned with the legacy `field` string
  anywhere member expressions get synthesized or rewritten.
- Several aggregate/object helpers synthesize `MemberExpr` nodes outside the
  main AST member-lowering path, so future slices must update those helpers if
  `MemberExpr` gains more parallel identity state.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
