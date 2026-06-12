# 219 Phase E1 control-flow identity helper contraction

## Goal

Move one control-flow identity-only helper or selected reader from prepared
control-flow lookup authority to direct BIR/route identity authority, while
retaining prepared ownership for branch policy, scheduling, diagnostics,
fallback, wrappers, and expected strings.

## Why This Exists

Phase E1 triage classified the control-flow identity helper family as ready to
draft one implementation idea. The family has a named semantic owner in the
BIR function/block graph, terminator successors, block labels, and selected
route edge/join annotations. It also has clear retained prepared surfaces:
edge-copy scheduling, branch spelling, block-order emission, join transfer
records, move-bundle interaction, route-debug rows, printer/debug rows,
fallback/oracle checks, wrappers, and expected strings.

This idea exists to turn that accepted readiness classification into one
bounded implementation runbook. It is not permission to retire
`PreparedControlFlow` as an aggregate or to rewrite branch/output policy.

## In Scope

- Select exactly one identity-only helper or one selected reader from the
  `PreparedControlFlow` family.
- Candidate surfaces include
  `resolve_prepared_block_label_id(...)`,
  `find_prepared_control_flow_function(...)`,
  `find_prepared_control_flow_block(...)`,
  `find_prepared_linear_join_predecessor(...)`,
  `find_prepared_control_flow_branch_target_labels(...)`, or a single
  selected branch/join facade helper.
- Read BIR function/block identity, block labels, terminator successor facts,
  or selected route edge/join identity directly for that one selected surface.
- Keep prepared fallback for absent, invalid, duplicate/conflict, and mismatch
  cases.
- Preserve branch spelling, edge-copy scheduling, move policy, printer/debug
  rows, wrapper output, helper-oracle behavior, and expected strings.

## Out Of Scope

- Aggregate `PreparedControlFlow` retirement or public prepared API deletion.
- `PreparedBirModule` or `PreparedFunctionLookups` retirement.
- Branch policy, edge-copy scheduling, block-order emission, move-bundle
  policy, join transfer records, wrapper behavior, route-debug rows, prepared
  printer/debug rows, diagnostics, helper-oracle strings, or expected-string
  changes.
- Route 8 return-chain work unless a separate source idea imports a return
  schema.
- Liveness, intrinsic metadata, aggregate forwarding, diagnostic/oracle row
  replacement, E2, E3, E4, draft 155, or E5 work.
- Baseline refreshes, unsupported downgrades, helper renames, timeout masking,
  or expectation rewrites as proof.

## Proof Requirements

- Positive agreement proof: the selected BIR/route identity read returns the
  same identity as the retained prepared helper for the chosen surface.
- Absent proof: missing BIR/route identity evidence keeps the prepared result
  or prepared diagnostic behavior.
- Invalid proof: invalid BIR/route references fail closed to prepared behavior.
- Duplicate/conflict proof: ambiguous identity evidence does not override the
  prepared result.
- Mismatch proof: BIR/route and prepared disagreement preserves prepared
  behavior.
- Policy fallback proof: branch spelling, edge-copy scheduling, move policy,
  wrapper output, printer/debug rows, helper-oracle output, and expected
  strings remain byte-stable.
- Nearby same-feature proof: at least one adjacent control-flow identity case
  remains covered so the slice is not shaped only around one testcase.

## Reviewer Reject Signals

- Reject broad prepared control-flow aggregate retirement or public API
  deletion.
- Reject moving branch policy, edge-copy scheduling, block-order emission,
  move-bundle policy, wrapper output, printer/debug rows, diagnostics, helper
  oracles, or expected strings into BIR/route authority.
- Reject testcase-shaped handling of one named control-flow case without
  absent, invalid, duplicate/conflict, mismatch, and policy fallback proof.
- Reject baseline refreshes, helper renames, unsupported downgrades, timeout
  masking, or expected-string rewrites as evidence.
- Reject retaining prepared-only behavior behind a newly route- or BIR-named
  facade without actually replacing one duplicate semantic identity read.
