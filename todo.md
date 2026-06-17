Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract Scalar Returns and Scalar Operations

# Current Packet

## Just Finished

Completed Step 3: Extract Scalar Returns and Scalar Operations. Moved RV64
prepared scalar value-home lookup, register/immediate selection, move-to-register
support, integer binary lowering, cast/select lowering, compare branch helpers,
prepared pointer-add materialization, and scalar return emission from `emit.cpp`
into `src/backend/mir/riscv/codegen/prepared_scalar_emit.{hpp,cpp}`. Added the
new owner `.cpp` to `src/backend/CMakeLists.txt`; call lowering, local-memory
load/store lowering, traversal dispatch, edge publication, and public module
glue remain outside the scalar owner.

## Suggested Next

Next coherent packet: Extract local frame-slot and pointer-memory load/store
lowering into `prepared_local_memory_emit.{hpp,cpp}` without changing supported
forms. Move the existing i32 stack load/store bytewise helpers with that local
memory owner, and keep call lowering, edge publication, traversal dispatch, and
module glue out of the packet.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- `prepared_scalar_emit.hpp` now exposes value-home lookup and move helpers used
  by call and local-memory lowering; avoid growing it into call or memory
  lowering ownership.
- `emit_i32_load_from_stack_offset(...)` is narrowly declared through the scalar
  header because scalar value movement can load i32 stack homes. The broader
  i32 bytewise load/store helper pair still belongs with the next local-memory
  extraction.
- `PhysReg` and callee-saved constraint helpers still sit in public namespace
  near the bottom as compatibility glue.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`

Result: passed; 23/23 `backend_rv64_runtime` tests passed after the build.
Proof log: `test_after.log`.
