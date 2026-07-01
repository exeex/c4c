# RV64 gcc_torture Runtime Mismatch Inventory

Status: Step 1 runtime divergence inventory complete from the freshest
full-scan artifacts.

## Artifact Provenance

This inventory uses the freshest RV64 gcc_torture backend full-scan artifacts
observed under `build/agent_state/`:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`
  - mtime: `2026-07-01 06:44:22.930115007 +0000`
  - shape: `# status<TAB>case<TAB>log`
  - row totals: `1467` cases, `314` pass rows, `1153` fail rows
- `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt`
  - mtime: `2026-07-01 06:44:42.242877002 +0000`
  - shape: one failed case ID per line
  - row total: `1153` failed case IDs
- Per-case logs referenced by the TSV live under
  `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.

No matching timestamped top-level `.full.log` was present next to these
artifacts. The row-level source of truth for runtime divergence is therefore
the `.full.tsv` row's `log` path plus the per-case marker described below.

## Extraction Criteria

Rows were included only when all of the following were true:

1. The TSV row status is `fail`.
2. The row's per-case log contains `[RV64_BACKEND_RUNTIME_MISMATCH]`.
3. The per-case log reached the runner's runtime comparison stage, meaning
   c4c object generation, clang reference compilation, c4c object linking, and
   both qemu runs completed far enough for the runner to compare exits and
   output streams.

Rows were explicitly excluded when the per-case log failed before runtime
comparison, including unsupported compile/codegen contract rows such as
`unsupported_move_bundle_target_shape`, `unsupported_instruction_fragment`,
`unsupported_stack_frame`, `unsupported_global_data`,
`unsupported_terminator_fragment`, `unsupported_local_memory_access`,
`unsupported_param_home`, `unsupported_floating_cast`, semantic
`lir_to_bir` admission failures, clang build failures, link failures, and
timeouts.

Reproduction of the inventory extraction:

```sh
awk -F '\t' 'NR > 1 && $1 == "fail" { print $2 "\t" $3 }' \
  build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv |
while IFS="$(printf '\t')" read -r case log; do
  if rg -q '\[RV64_BACKEND_RUNTIME_MISMATCH\]' "$log"; then
    printf '%s\t%s\n' "$case" "$log"
  fi
done
```

## Counts

| Population | Count |
| --- | ---: |
| Total TSV rows | 1467 |
| Passing rows | 314 |
| Failing rows | 1153 |
| Runtime mismatch rows included here | 40 |
| Non-runtime failing rows excluded here | 1113 |

All 40 runtime mismatch rows have `clang_exit=0` and empty captured
`clang_out`/`c4c_out` blocks in the per-case logs. The observable divergence is
therefore c4c runtime exit behavior, not textual stdout/stderr output.

## Runtime Failure-Mode Partitions

| Runtime divergence mode | Count | Representative cases | Evidence shape |
| --- | ---: | --- | --- |
| c4c aborts under qemu | 32 | `src/20000314-2.c`, `src/20000412-3.c`, `src/20010329-1.c`, `src/pr38533.c`, `src/pr83298.c` | `clang_exit=0 c4c_exit=Subprocess aborted` |
| c4c segfaults under qemu | 7 | `src/20080506-2.c`, `src/20120105-1.c`, `src/930725-1.c`, `src/pr43008.c`, `src/zerolen-1.c` | `clang_exit=0 c4c_exit=Segmentation fault` |
| c4c exits 255 under qemu | 1 | `src/pr81503.c` | `clang_exit=0 c4c_exit=255` |

These partitions are runtime-only. They do not include rows that fail closed
with unsupported prepared/BIR/object diagnostics, even when those unsupported
rows are higher-count or higher-priority in the whole-scan bucket map.

## Runtime Rows

| Case | Mode | Source log path |
| --- | --- | --- |
| `src/20000314-2.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20000314-2.c/case.log` |
| `src/20000412-3.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20000412-3.c/case.log` |
| `src/20000706-3.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20000706-3.c/case.log` |
| `src/20000715-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20000715-1.c/case.log` |
| `src/20010329-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20010329-1.c/case.log` |
| `src/20011019-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20011019-1.c/case.log` |
| `src/20020108-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20020108-1.c/case.log` |
| `src/20030128-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20030128-1.c/case.log` |
| `src/20030914-2.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log` |
| `src/20040311-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20040311-1.c/case.log` |
| `src/20071216-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_20071216-1.c/case.log` |
| `src/920612-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_920612-1.c/case.log` |
| `src/930622-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_930622-1.c/case.log` |
| `src/981206-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_981206-1.c/case.log` |
| `src/990604-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_990604-1.c/case.log` |
| `src/multdi-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_multdi-1.c/case.log` |
| `src/pr28651.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr28651.c/case.log` |
| `src/pr31136.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr31136.c/case.log` |
| `src/pr33779-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr33779-1.c/case.log` |
| `src/pr37102.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr37102.c/case.log` |
| `src/pr38422.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr38422.c/case.log` |
| `src/pr38533.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr38533.c/case.log` |
| `src/pr48973-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr48973-1.c/case.log` |
| `src/pr48973-2.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr48973-2.c/case.log` |
| `src/pr49123.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr49123.c/case.log` |
| `src/pr49161.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr49161.c/case.log` |
| `src/pr59014.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr59014.c/case.log` |
| `src/pr61306-3.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr61306-3.c/case.log` |
| `src/pr68376-1.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr68376-1.c/case.log` |
| `src/pr78436.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr78436.c/case.log` |
| `src/pr83298.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_pr83298.c/case.log` |
| `src/vrp-7.c` | c4c aborts under qemu | `build/rv64_gcc_c_torture_backend/src_vrp-7.c/case.log` |
| `src/20080506-2.c` | c4c segfaults under qemu | `build/rv64_gcc_c_torture_backend/src_20080506-2.c/case.log` |
| `src/20120105-1.c` | c4c segfaults under qemu | `build/rv64_gcc_c_torture_backend/src_20120105-1.c/case.log` |
| `src/930725-1.c` | c4c segfaults under qemu | `build/rv64_gcc_c_torture_backend/src_930725-1.c/case.log` |
| `src/pr43008.c` | c4c segfaults under qemu | `build/rv64_gcc_c_torture_backend/src_pr43008.c/case.log` |
| `src/pr60062.c` | c4c segfaults under qemu | `build/rv64_gcc_c_torture_backend/src_pr60062.c/case.log` |
| `src/pr79327.c` | c4c segfaults under qemu | `build/rv64_gcc_c_torture_backend/src_pr79327.c/case.log` |
| `src/zerolen-1.c` | c4c segfaults under qemu | `build/rv64_gcc_c_torture_backend/src_zerolen-1.c/case.log` |
| `src/pr81503.c` | c4c exits 255 under qemu | `build/rv64_gcc_c_torture_backend/src_pr81503.c/case.log` |

## Step 2 Recommendation

Step 2 should reproduce a small representative set rather than start with a
whole-scan rerun. Suggested representatives:

- Abort: `src/20000314-2.c`, `src/pr38533.c`, `src/pr83298.c`
- Segfault: `src/20080506-2.c`, `src/pr43008.c`, `src/zerolen-1.c`
- Exit 255: `src/pr81503.c`

For each representative, rerun the single backend object case and collect the
case log plus any qemu trace needed to determine whether the first owner is
RV64 target lowering, runtime ABI behavior, or a remaining producer semantic
gap. Do not mix this runtime reproduction with unsupported compile/codegen
bucket work.
