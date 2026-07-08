# Pointer Rhs Branch Stack-Source Policy Publication

Status: Closed
Type: Producer-side follow-up implementation idea
Parent: `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
Related:
- `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
Owning Layer: Shared prepared/prealloc branch stack-load producer policy

## Goal

Supply the missing producer/collector policy for pointer `Rhs` branch
stack-load sources so consumers can receive selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority for the exact
branch use.

## Why This Exists

Idea 593 closed after migrating the RV64 pointer `Lhs` branch stack-source
consumer and explicitly left pointer `Rhs` blocked because the shared
producer/collector recorded it as inventory-only with `policy=none` /
`status=missing_policy`.

Idea 594 activated that handoff and Step 1 confirmed the blocker still exists:
`PreparedBranchStackLoadRole::Rhs` is collected, but policy selection still
returns `policy=none`, `pointer_status=unknown`, and
`status=missing_policy`, so no selected authority is available for RV64 or any
other consumer. This must be repaired in the shared producer path, not by
manufacturing a target-local RV64 fallback.

## In Scope

- Identify the shared prepared/prealloc policy gate that selects
  `LoadFromStackSlot` for branch stack-load roles.
- Extend the semantic producer policy so pointer `Rhs` can publish selected
  `BranchStackLoadSource` / `BranchStackSlot` authority when the same-value,
  exact-branch, and terminator-position conditions are satisfied.
- Preserve inventory rows and diagnostics that distinguish missing policy from
  missing stack home, ambiguous authority, stale authority, wrong value, wrong
  use, future point, and stack-home-only evidence.
- Add or update focused prepared-side proof showing accepted pointer `Rhs`
  publication and fail-closed rejection for invalid authority.
- Leave RV64 consumer migration to idea 594 after selected producer authority
  exists.

## Out Of Scope

- RV64, AArch64, x86, string assembly, or other target-local consumer changes.
- Inferring freshness from stack homes, frame slots, aggregate lanes, clobber
  facts, register facts, operand shape, or named testcase shape.
- Broad branch lowering, ABI classification, BIR, MIR view, or freshness model
  redesign.
- Expectation downgrades, unsupported-marker edits, allowlist edits, or runtime
  behavior changes as proof of producer capability.

## Acceptance Criteria

- Pointer `Rhs` branch stack-load producer policy no longer reports
  `policy=none` / `status=missing_policy` when the semantic same-value,
  same-use, exact-branch, and terminator-position conditions are satisfied.
- The selected authority uses
  `PreparedValueFreshnessUseKind::BranchStackLoadSource` and
  `PreparedValueFreshnessSourceKind::BranchStackSlot`.
- Missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
  stack-home-only cases remain rejected with visible status or diagnostics.
- Focused prepared-side tests or dumps prove the accepted `Rhs` producer route
  and the rejected invalid routes.
- The closure note states that idea 594 may be reactivated for RV64 consumer
  migration, or explains the remaining producer-side blocker.

## Closure Note

Closed after the producer-side pointer `Rhs` policy gap was repaired in the
shared prepared/prealloc branch stack-load producer path.

Valid pointer `Rhs` branch stack-load uses now publish selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` /
`PreparedValueFreshnessSourceKind::BranchStackSlot` authority for the same
prepared source value, exact branch block, exact terminator instruction index,
`PreparedValueFreshnessProofKind::BranchTerminatorOrdering`, and
`PreparedValueFreshnessSourceRank::BranchStackSlot`. Valid collected `Rhs`
rows report `policy=LoadFromStackSlot`, `pointer_status=proven`, selected
source freshness, and `stack_slot_fresh_at_branch`.

Accepted proof covered direct planning and collected prepared-dump publication
for valid pointer `Rhs` authority. Rejected proof covered missing, ambiguous,
stale, wrong-value, wrong-use, future-point, and stack-home-only freshness
authority, plus the existing role-independent structural fail-closed cases for
unsupported homes, home/value mismatch, frame-slot mismatch, stack-object
mismatch, missing clobber safety, and unknown pointer status.

No producer-side blocker remains for
`ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`.
That idea may be reactivated for RV64 pointer `Rhs` consumer migration. The
consumer must require the selected shared producer authority named above and
must not infer freshness from stack homes, frame slots, aggregate lanes,
clobber facts, register facts, operand shape, or testcase shape.

Close-time guard used existing canonical backend logs:
`test_before.log` and `test_after.log` both reported 346/346 passing backend
tests, and the regression guard passed in documented non-decreasing mode for
this lifecycle-only close. The supervisor also reported a hook-managed
full-suite baseline after code commits accepted at 3375/3375.

## Reviewer Reject Signals

- Reject any RV64 or target-local fallback that makes `Rhs` emission succeed
  without selected shared producer authority.
- Reject named-case-only matching for the known pointer `Rhs` testcase instead
  of a semantic producer policy rule.
- Reject treating stack-home, frame-slot, aggregate-lane, clobber, register, or
  operand-shape evidence as freshness by itself.
- Reject helper renames, expectation rewrites, unsupported-marker edits,
  allowlist edits, or classification-only changes claimed as producer repair.
- Reject broad branch lowering, ABI, BIR, MIR, or freshness-model rewrites that
  bypass the narrow pointer `Rhs` producer policy gap.
- Reject closing this idea while `Rhs` still reports `policy=none` /
  `status=missing_policy` for a valid exact branch stack-load use.
