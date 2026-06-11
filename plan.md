# PreparedFunctionLookups Address Materialization Projection Runbook

Status: Active
Source Idea: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Continues: idea 175 after the `call_plans` aggregate field group was completed in commit `f1b5881d5`.

## Purpose

Continue contracting direct `PreparedFunctionLookups` aggregate exposure one
projected field group at a time. The next selected group is
`address_materializations`.

Goal: replace the AArch64 traversal read of
`prepared_lookups.address_materializations` with a narrow
`PreparedAddressMaterializationLookups` projection, while leaving the aggregate
available for all unselected fields.

Core Rule: do not hide or replace the aggregate wholesale; this step only owns
the `address_materializations` projection and must not become a renamed
lowering-plan aggregate.

## Read First

- `ideas/open/175_prepared_function_lookups_aggregate_privacy.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`

## Current Target

- Selected field group: `address_materializations`
- Owner boundary: target-local `prepare::PreparedAddressMaterializationLookups`
  built by `prepare::make_prepared_address_materialization_lookups(...)`
- Current direct aggregate read to remove:
  `function_context.address_materialization_lookups =
  &prepared_lookups.address_materializations` in
  `src/backend/mir/aarch64/codegen/traversal.cpp`
- Expected context projection:
  `FunctionLoweringContext::address_materialization_lookups`

## Remaining Aggregate Fields

- `move_bundles`: target-local value-location lookup projection; leave for a
  later group.
- `value_homes`: target-local value-location lookup projection; leave for a
  later group.
- `memory_accesses`: still read through broader aggregate consumers; do not
  fold into this packet.
- `edge_publications` and `edge_publication_source_producers`: still tied to
  Route 4/Route 5/publication consumer migrations; do not fold into this
  packet.
- `return_chains`: out of scope for idea 175; idea 176 owns return-chain
  owner/schema analysis.

## Non-Goals

- Do not remove `PreparedFunctionLookups` or
  `make_prepared_function_lookups(...)`.
- Do not migrate `return_chains`.
- Do not move memory-access, publication, source-producer, move-bundle, or
  value-home consumers under this address-materialization packet.
- Do not copy `PreparedAddress` or stack-layout/frame payloads into BIR.
- Do not weaken tests or expectations to claim aggregate privacy progress.

## Working Model

- `PreparedFunctionLookups` remains aggregate cache wiring for fields not yet
  projected separately.
- `address_materializations` is a target-local addressing/layout lookup, not a
  BIR semantic owner.
- The selected projection should be built beside the existing
  `call_plan_lookups` projection and wired only through the existing
  `FunctionLoweringContext` pointer.

## Execution Rules

- Keep this packet behavior-preserving.
- Prefer the existing builder in `addressing.hpp`; do not add a generic facade.
- Keep unselected aggregate fields and consumers unchanged.
- Contract includes only when the selected direct field read no longer requires
  them.
- If `FunctionLoweringContext`, aggregate type layout, or module/context
  projections change beyond the selected pointer assignment, request broader
  backend proof from the supervisor.

## Steps

### Step 1: Confirm selected address-materialization group

Goal: verify the chosen group is still the narrowest safe continuation after
the completed `call_plans` packet.

Actions:

- Confirm traversal still reads
  `prepared_lookups.address_materializations` only to populate
  `FunctionLoweringContext::address_materialization_lookups`.
- Confirm `prepare::make_prepared_address_materialization_lookups(...)` is
  available as the narrow builder.
- Confirm direct publication/source-producer, memory-access, move-bundle,
  value-home, and return-chain consumers remain outside this packet.

Completion check:

- `todo.md` names `address_materializations` as the selected field group and
  points execution at Step 2.

### Step 2: Project address-materialization lookups separately

Goal: provide the existing AArch64 context pointer without reading the selected
field from the aggregate.

Primary targets:

- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/prealloc/addressing.hpp`

Actions:

- Build a local `PreparedAddressMaterializationLookups` value with
  `prepare::make_prepared_address_materialization_lookups(...)`.
- Assign `function_context.address_materialization_lookups` from that local
  projection.
- Keep `function_context.prepared_lookups` and all unselected aggregate field
  use unchanged.
- Preserve object lifetimes for the full function-lowering loop, matching the
  local `prepared_call_plan_lookups` pattern.

Completion check:

- Production traversal no longer reads
  `prepared_lookups.address_materializations`.
- A compile proof plus the selected memory/addressing subset is green.

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

### Step 4: Prove the selected address-materialization subset and decide next

Goal: validate the selected projection and hand lifecycle control back for the
next group decision.

Actions:

- Run the supervisor-selected proof command for AArch64 address/materialization
  lowering.
- If module/context projection changed more broadly than Step 2 requires, run
  the broader backend check selected by the supervisor.
- Update `todo.md` with proof results, remaining direct aggregate fields, and
  whether the next field group can continue under idea 175.

Completion check:

- The selected field group has fresh proof.
- Remaining aggregate exposure is explicitly listed as selected-next,
  blocked-by-route, target-local, or return-chain.
