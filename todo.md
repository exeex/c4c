Status: Active
Source Idea Path: ideas/open/410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Representative And Residual Ownership

# Current Packet

## Just Finished

Step 3 proved the `20040709-2.c` representative after the I16 scalar-binop
materialization repair.

Changed files:

- `todo.md`

Representative result:

- `dump_bir_status=0`.
- `prepared_status=0`.
- `c4cll_status=0`.
- The old `fn1A` `scalar-binop semantic family` failure remains absent.
- Fresh semantic BIR still contains the expected packed-bitfield scalar
  operations in
  `fn1A`, including:
  `%t2.bf.shr = bir.lshr i16 %t2.bf.unit, 5`,
  `%t2.bf.mask = bir.and i16 %t2.bf.shr, 2047`,
  `%t2 = bir.zext i16 %t2.bf.mask to i32`,
  `%t3 = bir.add i32 %t2, %p.x`,
  `%t4 = bir.trunc i32 %t3 to i16`,
  `%t5.bf.clr = bir.and i16 %t5.bf.old, 31`,
  `%t5.bf.vm = bir.and i16 %t4, 2047`,
  `%t5.bf.vs = bir.shl i16 %t5.bf.vm, 5`, and
  `%t5.bf.comb = bir.or i16 %t5.bf.clr, %t5.bf.vs`.
- Fresh prepared dump succeeds and includes `prepared.summary @fn1A` and
  `prepared.func @fn1A`.
- The RV64 object route compile succeeds for the representative.
- No later residual appeared in this Step 3 proof.
- 410 appears closure-ready for plan-owner lifecycle review.

## Suggested Next

Ask the plan owner to review and close 410, or route any broader-suite residual
as a separate owner if one appears outside this representative proof.

## Watchouts

- This packet made no implementation edits and did not touch RV64 object
  emission.
- The proof does not reopen 409 packed aggregate global initializer admission.
- If a future broader proof finds a later RV64/prepared residual, route it
  separately; the original 410 semantic scalar-binop blocker is cleared here.

## Proof

Exact delegated proof command run:

```sh
cmake --build --preset default &&
mkdir -p build/agent_state/410_step3_i16_scalar_binop_proof &&
(build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.bir.txt 2> build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.bir.err; printf 'dump_bir_status=%s\n' "$?" > build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.bir.status) &&
(build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.prepared.txt 2> build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.prepared.err; printf 'prepared_status=%s\n' "$?" > build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.prepared.status) &&
(build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20040709-2.c > build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.out 2> build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.err; printf 'c4cll_status=%s\n' "$?" > build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.status) &&
cat build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.bir.status build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.prepared.status build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.status &&
rg -n 'semantic lir_to_bir function|fn1A|scalar-binop|unsupported|error|fatal|bir\.(lshr|and|shl|or|zext|trunc|add)|unsupported_.*|prepared module shape|object route|RV64' build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.bir.txt build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.bir.err build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.prepared.txt build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.prepared.err build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.out build/agent_state/410_step3_i16_scalar_binop_proof/20040709-2.err || true &&
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `dump_bir_status=0`.
- `prepared_status=0`.
- `c4cll_status=0`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
