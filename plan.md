# PreparedFunctionLookups Move Bundle Projection Runbook

Status: Active
Source Idea: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Continues: idea 175 after the `value_homes` aggregate field group was completed in commit `9dc99117c`.

## Purpose

Continue contracting direct `PreparedFunctionLookups` aggregate exposure one
projected field group at a time. The next selected group is `move_bundles`.

Goal: replace the AArch64 traversal read of `prepared_lookups.move_bundles`
with a narrow `PreparedMoveBundleLookups` projection, while preserving the
after-call result lane bindings that the aggregate builder currently publishes.

Core Rule: do not hide or replace the aggregate wholesale; this step only owns
the `move_bundles` projection and must not become a renamed lowering-plan
aggregate.

## Read First

- `ideas/open/175_prepared_function_lookups_aggregate_privacy.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`

## Current Target

- Selected field group: `move_bundles`
- Owner boundary: target-local `prepare::PreparedMoveBundleLookups` built from
  `prepare::make_prepared_move_bundle_lookups(...)` plus the existing
  after-call result lane binding publication behavior.
- Current direct aggregate read to remove:
  `function_context.move_bundle_lookups = &prepared_lookups.move_bundles` in
  `src/backend/mir/aarch64/codegen/traversal.cpp`
- Expected context projection: `FunctionLoweringContext::move_bundle_lookups`
- Required behavior to preserve:
  `module::find_prepared_after_call_result_lane_binding(...)` must keep seeing
  the lane bindings now populated inside `make_prepared_function_lookups(...)`.

## Remaining Aggregate Fields

- `memory_accesses`: still read by AArch64 aggregate consumers and should not
  be folded into this packet.
- `edge_publications` and `edge_publication_source_producers`: still tied to
  Route 4/Route 5/publication consumer migrations and should not be folded
  into this packet.
- `return_chains`: out of scope for idea 175; idea 176 owns return-chain
  owner/schema analysis.

## Non-Goals

- Do not remove `PreparedFunctionLookups` or
  `make_prepared_function_lookups(...)`.
- Do not migrate `return_chains`.
- Do not move memory-access, publication, source-producer, or return-chain
  consumers under this move-bundle packet.
- Do not change move-bundle semantics, move records, register allocation,
  storage plans, value-home policy, call ABI policy, or final target policy.
- Do not weaken tests or expectations to claim aggregate privacy progress.

## Working Model

- `PreparedFunctionLookups` remains aggregate cache wiring for fields not yet
  projected separately.
- `move_bundles` is a target/codegen value-location lookup, not a BIR semantic
  owner.
- A standalone move-bundle projection must include both the base move-bundle
  indexes and the after-call result lane binding index that call lowering
  queries through `FunctionLoweringContext::move_bundle_lookups`.
- The selected projection should be built beside the existing
  `call_plan_lookups`, `address_materialization_lookups`, and
  `value_home_lookups` projections and wired only through the existing
  `FunctionLoweringContext` pointer.

## Execution Rules

- Keep this packet behavior-preserving.
- Prefer a narrow helper or public builder surface in the prepared lookup
  family over duplicating lane-binding publication logic in traversal.
- Keep unselected aggregate fields and consumers unchanged.
- Contract includes only when the selected direct field read no longer requires
  them.
- If lane-binding publication must become public, expose only the narrow
  move-bundle projection operation needed by traversal; do not expose a generic
  aggregate-construction facade.
- If `FunctionLoweringContext`, aggregate type layout, or module/context
  projections change beyond the selected pointer assignment, request broader
  backend proof from the supervisor.

## Steps

### Step 1: Confirm selected move-bundle group

Goal: verify `move_bundles` is still the narrowest safe continuation after the
completed `value_homes` packet.

Actions:

- Confirm traversal still reads `prepared_lookups.move_bundles` only to
  populate `FunctionLoweringContext::move_bundle_lookups`.
- Confirm `prepare::make_prepared_move_bundle_lookups(...)` builds the base
  indexes needed by AArch64 lowering.
- Confirm the standalone projection also needs the current after-call result
  lane binding publication performed by
  `publish_prepared_after_call_result_lane_bindings(...)`.
- Confirm direct publication/source-producer, memory-access, and return-chain
  consumers remain outside this packet.

Completion check:

- `todo.md` names `move_bundles` as the selected field group and points
  execution at Step 2.

### Step 2: Project move-bundle lookups separately

Goal: provide the existing AArch64 context pointer without reading the selected
field from the aggregate.

Primary targets:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`

Actions:

- Add or expose a narrow builder path that returns a
  `prepare::PreparedMoveBundleLookups` with the same base indexes and
  after-call result lane bindings currently produced inside
  `make_prepared_function_lookups(...)`.
- Build a local `PreparedMoveBundleLookups` value in traversal using that
  narrow path.
- Assign `function_context.move_bundle_lookups` from that local projection.
- Keep `function_context.prepared_lookups` and all unselected aggregate field
  use unchanged.
- Preserve object lifetimes for the full function-lowering loop, matching the
  local `prepared_call_plan_lookups`,
  `prepared_address_materialization_lookups`, and
  `prepared_value_home_lookups` patterns.

Completion check:

- Production traversal no longer reads `prepared_lookups.move_bundles`.
- After-call result lane binding queries still observe the same indexed
  records through `FunctionLoweringContext::move_bundle_lookups`.
- A compile proof plus the selected AArch64 call/value-location lowering subset
  is green.

### Step 3: Contract selected direct dependency exposure

Goal: remove avoidable direct aggregate dependency tied only to the selected
field group.

Primary targets:

- `src/backend/mir/aarch64/codegen/traversal.cpp`
- Any selected local include made unnecessary by Step 2

Actions:

- Remove only includes or direct aggregate dependencies made unnecessary by the
  selected projection.
- Keep `prepared_lookups.hpp` exposure where traversal or other AArch64
  consumers still need unselected fields.
- Treat fixture aggregate-field assignments as test compatibility unless the
  supervisor explicitly delegates cleanup.

Completion check:

- No selected production consumer reads the contracted aggregate field
  directly.
- A compile proof demonstrates the contraction is behavior-preserving.

### Step 4: Prove the selected move-bundle subset and decide next

Goal: validate the selected projection and hand lifecycle control back for the
next group decision.

Actions:

- Run the supervisor-selected proof command for AArch64 move-bundle, call, and
  value-location lowering.
- If module/context projection changed more broadly than Step 2 requires, run
  the broader backend check selected by the supervisor.
- Update `todo.md` with proof results, remaining direct aggregate fields, and
  whether the next field group can continue under idea 175.

Completion check:

- The selected field group has fresh proof.
- Remaining aggregate exposure is explicitly listed as selected-next,
  blocked-by-route, target-local, or return-chain.
