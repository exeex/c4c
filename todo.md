Status: Active
Source Idea Path: ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Generate Ordered Follow-Up Ideas

# Current Packet

## Just Finished

Step 5 refreshed
`docs/rv64_gcc_torture_post_contract/followup_idea_plan.md` against the stable
2026-07-02 reset-main/post-cleanup RV64 gcc_torture backend-object evidence and
the Step 4 failure bucket map.

Created current-evidence follow-up ideas:

- `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
- `ideas/open/545_bir_semantic_producer_admission_reconstruction.md`
- `ideas/open/546_rv64_instruction_fragment_current_classification.md`
- `ideas/open/547_bir_local_memory_call_metadata_boundary_review.md`
- `ideas/open/548_prepared_global_stack_frame_infrastructure_review.md`
- `ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md`
- `ideas/open/550_rv64_scalar_fpr_residual_salvage.md`

The follow-up plan no longer treats stale `314/1153`, `404/1063`, or missing
old idea files as the current queue. It orders the 183-row
`unsupported_move_bundle_target_shape` bucket first, makes missing row-level
evidence into classification/reconstruction ideas instead of implementation
claims, and keeps F128 quarantined through
`ideas/open/426_f128_quarantine_and_external_softfloat_policy.md`.

## Suggested Next

Execute Step 6 by reviewing close readiness for the umbrella. Confirm the
handoff docs and generated ideas satisfy the source idea, verify no
implementation patches or expectation downgrades entered the umbrella, and
record the close-gate expectation for supervisor/plan-owner review.

## Watchouts

- `current_scan_summary.md` in this checkout still contains older Step 2 text;
  the Step 5 plan used the source umbrella and `failure_bucket_map.md` as the
  authoritative stable 2026-07-02 evidence for this packet.
- `regression_delta.md` and `try_gcc_torture_postmortem.md` were not present in
  this checkout during Step 5, despite earlier handoff notifications.
- The current bucket map has verified current diagnostic counts, but it does
  not have a refreshed row-level first-owner table for the 567 current failures
  that lack explicit `unsupported_*` ownership evidence.
- Do not implement RV64 fixes in this umbrella.
- Keep RV64 gcc_torture as external evidence, not a default CTest gate.
- Keep primary-F128 rows screened into the F128 quarantine lane unless fresh
  evidence proves broad non-F128 impact.
- Do not weaken unsupported markers, allowlists, expected output, runtime
  comparison, pass/fail accounting, or default CTest contracts.
- Keep `ideas/open/426_f128_quarantine_and_external_softfloat_policy.md` as
  the existing low-priority F128 policy lane.
- Leave `review/global_address_helper_cleanup_review.md` untouched.
- Treat `test_baseline.new.log` as a rejected full-suite candidate, not an
  accepted baseline.

## Proof

Ran `git diff --check --
docs/rv64_gcc_torture_post_contract/followup_idea_plan.md todo.md
ideas/open/*.md`; proof output was captured in `test_after.log`.
