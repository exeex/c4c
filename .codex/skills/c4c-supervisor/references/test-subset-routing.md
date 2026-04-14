# Test Subset Routing

Use this reference when the supervisor must choose the proving command for an
executor packet and for `test_before.log` baseline capture.

Default proof shape:

```bash
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '<matching-regex>'
```

Task-to-subset routing:

- backend, rv64, x86, codegen, lowering, ABI, instruction selection, register
  allocation, backend helpers:
  use `^backend_`
- focused HIR work:
  use `^frontend_hir_tests$`
- C negative diagnostics or parser rejection paths:
  use `^negative_tests_`
- verify-surface diagnostics:
  use `^verify_tests_`
- C positive semantic paths:
  use `^positive_sema_`
- standalone preprocessor work:
  use `^preprocessor_`
- C++ frontend smoke or stage coverage:
  use `^frontend_cxx_`
- tiny driver smoke path:
  use `^tiny_c2ll_tests$`
- split LLVM smoke path:
  use `^positive_split_llvm_`
- review harness work:
  use `^ccc_review_`

Selection rules:

1. Prefer the narrowest subset that honestly proves the owned slice.
2. If the task is clearly backend work and no better proof is specified,
   default to `^backend_`.
3. If multiple narrow subsets seem plausible, choose the one closest to the
   changed behavior, not the largest one.
4. If no narrow subset credibly proves the slice, the supervisor should decide
   the required broader proof before delegating.
5. Once the supervisor chooses the proving command, use that exact command for
   both `test_before.log` and `test_after.log`.
