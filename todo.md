Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Carried `LinkNameId` through `LirExternDecl` so late consumer boundaries now
resolve extern declaration spellings from ids when one is already present on
the carrier, while leaving unresolved externs without a real source of truth
at `kInvalidLinkName`.

## Suggested Next

Review backend diagnostic and reporting paths that still look at raw LIR
names instead of the resolved module view, and only extend `LinkNameId`
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
