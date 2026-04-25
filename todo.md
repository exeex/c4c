# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Add Struct Completeness and Member Lookup Mirrors

## Just Finished

Step 7 - Add Struct Completeness and Member Lookup Mirrors: added advisory
structured record mirrors for complete struct/union sets, field-name lookup,
static-member type lookup, and base-record traversal where sema can derive a
stable record key from the struct definition's namespace context and
`unqualified_text_id`.

Rendered `TypeSpec::tag`, `complete_structs_`/`complete_unions_`, rendered
field/static-member maps, and rendered base-tag lists remain authoritative.
The structured maps are dual-written from `note_struct_def` and dual-checked
from struct completeness checks, qualified static-member lookup, and implicit
method-body field lookup without changing HIR-facing behavior.

## Suggested Next

Delegate Step 8 to inventory the retained string fallbacks and remove only
sema-owned fallbacks that are proven structured-stable and not required as
HIR, diagnostics, consteval, or codegen bridges.

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
- `TypeSpec` still has rendered `tag` and string qualifier segments but no
  base-name/qualifier `TextId` metadata, so struct completeness remains
  string-authoritative and only compares against structured mirrors when the
  rendered tag can be bridged to a recorded struct definition key.
- Struct static-member mirrors intentionally follow the existing
  `struct_static_member_types_` behavior, including its current field-type
  contents; this packet did not tighten static-vs-instance member semantics.
- Base-record structured traversal is populated only when the base tag already
  maps to a stable recorded struct key; unresolved/pending template base
  records remain on the rendered/HIR-facing path.
- The rendered tag to structured record-key bridge now opts out on ambiguous
  tag-to-key mappings, so mirrored checks run only for stable record identity.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_' > test_after.log 2>&1`

Result: passed. `test_after.log` records 34/34 `positive_sema_` tests passing.
