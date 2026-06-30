# Branch Site Stack-Slot Current-Value No-Clobber Certificate

Status: Closed
Type: Producer/prepared branch-site stack-slot current-value and no-clobber certificate idea
Parent: `ideas/closed/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md`
Source Evidence: `build/agent_state/471_step4_residual_disposition/`
Owning Layer: Prepared branch-stack-load certificate production

## Goal

Publish producer-owned certificates proving that a branch-site stack slot
contains the current branch value and is not clobbered along the relevant path
before a later `load_from_stack_slot` branch materialization.

## Completion Notes

Closed after Step 4 residual disposition as a blocked/negative certificate
producer result. Idea 472 established that downstream branch-stack-load
certificates can consume current-value and no-clobber facts, but current
prepared evidence does not provide the lower-level source facts required to
produce those certificates.

The completed findings are:

- the branch-stack-load carrier and planner can consume downstream
  freshness/clobber facts;
- the representative `%t23` row has branch/value/home/frame-slot/object
  identity;
- no current prepared fact proves exact materialization of current `%t23` into
  slot `#21`;
- no current prepared fact proves same-slot no-clobber safety from a
  materialization source to `f.logic.end.14`;
- raw-shape inference from BIR adjacency, stack homes, object ids, value names,
  function names, testcase shape, block labels, or dump order remains rejected.

Representative residuals from Step 4:

- `f.logic.end.14` condition `%t23`, slot `#21`: identity exists, but exact
  materialization/current-value and no-clobber source facts are missing.
- `f.logic.end.14` lhs `%t22`, slot `#20`: select-result stack-destination and
  block-entry publication remain first.
- `f.block_1` `%t2` and `f.block_4` `%t8`: `unsupported_terminator`
  branch-site relationship ownership remains first.
- `f.block_1` `%t1` and `f.block_4` `%t7`: pointer/provenance plus
  branch-site relationship ownership remains first.

Follow-up source idea:
`ideas/open/473_branch_site_stack_slot_materialization_no_clobber_source.md`.

Branch-stack-load availability and RV64 branch-load consumption remain blocked
until explicit source facts exist and produce available
`PreparedBranchStackLoadAuthority` rows through the downstream certificate path.

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
