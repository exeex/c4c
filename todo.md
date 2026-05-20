Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Cast Source Publication Boundary

# Current Packet

## Just Finished

Lifecycle switch completed from umbrella idea 295 to focused idea 340. No
implementation work has been done under this active plan.

## Suggested Next

Begin Step 1 by reproducing the narrow `00143` prepared and printer evidence,
then localize where the prepared consumer stack-to-register move stops being
published as the scalar cast source operand.

## Watchouts

- Do not special-case `00143`, value ids, registers, source lines, instruction
  numbers, block names, or diagnostic strings.
- Do not frame this as an idea 339 local storage/writeback sizing residual
  unless new generated-code evidence moves the first bad fact there.
- Do not mutate files under `ideas/closed/`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.

## Proof

Lifecycle-only switch. No build or test command was run by the plan owner.
