Status: Active
Source Idea Path: ideas/open/327_frontend_llvm_indirect_function_pointer_return_type_regression.md
Source Plan Path: plan.md
Current Step ID: 2-3
Current Step Title: Add Focused Regression Coverage and Repair Indirect Call Result Typing

# Current Packet

## Just Finished

Completed Steps 2 and 3 by adding focused frontend LIR coverage for a nested
function-pointer-returning indirect call and repairing the shared
`FnPtrSig` return-type handoff. `sig_return_type()` now preserves the patched
`FnPtrSig::return_type.spec` when it explicitly carries function-pointer return
metadata, so indirect-call result typing remains `ptr` before
`resolve_call_target_info()` derives the LLVM return type.

## Suggested Next

Supervisor can review and commit this focused slice, or run broader validation
if treating `c_testsuite_src_00124_c` as a milestone.

## Watchouts

The fix is type-driven and does not weaken `c_testsuite_src_00124_c` or key on
`00124.c`, `f1`, `f2`, or local variable names. The delegated backend case glob
`tests/backend/codegen/cases/*function*pointer*.c` does not exist in this
checkout, so no backend case file was changed.

## Proof

Ran the delegated proof:
`set -o pipefail; (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|c_testsuite_src_00124_c)$') > test_after.log 2>&1`

Result: passed. `frontend_lir_call_type_ref` and `c_testsuite_src_00124_c`
both passed.
Proof log: `test_after.log`.
