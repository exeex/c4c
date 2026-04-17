Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded HIR
`InitListItem.field_designator` route in `hir_ir.hpp`, initializer lowering and
normalization helpers, const-init emission, HIR printing, and
`frontend_hir_tests.cpp`.

- Added parallel HIR `InitListItem::field_designator_text_id` storage so
  designated global-init items are no longer string-only for preserved field
  names.
- Threaded designator-name interning through lower/normalize/const-init helper
  paths using `module.link_name_texts` without collapsing `TextId`,
  `SymbolId`, or `LinkNameId`.
- Extended focused HIR proof so a designated init item still prints and lowers
  correctly after the legacy raw field-designator string is corrupted, proving
  the parallel `TextId` route is live.

## Suggested Next

Keep Step 2 bounded: choose one nearby persistent HIR carrier such as
`DeclStmt.symbol` or `DeclRef.user_name` instead of widening into broader
registry/index churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `InitListItem::field_designator_text_id` must stay aligned anywhere
  designated init items are synthesized, normalized, or rewritten into indexed
  forms.
- HIR printers and const-init/codegen helpers should prefer the `TextId` route
  when it is present so the migration is not just passive parallel storage.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
