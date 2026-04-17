Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Forwarded `LinkNameId` through resolved direct-call LIR carriers and taught
backend direct-call lowering to prefer that semantic callee identity over a
corrupted raw `@name` operand; `tests/frontend/frontend_hir_tests.cpp` now
proves BIR call lowering still emits `helper` after the legacy direct-call
carrier is deliberately corrupted.

## Suggested Next

Review the remaining late-consumer paths that still surface raw direct-call or
declaration-oriented names despite existing carriers, especially any LIR-side
text-emission or backend diagnostic/reporting surfaces, while continuing to
avoid fake ids for unresolved extern-call declarations.

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
Log: `test_after.log`
