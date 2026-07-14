# Current Packet

Status: Active
Source Idea Path: ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define and populate the selected memcpy typed authority

## Just Finished

- Lifecycle switch from blocked idea 734 is complete. Its accepted Step 7.19
  proof and Step 7.20 resumption record remain in the source idea.

## Suggested Next

- Execute Step 1 only: identify and populate typed current-function
  destination/source/size and pointer-object/lifetime authority for the one
  selected PL `emit_lval_dispatch` non-volatile `LirMemcpyOp` row. Do not edit
  Raw-BIR/importer code or broaden to another memory/object producer.

## Watchouts

- `LirOperand` display text is non-authoritative. Keep every other memcpy,
  stack, va-list, parameter, aggregate/vector, and memory family fail-closed.
- The exact selected producer source and focused fixture must be recorded in
  the later handoff; a test-name matcher is not authority.

## Proof

- After implementation: fresh build plus focused selected producer/verifier
  coverage. The supervisor owns matching regression logs and broader checks.
