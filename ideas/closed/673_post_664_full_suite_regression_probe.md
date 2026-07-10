# Post-664 Full-Suite Regression Probe

Status: Closed
Type: Investigation / Implementation
Parent: `ideas/open/664_riscv_object_emission_internal_probe.md`
Related:
- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `review/reviewA.md`
Owning Layer: RV64 object emission local-memory publication reloads
Queue Order: 73
Proof Surface: full-suite regression row 139 reported after the post-664
baseline candidate.

Split:
- Row 176 was split to `ideas/open/674_rv64_object_terminator_lowering.md`
  after Step 1 evidence proved a different first owner.

Closure:
- Closed after row 139 was repaired by commit `e046ee60a` and the comparable
  three-row guard passed.
- `test_before.log` from
  `build/agent_state/673_step1_regression_probe/focused_three_row_ctest.log`
  compared against current `test_after.log`: before `passed=1 failed=2
  total=3`, after `passed=2 failed=1 total=3`.
- The resolved failure was
  `backend_cli_riscv64_pointer_global_local_publication`; no new failures were
  introduced, and `backend_riscv_object_emission` remained passing.
- The remaining row 176 failure belongs to
  `ideas/open/674_rv64_object_terminator_lowering.md`.

## Goal

Classify and repair the row 139 full-suite regression observed after the idea
664 repair series:

- row 139: `backend_cli_riscv64_pointer_global_local_publication`

## Why This Exists

Idea 664 completed its focused row-256 proof for
`backend_riscv_object_emission`, but closure was rejected because the
post-664 full-suite baseline candidate regressed from the accepted 3386/3397
baseline to 3384/3397 by adding rows 139 and 176. Step 1 evidence classified
different first owners:

- row 139 belongs to RV64 object emission local-memory handling for live
  direct-global local pointer publication reloads.
- row 176 belongs to RV64 object emission terminator lowering and is now
  tracked by `ideas/open/674_rv64_object_terminator_lowering.md`.

This idea remains active only for row 139.

## In Scope

- Repair row 139's first proven owner: RV64 object emission local-memory
  handling for live direct-global local pointer publication reloads.
- Preserve the Step 1 classification evidence under
  `build/agent_state/673_step1_regression_probe/`.
- Verify row 139 improves without regressing the row-256 guard.
- Preserve the fixed `backend_riscv_object_emission` row.

## Out Of Scope

- Reopening the row-256 RISC-V object-emission route unless evidence shows row
  256 actually regressed.
- Repairing row 176; that belongs to
  `ideas/open/674_rv64_object_terminator_lowering.md`.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.
- Broad RV64 runtime rewrites, CLI redesigns, or object-emission rewrites
  without a named first failing contract.

## Acceptance Criteria

- Focused evidence names row 139's first owner and row 176 remains split to
  `ideas/open/674_rv64_object_terminator_lowering.md`.
- Any row-139 repair is semantic and tied to the proven owner, not to testcase
  names or baseline accounting.
- The accepted proof delta has no new failure for row 139 compared with the
  accepted 3386/3397 baseline.
- `backend_riscv_object_emission` remains passing in the supervisor-selected
  proof subset.

## Reviewer Reject Signals

- Reject claiming row-139 progress while its direct-global local pointer
  publication reload remains fail-closed in RV64 object emission.
- Reject fixing row 139 by changing expectations, unsupported markers,
  allowlists, timeout policy, runtime policy, or baseline accounting.
- Reject named-case shortcuts for
  `backend_cli_riscv64_pointer_global_local_publication`.
- Reject recoupling row 176 into this idea without new supervisor-approved
  evidence that its terminator lowering owner is required for row 139.
- Reject any change that reintroduces a `backend_riscv_object_emission`
  failure.
