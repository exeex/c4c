# Current Packet

Status: Active
Source Idea Path: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair anonymous layout / structured-call compatibility

## Just Finished

- Step 1 trace/selection is accepted in `827dae5bd3`: the bounded native
  anonymous-layout contract and its 754 handoff boundary were identified.
- Step 2 implementation commit `201f229d3` is unaccepted/regressed. Its
  post-commit full-suite candidate introduced 48 failures and was rejected by
  `scripts/plan_review_state.py reject-baseline`.
- Direct evidence: `ctest --test-dir build -R '^frontend_lir_call_type_ref$'
  --output-on-failure` fails with `LirVerifyError:
  LirCallOp.callee_signature: structured callee signature does not match call
  arguments`. Earlier focused frontend-HIR and backend proof was insufficient
  for this shared LIR call path.

## Suggested Next

- Step 2: repair the anonymous-layout / direct-complex structured-call
  compatibility regression. Keep the native field carrier checked and repair
  the construction/verification mismatch; do not weaken the signature
  verifier. Return to Step 3 only after all listed proof, including an accepted
  full baseline, is fresh.

## Watchouts

- Do not parse `LirTypeRef` display text, weaken `LirCallOp` signature checks,
  or add extractvalue field/index/result validation, Raw-BIR, or generic
  aggregate work. `backend_lir_to_bir_interface` covers malformed layout
  cases, but passing it does not replace the call-signature or full-baseline
  gate. 754 resumes only after Step 3's accepted handoff.

## Proof

- Required fresh ladder: `cmake --build --preset default`; `ctest --test-dir
  build -R '^frontend_lir_call_type_ref$' --output-on-failure`; `ctest
  --test-dir build -R '^frontend_hir_tests$' --output-on-failure` (direct-complex
  coverage); `ctest --test-dir build -R '^backend_lir_to_bir_interface$'
  --output-on-failure`; then supervisor-owned `ctest --test-dir build -j
  --output-on-failure` recorded in `test_after.log`, compared with
  `test_before.log`, and accepted with `scripts/plan_review_state.py
  accept-baseline` before Step 3.
