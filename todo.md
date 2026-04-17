Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 / Step 4 for the bounded HIR struct-definition
field-name route in `HirStructField`, `hir_printer.cpp`, and
`frontend_hir_tests.cpp`.

- Added parallel `HirStructField::field_text_id` storage and populated it while
  lowering non-template struct definitions so stable field spellings no longer
  depend on raw string ownership alone on that route.
- Switched HIR struct printing to prefer the new field-name `TextId` carrier
  before falling back to the legacy raw field spelling.
- Added focused HIR proof that corrupting the legacy raw struct field string
  does not break the printed field spelling while the `TextId` carrier is
  present.

## Suggested Next

Extend the same bounded struct-field-name pattern to template-instantiated
`HirStructField` construction so instantiated struct defs preserve parallel
field `TextId`s too, then keep proof in `frontend_hir_tests`.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- `HirStructField::field_text_id` is only populated on the non-template
  `lower_struct_def` route so far; template-instantiated field construction
  still needs the same parallel carrier.
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
