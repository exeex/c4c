# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 10
Current Step Title: Freeze per-revision constraint projection ownership

## Just Finished

- Plan Step 10 defined `ConstraintProjectionTransaction` as the sole shared
  subordinate projection/preservation authority and `ProjectedConstraintSet`
  plus `ProjectedConstraintKey` as the only later-revision product.
- Immutable C9 `BoundConstraintSet` remains keyed to exact Canonical. D1, D2,
  every D4 occurrence, initial D5, every E3 retry, and D5 copy resolution now
  invoke the shared authority inside their own failure-atomic transactions.
- D3-D5 and E1-E4 require the product keyed to their exact current revision;
  Canonical C9, predecessor projections, stable IDs, structural equality, and
  copied records cannot establish freshness.

## Suggested Next

- Execute Plan Step 11, "Reconcile legacy and core dispositions."

## Watchouts

- Step 11 must preserve the shared E1-E3 BIR allocation ownership and strict
  MIR non-ownership established by this projection and copy-resolution route.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && rg -n 'BoundConstraintSet|ProjectedConstraint|projection|project|preserv|Canonical|D1|D2|D4|D5|E1|E2|E3|E4|CopyResolution|PipelineStageStamp|revision|fingerprint|tombstone|invalidat|transaction|failure|stable ID|fresh' src/backend/bir/regalloc/constraints/README.md src/backend/bir/passes/pseudo_lowering/README.md src/backend/bir/passes/call_lowering/README.md src/backend/bir/passes/target/README.md src/backend/bir/passes/out_of_ssa/README.md src/backend/bir/analysis/liveness/README.md src/backend/bir/regalloc/README.md src/backend/bir/regalloc/spill_reload/README.md src/backend/bir/allocated/README.md src/backend/bir/verify/README.md src/backend/bir/pseudo/README.md src/backend/bir/README.md && ! rg -n '(D1|D2|D4|D5|E1|E2|E3|E4).*(consume|uses?).*Canonical[- ]keyed.*(constraint|BoundConstraintSet)|stable ID.*(prove|establish).*(fresh|key)' src/backend/bir`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
