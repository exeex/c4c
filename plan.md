# Private Detail Header Contraction Runbook

Status: Active
Source Idea: ideas/open/686_private_detail_header_contraction.md
Activated from: LIR -> BIR adapter boundary first wave, order 2 of 6
Previous Idea: ideas/open/685_lir_import_context_extraction.md

## Purpose

Contract `src/backend/bir/lir_to_bir/lowering.hpp` and adjacent private
adapter helper boundaries so split adapter translation units see only the
declarations they need.

## Goal

Make the private LIR-to-BIR adapter declaration surface narrower and clearer
without changing BIR output, diagnostics, notes, result-envelope behavior,
unsupported markers, tests, expectations, allowlists, object output, or runtime
behavior.

## Core Rule

This is a behavior-preserving private-header cleanup. Do not claim progress
through semantic lowering repair, expectation changes, unsupported downgrades,
public BIR model changes, or testcase-shaped shortcuts.

## Read First

- `ideas/open/686_private_detail_header_contraction.md`
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`

## Current Targets

- Primary private adapter detail header:
  - `src/backend/bir/lir_to_bir/lowering.hpp`
- Split LIR import implementation files under:
  - `src/backend/bir/lir_to_bir/`
- Private adapter helper declarations that are currently wider than their
  cross-translation-unit consumers require.
- Import-local state families that must remain private to LIR import:
  - `ValueMap`
  - `GlobalTypes`
  - `TypeDeclMap`
  - `FunctionSymbolSet`
  - CFG and phi scratch maps
  - memory side tables
  - raw LIR spelling and prescan state

## Non-Goals

- Do not edit public BIR route schemas, public BIR query surfaces, route
  records, printer, validator, prepared/prealloc products, lookup bundles,
  frame/stack/call/storage products, MIR consumers, target emission, tests,
  expectations, unsupported markers, allowlists, runtime behavior, or harness
  policy.
- Do not move public BIR model records into `src/backend/bir/lir_to_bir/`.
- Do not move adapter-private LIR spelling maps or scratch state into public
  BIR, prepared/prealloc, target, or MIR ownership.
- Do not combine this work with structured layout, initializer,
  memory/provenance, call ABI, BIR semantic model, or prepared handoff changes.
- Do not hide family-specific semantic repair inside header contraction.

## Working Model

`lowering.hpp` is a private adapter declaration surface, not a public BIR model
boundary. The cleanup should remove declarations that can be owned locally,
group declarations only when there is a real adapter-private ownership reason,
and introduce narrow helper headers only when they reduce actual
cross-translation-unit coupling.

The first executor packet should start by measuring current `lowering.hpp`
responsibilities and selecting one declaration family whose ownership can be
narrowed without changing behavior.

## Execution Rules

- Prefer small behavior-preserving steps that compile before continuing.
- Keep source-idea edits unnecessary unless durable source intent changes.
- Record packet progress and proof in `todo.md`.
- Localize declarations inside an existing translation unit when no other
  translation unit needs them.
- Introduce a private helper header only when localizing inside one
  translation unit is not viable and the helper boundary is clearly narrower
  than the old detail surface.
- If a packet discovers structured layout, initializer, memory/provenance, or
  call ABI semantic work, stop and route that to the later ordered idea instead
  of expanding this plan.
- If a proposed change only renames helpers while preserving the same broad
  responsibility pile, reject it as insufficient progress.

## Ordered Steps

### Step 1: Inventory Private Detail Header Responsibilities

Goal: identify the current declarations exposed by `lowering.hpp` and decide
which private family can be contracted first.

Primary targets:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- split implementation files under `src/backend/bir/lir_to_bir/`

Actions:
- Read the source idea and handoff docs listed in `Read First`.
- Inventory the top-level declaration families in `lowering.hpp`, including
  contexts, state maps, prescan facts, helper functions, private structs, and
  cross-file entry points.
- For each candidate family, classify whether it is used by one translation
  unit, several adapter translation units, or a non-adapter surface.
- Select one narrow first packet that reduces declaration width or clarifies
  private ownership without crossing into later ordered ideas.

Completion check:
- The selected packet is recorded in `todo.md`, names its owned files, and
  explains why it is adapter-private and behavior-preserving.

### Step 2: Localize Single-Translation-Unit Declarations

Goal: remove declarations from `lowering.hpp` when an existing implementation
file can own them locally.

Primary targets:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- the selected owning `.cpp` file under `src/backend/bir/lir_to_bir/`

Actions:
- Pick declarations from the Step 1 inventory that have only one real
  implementation-file consumer.
- Move those declarations into the owning `.cpp` file or an existing local
  namespace when that preserves behavior and include dependencies.
- Keep import-local state private to LIR import and out of public BIR,
  prepared/prealloc, target, and MIR layers.

Completion check:
- `lowering.hpp` exposes fewer declarations than before for the selected
  family, and the affected adapter translation units compile.

### Step 3: Narrow Cross-Translation-Unit Helper Boundaries

Goal: replace broad private-header coupling with a smaller adapter-owned
contract only where cross-file use is real.

Primary targets:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- selected private helper declarations or helper headers under
  `src/backend/bir/lir_to_bir/`

Actions:
- Choose one declaration family that still needs cross-translation-unit use
  after Step 2.
- Extract or regroup only the declarations required by that family.
- Prefer an existing private helper boundary when it can become narrower
  without becoming a dumping ground.
- Avoid changing public route schemas, prepared products, target codegen, or
  downstream behavior.

Completion check:
- The chosen cross-file helper boundary is narrower than the old detail
  surface and has a concrete adapter-private ownership reason.

### Step 4: Preserve Import-Local State Authority

Goal: ensure header contraction does not leak adapter-private authority into
public or downstream layers.

Primary targets:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- helper boundaries touched by Steps 2 and 3
- implementation files that own `ValueMap`, `GlobalTypes`, `TypeDeclMap`,
  `FunctionSymbolSet`, CFG/phi scratch maps, and memory side tables

Actions:
- Check that import-local maps and scratch state remain owned by the LIR import
  layer.
- Remove accidental includes or declarations that make downstream layers depend
  on adapter-private state.
- If a downstream dependency appears necessary, stop and route it to the
  correct later idea instead of expanding this plan.

Completion check:
- No downstream layer starts depending on adapter-private state, and any new
  private boundary is narrower than `lowering.hpp`.

### Step 5: Prove Behavior Preservation

Goal: validate the private-header contraction with fresh focused proof.

Actions:
- Run a fresh build or compile proof that covers the touched adapter
  translation units.
- Run the focused BIR adapter or LIR-to-BIR test subset selected by the
  supervisor when the changed declarations affect callable lowering behavior.
- Escalate only if a shared non-adapter header or downstream surface was
  touched.

Completion check:
- `todo.md` records the exact proof command and result.
- No expectation, unsupported marker, allowlist, runtime behavior, object
  output, or baseline policy changed.
