# Known Issues

## Clang/GCC Divergence On `bitfld-3.c` And `bitfld-5.c`

### Affected cases

- `llvm_gcc_c_torture_bitfld_3_c`
- `llvm_gcc_c_torture_bitfld_5_c`

### Environment observed

- host arch: `aarch64`
- date verified: `2026-03-11`

### Current behavior

These two gcc torture tests are not currently useful as c4cll regressions on this
machine because the host reference compilers disagree:

- `clang` compiles both cases but the produced binaries abort at runtime
- `gcc` compiles both cases and the produced binaries pass

This matches the local test-suite configuration in
`tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/CMakeLists.txt`,
which already lists `bitfld-3.c` and `bitfld-5.c` under "Tests where clang currently
has bugs or issues".

### Reproduction

```bash
clang tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-3.c -o /tmp/clang-bitfld-3.bin && /tmp/clang-bitfld-3.bin
gcc   tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-3.c -o /tmp/gcc-bitfld-3.bin && /tmp/gcc-bitfld-3.bin

clang tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-5.c -o /tmp/clang-bitfld-5.bin && /tmp/clang-bitfld-5.bin
gcc   tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/bitfld-5.c -o /tmp/gcc-bitfld-5.bin && /tmp/gcc-bitfld-5.bin
```

Expected locally:

- `clang bitfld-3`: aborts
- `gcc bitfld-3`: exits `0`
- `clang bitfld-5`: aborts
- `gcc bitfld-5`: exits `0`

### Practical guidance

Do not treat `bitfld-3` and `bitfld-5` failures as evidence of a new c4cll bitfield
regression unless the same behavior is first reproduced against the chosen reference
compiler for the target environment.

## Vector Types Are Modeled As Arrays In `TypeSpec`

### Affected cases

- `llvm_gcc_c_torture_pr53645_c`
- `llvm_gcc_c_torture_pr53645_2_c`
- `llvm_gcc_c_torture_simd_1_c`
- `llvm_gcc_c_torture_simd_2_c`

### Nearby vector/SIMD coverage

- `llvm_gcc_c_torture_simd_5_c`

`simd_1` and `simd_2` are the same broad class of issue: GCC vector extension
objects participate in arithmetic as vector values, but the current frontend/type
model mixes vector and array semantics, which leads to invalid or inconsistent
codegen.

`simd_5` exercises nearby vector/SIMD behavior as well, but in the current build
state it is passing. It should still be used as a regression check after the
vector-type refactor because it covers vector locals surviving across calls.

### Reproduction

```bash
ctest --test-dir build --output-on-failure -R '^llvm_gcc_c_torture_pr53645_c$|^llvm_gcc_c_torture_pr53645_2_c$'
ctest --test-dir build --output-on-failure -R '^llvm_gcc_c_torture_simd_1_c$|^llvm_gcc_c_torture_simd_2_c$|^llvm_gcc_c_torture_simd_5_c$'
```

### Current behavior

The parser records `__attribute__((vector_size(N)))` by setting `TypeSpec.is_vector = true`
while also encoding the type as an array through `array_rank/array_size/array_dims`.

That mixed representation leaks into codegen:

- array decay paths treat vector objects as pointers
- unary dereference and assignment paths recover pointer-shaped types instead of vector values
- binary arithmetic eventually emits invalid IR such as:

```llvm
%t10 = load <4 x i32>, ptr %t8
%t11 = udiv <4 x i32> %t10, %t9
store ptr %t11, ptr %p.x
```

or earlier variants like:

```llvm
%t14 = udiv ptr %t12, %t13
```

The root cause is not just one missing load/store. The frontend type model says
"vector" and "array" at the same time, so different codegen paths make different
choices about whether the object is a value, an aggregate slot, or a decayed pointer.

The `simd_1` / `simd_2` cases reinforce the same diagnosis from a different angle:
they use vector arithmetic on globals and locals and currently fail at runtime
with subprocess aborts, which is consistent with vector values being mishandled
across load/store and arithmetic boundaries.

### Reference implementation note

`ref/claudes-c-compiler/src/frontend` does not model GCC vectors as arrays.
It introduces a dedicated vector type in the frontend type system:

- parser wraps the base type as `Vector(inner, total_bytes)`
- sema uses vector-specific type rules
- binary result type picks the vector type directly when either operand is vector

That separation is the key design point to copy.

### Proposed implementation plan

Target design: make vector a first-class `TypeSpec` kind optimized for LLVM IR vectors,
not an array-shaped encoding.

1. Extend `TypeSpec` with dedicated vector metadata.
   Suggested shape:
   - keep `bool is_vector`
   - add `long long vector_lanes`
   - optionally add `long long vector_bytes` if useful for diagnostics/ABI
   - stop using `array_rank/array_size/array_dims` to represent vectors

2. Update parser vector attribute handling in `src/frontend/parser/types.cpp`.
   - for `vector_size(N)`, compute element lanes from `N / sizeof(base)`
   - set vector fields only
   - leave array fields in the non-array state

3. Update semantic type propagation.
   - add vector-specific rules in expression type resolution
   - for binary arithmetic, if either operand is vector, result type is that vector type
   - exclude vectors from array decay and pointer arithmetic rules

4. Update LLVM codegen around the LLVM vector value model.
   - `llvm_ty(vector)` should return `<N x T>`
   - `llvm_alloca_ty(vector)` should also return `<N x T>`
   - locals/globals/deref/load/store should consistently use vector values
   - `IndexExpr` on vector lvalues should index the object storage, not a decayed array

5. After the model change, remove any vector-specific fallback patches in codegen.
   The goal is to let vectors flow through the normal value paths instead of
   patching pointer cases one by one.

### Verification plan

1. Re-run:
   - `llvm_gcc_c_torture_pr53645_c`
   - `llvm_gcc_c_torture_pr53645_2_c`
   - `llvm_gcc_c_torture_simd_1_c`
   - `llvm_gcc_c_torture_simd_2_c`
   - `llvm_gcc_c_torture_simd_5_c`
2. Inspect generated IR and confirm:
   - arithmetic is emitted as `udiv/sdiv/urem/srem <N x T>`
   - stores use `<N x T>` instead of `ptr`
3. Run nearby vector-related torture tests if present.
4. Run full `ctest -j --output-on-failure` and compare before/after logs with the regression guard.
