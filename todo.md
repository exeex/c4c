Status: Active
Source Idea Path: ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Identify adapter and contraction prerequisites

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: converted the Step 1/2
`PreparedFunctionLookups` inventory and ownership classification into
per-lookup-group prerequisites for future contraction or adapter work. No
aggregate field was renamed, split, deleted, privatized, or recast as BIR-owned
without a one-boundary proof route.

Prerequisite table:

| Lookup group | Step 3 prerequisite or retained-ownership result | Adapter boundary and proof surface | Diagnostics/oracle or follow-up requirement |
| --- | --- | --- | --- |
| `call_plans` | Retain prepared/target ownership for whole call plans. Future contraction may only target one Route 6 semantic call-use source read at a time; ABI register/stack placement, variadic FPR counts, byval lanes, clobber/preserve sets, outgoing stack sizing, helper protocols, result lanes, and call-record spelling remain prepared/target policy. | Existing ready adapter boundary is the x86 scalar `find_consumed_scalar_i32_call_argument_source` / `Route6CallUseSourceIndex` agreement path. Future AArch64 or x86 work must key one call instruction plus one argument/result role, prove source-id/status agreement, reject absent/mismatched/ambiguous/ABI-bound cases, and fall back to `PreparedCallPlanLookups` or indexed call argument/result plans for policy-sensitive facts. Proof surface: selected Route 6 backend tests plus call-boundary/frame-stack sanity preserving ABI policy. | Retain prepared call-plan oracles, `backend_prepared_lookup_helper`, x86 route-debug compatibility rows, and prepared call-boundary tests until route-native helper/oracle coverage covers success, missing fact, invalid input, ambiguity, mismatch, and fallback without reading prepared call plans as authority. |
| `address_materializations` | Retain prepared/AArch64/target ownership. No contraction prerequisite exists for whole address materializations because Step 2 found target-addressing policy, frame/stack object offsets, TLS/global relocation choice, pointer materialization, base-plus-offset legality, volatile/address-space behavior, and memory operand formation outside route ownership. | No route-native adapter boundary is ready. A future follow-up would need one narrowly scoped semantic address/source question, explicitly excluding target address policy, and must not replace `PreparedAddressMaterializationLookups` or frame-address offset helpers wholesale. Proof surface would be an AArch64 addressing-sensitive backend test with prepared fallback and unchanged address-materialization policy oracles. | Retain address-materialization tests and prepared lookup helper coverage. Smallest follow-up: audit one AArch64 address-materialization reader to determine whether it asks a semantic identity question or only target address policy; if policy-only, leave it prepared-owned. |
| `memory_accesses` | Route-native candidate only for Route 3 semantic memory/access identity; aggregate contraction remains blocked by AArch64 production readers and policy-sensitive fallbacks. Target address formation, frame/TLS/global relocation, volatile/address-space behavior, and materialization policy remain prepared/target-owned. | Required adapter boundary: one selected Route 3 memory/source consumer at a time, such as a global-load, same-block global-load, load-local, store-source, or x86 local-slot semantic memory-source read. It must prove memory access id, source value/global/local identity, block/instruction compatibility, ambiguity rejection, prepared mismatch fallback, and target-policy exclusion. Proof surface: AArch64 Route 3 oracle-backed tests first, with target-wrapper tests only after the AArch64 semantic boundary is proven. | Retain `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`, global-load/same-block/load-local helpers, store-source publication helper inputs, prepared lookup helper oracle, and memory/frame/stack oracles. Smallest follow-up: pick one direct AArch64 `memory_accesses` reader and prove route-first semantic identity with prepared fallback and target-addressing policy preservation. |
| `move_bundles` | Retain prepared/target ownership. Move scheduling, before-call argument moves, before-return ABI moves, after-call result lane bindings, out-of-SSA placement, cycle temporaries, execution sites, homes, storage, and final move records are not BIR route facts. | No contraction adapter is ready for the group. Route 5 or Route 6 may identify adjacent semantic source facts, but a move-bundle adapter would have to be scoped to one consumer that asks only source identity and explicitly falls back to `PreparedMoveBundleLookups` for ordering, storage, ABI move, and scheduling policy. Proof surface would be selected backend movement tests plus unchanged call/return/edge-copy move-policy oracles. | Retain move-bundle oracle tests and prepared lookup helper coverage. Smallest follow-up: audit one move-bundle reader to separate semantic source identity from move-order/policy use; if it consumes move policy, keep prepared ownership. |
| `value_homes` | Retain prepared/target ownership. Storage/home authority, register choice, stack slot/offset, rematerialization spelling, formal-publication inputs, decoded-home rendering, and value-home policy remain prepared/target-owned; route facts may only reference value identities around those policies. | No route-native contraction boundary is ready. Existing x86 decoded-home usage with `value_home_lookups = nullptr` is not deletion evidence. A future adapter must target one semantic value-id or source-id read, not home/storage policy, and prove route/prepared agreement plus fallback without changing operand rendering or storage decisions. Proof surface: one target-local context or wrapper test naming route authority, prepared fallback, and unchanged home/storage policy. | Retain `PreparedValueHomeLookups`, decoded-home/formal-publication inputs, AArch64 operand/prologue/dispatch/publication/call/memory consumers, BIR/prealloc value-home oracles, and prepared-memory-operand tests. Smallest follow-up: identify one value-home consumer that can be rewritten to use a semantic route id while leaving storage/home lookup as prepared-owned. |
| `edge_publications` | Route-native candidate for Route 4 publication identity and Route 5 edge/join-source identity only. Aggregate contraction is blocked by direct AArch64 readers plus x86/riscv prepared wrapper readers; publication record construction, block-order emission, move/homes/storage policy, stack-source extension, and final edge-copy records remain prepared/target-owned. | Required adapter boundary: one edge-publication consumer or wrapper at a time. x86/riscv candidates must consume only Route 4/5 source or publication identity, prove predecessor/successor or block-label agreement, destination/source value agreement, duplicate/ambiguous/conflict rejection, and prepared fallback while preserving emitted move/register/stack policy. Proof surface: AArch64 Route 4/5 oracle coverage before target-wrapper reuse, then x86 or riscv wrapper fallback tests for the selected boundary. | Retain `PreparedEdgePublicationLookups`, indexed/unique publication helpers, block-entry parallel-copy helpers, stack-source and publication-match helpers, AArch64 dispatch/memory/publication fallbacks, x86 prepared dispatch, riscv prepared emission, and wrapper tests. Smallest follow-up: one x86 or riscv edge-publication semantic source adapter after an AArch64 Route 4/5 proof for the exact fact. |
| `edge_publication_source_producers` | Route-native candidate for narrow producer/constant, select-chain/direct-global, join-source, call-use, and comparison provenance facts, but retained as production fallback and printer/debug oracle. Materialization sequence choice, direct-global policy when coupled to call/memory/publication contexts, storage/home/move policy, call/publication routing policy, and final records remain prepared/target-owned. | Required adapter boundary: one route family and one consumer role at a time: Route 1 producer/constant, Route 2 select-chain/direct-global, Route 5 join-source, Route 6 call-use source, or Route 7 comparison provenance. Each adapter must prove route/prepared agreement, absent/mismatched/ambiguous fallback, and policy exclusion for the selected semantic fact. Proof surface: route-family-specific backend tests plus unchanged prepared fallback/oracle tests; printer/debug replacement must be separate from production lowering. | Retain source-producer helpers, prepared printer `select_chains` output, dispatch/lookup/store-source/prepared-memory/call/frame-stack oracles, and helper fallback coverage. Smallest follow-up: route-native helper or printer oracle for one semantic lookup family before contracting any public source-producer diagnostic surface. |

Readiness summary:

- No `PreparedFunctionLookups` group is ready for deletion, privatization, or
  aggregate-wide replacement from this audit state.
- `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` have route-native semantic subfacts, but
  each needs one-consumer adapter proof with prepared fallback and replacement
  oracle coverage.
- `call_plans` has only the selected Route 6 semantic call-use source path and
  x86 scalar source reuse as ready adapter evidence; whole call-plan ownership
  remains target/prepared.
- `address_materializations`, `move_bundles`, and `value_homes` remain
  target/prepared policy or transient pass-context groups unless a future
  one-consumer audit proves a semantic read separable from policy.
- x86/riscv wrapper work must reuse already-proven AArch64 semantic route
  views and preserve target-local policy; no target-only BIR adapter or broad
  aggregate replacement should be opened from this step.

## Suggested Next

Execute Step 4 by writing the durable ownership/readiness map under
`docs/bir_prealloc_fusion/`. The artifact should carry the Step 1 reader
inventory, Step 2 ownership classification, and this Step 3 prerequisite table
into a stable lookup-group map with columns for current readers, ownership,
route-view replacement status, fallback/oracle surfaces, adapter need,
blockers, and future readiness.

## Watchouts

- Keep future work one consumer or adapter boundary at a time. Do not propose a
  broad `PreparedFunctionLookups` replacement, BIR-owned clone, rename-only
  progress, or test expectation weakening.
- Route-native candidates are semantic subfact candidates, not ownership of
  target policy. ABI, addressing, home/storage, move scheduling, wrapper
  construction, and final emission records stay target/prepared-owned unless a
  separate implementation idea proves otherwise.
- Compatibility adapters are retained public surfaces until route-native
  diagnostics/oracles cover success, missing-input, invalid-input,
  ambiguity/conflict, mismatch, and prepared fallback without using prepared
  data as the route source of truth.
- RISC-V has no ready route-view reuse proof. Any riscv packet should begin
  from an already-proven AArch64 Route 4/5 semantic fact and preserve riscv
  register, stack, operand, and emission policy.
- Prepared printer/debug and `backend_prepared_lookup_helper` coverage are
  retained oracle surfaces. Do not treat production-only route migration as
  diagnostic/oracle replacement.

## Proof

Docs/lifecycle-only prerequisite packet. No build/test proof was required and
no `test_after.log` was generated. Validation/searches performed:

- Read `AGENTS.md`, active `plan.md`, source idea 196, and current `todo.md`.
- Read `src/backend/prealloc/prepared_lookups.hpp` to confirm the seven
  `PreparedFunctionLookups` lookup groups.
- Ran targeted `rg` scans for `PreparedFunctionLookups`, `prepared_lookups`,
  direct lookup group names, `shared_function_lookups`,
  `shared_call_plan_lookups`, and cross-target wrapper references in `src`,
  `tests`, and `docs/bir_prealloc_fusion`.
- Cross-checked prerequisite wording against
  `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
  and `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`.
