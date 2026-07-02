# RV64 gcc_torture Follow-Up Idea Plan

Status: Step 5 refreshed from stable 2026-07-02 evidence.

## Evidence Anchor

This plan uses the stable 2026-07-02 reset-main/post-cleanup RV64
gcc_torture backend-object evidence recorded by the source umbrella and the
current failure bucket map:

- `1467` total cases
- `349` pass
- `1118` fail
- `0` pass-to-fail changes between the two 2026-07-02 scans
- `0` fail-to-pass changes between the two 2026-07-02 scans
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T032151Z.log`
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`

Older `404/1063` and `314/1153` summaries are historical only. They are not
the current queue anchor, and old missing idea files are not treated as live
follow-up state.

RV64 gcc_torture remains external evidence, not default CTest coverage. F128
is quarantined through
`ideas/open/426_f128_quarantine_and_external_softfloat_policy.md` and must not
drive ordinary-C RV64 recovery.

## Ordering Rule

Order ideas by current ordinary-C impact, first-owner clarity, and route
quality:

- start with the largest explicit current ordinary-C bucket;
- split producer/authority gaps from MIR/RV64 lowering before implementation;
- create classification or reconstruction ideas when current row-level
  evidence is missing;
- keep runtime/crash/no-diagnostic rows out of implementation queues until
  they have reproducible first-owner evidence;
- keep primary-F128 rows in the F128 quarantine lane unless fresh evidence
  proves broad non-F128 impact.

## Ordered Follow-Up Queue

| Rank | Idea | Current evidence | Owner | Route decision |
| ---: | --- | --- | --- | --- |
| 1 | `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md` | 183 current `unsupported_move_bundle_target_shape` rows | Prepared contract plus RV64/MIR boundary | First queue item. Split coherent RV64 move materialization from prepared/BIR authority gaps before any lowering work. |
| 2 | `ideas/open/545_bir_semantic_producer_admission_reconstruction.md` | Required by source idea; no verified current row count in the Step 4 map | BIR semantic producer | Classification/reconstruction lane. Rebuild current row evidence before creating producer implementation slices. |
| 3 | `ideas/open/546_rv64_instruction_fragment_current_classification.md` | 137 current `unsupported_instruction_fragment` rows | RV64/MIR object lowering, with F128 screened out | Reclassify current rows before reusing older instruction-fragment taxonomy or creating implementation packets. |
| 4 | `ideas/open/547_bir_local_memory_call_metadata_boundary_review.md` | 44 current `unsupported_local_memory_access` rows plus source-required call metadata lane without verified count | BIR semantic producer and prepared contract | Review producer authority and call/local-memory facts before RV64 memory/call lowering consumes them. |
| 5 | `ideas/open/548_prepared_global_stack_frame_infrastructure_review.md` | 85 `unsupported_stack_frame`, 43 `unsupported_global_data`, 26 `unsupported_prepared_move_bundle_classification` rows | Prepared contract and RV64 infrastructure boundary | Infrastructure review for global data, stack frame, and prepared move-bundle classification facts. |
| 6 | `ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md` | 503 compile failures without explicit `unsupported_*`, 57 other non-unsupported failures, 7 segmentation-fault exits | Runtime mismatch, crash triage, or evidence reconstruction | Reproduce and classify before claiming ordinary-C capability progress. |
| 7 | `ideas/open/550_rv64_scalar_fpr_residual_salvage.md` | 3 scalar-compare, 2 floating-cast, 1 variadic-helper rows | RV64 scalar/FPR/helper boundary | Low-volume ordinary-C salvage lane after higher-count buckets. Keep separate from F128. |
| 8 | `ideas/open/426_f128_quarantine_and_external_softfloat_policy.md` | Existing policy lane; primary-F128 rows are lowest priority | F128 quarantine and external soft-float policy | Screen primary-F128 rows away from ordinary-C accounting. Do not duplicate this idea. |

## Dependency Notes

- The move-bundle split idea is the first concrete bucket review because the
  source umbrella records `unsupported_move_bundle_target_shape` as the largest
  explicit current ordinary-C bucket.
- Rows that need missing BIR or prepared facts must move to producer-owned
  ideas before dependent MIR/RV64 lowering resumes.
- The BIR semantic producer admission lane is deliberately a reconstruction
  idea because the current Step 4 map does not verify a row count for that
  family.
- The instruction-fragment lane must refresh the current 137-row table before
  any older `unsupported_instruction_fragment` sub-bucket counts are reused.
- Runtime, crash, timeout, and no-diagnostic rows must not be counted as
  implementation-ready ordinary-C progress until targeted reproduction assigns
  first ownership.

## Reviewer Reject Signals For This Plan

- Reject treating stale `404/1063`, `314/1153`, or missing old idea files as
  the current queue.
- Reject creating implementation ideas from unverified row counts.
- Reject any MIR/RV64 idea that continues after discovering a missing
  BIR/prepared producer fact instead of switching to a producer-owned idea.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  weaker runtime comparison as evidence of RV64 progress.
- Reject F128 promotion above ordinary-C buckets unless fresh row evidence
  proves broad non-F128 impact.
