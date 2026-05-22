# AArch64 Hanoi Starting-State Output Mismatch

Status: Parked
Created: 2026-05-21
Split From: ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md

## Parked Outcome

Parked: 2026-05-21

The starting-state owner was repaired by commit
`87e79a50a Reload select-store globals from memory`. The accepted slice kept
the aligned direct `LoadGlobal` current-memory select-store fix and focused
dispatch coverage, and removed the reviewed unguarded materialized
pointer-addressed `StoreLocal` fallback.

Focused proof after narrowing was 6/7: backend contracts, `00170`, and
`00189` pass. `00181` still fails, but its starting state is now correct:
`A: 1 2 3 4`, `B: 0 0 0 0`, `C: 0 0 0 0`. Later Hanoi moves remain
unchanged because the pointer-addressed store fallback was removed.

Close was rejected by the plan-owner close gate because
`c4c-regression-guard` reported no strict pass-count increase for the focused
before/after logs: both runs remained 6/7 with `00181` failing. The residual
is split to `ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md`.

Refresh parked: 2026-05-22

The refresh runbook activated by `6fd359d53` and classified by `6e6f420db`
found no live in-scope Hanoi starting-state value-flow first bad fact. The
focused proof recorded in `todo.md` passed 7/7, including
`c_testsuite_aarch64_backend_src_00181_c`, with the nearby backend guardrails
stable.

Close remains rejected because the strict monotonic close gate fails even with
matching green proof: `test_before.log` and `test_after.log` both pass 7/7,
so the pass-count delta is 0 and no strict before/after improvement is
established. The idea stays open and parked rather than active.

## Goal

Repair the AArch64 lowering residual where `00181` prints an incorrect initial
Tower of Hanoi state, with the third source-tower element emitted as `0`
instead of `3`, before the recursive post-call pointer-formal path from idea
359 is observable.

## Why This Exists

Idea 359 repaired the stack-preserved pointer formal post-call overwrite for
`%p.spare`. After that repair, `00181` no longer segfaults and no longer has
the bad post-call overwrite before the final recursive call. The first visible
remaining failure is now the starting state:

- expected: `A: 1 2 3 4`
- actual: `A: 1 2 0 4`

That mismatch is visible before the repaired recursive `%p.spare` reuse point,
so it is not evidence that stack-preserved pointer formal post-call handling
remains broken.

## In Scope

- Localize the first bad fact for `00181`'s initial tower state before the
  first recursive post-call `%p.spare` reuse point.
- Compare semantic BIR, prepared BIR, and generated AArch64 for the source
  tower initialization, store, reload, and print paths.
- Repair the general lowering rule that causes one initialized tower element
  to be lost or read as zero.
- Add focused backend coverage for the localized value-flow shape once known.
- Prove `c_testsuite_aarch64_backend_src_00181_c` advances or passes while
  preserving the repairs from ideas 357, 358, and 359.

## Out Of Scope

- Stack-preserved pointer formal post-call overwrite handling for `%p.spare`,
  already parked under idea 359.
- Scalar formal post-call reloads for `%p.n`, already parked under idea 358.
- Callee-saved pointer-formal home publication for `%p.source` and `%p.dest`,
  already closed under idea 357.
- Address-valued memory and call-argument publication already closed under
  idea 355.
- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, unrelated scalar compare/select residuals, expectation changes,
  unsupported classifications, runner behavior, timeout policy,
  CTest-registration, or proof-log policy.

## Acceptance Criteria

- The first bad fact is localized to a concrete value-flow boundary before the
  recursive post-call `%p.spare` reuse point, not to the `00181` filename or a
  source-name shortcut.
- Focused backend coverage fails before the repair and passes after it for the
  localized value-flow shape.
- `00181` advances or passes without expectation, runner, timeout, or
  filename-specific changes.
- The idea 357, 358, and 359 repairs remain stable: pointer-formal
  callee-saved homes, scalar `%p.n` post-call reloads, and stack-preserved
  pointer formal post-call homes do not regress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Hanoi`, the literal value `3`, one tower name, one
  source line, one stack offset, one ABI register, or one emitted instruction
  neighborhood instead of repairing the localized lowering rule;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, emitted-text
  rewrites, or pass-count movement while the initial printed state can still
  lose a source-tower element before recursion;
- reopens stack-preserved pointer formal post-call handling, scalar formal
  reloads, callee-saved pointer-formal publication, address-valued publication,
  semantic string loads, frontend admission, ABI composite work, or
  variadic/floating residuals without fresh first-bad-fact evidence and a
  lifecycle split;
- regresses `00170`, `00189`, or focused backend coverage for the already
  repaired ideas 357, 358, and 359.
