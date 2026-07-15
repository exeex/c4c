# Current Packet

Status: Active
Source Idea Path: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair anonymous layout / structured-call compatibility
你該做code review了

## Just Finished

- 802 is accepted and closed in `8218993a5`: it validates a structured `selector_type_ref` before comparing its integer width to the selector-selected definition. Nearby coverage rejects missing, non-integer, stale-width, and same-width incoherent references without display text being authoritative.
- 801 Step 1 trace/selection remains accepted in `827dae5bd3`. Its preserved `args.cpp`, `target.cpp`, and `verify.cpp` Step 2 repair plus the related `frontend_hir_tests.cpp` changes remain unaccepted and unmodified.

## Suggested Next

- Step 2: evaluate the preserved native anonymous-layout/structured-call repair. Keep native field facts checked and do not weaken the argument-mirror or callee-signature contracts. Obtain the focused call/frontend/backend ladder and supervisor-accepted full baseline before considering Step 3.

## Watchouts

- Do not parse `LirTypeRef` display text, use rendered diagnostics as argument type authority, weaken `LirCallOp` mirror/signature checks, or add extractvalue field/index/result validation, Raw-BIR, or generic aggregate work. 802 is closed; its selector contract is an environmental prerequisite, not accepted 801 progress.

## Proof

- 802 accepted proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`; matching regression-guard logs passed (`test_before.log` / `test_after.log`).
- 801 Step 2 still requires a fresh build, focused call/frontend/backend checks, and the supervisor-accepted full baseline; the 802 proof does not satisfy that gate.
