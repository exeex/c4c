# Stack Addressing And Dynamic Local Access For X86 Backend

Status: Closed
Created: 2026-04-20
Closed: 2026-04-21
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Closure Summary

Closed after the last named idea-62 case graduated out of semantic
stack/addressing ownership:

- `c_testsuite_x86_backend_src_00204_c` no longer fails in semantic
  `lir_to_bir` / stack-addressing or scalar/local-memory ownership
- the case now reaches the later x86 prepared/emitter restriction
  `error: x86 backend emitter only supports a minimal i32 return function
  through the canonical prepared-module handoff`
- follow-on work for that case now belongs to
  `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`

No other named cases currently remain in this stack/member/array addressing
leaf.

## Intent

Make stack layout and addressing semantics canonical enough that x86 backend
cases involving dynamic locals, member access, and array access reach prepared
consumption and execute through a real addressing model.

## Owned Failure Family

This idea owns the stack/addressing subset of backend failures that still
reported the semantic `lir_to_bir` lowering diagnostic when the underlying
missing capability was local/member/array access or other stack/addressing
semantics needed for canonical prepared consumption.

## Current Known Failed Cases It Owned

The final named case in this family was:

- `c_testsuite_x86_backend_src_00204_c`

The earlier idea-62 close was superseded until commit `5a81abdb` repaired
aggregate-typed phi joins in semantic `lir_to_bir`, which advanced
`c_testsuite_x86_backend_src_00204_c` out of `myprintf` /
`scalar-control-flow semantic family` and exposed a later
`myprintf` / `gep local-memory semantic family` blocker that belongs back in
this stack/addressing leaf.

## Latest Durable Note

As of 2026-04-21, the reopened `myprintf` stack/member-addressing seam for
`c_testsuite_x86_backend_src_00204_c` has been repaired. The case now exits
idea-62 ownership cleanly and fails later in the x86 scalar/prepared emitter
route, so this leaf is closed again.

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
