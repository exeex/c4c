# Branch Stack-Load Policy Freshness

Status: Closed
Type: Producer/prepared branch-site load policy and freshness idea
Parent: `ideas/closed/469_branch_stack_load_authority_metadata.md`
Source Evidence: `build/agent_state/469_step6_residual_disposition/`
Owning Layer: Prepared branch-stack-load policy, freshness, and clobber-safety production

## Goal

Produce the branch-site `load_from_stack_slot` policy, freshness, and
clobber-safety facts required to turn eligible `PreparedBranchStackLoadAuthority`
records from unavailable to available.

## Completion Notes

Closed after Step 4 residual disposition as a split/blocker. Idea 470 completed
the carrier and contract review for available branch-stack-load records:

- `PreparedBranchStackLoadAuthorityInputs` can carry branch-site load policy,
  stack freshness, and clobber-safety fields;
- the planner models `missing_policy`, `missing_stack_freshness`, and
  `missing_stack_clobber_safety`;
- the current real collector keeps representative rows fail-closed instead of
  inventing authority from stack homes or raw shape;
- focused validation for the Step 3 route passed before this lifecycle review.

The source idea is not available-record complete because current prepared facts
do not prove branch-site stack-slot freshness or clobber safety. Closing this
idea preserves that as a blocker instead of broadening it into raw inference or
RV64 consumer work.

Representative residuals from Step 4:

- `f.logic.end.14` condition `%t23`: populated value/home/slot/object facts,
  `status=missing_policy`; first follow-up owner is branch-site stack-slot
  freshness and clobber-safety metadata.
- `f.logic.end.14` lhs `%t22`: populated value/home/slot/object facts,
  `status=missing_policy`; select-result stack-destination remains separate
  before target consumption.
- `f.block_1` condition `%t2` and `f.block_4` condition `%t8`:
  `status=unsupported_terminator`; branch-site relationship acceptance remains
  a prerequisite before freshness can be first owner.
- `f.block_1` lhs `%t1` and `f.block_4` lhs `%t7`:
  `status=unsupported_terminator`, `pointer_status=unknown`; pointer/provenance
  remains separate.

Follow-up source idea:
`ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md`.

RV64 consumption remains blocked until a later residual disposition shows
available branch-stack-load records backed by explicit freshness and
clobber-safety evidence.

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
