Status: Active
Source Idea Path: ideas/open/585_bir_vector_binop_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Or Fail-Close Vector-Binop Producer Admission

# Current Packet

## Just Finished

Step 2 of `plan.md` selected and implemented the fail-closed producer rule for
fixed-vector `LirBinOp`: do not publish scalar `BinaryInst` facts for LLVM
fixed-vector arithmetic until BIR has a lane-aware vector-binop fact shape.

Focused coverage added in
`tests/backend/bir/backend_lir_to_bir_notes_test.cpp` builds a representative
`<8 x i8> mul` LIR fixture and asserts semantic BIR lowering fails closed with
`vector-binop semantic family`.

Implementation notes:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp` detects fixed-vector
  `LirBinOp` type text before scalar-binop type admission and reports the
  explicit vector-binop owner family.
- `src/backend/bir/lir_to_bir/scalar.cpp` also guards the scalar binop producer
  path against direct fixed-vector admission, so the scalar helper cannot
  publish a lane-losing scalar carrier if reached from another route.
- No expectations, unsupported markers, allowlists, or named testcase handling
  were changed.

Representative movement checked:
- Direct `build/c4cll --dump-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/simd-6.c` now reports latest function
  failure `foo` in `vector-binop semantic family`. This is intentional
  producer fail-close behavior, not downstream RV64/vector ABI progress.

## Suggested Next

Step 3 should run the supervisor-selected broader validation or close-gate
handoff and ask the plan owner whether the source idea can close with an
explicit fail-closed vector-binop producer boundary.

## Watchouts

The BIR `BinaryInst` shape carries only scalar `TypeKind` operand facts, while
existing lane-aware vector operation facts live on intrinsic carriers. Treating
`<8 x i8> mul` as `I64` would lose lane semantics, so this slice deliberately
fails closed. A future repair packet that wants producer admission should first
add a lane-aware vector-binop fact shape instead of widening scalar carriers.

## Proof

Delegated proof command: `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'`.

Fresh delegated proof output is recorded in `test_after.log`.
Result: passed; `346/346` `backend_` tests passed.

Additional representative check:
- `build/c4cll --dump-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/simd-6.c`
  Result: failed closed at `vector-binop semantic family`.
