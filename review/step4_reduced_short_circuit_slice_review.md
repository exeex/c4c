# Step 4 Reduced Short-Circuit Slice Review

## Base

- Chosen base commit: `94d9245c` (`[plan] reset step 3 scalar local-slot route`).
- Rationale: this is the latest lifecycle checkpoint that explicitly reset the rejected Step 3 scalar local-slot route and narrowed the remaining dirty work to the Step 4 short-circuit/control-flow authority slice. The current review question is about the dirty worktree after that reset, so this is the right checkpoint.
- Commits since base: `0`.
- Reviewed diff: current worktree diff from `HEAD`, focused on `src/backend/mir/x86/module/module.cpp`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`, `todo.md`, and `test_after.log`.

## Findings

1. Medium: validation evidence is compile/build plus an unrelated red aggregate, not semantic execution proof for the changed short-circuit cases.
   - `test_after.log` shows configure and build passed, and `backend_x86_prepared_handoff_label_authority` passed, but `backend_x86_handoff_boundary` stops at `minimal local-slot return route` before `run_backend_x86_handoff_boundary_short_circuit_tests()` is reached.
   - This is not a route blocker for the reduced slice because the failing scalar boundary is the expected result of retiring the rejected local-slot helpers. It does mean the supervisor should not present the red aggregate as proof that the new short-circuit negative/positive cases executed. If commit policy needs execution proof beyond compile, use a narrower short-circuit-only harness or temporarily reordered local proof outside the committed slice.

2. Low: the retained renderer is still specialized to the local-slot short-circuit/guard shape, but it is now tied to Step 4 prepared control-flow authority rather than standalone scalar sequence recovery.
   - `src/backend/mir/x86/module/module.cpp:3003` gates the path on prepared control-flow, value-location, addressing, a short-circuit join context, authoritative branch-owned parallel-copy bundles, prepared branch-plan labels, prepared branch conditions, and prepared frame-slot accesses.
   - `src/backend/mir/x86/module/module.cpp:3191` still walks the entry-block instructions before the compare load and only accepts immediate stores to the same prepared frame slot. That is shape-limited, but in this reduced slice it is inseparable setup for the prepared short-circuit proof and does not reintroduce the removed no-branch scalar local-slot return, i16 increment, subtract, or add-chain helpers.
   - No blocking exact-sequence scalar helper chain remains in the dirty diff.

## Required Fixes

No blocking fixes found for the reduced dirty slice.

Do not add the retired scalar helper chain back before committing this Step 4 slice. If the supervisor wants stronger proof before commit, the required action is validation routing, not implementation repair: run a short-circuit-only proof that can execute past the retired scalar boundary.

## Judgments

- Plan alignment: `on track`.
- Idea alignment: `matches source idea`.
- Technical debt: `watch`.
- Validation sufficiency: `needs broader proof` for acceptance of the full selected x86 handoff subset; compile/build proof is enough to show the reduced dirty slice is not blocked by the retired scalar boundary.
- Reviewer recommendation: `continue current route`.

## Answer

The reduced dirty slice is a coherent Step 4 semantic prepared-control-flow advance worth committing, provided the commit message and todo proof state the selected aggregate remains red at the unrelated retired scalar boundary. The current diff does not contain the drifting scalar helper chain that caused the prior Step 3 rejection, and the remaining local-slot work is scoped to prepared short-circuit/control-flow authority.
