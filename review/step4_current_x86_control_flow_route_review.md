# Step 4 Current X86 Control-Flow Route Review

## Review Base

- Chosen lifecycle base: `5dbb6a07 [plan] Activate x86 prepared module renderer recovery plan`.
- Rationale: this commit activated the current idea 121 runbook and created the active `plan.md`/`todo.md`. The later `08d1e047 [plan+idea] update plan` broadened acceptance wording for x86 BIR/handoff surfaces but did not reset or reviewer-checkpoint the route.
- Commits since base: 25.
- Current dirty scope reviewed: `HEAD -> worktree` changes in `src/backend/mir/x86/module/module.cpp` and `todo.md`.

## Findings

### High: Missing prepared block identity is accepted without tying the skipped prepared block to the authoritative edge it supposedly represents

`src/backend/mir/x86/module/module.cpp:258` adds `prepared_compare_join_owns_superseded_prepared_block`, and `validate_prepared_block_targets` returns early at `src/backend/mir/x86/module/module.cpp:390` when it says yes. The predicate proves that some param-zero materialized compare-join context exists and that the prepared block branches to the prepared join label, but it does not prove that `prepared_block.block_label` is one of the true/false branch targets, one of the edge-transfer predecessor labels, or otherwise the specific superseded label owned by that compare-join.

That weakens the validator from "this prepared control-flow block must match BIR identity" into "any missing branch prepared block targeting this join may be skipped if a nearby compare-join context exists." This is not the Step 4 contract in `plan.md:162`: missing or drifted prepared identities should be surfaced unless directly consumed through prepared label authority.

### High: The new bridge-block exception recovers an unprepared BIR block through raw label topology

`validate_prepared_control_flow_handoff` now accepts a BIR block with no prepared block id when `prepared_compare_join_owns_passthrough_bridge_block` returns true (`src/backend/mir/x86/module/module.cpp:809`). That helper checks an empty branch-only block, but the actual edge from prepared predecessor to candidate and candidate to join is proven through raw BIR label strings at `src/backend/mir/x86/module/module.cpp:712` and `src/backend/mir/x86/module/module.cpp:723`.

This conflicts with `plan.md:173`, `plan.md:181`, and the source idea acceptance criterion at `ideas/open/121_x86_prepared_module_renderer_recovery.md:56`: the x86 proof must consume prepared label ids directly and reject missing or drifted ids. The code is not a named fixture shortcut, but it is still a broad raw-label recovery path for a block explicitly missing prepared identity. `todo.md:11` also records this as "without raw label recovery", which does not match the implementation.

### Medium: Parallel-copy authority fallback no longer requires the bundle to name the authoritative edge predecessor

`find_prepared_compare_join_parallel_copy_bundle_for_edge` accepts a parallel-copy bundle if it has the same successor and a move matching the edge transfer (`src/backend/mir/x86/module/module.cpp:2198`). It intentionally ignores `edge_transfer.predecessor_label`. `require_prepared_compare_join_parallel_copy` then allows that fallback when the direct predecessor/successor lookup fails (`src/backend/mir/x86/module/module.cpp:2229`).

The fallback is guarded by uniqueness and by an out-of-SSA move bundle at the execution predecessor block, so this is less severe than a free raw-label skip. Still, it changes "authoritative prepared parallel-copy bundle for this edge" into "a unique same-successor bundle with matching move values plus a move bundle at this block." That may be a legitimate superseded-label repair, but the current slice does not add nearby same-feature tests proving ambiguous or drifted-edge rejection. Keep this out of acceptance until the predecessor relationship is explicitly proven or covered.

### Medium: Branch-condition validation can silently skip dead-looking metadata records

`validate_prepared_branch_condition` now returns early when a condition block label resolves as a prepared label but has no matching BIR block, no prepared control-flow block, and no authoritative branch-owned join transfer (`src/backend/mir/x86/module/module.cpp:495`). This may be intended as a stale-record skip, but it is another validator weakening in the same Step 4 family. Without tests around stale vs live prepared branch-condition records, it is hard to distinguish semantic cleanup from hiding control-flow drift.

## Judgments

- Plan alignment: `drifting`.
- Idea alignment: `drifting from source idea`.
- Technical debt: `action needed`.
- Validation sufficiency: `needs broader proof`.

The current slice is not primarily a named-testcase dispatcher and it does use real prepared compare-join/value-location records in several places. However, the control-flow validator is accumulating compare-join-specific exceptions that accept missing prepared identity instead of forcing the prepared path to publish or consume the identity directly. That is the route the plan explicitly warns against.

The recorded proof is also not acceptance-ready: `test_after.log` shows the delegated command rebuilt the two x86 targets, kept `backend_x86_prepared_handoff_label_authority` passing, and still failed `backend_x86_handoff_boundary` with `compare-join edge has no authoritative prepared parallel-copy bundle`.

## Recommendation

`rewrite plan/todo before more execution`.

Supervisor should reject/rework the dirty Step 4 slice as currently shaped, or narrow the next packet to replace the validator escape paths with a semantic prepared-identity publication/consumption repair. In particular:

- Do not continue by adding another exception for the next blocker.
- Require the superseded prepared-block path to prove the exact skipped label is owned by the compare-join edge/branch metadata, or remove that skip.
- Replace the unprepared bridge-block raw label topology proof with prepared identity publication, an explicit prepared bridge-block record, or a deliberately documented plan split if unprepared bridge blocks are now a real compiler concept.
- Add same-feature negative coverage for drifted/missing prepared ids and ambiguous parallel-copy bundles before treating this route as progress.
