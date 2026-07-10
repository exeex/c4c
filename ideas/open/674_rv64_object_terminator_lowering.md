# RV64 Object Terminator Lowering

Status: Open
Type: Investigation / Implementation
Parent: `ideas/open/673_post_664_full_suite_regression_probe.md`
Related:
- `ideas/open/673_post_664_full_suite_regression_probe.md`
- `ideas/open/664_riscv_object_emission_internal_probe.md`
Owning Layer: RV64 object emission terminator lowering
Queue Order: 74
Proof Surface: row 176
`backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`

## Goal

Repair the row 176 RV64 object-emission failure where a prepared conditional
or fused pointer branch reaches `--codegen obj` and fails with
`unsupported_terminator_fragment`.

## Why This Exists

Idea 673 Step 1 classified rows 139 and 176 from the post-664 full-suite
regression probe. The rows share the broad RV64 object-emission phase, but
their first failing contracts differ. Row 176's prepared dump succeeds and
shows a prepared fused pointer compare branch with available branch stack load
authorities, while object emission stops before clang link or QEMU runtime
with:

`unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering`

That owner is separate from row 139's direct-global local-memory publication
reload path, so row 176 must not be repaired inside idea 673.

## In Scope

- Reproduce row 176 with the focused runtime/object-emission proof command.
- Inspect the prepared branch facts and RV64 object terminator lowering path.
- Add semantic object-emission lowering for the proven prepared conditional or
  fused pointer branch contract.
- Preserve existing prepared dump behavior and row-256
  `backend_riscv_object_emission` success.
- Compare against the accepted 3386/3397 baseline when closure is considered.

## Out Of Scope

- Repairing row 139
  `backend_cli_riscv64_pointer_global_local_publication`; that remains in
  idea 673.
- Test expectation rewrites, unsupported-marker changes, allowlists, timeout
  changes, runtime policy changes, or baseline accounting changes.
- Broad terminator or RV64 runtime rewrites without a named first failing
  branch-lowering contract.
- Reopening the idea 664 row-256 object-emission path unless it actually
  regresses.

## Acceptance Criteria

- Row 176 no longer fails at `--codegen obj` with
  `unsupported_terminator_fragment`.
- The repair is semantic terminator lowering, not testcase-shaped matching or
  expectation churn.
- The supervisor-selected focused proof shows row 176 improvement and keeps
  `backend_riscv_object_emission` passing.
- Broader regression proof has no new failures against the accepted baseline
  before this idea is closed.

## Reviewer Reject Signals

- Reject changes that only rename, mask, or reclassify
  `unsupported_terminator_fragment` while row 176 still cannot lower the
  prepared branch to RV64 object code.
- Reject named-case shortcuts for
  `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime-policy,
  or baseline-accounting changes claimed as progress.
- Reject broad RV64 runtime or terminator rewrites that are not tied to the
  proven prepared conditional/fused pointer branch lowering contract.
- Reject any change that regresses row 256 `backend_riscv_object_emission` or
  recouples row 139 into this terminator-lowering route.
