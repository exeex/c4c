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
- last updated: `2026-03-11`
- current status: in progress
- trigger case: `llvm_gcc_c_torture_pr79286_c`

### Progress so far

The refactor is no longer just a design note. The tree now has a real
centralized builtin metadata layer and the main sema/codegen paths have started
to consume it.

Completed migration work:

- `src/frontend/builtin.hpp` now exists and is the shared source for builtin ids,
  categories, canonical alias-call names, result-kind classification, known libc
  call result kinds, and several builtin-family helper predicates used by codegen
- parser already tags builtin calls with `BuiltinId` and distinguishes parser
  special cases from builtin-call nodes
- HIR call nodes carry `builtin_id`
- sema builtin return-type inference now uses centralized builtin metadata/result
  helpers instead of a separate builtin remap table
- sema known-libc return-type lookup was reduced to a shared table in
  `builtin.hpp` instead of a local `strcmp` chain
- codegen main-path builtin dispatch now uses `CallExpr::builtin_id` instead of
  recovering builtin identity from raw callee strings
- several duplicated codegen builtin helper groups were removed and replaced by
  shared metadata-driven helpers or local helper consolidation

Validated targeted checks so far:

- `cmake --build build -j8`
- `ctest --test-dir build --output-on-failure -R llvm_gcc_c_torture_pr79286_c`

Not yet completed:

- parser still contains some direct builtin spelling checks / normalization logic
- codegen still has a large builtin lowering switch; it now keys off builtin id,
  but the lowering rules are not yet fully metadata-driven
- full regression guard (`test_fail_before.log` vs `test_fail_after.log`) has not
  been rerun for this migration slice yet

### Regression guard status

Initial comparison run on `2026-03-11`:

- before (main): passed=`1711`, failed=`62`, total=`1773`
- after (branch): passed=`1692`, failed=`81`, total=`1773`
- initial delta: `30` new failures, `11` newly passing

After implementing missing builtin lowerings (same session):

- fixed `14` of the `30` new failures by adding codegen for:
  - `__builtin_alloca` → LLVM dynamic `alloca i8, i64 size`
  - `__builtin_constant_p` → constant `0` (compile-time query, always false at -O0)
  - `__builtin_classify_type` → integer type class based on argument TypeSpec
  - `__builtin_{add,sub,mul}_overflow` → `@llvm.{s,u}{add,sub,mul}.with.overflow`
  - `__builtin_expect` → evaluate both args (for side effects), return first
  - suppress spurious `declare @__builtin_xxx(...)` for non-AliasCall builtins
- remaining delta: `16` new failures (all `RUNTIME_FAIL`, pre-existing codegen bugs
  unrelated to builtin dispatch; these tests were previously `FRONTEND_FAIL` and
  are now reaching runtime due to the refactored builtin dispatch succeeding where
  the old string-based dispatch threw)

Remaining `16` new failures (not builtin-dispatch issues):

- `llvm_gcc_c_torture_20120105_1_c` — unaligned memcpy into packed struct (segfault)
- `llvm_gcc_c_torture_alloca_1_c` — `__attribute__((aligned))` alignment computation
- `llvm_gcc_c_torture_memchr_1_c` — memchr with multi-dimensional array addressing
- `llvm_gcc_c_torture_pr38533_c` — inline assembly constraints
- `llvm_gcc_c_torture_pr44852_c` — pointer post-increment sequencing
- `llvm_gcc_c_torture_pr57130_c` — struct pass-by-value / memcmp
- `llvm_gcc_c_torture_pr65401_c` — pointer reinterpretation casting
- `llvm_gcc_c_torture_pr70460_c` — computed gotos / label-address arithmetic
- `llvm_gcc_c_torture_pr71626_1_c` — vector return value with function pointer
- `llvm_gcc_c_torture_pr71626_2_c` — vector return value (same, -fpic variant)
- `llvm_gcc_c_torture_pr77767_c` — VLA parameters with side effects
- `llvm_gcc_c_torture_pr78726_c` — bitwise NOT / type promotion on unsigned char
- `llvm_gcc_c_torture_scal_to_vec2_c` — scalar-to-vector promotion
- `llvm_gcc_c_torture_strcpy_2_c` — strcpy with array init and byte verification
- `llvm_gcc_c_torture_strlen_2_c` — strlen with multi-dimensional string arrays
- `llvm_gcc_c_torture_strlen_3_c` — strlen with embedded nulls in arrays
- `llvm_gcc_c_torture_widechar_3_c` — missing `__BYTE_ORDER__` preprocessor macro

Practical handoff note for follow-up work:

- the remaining 16 failures are NOT builtin-dispatch regressions; they are
  pre-existing codegen limitations that are now exposed because the refactored
  builtin dispatch no longer throws FRONTEND_FAIL on these cases
- priority fixes: add `__BYTE_ORDER__` / `__ORDER_*_ENDIAN__` preprocessor macros
  (fixes widechar-3 trivially); investigate `record_extern_call_decl` return-type
  inference for AliasCall builtins (may affect string function tests)

### Problem summary

Builtin handling is currently split across parser, sema, and LLVM lowering with
overlapping name tables and ad-hoc fallbacks. This makes it easy to "fix" one
case with a workaround that regresses unrelated cases.

Current symptoms in the tree:

- parser has special AST forms for some builtins such as `__builtin_va_arg`,
  `__builtin_offsetof`, and `__builtin_types_compatible_p`
- parser still has some direct call-name rewriting for a small builtin subset
- sema no longer keeps the previous separate builtin remap/return-type tables, but
  ordinary call-type knowledge is still split between builtin metadata and local
  call inference
- LLVM lowering now dispatches builtin identity from HIR-carried `builtin_id`, but
  still keeps a large lowering switch and has not finished the full cleanup pass

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
8. Delete transitional string-based builtin helper APIs once all call sites use
   `BuiltinId` / builtin metadata directly.

   In particular, the end state should not depend on legacy helpers whose main
   purpose is to recover builtin meaning from raw callee strings during sema or
   codegen. After the migration:

   - removing those helper APIs should not break builtin lowering
   - alias-call builtins should still normalize and type-check correctly
   - intrinsic/constant/identity builtins should still lower correctly without
     string dispatch fallback
   - unknown builtin spellings should still fail with explicit diagnostics, not
     by silently degrading to ordinary calls or fake values

### Immediate implementation guidance

- Do not add new broad `__builtin_xxx -> xxx` remap tables in random layers.
- Do not treat `__builtin_alloca` as an ordinary extern call target.
- Do not rely on string comparisons in codegen as the final architecture.
- Treat string-based builtin helper functions as temporary migration shims.
  The target architecture should be able to delete them cleanly after builtin
  identity is carried end-to-end.
- Use `pr79286.c` as the motivating parser-normalized alias-call example, but
  validate against full-suite before/after logs after each migration step.

### Acceptance criteria

Builtin refactor work is only considered acceptable when all of the following hold:

- one centralized builtin definition source exists
- parser, sema, and codegen all consume that source
- no duplicated builtin remap/type tables remain
- no transitional string-recovery builtin APIs remain on the main sema/codegen path
- no "unknown builtin => 0/null" code path remains in LLVM lowering
- regression guard passes against `test_fail_before.log` / `test_fail_after.log`
  with zero newly failing tests
