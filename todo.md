Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 8
Current Step Title: Extract Call Printing And Record Construction

# Current Packet

## Just Finished

Completed Step 8 - Extract Call Printing And Record Construction.

Added `src/backend/mir/aarch64/codegen/calls_printing.cpp` and
`src/backend/mir/aarch64/codegen/calls_printing.hpp`, registered the new source
in `src/backend/CMakeLists.txt`, and made `calls.cpp` include the new printing
header.

Mechanically moved the call-boundary status helpers, call/call-boundary
instruction record construction helpers, call print helpers, frame-slot and
aggregate-lane print support, and the local call-boundary machine-instruction
wrapper out of `calls.cpp`.

Kept call-plan lookup, required-plan diagnostics, preserve-effect publication
state, and prepared call lowering in `calls.cpp`. `calls_printing.cpp` declares
`publish_prepared_call_preserve_effects()` locally because the moved call record
constructor still needs that state owned by `calls.cpp`.

## Suggested Next

Run supervisor-side review or validation for the accumulated mechanical split,
then choose the next active-plan packet or lifecycle close/split decision.

## Watchouts

- Unrelated dirty implementation/test files were already present when this
  lifecycle state was activated; do not revert or sweep them into a lifecycle
  commit.
- Keep this idea behavior-preserving and reject testcase-shaped shortcuts,
  expectation downgrades, or backend capability changes.
- `append_call_diagnostic` already has an external implementation in
  `dispatch_diagnostics.cpp`; do not add a second same-signature body in
  `calls_common.cpp` unless that broader diagnostic ownership is explicitly
  changed.
- `calls_common.hpp` currently includes `calls.hpp` for shared call/codegen
  types. Later narrower headers can reduce that include after more clusters are
  split.
- `calls_printing.cpp` includes `calls_byval_aggregates.hpp`, `calls_effects.hpp`,
  `float_ops.hpp`, `machine_printer.hpp`, `memory.hpp`, and `variadic.hpp` for
  dependencies exposed by the moved print/status/record cluster.
- `calls_moves.cpp` already has its own immediate-cast publication helper copy;
  this packet did not touch that unowned file.
- `clang-format` was unavailable in the environment.

## Proof

Step 8 proof recorded in `test_after.log`:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Result: green; all 3 selected tests passed.

Additional AST-backed checks run during the packet:

```bash
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_call_instruction build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp make_call_instruction build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_printing.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_printing.cpp make_call_instruction build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_printing.cpp print_call_boundary_move build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_printing.cpp call_boundary_move_selection_status build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_printing.cpp print_aggregate_register_lane_publication_lines build/compile_commands.json
```
