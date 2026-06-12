# 224 Phase E2 control-flow branch target helper private pass-context

## Goal

Contract the public prepared helper family around
`find_prepared_control_flow_branch_target_labels(...)` toward private
pass-context use for the selected branch-target identity read, while retaining
prepared ownership for fallback, branch policy, printer/debug output, wrappers,
helper-oracle behavior, and expected strings.

This idea is scoped to exactly one helper family. It is not permission to
delete aggregate control-flow APIs, retire `PreparedControlFlow`, hide
`PreparedFunctionLookups`, or move branch/output policy into BIR.

## Why This Exists

Phase E1 closed the selected control-flow helper contraction and proved that
the BIR function/block graph, block labels, and structured terminator
successors can supply the branch-target label identity for this helper under
agreement with prepared control-flow facts.

Phase E2 classified this helper family as ready to draft one follow-up because
the semantic owner is proven, but public consumers and retained prepared
responsibilities still exist. This follow-up exists to turn that readiness into
one bounded implementation idea before any privatization or deletion claim.

## Proven Semantic Owner

The proven semantic owner is BIR structured control-flow identity:

- BIR function/block graph identity;
- block labels;
- structured terminator successor facts for the selected branch target labels;
- the agreement gate added by the closed Phase E1 control-flow helper slice.

BIR ownership is limited to the selected identity read. Prepared remains the
owner for non-agreement behavior and target-sensitive output.

## Public Consumers Still Present

- Public helper definitions and wrappers in `src/backend/prealloc/control_flow.hpp`,
  including `find_prepared_control_flow_branch_target_labels(...)`.
- Prepared helper-oracle tests for positive, absent, invalid-id, mismatch, and
  fallback behavior in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Prepared control-flow printer/debug rows for branch, join-transfer, and
  parallel-copy output in `src/backend/prealloc/prepared_printer/control_flow.cpp`.
- x86 module lowering and joined-branch wrapper coverage that still consume
  prepared control-flow branch/handoff behavior.
- Target tests and expected strings that protect branch/wrapper output.

## Retained Prepared Fallback, Policy, And Oracle

Retain prepared authority for:

- absent, invalid-id, duplicate/conflict, mismatch, and non-agreement fallback;
- branch spelling and target branch/output policy;
- edge-copy scheduling, block-order emission, join-transfer records, and move
  policy;
- wrapper output and target-specific handoff behavior;
- prepared printer/debug rows;
- helper-oracle names, statuses, and failure-mode strings;
- expected strings and supported-path status.

## In Scope

- Account for every public caller of
  `find_prepared_control_flow_branch_target_labels(...)` and its immediate
  wrapper family.
- Move only the selected agreement-proven BIR structured successor identity
  read behind private pass-context access where all consumers can use the new
  boundary safely.
- Preserve the public prepared fallback/oracle surface until proof shows it no
  longer needs public helper access.
- Add or update focused tests only to prove the selected helper-family
  contraction and retained fallback behavior.

## Out Of Scope

- Aggregate `PreparedControlFlow`, `PreparedFunctionLookups`, or
  `PreparedBirModule` retirement.
- Public prepared API deletion beyond this exact helper-family boundary.
- Branch policy, edge-copy scheduling, block-order emission, move-bundle
  policy, wrapper behavior, route-debug rows, prepared printer/debug rows,
  diagnostics, helper-oracle strings, expected-string changes, E3, E4, Route 8,
  draft 155, or E5 work.
- Facade renames, wrapper moves, construction reshuffles, baseline refreshes,
  unsupported downgrades, timeout masking, or expectation rewrites as proof.

## Proof Required Before Privatization Or Deletion

- Positive agreement proof: BIR structured successor identity supplies the
  same branch-target labels as the retained prepared helper for the selected
  helper family.
- Absent proof: missing BIR successor evidence preserves prepared behavior.
- Invalid proof: invalid BIR block or label references fail closed to prepared
  behavior.
- Duplicate/conflict proof: ambiguous branch-target identity evidence does not
  override prepared behavior.
- Mismatch proof: BIR/prepared disagreement preserves prepared fallback.
- Public-consumer proof: every current public production, wrapper,
  printer/debug, oracle, and expected-string consumer is either migrated to the
  private pass-context boundary or explicitly retained on a public prepared
  surface.
- Policy fallback proof: branch spelling, edge-copy scheduling, block-order
  emission, move policy, wrapper output, printer/debug rows, helper-oracle
  output, and expected strings remain byte-stable.
- Nearby same-feature proof: at least one adjacent control-flow helper case
  remains covered so the implementation is not shaped only around one testcase.

## Reviewer Reject Signals

- Reject deleting or privatizing aggregate control-flow APIs, aggregate
  `PreparedFunctionLookups`, or `PreparedBirModule` from this idea.
- Reject moving branch spelling, edge-copy scheduling, block-order emission,
  move policy, wrapper output, printer/debug rows, diagnostics, helper-oracle
  strings, or expected strings into BIR authority.
- Reject testcase-shaped handling of one branch-target case without absent,
  invalid, duplicate/conflict, mismatch, public-consumer, and policy fallback
  proof.
- Reject unsupported downgrades, baseline refreshes, helper renames, facade
  moves, timeout masking, or expected-string rewrites as progress.
- Reject retaining the old public prepared failure mode behind a new
  private-pass-context name without actually removing the duplicate public
  semantic identity read for this helper family.

## Closure Note

Closed after the active runbook moved the selected agreement-proven BIR
structured successor label read behind
`prepare::detail::BranchTargetIdentityPassContext` /
`prepare::detail::read_agreeing_bir_branch_target_labels(...)` and migrated the
safe AArch64 selected-identity consumer. The public prepared helper retained
prepared-first lookup and prepared fallback for absent context, invalid IDs,
mismatch/conflict, non-conditional BIR, and non-agreement paths.

Focused helper tests now cover positive private agreement, absent context,
invalid IDs, conflict/mismatch, raw-label drift with agreeing IDs,
non-conditional rejection, and same-feature public fallback behavior. Guardrails
held: no aggregate prepared API retirement, no printer/debug or wrapper
behavior change, no helper-oracle weakening, no unsupported downgrade, no
expected-string rewrite, and no E3/E4/Route 8/draft 155/E5 work entered the
slice.

Close proof used the backend regression scope:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Canonical regression guard passed with `test_before.log` and `test_after.log`:
180/180 backend tests passed before and after, with no new failures.
