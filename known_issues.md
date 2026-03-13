# Known Issues

## Status on 2026-03-13

Full-suite regression guard currently passes after this patch set.

- `before`: passed `1744`, failed `21`
- `after`: passed `1751`, failed `14`
- delta: `+7 passed`, `0 newly failing tests`

Resolved in this round:

- `llvm_gcc_c_torture_20010904_1_c`
- `llvm_gcc_c_torture_20010904_2_c`
- `llvm_gcc_c_torture_20040411_1_c`
- `llvm_gcc_c_torture_20120105_1_c`
- `llvm_gcc_c_torture_970217_1_c`
- `llvm_gcc_c_torture_pr23467_c`
- `llvm_gcc_c_torture_pr77767_c`

## Remaining failures

| Test | Failure type | Problem summary |
| --- | --- | --- |
| `llvm_gcc_c_torture_20030330_1_c` | `BACKEND_FAIL` | `__builtin_constant_p(delay)` is lowered to a constant-false branch, but the dead `link_error()` block is still emitted, so final linking fails on the unresolved symbol. |
| `llvm_gcc_c_torture_strlen_4_c` | `BACKEND_FAIL` | Global initializer lowering for pointers-to-arrays is still invalid. Generated IR emits malformed globals such as `@pa0 = internal global ptr [ptr ...]`. |
| `llvm_gcc_c_torture_20001203_2_c` | `RUNTIME_FAIL` | GNU statement-expression side effects inside a ternary arm are lowered eagerly before the branch, so the `objfile != 0 ? ... : xmalloc(...)` path dereferences null before the condition is applied. |
| `llvm_gcc_c_torture_20010325_1_c` | `RUNTIME_FAIL` | Still failing at runtime; root cause not yet re-triaged after the current struct/layout fixes. |
| `llvm_gcc_c_torture_20021127_1_c` | `RUNTIME_FAIL` | Still failing at runtime; root cause not yet re-triaged after the current struct/layout fixes. |
| `llvm_gcc_c_torture_990130_1_c` | `RUNTIME_FAIL` | Still failing at runtime; root cause not yet re-triaged after the current struct/layout fixes. |
| `llvm_gcc_c_torture_pr70460_c` | `RUNTIME_FAIL` | Computed-goto label differences are still lowered incorrectly; static label-delta table contents are wrong. |
| `llvm_gcc_c_torture_align_3_c` | `RUNTIME_FAIL` | Still failing at runtime; likely alignment/layout related, but not re-triaged in this round. |
| `llvm_gcc_c_torture_comp_goto_1_c` | `RUNTIME_FAIL` | Still failing at runtime; likely in the same GCC computed-goto bucket as `pr70460_c`. |
| `llvm_gcc_c_torture_pr43784_c` | `RUNTIME_FAIL` | Still failing at runtime; root cause not yet re-triaged after the current struct/layout fixes. |
| `llvm_gcc_c_torture_pr71626_1_c` | `RUNTIME_FAIL` | Still failing at runtime; root cause not yet re-triaged after the current struct/layout fixes. |
| `llvm_gcc_c_torture_pr71626_2_c` | `RUNTIME_FAIL` | Still failing at runtime; root cause not yet re-triaged after the current struct/layout fixes. |
| `llvm_gcc_c_torture_scal_to_vec2_c` | `RUNTIME_FAIL` | Still failing at runtime; likely vector/scalar coercion related, but not re-triaged in this round. |
| `llvm_gcc_c_torture_strlen_5_c` | `RUNTIME_FAIL` | Still failing at runtime; likely adjacent to the `strlen-4.c` pointer/array lowering bucket, but not re-triaged in this round. |

## Suggested next targets

1. `llvm_gcc_c_torture_20030330_1_c`
2. `llvm_gcc_c_torture_20001203_2_c`
3. `llvm_gcc_c_torture_strlen_4_c`
4. computed-goto bucket: `pr70460_c`, `comp_goto_1_c`
