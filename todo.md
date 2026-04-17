Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed the first parser-to-HIR `TextId` handoff slice on qualified-name
namespace qualifiers.

- Added parallel AST `TextId` fields for `Node::unqualified_name` and
  `Node::qualifier_segments` so parser `QualifiedNameRef` identity survives the
  parser/AST boundary.
- Added parallel HIR `NamespaceQualifier::segment_text_ids` and threaded
  `make_ns_qual(...)` through the module text table so qualified decl-ref and
  declaration namespace qualifiers are no longer string-only on the migrated
  route.
- Added focused parser and HIR tests covering AST qualified-name handoff and a
  qualified decl-ref lowering path (`inner::helper`) that now preserves HIR
  qualifier `TextId`s.

## Suggested Next

Start Step 3 from `plan.md`: inspect the new `NamespaceQualifier` consumers and
confirm they continue to treat qualifier text as `TextId`-backed TU identity
without collapsing into `SymbolId` or `LinkNameId`, then choose the next
bounded HIR string carrier adjacent to this route (for example a decl/tag base
name field) instead of broadening into general string cleanup.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Baseline:
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$'`

Packet proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$'`

Result:
Passed on 2026-04-17 via `test_after.log`.
