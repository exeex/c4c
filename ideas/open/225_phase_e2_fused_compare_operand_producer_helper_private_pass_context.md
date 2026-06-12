# 225 Phase E2 fused compare operand producer helper private pass-context

## Goal

Contract the public prepared helper family around
`find_prepared_fused_compare_operand_producer_facts(...)` toward private
pass-context use for the selected AArch64 comparison-provenance identity read,
while retaining prepared/AArch64 ownership for fallback, target branch policy,
printer/debug output, wrappers, helper-oracle behavior, and expected strings.

This idea is scoped to exactly one helper family. It is not permission to
delete route lookup groups, retire `PreparedFunctionLookups`, open broad Route
1 through Route 7 migration, or move AArch64 branch policy into route
authority.

## Why This Exists

Phase E1 closed the selected route identity helper contraction and proved that
Route 7 comparison provenance can agree with prepared fused-compare operand
producer facts for the AArch64 conditional-branch lowering boundary.

Phase E2 classified this helper family as ready to draft one follow-up because
the route semantic owner is proven, but public consumers and retained
prepared/target responsibilities still exist. This follow-up exists to turn
that readiness into one bounded implementation idea before any privatization or
deletion claim.

## Proven Semantic Owner

The proven semantic owner is Route 7 comparison provenance under
route/prepared agreement:

- Route 7 producer facts for the selected fused-compare operands;
- the prepared agreement gate used by AArch64 comparison lowering;
- the selected consumer boundary in
  `lower_prepared_conditional_branch_terminator(...)`.

Route ownership is limited to the selected comparison-provenance identity read.
Prepared and AArch64 target lowering retain authority for non-agreement
behavior and emitted branch output.

## Public Consumers Still Present

- AArch64 comparison lowering in
  `src/backend/mir/aarch64/codegen/comparison.cpp`, including reads of
  prepared fused-compare branch facts and comparison between Route 7 and
  prepared operand producer facts.
- AArch64 branch lowering policy in the same file for branch targets, suffixes,
  fused legality, hazards, emitted-register state, and final assembler order.
- Prepared helper-oracle tests for fused, materialized-condition, absent-route,
  invalid-reference, duplicate, mismatch, and unfused fallback behavior in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- AArch64 branch-control and machine-printer tests that protect final output,
  debug rows, and expected strings.
- Any retained route/prepared aggregate or lookup public surface needed to
  preserve fallback and oracle behavior while the helper-family boundary is
  contracted.

## Retained Prepared Fallback, Policy, And Oracle

Retain prepared/AArch64 authority for:

- absent-route, invalid-reference, duplicate/conflict, mismatch, unfused, and
  non-agreement fallback;
- branch targets, suffix mapping, fused legality, hazard handling,
  emitted-register state, final instruction order, and final assembler rows;
- prepared printer/debug and machine-printer output;
- helper-oracle names, statuses, and failure-mode strings;
- wrapper behavior, expected strings, and supported-path status;
- aggregate route/lookup public surfaces not directly owned by this one helper
  family.

## In Scope

- Account for every public caller of
  `find_prepared_fused_compare_operand_producer_facts(...)` and its immediate
  helper family.
- Move only the selected agreement-proven Route 7 comparison-provenance
  identity read behind private pass-context access for the AArch64
  comparison-lowering boundary.
- Preserve prepared/AArch64 fallback and target branch policy for every
  non-agreement path.
- Add or update focused tests only to prove the selected helper-family
  contraction and retained fallback behavior.

## Out Of Scope

- Broad Route 1 through Route 7 migration, generic route-index facades,
  aggregate route view replacement, `PreparedFunctionLookups` retirement, or
  `PreparedBirModule` retirement.
- Branch target selection, suffix mapping, fused legality, hazard handling,
  emitted-register state, final instruction order, final assembler rows,
  wrappers, printer/debug rows, helper-oracle strings, expected-string changes,
  E3, E4, Route 8, draft 155, or E5 work.
- Facade renames, wrapper moves, construction reshuffles, baseline refreshes,
  unsupported downgrades, timeout masking, or expectation rewrites as proof.

## Proof Required Before Privatization Or Deletion

- Positive agreement proof: Route 7 comparison provenance supplies the same
  selected fused-compare operand producer facts as the retained prepared
  helper for the AArch64 conditional-branch consumer.
- Absent-route proof: missing Route 7 provenance preserves prepared behavior.
- Invalid-reference proof: wrong or unusable route references fail closed to
  prepared behavior.
- Duplicate/conflict proof: duplicate, conflicting, or ambiguous route facts do
  not override prepared behavior.
- Mismatch proof: route/prepared disagreement preserves prepared fallback.
- Public-consumer proof: every current public production, wrapper,
  printer/debug, oracle, and expected-string consumer is either migrated to the
  private pass-context boundary or explicitly retained on a public prepared
  surface.
- Target-policy fallback proof: branch targets, suffix mapping, fused legality,
  hazards, emitted-register state, final instruction order, final assembler
  rows, printer/debug output, helper-oracle output, wrappers, and expected
  strings remain byte-stable.
- Nearby same-feature proof: at least one adjacent comparison-provenance or
  materialized-condition case remains covered so the implementation is not
  shaped only around one testcase.

## Reviewer Reject Signals

- Reject broad Route 1 through Route 7 migration, generic route facade work,
  aggregate route lookup replacement, or prepared aggregate retirement.
- Reject moving branch targets, suffix mapping, fused legality, hazards,
  emitted-register state, final assembler order, printer/debug rows, wrappers,
  helper-oracle strings, or expected strings into route authority.
- Reject testcase-shaped handling of one fused-compare case without
  absent-route, invalid-reference, duplicate/conflict, mismatch,
  public-consumer, and target-policy fallback proof.
- Reject unsupported downgrades, baseline refreshes, helper renames, facade
  moves, timeout masking, or expected-string rewrites as progress.
- Reject retaining the old public prepared failure mode behind a new
  private-pass-context name without actually removing the duplicate public
  semantic identity read for this helper family.
