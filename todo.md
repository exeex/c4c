Status: Active
Source Idea Path: ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Address-Exposed Local Ownership

# Current Packet

## Just Finished

Step 1 localized the `00004.c` and `00005.c` AArch64 backend
`[RUNTIME_NONZERO] exit=1` failures to the prepared memory/address-exposed
local pointer lowering path.

Both failures are in the same owner family, with two concrete sub-faults:

- `src/backend/mir/aarch64/codegen/memory.cpp::make_store_memory_instruction_record()`
  and the `StoreLocalInst` prepared path treat `ptr %lv.x` / `ptr %lv.p`
  stores as ordinary scalar value stores. For `p = &x` and `pp = &p`, the
  backend stores the current value register (`x13` / `x19`) instead of
  materializing and storing the address of the addressed frame slot.
- `src/backend/mir/aarch64/codegen/memory.cpp::make_memory_record_from_prepared_access()`
  plus `memory_address()` preserve only the prepared access byte offset within
  a slot. `memory_address()` then prints every frame-slot access as `[sp]` or
  `[sp, #byte_offset]`, while ignoring the prepared `frame_slot_order` base
  offsets. `src/backend/mir/aarch64/codegen/machine_printer.cpp::print_memory()`
  faithfully prints that already-incomplete `MemoryOperand`.

Generated assembly evidence:

```asm
# build-aarch64-scan/c_testsuite_aarch64_backend/src/00004.c.s
movz w9, #4
str w9, [sp]
str x13, [sp]
ldr x19, [sp]
movz w9, #0
str w9, [sp]
ldr x19, [sp]
ldr w19, [sp]
ret
```

`00004.c` prepared facts put `%lv.x` in slot #0 at offset 0 and `%lv.p` in
slot #1 at offset 8, but both the integer store and pointer store print
`[sp]`. The `p = &x` store prints `str x13, [sp]` instead of storing the frame
address for `%lv.x` into `%lv.p`'s slot.

```asm
# build-aarch64-scan/c_testsuite_aarch64_backend/src/00005.c.s
movz w9, #0
str w9, [sp]
str x13, [sp]
str x19, [sp]
ldr x20, [sp]
ldr w20, [sp]
mov w0, #1
ret
```

`00005.c` prepared facts put `%lv.x` in slot #0 at offset 0, `%lv.p` in slot
#1 at offset 8, and `%lv.pp` in slot #2 at offset 16, but all three stores
print `[sp]`. The `p = &x` and `pp = &p` stores also use the local value
registers rather than frame-slot addresses.

## Suggested Next

Execute Step 2 by repairing the AArch64 prepared memory path for address-exposed
locals. The focused repair should make frame-slot memory operands include the
prepared frame-slot base offset and make local-address pointer stores
materialize the addressed local's frame-slot address before storing it. If the
supervisor wants a smaller split, repair frame-slot offset resolution first,
then address materialization for `ptr %lv.*` stores second.

Suggested Step 2 proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution|backend_aarch64_return_lowering)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00004|00005)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not fold `00006.c` loop/branch control into this packet; it belongs to
  `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`.
- Do not special-case c-testsuite filenames or exact emitted instruction text.
- Preserve the idea 278 local immediate store/load repair and reserved MIR
  scratch register use.
- `00005.c` also showed a prepared dump discrepancy where stack layout reports
  `frame_size=32` while frame plan reports `frame_size=24`; the first observed
  runtime owner is still the all-slots-alias-at-`[sp]` memory operand path.

## Proof

Proof command:

```sh
set -o pipefail; { cmake --build --preset default && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00004|00005)_c$'; } 2>&1 | tee test_after.log
```

Result: FAIL, command exited 8. The build/configure steps completed, then
`c_testsuite_aarch64_backend_src_00004_c` and
`c_testsuite_aarch64_backend_src_00005_c` both failed as
`[RUNTIME_NONZERO] exit=1`. Proof log: `test_after.log`.
