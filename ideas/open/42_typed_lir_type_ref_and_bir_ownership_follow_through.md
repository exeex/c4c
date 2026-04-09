# Typed LIR TypeRef and BIR Ownership Follow-Through

Status: Open
Last Updated: 2026-04-03

## Goal

Upgrade the LIR type representation from "text plus coarse classification" to
a structured typed object that retains renderable text while carrying the exact
semantic information needed by downstream lowerings.

As part of that upgrade, change `src/backend/lowering/lir_to_bir.cpp` to use
typed LIR type information as the primary source of truth and remove string
matching as the main lowering decision mechanism for scalar type selection.

Then complete the backend-ownership follow-through that this type boundary work
unblocks: move backend-facing control-flow normalization, phi handling
responsibility, and regalloc ownership off ad hoc LIR-side scaffolding and onto
the BIR-owned route that idea 41 is building.

## Why This Idea Exists

The current LIR layer already wraps type text in `LirTypeRef`, but the typed
payload is still too weak for backend-facing lowering:

- `LirTypeRef` stores the original text and a coarse `LirTypeKind`
  classification such as `Integer`, `Pointer`, or `Struct`
- that classification does not preserve the exact integer width, pointer
  payload, or other precise shape details needed by `lir_to_bir.cpp`
- `lir_to_bir.cpp` therefore still uses helpers such as
  `lower_scalar_type_text(std::string_view)` to recover `i8`, `i32`, and `i64`
  by string comparison
- this keeps a text-first boundary alive in a place that should be a typed IR
  translation boundary

This is a sign that the front-end/LIR refactor is not yet complete at the type
boundary. The LIR object model exists, but it does not yet carry enough type
information for string-free lowering into BIR.

There is also a second, adjacent incompleteness in the current backend split:

- BIR exists nominally as the backend-owned route, but key backend mechanics
  such as liveness/regalloc ownership still hang directly off `LIR`
- phi handling is not yet owned by one clean backend stage; some paths keep phi
  nodes alive, while some target-specific emitters perform local predecessor
  copy expansion themselves
- CFG-shape normalization still lives largely in legacy or target-local
  adapters instead of a single canonical BIR-side lowering contract

That means the current architecture still mixes:

- `LIR` as the place where exact backend analysis data is computed
- `BIR` as a partial backend interchange format
- target emitters as the place where some backend-lowering cleanups still occur

This idea exists to line those boundaries back up after idea 41 removes the
legacy `ir.*` bridge.

## Current Problem Surface

Today the backend BIR bridge mixes two worlds:

- function parameters can sometimes lower from preserved `TypeSpec`
- instruction-local types in LIR binary/cmp/cast/ret paths still lower from
  LLVM-style type text such as `"i8"` or `"i32"`

Today the backend ownership story also mixes multiple layers:

- liveness and register allocation operate on `LirFunction`
- phi-aware stack-layout helpers also inspect `LIR` directly
- some target paths still perform phi-copy insertion or CFG-sensitive cleanup
  inside the emitter
- `lir_to_bir.cpp` is not yet the single place where backend-normalized
  control/data-flow ownership is established

That split causes several problems:

- backend lowering correctness depends on the exact emitted spelling of LIR
  type strings
- type interpretation is duplicated at multiple boundaries instead of being
  decided once when constructing LIR
- `LirTypeRef.kind()` is not rich enough to support BIR lowering directly
- future BIR growth will keep dragging string parsing logic forward unless the
  type boundary is fixed now
- backend analysis ownership remains attached to the wrong layer, which makes
  "BIR-owned backend codegen" incomplete even if type lowering becomes typed
- new targets such as RV32/RV64 would otherwise inherit LIR-coupled regalloc
  and CFG cleanup instead of consuming a cleaner backend-owned contract

## Execution Notes

- 2026-04-09: a follow-up Step 5 audit confirmed that the shared
  `try_lower_conditional_return_select_function(...)` matcher already handles
  interleaved branch-only conditional-return arm layouts where the true-arm
  and false-arm empty goto chains are physically interleaved in
  `function.blocks`; added focused shared lowering and AArch64 pipeline
  regressions to lock that ownership boundary down
- 2026-04-09: the remaining Step 1 `lir_to_bir.cpp` helper re-audit confirmed
  that `lower_binary(...)`, `lower_compare_materialization(...)`, and
  `lower_select_materialization(...)` already consume semantic `LirTypeRef`
  width metadata via `lower_scalar_type(const LirTypeRef&)` rather than
  reparsing raw type text
- 2026-04-09: any remaining raw-text checks in `src/backend/lowering/lir_to_bir.cpp`
  are now specific fallback recognizers or backend-ownership seams, not generic
  scalar/materialization helpers; the next active migration slice should move
  to canonical BIR-owned phi/CFG normalization instead of extending the typed
  helper audit

## Concrete File Inventory

These are the current concrete ownership points that make this work necessary.

### LIR Type Boundary

- `src/codegen/lir/types.hpp`
  - `LirTypeRef` currently stores text plus coarse `LirTypeKind`, but not the
    exact typed payload needed for string-free backend lowering
- `src/backend/lowering/lir_to_bir.cpp`
  - still contains text-driven scalar type interpretation such as
    `lower_scalar_type_text(std::string_view)` and related helpers
- `src/codegen/lir/verify.cpp`
  - will need to become the enforcement point for typed/type-text consistency

### LIR-Owned Liveness / Regalloc

- `src/backend/liveness.hpp`
  - exposes `compute_live_intervals(const c4c::codegen::lir::LirFunction&)`
- `src/backend/liveness.cpp`
  - computes liveness directly from `LIR`, including explicit handling for
    `LirPhiOp`
- `src/backend/regalloc.hpp`
  - exposes `allocate_registers(const c4c::codegen::lir::LirFunction&, ...)`
- `src/backend/regalloc.cpp`
  - performs register allocation over `LIR`-derived live intervals
- `src/backend/generation.hpp`
  - exposes `run_regalloc_and_merge_clobbers(const c4c::codegen::lir::LirFunction&, ...)`
- `src/backend/generation.cpp`
  - integrates LIR-side regalloc output with clobber tracking

### Stack Layout / Phi-Aware Helpers Still Reading LIR

- `src/backend/stack_layout/analysis.cpp`
  - contains phi-aware stack-layout analysis over `LIR`
- `src/backend/stack_layout/alloca_coalescing.cpp`
  - still recognizes `LirPhiOp`
- `src/backend/stack_layout/slot_assignment.cpp`
  - still recognizes `LirPhiOp`
- `src/backend/stack_layout/regalloc_helpers.*`
  - currently integrate with the LIR-owned regalloc result shape

### Legacy / Adapter-Side CFG Normalization

- `src/backend/lowering/lir_to_backend_ir.cpp`
  - still contains a large amount of backend-facing normalization/adaptation
    logic for branches, joins, loops, calls, and phi-bearing shapes
- `src/backend/ir.hpp`
  - still defines the legacy backend instruction/control-flow model that these
    adapters normalize into
- `src/backend/ir_validate.cpp`
  - still owns validation for the legacy backend CFG/phi model

### Target-Local Phi / CFG Cleanup

- `src/backend/aarch64/codegen/emit.cpp`
  - contains target-local phi-copy insertion and other LIR-sensitive cleanup in
    the legacy/general emitter path
- `src/backend/x86/codegen/emit.cpp`
  - still contains target-local pattern handling that assumes legacy backend
    normalization has already happened outside canonical BIR

### Routing / Ownership Boundaries That Need Revisit After Idea 41

- `src/backend/backend.cpp`
  - will need a post-41 audit to ensure backend ownership does not silently
    remain on LIR-side liveness/regalloc helpers after legacy IR removal
- `src/backend/backend.hpp`
  - will need a post-41 audit once the canonical backend-owned analysis/input
    contract is finalized

## Post-41 Ownership Decision

This idea adopts the following architecture decision rather than leaving it
open-ended:

- `BIR` remains the one canonical target-neutral backend IR
- each production backend may lower from BIR into its own `MIR` (Machine IR)
- `MIR` is the target-specific layer that may carry ABI/calling-convention,
  register, stack-slot, and machine-legalization detail
- liveness, regalloc, stack layout, and other machine-facing analysis should
  move from `LIR` onto backend-specific `MIR`
- target-specific printers/debug dumps may exist for `MIR`, while final asm
  emission should be produced from `MIR`

This means the migration target for regalloc ownership is not raw canonical
`BIR`, but the target `MIR` layer that is derived from BIR.

## Desired End State

LIR type objects should support both of these needs at once:

- expose a structured typed representation that downstream passes can inspect
  without reparsing text
- preserve a canonical textual rendering for LLVM/LIR printing and debugging

Concretely, after this work:

- integer LIR types should know their exact width, not only "Integer"
- pointer, void, floating, aggregate, function, and opaque/raw cases should
  have explicit typed variants or payloads appropriate to the current LIR model
- instruction forms such as `LirBinOp`, `LirCmpOp`, `LirCastOp`, `LirRet`, and
  function-signature surfaces should expose enough typed type information for
  BIR lowering to operate without string matching as its primary discriminator
- `lir_to_bir.cpp` should lower from typed LIR type data and treat text only as
  a rendering/debugging concern, not as the authoritative semantic source
- backend-facing CFG normalization should be owned by the BIR lowering path or
  by an explicitly BIR-owned follow-on stage, not by scattered LIR/target-local
  adapters
- phi handling should have one canonical ownership point on the BIR side,
  whether that means preserving phi in canonical BIR or eliminating it in a
  defined BIR-to-target preparation stage
- register allocation should stop treating `LIR` as the canonical backend
  analysis substrate; it should consume backend-specific `MIR` derived from
  canonical BIR, not raw `LIR`

## Scope

### In Scope

- redesigning `LirTypeRef` so it can carry exact structured type payloads
- deciding how canonical type text is derived from typed data and/or cached
- updating LIR constructors/builders so typed information is attached at object
  creation time rather than reconstructed downstream
- updating LIR verification to validate typed/type-text consistency
- changing `src/backend/lowering/lir_to_bir.cpp` to use typed type information
  instead of `std::string_view` matching for scalar type lowering
- removing now-redundant text-based type decoding helpers from the BIR lowering
  path
- auditing which backend-normalization responsibilities currently sit on `LIR`
  only because BIR is still incomplete
- defining the intended post-41 ownership for phi handling, CFG normalization,
  liveness, and register allocation
- moving backend analysis ownership from `LIR` onto backend-specific `MIR`
  produced from canonical BIR
- adding regressions that prove BIR lowering behavior no longer depends on type
  string matching

### Out of Scope

- a full semantic redesign of all LIR instructions unrelated to type carriage
- forcing BIR to support more types than its current planned expansion requires
- deleting textual rendering from LIR entirely

## Proposed Approach

### Concrete Migration Slice Order

Unless execution uncovers a blocking dependency that must be split out
separately, this idea should proceed in the following fixed order:

1. typed LIR type ownership
2. canonical BIR phi/control-flow surface
3. MIR-owned liveness
4. MIR-owned regalloc and stack-layout integration
5. delete temporary LIR-side backend shims

This order is intentional.

- typed type ownership comes first because later BIR lowering should not keep
  growing on top of text-first type decisions
- BIR phi/control-flow ownership comes next because liveness and regalloc need
  a stable backend-owned CFG/value model
- liveness moves before regalloc because regalloc is downstream of live-range
  construction
- regalloc moves before shim deletion so there is a bounded overlap period
  where new BIR-owned analysis can be validated against the old LIR-owned path
- LIR-side backend shims are removed last so deletion happens only after the
  replacement path is validated

For execution purposes, the migration slices should be interpreted as follows.

### Slice 1: Typed Type Ownership

- finish the `LirTypeRef` typed payload work
- make `lir_to_bir.cpp` consume typed type information instead of string-first
  decoding
- strengthen `src/codegen/lir/verify.cpp` so typed/text consistency failures
  surface early

Completion signal:

- BIR type lowering no longer depends primarily on `\"i8\"` / `\"i32\"` /
  `\"i64\"` string matching

### Slice 2: Canonical BIR Phi / CFG Surface

- decide and implement the post-41 canonical BIR representation for backend
  joins/loops/branch structure
- move phi/control-flow normalization responsibility behind the BIR boundary
- remove target-local cleanup that exists only because BIR lacks ownership here

Completion signal:

- backend-facing phi/control-flow normalization has one canonical BIR-side home

### Slice 3: MIR-Owned Liveness

- introduce `MIR`-side liveness construction over the canonical BIR-derived
  backend control/data-flow model
- keep the LIR-side liveness path only as a comparison shim during migration
- validate that `MIR`-side live intervals match the intended backend slices

Completion signal:

- backend liveness can be computed from target `MIR` without requiring
  `LirFunction`

### Slice 4: MIR-Owned Regalloc and Stack Layout

- introduce the target `MIR` layer where it does not already exist
- retarget regalloc interfaces to backend-specific `MIR`
- migrate stack-layout and regalloc integration helpers off the LIR-owned
  substrate
- keep temporary compatibility wiring only long enough to compare behavior

Completion signal:

- production regalloc/stack-layout ownership lives on backend `MIR`, not raw
  `LIR`

### Slice 5: Delete LIR-Side Backend Shims

- remove temporary LIR-side liveness/regalloc compatibility entry points
- remove backend paths that still treat `LIR` as the canonical owner of
  phi-lowering or backend analysis state
- update docs/tests to reflect the final ownership model

Completion signal:

- `LIR` is no longer the hidden permanent owner of backend analysis state after
  idea 41

### Phase 1: Type model and ownership audit

- inventory every current `LirTypeRef` use site
- classify which sites only need coarse kind information versus exact type
  payload
- define a typed LIR type model that can represent the currently emitted LLVM
  type strings without losing round-trip renderability
- inventory current backend ownership leakage across:
  - `lir_to_bir.cpp`
  - liveness/regalloc
  - stack-layout phi-aware helpers
  - target-specific phi/cfg cleanup in emitters
- decide which of these belong in canonical BIR versus backend-specific `MIR`
  built from BIR, following the architecture decision above

Key design rule:

- type semantics should be determined once, when the LIR type object is formed,
  not re-derived later by ad hoc string checks

Key ownership rule:

- once idea 41 is complete, backend ownership should not silently remain on
  `LIR` just because that is where legacy scaffolding happened to live

### Phase 2: Construct typed types at the source

- update the LIR-producing pipeline so type objects are created with exact typed
  payloads at construction time
- preserve or derive canonical text from the typed model
- ensure existing printer/debug behavior remains readable and stable

### Phase 3: Strengthen verification

- extend `src/codegen/lir/verify.cpp` so it checks the internal consistency of
  the typed payload and any cached/rendered text
- fail early when a type object claims one semantic form but renders as another

### Phase 4: Convert LIR-to-BIR lowering

- replace `lower_scalar_type_text(std::string_view)`-style helpers with typed
  lowering helpers that consume the structured LIR type object
- update `LirBinOp`, `LirCmpOp`, `LirCastOp`, return lowering, and signature
  lowering paths so they use typed type payloads directly
- leave text lookups only where they are unavoidable for diagnostics or legacy
  compatibility, not for primary semantic branching

### Phase 5: Pull backend normalization behind the BIR boundary

- move backend-facing CFG normalization out of legacy/LIR-only adaptation code
  and into `lir_to_bir.cpp` or an explicit BIR-owned normalization stage
- define one canonical phi strategy for the post-41 backend:
  - either preserve phi in BIR and make downstream consumers own it uniformly
  - or eliminate phi in a BIR-to-`MIR` lowering stage before target emission
- remove target-local phi cleanup that exists only because BIR ownership is
  incomplete

### Phase 6: Move liveness and regalloc ownership off LIR

- audit the current `compute_live_intervals(const LirFunction&)` and regalloc
  interfaces and introduce backend-`MIR`-owned equivalents
- migrate stack-layout helpers and regalloc integration to the chosen
  backend-`MIR` substrate
- leave LIR-side regalloc only as a temporary compatibility shim during the
  transition, not as the permanent owner

### Phase 7: Remove text-first and LIR-first fallback logic

- delete or quarantine the old string-based lowering helpers once the typed
  path is complete
- add tests that would fail if type strings were the only source of truth
- audit for other backend paths that still branch primarily on textual type
  spellings and either fix them here or file explicitly separate follow-up ideas
- audit for backend paths that still assume `LIR` is the canonical owner of
  liveness/regalloc/phi-lowering state and either migrate them here or file
  explicitly separate follow-up ideas

## Acceptance Criteria

- [ ] `LirTypeRef` carries exact typed payloads, not only coarse kind
- [ ] typed LIR type objects can still produce canonical text for printing and
      diagnostics
- [ ] LIR construction attaches type semantics at object creation time
- [ ] `src/codegen/lir/verify.cpp` validates typed/type-text consistency
- [ ] `src/backend/lowering/lir_to_bir.cpp` uses typed LIR type information as
      its primary type discriminator
- [ ] string matching such as `"i8"` / `"i32"` / `"i64"` is no longer the main
      method for scalar type lowering in `lir_to_bir.cpp`
- [ ] focused tests cover typed LIR type creation and typed LIR-to-BIR lowering
- [ ] any remaining unavoidable text-based type handling is explicitly bounded
      and documented as non-primary
- [ ] a post-41 backend ownership plan exists for phi handling, CFG
      normalization, liveness, and regalloc
- [ ] each production backend has a defined `BIR -> MIR` ownership boundary for
      machine-facing lowering
- [ ] regalloc no longer treats raw `LIR` as the permanent backend ownership
      layer and instead runs on backend `MIR`
- [ ] target-local phi cleanup that only exists due to incomplete BIR ownership
      is either removed or explicitly staged behind a BIR-owned lowering step

## Risks

- this touches a cross-cutting representation boundary, so shallow partial
  conversions could leave the codebase in a worse mixed typed/text state
- if typed payload and text are both stored, drift between them becomes a new
  consistency risk unless verification is upgraded first or in lockstep
- some current backend or printer paths may implicitly rely on exact text
  spellings, which will need careful audit during migration
- introducing `MIR` too vaguely could just rename the current ownership problem
  instead of fixing it; the BIR/MIR boundary must stay explicit
- migrating regalloc ownership too early, before BIR control/data-flow coverage
  is stable, could create churn across multiple moving layers at once
- leaving regalloc ownership on `LIR` permanently would undercut the point of a
  canonical BIR-owned backend even if typed type lowering is fixed

## Relationship To Existing Work

This should execute immediately after idea 41, not in parallel with it.

- idea 41 removes legacy `ir.*`, migrates emitters, and makes BIR the only
  production backend route
- this idea is the follow-through that cleans up the still-LIR-owned backend
  scaffolding that idea 41 intentionally does not absorb

This is adjacent to, but not identical with, the active BIR expansion idea.

- the active BIR idea focuses on backend IR consolidation and legacy IR removal
- this idea focuses on the earlier LIR type boundary plus the remaining
  backend-ownership seams that would otherwise stay stuck on `LIR` after the
  consolidation

The two efforts should align, but this should remain a separate tracked idea so
it does not silently expand the scope of the current active plan.
