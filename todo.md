Status: Active
Source Idea Path: ideas/open/606_bir_global_initializer_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Initializer Bootstrap Proof Rows

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: selected representative BIR global initializer bootstrap rows from the current July 8 RV64 gcc_torture backend-object artifacts, plus adjacent-owner guard rows.

Refreshed evidence source:
- Current summary: `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- Current failed list: `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- Per-case logs: `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- Baseline count from `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`: `470/1467` pass rows, `997` fail rows, `0` missing rows; current mutable row artifacts have July 8 mtimes and supersede the older `438/1467` pointer log.

Selected in-scope bootstrap proof rows:
- `src/20040302-1.c` -> `build/rv64_gcc_c_torture_backend/src_20040302-1.c/case.log`: `fail`; stops before prepared object handoff with `backend object route requires semantic lir_to_bir lowering` and the bootstrap diagnostic for unsupported global initializer shapes requiring honest byte-address semantics.
- `src/20021010-2.c` -> `build/rv64_gcc_c_torture_backend/src_20021010-2.c/case.log`: `fail`; same BIR producer stop and same aggregate/byte-address global initializer bootstrap diagnostic.
- `src/20021120-1.c` -> `build/rv64_gcc_c_torture_backend/src_20021120-1.c/case.log`: `fail`; same BIR producer stop and same aggregate/byte-address global initializer bootstrap diagnostic.

String-pool sentinel, tracked but not counted as ordinary aggregate/byte initializer progress:
- `src/20010325-1.c` -> `build/rv64_gcc_c_torture_backend/src_20010325-1.c/case.log`: `fail`; stops before prepared object handoff with `bootstrap lir_to_bir only supports byte-addressable string-pool constants right now`. This is useful breadth context from the source idea's `2` string-pool rows, but Step 3 should not claim it as ordinary global initializer progress unless explicitly delegated.

Guard rows for adjacent owners:
- Prepared/global authority guard: `src/20010924-1.c` -> `build/rv64_gcc_c_torture_backend/src_20010924-1.c/case.log`: `fail`; reaches prepared/global territory and reports `unsupported_global_data: prepared selected object-data contract status=unsupported_but_coherent ... emitted_byte_count=0`.
- Prepared/global authority guard: `src/strlen-7.c` -> `build/rv64_gcc_c_torture_backend/src_strlen-7.c/case.log`: `fail`; reports `unsupported_global_data: RV64 object route requires supported prepared global memory facts`.
- RV64/global consumer guard: `src/20020118-1.c` -> `build/rv64_gcc_c_torture_backend/src_20020118-1.c/case.log`: `fail`; reports `unsupported_global_data: RV64 object route cannot emit prepared global symbol`.
- RV64/global consumer guard: `src/ieee/fp-cmp-2.c` -> `build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-2.c/case.log`: `fail`; reports `unsupported_global_data: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses`.

## Suggested Next

Step 2 should trace the selected rows through HIR/BIR/object-route evidence to confirm the first missing fact is BIR global initializer byte/aggregate production, then write `build/agent_state/606_initializer_bootstrap_step3.allowlist` for the later Step 3 proof subset.

## Watchouts

- Keep prepared/global authority and RV64/global consumer failures separate from this BIR initializer bootstrap route.
- Do not pull string-library policy, runtime/link behavior, expectations, unsupported markers, allowlists, timeouts, or accounting into this plan.
- Reject named-case fixes, especially routes centered only on `src/20040302-1.c`.
- The current ordinary initializer proof set intentionally uses multiple same-diagnostic rows (`src/20040302-1.c`, `src/20021010-2.c`, `src/20021120-1.c`) so Step 3 cannot be accepted as a named-case-only route.
- Guard rows should remain downstream or adjacent failures after a BIR bootstrap repair; making them pass is not required for this plan and may indicate owner drift.

## Proof

No build or test run for this lifecycle/artifact-selection packet; no `test_after.log` was produced.

Future delegated narrow proof command for code-changing packets:

```sh
cmake --build --preset default && ALLOWLIST=build/agent_state/606_initializer_bootstrap_step3.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```
