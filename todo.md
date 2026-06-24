Status: Active
Source Idea Path: ideas/open/327_frontend_llvm_indirect_function_pointer_return_type_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Locate The Type Loss

# Current Packet

## Just Finished

No executor packet has run yet.

## Suggested Next

Start Step 1 by reproducing `c_testsuite_src_00124_c`, capturing the current
bad LLVM IR, and locating the indirect-call lowering point that chooses the
wrong return type.

## Watchouts

Do not weaken `c_testsuite_src_00124_c` or add named-case special handling for
`00124.c`, `f1`, `f2`, or local variable `p`.

## Proof

No proof has run yet for this active plan.
