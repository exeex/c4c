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

#### Step 1.5.1: Classify Transitional Wrapper Surfaces Across Shared Seams

Goal: make the reviewed `api/`, `module/`, and top-level compatibility wrappers
explicit so step-1 seam materialization does not quietly re-expand mixed-owner
entrypoints.

Primary targets:

- legacy forwarding surfaces under `src/backend/mir/x86/codegen/`
- top-level wrapper entrypoints that should stay compatibility-thin

Actions:

- classify wrapper-heavy shared-seam files as forwarding, compatibility, or
  still-pending extraction points
- keep reviewed `api/` and `module/` owners real while preserving the legacy
  public symbol contracts
- prevent top-level orchestration from drifting back into wrapper files during
  shared-seam materialization

Completion check:

- compatibility wrappers are explicit and thin, and reviewed shared seams own
  the extracted responsibility they already absorbed

#### Step 1.5.2: Narrow Reviewed Shared-Seam Consumers To Their Real Owners

Goal: replace accidental `x86_codegen.hpp` reach-through for reviewed
`api/`, `abi/`, `module/`, and route-debug surfaces with the narrow owners that
now exist in source form.

Primary targets:

- reviewed shared-seam consumers under `src/backend/mir/x86/codegen/`
- backend tests that only need declarations now owned by narrow reviewed seams

Actions:

- move reviewed-seam consumers onto narrow includes when the needed owner is
  already live
- keep helper-heavy or still-unowned dependencies on `x86_codegen.hpp`
  explicitly classified instead of faking narrower ownership
- preserve buildability while reducing duplicate declaration ownership

Completion check:

- consumers that only need live reviewed shared seams no longer depend on broad
  header reach-through, and remaining broad-header use is explicit about why it
  still exists

#### Step 1.5.3: Audit Remaining Broad-Header Holdouts Before Lowering Migration

Goal: finish step-1 forwarding audit by separating real prepared or
still-unowned compatibility dependencies from any remaining shared-seam
consumers that can already move off `x86_codegen.hpp`.

Primary targets:

- remaining `x86_codegen.hpp` consumers under `src/backend/mir/x86/codegen/`
- backend tests and helper entrypoints that still rely on broad-header staging

Actions:

- audit the remaining broad-header consumers and group them by reviewed-owner
  narrowing opportunity versus honest compatibility holdout
- annotate prepared-route or later-step dependencies so step 2 does not inherit
  ambiguous broad-header usage
- keep the backend building while recording which holdouts stay intentionally
  staged for the later `prepared/` and lowering conversions

Completion check:

- remaining `x86_codegen.hpp` usage is either narrowed to a live reviewed owner
  or explicitly classified as a real staged compatibility dependency before
  step 2 begins

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

### Step 2.1: Stand Up Frame And Memory Lowering Owners

Goal: make the reviewed `lowering/frame_lowering.*` and
`lowering/memory_lowering.*` seams real first so later lowering and prepared
packets can query canonical frame-home and operand services instead of
continuing to reach through legacy mixed files.

Primary targets:

- `src/backend/mir/x86/codegen/lowering/`
- legacy frame and memory helpers under `src/backend/mir/x86/codegen/`

Actions:

- add the reviewed frame and memory lowering headers and source files
- move one coherent frame-home, stack-layout, or memory-operand helper family
  at a time behind those owners
- keep legacy and prepared callers compiling through transitional forwarding
  instead of re-expanding mixed ownership

Completion check:

- canonical frame and memory services live behind reviewed `lowering/` owners
  and existing callers use explicit forwarding instead of broad hidden
  reach-through

#### Step 2.1.1: Materialize Reviewed Frame And Memory Lowering File Boundaries

Goal: make the reviewed `frame_lowering.*` and `memory_lowering.*` seams exist
in real source form with explicit compatibility forwarding instead of leaving
frame and memory ownership fused into legacy mixed files.

Primary targets:

- `src/backend/mir/x86/codegen/lowering/frame_lowering.*`
- `src/backend/mir/x86/codegen/lowering/memory_lowering.*`
- legacy frame and memory entrypoints that should forward into the new owners

Actions:

- add the reviewed frame and memory lowering headers and source files
- route legacy frame and memory entrypoints through explicit forwarding into
  the new seams instead of broad include reach-through
- keep prepared and non-prepared callers compiling while the real owners are
  stood up

Completion check:

- reviewed frame and memory lowering seams exist in source form and legacy
  callers compile through explicit forwarding rather than hidden mixed-file
  ownership

#### Step 2.1.2: Move Canonical Frame-Home And Stack-Layout Queries Behind Lowering Owners

Goal: migrate frame-home lookup, stack-slot, and stack-layout helper families
behind `frame_lowering.*` so later lowering packets can depend on canonical
frame services instead of querying legacy orchestration files directly.

Primary targets:

- frame-home and stack-layout helpers under `src/backend/mir/x86/codegen/`
- `src/backend/mir/x86/codegen/lowering/frame_lowering.*`

Actions:

- move one coherent frame-home or stack-layout helper family at a time behind
  `frame_lowering.*`
- keep legacy callers working through explicit forwarding while ownership
  shifts to the reviewed seam
- avoid widening the step into ABI policy or public entrypoint rewiring

Completion check:

- canonical frame-home and stack-layout queries live behind
  `frame_lowering.*`, and legacy callers use explicit forwarding to reach them

#### Step 2.1.3: Move Prepared-Home Selection And Memory Render Helpers Behind Lowering Owners

Goal: migrate prepared-home selection, stack-address computation, and local
load/store render helpers behind `frame_lowering.*` and `memory_lowering.*`
so prepared local-slot rendering stops rebuilding memory ownership inline.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/lowering/frame_lowering.*`
- `src/backend/mir/x86/codegen/lowering/memory_lowering.*`

Actions:

- move prepared-home selection helpers behind lowering-owned frame services
- move stack-address and memory render helper families behind
  `memory_lowering.*`
- keep prepared callers compiling through explicit forwarding instead of
  leaving `PreparedValueHome` inspection scattered inline

Completion check:

- prepared local-slot render paths delegate home selection and memory emission
  to lowering-owned helpers instead of rebuilding those decisions inline

##### Step 2.1.3.1: Move Prepared Load/Source And Operand Helpers Behind Lowering Owners

Goal: migrate the prepared local-load, block-source fallback, named-operand,
and load-finalization helper families behind `memory_lowering.*` so
prepared-local-slot rendering stops duplicating `I32` source selection and
home-sync policy inline.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/lowering/memory_lowering.*`

Actions:

- move prepared local-load and block-source fallback helpers behind
  `memory_lowering.*`
- move named `I32` source/operand adapter helpers behind the lowering seam
- move load-finalization and stack-home sync helpers behind lowering-owned
  `PreparedNamedI32Source` flows

Completion check:

- prepared local-load, source/operand, and load-finalization helpers live
  behind `memory_lowering.*`, and `prepared_local_slot_render.cpp` only keeps
  thin bridging logic for those flows

##### Step 2.1.3.2: Move Remaining Prepared `I32` Binary And Select Render Helpers Behind Lowering Owners

Goal: migrate the remaining prepared-side `I32` binary/select register-move
and result-publication helpers behind lowering owners so
`prepared_local_slot_render.cpp` stops making local `eax`/`ecx` move choices
around lowering-owned `PreparedNamedI32Source` flows.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/lowering/memory_lowering.*`

Actions:

- move the remaining prepared `I32` binary/select helpers that still choose
  current/previous-register moves inline in `prepared_local_slot_render.cpp`
- keep overload wrappers thin by delegating authoritative memory rendering and
  result-publication policy into lowering-owned helpers
- avoid widening the packet into scalar-lowering ownership, frame lowering, or
  prepared entry-surface rewiring

Completion check:

- prepared `I32` binary/select helper paths delegate register-move and result
  publication decisions to lowering-owned helpers, and
  `prepared_local_slot_render.cpp` remains a thin bridge for those flows

#### Step 2.1.4: Audit Remaining Frame/Memory Holdouts Before Later Lowering Families

Goal: leave step 2.1 with explicit frame and memory compatibility holdouts so
later call, compare, scalar, and float packets inherit clear ownership rather
than ambiguous mixed helpers.

Primary targets:

- remaining frame and memory helper holdouts under `src/backend/mir/x86/codegen/`
- explicit forwarding points into `lowering/frame_lowering.*` and
  `lowering/memory_lowering.*`

Actions:

- classify any remaining frame or memory helpers as lowered, forwarded, or
  honest later-step holdouts
- remove accidental duplicate ownership created while moving helper families
- keep the backend building while recording which holdouts intentionally remain
  for later lowering packets

Completion check:

- remaining frame and memory helper ownership is explicit, duplicate ownership
  is not silently growing, and later lowering packets can build on clear
  canonical services

### Step 2.2: Migrate Canonical Call And Return Families

Goal: move canonical call setup, call issuance, cleanup, result publication,
and return-lane handoff behind reviewed `call_lowering` and
`return_lowering` owners.

Primary targets:

- `src/backend/mir/x86/codegen/lowering/`
- call and return helper surfaces currently spread across legacy mixed files

Actions:

- stand up the reviewed call and return lowering files
- move one coherent call-lane or return-publication family at a time behind
  the new owners
- keep module and prepared compatibility callers forwarding into the new seams
  without changing their admission logic yet

Completion check:

- step 2.2.1 through step 2.2.3 are complete, canonical call and return
  helpers are owned by reviewed lowering seams, and legacy callers only remain
  as explicit forwarding or compatibility shells

#### Step 2.2.1: Migrate Canonical Call Setup And Shared Operand Prep Helpers

Goal: finish the foundational call-lane migration by standing up the reviewed
call/return lowering file boundaries and moving canonical call-setup,
operand-pair loading, and shared result-prep helpers behind compiled lowering
owners instead of dormant mixed support files.

Primary targets:

- `src/backend/mir/x86/codegen/lowering/call_lowering.*`
- `src/backend/mir/x86/codegen/lowering/return_lowering.*`
- dormant or mixed shared support around canonical call setup and operand prep

Actions:

- keep reviewed call/return lowering files as the compiled ownership seams for
  canonical call and return helpers
- move call-setup and shared operand/result-prep helper families out of
  dormant support files one coherent cluster at a time
- leave module and prepared compatibility entrypoints as forwarding callers
  instead of reviving mixed legacy owners

Completion check:

- canonical call-setup and shared operand/result-prep helpers are owned by
  compiled reviewed lowering seams and dormant mixed support no longer owns
  that family

#### Step 2.2.2: Migrate Canonical Call Issuance, Cleanup, And Result Publication

Goal: move the next live canonical call family so actual call issuance,
post-call cleanup, and result publication stop depending on mixed or dormant
support ownership.

Primary targets:

- canonical call/result publication helpers under
  `src/backend/mir/x86/codegen/lowering/`
- remaining mixed or dormant support around call cleanup and result lanes

Actions:

- classify the next coherent helper family still living around call issuance,
  cleanup, or result publication
- migrate that family behind reviewed lowering or compiled owners without
  widening into prepared-route admission or ABI-policy work
- keep compatibility callers explicit and thin while the canonical owners take
  responsibility for the moved family

Completion check:

- canonical call issuance, cleanup, and result publication helpers are owned
  by reviewed lowering seams and no mixed support file remains a hidden owner
  for that family

#### Step 2.2.3: Migrate Canonical Return-Lane Handoff And Audit Remaining Holdouts

Goal: finish the call/return migration by moving return-lane handoff and any
remaining canonical call/return holdouts behind reviewed lowering seams, then
make the leftover compatibility surface explicit.

Primary targets:

- remaining canonical return-lane helpers under
  `src/backend/mir/x86/codegen/lowering/`
- compatibility wrappers and residual mixed call/return support files

Actions:

- move return-lane handoff helpers and any remaining canonical call/return
  holdouts behind `return_lowering` or adjacent reviewed lowering owners
- classify leftover legacy call/return helpers as forwarded compatibility
  shells or honest later-step holdouts
- keep prepared-route admission, ABI policy, and non-canonical lowering work
  out of this step

Completion check:

- canonical return-lane handoff is owned by reviewed lowering seams and the
  remaining legacy call/return surface is explicitly classified instead of
  silently retaining mixed ownership

### Step 2.3: Migrate Scalar And Comparison Lowering Families

Goal: centralize integer-scalar arithmetic, casts, width conversion, compare
materialization, and fused compare/branch behavior behind reviewed scalar and
comparison owners.

Primary targets:

- `src/backend/mir/x86/codegen/lowering/`
- scalar and comparison helper-heavy legacy files under
  `src/backend/mir/x86/codegen/`

Actions:

- stand up the reviewed scalar and comparison lowering files
- move coherent scalar helper groups and predicate/branch lowering helpers
  behind those owners instead of growing testcase-shaped matchers
- keep dependencies on frame and memory services flowing through the new
  lowering seams

Completion check:

- scalar and comparison behavior is owned by reviewed lowering seams and later
  prepared work can consume those helpers without reopening legacy ownership

### Step 2.4: Migrate Float And Special-Case Return Support

Goal: move scalar floating arithmetic plus legacy `f128` and related special
return helpers behind the reviewed float and return lowering owners.

Primary targets:

- `src/backend/mir/x86/codegen/lowering/`
- floating and special return helpers currently living in legacy mixed files

Actions:

- stand up the reviewed float lowering files and connect them to canonical
  memory and return services
- move coherent floating helper families, including `f128` address/home
  support, without leaking ownership back into prepared or module files
- keep special return behavior routed through the reviewed return seam instead
  of ad hoc helper reach-through

Completion check:

- float and special return behavior is owned by reviewed lowering seams and no
  longer depends on hidden legacy mixed-owner paths

### Step 2.5: Migrate Atomics And Intrinsics Lowering Families

Goal: finish step 2 by moving atomics and intrinsics behavior behind the
reviewed `atomics_intrinsics_lowering` seam and classifying any remaining
mixed-owner helpers that step 3 must still treat as compatibility holdouts.

Primary targets:

- `src/backend/mir/x86/codegen/lowering/`
- remaining atomics/intrinsics helper surfaces under
  `src/backend/mir/x86/codegen/`

Actions:

- stand up the reviewed atomics/intrinsics lowering files
- move coherent atomics or intrinsic helper families behind that owner
- record any honest compatibility holdouts explicitly so prepared-adapter work
  in step 3 starts from a clear boundary

Completion check:

- the reviewed lowering family seams cover atomics and intrinsics behavior and
  step 3 starts from explicit canonical owners plus clearly classified holdouts

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
