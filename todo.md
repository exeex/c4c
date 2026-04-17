Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended Step 4 through another late text-emitting boundary: LIR
specialization metadata now carries `mangled_link_name_id` alongside the legacy
string, `lir::print_llvm` resolves metadata-emitted mangled names through the
shared `LinkNameId` table when available, and
`tests/frontend/frontend_hir_tests.cpp` proves the printer still emits the
semantic specialization name after the raw metadata string is deliberately
corrupted.

## Suggested Next

Review the remaining link-visible late-consumer surfaces to decide whether the
next bounded slice should migrate another printer-emitted metadata/declaration
path or move from LLVM text emission into a backend-side symbol spelling
boundary that can resolve `LinkNameId` instead of trusting legacy strings.

## Watchouts

- `LirModule` currently shares HIR’s link-name table to make the printer route
  real; keep the source-idea goal in view and avoid hardening that into a
  permanent second string store once the shared TU text-table boundary is
  available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- unresolved extern-call declarations still print names from raw strings today
  because this route does not yet have a real `LinkNameId` source of truth;
  avoid faking that boundary with ad hoc interning
- keep forwarding explicit ids through LIR carriers rather than treating
  `name` strings as the semantic source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^cpp_llvm_.*specialization_metadata'`
Log: `test_after.log`
