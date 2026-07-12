# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Delete the ALU return-chain reconstruction

## Just Finished

- Lifecycle switch completed: idea 727 closed after its typed traversal-attached
  return-chain relation passed the Step 4 handoff audit and close-time focused
  regression guard.

## Suggested Next

- Execute Plan Step 2.1 against `src/backend/mir/aarch64/codegen/alu.cpp`.

## Watchouts

- Consume the attached classification only; do not retain any move-bundle,
  successor-home, scalar-producer, operand-walk, or generated-lookup fallback
  in AArch64.

## Proof

- Lifecycle-only activation; implementation proof belongs to the next executor
  packet.
