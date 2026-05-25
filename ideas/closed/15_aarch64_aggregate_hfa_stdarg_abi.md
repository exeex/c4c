# AArch64 Aggregate HFA Stdarg ABI

## Goal

Repair the AArch64 aggregate, HFA, and stdarg ABI failure represented by
`c_testsuite_aarch64_backend_src_00204_c`.

## Why This Exists

The prior-preservation baseline drift runbook left `00204` red after the
source-selection family was repaired. The failure was classified as a separate
AArch64 aggregate/HFA/stdarg ABI family, not as another source-selection
preservation issue.

## In Scope

- Reproduce `c_testsuite_aarch64_backend_src_00204_c`.
- Inspect AAPCS64 aggregate/HFA classification and variadic argument lowering
  for the failing path.
- Repair the semantic ABI classification, access plan, or lowering rule that
  causes the mismatch.
- Add focused AArch64 backend tests for aggregate/HFA/stdarg behavior.

## Out Of Scope

- Reopening prior-preservation selected fallback behavior.
- Generic aggregate initializer layout repair for `00216`.
- Recursion/global-array runtime repair for `00181`.
- Rewriting the full call ABI pipeline without narrowing the failing rule.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00204_c` passes.
- Focused tests prove the repaired AArch64 aggregate/HFA/stdarg ABI rule.
- Existing variadic aggregate and HFA backend tests remain green.
- No unsupported markings, expectation weakening, or named-test shortcuts are
  used.

## Reviewer Reject Signals

- A patch special-cases `00204`, one literal aggregate shape, or one function
  name instead of repairing ABI classification or stdarg lowering.
- A patch hides the failure by weakening c_testsuite expectations or disabling
  the test.
- A patch treats HFA, aggregate byval, and variadic access plans as
  interchangeable without proving the AAPCS64 rule.
- A patch restores broad target-local fallback selection as a substitute for
  ABI-correct prepared facts.

## Closure Note

Closed after the active runbook repaired the AArch64 small byval
register-lane publication rule so discovered payload stores win over aggregate
home or preservation sources. Focused dispatch coverage now covers the
one-byte register-home and two-byte stack-home reduced cases, and
`c_testsuite_aarch64_backend_src_00204_c` passes.

Close proof used the canonical matching logs:

- `test_before.log`
- `test_after.log`

Both logs cover:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00204_c)$'`

The close-time regression guard passed with
`--allow-non-decreasing-passed`; both logs were already green for the same
2-test scope, so the pass count was unchanged. Supervisor also accepted the
post-commit full-suite baseline in `test_baseline.log` after commit
`ee1790c10`, with 3410/3410 tests passing.
