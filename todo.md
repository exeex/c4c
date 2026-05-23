Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Extract Argument Sources And Bindings

# Current Packet

## Just Finished

Completed Step 3 - Extract Argument Sources And Bindings.

Added `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp` and
`src/backend/mir/aarch64/codegen/calls_argument_sources.hpp`, registered the new
source in `src/backend/CMakeLists.txt`, and made `calls.cpp` include the new
helper header.

Mechanically moved the prepared argument/result binding lookup helpers,
prepared register/immediate/f128 argument-source helpers, frame-slot/local-frame
address/stack/aggregate argument source construction helpers, and the prepared
indirect-callee plus memory-return storage helpers.

After supervisor route review, narrowed the slice by restoring byval aggregate
register-lane helpers and preservation-analysis helpers to `calls.cpp` for the
later clusters.

## Suggested Next

Execute the next mechanical extraction packet from the active plan, keeping
byval aggregate-lane and preservation-analysis work out of Step 3 unless that
later packet explicitly owns it.

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
- Byval aggregate register-lane helpers remain in `calls.cpp`: they are reserved
  for later plan clusters and also feed the current aggregate lane print/copy
  paths.
- Preservation-analysis helpers remain in `calls.cpp`: they are reserved for a
  later plan cluster and are not required by the narrowed Step 3 helper set.
- `calls.cpp` now includes `dispatch_lookup.hpp` and `float_ops.hpp` directly
  for existing external helper declarations that Step 3 no longer re-exports.

## Proof

Step 3 proof recorded in `test_after.log`:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Result: green; all 3 selected tests passed.

Additional AST-backed checks run during the packet:

```bash
command -v c4c-clang-tool c4c-clang-tool-ccdb
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_prepared_argument_plan build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_register_operand_from_prepared_authority build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_register_operand_from_prepared_authority build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_frame_slot_call_argument_source build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_frame_slot_call_argument_source build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_argument_sources.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_indirect_callee_register build/compile_commands.json
```
