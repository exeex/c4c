Status: Active
Source Idea Path: ideas/open/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate the Shared Address Authority Break

# Current Packet

## Just Finished

Step 1 investigation completed for `src/00217.c`, `src/00032.c`,
`src/00130.c`, and `src/00180.c`; no implementation files were changed.

First shared break: pointer-derived address authority is lost before final
AArch64 printing. The semantic/prepared BIR route tracks local/global pointer
provenance in side maps such as `local_slot_pointer_values_`,
`local_slot_address_slots_`, `local_pointer_slot_addresses_`,
`pointer_value_addresses_`, and `global_pointer_slots_`, but those facts are
not consistently published as concrete pointer values or address
materializations for value consumers. The result is BIR with named pointer
values that have no defining address-producing instruction, followed by
prealloc/AArch64 selecting fallback spill slots or stale registers.

Generated-assembly symptoms:

- `00032.c`: `p = &arr[0]` and later pointer increments store `p` from
  out-of-frame spill slots such as `[sp, #64]`, `[sp, #72]`, `[sp, #56]`,
  or from stale registers (`x13`, `x19`) instead of `add xN, sp, #arr`.
  Prepared BIR already has `bir.store_local %lv.p, ptr %t5` and similar
  `%t7/%t13/...` pointer values with no visible defining address value.
- `00130.c`: `p = arr` and `q = &arr[1][3]` store pointer locals from
  uninitialized/stale registers (`x13`, `x20`). The direct scalar
  `arr[1][3]` slot exists, but pointer-valued consumers do not receive the
  local subobject address.
- `00180.c`: `strcpy(a, "abcdef")` passes `x20` as `a`, but `x20` is only
  saved/restored and never materialized from `sp`; `printf("%s", &a[1])`
  loads `w1` from the saved-register spill area instead of passing
  `sp + 1`. Prepared BIR/call plans treat `%lv.a.0` and `%t5` as call
  argument pointer values without a local-frame address materialization.
- `00217.c`: `char *data = t` is stored from an uninitialized register, and
  the later `printf(..., data)` reloads that bad pointer. The compound
  update also collapses the lvalue to a direct `@t+4` access in BIR/assembly
  rather than preserving the runtime pointer-derived address through
  load-compute-store.

## Suggested Next

Execute the first implementation packet against the semantic-BIR to prealloc
address-publication boundary.

Smallest first owner: local frame-slot and local subobject address publication
for pointer-valued consumers. Likely touched surfaces are
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`,
`src/backend/bir/lir_to_bir/memory/addressing.cpp`,
`src/backend/bir/lir_to_bir/memory/provenance.cpp`, and, if the selected
repair publishes explicit prepared materializations,
`src/backend/prealloc/addressing.hpp`,
`src/backend/prealloc/stack_layout/coordinator.cpp`, and
`src/backend/mir/aarch64/codegen/globals.cpp`.

Exact proof command for that packet:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00032|00130|00180|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

## Watchouts

- Do not repair this by naming the four tests or rewriting expectations. The
  address fact must become an authoritative value/materialization that ordinary
  load/store/call consumers can use.
- The starters may split after the local-address repair: `00032.c`,
  `00130.c`, and `00180.c` are primarily local frame-slot/subobject address
  publication; `00217.c` also exercises global pointer provenance and
  compound lvalue update preservation.
- If the first local-address packet leaves `00217.c` failing only in the
  `data + r` compound update path, route the next packet to global/runtime
  `PointerAddress` propagation through load-compute-store rather than widening
  the local frame-slot patch.
- Generated assembly must be inspected after the implementation packet. The
  expected disappearance is uninitialized/stale pointer sources and
  out-of-frame pointer reloads, replaced by `sp`-relative local addresses,
  global symbol addresses, or pointer-register based memory operands as
  appropriate.

## Proof

Reused supervisor-provided `test_before.log`; no new proof command was run and
no `test_after.log` was produced because this packet was investigation-only.
`test_before.log` shows all four starter tests failing:

- `00032.c`: runtime nonzero, `exit=1`.
- `00130.c`: runtime nonzero, `exit=1`.
- `00180.c`: runtime nonzero, `exit=Segmentation fault`.
- `00217.c`: runtime mismatch; expected `data = "0123-5678"`, actual output
  uses a corrupted `data` pointer.

Generated assembly was inspected under
`build-aarch64-scan/c_testsuite_aarch64_backend/src/`. Stale-process check:
`ps -eo pid,ppid,stat,comm,args | rg '(ctest|c4cll|qemu|build-aarch64-scan)'`
found no stale `ctest`, `c4cll`, `qemu`, or build/test process beyond the
check command itself.
