Status: Active
Source Idea Path: ideas/open/384_aarch64_dispatch_mechanical_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Extract Publication Helpers

# Current Packet

## Just Finished

Step 5 extracted the value, store, and edge publication helper cluster into
`dispatch_publication.cpp/hpp` and wired the new source into
`src/backend/CMakeLists.txt`. The moved cluster preserves publication ordering,
inline-asm note construction, stack-store publication, edge-source publication,
and current block entry publication behavior without adding lowering support.

## Suggested Next

Execute Step 6 from `plan.md`: use AST-backed queries to identify the
mechanically separable call boundary and indirect-call helper cluster before
moving call-related logic out of `dispatch.cpp`.

## Watchouts

- AST evidence recorded for Step 5:
  `c4c-clang-tool-ccdb function-signatures .../dispatch.cpp build/compile_commands.json`,
  `function-callees/function-callers emit_value_publication_to_register`,
  `function-callees/function-callers lower_store_local_value_publication`,
  plus `function-callees` on
  `emit_select_chain_value_to_register`,
  `make_select_chain_materialization_instruction`, and
  `lower_pending_store_global_stack_value_publications`.
- `dispatch.cpp` is now 3927 lines, below the source idea's first-pass target.
- `dispatch_publication.hpp` exposes several mechanically shared publication
  support helpers because residual call-boundary and missing-operand code still
  uses the same publication contracts. Keep Step 6 limited to call-boundary and
  indirect-call extraction rather than reshaping these publication APIs.
- Two publication support helper names were made publication-specific to avoid
  colliding with existing memory lowering symbols:
  `dispatch_publication_scalar_type_size_bytes` and
  `publication_parse_va_list_field_suffix`.

## Proof

Fresh Step 5 executor proof passed:

```bash
(cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure) > test_after.log 2>&1
```

Supervisor milestone proof also passed and is now preserved in `test_after.log`:

```bash
ctest --test-dir build -j10 --output-on-failure > test_after.log 2>&1
```

Full-suite result: 3381/3381 tests passed. Regression guard against the last
accepted full-suite baseline passed with no new failures.
