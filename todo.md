Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Extract Move Lowering

# Current Packet

## Just Finished

Completed Step 6 - Extract Move Lowering.

Added `src/backend/mir/aarch64/codegen/calls_moves.cpp` and
`src/backend/mir/aarch64/codegen/calls_moves.hpp`, registered the new source in
`src/backend/CMakeLists.txt`, and made `calls.cpp` include the new move
lowering header.

Mechanically moved the call-boundary move-lowering entry points and their local
support out of `calls.cpp`: before-call, after-call, before-return, and value
move lowering; outgoing stack-base setup; stack/value move assembly helpers;
aggregate stack/byval stack move helpers; and immediate-cast call-argument
publication helpers used by the moved lowering path.

Kept call lowering, effect/public instruction construction, and print-body
ownership in `calls.cpp`; `calls_moves.cpp` uses tiny local copies of the
machine-instruction wrapper, operand-to-effect conversion, and frame-slot address
materialization needed by the moved helpers.

## Suggested Next

Execute the next mechanical extraction packet from the active plan, keeping
print-body ownership and broad effect construction out of the move-lowering
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
- `calls_moves.cpp` duplicates the small local operand-effect conversion and
  frame-slot address materialization helpers because those bodies still serve
  print/effect construction in `calls.cpp`, which was outside this packet.
- `clang-format` was unavailable in the environment.

## Proof

Step 6 proof recorded in `test_after.log`:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Result: green; all 3 selected tests passed.

Additional AST-backed checks run during the packet:

```bash
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp lower_before_call_move build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp lower_after_call_move build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp lower_before_call_moves build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp lower_value_moves build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_moves.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_moves.cpp lower_before_call_move build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls_moves.cpp lower_before_call_move build/compile_commands.json
```
