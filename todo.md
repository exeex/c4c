Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md
Source Plan: plan.md

# Current Packet

## Just Finished

Tightened `hir_expr_call.cpp`'s post-`lower_expr` direct-call fallback so it
only reattaches `LinkNameId` from already-materialized function carriers
instead of interning raw callee names, then added a builtin-alias regression
that proves alias calls intentionally stay on the invalid-id fallback when no
semantic carrier exists.

## Suggested Next

Audit the remaining helper-built method/self-call routes in
`hir_expr_call.cpp` that still synthesize direct-call `DeclRef`s before their
emitted function carrier is guaranteed to exist, and keep those routes on the
intentional invalid-id fallback unless they can recover a real semantic
carrier without raw-name interning.

## Watchouts

- `LirModule` currently shares HIR’s link-name table to make the printer route
  real; keep the source-idea goal in view and avoid hardening that into a
  permanent second string store once the shared TU text-table boundary is
  available
- keep `LinkNameId` distinct from both `TextId` and parser/source-atom
  `SymbolId`; the new HIR fields are parallel carriers, not replacements
- the shared helper still only copies ids from carriers already present in
  `global_index` or `fn_index`; some method-family helper calls can still
  legitimately stay invalid until their emitted carrier exists at lowering time
- post-`lower_expr` direct-call fallback now only reattaches ids from existing
  function carriers; builtin alias calls still intentionally use
  `kInvalidLinkName` because they do not originate from an HIR semantic
  carrier
- extern declaration dedup now has two paths: semantic `LinkNameId` keys when
  present, raw-name fallback only when the carrier still lacks a semantic id
- module finalization now filters local function declarations by semantic id
  before falling back to `hir_mod.fn_index` name checks; keep that split
  intact instead of collapsing `LinkNameId` back into string keys
- builtin alias calls still intentionally stay on the raw-name fallback path
  because they do not originate from HIR semantic carriers
- `lower_var_expr` now copies ids only from existing `Function`/`GlobalVar`
  carriers; do not widen that helper into raw-string interning or local/param
  naming paths
- helper-built member-call decl refs in `hir_expr_call.cpp` can still
  legitimately keep `kInvalidLinkName` when their method carrier is not yet in
  `fn_index`; do not force raw-name interning there just to make the field
  non-zero
- plain `foo()` direct-call lowering still patches the callee `DeclRef` in
  place after `lower_expr`, but that fallback is now carrier-only rather than
  raw-name interning
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
- the frontend HIR regressions now expect `lower_var_expr`-built function
  designators and global refs to arrive with semantic ids already attached;
  keep broader builder migration under the same carrier-based rule
- keep forwarding explicit ids through LIR carriers and resolve them only at
  late consumers rather than treating legacy `name` strings as the semantic
  source of truth
- avoid testcase-overfit proof or brittle emitted-text substring matching as a
  substitute for a real id path
- the new HIR regressions now cover both helper-built object calls and builtin
  alias fallback; add more helper-family coverage only when the route already
  has a real semantic carrier at HIR lowering time

## Proof

Build: `cmake --build --preset default -j4`
Narrow proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
Result: passed
Log: `test_after.log`
