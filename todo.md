Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Repair the Next Store Producer Subfamily

# Current Packet

## Just Finished

Blocked Step 7 implementation before code changes because the delegated local aggregate subobject store producer target is already present for the representative local-only shapes.

Focused local-only probes for the classified subfamily already lower to `bir.store_local`:
- Nested local struct field stores such as `v.t.a = 1` lower into scalar leaf-slot stores.
- Packed/bitfield local struct field stores lower into load/mask/or/store sequences over the local leaf slots.

The reproduced remaining stop is the later aggregate-to-global copy handoff, not the local subobject store producer:

```c
struct T { char a,b,c,d; };
struct S { struct T t; } u;
void f(void) {
  struct S v = {0};
  v.t.a = 1;
  v.t.b = 2;
  v.t.c = 3;
  v.t.d = 4;
  u = v;
}
```

`build/c4cll --dump-bir --target riscv64-linux-gnu /tmp/local_agg_global_copy_probe.c` still reports `semantic lir_to_bir function 'f' failed in store local-memory semantic family`, while the same probe without `u = v` dumps BIR successfully. Implementing that handoff in this packet would cross the Step 7 boundary into aggregate/global-data store ownership.

## Suggested Next

Supervisor should not accept the local subobject-store implementation packet as currently scoped.

Smallest coherent next packet options:
- Reclassify Step 7 through plan-owner/reviewer because the Step 6 selected representatives are dominated by aggregate-to-global handoff after already-supported local subobject stores.
- If the supervisor approves crossing that boundary, delegate a new packet explicitly scoped to aggregate value copy/store to global/static storage, with `src/pr22141-1.c`, `src/compndlit-1.c`, and `src/pr57344-1.c` treated as aggregate/global handoff representatives rather than local subobject-store representatives.

## Watchouts

- Do not implement `u = v`, `x = foo(&i)`, `s[1] = t`, or `x = <clit>` under the current local aggregate subobject-store packet; those are aggregate/global handoff shapes on the current evidence.
- Keep the local-only subobject-store evidence separate from current full-row failures. The full rows can still report the broad `store local-memory semantic family` diagnostic even after their local subobject stores lower correctly.
- Exclude pointer/address-authority rows from the first implementation packet unless the code path naturally handles them without guessing authority: `src/20030913-1.c`, `src/alias-1.c`, `src/pr36343.c`, `src/pr36765.c`, `src/pr60072.c`, `src/pr69691.c`, and `src/pr79043.c` are representative pointer-dereference store rows.
- Exclude function-label/local pointer-array rows as a separate source/destination shape: `src/920501-5.c`, `src/990208-1.c`, `src/990525-1.c`, `src/pr71626-1.c`, and `src/pr71626-2.c`.
- Keep `src/pr39120.c` separate. The focused failing function is now `main` with `x = foo(&i)`, so it remains best treated as an aggregate-to-global handoff risk unless a later probe proves a first missing local subobject store inside the same function.
- Treat whole aggregate/static-global copy rows as downstream or adjacent-owner evidence until local subobject store production is repaired: examples include `src/20040707-1.c`, `src/930126-1.c`, `src/981130-1.c`, `src/991118-1.c`, `src/alias-access-path-1.c`, `src/lto-tbaa-1.c`, `src/pr44164.c`, `src/pr52979-1.c`, `src/pr52979-2.c`, `src/pr58365.c`, `src/pr70127.c`, `src/pr79737-1.c`, `src/pr82388.c`, and `src/struct-cpy-1.c`.
- Guard rows for the proposed Step 7 proof: prior Step 3 successes `src/20010605-2.c`, `src/20020413-1.c`, `src/strct-pack-2.c`; pointer/address guard `src/20030913-1.c`; function-label/local-array guard `src/920501-5.c`; aggregate/global handoff guard `src/pr39120.c`; non-store owner guards `src/20041124-1.c` for load, `src/20180921-1.c` for alloca, `src/20010924-1.c` for prepared/global data, and `src/20020118-1.c` for RV64/global data.
- Treat stale backend case logs under `build/rv64_gcc_c_torture_backend/` as historical evidence unless the supervisor refreshes the RV64 backend-object scan.
- Do not change expectations, unsupported markers, allowlists, runtime, timeout, accounting behavior, RV64 lowering, or adjacent owner routes.

## Proof

Blocked implementation packet; no build or CTest proof was run, and no `test_after.log` was created.

Transient probes used:
- `build/c4cll --dump-bir --target riscv64-linux-gnu /tmp/local_agg_store_probe.c`: local nested aggregate field stores dump BIR successfully.
- `build/c4cll --dump-bir --target riscv64-linux-gnu /tmp/local_bitfield_store_probe.c`: local packed/bitfield aggregate field stores dump BIR successfully.
- `build/c4cll --dump-bir --target riscv64-linux-gnu /tmp/local_agg_global_copy_probe.c`: adding the aggregate-to-global copy reproduces the remaining store-family stop.

No root-level logs were written.
