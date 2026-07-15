# LIR Instruction, Terminator, and Inline-Assembly Residual Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/closed/793_lir_to_new_bir_remaining_coverage_umbrella.md`
Related: `ideas/open/761_lir_call_signature_type_mirror_convergence.md`
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Establish one bounded residual instruction/terminator/inline-assembly
value-or-type authority handoff without reopening accepted CFG/PHI work or
parsing opaque assembly text.

## In Scope

- Select exactly one residual family only when its native value, edge, and
  type facts can be published and verified; retain inline-asm templates and
  constraints as opaque text.
- Record malformed-authority rejection, focused proof, and a one-row future
  receiver handoff where the selected family permits it.

## Out Of Scope

- Raw-BIR receipt, a combined residual sweep, prior CFG/PHI authority,
  type-mirror work owned by 761, or template/constraint parsing.

## Acceptance Criteria

- The selected family is explicit, bounded, natively structured, and covered
  by focused positive/negative producer proof; every nonselected residual stays
  fail closed.

## Active Baseline-Blocker Intake

The isolated accepted 810/795 tree's fresh comparable full gate passed
3013/3037 after `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure`. Twenty-two failures spanning positive/C++/c-testsuite and
GCC-torture coverage report `LirCastOp.result: expected operand kind mismatch
... got raw-text`. This selects only the native cast-result authority family
for 796's next trace; it does not authorize a combined residual sweep.

The same gate also has `frontend_lir_call_type_ref` (owned by 801) and
`llvm_gcc_c_torture_src_20060910_1_c` PHI authority (owned by 806). Do not
absorb either. Once this cast family is accepted or separately split, return
to 810's preserved Step 3 for the exact comparable full-baseline retry.

## Resumption Record: accepted cast-result authority return to 810

- Last accepted progress: Step 1, *Trace and select the native cast-result
  authority family*, was accepted in `e92437aca`. Step 2, *Repair the selected
  cast-result handoff*, was accepted in `387af7745`. The selected scalar
  `StmtEmitter::coerce` cast paths now publish native `LirValueId` and
  `LirTypeRef` authority; complex FPExt positive coverage and missing,
  invalid, duplicate, and foreign-authority rejection coverage were added.
  Vector and aggregate compatibility producers remain fail-closed.
- Interrupted runbook step and disposition: Step 3, *Prove the bounded route
  and return to 810*, is accepted for this bounded route. The fresh exact
  focused proof `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1, and the
  matching regression guard was accepted as non-decreasing (1/1 before/after).
- Exact return point: resume
  `ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md`
  unchanged at Step 3, *Prove the blocker and return control to 801*, and run
  exactly `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure`. This record does not claim that full gate passes or
  that 801/806 residuals are cleared.
- Remaining boundary: any nonselected cast producer family remains fail-closed
  and requires separately evidenced scope before work resumes under 796; 801
  structured-call and 806 PHI residual authority remain separate owners.

## Resumption Record: recurring cast-result authority return from 801

The original 796 Steps 1--3 remain accepted: native scalar cast tracing in
`e92437aca`, the scalar `StmtEmitter::coerce` handoff in `387af7745`, and its
focused proof/regression guard. This record does not reopen or downgrade that
accepted route.

801 Step 2's new full-tree comparison attempt observed `LirCastOp.result:
expected operand kind mismatch ... got raw-text` across positive, LLVM, and
c-testsuite coverage. Its fresh focused build/subset passed 7/7, but the
before attempt was not clean-first and has 24 failures; the foreground-capped
after log ended before a summary. Neither is a usable comparable guard, and
the stale rejected `test_baseline.new.log` candidate is not evidence.

796 Step 4, `Re-trace the recurrent cast-result raw-text family`, is accepted
as trace/selection only. A fresh build and the positive, LLVM, and c-testsuite
representatives selected the non-parameter pointer-subtraction producer/
immediate-handoff in `expr/binary.cpp`: raw `fresh_tmp` `PtrToInt` results and
their dependent subtraction/scaling results reach result verification before
cast-specific authority checks. This is distinct from the accepted scalar
`StmtEmitter::coerce` route. Step 5 may change only those selected result
values to native `fresh_value` authority and must provide nearby positive and
malformed-authority proof. Preserve 801's
unaccepted anonymous-layout/direct-complex structured-call hunks without
modification or credit.

796 Step 5 is supervisor-accepted as that bounded repair: both pointer-pointer
subtraction `PtrToInt` results, the immediate byte subtraction, and optional
element-size scaling in `expr/binary.cpp` now use native `fresh_value`
authority. Fresh focused proof passed 4/4 with `cmake --build --preset default
&& ctest --test-dir build -j --output-on-failure -R
'^(positive_sema_ok_expr_unary_binary_runtime_c|llvm_gcc_c_torture_src_20010904_1_c|c_testsuite_src_00037_c|frontend_lir_call_type_ref)$'`.
No verifier, scalar `StmtEmitter::coerce`, text, vector, aggregate, or GEP
route changed, and no canonical regression guard was made.

Exact return point: obtain or explicitly route a fresh comparable clean
baseline for the interrupted 801 Step 2 gate. Only after that baseline permits
the retry, reactivate 801 unchanged at Step 2 for its fresh build, focused
call/frontend/backend ladder, and supervisor-accepted full gate. This record
does not claim 801 Step 2 acceptance.

## Resumption Record: accepted bounded recurrence return to 801

- Last accepted progress: Steps 4 and 5 are accepted. Step 4 selected only the
  non-parameter pointer-subtraction producer/immediate-handoff family; Step 5
  published native `fresh_value` authority for its `PtrToInt`, subtraction,
  and optional scaling values. The supervisor accepted the focused 4/4 proof.
- Interrupted step and disposition: Step 6, *Accept the bounded 796 repair and
  return to 801*, is complete as a lifecycle return. The exact clean-first
  command `cmake --build --preset default --clean-first && ctest --test-dir
  build -j 32 --output-on-failure > test_before.log` completed with 3034/3037
  passed. Its only failures were `cpp_qualified_template_call_template_arg_perf`,
  `llvm_gcc_c_torture_src_20060910_1_c` (the 806-owned PHI family), and
  `llvm_gcc_c_torture_src_pr28982b_c`.
- Exact return point: resume
  `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` unchanged at
  Step 2, *Repair anonymous layout / structured-call compatibility*. Preserve
  its dirty anonymous-layout/direct-complex hunks uncredited, then run its
  fresh build, focused call/frontend/backend ladder, and supervisor-accepted
  full gate. This comparable baseline enables the retry; it does not accept
  801 Step 2.

## Reviewer Reject Signals

- Reject a catch-all instruction/terminator conversion, text parsing, receiver
  changes, testcase-shaped shortcuts, or weaker verifier/test contracts.
