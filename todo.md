Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Taught the LIR printer to resolve direct global-call callees through
`direct_callee_link_name_id` at the late text-emission boundary instead of
trusting the raw `@name` operand; `tests/frontend/frontend_hir_tests.cpp` now
proves LLVM text emission still prints `@helper(...)` after the legacy
direct-call operand is deliberately corrupted.

## Suggested Next

Review the remaining late-consumer surfaces that still expose raw
declaration-oriented names despite existing carriers, especially unresolved
extern-call declarations and any backend diagnostic/reporting paths, while
continuing to avoid fake ids where HIR has no real semantic source of truth.

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
- the new direct-call carrier only becomes valid when the callee resolves to an
  existing HIR `Function`; intrinsic/builtin aliases and unresolved extern
  calls should continue to flow with `kInvalidLinkName`
- keep forwarding explicit ids through LIR carriers and resolve them only at
  late consumers rather than treating legacy `name` strings as the semantic
  source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
Result: passed
Log: `test_after.log`
