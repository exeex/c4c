# AArch64 Vararg Consumption Source And Progression

Status: Open
Created: 2026-05-19
Split From: ideas/closed/322_aarch64_va_start_destination_address_materialization.md

## Goal

Repair AArch64 `va_arg` source selection, value transport, and `va_list`
progression for aggregate, floating, and long-double consumption after
`va_start` has initialized a real local `va_list` object.

## Why This Exists

Idea 322 repaired the AArch64 `va_start` destination publication fault. The
focused `00204.c` representative now materializes the local `va_list`
destination before field stores and no longer contains raw `va.arg.aggregate*`
helper text, but it still fails at runtime.

The current first bad fact is later in generated `myprintf`: execution reaches
scalar and string varargs, then prints corrupt long-double/floating values and
exits with a segmentation fault while consuming the initialized `va_list`. The
owning surface is AArch64 `va_arg` source selection, floating/long-double value
transport, and source/progression coordination, not `va_start` destination
address publication.

## In Scope

- Localize the post-`va_start` consumer path for scalar, aggregate, floating,
  and long-double `va_arg` records in generated `00204.c` output.
- Verify whether AArch64 `va_arg` source selection uses the correct `va_list`
  fields, register-save areas, overflow stack area, alignment rules, and
  progression updates for aggregate, floating, and long-double consumers.
- Repair general AArch64 `va_arg` consumption lowering for the localized first
  bad fact.
- Preserve the prior `va_start` destination materialization, aggregate helper
  lowering, F128 transport addressability, HFA argument ABI, scalar immediate,
  frame, and stack-offset repairs.
- Add focused backend coverage that proves the repaired consumer source and
  progression contract without relying only on the external c-testsuite case.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  proof after local consumer coverage is repaired.

## Out Of Scope

- Reopening idea 322's `va_start` destination address publication unless
  generated evidence proves the materialized address points at the wrong
  writable object.
- Reopening idea 321's raw aggregate `va_arg` helper text lowering.
- Reopening idea 320's F128 transport addressability owner.
- Reopening idea 319's HFA argument call-lane owner, except where the current
  consumer-side `va_arg` source/progression evidence requires HFA or floating
  retrieval rules.
- Reopening scalar ALU immediate materialization, large frame adjustment
  materialization, large stack-offset spelling, semantic admission, global
  initializer emission, runners, timeout policy, expectations, unsupported
  classifications, or CTest registration.
- Treating corrupt output as fixed by changing print expectations, weakening
  tests, or bypassing the runtime representative.

## Acceptance Criteria

- The first post-`va_start` vararg consumption fault is localized to concrete
  AArch64 prepared records, access plans, machine records, or printer code.
- The repair is a general AArch64 `va_arg` source/progression or value
  transport rule, not a `00204.c`, `myprintf`, one type, one register, one
  slot, one offset, or one emitted-line shortcut.
- Focused backend coverage exercises the repaired consumer behavior for the
  relevant aggregate/floating/long-double path.
- Prior-owner guardrails remain cleared under a fresh build and focused CTest
  proof.
- If `00204.c` advances to another first bad fact outside vararg consumption
  source/progression, the residual is classified into a separate owner.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `stdarg`, `myprintf`, one argument index, one
  floating type, one aggregate shape, one register, one stack slot, one
  offset, or one emitted instruction sequence instead of repairing general
  AArch64 `va_arg` consumption;
- hides the runtime corruption by weakening expected output, unsupported
  classifications, runner behavior, timeout policy, proof-log policy, or CTest
  registration;
- claims progress while the generated consumer still reads from or advances
  the wrong `va_list` field for the localized path;
- reopens `va_start` destination publication, aggregate helper text lowering,
  F128 transport, frame materialization, scalar immediates, or global
  initializer emission without generated-code evidence tying that work to the
  current consumer fault;
- adds coverage that only checks the c-testsuite representative while omitting
  focused backend coverage for the repaired `va_arg` source/progression
  contract.
