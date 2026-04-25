# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Add Dual Type-Binding Lookup

## Just Finished

Step 6 - Add Dual Type-Binding Lookup: added advisory sema-owned type-binding
mirrors for consteval template substitutions. `ConstEvalEnv` now carries
optional type-binding `TextId` and structured-key maps beside the rendered
`TypeBindings` map, and type resolution compares advisory mirror results while
keeping `TypeSpec::tag` string lookup authoritative.

Consteval call binding now records structured type-binding keys from the
callee template declaration identity plus template parameter index when that
metadata exists. Sema static-assert consteval calls also bind explicit/default
template type arguments before interpretation, using the same advisory mirror
storage.

## Suggested Next

Delegate Step 7 to add struct completeness and member/static-member lookup
mirrors only where sema can derive stable structured record identity without
requiring HIR data-model cleanup.

## Watchouts

- Do not require HIR data-model cleanup in this plan.
- Do not remove `Node::name`, `TypeSpec::tag`, rendered diagnostics, mangled
  names, or link-name surfaces.
- Do not add testcase-shaped special cases or expectation downgrades.
- Local structured mirrors intentionally remain advisory; the string-keyed scope
  map is still authoritative for lookup, mutation, redefinition diagnostics, and
  fallback behavior.
- Global, function, and overload structured mirrors are also advisory; rendered
  maps remain authoritative and the new dual-read paths discard comparison
  results.
- Const-int structured/text mirrors are advisory; rendered constant maps remain
  authoritative for sema, consteval, and HIR-facing lookup behavior.
- Consteval function registry mirrors are advisory; rendered-name
  `consteval_funcs_` and HIR compile-time engine string maps remain
  authoritative behavior sources.
- Consteval interpreter parameter/local mirrors are advisory; string-keyed
  locals remain authoritative for binding, assignment, and return evaluation.
- `ConstEvalEnv` now has optional NTTP text/key mirror slots, but
  `Node::template_param_names` and forwarded `template_arg_nttp_names` still do
  not carry source `TextId` metadata, so current NTTP binding behavior remains
  string-authoritative.
- `NK_ENUM_DEF` still carries enum variant names and values but no per-variant
  `TextId` metadata, so enum value/type rendered maps remain the only populated
  behavior source for enum constants in this packet.
- The symbol-level helper intentionally uses resolved namespace context plus
  base `TextId`, not source qualifier segments, so unqualified and qualified
  references can compare against the same declaration mirror.
- String-only local bindings such as predefined function identifiers, `this`,
  local enum constants, and template NTTP names still do not have per-binding
  `Node` metadata for `sema_local_name_key`.
- `TypeSpec::tag` and `TypeSpec::qualifier_segments` do not currently carry
  base-name `TextId` metadata, so `TypeSpec::tag` string lookup remains the
  authoritative type-binding behavior and mirror comparison bridges through
  the rendered tag.
- Type-binding `TextId` mirror slots are present but remain empty until
  template parameter `TextId` metadata is available at the sema boundary.
- Type-binding structured mirrors use the callee template declaration
  namespace/context text identity plus the template parameter index; they are
  advisory and do not affect HIR-facing type tags or diagnostics.
- HIR compile-time-engine type bindings still use rendered strings and were
  not converted in this packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_' > test_after.log 2>&1`

Result: passed. `test_after.log` records 34/34 `positive_sema_` tests passing.
