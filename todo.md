Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded template-instantiated HIR
struct-definition field-name route in
`hir_templates_struct_instantiation.cpp` and `frontend_hir_tests.cpp`.

- Added the missing `field_text_id` assignment in
  `instantiate_template_struct_field` so template-instantiated
  `HirStructField` metadata now interns stable field spellings into
  `module.link_name_texts` on the same route as non-template struct defs.
- Extended the existing struct-definition HIR test to prove instantiated struct
  fields preserve a valid `TextId` and still print the original field spelling
  after the legacy raw field string is intentionally corrupted.

## Suggested Next

Choose one remaining string-only HIR text carrier outside struct-definition
metadata, such as `PendingConstevalExpr::fn_name`, and prove it in
`frontend_hir_tests` without widening into registry churn.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- The struct-definition field-name route now covers both non-template and
  template-instantiated `HirStructField` construction; keep future proof
  focused on consumers that can survive intentional corruption of the legacy
  raw string sidecar.
- HIR-side `TextId` tests should keep proving the route still works after the
  legacy string sidecar is intentionally corrupted, so the migration is not
  passive storage only.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
