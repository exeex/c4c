Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Proof Summary and Closure Readiness

# Current Packet

## Just Finished

Completed Step 4 same-family breadth evidence for the current visible BIR local-memory store row set.

Artifact baseline:
- The active idea and plan still cite the older `56` local-memory store rows from the RV64 gcc_torture scan.
- Current available row evidence for this packet exposes `46` diagnostic store-family labels, matching the 46 store rows named by the delegated proof regex. The older 56-row source list is not available as row-level current artifacts in this workspace.
- Existing backend case logs under `build/rv64_gcc_c_torture_backend/` are stale for rows already proven by Step 3 focused probes; they still show old `store local-memory semantic family` stops for `src/20010605-2.c`, `src/20020413-1.c`, `src/pr39120.c`, and `src/strct-pack-2.c`.

Current breadth classification from transient `--dump-bir --target riscv64-linux-gnu` probes over the 46 visible store rows:
- Progressed beyond the old full-row store producer stop: `6/46`.
- BIR dump now succeeds for `src/20010605-2.c`, `src/20020413-1.c`, `src/20040208-1.c`, `src/930526-1.c`, `src/ieee/inf-1.c`, and `src/strct-pack-2.c`.
- Still show local-memory store limitations: `40/46`. These rows continue to report `latest function failure: semantic lir_to_bir function ... failed in store local-memory semantic family`.
- Visible adjacent-owner handoff remains limited to the Step 3 focused `src/pr39120.c` finding: the old `bar` store-through-loaded-pointer boundary moved to a later `main` aggregate-to-global store boundary. The full-row breadth probe still reports the remaining `main` stop through the store local-memory family label, so it is not counted as a full-row BIR dump success here.

The established guard rows were included in the delegated proof and kept their non-store ownership contract: `src/20041124-1.c` (load), `src/20030717-1.c` (GEP), `src/20180921-1.c` (alloca), `src/20000722-1.c` (prepared/RV64 local memory), `src/20000603-1.c` (ABI), `src/pr38533.c` (runtime), `src/20010924-1.c` (prepared/global-data), and `src/20020118-1.c` (RV64/global-data).

## Suggested Next

Advance to Step 5 final proof summary and closure-readiness decision. The Step 5 packet should decide whether `6/46` full-row BIR dump successes plus the focused `src/pr39120.c` adjacent-owner movement satisfy this source idea, or whether the remaining `40/46` store-family rows require another local-memory store producer packet before lifecycle closure.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Treat `src/pr39120.c` and `src/strct-pack-2.c` as probes for semantic destination/address repair, not implementation keys.
- Do not infer full closure from the delegated LLVM gcc_torture CTest subset alone; it passed, but the BIR breadth probes still show 40 visible store-family stops.
- Treat the stale backend case logs as historical evidence unless a supervisor refreshes the RV64 backend-object scan.
- Do not change tests, expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, implementation files, or source ideas in the Step 4 evidence slice.

## Proof

Delegated proof passed and was written to `test_after.log`:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(20041124_1|20030717_1|20180921_1|20000722_1|20000603_1|pr38533|20010924_1|20020118_1)_c|llvm_gcc_c_torture_src_20010605_2_c|llvm_gcc_c_torture_src_20020413_1_c|llvm_gcc_c_torture_src_20030913_1_c|llvm_gcc_c_torture_src_20040208_1_c|llvm_gcc_c_torture_src_20040707_1_c|llvm_gcc_c_torture_src_20131127_1_c|llvm_gcc_c_torture_src_920501_5_c|llvm_gcc_c_torture_src_930126_1_c|llvm_gcc_c_torture_src_930526_1_c|llvm_gcc_c_torture_src_930719_1_c|llvm_gcc_c_torture_src_931102_2_c|llvm_gcc_c_torture_src_981130_1_c|llvm_gcc_c_torture_src_990208_1_c|llvm_gcc_c_torture_src_990525_1_c|llvm_gcc_c_torture_src_991118_1_c|llvm_gcc_c_torture_src_alias_1_c|llvm_gcc_c_torture_src_alias_access_path_1_c|llvm_gcc_c_torture_src_compndlit_1_c|llvm_gcc_c_torture_src_ieee_inf_1_c|llvm_gcc_c_torture_src_lto_tbaa_1_c|llvm_gcc_c_torture_src_pr15262_2_c|llvm_gcc_c_torture_src_pr22141_1_c|llvm_gcc_c_torture_src_pr22141_2_c|llvm_gcc_c_torture_src_pr36343_c|llvm_gcc_c_torture_src_pr36765_c|llvm_gcc_c_torture_src_pr39120_c|llvm_gcc_c_torture_src_pr44164_c|llvm_gcc_c_torture_src_pr52979_1_c|llvm_gcc_c_torture_src_pr52979_2_c|llvm_gcc_c_torture_src_pr57344_1_c|llvm_gcc_c_torture_src_pr57344_2_c|llvm_gcc_c_torture_src_pr57344_3_c|llvm_gcc_c_torture_src_pr57344_4_c|llvm_gcc_c_torture_src_pr58277_1_c|llvm_gcc_c_torture_src_pr58365_c|llvm_gcc_c_torture_src_pr60072_c|llvm_gcc_c_torture_src_pr69691_c|llvm_gcc_c_torture_src_pr70127_c|llvm_gcc_c_torture_src_pr71626_1_c|llvm_gcc_c_torture_src_pr71626_2_c|llvm_gcc_c_torture_src_pr78170_c|llvm_gcc_c_torture_src_pr79043_c|llvm_gcc_c_torture_src_pr79737_1_c|llvm_gcc_c_torture_src_pr82388_c|llvm_gcc_c_torture_src_strct_pack_2_c|llvm_gcc_c_torture_src_struct_cpy_1_c)$' >> test_after.log 2>&1
```

Result: build passed; selected CTest subset passed `55/55`, including `backend_lir_to_bir_notes`, 8 guard rows, and 46 visible store-family evidence rows.

Supplemental classification command was transient and did not write root-level logs: `build/c4cll --dump-bir --target riscv64-linux-gnu <row>`. Result over the 46 visible store rows: `6` BIR dump successes, `40` remaining `store local-memory semantic family` stops.
