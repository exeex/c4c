Status: Active
Source Idea Path: ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Address-Exposed Local Or Pointer Lowering

# Current Packet

## Just Finished

Step 2 repaired the next `00004.c` owner layer: memory loads whose result is
the function return value now honor prepared `BeforeReturn` `FunctionReturnAbi`
placement by loading directly into the AArch64 return register (`w0` for i32)
before `ret`.

`src/backend/mir/aarch64/codegen/memory.cpp` now retargets selected load
results to the prepared return ABI register when the loaded value has a
matching `BeforeReturn` move bundle. This reuses the existing scalar-state
handoff: dispatch records the emitted memory result register, and return
lowering then returns through that emitted `w0` value.

`tests/backend/mir/backend_aarch64_return_lowering_test.cpp` adds focused
coverage for a frame-slot load whose storage home is non-ABI (`w19`) while the
prepared return move requires `FunctionReturnAbi` slot 0; the test proves the
load result and return record both use `w0`.

The earlier Step 2 address-exposed local repair remains preserved:

- `src/backend/mir/aarch64/codegen/memory.cpp` now resolves frame-slot memory
  operands against the prepared stack layout during real lowering, so distinct
  local slots use distinct `[sp, #offset]` addresses instead of all aliasing at
  `[sp]`.
- `src/backend/mir/aarch64/codegen/memory.cpp` rewrites `ptr %lv.*` local
  stores whose source is an address-taken stack object into a `FrameSlotOperand`
  source instead of using the local's scalar value register.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` prints that
  `FrameSlotOperand` store source by materializing the local frame-slot address
  through reserved MIR scratch `x9`, then storing `x9` to the destination slot.
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  covers the printable local-address store form and preserves the existing
  non-trivial immediate store `w9/x9` scratch materialization contract.

Generated `00004.c` assembly after this repair shows both the local pointer
repair and return-register handoff:

```asm
# build-aarch64-scan/c_testsuite_aarch64_backend/src/00004.c.s
movz w9, #4
str w9, [sp]
add x9, sp, #0
str x9, [sp, #8]
ldr x19, [sp, #8]
movz w9, #0
str w9, [sp]
ldr x19, [sp, #8]
ldr w0, [sp]
ret
```

```asm
# build-aarch64-scan/c_testsuite_aarch64_backend/src/00005.c.s
movz w9, #0
str w9, [sp]
add x9, sp, #0
str x9, [sp, #8]
add x9, sp, #8
str x9, [sp, #16]
ldr x20, [sp, #8]
ldr w20, [sp]
mov w0, #1
ret
```

The original address-exposed local owner remains repaired: `%lv.x`, `%lv.p`,
and `%lv.pp` no longer all alias at `[sp]`, and `p = &x` / `pp = &p` store
frame addresses rather than unrelated value registers. `00004.c` now passes the
AArch64 backend runtime route.

## Suggested Next

Localize or repair `00005.c` as a separate branch-control follow-up. It is not
the same return-handoff owner as `00004.c`: its pointer slot stores are correct,
but generated assembly still emits straight-line `mov w0, #1; ret` for the
first `if` path instead of branch control over the loaded condition. Keep
`00006.c` loop control out of scope unless the supervisor explicitly switches
to `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`.

Suggested focused proof command for the next repair packet:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not fold `00006.c` loop control into this plan; it remains owned by
  `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`.
- Do not weaken c-testsuite expectations or special-case `00004.c`/`00005.c`.
- The Step 2 address/pointer repair now has runtime pass evidence through
  `00004.c`; `00005.c` remains a branch-control follow-up, not the same
  return-handoff owner.
- The stale baseline candidate was rejected after the dispatch regression
  fix-back.
- Preserve reserved MIR scratch `x9` for both immediate materialization and
  local-address materialization; do not use indirect-call scratch `x16/x17`.

## Proof

Proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution|backend_aarch64_return_lowering)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004)_c$'; } 2>&1 | tee test_after.log
```

Result: PASS, command exited 0. The backend focused subset passed:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_records`,
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_memory_operand_contract`,
`backend_aarch64_operand_resolution`, and
`backend_aarch64_return_lowering`. The AArch64 backend runtime route passed
`00001.c`, `00002.c`, `00003.c`, and `00004.c`.
Proof log: `test_after.log`.
