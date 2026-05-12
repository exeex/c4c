# Current Packet

Status: Active
Source Idea Path: ideas/open/181_function_pointer_signature_type_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate and Summarize

## Just Finished

Step 5 from `plan.md`: validated and summarized the selected structured
function-pointer indirect-call signature path for supervisor/plan-owner review.

Validated path:

- Metadata-rich indirect calls now carry `LirCallOp::callee_signature` from
  HIR `FnPtrSig` through LIR call construction. The carrier records return
  type ref, fixed parameter ABI type spellings, fixed parameter type refs,
  variadic state, unspecified-parameter-list state, and void-parameter-list
  state when available.
- Existing rendered call spelling is preserved as display/compatibility text:
  `callee_type_suffix`, `args_str`, printed LIR, and final output syntax remain
  stable.
- LIR verification checks the structured callee signature against the call
  result, fixed parameter mirrors, and call arguments when metadata is present.
- BIR indirect-call lowering prefers `LirCallOp::callee_signature` over
  reparsing `callee_type_suffix`. The backend handoff coverage proves stale
  suffix text is not semantic authority for metadata-rich indirect calls, and
  mismatched structured metadata fails closed instead of falling back to
  rendered text.
- Focused collision coverage exists for fixed scalar indirect calls, variadic
  function-pointer state, unspecified parameter lists, stale suffix
  compatibility, and BIR preference/fail-closed behavior. Same-feature backend
  indirect aggregate and variadic route tests remained green.

## Suggested Next

This runbook appears complete for the selected bounded path. Supervisor should
route to plan-owner review/closure decision rather than assigning another
implementation packet from this runbook.

## Watchouts

- Remaining out-of-scope function-pointer identity surfaces: parser-only
  `TypeSpec::is_fn_ptr` comparisons, broad sema canonical type equality
  replacement, direct-call function signature metadata, final emitted syntax
  formatting, and backend MIR/prealloc ABI decisions after BIR has already
  lowered the structured call facts.
- Nominal aggregate function-pointer parameter identity remains an adjacent
  concern if frontend lowering later carries aggregate signature refs for
  function-pointer params end to end. This should be treated as follow-up
  scope, not a blocker for the selected indirect-call LIR/BIR bridge.
- Do not expand this runbook into a full parser or type-system replacement.
- `ideas/open/182_type_identity_migration_closure_gate.md` remains blocked
  until this source idea is complete.

## Proof

Passed. Canonical focused proof is now rolled forward in `test_before.log`;
full-suite baseline is recorded in `test_baseline.log`.

Accepted evidence:

- Focused 6-test proof passed:
  `( cmake --build build --target frontend_lir_call_type_ref_test frontend_lir_function_signature_type_ref_test backend_lir_to_bir_notes_test -j && ctest --test-dir build -R '^(frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref|backend_lir_to_bir_notes|backend_codegen_route_x86_64_indirect_aggregate_param_return_pair_observe_semantic_bir|backend_codegen_route_x86_64_indirect_variadic_sum2_observe_semantic_bir|backend_codegen_route_x86_64_indirect_variadic_pair_second_observe_semantic_bir)$' --output-on-failure ) > test_after.log 2>&1`
  The accepted result was rolled forward into canonical `test_before.log`.
- Regression guard passed with matching before/after counts: before=6,
  after=6.
- Hook full-suite baseline accepted at commit `47de3a1a6` with 3137/3137
  passing in `test_baseline.log`.

Focused proof result: build completed and CTest passed
`frontend_lir_function_signature_type_ref`, `frontend_lir_call_type_ref`,
`backend_lir_to_bir_notes`,
`backend_codegen_route_x86_64_indirect_aggregate_param_return_pair_observe_semantic_bir`,
`backend_codegen_route_x86_64_indirect_variadic_sum2_observe_semantic_bir`, and
`backend_codegen_route_x86_64_indirect_variadic_pair_second_observe_semantic_bir`
(6/6).
