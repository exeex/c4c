# Stack Addressing And Dynamic Local Access For X86 Backend

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-21
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make stack layout and addressing semantics canonical enough that x86 backend
cases involving dynamic locals, member access, and array access reach prepared
consumption and execute through a real addressing model.

## Owned Failure Family

This idea owns the stack/addressing subset of backend failures that still
reported the semantic `lir_to_bir` lowering diagnostic when the underlying
missing capability was local/member/array access or other stack/addressing
semantics needed for canonical prepared consumption.

## Current Known Failed Cases It Owns

Representative named failures currently back in this family:

- `c_testsuite_x86_backend_src_00204_c`

The earlier idea-62 close is now superseded: commit `5a81abdb` repaired
aggregate-typed phi joins in semantic `lir_to_bir`, which advanced
`c_testsuite_x86_backend_src_00204_c` out of `myprintf` /
`scalar-control-flow semantic family` and exposed a later
`myprintf` / `gep local-memory semantic family` blocker that belongs back in
this stack/addressing leaf.

## Latest Durable Note

As of 2026-04-21, the local dynamic-member/array backend route neighborhood
still passes, but `c_testsuite_x86_backend_src_00204_c` has re-entered this
leaf with a later stack/member-addressing seam in function `myprintf` after
the upstream aggregate-phi repair under idea 58. The leaf is reopened because
the prior close assumption that no named stack/addressing cases remained is no
longer true.

## Scope Notes

Expected repair themes include:

- canonical local-memory and addressing ownership once broader scalar-control
  flow or call lowering already succeeds
- stack, aggregate, member, and array access semantics that still stop before
  prepared x86 can consume the route
- clearer ownership between semantic addressing lowering and later prepared
  module or scalar-emitter consumption

## Boundaries

This idea does not own:

- broad scalar-control-flow ownership that still belongs in idea 58
- direct-call, indirect-call, or call-return failures that still belong in
  idea 65
- prepared call-bundle, prepared-module, CFG, or scalar-emitter work once
  semantic lowering already succeeds
- runtime correctness bugs once codegen already succeeds

## Dependencies

This idea sits after idea 58 for cases whose next real blocker is no longer a
broad semantic-lowering seam but a later stack/member/addressing lane before
prepared x86 can consume the route.

## Completion Signal

This idea is complete when the owned stack/addressing cases stop failing for
semantic addressing lowering and instead participate in the normal
prepared-x86 route or graduate into a better downstream leaf.
