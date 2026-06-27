# RV64 gcc_torture Post-Contract Umbrella

Status: Open
Type: Umbrella triage and follow-up idea
Parent: `ideas/closed/412_prepared_fact_contract_normalization_analysis.md`
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Re-baseline and classify RV64 gcc_torture backend failures after the prepared
fact contract normalization round, then generate the next implementation ideas
without mixing BIR/prepared producer repairs into MIR/RV64 lowering.

## Why This Exists

Ideas 413 through 418 converted several suspected target-side recovery paths
into typed prepared contracts and fail-closed diagnostics. A fresh RV64
gcc_torture backend scan after that round reported:

- `1467` total cases
- `404` pass
- `1063` fail

The previous accepted scan from 2026-06-27 06:32 reported `421` pass and
`1046` fail. The observed pass-count drop was concentrated in fail-closed
global/object-data contract behavior, while default CTest still passed
`3356/3356`.

The next RV64 work should start from that post-contract shape. It should
separate target-lowering gaps from BIR/prepared producer gaps before creating
implementation slices, so gcc_torture pressure does not push semantic recovery
back into MIR/RV64 fixups.

## In Scope

- Record a current RV64 gcc_torture backend baseline in
  `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`.
- Compare the current scan against the previous accepted scan and record
  pass-to-fail / fail-to-pass deltas in
  `docs/rv64_gcc_torture_post_contract/regression_delta.md`.
- Classify remaining failures in
  `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`, including:
  - RV64 target lowering gaps,
  - prepared fail-closed contract gaps,
  - runtime abort / segfault / mismatch families,
  - compile timeouts and run timeouts,
  - BIR or semantic producer gaps that must not be patched in MIR/RV64.
- Generate follow-up ideas under `ideas/open/` for the next coherent work
  slices, with dependency order and consumed docs rows recorded in
  `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`.
- Prefer follow-up ideas that improve RV64 gcc_torture capability while keeping
  default CTest stable.

## Out Of Scope

- Implementing RV64 fixes inside this umbrella idea.
- Adding RV64 gcc_torture to the default CTest harness.
- Treating RV64 gcc_torture pass count as the default non-regression gate.
- Fixing BIR or prepared producer gaps inside MIR/RV64 lowering.
- Weakening unsupported markers, allowlists, runtime comparison, expected
  output, or default CTest contracts.
- Broad rewrites of the RV64 backend without row-level ownership from the
  docs artifacts.

## Acceptance Criteria

- The handoff directory contains current scan, regression delta, failure bucket,
  and follow-up idea artifacts.
- The umbrella explicitly records that RV64 gcc_torture is external evidence,
  not default harness coverage.
- Default `ctest --test-dir build -j --output-on-failure` / normal CTest does
  not regress.
- Temporary RV64 gcc_torture regressions are accepted only when classified as
  precise fail-closed contract behavior; silent miscompiles or broad runtime
  regressions are not accepted as cleanup.
- Every follow-up implementation idea names its owning layer: RV64/MIR,
  prepared contract, BIR/semantic producer, runtime mismatch, or test
  infrastructure.
- Any BIR/prepared producer gap discovered by this umbrella becomes a separate
  idea that must close before a dependent MIR/RV64 lowering idea consumes that
  fact.

## Reviewer Reject Signals

- Reject testcase-shaped RV64 fixes, named-case shortcuts, fallback name
  recovery, raw BIR shape matching, or local target inference used to bypass a
  missing BIR/prepared fact.
- Reject umbrella output that only lists pass/fail counts without row-level
  ownership and follow-up ideas.
- Reject treating RV64 gcc_torture as part of the default harness or requiring
  monotonic RV64 gcc_torture pass count as the close gate.
- Reject follow-up ideas that mix BIR producer repair and MIR/RV64 lowering in
  one implementation slice.
- Reject expectation rewrites, allowlist filtering, unsupported downgrades, or
  weaker runtime checks as evidence of progress.
- Reject broad RV64 rewrites that do not cite rows from
  `docs/rv64_gcc_torture_post_contract/`.
