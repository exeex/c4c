Status: Active
Source Idea Path: ideas/open/301_rv64_prepared_emitter_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Extract Direct Scalar Calls

# Current Packet

## Just Finished

Completed Step 5: Extract Direct Scalar Calls. Moved RV64 direct prepared
scalar call-plan validation, GPR argument/result register movement,
local-frame-address argument materialization, and direct call emission from
`emit.cpp` into `src/backend/mir/riscv/codegen/prepared_call_emit.{hpp,cpp}`.
Added the new owner `.cpp` to `src/backend/CMakeLists.txt`; edge publication,
traversal dispatch, and public module glue remain outside the call owner.

## Suggested Next

Next coherent packet: inspect edge-publication move adaptation and extract it
into a dedicated owner only if the dependency boundary is mechanical and
behavior-preserving.

## Watchouts

- This idea is behavior-preserving. Do not add rv64 capabilities, weaken tests,
  accept BIR/LLVM fallback, or split by testcase/idea number.
- `prepared_call_emit.cpp` depends on the scalar move helper for fallback
  argument materialization and on the frame helper for local-frame-address
  immediate range checks; keep the call owner limited to direct scalar calls.
- Unsupported indirect, vararg/FPR, stack-argument, memory-return, and aggregate
  call forms still fail closed through the moved validation checks.
- `PhysReg` and callee-saved constraint helpers still sit in public namespace
  near the bottom as compatibility glue.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'`

Result: passed; 23/23 `backend_rv64_runtime` tests passed after the build.
Proof log: `test_after.log`.
