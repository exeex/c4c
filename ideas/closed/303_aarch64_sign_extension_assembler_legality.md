# AArch64 Sign-Extension Assembler Legality

Status: Closed
Created: 2026-05-19
Split From: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md
Closed: 2026-05-19

## Goal

Repair the AArch64 backend route that emits illegal sign-extension assembly
spelling such as `sxtw w9, w13`.

## Why This Exists

While executing idea 302, focused case `00205` moved past the old scalar
`logical_shift_right` unsigned-reduction machine-printer diagnostic and failed
later during backend assembler validation:

```text
[BACKEND_FAIL] build/c_testsuite_aarch64_backend/src/00205.c.s:29: sxtw w9, w13
```

That failure is not a scalar arithmetic/reduction machine-node operand-form
printer diagnostic. It is an assembler-legality issue in sign-extension
instruction selection or spelling, currently observed through the fused
compare-branch route. It needs its own owner so idea 302 does not absorb a
separate backend legality bucket from a testcase-count movement alone.

## In Scope

- AArch64 sign-extension instruction selection, register-width selection, and
  assembler spelling for generated integer extension routes.
- The concrete illegal spelling `sxtw w9, w13` exposed by focused case `00205`.
- Inspection of the fused compare-branch or compare preparation route only as
  needed to find where the bad sign-extension operation is selected or printed.
- Fresh build proof and the supervisor-selected focused backend c-testsuite
  subset that demonstrates the illegal sign-extension spelling no longer
  reaches assembler validation.

## Out Of Scope

- Scalar arithmetic/reduction machine-node operand-form repairs owned by
  idea 302.
- Broad fused compare-branch rewrites unrelated to sign-extension legality.
- Reopening closed owners from failure counts alone.
- Expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, or CTest registration
  changes.
- Treating `00205` as passed merely because the old unsigned-reduction
  diagnostic is gone while a later illegal assembly instruction remains.

## Acceptance Criteria

- Generated AArch64 assembly no longer contains illegal sign-extension spelling
  of the form `sxtw` with a 32-bit destination register.
- The repair explains the semantic width rule being enforced, such as when the
  backend should emit `sxtw` to an X register, use a 32-bit sign-extension
  alias, or avoid the extension entirely.
- Focused proof records the supervisor-selected backend case subset and does
  not claim pass-count progress if `00205` still fails later for another
  unrelated bucket.
- The implementation does not special-case the filename `00205` or the exact
  generated temporary register numbers.

## Lifecycle Note

Parked on 2026-05-19 after the sign-extension legality repair in commit
`9e4565365` changed the fused compare-branch route from illegal
`sxtw w9, w13` to legal `sxtw x9, w13`.

Close rejected: the source goal appears satisfied, but plan-owner close-gate
proof could not be accepted because only `test_before.log` was present and the
matching `test_after.log` was absent. The delegated constraints forbade proof
log generation or mutation during this lifecycle pass. Running the regression
guard checker therefore failed with input error `after log not found:
test_after.log`.

The residual focused `00205` failure is now a timeout after 5.01 seconds, not
the old assembler-legality failure. That timeout is split to
`ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md` and must not be
claimed as pass-count progress for this idea.

Closed after Step 1 verification recorded in commit `69eed4707`. The source
intent is satisfied by the sign-extension legality repair from illegal
`sxtw w9, w13` to legal `sxtw x9, w13`; later `00205` timeout behavior remains
owned by idea 304, and the later value-materialization residual was split to
and closed under idea 305. Close-gate validation used matching canonical
backend logs and passed with
`--allow-non-decreasing-passed`: before 139/139, after 139/139, no new
failures, and no slow tests.

## Reviewer Reject Signals

Reject the route if it:

- matches only `00205`, line 29, `w9`, `w13`, or the exact string
  `sxtw w9, w13` instead of enforcing a general AArch64 sign-extension width
  legality rule;
- fixes the failure by rewriting expectations, allowlists, unsupported
  classifications, timeout policy, runner behavior, or CTest registration;
- claims idea 302 scalar operand-form completion or pass-count progress from
  this split sign-extension repair;
- performs a broad compare-branch or dispatch rewrite whose accepted evidence
  is only the disappearance of one named illegal instruction;
- hides the same illegal sign-extension spelling behind a helper rename,
  formatting change, or delayed assembler failure;
- changes unrelated arithmetic/reduction operand materialization, runtime
  output handling, `lir_to_bir`, libc, ABI, aggregate, pointer, string,
  floating, timeout, or output-storm buckets while claiming this idea's
  capability progress.
