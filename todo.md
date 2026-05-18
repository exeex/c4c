Status: Active
Source Idea Path: ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Address-Exposed Local Or Pointer Lowering

# Current Packet

## Just Finished

Step 2 fix-back repaired the `backend_aarch64_instruction_dispatch` regression
introduced by the address-exposed local pointer repair while preserving the
semantic repair.

`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` now gives the
affected dispatch load/store fixtures explicit prepared stack-layout slot
snapshots. That matches the implementation contract added in the local pointer
repair: real AArch64 memory lowering now consumes authoritative prepared
frame-slot offsets before selecting printable memory nodes. The fixture update
does not weaken the expected selected load/store contract; it supplies the
authority the implementation now requires.

The preserved Step 2 repair remains:

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

Generated assembly evidence after the fix-back still shows the local pointer
repair:

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
ldr w19, [sp]
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
frame addresses rather than unrelated value registers.

## Suggested Next

Localize the next owner layer exposed by the Step 2 repair. `00004.c` now loads
the final value into `w19` and reaches `ret` without materializing that return
value into `w0`. `00005.c` now has correct pointer slot stores, but still emits
straight-line `mov w0, #1; ret` for the first `if` path instead of branch
control over the loaded condition. If the supervisor wants the smallest split,
localize/repair the `00004.c` load-result return materialization first, then
keep `00005.c` branch control separate from this address-exposed-local slice.

Suggested focused proof command for the next repair packet:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not fold `00006.c` loop control into this plan; it remains owned by
  `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`.
- Do not weaken c-testsuite expectations or special-case `00004.c`/`00005.c`.
- The Step 2 address/pointer repair is not proved by runtime pass evidence yet:
  it is proved by focused backend tests and generated assembly inspection, while
  runtime still fails at the next owner layer.
- The stale baseline candidate was rejected after the dispatch regression
  fix-back; the delegated proof still exits nonzero only because `00004.c` and
  `00005.c` expose next owner runtime failures.
- Preserve reserved MIR scratch `x9` for both immediate materialization and
  local-address materialization; do not use indirect-call scratch `x16/x17`.

## Proof

Proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution|backend_aarch64_return_lowering)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005)_c$'; } 2>&1 | tee test_after.log
```

Result: FAIL, command exited 8. The backend focused subset passed:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_records`,
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_memory_operand_contract`,
`backend_aarch64_operand_resolution`, and
`backend_aarch64_return_lowering`. The AArch64 backend runtime route passed
`00001.c`, `00002.c`, and `00003.c`; `00004.c` and `00005.c` still failed
`[RUNTIME_NONZERO] exit=1` at the next owner layers described above.
Proof log: `test_after.log`.
