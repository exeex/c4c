# Row 322 Later-Lane Source Owner Review

## Review Scope

- Active source idea: `ideas/open/665_aarch64_instruction_dispatch_internal.md`
- Review base: `85c52de3e` (`[plan] Activate AArch64 instruction dispatch plan`)
- Base rationale: this is the commit that activated the current AArch64 instruction dispatch plan and created the matching `todo.md`; later lifecycle commits are packet shaping/evidence updates for the same active idea, not a new active-idea checkpoint.
- Commits since base: 7
- Focus: uncommitted changes in `src/backend/prealloc/call_plans.cpp` and `todo.md`, especially `find_later_stack_aggregate_carrier_lane_run_source_identity`.

## Findings

### High: later-call lookahead is temporal route drift, not a defensible current-call publication rule

The uncommitted helper `find_later_stack_aggregate_carrier_lane_run_source_identity` derives the current call argument source from instructions after the current call. It scans forward from `call_instruction_index + 1` to the next later `CallInst` (`src/backend/prealloc/call_plans.cpp:1695`), walks that later call's aggregate lane groups (`src/backend/prealloc/call_plans.cpp:1703`), finds a `StoreLocalInst` between the current call and that later call for the later call's terminal lane value (`src/backend/prealloc/call_plans.cpp:1715`), and returns that store-source publication or named value as the current argument identity (`src/backend/prealloc/call_plans.cpp:1723`, `src/backend/prealloc/call_plans.cpp:1731`).

That is not a general prepared publication rule for the current AArch64 call. A current call argument's `source_value_id` should be anchored in the current argument, current call ABI/routing metadata, or facts already proven for that current call boundary. A later call/store can be evidence for debugging the missing owner, but using it to satisfy the current row-322 snippet imports future block behavior into the current call plan. This is especially risky because call-plan publication is supposed to describe the source identity of each prepared argument, not infer it from a later use that happens to expose the desired owner.

### High: the matching gates are testcase-shaped and do not prove semantic identity

The new helper does not prove that the later aggregate lane group is the same aggregate source as the current argument. The current argument is only gated by nonzero stack destination, nonzero size, valid aggregate-carrier routing, and lane index 0 (`src/backend/prealloc/call_plans.cpp:1684`). After that, the helper uses a hard-coded `target_lane_ordinal = 0` (`src/backend/prealloc/call_plans.cpp:1693`) and selects the first matching aggregate lane group in the next later call (`src/backend/prealloc/call_plans.cpp:1713`). It does not compare the later call's aggregate source name/lane count to the current routing, does not compare current/later destination offsets, and does not establish dominance or ABI equivalence between the current argument and the later lane group.

This is a narrow shape match for the observed row-322 progression: the current `arg index=12` wants the terminal-lane producer that appears before a later aggregate-lane call. The code makes that case green by recognizing a nearby later-call shape, not by repairing a missing general AArch64 prepared publication fact.

### High: the later-derived identity overrides established current-call identities

`populate_call_plans` now computes `later_stack_lane_run_source_identity` for every argument and gives it priority over `aggregate_source_identity`, `stack_lane_run_source_identity`, and the original source plan for `source_value_id`, `source_register_bank`, and `source_identity_value_name` (`src/backend/prealloc/call_plans.cpp:4227`, `src/backend/prealloc/call_plans.cpp:4251`, `src/backend/prealloc/call_plans.cpp:4266`, `src/backend/prealloc/call_plans.cpp:4277`). This makes a future-use heuristic authoritative for the current row's source identity.

That priority is not fail-closed in the way the active idea asks for. If a later call in the same block happens to expose a compatible-looking aggregate lane group, the current call source can be rewritten even when the current call's own routing already has an aggregate source identity.

### Medium: `todo.md` records the patch as accepted despite route ambiguity

The uncommitted `todo.md` says "Step 3 completed" and describes the rule as selecting the terminal lane producer from the next later aggregate lane group. That accurately describes the implementation, but it records the route as accepted before resolving whether future-call evidence is a valid publication authority. Because this review finds the route overfit, that completion text should not be accepted as canonical progress.

## Judgments

- Idea-alignment judgment: `drifting from source idea`
- Runbook-transcription judgment: `plan matches idea`
- Route-alignment judgment: `route reset needed`
- Technical-debt judgment: `action needed`
- Validation sufficiency: `needs broader proof`
- Reviewer recommendation: `rewrite plan/todo before more execution`

## Recommendation

Supervisor should reject this uncommitted slice as testcase-overfit/temporal route drift. The focused proof passing is not sufficient because the implementation uses a later call/store to satisfy the current row-322 snippet without proving a current-call AArch64 publication rule.

The next packet should reset to the actual missing authority for `arg index=12`: either publish/consume a current-call aggregate carrier source fact that directly names the desired owner, or fail closed with evidence that row 322 belongs outside the current AArch64 publication route. Do not accept the later-call lookahead as the prepared source-owner rule.
