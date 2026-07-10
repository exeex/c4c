# Post-664 Full-Suite Regression Probe

Status: Open
Type: Investigation / Implementation
Parent: `ideas/open/664_riscv_object_emission_internal_probe.md`
Related:
- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `review/reviewA.md`
Owning Layer: To be classified
Queue Order: 73
Proof Surface: full-suite regression rows 139 and 176 reported after the
post-664 baseline candidate.

## Goal

Classify and repair or route the two full-suite regressions observed after the
idea 664 repair series:

- row 139: `backend_cli_riscv64_pointer_global_local_publication`
- row 176: `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`

## Why This Exists

Idea 664 completed its focused row-256 proof for
`backend_riscv_object_emission`, but closure was rejected because the
post-664 full-suite baseline candidate regressed from the accepted 3386/3397
baseline to 3384/3397 by adding rows 139 and 176. Supervisor evidence says
rows 139 and 176 were already failing in a temporary worktree at pre-sret
commit `a02cc6e25`, so they are not owned by the final local frame-address
publication patch. Their first owner still needs a separate probe.

## In Scope

- Reproduce rows 139 and 176 with exact focused commands.
- Determine whether either regression is caused by the idea 664 implementation
  series, pre-existing local changes, stale baseline state, or an unrelated
  owner.
- Repair only the first proven owner if it is a narrow backend/CLI/runtime
  contract.
- If the two rows have different owners, split the second row into a separate
  follow-up idea instead of coupling unrelated repairs.
- Preserve the fixed `backend_riscv_object_emission` row.

## Out Of Scope

- Reopening the row-256 RISC-V object-emission route unless evidence shows row
  256 actually regressed.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline acceptance changes.
- Broad RV64 runtime rewrites, CLI redesigns, or object-emission rewrites
  without a named first failing contract.

## Acceptance Criteria

- Focused evidence names the first owner for row 139 and row 176, or explicitly
  splits one row to a separate source idea.
- Any repair is semantic and tied to the proven owner, not to testcase names or
  baseline accounting.
- The accepted full-suite baseline delta has no new failures for rows 139 and
  176 compared with the accepted 3386/3397 baseline, or the lifecycle records a
  supervisor-approved split/deactivation for any row that remains outside the
  repaired owner.
- `backend_riscv_object_emission` remains passing in the supervisor-selected
  proof subset.

## Reviewer Reject Signals

- Reject claiming idea 664 closure while rows 139 and 176 remain unexplained
  full-suite regressions.
- Reject fixing either row by changing expectations, unsupported markers,
  allowlists, timeout policy, runtime policy, or baseline accounting.
- Reject named-case shortcuts for either `backend_cli_riscv64_pointer_global_local_publication`
  or `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`.
- Reject coupling rows 139 and 176 into one implementation if evidence shows
  separate owners.
- Reject any change that reintroduces a `backend_riscv_object_emission`
  failure.
