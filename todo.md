Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Added a parallel `DeclRef.link_name_id` carrier for direct-call callees in HIR
and threaded it into `StmtEmitter` so unresolved extern-call declarations and
direct call sites now preserve upstream semantic names instead of depending
only on raw fallback strings.

## Suggested Next

Audit the remaining direct-name consumers that still key lookup or dedup by raw
`name` strings even though call lowering and late emission now have real
parallel `LinkNameId` carriers; keep the next slice semantic and avoid turning
`DeclRef` into a repo-wide migration.

## Watchouts

- `LirModule` currently shares HIR’s link-name table to make the printer route
  real; keep the source-idea goal in view and avoid hardening that into a
  permanent second string store once the shared TU text-table boundary is
  available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- unresolved direct-call decl refs now intern `LinkNameId` in HIR call
  lowering, but builtin alias calls still intentionally use
  `kInvalidLinkName` because they do not originate from an HIR semantic
  carrier
- this packet only hardened function-level backend failure notes; other
  reporting surfaces should still be audited individually before assuming the
  backend is fully id-driven, but within the owned `lir_to_bir` module,
  analysis, and calling files the remaining declaration/global/function paths
  already route through resolved semantic names before they report or lower
- `lower_extern_decl` and `lower_decl_function` still copy `decl.name` /
  `function.name`, but their only owned caller is `lower_module`, which
  resolves semantic names first; do not duplicate fallback lookup inside those
  helpers unless a new direct caller appears
- plain `foo()` direct-call lowering now patches the callee `DeclRef` in place
  after `lower_expr`, so this route stays bounded to call sites instead of
  teaching every `DeclRef` construction path about `LinkNameId`
- `build_fn_signature` now resolves emitted function spellings late from
  `fn.link_name_id`, but dedup/index selection in HIR and lowering still keys
  off legacy `name` strings and should not be conflated with emission-time
  spelling
- `const_init_emitter` now resolves emitted decl-backed globals/functions late
  from `LinkNameId`, including `blockaddress` and constant-expression GEP
  spellings, but its raw decl strings still remain the lookup key used to find
  those semantic carriers
- unresolved extern-call declarations now forward the callee `DeclRef`’s
  `LinkNameId` into `record_extern_decl`, and direct call ops reuse that same
  id for late callee spelling in the LIR printer
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
