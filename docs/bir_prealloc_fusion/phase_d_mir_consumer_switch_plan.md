# Phase D MIR Consumer Switch Plan

Source idea: `ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md`
Status: Finalized for lifecycle closure decision.

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

## Step 3 BIR View and Adapter Boundaries

Step 3 defines the consumer-facing boundary shape for the mapped families. A
Phase D BIR view is a narrow semantic query over route-owned records, typed
indexes, or an already validated facade. It is not a replacement
`PreparedFunctionLookups` aggregate, not a BIR rescan hook, and not a place to
store target lowering policy. Each adapter must be keyed by the consumer's
existing semantic handle, fail closed when the route record is absent, and keep
the matching prepared helper public as the oracle/fallback until route/prepared
equivalence is proven for that consumer group.

| Route/family boundary | Route-backed BIR view or adapter | Prepared fallback/oracle boundary | Durable non-migration boundary |
| --- | --- | --- | --- |
| Route 1 producer and constant identity | Expose a producer/constant view keyed by the consuming BIR value or instruction-local operand reference. The view may answer same-block scalar producer identity and integer-constant facts already recorded by Route 1 annotations or typed indexes. | Keep `find_prepared_same_block_scalar_producer(...)`, `evaluate_prepared_same_block_integer_constant(...)`, scalar operand helpers, and prepared value-publication paths available as comparison or fallback answers. | Do not move value homes, storage encodings, rematerialization spelling, register choice, or emitted machine records into the Route 1 view. |
| Route 2 select-chain and direct-global dependency | Expose a select-chain/direct-global view keyed by the selected value, root instruction, or publication/materialization operand. It should return only select-root identity, root instruction index, scalar eligibility, and direct-global dependency presence. | Keep prepared select-chain materialization and direct-global dependency helpers public until each materialization, publication, call, memory, and ALU consumer proves equivalent route reads. | Do not encode target materialization sequence choice, storage/home selection, memory operand formation, or call/publication policy in the Route 2 view. |
| Route 3 memory/access identity | Expose a memory semantic view keyed by a load/store/global-load access or source-value relationship. It may answer access identity, same-block global-load identity, and load-local stored-value source relationships. | Keep `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`, `find_prepared_global_load_access(...)`, `find_prepared_same_block_global_load_access(...)`, and load-local source helpers as public oracles. | Keep address base kind, offsets, stack/frame objects, TLS/global relocation data, pointer materialization, base-plus-offset legality, volatile/address-space behavior, and final memory operand records prepared/AArch64-owned or target-owned. |
| Route 4 publication availability | Expose a current/block-entry publication view through typed Route 4 references or the existing validated facade style. It should answer publication identity and availability facts for a single consumer path. | Keep prepared current-block and block-entry publication helpers, edge-publication lookup surfaces, prepared printer consumers, x86 wrappers, and route-oracle tests public while each selected publication path migrates. | Do not move value-home lookup, move planning, storage availability bookkeeping, final publication record construction, or block-order emission into Route 4. |
| Route 5 edge join-source identity | Expose an edge/join-source view keyed by the edge, predecessor, destination block, and copied value role. It should answer join-source identity, missing-predecessor, no-source, and memory-source semantic cases through typed Route 5 records. | Keep `PreparedEdgePublicationLookups`, `PreparedEdgePublicationSourceProducerLookups`, `find_prepared_current_block_join_parallel_copy_source(...)`, and move-bundle helpers as equivalence or fallback surfaces. | Keep parallel-copy scheduling, out-of-SSA execution placement, source/destination homes, move bundles, branch instruction spelling, and final edge-copy records in prepared or target code. |
| Route 6 call-use source facts | Expose a call-use source view keyed by call instruction plus argument/result role and value role. It should answer semantic source-producer, direct-global, and publication-source facts one argument/result class at a time. | Keep `PreparedCallPlanLookups`, call argument/result plans, publication-routing helpers, `find_prepared_call_argument_source_producer_materialization(...)`, value-home lookups, move-bundle lookups, and call-boundary effect plans public. | Keep ABI register/stack placement, variadic FPR counts, clobber/preserve sets, byval lanes, outgoing stack sizing, late-publication mechanics, helper resources, carrier protocols, and call machine-record spelling target-owned. |
| Route 7 comparison and materialized-condition provenance | Expose a condition/comparison provenance view keyed by the comparison or branch operand consumer. It should answer materialized-condition identity and fused-compare operand provenance without deciding target branch lowering. | Keep comparison prepared helpers, scalar producer/select-chain fallbacks, Route 7 facade validation surfaces, and fused-compare oracle tests visible until each branch/comparison consumer has equivalence coverage. | Keep branch spelling, fused legality, condition-code selection, hazard handling, emitted-register state, final branch/compare record ordering, and ALU result storage outside BIR ownership. |

## Step 3 Cross-Consumer Adapter Rules

- AArch64 traversal should continue to build and thread `PreparedBirModule` and
  the prepared lookup bundle as pass-local context. Follow-up migrations may add
  narrow route-view parameters beside the prepared context, but must not replace
  the bundle with a BIR-owned clone of `PreparedFunctionLookups`.
- Adapters should be route-specific and consumer-specific: one publication
  adapter, call-use adapter, memory semantic adapter, or comparison provenance
  adapter at a time. They should never scan all BIR nodes to rediscover a fact
  that the route index does not already own.
- Prepared printer, debug output, x86 route-debug summaries, and oracle tests
  remain public validation surfaces. Production AArch64 consumer migration does
  not by itself justify hiding these APIs.
- x86 and riscv remain interface-level consumers in this Phase D ladder. After
  AArch64 proves a semantic route view, those targets can consume the same
  route-facing interface while keeping their own instruction selection, frame,
  register-allocation, target formatting, and call ABI policy local.
- Wide-value carriers, runtime helper protocols, variadic entry helpers,
  intrinsics, inline asm, atomics, dynamic stack, frame helpers, and special
  target entries are explicit non-migration surfaces for Routes 1-7. A route
  view may expose only an underlying semantic source identity that such helpers
  already reference; carrier layout, lane transport, scratch resources, helper
  calls, and target records stay prepared/AArch64-owned or target-owned.
- Return-chain remains a no-route blocker for this active plan. Consumers of
  `PreparedReturnChainLookups`, terminal return-chain values, and next-operand
  values need the separate owner/schema decision before any migration, and must
  not be hidden under Route 1 producer identity, Route 7 comparison provenance,
  predecessor rescans, name matching, or a generic route-index facade.

## Step 3 Handoff To Step 4

Step 4 should order the migration ladder by choosing bounded follow-up ideas
from these boundaries. Each follow-up should name one route-backed consumer
group, the exact BIR view or facade it will read, the prepared oracle it will
compare against, the target/layout/codegen policy it refuses to migrate, and
the proof ladder needed before any prepared API contraction claim.

## Step 4 Ordered Migration Ladder

The Phase D migration ladder should start with consumer groups that already
have route-backed records, typed indexes, or selected facade validation, then
move outward to mixed consumers that still rely on prepared target policy. Each
rung is a bounded follow-up idea candidate. None of these rungs claims prepared
API deletion; prepared helpers remain public oracles/fallbacks until a later
contraction idea proves that every residual production, printer, debug, target,
and test consumer has moved or has an intentional replacement.

| Order | Follow-up idea summary | Consumer group | BIR route prerequisite/view | Prepared oracle/fallback boundary | Out-of-scope target policy | Proof recommendation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Switch one AArch64 current/block-entry publication reader to the Route 4 typed publication view or existing validated facade style. | AArch64 value publication and dispatch publication paths that ask for current/block-entry publication identity. | Route 4 publication availability records, typed block-entry/current-block references, and the fail-closed facade pattern proven by the selected Route 4 facade work. | Keep prepared current-block and block-entry publication helpers, edge-publication lookup surfaces, prepared printer/debug consumers, x86 wrappers, and oracle tests as public comparison answers. | Value-home lookup, storage availability, move planning, publication record construction, and block-order emission stay prepared/AArch64-owned. | Add route/prepared equivalence tests for the selected publication reader, including present, absent, and mismatched-reference cases; run the narrow AArch64 publication subset plus the existing Route 4 oracle tests before considering any later contraction. |
| 2 | Switch one AArch64 edge-copy join-source reader to a Route 5 edge/join-source view. | AArch64 edge-copy and join parallel-copy source recovery. | Route 5 edge join-source records keyed by edge, predecessor, destination block, and copied value role, including missing-predecessor, no-source, and memory-source cases. | Keep `PreparedEdgePublicationLookups`, `PreparedEdgePublicationSourceProducerLookups`, `find_prepared_current_block_join_parallel_copy_source(...)`, and move-bundle helpers as fallbacks and equivalence oracles. | Parallel-copy scheduling, out-of-SSA placement, source/destination homes, move-bundle selection, branch spelling, and final edge-copy records stay prepared or target-owned. | Prove same-answer behavior against prepared join-source helpers across normal predecessor, missing predecessor, no-source, memory-source, and absent-route cases; include route-index fail-closed coverage before proposing follow-up contraction. |
| 3 | Switch one scalar value materialization or publication subpath to Route 1 producer/constant facts. | AArch64 scalar producer, integer-constant, and value-publication subpaths that currently query same-block scalar producer or integer constant helpers. | Route 1 producer and constant identity view keyed by the consuming BIR value or instruction-local operand reference. | Keep `find_prepared_same_block_scalar_producer(...)`, `evaluate_prepared_same_block_integer_constant(...)`, scalar operand helpers, and prepared value-publication paths public. | Value homes, storage encodings, register choice, rematerialization spelling, emitted machine records, and move generation stay prepared/AArch64-owned. | Compare route-first answers with prepared helper answers for same-block producer, constant, no-producer, non-constant, and cross-block cases; pair the selected consumer test with existing Route 1 oracle coverage. |
| 4 | Switch one select-chain/direct-global materialization reader to the Route 2 select-chain view. | AArch64 select materialization, direct-global dependency checks, and scalar publication/call/memory subpaths that need select-root facts. | Route 2 select-chain/direct-global view returning select-root identity, root instruction index, scalar eligibility, and direct-global dependency presence. | Keep prepared select-chain materialization and direct-global dependency helpers as public fallbacks for publication, call, memory, ALU, and printer/test consumers. | Target materialization sequence choice, storage/home selection, memory operand formation, call-publication policy, and final record spelling remain outside the Route 2 view. | Prove route/prepared equivalence for selected direct-global present/absent, scalar-eligible/ineligible, and nested select-chain cases; do not weaken prepared oracle tests when moving the consumer. |
| 5 | Switch one semantic memory source reader to the Route 3 memory/access identity view. | AArch64 memory, globals, and load-local source recovery paths that need semantic access identity rather than target address formation. | Route 3 memory semantic view for access identity, same-block global-load identity, and load-local stored-value source relationships. | Keep `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`, global-load helpers, same-block global-load helpers, and load-local source helpers public. | Address base kind, offsets, stack/frame objects, TLS/global relocation data, pointer materialization, base-plus-offset legality, volatile/address-space behavior, and memory operand records stay prepared/AArch64-owned or target-owned. | Test semantic identity agreement for global-load, same-block global-load, local-stored source, absent-route, and non-semantic target-address-only cases; keep address-materialization tests as prepared-owned policy proof. |
| 6 | Switch one scalar call argument/result source reader to the Route 6 call-use source view. | AArch64 call argument/result source-producer and publication-source materialization for one role class at a time. | Route 6 call-use source records keyed by call instruction plus argument/result role and value role. | Keep `PreparedCallPlanLookups`, call argument/result plans, publication-routing helpers, `find_prepared_call_argument_source_producer_materialization(...)`, value-home lookups, move-bundle lookups, and call-boundary effect plans public. | ABI register/stack placement, variadic FPR counts, clobber/preserve sets, byval lanes, outgoing stack sizing, late-publication mechanics, helper resources, carrier protocols, and call record spelling stay target-owned. | Prove the selected argument/result class against prepared call-source helpers, including direct source, publication source, missing source, and recursive operand fallback cases; include existing call-plan tests to show ABI policy did not move. |
| 7 | Switch one comparison or branch operand provenance reader to the Route 7 comparison view. | AArch64 fused-compare, materialized-condition, branch operand, and scalar comparison provenance readers. | Route 7 condition/comparison provenance view or validated facade keyed by comparison or branch operand consumer. | Keep comparison prepared helpers, scalar producer/select-chain fallbacks, Route 7 facade validation, and fused-compare oracle tests visible. | Branch spelling, fused legality, condition-code selection, hazard handling, emitted-register state, final branch/compare record ordering, ALU result storage, and return-chain operand recovery stay outside BIR ownership. | Prove provenance equality for fused-compare, materialized condition, unfused fallback, absent-route, and invalid-reference cases; route-first behavior must still defer to prepared/target branch policy. |
| 8 | Reuse proven AArch64 route-view interfaces at x86 or riscv prepared wrapper boundaries. | Future x86 `ConsumedPlans` wrappers and riscv `emit_prepared_module(...)`/prepared lookup threading. | Only route views already proven by AArch64 rungs above; no x86-only or riscv-only BIR adapter should be invented first. | Keep x86 consumed prepared wrappers, riscv prepared-module emission, route debug, and target-local prepared lookup threading as fallbacks. | x86/riscv instruction selection, target formatting, frame layout, register allocation, call ABI policy, and emission records remain target-local. | After an AArch64 route-view rung is green, add interface-level compile and route/debug tests for the target wrapper consuming the same view; do not use these targets as the first proof of a new semantic route view. |

## Step 4 Deferred Or Blocked Items

- `PreparedFunctionLookups` aggregate contraction is deferred until multiple
  route-view consumer switches prove that the aggregate fields have no residual
  production, printer, debug, target-wrapper, or oracle-test users. A follow-up
  may thread narrow route views beside the prepared context, but must not build
  a BIR-owned clone of the aggregate.
- Prepared printer, debug, route-debug, and oracle-test surfaces remain public
  validation consumers. They can move only after the production consumer family
  they validate has an equivalent BIR route view and replacement diagnostics.
- Wide-value carriers, runtime helper protocols, variadic entry, intrinsics,
  inline asm, atomics, dynamic stack, frame helpers, and special target entries
  are not Phase D route-backed migration rungs. They may consume semantic
  source identities exposed by earlier rungs, but carrier layout, helper
  protocols, scratch resources, frame effects, and target records remain
  prepared/AArch64-owned or target-owned.
- Return-chain consumers are blocked behind the separate return-chain
  owner/schema line. They should remain prepared-owned until that result is
  explicitly imported or activated; this ladder must not hide return-chain
  terminal or next-operand reads under Route 1, Route 7, predecessor rescans,
  name matching, or a generic route-index facade.

## Step 4 Follow-Up Idea Template

Each follow-up idea should stay one-rung wide and use this payload shape:

- Consumer group: the exact AArch64, x86, or riscv consumer family being moved.
- BIR route prerequisite/view: the route record, typed index, or validated
  facade the consumer will read, including fail-closed behavior.
- Prepared oracle/fallback boundary: the prepared helper family that remains
  public for route/prepared comparison and fallback.
- Out-of-scope target policy: the ABI, storage, layout, helper, address,
  branch, move, or final-record policy that the idea refuses to migrate.
- Proof recommendation: narrow route/prepared equivalence coverage for the
  selected consumer plus existing route oracle tests and any target-policy
  preservation checks needed to prove no prepared policy moved into BIR.

## Step 5 Final Readiness And Lifecycle Handoff

This artifact satisfies the Phase D source idea completion criteria and is ready
for lifecycle closure review. It remains an analysis artifact only: it does not
modify AArch64, x86, or riscv lowering, and it does not claim that selected
consumer migrations from ideas 167-174 allow broad prepared API deletion.

Completion criteria check:

- Phase A-C and route-closure links are recorded in the Step 1 evidence table,
  including the Phase A/B/C documents, closed route ideas 167-174, and the
  separate return-chain owner/schema line.
- Phase C residual-consumer findings are preserved: each route closure is
  recorded as selected-consumer or narrow-facade progress, while prepared
  helpers remain public oracles, fallbacks, target-owned policy surfaces, or
  residual production/debug/test consumers until separate equivalence and
  contraction work proves otherwise.
- The Step 2 dependency map covers AArch64 traversal and dispatch, calls,
  memory/addressing, value materialization and publication, comparison/ALU,
  edge copies/control flow, wide values/runtime helpers, and future x86/riscv
  interface-level consumers.
- The Step 3 boundaries define route-specific BIR views and temporary adapters
  without introducing broad BIR rescans, `PreparedFunctionLookups` clones,
  target-policy ownership in BIR, or target-specific Phase D adapters ahead of
  AArch64 route-view proof.
- The Step 4 migration ladder gives each proposed follow-up a consumer group,
  BIR route prerequisite or view, prepared oracle/fallback boundary,
  out-of-scope target policy, and proof recommendation.
- Return-chain remains a no-route blocker for this active Phase D route. It is
  linked to `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`
  and closed ideas 176-180 as separate context, and is not absorbed into Route
  1 producer identity, Route 7 comparison provenance, predecessor rescans, name
  matching, or a generic route-index facade.
- No implementation source changes are required or proposed by this Phase D
  artifact.

The clean route review in `review/phase_d_artifact_route_review.md` found no
blocking route-quality issues and judged the artifact aligned with the source
idea, on track, and sufficiently proven by narrow docs-only validation. The
only review note was to keep lifecycle metadata cleanup in `todo.md`, not to
rewrite the route.

Plan-owner lifecycle work can now decide whether to close the active Phase D
idea or create/activate separate follow-up implementation ideas from the Step 4
ladder. Those follow-ups should remain one-rung slices and should not be
created from this executor packet.
