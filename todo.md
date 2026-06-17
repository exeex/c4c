Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Extract Edge Publication If Clean

# Current Packet

## Just Finished

Completed Step 6: Extract Edge Publication If Clean. Used `c4c-clang-tools`
to confirm the edge-publication caller/callee boundary before moving the RV64
prepared edge-publication source rendering, route-agreement checks, move
adapter, and public consume/append entrypoint definitions from `emit.cpp` into
`src/backend/mir/riscv/codegen/prepared_edge_publication_emit.{hpp,cpp}`.
Added the new owner `.cpp` to `src/backend/CMakeLists.txt`; traversal dispatch
and public module glue remain in `emit.cpp`.

## Suggested Next

Next coherent packet: Step 7, trim module/function orchestration by extracting
prepared function/block traversal or prepared module orchestration into a
dedicated owner while keeping `emit.cpp` as public compatibility glue.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- `prepared_edge_publication_emit.cpp` now depends on frame immediate/load-offset
  helpers and prepared memory-address lookup helpers, but it does not require
  scalar, call, traversal, or module ownership to move with it.
- The public edge-publication declarations remain in `emit.hpp` for existing
  tests and callers; the definitions now live in the edge-publication owner.
- `PhysReg` and callee-saved constraint helpers still sit in public namespace
  near the bottom as compatibility glue.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`

Result: passed; 23/23 `backend_rv64_runtime` tests passed after the build.
Proof log: `test_after.log`.
