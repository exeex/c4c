# HIR Aggregate-Owner Function-Parameter Crash Repair Runbook

Status: Active
Source Idea: ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md
Switched from: 831 Step 1 ordered-successor decision

## Purpose

Repair only the pre-existing HIR aggregate-owner/function-parameter crash
before 831 activates the separately owned truthiness-authority route.

## Core Rule

Preserve native aggregate ownership and type-specification invariants. Do not
hide the crash with a fallback or widen into truthiness, direct-call, or 830.

## Read First

- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
- `test_hir_to_lir_object_helper_callees_prefer_link_name_ids`
- `typespec_aggregate_owner_key`, `lir_owned_type_spec`, and
  `populate_lir_function_params`

## Non-Goals

- Truthiness-LHS authority, GCC torture work, direct-call identity, 830/829,
  Raw-BIR, generic calls, and broad type-system or signature redesign.

## Ordered Steps

### Step 1 - State the aggregate-owner parameter-lowering invariant

Goal: identify exactly why the named HIR test reaches
`typespec_aggregate_owner_key` without a valid aggregate owner.

Actions:

- trace the selected function-parameter path through `lir_owned_type_spec`
  and `populate_lir_function_params` to the first invalid ownership fact;
- distinguish valid aggregate-owned parameter construction from missing,
  foreign, and type-incoherent owner cases; and
- select the smallest construction or verifier seam that can repair this
  ownership relation without a null/default fallback.

Completion check: a native ownership/type-specification contract and bounded
repair seam are explicit; no truthiness or 830 work is admitted.

### Step 2 - Repair and verify the selected owner relation

Goal: prevent the crash through coherent native ownership enforcement.

Actions:

- implement only the Step 1 selected construction/verifier correction; and
- add nearby positive and malformed/foreign/missing/incoherent coverage.

Completion check: valid aggregate-owned parameters lower without a crash and
invalid ownership fails closed.

### Step 3 - Prove the HIR route and return to 831

Goal: provide accepted focused evidence for the first ordered successor.

Actions:

- run a fresh build and focused `frontend_hir_tests` proof selected by the
  supervisor; and
- record the accepted result, then reactivate 831 solely to activate 833.

Completion check: the named test clears the former crash with accepted focused
proof; no full baseline clearance is claimed.
