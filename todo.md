Status: Active
Source Idea Path: ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish the Evidence Baseline

# Current Packet

## Just Finished

Completed Step 1: Establish the Evidence Baseline by documenting the current
RV64 gcc_torture backend scan evidence in
`docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`, including
the scan command, artifact paths, `1467` total rows, `470` passed rows, `997`
failed rows, `0 missing` rows, freshness context, and the historical-only
relationship to the older `349/1467` baseline.

## Suggested Next

Begin Step 2 in `plan.md`: classify the `997` current failures by first owner
and capability family in
`docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`.

## Watchouts

- This umbrella is triage and follow-up generation only; do not edit
  implementation, harness, expectation, unsupported-marker, allowlist,
  runtime, timeout, or accounting behavior.
- Use the July 8 mutable summary and failed list as the current `470/1467`
  baseline; no newer scan artifact was found that supersedes it.
- The pointer file still names an older July 3 `438/1467` timestamped log, and
  the `349/1467` post-contract baseline is historical only.

## Proof

Delegated proof passed and was saved to `test_after.log`:

```sh
test -f docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md && rg '1467|470|997|0 missing|349/1467|scripts/check_progress_rv64_gcc_c_torture_backend.sh' docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md
```
