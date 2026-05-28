Status: Active
Source Idea Path: ideas/open/59_aarch64_dispatch_family_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Materialize Follow-Up Source Ideas

# Current Packet

## Just Finished

Completed `plan.md` Step 4 for idea 59 by materializing all eight Step 3
follow-up drafts as small routeable source ideas under `ideas/open/`.
No implementation or test files were edited, and no compiler capability
progress is claimed.

Created follow-up source ideas:

1. `ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md`
   - Ownership class: mechanical fold-back.
   - Covers thin prepared lookup, BIR query, producer, and publication
     result-value wrappers.
2. `ideas/open/61_aarch64_shared_same_block_materialization_authority.md`
   - Ownership class: shared-authority migration.
   - Covers same-block scalar/source-producer facts for recursive
     materialization.
3. `ideas/open/62_aarch64_shared_edge_dependency_authority.md`
   - Ownership class: shared-authority migration.
   - Covers edge dependency facts and null-publication fallback disposal while
     preserving local edge emission.
4. `ideas/open/63_aarch64_shared_select_chain_dependency_authority.md`
   - Ownership class: shared-authority migration plus target-owner split.
   - Covers direct-global/select-chain dependency facts and precise local select
     materialization ownership.
5. `ideas/open/64_aarch64_join_parallel_copy_prepared_query.md`
   - Ownership class: shared-authority migration.
   - Covers current-block join parallel-copy source relationships.
6. `ideas/open/65_aarch64_target_owner_relocation.md`
   - Ownership class: target-owner split.
   - Covers globals, publication spelling, value materialization, scratch/write
     hazards, and address-materialization retry routing.
7. `ideas/open/66_aarch64_local_dispatch_block_route.md`
   - Ownership class: target-owner split with keep-local preservation.
   - Covers the local prepared-block dispatch route, diagnostics, hook wiring,
     and sequencing.
8. `ideas/open/67_aarch64_local_slot_address_offset_probe.md`
   - Ownership class: evidence-gathering.
   - Covers the `local_slot_address_frame_offset` null path and adjacent
     local-slot/frame-address proof.

No Step 3 draft was deferred. Each new idea includes goal, why it exists,
in-scope work, out-of-scope work, acceptance criteria, and concrete reviewer
reject signals.

## Suggested Next

Proceed to `plan.md` Step 5, the closure readiness review for idea 59. Confirm
the Step 2 classification and the eight materialized follow-up ideas are
durable enough to route without rediscovering dispatch-family ownership.

## Watchouts

- Idea 59 remains active; this packet did not close or deactivate the source
  idea.
- Follow-up ideas 61-64 are intentionally separate shared-authority migrations.
  Combining same-block scalar recursion, edge fallback, select-chain
  direct-global discovery, and join-copy cache rebuilding would blur ownership
  and proof.
- Follow-up idea 65 is target-owner relocation, not a semantic migration.
  Preserve scratch/clobber handling, register spelling, instruction spelling,
  frame-address rendering, diagnostics, and final machine-instruction
  sequencing as local AArch64 behavior.
- Follow-up idea 66 should wait until at least the major helper splits are
  decided; it is a narrowing/preservation cleanup for the block route, not the
  first contraction slice.
- Follow-up idea 67 must stay evidence-only until the null local-slot address
  offset path is proven dead, live, or intentionally disabled.

## Proof

Lifecycle/source-idea packet only. No build or test required by the delegated
proof; no `test_after.log` update made. Verified by materializing the eight
follow-up source idea files and updating this Step 4 result.
