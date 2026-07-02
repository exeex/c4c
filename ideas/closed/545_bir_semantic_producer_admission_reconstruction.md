# BIR Semantic Producer Admission Reconstruction

Status: Closed
Type: Current-evidence reconstruction and producer follow-up generator
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: BIR semantic producer

## Goal

Rebuild current row-level evidence for high-impact `semantic lir_to_bir`
admission failures and split them into producer-owned cleanup ideas.

## Why This Exists

The umbrella requires BIR semantic producer admission cleanup, but the current
failure bucket map does not verify a current row count for that family. The
right next action is evidence reconstruction, not an implementation claim based
on stale or missing counts.

## In Scope

- Reproduce or extract current rows whose first failure is BIR semantic
  admission.
- Classify rows by producer topic, including local-memory, call metadata,
  aggregate facts, publication gaps, and malformed or intentionally rejected
  inputs.
- Create narrower producer-owned ideas for high-frequency current families.
- Record rows that are not BIR producer failures and route them back to the
  correct owner.

## Out Of Scope

- RV64 object lowering changes.
- Prepared-module or MIR consumer fixes that bypass missing BIR facts.
- Using historical branch counts as current evidence.
- Weakening semantic admission checks to make rows advance farther.

## Acceptance Criteria

- Current row-level evidence identifies the BIR admission rows or states that
  no verified current row set exists.
- High-frequency producer families are split into coherent follow-up ideas
  with owner, proof shape, and rejection criteria.
- Downstream RV64/MIR work is blocked from consuming missing producer facts
  until the producer idea closes.
- The result preserves default CTest behavior and does not change
  gcc_torture expectations.

## Closure Evidence

Closed after the Step 4 evidence outcome in
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_outcome.md`.

The reconstruction found `373` exact current `semantic lir_to_bir` rows with
visible first-topic evidence:

- `264` local-memory rows.
- `55` call metadata rows.
- `34` runtime/intrinsic rows.
- `20` scalar/signature/control rows.

It also found `44` related bootstrap/global data-shape handoff rows that are
not counted as exact semantic rows.

Follow-up source ideas were created for each routed lane:

- `ideas/open/557_bir_local_memory_semantic_producer_admission.md`
- `ideas/open/558_bir_call_metadata_semantic_producer_admission.md`
- `ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md`
- `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
- `ideas/open/561_bir_bootstrap_global_data_shape_handoff_support.md`

The close-time regression gate used existing `test_before.log` and
`test_after.log` with `--allow-non-decreasing-passed` because this was a
docs/lifecycle-only close. Both logs reported `346` passed, `0` failed, and no
new failures.

## Reviewer Reject Signals

- Reject claiming a BIR admission cleanup from stale `semantic lir_to_bir`
  counts not reproduced on the current scan.
- Reject RV64 or prepared workarounds that infer facts the BIR producer did
  not publish.
- Reject changing admission diagnostics, names, or expectations without
  repairing the underlying semantic producer capability.
- Reject broad producer rewrites that are not tied to reconstructed current
  row families.
- Reject unsupported downgrades or weaker test contracts as progress.
