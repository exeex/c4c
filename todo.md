Status: Active
Source Idea Path: ideas/open/377_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit First Instruction Fragment

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
`plan.md`.

## Suggested Next

Audit the first prepared ordinary instruction fragment behind the current
`unsupported_instruction_fragment` failure for `src/20000217-1.c`.

## Watchouts

- Do not key behavior on testcase names, value ids, frame slots, registers,
  instruction indexes, source syntax, or prepared-BIR text matching.
- Do not reopen idea 375 compare/trunc or idea 376 move-bundle ownership.
- Diagnostic-only changes are not capability progress unless followed by
  semantic lowering.

## Proof

Lifecycle-only activation; no build required.
