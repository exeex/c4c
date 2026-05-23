Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Extract Byval And Aggregate Lane Helpers

# Current Packet

## Just Finished

Completed Step 4 - Extract Byval And Aggregate Lane Helpers.

Added `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp` and
`src/backend/mir/aarch64/codegen/calls_byval_aggregates.hpp`, registered the new
source in `src/backend/CMakeLists.txt`, and made `calls.cpp` include the new
helper header.

Mechanically moved the aggregate stack-copy primitives, aggregate register-lane
load/memory/destination/publication helpers, byval register-lane store
collection and prepared-source helpers, aggregate lane store memory construction,
and the byval ABI-size predicates out of `calls.cpp`.

Kept call lowering, preservation analysis, effects, and print helper bodies in
`calls.cpp`.

## Suggested Next

Execute the next mechanical extraction packet from the active plan, keeping
preservation-analysis, move-lowering, effects, and print-body changes out of
the byval aggregate helper slice unless that later packet explicitly owns them.

## Watchouts

- Unrelated dirty implementation/test files were already present when this
  lifecycle state was activated; do not revert or sweep them into a lifecycle
  commit.
- Keep this idea behavior-preserving and reject testcase-shaped shortcuts,
  expectation downgrades, or backend capability changes.
- `calls.cpp` was already dirty during this mapping packet; the AST evidence
  reflects the current worktree content.
- `append_call_diagnostic` already has an external implementation in
  `dispatch_diagnostics.cpp`; do not add a second same-signature body in
  `calls_common.cpp` unless that broader diagnostic ownership is explicitly
  changed.
- `calls_common.hpp` currently includes `calls.hpp` for shared call/codegen
  types. Later narrower headers can reduce that include after more clusters are
  split.
- Preservation-analysis helpers remain in `calls.cpp`: they are reserved for a
  later plan cluster and are not required by the Step 4 helper set.
- `calls.cpp` now includes `dispatch_lookup.hpp` and `float_ops.hpp` directly
  for existing external helper declarations from earlier split packets.
- `calls_byval_aggregates.cpp` includes `dispatch_lookup.hpp` for
  `find_value_home` and keeps tiny local copies of the GP-register equality and
  aggregate frame-slot direct-offset predicates instead of moving call-boundary
  print helpers.
- `clang-format` was unavailable in the environment.

## Proof

Step 4 proof recorded in `test_after.log`:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Result: green; all 3 selected tests passed.

Additional AST-backed checks run during the packet:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp collect_byval_register_lane_stores build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp collect_byval_register_lane_stores build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp aggregate_register_lane_scratch build/compile_commands.json
```
