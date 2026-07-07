Status: Active
Source Idea Path: ideas/open/585_bir_vector_binop_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Vector-Binop Producer Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` refreshed the current vector-binop producer evidence without
implementing the producer decision.

Representative set inspected:
- `tests/c/external/gcc_torture/src/simd-6.c`: now reaches the vector-binop
  producer boundary. Direct `build/c4cll --dump-bir --target
  riscv64-linux-gnu .../simd-6.c` and the refreshed
  `build/rv64_gcc_c_torture_backend/src_simd-6.c/case.log` report latest
  function failure `foo` in `scalar-binop semantic family`. This is the active
  vector-binop ambiguity, because the 8-byte vector signature has advanced far
  enough for the `x * y` fixed-vector multiply to hit LIR-to-BIR binop
  admission.
- `tests/c/external/gcc_torture/src/pr70903.c`: still signature admission.
  Direct `--dump-bir` and the refreshed case log report `foo` in
  `function-signature semantic family`; this is wide-vector ABI/signature
  carrier ownership, not this vector-binop producer packet.
- `tests/c/external/gcc_torture/src/scal-to-vec2.c`: scalar/local-memory owner
  family. Direct `--dump-bir` and the refreshed case log report `main` in
  `scalar/local-memory semantic family`; this should not be treated as
  vector-binop producer evidence.
- `tests/c/external/gcc_torture/src/simd-1.c`: bootstrap global/data shape
  owner. Direct `--dump-bir` and the refreshed case log stop before semantic
  function lowering on the bootstrap global/data shape admission note.
- `tests/backend/bir/backend_prepare_structured_context_test.cpp`: existing
  signature coverage documents small vector signature carriers (`<2 x i32>`,
  `<4 x i8>`), 16-byte vector parameter carrier publication as `I128` ABI
  memory, and fail-closed 32-byte/unsupported signature cases. This coverage
  separates wide-vector ABI signature admission from vector-binop producer
  admission.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` and
  `tests/backend/bir/backend_x86_handoff_boundary_lir_test.cpp`: existing
  scalar-binop coverage admits scalar integer/float/F128 variable binops and
  checks scalar-binop fail-closed notes, but does not cover fixed-vector
  `LirBinOp` opcode or operand fact publication.

Instruction-lowering boundary inspected:
- `src/backend/bir/lir_to_bir/scalar.cpp` `lower_scalar_family_inst` handles
  `LirBinOp` through `lower_scalar_binary_opcode`,
  `lower_scalar_or_function_pointer_type`, and `lower_scalar_binop_operands`.
  `lower_scalar_or_function_pointer_type` admits scalar integer, float,
  pointer, F128, and VRM mask carriers, but not LLVM fixed-vector type strings
  such as `<8 x i8>`. Therefore `simd-6.c` is not a downstream RV64 lowering
  failure yet; it is the producer-side decision point for whether fixed-vector
  arithmetic binops should publish BIR facts or fail closed explicitly.

## Suggested Next

Step 2 should add focused LIR-to-BIR coverage for a representative fixed-vector
arithmetic `LirBinOp` such as `<8 x i8> mul` and then implement the real
producer decision in `lower_scalar_family_inst`: either publish semantically
valid vector-binop opcode and operand facts, or add an explicit fail-closed
vector-binop owner diagnostic. Do not use `pr70903.c` as the next packet,
because it is still wide-vector ABI signature admission rather than the
vector-binop producer boundary.

## Watchouts

Keep vector-binop producer admission separate from scalar-binop F128 repair and
wide-vector ABI signature representation. Do not accept expectation rewrites,
unsupported downgrades, allowlist changes, or named-case handling as producer
progress. If `simd-6.c` advances after a correct producer decision, classify
the new boundary separately; likely downstream owners include vector ABI,
local-memory, or RV64 vector/object lowering, but none were reached by the
current evidence refresh.

## Proof

Delegated proof command: `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^backend_'`.

Additional evidence commands run before the delegated proof:
- `build/c4cll --dump-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/simd-6.c`
- `build/c4cll --dump-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/pr70903.c`
- `build/c4cll --dump-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/scal-to-vec2.c`
- `build/c4cll --dump-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/simd-1.c`
- `ctest --test-dir build --output-on-failure -R
  '^(backend_prepare_structured_context|llvm_gcc_c_torture_src_simd_6_c|llvm_gcc_c_torture_src_pr70903_c|llvm_gcc_c_torture_src_scal_to_vec2_c)$'`

Fresh delegated proof output is recorded in `test_after.log`.
Result: passed; build was up to date and `346/346` `backend_` tests passed.
