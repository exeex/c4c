# Typed LIR TypeRef and BIR Ownership Follow-Through

Status: Active
Source Idea: ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md
Activated from: prompts/ACTIVATE_PLAN.md

## Purpose

Turn the LIR type boundary into a typed semantic boundary, then finish the
backend-ownership migration that should move phi/CFG normalization, liveness,
regalloc, and stack-layout integration off ad hoc LIR-side scaffolding and onto
the canonical BIR -> backend MIR route.

## Goal

Make typed LIR type data the primary source of truth for BIR lowering and move
machine-facing backend ownership off raw LIR.

## Core Rule

Keep this work scoped to the linked idea. If execution uncovers a separate
initiative, record it under `ideas/open/` instead of silently expanding this
runbook.

## Read First

- [ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md](/workspaces/c4c/ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md)
- [src/codegen/lir/types.hpp](/workspaces/c4c/src/codegen/lir/types.hpp)
- [src/codegen/lir/verify.cpp](/workspaces/c4c/src/codegen/lir/verify.cpp)
- [src/backend/lowering/lir_to_bir.cpp](/workspaces/c4c/src/backend/lowering/lir_to_bir.cpp)
- [src/backend/liveness.hpp](/workspaces/c4c/src/backend/liveness.hpp)
- [src/backend/liveness.cpp](/workspaces/c4c/src/backend/liveness.cpp)
- [src/backend/regalloc.hpp](/workspaces/c4c/src/backend/regalloc.hpp)
- [src/backend/regalloc.cpp](/workspaces/c4c/src/backend/regalloc.cpp)
- [src/backend/generation.cpp](/workspaces/c4c/src/backend/generation.cpp)
- [src/backend/bir.hpp](/workspaces/c4c/src/backend/bir.hpp)
- [src/backend/stack_layout/analysis.cpp](/workspaces/c4c/src/backend/stack_layout/analysis.cpp)
- [src/backend/stack_layout/slot_assignment.cpp](/workspaces/c4c/src/backend/stack_layout/slot_assignment.cpp)

## Current Targets

- Typed `LirTypeRef` payloads with canonical text preservation
- Typed `lir_to_bir.cpp` lowering without string-first scalar decoding
- Verified typed/text consistency in LIR
- Canonical BIR-side ownership for phi/control-flow normalization
- Backend MIR-owned liveness, regalloc, and stack-layout integration
- Removal of temporary LIR-side backend ownership shims

## Non-Goals

- Broad redesign of unrelated LIR instructions
- Expanding BIR beyond what this migration needs
- Removing textual LIR rendering entirely
- Silent scope creep into unrelated backend initiatives

## Working Model

- `LirTypeRef` must carry exact semantic payload plus renderable canonical text.
- Text is a rendering/debug surface, not the primary semantic discriminator.
- Canonical backend ownership is `BIR -> target MIR -> machine-facing analysis`.
- LIR-side liveness/regalloc paths may exist only as temporary migration shims.
- Phi and CFG normalization must have one backend-owned home after the BIR
  boundary.

## Execution Rules

- Follow the migration order in this runbook unless a blocking dependency forces
  a separately tracked idea.
- Use test-first slices for implementation work.
- Compare frontend/backend output against Clang or existing backend contracts
  when behavior is unclear.
- Keep `todo.md` current whenever the active slice changes.
- Do not delete compatibility shims until their replacements are validated.

## Ordered Steps

### Step 1: Audit the typed type boundary and backend ownership seams

Goal: establish the exact set of LIR type consumers and the backend ownership
surfaces that still depend on LIR-side scaffolding.

Primary targets:
- [src/codegen/lir/types.hpp](/workspaces/c4c/src/codegen/lir/types.hpp)
- [src/backend/lowering/lir_to_bir.cpp](/workspaces/c4c/src/backend/lowering/lir_to_bir.cpp)
- [src/backend/liveness.cpp](/workspaces/c4c/src/backend/liveness.cpp)
- [src/backend/regalloc.cpp](/workspaces/c4c/src/backend/regalloc.cpp)
- [src/backend/stack_layout/analysis.cpp](/workspaces/c4c/src/backend/stack_layout/analysis.cpp)
- [src/backend/aarch64/codegen/emit.cpp](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)
- [src/backend/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)

Concrete actions:
- inventory `LirTypeRef` construction and use sites
- classify which users need coarse kind versus exact typed payload
- inventory every backend path that still depends on LIR for phi, CFG,
  liveness, regalloc, or stack-layout ownership
- record any truly separate initiative under `ideas/open/` instead of mutating
  this runbook

Completion check:
- the active slice names the concrete files and interfaces to change first
- the BIR-owned versus MIR-owned follow-through targets are explicit in
  `todo.md`

### Step 2: Finish typed LIR type ownership

Goal: redesign `LirTypeRef` so exact semantic payload is attached when the type
object is created and can still render canonical text.

Primary targets:
- [src/codegen/lir/types.hpp](/workspaces/c4c/src/codegen/lir/types.hpp)
- LIR builders/constructors that form `LirTypeRef`

Concrete actions:
- add exact typed payload variants for the LIR type forms needed by current
  lowering
- keep canonical text derivation or caching bounded and explicit
- update LIR construction sites so semantics are attached at creation time
- add focused tests for typed type creation and round-trip rendering where the
  repo already covers LIR type behavior

Completion check:
- `LirTypeRef` carries exact typed payloads needed downstream
- typed objects still provide stable text for printing/diagnostics

### Step 3: Strengthen LIR verification

Goal: fail early when typed payload and rendered/cached text drift apart.

Primary targets:
- [src/codegen/lir/verify.cpp](/workspaces/c4c/src/codegen/lir/verify.cpp)

Concrete actions:
- validate typed/type-text consistency
- add narrow regressions for inconsistent or unsupported type constructions

Completion check:
- verifier catches mismatched type semantics before backend lowering

### Step 4: Convert LIR-to-BIR lowering to typed type data

Goal: make typed LIR type information the primary discriminator in BIR
lowering.

Primary targets:
- [src/backend/lowering/lir_to_bir.cpp](/workspaces/c4c/src/backend/lowering/lir_to_bir.cpp)

Concrete actions:
- replace string-first helpers such as scalar type text decoding with typed
  lowering helpers
- update binop, cmp, cast, return, and signature lowering paths to inspect typed
  payloads directly
- keep text lookups only where they remain necessary for diagnostics or bounded
  legacy compatibility
- add regressions proving lowering does not depend primarily on `"i8"`,
  `"i32"`, or `"i64"` string matching

Completion check:
- BIR scalar type lowering no longer branches primarily on type text

### Step 5: Pull phi and CFG normalization behind the BIR boundary

Goal: give backend-facing phi/control-flow normalization one canonical
ownership point.

Primary targets:
- [src/backend/lowering/lir_to_bir.cpp](/workspaces/c4c/src/backend/lowering/lir_to_bir.cpp)
- [src/backend/bir.hpp](/workspaces/c4c/src/backend/bir.hpp)
- [src/backend/liveness.cpp](/workspaces/c4c/src/backend/liveness.cpp)
- target emitters that still perform phi/CFG cleanup

Concrete actions:
- define the canonical post-41 phi strategy
- move backend-facing CFG normalization into BIR lowering or an explicit
  BIR-owned normalization stage that feeds the backend-owned CFG/liveness path
- remove target-local cleanup that exists only because BIR ownership is missing

Completion check:
- phi and CFG normalization have one canonical BIR-side home

### Step 6: Move liveness to backend MIR

Goal: compute liveness from backend MIR rather than raw `LirFunction`.

Primary targets:
- [src/backend/bir.hpp](/workspaces/c4c/src/backend/bir.hpp)
- [src/backend/liveness.hpp](/workspaces/c4c/src/backend/liveness.hpp)
- [src/backend/liveness.cpp](/workspaces/c4c/src/backend/liveness.cpp)
- [src/backend/stack_layout/slot_assignment.cpp](/workspaces/c4c/src/backend/stack_layout/slot_assignment.cpp)
- backend MIR / backend-owned CFG definitions or lowering entry points
  introduced for this migration

Concrete actions:
- add a backend-owned CFG/function representation that can carry the liveness
  inputs needed after the BIR boundary
- provide lowering entry points from current backend-owned forms into that CFG,
  starting with BIR and any temporary LIR compatibility lowering still needed
- make `LivenessInput` derive from the backend-owned CFG representation rather
  than constructing it directly from raw `LirFunction`
- move production callers to `backend CFG -> LivenessInput ->
  compute_live_intervals(...)`
- keep `LirFunction -> backend CFG` only as a temporary comparison/transition
  shim while non-production callers are migrated
- validate interval parity or intended equivalence on narrow backend tests

Suggested slice order:
1. introduce the backend-owned CFG carrier and `backend CFG -> LivenessInput`
   lowering
2. add `bir::Function -> backend CFG` lowering for the lowerable subset already
   represented in BIR
3. retarget production callers that still ask liveness to read raw
   `LirFunction`
4. retarget helper/tests to the backend-owned CFG seam and leave direct-LIR
   lowering as compatibility-only
5. remove or sharply narrow the raw-LIR public liveness entrypoint once parity
   coverage exists

Completion check:
- production backend liveness derives from backend-owned CFG/MIR inputs rather
  than raw `LirFunction`
- any remaining `LirFunction` liveness entrypoint is explicitly compatibility-
  only and no longer the primary production surface

### Step 7: Move regalloc and stack layout to backend MIR

Goal: retarget regalloc and stack-layout integration to machine-facing MIR
ownership.

Primary targets:
- [src/backend/regalloc.hpp](/workspaces/c4c/src/backend/regalloc.hpp)
- [src/backend/regalloc.cpp](/workspaces/c4c/src/backend/regalloc.cpp)
- [src/backend/generation.cpp](/workspaces/c4c/src/backend/generation.cpp)
- [src/backend/stack_layout/analysis.cpp](/workspaces/c4c/src/backend/stack_layout/analysis.cpp)
- related stack-layout/regalloc helper files

Concrete actions:
- introduce or complete target MIR where needed
- retarget regalloc interfaces to MIR
- migrate stack-layout/regalloc integration off LIR-owned interval and phi
  assumptions
- keep temporary compatibility wiring only long enough to compare behavior

Completion check:
- production regalloc and stack-layout ownership live on backend MIR

### Step 8: Delete temporary LIR-side backend shims

Goal: remove fallback logic that leaves LIR as the hidden owner of backend
analysis state.

Primary targets:
- LIR-side liveness/regalloc compatibility entry points
- backend paths that still treat LIR as the canonical phi/regalloc owner
- docs/tests covering final ownership

Concrete actions:
- remove obsolete string-first lowering helpers
- remove temporary LIR-owned analysis shims
- update tests and docs to match the final ownership model

Completion check:
- LIR is no longer the permanent owner of backend analysis state

## Acceptance Checks

- `LirTypeRef` carries exact typed payloads, not only coarse kind
- typed LIR objects still provide canonical renderable text
- LIR construction attaches type semantics at creation time
- verifier checks typed/text consistency
- `lir_to_bir.cpp` uses typed type information as the primary discriminator
- scalar lowering no longer depends primarily on string matching
- focused tests cover typed type creation and typed LIR-to-BIR lowering
- the post-41 ownership plan for phi, CFG normalization, liveness, and regalloc
  is explicit in code and planning state
- production regalloc no longer treats raw LIR as the permanent backend layer

## Validation

- capture a full-suite baseline before implementation work
- add or update the narrowest test for each slice before code changes
- re-run targeted coverage and then full `ctest` before handoff
- require monotonic full-suite results
