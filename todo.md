Status: Active
Source Idea Path: ideas/open/384_aarch64_dispatch_mechanical_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Extract Lookup and Producer Lookup Helpers

# Current Packet

## Just Finished

Step 3 extracted the value lookup and producer lookup helper cluster into
`dispatch_lookup.cpp/hpp` and wired the new source into
`src/backend/CMakeLists.txt`. The moved cluster keeps the same prepared-value
lookup fallbacks, emitted-scalar lookup behavior, same-block producer scans, and
load-local producer checks.

## Suggested Next

Execute Step 4 from `plan.md`: use AST-backed queries to identify the branch
fusion helper cluster, then extract mechanically separable branch fusion helpers
into `dispatch_branch_fusion.cpp/hpp`.

## Watchouts

- AST evidence recorded for Step 3:
  `c4c-clang-tool-ccdb function-signatures .../dispatch.cpp build/compile_commands.json`,
  `function-callees/function-callers prepared_named_value_id`,
  `function-callees/function-callers find_value_home`,
  `function-callees/function-callers prepared_value_id`,
  `function-callees/function-callers make_named_prepared_result_register`,
  `function-callees/function-callers emitted_scalar_value_available`,
  `function-callees/function-callers find_same_block_scalar_producer`,
  `function-callees/function-callers has_same_block_load_local_producer`, and
  `function-callers is_scalar_call_argument_producer_opcode`.
- `is_scalar_call_argument_producer_opcode` is public in
  `dispatch_lookup.hpp` because AST and build proof showed both
  `find_same_block_scalar_producer` and the existing
  `materialize_scalar_call_argument_value` caller need it.
- Lookup helpers now live outside `dispatch.cpp`, so later extractions should
  include `dispatch_lookup.hpp` instead of recreating local lookup glue.

## Proof

Fresh Step 3 proof passed and is preserved in `test_after.log`:

```bash
(cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure) > test_after.log 2>&1
```
