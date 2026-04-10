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
| `eastl_type_traits_simple.cpp` | `SEMA` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_type_traits_simple.cpp` | `--parse-only` now succeeds. Canonical/sema reaches the same undeclared-identifier cluster as `eastl_integer_sequence_simple.cpp`, starting around `tests/cpp/eastl/eastl_type_traits_simple.cpp:766:1` with `mPart0` / `mPart1` and related locals missing from EASTL internals. |
| `eastl_utility_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_utility_simple.cpp` | `--parse-only` still succeeds in about `10.680s`, and `--dump-canonical` now completes in about `10.531s`. The old `eastl::pair` piecewise delegating-helper failure and the later canonical/HIR `SIGSEGV` are both gone after reserving re-entrant template-method lowering slots by `FunctionId`. |
| `eastl_memory_simple.cpp` | `SEMA` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_memory_simple.cpp` | `--parse-only` now succeeds again. The active blocker has moved past the old parse timeout into a later canonical/sema timeout under the heavier libstdc++ / EASTL stack, so this case is no longer blocked on the structured-binding bridge. |
| `eastl_memory_uses_allocator_frontier.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_memory_uses_allocator_frontier.cpp` | Reduced header-only memory frontier now completes through both `--parse-only` and `--dump-canonical`. The old timeout was caused by the unsupported structured-binding bridge in `EASTL/utility.h` / `EASTL/tuple.h`, which is now disabled by predefined `EA_COMPILER_NO_STRUCTURED_BINDING` for C++ source profiles. |
| `eastl_tuple_simple.cpp` | `PASS` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_tuple_simple.cpp` | `--parse-only` still succeeds, and `--dump-canonical` now completes within the old timeout window after the re-entrant template-method HIR fix that also unblocked `eastl_utility_simple.cpp`. This case is no longer the smallest active frontier. |
| `eastl_vector_simple.cpp` | `SEMA` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_vector_simple.cpp` | `--parse-only` now succeeds again within the workflow timeout, so the old parser timeout and incomplete-type frontier are gone. `--dump-canonical` now reaches a real semantic cluster instead, starting with `ref/EASTL/include/EASTL/vector.h:2066:2: error: conflicting types for function 'operator_eq'`, then the same `allocator.h:210:16` incompatible-return and `memory.h` `eastl::size` failures already visible from `eastl_memory_simple.cpp`. |

Current explicit workflow coverage:

- `run_eastl_parse_recipe.cmake`: reusable parse-only recipe for expected success
  or expected parser failure.
- `cpp_eastl_vector_parse_recipe`: positive parse-only workflow coverage for
  `eastl_vector_simple.cpp` now that the old parser timeout frontier is gone
  and the case reaches a later canonical/sema failure.
- `run_eastl_type_traits_simple_workflow.cmake`: older end-to-end workflow kept
  for future reactivation once the current `is_complete_type__spec_167` /
  `function_detail.h` frontiers move.
- `cpp_eastl_memory_uses_allocator_parse_recipe`: positive workflow coverage for
  the reduced `EASTL/internal/memory_uses_allocator.h` frontier now that it is
  expected to parse successfully.
