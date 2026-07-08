Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local-Memory Store Production

# Current Packet

## Just Finished

Completed Step 3 broader-backend contract repair after the ordinary local `f128` store source-value change. `backend_lir_to_bir_notes` was still asserting the old unsupported boundary for `f128` scalar constants; the test now admits the generic scalar `f128` literal capability and verifies the lowered `BinaryInst` carries typed operands plus a structured `bir::Value::immediate_f128_bits(...)` payload.

The failure was a stale unit contract, not an implementation overreach. The existing scalar implementation remains generic to canonical `0xL<32 hex>` / `0x<32 hex>` `f128` immediates and still feeds the existing local-store producer path without testcase-shaped matching.

Confirmed contracts:
- `backend_lir_to_bir_notes` now verifies full-width `f128` literal materialization in scalar binops instead of requiring rejection through the old 64-bit immediate lane.
- `build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function main tests/c/external/gcc_torture/src/20010605-2.c` now succeeds and emits local `f128` stores for `%lv.z.0` and `%lv.z.16`.
- `build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function test tests/c/external/gcc_torture/src/20020413-1.c` now succeeds and emits `bir.store_local %lv.tmp, f128 0x00000000000000003FFF000000000000, addr %lv.tmp`, plus later computed `f128` stores.
- `build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function bar tests/c/external/gcc_torture/src/pr39120.c` still fails in `store local-memory semantic family`.
- `build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function main tests/c/external/gcc_torture/src/strct-pack-2.c` still fails in `store local-memory semantic family`.

The selected non-store guard rows and selected RV64 local-memory rows were included in the delegated CTest proof and stayed green.

## Suggested Next

Continue Step 3 with the distinct pointer/address-authority store producer repair for `src/pr39120.c` and `src/strct-pack-2.c`, starting from `try_lower_pointer_provenance_store(...)`, `try_lower_addressed_pointer_store(...)`, and the local-slot pointer/address tracking paths in `try_lower_local_slot_store(...)`.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Treat `src/pr39120.c` and `src/strct-pack-2.c` as probes for semantic destination/address repair, not implementation keys.
- Keep ordinary `f128` source-value support separate from pointer/address authority. The `f128` repair lives in generic scalar value lowering and already feeds existing local store production.
- The selected non-store guard rows remain excluded from Step 3 implementation ownership: `src/20041124-1.c` (load), `src/20030717-1.c` (GEP), `src/20180921-1.c` (alloca), `src/20000722-1.c` (prepared/RV64 local memory), `src/20000603-1.c` (ABI), `src/pr38533.c` (runtime), `src/20010924-1.c` (prepared/global-data), and `src/20020118-1.c` (RV64/global-data).
- Do not change tests, expectations, unsupported markers, allowlists, runtime, timeout, or accounting behavior.
- Preserve the completed load-semantics boundary from idea 602.

## Proof

Delegated proof passed and was written to `test_after.log`:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(20010605_2|20020413_1|pr39120|strct_pack_2|20041124_1|20030717_1|20180921_1|20000722_1|20000603_1|pr38533|20010924_1|20020118_1)_c)$' >> test_after.log 2>&1
```

Result: build passed; selected CTest subset passed 13/13, including `backend_lir_to_bir_notes` and the 12 selected RV64 rows.

Supervisor-side broader backend validation after the contract fix also passed:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: broader backend CTest subset passed 346/346.
