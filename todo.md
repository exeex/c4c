Status: Active
Source Idea Path: ideas/open/59_aarch64_dispatch_family_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Split Coherent Follow-Up Work

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for idea 59 by grouping the Step 2 classification
table into bounded follow-up idea drafts. This is audit-only grouping; no
follow-up idea files were created and no implementation capability progress is
claimed.

Proposed numbered follow-up drafts:

1. Mechanical prepared/BIR lookup wrapper fold-back.
   Owned files/helper groups: `dispatch_lookup.hpp`,
   `dispatch_lookup.cpp`, `dispatch_producers.hpp`,
   `dispatch_producers.cpp`, thin prepared lookup wrappers
   (`prepared_named_value_id`, `prepared_value_id`, `find_value_home`),
   producer wrappers over `mir::query`
   (`find_same_block_binary_producer`, `find_same_block_named_producer`,
   `evaluate_same_block_integer_constant`), and publication BIR result-value
   helpers (`instruction_result_value`, `instruction_result_value_ref`).
   Ownership class: `fold-back`.
   Expected validation scope: build plus focused AArch64 backend tests covering
   comparison/FP materialization, dispatch hooks, publication result extraction,
   and any existing tests that exercise prepared value-home lookups. Broader
   regression guard is appropriate if wrapper removal changes public headers.
   Concrete reject signals: new semantic lookup policy appears in AArch64 code;
   call sites start reimplementing raw BIR scans; wrapper deletion requires
   expectation downgrades; or folded helpers become target-specific instead of
   shared/inlined convenience.

2. Shared scalar/source-producer authority for same-block materialization.
   Owned files/helper groups: `dispatch_lookup.cpp` same-block scalar and
   load-local availability helpers, `dispatch_value_materialization.cpp`
   recursive same-block/source discovery, relevant `dispatch_producers.*`
   wrapper consumers, and the shared prepared/query owner that replaces raw
   same-block scans.
   Ownership class: `move-forward`, with local AArch64 recursive emission kept
   only after semantic source facts come from shared/prepared authority.
   Expected validation scope: build plus focused backend tests for scalar
   materialization, load-local reuse, integer constants, ALU/comparison/FP
   recursive value publication, and nearby negative cases where missing
   prepared facts should fail closed or report a durable diagnostic.
   Concrete reject signals: a new named-case or same-block scan is added under
   AArch64 dispatch; recursive materialization still decides semantic producer
   identity locally; coverage proves only one narrow testcase while adjacent
   scalar/load-local paths remain unaudited; or expectations are weakened to
   hide missing shared facts.

3. Shared edge dependency authority and null-publication fallback disposal.
   Owned files/helper groups: `dispatch_edge_copies.cpp` edge local-fallback
   producer group (`find_edge_named_producer` and null-publication callers),
   prepared edge-publication consumers, and whichever shared edge dependency or
   prepared publication query owns successor/edge/predecessor source facts.
   Ownership class: `move-forward` for fallback producer discovery, while edge
   prepared-consumer emission and edge scratch/hazard ordering remain
   `keep-local`.
   Expected validation scope: build plus focused edge-copy/publication tests for
   prepared edge sources, missing publication facts, va-list carrier emission,
   predecessor/successor edge inputs, and clobber-sensitive copy ordering.
   Concrete reject signals: predecessor-depth scans survive as durable
   AArch64 authority; null-publication fallback deletion lacks caller proof;
   clobber/scratch behavior regresses while moving semantic authority; or the
   slice downgrades unsupported/error expectations instead of proving prepared
   edge facts.

4. Shared select-chain and direct-global dependency authority.
   Owned files/helper groups: `dispatch_edge_copies.cpp`
   `emit_select_chain_value_to_register`,
   `materialize_direct_global_select_chain_call_argument`,
   `dispatch_producers.cpp` `prepared_select_chain_contains_direct_global_load`
   and prepared select bridge fallback behavior, `calls.cpp` call-argument
   consumers, ALU/value-materialization select-chain callers, and the shared
   prepared select-chain or call-plan owner.
   Ownership class: `move-forward` for direct-global/select-chain dependency
   discovery and `split-owner` for relocating target select materialization to a
   precise local owner.
   Expected validation scope: build plus focused tests for non-edge select
   chains, direct-global select-chain call arguments, ALU select materialization,
   value publication through select chains, and equivalence with shared
   prealloc/publication-plan dependency coverage.
   Concrete reject signals: duplicated direct-global traversal remains in
   AArch64 dispatch; `PreparedCallPlan` or shared select-chain facts do not
   cover existing call-argument cases; non-edge callers are not proven; or the
   result works only by adding select-chain-shaped special cases.

5. Join parallel-copy prepared query migration.
   Owned files/helper groups: `dispatch_producers.cpp`
   `build_current_block_join_parallel_copy_cache`, dispatch hook consumers of
   current-block join copies, prepared move-bundle/value-home inputs, and the
   shared prepared join-copy query owner.
   Ownership class: `move-forward`.
   Expected validation scope: build plus focused join/parallel-copy tests for
   current-block joins, incoming expression/source relationships, prepared homes,
   and nearby branch-fusion or before-return publication consumers.
   Concrete reject signals: local AArch64 code continues reconstructing join
   source relationships from raw prepared homes and move bundles; shared query
   semantics diverge from existing prepared move authority; proof covers only a
   single dispatch caller; or follow-up changes branch-fusion sequencing while
   claiming only query migration.

6. Target-owner relocation for globals, publication spelling, and value
   emission.
   Owned files/helper groups: `dispatch_publication.*` target-spelling and
   prepared-consumer groups, `dispatch_value_materialization.*` prepared
   global/load-local/local-slot consumers and scratch/write hazards,
   `dispatch_producers.*` load-global target/symbol helpers, `dispatch.cpp`
   address-materialization retry routing, and likely precise local owners such
   as `globals.cpp`, `memory.cpp`, `alu.cpp`, publication/address helpers, or a
   non-dispatch value-materialization owner.
   Ownership class: mostly `split-owner` plus `keep-local`; this draft moves
   AArch64 target emission out of broad dispatch-family files without moving
   semantic authority to shared code.
   Expected validation scope: build plus focused AArch64 backend tests for
   global load spelling, GOT/direct symbol labels, frame-slot address spelling,
   fixed-formal stores, scalar load/store mnemonics, local-slot/global value
   materialization, scratch-register hazards, and address-materialization retry
   paths. Escalate if public helper headers are deleted or target emission is
   redistributed across multiple compilation units.
   Concrete reject signals: route turns into a bulk merge into `dispatch.cpp`;
   target-local clobber/register-spelling behavior moves to shared code; helper
   relocation changes semantic producer discovery; or implementation claims
   contraction while leaving public dispatch-family surfaces unchanged.

7. Preserve and narrow the local dispatch block route.
   Owned files/helper groups: `dispatch.hpp`, `dispatch.cpp` public block entry
   points (`make_block_lowering_context`, `dispatch_prepared_block`), routing
   and diagnostics group (`classify_instruction`, unsupported diagnostics, BIR
   block/context lookup, `make_bir_machine_instruction`), before-return
   publication sequencing, branch-fusion hook wiring, and final
   machine-instruction sequencing.
   Ownership class: `keep-local` with small `split-owner` cleanups only after
   producer/publication/value-materialization authority has left broad
   dispatch files.
   Expected validation scope: build plus focused block traversal/routing tests,
   unsupported instruction/terminator diagnostics, before-return publication,
   branch-fusion hooks, and representative prepared-block lowering. This can be
   a final contraction cleanup after drafts 1-6 land.
   Concrete reject signals: block dispatch starts owning semantic producer or
   publication decisions; diagnostics are weakened to pass moved helpers;
   sequencing changes before dependent owner splits are complete; or the slice
   attempts to solve all dispatch-family contraction at once.

8. Evidence-only probe for local-slot address offset null path.
   Owned files/helper groups: `dispatch_publication.cpp`
   `local_slot_address_frame_offset`, adjacent
   `local_aggregate_address_frame_offset`/frame-address materialization
   consumers, local-slot address publication paths, and existing tests or new
   narrow probes that can hit or prove dead the null implementation.
   Ownership class: `needs-more-evidence`.
   Expected validation scope: no contraction in the first packet; run a narrow
   caller/runtime proof for local-slot address cases that would reach the null
   path, then decide whether a later implementation idea should delete dead
   code, implement missing prepared offset coverage, or keep the disabled path
   documented.
   Concrete reject signals: this is bundled with unrelated contraction; the null
   path is removed without caller/runtime proof; a testcase is marked
   unsupported to avoid the path; or the draft mutates shared frame-address
   authority before proving the exact gap.

## Suggested Next

Proceed to `plan.md` Step 4 by materializing selected follow-up drafts into
`ideas/open/` files. Recommended first materialization order is draft 1
(`fold-back`) or draft 8 (`needs-more-evidence`) if the supervisor wants the
lowest implementation risk, then one shared-authority draft at a time.

## Watchouts

- Step 4 should create idea files only for selected drafts; this packet did not
  materialize new source ideas.
- Drafts 2-5 are intentionally separate shared-authority migrations. Combining
  same-block scalar recursion, edge fallback, select-chain direct-global
  discovery, and join-copy cache rebuilding would blur ownership and proof.
- Draft 6 is target-owner relocation, not a semantic migration. Preserve
  scratch/clobber handling, register spelling, instruction spelling,
  frame-address rendering, diagnostics, and final machine-instruction
  sequencing as local AArch64 behavior.
- Draft 7 should wait until at least the major helper splits are decided; it is
  a narrowing/preservation cleanup for the block route, not the first
  contraction slice.
- Draft 8 must stay evidence-only until the null local-slot address offset path
  is proven dead, live, or intentionally disabled.

## Proof

Audit-only packet. No build or test required by the delegated proof; no
`test_after.log` update made. Evidence used was the Step 2 classification table
already recorded in this file.
