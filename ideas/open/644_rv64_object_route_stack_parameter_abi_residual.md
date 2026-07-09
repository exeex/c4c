# RV64 Object Route Stack Parameter ABI Residual

Status: Open
Type: Implementation
Parent: `ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md`
Related:
- `ideas/closed/374_rv64_object_route_non_register_param_homes.md`
- `ideas/closed/512_stack_passed_parameter_home_publication.md`
- `ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md`
- `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
Owning Layer: RV64 object-route parameter ABI admission
Queue Order: 44
Proof Surface: `src/20001017-1.c` currently stops at `unsupported_call_abi`
after branch clobber-safety authority is no longer first owner.

## Goal

Classify and repair the RV64 object-route parameter ABI residual exposed by
`src/20001017-1.c` without reopening branch stack clobber-safety authority.

## Why This Exists

Idea 635 moved the representative branch stack-source family past
`missing_stack_clobber_safety`. The remaining first owner for `src/20001017-1.c`
is `unsupported_call_abi`: the RV64 object route still requires all parameters
to arrive in supported GPR or prepared FPR register homes.

## In Scope

- Refresh the `src/20001017-1.c` diagnostic and identify the exact unsupported
  parameter ABI/home shape.
- Compare the current shape against closed stack-parameter and non-register
  parameter-home work.
- Add producer or RV64 consumer support only from explicit prepared ABI/home
  facts.
- Preserve fail-closed diagnostics for unsupported, missing, ambiguous, or
  layout-only parameter homes.

## Out Of Scope

- Branch stack-source freshness or clobber-safety authority.
- RV64 terminator fragment lowering.
- Generic call lowering, runtime fixes, expectations, unsupported markers,
  allowlists, timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe records the first owner and exact ABI/home shape for
  `src/20001017-1.c`.
- The row either advances through a semantic prepared ABI/home repair or is
  reclassified to a more precise existing owner with current evidence.
- Negative proof keeps unsupported parameter ABI shapes fail-closed.

## Reviewer Reject Signals

- Reject source-file-only or named-case fixes for `src/20001017-1.c`.
- Reject reconstructing parameter placement from source syntax, stack offsets,
  or final assembly instead of consuming explicit prepared ABI/home facts.
- Reject broad call ABI rewrites that do not prove this parameter-home
  boundary and nearby fail-closed cases.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject claiming branch clobber-safety progress when the first owner remains
  an ABI/home admission diagnostic.
