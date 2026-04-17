Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Hardened backend `lir_to_bir` failure-note reporting so function-level
diagnostic paths resolve semantic names from `LinkNameId` when present instead
of trusting a possibly corrupted raw LIR `name`, and added backend notes
coverage that proves the reported function name comes from the id-backed path.

## Suggested Next

Review whether any remaining backend module-level or non-failure reporting
surfaces still depend on raw LIR symbol names, and only extend `LinkNameId`
carriers further when a real HIR or LIR semantic source exists.

## Watchouts

- `LirModule` currently shares HIR’s link-name table to make the printer route
  real; keep the source-idea goal in view and avoid hardening that into a
  permanent second string store once the shared TU text-table boundary is
  available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- unresolved extern-call declarations still only get semantic names if an
  upstream carrier already has a real `LinkNameId`; do not fake that boundary
  by interning raw fallback names during lowering
- the new direct-call carrier only becomes valid when the callee resolves to an
  existing HIR `Function`; intrinsic/builtin aliases and unresolved extern
  calls should continue to flow with `kInvalidLinkName`
- this packet only hardened function-level backend failure notes; other
  reporting surfaces should still be audited individually before assuming the
  backend is fully id-driven
- keep forwarding explicit ids through LIR carriers and resolve them only at
  late consumers rather than treating legacy `name` strings as the semantic
  source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed
Log: `test_after.log`
