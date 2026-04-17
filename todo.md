Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started Step 4 at a real late text-emitting boundary: `codegen::lir::LirModule`
now retains the shared HIR link-name table during lowering, `lir::print_llvm`
resolves emitted function signatures and global definitions through
`LinkNameId` lookup when available, and
`tests/frontend/frontend_hir_tests.cpp` proves the printer still emits the
semantic names after the raw LIR function/global string carriers are
deliberately corrupted.

## Suggested Next

Extend the same late-consumer lookup contract to the remaining link-visible
printer surfaces that still spell names from raw strings, or choose the next
backend/text consumer boundary that should resolve `LinkNameId` instead of
trusting legacy string fields.

## Watchouts

- `LirModule` currently shares HIR’s link-name table to make the printer route
  real; keep the source-idea goal in view and avoid hardening that into a
  permanent second string store once the shared TU text-table boundary is
  available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- extern declarations and specialization metadata still print names from raw
  strings today even though the emitted function/global route now resolves at
  the printer boundary
- keep forwarding explicit ids through LIR carriers rather than treating
  `name` strings as the semantic source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
Log: `test_after.log`
