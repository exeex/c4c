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
| `eastl_memory_simple.cpp` | `PARSE` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_memory_simple.cpp` | `--parse-only` still stalls past a 5s timeout with no terminal output. After the qualified-template `::operator=` parser fix, `--dump-canonical` advances into later parser failures in `/usr/include/c++/14/bits/ranges_util.h`, `/usr/include/c++/14/tuple`, and the existing `ref/EASTL/include/EASTL/internal/function_detail.h:237:16` `unexpected token in expression: .` frontier. |
| `eastl_tuple_simple.cpp` | `PARSE` | `build/c4cll --dump-canonical -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_tuple_simple.cpp` | `--parse-only` still stalls past a 5s timeout with no terminal output. The earlier `ref/EASTL/include/EASTL/tuple.h:346` `TupleLeaf<...>::operator=` parser blocker is fixed; the next visible frontier now matches later parser pressure in `/usr/include/c++/14/bits/ranges_util.h`, `/usr/include/c++/14/tuple`, and `ref/EASTL/include/EASTL/internal/function_detail.h:237:16`. |
| `eastl_vector_simple.cpp` | `PARSE` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_vector_simple.cpp` | The earlier incomplete-type blocker is gone. The current parser frontier is `ref/EASTL/include/EASTL/internal/function_detail.h:237:16` with `unexpected token in expression: .`; covered by the negative vector parse recipe. |

Current explicit workflow coverage:

- `run_eastl_parse_recipe.cmake`: reusable parse-only recipe for expected success
  or expected parser failure.
- `run_eastl_vector_parse_recipe.cmake`: negative parser regression recipe pinned
  to the current `eastl_vector_simple.cpp` failure.
- `run_eastl_type_traits_simple_workflow.cmake`: older end-to-end workflow kept
  for future reactivation once the current `is_complete_type__spec_167` /
  `function_detail.h` frontiers move.
