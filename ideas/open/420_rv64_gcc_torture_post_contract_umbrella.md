# RV64 gcc_torture Current-Main Replan Umbrella

Status: Open
Type: Umbrella triage and follow-up idea generator
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Use current reset-main RV64 gcc_torture evidence to generate an ordered
follow-up plan that prioritizes high-frequency ordinary C coverage over
low-priority F128 work.

## Why This Exists

After the prepared fact contract normalization round and the BIR/RV64 cleanup
pass, the project needs a current-main RV64 gcc_torture recovery queue instead
of more local testcase chasing. F128 support should be treated as the
lowest-priority RV64 feature family unless it is strictly needed as isolated
external soft-float ABI glue. Fresh F128 failures should be classified into the
F128 quarantine path first, not used to steer the ordinary-C RV64 recovery
route. When a gcc_torture testcase is primarily F128, the preferred immediate
action is to screen it out of the main progress bucket with an explicit
quarantine/unsupported classification rather than spending implementation
budget on it.

The umbrella must classify current failures and create follow-up ideas in an
order that improves broad RV64 coverage without letting BIR or prepared
producer gaps leak into MIR/RV64 fixups.

## Current Evidence Update

After the BIR and RV64 file-ownership cleanup pass, two full RV64 gcc_torture
backend-object scans on 2026-07-02 produced the same result and the same
pass/fail case set:

- `1467` total cases
- `349` pass
- `1118` fail
- `0` pass-to-fail changes between
  `build/agent_state/rv64_gcc_torture_backend_current_20260702T032151Z.log`
  and
  `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- `0` fail-to-pass changes between those two scans

This stable post-cleanup scan should supersede older stale handoff counts such
as the 2026-06-30 `404/1063` summary and the later stale `314/1153` merged
summary when choosing the next follow-up queue. The umbrella should refresh the
handoff docs so all current-scan, failure-bucket, and follow-up-plan artifacts
cite one coherent scan timestamp and matching mutable summary files.

The largest current explicit prepared/module-shape bucket is
`unsupported_move_bundle_target_shape` with 183 rows in the stable 2026-07-02
scan. That bucket is the highest expected-value ordinary-C follow-up candidate
after the umbrella finishes evidence refresh and idea materialization.

## In Scope

- Record a fresh RV64 gcc_torture backend baseline on reset `main` in
  `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`.
- Replace stale or conflicting handoff counts with the stable 2026-07-02
  `349/1118` full-scan evidence and its timestamped log path.
- Classify the remaining failures by first owning layer in
  `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`, with priority
  based on broad case count and ordinary C usefulness, not novelty.
- Generate follow-up ideas under `ideas/open/` and record their order in
  `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`.
- Explicitly rank F128 as lowest priority. It may only appear as a quarantine,
  feature-gate, or external soft-float ABI glue follow-up, not as the main
  route.
- Treat primary-F128 testcase rows as quarantine candidates first. They should
  be screened out of ordinary-C progress accounting with explicit F128
  classification unless fresh evidence proves the row also blocks a broad
  non-F128 capability.

## Out Of Scope

- Implementing RV64 fixes inside this umbrella idea.
- Continuing the `conversion.c` F128 route as the primary KPI.
- Letting primary-F128 testcase rows remain in the ordinary-C repair queue when
  they can be explicitly quarantined.
- Treating RV64 gcc_torture pass count as the default CTest non-regression
  gate.
- Adding RV64 gcc_torture to the default harness.
- Fixing BIR or prepared producer gaps inside MIR/RV64 lowering.
- Weakening unsupported markers, allowlists, expected output, runtime
  comparison, pass/fail accounting, or default CTest contracts.

## Priority Model

Follow-up ideas must be ordered by expected broad RV64 gcc_torture impact and
semantic importance:

1. Prepared move-bundle materialization and authority gaps when fresh bucket
   evidence shows `unsupported_move_bundle_target_shape` is the largest
   ordinary-C failure owner. Split coherent prepared facts that RV64 can
   lower from producer/authority gaps that require prepared/BIR follow-up.
   Under the stable 2026-07-02 scan this is the first implementation-planning
   target, with 183 current rows.
2. BIR semantic producer gaps, especially local-memory load/store/GEP,
   call-argument metadata, and memcpy/memset families.
3. Generic RV64 `unsupported_instruction_fragment` bucket classification and
   high-frequency non-F128 lowering.
4. Runtime mismatches where the RV64 object route already emits and links code
   but produces aborts, segfaults, or wrong output.
5. Prepared/global-data and stack-frame infrastructure.
6. Scalar/FPR work only when current bucket evidence shows broad value.
7. F128 quarantine or external soft-float ABI glue. F128 is lowest priority and
   must not drive the umbrella. Primary-F128 testcase failures should normally
   be filtered into this quarantine lane before ordinary-C repair work is
   selected.

## Required Follow-Up Ideas

This umbrella should produce, at minimum, these follow-up ideas unless fresh
evidence proves a better split:

- Prepared/RV64 move-bundle materialization bucket review when the fresh scan
  shows it dominates ordinary-C failures.
- A first concrete follow-up that consumes the stable 2026-07-02
  `unsupported_move_bundle_target_shape` evidence, splits its 183 rows by first
  owner, and decides which rows are coherent RV64 consume work versus
  prepared/BIR authority work.
- BIR semantic producer admission cleanup for high-frequency `semantic
  lir_to_bir` failures.
- RV64 instruction-fragment bucket classification, excluding F128 as a primary
  route.
- BIR semantic producer local-memory and call metadata cleanup.
- RV64 runtime mismatch triage for existing abort/segfault families.
- Prepared global-data and stack-frame infrastructure review.
- Scalar/FPR current-bucket salvage plan only if current evidence shows broad
  non-F128 value.
- F128 quarantine and external soft-float ABI policy, explicitly marked lowest
  priority.

## Acceptance Criteria

- The handoff directory contains current scan, failure bucket, and follow-up
  idea plan artifacts.
- The current scan, failure bucket, and follow-up plan artifacts agree on the
  same stable post-cleanup scan timestamp, total/pass/fail counts, and summary
  files.
- The umbrella explicitly records that RV64 gcc_torture is external evidence,
  not default harness coverage.
- The follow-up plan explains why F128 is not the main route and why
  `conversion.c` should not be used as the umbrella KPI.
- The follow-up plan records how primary-F128 testcase rows are quarantined or
  screened from ordinary-C progress accounting.
- Every generated follow-up idea names its owning layer: RV64/MIR,
  BIR/semantic producer, prepared contract, runtime mismatch, test
  infrastructure, or F128 quarantine.
- Any BIR/prepared producer gap discovered by this umbrella becomes a separate
  idea that must close before dependent MIR/RV64 lowering consumes that fact.
- Any active MIR/RV64 idea that discovers a BIR/prepared producer gap must stop
  that route, create or select the BIR/prepared idea, switch lifecycle state to
  that producer idea, and only return to the MIR/RV64 idea after the producer
  idea closes with proof.
- Default `ctest --test-dir build -j --output-on-failure` must not regress for
  lifecycle close.

## Reviewer Reject Signals

- Reject continuing the `conversion.c`/F128 route as the main umbrella output.
- Reject treating F128 as high or medium priority unless a fresh failure bucket
  proves it blocks broad non-F128 coverage.
- Reject allowing primary-F128 testcase rows to drive ordinary-C RV64 repair
  selection when they can be explicitly quarantined or marked unsupported under
  the F128 policy.
- Reject umbrella output that only lists pass/fail counts without row-level
  ownership and follow-up ideas.
- Reject leaving stale conflicting scan summaries in the handoff docs after a
  stable newer full scan exists.
- Reject follow-up ideas that mix BIR producer repair and MIR/RV64 lowering in
  one implementation slice.
- Reject continuing a MIR/RV64 implementation after it discovers a missing
  BIR/prepared producer fact instead of switching to a producer-owned idea.
- Reject testcase-shaped RV64 fixes, named-case shortcuts, fallback name
  recovery, raw BIR shape matching, or local target inference used to bypass a
  missing BIR/prepared fact.
- Reject expectation rewrites, allowlist filtering, unsupported downgrades, or
  weaker runtime checks as evidence of progress.
