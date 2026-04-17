Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started Step 2: HIR now owns a live `LinkNameTable` at the materialization
boundary, `hir::Function` / `hir::GlobalVar` / `hir::HirTemplateInstantiation`
carry parallel link-name ids, HIR lowering interns emitted symbol names for
functions/globals/template instances, and `tests/frontend/frontend_hir_tests.cpp`
now exercises a real lowered HIR module to prove those ids materialize and
resolve back to spelling.

## Suggested Next

Start Step 3 from `plan.md`: extend the first bounded LIR carrier path with
parallel link-name ids and forward the HIR-owned ids through one real
HIR->LIR symbol path without collapsing back to string-only reconstruction.

## Watchouts

- `hir::Module` currently owns the link-name text table for this slice; keep
  the source-idea goal in view and avoid hardening that into a permanent
  second string store once the shared TU text-table boundary is available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- Step 3 should forward the new ids explicitly into LIR rather than reading
  back `Function.name` / `GlobalVar.name` as an implicit side channel
- late consumer lookup is still string-based today, so the next slice should
  stay on one bounded HIR->LIR path before touching backend/text emission
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
Log: `test_after.log`
