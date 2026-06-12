# Phase E0 Prepared/BIR Merge Readiness

Status: Step 4 E1-E5 readiness synthesis complete.

Source idea: `ideas/open/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`

## Evidence Inventory

This document is the durable Phase E0 analysis surface for the future
Prepared/BIR merge readiness map. Step 1 is inventory only: it records the
inputs, structural surfaces, and retained-boundary evidence that the later
sections classify. It does not implement, delete, privatize, rename, or weaken
prepared APIs, diagnostics, tests, or target behavior.

### Sources Read

The Step 1 inventory used these required inputs:

| Source | Evidence captured |
| --- | --- |
| `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md` | Phase D direction: migrate one semantic consumer group at a time, keep prepared answers as oracles until equivalence is proven, keep target/layout/codegen policy outside BIR, and treat return-chain as a separate owner/schema line. |
| `ideas/closed/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md` | Ideas 182-189 are Phase D follow-up slices, not true Phase E retirement. Draft 155 was not ready because field ownership, fallback/oracle, target-policy, cross-target, and return-chain blockers remained. |
| `ideas/closed/192_residual_route_view_consumer_migration_map.md` | Route 1-7 residual production, printer/debug, target-wrapper, oracle/test, and retained-policy consumers remain after selected readers moved. |
| `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md` | D2 retained surfaces remain one-surface or one-row follow-up candidates only; aggregate `PreparedFunctionLookups`, aggregate `PreparedBirModule`, broad wrappers, baseline/string authority, and return-chain contraction are no-action. |
| `ideas/closed/207_route3_memory_source_semantic_reader.md` | Route 3 can supply selected memory/source identity only under route/prepared agreement; address formation, relocation, addressing legality, materialization, final operands, and target policy remain prepared/target-owned. |
| `ideas/closed/208_route3_memory_source_oracle_printer_row.md` | Route 3 diagnostic replacement is row-scoped; prepared diagnostics remain authoritative for fallback, non-agreement, ambiguity, mismatch, and target-policy-sensitive output. |
| `ideas/closed/209_route4_publication_source_semantic_reader.md` | Route 4 can identify a selected publication source only with prepared publication fallback for absent, invalid, ambiguous, and mismatched facts. |
| `ideas/closed/210_route4_block_entry_publication_printer_debug_row.md` | Route 4 printer/debug replacement is one byte-stable block-entry row with prepared agreement and fail-closed fallback; wrapper and publication mechanics remain retained. |
| `ideas/closed/211_route5_current_block_join_source_semantic_reader.md` | Route 5 can supply one current-block join-source identity under agreement; join, edge, move-bundle, branch, parallel-copy, output, and wrapper behavior remain prepared/target-owned. |
| `ideas/closed/212_route5_edge_join_oracle_printer_row.md` | Route 5 diagnostic metadata is retained row-scoped evidence only; absent, invalid, duplicate/conflict, memory-source, mismatch, unsupported, branch/parallel-copy, printer, and wrapper paths stay retained. |
| `ideas/closed/213_route6_call_source_consumer.md` | Route 6 can supply one call argument/result source under agreement; ABI, wrapper, aggregate transport, call records, printer behavior, storage, movement, and emitted behavior remain prepared/target-owned. |
| `ideas/closed/214_route6_x86_scalar_source_route_debug_row.md` | One x86 Route 6 scalar source route-debug row is proven through `ConsumedPlans`; this is not broad x86 call-wrapper migration or `ConsumedPlans` removal. |
| `ideas/closed/215_route7_comparison_provenance_consumer.md` | Route 7 can supply one comparison provenance consumer under agreement; branch behavior, suffixes, fused legality, final assembler, and expected-string output remain AArch64/prepared-owned. |
| `ideas/closed/216_route7_comparison_oracle_row.md` | One Route 7 materialized-condition helper-oracle row can use route-native evidence while preserving prepared oracle assertions, fallback, wrappers, branch-control output, and expected strings. |
| `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md` | Selected-consumer migrations are useful evidence but not prepared API deletion, route-wide migration, or aggregate contraction. It records retained fallback/oracle, target-policy, cross-target, return-chain, and draft-readiness blockers. |
| `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md` | Route 1-7 map of selected-reader evidence, residual consumers, required route facts, fallback/oracle surfaces, and proof expectations. |
| `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md` | Retained-surface dependency map, one-surface follow-up decisions, no-action decisions for aggregates/pass-context/wrappers/baseline/Route 8, and current consumer anchors. |
| `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md` | Lookup group ownership map: route-native subfacts exist inside some groups, but no lookup group is deletion-, privatization-, or aggregate-replacement-ready. |
| `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md` | Draft Phase E intent: stop treating `PreparedBirModule` as a second IR only after BIR normalization, annotation schema, cache contraction, and MIR consumer migration have enough coverage; answer field durability, BIR annotation, private pass-context, wrapper, and compatibility questions; inspect construction, mutation, consumers, printer/dump tooling, and Phase A-D proof; reject premature removal, renamed-wrapper hiding, lost diagnostics, and unrelated backend feature bundling. |
| `src/backend/prealloc/module.hpp` | Structural inventory of `PreparedBirModule` fields and public inline lookup/helper surfaces. |
| `src/backend/prealloc/prepared_lookups.hpp` | Structural inventory of `PreparedFunctionLookups` lookup groups and construction entry points. |

Draft 155 was read as an evidence input only. This inventory does not activate,
rewrite, or execute that draft; it uses the draft to preserve the intended
Phase E retirement criteria and reject signals while this Phase E0 pass maps
the safer ownership boundaries first.

### PreparedBirModule Field Inventory

`PreparedBirModule` currently aggregates target-neutral BIR, target profile and
route metadata, prepared semantic/policy products, target-specific helper
plans, diagnostics, and phase notes:

| Field | Raw surface captured for later classification |
| --- | --- |
| `module` | BIR module payload; the later map must separate BIR-owned semantic state from prepared wrapper compatibility. |
| `target_profile` | Target ABI/layout/profile policy input; retained target/prepared policy unless a later source names a target-neutral replacement. |
| `route` | Preparation route selection/defaulting surface; likely pass-context/diagnostic metadata until classified. |
| `invariants` | Prepared invariant diagnostics/validation state; diagnostic/oracle authority must remain visible. |
| `names` | Prepared name tables and id/string bridge; may support BIR identity lookup but also diagnostics and public compatibility. |
| `control_flow` | Prepared control-flow products consumed by lowering and lookup construction; classify semantic CFG facts separately from target traversal/policy. |
| `value_locations` | Value location/home information; target/prepared policy for homes, registers, stack slots, storage, and rendering remains retained. |
| `stack_layout` | Stack/frame layout policy; retained target/prepared policy. |
| `addressing` | Address materialization and target addressing policy; retained target/prepared policy. |
| `liveness` | Prepared liveness/pass state; classify as BIR annotation candidate or transient pass context only with consumer proof. |
| `register_group_overrides` | Register group override policy; retained target/prepared policy unless later proof isolates pure semantic annotation. |
| `regalloc` | Register allocation state; retained target/prepared policy. |
| `frame_plan` | Frame plan policy; retained target/prepared policy. |
| `dynamic_stack_plan` | Dynamic stack plan policy; retained target/prepared policy. |
| `call_plans` | Call ABI/layout/helper plans; Route 6 subfacts may be semantic, but whole plans remain target/prepared policy and fallback/oracle. |
| `store_source_publications` | Store-source publication planning; overlaps Routes 1/2/3/4/6 source facts but still includes publication/materialization policy and fallback surfaces. |
| `variadic_entry_plans` | Variadic entry ABI/helper policy; retained target/prepared policy. |
| `storage_plans` | Storage/home/encoding policy; retained target/prepared policy. |
| `i128_carriers` | Wide-value carrier protocol; retained target/prepared or helper policy. |
| `f128_carriers` | Wide-value carrier protocol; retained target/prepared or helper policy. |
| `atomic_operations` | Atomic operation lowering/helper policy; retained target/prepared policy unless a semantic-only reader is later proven. |
| `intrinsic_carriers` | Intrinsic carrier/helper protocol; retained target/prepared/helper policy. |
| `inline_asm_carriers` | Inline asm carrier/protocol state; retained target/prepared policy. |
| `f128_runtime_helpers` | Runtime helper policy/protocol; retained target/prepared/helper policy. |
| `i128_runtime_helpers` | Runtime helper policy/protocol; retained target/prepared/helper policy. |
| `completed_phases` | Phase/debug metadata; diagnostic/pass-context surface. |
| `notes` | Prepared notes/diagnostics; diagnostic/string authority. |

The header also exposes public inline lookup helpers over the aggregate, such
as register-group overrides, addressing, value-location functions, frame and
dynamic-stack plans, call plans, variadic helper operand homes, storage plans,
wide carriers, atomics, intrinsics, inline asm, and runtime helpers. Later
steps must treat these as public prepared surfaces until every production,
wrapper, printer/debug, and oracle consumer has a replacement or retained-owner
decision.

## PreparedBirModule Ownership Classification

This Step 2 map classifies every `PreparedBirModule` field. A deletion or
contraction candidate below is code-size-positive only for the named duplicate
helper/API family or selected semantic subfact; it is not permission to delete
the whole field, move target policy into BIR, or collapse public prepared
fallback/oracle surfaces.

| Field | Classification | Ownership boundary | Duplicate helper/API family candidate | Replacement BIR fact | Retained fallback/policy/oracle | Proof required before deletion or contraction |
| --- | --- | --- | --- | --- | --- | --- |
| `module` | BIR-owned semantic fact/index/identity candidate plus cross-target compatibility boundary | The `bir::Module` payload is the canonical target-neutral IR and should own durable semantic facts that later routes read directly. The `PreparedBirModule` aggregate still owns compatibility delivery while prepared consumers and printers expect the wrapper. | Aggregate `PreparedBirModule` forwarding where a consumer only needs `bir::Module` or route facts. | BIR module functions, blocks, typed instructions, terminators, values, calls, memory addresses, comparison facts, and future route annotations. | Prepared wrapper compatibility, target profile threading, pass ordering, prepared printer/dump entry points, and public fallback/oracle calls. | Every selected consumer in the replacement family must read the BIR fact directly with prepared agreement proof, byte-stable diagnostics or retained oracle rows, and no target-policy dependency hidden in the BIR lookup. |
| `target_profile` | Retained target/prepared policy | Target ABI, arch, register, layout, and feature policy stay outside target-neutral BIR. | None. | None. | ABI/layout/register/feature policy and target-specific fallback behavior. | Not deletion-ready; only duplicate local threading could contract after all users receive an explicit target-policy owner. |
| `route` | Private/transient pass context and diagnostic/string authority | Route selection/defaulting is preparation control state, not a BIR semantic fact. | Public route-checking helpers only if they duplicate an internal pass-context decision. | None. | Route-debug output, fallback selection, compatibility messaging, and preparation notes. | Prove all route-visible diagnostics keep their expected strings and no public consumer relies on route value identity. |
| `invariants` | Retained public fallback/oracle and diagnostic/string authority | Prepared invariants remain the agreement oracle for semantic-route migrations and the diagnostic surface for mismatch/fallback cases. | Row-scoped invariant/oracle helper rows after route-native replacement exists. | Route/prepared agreement facts for the specific migrated semantic row. | Positive, absent, invalid, duplicate/conflict, mismatch, fallback, and target-policy-sensitive diagnostics. | One row at a time: route-native evidence must reproduce the row byte-stably or keep the prepared oracle authoritative for every non-agreement path. |
| `names` | BIR annotation candidate plus retained diagnostic/string authority | Name/id tables bridge BIR identities to prepared value/function/block/link names. Pure identity annotations may move closer to BIR; printable strings and compatibility lookup remain retained. | Name-resolution helpers such as prepared function/value/block/link lookup wrappers when used only to recover BIR identity. | Stable BIR function, block, value, link, and source identity annotations. | Expected strings, prepared printer labels, helper-oracle names, public compatibility APIs, and ambiguity/fallback handling. | All consumers in a named family must use stable BIR identity annotations while byte-stable printers/oracles continue to resolve names for expected output. |
| `control_flow` | BIR-owned semantic fact/index/identity candidate with retained prepared traversal policy | Target-neutral CFG identity, predecessor/successor facts, block labels, and selected route join/publication identities are BIR-owned candidates. Prepared traversal, lowering order, and lookup construction remain retained until consumers migrate. | Control-flow lookup/facade helpers that only rediscover BIR block/function/edge identity. | BIR function/block graph, terminator successors, route edge/join annotations, and future return-chain owner/schema facts. | Edge-copy scheduling, branch policy, block-order emission, wrappers, printer/debug rows, and fallback/oracle checks. | Per-route consumer proof for positive/absent/invalid/duplicate/mismatch cases, plus no regression in branch, edge-copy, or printer expected strings. |
| `value_locations` | Retained target/prepared policy with narrow BIR annotation candidates | Homes, registers, stack slots, rematerialization spelling, rendering, and storage choices are target/prepared policy. Only source-value identity subfacts may become BIR annotations when proven separately. | Selected source-identity reads that currently pass through value-home helpers. Whole `find_prepared_value_location_function` and value-home APIs are not deletion-ready. | Route source identity annotations for the specific migrated consumer. | Value-home policy, register/stack selection, rematerialization strings, move records, storage rendering, and fallback/oracle behavior. | A source-reader replacement must prove route/prepared agreement and keep all home/layout/rendering decisions prepared-owned. |
| `stack_layout` | Retained target/prepared policy | Stack objects, slots, offsets, exposure, frame interaction, and layout order are target policy. | None at field level. | None. | Stack/frame layout, address-exposed object handling, slot naming, printer output, and target compatibility. | Not deletion-ready; any future contraction requires a separate target-policy owner and byte-stable stack/frame diagnostics. |
| `addressing` | Retained target/prepared policy | Address formation, materialization, relocation, offsets, legality, final operands, and address-space/volatile handling remain prepared/target-owned. | None at field level; selected source identity must not be confused with address policy. | Route 3 memory/source identity only for agreement-gated readers. | `find_prepared_addressing`/address-materialization lookups, relocation and legality policy, target wrappers, and prepared fallback/oracle rows. | Route 3 source readers must prove agreement while address formation and materialization expected strings remain prepared-owned. |
| `liveness` | BIR annotation candidate and private/transient pass context | Pure live ranges, use/def facts, and address-taken annotations may become BIR annotations, but allocation pressure and later target decisions remain pass-context/policy. | Liveness lookup helpers that merely recompute BIR use/def/live interval annotations. | BIR value use/def, live interval, address-taken, and requires-home annotations. | Prepared pass ordering, stack-object association, diagnostics, and consumers that combine liveness with target allocation policy. | Consumers must read equivalent BIR annotations with agreement proof and retain prepared diagnostics for ambiguity or stack-object mismatch. |
| `register_group_overrides` | Retained target/prepared policy | Register bank/class override policy is target-specific allocation guidance. | None. | None. | `find_prepared_register_group_override`, register group diagnostics, and target allocation behavior. | Not deletion-ready unless a later target-policy owner replaces every override consumer and diagnostic. |
| `regalloc` | Retained target/prepared policy | Allocation results, spill choices, register classes, and target hazards are prepared/target-owned. | None at field level. | None. | Register allocation state, fallback, diagnostics, and final emission expectations. | Not deletion-ready; requires a separate regalloc owner, not a BIR semantic route fact. |
| `frame_plan` | Retained target/prepared policy | Frame-pointer use, saved registers, spill slots, fixed slots, and frame bookkeeping are target ABI/layout policy. | None at field level; `find_prepared_frame_plan` remains public prepared policy. | None. | Frame layout, saved-register policy, stack slot printing, and target wrapper behavior. | Not deletion-ready without a new target frame-policy owner and byte-stable frame diagnostics. |
| `dynamic_stack_plan` | Retained target/prepared policy | Dynamic stack operations, probing/adjustment sites, and frame-pointer interactions are target policy. | None at field level; `find_prepared_dynamic_stack_plan` remains retained. | None. | Dynamic stack lowering policy, diagnostics, and target emission compatibility. | Not deletion-ready without target-policy replacement and preserved dynamic-stack printer rows. |
| `call_plans` | Retained target/prepared policy with selected BIR-owned semantic subfacts | ABI placement, wrappers, clobbers, outgoing stack, byval/variadic lanes, helper/carrier protocols, result lanes, final call records, and storage remain prepared/target-owned. Route 6 may own selected call argument/result source identity. | Selected Route 6 source-reader helpers and row-scoped call-source oracle/debug helpers. Whole `find_prepared_call_plans` and call-plan lookups are not deletion-ready. | Route 6 call-use source identity annotation under route/prepared agreement. | Call ABI/layout policy, aggregate transport, helper protocols, call records, `ConsumedPlans`, printer/debug rows, and fallback/oracle behavior. | Per-consumer Route 6 agreement proof across positive and failure cases, including x86 `ConsumedPlans` compatibility where applicable, with no call-wrapper or expected-string regression. |
| `store_source_publications` | BIR-owned semantic subfact candidate plus retained publication policy/fallback | Store/publication source identity overlaps Routes 1-4 and 6, but publication construction, materialization, block order, invalid/ambiguous handling, and output mechanics stay retained. | Selected store/publication source producer helpers and row-scoped publication-source printer/oracle helpers. | Route 1/2/3/4/6 source identity or publication-source annotations under agreement. | Publication plans, materialization policy, fallback for absent/invalid/ambiguous/mismatch, wrappers, block-entry printer rows, and expected strings. | One route/source row at a time with route/prepared agreement and preserved publication mechanics and diagnostics. |
| `variadic_entry_plans` | Retained target/prepared policy | Variadic ABI helper entry plans and helper operand homes are target ABI/helper policy. | None at field level; `find_prepared_variadic_entry_plan` and helper operand-home lookup remain retained. | None. | Variadic ABI, helper operand homes, target wrappers, and diagnostics. | Not deletion-ready without a target ABI owner and byte-stable variadic helper diagnostics. |
| `storage_plans` | Retained target/prepared policy | Storage class, register/stack/symbol placement, computed address policy, and rendering are target/prepared-owned. | None at field level; `find_prepared_storage_plan` remains retained. | None. | Storage placement, final operand rendering, symbol/address policy, printer rows, and fallback/oracle behavior. | Not deletion-ready; selected route facts must not absorb storage policy. |
| `i128_carriers` | Retained target/prepared/helper policy | Wide integer lane/carrier protocol, lane roles, split locations, and helper movement are target/helper policy. | None at field level; carrier lookup helpers remain retained. | None. | I128 carrier protocol, ABI/helper movement, printer rows, and runtime-helper coordination. | Not deletion-ready without a wide-value helper-policy owner and unchanged helper/oracle expected strings. |
| `f128_carriers` | Retained target/prepared/helper policy | Wide float carrier protocol, lane/scalar homes, helper movement, and comparison support are target/helper policy. | None at field level; carrier lookup helpers remain retained. | None. | F128 carrier protocol, ABI/helper movement, printer rows, and runtime-helper coordination. | Not deletion-ready without a wide-value helper-policy owner and unchanged helper/oracle expected strings. |
| `atomic_operations` | Retained target/prepared policy | Atomic lowering, memory ordering, address-space, helper selection, and final emission are target/prepared policy. | None at field level unless a later semantic-only atomic fact is introduced. | None currently identified. | Atomic lowering policy, target features, memory/addressing policy, diagnostics, and final emitted behavior. | Blocked/unknown pending a future semantic-only atomic reader and target-policy split proof. |
| `intrinsic_carriers` | Retained target/prepared/helper policy | Intrinsic family/operation metadata begins in BIR, but carrier completeness, features, operand/result homes, memory operands, helper call linkage, and target lowering stay prepared/helper-owned. | Only duplicate intrinsic metadata adapters that merely restate BIR intrinsic family/operation could contract; carrier lookup remains retained. | BIR intrinsic family, operation, feature, operand-role, memory-access, and side-effect facts. | Required-feature policy, prepared call plans, operand/result homes, helper protocols, diagnostics, and fallback for missing required facts. | Prove each metadata consumer reads BIR intrinsic facts while carrier completeness, target lowering, and expected diagnostics remain prepared-owned. |
| `inline_asm_carriers` | Retained target/prepared policy and cross-target compatibility boundary | Constraint parsing, operand homes, memory-address selection, clobbers, result-home rules, and missing-fact diagnostics are target/compiler policy. | None at field level. | None. | Inline asm carrier protocol, constraint diagnostics, memory-address authority, target wrappers, and emitted behavior. | Not deletion-ready; requires a dedicated inline-asm policy owner and preserved diagnostics. |
| `f128_runtime_helpers` | Retained target/prepared/helper policy | Runtime helper family/kind, ABI transitions, marshalling, carrier bindings, result ownership, and comparison helper rows are helper/target policy. | Row-scoped helper-oracle readers only when a route-native semantic row has already been proven. Whole runtime-helper lookup remains retained. | Route 7 comparison provenance or selected BIR intrinsic/f128 operation facts for the narrow row. | Helper ABI policy, marshalling moves, carrier bindings, expected helper strings, fallback/oracle assertions, and target wrappers. | One helper/oracle row at a time with byte-stable output and retained helper ABI behavior. |
| `i128_runtime_helpers` | Retained target/prepared/helper policy | Runtime helper family/kind, lane marshalling, ABI transitions, result ownership, and helper call records are helper/target policy. | None at field level except duplicate metadata adapters once BIR owns an equivalent semantic fact. | BIR i128 operation or intrinsic facts for narrow metadata-only consumers. | Helper ABI policy, lane movement, carrier bindings, expected helper strings, fallback/oracle assertions, and target wrappers. | Prove metadata-only contraction separately while preserving helper ABI/marshalling behavior and diagnostics. |
| `completed_phases` | Private/transient pass context and diagnostic/string authority | Phase completion is execution metadata for preparation and debug output. | Public phase-status checks if they duplicate internal pass-context state. | None. | Debug output, pass sequencing, and compatibility with current dump text. | Prove no public consumer depends on phase names or ordering, or preserve the names in a retained diagnostic surface. |
| `notes` | Retained public fallback/oracle and diagnostic/string authority | Notes are prepared diagnostics and reject/fallback explanations. | Row-scoped note/oracle helpers after equivalent route-native diagnostics exist. | None unless a future route diagnostic schema owns the exact row. | Existing diagnostic text, expected strings, fallback messages, and helper-oracle authority. | Byte-stable diagnostic replacement or explicit retained prepared authority for every positive and failure path. |

### Code-Size-Positive Candidate Summary

The realistic code-size-positive candidates are narrow semantic duplicate
families, not target-policy fields:

| Candidate family | Fields involved | Replacement authority | Surfaces that must remain until later proof |
| --- | --- | --- | --- |
| Aggregate BIR semantic forwarding | `module`, selected `control_flow`, selected `names` | Direct BIR module graph and stable BIR identity annotations | `PreparedBirModule` wrapper, prepared printers, target profile threading, fallback/oracle diagnostics |
| Route source/publication identity helpers | `store_source_publications`, selected `control_flow`, selected `value_locations`, selected `call_plans` | Route 1-7 source, publication, join, call-use, and comparison provenance annotations under route/prepared agreement | Addressing, homes, storage, move scheduling, call ABI, publication mechanics, wrappers, printer/debug rows, and helper oracles |
| Liveness identity annotations | `liveness`, selected `names` | BIR use/def, live interval, address-taken, and requires-home annotations | Prepared pass-context ordering, stack-object association, diagnostics, and allocation-policy consumers |
| Intrinsic metadata adapters | `intrinsic_carriers` | BIR intrinsic family/operation/feature/operand-role/memory/side-effect facts | Carrier completeness, required-feature policy, prepared call plans, operand/result homes, helper protocols, and missing-fact diagnostics |
| Row-scoped diagnostics/oracles | `invariants`, `notes`, selected `f128_runtime_helpers`, selected `call_plans`, selected `store_source_publications` | Route-native or BIR-native row facts with byte-stable printer/oracle output | Prepared fallback for absent, invalid, duplicate/conflict, mismatch, unsupported, target-policy-sensitive, and compatibility-sensitive paths |

The fields classified as retained target/prepared policy are deliberately not
reclassified as BIR-owned: `target_profile`, `register_group_overrides`,
`regalloc`, `stack_layout`, `addressing`, `frame_plan`,
`dynamic_stack_plan`, `variadic_entry_plans`, `storage_plans`,
`i128_carriers`, `f128_carriers`, `atomic_operations`,
`inline_asm_carriers`, and the policy portions of `value_locations`,
`call_plans`, `intrinsic_carriers`, `f128_runtime_helpers`, and
`i128_runtime_helpers`.

### PreparedFunctionLookups Group Inventory

`PreparedFunctionLookups` currently groups seven per-function lookup families:

| Lookup group | Raw surface captured for later classification |
| --- | --- |
| `call_plans` | Call-plan lookups. Route 6 can replace selected source identity reads, but ABI placement, helper/carrier protocol, result lanes, call records, and call-boundary oracles stay retained. |
| `address_materializations` | Address-materialization lookups. Current evidence classifies this as target/prepared policy: frame/stack/global/TLS relocation, offsets, base-plus-offset legality, volatile/address-space behavior, and final operands are not route facts. |
| `memory_accesses` | Memory access lookups. Route 3 may own selected memory/source identity, but prepared memory/addressing fallback and target policy remain retained. |
| `move_bundles` | Move-bundle lookups. Route 5/6 may provide adjacent source identity, but scheduling, homes, storage, cycle temporaries, ABI move phases, and final move records stay prepared/target-owned. |
| `value_homes` | Value-home lookups. Evidence retains prepared/target ownership for storage, register choice, stack slot/offset, rematerialization spelling, rendering, and home policy. |
| `edge_publications` | Edge-publication lookups. Routes 4/5 may own selected publication or edge/join identity, while publication construction, block order, moves/homes/storage, stack-source extension, wrappers, and final records remain retained. |
| `edge_publication_source_producers` | Source-producer lookups. Route 1, 2, 5, 6, and 7 subfacts may be semantic candidates one consumer at a time; printer/debug, route-debug, helper-oracle, materialization, call/publication routing, and final records remain retained. |

The only construction entry points in the header are
`make_prepared_function_lookups(...)` and
`make_prepared_move_bundle_lookups(...)`. This supports the current
classification of the aggregate as compatibility/pass-context delivery rather
than a deletion-ready owner boundary.

## PreparedFunctionLookups Ownership Classification

This Step 3 map classifies all seven `PreparedFunctionLookups` groups. A
route-fact duplicate candidate below is code-size-positive only for the named
semantic subfact, selected consumer, or diagnostic row. It is not permission to
delete the lookup group, build a BIR-owned clone of the aggregate, privatize
public prepared fallback/oracle surfaces, or move target policy into BIR.

| Lookup group | Classification | Route-fact duplicate candidates | Retained target/prepared policy and fallback/oracle authority | Diagnostic/string and cross-target compatibility boundary | Realistic E2 public prepared API/private pass-context contraction candidate | Proof required before deletion or contraction |
| --- | --- | --- | --- | --- | --- | --- |
| `call_plans` | Retained target/prepared policy with selected Route 6 semantic subfact candidates | One call argument/result source identity under route/prepared agreement; the x86 scalar `ConsumedPlans` row is evidence for a narrow compatibility boundary only. | ABI register/stack placement, call wrapper kind, clobbers, outgoing stack sizing, byval/variadic lanes, helper/carrier protocols, result lanes, publication routing, aggregate transport, final call records, storage, movement, and emitted behavior stay prepared/target-owned. | Prepared call-plan summaries, call-boundary helper oracles, x86 route-debug compatibility rows, AArch64 call oracles, and wrapper output remain retained. x86 keeps `ConsumedPlans` as compatibility context; riscv has no broad proof. | A single Route 6 call-use source adapter or private pass-context handle for one argument/result reader. Public call-plan lookup helpers can contract only when the selected reader no longer needs public prepared delivery and all fallback/oracle rows remain available elsewhere. | Prove positive, absent, invalid, ambiguous/conflict, mismatch, ABI-bound fallback, and unchanged call-policy/output cases for the one reader; no whole `call_plans` deletion, privatization, or wrapper migration is supported. |
| `address_materializations` | Retained target/prepared policy | None currently identified at the group level. A later semantic address/source audit would need to exclude address policy explicitly before claiming any route fact. | Frame/stack/global/TLS relocation, offsets, pointer materialization, base-plus-offset legality, volatile/address-space handling, final memory operands, and address materialization sequencing remain prepared/target-owned. | Prepared address-materialization rows, frame-address offset helpers, address tests, lookup-helper oracles, and target wrapper behavior remain retained. Cross-target callers must keep target-local addressing policy. | No E2 contraction candidate beyond local private threading cleanup if it preserves public fallback and does not remove address policy APIs. | Not deletion-ready. Any future candidate must name one semantic consumer, prove it does not consume address policy, and preserve prepared fallback and byte-stable address diagnostics. |
| `memory_accesses` | Mixed: Route 3 memory/source semantic candidates plus retained prepared/target policy | Selected memory access id and source value/global/local identity for one reader under route/prepared agreement. | Address formation, frame/global/TLS relocation, offsets, address spaces, volatile handling, materialization policy, final operands, source-home policy, and prepared mismatch/fallback behavior remain prepared/target-owned. | Prepared memory lookup helpers, load-local/global-source fallback, memory/frame/stack oracles, prepared lookup-helper rows, and target-wrapper guards remain retained. x86/riscv reuse waits for AArch64 route-backed proof. | One AArch64 Route 3 memory/source reader adapter or private pass-context source-id delivery path. Public memory-access lookup APIs may contract only for that selected semantic read, not for address/materialization policy. | Prove memory access id, source identity, block/instruction compatibility, absent/invalid/ambiguous/mismatch fallback, and unchanged addressing/materialization output for the one consumer. |
| `move_bundles` | Retained target/prepared policy and transient pass context | Only adjacent source-identity questions from Route 5 or Route 6 are plausible future audits; no current move-bundle route replacement is ready. | Move ordering, out-of-SSA placement, cycle temporaries, source/destination homes, storage, ABI move phases, execution sites, final move records, and emitted behavior stay prepared/target-owned. | Move-bundle oracle tests, edge-copy/move-policy diagnostics, wrapper paths, and helper rows remain retained. Cross-target wrappers keep prepared move policy. | Potentially a private pass-context source-id adapter for one consumer that currently receives move lookup context but only needs semantic identity. Move-bundle lookup APIs themselves remain retained. | Prove the selected consumer ignores move policy and preserves prepared fallback, move ordering, target rendering, and oracle strings. No group deletion or broad privatization is supported. |
| `value_homes` | Retained target/prepared policy | A future one-reader value/source identity audit may be possible, but no route-native contraction boundary is ready from current evidence. | Storage authority, register choice, stack slot/offset, rematerialization spelling, operand rendering, decoded-home behavior, formal-publication inputs, and value-home policy remain prepared/target-owned. | Value-home oracles, prepared-memory-operand rows, target-wrapper tests, riscv edge-publication operand rendering, and lookup-helper coverage remain retained. | Only local private pass-context threading cleanup is realistic if a reader is proven to need identity only. Public value-home lookup surfaces remain required for policy and compatibility. | Prove route/prepared agreement for the identity-only reader, prepared fallback for all failure modes, and unchanged home/storage/rendering behavior. Whole `value_homes` contraction is blocked. |
| `edge_publications` | Mixed: Route 4 publication identity and Route 5 edge/join-source candidates plus retained publication policy | One block-entry/current-block publication identity or edge/join-source identity for one consumer or wrapper boundary after exact AArch64 proof. | Publication record construction, block-order emission, edge-copy mechanics, move/home/storage policy, stack-source extension, immediate/output spelling, final edge-copy records, and wrapper formatting remain prepared/target-owned. | Publication printer/debug rows, wrapper tests, prepared publication oracles, x86 prepared dispatch, and riscv prepared emission remain retained. x86/riscv adapters require compatibility proof, not route-wide migration. | One edge-publication semantic adapter for an AArch64 reader or a single x86/riscv wrapper input. Public indexed/unique publication helpers may contract only for the selected semantic role with prepared fallback preserved. | Prove predecessor/successor or block-label agreement, destination/source agreement, duplicate/ambiguous/conflict rejection, mismatch fallback, and unchanged move/register/stack/output policy. No group deletion is ready. |
| `edge_publication_source_producers` | Mixed route-native semantic candidates plus retained production fallback and printer/debug oracle | Route 1 producer/constant, Route 2 select-chain/direct-global, Route 5 join-source, Route 6 call-use source, and Route 7 comparison provenance can each be candidates for one semantic lookup family or one printer/debug row. | Materialization sequence choice, direct-global policy when coupled to call/memory/publication contexts, storage/home/move policy, call/publication routing, publication mechanics, final records, and prepared fallback remain retained. | Prepared printer `select_chains`, source-producer helper tests, route-debug rows, helper-oracle strings, x86/riscv wrapper-adjacent paths, and lookup-helper fallback coverage remain retained. | Best E2 candidate: one route-native source-producer helper or printer/oracle row for a single route family. Public source-producer lookup APIs can contract only after that row has byte-stable route-native diagnostics and retained fallback. | Prove route/prepared agreement plus absent, invalid, ambiguous/conflict, mismatch, policy-sensitive fallback, and unchanged printer/debug/oracle strings for the one route family. Aggregate source-producer or lookup-group deletion is blocked. |

### PreparedFunctionLookups E2 Candidate Summary

The realistic code-size-positive E2 work is public prepared API contraction into
private pass-context or one-reader adapters, not aggregate removal:

| Candidate | Lookup groups involved | Replacement authority | Retained surfaces |
| --- | --- | --- | --- |
| Route 6 call-use source adapter | `call_plans` | Route 6 call argument/result source identity under route/prepared agreement | Call ABI/layout policy, helper/carrier protocols, call records, x86 `ConsumedPlans`, wrappers, printer/debug, and call oracles |
| Route 3 memory/source adapter | `memory_accesses` | Route 3 memory access/source identity for one AArch64 reader | Address formation, materialization, relocation, value homes, target wrappers, memory/address diagnostics, and prepared fallback |
| Route 4/5 publication or edge/join-source adapter | `edge_publications`, selected `edge_publication_source_producers` | Route 4 publication identity or Route 5 edge/join-source identity for one consumer or wrapper input | Publication construction, block order, move/home/storage policy, stack-source extension, wrappers, printer/debug, and publication oracles |
| Route 1/2/5/6/7 source-producer diagnostic row | `edge_publication_source_producers` | One route-native producer/select-chain/join/call/comparison provenance fact | Prepared printer `select_chains`, route-debug/helper-oracle strings, materialization/call/publication policy, wrappers, and fallback |
| Identity-only audit candidates | `move_bundles`, `value_homes` | A future proven source/value identity question only | Move scheduling, home/storage policy, target rendering, decoded-home/formal-publication behavior, wrappers, and oracles |

`address_materializations` has no current E2 contraction candidate. The
aggregate construction APIs, `make_prepared_function_lookups(...)` and
`make_prepared_move_bundle_lookups(...)`, remain compatibility/pass-context
delivery boundaries until all public production, wrapper, printer/debug, and
oracle consumers for a named sub-surface have an equivalent route-native or
retained-owner path.

## E1-E5 Readiness Synthesis

This Step 4 synthesis converts the field and lookup maps into a conditional
later-phase path. It does not claim that `PreparedBirModule`,
`PreparedFunctionLookups`, or their aggregate construction/delivery APIs are
deletion-ready. The only currently realistic code-size-positive targets are
named duplicate semantic helper/API families, one selected consumer or
diagnostic row at a time, with prepared policy, fallback, oracle, and
cross-target compatibility preserved until replacement proof exists.

### Target End-State Map

| End-state owner | Surfaces that can move or contract | Surfaces that must stay retained | Readiness recommendation |
| --- | --- | --- | --- |
| BIR owns semantics | Target-neutral module/function/block/value identity; CFG facts; route source, publication, join, call-use, and comparison provenance annotations; liveness/use-def annotations; intrinsic metadata facts; future Route 8 return-chain schema if designed as its own owner line. | ABI/layout/register/stack/addressing/move/call/helper/emission policy; wrapper behavior; prepared fallback and printer/oracle strings. | E1 should add or reuse BIR/route facts only where a selected reader can prove route/prepared agreement and failure-mode fallback. Do not build a BIR-owned clone of prepared policy containers. |
| Prepared owns policy | `target_profile`, register overrides, regalloc, frame and dynamic stack plans, address materialization, value-home storage policy, move scheduling, call ABI, carrier/helper protocols, atomic/inline-asm lowering, final records, and target output spelling. | Narrow source identity or metadata subfacts may be read from BIR after proof, but the policy decision remains prepared/target-owned. | Later phases should keep policy APIs public or pass-context-owned until a separate target-policy owner exists. Moving these fields into BIR would be route drift. |
| Prepared APIs become private pass context | One-reader adapters for Route 3 memory/source, Route 4/5 publication or edge/join identity, Route 6 call-use source, Route 1/2/5/6/7 source-producer rows, and identity-only audits through `move_bundles` or `value_homes`. | Aggregate lookup construction, public fallback/oracle paths, wrapper inputs, diagnostic rows, and cross-target compatibility handles. | E2 can contract only a named public prepared lookup/helper after all direct public consumers of that selected semantic read move behind a private pass-context or route-native adapter. |
| Duplicate semantic helpers are deletion candidates | Helpers that only rediscover BIR identity, selected route source/publication/join/call/comparison facts, liveness identity, intrinsic metadata, or one byte-stable diagnostic/oracle row already reproduced from route-native facts. | Any helper carrying target policy, expected-string authority, fallback decisions, or wrapper compatibility. | Deletion is conditional on replacing every semantic consumer in the helper family, preserving prepared fallback for non-agreement paths, and proving byte-stable diagnostics where the helper had oracle authority. |

### E1 Semantic Duplicate Candidates

E1 should focus on semantic duplicate candidates where the existing maps name a
clear replacement fact and a retained owner for everything else:

| Candidate | Replacement semantic authority | Retained boundary |
| --- | --- | --- |
| Aggregate BIR semantic forwarding | Direct `bir::Module` graph, stable BIR function/block/value/link identity, and route annotations. | `PreparedBirModule` wrapper delivery, target-profile threading, prepared printers/dumps, and fallback/oracle calls. |
| Control-flow identity helpers | BIR function/block graph, terminator successors, block labels, route edge/join annotations, and any future separate return-chain schema. | Edge-copy scheduling, branch policy, block-order emission, wrappers, route-debug rows, and expected strings. |
| Route source/publication identity helpers | Route 1-7 source, publication, join, call-use, and comparison provenance annotations under route/prepared agreement. | Addressing, value homes, storage, move scheduling, call ABI, publication mechanics, wrappers, printer/debug rows, and helper oracles. |
| Liveness identity helpers | BIR use/def, live interval, address-taken, and requires-home annotations. | Prepared pass ordering, stack-object association, allocation-policy consumers, and diagnostics. |
| Intrinsic metadata adapters | BIR intrinsic family, operation, feature, operand-role, memory, and side-effect facts. | Carrier completeness, required-feature policy, call plans, operand/result homes, helper protocols, and missing-fact diagnostics. |
| Row-scoped diagnostic/oracle helpers | Route-native or BIR-native facts for one positive row. | Prepared fallback for absent, invalid, duplicate/conflict, mismatch, unsupported, target-policy-sensitive, and compatibility-sensitive paths. |

### E2 Public API And Private Pass-Context Candidates

E2 should be scoped as public prepared API contraction into private pass context,
not aggregate API removal:

| Candidate | API family or lookup group | Required E2 shape |
| --- | --- | --- |
| Route 6 call-use source adapter | Selected `call_plans` source lookup/helper path. | One argument/result source reader moves to route-native or private pass-context delivery while call ABI/layout policy, `ConsumedPlans`, wrappers, printer/debug, and call oracles remain retained. |
| Route 3 memory/source adapter | Selected `memory_accesses` lookup/helper path. | One AArch64 memory/source identity consumer moves behind a route-native adapter while address formation, materialization, relocation, value homes, target wrappers, and fallback diagnostics remain prepared-owned. |
| Route 4/5 publication or edge/join adapter | Selected `edge_publications` and `edge_publication_source_producers` paths. | One publication, edge, or join identity reader moves to route-native/private delivery while publication construction, move/home/storage policy, block-order output, wrappers, and oracles remain retained. |
| Route 1/2/5/6/7 source-producer row | Selected source-producer helper, route-debug, printer, or helper-oracle row. | One route family gets byte-stable route-native diagnostics with prepared fallback retained for all non-agreement cases. |
| Identity-only audit adapter | Selected `move_bundles` or `value_homes` consumer proven not to need policy. | Only the identity subquestion contracts; move scheduling, storage/home policy, target rendering, decoded-home/formal-publication behavior, wrappers, and oracles remain retained. |

`make_prepared_function_lookups(...)`,
`make_prepared_move_bundle_lookups(...)`, and aggregate
`PreparedBirModule` construction are not E2 candidates until every public
production, wrapper, printer/debug, and oracle consumer for a named sub-surface
has either moved to a route-native/private context or been assigned an explicit
retained owner.

### Code-Size Reduction Candidates And Required Proof

Realistic code-size reduction is limited to helper/API families where the code
being deleted stops duplicating semantic authority rather than merely renaming
or rewrapping the same prepared state:

| Helper/API family | Why reduction is realistic | Required proof before deletion |
| --- | --- | --- |
| Selected route source/publication/join/call/comparison identity helpers | Route 1-7 evidence already has agreement-gated semantic readers for narrow facts. | Positive route/prepared agreement, absent/invalid/ambiguous-or-conflict/mismatch fallback, unchanged policy-sensitive output, and no public fallback/oracle consumer left behind for that helper family. |
| BIR identity and control-flow forwarding helpers | BIR already owns function/block/value identity and CFG shape. | All consumers in the helper family read BIR identity directly, printer/debug labels remain byte-stable, and branch/edge-copy/output policy stays prepared-owned. |
| Liveness identity helper adapters | Liveness facts can become BIR annotations where they are pure use/def/live-interval questions. | Equivalent annotation reads, preserved stack-object ambiguity diagnostics, and no allocation-policy behavior removed. |
| Intrinsic metadata adapters | Some intrinsic family/operation/feature facts are semantic BIR facts. | Metadata consumers read BIR facts directly while carrier completeness, feature policy, helper calls, operand/result homes, and missing-fact diagnostics remain prepared-owned. |
| Row-scoped diagnostic/oracle helpers | Existing route rows show that one byte-stable row can move without broad retirement. | Byte-stable positive row output plus retained prepared authority for every failure, fallback, unsupported, and target-policy-sensitive path. |

### Surfaces Where Code-Size Reduction Is Not Realistic Yet

These surfaces should not be counted as code-size-positive Phase E progress:

- Aggregate `PreparedBirModule` or `PreparedFunctionLookups` deletion,
  construction removal, or facade renaming.
- Target policy fields and lookup groups: `target_profile`,
  `register_group_overrides`, `regalloc`, `stack_layout`, `addressing`,
  `frame_plan`, `dynamic_stack_plan`, `address_materializations`,
  `value_homes` policy, move scheduling, call ABI, carriers, atomics,
  inline asm, runtime helpers, final records, and emitted output.
- Public fallback/oracle and diagnostic/string authority: `invariants`,
  `notes`, prepared printer/debug rows, route-debug rows, helper-oracle names,
  baselines, and expected strings.
- Cross-target wrappers and compatibility handles, including x86
  `ConsumedPlans`, riscv wrapper-boundary behavior, target-local formatting,
  frame/register/ABI rendering, and wrapper input/output compatibility.
- Route 8 return-chain contraction, unless a separate return-chain owner/schema
  is designed and proven; it must not be hidden inside Route 1, Route 7,
  predecessor rescans, name matching, or a generic route-index facade.

### E3 Diagnostic And Oracle Prerequisites

E3 can start only after a named diagnostic or oracle row has:

- A route-native or BIR-native source fact for the positive row.
- Explicit prepared fallback for absent, invalid, duplicate/conflict,
  ambiguous, mismatch, unsupported, target-policy-sensitive, and
  compatibility-sensitive cases.
- Byte-stable printer/debug, route-debug, helper-oracle, baseline, and
  expected-string output, or an explicit retained prepared authority for that
  output.
- No weakening of supported-path status, expected strings, or helper-oracle
  coverage.
- Target-policy separation: the replacement row must not infer register,
  stack, ABI, address, move, branch, helper, or final-emission decisions from
  target-neutral BIR semantics.

### E4 Cross-Target Wrapper Blockers

E4 cross-target work is blocked until each wrapper boundary names both the
route-native semantic input and the retained target-local owner:

- AArch64 remains the primary proof target; selected-reader proof there is
  required before importing a similar boundary to x86 or riscv.
- x86 currently has only narrow Route 6 scalar source evidence through
  `ConsumedPlans` and one route-debug row. `ConsumedPlans`, call wrapper
  compatibility, route-debug strings, frame/register/ABI formatting, and
  instruction-selection/emission behavior remain blockers.
- riscv has no imported route-view migration proof; edge-publication and
  wrapper-boundary output remain prepared/target-owned until AArch64 evidence
  plus riscv-specific formatting/emission proof exists.
- Wrapper candidates must preserve target-local address formation, value homes,
  move bundles, register/stack policy, call ABI, publication mechanics,
  branch/fused-compare policy, helper protocols, final records, and expected
  strings.

### Draft 155 And E5 Rewrite Criteria

Draft 155 / E5 should remain unopened as an execution plan and unrevised as a
retirement plan until all of these criteria are met:

1. Field ownership is complete for every `PreparedBirModule` field, with target
   policy, fallback/oracle, diagnostics, wrappers, and compatibility surfaces
   explicitly retained or assigned to a concrete replacement owner.
2. Lookup-group ownership is complete for every `PreparedFunctionLookups`
   group, with aggregate construction/delivery separated from one-reader
   semantic adapters and diagnostic rows.
3. E1 has proven the selected semantic duplicate families through
   route/prepared agreement and failure-mode fallback without moving target
   policy into BIR.
4. E2 has contracted only named public prepared APIs into private pass context
   after every public consumer, wrapper, printer/debug row, and oracle for that
   selected surface has a replacement or retained-owner path.
5. E3 has byte-stable diagnostic/oracle replacements or explicit retained
   prepared authority for positive and failure paths.
6. E4 has cross-target wrapper proof for AArch64-first boundaries and separate
   x86/riscv compatibility proof where those targets participate.
7. The proposed E5 work can name concrete helper/API deletions and expected net
   code removal. Facade moves, container reshuffles, wrapper renames, and
   aggregate type demotions do not qualify.

Until those criteria are satisfied, draft 155 remains evidence for retirement
intent and reject signals only. E5 must not claim broad prepared retirement,
aggregate `PreparedBirModule` deletion, aggregate `PreparedFunctionLookups`
deletion, target-policy migration into BIR, or diagnostic/oracle weakening.

### Phase D, D2, And Route 1-8 Constraints

Phase D constraints:

- Selected consumer migrations prove bounded route-view use, not route-wide
  migration, prepared API deletion, or `PreparedBirModule` retirement.
- Prepared answers remain public fallback/oracle surfaces until route/prepared
  equivalence and replacement diagnostics are proven.
- Target/layout/codegen policy stays outside BIR: homes, registers, stack
  slots, ABI placement, move scheduling, branch spelling, fused legality,
  emitted storage, and final instruction order are not semantic route facts.
- Route index/facade work is not a generic `PreparedFunctionLookups` clone.
- Return-chain is separate owner/schema evidence and must not be folded into
  Route 1 producer identity, Route 7 comparison provenance, predecessor
  rescans, name matching, or a generic route-index facade.

Phase D2 constraints:

- No C2/D2 retained surface is broad contraction-ready.
- Future implementation work must be one prepared surface or one
  diagnostic/oracle row wide.
- Aggregate `PreparedFunctionLookups`, aggregate `PreparedBirModule`, AArch64
  lookup threading/pass-context, broad x86/riscv wrapper work, baseline/string
  authority, and Route 8 contraction are no-action as broad targets.
- Accepted follow-up categories are semantic reader migration or row-scoped
  diagnostic/oracle replacement with retained prepared fallback and
  positive/absent/invalid/duplicate-or-conflict/mismatch/fallback proof.

Route constraints:

| Route | Constraint inventory |
| --- | --- |
| Route 1 producer/constant | One selected publication producer read moved earlier, but scalar materialization, producer/constant, ALU, memory/call operand, publication, printer, and oracle consumers remain. Homes, storage, registers, rematerialization, move records, and final records stay retained. |
| Route 2 select-chain/direct-global | One select-chain publication path moved earlier, but direct-global/select-chain helpers remain fallback/oracle for publication, call, memory, ALU, FP, producer, edge-copy, and diagnostics. Materialization sequence and context policy stay retained. |
| Route 3 memory/source | Selected memory/source identity readers and one oracle row are proven only under route/prepared agreement. Address formation, frame/global/TLS relocation, offsets, addressing legality, source-home policy, materialization, final operands, x86 wrapper behavior, and target policy stay retained. |
| Route 4 publication | Selected publication-source and block-entry row evidence is agreement-gated. Publication mechanics, move/home/storage policy, immediate/output spelling, wrapper compatibility, block-order emission, printer/debug, and oracle surfaces remain retained. |
| Route 5 edge/join-source | Selected join-source reader and one oracle row are agreement-gated. Parallel-copy scheduling, source/destination homes, move bundles, execution sites, cycle temporaries, branch policy, wrappers, final edge-copy records, and emitted output remain retained. |
| Route 6 call-use source | Selected AArch64 and one x86 scalar route-debug source boundary are agreement-gated. ABI placement, wrapper kind, clobbers, outgoing stack sizing, byval lanes, variadic counts, helper/carrier protocols, value homes, move bundles, aggregate transport, final call records, storage, movement, printer/debug, and emitted output remain retained. |
| Route 7 comparison/condition | Selected comparison provenance consumer and one oracle row are agreement-gated. Branch targets, suffix mapping, fused legality, hazards, emitted-register state, final order, final assembler rows, branch policy, printer/debug, wrappers, and expected strings remain retained. |
| Route 8 return-chain | Importable only as its own owner/schema line. It owns target-neutral same-block return-chain identity, while return ABI, homes, registers, scratch policy, ALU records, final return emission order, helper/oracle expected strings, and target policy remain outside Route 8. |

### Consumer-Boundary Evidence

| Target | Evidence inventory |
| --- | --- |
| AArch64 | AArch64 remains the primary selected-reader proof target. It can thread narrow route views beside prepared context for selected semantic readers, but `PreparedBirModule`, `PreparedFunctionLookups`, call-plan, address-materialization, move-bundle, and value-home lookup handles remain production/pass-context delivery surfaces. AArch64 also retains target ownership for ABI, frame/stack layout, address formation, value homes, move planning, branch/fused-compare policy, helper protocols, final records, diagnostics, and expected strings. |
| x86 | Proven reuse is limited to Route 6 scalar source agreement through `ConsumedPlans` and one route-debug row. `ConsumedPlans` remains the compatibility boundary; broad x86 call wrapper, memory/publication wrapper, route-debug, frame/register/ABI formatting, instruction-selection, and emission migration are not proven. Prepared fallback remains required for absent, invalid, duplicate/conflict, mismatched, and compatibility-sensitive cases. |
| riscv | Current evidence is wrapper-boundary and no-change evidence only, especially around prepared edge-publication output. No imported riscv route-view migration is proven. Any riscv migration must wait for an AArch64-proven semantic route view or retained-owner boundary and must preserve riscv target-local frame/register/ABI/formatting/emission policy. |

### Missing Or Ambiguous Evidence

- No required Step 1 source was missing.
- Draft 155 supplies retirement intent and reject signals, not executable
  ownership decisions. The Step 2 through Step 4 maps classify fields, lookup
  groups, wrappers, diagnostics, compatibility adapters, proof criteria, and
  blockers as readiness analysis only; final validation must still confirm
  every expected output and reject signal before any retirement or demotion
  recommendation can be treated as ready.
- The field, lookup-group, and E1-E5 readiness maps are now analysis outputs,
  not implementation permission. Final validation still needs to check this
  document against every source-idea expected output and reject signal before
  any later execution plan treats the recommendations as acceptance-ready.
