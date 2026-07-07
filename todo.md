Status: Active
Source Idea Path: ideas/open/574_rv64_floating_point_binary_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Representative Route

# Current Packet

## Just Finished

Completed Step 4 follow-up: repaired the RV64 object-route F64 binary
operand-materialization gap for semantic immediate operands such as
`bir.sdiv double 1.0, %t4`.

The prepared F64 binary path now materializes immediate F64 bit patterns into
scratch FPRs through the existing RV64 immediate loader and `fmv.d.x`, while
preserving the existing FPR-home operand path for `add`, `sub`, `mul`, and
`div`. The focused object-emission coverage proves both lhs-immediate and
rhs-immediate F64 `sdiv` shapes, and the existing F128/remainder unsupported
forms remain fail-closed.

Representative result: `src/20000605-1.c` now advances past the old
`owner=double %t5` blocker. The next unsupported owner reached is:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering;
function=render_image_rgb_a; block=entry; block_index=0; instruction_index=11;
instruction_kind=BinaryInst; owner=float %t10
```

## Suggested Next

Delegate a bounded follow-up for Step 4 to inspect and repair the new
representative blocker at `owner=float %t10`, likely the corresponding F32
floating binary lowering/materialization gap.

## Watchouts

- The `double %t5` representative owner is no longer the first blocker.
- F64 immediate materialization reinterprets the full 64-bit immediate payload
  into the signed immediate loader, so sign-bit-set FP constants are not
  artificially rejected by this helper.
- Scratch FPR selection avoids the destination and existing operand FPR homes.
- The F128 and F64 remainder cases remain deliberate fail-closed coverage.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed. Proof log: `test_after.log`.

Supervisor guard result:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 346/346.

Representative rerun command used the requested fresh Step 4b artifact path:

```sh
cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000605-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4b/src_20000605-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4b/src_20000605-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4b/src_20000605-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake
```

Result: failed as an object-route compile failure with exit code 1, but
advanced past `owner=double %t5` and reached `owner=float %t10`.

Representative artifacts:

- `build/agent_state/574_rv64_floating_point_binary_lowering/step4b/src_20000605-1.c/object-route.log`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4b/src_20000605-1.c/object-route.rc`

Proof log: `test_before.log` after supervisor roll-forward.
