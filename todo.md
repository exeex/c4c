# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Add Dual Enum and Const-Int Binding Maps

## Just Finished

Step 4 - Add Dual Enum and Const-Int Binding Maps: added consteval-facing
`TextId` and structured-key constant lookup mirrors beside the existing
rendered-name maps, then changed constant evaluation to dual-read/compare those
mirrors while still returning the rendered-name result as authoritative.

Global and local const-int bindings now dual-write string, `TextId`, and
structured mirrors where declaration metadata exists. Sema static assertions,
local const folding, case-label evaluation, and consteval expression lookup all
receive the mirror maps through `ConstEvalEnv`.

## Suggested Next

Delegate Step 5 to add dual consteval function registry and interpreter binding
maps, including structured lookup for consteval callees plus interpreter
locals/parameter bindings where source `TextId` metadata is available.

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
