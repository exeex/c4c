Status: Active
Source Idea Path: ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm `Rhs` Producer Readiness

# Current Packet

## Just Finished

Step 1 confirmed the exact 593 closure-note handoff still applies: "pointer
`Rhs` remains unwired because the producer/collector records it as
inventory-only with `policy=none` / `status=missing_policy`".

Current shared prepared/prealloc behavior still collects a
`PreparedBranchStackLoadRole::Rhs` row for the pointer branch stack-load, but
the selected authority is not ready for RV64 consumption. The collector only
selects `LoadFromStackSlot` policy for `Condition` or proven pointer `Lhs`;
`Rhs` therefore enters `plan_prepared_branch_stack_load_authority` with
`policy=none`, `pointer_status=unknown`, and returns `status=missing_policy`
before a selected `BranchStackLoadSource` / `BranchStackSlot` authority can be
accepted. The existing focused prepared test and dump expectation preserve the
blocked collector row as `role=rhs value=%rhs value_id=4 policy=none
pointer_status=unknown status=missing_policy`.

## Suggested Next

Lifecycle-block recommendation: stop 594 RV64 consumption work and return the
pointer `Rhs` producer policy gap to the 592 family. Do not delegate Step 2
RV64 audit until shared prepared/prealloc producer repair changes `Rhs` from
`policy=none` / `status=missing_policy` to selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority for the same
prepared source value, exact branch block, and terminator instruction index.

## Watchouts

- Do not add an RV64 fallback for missing pointer `Rhs` producer authority.
- Do not infer freshness from stack homes, frame slots, aggregate lanes,
  clobber facts, register facts, or operand shape.
- `Rhs` currently has inventory facts and one source-freshness candidate in the
  prepared collector row, but no selected consumer authority because the policy
  gate remains absent.
- Step 2 constraints, if producer repair later makes `Rhs` ready: audit only
  the narrow RV64 fused pointer conditional branch `Rhs` stack-source consumer,
  and require same prepared source value, `BranchStackLoadSource` use,
  `BranchStackSlot` source kind, exact branch block, and exact terminator
  instruction index before any emission path can accept it.

## Proof

Audit-only packet; no tests or code were edited, so the delegated proof command
was intentionally not run and no new root-level log was created. Existing
focused proof inspected:
`tests/backend/bir/backend_prepare_stack_layout_test.cpp`
`check_branch_stack_load_authority_contract`, which asserts the collected
`Rhs` row remains `MissingPolicy` / `None` and the prepared dump reports
`policy=none` / `status=missing_policy`.
