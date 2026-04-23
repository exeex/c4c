# Prepared Local-Slot Handoff Consumption For X86 Backend

Status: Active
Source Idea: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md
Activated from: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md

## Purpose

Make the x86 prepared emitter consume authoritative prepared local-slot helper
and continuation handoff facts as a normal backend capability instead of
rejecting multi-block helper routes after they already clear semantic lowering
and prepared-module traversal.

## Goal

Replace authoritative prepared local-slot handoff rejection with one generic
prepared helper plan that `prepared_local_slot_render.cpp` can consume without
reconstructing topology or testcase shape.

## Core Rule

Fix this by consuming or extending the prepared local-slot/control-flow
contract, not by adding one more helper-shaped x86 matcher.

## Read First

- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `test_after.log`

## Scope

- authoritative prepared local-slot helper or continuation handoff failures in
  the x86 backend
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- shared prepared control-flow or value-home surfaces only when the current
  stored contract is insufficient for a generic local-slot helper plan

## Non-Goals

- short-circuit or guard-chain ownership gaps that still belong to idea 59
- scalar matcher restrictions that still belong to idea 60
- prepared-module or call-bundle traversal work that still belongs to idea 61
- trace-only observability work or runtime correctness follow-ons
- testcase-shaped x86 fast paths for one named failure

## Working Model

- treat repeated `authoritative prepared local-slot` failures as one family,
  not as isolated testcase repairs
- separate true local-slot handoff misses from routes that really belong to
  guard-chain, short-circuit, scalar, or prepared-module leaves before editing
  code
- prefer `build -> focused backend subset -> broader x86 backend recheck`
  whenever a packet changes shared prepared-contract consumers

## Execution Rules

- start each packet by confirming the failing route is still owned by idea 68
- keep `prepared_local_slot_render.cpp` moving toward normalized helper-plan
  consumption rather than inline block-shape recovery
- if the current prepared contract cannot express the needed helper plan,
  extend shared prepared structures before growing x86-local branching
- graduate cases out of this idea as soon as their top-level blocker moves to
  idea 59, 60, 61, or a later downstream runtime leaf
- preserve `todo.md` as the packet log; only rewrite this runbook when the
  route itself changes

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
- choose one coherent same-family subset whose failing helper route shares one
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
