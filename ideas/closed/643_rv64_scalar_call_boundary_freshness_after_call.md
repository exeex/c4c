# RV64 Scalar Call-Boundary Freshness After Call

Status: Closed
Type: Implementation
Parent: `ideas/closed/634_large_selected_pointer_offset_local_memory_policy.md`
Related:
- `ideas/closed/634_large_selected_pointer_offset_local_memory_policy.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md`
Owning Layer: RV64 scalar value freshness across call boundaries
Queue Order: 43
Prerequisites: prepared call plans, call clobber facts, and scalar value
freshness or preservation authority must be explicit before RV64 consumes a
post-call scalar value from an ABI-clobbered register.
Proof Surface: `src/ipa-sra-2.c` runtime mismatch after the large selected
pointer-offset local-memory access has already moved past object emission.

## Goal

Define and repair the RV64 scalar call-boundary freshness policy exposed when a
value needed after a call is read from a register clobbered by that call rather
than from an explicit preserved, republished, or rematerialized source.

## Why This Exists

Idea 634 moved `src/ipa-sra-2.c` selected pointer offset `3999996` past
`unsupported_local_memory_access`; object evidence shows the row now
materializes the large offset through `t6` and emits `lw ..., 0(t6)`. The
remaining first owner is a runtime segfault: `main` computes the later `foo`
predicate from the post-`calloc` return value in `a0` instead of from a
freshly preserved or republished `argc` value. That is a scalar call-boundary
freshness/clobber problem, not large selected pointer-offset local-memory
policy.

## In Scope

- Reproduce the `src/ipa-sra-2.c` post-call scalar freshness residual with
  object, disassembly, and runtime evidence.
- Identify the prepared value, call boundary, clobbered register, expected
  preserved or republished source, and stale post-call register consumption.
- Add producer or RV64 consumer support only when explicit preservation,
  republication, or rematerialization authority proves the scalar value is
  fresh after the call.
- Preserve fail-closed diagnostics when a scalar value is needed after a call
  but no freshness or preservation authority exists.

## Out Of Scope

- Large selected pointer-offset materialization closed by idea `634`.
- Aggregate call result, sret/byval, global data, string pointer, branch, or
  move-bundle fan-in policy.
- Runtime-library behavior, expectations, unsupported markers, allowlists,
  timeouts, or pass/fail accounting.

## Acceptance Criteria

- A refreshed probe proves the first owner for `src/ipa-sra-2.c` is stale
  scalar value consumption across a call boundary.
- At least one complete-authority scalar call-boundary freshness row moves
  past stale clobbered-register consumption, or the route records the precise
  missing preservation, republication, or rematerialization authority.
- Negative proof rejects post-call reads from ABI-clobbered registers unless a
  fresh scalar source is explicit.

## Closure Notes

Closed after Step 5 final lifecycle review. Step 3 accepted the RV64 scalar
call-boundary freshness repair: stale post-call direct register homes now fail
closed unless explicit prior preservation exists, and RV64 call plans synthesize
complete same-block consumer-move preservation for clobbered direct-register
scalar homes into non-overlapping saved callee registers. Focused positive and
negative tests covered the accepted path.

Step 4 validation accepted the 22-test structural backend subset plus the
focused `src/ipa-sra-2.c` RV64 torture probe. The focused row passed without
expectation, unsupported-marker, allowlist, timeout, runtime comparison, or
pass/fail accounting changes. The broader `ctest -L backend` residual failures
remain outside this idea's closure scope.

## Reviewer Reject Signals

- Reject named-case fixes for `src/ipa-sra-2.c`, `calloc`, `foo`, `argc`, or
  the physical register `a0` without a semantic call-boundary freshness rule.
- Reject treating a pre-call register home as fresh after a call merely because
  final assembly still names the same register.
- Reject weakening call-clobber, preservation, republication, or
  rematerialization checks to make the residual run.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime
  comparison, or accounting changes as capability progress.
- Reject broad ABI or call-lowering rewrites that do not prove the exact
  stale scalar freshness boundary and nearby fail-closed behavior.
