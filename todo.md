Status: Active
Source Idea Path: ideas/open/410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify The Fn1A Scalar Binop Residual

# Current Packet

## Just Finished

Step 1 classified the fresh `20040709-2.c` semantic lowering residual after
409 closed.

Statuses:

- `dump_bir_status=1`.
- `llvm_status=0`.
- `c4cll_status=0`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.

First diagnostic:

- `error: backend BIR dump requires semantic lir_to_bir lowering before the prepared handoff: semantic lir_to_bir failed outside the currently admitted capability buckets ... latest function failure: semantic lir_to_bir function 'fn1A' failed in scalar-binop semantic family`.

`fn1A` LLVM-route operation sequence:

- `alloca %struct.A`, `load %struct.A, ptr @sA`, `store %struct.A %t0, ptr %lv.y`.
- `getelementptr %struct.A, ptr %lv.y, i32 0, i32 0`.
- `%t2.bf.unit = load i16, ptr %t1`.
- `%t2.bf.shr = lshr i16 %t2.bf.unit, 5`.
- `%t2.bf.mask = and i16 %t2.bf.shr, 2047`.
- `%t2 = zext i16 %t2.bf.mask to i32`.
- `%t3 = add i32 %t2, %p.x`.
- `%t4 = trunc i32 %t3 to i16`.
- `%t5.bf.old = load i16, ptr %t1`.
- `%t5.bf.clr = and i16 %t5.bf.old, -65505`.
- `%t5.bf.vm = and i16 %t4, 2047`.
- `%t5.bf.vs = shl i16 %t5.bf.vm, 5`.
- `%t5.bf.comb = or i16 %t5.bf.clr, %t5.bf.vs`.
- `store i16 %t5.bf.comb, ptr %t1`.
- Later readback repeats `load i16`, `lshr i16`, `and i16`, `zext i16 to i32`.

First unsupported semantic operation:

- The first unsupported scalar-binop operation is the first `lshr i16`
  (`%t2.bf.shr = lshr i16 %t2.bf.unit, 5`).
- `src/backend/bir/lir_to_bir/scalar.cpp` already maps
  `LirBinaryOpcode::LShr` to `bir::BinaryOpcode::LShr`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp` routes `LirBinOp` through
  `lower_scalar_family_inst(...)` and reports the `scalar-binop semantic
  family` failure when that returns false.
- The narrowing point is operand/value materialization for I16 in
  `BirFunctionLowerer::lower_value(...)`: named SSA operands work, but the
  immediate RHS `5` is lowered with expected type `I16`, and the immediate
  switch currently handles `I1`, `I8`, `I32`, and `I64` but not `I16`.
- `I16` type recognition and immediate construction already exist in
  `lower_integer_type(...)`, `integer_type_bit_width(...)`, and
  `make_integer_immediate(...)`; the gap is the scalar value-materialization
  path used by semantic binop operands.

Classification:

- This is a producer semantic LIR-to-BIR scalar-binop/value-materialization gap,
  not packed global initializer admission and not RV64 object emission.
- Step 2 should implement the first reusable missing family: I16 scalar operand
  immediate support in `src/backend/bir/lir_to_bir/scalar.cpp`, proved first by
  `lshr i16` and then by adjacent I16 bitfield binops (`and`, `shl`, `or`) if
  the same path naturally covers them.
- Do not implement a testcase-shaped bitfield chain shortcut. Let existing BIR
  `BinaryOpcode` lowering own the individual operations once I16 operands can
  be represented.

## Suggested Next

Execute Step 2 as a narrow semantic scalar lowering repair in
`src/backend/bir/lir_to_bir/scalar.cpp`: add I16 immediate operand/value
materialization, add focused BIR lowering tests for `lshr i16` and adjacent I16
mask/update binops if local proof shows they share the same path, then rerun
the representative proof.

## Watchouts

- Keep 409 packed aggregate global initializer admission closed; this residual
  occurs after global admission.
- Do not edit RV64 object emission for this pre-prepared semantic failure.
- Preserve width semantics for I16 immediates, including signed textual values
  such as `-65505` flowing through I16 masks.
- The first operation is `lshr i16`, but Step 2 should repair the semantic I16
  operand/value support rather than special-casing `lshr` or `fn1A`.
- `--dump-bir` is the semantic BIR proof surface; `--codegen llvm` is the
  LLVM-route comparison surface because there is no standalone `--dump-lir`.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/410_step1_fn1A_scalar_binop && (build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.bir.txt 2> build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.bir.err; printf 'dump_bir_status=%s\n' "$?" > build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.bir.status) && (build/c4cll --codegen llvm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.llvm.txt 2> build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.llvm.err; printf 'llvm_status=%s\n' "$?" > build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.llvm.status) && (build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.out 2> build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.err; printf 'c4cll_status=%s\n' "$?" > build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.status) && cat build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.bir.status build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.llvm.status build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.status && rg -n 'semantic lir_to_bir function|fn1A|scalar-binop|lshr|ashr|shl|and|or|xor|zext|sext|trunc|add|sub|load i16|store i16|%struct\.A|packed|unsupported|error|fatal' build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.bir.txt build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.bir.err build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.llvm.txt build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.llvm.err build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.out build/agent_state/410_step1_fn1A_scalar_binop/20040709-2.err || true && rg -n 'scalar-binop|ScalarBinop|BinaryOpcode|LirBinary|lshr|ashr|shl|and| or |xor|zext|trunc|semantic family|semantic lir_to_bir' src/backend/bir/lir_to_bir tests/backend/bir | sed -n '1,260p' && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `dump_bir_status=1` with the `fn1A` scalar-binop diagnostic above.
- `llvm_status=0`; the LLVM-route output provided the operation sequence.
- `c4cll_status=0`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
