Status: Active
Source Idea Path: ideas/open/270_aarch64_prologue_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Compiled Prologue Owner

# Current Packet

## Just Finished

Completed plan Step 2, `Add The Compiled Prologue Owner`, by adding
`src/backend/mir/aarch64/codegen/prologue.hpp` and
`src/backend/mir/aarch64/codegen/prologue.cpp` as the compiled AArch64 prologue
owner. The new owner exposes
`insert_prepared_frame_boundary_nodes(const module::FunctionLoweringContext&,
const prepare::PreparedControlFlowFunction&, module::MachineFunction&)` and
owns the existing simple fixed-frame boundary: the prepared-frame predicate,
the `FrameInstructionRecord`-backed machine-instruction construction, and
insertion of `PrologueSetup` / `EpilogueTeardown` nodes. `traversal.cpp` now
remains the traversal coordinator and calls the prologue owner after block
dispatch. `src/backend/CMakeLists.txt` now builds the new prologue translation
unit.

No ABI-visible behavior was intentionally changed. The moved boundary still
accepts only nonzero fixed prepared frames without dynamic stack plans and
without saved callee registers. This packet did not add callee-save,
dynamic-stack, frame-pointer, variadic-entry, outgoing-area, or concrete
save/restore behavior.

## Suggested Next

Execute Step 3 from `plan.md`: review whether any remaining route or record
integration should point at the compiled prologue boundary, keeping changes
behavior-preserving and narrower than the broad backend driver.

## Watchouts

`make_frame_instruction` and frame status/opcode/side-effect helpers still
belong to `instruction.{hpp,cpp}` for generic instruction consumers. Step 3
should avoid broad rewrites of calls, returns, memory, or module compile unless
there is a narrow compiled integration point to route through `prologue.*`.
`clang-format` is not installed in this container, so no formatter pass was
available after the edit.

## Proof

Ran the supervisor-selected proof:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.
Result: passed; all 139 selected backend tests passed. Proof log:
`test_after.log`.
