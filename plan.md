# Prealloc Inline Asm Memory Effect Metadata Contract Runbook

Status: Active
Source Idea: ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md

## Purpose

Make prealloc inline-asm stack placement consequences consume structured BIR
memory/address metadata whenever available, and document any remaining
unstructured conservative summary path.

## Goal

Classify inline-asm memory/address effect consumers in prealloc stack-layout
analysis, prefer structured metadata where present, and prove a structured
inline-asm memory operand drives the intended placement consequence.

## Core Rule

BIR owns inline-asm and intrinsic memory/address semantic facts when represented
by `InlineAsmOperandMetadata`, `MemoryAddress`, or `IntrinsicOperation`
metadata. Prealloc owns placement consequences such as conservative home slots,
target-facing operand storage, and stack object retention. Do not move
placement into BIR, and do not leave raw use-shape scans as undocumented memory
provenance authority.

## Read First

- `ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md`
- `src/backend/prealloc/stack_layout/inline_asm.cpp`
- Related stack-layout hint consumers for inline-asm memory/address effects
- BIR inline-asm operand metadata and `MemoryAddress` publication surfaces
- Existing backend tests for inline asm, stack layout, and prepared memory
  publication

## Current Targets

- Inline-asm memory/address effect tracing in
  `src/backend/prealloc/stack_layout/inline_asm.cpp`.
- Stack-layout hint consumers affected by inline-asm memory operands.
- Structured `InlineAsmOperandMetadata` and `MemoryAddress` consumers.
- Retained raw or conservative summary paths, if any, as named safety policy.

## Non-Goals

- Do not rewrite inline asm parsing or target asm constraint lowering broadly.
- Do not move stack placement, register homes, or operand storage decisions
  into BIR.
- Do not change unrelated intrinsic memory lowering unless it shares the same
  metadata contract.
- Do not weaken inline-asm expectations or mark supported cases unsupported.

## Working Model

Inline asm and intrinsic memory operands are semantic BIR facts when represented
by structured operand or memory/address metadata. Prealloc may conservatively
retain stack homes or operand storage because of those facts, but the fact
source must be explicit. Any raw-shape fallback must be named as conservative
placement policy with clear limits rather than semantic memory provenance
authority.

## Execution Rules

- Keep inventory and classification notes in `todo.md`.
- Prefer structured BIR inline-asm memory/address metadata where present.
- Name any retained raw summary path and document it as conservative placement,
  not BIR provenance authority.
- Preserve prealloc ownership of stack placement, homes, and target-facing
  operand storage.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Escalate validation if shared stack-layout, inline-asm metadata, or intrinsic
  memory interfaces change.

## Step 1: Inventory Inline Asm Memory Effect Consumers

Goal: trace inline-asm memory/address effect consumers and identify their
current fact sources.

Primary targets:

- `src/backend/prealloc/stack_layout/inline_asm.cpp`
- Related stack-layout hint consumers
- BIR inline-asm operand metadata and `MemoryAddress` producers
- Existing inline-asm and stack-layout tests

Actions:

- Trace how inline-asm memory operands influence stack object publication or
  home-slot retention.
- Identify consumers of structured `InlineAsmOperandMetadata` and
  `MemoryAddress` facts.
- Identify raw use-shape or conservative summary paths.
- Separate semantic memory/address facts from prealloc placement consequences.
- Record candidate contract gaps and proof gaps in `todo.md`.

Completion check:

- `todo.md` lists each inline-asm memory effect route, current fact source, and
  placement consequence.
- Analysis proof confirms no implementation diff under `src/backend/bir` or
  `src/backend/prealloc`.

## Step 2: Decide The Metadata Contract

Goal: decide which inline-asm placement summaries must consume structured
metadata and which raw summaries may remain as named conservative policy.

Actions:

- Compare Step 1 findings against the source idea acceptance criteria.
- Decide the structured metadata authority for inline-asm memory/address
  operands.
- Decide whether any unstructured conservative summary path is legitimate and
  name its limits.
- Identify focused proof for a structured inline-asm memory operand driving
  stack publication or home-slot retention.
- Reject routes that only rename raw instruction-shape scans while preserving
  them as semantic authority.

Completion check:

- `todo.md` records the metadata contract, implementation targets, retained
  conservative paths if any, and proof targets.
- Analysis proof confirms no implementation diff unless this packet
  intentionally begins implementation.

## Step 3: Implement Or Document The Contract

Goal: make inline-asm stack-layout consequences consume the chosen structured
metadata or named conservative summary policy.

Actions:

- Update inline-asm stack-layout consumers to prefer structured
  `InlineAsmOperandMetadata` or `MemoryAddress` facts where present.
- Document or isolate any retained unstructured conservative summary as
  placement policy, not provenance authority.
- Preserve stack placement, register homes, and operand storage decisions in
  prealloc.
- Avoid unrelated inline-asm parser, target constraint, intrinsic, or stack
  layout redesign.

Completion check:

- The default build passes.
- Structured metadata is the visible authority when available.
- Any retained raw summary path is named, bounded, and not used to claim BIR
  memory provenance authority.

## Step 4: Add Focused Inline Asm Proof

Goal: prove that a structured inline-asm memory operand drives the intended
prealloc placement consequence.

Actions:

- Add or strengthen a test for an inline-asm memory operand represented through
  structured metadata.
- Assert the relevant stack publication or home-slot retention consequence.
- Add fail-closed or fallback-policy proof if an unstructured summary is
  retained.
- Prefer prepared stack-layout or contract assertions over asm string spelling
  symptoms.

Completion check:

- The default build passes.
- Focused inline-asm/stack-layout tests pass.
- Tests do not weaken inline-asm memory expectations or mark cases unsupported.

## Step 5: Final Validation And Close Readiness

Goal: prove the completed route and prepare the source idea for closure review.

Actions:

- Run the default build and the relevant backend inline-asm/stack-layout subset.
- Include broader `^backend_` validation if shared stack-layout, inline-asm
  metadata, or intrinsic memory interfaces changed.
- Update `todo.md` with final proof, retained conservative summary details if
  any, and close-readiness notes.

Completion check:

- Final delegated proof passes.
- `todo.md` shows structured inline-asm memory operand coverage and the
  intended prealloc placement consequence.
- The source idea acceptance criteria are satisfied without moving placement
  decisions into BIR or broad inline-asm/intrinsic redesign.
