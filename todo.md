# Current Packet

Status: Active
Source Idea Path: ideas/open/367_semantic_bir_indirect_local_memory_lvalue_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Semantic Admission

## Just Finished

Step 3 repaired the remaining semantic admission leaf for casted byte-pointer
local memory. Dynamic local byte-array GEPs and dynamic pointer-value GEPs now
publish a computed pointer address for the GEP result, so wider scalar load/store
through a computed `i8*` lowers as pointer-value `MemoryAddress` operations
instead of being rejected as a same-element `i8` array selection. The addressed
byte-pointer scalar admission rule also accepts opaque addressed `i8` pointer
bases at offset zero for wider scalar access.

The focused `loaded_pointer_addressed_store` and
`casted_byte_pointer_i32_update` contracts now both pass. `00005` and `00173`
remain passing. `00217` advanced past semantic handoff and now fails later as
an AArch64 runtime mismatch.

## Suggested Next

Supervisor should route the new `00217` runtime residual to the AArch64
prepared/MIR side: semantic BIR admission is no longer the first bad fact.

## Watchouts

- Keep AArch64/runtime/runner/expectation changes out of scope.
- Do not treat the current local backend-route snippet drift as progress for
  this semantic admission owner.
- Preserve the completed `00173` pointer-derived load/publication path.
- `00173` remained passing in the Step 3 proof.
- `00005` now passes; keep the `.store.addr` scratch separation from regressing
  when adjusting adjacent pointer-value memory paths.
- New `00217` first bad fact after this packet: AArch64 runtime output is
  `data = "0123"` but expected `data = "0123-5678"`.
- Do not broaden this semantic owner into AArch64 instruction selection or
  runtime-output repair.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_(00005|00173|00217)_c)$' | tee test_after.log
```

Result: build succeeded. `backend_lir_to_bir_notes`, `00005`, and `00173`
passed. `00217` advanced past semantic handoff and failed with an AArch64
runtime mismatch:

- expected: `data = "0123-5678"`
- actual: `data = "0123"`

Proof log: `test_after.log`.
