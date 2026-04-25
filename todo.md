Status: Active
Source Idea Path: ideas/open/06_bir-call-lowering-exit-from-memory-coordinator.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Declare Call Lowering Member

# Current Packet

## Just Finished

Step 1 declared `lower_call_inst(const c4c::codegen::lir::LirCallOp& call,
std::vector<bir::Inst>* lowered_insts)` exactly once as a
`BirFunctionLowerer` member in `src/backend/bir/lir_to_bir/lowering.hpp`.

## Suggested Next

Continue with Step 2 in `plan.md`: move only the existing `LirCallOp` lowering
body from `memory/coordinator.cpp` into `BirFunctionLowerer::lower_call_inst`,
preferably in `calling.cpp`.

## Watchouts

Keep this as a behavior-preserving placement cleanup. Do not change call ABI,
pointer provenance, memory intrinsic behavior, or test expectations. Keep
`try_lower_direct_memory_intrinsic_call` integration intact when moving the
body.

## Proof

Ran `cmake --build --preset default --target c4c_codegen && ctest --test-dir
build -j --output-on-failure -R '^backend_'`; build succeeded and all enabled
backend tests passed. Proof output is preserved in `test_after.log`.
