Status: Active
Source Idea Path: ideas/open/574_rv64_floating_point_binary_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Representative Route

# Current Packet

## Just Finished

Completed Step 4: reran the `src/20000605-1.c` RV64 object-route
representative in a fresh Step 4 artifact directory.

Result: the route does not advance past the old 570 owner. It still fails at
the same generic unsupported instruction fragment:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering;
function=render_image_rgb_a; block=entry; block_index=0; instruction_index=5;
instruction_kind=BinaryInst; owner=double %t5
```

The Step 3 prepared F64/FPR binary support is present in the local source and
`build/c4cll` is newer than the edited implementation file, so this Step 4
result appears to reflect a remaining representative-shape gap rather than a
stale binary. The Step 1 owner has an immediate `double 1.0` lhs and an FPR
producer rhs, while Step 3 explicitly covered prepared FPR-home operands.

## Suggested Next

Delegate a bounded follow-up that inspects and repairs the representative
operand-materialization gap for `bir.sdiv double 1.0, %t4`, likely by extending
the semantic F64 binary path to materialize supported F64 immediate operands
instead of requiring both operands to already have FPR homes.

## Watchouts

- The representative still stops at the generic `BinaryInst` owner before any
  later cast, truncation, comparison, or runtime issue is reached.
- The old owner uses `BinaryOpcode::SDiv` for floating division, but the lhs is
  the immediate constant `double 1.0`; this is outside the current FPR-home-only
  Step 3 shape.
- The F128 and F64 remainder cases remain deliberate fail-closed coverage.

## Proof

Delegated proof command:

```sh
cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000605-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4/src_20000605-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4/src_20000605-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step4/src_20000605-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake
```

Result: failed as an object-route compile failure with exit code 1.

The old generic FP binary owner is not gone:
`unsupported_instruction_fragment ... instruction_kind=BinaryInst; owner=double %t5`.
No later owner/blocker was reached.

Proof artifacts:

- `build/agent_state/574_rv64_floating_point_binary_lowering/step4/src_20000605-1.c/object-route.log`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4/src_20000605-1.c/object-route.rc`

No CTest proof was requested for this proof-only packet, and `test_after.log`
was not updated.
