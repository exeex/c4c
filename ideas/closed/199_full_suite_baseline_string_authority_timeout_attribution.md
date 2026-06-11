# 199 Full-suite baseline string-authority and timeout attribution

Closed: accepted 2026-06-11.

Closure evidence:
- The accepted 3428/3428 baseline was preserved until first-bad attribution was
  complete.
- The string-authority failure was isolated to `1496a715d -> 9e4892bcd` and
  repaired by adding the exact classification for
  `src/backend/mir/aarch64/codegen/calls.cpp` /
  `find_prepared_indirect_callee_stored_value_source_fallback`.
- `c_testsuite_aarch64_backend_src_00040_c` was classified separately as
  timeout/noise for this sequence and passed in isolated Step 5 proof.
- Final proof passed the narrow string-authority guard, the isolated `00040`
  test, and produced a non-regressive full-suite baseline candidate at
  `42fbc68e851f1dc929097fda1c91da182aac17c7` with `3428/3428` tests passing.
- The supervisor accepted that candidate into canonical `test_baseline.log`.

## Goal

Find the exact step where the accepted 3428/3428 full-suite baseline drifted
to the latest rejected 99% candidate, then decide whether the failures are
ambient noise, a docs/lifecycle workflow regression, or evidence that the
Phase D follow-up readiness artifacts exposed a missing string-authority or
baseline-sanity design rule.

This is analysis-first. Do not repair or accept a new baseline until the
failure family and first bad step are understood.

## Why This Exists

The accepted `test_baseline.log` records a clean full-suite result:

- result: 3428/3428 passing

The newer `test_baseline.new.log` records a rejected candidate:

- commit: `1d1c506f05dbaf783f6293d61e4b05d339395c4e`
- subject: `Write lifecycle naming cleanup note`
- result: 3426/3428 passing, 2 failing tests

Known failing tests in the rejected candidate:

- `string_authority_guard`
- `c_testsuite_aarch64_backend_src_00040_c` timeout

This failure shape is different from idea 190's Route 3 deterministic AArch64
regression. It appears during the Phase D follow-up readiness/naming cleanup
sequence, where most changes are analysis documents and lifecycle artifacts.
That means the investigation must consider both ordinary flakiness and the
possibility that a documentation or naming artifact violates a repo
string-authority guard or exposes a missing lifecycle/baseline rule.

## In Scope

- Inspect baseline and proof logs in chronological order.
- Identify the last accepted 100% full-suite baseline and the first 99%
  candidate with `string_authority_guard`.
- Correlate the candidate with the ordered commits from ideas 191-198.
- Reproduce `string_authority_guard` narrowly before making any repair.
- Re-run or classify the `00040` timeout separately from the deterministic
  string-authority failure.
- Determine whether the first bad step is a docs artifact, lifecycle note,
  generated route/readiness text, or unrelated implementation state.
- If deterministic, isolate what string authority or lifecycle naming rule was
  violated and whether the rule reveals a deeper design gap in the Phase D
  follow-up artifacts.
- Produce a repair idea or implementation/docs packet only after first-bad
  evidence exists.

## Required Log Review

Review logs by filesystem timestamp and commit order. Do not assume the latest
candidate's baseline commit is the first bad step without proof.

Minimum artifacts to inspect:

- `test_baseline.log`
- `test_baseline.new.log`
- `test_before.log`
- `test_after.log`
- closed ideas 191-198 and their durable docs artifacts
- `todo.md` history around any rejected baseline candidate
- any workflow or string-authority guard documentation/tests

Recommended investigation commands:

```sh
ls -lt test_baseline.log test_baseline.new.log test_before.log test_after.log 2>/dev/null || true
git log --oneline --date-order --decorate -40
git show --stat 1d1c506f05dbaf783f6293d61e4b05d339395c4e
rg -n "string_authority_guard|test_baseline.new|3426/3428|00040|99%|baseline candidate|string authority" ideas/closed ideas/open docs review todo.md -g '*.md'
```

If historical baseline logs exist under `log/`, sort them by timestamp and
check whether `string_authority_guard` appears before the Phase D follow-up
readiness/naming sequence.

## Out Of Scope

- Accepting `test_baseline.new.log` while it has fewer passing tests than the
  accepted baseline.
- Treating the `00040` timeout and `string_authority_guard` failure as the same
  root cause without evidence.
- Weakening or disabling `string_authority_guard`.
- Deleting, renaming, or rewriting Phase D/Phase E docs just to satisfy the
  guard without understanding the authority rule it enforces.
- Opening draft 155 or proceeding with real Phase E retirement while the
  baseline candidate is regressive.
- Repairing unrelated failures discovered during the audit unless they are
  part of the same first-bad family.

## Acceptance Criteria

- A chronological baseline timeline identifies:
  - the last accepted 100% full-suite baseline;
  - the first observed 99% candidate with `string_authority_guard`;
  - the commit or lifecycle slice where the string-authority failure first
    appears;
  - whether `00040` reproduces or is timeout/noise in this sequence.
- The investigation distinguishes deterministic string-authority failure from
  timeout/flaky behavior.
- If deterministic, the report explains what artifact or rule caused the
  failure and whether that exposes a missing design constraint for Phase D
  follow-up readiness or lifecycle naming.
- Any repair preserves the purpose of `string_authority_guard` and does not
  hide the violation through expectation weakening.
- The final state either restores a 100% full-suite baseline candidate or opens
  a focused repair idea with first-bad evidence attached.

## Proof Route

Start with no code changes.

1. Re-run `string_authority_guard` by itself and record whether it
   deterministically fails.
2. Re-run `c_testsuite_aarch64_backend_src_00040_c` separately enough to
   classify timeout versus deterministic backend behavior.
3. If `string_authority_guard` reproduces, manually bisect across the Phase D
   follow-up readiness/naming commits using that narrow command.
4. After a candidate repair, run the narrow guard, the separated timeout
   reproducer if needed, any implicated workflow/doc tests, and then a
   full-suite baseline candidate.
5. Accept a refreshed `test_baseline.log` only if the full-suite result is
   non-regressive relative to the accepted 3428/3428 baseline.

## Reviewer Reject Signals

- The route claims success by accepting or ignoring the 3426/3428 candidate.
- The investigation skips chronological log ordering or guesses first-bad from
  the candidate subject alone.
- `string_authority_guard` is weakened, disabled, or treated as cosmetic
  without explaining the guarded authority rule.
- The `00040` timeout is merged with the string-authority failure without
  separate reproduction evidence.
- Docs or lifecycle notes are rewritten broadly without preserving the Phase D
  versus true Phase E distinction.
- The repair hides a design gap in the Phase D follow-up readiness artifacts
  instead of naming it as a future constraint or follow-up.
