# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Dual Local-Scope Symbol Maps

## Just Finished

Step 2 - Add Dual Local-Scope Symbol Maps: added a structured local-scope stack
beside the authoritative string-keyed `scopes_` map in `validate.cpp`.

Local declarations, function parameters, and range-for declarations now
dual-write a `sema_local_name_key` mirror when the AST node exposes a stable
`unqualified_text_id`. Variable lookup and local-initialization marking
dual-read/compare the structured mirror against the string lookup while still
returning and mutating the legacy string-keyed result, so user behavior remains
unchanged and nodes without stable metadata keep the existing fallback path.

## Suggested Next

Delegate Step 3 to add dual global, function, and overload maps using structured
sema symbol keys where namespace context and base `TextId` metadata are
available, while preserving rendered-name maps for diagnostics, HIR handoff, and
legacy call sites.

## Watchouts

- Do not require HIR data-model cleanup in this plan.
- Do not remove `Node::name`, `TypeSpec::tag`, rendered diagnostics, mangled
  names, or link-name surfaces.
- Do not add testcase-shaped special cases or expectation downgrades.
- Local structured mirrors intentionally remain advisory; the string-keyed scope
  map is still authoritative for lookup, mutation, redefinition diagnostics, and
  fallback behavior.
- String-only local bindings such as predefined function identifiers, `this`,
  local enum constants, and template NTTP names still do not have per-binding
  `Node` metadata for `sema_local_name_key`.
- `TypeSpec::tag` and `TypeSpec::qualifier_segments` do not currently carry
  base-name `TextId` metadata, so this packet did not add a TypeSpec tag key
  helper.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_' > test_after.log 2>&1`

Result: passed. `test_after.log` records 34/34 `positive_sema_` tests passing.
