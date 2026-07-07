Status: Active
Source Idea Path: ideas/open/562_bir_scalar_binop_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar-Binop Producer Boundary

# Current Packet

## Just Finished

Step 2 repaired the F128 scalar-binop producer boundary for
`ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.

Decision: admit named F128 arithmetic through the general scalar-binop producer
rule now, while keeping arbitrary F128 literal constants fail-closed at the
existing 64-bit immediate lane boundary.

Implemented behavior:
- `fneg fp128` now synthesizes a full-width F128 zero operand and emits the
  existing BIR `Sub` scalar-binop shape.
- Named `fmul fp128` and `fsub fp128` continue through the shared scalar-binop
  opcode and operand path as BIR `Mul` and `Sub`.
- Focused BIR notes coverage now verifies named F128 `fneg`/`fmul`/`fsub`
  publication with F128 operand/result typing, while the existing F128
  constant-binop fail-closed test remains in place.

Representative movement:
- `src/960513-1.c`, function `f`: advances beyond the original BIR
  `scalar-binop semantic family` boundary; its generated
  `llvm_gcc_c_torture_src_960513_1_c` test now passes.

## Suggested Next

Execute Step 3: run closure-focused validation and hand off to the plan owner
to decide whether scalar-binop producer admission is complete or whether the
remaining vector-binop owner-boundary evidence needs a split.

## Watchouts

- F128 scalar constants are still deliberately fail-closed unless a future
  packet adds full-width literal parsing beyond the current producer scope.
- `src/simd-6.c` is vector arithmetic after small-vector signature admission;
  keep it separate from scalar-binop closure unless the supervisor explicitly
  routes a vector-binop owner-boundary packet.
- No expectations, unsupported markers, allowlists, or row classifications were
  changed for this slice.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: backend subset passed `346/346`; `test_after.log` contains the fresh
proof.

Additional representative check:
`ctest --test-dir build --output-on-failure -R '^llvm_gcc_c_torture_src_960513_1_c$'`

Result: passed.
