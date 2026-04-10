# EASTL Bring-Up Inventory

Shared include flags for every bounded reproduction in this directory:

```sh
-I /workspaces/c4c/ref/EASTL/include -I /workspaces/c4c/ref/EABase/include/Common
```

Shared parse-only recipe:

```sh
/workspaces/c4c/build/c4cll --parse-only \
  -I /workspaces/c4c/ref/EASTL/include \
  -I /workspaces/c4c/ref/EABase/include/Common \
  /workspaces/c4c/tests/cpp/eastl/<case>.cpp
```

Observed with `build/c4cll` on 2026-04-10:

| Case | Current earliest failing stage | Reproducible command | Notes |
| --- | --- | --- | --- |
| `eastl_piecewise_construct_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_piecewise_construct_simple.cpp` | Canonical now completes in about `0.339s`; the older `mPart0` sema cluster is no longer the current frontier for this case. |
| `eastl_tuple_fwd_decls_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_tuple_fwd_decls_simple.cpp` | Canonical now completes in about `0.351s`; this header-only tuple forward-decl case is no longer failing in sema. |
| `eastl_integer_sequence_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_integer_sequence_simple.cpp` | Canonical now completes in about `1.236s`; the older `mPart0` / `mPart1` sema cluster is gone here too. |
| `eastl_type_traits_simple.cpp` | `RUNTIME_MISMATCH` | `cmake --build build --target eastl_type_traits_simple_workflow -j8` | `--parse-only` and `--dump-canonical` still succeed, and the old `std::byte` LLVM verifier failure is gone after scoped enums stopped misparsing as fake structs. The signedness half of the standalone workflow now agrees again, but the c4c-built binary still exits `22` while the host binary exits `0`, so the remaining frontier is alias-transformed EASTL trait semantics (`add_lvalue_reference_t`, `remove_reference_t`, `remove_cv_t`, `conditional_t`, `enable_if_t`) rather than parser/codegen corruption. |
| `eastl_utility_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_utility_simple.cpp` | `--parse-only` still succeeds in about `10.680s`, and `--dump-canonical` now completes in about `10.531s`. The old `eastl::pair` piecewise delegating-helper failure and the later canonical/HIR `SIGSEGV` are both gone after reserving re-entrant template-method lowering slots by `FunctionId`. |
| `eastl_memory_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_memory_simple.cpp` | `--parse-only` still succeeds, and `--dump-canonical` now completes too. The old shared `EASTL/memory.h` `eastl::size` undeclared-identifier cluster is gone after restoring local parameter lookup for unqualified names inside namespace functions. |
| `eastl_memory_uses_allocator_frontier.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_memory_uses_allocator_frontier.cpp` | Reduced header-only memory frontier now completes through both `--parse-only` and `--dump-canonical`. The old timeout was caused by the unsupported structured-binding bridge in `EASTL/utility.h` / `EASTL/tuple.h`, which is now disabled by predefined `EA_COMPILER_NO_STRUCTURED_BINDING` for C++ source profiles. |
| `eastl_tuple_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_tuple_simple.cpp` | `--parse-only` still succeeds, and `--dump-canonical` now completes within the old timeout window after the re-entrant template-method HIR fix that also unblocked `eastl_utility_simple.cpp`. This case is no longer the smallest active frontier. |
| `eastl_vector_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_vector_simple.cpp` | `--parse-only` still succeeds within the workflow timeout, and `--dump-canonical` now completes too. The old allocator `operator=` owner-resolution failure and `base_type::allocator_type` functional-cast rejection are both gone, so vector is no longer the active EASTL sema frontier. |

Current explicit workflow coverage:

- `run_eastl_parse_recipe.cmake`: reusable parse-only recipe for expected success
  or expected parser failure.
- `cpp_eastl_vector_parse_recipe`: positive parse-only workflow coverage for
  `eastl_vector_simple.cpp` now that the old parser timeout frontier is gone
  and the case reaches a later canonical/sema failure.
- `run_eastl_type_traits_simple_workflow.cmake`: active standalone workflow for
  the current `eastl_type_traits_simple.cpp` runtime frontier. It now runs past
  the earlier `std::byte` frontend/codegen blocker and the old
  `eastl::is_signed_v<int>` / `eastl::is_unsigned_v<int>` mismatch, but it
  still exposes the next alias-backed trait mismatch (`c4c` binary exit `22`
  versus host exit `0`).
- `cpp_eastl_memory_uses_allocator_parse_recipe`: positive workflow coverage for
  the reduced `EASTL/internal/memory_uses_allocator.h` frontier now that it is
  expected to parse successfully.
