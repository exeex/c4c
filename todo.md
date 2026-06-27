Status: Active
Source Idea Path: ideas/open/410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Lower The First Supported Scalar Binop Family

# Current Packet

## Just Finished

Step 2 implemented reusable semantic LIR-to-BIR I16 immediate/value
materialization for scalar binop operands.

Changed files:

- `src/backend/bir/lir_to_bir/scalar.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `todo.md`

Implementation:

- Added `bir::TypeKind::I16` immediate handling to
  `BirFunctionLowerer::lower_value(...)`.
- This lets the existing scalar binary opcode path lower I16 immediates through
  already-supported `bir::BinaryOpcode` values such as `LShr`, `And`, `Shl`,
  and `Or`.
- No bitfield-chain special case, filename-specific handling, packed global
  initializer change, or RV64 object-emission inference was added.

Focused test coverage:

- Added `expect_admitted_i16_scalar_bitfield_binops()` in
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`.
- The fixture lowers ordinary `LirBinOp` instructions for `lshr i16`, `and
  i16`, `shl i16`, and `or i16`.
- The assertion checks the emitted BIR `BinaryInst` opcodes and I16 operand
  typing, and verifies the textual negative mask `-65505` is represented with
  the existing truncated signed I16 immediate semantics.

Representative result:

- `dump_bir_status=0`.
- `llvm_status=0`.
- `c4cll_status=0`.
- The old `fn1A` `scalar-binop semantic family` failure is cleared.
- Fresh semantic BIR contains the expected packed-bitfield scalar operations in
  `fn1A`, including:
  `%t2.bf.shr = bir.lshr i16 %t2.bf.unit, 5`,
  `%t2.bf.mask = bir.and i16 %t2.bf.shr, 2047`,
  `%t5.bf.vs = bir.shl i16 %t5.bf.vm, 5`, and
  `%t5.bf.comb = bir.or i16 %t5.bf.clr, %t5.bf.vs`.
- No remaining Step 2 scalar-binop pre-handoff residual appeared in the
  delegated proof.

## Suggested Next

Execute Step 3 proof/classification for `20040709-2.c`: confirm the semantic
BIR path remains clear after I16 scalar-binop materialization, run the
supervisor-selected object/prepared route proof, and classify any new residual
outside 410 if one appears.

## Watchouts

- Keep 409 packed aggregate global initializer admission closed; this patch
  only repairs semantic scalar operand materialization.
- Do not edit RV64 object emission for this pre-prepared semantic family.
- The focused fixture intentionally avoids unrelated I16 formal/return
  surfaces so it proves the scalar-binop immediate/value path directly.
- Preserve existing I1/I8/I32/I64 immediate behavior and F128 fail-closed
  behavior.

## Proof

Focused command run before the full delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'
```

Result:

- Passed: `backend_lir_to_bir_notes`.

Exact delegated proof command run:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' && mkdir -p build/agent_state/410_step2_i16_scalar_binop && (build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step2_i16_scalar_binop/20040709-2.bir.txt 2> build/agent_state/410_step2_i16_scalar_binop/20040709-2.bir.err; printf 'dump_bir_status=%s\n' "$?" > build/agent_state/410_step2_i16_scalar_binop/20040709-2.bir.status) && (build/c4cll --codegen llvm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step2_i16_scalar_binop/20040709-2.llvm.txt 2> build/agent_state/410_step2_i16_scalar_binop/20040709-2.llvm.err; printf 'llvm_status=%s\n' "$?" > build/agent_state/410_step2_i16_scalar_binop/20040709-2.llvm.status) && (build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step2_i16_scalar_binop/20040709-2.out 2> build/agent_state/410_step2_i16_scalar_binop/20040709-2.err; printf 'c4cll_status=%s\n' "$?" > build/agent_state/410_step2_i16_scalar_binop/20040709-2.status) && cat build/agent_state/410_step2_i16_scalar_binop/20040709-2.bir.status build/agent_state/410_step2_i16_scalar_binop/20040709-2.llvm.status build/agent_state/410_step2_i16_scalar_binop/20040709-2.status && rg -n 'semantic lir_to_bir function|fn1A|scalar-binop|lshr|ashr|shl|and|or|xor|zext|sext|trunc|add|sub|load i16|store i16|%struct\.A|packed|unsupported|error|fatal|bir\.(lshr|and|shl|or|zext|trunc|add)' build/agent_state/410_step2_i16_scalar_binop/20040709-2.bir.txt build/agent_state/410_step2_i16_scalar_binop/20040709-2.bir.err build/agent_state/410_step2_i16_scalar_binop/20040709-2.llvm.txt build/agent_state/410_step2_i16_scalar_binop/20040709-2.llvm.err build/agent_state/410_step2_i16_scalar_binop/20040709-2.out build/agent_state/410_step2_i16_scalar_binop/20040709-2.err || true && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `dump_bir_status=0`.
- `llvm_status=0`.
- `c4cll_status=0`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
