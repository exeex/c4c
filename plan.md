## Prepared Local-Slot Handoff Consumption For X86 Backend

Status: Active
Source Idea: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md
Activated from: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md

## Purpose

Make the x86 prepared emitter consume authoritative prepared local-slot
handoff facts as a normal capability instead of rejecting multi-block helper
routes after they already clear semantic lowering and module traversal.

## Goal

Replace repeated local-slot handoff rejection with one generic prepared helper
plan that `prepared_local_slot_render.cpp` can consume without reconstructing
topology or testcase shape.

## Core Rule

Fix this by consuming or extending the prepared local-slot and CFG contract,
not by adding one more helper-shaped x86 matcher.

## Read First

- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `test_after.log`

## Scope

- authoritative prepared local-slot handoff failures in the x86 backend
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- shared prepared control-flow or value-home surfaces only when the current
  stored contract is insufficient for a generic local-slot helper plan

## Non-Goals

- short-circuit or guard-chain ownership gaps that still belong to idea 59
- scalar matcher restrictions that still belong to idea 60
- prepared-module or call-bundle traversal work that still belongs to idea 61
- trace-only observability work or runtime correctness follow-ons

## Working Model

- treat repeated `authoritative prepared local-slot` failures as one family,
  not as isolated testcase repairs
- separate true local-slot handoff misses from cases that really belong to
  guard-chain, short-circuit, or prepared-module leaves before editing code
- prefer `build -> focused backend subset -> broader x86 backend recheck`
  for each packet

## Execution Rules

- start each packet by confirming the failing route is still owned by idea 68
- keep `prepared_local_slot_render.cpp` moving toward normalized helper-plan
  consumption rather than inline block-shape recovery
- if the existing prepared contract cannot express the needed helper plan,
  extend shared prepared structures before growing x86-local branching
- graduate cases out of this idea as soon as their top-level blocker moves to
  idea 59, 60, 61, or a later runtime leaf

## Step 1: Re-establish The Current Owned Failure Inventory

Goal: confirm which current `c_testsuite x86_backend` failures really stop in
the authoritative prepared local-slot family before taking a new packet.

Primary targets:

- `test_after.log`
- focused `c_testsuite x86_backend` cases that currently report the
  authoritative prepared local-slot diagnostic

Actions:

- collect the current named cases that fail in the authoritative prepared
  local-slot family from `test_after.log`
- separate out nearby guard-chain, short-circuit, scalar, and prepared-module
  cases that should stay with ideas 59, 60, or 61
- choose a coherent same-family subset whose failing helper route shares one
  local-slot handoff shape

Completion check:

- the active subset is explicitly owned by idea 68 and does not mix in cases
  that belong to adjacent backend leaves

## Step 2: Normalize The Needed Local-Slot Handoff Contract

Goal: identify the minimal prepared value/control-flow facts required to
render the owned helper route generically.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- prepared control-flow and value-home structures or helpers consumed by that
  renderer

Actions:

- trace the owned helper route to the point where local-slot rendering still
  re-derives meaning from raw block shape, continuation shape, or inline home
  inspection
- map that missing meaning onto existing prepared local-slot, control-flow, or
  value-home facts when possible
- if the stored contract is genuinely insufficient, define the missing shared
  helper or struct extension before changing x86 rendering logic

Completion check:

- there is one explicit generic local-slot helper plan or a clearly bounded
  shared-contract addition that explains the owned failure subset

## Step 3: Rewire Local-Slot Rendering To Consume The Generic Plan

Goal: make `prepared_local_slot_render.cpp` render from the normalized
local-slot helper plan instead of rejecting the owned helper route.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- any shared helper or prepared-contract consumer introduced in step 2

Actions:

- move the owned local-slot handoff decision-making behind the normalized plan
- keep x86-side logic focused on rendering from authoritative prepared facts
  rather than reopening topology or testcase matching
- preserve clean boundaries with idea 59 guard-chain/short-circuit ownership
  and idea 61 prepared-module ownership

Completion check:

- the owned local-slot subset emits through the generic plan without regrowing
  helper-shaped special cases in the x86 renderer

## Step 4: Prove The Family And Re-route Graduated Cases Honestly

Goal: show the repaired local-slot family moves forward and record cases that
now belong to a downstream leaf instead of keeping them here.

Primary targets:

- the focused owned subset from step 1
- broader `c_testsuite x86_backend` coverage when the packet blast radius
  justifies it

Actions:

- run build proof plus the chosen focused backend subset for the repaired
  local-slot family
- rerun a broader backend check when the packet advances more than one helper
  lane or changes shared prepared contract
- update `todo.md` so graduated cases are routed to idea 59, 60, 61, or a
  later downstream leaf instead of being counted as idea-68 wins

Completion check:

- the packet proves real progress for the authoritative prepared local-slot
  family, and any downstream blockers are explicitly re-homed instead of being
  absorbed into this idea
- preserve existing debug/admission output while ending transitional ownership
  of operand/home policy

Completion check:

- stack/local-slot declaration holdouts have moved behind owned seams, and the
  remaining prepared debug/admission surfaces only observe canonical decisions

## Step 4: Shift Module Entry And Legacy Dispatch To The Replacement Graph

Goal: make the reviewed replacement ownership graph the live path while
classifying any remaining legacy forwarding explicitly.

Primary targets:

- public entrypoints and module orchestration in `src/backend/mir/x86/codegen/`
- remaining legacy handoff points

Actions:

- route public entrypoints through the new `api` and `module` owners
- classify leftover legacy files as forwarding, compatibility, or retirement
  candidates
- prove that the active path really flows through the replacement seams

Completion check:

- the reviewed replacement graph is the live dispatch path for the migrated
  capability families and leftover legacy roles are explicit

### Step 4.1: Rewire X86-Only Backend Entry Surfaces To Explicit Replacement Owners

Goal: finish separating x86-only backend entrypoints from shared compatibility
names so the live x86 route is visible in the reviewed `api` and `module`
surfaces.

Primary targets:

- public x86-facing entrypoints under `src/backend/mir/x86/codegen/`
- backend-facing x86 target wrappers that still route through generic
  compatibility names

Actions:

- audit the remaining x86-only backend entrypoints for generic names that hide
  replacement ownership
- rewire x86-only callers to explicit replacement-owned entry surfaces such as
  `emit_x86_bir_module_entry(...)` where that ownership is already live
- preserve honest non-x86 compatibility call sites instead of pretending they
  are x86-specific dispatch

Completion check:

- x86-only backend entry surfaces route through explicit replacement owners,
  while shared compatibility names remain only where cross-target contracts
  still require them

### Step 4.2: Classify Residual Target Shims And Legacy Forwarding Roles

Goal: make the remaining target shims and legacy x86 forwarding paths explicit
so step 4 stops accumulating ambiguous compatibility behavior.

Primary targets:

- target-wrapper shims adjacent to x86 backend dispatch
- residual forwarding files under `src/backend/mir/x86/codegen/`

Actions:

- classify the leftover target shims as replacement-owned dispatch,
  compatibility forwarding, or later retirement candidates
- keep generic wrapper surfaces thin instead of letting x86 orchestration drift
  back into shared names
- record any intentional holdouts through explicit classification rather than
  silent mixed ownership

Completion check:

- the remaining target shims and legacy forwarding files have explicit roles,
  and compatibility wrappers stay thin instead of regaining live x86 ownership

### Step 4.3: Prove The Live Replacement Dispatch Path Before Legacy Retirement

Goal: confirm that the active backend route really flows through the reviewed
replacement graph before step 5 starts deleting or retiring residual legacy
shape.

Primary targets:

- backend/front-door proof coverage for x86 dispatch ownership
- module-entry and target-wrapper flows touched by step 4 rewiring

Actions:

- extend or tighten backend proof where needed so the active x86 route is
  observed through the explicit replacement entry surfaces
- verify the narrowed compatibility wrappers still preserve honest non-x86
  contracts
- leave step-4 proof artifacts ready for the broader validation expected in
  step 5

Completion check:

- backend proof shows the live x86 dispatch path flowing through the reviewed
  replacement owners, and remaining compatibility use is explicit before step 5

## Step 5: Retire Or Classify Residual Legacy Shape And Run Broader Validation

Goal: clean up the remaining legacy surface honestly and confirm the migrated
graph with broader proof.

Primary targets:

- residual legacy `src/backend/mir/x86/codegen/` files
- broader backend validation scope

Actions:

- delete or clearly classify legacy files only after the new owner is in use
- run broader validation appropriate for the migrated blast radius
- record any intentionally deferred legacy pockets explicitly instead of
  leaving them ambiguous

Completion check:

- the migrated ownership graph is explicit in live code, the remaining legacy
  surface is honestly classified, and broader proof supports the current
  milestone
