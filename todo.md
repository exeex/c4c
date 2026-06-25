Status: Active
Source Idea Path: ideas/open/357_rv64_object_route_data_sections_globals_strings.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run Milestone Validation and Route Residuals

# Current Packet

## Just Finished

Completed Plan Step 5 milestone validation for idea 357 without implementation
or expectation edits. The delegated build was up to date, and the selected
object/roundtrip CTest regex ran the tests present in this build:
`backend_cli_riscv64_unsupported_global_diagnostic_obj`,
`backend_object_model_records`, and `backend_riscv_object_emission`; all passed.

The full RV64 gcc torture backend scan completed with
`total=1467 passed=145 failed=1322`. The Step 4 representatives stayed in the
expected routed state: `src/20000223-1.c` and `src/20000227-1.c` pass, while
`src/20000112-1.c` and `src/20000224-1.c` fail with
`unsupported_terminator_fragment`, not data section, global symbol, string
constant, or relocation failures.

Residual data-looking failures are now precise classifications rather than the
old blanket `prepared.module.globals/string_constants` rejection, which does
not appear in `test_after.log` or the generated case logs. The scan contains 44
`unsupported_global_data` object-route residuals and 52 upstream LIR-to-BIR
global/string publication gaps. Representative residuals:
`src/20000412-1.c` requires supported prepared global memory facts,
`src/20001121-1.c` exceeds the currently supported 1-, 2-, 4-, and 8-byte
prepared global memory access widths, `src/20010924-1.c` uses non-immediate
or non-linear global storage, `src/pr68624.c` needs prepared direct
global-symbol base-plus-offset memory addressing, `src/widechar-3.c` needs
non-byte-addressable string-pool support before prepared object handoff, and
`src/strlen-3.c` needs broader LIR-to-BIR global publication. These are outside
the completed supported scalar/global/string section emission path and should
be handled as follow-up prepared-data/global-addressing initiatives rather than
expanding idea 357's RV64 object section work.

## Suggested Next

Recommend supervisor-side lifecycle review/close for idea 357 if the source
idea acceptance criteria are judged to be the supported prepared data bucket:
object tests are green, the representative supported string/global cases pass
or route to later backend buckets, and remaining data residuals are precise
unsupported contract/global-addressing follow-ups.

## Watchouts

- Do not treat the 1322 full-scan failures as idea 357 regressions; the
  milestone scan is a broad routing artifact, and the focused data
  representatives show the supported data path moved.
- `unsupported_terminator_fragment` remains a later RV64 object control-flow
  bucket; do not fold terminator lowering into idea 357 closure work.
- The remaining `unsupported_global_data` examples include precise follow-up
  shapes: missing prepared global memory facts, wider/non-scalar global memory
  accesses, non-immediate/non-linear storage, and direct global-symbol
  base-plus-offset memory addressing.
- Upstream LIR-to-BIR publication gaps remain separate from RV64 object
  emission: broader global kinds and non-byte-addressable string-pool constants
  must be represented before the prepared object handoff.

## Proof

Proof ran:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_riscv_object_emission|backend_object_model_records|backend_cli_riscv64_unsupported_global_diagnostic_obj|backend_c4c_as|backend_c4c_objdump)$' && CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
```

The build passed, the selected CTest subset passed, and the full RV64 gcc
torture backend scan wrote `test_after.log`. The scan summary is preserved at
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`, with failed cases
listed in `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` and
per-case logs under `build/rv64_gcc_c_torture_backend/`.
