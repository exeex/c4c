Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Reprove Store-Family Breadth After Any Follow-Up Repair

# Current Packet

## Just Finished

Completed Step 8 implementation for aggregate/vector typed stores into local frame aggregate destinations.

Implementation surface:
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- Added semantic vector store support for a single-lane SSA vector value into local vector lane slots, while preserving existing zero-initializer vector stores.
- Added dynamic local aggregate array element copy support for aggregate source aliases. The repair copies each scalar aggregate leaf into the selected local aggregate array element, using the existing dynamic index to preserve unselected elements.

Progressed rows:
- `src/990525-1.c` now dumps BIR. The old `die` store local-memory producer stop is repaired for by-value aggregate source copied into `struct blah buf[1]`.
- `src/pr71626-1.c` progressed beyond the old store local-memory producer stop and now reaches a later `scalar/local-memory semantic family` boundary.
- `src/pr71626-2.c` progressed beyond the old store local-memory producer stop and now reaches the same later `scalar/local-memory semantic family` boundary.

Preserved guards:
- Current BIR-success store rows in the proof subset stayed green: `src/20010605-2.c`, `src/20020413-1.c`, and `src/strct-pack-2.c`.
- Pointer/address guard `src/20030913-1.c`, function-label/local-array guards `src/920501-5.c` and `src/990208-1.c`, aggregate/global handoff guards `src/pr22141-1.c`, `src/pr57344-1.c`, and `src/pr39120.c`, and non-store guards `src/20041124-1.c`, `src/20180921-1.c`, `src/20010924-1.c`, and `src/20020118-1.c` stayed on their expected test outcomes.

## Suggested Next

Run a short Step 9 evidence pass over the remaining visible store-family rows after this repair.

Recommended decision point:
- If only the singleton local union/overlap row `src/931102-2.c` remains in-scope for idea 603, either delegate a narrow singleton repair packet or park idea 603 and route the larger remaining population to pointer/address authority, function-label/local pointer-array, aggregate/global handoff idea 619, or existing global-data owners.
- Keep `src/pr71626-1.c` and `src/pr71626-2.c` out of the next local-store packet unless the later scalar/local-memory boundary is proven to be another local-memory store producer issue.

## Watchouts

- The dynamic aggregate copy path is intentionally local-frame only. It does not implement aggregate/static global handoff rows such as `u = v`, `s[1] = t`, `x = foo(&i)`, or `x = <clit>`.
- The vector store repair is deliberately limited to single-lane SSA vector stores into local vector slots. Multi-lane non-zero vector stores still need a lane-source fact before they should be accepted.
- `src/931102-2.c` still fails in `store local-memory semantic family` and remains the local union/overlap watchout.
- Pointer/address rows remain adjacent unless a later packet proves a store producer failure without guessing address authority.
- Function-label/local pointer-array rows remain adjacent; the `pr71626` rows no longer stop at the local vector store producer.
- `src/pr39120.c` remains an aggregate/global handoff guard after the earlier `bar` pointer-store boundary moved.
- Treat stale backend case logs under `build/rv64_gcc_c_torture_backend/` as historical evidence unless the supervisor refreshes the RV64 backend-object scan.
- Do not change expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, RV64 lowering, or adjacent owner routes.

## Proof

Delegated Step 8 proof was produced as `test_after.log`, accepted, and rolled forward to current `test_before.log`:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(990525_1|pr71626_1|pr71626_2|931102_2|20010605_2|20020413_1|strct_pack_2|20030913_1|920501_5|990208_1|pr22141_1|pr57344_1|pr39120|20041124_1|20180921_1|20010924_1|20020118_1)_c)$' >> test_after.log 2>&1
```

Result:
- Build passed.
- `ctest` subset passed: `18/18` tests.
- Focused post-build probes confirmed `src/990525-1.c` dumps BIR, while `src/pr71626-1.c` and `src/pr71626-2.c` progress from `store local-memory semantic family` to a later `scalar/local-memory semantic family` boundary.

Supervisor-side broader backend validation also passed:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: `346/346` tests passed.

Current rolled-forward proof log path: `test_before.log`.
