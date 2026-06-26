Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Unsupported Instruction Fragment Evidence

# Current Packet

## Just Finished

Activation created `plan.md` and this executor-compatible `todo.md` skeleton
for Step 1.

## Suggested Next

Delegate Step 1: capture the exact prepared/BIR instruction fragment behind
`unsupported_instruction_fragment` for `va-arg-13.c` and `920908-1.c`, then
record the instruction kind, operands, object types, storage facts, and current
diagnostic path here.

## Watchouts

- Do not infer the missing lowering from testcase names.
- Do not reopen idea 374 parameter-home support unless fresh evidence proves
  the diagnostic still comes from parameter-home facts.
- Do not downgrade expectations or add named-case-only handling.

## Proof

Lifecycle-only activation. No build or test run required.
