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

### Step 1.1: Land Replacement `api` And `core` File Boundaries

Goal: make the reviewed `api/` and `core/` seams real in source form without
changing who owns lowering behavior yet.

Primary targets:

- `src/backend/mir/x86/codegen/api/`
- `src/backend/mir/x86/codegen/core/`

Actions:

- add the reviewed `api/` and `core/` headers and source files that define the
  replacement surface area
- move declarations and light helper wiring out of legacy mixed headers into
  the new file boundaries
- keep legacy entrypoints compiling through forwarding or wrapper includes
  where needed

Completion check:

- the reviewed `api/` and `core/` seam files exist and compile without forcing
  public entrypoint rewiring yet

### Step 1.2: Extract Shared Core Helpers Behind Narrow Owners

Goal: peel reusable shared state, helper logic, and common queries behind the
reviewed `core/` owners instead of leaving them fused into legacy orchestration
files.

Primary targets:

- shared helper-heavy files under `src/backend/mir/x86/codegen/`
- reviewed `core/` implementation seams

Actions:

- move one coherent helper family at a time behind the reviewed `core/`
  boundaries
- replace broad `x86_codegen.hpp` reach-throughs with narrower includes and
  local adapters where possible
- preserve behavior by keeping legacy callers routed through the new helpers
  until later steps rewire ownership

Completion check:

- shared helper families start living behind reviewed `core/` owners and the
  backend still builds through both old and new call paths

### Step 1.3: Stand Up The Reviewed `abi` Support Surface

Goal: materialize the reviewed `abi/` seam so calling-convention and data-layout
support stop depending on ad hoc legacy header reach-through.

Primary targets:

- `src/backend/mir/x86/codegen/abi/`
- legacy ABI-related helper surfaces that should forward into the new seam

Actions:

- add the reviewed `abi/` replacement files and wire them into the build
- move or wrap existing ABI support helpers behind the new seam names
- keep consumers compiling through compatibility forwarding until canonical
  lowering families migrate in step 2

Completion check:

- the reviewed `abi/` seam is present in real source form and legacy callers
  can still compile through transitional forwarding

### Step 1.4: Materialize Module Support Seams While Preserving Compatibility Wrappers

Goal: move whole-module support helpers behind the reviewed `module/` seams
while keeping the legacy prepared-module route as a thin compatibility layer.

Primary targets:

- `src/backend/mir/x86/codegen/module/`
- compatibility wrappers that still expose legacy module entrypoints

Actions:

- extract concrete module helper families into reviewed `module/` owners one
  packet at a time
- keep `prepared_module_emit.cpp` and similar legacy module surfaces wrapper-thin
  instead of letting orchestration drift back into compatibility files
- preserve legacy symbol contracts until step 4 explicitly rewires module entry
  and dispatch ownership

Completion check:

- module support helpers live behind reviewed `module/` seams and compatibility
  wrappers remain thin while the backend still builds

#### Step 1.4.1: Extract Bounded Same-Module Support Helpers Out Of Top-Level Module Emission

Goal: move helper-prefix, bounded-helper activation, and same-module contract
support out of top-level module emission flow and into a reviewed
`module/`-owned helper cluster.

Primary targets:

- `src/backend/mir/x86/codegen/module/module_emit.cpp`
- bounded same-module helper support used by prepared multi-function emission

Actions:

- extract helper-prefix and bounded-helper support from
  `emit_prepared_module_text(...)` into local `module/`-owned helpers
- keep helper-set selection and contract-detail annotation routed through the
  new helper cluster instead of top-level orchestration lambdas
- preserve the compatibility wrapper surface without changing public entrypoint
  ownership

Completion check:

- top-level module emission delegates bounded same-module support to a local
  helper owner and the legacy wrapper remains compatibility-thin

#### Step 1.4.2: Extract Function-Level Dispatch Context Assembly Behind Module Owners

Goal: peel the next bounded support cluster out of
`render_defined_function(...)` so function-local dispatch setup and prepared
return/home-move scaffolding stop living inline in module orchestration.

Primary targets:

- `src/backend/mir/x86/codegen/module/module_emit.cpp`
- function-level module support helpers adjacent to reviewed `module/` seams

Actions:

- extract one coherent helper family from `render_defined_function(...)`,
  starting with dispatch-context assembly or prepared return/home-move support
- keep the extracted logic owned by `module/` helpers rather than re-expanding
  legacy compatibility files
- preserve existing lowering decisions while narrowing module-orchestration
  bodies

Completion check:

- `render_defined_function(...)` delegates a concrete function-support cluster
  to reviewed `module/`-owned helpers and still compiles through the current
  route

#### Step 1.4.3: Classify Remaining Module-Orchestration Support Between `module_emit` And `module_data_emit`

Goal: make the remaining step-1 module support boundary explicit so module
orchestration, data publication, and compatibility forwarding do not drift
back together.

Primary targets:

- `src/backend/mir/x86/codegen/module/module_emit.cpp`
- `src/backend/mir/x86/codegen/module/module_data_emit.cpp`
- remaining compatibility wrappers under `src/backend/mir/x86/codegen/`

Actions:

- move or classify any leftover module-local support helpers so data and symbol
  publication stay behind `module_data_emit.*`
- keep `module_emit.*` focused on whole-module orchestration and delegation
- record any remaining compatibility wrappers explicitly instead of leaving
  mixed ownership ambiguous

Completion check:

- module-support responsibilities are explicitly split between
  `module_emit.*`, `module_data_emit.*`, and thin compatibility wrappers
  without silent ownership overlap

### Step 1.5: Audit Transitional Forwarding And Buildability Across Shared Seams

Goal: leave step 1 with explicit transitional forwarding and a buildable backend
instead of an ambiguous mix of new files and silent legacy reach-through.

Primary targets:

- transitional wrappers and forwarding points under `src/backend/mir/x86/codegen/`
- build wiring for the reviewed shared seams

Actions:

- classify remaining shared-seam legacy files as forwarding, compatibility, or
  still-pending extraction points
- remove accidental duplicate ownership created during seam materialization
- prove the backend still builds before step 2 starts moving canonical lowering
  families

Completion check:

- shared-seam forwarding is explicit, duplicate ownership is not silently
  growing, and the backend still builds with the legacy route intact

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
