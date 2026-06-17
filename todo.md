Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Frame, Label, and Basic Support

# Current Packet

## Just Finished

Completed Step 2: Extract Frame, Label, and Basic Support. Moved the RV64
signed-12-bit checks, stack frame/slot alignment helpers, prepared saved
register stack-end helper, sanitized local block label spelling helpers, and
frame-slot SP offset helpers from `emit.cpp` into
`src/backend/mir/riscv/codegen/prepared_frame_emit.{hpp,cpp}`. Added the new
owner `.cpp` to `src/backend/CMakeLists.txt`; `emit.cpp` now uses the owner
header and still owns scalar, call, local-memory, traversal dispatch, edge
publication, and public module glue behavior.

## Suggested Next

Next coherent packet: Extract scalar return/value movement and simple scalar
operations into `prepared_scalar_emit.{hpp,cpp}` without changing supported
forms. Keep call lowering, local-memory load/store lowering, edge publication,
and module glue in `emit.cpp` unless scalar extraction forces only narrow
forward declarations.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- `prepared_frame_emit.hpp` is now a shared support surface used by remaining
  owner extractions; avoid turning it into a scalar/local-memory catch-all.
- `emit_i32_load_from_stack_offset(...)` remains a local-memory helper with an
  existing forward declaration; do not pull bytewise stack load/store helpers
  into scalar extraction unless a narrow declaration is enough.
- `PhysReg` and callee-saved constraint helpers still sit in public namespace
  near the bottom as compatibility glue.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`

Result: passed; 23/23 `backend_rv64_runtime` tests passed after the build.
Proof log: `test_after.log`.
