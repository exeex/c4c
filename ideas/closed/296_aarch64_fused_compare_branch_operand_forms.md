# AArch64 Fused Compare-Branch Operand Forms

Status: Closed
Created: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md
Closed: 2026-05-19

## Intent

Repair the AArch64 backend compare-branch lowering/printer contract so fused
compare-branch forms publish and print valid semantic operands.

## Why This Exists

Umbrella inventory idea 295 classified the current main-build
`ctest -R backend` result as 352 selected tests, 272 passed tests, and 80
failed tests, all under `c_testsuite_aarch64_backend_*`.

The largest coherent compile-stage owner is a 22-case machine-printer family
where fused compare-branch forms fail around operand shape or operand
publication:

```text
00030 00034 00037 00038 00041 00054 00055 00057 00059 00076 00077
00085 00092 00093 00101 00127 00200 00203 00207 00212 00214 00215
```

This family is separate from the closed runtime owners 285 through 294 and from
the remaining scalar machine-printer, `lir_to_bir` admission, runtime, and
timeout buckets recorded by the umbrella inventory.

## In Scope

- Inspect representative failures from the 22-case set to confirm the shared
  fused compare-branch operand-form failure.
- Trace where AArch64 compare-branch operands are produced, normalized, and
  printed.
- Fix semantic operand publication or printing for compare-branch forms so the
  printer consumes a valid lowering contract.
- Prove the repair against the full 22-case focused subset, not only one
  starter testcase.
- Record any cases that prove not to belong to this operand-form family as
  follow-up inventory for a separate owner.

## Out of Scope

- Matching exact c-testsuite filenames, test numbers, or emitted instruction
  strings.
- Changing test expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Reclassifying supported tests as unsupported.
- Reopening closed AArch64 runtime owners 285 through 294 from these
  compile-stage failures.
- Repairing the remaining scalar machine-printer failures, `lir_to_bir`
  admission failures, runtime mismatches, or timeout cases from umbrella idea
  295.
- Broad backend rewrites that are not needed to establish the compare-branch
  operand contract.

## Completion Criteria

- The shared operand-form contract for fused compare-branch lowering/printing
  is identified in `todo.md`.
- The implementation publishes or prints semantic compare-branch operands
  without testcase-shaped matching.
- Representative cases no longer fail at the old machine-printer operand-form
  point.
- The full 22-case focused subset is rerun with the supervisor-selected proof
  command and recorded in `todo.md`.
- Remaining failures, if any, are classified without weakening tests or
  claiming progress through expectation/runner changes.

## Closure Note

Closed on 2026-05-19 after the focused fused compare-branch printer blockers
were repaired and the residual focused-scope failures were classified outside
this idea's owner.

Accepted close proof used the supervisor-provided focused baseline in
`test_before.log`: the 27-test scope reported 23 passed and 4 failed after the
recent committed slices. The repaired cases include the immediate-left compare
forms, both-immediate constant compares, and non-encodable register/immediate
compare operands. `00041` and `00203` now pass and no longer fail at
`opcode=compare_branch: fused compare branch operands are not printable`.

The remaining focused failures are not retained under this idea:

- `00200` is a runtime mismatch after machine printing succeeds.
- `00207`, `00214`, and `00215` fail later in scalar `add`/`xor` immediate
  printing when the immediate is outside the plain `#imm` encoding range.

No test expectations, allowlists, unsupported classifications, runner behavior,
timeout policy, or CTest registration changes are part of this closure.

## Reviewer Reject Signals

Reject the route if it:

- adds filename, test-number, or exact-emitted-instruction matching for any of
  `00030`, `00034`, `00037`, `00038`, `00041`, `00054`, `00055`, `00057`,
  `00059`, `00076`, `00077`, `00085`, `00092`, `00093`, `00101`, `00127`,
  `00200`, `00203`, `00207`, `00212`, `00214`, or `00215`;
- changes expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration and claims that as backend
  capability progress;
- proves only one named failing test while nearby fused compare-branch cases
  remain unexamined;
- hides the same compare-branch operand-form failure behind a helper rename or
  abstraction with no semantic operand publication/printing repair;
- expands into unrelated scalar machine-printer, `lir_to_bir` admission,
  runtime, timeout, or closed-owner work instead of splitting a separate idea;
- reopens closed owners 285 through 294 without generated-code or proof
  evidence that contradicts their closure boundaries.
