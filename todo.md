Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started Step 3 on the direct emitted-function path: `codegen::lir::LirFunction`
now carries a parallel `link_name_id`, HIR-to-LIR lowering forwards
`hir::Function.link_name_id` for both declarations and definitions, and
`tests/frontend/frontend_hir_tests.cpp` now lowers a real HIR fixture into LIR
to prove the forwarded id survives on the first bounded HIR->LIR carrier.

## Suggested Next

Continue Step 3 with the next bounded symbol family: decide whether
`LirGlobal`, specialization metadata, or another directly emitted link-visible
carrier should be the second explicit id path before widening into late
consumer lookup in Step 4.

## Watchouts

- `hir::Module` currently owns the link-name text table for this slice; keep
  the source-idea goal in view and avoid hardening that into a permanent
  second string store once the shared TU text-table boundary is available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- the first Step 3 slice proves only the emitted-function carrier; `LirGlobal`
  and late consumer lookup are still string-based today
- keep forwarding explicit ids through LIR carriers rather than treating
  `name` strings as the semantic source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
Log: `test_after.log`
