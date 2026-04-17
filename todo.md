Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Audited the remaining owned backend `lir_to_bir` module-level and non-failure
reporting surfaces and confirmed they already resolve semantic names from
`LinkNameId` before reporting or lowering, so no further code change was needed
in this packet beyond recording the audit result.

## Suggested Next

Audit late consumer boundaries outside this backend `lir_to_bir` slice only
where final symbol spelling is still emitted or reported, and only add new
`LinkNameId` plumbing when a real upstream semantic carrier already exists.

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
  backend is fully id-driven, but within the owned `lir_to_bir` module,
  analysis, and calling files the remaining declaration/global/function paths
  already route through resolved semantic names before they report or lower
- `lower_extern_decl` and `lower_decl_function` still copy `decl.name` /
  `function.name`, but their only owned caller is `lower_module`, which
  resolves semantic names first; do not duplicate fallback lookup inside those
  helpers unless a new direct caller appears
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
