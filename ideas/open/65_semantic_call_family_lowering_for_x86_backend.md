# Semantic Call Family Lowering For X86 Backend

Status: Open
Created: 2026-04-21
Last-Updated: 2026-04-21
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make semantic `lir_to_bir` lower direct-call, indirect-call, and call-return
families canonically enough that x86 backend cases reach the prepared-x86
handoff instead of stopping in semantic call-family diagnostics.

This leaf exists because some current failures are no longer explained by
stack/addressing semantics or prepared-module consumption. They now fail in
semantic call-family lowering before prepared x86 can consume them.

## Owned Failure Family

This idea owns backend failures that still report:

`error: x86 backend emit path requires semantic lir_to_bir lowering before ...`

when the latest function failure is one of:

- `direct-call semantic family`
- `indirect-call semantic family`
- `call-return semantic family`

and the case is not better explained by a narrower already-open leaf.

## Current Known Failed Cases It Owns

Named cases already identified in this family:

- `c_testsuite_x86_backend_src_00204_c`

As additional backend cases are confirmed to fail for the same underlying
semantic call-family reason, they should be routed here instead of being mixed
back into the broader idea-58 semantic-lowering bucket or the exhausted
idea-62 stack/addressing leaf.

## Latest Durable Note

As of 2026-04-21, `c_testsuite_x86_backend_src_00204_c` has graduated out of
idea 62's stack/addressing ownership. The case now lowers past the old
aggregate local/member seam and fails later in function `arg` with
`latest function failure: semantic lir_to_bir function 'arg' failed in
semantic call family 'direct-call semantic family'`.

## Scope Notes

Expected repair themes include:

- direct and indirect call lowering once address/value meaning is already
  canonical
- call-result ownership and return-lane lowering at semantic BIR boundaries
- clearer ownership between semantic call lowering and later prepared call
  bundle or prepared-module consumption

## Boundaries

This idea does not own:

- stack/addressing failures before call lowering begins
- prepared call-bundle or prepared-module consumption after semantic lowering
  already succeeds
- short-circuit or guard-chain prepared CFG consumption
- runtime correctness bugs once codegen already succeeds

## Dependencies

This idea sits between the broad semantic-lowering leaf in idea 58 and the
prepared call/module leaf in idea 61. Use it when semantic lowering still
fails in a call-family bucket after stack/addressing meaning is already
canonical enough.

## Completion Signal

This idea is complete when the owned semantic call-family cases stop failing
for semantic call lowering and instead participate in the normal prepared-x86
route or graduate into a better downstream leaf.
