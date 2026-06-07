# Current Packet

Status: Active
Source Idea Path: ideas/open/120_aarch64_calls_after_call_result_value_local_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Neighboring After-Call Result Routes

## Just Finished

Step 3 deferred a separate helper-file split for the in-place
`AfterCallMoveLocalOwner`.

Changed files:

- `todo.md`

No code moved. The owner boundary is explicit and target-local, but the current
implementation still depends on nearby private `calls.cpp` surfaces:
`append_call_diagnostic`, `F128CarrierCallOperandOwner`,
`make_register_operand_from_prepared_authority`, scalar register-view helpers,
`register_name_with_expected_view`, call-boundary instruction wrapping, and
frame-slot memory record spelling. Splitting now would require exporting or
relocating those helper surfaces, or duplicating AArch64 register and memory
policy in a new file. That would increase coupling and declaration surface
without narrowing the after-call owner scope.

The deferred split preserves the prepared-input contract from Step 2:
`lower_after_call_move` remains the collection surface for destination home,
call-boundary classification, prepared f128 carrier table, and selected
destination f128 carrier, while `AfterCallMoveLocalOwner::instruction` owns the
AArch64-local translation into register operands, q/f128 rendering,
frame-slot store publication, call-boundary records, diagnostics, and fallback
FPR handling.

## Suggested Next

Execute Step 4: prove neighboring after-call result routes across the focused
backend/AArch64 subset. Confirm scalar integer, scalar FP, f128/vector-carrier,
and frame-slot result storage coverage, and record any remaining coverage gap
with the exact route that still needs a follow-up.

## Watchouts

- Do not reopen destination-home, stack frame-slot, or f128 carrier authority
  from closed ideas.
- Do not move target-specific ABI register spelling, register-view policy,
  q/f128 rendering, or memory operand spelling into shared prepared code.
- Treat expectation downgrades, unsupported-path rewrites, and named-case-only
  proof as route failures.
- Keep the synthetic register-to-stack result publication path in
  `lower_after_call_moves` intact; it is a real after-call result route, not
  dead compatibility code.
- The f128/vector-carrier register-to-register after-call result coverage gap
  is still present. Current proof covers f128 q-register result publication to
  a frame slot, while register-home after-call result coverage remains strongest
  for GPR and scalar FPR/HFA lanes.
- Revisit file splitting only if a later helper extraction creates a smaller
  target-local interface that does not require exporting the private calls
  diagnostic, operand, f128, memory, and call-boundary record helpers together.

## Proof

No build or test run was required for Step 3 because no code, build metadata,
headers, or test expectations changed. The packet made a boundary decision and
updated canonical execution state only; the previous `test_after.log` was left
unchanged.
