# Load Local-Memory Semantic Family Follow-On For X86 Backend

Status: Open
Created: 2026-04-21
Last-Updated: 2026-04-21
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make semantic `lir_to_bir` lower the remaining load local-memory family
canonically enough that x86 backend cases which already clear call lowering can
continue toward the prepared-x86 handoff instead of stopping in
`load local-memory semantic family`.

This leaf exists because some cases that no longer belong to semantic
call-family lowering still fail before prepared x86 consumption on a later
local-memory load seam.

## Owned Failure Family

This idea owns backend failures that still report:

`error: x86 backend emit path requires semantic lir_to_bir lowering before ...`

when the latest function failure is:

- `load local-memory semantic family`

and the case is not better explained by an already-open narrower leaf.

## Current Known Failed Cases It Owns

No named cases currently remain in this family.

`c_testsuite_x86_backend_src_00204_c` advanced out of this leaf on 2026-04-21
after commit `736d15d6` preserved loaded opaque pointer provenance and moved
the case downstream into function `myprintf` / `scalar-control-flow semantic
family`, which belongs back under idea 58 unless a narrower leaf is activated.

As additional backend cases are confirmed to fail for the same underlying
local-memory load reason, they should be routed here instead of being mixed
back into idea 65's semantic call-family lane or the broader idea-58 bucket.

## Latest Durable Note

As of 2026-04-21, the direct-call byval-global repair under idea 65 advanced
`c_testsuite_x86_backend_src_00204_c` out of function `arg` and the
`direct-call semantic family`, and commit `736d15d6` then advanced the case
through function `match` so it no longer fails in `load local-memory semantic
family`. The current known case now fails later in function `myprintf` with
`latest function failure: semantic lir_to_bir function 'myprintf' failed in
scalar-control-flow semantic family`, so idea 66 is currently parked without a
named owned case until another backend failure is confirmed to stop on a real
load-local-memory seam.

## Scope Notes

Expected repair themes include:

- canonical local-memory load ownership once call lowering already succeeds
- load semantics for stack, aggregate, or byval-backed values that still stop
  before prepared x86 can consume the route
- clearer ownership between semantic local-memory load lowering and later
  prepared-module or scalar-emitter consumption

## Boundaries

This idea does not own:

- direct-call, indirect-call, or call-return failures that still belong in
  idea 65
- prepared call-bundle or prepared-module consumption after semantic lowering
  already succeeds
- CFG or scalar-emitter work once a case already reaches those downstream
  routes
- runtime correctness bugs once codegen already succeeds

## Dependencies

This idea sits after idea 65 for cases whose next real blocker is no longer
call-family lowering but a later `load local-memory semantic family` seam
before prepared x86 can consume the route.

## Completion Signal

This idea is complete when the owned load local-memory cases stop failing for
semantic local-memory load lowering and instead participate in the normal
prepared-x86 route or graduate into a better downstream leaf.
