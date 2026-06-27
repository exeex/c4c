# RV64 gcc torture post-contract artifacts

This directory is the shared analysis workspace for RV64 gcc_torture work after
the prepared fact contract normalization round.

The umbrella idea should write durable triage and handoff artifacts here.
Closure notes may summarize the result, but cross-idea handoff data belongs in
these files so later RV64, prepared, or BIR ideas consume explicit bucket rows
instead of mining closed idea text.

## Expected Artifacts

- `current_scan_summary.md`
  - Records the exact RV64 gcc_torture backend scan used as the umbrella
    baseline, including total/pass/fail counts, scan command, timestamp, and
    comparison against the previous accepted scan.
- `failure_bucket_map.md`
  - Classifies remaining failures into target lowering gaps, prepared
    fail-closed contract gaps, runtime mismatch families, timeouts, and BIR or
    semantic producer gaps.
  - Each row should name representative cases and the owning layer.
- `regression_delta.md`
  - Records pass-to-fail and fail-to-pass deltas caused by the prepared
    contract round, with special attention to fail-closed contract regressions.
- `followup_idea_plan.md`
  - Lists the follow-up ideas generated from the umbrella, their owning layer,
    dependency order, and the docs rows they consume.

## Handoff Rules

- RV64 gcc_torture is not part of the default CTest harness. It may be used as
  external evidence, but default `ctest --test-dir build -j --output-on-failure`
  must not regress.
- Temporary RV64 gcc_torture pass-count regression is acceptable only when it
  is a precise fail-closed contract classification or an explicitly accepted
  external-test tradeoff.
- If a row identifies a BIR or prepared producer gap, create a separate BIR or
  prepared idea and close that idea before returning to MIR/RV64 lowering for
  that row.
- Do not fix BIR-owned facts by adding MIR/RV64 inference, fallback name
  recovery, raw BIR shape matching, or named testcase shortcuts.
- Later ideas should cite the artifact rows they consume. If a later idea
  disagrees with an earlier row, record the correction in the later artifact
  and propose a follow-up idea to reconcile the source artifact.
