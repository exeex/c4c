Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Classify the Remaining Store Subfamily

# Current Packet

## Just Finished

Completed Step 5 final proof summary and closure-readiness decision for idea 603.

The accepted route checkpoint is:
- Current implementation is real same-family progress, not closure: `src/20010605-2.c`, `src/20020413-1.c`, `src/strct-pack-2.c`, and the earlier `src/pr39120.c` `bar` boundary moved beyond the prior local-memory store producer stops.
- Step 4 breadth evidence found `6/46` full-row BIR dump successes and `40/46` remaining visible rows still reporting the store local-memory semantic family.
- Guard rows for load, GEP, alloca, prepared/RV64, ABI, runtime, prepared/global-data, and RV64/global-data ownership were preserved in the delegated proof.
- Idea 603 is not closure-ready because the remaining visible store-family stops still belong to the source idea's BIR local-memory store producer scope.

## Suggested Next

Execute Step 6: classify the remaining store subfamily.

The next executor packet should:
- Sample the remaining `40/46` visible store-family stops with focused HIR and BIR dumps.
- Group failures by first missing store fact, source-value shape, destination/address authority, and aggregate/global-data handoff risk.
- Select at least two representative rows for the next local-memory store producer subfamily.
- Keep `src/pr39120.c` separate unless the focused failing function proves it is still a local-memory store producer problem rather than the later aggregate-to-global boundary.
- Record representative rows, excluded adjacent-owner rows, guard rows, and the exact supervisor proof command here.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Do not infer closure from the green LLVM gcc_torture CTest subset alone; the BIR breadth probes still show `40` visible store-family stops.
- Treat stale backend case logs under `build/rv64_gcc_c_torture_backend/` as historical evidence unless the supervisor refreshes the RV64 backend-object scan.
- Do not change expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, RV64 lowering, or adjacent owner routes.

## Proof

Lifecycle-only update; no build or tests were run.

Latest accepted implementation proof remains the Step 4 command recorded before this reset: build passed and the selected CTest subset passed `55/55`, including `backend_lir_to_bir_notes`, 8 guard rows, and 46 visible store-family evidence rows.

Supplemental Step 4 classification used transient `build/c4cll --dump-bir --target riscv64-linux-gnu <row>` probes and found `6` BIR dump successes versus `40` remaining `store local-memory semantic family` stops.
