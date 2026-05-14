# AArch64 Module Phoenix Stage 4 Implementation Runbook

Status: Active
Source Idea: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Activated from: closed Stage 3 draft set in `ideas/closed/227_aarch64_module_phoenix_replacement_drafts.md`

## Purpose

Convert the accepted Stage 3 AArch64 module replacement drafts into real
implementation through staged migration and proof.

Goal: route prepared BIR directly into typed MIR machine nodes through the
reviewed ownership seams, then retire legacy module-emitter ownership only
after the replacement path is live and proved.

## Core Rule

Stage 4 owns real `.cpp` / `.hpp` implementation, build wiring, dispatcher
rewiring, proof, and legacy retirement. The legacy AArch64 module emitter has
already been removed by `252dbc50c` and the fresh skeleton was added by
`0fce192f4`, so current execution must continue from that state. Do not restore
the broad legacy emitter or old module-record API to satisfy stale tests; repair
tests and implementation around the new route, then prove each replacement seam
before claiming capability progress.

## Read First

- `.codex/skills/phoenix-rebuild/SKILL.md`
- `ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md`
- `ideas/open/224_common_mir_container_and_target_printer_boundary.md`
- `ideas/closed/227_aarch64_module_phoenix_replacement_drafts.md`
- `src/backend/mir/mir.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/mir/aarch64/codegen/traversal.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/codegen/operands.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/returns.hpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`

## Current Targets

Implementation conversion targets from the reviewed Stage 3 draft map now live
as compiled C++ surfaces:

- `src/backend/mir/mir.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/mir/aarch64/codegen/traversal.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/codegen/operands.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/returns.hpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp` and
  `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`, if the
  compatibility projection is split out of `emit.cpp`
- related build wiring only when needed to compile and route the converted
  implementation

## Current State Context

The Stage 3 module implementation drafts and intermediate module-subdirectory
surfaces have already been retired. Do not look for or recreate deleted files
such as `module/function_traversal.cpp`, `module/operand_resolution.cpp`,
`module/instruction_lowering.cpp`, `module/branch_control_lowering.cpp`, or
`module/call_lowering.cpp`.

Current implementation ownership is in the AArch64 codegen surfaces under
`src/backend/mir/aarch64/codegen/`:

- traversal and module/function/block walking:
  `codegen/traversal.*`
- operand and location conversion: `codegen/operands.*`
- dispatch and top-level lowering selection: `codegen/dispatch.*`
- scalar ALU, comparison, and return lowering:
  `codegen/alu.*`, `codegen/comparison.*`, and `codegen/returns.*`
- typed target instruction records: `codegen/instruction.*`
- public module build/emit entrypoint and current compatibility projection:
  `codegen/emit.*`

Earlier extraction, skeleton migration, and initial lowering-family migration
are current-state context for this active runbook, not future work to repeat.
The next executable ownership question is whether the compatibility projection
currently inside `codegen/emit.cpp` should be split into
`codegen/compatibility_projection.*` or clearly classified in place.

## Non-Goals

- Do not reopen the Stage 2 layout without lifecycle repair.
- Do not reinterpret prepared authority semantics for frame, call, register
  allocation, spill/reload, storage plan, data, or control-flow facts.
- Do not broaden instruction coverage beyond the migrated seam being proved.
- Do not weaken tests, mark supported paths unsupported, or accept
  expectation-only progress.
- Do not add named-case shortcuts or testcase-shaped lowering.
- Do not preserve the old module emitter and record design as the real owner
  under replacement filenames.
- Do not delete compatibility code before the replacement owner is live and
  proved.

## Working Model

- `prepare::PreparedBirModule` is the semantic authority for module dispatch,
  function traversal, operand resolution, instruction lowering, branch/control
  lowering, call lowering, public assembly bridging, and compatibility
  projection.
- Canonical output is hierarchical typed MIR: module, function, block, and
  instruction or equivalent typed concepts.
- The common carrier lives under `src/backend/mir/` and AArch64 supplies
  target-owned instruction, operand, register, and rendering surfaces.
- `FunctionRecord::machine_nodes` and broad inspection records are derived
  compatibility surfaces after canonical lowering.
- Cached display strings, source spellings, broad public records, raw prepared
  views, and register strings are never semantic lowering authority.
- The shared `mir_printer` owns traversal and common emission mechanics;
  AArch64 owns target rendering hooks and target-specific operand/data forms.
- Public target rendering APIs use ordinary C++ names, not `__repr__`.
- `src/backend/mir/aarch64/codegen/machine_printer.*` remains a temporary
  terminal assembly route until idea 224 replaces it with the shared MIR printer
  boundary; do not grow it as a Stage 4 owner.

## Execution Rules

- Migrate in behavior-preserving slices where possible. For seams already
  affected by the legacy emitter removal, each slice must state what new-route
  responsibility moved, which deleted legacy surfaces are intentionally not
  restored, and what proof covers the repaired seam.
- Establish the common MIR carrier before translating the AArch64 public
  module skeleton, so the target header does not become the canonical
  container owner.
- Translate the Stage 3 drafts in dependency order instead of preserving the
  legacy catch-all assembler first.
- Do not attempt to keep or resurrect the deleted legacy module emitter as the
  active path. Until replacement lowering is compiled, routed, and proved, keep
  public assembly paths fail-closed rather than falling back to the old record
  pile.
- Build after each code-changing packet and run the delegated narrow proof;
  escalate to broader backend or full CTest when dispatcher/build wiring or
  shared lowering behavior changes.
- Treat test expectation downgrades, unsupported-path rewrites, and
  classification-only changes as route failures unless explicitly approved.
- If implementation diverges from the reviewed drafts, stop for lifecycle
  repair rather than silently changing the Stage 4 contract.

## Ordered Steps

### Step 1: Establish Common Hierarchical MIR Carrier

Goal: make the shared MIR carrier explicitly model module, function, block,
and instruction hierarchy before AArch64 module implementation depends on it.

Primary target:

- `src/backend/mir/mir.hpp`

Actions:

- Refine the shared carrier around module -> function -> block -> ordered
  instruction storage.
- Require durable BIR function identity on `MachineFunction`.
- Require durable BIR block identity on `MachineBlock`.
- Keep instruction/node provenance lightweight and optional.
- Preserve existing behavior and keep any flat instruction views as helpers or
  compatibility utilities, not the canonical carrier.

Completion check:

- The project builds with the common hierarchical MIR carrier.
- AArch64 module code can depend on the shared carrier without making
  target-local vectors the canonical MIR shape.
- No dispatcher path relies on incomplete replacement lowering for behavior.

### Step 2: Establish AArch64 Header And Module Skeleton

Goal: create the real public AArch64 module surface on top of the shared MIR
carrier without routing behavior through incomplete lowering yet.

Primary targets:

- `src/backend/mir/aarch64/module.hpp`
- `src/backend/mir/aarch64/module.cpp`
- build wiring required for any new implementation files introduced by this
  step

Actions:

- Convert the reviewed `module.hpp.md` vocabulary into real declarations for
  build results, AArch64 target-owned operands, registers, provenance, printer
  hooks, and compatibility projection surfaces.
- Map `Module` and AArch64 lowering products onto the shared MIR carrier from
  Step 1 instead of declaring a separate canonical target-local container.
- Keep `module.hpp` as the single non-helper public header.
- Add only minimal implementation scaffolding needed to compile the new
  surface beside the legacy route.
- Preserve existing public behavior while the replacement route is incomplete.

Completion check:

- The project builds with the new AArch64 header/module skeleton.
- No dispatcher path relies on incomplete replacement lowering for behavior.
- No component-level public header has been introduced.

### Step 3: Migrate Legacy Module-Record Compile Tests

Goal: repair stale legacy module-record compile tests after the fresh AArch64
skeleton removed the old record-pile API, without restoring the legacy module
emitter or weakening coverage.

Primary targets:

- stale compile tests and test helpers that still depend on deleted legacy
  module-record APIs
- focused new-route tests for the common MIR carrier and fresh AArch64
  skeleton behavior

Actions:

- Replace, update, or remove stale compile dependencies on deleted legacy
  record APIs such as `OperandRecord`, `FrameRecord`, broad
  `FunctionRecord` record piles, `module.globals` legacy views, and other
  old module-emitter inspection surfaces.
- Add or migrate tests toward the new route: common MIR carrier structure,
  fresh AArch64 skeleton construction, fail-closed assembly/no-machine-nodes
  behavior, and the future traversal/operand contracts that the next
  implementation step will satisfy.
- Keep this as a test-contract migration, not a test downgrade: do not mark
  supported paths unsupported, do not hide expected machine-node behavior, and
  do not claim capability progress through expectation-only edits.
- do not restore `src/backend/mir/aarch64/module.cpp` legacy emitter logic or
  recreate the old broad record pile just to satisfy stale tests.

Completion check:

- The project builds after stale module-record compile dependencies are
  migrated away from deleted APIs.
- Tests now describe the common MIR carrier, the fresh AArch64 skeleton,
  fail-closed assembly/no-machine-nodes behavior, and the future
  traversal/operand contract instead of the old record pile.
- No legacy module emitter or deleted record API has been restored.

### Step 4: Implement Function Traversal And Operand Resolution

Goal: make prepared function/block traversal and typed AArch64 operand
resolution usable by later lowering families.

Primary targets:

- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/mir/aarch64/codegen/traversal.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/codegen/operands.hpp`
- private declarations in adjacent `codegen/*` headers only when needed by
  the current target-private contract

Actions:

- Convert prepared module/function/block order into canonical MIR containers
  with lightweight provenance.
- Implement target operand/register/location conversion from prepared value
  homes, storage plans, regalloc assignments, frame slots, spill slots,
  immediates, symbols, labels, and target registers.
- Normalize storage precedence so downstream lowering does not choose among
  broad optional public records.
- Keep legacy register-string fallback fail-closed and diagnostic-only.

Completion check:

- The project builds and focused backend proof covers the traversal/operand
  surfaces that were migrated.
- Instruction, branch, and call code can consume typed operands without
  reaching back into cached display strings or broad prepared/source records.
- Current-state note: this work now lives in the `codegen/traversal.*` and
  `codegen/operands.*` surfaces; do not recreate the retired module draft
  filenames.

### Step 5: Implement One Lowering Family At A Time

Goal: move semantic target lowering into reviewed component owners while
keeping each packet narrow enough to prove.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/returns.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`

Actions:

- Migrate scalar ALU, memory, move, spill/reload, parallel-copy, and
  return-value materialization through semantic instruction families.
- Migrate terminators, conditional/unconditional branches, compare/condition
  handling, return control flow, and successor metadata.
- Migrate call-plan lowering for argument/result locations, call-adjacent
  moves, preserved values, clobbers, indirect callees, memory returns, and
  call-site ABI bindings.
- Keep each executor packet tied to one coherent capability family and its
  matching proof.

Completion check:

- Each migrated family lowers into canonical MIR nodes and has matching build
  plus backend proof.
- Compatibility records are not used as semantic lowering inputs.
- Current-state note: migrated lowering families now live in `codegen/alu.*`,
  `codegen/comparison.*`, `codegen/returns.*`, `codegen/dispatch.*`, and
  `codegen/instruction.*`; do not recreate retired module draft filenames.

### Step 6: Split Or Classify Compatibility Projection

Goal: keep legacy compatibility projection derived after lowering without
making it semantic lowering authority; classify or split the projection now
that initial codegen extraction is live.

Primary targets:

- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`, if split
- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`, if split
- build wiring required for any new `compatibility_projection.*` files

Actions:

- Inspect the compatibility projection currently embedded in `codegen/emit.cpp`
  and decide whether it should become target-private
  `codegen/compatibility_projection.*`.
- If split, move only projection derivation out of `emit.cpp`; keep lowering,
  dispatch, traversal, and printer routing in their current owners.
- If not split, classify the in-place projection clearly in `todo.md` and make
  sure `emit.cpp` names it as derived compatibility, not lowering authority.
- Keep `FunctionRecord::machine_nodes`, broad inspection records, raw
  source/prepared provenance views, label views, and fail-closed legacy
  register diagnostics derived after canonical lowering.
- Document remaining compatibility surfaces and removal conditions in
  `todo.md` during execution.
- Explicitly defer shared MIR printer routing to idea 224. `machine_printer.*`
  remains the current temporary terminal assembly route and must not be grown
  into the 228 semantic owner.

Completion check:

- Compatibility records are derived projections and cannot become fallback
  lowering authority.
- The compatibility projection is either split into
  `codegen/compatibility_projection.*` with build wiring, or classified in
  place with a clear reason in `todo.md`.
- Focused backend proof covers the compatibility projection route.
- Shared printer routing remains deferred to idea 224 and is not required for
  228 closure.

### Step 7: Rewire Dispatch And Retire Dead Legacy Ownership

Goal: make the reviewed replacement path the active owner for the migrated
surface, then delete or retire unreachable legacy code.

Primary targets:

- `src/backend/mir/aarch64/module.cpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- CMake/build wiring for the replacement component files
- retained legacy surfaces only after replacement ownership is live and proved

Actions:

- Rewire dispatch so prepared BIR flows through the replacement module,
- traversal, operand, lowering, emit, and compatibility projection components.
- Keep any retained old code explicitly classified as compatibility or
  follow-up scope.
- Delete or disconnect dead legacy code only after matching proof shows the
  replacement owner is active.
- Keep `codegen/machine_printer.*` classified as follow-up scope owned by idea
  224 unless an executor finds dead/unreachable code that can be removed
  without replacing shared printer routing.
- Run broader backend validation after dispatch/build wiring changes.

Completion check:

- The new ownership seams are actually in use for the migrated surface.
- Legacy code is deleted, disconnected, or explicitly classified.
- Shared printer routing is explicitly deferred to idea 224 rather than treated
  as a 228 closure dependency.
- Backend validation passes with no expectation downgrades.

### Step 8: Closure Review And Final Proof

Goal: prove Stage 4 completion against the source idea and phoenix reject
signals.

Primary targets:

- active implementation and build wiring touched by prior steps
- `todo.md` proof summary for final supervisor review

Actions:

- Audit implementation against the accepted Stage 3 drafts and Stage 4 source
  idea.
- Confirm every migrated capability family has build plus focused proof.
- Run broad enough validation for a closure-quality milestone.
- Record any residual legacy compatibility or target-printer follow-up as
  explicit follow-up scope instead of hiding it in the completed idea.
- Confirm the compatibility projection split/classification from Step 6 is
  complete and cannot act as semantic lowering authority.
- Confirm idea 224 still owns shared MIR printer replacement for
  `codegen/machine_printer.*`; do not require that replacement for 228 closure.

Completion check:

- The reviewed draft set has been converted into real implementation.
- The new ownership seams are active.
- Remaining legacy code is classified as compatibility or follow-up.
- Compatibility projection is split or explicitly classified.
- Shared printer routing is recorded as idea 224 follow-up scope, not hidden
  228 work.
- Proof shows migrated capability families still work.

## Acceptance

The active idea is complete only when the reviewed draft set has been converted
into real implementation, the new ownership seams are actually in use,
remaining legacy code is explicitly classified as compatibility or follow-up
scope, direct prepared-BIR-to-MIR machine-node lowering is the active route for
the migrated surface, compatibility projection is split or explicitly
classified as derived state, shared MIR printer replacement is deferred to
idea 224 when still needed, and proof shows the migrated capability families
still work.
