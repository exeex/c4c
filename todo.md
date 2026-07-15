# Current Packet

Status: Active
Source Idea Path: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair anonymous layout / structured-call compatibility

## Just Finished

796's bounded pointer-subtraction cast-result repair is supervisor-accepted
with focused 4/4 proof. Its required clean-first comparable command completed
3034/3037; the only failures were `cpp_qualified_template_call_template_arg_perf`,
the 806-owned `llvm_gcc_c_torture_src_20060910_1_c` PHI family, and
`llvm_gcc_c_torture_src_pr28982b_c`. This enables the 801 retry only.

## Suggested Next

Step 2: evaluate the preserved dirty anonymous-layout/direct-complex
structured-call repair under 801's existing scope. Keep it unaccepted until
the Step 2 contract, fresh build, focused call/frontend/backend ladder, and
supervisor-accepted comparable full baseline are satisfied.

## Watchouts

Do not modify, discard, or credit the preserved unaccepted Step 2 hunks in
`call/args.cpp`, `call/target.cpp`, `verify.cpp`, or `frontend_hir_tests.cpp`
through this lifecycle return. Do not parse `LirTypeRef` display text, use
rendered diagnostics as argument type authority, weaken `LirCallOp`
mirror/signature checks, or add extractvalue field/index/result validation,
Raw-BIR, or generic aggregate work.

## Proof

801 Step 2 requires a fresh `cmake --build --preset default`, the focused
`^(frontend_hir_tests|frontend_lir_call_type_ref|backend_)$` ladder, and a
supervisor-accepted comparable full baseline before Step 3. The 796
clean-first 3034/3037 baseline enables this retry but does not accept 801's
preserved repair.
