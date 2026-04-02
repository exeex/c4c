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
| `eastl_integer_sequence_simple.cpp` | `SEMA` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_integer_sequence_simple.cpp` | The `__underlying_type(T)` parser blocker is fixed. The current earliest failure is `tests/cpp/eastl/eastl_integer_sequence_simple.cpp:6:1` with `object has incomplete type: eastl::has_unique_object_representations`. |
| `eastl_type_traits_simple.cpp` | `SEMA` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_type_traits_simple.cpp` | Same `has_unique_object_representations` incomplete-type blocker at `tests/cpp/eastl/eastl_type_traits_simple.cpp:55:1`. The standalone workflow target remains useful historically but no longer matches the active frontier. |
| `eastl_utility_simple.cpp` | `SEMA` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_utility_simple.cpp` | Same `has_unique_object_representations` incomplete-type blocker at `tests/cpp/eastl/eastl_utility_simple.cpp:7:1`. |
| `eastl_memory_simple.cpp` | `SEMA` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_memory_simple.cpp` | Same `has_unique_object_representations` incomplete-type blocker at `tests/cpp/eastl/eastl_memory_simple.cpp:36:1`. |
| `eastl_tuple_simple.cpp` | `SEMA` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_tuple_simple.cpp` | Same `has_unique_object_representations` incomplete-type blocker at `tests/cpp/eastl/eastl_tuple_simple.cpp:16:1`. |
| `eastl_vector_simple.cpp` | `SEMA` | `build/c4cll --parse-only -I ref/EASTL/include -I ref/EABase/include/Common tests/cpp/eastl/eastl_vector_simple.cpp` | Same `has_unique_object_representations` incomplete-type blocker at `tests/cpp/eastl/eastl_vector_simple.cpp:32:1`; covered by the existing negative parse recipe. |

Current explicit workflow coverage:

- `run_eastl_parse_recipe.cmake`: reusable parse-only recipe for expected success
  or expected parser failure.
- `run_eastl_vector_parse_recipe.cmake`: negative parser regression recipe pinned
  to the current `eastl_vector_simple.cpp` failure.
- `run_eastl_type_traits_simple_workflow.cmake`: older end-to-end workflow kept
  for future reactivation once the `has_unique_object_representations`
  incomplete-type blocker is removed.
