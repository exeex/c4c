# Branch Site Stack-Slot Materialization No-Clobber Source

Status: Open
Type: Producer/prepared branch-site stack-slot materialization and no-clobber source idea
Parent: `ideas/closed/472_branch_site_stack_slot_current_value_no_clobber_certificate.md`
Source Evidence: `build/agent_state/472_step4_residual_disposition/`
Owning Layer: Prepared source facts for branch-stack-load certificates

## Goal

Publish lower-level source facts that identify exact frame-slot
materialization/current-value writes and prove no same-slot clobber reaches the
branch site.

## Why This Exists

Idea 472 proved that downstream branch-stack-load certificates need explicit
current-value and no-clobber sources, but current prepared evidence does not
state that `%t23` was written or materialized into slot `#21`, nor does it
prove that slot `#21` remains safe until `f.logic.end.14`. Without these source
facts, certificate production would require raw-shape inference from BIR
adjacency, stack homes, object ids, value names, function names, testcase shape,
or dump order.

## In Scope

- Audit the representative `f.logic.end.14` condition `%t23`, slot `#21`, and
  nearby prepared facts for exact materialization/current-value source rows.
- Define a producer-owned fact for source value plus exact frame-slot
  materialization or write into a stack slot.
- Define path or dominance validity from that materialization/write to the
  branch site.
- Define same-slot stack-write exclusion for the relevant frame slot/object.
- Define call, helper, and inline-asm effect safety for the stack slot.
- Define publication, move-bundle, and parallel-copy non-clobber safety for the
  stack slot.
- Expose records/statuses that later branch-stack-load certificate logic can
  consume.

## Out Of Scope

- RV64 branch-load emission or target lowering.
- Populating available `PreparedBranchStackLoadAuthority` records directly.
- Treating unavailable branch-stack-load records as target authority.
- Pointer-value/provenance repair for `%t1` or `%t7`.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring materialization or no-clobber facts from stack homes, object ids,
  offsets, raw BIR adjacency, value names, block labels, function names,
  testcase names, or dump order.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Source records identify exact source value, frame slot/object, and
  materialization/write event before any downstream certificate consumes them.
- Positive coverage demonstrates a scalar branch condition source only when
  exact materialization, path validity, same-slot write exclusion,
  call/helper/inline-asm effect safety, and publication/move/parallel-copy
  non-clobber facts are explicit.
- Negative coverage rejects missing materialization, invalid path, same-slot
  writes, unknown call/helper/inline-asm effects, clobbering publications,
  clobbering move bundles, select-result stack destinations,
  pointer/provenance gaps, unsupported terminator rows, and raw-shape fixtures.
- Fresh residual disposition states whether downstream certificate production
  can resume or whether another source-fact owner remains first.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as source-fact producer work.
- Reject directly marking branch-stack-load records available from this idea.
- Reject inferring materialization/current-value/no-clobber safety from stack
  homes, offsets, object ids, raw BIR shape, value names, block labels,
  function names, testcase names, or dump order.
- Reject folding pointer-value/provenance, select-result stack-destination, or
  unsupported-terminator branch-site relationship repair into this source idea.
- Reject weakening GPR-compatible branch predicates, generic move support,
  expectation rewrites, unsupported downgrades, allowlists, baseline churn, or
  classification-only changes claimed as capability progress.
