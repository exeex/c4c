Status: Active
Source Idea Path: ideas/open/325_aarch64_variadic_local_value_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Value Home Publication Fault

# Current Packet

## Just Finished

Lifecycle reset after closing idea 324 and activating idea 325. Begin Step 1
by localizing the local/value-home publication fault recorded in the idea 324
closure: generated `myprintf` no longer shows the prior frame/formal blockers,
but it reads ordinary local/value homes and an undefined register before
same-function publication.

## Suggested Next

Execute Step 1 from `plan.md`: map the first read-before-publication local,
constant, pattern operand, or temporary back to prepared homes, machine
records, dispatch paths, and printer helpers. Record owning surfaces and the
smallest focused proof command in this file.

## Watchouts

- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, aggregate/floating `va_arg` source/progression, frame-size
  coverage, and fixed-formal publication.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, the format loop, one
  local, one stack slot, one register, one offset, or one emitted instruction
  sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Reopen frame/formal publication only if fresh generated output again shows
  uncovered stack references or fixed-formal clobber before publication.

## Proof

Close gate for idea 324 passed using existing focused `test_before.log` and
`test_after.log` in non-decreasing mode: 10 passed, 1 failed, no new failures.
