Status: Active
Source Idea Path: ideas/open/282_aarch64_loop_branch_control_runtime_hang.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Supported Conditional Branch Lowering

# Current Packet

## Just Finished

Step 2 repaired supported fused-compare conditional branch lowering for the
`00005.c` first-`if` owner. Lowering now accepts the prepared no-target-facing
`i1` form where the branch condition carrier is promoted to `i32`, resolves
the fused compare operands into printable register/immediate operands, and the
terminal AArch64 printer emits `cmp` plus true/false branch control instead of
dropping the selected `CompareBranch` record.

Generated `00005.c` assembly now preserves the prior local pointer repair and
contains truthful first-`if` branch control:

```asm
ldr x20, [sp, #8]
ldr w20, [sp]
cmp w20, #0
b.ne .LBB1_2
b .LBB1_3
```

Changed files:
`src/backend/mir/aarch64/codegen/comparison.cpp`,
`src/backend/mir/aarch64/codegen/instruction.cpp`,
`src/backend/mir/aarch64/codegen/machine_printer.cpp`,
`tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`,
`todo.md`, and `test_after.log`.

## Suggested Next

Execute the next Step 3 packet from `plan.md`: localize the remaining
`00006.c` loop-control owner. Start from the now-green `00005.c` branch route
and inspect whether `00006.c` is still the unconditional self-loop/control-edge
owner described by the active idea. A focused localization proof command should
be:

```sh
set -o pipefail; { cmake --build --preset default && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00006_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not reopen idea 281 address-exposed pointer storage unless trace evidence
  proves the exact owner returned.
- Do not weaken c-testsuite expectations, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy.
- Do not add filename-specific handling for `00005.c` or `00006.c`.
- Use timeout-bounded runtime proof or assembly-only localization while the
  `00006.c` hang remains active.
- Preserve prior green routes for `00001.c`, `00002.c`, `00003.c`, and
  `00004.c`; `00005.c` is now part of the green route.
- Do not weaken fused-compare branch records into unsupported forms. This
  packet made the selected supported form printable and covered the promoted
  `i32` branch carrier shape.
- `00006.c` remains the next loop-control owner; do not fold it into this
  completed `00005.c` first-`if` repair slice.

## Proof

Step 2 proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005)_c$'; } 2>&1 | tee test_after.log
```

Result: PASS, exit 0. The seven focused backend tests passed, and
`c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005)_c` all passed
through the AArch64 backend runtime route. `test_after.log` contains the full
command output.
