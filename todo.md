Status: Active
Source Idea Path: ideas/open/93_aarch64_calls_stack_frame_slot_operand_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Operand Boundary And Proof Subset

# Current Packet

## Just Finished

Lifecycle activation only: `ideas/open/93_aarch64_calls_stack_frame_slot_operand_owner.md` is now the active source idea, with `plan.md` and this `todo.md` aligned to Step 1.

## Suggested Next

Execute Step 1 by mapping the in-scope stack/frame-slot operand helpers in `src/backend/mir/aarch64/codegen/calls.cpp`, separating operand rendering from selection/publication authority, and choosing the exact focused proof subset before implementation.

## Watchouts

- Keep the boundary AArch64-local and prepared-fact-consuming.
- Do not move prepared call plan, move-bundle, publication, result, preservation, aggregate transport, or scalar producer authority into the operand owner.
- Do not weaken diagnostics, selection status, assembly output, or test expectations.
- Keep ideas `94` and `95` out of this route unless a supervisor-approved lifecycle switch happens.

## Proof

No validation run for activation-only lifecycle work. No implementation files were touched.
