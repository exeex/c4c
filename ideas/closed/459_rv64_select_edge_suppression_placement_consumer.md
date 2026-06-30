# RV64 Select-Edge Suppression Placement Consumer

Status: Closed
Type: RV64 prepared consumer idea
Parent: `ideas/closed/458_select_edge_source_producer_move_bundle_placement_authority.md`
Source Evidence: `build/agent_state/458_step4_residual_disposition/`
Close Evidence: `build/agent_state/459_step4_residual_disposition/disposition.md`
Owning Layer: RV64 before-instruction move-bundle suppression from explicit
select-edge source-producer placement authority
Closed Disposition: Complete for explicit
`predecessor_edge_consumed_suppression` consumption. Follow-up:
`ideas/open/460_rv64_move_bundle_residual_owner_audit.md`.

## Goal

Teach RV64 object emission to consume explicit
`predecessor_edge_consumed_suppression` placement metadata for select-edge
binary source-producer before-instruction register-destination bundles, without
adding generic stack/register move support or inferring placement from raw
shape.

## Why This Exists

Idea 458 produced the missing placement authority for the `20010329-1`
`block_index=4 instruction_index=2` target bundle. The metadata says the
source-producer bundle is consumed by predecessor-edge publication and should
be suppressible at the join instruction. RV64 still needs a separate consumer
packet to honor that explicit record while keeping generic move bundles and
stale stack-load paths fail-closed.

## In Scope

- Audit the Step 3/4 placement metadata and the target before-instruction
  register-destination bundle.
- Define the RV64 consumer conditions for
  `predecessor_edge_consumed_suppression`, including source producer,
  predecessor/successor edge identity, select carrier, target bundle site,
  move identities, and destination register safety.
- Suppress only bundles authorized by explicit placement metadata.
- Add focused RV64 object coverage for accepted suppression and fail-closed
  missing, mismatched, unsupported, generic, stale stack-load, and raw-inferred
  cases.
- Re-probe `20010329-1` and classify any remaining residuals.

## Out Of Scope

- Producing placement metadata, closed by idea 458.
- Generic stack-to-register or register-to-register move lowering.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening explicit cast-dependency authority consumption closed by idea 456.
- Generic stack-home branch operand/condition materialization, pointer-value
  provenance, local/global store publication, and generic instruction-side
  lowering.
- Inferring suppression from raw BIR shape, value ids, block indexes,
  instruction indexes, filenames, function names, or one prepared dump.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- RV64 suppresses only before-instruction register-destination bundles with
  explicit `predecessor_edge_consumed_suppression` placement metadata.
- Missing, mismatched, unsupported, generic, stale stack-load, raw-inferred, or
  unrelated bundles remain fail-closed with focused tests.
- `20010329-1` no longer fails on the target placement-consumer owner, or the
  next first owner is classified with evidence.
- No expectation-only or generic move-support change is claimed as progress.

## Completion Notes

Idea 459 completed the explicit RV64 consumer for
`predecessor_edge_consumed_suppression` placement metadata. Step 3 added record
collection, placement-record/move-bundle matching, narrow traversal visibility
for available suppression records, and RV64 empty-fragment suppression.
Focused RV64 tests cover accepted and mismatched cases.

Fresh `20010329-1` probing still fails with
`unsupported_move_bundle_target_shape`, but Step 4 found no exact remaining
459 suppression-consumer packet. Existing diagnostics do not identify the
exact event coordinate; read-only debugger probing showed multiple
move-bundle fragments before failure. The remaining owner is a separate
residual-owner audit for RV64 move-bundle materialization/suppression outside
explicit `predecessor_edge_consumed_suppression`.

Stale `load_from_stack_slot missing_stack_freshness`, generic register/stack
move support, raw-shape inference, and expectation rewrites remain outside this
idea.

## Validation

- Step 3 backend proof passed before log roll-forward:
  `cmake --build build -j2` plus
  `ctest --test-dir build -j2 --output-on-failure -R '^backend_'`.
- Step 4 lifecycle proof: `git diff --check` passed.
- Close-time regression sanity used the rolled-forward backend guard log and
  found no regression.

## Reviewer Reject Signals

- Reject RV64 suppression based on value ids, block indexes, instruction
  indexes, raw BIR shape, function names, filenames, or one prepared dump.
- Reject broad generic move-bundle support presented as placement consumer
  progress.
- Reject consuming `load_from_stack_slot missing_stack_freshness`.
- Reject weakening idea 456 cast-dependency guards or idea 458 false-by-default
  placement semantics.
- Reject testcase-shaped fixes keyed to `20010329-1`, `logic.rhs.end.13`,
  `logic.end.14`, `%t18`, `%t22`, value ids 7/9/10, or one prepared dump.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
