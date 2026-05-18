Status: Active
Source Idea Path: ideas/open/282_aarch64_loop_branch_control_runtime_hang.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Runtime And Regression Coverage

# Current Packet

## Just Finished

Steps 3 and 4 are proved after commit `f0b415a88` (`Print AArch64 fused
compare branches`). The same fused-compare branch repair that fixed the
`00005.c` first-`if` fallthrough also resolves the `00006.c` loop-control hang:
the loop now has a conditional exit edge instead of an unconditional self-loop.

Generated `00006.c` assembly evidence:

```asm
movz w9, #50
str w9, [sp]
b .LBB1_2
.LBB1_2:
ldr w13, [sp]
cmp w13, #0
b.ne .LBB1_3
b .LBB1_4
.LBB1_3:
ldr w13, [sp]
sub w19, w13, #1
str w19, [sp]
b .LBB1_2
.LBB1_4:
ldr w0, [sp]
ret
```

Changed files:
`todo.md` and `test_after.log`.

## Suggested Next

Idea 282 is ready for Step 5 plan-owner closure review. The focused backend
tests are green, `00001.c` through `00006.c` are green through the AArch64
backend runtime route, and no residual blocker is known in this plan scope.

## Watchouts

- Do not reopen idea 281 address-exposed pointer storage unless trace evidence
  proves the exact owner returned.
- Do not weaken c-testsuite expectations, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy.
- Do not add filename-specific handling for `00005.c` or `00006.c`.
- Preserve prior green routes for `00001.c`, `00002.c`, `00003.c`, and
  `00004.c`; `00005.c` and `00006.c` are now part of the green route.
- Do not weaken fused-compare branch records into unsupported forms. This
  packet made the selected supported form printable and covered the promoted
  `i32` branch carrier shape.
- Closure review should still check that idea 282 is only claiming the branch
  and loop-control family covered by the focused proof, not broader AArch64
  backend completeness.

## Proof

Step 3/4 proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c$'; } 2>&1 | tee test_after.log
```

Result: PASS, exit 0. The seven focused backend tests passed, and
`c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c` all
passed through the AArch64 backend runtime route. `test_after.log` contains the
full command output.
