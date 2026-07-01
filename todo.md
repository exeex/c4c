Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Stack-Parameter Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution-state skeleton
for Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: rebuild the stack-passed parameter-home evidence for
`src/20001017-1.c`, confirm the missing caller/callee prepared facts, and
identify the current RV64 rejection point.

## Watchouts

- Keep the route limited to producer ABI/prealloc publication of explicit
  stack-passed parameter homes.
- Do not infer stack argument homes in RV64 from argument indexes, ABI
  formulas, source call shape, parameter names, or named gcc_torture rows.
- Keep varargs, F128, aggregate ABI, dynamic stack work, broad RV64 call
  lowering, and unrelated ABI repairs out of this plan.

## Proof

- Pending executor proof for Step 1.
