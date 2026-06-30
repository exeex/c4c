# Current RV64 gcc_torture Backend Scan Evidence

Status: Step 2 row evidence regenerated and classified.

## Existing Artifacts

- `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt` points to
  `build/agent_state/rv64_gcc_torture_backend_current_20260630T032216Z.log`.
- `build/agent_state/rv64_gcc_torture_backend_current_20260630T032216Z.log`
  records the Step 2 full RV64 backend scan with
  `total=1467 passed=404 failed=1063`.
- `build/agent_state/423_step2_rv64_backend_refresh.log` records an earlier
  full RV64 backend refresh with `total=1467 passed=433 failed=1034`.
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` are the mutable
  summary and failed-case files written by
  `scripts/check_progress_rv64_gcc_c_torture_backend.sh`.
- Per-case logs live under
  `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.

## Evidence Decision

The handoff directory does not contain a checked-in durable row-level artifact
for `unsupported_instruction_fragment`. The local build tree has enough
information to extract rows, but the summary files are mutable and were stale
before Step 2 because earlier named scan logs recorded different totals.

Therefore, Step 2 regenerated the row TSV from one matching scan run instead
of classifying from the earlier mutable summary files alone.

## Exact Command For Row Evidence

Run the full RV64 gcc_torture backend scan, preserve its top-level log with a
timestamp, then extract rows whose per-case logs contain the target bucket:

```sh
ts="$(date -u +%Y%m%dT%H%M%SZ)"
BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh \
  >"build/agent_state/rv64_gcc_torture_backend_current_${ts}.log" 2>&1 || true
printf '%s\n' "build/agent_state/rv64_gcc_torture_backend_current_${ts}.log" \
  > build/agent_state/rv64_gcc_torture_backend_current_log_path.txt
awk -F '\t' 'NR > 1 && $1 == "fail" { print $2 "\t" $3 }' \
  build/agent_state/rv64_gcc_c_torture_backend_summary.tsv |
while IFS="$(printf '\t')" read -r case log; do
  if rg -q 'unsupported_instruction_fragment' "$log"; then
    printf '%s\t%s\n' "$case" "$log"
  fi
done > build/agent_state/unsupported_instruction_fragment_rows.tsv
```

The resulting Step 2 row artifact is:

- `build/agent_state/unsupported_instruction_fragment_rows.tsv`

It contains 190 rows from the 2026-06-30 scan. Each row is
`case<TAB>case-log-path`. The case log is the row-level evidence for the
diagnostic text:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

## Step 2 Classification Inputs

Step 2 also produced local prepared-BIR inspection dumps under
`build/agent_state/421_step2_prepared/` for the 190 regenerated rows. Those
dumps are derived artifacts used only to classify the row TSV by opcode, type,
prepared fact surface, and likely first owner.

## Missing Or Stale Inputs

- The row TSV, mutable summary TSV, failed-case list, per-case logs, and
  pointer file now all correspond to
  `rv64_gcc_torture_backend_current_20260630T032216Z.log`.
- Prior full scan logs record different pass/fail totals, so bucket counts
  should always cite this timestamped scan log and matching row TSV together.
