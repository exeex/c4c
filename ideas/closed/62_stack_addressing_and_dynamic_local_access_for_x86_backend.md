# Stack Addressing And Dynamic Local Access For X86 Backend

Status: Closed
Created: 2026-04-20
Closed: 2026-04-21
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Make stack layout and addressing semantics canonical enough that x86 backend
cases involving dynamic locals, member access, and array access reach prepared
consumption and execute through a real addressing model.

## Owned Failure Family

This idea owned the stack/addressing subset of backend failures that still
reported the semantic `lir_to_bir` lowering diagnostic when the underlying
missing capability was local/member/array access or other stack/addressing
semantics needed for canonical prepared consumption.

## Completion Summary

The remaining named stack/addressing family is exhausted.

- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  now passes
- `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`
  now passes
- the nearby dynamic-member/array route neighborhood now passes under narrow
  audit coverage
- `c_testsuite_x86_backend_src_00204_c` no longer fails in the idea-62
  local-memory / stack-addressing seam and now fails later in function `arg`
  under the downstream `direct-call semantic family`

## Durable Follow-On

Future work on `c_testsuite_x86_backend_src_00204_c` should continue under the
separate semantic-call-family initiative rather than reopening this
stack/addressing leaf.
