Status: Active
Source Idea Path: ideas/open/365_aarch64_string_literal_pointer_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Pointer-Value Publication Boundary

# Current Packet

## Just Finished

Lifecycle activation created this packet for Plan Step 1. No executor work has
run for this active plan yet.

## Suggested Next

Execute Plan Step 1: localize where the string literal/global address pointer
value loses `.str0` or equivalent global-symbol materialization and becomes a
frame-spill address before local pointer storage.

## Watchouts

- Do not special-case `00173`, `.str0`, one literal, one stack slot, one stack
  offset, one ABI register, one loop body, or one emitted instruction
  sequence.
- Preserve idea 356's repaired dynamic pointer-derived byte-load behavior.
- Keep direct fixed global/string byte loads distinct from local pointer
  address publication.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.

## Proof

Activation-only lifecycle update. No code proof required. Run `git diff
--check` before handing back.
