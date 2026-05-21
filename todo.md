Status: Active
Source Idea Path: ideas/open/372_aarch64_pointer_valued_subobject_address_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Pointer-Local Address Publication

# Current Packet

## Just Finished

Executed idea 372 Steps 2 and 3 as one implementation packet.

Coverage added:

- `backend_aarch64_instruction_dispatch` now has a focused
  pointer-local reassignment fixture where a pointer local first receives one
  address, then receives a distinct computed global-subobject address, and a
  later reload/dereference must observe the reassigned address.

Repair:

- BIR lowering for AArch64 now publishes nonzero global/subobject GEP pointer
  values as explicit pointer `add` values instead of only recording them in
  global-address side tables. This gives later scalar pointer-local stores a
  real `%t34 = bir.add ptr @global, offset` source instead of an anonymous
  side-table-only address.
- AArch64 `StoreLocal` publication now materializes direct global pointer
  values and `pointer_base_plus_offset` computed-address homes into a scratch
  register and stores that address into the pointer local home before later
  reload/dereference consumers.

Representative result: `00163` passes. Generated `00163.c.s` now materializes
the final `&(bolshevic.b)` assignment as `adrp/add bolshevic`, `add x13, x20,
#4`, then `str x13, [sp, #8]`; the final reload uses `[sp, #8]` and dereferences
the stored global-subobject pointer, so the old `b = &a` value is no longer
consumed.

## Suggested Next

Execute Step 4 narrow proof/classification for idea 372: rerun the
representative `00163` proof from the committed baseline, inspect the final
assembly publication shape, and record whether any residual remains. Supervisor
still owns broader backend-regex/full-suite guard before closure.

## Watchouts

The repair is semantic, not `00163`-shaped: it covers pointer-valued
global/subobject address publication into scalar pointer locals and leaves the
external-call return, static selected arrays, aggregate initializer, FP,
bit-field, timeout, and idea 371 selected-aggregate buckets untouched. Closed
ideas 294/355 remain adjacent archive context only.

## Proof

Delegated Steps 2/3 command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00163_c)$' > test_after.log 2>&1
```

Result: build completed; CTest selected 144 tests and all 144 passed,
including local `backend_.*` coverage and
`c_testsuite_aarch64_backend_src_00163_c`. Proof log is `test_after.log`.
