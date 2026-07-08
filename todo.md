Status: Active
Source Idea Path: ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The `Rhs` Producer Policy Gap

# Current Packet

## Just Finished

Completed Step 1 audit for `Rhs` branch stack-load producer authority.
`PreparedBranchStackLoadRole::Rhs` reaches inventory through
`collect_prepared_branch_stack_load_authorities` ->
`collect_branch_stack_load_authority_for_role` ->
`make_branch_stack_load_authority_record`, which publishes a
`BranchStackLoadSource` / `BranchStackSlot` freshness candidate for `%rhs` but
then calls `plan_prepared_branch_stack_load_authority` with policy/status
inputs selected by the collected-policy helpers.

## Suggested Next

Execute Step 2 in `src/backend/prealloc/publication_plans.cpp`: generalize the
collected pointer-side policy proof currently named for `Lhs` so valid pointer
`Rhs` fused-compare branch stack-load rows can pass
`LoadFromStackSlot`, `pointer_status=proven`, and clobber-safety inputs into
`plan_prepared_branch_stack_load_authority`.

## Watchouts

- Exact policy gap: `prepared_collected_branch_stack_load_policy` returns
  `PreparedBranchStackLoadPolicy::None` unless the role is `Condition` or
  `Lhs` with `lhs_pointer_proven`, so a valid collected pointer `Rhs` row is
  forced to `status=missing_policy` before selected freshness can be queried.
- Adjacent Step 2 gates must move with the policy: current
  `prepared_branch_stack_load_lhs_pointer_is_proven`,
  `prepared_collected_branch_stack_load_pointer_status`, and
  `prepared_collected_branch_stack_load_clobber_safe` only prove/clobber-accept
  `Lhs`. A policy-only change would next fail as
  `pointer_status_unknown` or `missing_stack_clobber_safety`.
- Keep the repair semantic and shared: accept only pointer fused-compare
  branch operands that are the same prepared source value for the same
  branch use at the exact branch block and terminator instruction position.
  Do not add RV64/AArch64/x86/string assembly consumer inference or any
  target-local fallback.
- Focused proof surface is existing
  `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  `check_branch_stack_load_authority_contract`, especially the collector
  section that currently asserts `role=rhs value=%rhs value_id=4 policy=none
  pointer_status=unknown status=missing_policy` while also observing one
  published source freshness candidate.
- Existing dump surface is `prepare::print(prepared)` /
  `append_branch_stack_load_authorities`; the expected repaired dump should
  show the `rhs` row as selected `branch_stack_slot` authority, symmetric with
  the accepted `lhs` row, while invalid freshness routes remain fail-closed.

## Proof

Audit-only packet. Read-only inspection of `plan.md`, `todo.md`, required idea
files, `src/backend/prealloc/publication_plans.cpp`,
`src/backend/prealloc/publication_plans.hpp`,
`src/backend/prealloc/prepared_printer/select_chains.cpp`, and
`tests/backend/bir/backend_prepare_stack_layout_test.cpp`; no build/tests run
and no proof logs written.
