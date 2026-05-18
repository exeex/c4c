Status: Active
Source Idea Path: ideas/open/283_aarch64_for_loop_control_runtime_hang.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize For-Loop Control Ownership

# Current Packet

## Just Finished

Step 1 localized the `00007.c` `for`/loop-control runtime hang. The delegated
timeout-bounded proof exits 124, and `timeout` cleaned up the runtime process;
no lingering `00007.c.bin`, runner, or CTest process remained.

BIR and prepared control flow preserve the loop condition and exit edges. The
first loop is represented as:

```text
entry -> for.cond.1
for.cond.1: cond_br %t1, block_1, block_2
block_1 -> for.latch.1
for.latch.1 -> for.cond.1
block_2 -> post-loop check
```

Prepared branch facts also retain the fusable compare:

```text
branch_condition for.cond.1 kind=fused_compare condition=%t1
  compare=ne i32 %t0, 0 can_fuse_with_branch=yes true=block_1 false=block_2
```

The generated AArch64 assembly loses that condition at the loop header. It
labels the loop header, emits a condition load, then falls through into the
latch decrement and branches back unconditionally:

```asm
.LBB1_2:
ldr w13, [sp]
ldr w13, [sp]
sub w19, w13, #1
str w19, [sp]
b .LBB1_2
```

The second `for (;x;)` loop has the same shape:

```asm
.LBB1_8:
ldr w13, [sp]
b .LBB1_8
```

Likely owner: AArch64 MIR block/terminator lowering around
`src/backend/mir/aarch64/codegen/dispatch.cpp::dispatch_prepared_block`,
`src/backend/mir/aarch64/codegen/comparison.cpp::lower_prepared_conditional_branch_terminator`,
and machine block label/edge printing through
`src/backend/mir/printer.hpp::print_machine_function`. The prepared facts are
correct; the failure appears after prepared control-flow selection, where a
loop-header conditional terminator is not emitted/printed as a `cmp` plus
true/false branches and physical fallthrough reaches the latch/body block. This
is a backend loop/control lowering owner layer, not runtime infrastructure,
frontend BIR, c-testsuite expectations, or the already repaired scalar local /
addressed-local / fused-compare primitive branch support.

## Suggested Next

Execute Step 2 from `plan.md`: repair loop-header conditional branch emission
so supported `for`/loop-control blocks cannot fall through into their latch or
body without the prepared condition edge. Suggested focused proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && timeout 30s ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006|00007)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Use timeout-bounded runtime proof or assembly-only localization while the
  `00007.c` hang remains active.
- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy.
- Do not add filename-specific handling for `00007.c`.
- Preserve the already-green AArch64 backend runtime route for `00001.c`
  through `00006.c`.
- Do not reopen completed scalar local, address-exposed pointer storage, or
  `00005.c`/`00006.c` fused-compare branch repairs unless their exact old
  owner-layer failures return.
- Check both `for.cond.1` and `for.cond.5`; the first loop has an empty body
  and a separate latch decrement, while the second has the decrement in the
  body and an empty latch. Both currently emit unconditional self-loops.

## Proof

Step 1 proof command:

```sh
set -o pipefail; { cmake --build --preset default && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && timeout 30s ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00007_c$'; } 2>&1 | tee test_after.log
```

Result: TIMEOUT, exit 124. `c_testsuite_aarch64_backend_src_00007_c` started
and did not complete within the delegated 30 second timeout. This is
`[RUNTIME_HANG]` evidence, not pass evidence. `test_after.log` contains the
timeout-bounded command output plus an explicit timeout note.
