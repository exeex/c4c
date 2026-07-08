Status: Active
Source Idea Path: ideas/open/606_bir_global_initializer_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair In-Scope Initializer Bootstrap

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: repaired the semantic BIR producer path for ordinary global initializer bootstrap rows without changing expectations, unsupported markers, or the global torture allowlist.

Implementation surface:
- `src/backend/bir/lir_to_bir/globals.cpp`: ordinary array and inline aggregate globals with non-struct `LirTypeRef` metadata now use the existing aggregate layout/initializer text path; named struct refs keep the strict structured metadata path and fail closed on stale or missing struct identity.
- `src/backend/bir/lir_to_bir/global_initializers.cpp`: scalar pointer initializers now admit LLVM `inttoptr (i64 ... to ptr)` constants as pointer immediates, and `float`/`double` decimal zero literals (`0.0`, `+0.0`) lower to typed zero bits inside aggregate/array initializers.

Selected row movement:
- `src/20021010-2.c`: `--dump-bir --target riscv64-linux-gnu` now succeeds. The RV64 object proof moved beyond the original `lir_to_bir` bootstrap stop and now fails in `rv64_prepared_move_bundle_consumer` on an ambiguous non-parallel multi-source stack destination.
- `src/20021120-1.c`: `--dump-bir --target riscv64-linux-gnu` now succeeds. The RV64 object proof moved beyond the original `lir_to_bir` bootstrap stop and now fails in RV64 prepared pointer arithmetic lowering for loaded pointer base plus scaled integer byte offset.
- `src/20040302-1.c`: still fails at the original semantic BIR producer bootstrap stop because its `[2 x ptr]` global carries `blockaddress(...)` pointer initializers. I left that out of this slice because representing function label addresses would cross into blockaddress/function-address authority rather than the ordinary byte/aggregate initializer repair.

Guard preservation:
- `src/20010325-1.c` remains the separate string-pool sentinel with `bootstrap lir_to_bir only supports byte-addressable string-pool constants right now`.
- `src/20010924-1.c` remains prepared/global object-data authority with `prepared selected object-data contract status=unsupported_but_coherent`.
- `src/strlen-7.c` remains RV64 prepared global memory facts/string-adjacent ownership with `RV64 object route requires supported prepared global memory facts`.
- `src/20020118-1.c` remains RV64/global consumer ownership with `RV64 object route cannot emit prepared global symbol`.
- `src/ieee/fp-cmp-2.c` remains RV64/global consumer ownership with the prepared global access-size limitation.

## Suggested Next

Step 4 should prove same-family breadth for ordinary initializer bootstrap rows now covered by the semantic producer repair, while keeping `blockaddress(...)` pointer-array initializers and string-pool constants as separate owner candidates unless the supervisor delegates those explicitly.

## Watchouts

- Do not claim `src/20040302-1.c` as fixed by this packet; its remaining first stop is `blockaddress(...)` pointer initializer representation.
- The current repair intentionally preserves strict named-struct `LirTypeRef` behavior; do not broaden it into stale structured metadata recovery.
- The proof command exits nonzero because the delegated allowlist includes guard/downstream rows. That is expected for this packet; the meaningful movement is that two selected rows no longer fail at the old BIR initializer bootstrap stop.
- Keep prepared/global authority, RV64/global consumers, string-pool policy, runtime/link behavior, expectations, unsupported markers, and repository allowlists out of the next packet unless explicitly delegated.

## Proof

Ran the supervisor-delegated proof command exactly:

```sh
{ cmake --build --preset default && ALLOWLIST=build/agent_state/606_initializer_bootstrap_step3.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1
```

Result: command exited `1`; `test_after.log` is the canonical proof log. Build passed, then the eight-row delegated allowlist reported `total=8 passed=0 failed=8`.

Relevant per-case proof logs:
- `build/rv64_gcc_c_torture_backend/src_20021010-2.c/case.log`: downstream `rv64_prepared_move_bundle_consumer` failure after BIR bootstrap.
- `build/rv64_gcc_c_torture_backend/src_20021120-1.c/case.log`: downstream RV64 prepared pointer arithmetic failure after BIR bootstrap.
- `build/rv64_gcc_c_torture_backend/src_20040302-1.c/case.log`: remaining BIR bootstrap failure from `blockaddress(...)` pointer-array initializer.
- Guard logs under `build/rv64_gcc_c_torture_backend/<case-id>/case.log` preserved the Step 2 owner split for string-pool, prepared/global, and RV64/global consumer rows.
