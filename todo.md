Status: Active
Source Idea Path: ideas/open/384_aarch64_dispatch_mechanical_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Extract Call Boundary and Indirect Call Helpers

# Current Packet

## Just Finished

Step 6 extracted the call boundary, indirect call, scalar call-argument
materialization, call-result publication, before-call move ordering, missing
frame-slot call argument materialization, and stack-preserved call value helper
cluster into `dispatch_calls.cpp/hpp`. The new source is wired into
`src/backend/CMakeLists.txt`, and `dispatch.cpp` calls through the same helper
names without adding call lowering support or changing test expectations.

## Suggested Next

Execute Step 7 from `plan.md`: review the residual `dispatch.cpp` shape,
confirm the public dispatch API remains small, check extracted files are normal
CMake sources, and run the close-readiness validation selected by the
supervisor.

## Watchouts

- AST evidence recorded for Step 6:
  `c4c-clang-tool-ccdb function-signatures .../dispatch.cpp build/compile_commands.json`,
  `function-callees/function-callers materialize_call_boundary_source_to_destination`,
  `function-callees/function-callers materialize_indirect_call_callee_to_prepared_register`,
  `function-callees/function-callers lower_scalar_call_argument_producers`,
  `function-callees/function-callers publish_stack_preserved_call_values`,
  plus `function-callees` on `materialize_scalar_call_argument_value`,
  `order_before_call_moves_for_source_preservation`, and
  `materialize_missing_frame_slot_call_arguments`.
- AST showed the moved public Step 6 surface was directly called from
  `dispatch_prepared_block`; the smaller local-load/store helpers,
  indirect-callee scratch selection helpers, preserved-call-value lookup
  helpers, and `find_bir_value_for_prepared_name` moved with that surface as
  same-cluster support.
- `dispatch.cpp` is now 2745 lines after the Step 6 extraction.
- `dispatch_calls.cpp` includes the call wrapper that still delegates to the
  existing dynamic-stack, variadic, prepared-call, diagnostic, lookup,
  publication, and branch-fusion helpers. This is a mechanical split only, not
  a move into long-term `calls.cpp` ownership.

## Proof

Fresh Step 6 executor proof passed:

```bash
(cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure) > test_after.log 2>&1
```

Result: 3/3 selected tests passed. `test_after.log` contains the fresh proof
output from the delegated command.
