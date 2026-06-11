# Phase D MIR Consumer Switch Plan

Source idea: `ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md`
Status: Step 2 consumer-family map drafted.

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

## Step 2 Prepared Consumer Family Dependency Map

Step 2 searched the live MIR/codegen and prealloc surfaces for
`PreparedBirModule`, `PreparedFunctionLookups`, and domain `Prepared*` query
structs. The map below classifies each concrete consumer family with the
four-category working model from `plan.md`:

- route-backed semantic fact ready for a bounded BIR-view migration idea;
- temporary migration oracle that must remain public until equivalence is
  proven;
- durable target/layout/codegen policy surface that stays prepared-owned or
  target-owned;
- no-route gap requiring a separate owner/schema decision before migration.

| Consumer family | Concrete prepared APIs/helpers still read | Classification | Proposed BIR view/adaptor or blocker |
| --- | --- | --- | --- |
| AArch64 traversal and dispatch context | `PreparedBirModule`, `make_prepared_function_lookups(...)`, `make_prepared_call_plan_lookups(...)`, `make_prepared_address_materialization_lookups(...)`, `make_prepared_value_home_lookups(...)`, `make_prepared_move_bundle_lookups(...)`, plus `FunctionLoweringContext::{prepared,prepared_lookups,call_plan_lookups,address_materialization_lookups,move_bundle_lookups,value_home_lookups}` in `src/backend/mir/aarch64/codegen/traversal.cpp` and `module/module.hpp`. | Temporary migration oracle plus durable target/layout/codegen policy surface. The aggregate bundle is cache wiring, while the projected plans include ABI, frame, storage, register, and move policy. | Keep the aggregate as a pass-local oracle during Phase D. Later migrations should pass narrow route views per semantic group and leave `PreparedBirModule`/target plans as the owner for frame, storage, register allocation, dynamic stack, ABI, and final machine-record policy. Do not replace this with a BIR-owned `PreparedFunctionLookups` clone. |
| AArch64 calls and call-boundary lowering | `PreparedCallPlanLookups`, `find_indexed_prepared_call_plan(...)`, `PreparedCallPlan`, `PreparedCallArgumentPlan`, `PreparedCallResultPlan`, `find_prepared_call_argument_publication_source_routing(...)`, `find_prepared_call_result_late_publication(...)`, `PreparedCallBoundaryEffectPlan`, `PreparedCallPreservedValue`, `PreparedClobberedRegister`, `PreparedValueHomeLookups`, `PreparedMoveBundleLookups`, `find_prepared_call_argument_source_producer_materialization(...)`, edge-publication source-producer lookups, and direct `find_prepared_addressing(...)` reads in `calls.cpp`. | Mixed. Route 6 source-producer/direct-global/publication source facts are route-backed semantic facts for bounded migration ideas; call ABI placement, variadic FPR count, clobbers, byval lanes, preservation moves, result homes, outgoing stack sizing, and helper/carrier protocols are durable target/codegen policy. Prepared call answers remain temporary migration oracles. | Proposed view: a narrow Route 6 call-use source view keyed by call instruction, argument/result role, and value role for semantic source facts only. Prepared fallback: call-plan and publication-routing helpers stay public until equivalence covers each argument/result class. Blocker: no BIR view should own ABI register/stack placement, move bundles, clobbers, late-publication mechanics, or call helper resources. |
| AArch64 memory and addressing | `PreparedAddressingFunction`, `PreparedAddressMaterializationLookups`, `PreparedMemoryAccessLookups`, `PreparedAddress`, `PreparedMemoryAccess`, `find_prepared_memory_access(...)`, `find_prepared_global_load_access(...)`, `find_prepared_same_block_global_load_access(...)`, `find_prepared_same_block_load_local_stored_value_source(...)`, frame-slot and pointer-base-plus-offset helpers, `PreparedStoreSourcePublicationPlan`, and `PreparedEdgePublicationSourceProducerLookups` in `memory.cpp`, `globals.cpp`, `frame_slot_address.cpp`, and `calls.cpp`. | Mixed. Route 3 memory/access identity, same-block global-load identity, and load-local stored-value source are route-backed semantic facts. Address base kind, offsets, stack objects, frame slots, TLS/global relocation fields, pointer materialization, base-plus-offset legality, memory operand formation, volatile/address-space lowering, and store/publication writeback policy are durable target/layout/codegen policy. Prepared memory answers remain temporary migration oracles. | Proposed view: Route 3 memory semantic view for access identity and source-value relationships, with prepared memory/access helpers retained as fallback/oracle. Blocker: target addressing and memory operand records stay prepared/AArch64-owned; migration must not force consumers to rescan BIR or copy `PreparedAddress`/`PreparedMemoryAccess` wholesale into BIR. |
| AArch64 value materialization and publication | `PreparedValueHomeLookups`, `find_prepared_value_home(...)`, `find_prepared_value_home_for_bir_value(...)`, `find_indexed_prepared_value_home(...)`, `PreparedValueHome`, `PreparedStoragePlanFunction`, `PreparedStoragePlanValue`, `PreparedMoveBundleLookups`, `PreparedMoveBundle`, current/block-entry publication helpers, `PreparedEdgePublicationLookups`, `PreparedEdgePublicationSourceProducerLookups`, `find_prepared_same_block_scalar_producer(...)`, `evaluate_prepared_same_block_integer_constant(...)`, select-chain materialization helpers, and direct-global/select-chain dependency helpers across `dispatch_producers.cpp`, `dispatch_publication.cpp`, `dispatch_value_materialization.cpp`, `fp_value_materialization.cpp`, `select_materialization.cpp`, `operands.cpp`, `prologue.cpp`, `calls.cpp`, and `memory.cpp`. | Mixed. Routes 1, 2, 4, and 5 provide route-backed semantic facts for producer identity, integer constants, select-chain/direct-global dependency, current/block-entry publication identity, and edge join-source identity. Homes, storage encodings, registers, stack slots, move phases, emitted storage availability, rematerialization spelling, and final publication/move records are durable target/codegen policy. Prepared helper families remain temporary migration oracles. | Proposed views: Route 1 producer/constant view, Route 2 select-chain view, Route 4 publication availability view or typed facade, and Route 5 edge/join-source view. Prepared fallback: value-home, storage, move-bundle, and edge-publication helpers stay visible until each selected materialization/publication path proves route/prepared equivalence. |
| AArch64 comparison and ALU | `find_materialized_condition_producer_identity(...)`, Route 7 facade validation, `find_prepared_same_block_scalar_producer(...)`, `find_prepared_scalar_select_chain_materialization(...)`, `find_prepared_return_chain_terminal_value(...)`, `find_prepared_return_chain_next_operand_value(...)`, `find_prepared_memory_access(...)`, `PreparedValueHome`, `PreparedStoragePlanFunction`, scalar operand/result helpers, and fused compare/branch record helpers in `comparison.cpp`, `alu.cpp`, `float_ops.hpp`, `cast_ops.cpp`, `atomics.cpp`, and related scalar record builders. | Mixed. Route 7 comparison/materialized-condition provenance, Route 1 producer/constant facts, and Route 2 select-chain facts are route-backed semantic facts. Branch spelling, fused-compare legality, condition-code selection, scalar register/storage selection, ALU machine-record formation, hazards, and emitted-register state are durable target/codegen policy. Return-chain reads are a no-route gap. | Proposed views: Route 7 condition/comparison provenance plus Route 1/2 semantic producer views where ALU operands currently ask prepared helpers for identity. Prepared fallback: comparison and scalar operand helpers remain public oracles. Blocker: return-chain helpers require a separate owner/schema decision before migration; do not fold them into Route 1 or Route 7. |
| AArch64 edge copies and control flow | `PreparedEdgePublicationLookups`, `PreparedEdgePublication`, `PreparedEdgePublicationSourceProducerLookups`, `PreparedCurrentBlockJoinParallelCopySourceFact`, `find_prepared_current_block_join_parallel_copy_source(...)`, `find_prepared_current_block_entry_publication(...)`, `PreparedMoveBundleLookups`, `find_prepared_move_bundle(...)`, block-entry/current-block publication helpers, and branch-target record helpers in `dispatch_edge_copies.cpp`, `dispatch_publication.cpp`, `dispatch.cpp`, and `comparison.cpp`. | Mixed. Routes 4 and 5 contain route-backed semantic publication and join-source facts. Move bundles, parallel-copy scheduling, out-of-SSA execution sites, source/destination homes, branch instruction spelling, and block-order emission remain durable target/codegen policy. Prepared edge-copy answers stay temporary migration oracles. | Proposed views: Route 4 current/block-entry publication view and Route 5 edge join-source view, ideally through typed references/facades where available. Prepared fallback: edge-publication and move-bundle helpers stay public while route equivalence proves missing predecessor, no-source, memory-source, and parallel-copy cases. |
| AArch64 wide values and runtime helpers | `PreparedI128CarrierFunction`, `PreparedI128Carrier`, `find_prepared_i128_carrier(...)`, `PreparedI128RuntimeHelperFunction`, `find_prepared_i128_runtime_helpers(...)`, `PreparedF128CarrierFunction`, `PreparedF128Carrier`, `find_prepared_f128_carrier(...)`, `PreparedF128TransportRecord*`, `PreparedI128TransportRecord*`, and call/result carrier interactions in `i128_ops.cpp`, `f128.cpp`, `memory.cpp`, and `calls.cpp`. | Durable target/layout/codegen policy surface, with occasional route-backed source identities only where carrier operands refer back to values already covered by Routes 1, 3, 4, or 6. | Blocker/adaptor boundary: keep carrier selection, helper boundary records, lane transport, memory backing, register-pair/full-width-register decisions, and runtime helper protocols prepared/AArch64-owned. Any future BIR view should expose only underlying semantic value/source identity and leave carrier layout and helper calls outside BIR ownership. |
| AArch64 runtime helper and special-entry interfaces | `PreparedVariadicEntryPlanFunction`, `PreparedVariadicEntryHelperOperandHomes`, `find_prepared_variadic_entry_plan(...)`, `find_prepared_variadic_entry_helper_operand_homes(...)`, `PreparedIntrinsicCarrier`, `PreparedInlineAsmCarrier`, `PreparedAtomicOperation`, dynamic-stack and frame helpers, plus target helper scratch requirements in `variadic.cpp`, `intrinsics.cpp`, `inline_asm.cpp`, `atomics.cpp`, and `memory_dynamic_stack.cpp`. | Durable target/layout/codegen policy surface. These helpers encode ABI, target helper protocol, storage homes, scratch resources, frame/dynamic-stack effects, or inline-asm/intrinsic carrier policy rather than one of the seven Phase A-C semantic route families. | Blocker/adaptor boundary: keep these prepared-owned unless a separate Phase A/B/C route defines target-neutral source facts. Step 3 should name them as explicit non-migration surfaces for this Phase D ladder, not as missing Route 1-7 work. |
| Future x86 prepared interface | `ConsumedPlans::prepared_lookups`, `consume_prepared_function_lookups(...)`, `shared_function_lookups()`, `shared_call_plan_lookups()`, `find_consumed_call_plan(...)`, `find_consumed_call_argument_plan(...)`, and `find_consumed_call_result_plan(...)` in `src/backend/mir/x86/x86.hpp`; route debug still accepts `PreparedBirModule`. | Interface-level temporary migration oracle plus durable target/codegen policy surface. Current x86 prepared use is a wrapper/facade over call-plan and route-debug reads, not a proven BIR semantic migration point. | Proposed boundary: keep x86 wrappers as interface-level consumers until AArch64 proves narrow route views. Future x86 migrations should consume the same semantic Route 6 call-use view or typed route facades, while retaining target-owned lowering, frame, regalloc, and call ABI policy. |
| Future riscv prepared interface | `emit_prepared_module(const PreparedBirModule&)`, local `make_prepared_function_lookups(...)`, and helper parameters taking `PreparedFunctionLookups*` in `src/backend/mir/riscv/codegen/emit.cpp` and `emit.hpp`. | Interface-level temporary migration oracle. The riscv path currently builds and threads the aggregate for prepared emission support; no route-backed consumer migration has been proven there. | Proposed boundary: do not design riscv-only BIR adapters in Phase D. Reuse route views validated by AArch64 follow-up ideas, then expose riscv consumers through the same semantic interfaces while keeping instruction selection and target formatting local. |
| Prepared printer, debug, and tests | `print_prepared_bir(...)`, prepared-printer submodules, x86 route-debug summaries, and oracle tests for prepared lookup helpers and route equivalence. | Temporary migration oracle. These surfaces are validation and diagnostic consumers rather than production semantic owners. | Keep as public oracle/fallback surfaces until each migration has equivalence coverage. Do not claim prepared API contraction merely because production AArch64 consumers switch first. |
| Return-chain consumers | `PreparedReturnChainLookups`, `prepared_return_chain_value_key(...)`, `make_prepared_return_chain_lookups(...)`, `find_prepared_return_chain_terminal_value(...)`, and `find_prepared_return_chain_next_operand_value(...)`, with AArch64 ALU consumers recovering terminal/next operand homes. | No-route gap requiring a separate owner/schema decision before migration. | Blocker: return-chain is outside Routes 1-7 for this active plan. It must remain prepared-owned until the separate return-chain owner/schema line is imported or activated; it must not be hidden under producer identity, comparison provenance, or a generic route-index facade. |

## Step 2 Handoff To Step 3

Step 3 should turn the route-backed rows above into explicit BIR view and
adapter boundaries. The lowest-risk semantic adapters appear to be:

- Route 4/5 publication and edge/join-source views, because typed references
  and selected facade validation already exist for some publication records.
- Route 1/2 producer, integer-constant, select-chain, and direct-global views
  for materialization/publication subpaths that already use prepared helpers as
  semantic oracles.
- Route 3 memory/source identity views for same-block global-load and
  load-local source recovery, while leaving address formation and frame/TLS
  policy prepared-owned.
- Route 6 call-use source views for one call argument/result source class at a
  time.
- Route 7 comparison/materialized-condition provenance views for consumers
  that need semantic condition identity without target branch policy.

Step 3 should also write durable non-migration boundaries for call ABI,
storage/home/move policy, memory operand formation, wide-value carriers,
runtime helpers, debug/printer oracle surfaces, x86/riscv interface wrappers,
and return-chain as a no-route blocker.
