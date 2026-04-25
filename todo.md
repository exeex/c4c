# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add Dual Global, Function, and Overload Maps

## Just Finished

Step 3 - Add Dual Global, Function, and Overload Maps: added symbol-level
structured sema keys using namespace context plus base `TextId` metadata, then
dual-wrote mirrors for globals, function declarations, reference overload sets,
and C++ overload sets.

Global/function lookup and overload resolution now dual-read/compare the
structured mirrors where AST reference metadata is available, while still
returning the rendered-name map result as the authoritative behavior path for
diagnostics, HIR handoff, and legacy callers.

## Suggested Next

Delegate Step 4 to add dual enum and const-int binding maps, including global
and local constant bindings plus sema/consteval lookup paths, while preserving
the rendered-name maps as the behavior source.

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
