Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Common Low-Level Helpers

# Current Packet

## Just Finished

Completed Step 2 - Extract Common Low-Level Helpers.

Added `src/backend/mir/aarch64/codegen/calls_common.cpp` and
`src/backend/mir/aarch64/codegen/calls_common.hpp`, registered the new source in
`src/backend/CMakeLists.txt`, and made `calls.cpp` include the helper header.

Mechanically moved these helper definitions from the anonymous namespace in
`calls.cpp` into `calls_common.cpp`: `align_to`,
`incoming_stack_argument_size_bytes`,
`incoming_stack_argument_alignment_bytes`,
`outgoing_stack_argument_size_bytes`, `outgoing_stack_argument_bytes`,
`outgoing_stack_argument_base_register`, `entry_param_uses_incoming_stack`,
`named_incoming_stack_bytes`, `function_has_call`,
`fixed_frame_adjustment_bytes`, `va_start_overflow_area_stack_offset`,
`register_class_from_bank`, `register_display_name`,
`occupied_register_views`, and `prepared_clobber_expected_view`.

`calls_common.hpp` also declares `append_call_diagnostic` for `calls.cpp`
consumers. Its body was not duplicated because the first build found an
existing identical external implementation in `dispatch_diagnostics.cpp`; the
owned-file-safe resolution is to keep the common declaration and let the
existing implementation satisfy the symbol.

## Suggested Next

Execute the next mechanical extraction packet for the argument-source helper
cluster that now depends on `calls_common.hpp`, keeping the move-lowering body
in `calls.cpp` until its own packet.

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

## Proof

Step 2 proof recorded in `test_after.log`:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Result: green; all 3 selected tests passed.

Additional AST-backed checks run during the packet:

```bash
command -v c4c-clang-tool c4c-clang-tool-ccdb
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp fixed_frame_adjustment_bytes build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp va_start_overflow_area_stack_offset build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp register_class_from_bank build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp append_call_diagnostic build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp fixed_frame_adjustment_bytes build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp va_start_overflow_area_stack_offset build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp append_call_diagnostic build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp occupied_register_views build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp prepared_clobber_expected_view build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_common.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp outgoing_stack_argument_base_register build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp append_call_diagnostic build/compile_commands.json
```
