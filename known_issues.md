# Known Issues

## Status on 2026-03-13

Full-suite regression guard currently passes after this patch set.

- `before`: passed `1751`, failed `14`
- `after`: passed `1759`, failed `6`
- delta: `+8 passed`, `0 newly failing tests`

Resolved in this round:

- `llvm_gcc_c_torture_20030330_1_c` — dead static function elimination (unreferenced internal fns with __builtin_constant_p dead code)
- `llvm_gcc_c_torture_990130_1_c` — inline asm +r constraint side effect evaluation
- `llvm_gcc_c_torture_20021127_1_c` — abs/labs/llabs recognized as implicit builtins via llvm.abs intrinsic
- `llvm_gcc_c_torture_20010325_1_c` — wide string literal type resolution (TB_INT instead of TB_CHAR for indexing stride)
- `llvm_gcc_c_torture_pr71626_1_c` — vector type initializer lists in local variable declarations
- `llvm_gcc_c_torture_pr71626_2_c` — same as pr71626_1
- `llvm_gcc_c_torture_pr43784_c` — nested member access in global initializer constant expressions
- `llvm_gcc_c_torture_align_3_c` — __alignof__(expr) with function aligned attribute

## Remaining failures

| Test | Failure type | Problem summary |
| --- | --- | --- |
| `llvm_gcc_c_torture_20001203_2_c` | `RUNTIME_FAIL` | GNU statement-expression side effects inside a ternary arm are lowered eagerly before the branch, so the `objfile != 0 ? ... : xmalloc(...)` path dereferences null before the condition is applied. Deep codegen issue. |
| `llvm_gcc_c_torture_pr70460_c` | `RUNTIME_FAIL` | Computed-goto label differences are still lowered incorrectly; static label-delta table contents are wrong. |
| `llvm_gcc_c_torture_comp_goto_1_c` | `RUNTIME_FAIL` | Computed-goto with label address arithmetic; same bucket as `pr70460_c`. |
| `llvm_gcc_c_torture_strlen_4_c` | `BACKEND_FAIL` | Global initializer lowering for pointers-to-arrays is still invalid. |
| `llvm_gcc_c_torture_strlen_5_c` | `RUNTIME_FAIL` | Likely adjacent to `strlen_4_c` pointer/array lowering bucket. |
| `llvm_gcc_c_torture_scal_to_vec2_c` | `RUNTIME_FAIL` | Scalar-to-vector coercion issue; investigating. |

## Suggested next targets

1. `llvm_gcc_c_torture_scal_to_vec2_c` — likely fixable vector coercion issue
2. `llvm_gcc_c_torture_strlen_4_c` / `strlen_5_c` — pointer-to-array global init
3. computed-goto bucket: `pr70460_c`, `comp_goto_1_c`
4. `llvm_gcc_c_torture_20001203_2_c` — complex stmt-expression in ternary (deep)
