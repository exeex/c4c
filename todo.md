Status: Active
Source Idea Path: ideas/open/574_rv64_floating_point_binary_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Representative Route

# Current Packet

## Just Finished

Completed Step 4 follow-up: repaired the RV64 object-route F32 floating binary
gap reached at `owner=float %t10`.

The exact observed shape is a prepared same-type F32 multiply with FPR homes:

```text
%t10 = bir.mul float %t9, %t8
home %t10 value_id=9 kind=register reg=fs2
move_bundle phase=before_instruction block_index=0 instruction_index=11
  move from_value_id=8 to_value_id=9 destination_storage=register placement=fpr:callee_saved#1/w1
  move from_value_id=7 to_value_id=9 destination_storage=register placement=fpr:callee_saved#1/w1
```

The prepared scalar FP binary path is now type-parametric for F32/F64 hardware
`add`, `sub`, `mul`, and `div`, while F128 and unsupported FP remainder forms
remain fail-closed. Focused object-emission coverage now proves F32 `fmul.s`
through FPR homes in addition to the existing F64 `fdiv.d` and F64 immediate
coverage.

Representative result: `src/20000605-1.c` advances past the old
`owner=float %t10` unsupported `BinaryInst`. The next blocker is:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel multi-source
stack-destination authority
```

## Suggested Next

Delegate a bounded follow-up for Step 4 to classify and repair the ambiguous
non-parallel multi-source stack-destination move-bundle authority now reached
by the representative object route, if it belongs to the FP binary route rather
than a separate prepared move-bundle capability.

## Watchouts

- The `float %t10` representative owner is no longer the first blocker.
- The F32 `%t10` repair is semantic FPR-home lowering; it does not match source
  filenames, function names, block names, or value names.
- Scratch FPR selection still avoids the destination and existing operand FPR
  homes.
- The F128 and F32/F64 remainder cases remain deliberate fail-closed coverage.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed. Proof log: `test_after.log`.

Supervisor guard result:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 346/346.

Representative rerun command used the requested fresh Step 4c artifact path:

```sh
cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000605-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake
```

Result: failed as an object-route compile failure with exit code 1, but
advanced past `owner=float %t10` and reached the ambiguous non-parallel
multi-source stack-destination move-bundle authority blocker.

Representative artifacts:

- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/prepared-focus.txt`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/object-route.log`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/object-route.rc`

Proof log: `test_before.log` after supervisor roll-forward.
