Status: Active
Source Idea Path: ideas/open/590_branch_stack_load_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Branch Stack-Load Consumers

# Current Packet

## Just Finished

Completed Step 1, "Audit Branch Stack-Load Consumers", from `plan.md`.

Audited helper/API surface around branch stack-load authority:

- `branch_stack_load_value_for_role`: maps `Condition`, `Lhs`, and `Rhs` to
  the prepared branch condition value, fused compare lhs, or fused compare rhs.
- `plan_prepared_branch_stack_load_authority`: direct authority planner for
  branch stack-load roles; it already fail-closes on missing names, branch
  condition, terminator, unsupported/non-conditional terminator, target drift,
  value/home mismatch, non-stack homes, missing/mismatched frame slot, missing
  or mismatched stack object, missing policy, missing stack freshness, missing
  stack clobber safety, and pointer sources without proven pointer status.
- `prepared_branch_stack_load_authority_available`: accepts only
  `PreparedBranchStackLoadAuthorityStatus::Available`.
- `make_branch_stack_load_authority_record` and
  `collect_branch_stack_load_authority_for_role`: shared collector path for
  named branch values whose current home is a stack slot; records currently call
  the planner with `policy=none`, so they expose inventory rows rather than a
  migrated freshness-authorized route.
- `collect_prepared_branch_stack_load_authorities`: visits every prepared
  branch condition and tries `Condition`, `Lhs`, then `Rhs`.
- `append_branch_stack_load_authorities`: printer consumer for collected rows
  and current status/policy/pointer metadata.
- Nearby role-sharing source-fact APIs:
  `make_frame_slot_source_fact_record`,
  `collect_frame_slot_source_fact_for_role`, and
  `collect_prepared_frame_slot_source_facts`; these reuse
  `PreparedBranchStackLoadRole` but are frame-slot source-fact inventory, not
  the selected branch stack-load freshness migration route.

Representative route selected for migration: the shared collector's scalar
`Condition` stack-slot row,
`collect_prepared_branch_stack_load_authorities -> collect_branch_stack_load_authority_for_role(Condition) -> make_branch_stack_load_authority_record -> plan_prepared_branch_stack_load_authority`.
This is the narrowest valid first route because it already has branch
terminator identity, target labels, named value identity, prepared stack home,
frame slot, stack object, clobber/freshness status slots, fail-closed
diagnostics, and prepared dump visibility, while avoiding pointer provenance
and fused-operand complications. The direct planner test route already appears
locally protected by `stack_slot_fresh_at_branch`, but the shared collector
route is not migrated yet because it still publishes `policy=none` and does not
select shared freshness authority.

## Suggested Next

Execute Step 2 from `plan.md`: define the branch-point freshness ownership rule
for the selected scalar `Condition` stack-load collector route, including the
freshness use kind, accepted source kinds, and the branch terminator ordering
point.

## Watchouts

- Do not treat a complete stack home, stack object, branch payload, or
  clobber-safety fact as freshness authority.
- Do not edit expectations, unsupported markers, allowlists, or runtime output
  as proof of progress.
- Current protection/blocking classifications:
  - Protected: direct `plan_prepared_branch_stack_load_authority` calls for
    scalar `Condition` and pointer `Lhs` already require explicit local policy,
    freshness, clobber safety, and pointer proof where applicable before
    returning `Available`; structural-only rows fail closed.
  - Protected: collector rows are inventory-only today because
    `policy=none` produces `MissingPolicy`; printer output exposes that status
    instead of claiming availability.
  - Blocked: pointer `Lhs`/`Rhs` branch stack-load migration needs a real
    pointer/source freshness story before it can move beyond
    `PointerStatusUnknown` or local `Proven`.
  - Blocked: frame-slot source-fact branch role rows deliberately classify
    pointer, select-materialization, and unsupported-terminator boundaries as
    `UnsupportedBoundary`; do not fold those into the first branch freshness
    migration.
  - Deferred: non-selected fused compare `Lhs`/`Rhs` stack-slot rows, full
    migration of all branch forms, and target-specific RV64/AArch64/x86
    consumers.
  - Out of scope: typed or aggregate stack-source producer/publication facts,
    target ABI/physical register policy, target-local branch lowering, and
    expectation/allowlist/runtime-output rewrites.

## Proof

`cmake --build --preset default > test_after.log 2>&1` passed. Proof log:
`test_after.log`.
