# Branch Site Stack-Slot Freshness Clobber Safety Metadata

Status: Closed
Type: Producer/prepared branch-site stack-slot freshness and clobber-safety idea
Parent: `ideas/closed/470_branch_stack_load_policy_freshness.md`
Source Evidence: `build/agent_state/470_step4_residual_disposition/`
Owning Layer: Prepared branch-stack-load freshness and clobber-safety production

## Goal

Publish producer-owned evidence that a branch-site stack slot still contains
the current branch value and cannot be clobbered before a later
`load_from_stack_slot` branch materialization.

## Completion Notes

Closed after Step 4 residual disposition as a negative producer-feasibility
result. Idea 471 established that the branch-stack-load carrier and planner are
adequate, and that available records must remain blocked until a producer owns
current-value source identity plus no-clobber evidence.

The completed findings are:

- `PreparedBranchStackLoadAuthorityInputs` can carry policy, stack freshness,
  and clobber safety;
- the planner keeps records fail-closed as `missing_policy`,
  `missing_stack_freshness`, or `missing_stack_clobber_safety`;
- real collection does not provide durable source identity or no-clobber
  certificates for scalar branch condition rows;
- using stack homes, object identity, raw BIR adjacency, function names,
  testcase shape, block labels, or dump order would violate the source idea.

Representative residuals from Step 4:

- `f.logic.end.14` condition `%t23`, slot `#21`: branch/value/home/frame-slot
  identity exists, but no durable fact proves slot `#21` contains current
  `%t23` at the branch site or remains unclobbered.
- `f.logic.end.14` lhs `%t22`, slot `#20`: select-result stack-destination
  ownership remains first.
- `f.block_1` condition `%t2` and `f.block_4` condition `%t8`:
  `unsupported_terminator` relationship ownership remains first.
- `f.block_1` lhs `%t1` and `f.block_4` lhs `%t7`: pointer/provenance plus
  branch-site relationship ownership remains first.

Follow-up source idea:
`ideas/open/472_branch_site_stack_slot_current_value_no_clobber_certificate.md`.

RV64 branch-load consumption remains blocked until available
`PreparedBranchStackLoadAuthority` records exist.

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
