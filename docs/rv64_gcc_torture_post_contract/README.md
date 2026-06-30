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
- `try_gcc_torture_postmortem.md`
  - Reviews the preserved `try_gcc_torture` branch as an exploratory run.
  - Records which changes are worth rewriting, which route choices were traps,
    and why F128 must be treated as lowest-priority quarantine work instead of
    the main RV64 gcc_torture route.
- `regression_delta.md`
  - Records pass-to-fail and fail-to-pass deltas caused by the prepared
    contract round and by comparison with `try_gcc_torture`, with special
    attention to fail-closed contract regressions and broad pass-count drops.
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
- If an active MIR/RV64 route discovers such a producer gap, stop that route
  and switch lifecycle state to the BIR/prepared idea. Do not keep the
  MIR/RV64 idea active while adding inference or fixups for the missing fact.
- Do not fix BIR-owned facts by adding MIR/RV64 inference, fallback name
  recovery, raw BIR shape matching, or named testcase shortcuts.
- Do not let F128 or `conversion.c` drive the postmortem umbrella. F128 is
  lowest priority unless fresh bucket evidence proves it blocks broad non-F128
  coverage; eventual F128 support should be external soft-float ABI glue, not
  compiler-internal softfloat implementation.
- Later ideas should cite the artifact rows they consume. If a later idea
  disagrees with an earlier row, record the correction in the later artifact
  and propose a follow-up idea to reconcile the source artifact.
