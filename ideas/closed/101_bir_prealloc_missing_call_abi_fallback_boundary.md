# BIR Prealloc Missing Call ABI Fallback Boundary

## Goal

Eliminate, constrain, or document the prealloc fallbacks that synthesize missing
BIR call, parameter, result, and return ABI facts from scalar types before call
planning.

## Why This Exists

The call ABI authority audit found concrete duplication when BIR ABI metadata
is absent. `legalize.cpp::infer_call_arg_abi_impl`,
`infer_call_result_abi`, `infer_function_return_abi`, `legalize_module`
fallback sites, type-only register-bank fallbacks in `call_plans.cpp`, and
scalar return fallback in `regalloc/call_moves.cpp` can reconstruct ABI facts
that BIR producers normally own through `CallInst::arg_abi`,
`CallInst::result_abi`, `Function::params[*].abi`, and
`Function::return_abi`.

Some bootstrap repair may be intentional, but the contract is currently
unclear. This idea owns the narrow boundary between required BIR ABI carriers
and any remaining compatibility fallback.

## In Scope

- Trace all prealloc missing-ABI fallback sites for call args, params, call
  results, and function returns.
- Decide which fallbacks are legitimate compatibility/bootstrap repairs and
  which should become assertions or earlier BIR lowering fixes.
- Keep target-sensitive promotion and physical placement in prealloc.
- Add proof that ordinary lowered calls/functions arrive at prealloc with the
  expected ABI carriers.

## Out Of Scope

- Moving physical register, stack slot, clobber, or move planning into BIR.
- Changing target ABI classification rules for correctly populated BIR facts.
- Rewriting runtime helper ABI modeling; that has its own follow-up idea.
- Weakening tests or accepting missing ABI metadata as progress without an
  explicit compatibility rationale.

## Acceptance Criteria

- Each missing-ABI fallback has a documented status: removed, asserted
  unreachable for ordinary calls, or retained as a named compatibility path.
- Ordinary source call args/results and function params/returns have proof that
  BIR supplies the relevant ABI metadata before prealloc planning.
- Remaining fallback paths cannot silently become the primary authority for
  scalar call ABI classification.

## Reviewer Reject Signals

- The patch claims progress by adding another type-only ABI inference path in
  prealloc.
- The route treats target-sensitive legalization as a reason to keep
  undocumented missing-BIR-fact reconstruction.
- Proof covers only one scalar call while return, parameter, or result fallback
  paths remain untested or unexamined.
- Existing failures are hidden by expectation downgrades, unsupported markings,
  or helper renames.
- The implementation broadens into unrelated ABI redesign instead of the
  missing-metadata boundary.

## Close Note

Closed on 2026-06-03.

The audit and implementation completed the missing call ABI fallback boundary.
Silent ordinary type-only ABI authority was removed from shared call argument,
call result, and function return planning paths. Ordinary lowered calls and
functions now depend on explicit BIR carriers for `Param::abi`,
`CallInst::arg_abi`, `CallInst::result_abi`, and `Function::return_abi`.

Retained fallback behavior is constrained to named direct-BIR/bootstrap
compatibility repairs:

- `direct_bir_call_arg_abi_repair`
- `direct_bir_call_result_abi_repair`
- `direct_bir_function_return_abi_repair`
- `direct_bir_call_arg_bank_display`
- `direct_bir_call_result_bank_display`
- `direct_bir_function_return_move_repair`

Target-sensitive physical placement, no-op move decisions, and register/stack
planning remain prealloc authority after the BIR ABI carriers exist. The close
keeps that boundary explicit: BIR publishes semantic ABI metadata, while
prealloc owns target placement and movement decisions.

Proof status: focused carrier tests and final delegated validation passed, and
the close-time backend regression guard passed with `389/390` before and
after, no new failures, and the same existing
`c_testsuite_aarch64_backend_src_00204_c` failure.
