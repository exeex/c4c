# AArch64 Variadic Aggregate Va Arg Post-Consumption Call Setup

Status: Open
Created: 2026-05-20
Split From: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md

## Goal

Repair AArch64 call setup after non-HFA variadic aggregate `va_arg`
consumption, so ordinary calls that follow the consumed aggregate publish all
fixed operands, including format string constants and aggregate-member
addresses, into the correct ABI argument registers before the branch.

## Why This Exists

Idea 328 repaired caller-side byval aggregate call-argument lane publication.
The focused `00204.c` representative now reaches a different first bad fact
inside generated `myprintf`: the `%9s` branch reaches libc `printf` with
`x0 == 0x1` instead of loading `.str33` (`"%.9s"`) into `x0`, while `x1`
holds the aggregate text-buffer address. The `%7s` branch has the same
missing `.str31` format-argument shape.

This failure occurs after aggregate `va_arg` handling in a variadic function
and before the subsequent call to `printf`. The first observed residual is a
non-HFA aggregate string path, not remaining idea 328 fixed byval
call-argument publication and not yet the parked idea 326 HFA/floating
residual.

## In Scope

- Localize the generated `myprintf` `%7s` and `%9s` branches from `match`,
  through aggregate `va_arg` consumption, through the following `printf` call
  setup.
- Determine whether the missing `x0` publication belongs to call-operand
  materialization, string-constant lowering, aggregate-member address
  publication, variadic `va_arg` temporary lifetime, register assignment,
  instruction ordering, or a clobber between `va_arg` and `bl printf`.
- Repair the general AArch64 post-`va_arg` call setup path for ordinary calls
  that consume aggregate-derived values, not only `00204.c`.
- Add focused backend coverage for a non-HFA variadic aggregate `va_arg`
  followed by a call that requires a fixed string constant in `x0` and an
  aggregate-member pointer or equivalent value in a later argument register.
- Preserve completed guardrails for byval aggregate call-argument lane
  publication, fixed-formal entry publication, local/value-home publication,
  frame/formal publication, and fixed HFA argument/return lanes.

## Out Of Scope

- Reopening idea 328 byval aggregate call-argument lane publication unless
  generated code again shows `arg`-style fixed aggregate callsites failing to
  publish prepared aggregate bytes into ABI register lanes before `bl`.
- Reopening idea 326 HFA/floating variadic residuals unless fresh evidence
  reaches HFA/floating corruption, floating register-save-area progression,
  overflow-area progression, or HFA lane materialization after this
  non-HFA aggregate call-setup issue is repaired.
- Reopening callee fixed-formal entry publication, local/value-home
  publication, frame/formal publication, global initializer emission, runner,
  expectation, unsupported, timeout, or proof-log behavior without direct
  generated-code evidence tying that owner to the current first bad fact.
- Fixing only `%7s`, `%9s`, `.str31`, `.str33`, `myprintf`, one stack offset,
  one literal, one callsite, or one emitted instruction sequence.

## Acceptance Criteria

- The first bad fact is localized to concrete semantic/prepared call operands,
  aggregate `va_arg` state, generated source storage, ABI argument registers,
  and emitted instructions for the failing post-`va_arg` call.
- Generated AArch64 for the `%7s` and `%9s` paths loads the required format
  string constants into `x0` and publishes the aggregate-member text address
  into the expected following argument register before `bl printf`.
- Focused backend coverage proves the repaired post-`va_arg` call-setup owner
  and fails without the repair.
- Adjacent guardrails remain stable, especially idea 328 byval aggregate
  call-argument lane publication and prior AArch64 variadic publication
  repairs.
- The focused `00204.c` representative either passes or advances to a newly
  classified first bad fact for lifecycle handoff.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `myprintf`, `%7s`, `%9s`, `.str31`, `.str33`,
  `x0 == 0x1`, one aggregate size, one stack offset, one local temporary, or
  one emitted `printf` sequence instead of repairing a general AArch64
  post-`va_arg` ordinary call-setup capability;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the remaining
  representative failure;
- claims capability progress only through helper renames, dump text changes,
  expectation rewrites, or classification notes without generated callsites
  loading the fixed format string into `x0` and publishing the aggregate
  member argument before `bl printf`;
- reopens byval fixed-call publication, fixed-formal entry publication,
  local/value-home publication, HFA/floating variadic lowering, global data,
  frame layout, or large stack behavior without fresh generated-code evidence
  tying that owner to the current first bad fact;
- leaves the exact old failure mode in place behind a new abstraction, such
  that the failing branch can still reach libc `printf` with `x0 == 0x1`
  instead of the required format string pointer;
- proves only the external `00204.c` run while nearby focused backend output
  lacks assertions for the post-`va_arg` call operand publication being
  repaired.
