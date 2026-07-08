Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Proof Summary and Closure Readiness

# Current Packet

## Just Finished

Completed Step 5 final proof summary and closure-readiness recommendation for idea 603.

Implementation surface proven by the current slice:
- `src/backend/bir/lir_to_bir/scalar.cpp` now supports generic `f128` literal materialization for scalar value lowering, enabling ordinary local `f128` immediate stores to produce structured BIR values.
- `src/backend/bir/lir_to_bir/memory/provenance.cpp` now publishes opaque runtime `PointerAddress` authority for loaded global pointer fields when no stronger fact exists.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp` now lowers dynamic local pointer-array stores into selected per-element `bir::StoreLocalInst`s while clearing precise address side tables conservatively.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` now asserts the new structured `f128` immediate payload contract instead of the stale unsupported boundary.

Progressed rows and downstream movement:
- Focused Step 3 proof showed `src/20010605-2.c` and `src/20020413-1.c` progress beyond the old `f128` local-store producer stop.
- Focused Step 3 proof showed `src/strct-pack-2.c` now dumps BIR for `main`.
- Focused Step 3 proof showed `src/pr39120.c` progress beyond the old `bar` store-through-loaded-pointer boundary to a later `main` aggregate-to-global store boundary.
- Step 4 transient BIR breadth probes over the 46 visible store-family rows found full-row BIR dump success for `6/46`: `src/20010605-2.c`, `src/20020413-1.c`, `src/20040208-1.c`, `src/930526-1.c`, `src/ieee/inf-1.c`, and `src/strct-pack-2.c`.

Remaining local-memory store limitations:
- `40/46` visible store-family rows still report `latest function failure: semantic lir_to_bir function ... failed in store local-memory semantic family`.
- The source idea and plan cite the older `56` local-memory store rows from the RV64 gcc_torture scan; current row-level artifacts only exposed 46 diagnostic store-family labels during Step 4, so the exact `56` to current-state comparison remains incomplete.
- Existing backend case logs under `build/rv64_gcc_c_torture_backend/` are stale for some Step 3-proven rows and should not be used as the final closure evidence without a refreshed scan.

Adjacent-owner handoffs:
- `src/pr39120.c` has a defensible downstream aggregate-to-global store boundary after the focused `bar` repair, but the full-row probe still reports a later `main` stop under the store local-memory family label. It should not be counted as full idea closure evidence.
- No Step 5 evidence moved ownership into local-memory GEP, alloca, ABI, runtime, RV64 target lowering, or expectation/allowlist handling.

Guard preservation:
- The Step 4 delegated proof included and preserved the established non-store guard rows: `src/20041124-1.c` (load), `src/20030717-1.c` (GEP), `src/20180921-1.c` (alloca), `src/20000722-1.c` (prepared/RV64 local memory), `src/20000603-1.c` (ABI), `src/pr38533.c` (runtime), `src/20010924-1.c` (prepared/global-data), and `src/20020118-1.c` (RV64/global-data).

Closure readiness:
- The implemented slice is ready for supervisor review as real same-family progress: multiple store-shaped rows progressed, guards were preserved, and no evidence shows testcase matching, expectation weakening, RV64 compensation, or owner reclassification.
- Idea 603 is not closure-ready on the current evidence. The remaining `40/46` visible store-family stops mean the source idea's broader goal of repairing BIR local-memory store production across the current store-owned family is not yet satisfied.
- Recommended lifecycle action: commit the validated progress slice if supervisor review accepts the implementation quality, then continue idea 603 with another local-memory store producer packet or request plan-owner route refinement if the remaining rows split into a distinct subfamily.

## Suggested Next

Supervisor should review and commit the current coherent implementation plus `todo.md` evidence slice if acceptable, then delegate a follow-up packet that samples the remaining `40/46` visible store-family stops and identifies the next common local-memory store producer limitation.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Treat `src/pr39120.c` as an adjacent-owner handoff only after separating the later aggregate-to-global `main` stop from the earlier local-memory store repair.
- Do not infer idea closure from the green LLVM gcc_torture CTest subset alone; it passed, but the BIR breadth probes still show 40 visible store-family stops.
- Treat the stale backend case logs as historical evidence unless the supervisor refreshes the RV64 backend-object scan.
- Do not change tests, expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, implementation files, `plan.md`, or source ideas in this evidence-only Step 5 slice.

## Proof

No build or tests were required or run for Step 5; this was an evidence-summary packet using the already recorded Step 3 and Step 4 proof results.

The Step 4 proof was produced as `test_after.log` during that proof run, then accepted and rolled forward to the current canonical `test_before.log`. Current root log state is `test_baseline.log` plus `test_before.log`; `todo.md` should not be read as claiming a current `test_after.log` proof file exists.

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(20041124_1|20030717_1|20180921_1|20000722_1|20000603_1|pr38533|20010924_1|20020118_1)_c|llvm_gcc_c_torture_src_20010605_2_c|llvm_gcc_c_torture_src_20020413_1_c|llvm_gcc_c_torture_src_20030913_1_c|llvm_gcc_c_torture_src_20040208_1_c|llvm_gcc_c_torture_src_20040707_1_c|llvm_gcc_c_torture_src_20131127_1_c|llvm_gcc_c_torture_src_920501_5_c|llvm_gcc_c_torture_src_930126_1_c|llvm_gcc_c_torture_src_930526_1_c|llvm_gcc_c_torture_src_930719_1_c|llvm_gcc_c_torture_src_931102_2_c|llvm_gcc_c_torture_src_981130_1_c|llvm_gcc_c_torture_src_990208_1_c|llvm_gcc_c_torture_src_990525_1_c|llvm_gcc_c_torture_src_991118_1_c|llvm_gcc_c_torture_src_alias_1_c|llvm_gcc_c_torture_src_alias_access_path_1_c|llvm_gcc_c_torture_src_compndlit_1_c|llvm_gcc_c_torture_src_ieee_inf_1_c|llvm_gcc_c_torture_src_lto_tbaa_1_c|llvm_gcc_c_torture_src_pr15262_2_c|llvm_gcc_c_torture_src_pr22141_1_c|llvm_gcc_c_torture_src_pr22141_2_c|llvm_gcc_c_torture_src_pr36343_c|llvm_gcc_c_torture_src_pr36765_c|llvm_gcc_c_torture_src_pr39120_c|llvm_gcc_c_torture_src_pr44164_c|llvm_gcc_c_torture_src_pr52979_1_c|llvm_gcc_c_torture_src_pr52979_2_c|llvm_gcc_c_torture_src_pr57344_1_c|llvm_gcc_c_torture_src_pr57344_2_c|llvm_gcc_c_torture_src_pr57344_3_c|llvm_gcc_c_torture_src_pr57344_4_c|llvm_gcc_c_torture_src_pr58277_1_c|llvm_gcc_c_torture_src_pr58365_c|llvm_gcc_c_torture_src_pr60072_c|llvm_gcc_c_torture_src_pr69691_c|llvm_gcc_c_torture_src_pr70127_c|llvm_gcc_c_torture_src_pr71626_1_c|llvm_gcc_c_torture_src_pr71626_2_c|llvm_gcc_c_torture_src_pr78170_c|llvm_gcc_c_torture_src_pr79043_c|llvm_gcc_c_torture_src_pr79737_1_c|llvm_gcc_c_torture_src_pr82388_c|llvm_gcc_c_torture_src_strct_pack_2_c|llvm_gcc_c_torture_src_struct_cpy_1_c)$' >> test_after.log 2>&1
```

Result: build passed; selected CTest subset passed `55/55`, including `backend_lir_to_bir_notes`, 8 guard rows, and 46 visible store-family evidence rows.

Supplemental classification command was transient and did not write root-level logs: `build/c4cll --dump-bir --target riscv64-linux-gnu <row>`. Result over the 46 visible store rows: `6` BIR dump successes, `40` remaining `store local-memory semantic family` stops.
