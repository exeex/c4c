Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Changed stmt-emitter local-function resolution to prefer semantic
`LinkNameId` lookup before raw-name fallback on the bounded direct-call route,
including direct call-target recovery and decl-ref function lvalue emission.

## Suggested Next

Extend the same bounded semantic lookup split into the remaining decl-backed
function consumers that still key off raw `name` strings in
`stmt_emitter_expr.cpp`, especially call-result type inference and function
designator rvalue emission, before considering broader `DeclRef` cleanup.

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
- extern declaration dedup now has two paths: semantic `LinkNameId` keys when
  present, raw-name fallback only when the carrier still lacks a semantic id
- module finalization now filters local function declarations by semantic id
  before falling back to `hir_mod.fn_index` name checks; keep that split
  intact instead of collapsing `LinkNameId` back into string keys
- builtin alias calls still intentionally stay on the raw-name fallback path
  because they do not originate from HIR semantic carriers
- plain `foo()` direct-call lowering now patches the callee `DeclRef` in place
  after `lower_expr`, so this route stays bounded to call sites instead of
  teaching every `DeclRef` construction path about `LinkNameId`
- direct-call target resolution now suppresses redundant extern-decl
  bookkeeping when a local declaration/definition can be recovered by
  `LinkNameId`; keep builtin-alias and invalid-id fallback behavior intact
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
Supplemental: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'` passed
