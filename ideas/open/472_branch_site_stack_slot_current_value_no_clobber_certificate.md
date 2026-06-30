# Branch Site Stack-Slot Current-Value No-Clobber Certificate

Status: Open
Type: Producer/prepared branch-site stack-slot current-value and no-clobber certificate idea
Parent: `ideas/closed/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md`
Source Evidence: `build/agent_state/471_step4_residual_disposition/`
Owning Layer: Prepared branch-stack-load certificate production

## Goal

Publish producer-owned certificates proving that a branch-site stack slot
contains the current branch value and is not clobbered along the relevant path
before a later `load_from_stack_slot` branch materialization.

## Why This Exists

Idea 471 proved the branch-stack-load carrier and planner are adequate, but no
current prepared evidence links source identity, path validity, stack-write
exclusion, call/helper effects, publications, move bundles, and parallel-copy
effects into a durable current-value/no-clobber certificate. Without that
certificate, available branch-stack-load records would require raw-shape or
stack-home inference, which is rejected.

## In Scope

- Audit the scalar representative row `f.logic.end.14` condition `%t23`, slot
  `#21`, plus adjacent fail-closed rows from idea 471.
- Define current-value source identity for a stack slot at an exact branch
  site.
- Define path or dominance validity from the source identity to the branch
  site.
- Prove no stack write to the same frame slot/object invalidates the slot.
- Model call, helper, and inline-asm effects that may clobber the slot.
- Prove publication, move-bundle, and parallel-copy events do not overwrite or
  invalidate the slot before the branch load.
- Populate explicit certificate facts that later
  `PreparedBranchStackLoadAuthorityInputs` can consume.

## Out Of Scope

- RV64 branch-load emission or target lowering.
- Consuming unavailable branch-stack-load records as target authority.
- Pointer-value/provenance repair for `%t1` or `%t7`.
- Select-result/block-entry stack-destination repair for `%t22/%t23`.
- Branch-site relationship acceptance for `unsupported_terminator` rows.
- Inferring current value or no-clobber safety from stack homes, object ids,
  offsets, raw BIR adjacency, block labels, function names, testcase names, or
  dump order.
- Weakening GPR-compatible branch predicates.
- Generic stack-to-register materialization.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- A certificate can be produced only when current-value source identity, path
  validity, stack-write exclusion, call/helper/inline-asm effects, and
  publication/move-bundle/parallel-copy non-clobber facts are explicit.
- Positive coverage demonstrates at least one scalar branch condition row with
  a coherent current-value/no-clobber certificate.
- Negative coverage rejects missing source identity, invalid path, same-slot
  stack writes, unknown call/helper/inline-asm effects, clobbering
  publications, clobbering move bundles, select-result stack destinations,
  pointer-status/provenance gaps, unsupported terminator rows, and raw-shape
  fixtures.
- Fresh residual disposition states whether branch-stack-load availability can
  resume or whether another producer owner remains first.

## Reviewer Reject Signals

- Reject RV64 lowering changes presented as certificate producer work.
- Reject accepting unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Reject inferring current value or no-clobber safety from stack homes,
  offsets, object ids, raw BIR shape, block labels, function names, testcase
  names, or dump order.
- Reject folding pointer-value/provenance, select-result stack-destination, or
  unsupported-terminator branch-site relationship repair into this certificate
  idea.
- Reject weakening GPR-compatible branch predicates, generic move support,
  expectation rewrites, unsupported downgrades, allowlists, baseline churn, or
  classification-only changes claimed as capability progress.
