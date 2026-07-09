Status: Active
Source Idea Path: ideas/open/618_runtime_mismatch_ownership_investigation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Write Runtime Symptom Map

# Current Packet

## Just Finished

Step 2 for `ideas/open/618_runtime_mismatch_ownership_investigation.md`
created `docs/runtime_mismatch_ownership/01_runtime_symptom_map.md` from the
accepted full RV64 gcc torture backend scan evidence.

The symptom map records:

- Full scan command:
  `BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > /tmp/c4c_618_full_scan.log 2>&1`.
- Summary artifacts:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`, both with mtime
  `2026-07-09 01:31:56 +0000`.
- Runtime symptom total: `217` rows: `212`
  `[RV64_BACKEND_RUNTIME_MISMATCH]` rows and `5`
  `[RV64_C4C_RUN_TIMEOUT]` rows.
- Abort/assertion family: `110` rows, including the explicit
  `src/990106-1.c` dynamic-loader assertion subcase.
- Segfault family: `102` rows.
- Wrong-output family: `0` rows in the accepted baseline.
- Timeout family: `5` rows.
- Non-runtime context kept separate: `738` object compile failures, `11`
  object compile timeouts, and `28` link failures, totaling `777`
  compile/link failures outside the runtime symptom set.

The document cites representative logs for abort/assertion, segfault, and
timeout symptoms and records the evidence commands used to classify the rows.

## Suggested Next

Proceed to Step 3 and write
`docs/runtime_mismatch_ownership/02_likely_first_owner_map.md`, mapping the
stable abort/assertion, segfault, and timeout families to likely first owners
while keeping wrong output as a stable empty family for this baseline.

## Watchouts

`src/990106-1.c` is not a normal `Subprocess aborted` row; it exits `127`
after a dynamic-loader assertion. Keep it explicit as an abort/assertion subcase
in Step 3 owner mapping.

The `217` runtime symptom rows remain separate from the `777` compile/link
failures. Step 3 should not create a generic all-runtime owner bucket; it should
map symptom families to ABI, layout, local/global memory, call lowering, true
runtime support, or unresolved using concrete evidence.

## Proof

Documentation-only packet. Verified
`docs/runtime_mismatch_ownership/01_runtime_symptom_map.md` exists and cites the
accepted full-scan command, summary artifacts, representative logs, symptom
inventory commands, runtime symptom counts, and non-runtime context. No build or
backend proof was run. `test_after.log` was not created or modified by this
packet.
