Status: Active
Source Idea Path: ideas/open/574_rv64_floating_point_binary_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize The FP Binary Owner

# Current Packet

## Just Finished

Completed Step 1: reproduced and localized the active RV64 FP binary object-route
owner without code changes.

Inspected 570 evidence:

- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000605-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000605-1.c/object-route.log`

Current representative rerun:

```sh
mkdir -p build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c && cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000605-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > /workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c/object-route.log 2>&1; printf '%s\n' "$?" > /workspaces/c4c/build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c/object-route.rc
```

Result: exit code `1`, still `[RV64_C4C_OBJ_COMPILE_FAIL]` with
`unsupported_instruction_fragment` at `function=render_image_rgb_a`,
`block=entry`, `block_index=0`, `instruction_index=5`,
`instruction_kind=BinaryInst`, `owner=double %t5`.

Exact FP binary owner from prepared BIR:

- Owner: `double %t5`
- Operation: `%t5 = bir.sdiv double 0x3FF0000000000000, %t4`
- Left operand source: immediate double constant `0x3FF0000000000000` (`1.0`)
- Right operand source: `%t4 = bir.fpext float %t3 to double`
- `%t3` source: `%t3 = bir.load_local float %t3.addr, addr %p.info+4`
- Result home: `%t5` has prepared FPR register home `ft0`

Implementation surface for Step 2/3:

- `src/backend/mir/riscv/codegen/object_emission.cpp`: `fragment_for_prepared_instruction` dispatches ordinary `BinaryInst` through `fragment_for_prepared_binary`; when that returns `std::nullopt`, this owner reaches the generic unsupported-instruction path.
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`: `fragment_for_prepared_binary` currently accepts only `I32` and `I64` result types before selecting integer/GPR encodings, so the `double` binary owner fails before operand materialization or result publication.
- `src/backend/bir/lir_to_bir/scalar.cpp`: `FDiv` lowers to `bir::BinaryOpcode::SDiv`, so FP binary lowering must be keyed by floating result/operand type as well as opcode.

## Suggested Next

Execute Step 2 from `plan.md`: add focused backend object-emission coverage for
supported scalar double FP binary lowering and fail-closed unsupported FP binary
forms before changing the lowering implementation.

## Watchouts

- Do not match `src/20000605-1.c`, `render_image_rgb_a`, `%t5`, or exact value
  names.
- Do not mix FP casts, truncation, comparisons, pointer arithmetic, select, or
  inline asm work into this plan unless a separate follow-up idea is created.
- Diagnostic-only edits are not capability progress unless paired with real
  semantic lowering or a narrower fail-closed unsupported path.
- The immediate representative uses `bir.sdiv double` for FP division because
  BIR maps `FDiv` to `BinaryOpcode::SDiv`; tests and implementation should avoid
  interpreting every `SDiv` as integer division without checking type.
- The first later consumer is `%t6 = bir.fptrunc double %t5 to float`, but Step
  2/3 should not widen into FP cast repair unless the binary owner has been
  fixed and a fresh rerun proves the route advanced.

## Proof

Evidence-only packet. No CTest proof was delegated or run.

Representative object-route proof log:
`build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c/object-route.log`

Representative result code:
`build/agent_state/574_rv64_floating_point_binary_lowering/step1/src_20000605-1.c/object-route.rc`
