# Uniform Target Register Identity Policy

Status: Open
Type: Narrow implementation idea
Parent: `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
Owning Layer: Prepared target register profile and ABI physical identity publication

## Goal

Extend the prepared target register profile into the uniform policy owner for
x86, AArch64, and RV64 ABI register pools, ABI placements, and
`PreparedTargetRegisterIdentity` publication where physical register identity
is required by prepared/prealloc or target consumers.

This idea should make physical ABI register identity a shared prepared fact
instead of something each backend rediscovers from register spellings or
target-local conversion helpers.

## Why This Exists

The target ABI contract research in
`docs/target_abi_contract_research/` concluded that the current pipeline
direction is healthy:

```text
TargetProfile -> BIR semantic ABI facts -> prepared/prealloc placements ->
target backend consumption
```

The same research identified a narrow limitation: `PreparedTargetRegisterIdentity`
is expressive enough to carry target physical identity, but publication is
target-asymmetric. RV64 ABI placements can currently become physical identities
through `target_register_identity_for_abi_register_placement(...)`, while
AArch64 and x86 consumers rely more on prepared register names, placement
conversion, or backend-local physical-register rendering.

The smallest implementation step is to make
`src/backend/prealloc/target_register_profile.*` the coherent shared policy
surface for:

- ABI argument/result register pools
- caller-saved and callee-saved pools
- ABI `PreparedRegisterPlacement`
- `PreparedTargetRegisterIdentity` for ABI placements where the physical
  identity is meaningful

This is deliberately narrower than solving value freshness,
`PriorPreservation`, producer rematerialization, or move-bundle source
authority.

## In Scope

- Audit the current target-register policy helpers in
  `src/backend/prealloc/target_register_profile.*`.
- Extend ABI placement-to-`PreparedTargetRegisterIdentity` publication beyond
  the currently supported RV64 ABI placement path, starting with AArch64 and
  x86 where the target has stable physical argument/result register identities.
- Keep `PreparedRegisterPlacement` as the target-profile-relative allocation
  slot and `PreparedTargetRegisterIdentity` as the concrete physical identity.
- Add focused prealloc/backend-BIR tests proving that ABI argument/result
  placements for RV64, AArch64, and x86 publish the expected stable physical
  identities without backend-local rediscovery.
- Preserve existing target register names and placements for call plans,
  value homes, move bundles, and prepared printers unless a small adaptation is
  required to consume the newly published shared identity.
- Document any target or ABI placement shape that intentionally remains
  identity-less because the physical identity is not stable, not meaningful, or
  not yet supported.

## Out Of Scope

- Changing BIR semantic ABI classification in `CallArgAbiInfo` or
  `CallResultAbiInfo`.
- Changing the target triple or `TargetProfile` ABI selection model.
- Defining value freshness authority, producer-rematerialization precedence,
  `PriorPreservation` semantics, or stale-home invalidation rules.
- Reworking move-bundle source authority or stack-destination fan-in semantics.
- Broad backend call-lowering rewrites for RV64, AArch64, or x86.
- Replacing final target instruction rendering or assembler register spelling.
- Expectation rewrites, unsupported-marker changes, allowlist edits, or
  runtime-comparison changes as proof of progress.

## Acceptance Criteria

- `target_register_profile` is the shared source for ABI argument/result
  physical identity publication across RV64, AArch64, and x86 where supported.
- Focused tests prove that `PreparedRegisterPlacement` for call argument and
  call result ABI pools maps to the expected `PreparedTargetRegisterIdentity`
  for each supported target family.
- Existing RV64 ABI identity behavior remains intact.
- AArch64 and x86 either publish stable ABI placement identities through the
  same shared surface or fail closed / report unsupported identity publication
  with a documented reason.
- No semantic ABI classification, value freshness, preservation fallback,
  producer rematerialization, move-bundle authority, or broad backend emission
  behavior is changed.
- Existing backend/prealloc tests for prepared call contracts and target
  register placement continue to pass.

## Reviewer Reject Signals

- Reject a slice that changes BIR call ABI classification to compensate for
  missing physical identity publication.
- Reject backend-local physical-register rediscovery presented as a shared
  `PreparedTargetRegisterIdentity` repair.
- Reject RV64-only expansion that leaves AArch64 and x86 without either shared
  identity publication or an explicit fail-closed/documented unsupported path.
- Reject broad call-lowering rewrites, move-bundle rewrites, or
  `PriorPreservation` behavior changes under this idea.
- Reject string-only register-name comparisons when a stable
  `PreparedTargetRegisterIdentity` should be produced and asserted.
- Reject expectation rewrites, unsupported-marker changes, allowlist changes,
  or runtime-output changes claimed as target-register policy progress.
