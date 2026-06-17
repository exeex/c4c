Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Local Memory and Pointer Memory

# Current Packet

## Just Finished

Completed Step 4: Extract Local Memory and Pointer Memory. Moved RV64 prepared
local frame-slot loads/stores, pointer-value i32 memory loads, pointer
frame-slot address materialization handling, and the existing i32 stack
load/store bytewise helper pair from `emit.cpp` into
`src/backend/mir/riscv/codegen/prepared_local_memory_emit.{hpp,cpp}`. Added the
new owner `.cpp` to `src/backend/CMakeLists.txt`; call lowering, edge
publication, traversal dispatch, and public module glue remain outside the
local-memory owner.

## Suggested Next

Next coherent packet: Extract direct prepared call lowering into a dedicated
owner without changing supported call forms. Keep edge publication, traversal
dispatch, and module glue out of that packet.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- `prepared_scalar_emit.hpp` now exposes value-home lookup and move helpers used
  by call and local-memory lowering; avoid growing it into call or memory
  lowering ownership.
- `prepared_scalar_emit.cpp` now includes `prepared_local_memory_emit.hpp` only
  for `emit_i32_load_from_stack_offset(...)`, because scalar value movement can
  load i32 stack homes. The scalar header no longer declares local-memory
  helpers.
- `PhysReg` and callee-saved constraint helpers still sit in public namespace
  near the bottom as compatibility glue.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`

Result: passed; 23/23 `backend_rv64_runtime` tests passed after the build.
Proof log: `test_after.log`.
