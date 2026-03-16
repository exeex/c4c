# Known Issues

## Status on 2026-03-16

Full-suite: **1786/1786 passed, 0 failures**

All previously-known remaining failures have been resolved:

- `llvm_gcc_c_torture_20001203_2_c` — statement-expression in ternary (fixed)
- `llvm_gcc_c_torture_pr70460_c` — computed-goto label differences (fixed)
- `llvm_gcc_c_torture_comp_goto_1_c` — computed-goto label arithmetic (fixed)
- `llvm_gcc_c_torture_strlen_4_c` — pointer-to-array global init (fixed)
- `llvm_gcc_c_torture_strlen_5_c` — pointer/array lowering (fixed)
- `llvm_gcc_c_torture_scal_to_vec2_c` — scalar-to-vector coercion (fixed)

## Newly enabled tests (this session)

- `scal-to-vec1.c` — fixed scalar-to-vector splatting in early vector op path + float vector ops
- `pr60960.c` — previously blocked by sema false positive, now passes

## Remaining failures

None.

## Remaining commented-out allowlist entries

Most commented-out entries fall into:
- **CLANG_COMPILE_FAIL**: test doesn't compile with Clang (strict mode); can't be validated by harness
- **GCC nested functions**: unsupported extension (`__label__`, local function defs)
- **Unsupported builtins**: `__builtin_return_address`, `__builtin_clrsb`, `__builtin_apply`, `__builtin_longjmp`
- **VLA struct members / _Decimal64**: complex extensions not yet supported
- **Pointer-to-array global initializers**: `&arr[0]` in global init emits null (HIR lowering gap)
