# Branch Stack-Load Authority Metadata

Status: Open
Type: Producer/prepared branch-stack-load authority metadata idea
Parent: `ideas/closed/451_stack_home_branch_operand_materialization.md`
Source Evidence: `build/agent_state/451_step4_residual_disposition/`
Owning Layer: Prepared branch operand/condition stack-load authority

## Goal

Publish explicit prepared metadata that says whether a branch operand or branch
condition value with a stack-slot home may be materialized by loading that
stack slot at the branch site.

## Why This Exists

Idea 451 classified current stack-home branch facts as insufficient for RV64
materialization. Prepared output exposes stack homes, slot ids, offsets, object
size/alignment, and branch conditions, but it does not expose branch-site load
policy, value freshness, clobber-safety proof, scratch/order constraints, or
pointer-provenance status. Stack object identity and offsets are necessary
evidence, not target-consumable authority.

## In Scope

- Audit representative stack-home branch rows from
  `build/agent_state/451_step4_residual_disposition/`, especially the
  `f.block_1` `%t2 = ult ptr %t1, %p.reg2` candidate.
- Define records that tie a prepared branch condition to stack-home
  condition/lhs/rhs values, operand role, value id/name, stack slot id, offset,
  size, alignment, and stack-layout object match.
- Publish available or unavailable branch-stack-load authority metadata with
  explicit load policy, value freshness, clobber safety, scratch/order
  constraints, and pointer-provenance status.
- Add focused producer/prepared coverage for accepted rows and fail-closed
  unavailable statuses.

## Out Of Scope

- RV64 stack-home branch load emission before available metadata exists.
- Weakening existing GPR-compatible branch publication predicates.
- Inferring branch loads, freshness, operands, conditions, or stack values from
  raw BIR shape, stack-slot spelling, block order, filenames, function names,
  or one prepared dump.
- Pointer-value/provenance repair for `%t7` or external formal provenance.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Prepared metadata can represent available and unavailable branch-stack-load
  authority records for stack-home branch operands or conditions.
- Positive coverage proves at least one accepted branch stack-load authority
  row only when branch condition, operand role, stack object, load policy,
  freshness, and clobber-safety facts are coherent.
- Negative coverage rejects missing policy, missing freshness, clobbered slot,
  mismatched branch condition, mismatched role, wrong slot/object, pointer
  provenance unknowns, select-result stack destinations, and raw-shape-only
  fixtures.
- Fresh residual disposition states whether an RV64 consumer idea can resume or
  whether another producer/provenance owner remains.

## Reviewer Reject Signals

- Reject RV64 branch-load emission or target lowering changes presented as
  producer/prepared metadata work.
- Reject accepting stack homes as branch-load authority from slot ids, offsets,
  stack-slot spelling, raw BIR shape, block order, filenames, function names,
  or testcase shape alone.
- Reject weakening GPR-compatible branch predicates so stack homes become
  implicitly target-consumable.
- Reject folding pointer-value/provenance or select-result stack-destination
  repair into this metadata idea.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, baseline/log churn, or classification-only
  changes presented as capability progress.
