# Stack Addressing And Dynamic Local Access For X86 Backend

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make stack layout and addressing semantics canonical enough that x86 backend
cases involving dynamic locals, member access, and array access reach prepared
consumption and execute through a real addressing model.

This leaf exists because some current failures are better explained as missing
stack/addressing semantics than as a generic semantic-lowering bucket.

## Owned Failure Family

This idea owns the stack/addressing subset of current backend failures that
still report:

`error: x86 backend emit path requires semantic lir_to_bir lowering before ...`

when the underlying missing capability is dynamic local/member/array access or
other stack/addressing semantics needed for canonical prepared consumption.

## Current Known Failed Cases It Owns

Named cases already identified in this family:

- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`
- `c_testsuite_x86_backend_src_00040_c`

As additional stack/addressing-related backend c-testsuite cases are confirmed
to fail for the same underlying reason, they should be routed here instead of
remaining in the generic idea 58 bucket.

## Scope Notes

Expected repair themes include:

- canonical stack slot meaning for dynamic locals
- address-form lowering for local/member/array access
- clearer ownership between semantic lowering, prepared addressing facts, and
  x86 consumption

## Boundaries

This idea does not own:

- generic semantic-lowering gaps unrelated to stack/addressing
- short-circuit or guard-chain prepared CFG consumption
- call-bundle and multi-function module support
- runtime correctness bugs once a case already executes

## Dependencies

This idea is adjacent to idea 58 and may need to move in lock-step with it.
Use it when the durable missing capability is specifically memory-address or
stack-layout semantics, not just "semantic lowering missing".

## Completion Signal

This idea is complete when the owned stack/addressing cases stop failing for
missing semantic/prepared addressing support and instead participate in the
normal prepared-x86 route.
