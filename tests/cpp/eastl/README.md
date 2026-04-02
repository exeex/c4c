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

Observed with `build/c4cll` on 2026-04-02:

| Case | Current earliest failing stage | Reproducible command | Notes |
| --- | --- | --- | --- |
| `eastl_piecewise_construct_simple.cpp` | `SEMA` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_piecewise_construct_simple.cpp` | `--parse-only` succeeds; canonical/sema fails with undeclared identifiers such as `mPart0` from instantiated EASTL internals. |
| `eastl_tuple_fwd_decls_simple.cpp` | `SEMA` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_tuple_fwd_decls_simple.cpp` | `--parse-only` succeeds; canonical/sema fails with the same undeclared-identifier cluster as `piecewise_construct`. |
| `eastl_integer_sequence_simple.cpp` | `SEMA` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_integer_sequence_simple.cpp` | `--parse-only` now succeeds. The next frontier matches the existing Step 2 sema cluster: canonical/sema stops around `tests/cpp/eastl/eastl_integer_sequence_simple.cpp:766:1` with undeclared identifiers such as `mPart0` and `mPart1` from instantiated EASTL internals. |
| `eastl_type_traits_simple.cpp` | `SEMA` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_type_traits_simple.cpp` | `--parse-only` now succeeds. Canonical/sema reaches the same undeclared-identifier cluster as `eastl_integer_sequence_simple.cpp`, starting around `tests/cpp/eastl/eastl_type_traits_simple.cpp:766:1` with `mPart0` / `mPart1` and related locals missing from EASTL internals. |
| `eastl_utility_simple.cpp` | `PARSE` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_utility_simple.cpp` | Parse-only now succeeds after the nested variable-template parser fix, so this case is ready for canonical / semantic follow-up. |
| `eastl_memory_simple.cpp` | `PARSE` | `build/c4cll --parser-debug --parser-debug-tentative --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_memory_simple.cpp` | The old `function_detail.h` dot-token parser blocker is gone, but `--parse-only`, `--parser-debug`, and `--dump-canonical` still time out under the heavier libstdc++ / EASTL stack. Current progress reaches much later parser work in `/usr/include/c++/14/type_traits`, `/usr/include/c++/14/tuple`, and `/usr/include/c++/14/bits/uses_allocator_args.h` before timing out, so the next blocker is no longer the old `allocator.deallocate` statement shape. |
| `eastl_tuple_simple.cpp` | `PARSE` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_tuple_simple.cpp` | `--dump-canonical` still times out after 20s with no terminal diagnostics, but the earlier `TupleLeaf<...>::operator=` and `function_detail.h` parser blockers are both cleared. The next actionable frontier still needs a smaller reduction from the remaining libstdc++ / EASTL parse pressure. |
| `eastl_vector_simple.cpp` | `PARSE` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_vector_simple.cpp` | The old `ref/EASTL/include/EASTL/internal/function_detail.h:237:16` `unexpected token in expression: .` parser blocker is fixed. The current negative workflow is now pinned to the later `ref/EASTL/include/EASTL/internal/function.h:66:26` incomplete-type cluster (`object has incomplete type: eastl::internal::function_detail`) that surfaces before the parse-only timeout. |

Current explicit workflow coverage:

- `run_eastl_parse_recipe.cmake`: reusable parse-only recipe for expected success
  or expected parser failure.
- `run_eastl_vector_parse_recipe.cmake`: negative workflow recipe pinned to the
  current `eastl_vector_simple.cpp` incomplete-type frontier in
  `ref/EASTL/include/EASTL/internal/function.h`.
- `run_eastl_type_traits_simple_workflow.cmake`: older end-to-end workflow kept
  for future reactivation once the current `is_complete_type__spec_167` /
  `function_detail.h` frontiers move.
