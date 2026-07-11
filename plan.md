# Structured Layout Bridge Isolation Runbook

Status: Active
Source Idea: ideas/open/687_structured_layout_bridge_isolation.md
Activated from: LIR -> BIR adapter boundary first wave, order 3 of 6
Previous Idea: ideas/open/686_private_detail_header_contraction.md

## Purpose

Isolate structured type and layout compatibility inside the `LIR -> BIR`
adapter so legacy type text parsing, type declarations, typed operand parsing,
and layout fallback maps remain import-local contracts.

## Goal

Make the structured layout bridge narrower and clearer without changing BIR
output, diagnostics, MIR/object output, runtime behavior, tests,
expectations, unsupported markers, allowlists, or harness policy.

## Core Rule

This is a behavior-preserving adapter bridge cleanup. Do not claim progress
through target aggregate transport changes, byval ABI placement changes,
prepared storage changes, expectation rewrites, unsupported downgrades, or
testcase-shaped layout matching.

## Read First

- `ideas/open/687_structured_layout_bridge_isolation.md`
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`

## Current Targets

- Structured type and layout adapter files:
  - `src/backend/bir/lir_to_bir/types.cpp`
  - `src/backend/bir/lir_to_bir/aggregate.cpp`
  - `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- Import-local layout state:
  - legacy type text parsing
  - type declarations and `TypeDeclMap`
  - typed operand parsing
  - structured layout fallback maps
  - aggregate layout lookup state
- Adjacent private adapter declarations only when they are required to narrow
  the structured layout bridge.

## Non-Goals

- Do not edit canonical BIR type/model authority, public BIR route schemas,
  target aggregate transport lanes, byval ABI placement, prepared storage
  layout, MIR consumers, tests, expectations, unsupported markers, allowlists,
  runtime behavior, or harness policy.
- Do not move raw LIR type spelling maps into public BIR, prepared/prealloc,
  target, or MIR ownership.
- Do not turn `memory_helpers.hpp` into a stateful lowerer-policy destination.
- Do not combine this work with initializer, memory/provenance, call ABI, BIR
  semantic model, or prepared/prealloc changes.
- Do not hide semantic layout repair inside bridge isolation.

## Working Model

The structured layout bridge is an adapter-owned compatibility layer between
raw LIR spelling and stable BIR facts. The cleanup should make that bridge
explicit, keep compatibility maps private to import, and use
`memory_helpers.hpp` only as a narrow pure layout/projection helper precedent.

The first executor packet should start by measuring current structured
type/layout ownership and selecting one behavior-preserving contraction that
does not cross into target, prepared, MIR, or public BIR policy.

## Execution Rules

- Prefer small behavior-preserving steps that compile before continuing.
- Keep source-idea edits unnecessary unless durable source intent changes.
- Record packet progress and proof in `todo.md`.
- Keep `TypeDeclMap`, structured layout fallback maps, typed operand parsing,
  and aggregate layout lookup state import-local unless a later source idea
  proves a target-neutral public BIR contract.
- Localize or narrow adapter-private helpers only when doing so reduces a real
  structured layout boundary.
- If a packet discovers initializer, memory/provenance, call ABI, BIR semantic
  model, prepared storage, or target ABI work, stop and route that to the
  appropriate later idea instead of expanding this plan.
- Reject helper renames or classification-only edits if they retain the same
  broad fallback behavior behind a new abstraction name.

## Ordered Steps

### Step 1: Inventory Structured Layout Bridge Ownership

Goal: identify the current structured type/layout compatibility state and
select the first behavior-preserving contraction.

Primary targets:
- `src/backend/bir/lir_to_bir/types.cpp`
- `src/backend/bir/lir_to_bir/aggregate.cpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- adjacent adapter-private declarations used by these files

Actions:
- Read the source idea and handoff docs listed in `Read First`.
- Inventory type text parsing, `TypeDeclMap`, typed operand parsing,
  structured layout fallback maps, and aggregate layout lookup responsibilities.
- Classify each responsibility as adapter-private compatibility, stable BIR
  fact production, pure layout projection, or downstream policy.
- Select one narrow first packet that reduces or clarifies adapter-private
  structured layout ownership without changing behavior.

Completion check:
- The selected packet is recorded in `todo.md`, names its owned files, and
  explains why it is structured-layout bridge work rather than public BIR,
  prepared/prealloc, target, MIR, initializer, memory/provenance, or call ABI
  work.

### Step 2: Isolate Type Declaration And Typed Operand Compatibility

Goal: keep legacy type declarations and typed operand parsing private to the
adapter boundary that consumes raw LIR spelling.

Primary targets:
- `src/backend/bir/lir_to_bir/types.cpp`
- adapter-private declarations supporting `TypeDeclMap` or typed operand
  parsing

Actions:
- Identify declarations or helpers whose only purpose is importing legacy type
  spelling into structured BIR facts.
- Localize those helpers or move them behind a narrower adapter-owned
  contract when existing cross-file use requires it.
- Preserve produced BIR facts, diagnostics, dump output, and downstream
  behavior.

Completion check:
- Type declaration and typed operand compatibility are no more public than
  before, and the affected adapter translation units compile.

### Step 3: Clarify Aggregate Layout Lookup Boundaries

Goal: separate import-local aggregate layout lookup and fallback maps from
public BIR, prepared/prealloc, target, or MIR policy.

Primary targets:
- `src/backend/bir/lir_to_bir/aggregate.cpp`
- structured layout fallback map ownership
- related adapter-private aggregate layout helpers

Actions:
- Trace current aggregate layout lookup paths from LIR import through produced
  BIR facts.
- Narrow fallback-map ownership or helper boundaries only where the bridge
  remains adapter-owned.
- Keep target aggregate transport lanes, byval ABI placement, prepared storage
  layout, and MIR consumers unchanged.

Completion check:
- Aggregate layout fallback state remains import-local and the selected
  contraction preserves existing output and diagnostics.

### Step 4: Keep `memory_helpers.hpp` Pure And Narrow

Goal: use `memory_helpers.hpp` only as a pure layout/projection helper
boundary, not as a destination for stateful lowerer policy.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- callers in structured layout import paths

Actions:
- Inspect whether structured layout helpers in `memory_helpers.hpp` are pure
  projections or have begun to accumulate lowerer state or policy.
- Move or narrow declarations only when the result keeps stateful import
  policy out of the helper.
- Avoid changing public BIR memory route behavior or memory/provenance import
  ownership.

Completion check:
- Any touched helper boundary is narrower or more clearly pure, and no
  stateful structured layout policy is moved into `memory_helpers.hpp`.

### Step 5: Prove Behavior Preservation

Goal: validate structured layout bridge isolation with fresh focused proof.

Actions:
- Run a fresh build or compile proof that covers the touched adapter
  translation units.
- Run focused existing type/layout adapter coverage or a dump-equivalence proof
  selected by the supervisor.
- Escalate proof only if a packet touches shared type/model or downstream
  layout surfaces.

Completion check:
- `todo.md` records the exact proof command and result.
- No BIR, MIR, object, runtime, diagnostic, expectation, unsupported-marker,
  allowlist, or harness-policy behavior changed.
