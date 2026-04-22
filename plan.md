# Convert Reviewed X86 Codegen Drafts To Implementation For Phoenix Rebuild

Status: Active
Source Idea: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Activated after closing: ideas/closed/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md

## Purpose

Turn the reviewed stage-3 x86 codegen draft package into real implementation
under `src/backend/mir/x86/codegen/` without collapsing the new ownership
graph back into the legacy mixed route.

## Goal

Land staged implementation slices that make the reviewed seams real, keep the
legacy path available until the new owners are proved, and leave the backend
closer to retiring the old subsystem shape.

## Core Rule

Do not change the reviewed draft contract during this runbook without first
repairing stage 3. Implementation should follow
`docs/backend/x86_codegen_rebuild/`, not improvise new ownership seams.

## Read First

- `ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md`
- `ideas/closed/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md`
- `docs/backend/x86_codegen_rebuild/index.md`
- `docs/backend/x86_codegen_rebuild/layout.md`
- `docs/backend/x86_codegen_rebuild/review.md`

## Scope

- real replacement `.cpp` / `.hpp` implementation under
  `src/backend/mir/x86/codegen/`
- staged migration of shared seams, canonical lowering families, prepared
  adapters, and dispatch ownership
- explicit classification or retirement of legacy files only after the new
  owners are live and proved

## Non-Goals

- changing the reviewed stage-3 ownership map without lifecycle repair
- deleting legacy routes before the replacement owners actually handle the
  migrated behavior
- claiming backend correctness beyond the capability slices proved by each
  packet

## Working Model

- treat `docs/backend/x86_codegen_rebuild/` as the implementation contract
- keep migration order `shared seams -> lowering families -> prepared rewiring
  -> legacy retirement`
- preserve a compileable route at each step, even if some legacy forwarding
  stays temporarily in place
- prefer small packets that move one coherent responsibility at a time and
  prove the affected backend scope

## Execution Rules

- keep new headers narrow; do not recreate `x86_codegen.hpp` under a new name
- route prepared behavior through canonical lowering seams rather than new
  matcher-shaped helpers
- when a packet introduces a new real owner, classify the leftover legacy code
  immediately instead of letting both paths silently drift
- prefer `build -> narrow backend proof`, then escalate to broader validation
  when the blast radius or milestone status justifies it

## Step 1: Materialize Shared Seam Scaffolding And Keep Legacy Route Compiling

Goal: create the real shared replacement headers and implementation scaffolds
for the reviewed `api`, `core`, `abi`, and `module` seams while preserving a
buildable backend.

Primary targets:

- `src/backend/mir/x86/codegen/`
- the reviewed `api/`, `core/`, `abi/`, and `module/` draft contracts

Actions:

- add the replacement shared seam files declared by the reviewed draft package
- move or wrap existing shared helpers behind the new file boundaries
- keep the old route compiling while the new seams are introduced

Completion check:

- the shared replacement seams exist in real source form and the backend still
  builds with the legacy route intact

## Step 2: Migrate Canonical Lowering Families Into Reviewed Owners

Goal: move frame, call, return, memory, comparison, scalar, float, and
atomics/intrinsics behavior behind the reviewed canonical lowering seams.

Primary targets:

- `src/backend/mir/x86/codegen/`
- the reviewed `lowering/` draft contracts

Actions:

- migrate one coherent lowering family at a time into its reviewed owner
- keep ABI, output, and frame dependencies flowing through the new shared seams
- prove each migrated family with the smallest backend subset that credibly
  exercises the moved behavior

Completion check:

- the major canonical lowering families exist behind the reviewed seam names
  and the migrated capability slices still pass their proving scope

## Step 3: Rewire Prepared Adapters And Debug Surfaces Over Canonical Seams

Goal: turn the prepared route into bounded adapters over the canonical owners.

Primary targets:

- prepared-route implementation under `src/backend/mir/x86/codegen/`
- the reviewed `prepared/` and `debug/` draft contracts

Actions:

- replace local-slot and prepared matcher ownership with query-shaped adapter
  calls into frame, call, memory, and comparison owners
- keep route-debug surfaces observational while they explain admission and
  fallback facts
- reject testcase-shaped matcher growth that would recreate the old route

Completion check:

- prepared fast paths consume canonical seams instead of owning local copies of
  frame, call-lane, address, or predicate policy

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
