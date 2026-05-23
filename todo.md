Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Extract Preservation Analysis

# Current Packet

## Just Finished

Completed Step 5 - Extract Preservation Analysis.

Added `src/backend/mir/aarch64/codegen/calls_preservation.cpp` and
`src/backend/mir/aarch64/codegen/calls_preservation.hpp`, registered the new
source in `src/backend/CMakeLists.txt`, and made `calls.cpp` include the new
preservation header.

Mechanically moved the prepared-block dominance helpers, move-bundle lookup,
prior preserved-value search helpers, later-use checks, prior preserved argument
source construction, and callee-saved preservation home population/republication
helpers out of `calls.cpp`.

Kept call lowering, effects, printing helpers, and the existing external
`find_value_home`/`prepared_value_id` ownership in their current files.

## Suggested Next

Execute the next mechanical extraction packet from the active plan, keeping
move-lowering, effects, and print-body changes out of the preservation helper
slice unless that later packet explicitly owns them.

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
- `calls.cpp` now includes `dispatch_lookup.hpp` and `float_ops.hpp` directly
  for existing external helper declarations from earlier split packets.
- `calls_byval_aggregates.cpp` includes `dispatch_lookup.hpp` for
  `find_value_home` and keeps tiny local copies of the GP-register equality and
  aggregate frame-slot direct-offset predicates instead of moving call-boundary
  print helpers.
- `calls_preservation.cpp` includes `dispatch_lookup.hpp` for the existing
  `find_value_home` dependency. `find_value_home` and `prepared_value_id` were
  listed in the source idea, but they already have shared external ownership in
  `dispatch_lookup.cpp/hpp`, which was outside this packet's owned files.
- `calls_preservation.cpp` uses a tiny local machine-instruction wrapper for
  moved callee-saved preservation construction instead of moving the broader
  call-boundary machine-instruction helper out of `calls.cpp`.
- `clang-format` was unavailable in the environment.

## Proof

Step 5 proof recorded in `test_after.log`:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Result: green; all 3 selected tests passed.

Additional AST-backed checks run during the packet:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_prior_preserved_value_for_call_argument build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp preserved_value_has_later_non_call_use build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_prior_preserved_call_argument_source build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_move_bundle build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_prior_preserved_value_for_call_argument build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_prior_preserved_value_for_value build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp find_prior_stack_preserved_value_before_instruction build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_preservation.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_callee_saved_preservation_home_population build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_preservation.cpp make_callee_saved_preservation_home_population build/compile_commands.json
```
