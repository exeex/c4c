Status: Active
Source Idea Path: ideas/open/592_typed_aggregate_branch_stack_source_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Backend Proof And Closure Inventory

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by writing lifecycle-ready closure inventory for
idea 592 without moving or closing the source idea.

Audited producers:
- Branch stack-load authority inventory flows through
  `collect_prepared_branch_stack_load_authorities`,
  `collect_branch_stack_load_authority_for_role`,
  `make_branch_stack_load_authority_record`, and
  `plan_prepared_branch_stack_load_authority`.
- `PreparedBranchStackLoadRole::{Condition,Lhs,Rhs}` source values come from
  `branch_stack_load_value_for_role`.
- Structural source facts also flow through frame-slot source-fact collection
  and aggregate stack-source authority, but stack homes, frame slots, stack
  objects, aggregate lanes, materialization/path facts, and clobber facts remain
  context only.

Publication contract completed in Step 2:
- `PreparedBranchStackSourceFreshnessPublicationInputs` and
  `publish_prepared_branch_stack_source_freshness_candidate` publish explicit
  source freshness only when the candidate matches the source value id/name,
  `PreparedValueFreshnessUseKind::BranchStackLoadSource`,
  `PreparedValueFreshnessSourceKind::BranchStackSlot`,
  `PreparedValueFreshnessProofKind::BranchTerminatorOrdering`,
  `PreparedValueFreshnessSourceRank::BranchStackSlot`, the stack-slot home, and
  the exact branch block plus terminator instruction point.
- The existing condition route keeps selected shared freshness, while pointer
  `Lhs` can publish a branch stack-slot freshness candidate before consumer
  migration. Register structural facts, wrong-value stack facts, and missing
  branch-point references cannot publish authority.

Migrated consumer:
- Only pointer `PreparedBranchStackLoadRole::Lhs` moved from inventory-only
  `policy=none` to `LoadFromStackSlot`.
- The `Lhs` consumer becomes available only with selected producer-published
  `BranchStackSlot` freshness for `BranchStackLoadSource`, pointer proof from
  the fused pointer compare, matching stack home/value/branch point metadata,
  and conservative same-block clobber safety with no intervening instructions
  before the branch terminator.
- Failure to meet those requirements still fails closed as
  `missing_source_freshness_authority`, `missing_stack_clobber_safety`, or
  `pointer_status_unknown`.

Covered proof cases:
- Accepted `Lhs` route selects the explicit producer-published
  `BranchStackSlot` authority at the exact branch terminator point.
- Focused fail-closed coverage includes missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only authority.
- Existing 587, 588, 589, and 590 freshness coverage stayed inside the backend
  proof subset, and no expectation, unsupported-marker, allowlist,
  target-emission, runtime-output, `plan.md`, or source-idea edits were needed
  to claim progress.

Deliberately blocked or out of scope:
- Pointer `Rhs` remains inventory-only/out of scope even when visible as a
  stack-backed row with a candidate.
- Aggregate-adjacent consumers remain blocked; their concrete source facts are
  inputs for a later producer rule, not direct branch freshness.
- Select consumers, edge-publication consumers, target branch emission
  inference, broader branch migrations, RV64, AArch64, and x86 consumption or
  emission work remain out of scope for idea 592 closure.

## Suggested Next

Supervisor should review the Step 5 closure inventory, then hand lifecycle
closure/deactivation/split decisions to plan-owner. Do not move or close the
source idea from this executor packet.

## Watchouts

- Only pointer `Lhs` was migrated. `Rhs`, aggregate-adjacent consumers,
  select/edge consumers, and target emission remain out of scope and blocked.
- The `Lhs` clobber proof is intentionally conservative: it only accepts a
  branch block with no intervening instructions before the terminator. Wider
  clobber classification belongs in a later packet.
- The current frame-slot source-fact collector treats pointer values and
  select-materialization results as unsupported boundaries; do not reuse those
  unsupported structural rows as later authority.
- Aggregate-adjacent authority currently exposes concrete source facts but
  reports missing aggregate copy authority; treat those facts as inputs to a
  future producer rule, not as direct branch freshness.
- Do not accept a branch stack source because a stack home, frame slot,
  aggregate lane, or clobber-safety fact exists.
- Do not use target-local branch emission inference as freshness authority.
- Do not weaken expectations, unsupported markers, allowlists, or runtime
  contracts as proof of capability progress.
- Keep broader branch, select, edge-publication, RV64, AArch64, and x86
  migrations out of the first packet.

## Proof

No command was delegated for Step 5; this was a closure-inventory-only packet.

Recorded accepted validation context:
- Step 4 backend proof command passed:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1
```

- `test_after.log` reports 100% tests passed, 0 failed out of 346 for the
  backend subset.
- Matching `test_before.log` / `test_after.log` backend regression-guard
  comparisons are green at 346/346 before and after.
- Hook-managed full-suite baselines after the code commits were accepted at
  3375/3375.
