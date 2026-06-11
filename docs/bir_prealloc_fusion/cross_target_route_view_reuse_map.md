# Cross-target Route-view Reuse Map

Status: durable audit artifact for idea 195.

Source idea: `ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md`

## Summary

The existing x86 Route 6 scalar helper is the only ready cross-target reuse
point found by this audit. It asks one semantic question: whether a Route 6
call-use source agrees with the prepared scalar `i32` call-argument source for
the same call and argument. The helper keeps x86 ABI, wrapper, move, and
emission policy local, and it falls back to the prepared source whenever the
route fact is absent, unavailable, mismatched, duplicate, ABI-bound, or not the
same scalar source case.

That evidence does not generalize to whole x86 `ConsumedPlans`, whole call
plans, the x86 prepared query facade, x86 edge or memory wrappers, or any riscv
wrapper. Those boundaries either lack a target wrapper adapter, lack
oracle-backed AArch64 route-view coverage for the exact semantic sub-read, or
mix semantic source identity with target-local ABI, register, stack, formatting,
move, addressing, and emission policy.

No boundary beyond x86 Route 6 has a clear proof route yet. Future work should
be split into one wrapper boundary at a time and should preserve prepared
fallback while proving route/prepared agreement and policy exclusion.

## Reuse Table

| Target | Wrapper boundary | Consumed fact/helper | Required route view | Required agreement/fallback checks | Retained target-local policy | Readiness | Proof route |
| --- | --- | --- | --- | --- | --- | --- | --- |
| x86 | `ConsumedPlans` / `consume_plans` aggregate in `src/backend/mir/x86/x86.hpp` | Prepared frame, dynamic-stack, control-flow, call-plan, regalloc, storage, function lookup banks, plus `Route6CallUseSourceIndex` | Only the Route 6 call-use source view is proven for the scalar argument-source sub-boundary | Route 6 call key, argument index, status, source value/id agreement with the prepared argument plan; fall back for absent, unavailable, mismatched, duplicate, ambiguous, ABI-bound, or non-scalar facts | Frame layout, dynamic stack, call ABI, regalloc, storage, wrapper construction, and emission | `ready` only for the selected Route 6 scalar source sub-boundary; broader aggregate migration is blocked or target-local | Existing x86 Route 6 proof: `ctest --test-dir build -R '^backend_x86_handoff_boundary$' --output-on-failure`, with adjacent `backend_prepare_frame_stack_call_contract` and `backend_aarch64_instruction_dispatch` sanity before any code-changing expansion |
| x86 | `find_consumed_scalar_i32_call_argument_source` | `route6_find_call_argument_source` over `Route6CallUseSourceIndex`, cross-checked against `PreparedCallArgumentPlan::source_value_id` | Route 6 call-use source view | Same call, argument index, available status, scalar source agreement, ABI-bound rejection, duplicate/no-match rejection; fall back to prepared source on any failure | Scalar `i32` wrapper decision and downstream ABI/move selection | `ready` and already proven | Same `backend_x86_handoff_boundary` route; no broader wrapper claim is implied |
| x86 | `find_consumed_call_plan`, `find_consumed_call_argument_plan`, `find_consumed_call_result_plan` | `PreparedCallPlanLookups`, indexed prepared call plans, argument plans, result plans | Route 6 may cover semantic call-use source identity, but no route view owns whole call plans or result lanes | A future semantic source read would need call instruction index, argument/result index or lane, source id, status, ambiguity rejection, and ABI-bound rejection; fall back to prepared call plans for whole-plan or policy-sensitive facts | ABI binding, call wrapper kind, outgoing stack, result lanes, moves, clobbers, and call record spelling | `blocked` for whole-plan reuse; possible only as one semantic source read at a time | No clear proof route for whole-plan substitution; a future idea must prove one semantic argument or result source read and reject ABI/result-lane migration |
| x86 | Direct extern and same-module call wrapper paths | `find_consumed_call_plan`, `find_consumed_scalar_i32_call_argument_source`, `PreparedCallWrapperKind`, `PreparedMoveBundle`, value homes, call argument/result ABI moves | Route 6 call-use source view only through the scalar helper | Use the helper agreement checks before source substitution; fall back to prepared call argument/result plans, moves, and homes when the helper rejects or the case is not scalar Route 6 source reuse | Wrapper construction, ABI moves, return handling, instruction spelling, and record emission | `target-local` with one ready semantic sub-read | Existing helper proof is an input only; a wrapper-specific code change would need unchanged wrapper-output tests with route-absent and route-mismatch fallback |
| x86 | ABI placement helpers (`select_prepared_call_argument_abi_register_if_supported`, `select_prepared_call_argument_abi_stack_offset_if_supported`) | `PreparedValueLocationFunction`, move bundles, move resolution, ABI binding | None for ABI placement | Reject route facts marked ABI-bound; keep prepared ABI placement for absent or policy-sensitive facts | ABI register/stack placement and move resolution | `target-local` | No route-view proof route; this should remain prepared/x86 policy |
| x86 | Edge-publication move intent and instruction emission in x86 prepared dispatch | `find_unique_indexed_prepared_edge_publication`, `PreparedEdgePublication`, `PreparedValueHome`, `PreparedMoveResolution` | Potential Route 4 publication identity or Route 5 edge/join-source identity | Future adapter must prove edge/source/destination ids, predecessor/successor or block labels, source value agreement, ambiguity/conflict rejection, and prepared fallback | Edge-copy scheduling, move bundles, homes, register/storage choices, and instruction emission | `unknown` for semantic source reuse; not ready | Future idea candidate: x86 edge-publication semantic source adapter at this wrapper boundary, backed by AArch64 Route 4/5 oracle coverage and x86 boundary fallback tests |
| x86 | Compare/join, countdown, and short-circuit helpers | Prepared control-flow facts, parallel-copy bundles, branch-condition helpers, out-of-SSA move helpers | Potential Route 5 join-source or Route 7 comparison provenance | Future adapter must prove branch/condition or join-edge ids, predecessor/source agreement, comparison operand provenance, conflict rejection, and prepared fallback | Parallel-copy scheduling, branch spelling, countdown/short-circuit emission, move order, and compare instruction policy | `unknown`; not ready | Possible only after narrowing to one compare/join semantic provenance read and pairing Route 5/7 agreement with adjacent branch/parallel-copy fallback cases |
| x86 | `x86::prepared::Query` facade | Mixed prepared addressing, value-location, call-plan, regalloc, storage, BIR lookup, decoded-home, call-boundary, formal-publication, and block-entry queries | No single required view; individual methods may map to Routes 3, 4, 5, 6, or 7 | Method-specific source-id, route-id, lookup agreement, ambiguity rejection, policy exclusion, and prepared fallback would be required | Facade shape, decoded homes, target wrapper lookups, and mixed prepared policy | `blocked` as a broad facade migration | No facade-wide proof route; split method-level ideas only |
| x86 | Grouped authority comments | Prepared frame, call, regalloc, and storage facts | None | No route checks; keep comments as prepared authority summaries | Debug/comment formatting and target handoff explanation | `target-local` | No route-view proof route |
| x86 | Value location, return/instruction moves, value homes, register homes | Prepared value-location, move-bundle, value-home, and register-home helpers | None for homes, moves, or register assignment | Reject route migration unless source/value provenance is separated from home/register/move policy; fall back to prepared helpers | Register homes, value homes, move selection, return/instruction move policy | `target-local` | No route-view proof route in this idea |
| x86 | Local-slot and compare-load helpers | Prepared addressing, memory access, frame-slot, and value-home facts | Possible Route 3 memory/source identity for a narrower semantic read | Future adapter must prove memory access id, source value/global/local identity, block/instruction compatibility, ambiguity rejection, and policy exclusion for frame slot, offset, and addressing legality | Frame slots, offsets, operand rendering, compare-load instruction choice, and target addressing legality | `blocked` for local-slot policy; `unknown` for a narrow semantic memory-source read | Possible future idea: x86 local-slot semantic memory-source adapter, after AArch64 Route 3 oracle-backed coverage and x86 addressing-sensitive fallback tests |
| riscv | `emit_prepared_module` prepared lookup setup | `PreparedBirModule::control_flow.functions`, `make_prepared_function_lookups`, `PreparedFunctionLookups`, `PreparedEdgePublicationLookups` | None at the aggregate level | No route-id checks found; continue prepared lookup setup unless a future semantic candidate supplies route identity and fallback | Prepared-module traversal and lookup packaging | `target-local`; no riscv route-view reuse proven | No proof route |
| riscv | Edge-publication move intent and instruction emission | Prepared function lookups, `find_unique_indexed_prepared_edge_publication`, `PreparedEdgePublication`, `PreparedValueHome`, `PreparedMoveResolution` | Potential Route 4 publication identity or Route 5 edge/join-source identity | Future adapter must prove predecessor/successor or block labels, destination value id, source value/producer agreement, unique publication match, ambiguity/conflict rejection, and prepared fallback | Edge-copy emission, move scheduling, homes, register naming, stack/immediate handling, and formatting | `blocked`; strongest future riscv candidate but not ready | Future idea candidate: riscv edge-publication semantic source adapter at one wrapper boundary, backed by Route 4/5 source identity agreement, duplicate/ambiguous rejection, and preserved prepared fallback |
| riscv | `render_edge_publication_source_operand` | `PreparedValueHome`, value-home lookup tables, register/immediate/stack/pointer-base/load-local source metadata | Route 4/5 may provide semantic source identity only after adapter proof | Future semantic reuse must prove source value/home agreement and reject facts that require operand-policy decisions; fall back to prepared homes for absent, mismatched, ambiguous, or policy-sensitive facts | Operand spelling, register/immediate/stack/pointer-base/load-local formatting, and emission | `target-local` for rendering; semantic source contribution remains `unknown` | No clear proof route until a source-id-only adapter is designed |
| riscv | `prepare_same_width_i32_stack_source_publication` use inside edge publication | Prepared compatibility helper over source/destination type, stack slot, offset, size, alignment, extension, destination bank, and register placement | No route view should own the whole helper; Route 4/5 could at most inform source identity | Future adapter must separate semantic source identity from width, stack, extension, and register-placement policy; fall back for absent, mismatched, ambiguous, width-sensitive, or stack/register-policy-sensitive facts | Typed stack-source publication, extension policy, stack layout, destination register bank/placement | `target-local` with possible `unknown` semantic source sub-read; not ready | No clear proof route until the source-id-only boundary is separated from stack/register policy |
| riscv | `riscv_gpr_register_name_from_placement` | `PreparedRegisterPlacement` bank, pool, slot, width | None | No route checks; route facts must not encode riscv register spelling | GPR bank/pool/slot/width naming | `target-local` | No route-view proof route |
| riscv | Stack-destination policy helpers | `EdgePublicationMoveIntent`, `PreparedValueHome`, `PreparedEdgePublication` metadata | None for policy decisions; Route 4/5 could at most inform semantic source identity after adapter proof | Future source identity reuse must exclude scratch, aliasing, offset, width, and concrete-register decisions; fall back for absent, mismatched, ambiguous, or policy-sensitive facts | Scratch choice, alias checks, stack offset/width handling, concrete register policy | `target-local`; no riscv route-view reuse proven | No current proof route |
| riscv | `emit_prepared_lir_module` | Direct `LirModule` shape | None | Not applicable | Direct LIR emission | `target-local` and outside route-view reuse scope | No route-view proof route |

## x86 Status

x86 has one ready semantic route-view reuse point:
`find_consumed_scalar_i32_call_argument_source` can prefer the Route 6
call-use source view when it agrees with the prepared scalar call-argument
source. This is a narrow source-identity adapter, not a whole-plan or
whole-wrapper migration.

All other x86 boundaries remain blocked, target-local, or unknown. Whole call
plans, ABI placement, value homes, moves, frame/storage/regalloc authority,
local-slot addressing, compare/join emission, edge-copy scheduling, branch
spelling, and facade packaging are not semantic route facts. A future x86 idea
must choose one wrapper boundary and one semantic sub-read, then prove
route/prepared agreement, missing/mismatched/ambiguous fallback, and unchanged
target policy.

Strongest future x86 candidates are:

- x86 edge-publication semantic source adapter at the edge-publication wrapper
  boundary.
- x86 local-slot semantic memory-source adapter at one load or compare helper
  boundary.
- x86 compare/join semantic provenance adapter after a single Route 5 or Route
  7 source read is identified.

Each candidate is blocked today. None should be treated as ready from the Route
6 scalar helper evidence.

## riscv Status

riscv has no ready route-view reuse point in this audit. The riscv wrapper code
currently consumes prepared edge-publication facts, prepared value homes,
prepared stack/register policy, and direct LIR shape. No discovered riscv
boundary consumes a proven route view.

The strongest future riscv candidate is an edge-publication semantic source
adapter that consumes only Route 4/5 source or publication identity while
preserving riscv move scheduling, operand rendering, stack/immediate handling,
register naming, and formatting locally. That candidate is blocked until a
riscv wrapper adapter and proof route exist. The current riscv prepared
edge-publication tests are valuable fallback and target-policy oracles, but
they are not route-view readiness evidence by themselves.

## Future Implementation Boundaries

Future implementation ideas should be one wrapper boundary wide:

| Future idea boundary | Scope | Minimum proof shape |
| --- | --- | --- |
| x86 edge-publication semantic source adapter | Consume a Route 4 publication identity or Route 5 edge/join-source identity at one x86 edge-publication wrapper boundary. | AArch64 Route 4/5 oracle coverage, x86 boundary test for source/destination agreement, duplicate/ambiguous rejection, prepared fallback, and unchanged emitted move policy. |
| riscv edge-publication semantic source adapter | Consume a Route 4/5 semantic source identity at one riscv edge-publication wrapper boundary. | riscv boundary test for predecessor/successor or block-label agreement, source/destination agreement, duplicate/ambiguous rejection, preserved prepared fallback, and unchanged riscv operand/register/stack policy. |
| x86 local-slot semantic memory-source adapter | Consume a Route 3 memory/source identity at one x86 load or compare helper boundary. | AArch64 Route 3 oracle-backed semantic memory-source case, x86 addressing-sensitive fallback, source/global/local identity agreement, and unchanged frame-slot/offset/operand rendering policy. |
| x86 compare/join semantic provenance adapter | Consume one Route 5 join-source or Route 7 comparison provenance fact at one compare/join wrapper boundary. | Candidate-specific branch or join-source agreement test, conflict rejection, prepared fallback, and adjacent branch/parallel-copy sanity cases. |
| x86 method-level prepared-query adapter | Replace one `x86::prepared::Query` method with a proven semantic route read. | Method-specific route/prepared agreement, ambiguity rejection, policy exclusion, and fallback. No facade-wide migration. |

Do not open a broad x86 or riscv route-view migration from this audit. A
follow-up plan that does not name one wrapper boundary, one semantic route fact,
the retained prepared fallback, and the target policy it refuses to migrate
should remain a planning task rather than an implementation task.
