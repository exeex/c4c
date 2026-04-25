# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Add Dual Consteval Registry and Interpreter Bindings

## Just Finished

Step 5 - Add Dual Consteval Registry and Interpreter Bindings: added
consteval function registry mirrors keyed by `TextId` and structured
`ConstEvalStructuredNameKey` values beside the existing rendered-name registry.
Sema static assertions and recursive consteval interpreter calls now dual-read
and compare those advisory mirrors while the rendered function map remains
authoritative.

The consteval interpreter now carries parameter/local bindings as string,
`TextId`, and structured-key maps where declaration or assignment `Node`
metadata exists. Local declarations, parameter binding, assignment, scope
shadow restore, and expression lookup keep the rendered-name values as behavior
source and use mirrors for comparison.

## Suggested Next

Delegate Step 6 to add dual type-binding lookup for sema-owned template/type
substitutions while preserving HIR-facing `TypeSpec::tag` behavior.

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
  base-name `TextId` metadata, so this packet did not add a TypeSpec tag key
  helper.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_' > test_after.log 2>&1`

Result: passed. `test_after.log` records 34/34 `positive_sema_` tests passing.
