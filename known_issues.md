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

## Builtin Handling Needs Centralized Refactor

### Status

- date logged: `2026-03-11`
- current status: open design issue
- trigger case: `llvm_gcc_c_torture_pr79286_c`

### Problem summary

Builtin handling is currently split across parser, sema, and LLVM lowering with
overlapping name tables and ad-hoc fallbacks. This makes it easy to "fix" one
case with a workaround that regresses unrelated cases.

Current symptoms in the tree:

- parser has special AST forms for some builtins such as `__builtin_va_arg`,
  `__builtin_offsetof`, and `__builtin_types_compatible_p`
- parser now also has direct call-name rewriting for a small builtin subset
- sema has a separate builtin remap table and a separate builtin return-type table
- LLVM lowering has its own builtin-name dispatch and an "unknown builtin => 0/null"
  fallback

This is not a coherent model. It is a set of partial workarounds.

### Desired design

Introduce a dedicated builtin definition layer, centered on a new
`src/frontend/builtin.hpp`, that acts as the single source of truth for builtin
classification.

That layer should define:

- builtin id enum
- builtin kind/category enum
- builtin metadata table
- helper lookup functions from source spelling to builtin id

Recommended builtin categories:

- `ParserSpecial`
  for constructs that must become dedicated AST nodes during parsing
  examples: `__builtin_va_arg`, `__builtin_offsetof`, `__builtin_types_compatible_p`
- `AliasCall`
  for builtins that are semantically ordinary library calls and may be normalized
  to canonical function names at parse/sema boundary
  examples: `__builtin_printf`, `__builtin_fprintf`, `__builtin_sprintf`,
  `__builtin_puts`, `__builtin_putchar`, `__builtin_abort`
- `ConstantExpr`
  for builtins that fold to constants
  examples: `__builtin_inf`, `__builtin_nan`, `__builtin_huge_val`
- `Identity`
  for builtins whose result is one of their inputs or a semantic no-op
  examples: `__builtin_expect`
- `Intrinsic`
  for builtins that require explicit lowering rules
  examples: `__builtin_alloca`, `__builtin_unreachable`, `__builtin_bswap*`,
  `__builtin_clz*`, `__builtin_ctz*`, `__builtin_popcount*`, overflow builtins,
  floating-point classification builtins, complex builtins, va builtins
- `Unsupported`
  for names that are recognized but intentionally not lowered yet

### Required structural changes

1. Add `builtin.hpp`

   This file should define builtin ids and metadata, for example:

   - canonical source spelling
   - category
   - optional remapped libc symbol
   - optional fixed return-type policy
   - optional lowering opcode / semantic tag

2. Add explicit builtin AST/HIR nodes

   The parser/sema pipeline should stop relying on raw string matching as the
   primary builtin model.

   At minimum, the IR should carry builtin identity explicitly, either by:

   - adding builtin-specific expr nodes, or
   - extending call expressions / callee references with builtin id metadata

   Raw `DeclRef.name == "__builtin_*"` checks in codegen should be treated as
   transition-only behavior.

3. Make parser classification explicit

   Parser responsibilities should be:

   - convert `ParserSpecial` builtins into dedicated AST nodes
   - normalize `AliasCall` builtins into ordinary call targets only if the builtin
     table marks them safe to rewrite
   - leave `Intrinsic` builtins as builtin-tagged call nodes, not stringly-typed vars

4. Make sema consult builtin metadata instead of duplicate local tables

   Sema should:

   - resolve builtin ids from the centralized table
   - provide type checking and return-type inference from builtin metadata
   - reject or flag unsupported builtins explicitly

   The existing duplicated helpers in `sema_driver.cpp` should be merged into this
   model and then removed.

5. Make LLVM lowering dispatch on builtin id / category

   LLVM lowering should:

   - lower `AliasCall` by emitting ordinary external calls
   - lower `ConstantExpr` directly to constants
   - lower `Identity` without creating fake external symbols
   - lower `Intrinsic` through dedicated lowering rules
   - delete the current "unknown builtin => 0/null" fallback

### Migration plan

1. Land `builtin.hpp` with a minimal seeded table covering currently supported builtins.
2. Convert parser special cases to consult that table instead of hardcoded names.
3. Add builtin identity to AST/HIR call representation.
4. Port sema return-type inference onto builtin metadata.
5. Port LLVM lowering onto builtin id dispatch.
6. Remove duplicated builtin remap tables from parser/sema/codegen.
7. Remove unsafe fallbacks and replace them with explicit diagnostics.

### Immediate implementation guidance

- Do not add new broad `__builtin_xxx -> xxx` remap tables in random layers.
- Do not treat `__builtin_alloca` as an ordinary extern call target.
- Do not rely on string comparisons in codegen as the final architecture.
- Use `pr79286.c` as the motivating parser-normalized alias-call example, but
  validate against full-suite before/after logs after each migration step.

### Acceptance criteria

Builtin refactor work is only considered acceptable when all of the following hold:

- one centralized builtin definition source exists
- parser, sema, and codegen all consume that source
- no duplicated builtin remap/type tables remain
- no "unknown builtin => 0/null" code path remains in LLVM lowering
- regression guard passes against `test_fail_before.log` / `test_fail_after.log`
  with zero newly failing tests
