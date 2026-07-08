Status: Active
Source Idea Path: ideas/open/606_bir_global_initializer_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Same-Family Breadth

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: proved same-family breadth for the ordinary global initializer bootstrap repair using a focused allowlist at `build/agent_state/606_step4_initializer_breadth.allowlist` and the supervisor-delegated RV64 gcc_torture backend-object proof.

Row selection:
- Started from the Step 2/Step 3 classified ordinary initializer rows: `src/20021010-2.c`, `src/20021120-1.c`, and `src/20040302-1.c`.
- Added same-family neighbors from the same visible clusters: `src/20021010-1.c`, `src/20021120-2.c`, and `src/20021120-3.c`.
- Kept the established adjacent-owner sentinels in the same proof: `src/20010325-1.c`, `src/20010924-1.c`, `src/strlen-7.c`, `src/20020118-1.c`, and `src/ieee/fp-cmp-2.c`.

Same-family movement:
- `src/20021010-1.c`: passed the RV64 backend-object route.
- `src/20021010-2.c`: remained beyond the original `lir_to_bir` initializer bootstrap stop and now stops in `rv64_prepared_move_bundle_consumer` on ambiguous non-parallel register fan-in to one stack destination.
- `src/20021120-1.c`: remained beyond the original `lir_to_bir` initializer bootstrap stop and now stops in RV64 prepared pointer arithmetic lowering for loaded pointer base plus scaled integer byte offset.
- `src/20021120-2.c`: passed the RV64 backend-object route.
- `src/20021120-3.c`: reached BIR/prepared handoff and now stops in `rv64_prepared_move_bundle_consumer` on ambiguous non-parallel register fan-in to one stack destination.

Remaining initializer-bootstrap limitation:
- `src/20040302-1.c` still fails at the semantic BIR producer bootstrap stop. Its static pointer array contains `blockaddress(...)` entries, so this remains outside the ordinary byte/aggregate initializer repair and should not be claimed as fixed.

Adjacent-owner preservation:
- `src/20010325-1.c` remains the separate string-pool sentinel with `bootstrap lir_to_bir only supports byte-addressable string-pool constants right now`.
- `src/20010924-1.c` remains prepared/global object-data authority with `prepared selected object-data contract status=unsupported_but_coherent`.
- `src/strlen-7.c` remains prepared/global memory-fact authority with `RV64 object route requires supported prepared global memory facts`.
- `src/20020118-1.c` remains RV64/global consumer ownership with `RV64 object route cannot emit prepared global symbol`.
- `src/ieee/fp-cmp-2.c` remains RV64/global consumer ownership with the prepared global access-size limitation.

## Suggested Next

Step 5 should prepare the closure readiness summary. Recommendation: Step 4 found no blocker for closing the ordinary global initializer bootstrap repair, but the closure handoff must explicitly carve out `blockaddress(...)` pointer-array initializers, string-pool constants, prepared/global authority, RV64/global consumers, runtime/link behavior, expectations, unsupported markers, and repository allowlist changes.

## Watchouts

- The mutable summary TSV at `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` now reflects only this focused 11-row allowlist run; use the July 8 docs for the broader `470/1467` baseline.
- Do not claim `src/20040302-1.c` as fixed; its remaining first stop is `blockaddress(...)` pointer initializer representation.
- Do not pull string-pool (`src/20010325-1.c`), prepared/global authority, RV64/global consumers, runtime/link behavior, expectations, unsupported markers, or repository allowlist changes into Step 5 closure.
- The proof command exits nonzero because the delegated allowlist intentionally includes remaining limitation and guard/downstream rows.

## Proof

Ran the supervisor-delegated proof command exactly:

```sh
{ cmake --build --preset default && ALLOWLIST=build/agent_state/606_step4_initializer_breadth.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1
```

Result: command exited `1`; `test_after.log` is the canonical proof log. Build passed, then the focused allowlist reported `total=11 passed=2 failed=9`.

Relevant per-case proof logs:
- `build/rv64_gcc_c_torture_backend/src_20021010-1.c/case.log`: pass.
- `build/rv64_gcc_c_torture_backend/src_20021010-2.c/case.log`: downstream `rv64_prepared_move_bundle_consumer` failure after BIR bootstrap.
- `build/rv64_gcc_c_torture_backend/src_20021120-1.c/case.log`: downstream RV64 prepared pointer arithmetic failure after BIR bootstrap.
- `build/rv64_gcc_c_torture_backend/src_20021120-2.c/case.log`: pass.
- `build/rv64_gcc_c_torture_backend/src_20021120-3.c/case.log`: downstream `rv64_prepared_move_bundle_consumer` failure after BIR/prepared handoff.
- `build/rv64_gcc_c_torture_backend/src_20040302-1.c/case.log`: remaining BIR bootstrap failure from `blockaddress(...)` pointer-array initializer.
- Guard logs under `build/rv64_gcc_c_torture_backend/<case-id>/case.log` preserved the Step 2 owner split for string-pool, prepared/global, and RV64/global consumer rows.
