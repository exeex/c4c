# AArch64 Codegen Layout Step 2 Review

Active source idea path: `ideas/open/aarch64-codegen-reference-layout-consolidation.md`

Chosen base commit: `661c4af18` (`[plan] Activate AArch64 codegen layout consolidation plan`)

Why this base is the active-idea checkpoint: this commit created the current active `plan.md` and `todo.md` for `ideas/open/aarch64-codegen-reference-layout-consolidation.md`. Later lifecycle commits under this idea are execution notes, not a source-idea reset or reviewer checkpoint.

Commit count since base: 9 commits through `HEAD`.

Working-tree note: `HEAD` includes the Step 2 implementation commits through `5fd33b557`; the working tree has one extra uncommitted `todo.md` review-reminder line and an unrelated untracked open idea. The review considers the reminder line as route context but does not treat the unrelated untracked idea as part of this route.

## Findings

### Medium: Step 2 has reached the useful limit of repeated one-header extraction packets

The route is still behavior-preserving and aligned with the source idea, but more packets of the same shape are becoming low-yield churn. `dispatch.hpp` shrank from 738 lines / 139 declarations at activation to 365 lines / 68 declarations now, while seven private `dispatch_*.hpp` files were added. That is real catch-all reduction, but the remaining `dispatch.hpp` still contains large catch-all sections for publication common helpers, value materialization, edge copies, publication helpers, and entry formals at `src/backend/mir/aarch64/codegen/dispatch.hpp:38`, `src/backend/mir/aarch64/codegen/dispatch.hpp:99`, `src/backend/mir/aarch64/codegen/dispatch.hpp:186`, `src/backend/mir/aarch64/codegen/dispatch.hpp:285`, and `src/backend/mir/aarch64/codegen/dispatch.hpp:360`.

Continuing with another tiny extraction such as only `dispatch_entry_formals.hpp` would be mechanically valid, but it risks optimizing the header count rather than advancing the source idea's visible-family model. Before more Step 2 work, the next packet should be narrowed to a meaningful boundary decision: either finish a cohesive remaining private surface, or stop Step 2 and move to Step 3 rename/merge/justify work.

### Low: The extracted headers are private and mostly cohesive, but some names still preserve dispatch scaffolding

The extracted declarations are not testcase-shaped, do not weaken tests, and no implementation bodies or expectations changed. The diffs in the `.cpp` files are include-only, which supports behavior preservation. The strongest examples are `dispatch_diagnostics.hpp` and `dispatch_dynamic_stack.hpp`, which are small and clear at `src/backend/mir/aarch64/codegen/dispatch_diagnostics.hpp:13` and `src/backend/mir/aarch64/codegen/dispatch_dynamic_stack.hpp:13`.

The larger headers are acceptable as private interim boundaries, but they still advertise historical dispatch scaffolding rather than durable families. `dispatch_calls.hpp` exposes calls bridge functions at `src/backend/mir/aarch64/codegen/dispatch_calls.hpp:15`; `dispatch_branch_fusion.hpp` exposes comparison-adjacent fusion plus hook plumbing at `src/backend/mir/aarch64/codegen/dispatch_branch_fusion.hpp:18`; `dispatch_store_sources.hpp` exposes a broad memory/store-source surface at `src/backend/mir/aarch64/codegen/dispatch_store_sources.hpp:20`. These fit Step 2 as private extraction, but they should not become the final layout answer.

### Low: Validation is sufficient for the current review question, but broader proof will be needed at a route boundary

Each code-changing Step 2 packet recorded the delegated backend proof in `todo.md`, and the current diff contains no test rewrites or emitted-behavior edits. That is enough for this header-surface review. Once Step 2 is stopped or Step 3 begins renames/merges, the supervisor should run broader route-boundary validation or regression guard before accepting the milestone.

## Judgments

Idea-alignment judgment: `matches source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `drifting`

Technical-debt judgment: `watch`

Validation sufficiency: `needs broader proof`

Reviewer recommendation: `narrow next packet`

## Recommendation

Do not reset the route. The accumulated Step 2 work is a behavior-preserving implementation of the active source idea: it reduces `dispatch.hpp`, creates private declaration surfaces, and avoids testcase-overfit. However, the next execution should be narrowed instead of continuing open-ended one-header extraction.

Recommended next action: rewrite only the `todo.md` packet guidance, not `plan.md` or the source idea, to make the next packet one of:

- final Step 2 closure packet: extract or explicitly justify the remaining entry-formal/publication/value-materialization/edge-copy surfaces as a grouped boundary decision, then stop Step 2; or
- Step 3 transition packet: begin rename/merge/justify work for one historical dispatch shard now that the broadest declaration surfaces have been reduced.

Avoid continuing a long sequence of tiny `dispatch_*.hpp` additions unless each one removes a meaningful remaining dependency from `dispatch.hpp` and directly prepares a Step 3 family rename or merge.
