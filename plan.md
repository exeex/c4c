# AArch64 Module Phoenix Stage 3 Draft Runbook

Status: Active
Source Idea: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Activated from: Stage 2 closure artifacts under `src/backend/mir/aarch64/module/`

## Purpose

Draft the AArch64 module phoenix replacement as markdown artifacts only, using
the accepted Stage 2 layout and handoff as the intake contract for Stage 3.

Goal: produce the complete replacement `.cpp.md` / `.hpp.md` draft set and a
draft-review artifact without editing real implementation, build, or test
files.

## Core Rule

Stage 3 is draft-only. Do not edit real `.cpp`, `.hpp`, build, or test files,
and do not delete or disconnect the compiled legacy `module.cpp`.

## Read First

- `.codex/skills/phoenix-rebuild/SKILL.md`
- `ideas/open/227_aarch64_module_phoenix_replacement_drafts.md`
- `src/backend/mir/aarch64/module/stage2_review_layout.md`
- `src/backend/mir/aarch64/module/stage2_to_stage3_handoff.md`
- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.md`

## Current Targets

Stage 3 must produce exactly the initial replacement draft map declared by
Stage 2.

Directory index draft:

- `src/backend/mir/aarch64/module/module.md`

Mandatory header draft:

- `src/backend/mir/aarch64/module/module.hpp.md`

Mandatory implementation drafts:

- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/function_traversal.cpp.md`
- `src/backend/mir/aarch64/module/operand_resolution.cpp.md`
- `src/backend/mir/aarch64/module/instruction_lowering.cpp.md`
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp.md`
- `src/backend/mir/aarch64/module/call_lowering.cpp.md`
- `src/backend/mir/aarch64/module/public_assembly_bridge.cpp.md`
- `src/backend/mir/aarch64/module/compatibility_projection.cpp.md`

Stage 3 must also produce one review artifact for the replacement draft set.

## Non-Goals

- Do not edit real `.cpp` or `.hpp` implementation files.
- Do not edit build wiring or tests.
- Do not change expectations or downgrade supported paths.
- Do not draft extra component files beyond the Stage 2 map unless the Stage 2
  contract is repaired first.
- Do not add component-level public headers.
- Do not introduce `helper.hpp` unless the executor explicitly justifies the
  allowed private-helper exception before drafting it.
- Do not preserve the legacy catch-all module emitter under renamed helpers.
- Do not use broad public inspection records, cached display strings, spelling
  recovery, or flat `FunctionRecord::machine_nodes` as semantic lowering
  authority.

## Working Model

- `prepare::PreparedBirModule` drives module dispatch, function traversal,
  operand resolution, instruction lowering, branch/control lowering, call
  lowering, public assembly bridging, and compatibility projection.
- Canonical output is hierarchical typed MIR: module, function, block, and
  instruction or equivalent typed concepts.
- `FunctionRecord::machine_nodes` is compatibility-only and must be projected
  after canonical MIR lowering.
- Origin/provenance is lightweight metadata: stable IDs, indices, labels,
  durable source locations, and reason tags for prepared authority.
- The shared `mir_printer` owns traversal and common emission mechanics.
  AArch64 owns opcode, operand, register, memory, label, symbol, and immediate
  rendering.
- Target render APIs must use ordinary C++ names, not `__repr__`.

## Execution Rules

- Treat `stage2_review_layout.md` and `stage2_to_stage3_handoff.md` as the
  authoritative Stage 3 contract.
- Partition drafts by responsibility and dependency direction, not by legacy
  source line ranges.
- Each draft file must state owned responsibility, owned inputs, owned outputs,
  indirect queries, forbidden knowledge, and whether it is dispatch, core
  lowering, optional fast path, public bridge, or compatibility.
- Preserve the parent 224 judgment: canonical MIR blocks plus target-owned
  printable nodes, with public records and flat vectors only as projections.
- If a planned file must be added or removed, stop and request Stage 2 contract
  repair instead of silently drifting.
- If a draft still requires two components to know each other's full internals,
  treat the seam as not clean enough and record the blocker in `todo.md`.

## Ordered Steps

### Step 1: Draft Replacement Index And Header Contract

Goal: establish the Stage 3 replacement vocabulary before component drafts
depend on it.

Primary targets:

- `src/backend/mir/aarch64/module/module.md`
- `src/backend/mir/aarch64/module/module.hpp.md`

Actions:

- Rewrite the directory index draft around the direct
  prepared-BIR-to-typed-MIR module boundary.
- Draft the single non-helper public header contract for `BuildResult`,
  `Module`, canonical MIR carrier concepts, compatibility projections,
  lightweight provenance, and target-owned printable surfaces.
- List the exact implementation draft set from Stage 2.
- Preserve the no-extra-public-header rule and the `helper.hpp` exception
  conditions.
- Do not edit real implementation or build files.

Completion check:

- Both targets are Stage 3 replacement drafts rather than Stage 1 extraction
  notes.
- The header draft defines the vocabulary needed by all later implementation
  drafts.
- The index points at the complete mandatory Stage 3 artifact map.

### Step 2: Draft Dispatch And Traversal Seams

Goal: draft the top-level module path and per-function traversal ownership.

Primary targets:

- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/function_traversal.cpp.md`

Actions:

- Draft module dispatch for target profile resolution, AArch64 prepared-handoff
  validation, top-level data/relocation orchestration, and public product
  construction from completed canonical MIR functions.
- Draft function traversal for prepared function/block order, lowering context
  construction, MIR function/block creation, and optional debug provenance.
- Keep module dispatch out of operation-specific lowering and keep traversal
  out of public flat `machine_nodes` authority.

Completion check:

- The dispatch and traversal drafts agree with the Step 1 carrier and
  provenance vocabulary.
- No draft keeps the legacy broad record assembler as the replacement driver.

### Step 3: Draft Operand Authority

Goal: draft the single typed authority model for AArch64 operands and prepared
value locations.

Primary target:

- `src/backend/mir/aarch64/module/operand_resolution.cpp.md`

Actions:

- Draft conversion from prepared value homes, storage plans, regalloc
  assignments, frame slots, spill slots, immediates, symbols, labels, and
  target registers into target operand/register representations.
- Normalize storage precedence so downstream components do not choose among
  broad optional public records.
- Isolate any legacy register-string fallback as fail-closed compatibility.

Completion check:

- Instruction, branch, and call drafts can consume typed operands without
  reaching back into broad prepared tables or source spellings.

### Step 4: Draft Instruction And Control Lowering

Goal: draft semantic target lowering families for non-call operations and
control flow.

Primary targets:

- `src/backend/mir/aarch64/module/instruction_lowering.cpp.md`
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp.md`

Actions:

- Draft scalar ALU, memory, move, spill/reload, parallel-copy, and return-value
  materialization as semantic families.
- Draft prepared terminator lowering, conditional/unconditional branches,
  compare/condition handling, valid branch fusion, return control flow, and
  successor metadata.
- Reject named-testcase operation expansion and cached display-string
  authority.

Completion check:

- Drafts lower into canonical MIR machine nodes and do not depend on
  compatibility projections for semantic decisions.

### Step 5: Draft Call Lowering

Goal: draft prepared call-plan lowering as one coherent ABI-owned component.

Primary target:

- `src/backend/mir/aarch64/module/call_lowering.cpp.md`

Actions:

- Draft argument/result locations, call-adjacent moves, preserved values,
  clobbers, indirect callees, memory returns, and call-site ABI bindings.
- Coordinate with operand resolution for target locations while keeping call
  semantics in the call-lowering draft.

Completion check:

- Calls produce canonical target MIR nodes and inspection records are described
  as projections of lowered call sequence plus prepared ABI facts.

### Step 6: Draft Public Bridge And Compatibility Projection

Goal: draft the emission bridge and migration-only compatibility surfaces after
canonical lowering exists.

Primary targets:

- `src/backend/mir/aarch64/module/public_assembly_bridge.cpp.md`
- `src/backend/mir/aarch64/module/compatibility_projection.cpp.md`

Actions:

- Draft the bridge from canonical MIR functions and module data to the shared
  `mir_printer` traversal.
- Keep AArch64 ownership limited to target rendering surfaces and data hooks,
  not traversal or `.s` file structure.
- Draft compatibility projection for `FunctionRecord::machine_nodes`, broad
  inspection records, raw source/prepared provenance, label views, and
  fail-closed legacy register diagnostics.

Completion check:

- Printing is routed through the shared `mir_printer`.
- Compatibility records are derived after canonical lowering and cannot become
  semantic authority.

### Step 7: Review Draft Set For Stage 4 Readiness

Goal: produce the Stage 3 draft-review artifact.

Primary target:

- `review/<stage3-aarch64-module-draft-review>.md`

Actions:

- Review the complete draft set against the Stage 2 handoff, parent 224
  judgment, phoenix rules, and idea 227 reviewer reject signals.
- Confirm whether the set is coherent enough for implementation conversion.
- Record any blockers as draft issues, not implementation edits.

Completion check:

- The review artifact says whether the draft set is ready for Stage 4.
- Any rejection is tied to concrete draft files and Stage 2 contract language.

## Acceptance

The active idea is complete only when every mandatory draft target exists in
Stage 3 replacement form, no extra public headers or undeclared component files
have been introduced, the required review artifact exists, and that review says
the draft set is coherent enough for implementation conversion.
