Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended Step 4 into an active backend-side consumer: semantic `lir_to_bir`
lowering now resolves function/global symbol spelling from existing
`LinkNameId` carriers before it keys backend-facing BIR state, and
`tests/frontend/frontend_hir_tests.cpp` proves backend lowering still emits the
semantic names after the legacy raw LIR function/global name carriers are
deliberately corrupted.

## Suggested Next

Review the remaining link-visible late-consumer surfaces and pick the next
bounded active backend consumer that already has a real `LinkNameId` carrier,
such as declaration-lowering or another prepared backend-facing module path,
without inventing ids for unresolved extern-call declarations.

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
- keep forwarding explicit ids through LIR carriers and resolve them only at
  late consumers rather than treating legacy `name` strings as the semantic
  source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
Log: `test_after.log`
