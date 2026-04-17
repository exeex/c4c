Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 3 / Step 4 for the next bounded qualified decl-ref
carrier adjacent to the namespace-qualifier route.

- Added parallel HIR `DeclRef::name_text_id` storage so the qualified decl-ref
  base name is no longer string-only on the migrated parser-to-HIR route.
- Added shared lowering helpers that intern HIR-side text into the module text
  table instead of reusing parser-owned `TextId` values across table
  boundaries.
- Extended focused HIR proof for `inner::helper` so the decl-ref now preserves
  both qualifier-segment `TextId`s and the unqualified base-name `TextId`
  alongside the existing `LinkNameId` path.

## Suggested Next

Keep Step 3 bounded: extend `DeclRef::name_text_id` coverage to the other
AST-origin decl-ref construction sites that do not flow through
`lower_var_expr(...)`, or stop and choose the next nearby HIR declaration/tag
base-name carrier if that broader decl-ref sweep would widen too far.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- HIR must intern its own spellings into `module.link_name_texts`; parser-owned
  `TextId`s cannot be copied directly into HIR because they belong to a
  different table.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
