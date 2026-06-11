Status: Active
Source Idea Path: ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify ownership and readiness

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: classified ownership/readiness for every
`PreparedFunctionLookups` lookup group from the Step 1 inventory without
renaming, splitting, deleting, privatizing, or weakening oracle coverage.

Classification table:

| Lookup group | Classification | Semantic route facts vs target/prepared policy | Partial route-view replacements | Fallback/oracle surfaces keeping it public | Residual reader or missing evidence blocking deletion/privatization |
| --- | --- | --- | --- | --- | --- |
| `call_plans` | Target/prepared policy; compatibility adapter; retained oracle. Route-native only for narrow Route 6 semantic source subfacts. | Route 6 may own argument/result source identity and direct-global/source-producer provenance for one role class at a time. ABI register/stack placement, variadic FPR counts, byval lanes, clobber/preserve facts, outgoing stack sizing, late publication, helper/carrier protocols, and call-record spelling stay prepared/target-owned. | Ideas 187 and 189 prove selected Route 6 reads: one AArch64 direct-global select-chain call-argument materialization consumer and one x86 scalar argument-source interface reuse point. | `PreparedCallPlanLookups`, `find_indexed_prepared_call_plan(...)`, call argument/result plan selectors, immediate-argument and preserved-value helpers, block-entry republication effects, prepared call-boundary/frame-stack tests, and x86 `ConsumedPlans::shared_call_plan_lookups()` remain public fallback/oracle surfaces. | AArch64 `calls.cpp` still consumes projected call-plan lookups for production lowering. x86 `ConsumedPlans` stores the aggregate and exposes `call_plans` through a compatibility adapter. Oracle tests attach `&prepared_lookups.call_plans`. No evidence exists that whole call plans, ABI placement, result lanes, or call-boundary policy can move to BIR. |
| `address_materializations` | Target/prepared policy; retained oracle. Not ready for BIR annotation ownership. | Route 3 may own semantic memory/source identity only. Address base kind, offsets, frame/stack objects, TLS/global relocation choice, pointer materialization, base-plus-offset legality, volatile/address-space behavior, and final memory operands stay prepared/AArch64-owned or target-owned. | Route 3 selected work proves a semantic stored-source/memory identity boundary, not address-materialization policy. No partial replacement for frame-address offset or target address materialization policy was found. | `PreparedAddressMaterializationLookups`, indexed address-materialization helpers, frame-address-offset helpers, address-materialization tests, memory/addressing oracle tests, and AArch64 prepared-memory-operand tests remain prepared surfaces. | AArch64 `globals.cpp`, `memory.cpp`, and `memory_store_retargeting.*` still read projected address-materialization lookups. Tests attach `&prepared_lookups.address_materializations`. Deletion is blocked by target-addressing policy ownership and lack of a route-native address-policy replacement, which the readiness docs explicitly reject as BIR-owned. |
| `memory_accesses` | BIR annotation candidate for semantic access identity; transient pass context and retained oracle today; blocked/unknown for aggregate contraction. | Semantic memory/access identity, source value/global identity, access kind, and block provenance can move toward Route 3. Target address formation, load/store operand selection, frame/TLS/global relocation, volatile/address-space behavior, and materialization policy remain prepared/target-owned. | Idea 186 proves one AArch64 indirect-callee stored-value source consumer; idea 193 hardens the Route 3 prepared-fallback and target-policy boundary. The residual map names future single-reader Route 3 memory/source candidates. | `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`, global-load/same-block global-load/load-local source helpers, store-source publication helper inputs, `backend_prepared_lookup_helper`, store-source publication tests, and memory/frame/stack oracles remain public. | AArch64 `alu.cpp` still directly reads `context.function.prepared_lookups->memory_accesses` for return-chain/value-home fallback paths, and publication-plans helpers still take memory-access lookup inputs. Route 3 does not yet cover every memory/source reader or policy-sensitive fallback, so privatization is blocked by direct production readers and missing per-consumer equivalence evidence. |
| `move_bundles` | Target/prepared policy; transient pass context; retained oracle. Not a BIR annotation owner. | Route 5 can own edge/join-source identity; Route 6 can own call-use source identity. Move ordering, before-call argument moves, before-return ABI moves, after-call result lane bindings, out-of-SSA placement, cycle temporaries, execution sites, homes, storage, and final move records remain prepared/target-owned. | Route 5 current-block join-source and Route 6 call-use migrations touch adjacent semantic questions, but no route view replaces move-bundle policy. | `PreparedMoveBundleLookups`, indexed move-bundle helpers, call-argument move helpers, return-ABI move helpers, after-call result lane helpers, AArch64 branch/current-block/dispatch tests, and prepared lookup helper tests remain public. | AArch64 `memory.cpp`, `calls.cpp`, `alu.cpp`, and `module.cpp` still consume projected move-bundle lookups. Tests attach `&prepared_lookups.move_bundles` and use empty/mutated move lookups for negative oracle coverage. Deletion is blocked because move scheduling and ABI move policy remain target/prepared-owned, not route facts. |
| `value_homes` | Target/prepared policy; transient pass context; retained oracle. Route-owned status is blocked except as compatibility metadata. | Route facts may reference source/destination values, but storage/home ownership, register choice, stack slot/offset, rematerialization spelling, formal publication inputs, and decoded-home/rendering policy remain prepared/target-owned. | Routes 1, 4, 5, and 6 use semantic value identities around producer/publication/edge/call facts, but no route view replaces home/storage policy. Existing x86 decoded-home adapter use passes `value_home_lookups = nullptr`, so it is not deletion evidence. | `PreparedValueHomeLookups`, indexed value-home/id helpers, decoded-home/formal-publication inputs, AArch64 operand/prologue/dispatch/publication/call/memory consumers, BIR/prealloc value-home oracles, and prepared-memory-operand tests remain public. | Many AArch64 production files still consume projected `context.function.value_home_lookups`, and tests attach or pass `lookups.value_homes`. Privatization is blocked by production home/storage policy readers and by oracle coverage that still checks prepared home answers. |
| `edge_publications` | BIR annotation candidate for publication/edge semantic identity; compatibility adapter; retained oracle; target/prepared policy for publication mechanics. | Route 4 may own current/block-entry publication identity and Route 5 may own edge/join-source identity. Publication record construction, block-order emission, move/homes/storage policy, stack-source extension, and final edge-copy records remain prepared/target-owned. | Ideas 182 and 183 prove selected Route 4 and Route 5 readers. x86/riscv edge-publication wrapper reuse is only future-candidate status from the cross-target map; riscv has no ready route-view reuse. | `PreparedEdgePublicationLookups`, indexed/unique edge-publication helpers, edge-copy source facts, block-entry parallel-copy helpers, stack-source and publication-match helpers, AArch64 dispatch/memory/publication fallbacks, x86 prepared dispatch, riscv prepared emission, wrapper tests, and prepared lookup helper tests remain public. | AArch64 `dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, and `memory.cpp` directly read `context.function.prepared_lookups->edge_publications`. x86 and riscv wrappers still consume edge publications through prepared aggregate compatibility paths. Deletion is blocked by direct production readers, target-wrapper readers, and missing route-native wrapper evidence. |
| `edge_publication_source_producers` | BIR annotation candidate for source-producer/select-chain semantic facts; target/prepared policy for materialization/call/publication use; retained oracle and printer/debug surface. | Routes 1, 2, 5, 6, and 7 may own narrow semantic producer, constant, select-chain, join-source, call-use, and comparison provenance facts. Materialization sequence choice, direct-global policy where coupled to call/memory/publication contexts, storage/home/move policy, call/publication routing policy, and final records remain prepared/target-owned. | Ideas 184, 185, 187, and 188 prove selected source-producer, select-chain, call-use, and comparison consumers; Route 5 covers one join-source reader. These are partial route-first replacements with prepared fallback, not route-wide replacement of the source-producer lookup group. | Source-producer helpers for same-block scalar producers, current-block publication consumption, select-chain/direct-global dependencies, scalar materialization, store-source, comparison, call/publication helpers, prepared printer select-chain output, dispatch tests, lookup helper tests, store-source publication tests, prepared-memory-operand tests, and frame/stack/call contract tests remain public. | AArch64 `dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, `memory.cpp`, `calls.cpp`, `comparison.cpp`, and `alu.cpp` directly read `context.function.prepared_lookups->edge_publication_source_producers` or local fallback lookups. `prepared_printer/select_chains.cpp` is a printer/debug reader. Deletion is blocked by broad residual production use, retained printer/debug oracle status, and missing route-native diagnostics/oracles. |

Readiness summary:

- No lookup group is ready for deletion or privatization from Step 2 evidence.
- `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` contain semantic facts that can move
  toward BIR annotation ownership one selected consumer at a time, but each
  still has direct AArch64 aggregate readers and prepared fallback/oracle
  surfaces.
- `call_plans` has a narrow Route 6 semantic source path and x86 compatibility
  reuse point, but whole call-plan ownership remains prepared/target policy.
- `address_materializations`, `move_bundles`, and `value_homes` are primarily
  target/prepared policy or transient pass-context groups with retained oracle
  coverage. Route views may cite identities around them, but Step 2 found no
  evidence that their policy payloads should become BIR-owned.
- x86/riscv aggregate use is compatibility-adapter or target-wrapper evidence,
  not proof of prepared aggregate retirement. x86 has only the selected Route 6
  scalar source reuse; riscv edge-publication consumption remains prepared.

## Suggested Next

Execute Step 3 by converting this classification into concrete prerequisites:
for each lookup group, name the smallest adapter boundary, retained-ownership
reason, replacement diagnostic/oracle requirement, or missing-evidence
follow-up needed before future aggregate contraction work.

## Watchouts

- Do not rename, split, delete, or privatize `PreparedFunctionLookups` fields
  under this audit.
- Do not classify target/prepared policy as BIR-owned just because a selected
  route consumer moved. Route facts are semantic identities; ABI, addressing,
  homes, storage, moves, emission records, and wrapper policy remain
  target/prepared-owned unless a separate implementation idea proves otherwise.
- Do not weaken oracle coverage, tests, unsupported markers, or expectations to
  make a lookup group appear ready.
- `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` still have direct AArch64 aggregate
  readers; aggregate contraction cannot treat the projected-pointer fields as
  representative of the whole type.
- `edge_publication_source_producers` is also a printer/debug input; Step 2
  retained it separately from production fallback needs.
- x86 and RISC-V are compatibility-adapter readers of the aggregate, not proof
  that all aggregate fields have cross-target semantic route ownership.
- Treat every compatibility adapter as transitional. Adapter presence keeps the
  prepared surface public until the route-native source and equivalent oracle
  coverage exist.

## Proof

Docs/lifecycle-only classification packet. No build/test proof was required and no
`test_after.log` was generated. Validation/searches performed:

- Read active `plan.md`, source idea 196, and the Step 1 inventory in
  `todo.md`.
- Read readiness and migration artifacts:
  `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`,
  `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`,
  and
  `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`.
- Ran targeted `rg` scans for `PreparedFunctionLookups`,
  `prepared_lookups`, `shared_function_lookups`,
  `shared_call_plan_lookups`, and direct field references to all seven lookup
  groups in `src` and `tests`.
- Ran a docs cross-check for route family, fallback, and lookup-group mentions
  under `docs/bir_prealloc_fusion`.
