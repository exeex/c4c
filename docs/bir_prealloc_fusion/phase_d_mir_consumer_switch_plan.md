# Phase D MIR Consumer Switch Plan

Source idea: `ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md`
Status: Step 1 evidence baseline drafted.

This artifact is the durable Phase D analysis surface for planning MIR/codegen
consumer switches from `Prepared*` wrappers and aggregate lookup caches to BIR
nodes, BIR-owned route annotations, typed route indexes, and narrow validated
facades. Phase D is analysis-only: it records evidence, classifications,
adapter boundaries, and follow-up idea payloads, but does not convert backend
lowering or delete prepared APIs.

## Step 1 Evidence Baseline

Phase D starts from the Phase A-C artifact chain plus the selected-consumer
route closures from ideas 167-174. The central finding is that BIR route
records and indexes exist for the seven semantic relationship families, but
Phase C and the later route closures did not prove full prepared API
contraction. Prepared answers remain public or fallback oracles for residual
MIR/codegen consumers until each consumer group proves BIR/prepared
equivalence.

| Source | Established evidence for Phase D | Consumer switch status to preserve |
| --- | --- | --- |
| `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` | Defines the seven target-neutral route families: producer/source identity, select-chain/direct-global dependency, memory/access identity, block-entry/current-block publication identity, CFG edge/join-source identity, call-use source facts, and comparison/condition provenance. It also rejects BIR ownership of homes, registers, stack layout, ABI placement, target addressing legality, move scheduling, branch policy, helper resources, and final instruction records. | Route-backed facts are semantic candidates only. They are not permission to move target/layout/codegen policy into BIR views. |
| `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md` | Classifies those routes into value, instruction, terminator, block, edge, and function-local annotation/index candidates. It records bridge/oracle use for the prepared query families and rejects a whole-`Prepared*` copy or BIR-owned lowering-plan aggregate. | Prepared queries remain comparison/fallback surfaces during migration. `PreparedFunctionLookups`-style aggregate maps are cache/facade pressure, not durable schema payload. |
| `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md` | Finds no prepared public surface that can be hidden immediately. It records private-cache candidates, public migration oracles, target-policy surfaces, route-specific blocked migrations, proof recommendations, and the return-chain no-route follow-up. | Phase D must map concrete residual consumers before proposing bounded follow-up ideas. Selected migrations do not equal route-wide contraction. |
| `ideas/closed/167_route1_producer_constant_oracle_thinning.md` | Selected AArch64 GP scalar value-publication materialization now reads Route 1-backed same-block producer and integer-constant queries through the MIR helper surface. | No prepared producer, constant, cache, or API surface was contracted. Remaining AArch64 and prealloc consumers still require these surfaces or use them as oracles. |
| `ideas/closed/168_route2_select_chain_direct_global_oracle_thinning.md` | Selected AArch64 scalar value-publication select-chain materialization now reads Route 2-backed select-root identity, root instruction index, scalar eligibility, and direct-global dependency presence. | No select-chain/direct-global/scalar-materialization API or cache surface was contracted. Residual consumers remain in publication/store planning, call planning, AArch64 calls, memory, ALU, FP, producer, edge-copy paths, and printer/test surfaces. |
| `ideas/closed/169_route3_semantic_memory_access_cache_split.md` | Selected AArch64 FP same-block global-load value materialization uses Route 3 semantic same-block global-load identity. | `PreparedMemoryAccess` still owns target addressing and address policy for that consumer. Direct non-FP consumers remain in AArch64 value materialization and globals paths, so same-block global-load helper contraction was not claimed. |
| `ideas/closed/170_route4_block_entry_publication_migration.md` | The selected residual block-entry MIR consumer, `mir::find_bir_block_entry_publication_identity(...)`, moved to Route 4 publication availability records and typed block-entry lookup. | Prepared block-entry helpers remain public for AArch64 dispatch/publication, prepared printer, x86 wrapper, scalar publication planning, and oracle tests. |
| `ideas/closed/171_route5_current_block_join_source_migration.md` | The selected helper/consumer route is proven through `Route5EdgeJoinSourceIndex`, including edge publication and join-source oracle equivalence, missing predecessor, no-source, and memory-source cases. | No additional prepared current-block join-source helper contraction was claimed. Remaining prepared public seams belong to broader aggregate/privacy work or future bounded Route 5 slices. |
| `ideas/closed/172_route6_call_use_semantic_source_migration.md` | The selected AArch64 scalar call-argument source-producer consumer reads indexed Route 6 call-use source records for semantic source-producer facts. | No prepared semantic-source surface was contracted. Prepared lookup remains fallback/oracle coverage and still supports recursive binary operand materialization where the Route 6 index is intentionally absent. |
| `ideas/closed/173_route7_comparison_condition_oracle_thinning.md` | The selected fused-compare operand consumer reads Route 7 provenance first. | Prepared comparison answers remain oracle/fallback surfaces for remaining branch and comparison consumers. Target branch spelling, fused legality, condition-code selection, hazards, emitted-register state, and final record order stay outside BIR. |
| `ideas/closed/174_route_index_facade_contraction.md` | The selected Route 4 block-entry facade boundary has typed fail-closed validation; the redundant public `route4_find_block_entry_publication` direct lookup surface was removed. | The facade is not a generic lowering-plan aggregate, not a broad BIR scan surface, and not a universal replacement for typed route indexes. Future categories need separate typed validators and consumer boundaries. |
| `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md` and closed ideas 176-180 | A separate return-chain line of work exists outside the seven Phase A-C route families. | For this Phase D route, return-chain remains a no-route gap unless the supervisor explicitly activates or imports that separate owner/schema result. It must not be folded into Route 1 producer identity, Route 7 comparison provenance, or generic prepared aggregate thinning. |

## Step 1 Residual Consumer Findings

- `PreparedFunctionLookups` remains the main aggregate/private-cache candidate,
  but Phase C kept it public because AArch64 traversal/context fields and
  multiple lowering files still project call, address, move, value-home,
  publication, select-chain/source-producer, and return-chain lookup groups.
- Routes 1, 2, 4, 5, 6, and 7 still require prepared helper families as public
  migration oracles until residual consumer groups switch to route-backed
  records or narrow facades and prove equivalence.
- Route 3 can migrate semantic memory/source reads, but target addressing,
  frame/layout, relocation/TLS, base-plus-offset legality, and operand-formation
  data remain prepared/prealloc or target-owned.
- Route-index facade coverage is intentionally selected and narrow. It validates
  typed references for specific Route 4 and Route 7 categories and must not be
  treated as a `PreparedFunctionLookups` replacement.
- Return-chain helper data is not covered by Routes 1-7 in this active Phase D
  plan. Any consumer switch or contraction in that area requires a distinct
  owner/schema decision and proof ladder.

## Step 1 Handoff To Step 2

Step 2 should map concrete MIR/codegen consumer families against this evidence
using four classifications:

- route-backed semantic fact ready for a bounded BIR-view migration idea;
- temporary migration oracle that must remain public until equivalence is
  proven;
- durable target/layout/codegen policy surface that stays prepared-owned or
  target-owned;
- no-route gap requiring a separate owner/schema decision before migration.

The Step 2 map should keep prepared answers as oracles, use route-owned typed
records or validated narrow facades where available, and reject any proposed
consumer migration that relies on broad BIR rescans, target-policy leakage, or
selected-testcase-only proof.
