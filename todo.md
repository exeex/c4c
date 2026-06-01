Status: Active
Source Idea Path: ideas/open/76_aarch64_publication_ordering_evidence_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Decide Publication-Order Authority

# Current Packet

## Prior Context

Step 1 traced block-entry out-of-SSA edge-copy publication. Prepared/shared
authority owns edge identity, source binding, destination binding, move
binding, and parallel-copy step metadata through
`make_prepared_edge_publication_lookups(...)`,
`prepare_edge_copy_source_facts(...)`, and
`prepare_block_entry_parallel_copy_edge_source_facts(...)`. AArch64 consumes
those facts in `lower_predecessor_select_parallel_copy_sources(...)` /
`lower_predecessor_join_source_publication(...)`; final AArch64 `lines`
serialization remains target-local record spelling.

Step 2 traced call-boundary publication. Prepared/shared authority owns call
identity, argument/result/preservation facts, clobber facts, and semantic
call-boundary effect ordering through `PreparedCallPlan`,
`PreparedMoveBundle`, `PreparedCallBoundaryEffectPlan`, and
`plan_prepared_call_boundary_effects(...)` / prepared
call-boundary-effect lookups. AArch64 consumes those plans in
`lower_before_call_moves(...)`, `lower_call_instruction(...)`, and
`lower_after_call_moves(...)`; hazard-driven materialization and concrete
record sequencing remain target-local.

Step 3 traced same-width `i32` typed stack-source publication from
`LoadLocalInst` / `LoadGlobalInst` into an out-of-SSA register move. Existing
`PreparedEdgePublicationLookups::publications` plus
`PreparedEdgePublication` identity select the unique publication by predecessor
label and `source_load_local` / `source_load_global`.
`prepare_same_width_i32_stack_source_publication(...)` validates and records
stack source, type, destination value, and prepared destination register
placement. AArch64 folds that typed publication into the load record through
`make_load_result_stack_publication_scratch(...)`; there is no separate typed
publication sequence to order beyond the selected edge publication.

## Just Finished

Completed `plan.md` Step 4 by comparing the three recorded traces and choosing
the concrete route: consume existing prepared authority and close the probe as
a no-code evidence conclusion. A new prepared publication-order query is not
required by the evidence from Steps 1 through 3.

Authority decision:
- Edge-copy publication ordering is already represented by existing prepared
  edge-publication/source-fact surfaces: `PreparedEdgePublicationLookups`,
  `PreparedEdgePublication`, `PreparedEdgeCopySourceFacts`,
  `prepare_edge_copy_source_facts(...)`, and
  `prepare_block_entry_parallel_copy_edge_source_facts(...)`. These own the
  semantic edge identity, source, destination, move, and parallel-copy step
  metadata. AArch64 may continue to serialize target-specific publication
  `lines` and record shape after consuming those prepared facts.
- Call-boundary publication ordering is already represented by existing
  prepared call-boundary surfaces: `PreparedCallPlan`, `PreparedMoveBundle`,
  `PreparedCallBoundaryEffectPlan`, prepared call-boundary-effect lookups, and
  `plan_prepared_call_boundary_effects(...)`. These own semantic before-call,
  call, after-call, argument/result, preservation, and clobber effect order.
  AArch64 may continue to perform hazard-driven materialization and concrete
  record sequencing from those ordered effects.
- Typed stack-source publication does not need an independent publication-order
  query for the representative path. `PreparedEdgePublicationLookups` /
  `PreparedEdgePublication` own the publication identity, and
  `prepare_same_width_i32_stack_source_publication(...)` owns the typed stack
  source contract. AArch64 embeds the prepared destination into the load record
  at the memory instruction's normal emission point; no extra semantic
  publication list is locally re-derived.

No proposed query is drafted because the compared paths do not show a missing
shared ordering authority. The owner remains the existing prepared/prealloc
surfaces named above. The consumers remain the AArch64 dispatch edge-copy,
call-boundary, and memory lowering paths. Target-local responsibilities remain
record construction, ABI register conversion, hazard-driven temporary
materialization, concrete instruction/operand spelling, and final AArch64
record emission order after prepared facts have selected the semantic facts.

Reviewer reject conditions that remain enforceable from this evidence:
- Reject adding a new local AArch64 ordering helper that bypasses the prepared
  edge-publication/source-fact or call-boundary-effect plans.
- Reject expectation downgrades, unsupported rewrites, or named-case ordering
  shortcuts as evidence of progress.
- Reject a new broad publication-order query unless a later trace proves a
  path with multiple independent semantic publications whose ordering is not
  already carried by `PreparedEdgePublicationLookups` or
  `PreparedCallBoundaryEffectPlan`.
- Reject bundling dispatch, call, and memory rewrites into this no-code probe.

## Suggested Next

Run `plan.md` Step 6 acceptance review and closure decision for idea 76. Step
5 implementation is not needed unless the reviewer identifies a concrete
ordering gap not covered by the Step 1-4 evidence.

## Watchouts

- This is a no-code conclusion, not an implementation packet.
- The conclusion is intentionally per-domain rather than a universal
  publication-order query: edge-copy order lives on prepared edge publication
  and source facts, call-boundary order lives on prepared call-boundary effect
  plans, and typed stack-source order is folded into selected edge-publication
  identity.
- If future evidence shows multiple typed stack-source publications that must
  be ordered independently of edge-publication identity, that should be a
  separate initiative with a new source idea rather than silent expansion here.

## Proof

No build required by delegated proof; evidence-only `todo.md` update. No
`test_after.log` was produced because the packet explicitly required no build
or test proof.
