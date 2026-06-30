# RV64 gcc_torture Post-Contract Replan Umbrella

Status: Open
Type: Umbrella postmortem, triage, and follow-up idea generator
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`
Reference Branch: `try_gcc_torture`

## Goal

Use the preserved `try_gcc_torture` branch as a failed exploratory run, extract
the useful lessons, and generate a better ordered RV64 gcc_torture plan that
prioritizes high-frequency ordinary C coverage over low-priority F128 work.

## Why This Exists

After the prepared fact contract normalization round, the RV64 gcc_torture
backend scan was:

- `1467` total cases
- `404` pass
- `1063` fail

The exploratory `try_gcc_torture` branch then spent hundreds of commits chasing
the focused `conversion.c`/F128 route. It did discover useful implementation
facts, but a later full scan on that branch showed:

- `1467` total cases
- `324` pass
- `1143` fail

That result is worse than the starting post-contract baseline and shows the
route overfit the investigation budget toward a niche F128 feature. F128 support
should be treated as the lowest-priority RV64 feature family unless it is
strictly needed as isolated external soft-float ABI glue.

The new umbrella must therefore do more than classify current failures. It must
review what the `try_gcc_torture` branch taught us, identify which changes are
worth redoing, identify which route choices were traps, and create follow-up
ideas in an order that improves broad RV64 coverage without letting BIR or
prepared producer gaps leak into MIR/RV64 fixups.

## In Scope

- Record a fresh RV64 gcc_torture backend baseline on reset `main` in
  `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`.
- Compare reset `main` with the preserved `try_gcc_torture` scan and record
  pass/fail deltas in
  `docs/rv64_gcc_torture_post_contract/regression_delta.md`.
- Write a `try_gcc_torture` postmortem in
  `docs/rv64_gcc_torture_post_contract/try_gcc_torture_postmortem.md` that
  answers:
  - which broad capabilities improved,
  - which changes regressed broad RV64 gcc_torture pass count,
  - where the route was pulled into `conversion.c`/F128 local optimization,
  - which F128 changes should be quarantined or rewritten,
  - which scalar/FPR or generic RV64 changes are worth redoing.
- Classify the remaining failures by first owning layer in
  `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`, with priority
  based on broad case count and ordinary C usefulness, not novelty.
- Generate follow-up ideas under `ideas/open/` and record their order in
  `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`.
- Explicitly rank F128 as lowest priority. It may only appear as a quarantine,
  feature-gate, or external soft-float ABI glue follow-up, not as the main
  route.

## Out Of Scope

- Implementing RV64 fixes inside this umbrella idea.
- Replaying the `try_gcc_torture` branch wholesale.
- Continuing the `conversion.c` F128 route as the primary KPI.
- Treating RV64 gcc_torture pass count as the default CTest non-regression
  gate.
- Adding RV64 gcc_torture to the default harness.
- Fixing BIR or prepared producer gaps inside MIR/RV64 lowering.
- Weakening unsupported markers, allowlists, expected output, runtime
  comparison, pass/fail accounting, or default CTest contracts.

## Priority Model

Follow-up ideas must be ordered by expected broad RV64 gcc_torture impact and
semantic importance:

1. Generic RV64 `unsupported_instruction_fragment` bucket classification and
   high-frequency non-F128 lowering.
2. BIR semantic producer gaps, especially local-memory load/store/GEP,
   call-argument metadata, and memcpy/memset families.
3. Runtime mismatches where the RV64 object route already emits and links code
   but produces aborts, segfaults, or wrong output.
4. Prepared/global-data and stack-frame infrastructure.
5. Selectively redo useful scalar/FPR work from `try_gcc_torture`, such as
   F32/F64 casts, scalar FPR binary ops, scalar floating select, and FPR
   local-store/reload, only after bucket evidence shows broad value.
6. F128 quarantine or external soft-float ABI glue. F128 is lowest priority and
   must not drive the umbrella.

## Required Follow-Up Ideas

This umbrella should produce, at minimum, these follow-up ideas unless fresh
evidence proves a better split:

- RV64 instruction-fragment bucket classification, excluding F128 as a primary
  route.
- BIR semantic producer local-memory and call metadata cleanup.
- RV64 runtime mismatch triage for existing abort/segfault families.
- Prepared global-data and stack-frame infrastructure review.
- Scalar/FPR salvage plan from `try_gcc_torture`.
- F128 quarantine and external soft-float ABI policy, explicitly marked lowest
  priority.

## Acceptance Criteria

- The handoff directory contains current scan, regression delta, failure bucket,
  `try_gcc_torture` postmortem, and follow-up idea plan artifacts.
- The umbrella explicitly records that RV64 gcc_torture is external evidence,
  not default harness coverage.
- The follow-up plan explains why F128 is not the main route and why
  `conversion.c` should not be used as the umbrella KPI.
- Every generated follow-up idea names its owning layer: RV64/MIR,
  BIR/semantic producer, prepared contract, runtime mismatch, test
  infrastructure, or F128 quarantine.
- Any BIR/prepared producer gap discovered by this umbrella becomes a separate
  idea that must close before dependent MIR/RV64 lowering consumes that fact.
- Default `ctest --test-dir build -j --output-on-failure` must not regress for
  lifecycle close.

## Reviewer Reject Signals

- Reject continuing the `conversion.c`/F128 route as the main umbrella output.
- Reject treating F128 as high or medium priority unless a fresh failure bucket
  proves it blocks broad non-F128 coverage.
- Reject wholesale cherry-picking from `try_gcc_torture` without bucket
  evidence, default CTest proof, and route-quality review.
- Reject umbrella output that only lists pass/fail counts without row-level
  ownership, postmortem lessons, and follow-up ideas.
- Reject follow-up ideas that mix BIR producer repair and MIR/RV64 lowering in
  one implementation slice.
- Reject testcase-shaped RV64 fixes, named-case shortcuts, fallback name
  recovery, raw BIR shape matching, or local target inference used to bypass a
  missing BIR/prepared fact.
- Reject expectation rewrites, allowlist filtering, unsupported downgrades, or
  weaker runtime checks as evidence of progress.
