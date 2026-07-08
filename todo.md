Status: Active
Source Idea Path: ideas/open/592_typed_aggregate_branch_stack_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Typed And Aggregate Branch Stack-Source Producers

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for typed and aggregate branch stack-source
producer paths.

Producer paths found:
- Branch stack-load inventory is collected by
  `collect_prepared_branch_stack_load_authorities` ->
  `collect_branch_stack_load_authority_for_role` ->
  `make_branch_stack_load_authority_record` ->
  `plan_prepared_branch_stack_load_authority`.
- Role values come from `branch_stack_load_value_for_role` for
  `Condition`, `Lhs`, and `Rhs`.
- Current branch freshness publication is condition-only inside
  `make_branch_stack_load_authority_record`: when the condition has a stack
  home and exact branch block/terminator indices, it creates one
  `PreparedValueFreshnessAuthority` for `BranchStackLoadSource` /
  `BranchStackSlot` / `BranchTerminatorOrdering` /
  `BranchStackSlot`.
- `Lhs` and `Rhs` rows reach the same inventory collector when they are named
  stack-slot homes, but their current collection policy is `policy=none`, no
  source freshness candidates are published, pointer status remains unknown for
  pointer operands, and clobber safety is not granted.
- Structural producer inventory also exists through
  `collect_prepared_frame_slot_source_facts` ->
  `collect_frame_slot_source_fact_for_role` ->
  `make_frame_slot_source_fact_record` ->
  `plan_prepared_frame_slot_source_fact`, plus aggregate-adjacent
  `prepare_aggregate_stack_source_authority`. These expose concrete stack
  homes, frame slots, stack objects, materialization/path/clobber facts, and
  aggregate source/destination lane facts, but they do not publish branch
  freshness today.

Narrowest blocked consumer candidate:
- Pointer `PreparedBranchStackLoadRole::Lhs` is the best first migration
  candidate. Existing coverage already shows a named pointer `Lhs` stack-home
  row with value id, slot id, stack object, exact branch block/index context,
  and dump visibility, while preserving the boundary as `policy=none`,
  `source_freshness_candidates=0`, `source_freshness_status=no_candidate`, and
  `pointer_status=unknown`.

Allowed freshness facts for Step 2:
- Only an explicit `PreparedValueFreshnessAuthority` matching the consumer
  value id/name, `PreparedValueFreshnessUseKind::BranchStackLoadSource`,
  `PreparedValueFreshnessSourceKind::BranchStackSlot`,
  `PreparedValueFreshnessProofKind::BranchTerminatorOrdering`,
  `PreparedValueFreshnessSourceRank::BranchStackSlot`, and a
  `PreparedValueFreshnessSourceReference` whose home pointer and block/index
  equal the exact branch terminator point may publish branch freshness.

Forbidden structural facts:
- Stack homes, frame slots, stack objects, aggregate stack-source authority,
  aggregate lane/destination mapping, materialization events, path coverage,
  same-slot write classification, and call/publication/move/parallel-copy
  clobber-safety are context only. None may create
  `BranchStackLoadSource` freshness by themselves.

## Suggested Next

Start `plan.md` Step 2 by adding the smallest shared-prealloc producer helper
or data path that can publish the allowed branch stack-slot freshness tuple for
the pointer `Lhs` candidate, while keeping structural source facts separate
from freshness authority.

## Watchouts

- Step 2 should not simply flip `Lhs` to `LoadFromStackSlot`; it must first
  publish an explicit branch-point freshness candidate and still require
  selected shared freshness, pointer proof, and clobber safety.
- The current frame-slot source-fact collector treats pointer values and
  select-materialization results as unsupported boundaries. If Step 2 needs
  pointer `Lhs`, it must define a typed producer rule rather than reuse that
  unsupported structural row as authority.
- Aggregate-adjacent authority currently exposes concrete source facts but
  reports missing aggregate copy authority; treat those facts as inputs to a
  future producer rule, not as direct branch freshness.
- Do not accept a branch stack source because a stack home, frame slot,
  aggregate lane, or clobber-safety fact exists.
- Do not use target-local branch emission inference as freshness authority.
- Do not weaken expectations, unsupported markers, allowlists, or runtime
  contracts as proof of capability progress.
- Keep broader branch, select, edge-publication, RV64, AArch64, and x86
  migrations out of the first packet.

## Proof

Audit-only packet. No build/tests run, and no proof log was created.
