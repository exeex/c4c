# Branch Site Stack-Slot Freshness Clobber Safety Metadata

Status: Open
Type: Producer/prepared branch-site stack-slot freshness and clobber-safety idea
Parent: `ideas/closed/470_branch_stack_load_policy_freshness.md`
Source Evidence: `build/agent_state/470_step4_residual_disposition/`
Owning Layer: Prepared branch-stack-load freshness and clobber-safety production

## Goal

Publish producer-owned evidence that a branch-site stack slot still contains
the current branch value and cannot be clobbered before a later
`load_from_stack_slot` branch materialization.

## Why This Exists

Idea 470 confirmed that the prepared branch-stack-load authority carrier and
planner can represent `load_from_stack_slot`, stack freshness, and clobber
safety. It did not have producer evidence to set those fields for real
representative rows. RV64 branch-load consumption remains blocked until
available records are backed by explicit freshness and clobber-safety facts.

## In Scope

- Audit branch-site lifetime, stack-write, call/helper, publication, and move
  bundle facts for representative rows from idea 470.
- Define durable freshness evidence that a stack slot contains the current
  branch operand or condition value at the exact branch site.
- Define durable clobber-safety evidence that no intervening store, helper,
  call, publication, move bundle, or stack write can invalidate that slot before
  the branch-site load.
- Integrate proven freshness and clobber-safety facts into
  `PreparedBranchStackLoadAuthorityInputs` only after they are explicit.
- Preserve fail-closed statuses for missing freshness, missing clobber safety,
  pointer-status unknown, select-result stack destinations, and unsupported
  terminator rows.

## Out Of Scope

- RV64 branch-load emission or target lowering.
- Treating unavailable `PreparedBranchStackLoadAuthority` records as target
  authority.
- Pointer-value/provenance repair for pointer-status unknown rows.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Inferring freshness or clobber safety from stack homes, offsets, object ids,
  raw BIR shape, block labels, function names, or testcase shape.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Available branch-stack-load records are produced only when branch-site stack
  freshness and clobber safety are explicitly proven.
- Positive coverage demonstrates at least one scalar branch stack-load row with
  coherent policy, freshness, and clobber-safety facts.
- Negative coverage rejects missing freshness, missing clobber safety, stale
  slots, clobbered slots, wrong branch/value/role, pointer-status unknown,
  select-result stack destinations, unsupported terminator rows, and raw-shape
  fixtures.
- Fresh residual disposition states whether RV64 branch-load consumption may
  resume or whether another producer/provenance owner remains first.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as freshness/clobber-safety producer
  work.
- Reject accepting unavailable branch-stack-load records as target authority.
- Reject inferring freshness or clobber safety from stack-slot spelling,
  offsets, object ids, raw BIR shape, block labels, function names, testcase
  names, or dump order.
- Reject folding pointer-value/provenance or select-result stack-destination
  repair into this idea.
- Reject weakening GPR-compatible branch predicates, adding generic move
  support, expectation rewrites, unsupported downgrades, allowlists, baseline
  churn, or classification-only changes claimed as capability progress.
