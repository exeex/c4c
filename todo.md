Status: Active
Source Idea Path: ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish the Post-E5 Evidence Base

# Current Packet

## Just Finished

Completed plan.md Step 1 evidence-base inspection for the post-E5 x86/riscv
BIR portability convergence audit.

Inspected closure/source evidence:

- `ideas/open/243_phase_f0_x86_riscv_bir_portability_convergence_audit.md`
- `ideas/closed/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`
- `ideas/closed/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `ideas/closed/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md`
- `ideas/closed/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md`
- `ideas/closed/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`

E4 follow-up 238 status: closed before F0/E5 successor evidence is being used,
but it is narrow evidence only for the x86 Route 6 scalar `i32` route-debug /
`ConsumedPlans` compatibility boundary. It is not riscv readiness, broad x86
call-wrapper readiness, route-wide wrapper convergence, or prepared aggregate
retirement evidence.

Closure evidence captured:

- E5/239 keeps draft 155 blocked, found no whole `PreparedBirModule` field
  group and no whole `PreparedFunctionLookups` group ready for deletion,
  privatization, aggregate replacement, or retirement, and opened only narrow
  adapter successors 240-242.
- 240 closed one AArch64 Route 3 same-block global-load memory/source identity
  adapter: `route3_agreed_same_block_global_load_access(...)` agrees
  `mir::find_bir_same_block_global_load_access_identity(...)` with
  `prepare::find_prepared_same_block_global_load_access(...)` before retaining
  the prepared emission path.
- 241 closed one AArch64 Route 5 current-block join-source adapter:
  `build_current_block_join_prepared_query_routing(...)` builds Route 5 edge
  join sources, verifies `current_block_join_route5_source_agrees_with_prepared(...)`,
  and lets `current_block_join_prepared_query_source(...)` consume
  `routing.sources[instruction_index]` only after agreement.
- 242 closed one AArch64 Route 6 call-result source-register adapter:
  `call_result_source_register_route6_evidence(...)` reads
  `bir::route6_find_call_result_source(...)` from
  `Route6CallUseSourceIndex::result_records`, requires agreement with
  `prepare::find_prepared_call_result_late_publication(...)` and the prepared
  value-home name, and otherwise falls back.

Current source/symbol surfaces inspected:

- `src/backend/prealloc/module.hpp`: AST-backed symbol inventory shows
  `c4c::backend::prepare::PreparedBirModule` at line 38. Fields include BIR
  module/name/control-flow state, target profile/route/invariants,
  value-locations, stack/frame/dynamic stack/addressing/liveness/regalloc,
  call/store-source/variadic/storage/carrier/atomic/intrinsic/inline-asm/runtime
  helper plans, completed phases, and notes. `BirPreAlloc::prepared()` and
  `prepare_*_bir_module_with_options(...)` remain public aggregate delivery
  surfaces.
- `src/backend/prealloc/prepared_lookups.hpp`: AST-backed symbol inventory
  shows `PreparedFunctionLookups` at line 15 with public groups
  `call_plans`, `address_materializations`, `memory_accesses`, `move_bundles`,
  `value_homes`, `edge_publications`, and
  `edge_publication_source_producers`; `make_prepared_function_lookups(...)`
  remains the public per-function lookup constructor.
- `src/backend/prealloc/prepared_lookups.cpp`: lookup construction still
  derives prepared dominance, value ids, and grouped caches from prepared
  control-flow, names, value locations, addressing, and publication/source
  helpers.
- `src/backend/mir/x86/x86.hpp`: `ConsumedPlans` retains frame, dynamic stack,
  control-flow, call, regalloc, storage, optional `PreparedFunctionLookups`,
  and optional `Route6CallUseSourceIndex`; `consume_plans(...)` still builds
  prepared lookups and a Route 6 call-use index side-by-side. The scalar `i32`
  argument helper requires Route 6 and prepared `source_value_id` agreement.
- `src/backend/mir/x86/prepared/prepared.hpp`: x86 prepared `Query` still reads
  prepared addressing, value locations, call plans, regalloc, storage, BIR
  function, formal publications, home/storage diagnostics, and edge-publication
  move intents as target-local policy/debug context.
- `src/backend/mir/riscv/codegen/emit.cpp`: riscv prepared emission rebuilds
  `PreparedFunctionLookups` per function with
  `make_prepared_function_lookups(...)`, then emits from
  `lookups.edge_publications.publications`; edge-publication move intent status
  uses prepared lookup/fallback statuses such as missing shared lookups, missing
  publication, unsupported publication, unsupported source home, and available.
- `src/backend/prealloc/prepared_printer.cpp`,
  `src/backend/prealloc/prepared_printer/value_locations.cpp`, and
  `src/backend/mir/x86/debug/prepared_route_debug.cpp.md`: prepared printer,
  route-debug, status rows, and human-readable route reports remain diagnostic
  and string-authority surfaces over `PreparedBirModule`.

Current blocker categories for Step 2:

- x86-only evidence cannot be generalized to riscv or to cross-target wrapper
  convergence.
- Whole-field and whole-lookup demotion/retirement remains blocked; only
  one-reader or one-row agreement-gated adapter candidates have evidence.
- Target policy must stay outside BIR: ABI placement, frame/register/stack,
  value homes/storage, addressing/materialization/relocation, move scheduling,
  helper/carrier protocols, formatting, instruction selection, emission, and
  wrapper output.
- Public prepared fallback, diagnostics, route-debug, helper-oracle names and
  status labels, expected strings, wrapper output, supported-path contracts, and
  baselines remain retained authority until replaced with equivalent proof.
- Duplicate semantic caches are deletion candidates only after the matching BIR
  facts are proven across x86 and riscv consumers, not merely AArch64 or x86
  consumers.

## Suggested Next

Execute Step 2: classify x86/riscv BIR fact parity for these concrete surfaces:
Route 3 memory/source identity (`memory_accesses`, same-block global-load,
load-local/source identity), Route 4/5 publication and edge/join identity
(`edge_publications`, `edge_publication_source_producers`, `routing.sources`),
Route 6 scalar call-use source identity (`call_plans`, `ConsumedPlans`,
`Route6CallUseSourceIndex` argument/result records), route/source/block/value
identity in `PreparedBirModule` names/control-flow/value-location fields,
diagnostic/oracle/fallback surfaces in prepared printer, x86 route-debug, helper
tests, and riscv/x86 target wrapper consumers.

## Watchouts

- This is analysis-only; do not implement adapter, demotion, or deletion work.
- Do not claim riscv readiness from x86-only evidence.
- Keep ABI, layout, register, stack, emission, formatting, and wrapper policy
  target-owned.
- Preserve strings, helper-oracle statuses, supported-path contracts, fallback
  behavior, route-debug output, wrapper output, and baseline expectations.
- Step 2 should distinguish portable BIR facts from target policy and from
  prepared public compatibility surfaces. The current riscv surface observed in
  `emit_prepared_module(...)` is prepared edge-publication consumption, not yet
  parity proof over the same semantic Route 3/5/6 facts.
- The closed 240-242 adapters are AArch64-selected readers; they provide
  agreement-gated patterns and blocker categories, not aggregate demotion or
  riscv parity.

## Proof

Analysis-only packet; no build or ctest required by delegated proof. Used
read-only inspection plus AST-backed `c4c-clang-tool list-symbols` for
`src/backend/prealloc/module.hpp` and
`src/backend/prealloc/prepared_lookups.hpp`. No `test_after.log` was generated
because this packet has no compile/test proof requirement and changed only
`todo.md`.
