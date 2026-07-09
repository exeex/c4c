# RV64 Object Route Stack Parameter ABI Residual

Status: Closed
Type: Implementation
Parent: `ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md`
Related:
- `ideas/closed/374_rv64_object_route_non_register_param_homes.md`
- `ideas/closed/512_stack_passed_parameter_home_publication.md`
- `ideas/closed/635_prepared_branch_stack_clobber_safety_authority.md`
- `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
Owning Layer: RV64 object-route parameter ABI admission
Queue Order: 44
Proof Surface: `src/20001017-1.c` was reclassified from
`unsupported_call_abi` to a missing explicit incoming stack formal authority
diagnostic after branch clobber-safety authority stopped being first owner.

## Completion Notes

Closed after `cf784a80c` classified the RV64 object-route residual more
precisely. The row no longer relies on RV64 consumer-side reconstruction of an
incoming stack formal offset; that rejected route was removed from
`object_emission.cpp` and `prepared_local_memory_emit.cpp`.

Current evidence shows that prepared formal publication provides
`IncomingStackToHome` plus the callee local stack-slot home, but does not
publish an explicit caller-stack incoming offset/address authority distinct
from the local spill-slot home. `src/20001017-1.c` now fails closed at:

`unsupported_param_home: RV64 object route requires explicit prepared incoming stack formal authority before consuming stack-passed scalar formal homes`

The remaining work is a separate producer/prealloc publication initiative,
recorded as `ideas/open/652_prepared_incoming_stack_formal_authority.md`.

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
