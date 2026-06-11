# 181 Phase D MIR consumer BIR view switch plan

## Goal

Plan how MIR/codegen consumers should switch from reading `Prepared*` wrappers
to reading BIR nodes and BIR-owned annotations.

This is Phase D of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up ideas for consumer migration, not directly convert
AArch64, x86, or riscv lowering.

## Why This Exists

Phase C is complete as an analysis artifact, not as full private-cache
contraction. Its durable handoff found no prepared public surface that could be
deleted or hidden immediately without follow-up consumer migration. Ideas
154-166 established BIR semantic/schema foundations, and ideas 167-174 closed
bounded selected-consumer migrations plus one narrow Route 4 facade contraction,
but they did not complete all MIR/codegen consumer switches.

Phase D should consume that handoff and decide the safe migration ladder from
prepared query surfaces to BIR annotation views. It should turn the remaining
consumer dependency map into separate implementation ideas, each with its own
proof route and oracle boundary.

## Shared Artifact Contract

This phase must read:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- closure notes for the Phase C follow-up implementation ideas:
  - `ideas/closed/167_route1_producer_constant_oracle_thinning.md`
  - `ideas/closed/168_route2_select_chain_direct_global_oracle_thinning.md`
  - `ideas/closed/169_route3_semantic_memory_access_cache_split.md`
  - `ideas/closed/170_route4_block_entry_publication_migration.md`
  - `ideas/closed/171_route5_current_block_join_source_migration.md`
  - `ideas/closed/172_route6_call_use_semantic_source_migration.md`
  - `ideas/closed/173_route7_comparison_condition_oracle_thinning.md`
  - `ideas/closed/174_route_index_facade_contraction.md`

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`

The closure note should summarize that document. Phase E must be able to
consume the artifact directly.

## Phase C Review Context

Phase D must preserve these review findings from the Phase C close audit:

- Route 1 migrated one selected AArch64 GP scalar value-publication
  materialization path, but no producer/constant/cache/API surface was safe to
  contract because remaining AArch64 and prealloc consumers still publicly
  require those surfaces.
- Route 2 migrated one selected AArch64 scalar value-publication select-chain
  materialization path, but select-chain/direct-global/scalar-materialization
  APIs and caches remain needed by publication/store planning, call planning,
  AArch64 calls, memory, ALU, FP, producer, edge-copy paths, and printer/test
  surfaces.
- Route 3 migrated one selected AArch64 FP same-block global-load value
  materialization family, but direct non-FP consumers remain in value
  materialization and globals paths, and target addressing/layout authority
  remains prepared-owned.
- Route 4 migrated the selected residual block-entry MIR query boundary, but
  prepared block-entry helpers remain public for AArch64 dispatch/publication,
  prepared printer, x86 wrapper, scalar publication planning, and oracle tests.
- Route 5 proved the selected helper/consumer route through
  `Route5EdgeJoinSourceIndex`, but no additional prepared current-block
  join-source helper contraction was claimed.
- Route 6 migrated one selected AArch64 scalar call-argument source-producer
  consumer, but no prepared semantic-source surface was contracted because the
  prepared lookup still serves as fallback/oracle coverage and supports
  recursive binary operand materialization where Route 6 records are absent.
- Route 7 migrated one selected fused-compare operand consumer, while prepared
  answers remain oracle/fallback surfaces for remaining branch/comparison
  consumers.
- Route index facade work removed the redundant public
  `route4_find_block_entry_publication` direct lookup surface only for the
  selected Route 4 block-entry facade boundary. It did not create a generic
  lowering-plan aggregate or universal replacement for typed route indexes.
- Return-chain lookup helpers remain a no-route gap and should not be folded
  into Phase D as if Route 1 producer identity or Route 7 comparison provenance
  already owns them. Any return-chain work needs its own owner/schema idea.

## Direction

The final consumer contract should be BIR annotated view -> MIR lowering. The
MIR layer should not need a separate `PreparedBirModule` container when BIR
node identity plus annotations are sufficient.

Consumer migration must proceed one semantic consumer group at a time. Prepared
answers should remain available as oracles until equivalence is proven, and
target/layout/codegen policy must remain outside BIR.

## In Scope

- Mapping MIR/codegen consumer dependencies on `PreparedBirModule`,
  `PreparedFunctionLookups`, and domain `Prepared*` query structs.
- Defining proposed BIR view or adapter APIs for consumers that can safely
  switch to existing BIR route records, typed indexes, or narrow route facades.
- Ordering bounded follow-up ideas for route-backed consumer migrations.
- Identifying consumer groups that still need Phase A/B/C prerequisite work
  before conversion.
- Recommending proof ladders that pair oracle-equivalence tests with the narrow
  target/backend subsets that observe each migrated consumer.

## Out Of Scope

- Directly converting AArch64, x86, or riscv lowering during this analysis
  phase.
- Deleting or hiding prepared APIs before every listed public consumer has a
  BIR-backed replacement or a deliberate target-local owner.
- Moving homes, registers, stack slots, ABI placement, move scheduling, branch
  spelling, fused-compare legality, emitted storage, final instruction order,
  or other target/codegen policy into BIR.
- Treating `RouteIndexReferenceFacade` as a generic replacement for typed route
  indexes or as a BIR-owned `PreparedFunctionLookups` clone.
- Solving return-chain ownership inside this phase without a separate
  return-chain owner/schema idea.

## Questions

- Which MIR/AArch64 consumers currently require `PreparedBirModule`,
  `PreparedFunctionLookups`, or domain `Prepared*` query structs?
- Which consumers can switch first to BIR annotations with minimal blast
  radius?
- Which consumers need adapter APIs during migration?
- Which consumers reveal missing Phase A/B normalization or annotation work?
- What proof ladder keeps backend behavior stable across the switch?
- Which residual Phase C selected-consumer migrations should become new
  bounded implementation ideas, and which are blocked by missing schema or
  target-local ownership decisions?

## Required Analysis

Map consumer families:

- AArch64 traversal and dispatch
- calls
- memory and addressing
- value materialization and publication
- comparison and ALU
- edge copies and control flow
- wide values and runtime helpers
- future x86/riscv planned consumers, at least at interface level

For each family, classify the prepared dependency as one of:

- route-backed semantic fact ready for a bounded BIR-view migration idea;
- temporary migration oracle that must remain public until equivalence is
  proven;
- durable target/layout/codegen policy surface that should stay prepared-owned
  or target-owned;
- no-route gap requiring a separate owner/schema decision before migration.

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`;
- a consumer dependency map from prepared APIs to proposed BIR view APIs;
- an ordered migration ladder;
- adapter boundaries that may be needed temporarily;
- follow-up ideas for each safe consumer migration slice;
- explicit blockers that must be solved by Phase A/B/C before conversion.

The artifact must identify which Phase A/B/C entries unblock each proposed
consumer migration. Follow-up ideas must remain bounded implementation slices;
the Phase D analysis should not directly implement them.

## Completion Criteria

- The durable Phase D artifact exists at
  `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.
- The artifact records the Phase C residual-consumer findings above and does
  not reinterpret ideas 167-174 as full private-cache contraction.
- Each proposed implementation follow-up names the consumer group, BIR route
  prerequisite, prepared oracle/fallback boundary, out-of-scope target policy,
  and proof recommendation.
- Return-chain is either explicitly excluded with a link to a separate
  owner/schema idea or identified as a no-route blocker, not absorbed into an
  unrelated route.
- No implementation source changes are required for this analysis phase.

## Reviewer Reject Signals

- Switching or rewriting MIR/codegen consumers while claiming to perform only
  Phase D analysis.
- Claiming Phase C completed all private-cache contraction despite the close
  reports preserving residual public prepared consumers and oracle surfaces.
- Treating selected-consumer migrations from ideas 167-174 as proof that a
  whole route family is ready for public API deletion.
- Reintroducing local BIR scans, predecessor rescans, name matching, or
  testcase-shaped shortcuts in proposed migration ideas.
- Moving target/layout/codegen policy into BIR view APIs to make prepared
  surfaces look removable.
- Treating AArch64-only convenience as the final cross-arch BIR view contract.
- Folding return-chain lookup contraction into Route 1, Route 7, or generic
  Phase D work without a separate return-chain owner/schema decision.
