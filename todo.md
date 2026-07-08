Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Repair the Next In-Scope Store Producer Subfamily

# Current Packet

## Just Finished

Completed revised Step 7 evidence classification after aggregate/static global handoff was split to idea 619.

Fresh transient `--dump-bir --target riscv64-linux-gnu` probes over the `46` artifact-derived store-family rows found:
- `6` current BIR successes/progressed rows: `src/20010605-2.c`, `src/20020413-1.c`, `src/20040208-1.c`, `src/930526-1.c`, `src/ieee/inf-1.c`, and `src/strct-pack-2.c`.
- `40` rows still report `store local-memory semantic family`.

Remaining row classification:
- In-scope idea 603 subfamily: aggregate/vector typed stores into local frame aggregate destinations. Representatives: `src/990525-1.c` stores a by-value `struct blah` argument into local `struct blah buf[1]`; `src/pr71626-1.c` and `src/pr71626-2.c` store a `<1 x i64>` function result into a local vector slot. These are local-frame store producer gaps, not aggregate/global handoffs.
- In-scope singleton/watchout: `src/931102-2.c` still fails on a local union/nested struct scalar field store (`reg.b.l = x`). It is local-memory owned, but it is not enough by itself to drive the next implementation packet.
- Pointer/address-authority rows: `src/20030913-1.c`, `src/930719-1.c`, `src/alias-1.c`, `src/alias-access-path-1.c`, `src/pr15262-2.c`, `src/pr36343.c`, `src/pr36765.c`, `src/pr58277-1.c`, `src/pr60072.c`, `src/pr69691.c`, and `src/pr79043.c`.
- Function-label/local pointer-array rows: `src/920501-5.c` and `src/990208-1.c`. `src/pr71626-1.c` and `src/pr71626-2.c` contain a function-label value source in `foo`, but that local vector construction already dumps BIR in isolation; the current full-row stop is the local vector aggregate store in `main`.
- Aggregate/static global handoff rows for idea 619 or existing global-data routes: `src/20040707-1.c`, `src/20131127-1.c`, `src/930126-1.c`, `src/981130-1.c`, `src/991118-1.c`, `src/compndlit-1.c`, `src/lto-tbaa-1.c`, `src/pr22141-1.c`, `src/pr22141-2.c`, `src/pr39120.c`, `src/pr44164.c`, `src/pr52979-1.c`, `src/pr52979-2.c`, `src/pr57344-1.c`, `src/pr57344-2.c`, `src/pr57344-3.c`, `src/pr57344-4.c`, `src/pr58365.c`, `src/pr70127.c`, `src/pr78170.c`, `src/pr79737-1.c`, `src/pr82388.c`, and `src/struct-cpy-1.c`.
- Non-store guard owners for any next proof: load `src/20041124-1.c`, alloca `src/20180921-1.c`, prepared/global data `src/20010924-1.c`, RV64/global data `src/20020118-1.c`, plus the six current BIR-success store rows listed above.

## Suggested Next

Execute Step 8 for the aggregate/vector typed local-store subfamily.

Suggested implementation target:
- Add generic BIR local-memory store producer support for aggregate/vector typed stores into local frame aggregate destinations.
- Primary representatives: `src/990525-1.c`, `src/pr71626-1.c`, and `src/pr71626-2.c`.
- Treat `src/931102-2.c` as a local union/overlap watchout, not the acceptance driver.
- Do not implement aggregate/static global handoff rows under idea 603; keep those routed to idea 619 or existing global-data ideas.

Proposed supervisor proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(990525_1|pr71626_1|pr71626_2|931102_2|20010605_2|20020413_1|strct_pack_2|20030913_1|920501_5|990208_1|pr22141_1|pr57344_1|pr39120|20041124_1|20180921_1|20010924_1|20020118_1)_c)$' >> test_after.log 2>&1
```

## Watchouts

- Keep the Step 8 repair scoped to local-frame aggregate/vector typed stores. It should not lower `u = v`, `s[1] = t`, `x = foo(&i)`, `x = <clit>`, or static/global aggregate stores.
- Preserve the earlier evidence that nested local scalar field stores and packed/bitfield local scalar field stores already lower in local-only probes.
- Pointer/address rows remain adjacent unless a Step 8 code path handles them without guessing authority.
- Function-label/local pointer-array rows remain adjacent; do not treat `920501-5.c` or `990208-1.c` as proof for aggregate/vector typed local stores.
- `src/pr39120.c` remains an aggregate/global handoff guard after the earlier `bar` pointer-store boundary moved.
- Treat stale backend case logs under `build/rv64_gcc_c_torture_backend/` as historical evidence unless the supervisor refreshes the RV64 backend-object scan.
- Do not change expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, RV64 lowering, or adjacent owner routes.

## Proof

Evidence-only classification; no build or CTest proof was required or run.

Transient probes used:
- Fresh `build/c4cll --dump-bir --target riscv64-linux-gnu <row>` probes across the `46` artifact-derived store-family rows.
- Focused `build/c4cll --dump-hir --target riscv64-linux-gnu <row>` probes to inspect failing functions and store shapes.
- Focused local probes for `struct` field stores, packed/bitfield stores, local union subfield stores, local aggregate array stores from by-value parameters, local vector function-label construction, and aggregate/global handoff.

No root-level logs were written.
