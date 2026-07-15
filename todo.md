# Current Packet

Status: Active
Source Idea Path: ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Accept the bounded 796 repair and return to 801

## Just Finished

796 Step 5 is supervisor-accepted as a bounded repair. In `expr/binary.cpp`,
pointer-pointer subtraction gives both `PtrToInt` outputs, the immediate byte
subtraction, and optional element-size scaling native `fresh_value(ctx)`
authority. No verifier, scalar-coerce, text, vector, aggregate, or GEP route
changed.

## Suggested Next

Step 6: obtain or explicitly route a fresh comparable clean baseline for the
interrupted 801 Step 2 gate. Do not claim 801 accepted or reactivate it until
that baseline permits the retry.

## Watchouts

Preserve 801's dirty anonymous-layout/direct-complex structured-call hunks
unchanged and uncredited. Do not broaden this repair into GEP, vector,
aggregate, generic casts, verifier changes, or Raw-BIR/backend work. The
focused Step 5 proof is not a canonical regression guard.

## Proof

Focused Step 5 proof passed: `cmake --build --preset default && ctest --test-dir build
-j --output-on-failure -R
'^(positive_sema_ok_expr_unary_binary_runtime_c|llvm_gcc_c_torture_src_20010904_1_c|c_testsuite_src_00037_c|frontend_lir_call_type_ref)$'`.
All four named representatives passed after rebuilding `binary.cpp`. No
canonical before/after regression guard was made.
