Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Trim Module and Function Orchestration

# Current Packet

## Just Finished

Completed Step 7: Trim Module and Function Orchestration. Used
`c4c-clang-tools` to confirm the traversal/module caller-callee boundary before
moving prepared function lookup, block traversal, instruction dispatch,
prologue/epilogue coordination, and terminator emission into
`src/backend/mir/riscv/codegen/prepared_function_emit.{hpp,cpp}`. Moved
prepared-module text orchestration into
`src/backend/mir/riscv/codegen/prepared_module_emit.{hpp,cpp}` and added both
new owner `.cpp` files to `src/backend/CMakeLists.txt`. `emit.cpp` now keeps
the public prepared-module wrapper, legacy direct-LIR tiny-add fallback,
`PhysReg` compatibility helpers, and assembler glue.

## Suggested Next

Next coherent packet: Step 8, broader backend validation for the completed
prepared emitter decomposition using the supervisor-selected broader proof.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- `emit_prepared_module(...)` remains the public wrapper in `emit.cpp`; the
  actual prepared-module orchestration lives in `emit_prepared_module_text(...)`.
- `prepared_module_emit.cpp` includes `emit.hpp` only for the existing public
  edge-publication append declaration; no edge-publication behavior moved in
  this packet.
- `PhysReg` and callee-saved constraint helpers still sit in public namespace
  near the bottom of `emit.cpp` as compatibility glue.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`

Result: passed; 23/23 `backend_rv64_runtime` tests passed after the build.
Proof log: `test_after.log`.
