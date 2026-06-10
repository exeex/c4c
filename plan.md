# PreparedFunctionLookups Aggregate Privacy Runbook

Status: Active
Source Idea: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Activated from: idea 175 after idea 171 retired its Route 5 runbook and idea 177 closed the ambient AArch64 f64 readback dispatch debt.

## Purpose

Contract direct `PreparedFunctionLookups` aggregate exposure one projected field
group at a time, after each selected group has a route-owned BIR replacement or
a target-local owner.

Goal: replace one direct aggregate field consumer group with a narrow projection
and prove the include/context contraction is behavior-preserving.

Core Rule: do not hide or replace the aggregate wholesale; each removed field
consumer must have a real owner boundary, not a renamed lowering-plan aggregate.

## Read First

- `ideas/open/175_prepared_function_lookups_aggregate_privacy.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`

## Current Targets

- `prepare::PreparedFunctionLookups`
- `prepare::make_prepared_function_lookups(...)`
- AArch64 `FunctionLoweringContext` aggregate and domain lookup pointers
- One projected field group selected from the aggregate fields:
  `call_plans`, `address_materializations`, `memory_accesses`, `move_bundles`,
  `value_homes`, `edge_publications`, or
  `edge_publication_source_producers`

## Non-Goals

- Do not remove `PreparedFunctionLookups` before all projected fields have
  route-owned or target-local replacements.
- Do not hide or migrate `return_chains` through this runbook; idea 176 owns
  return-chain owner/schema analysis.
- Do not create a new BIR lowering-plan aggregate or generic facade clone.
- Do not sweep unrelated context/module rewrites into an aggregate privacy
  slice.
- Do not weaken tests or expectations to claim cache contraction.

## Working Model

- `PreparedFunctionLookups` is aggregate cache wiring, not the semantic owner of
  every field it projects.
- AArch64 traversal may continue building the aggregate while individual
  consumers are moved to narrower projections.
- Route-owned semantic facts stay in their typed route records/indexes.
- Target-local facts such as homes, move bundles, addressing layout, call ABI,
  stack layout, and publication emission policy stay outside BIR.

## Execution Rules

- Select one projected field group before editing implementation code.
- Prefer consumers already close to a route-owned BIR index or an existing
  target-local `FunctionLoweringContext` pointer.
- Keep the aggregate available for unselected fields.
- Keep source idea edits unnecessary unless source intent genuinely changes.
- For code-changing steps, prove with build or compile proof plus the selected
  migrated route subset.
- Escalate to broad backend regression when the aggregate type,
  `FunctionLoweringContext`, or module/context projection changes.

## Steps

### Step 1: Inventory aggregate field consumers and choose the first group

Goal: choose one direct aggregate field group that can be contracted without
crossing idea 175 boundaries.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- AArch64 consumers that read `prepared_lookups->...`

Actions:

- List direct aggregate field reads by field group.
- Classify each group as route-owned, target-local, return-chain, or still
  blocked.
- Reject `return_chains` for this runbook and leave it to idea 176.
- Pick the smallest group with an existing owner boundary and a narrow proof
  command.
- Record the chosen group and proof route in `todo.md` before implementation.

Completion check:

- `todo.md` names one selected field group, its owner boundary, direct consumer
  files, and the exact proof command the supervisor should delegate to the
  executor.

### Step 2: Add or expose the narrow projection for the selected group

Goal: provide the selected consumers with a narrow BIR-backed or target-local
projection that does not require reading the aggregate field directly.

Primary targets:

- The selected consumer files from Step 1
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- The selected route-owned or target-local lookup header

Actions:

- Wire the selected projection through `FunctionLoweringContext` only as far as
  the chosen consumers need it.
- Avoid adding semantic payload copies or a generic aggregate replacement.
- Keep unselected aggregate fields and consumers unchanged.
- Preserve existing fallback behavior unless the selected owner already
  provides fail-closed answers.

Completion check:

- The selected consumers no longer need the direct aggregate field read.
- The code builds, and the narrow selected subset remains green.

### Step 3: Contract includes and direct aggregate dependency for the selected group

Goal: remove avoidable `prepared_lookups.hpp` exposure tied to the selected
field group without disturbing unrelated fields.

Primary targets:

- Selected consumer includes
- `src/backend/mir/aarch64/module/module.hpp`
- Any local helper header that can now depend on the narrower owner

Actions:

- Remove only includes and aggregate pointer dependencies made unnecessary by
  Step 2.
- Keep compatibility attachment helpers in tests only where the aggregate is
  still required for other fields.
- Do not claim aggregate privacy from helper renames alone.

Completion check:

- A compile proof demonstrates the include/context contraction is
  behavior-preserving.
- No selected consumer reads the contracted aggregate field directly.

### Step 4: Prove the selected route subset and decide the next lifecycle move

Goal: validate the contracted field group and decide whether another group can
continue under this runbook.

Actions:

- Run the selected narrow subset from Step 1.
- If `FunctionLoweringContext`, the aggregate type, or module projections
  changed, run a broader backend regression selected by the supervisor.
- Update `todo.md` with proof results, remaining fields, and whether the next
  executor packet should select another field group or request plan review.

Completion check:

- The selected field group has fresh proof.
- Remaining aggregate exposure is explicitly listed as selected-next,
  blocked-by-route, target-local, or return-chain.
