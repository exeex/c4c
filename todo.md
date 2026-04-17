Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Updated owned late text-emission sites in `codegen/lir` so function signature
emission and direct global/function reference emission now resolve existing
`LinkNameId` spellings when those carriers are already present, while leaving
unresolved extern fallback names string-backed rather than inventing new ids.

## Suggested Next

Audit the remaining LIR text and constant-initializer emission paths that still
spell global or function names from raw strings, and only migrate sites where
the upstream HIR/LIR carrier already owns a real `LinkNameId`.

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
- `stmt_emitter_expr` and `stmt_emitter_lvalue` now prefer `LinkNameId` only
  for resolved direct globals/functions already present in HIR; unresolved
  extern-call paths still intentionally flow through raw fallback names because
  no upstream semantic carrier exists there yet
- `build_fn_signature` now resolves emitted function spellings late from
  `fn.link_name_id`, but dedup/index selection in HIR and lowering still keys
  off legacy `name` strings and should not be conflated with emission-time
  spelling
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
