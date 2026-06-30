# RV64 Move-Bundle Residual Owner Audit

Status: Closed
Type: RV64 residual-owner audit idea
Parent: `ideas/closed/459_rv64_select_edge_suppression_placement_consumer.md`
Source Evidence: `build/agent_state/459_step4_residual_disposition/`
Close Evidence: `build/agent_state/460_step2_residual_disposition/disposition.md`
Owning Layer: RV64 move-bundle materialization/suppression residual
classification
Closed Disposition: Complete as residual-owner audit; blocked as an
implementation selector by missing coordinate-bearing RV64 move-bundle
rejection evidence. Follow-up:
`ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md`.

## Goal

Identify the next exact owner of the remaining `20010329-1` RV64 object-route
`unsupported_move_bundle_target_shape` after explicit select-edge suppression
consumption is complete, without widening into generic move support or raw
shape inference.

## Why This Exists

Idea 459 consumed explicit `predecessor_edge_consumed_suppression` placement
metadata and kept generic move support out of scope. Fresh `20010329-1` object
probing still fails with the broad move-bundle diagnostic, but Step 4 could not
identify the exact event coordinate from current diagnostics. A residual-owner
audit is needed before selecting another implementation packet.

## In Scope

- Reproduce the Step 4 prepared/object probes for `20010329-1`.
- Inspect prepared move-bundle rows and object-route diagnostics to identify
  the first unsupported move-bundle event after idea 459.
- Use read-only instrumentation, debugger probes, or targeted prepared-output
  analysis as needed to locate the exact event coordinate and ownership family.
- Classify the residual as an existing source idea, a new producer/prepared
  metadata gap, a narrow RV64 consumer packet, or a non-move-bundle owner.
- Preserve fail-closed treatment for stale stack-load authority, generic
  register/stack move support, and raw-shape inference.

## Out Of Scope

- Implementing RV64 move-bundle lowering in the audit step.
- Generic stack-to-register or register-to-register move support without a
  precise source idea.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening idea 459 explicit suppression consumption, idea 456
  cast-dependency consumption, or idea 458 producer placement metadata.
- Pointer-value provenance, generic stack-home branch materialization, local or
  global store publication, expectation rewrites, unsupported-marker changes,
  allowlist edits, baseline churn, and pass/fail accounting changes.

## Acceptance Criteria

- The remaining `20010329-1` object-route failure is classified with an exact
  first owner and evidence.
- Any follow-up implementation idea is specific to the identified owner and
  rejects stale stack-load authority, generic move support, and raw-shape
  inference.
- If no implementation packet is justified, the audit records the blocker and
  leaves the route fail-closed.
- No code changes or test expectation changes are claimed as progress for this
  audit idea.

## Completion Notes

Idea 460 reproduced the `20010329-1` routes and narrowed the candidate set to
exposed prepared move-bundle events, but it could not prove the exact first
failing event from current object diagnostics. Stderr reports only
`unsupported_move_bundle_target_shape`, and the inherited debugger probe hit
multiple `fragment_for_prepared_move_bundle` calls without optimized local
argument state.

The strongest candidate remains the before-instruction stack publication at
`block_index=4 instruction_index=1` for `%t17`, with available
cast-rematerialization authority and stale stack-load authority still rejected.
However, coordinate-bearing evidence is required before selecting lowering or
reopening ideas 456/458/459.

## Validation

- Step 2 lifecycle proof: `git diff --check` passed.
- Close-time regression sanity used the rolled-forward backend guard log and
  found no regression.

## Reviewer Reject Signals

- Reject implementation changes inside the residual-owner audit packet.
- Reject broad generic move-bundle support or stack/register move lowering
  without exact event ownership.
- Reject consuming `load_from_stack_slot missing_stack_freshness`.
- Reject classifying the residual from value ids, block indexes, instruction
  indexes, raw BIR shape, filenames, function names, or one prepared dump
  without corroborating route evidence.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
