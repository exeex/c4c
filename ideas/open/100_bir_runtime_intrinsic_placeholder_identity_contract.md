# BIR Runtime Intrinsic Placeholder Identity Contract

## Goal

Replace or explicitly document raw placeholder callee string matching for BIR
runtime/intrinsic calls with a structured identity contract that prealloc can
consume without treating display names as semantic authority.

## Why This Exists

The call ABI authority audit found that `calling.cpp::lower_runtime_intrinsic_inst`
creates placeholder calls with `CallInst::intrinsic`, a placeholder `callee`,
and intentionally invalid `callee_link_name_id`. Prealloc consumers including
`variadic.hpp::prepared_variadic_entry_helper_kind_for_callee`,
`variadic_entry_plans.cpp` helper scans, and
`call_plans.cpp::call_argument_allows_local_aggregate_address_publication`
currently use raw callee strings such as LLVM-prefixed names as the live
contract.

That is a narrow contract ambiguity: ordinary direct calls should use
structured BIR link-name identity, while runtime placeholders need an explicit
semantic identity or a deliberately documented string-only exception.

## In Scope

- Audit and adjust the BIR runtime/intrinsic placeholder identity carried by
  `CallInst`.
- Update prealloc runtime, variadic, and aggregate-address-publication
  consumers to use the chosen explicit contract.
- Preserve the intentional invalid `callee_link_name_id` behavior for
  placeholders if a separate intrinsic/helper identity remains the authority.
- Add focused proof for va helper classification and the local aggregate
  address publication policy that currently depends on raw `llvm.` matching.

## Out Of Scope

- Reworking ordinary direct-call link-name resolution.
- Changing helper lowering behavior beyond the identity contract.
- Reopening AArch64 call cleanup or implementing unrelated call-plan rewrites.
- Changing target ABI behavior or weakening existing test expectations.

## Acceptance Criteria

- Runtime/intrinsic placeholder identity has a named authority distinct from
  ordinary direct-call `callee_link_name_id`.
- Prealloc consumers no longer need to infer placeholder semantics from raw
  display strings, or the remaining string path is explicitly documented as a
  compatibility-only exception.
- Proof covers at least one variadic helper classification route and one
  aggregate-address-publication route affected by the old raw-name check.

## Reviewer Reject Signals

- The change makes ordinary direct calls fall back to raw callee strings when a
  valid link-name id exists.
- The route only renames string helpers while preserving raw placeholder string
  matching as the undocumented semantic authority.
- The proof is limited to one named testcase while nearby va helper or
  `llvm.` aggregate-address-publication cases remain unexamined.
- Tests are weakened, marked unsupported, or rewritten to hide the previous
  identity ambiguity.
- The implementation broadens into unrelated call-plan or target ABI cleanup.
