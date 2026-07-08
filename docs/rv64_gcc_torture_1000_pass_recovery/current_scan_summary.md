# Current RV64 gcc_torture Backend Scan Evidence

Status: current Step 1 evidence baseline for the 1000-pass recovery umbrella.

## Scan Command

The current row evidence is produced by:

```sh
BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

That script builds `c4cll`, walks
`tests/c/external/gcc_torture/allowlist.txt`, runs each case through the RV64
backend-object route, compares against the clang RV64 binary under qemu, and
writes mutable row artifacts under `build/agent_state/`.

## Current Artifacts

- Summary TSV:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- Failed-case list:
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- Per-case work directory:
  `build/rv64_gcc_c_torture_backend/`
- Per-case logs:
  `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- Historical top-level log pointer:
  `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`

The summary TSV and failed-case list both have filesystem mtime
`2026-07-08 11:19:33 +0000`. Representative per-case logs under
`build/rv64_gcc_c_torture_backend/` also have July 8 mtimes, including the
last allowlist rows such as `src_zero-struct-2.c/case.log`.

## Current Counts

The current mutable summary records:

- `1467` total data rows
- `470` passed rows
- `997` failed rows
- `0 missing` rows

The failed-case list has `997` rows, matching the `fail` rows in the summary.
The `0 missing` count is from the current summary statuses: all `1467` data
rows are either `pass` or `fail`; there are no rows with status `missing`.

## Freshness And Supersession

The latest evidence found for this umbrella is the July 8 mutable summary and
failed list at `470/1467`. No newer RV64 gcc_torture backend scan artifact was
found after those July 8 files, so no newer scan supersedes the `470` passed,
`997` failed, `1467` total baseline.

The historical pointer file still names:

```text
build/agent_state/rv64_gcc_torture_backend_current_20260703T050644Z.log
```

That timestamped log reports `total=1467 passed=438 failed=1029`. It is older
than the July 8 mutable row files and should not supersede the current
`470/1467` evidence.

Earlier timestamped logs include:

- `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`:
  `total=1467 passed=349 failed=1118`
- `build/agent_state/rv64_gcc_torture_backend_current_20260703T015523Z.log`:
  `total=1467 passed=425 failed=1042`
- `build/agent_state/rv64_gcc_torture_backend_current_20260703T050644Z.log`:
  `total=1467 passed=438 failed=1029`

These are useful historical movement markers only. The current Step 2
classification should use the July 8 summary, failed list, and per-case logs.

## Relationship To The Older 349/1467 Baseline

The previous post-contract umbrella documented `349/1467` as the stable
2026-07-02 baseline, with `1118` failures and no missing cases. That baseline
is historical context for broad progress from `349` passing rows to `470`
passing rows.

It is not current evidence for this umbrella. The current recovery queue starts
from:

```text
total=1467 passed=470 failed=997 missing=0
```

Step 2 should therefore classify the `997` current failures by first owner and
capability family, not the older `1118` failures from the `349/1467` baseline.
