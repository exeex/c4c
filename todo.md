Status: Active
Source Idea Path: ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify reuse candidates and retained target policy

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: classified the Step 1 x86/riscv wrapper
inventory as route-view ready, blocked, target-local, or unknown. The only
ready reuse point remains the already-proven x86 Route 6 scalar call-argument
source adapter guarded by prepared call-plan agreement; all riscv statuses are
explicit and do not inherit x86 readiness.

### Classification key

- `ready`: the target wrapper can consume an AArch64-proven semantic route view
  with source/route agreement checks and prepared fallback already identified.
- `blocked`: the boundary would need a missing target wrapper adapter,
  additional AArch64-proven route view coverage, or unresolved ownership before
  reuse can proceed.
- `target-local`: the inventoried behavior is ABI, frame, register, stack,
  wrapper, formatting, instruction-selection, or emission policy and must not
  migrate into a semantic route view.
- `unknown`: Step 1 found a prepared/helper boundary, but current evidence is
  insufficient to name a safe route-view owner or classify it as policy-only.

### x86 classification

| Boundary | Required AArch64-proven route view | Required checks | Fallback for absent/mismatched/ambiguous/policy-sensitive facts | Retained target-local policy | Status |
| --- | --- | --- | --- | --- | --- |
| `ConsumedPlans` / `consume_plans` aggregate in `x86.hpp` | Only the `Route6CallUseSourceIndex` call-use source view is proven for reuse; frame, dynamic-stack, control-flow, call-plan, regalloc, storage, and function-lookup banks have no aggregate route-view replacement. | Route 6 call key must match the prepared call site, argument index, and `PreparedCallArgumentPlan::source_value_id`; other banks require no route-id check because they remain prepared-owned. | Use prepared banks when Route 6 is absent, unavailable, mismatched, duplicate/ambiguous, ABI-bound, or not the scalar argument-source case. | x86 frame layout, dynamic stack, call ABI, regalloc, storage, wrapper construction, and emission. | `ready` only for the selected Route 6 scalar source sub-boundary; aggregate remains `target-local`/`blocked` for broader migration. |
| `ConsumedPlans::shared_function_lookups` and `shared_call_plan_lookups` | None. These are prepared lookup compatibility adapters. | Prepared lookup identity/index consistency only; no route-id contract found. | Continue using `make_prepared_function_lookups` and prepared call-plan lookups. | Lookup packaging and compatibility surface. | `target-local` as a compatibility adapter. |
| `find_consumed_call_plan`, `find_consumed_call_argument_plan`, `find_consumed_call_result_plan` | Route 6 can cover semantic call-use source identity, but not whole call plans or ABI/result plan ownership. | If later narrowed to a semantic source read, require call instruction index, argument/result index or lane, source id, status, and ABI-bound rejection checks. | Fall back to `PreparedCallPlanLookups` and indexed prepared call plans for missing, mismatched, ambiguous, result-lane duplicate, or ABI/layout-bound facts. | ABI binding, call wrapper kind, outgoing stack, result lanes, moves, clobbers, and call record spelling. | `blocked` for whole-plan reuse; only adjacent semantic source reads may become ready one class at a time. |
| `find_consumed_scalar_i32_call_argument_source` | Route 6 call-use source view, specifically `route6_find_call_argument_source` over `Route6CallUseSourceIndex`. | Call route key, argument index, available status, source value/id agreement with the prepared argument plan, and ABI-bound/duplicate/no-match rejection. | Return the prepared argument source when Route 6 is absent, unavailable, wrong-call, missing, duplicate, no-match, ABI-bound, or source-id mismatched. | x86 scalar-i32 wrapper decision and downstream ABI/move selection. | `ready` and already proven by the x86 Route 6 boundary. |
| `append_prepared_direct_extern_call_argument`, `append_prepared_direct_extern_call_return_function`, and same-module call wrapper path | Route 6 call-use source view only through the scalar source helper above. | Must rely on the helper's source agreement checks before substituting a semantic source; wrapper call-plan and move facts must still come from prepared data. | Use prepared call argument/result plans, move bundles, and value homes whenever the helper rejects or the case is not scalar Route 6 source reuse. | Direct extern/same-module wrapper construction, ABI moves, return handling, instruction spelling, and record emission. | `target-local` with one `ready` semantic sub-read. |
| `select_prepared_call_argument_abi_register_if_supported` and `select_prepared_call_argument_abi_stack_offset_if_supported` | None for ABI placement; Route 6 intentionally excludes ABI-bound source selection. | Reject route facts marked ABI-bound; any future semantic read must not decide register/stack placement. | Keep prepared ABI binding and move-resolution path. | ABI register/stack placement and move resolution. | `target-local`. |
| `consume_edge_publication_move_intent` / `append_edge_publication_move_instruction` in x86 prepared dispatch | Potentially Route 4 publication identity or Route 5 edge/join-source identity, but no x86 wrapper adapter proof exists. | Would need edge/source/destination ids, predecessor/successor or block labels, publication source value agreement, and ambiguity/conflict checks against prepared edge publication. | Use `find_unique_indexed_prepared_edge_publication`, `PreparedValueHome`, and `PreparedMoveResolution` for absent, mismatched, ambiguous, or policy-sensitive facts. | Edge-copy scheduling, move bundles, value homes, register/storage choices, and instruction emission. | `unknown` for semantic edge/publication source reuse; target policy remains local. |
| `append_prepared_compare_join_parallel_copy` and related compare/join, countdown, and short-circuit helpers | Route 5 may cover current-block join/edge source identity; Route 7 may cover comparison/condition provenance. No x86 wrapper proof ties these helpers to those views. | Would need branch/condition or join edge ids, predecessor/source agreement, comparison operand provenance checks, and conflict rejection. | Keep prepared control-flow, parallel-copy bundles, out-of-SSA moves, and branch-condition helpers for all absent/mismatched/ambiguous or policy-sensitive cases. | Parallel-copy scheduling, branch spelling, countdown/short-circuit emission, move order, and compare instruction policy. | `unknown` for semantic provenance; retained behavior is target-local. |
| `x86::prepared::Query` facade | No single required view; individual methods could map to Routes 3, 4, 5, 6, or 7 only after a method-specific AArch64 proof. | Method-specific source-id, route-id, lookup agreement, ambiguity, and policy-exclusion checks would be required. | Continue direct prepared queries unless a single method gets a proven semantic route view with fallback. | Facade shape, decoded homes, target wrapper lookups, and mixed prepared policy. | `blocked` as a broad facade migration; method-level status remains `unknown`. |
| `append_grouped_authority_comments` | None. Comments summarize prepared frame/call/regalloc/storage authority, not a semantic route-view consumer. | No route checks required. | Continue prepared authority comments. | Debug/comment formatting and target handoff explanation. | `target-local`. |
| `require_prepared_value_location_function`, `require_prepared_i32_return_move`, `require_prepared_i32_instruction_move`, `require_prepared_i32_value_home`, `require_prepared_i32_register_home` | None for homes/moves/register assignment; future return-chain work is separate and not established here. | Reject route migration unless source/value provenance is separated from home/register/move policy. | Keep prepared value-location, move-bundle, and value-home queries. | Register homes, value homes, move selection, return/instruction move policy. | `target-local`. |
| Local-slot and compare-load helpers in `module.cpp` | Route 3 memory/source identity could cover semantic memory/access source, but x86 local-slot addressing and compare-load use lacks wrapper proof. | Would need memory access id, source value/global/local identity, block/instruction compatibility, and policy-exclusion for frame slot/offset/addressing legality. | Use prepared addressing, memory access, frame-slot, and value-home facts for absent, mismatched, ambiguous, or addressing-sensitive facts. | Frame slots, offsets, operand rendering, compare-load instruction choice, and target addressing legality. | `blocked` for local-slot policy; `unknown` for a narrower semantic memory-source read. |

### riscv classification

| Boundary | Required AArch64-proven route view | Required checks | Fallback for absent/mismatched/ambiguous/policy-sensitive facts | Retained target-local policy | Status |
| --- | --- | --- | --- | --- | --- |
| `emit_prepared_module` prepared lookup setup | None at this aggregate level; it threads `PreparedFunctionLookups` and edge-publication lookups. | No route-id checks found in the riscv wrapper. A future semantic candidate would need function/source route identity before threading. | Continue `make_prepared_function_lookups` and prepared edge-publication lookup setup. | riscv prepared-module traversal and lookup packaging. | `target-local`; no riscv route-view reuse proven. |
| `consume_edge_publication_move_intent` / `append_edge_publication_move_instruction` | Potentially Route 4 publication identity or Route 5 edge/join-source identity. No riscv adapter proof exists. | Would need predecessor/successor or block labels, destination value id, source value/producer agreement, unique publication match, and ambiguity/conflict rejection against prepared edge publication. | Keep prepared `find_unique_indexed_prepared_edge_publication`, value homes, and move resolution when route facts are absent, mismatched, duplicate, ambiguous, or policy-sensitive. | riscv edge-copy instruction emission, move scheduling, value-home rendering, register naming, stack/immediate handling. | `blocked` for route-view reuse until a riscv wrapper adapter and proof route exist. |
| `render_edge_publication_source_operand` | Route 4/5 may provide semantic source identity, but operand rendering requires prepared homes and riscv formatting. | Any semantic reuse would need source value/home agreement and explicit rejection of facts that require target operand policy. | Render from `PreparedValueHome` and lookup tables when route facts are absent, mismatched, ambiguous, or require operand-policy decisions. | Register/immediate/stack/pointer-base/load-local operand spelling and riscv emission formatting. | `target-local`; semantic source contribution remains `unknown`. |
| `prepare_same_width_i32_stack_source_publication` use in riscv edge publication | No route view should own the whole helper; semantic source identity may overlap Route 4/5, but width/stack/register placement policy is prepared/riscv-owned. | Would need same source/destination semantic identity plus explicit exclusion of stack slot, offset, size, alignment, extension, and register-placement policy from the route view. | Use the prepared compatibility helper for absent, mismatched, ambiguous, width-sensitive, or stack/register-policy-sensitive facts. | Typed stack-source publication, extension policy, stack layout, destination register bank/placement. | `target-local` with possible `unknown` semantic source sub-read. |
| `riscv_gpr_register_name_from_placement` | None. Register placement/name mapping is target policy. | No route checks required; route facts must not encode riscv register spelling. | Continue prepared placement to riscv register name conversion. | riscv GPR bank/pool/slot/width naming. | `target-local`. |
| Stack-destination policy helpers (`has_direct_register_source_for_stack_destination`, `has_rematerializable_i32_source_for_stack_destination`, `has_non_aliasing_i32_stack_source_for_stack_destination`, `has_existing_concrete_i64_stack_source_register_policy`) | None for the policy decisions; Route 4/5 could at most inform semantic source identity after adapter proof. | Would need source identity agreement plus conflict checks, while excluding scratch, aliasing, offset, width, and concrete-register policy from the route. | Keep `EdgePublicationMoveIntent`, `PreparedValueHome`, and `PreparedEdgePublication` decisions for all absent/mismatched/ambiguous or policy-sensitive facts. | riscv scratch choice, alias checks, stack offset/width handling, concrete register policy. | `target-local`; no riscv route-view reuse proven. |
| `emit_prepared_lir_module` | None. This consumes direct `LirModule` shape, not prepared route facts or route views. | No route checks required. | Not applicable. | Direct LIR emission. | `target-local` and outside route-view reuse scope. |

### Readiness summary

| Target | Ready | Blocked | Target-local | Unknown |
| --- | --- | --- | --- | --- |
| x86 | `find_consumed_scalar_i32_call_argument_source` using Route 6 with prepared source-id agreement. | Whole call-plan reuse, broad `x86::prepared::Query`, and local-slot/compare-load migration. | ABI placement, wrapper emission, frame/storage/regalloc comments, value homes, moves, registers, and aggregate prepared lookup packaging. | Edge/publication and compare/join semantic sub-reads where Route 4/5/7 may apply but no x86 adapter proof exists. |
| riscv | None. | Edge-publication move reuse is blocked until a riscv adapter/proof exists for Route 4/5 semantics. | Prepared-module setup, operand rendering policy, stack/register policy helpers, register naming, stack-source publication policy, and direct LIR emission. | Only narrow semantic source sub-reads inside operand/stack-source publication remain unknown; they are not ready. |

## Suggested Next

Start Step 3 by assigning proof routes only for the classified candidates:
the x86 Route 6 ready boundary can cite the existing source-id agreement proof,
while riscv edge-publication should be recorded as blocked pending a riscv
Route 4/5 adapter and narrow proof route.

## Watchouts

- Do not treat x86 Route 6 `ConsumedPlans` as broad cross-target readiness.
- Do not migrate ABI, frame, register, wrapper, formatting, instruction
  selection, or emission policy into shared route facts.
- Do not weaken test or oracle expectations to make a boundary appear ready.
- x86 readiness is limited to the scalar Route 6 source helper; broad call-plan,
  prepared facade, local-slot, edge/publication, and compare/join reuse still
  need separate boundary-level proof.
- riscv currently has no ready route-view reuse point. Its edge-publication
  path is the most plausible future candidate, but current evidence supports
  `blocked`, not `ready`.
- Treat `unknown` as a demand for more boundary evidence, not permission to
  implement a target-specific BIR adapter.

## Proof

Lifecycle/docs-only packet. Used the Step 1 inventory in `todo.md` as the
primary input, then targeted `rg` confirmation of the AArch64-proven route-view
evidence in `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
and `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`.
No build or test proof was required, and no `test_after.log` was generated for
this docs-only classification.
