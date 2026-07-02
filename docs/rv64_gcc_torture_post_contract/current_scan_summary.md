# Current RV64 gcc_torture Backend Scan Evidence

Status: stable reset-main/post-cleanup evidence reconciled for close-readiness.

## Existing Artifacts

- `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt` points to
  `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`.
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T032151Z.log`
  records a reset-main/post-cleanup full RV64 backend scan with
  `total=1467 passed=349 failed=1118`.
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
  records a second reset-main/post-cleanup full RV64 backend scan with
  `total=1467 passed=349 failed=1118`.
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` are the mutable
  summary and failed-case files written by
  `scripts/check_progress_rv64_gcc_c_torture_backend.sh`.
- Per-case logs live under
  `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.

## Evidence Decision

The stable current planning anchor is the matching 2026-07-02 pair of full
RV64 gcc_torture backend-object scans:

- `1467` total cases
- `349` pass
- `1118` fail
- `0` pass-to-fail changes between the two 2026-07-02 scans
- `0` fail-to-pass changes between the two 2026-07-02 scans

This agrees with `failure_bucket_map.md` and `followup_idea_plan.md`. The
mutable summary TSV currently has 1467 data rows and the mutable failed-case
file has 1118 rows, so the current mutable artifacts are aligned with the
timestamped 2026-07-02 evidence.

Older scan totals such as `433/1034`, `404/1063`, and `314/1153` are
historical context only. They are useful for understanding how earlier packets
classified rows, but they are not the current queue anchor and must not drive
new follow-up ordering without fresh 2026-07-02 row evidence.

## Exact Command For Row Evidence

Run the full RV64 gcc_torture backend scan and preserve its top-level log with
a timestamp:

```sh
ts="$(date -u +%Y%m%dT%H%M%SZ)"
BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh \
  >"build/agent_state/rv64_gcc_torture_backend_current_${ts}.log" 2>&1 || true
printf '%s\n' "build/agent_state/rv64_gcc_torture_backend_current_${ts}.log" \
  > build/agent_state/rv64_gcc_torture_backend_current_log_path.txt
```

Then derive row-level diagnostic buckets from the matching summary and per-case
logs. For example, the historical Step 2 instruction-fragment extraction used:

```sh
awk -F '\t' 'NR > 1 && $1 == "fail" { print $2 "\t" $3 }' \
  build/agent_state/rv64_gcc_c_torture_backend_summary.tsv |
while IFS="$(printf '\t')" read -r case log; do
  if rg -q 'unsupported_instruction_fragment' "$log"; then
    printf '%s\t%s\n' "$case" "$log"
  fi
done > build/agent_state/unsupported_instruction_fragment_rows.tsv
```

The current `failure_bucket_map.md` is the authoritative Step 4 bucket summary
for the 2026-07-02 row set. It directly counts the current `1118` failures and
routes the 567 rows without explicit current ownership evidence as evidence
gaps rather than implementation-ready work.

## Historical Step 2 Classification Inputs

Step 2 also produced local prepared-BIR inspection dumps under
`build/agent_state/421_step2_prepared/` for the 190 regenerated rows. Those
dumps are derived artifacts used only to classify the row TSV by opcode, type,
prepared fact surface, and likely first owner.

The Step 2 row artifact was:

- `build/agent_state/unsupported_instruction_fragment_rows.tsv`

It contained 190 rows from the 2026-06-30 scan and used each case log as
row-level evidence for this diagnostic text:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

That table remains historical support only. Current instruction-fragment work
must rerun row-level classification against the 137 current
`unsupported_instruction_fragment` rows before turning old sub-bucket counts
into implementation packets.

## Missing Or Stale Inputs

- The current summary TSV, failed-case list, per-case logs, and pointer file
  correspond to
  `rv64_gcc_torture_backend_current_20260702T151551Z.log`, which matches the
  earlier same-day stable scan
  `rv64_gcc_torture_backend_current_20260702T032151Z.log`.
- The current summary TSV records only `status`, `case`, and `log`. It does
  not contain durable row-level first-owner annotations.
- Historical 2026-06-30 and 2026-07-01 counts must be cited as historical
  support only. Current follow-up work should cite the timestamped 2026-07-02
  scan logs and the Step 4 bucket map together.
- RV64 gcc_torture remains external evidence. It is not default CTest coverage
  and must not be converted into an expectation-downgrade or unsupported-marker
  weakening route.
