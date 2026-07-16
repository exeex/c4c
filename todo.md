Status: Active
Source Idea Path: ideas/open/864_lir_next_non_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and hand off one next non-body-parameter authority row

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting and publishing exactly one next
producer-side non-body-parameter row:
`LirCallOp.direct_one_double_arg_scalar_floating_call_authority` for a direct
nonvariadic `double(double)` call result.
The row is outside 734's accepted history through Step 7.50: Steps 7.34-7.49
cover body-parameter rows, Steps 7.40/7.41 cover fixed direct-call arguments
0/1, and Step 7.50 covers only direct zero-argument scalar floating call-result
authority.

## Suggested Next

Have plan-owner close or route 864, then reactivate 734 for one bounded
receiver packet for only the selected `double(double)` direct call-result row.

## Watchouts

Do not select fixed direct-call argument 0 or fixed direct-call argument 1
parameter authority. Idea 734 already accepted argument 0 in Step 7.40 and
argument 1 in Step 7.41, and backend receiver coverage already includes
`test_fixed_direct_call_argument0_parameter_authority_receipt_and_rejections`.
The selected row is not a generic call-result sweep: zero-argument scalar
floating call results remain accepted 863/734 Step 7.50 work, and other
one-argument types, variadic calls, indirect calls, aggregate results, and
parameter-use families remain separately scoped and fail closed.
This is now a call-result-only row: selected downstream floating binary LHS
consumer coherence is deliberately not part of the handoff contract.

## Proof

Ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`.
Result: passed, `frontend_lir_call_type_ref` 1/1, with proof log at
`test_after.log`.
