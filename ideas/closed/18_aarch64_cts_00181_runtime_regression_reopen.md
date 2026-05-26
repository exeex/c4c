# AArch64 c_testsuite 00181 Runtime Regression Reopen

## Goal

Reopen the AArch64 `c_testsuite_aarch64_backend_src_00181_c` runtime failure
after the recent edge-publication and absent-selection fallback cleanup batch.

## Why This Exists

Idea `13_aarch64_cts_00181_recursion_global_array_runtime.md` previously
closed with `00181` passing, and idea `15_aarch64_aggregate_hfa_stdarg_abi.md`
recorded a later full-suite baseline at 3410/3410. However, idea
`17_aarch64_absent_selection_fallback_retirement.md` closed with matched
backend/c_testsuite AArch64 logs where `00181` was the only remaining failure.

A fresh narrow check after closure confirms the test is currently red:

`ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00181_c$'`

The observed failure is `[RUNTIME_NONZERO]` with `exit=Segmentation fault`.

This should be treated as a reopened runtime regression/baseline drift family,
not as part of the already-closed edge-publication authority or absent-selection
fallback retirement scopes.

## In Scope

- Reproduce `c_testsuite_aarch64_backend_src_00181_c` from the current tree.
- Compare the current failure against the original idea 13 repair assumptions.
- Identify whether the drift came from edge-publication authority,
  absent-selection fallback retirement, call-boundary publication, global-array
  addressing, recursion/call preservation, or unrelated AArch64 runtime state.
- Repair the semantic lowering rule that causes the current segmentation fault.
- Add focused regression coverage that would have caught the reintroduced
  `00181` failure mode.
- Restore a matching backend/c_testsuite AArch64 proof with no new failures.

## Out Of Scope

- Reopening the full BIR edge value-flow authority migration.
- Reopening absent-selection fallback retirement unless evidence shows that
  one retired path directly caused the `00181` regression.
- Weakening or disabling c_testsuite expectations.
- Treating `00181` as an acceptable known failure without explicit user
  approval.
- Broad AArch64 calls or dispatch cleanup unrelated to the failing rule.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00181_c` passes in the current tree.
- The repair explains why the previous idea 13 fix no longer held.
- Focused same-feature coverage proves the repaired runtime rule without a
  named-test shortcut.
- Matching before/after validation shows no new backend or c_testsuite AArch64
  failures.
- The close note records whether the regression was caused by the 16/17 batch
  or by an older baseline drift that resurfaced during closure validation.

## Reviewer Reject Signals

- A patch special-cases `00181`, Tower of Hanoi symbols, or one exact global
  array shape.
- A patch marks the testcase unsupported or weakens runtime expectations.
- A patch restores broad target-local fallback selection as a substitute for
  complete prepared facts.
- A patch claims closure while `00181` remains the only known red test in the
  proving subset.

## Closure Note

Closed after repairing the nested AArch64 value/edge publication hazard that
clobbered the pointer base during scaled-index materialization. The stale
shape in `Move` overwrote the selected pointer base with the element scale
while forming `source[i]` or `dest[j]`; the repair keeps nested RHS
materialization from reusing the live target/base register as scratch.

The focused backend coverage in `backend_aarch64_instruction_dispatch` now
asserts predecessor edge publication of `base + sext(index) * 4` without
returning to the stale `mov x9, #4` / `mul x10, x10, x9` clobber pattern.
This is a semantic register-hazard fix, not a shortcut for `00181`, Tower of
Hanoi symbols, or one global-array shape.

Closure proof used the matched AArch64 backend/c_testsuite subset:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_|c_testsuite_aarch64_backend_)'
```

The canonical close logs record 248/248 passing tests, including
`c_testsuite_aarch64_backend_src_00181_c` and
`backend_aarch64_instruction_dispatch`, with no new failures. Because the
baseline had already been rolled forward after Step 5 validation, the close
regression guard was checked in non-decreasing mode and passed with equal
248/248 before/after counts.
