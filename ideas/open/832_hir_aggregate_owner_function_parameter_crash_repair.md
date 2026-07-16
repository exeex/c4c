# HIR Aggregate-Owner Function-Parameter Crash Repair

Status: Open
Type: bounded HIR-to-LIR aggregate owner/function-parameter ownership repair
Blocked Parent: `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`, Step 2
Ordered Before: `ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md`

## Goal

Diagnose and repair the first-owner crash at `typespec_aggregate_owner_key`
while lowering aggregate-owned function parameters, so
`test_hir_to_lir_object_helper_callees_prefer_link_name_ids` completes without
weakening ownership or type-specification contracts.

## Why This Exists

The exact baseline subset identifies a pre-existing `frontend_hir_tests`
segfault in `typespec_aggregate_owner_key`, reached through
`lir_owned_type_spec` and `populate_lir_function_params`. A clean isolated
`f0fc85e4f^` backend-enabled build reproduces it, so it is not 830's
direct-call argument-1 change. The 13 truthiness failures have a distinct LIR
verifier owner and are intentionally sequenced after this HIR route.

## In Scope

- Trace the aggregate owner key and function-parameter lowering ownership
  invariant in the named HIR regression.
- Make the smallest native ownership/type-specification correction that
  prevents the crash while preserving existing valid owner relationships.
- Add nearby positive and malformed/foreign/missing ownership coverage and
  prove the selected HIR test proceeds without the crash.
- Return accepted focused evidence to 831; do not clear the overall baseline.

## Out Of Scope

- `LirCmpOp.truthiness_lhs_parameter_authority` or any GCC torture repair.
- Direct-call argument identity, 830/829 authority, Raw-BIR, generic calls,
  function-signature redesign, or broad aggregate/type-system rewrites.
- Null/default owner fallbacks, exception swallowing, test filtering,
  expectation weakening, unsupported markers, or harness changes.

## Acceptance Criteria

1. The crash's aggregate-owner/function-parameter invariant is explicit and
   repaired at its native first-owner seam.
2. Nearby coverage accepts the intended owner relationship and rejects
   missing, foreign, or incoherent ownership without a crash.
3. A fresh build and focused `frontend_hir_tests` proof establish that the
   named regression completes beyond the former crash.
4. The completion record returns only to 831 for activation of 833; it does
   not claim full baseline clearance.

## Reviewer Reject Signals

- Reject any truthiness-authority, direct-call, 830/829, Raw-BIR, or generic
  call change presented as this HIR ownership repair.
- Reject a null/default aggregate-owner fallback, exception swallowing, or
  weakened ownership/type-specification verification that merely hides the
  crash.
- Reject named-test-only conditionals, expectation downgrades, filters,
  unsupported markers, or weaker test/harness contracts as progress.
- Reject broad aggregate or function-signature redesign when the evidenced
  `typespec_aggregate_owner_key` / parameter-lowering seam can be corrected
  locally.
