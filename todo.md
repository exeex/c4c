Status: Active
Source Idea Path: ideas/open/321_aarch64_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Aggregate Va Arg Helper Records

# Current Packet

## Just Finished

Lifecycle transition completed: idea 320 closed after the F128 transport
addressability blocker was repaired and the remaining `00204.c` residual was
classified as raw aggregate `va_arg` helper text. Idea 321 is now active.

## Suggested Next

Execute Step 1 by localizing the raw `va.arg.aggregate`,
`va.arg.aggregate.source`, and `va.arg.aggregate.progress` records to their
AArch64 prepared helper and machine-printer owners, then record the smallest
focused proof command for the repair.

## Watchouts

- Do not reopen F128 transport addressability; that owner is closed.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  register, one stack slot, one helper index, or one emitted line.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, and 320.
- Keep global initializer emission separate unless it becomes the first bad
  fact after aggregate `va_arg` helper text is gone.

## Proof

Close gate for idea 320 reused the existing focused `test_before.log` and
`test_after.log` with the same 11-test scope. The monotonic regression guard
passed with 10 passed, 1 failed, and no new failures; the remaining failure is
the raw `va.arg.aggregate` assembler text now owned by idea 321.
