# Branch Stack-Load Policy Freshness

Status: Open
Type: Producer/prepared branch-site load policy and freshness idea
Parent: `ideas/closed/469_branch_stack_load_authority_metadata.md`
Source Evidence: `build/agent_state/469_step6_residual_disposition/`
Owning Layer: Prepared branch-stack-load policy, freshness, and clobber-safety production

## Goal

Produce the branch-site `load_from_stack_slot` policy, freshness, and
clobber-safety facts required to turn eligible `PreparedBranchStackLoadAuthority`
records from unavailable to available.

## Why This Exists

Idea 469 completed the prepared branch-stack-load authority record surface:
records can be planned, collected from real prepared modules, printed/probed,
and kept fail-closed. The representative `930930-1` rows still remain
unavailable because no producer currently supplies branch-site load policy,
freshness, or clobber-safety. RV64 branch-load consumption must wait until
available records exist.

## In Scope

- Audit representative branch-stack-load records from
  `build/agent_state/469_step6_residual_disposition/`.
- Define when a stack-home branch condition or operand may be reloaded at a
  branch site using explicit `load_from_stack_slot` policy.
- Prove the stack slot contains the current value at the branch site.
- Prove no intervening store, call, helper, move bundle, publication, or stack
  write clobbers the slot before the branch load.
- Populate available or more precise unavailable records for scalar
  branch-stack-load rows where pointer/provenance and select-result boundaries
  do not own the first failure.

## Out Of Scope

- RV64 branch-load emission or target lowering.
- Pointer-value/provenance repair for `%t7` or external formal provenance.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Treating pointer operands as proven when `pointer_status=unknown`.
- Accepting raw BIR shape, stack-slot spelling, offsets, object ids, block
  order, filenames, function names, or testcase shape as load authority.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Branch-stack-load records can become available only when branch-site load
  policy, freshness, and clobber-safety are explicitly proven.
- Positive coverage proves at least one scalar branch stack-load row with
  coherent policy/freshness/clobber facts.
- Negative coverage rejects missing policy, stale slot, clobbered slot, wrong
  branch/role/value, pointer-status unknown, pointer-value provenance gaps,
  select-result stack destinations, and raw-shape-only fixtures.
- Fresh residual disposition states whether RV64 branch-load consumption may
  resume or whether another producer/provenance owner remains.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as policy/freshness producer work.
- Reject accepting unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Reject accepting stack homes from slot ids, offsets, object ids, stack-slot
  spelling, raw BIR shape, block order, filenames, function names, or testcase
  shape alone.
- Reject folding pointer-value/provenance or select-result stack-destination
  repair into this policy/freshness idea.
- Reject weakening GPR-compatible branch predicates, generic move support,
  expectation rewrites, unsupported downgrades, allowlists, baseline/log churn,
  or classification-only changes presented as capability progress.
