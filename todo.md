Status: Active
Source Idea Path: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Split Runtime Failures By Exit Class

# Current Packet

## Just Finished

Step 1: Normalize Latest Probe Result inspected the existing
`build/rv64_c_testsuite_probe_latest/summary.tsv` artifact without
regenerating the probe. The artifact is usable for current triage: it has 222
rows total, including 2 allowlist comment rows and 220 real `src/...`
c-testsuite rows.

Artifact provenance:

- Path: `build/rv64_c_testsuite_probe_latest/summary.tsv`
- Size: 9748 bytes
- mtime: `2026-06-22 21:20:36.670848000 +0000`
- SHA-256:
  `e3e7e87637a657d38fd64a90f5d184ce3105c7901c5bacd2412ac082facf2da7`

Normalized status counts, counting only rows whose first column starts with
`src/`:

| Status | Count |
| --- | ---: |
| PASS | 67 |
| EMIT_FAIL | 26 |
| CLANG_FAIL | 4 |
| QEMU_NONZERO | 122 |
| QEMU_TIMEOUT | 1 |
| TOTAL | 220 |

Undefined-main result: 0. A diagnostic search for undefined/main signatures in
`summary.tsv`, `*.clang.out`, and `*.emit.out` found only undefined temporary
labels such as `.Lmain_block_*`, not missing or undefined `main`.

## Suggested Next

Start Step 2 from `plan.md`: split the 122 `QEMU_NONZERO` rows and the single
`QEMU_TIMEOUT` row by observable exit class, preserving representative case
names for each bucket.

## Watchouts

This active idea is triage-only. Do not implement runtime fixes, weaken test
contracts, or turn the full 220-case sweep into mandatory CTest coverage. The
two allowlist comment rows in `summary.tsv` currently show `EMIT_FAIL` but are
not real c-testsuite source cases and must stay excluded from normalized
counts.

## Proof

No build or CTest proof required for this triage-only todo update. Read-only
artifact inspection commands were run against
`build/rv64_c_testsuite_probe_latest/summary.tsv` and related probe diagnostics;
no `test_after.log` was produced because the delegated proof explicitly did not
require build/ctest proof.
