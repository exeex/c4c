Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Repair the Next Store Producer Subfamily

# Current Packet

## Just Finished

Completed Step 6 classification of the remaining visible local-memory store subfamily.

Starting from the Step 5 checkpoint, the current per-case logs still identify `46` artifact-derived rows whose first stop says `store local-memory semantic family`. The six known full-row BIR dump successes remain excluded from the next implementation target: `src/20010605-2.c`, `src/20020413-1.c`, `src/20040208-1.c`, `src/930526-1.c`, `src/ieee/inf-1.c`, and `src/strct-pack-2.c`.

Focused transient `build/c4cll --dump-bir --target riscv64-linux-gnu <row>` probes over the other `40` rows still stop in the store local-memory family. The next common in-scope subfamily is local aggregate subobject store production: scalar or address/function-label values stored into local compound-literal, local struct, local array, or nested field/index destinations before any later aggregate/global handoff.

Representative next-subfamily rows:
- `src/pr22141-1.c` and `src/pr22141-2.c`: local compound-literal nested struct field stores such as `<clit>.t.a = 1`, followed by a global aggregate copy.
- `src/compndlit-1.c`: local compound-literal field stores with scalar/select sources before `x = <clit>`.
- `src/pr57344-1.c` through `src/pr57344-4.c`: local packed/bitfield struct field stores before `s[1] = t`.

Failure grouping from the sampled HIR/BIR evidence:
- First missing store fact: production of `StoreLocalInst`-equivalent facts for stores into structured local aggregate subobjects, including field paths, nested field paths, local array elements, and bitfield-adjacent fields.
- Source-value shape: immediate scalar constants, selected scalar values, address/function-label constants, and local aggregate values already present as ordinary HIR expressions. This is not primarily another `f128` literal issue.
- Destination/address authority: structured local slots and local subobject paths dominate the next target; pointer-dereference rows remain separate because they need pointer/address authority rather than only local subobject store production.
- Aggregate/global-data handoff risk: many representatives later copy the initialized local aggregate into global/static storage, but the first Step 7 target should stop at the local subobject store producer and let any later aggregate/global copy remain an observable downstream owner.

## Suggested Next

Execute Step 7: repair local aggregate subobject store production.

Suggested narrow implementation target:
- Add the generic BIR local-memory store producer support needed for scalar/address stores into structured local aggregate subobjects.
- Use `src/pr22141-1.c`, `src/pr22141-2.c`, `src/compndlit-1.c`, and at least one `src/pr57344-*.c` row as representatives.
- Preserve the Step 3 repairs for `f128` local immediates, loaded global pointer-field authority, and dynamic local pointer-array stores.

Proposed supervisor proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(pr22141_1|pr22141_2|compndlit_1|pr57344_1|pr57344_2|20010605_2|20020413_1|strct_pack_2|20030913_1|920501_5|pr39120|20041124_1|20180921_1|20010924_1|20020118_1)_c)$' >> test_after.log 2>&1
```

## Watchouts

- Keep the Step 7 repair limited to BIR local aggregate subobject store production.
- Exclude pointer/address-authority rows from the first implementation packet unless the code path naturally handles them without guessing authority: `src/20030913-1.c`, `src/alias-1.c`, `src/pr36343.c`, `src/pr36765.c`, `src/pr60072.c`, `src/pr69691.c`, and `src/pr79043.c` are representative pointer-dereference store rows.
- Exclude function-label/local pointer-array rows as a separate source/destination shape: `src/920501-5.c`, `src/990208-1.c`, `src/990525-1.c`, `src/pr71626-1.c`, and `src/pr71626-2.c`.
- Keep `src/pr39120.c` separate. The focused failing function is now `main` with `x = foo(&i)`, so it remains best treated as an aggregate-to-global handoff risk unless a later probe proves a first missing local subobject store inside the same function.
- Treat whole aggregate/static-global copy rows as downstream or adjacent-owner evidence until local subobject store production is repaired: examples include `src/20040707-1.c`, `src/930126-1.c`, `src/981130-1.c`, `src/991118-1.c`, `src/alias-access-path-1.c`, `src/lto-tbaa-1.c`, `src/pr44164.c`, `src/pr52979-1.c`, `src/pr52979-2.c`, `src/pr58365.c`, `src/pr70127.c`, `src/pr79737-1.c`, `src/pr82388.c`, and `src/struct-cpy-1.c`.
- Guard rows for the proposed Step 7 proof: prior Step 3 successes `src/20010605-2.c`, `src/20020413-1.c`, `src/strct-pack-2.c`; pointer/address guard `src/20030913-1.c`; function-label/local-array guard `src/920501-5.c`; aggregate/global handoff guard `src/pr39120.c`; non-store owner guards `src/20041124-1.c` for load, `src/20180921-1.c` for alloca, `src/20010924-1.c` for prepared/global data, and `src/20020118-1.c` for RV64/global data.
- Treat stale backend case logs under `build/rv64_gcc_c_torture_backend/` as historical evidence unless the supervisor refreshes the RV64 backend-object scan.
- Do not change expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, RV64 lowering, or adjacent owner routes.

## Proof

Evidence-only classification; no build or CTest proof was required or run.

Transient probes used:
- `build/c4cll --dump-bir --target riscv64-linux-gnu <row>` across the `40` remaining visible rows.
- `build/c4cll --dump-hir --target riscv64-linux-gnu <representative-row>` for `src/20030913-1.c`, `src/20040707-1.c`, `src/920501-5.c`, `src/alias-1.c`, `src/alias-access-path-1.c`, `src/compndlit-1.c`, `src/pr22141-1.c`, `src/pr39120.c`, `src/pr57344-1.c`, `src/pr58277-1.c`, `src/pr71626-1.c`, `src/pr71626-2.c`, and `src/struct-cpy-1.c`.

No root-level logs were written. Existing `test_after.log` was not present in this workspace snapshot, so no canonical proof log was modified.
