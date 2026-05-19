Status: Active
Source Idea Path: ideas/open/320_aarch64_f128_transport_addressability.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize F128 Transport Address Shape

# Current Packet

## Just Finished

Lifecycle close accepted for idea 319. Its HFA/aggregate argument ABI owner is
complete, and the remaining `00204.c` focused failure is now the separate
AArch64 `f128_transport` machine-printer addressability blocker captured by
idea 320.

## Suggested Next

Execute Step 1 of `plan.md`: localize the exact `f128_transport` node, memory
address shape, and backend owner surface that produces
`f128 memory transport address is not printable`.

## Watchouts

- Preserve the fixed HFA lane repair, caller-side variadic HFA lane handoff,
  and callee-side aggregate `va_arg` helper handoff from idea 319.
- Do not reopen scalar ALU immediate materialization, raw `va_start` helper
  text, frame adjustment materialization, stack-offset spelling, runner
  behavior, timeout behavior, proof-log policy, or c-testsuite expectations.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  output line, one register, one stack slot, or one instruction index.
- The separate global data emission symptom still exists in generated assembly.
  Do not mix global initializer emission into this packet unless it becomes the
  first bad fact after `f128_transport` addressability advances.

## Proof

Close-time regression guard used the existing focused proof scope from
`test_before.log` without modifying proof logs, per the delegated file
boundary. Non-decreasing comparison passed at 10/11 before and 10/11 after,
with no new failures.
