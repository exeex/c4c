Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 10
Current Step Title: Updated Closure Readiness

# Current Packet

## Just Finished

Completed Step 10 updated closure readiness for `ideas/open/603_bir_local_memory_store_semantics.md`.

Accepted implementation surface under this runbook:
- Full-width `f128` local literal source values now lower into BIR scalar values instead of stopping at the store source-value boundary.
- Loaded global pointer fields now retain enough pointer-address authority for local store production when no stronger provenance fact exists.
- Dynamic local pointer-array stores now lower selected element updates while preserving unselected local values.
- Dynamic local aggregate-array element copies now lower from aggregate source aliases into local aggregate destinations.
- Single-lane SSA vector stores into local vector lane slots now lower as local store facts.

Source-idea acceptance status:
- Satisfied under the refreshed evidence. Same-family local-memory store rows progressed beyond the old BIR store producer stop, proof covered more than one store-shaped row, and non-store guard owners preserved their established ownership.
- Step 9 refreshed the 46 visible store-row set from the Step 5 checkpoint of `6/46` full-row BIR successes and `40/46` remaining store-family stops to `7/46` full-row BIR successes, `2/46` rows progressed beyond the old store producer to downstream `scalar/local-memory semantic family`, and `37/46` rows still reporting the store local-memory family.
- The same-family movement is semantic, not testcase-shaped: `src/990525-1.c` now completes BIR after local aggregate-array element copy repair, and `src/pr71626-1.c` plus `src/pr71626-2.c` moved beyond the local vector store producer into a downstream boundary.

Remaining local-memory store limitation after adjacent-owner separation:
- `src/931102-2.c` remains the only clearly in-scope local store limitation from the refreshed set, failing in `f` on the local union/overlap store shape.
- This is a real remaining local-memory store limitation, but it is a singleton and does not justify another broad producer packet under the current runbook without fresh evidence of a shared subfamily.

Guard-owner preservation:
- Pointer/address-authority rows remain store-family owned by adjacent address authority: `src/20030913-1.c`, `src/930719-1.c`, `src/alias-1.c`, `src/alias-access-path-1.c`, `src/pr15262-2.c`, `src/pr36343.c`, `src/pr36765.c`, `src/pr58277-1.c`, `src/pr60072.c`, `src/pr69691.c`, and `src/pr79043.c`.
- Function-label/local pointer-array rows remain adjacent: `src/920501-5.c` and `src/990208-1.c`.
- Non-store guard rows in the proof subset preserved expected outcomes: `src/20041124-1.c`, `src/20030717-1.c`, `src/20180921-1.c`, `src/20000722-1.c`, `src/20000603-1.c`, `src/pr38533.c`, `src/20010924-1.c`, and `src/20020118-1.c`.

Downstream handoffs:
- Aggregate/static global handoff rows still belong to idea 619 or existing global-data routes: `src/20040707-1.c`, `src/20131127-1.c`, `src/930126-1.c`, `src/981130-1.c`, `src/991118-1.c`, `src/compndlit-1.c`, `src/lto-tbaa-1.c`, `src/pr22141-1.c`, `src/pr22141-2.c`, `src/pr39120.c`, `src/pr44164.c`, `src/pr52979-1.c`, `src/pr52979-2.c`, `src/pr57344-1.c`, `src/pr57344-2.c`, `src/pr57344-3.c`, `src/pr57344-4.c`, `src/pr58365.c`, `src/pr70127.c`, `src/pr78170.c`, `src/pr79737-1.c`, `src/pr82388.c`, and `src/struct-cpy-1.c`.
- `src/pr71626-1.c` and `src/pr71626-2.c` should be treated as downstream scalar/local-memory classification candidates, not store producer candidates, unless a focused probe proves otherwise.

## Suggested Next

Recommend closing idea 603 as acceptance-satisfied under the refreshed evidence.

Rationale:
- The runbook accepted generic local-memory store producer repairs across multiple source-value and destination shapes.
- No broad remaining multi-row in-scope local-memory store producer subfamily is visible after excluding adjacent owners.
- Continuing here would either chase a singleton local union/overlap case or widen into pointer/address authority, function-label/local pointer-array, aggregate/global handoff, downstream scalar/local-memory classification, or global-data ownership.
- Idea 619 remains the correct separate lifecycle route for aggregate/static global store handoff; do not switch to it before closing or otherwise resolving the active 603 lifecycle state.

## Watchouts

- The Step 9 BIR probes were transient command output only; do not treat stale backend case logs under `build/rv64_gcc_c_torture_backend/` as refreshed evidence unless the supervisor reruns that scan.
- `src/931102-2.c` is a singleton. It is in-scope local-memory store evidence, but it is not a multi-row subfamily by itself.
- Pointer/address rows remain adjacent unless a later packet proves a store producer failure without guessing address authority.
- Function-label/local pointer-array rows remain adjacent; the `pr71626` rows no longer stop at the local vector store producer.
- `src/pr39120.c` remains an aggregate/global handoff guard after the earlier `bar` pointer-store boundary moved.
- Do not change expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, RV64 lowering, or adjacent owner routes.

## Proof

Evidence-only Step 10 packet; no build was required and no code changed.

Step 9 proof already passed, was produced as `test_after.log` during that run, and was accepted/rolled forward to `test_before.log`:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(20041124_1|20030717_1|20180921_1|20000722_1|20000603_1|pr38533|20010924_1|20020118_1)_c|llvm_gcc_c_torture_src_20010605_2_c|llvm_gcc_c_torture_src_20020413_1_c|llvm_gcc_c_torture_src_20030913_1_c|llvm_gcc_c_torture_src_20040208_1_c|llvm_gcc_c_torture_src_20040707_1_c|llvm_gcc_c_torture_src_20131127_1_c|llvm_gcc_c_torture_src_920501_5_c|llvm_gcc_c_torture_src_930126_1_c|llvm_gcc_c_torture_src_930526_1_c|llvm_gcc_c_torture_src_930719_1_c|llvm_gcc_c_torture_src_931102_2_c|llvm_gcc_c_torture_src_981130_1_c|llvm_gcc_c_torture_src_990208_1_c|llvm_gcc_c_torture_src_990525_1_c|llvm_gcc_c_torture_src_991118_1_c|llvm_gcc_c_torture_src_alias_1_c|llvm_gcc_c_torture_src_alias_access_path_1_c|llvm_gcc_c_torture_src_compndlit_1_c|llvm_gcc_c_torture_src_ieee_inf_1_c|llvm_gcc_c_torture_src_lto_tbaa_1_c|llvm_gcc_c_torture_src_pr15262_2_c|llvm_gcc_c_torture_src_pr22141_1_c|llvm_gcc_c_torture_src_pr22141_2_c|llvm_gcc_c_torture_src_pr36343_c|llvm_gcc_c_torture_src_pr36765_c|llvm_gcc_c_torture_src_pr39120_c|llvm_gcc_c_torture_src_pr44164_c|llvm_gcc_c_torture_src_pr52979_1_c|llvm_gcc_c_torture_src_pr52979_2_c|llvm_gcc_c_torture_src_pr57344_1_c|llvm_gcc_c_torture_src_pr57344_2_c|llvm_gcc_c_torture_src_pr57344_3_c|llvm_gcc_c_torture_src_pr57344_4_c|llvm_gcc_c_torture_src_pr58277_1_c|llvm_gcc_c_torture_src_pr58365_c|llvm_gcc_c_torture_src_pr60072_c|llvm_gcc_c_torture_src_pr69691_c|llvm_gcc_c_torture_src_pr70127_c|llvm_gcc_c_torture_src_pr71626_1_c|llvm_gcc_c_torture_src_pr71626_2_c|llvm_gcc_c_torture_src_pr78170_c|llvm_gcc_c_torture_src_pr79043_c|llvm_gcc_c_torture_src_pr79737_1_c|llvm_gcc_c_torture_src_pr82388_c|llvm_gcc_c_torture_src_strct_pack_2_c|llvm_gcc_c_torture_src_struct_cpy_1_c)$' >> test_after.log 2>&1
```

Result:
- Build passed.
- `ctest` subset passed: `55/55` tests.
- Focused 46-row BIR refresh found `7/46` BIR successes, `2/46` downstream scalar/local-memory stops, and `37/46` remaining store local-memory stops.

Proof log path after supervisor roll-forward: `test_before.log`.
