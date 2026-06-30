# Branch Site Stack-Slot Materialization No-Clobber Source

Status: Closed
Type: Producer/prepared branch-site stack-slot materialization and no-clobber source idea
Parent: `ideas/closed/472_branch_site_stack_slot_current_value_no_clobber_certificate.md`
Source Evidence: `build/agent_state/472_step4_residual_disposition/`
Owning Layer: Prepared source facts for branch-stack-load certificates

## Goal

Publish lower-level source facts that identify exact frame-slot
materialization/current-value writes and prove no same-slot clobber reaches the
branch site.

## Completion Notes

Closed after Step 4 residual disposition as a routed/blocked source-fact
initiative. Idea 473 established that current prepared/prealloc data does not
expose an independent carrier or producer for the source facts required by
branch-site stack-slot certificate production.

The completed findings are:

- `PreparedBranchStackLoadAuthorityInputs` is a downstream authority input, not
  a source-fact carrier;
- `collect_prepared_branch_stack_load_authorities` reaches branch condition,
  terminator, value home, frame slot, and stack object identity, but does not
  locate materialization writes or clobber history;
- no prepared record names current `%t23` as written/materialized into slot
  `#21`;
- no record proves path/dominance validity to `logic.end.14`;
- no summary excludes same-slot writes, call/helper/inline-asm effects, or
  publication/move-bundle/parallel-copy clobbers for slot `#21`;
- raw-shape inference from BIR adjacency, stack homes/storage, object ids,
  value names, function names, testcase shape, block labels, or dump order
  remains rejected.

Follow-up source idea:
`ideas/open/474_prepared_frame_slot_materialization_no_clobber_facts.md`.

Downstream branch-stack-load availability and RV64 branch-load consumption
remain blocked until the lower-level source-fact surface exists and later
packets consume it.

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
