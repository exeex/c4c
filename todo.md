Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Source Plan: plan.md

# Current Packet

## Just Finished

Activation only. No executor packet has completed yet.

## Suggested Next

Start Step 1 from `plan.md`: choose one existing parser `TextId` carrier and
one adjacent HIR string-backed carrier that can be migrated through a single
parser-to-HIR route.

## Watchouts

- Do not collapse `TextId`, `SymbolId`, and `LinkNameId`.
- Keep diagnostic/debug/serialization strings out of scope.
- Do not absorb unrelated EASTL lifecycle churn into this plan.

## Proof

Pending. Close-time guard for idea 14 uses `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`.
