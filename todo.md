Status: Active
Source Idea Path: ideas/open/283_aarch64_for_loop_control_runtime_hang.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Supported For-Loop Control Lowering

# Current Packet

## Just Finished

Step 2 repaired the supported AArch64 `for`/loop-control lowering owner, then
fixed back the reviewer-found contract regression in the prepared branch-record
boundary. The original loop-control root cause was dispatch comparing prepared
control-flow target IDs directly against retained BIR terminator label IDs.
Those IDs can come from different name tables; loop-generated labels exposed
the mismatch, so loop-header conditional branches and some related loop
unconditional edges failed record validation and silently emitted no terminator
instruction.

The accepted fix keeps prepared control-flow authority as the owner while
preserving the raw prepared branch-record fail-closed contract. Dispatch-time
lowering canonicalizes retained BIR terminator targets to prepared labels when
their spelling matches; `make_prepared_conditional_branch_record()` itself once
again requires structured target IDs to match exactly and rejects genuinely
mismatched records with `TerminatorTargetMismatch`.

Focused backend coverage now includes divergent prepared/BIR label-ID cases for
both an unconditional branch and a `for.cond.1` fused-compare loop-header
conditional branch. The loop-header case proves selection of
`MachineOpcode::CompareBranch`, prepared true/false successors, and printed
`cmp` plus branch lines.

Generated `00007.c` AArch64 now emits truthful branch control at both loop
headers. Representative repaired assembly:

```asm
.LBB1_2:
ldr w13, [sp]
cmp w13, #0
b.ne .LBB1_3
b .LBB1_4
...
.LBB1_8:
ldr w13, [sp]
cmp w13, #0
b.ne .LBB1_9
b .LBB1_10
```

Current fix-back changed files: `src/backend/mir/aarch64/codegen/comparison.cpp`,
`todo.md`, and `test_after.log`. The existing
`tests/backend/mir/backend_aarch64_prepared_branch_records_test.cpp` mismatch
case and the loop-header divergent-label dispatch coverage both remain green.

## Suggested Next

Proceed to the next plan step: prove nearby runtime/regression coverage or move
idea 283 toward closure review, depending on the plan-owner route.

## Watchouts

- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy.
- Do not add filename-specific handling for `00007.c`.
- Preserve the already-green AArch64 backend runtime route for `00001.c`
  through `00006.c`.
- Do not reopen completed scalar local, address-exposed pointer storage, or
  `00005.c`/`00006.c` fused-compare branch repairs unless their exact old
  owner-layer failures return.
- Keep timeout-bounded runtime proof for loop-control cases if this route is
  broadened again; do not treat hangs as pass evidence.

## Proof

Step 2 proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && timeout 30s ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006|00007)_c$'; } 2>&1 | tee test_after.log
```

Earlier Step 2 result: PASS, exit 0. The seven focused backend tests passed, and
`c_testsuite_aarch64_backend_src_00001_c` through
`c_testsuite_aarch64_backend_src_00007_c` all passed through the AArch64 backend
runtime route within the delegated 30 second timeout. `00007.c` no longer hangs.

Fix-back proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_branch_records|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && timeout 30s ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006|00007)_c$'; } 2>&1 | tee test_after.log
```

Fix-back result: PASS, exit 0. The eight focused backend tests passed,
including `backend_aarch64_prepared_branch_records` and
`backend_aarch64_branch_control_lowering`; `c_testsuite_aarch64_backend_src_00001_c`
through `c_testsuite_aarch64_backend_src_00007_c` all passed through the
AArch64 backend runtime route within the delegated 30 second timeout.
