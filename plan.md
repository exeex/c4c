# RV64 Explicit-Register Inline-Assembly Syntax Research Runbook

Status: Active
Source Idea: ideas/open/725_rv64_explicit_register_inline_asm_syntax_research.md
Activated after parking: ideas/open/724_prepared_inline_asm_explicit_register_allocation_constraints.md

## Purpose

Resolve the syntax and ownership premise that blocked idea 724 before any new
inline-assembly or allocation capability is implemented.

## Goal

Produce exactly three documents under
`docs/rv64_explicit_register_inline_asm/` with a source-to-BIR trace and a
narrow support-or-stop decision.

## Core Rule

Physical register validation after allocation is not evidence of semantic
operand ingress. Prove the source/LIR route and preserve clobber semantics.

## Read First

- `ideas/open/725_rv64_explicit_register_inline_asm_syntax_research.md`
- `ideas/open/724_prepared_inline_asm_explicit_register_allocation_constraints.md`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/codegen/lir/verify.cpp`

## Current Scope

- Current source, LIR, verifier, and BIR constraint token flow.
- RV64 explicit-register operand syntax ownership and structured metadata.
- Narrow support versus stop decision.

## Non-Goals

- Do not edit implementation, tests, expectations, or runtime behavior.
- Do not enter regalloc, publication, or target emission.
- Do not treat clobbers or fixture metadata as value constraints.

## Execution Rules

- Use concrete code and test evidence for every accepted/rejected token claim.
- Separate current fact, inference, and proposed syntax.
- Keep exactly one answer per research question plus the index.
- Prefer a stop decision over broad syntax redesign.

## Ordered Steps

### Step 1: Trace source-to-BIR constraint syntax

Goal: document accepted tokens and the exact explicit-register discontinuity.

Actions:

- Trace source inline assembly through LIR storage and verification.
- Trace token classification in `make_inline_asm_metadata`.
- Distinguish input/output, ties, vector classes, and clobbers.
- Write `docs/rv64_explicit_register_inline_asm/01_source_to_bir_syntax_flow.md`.

Completion check:

- The document identifies the earliest owner/rejection boundary with concrete
  symbols and proves clobbers are semantically separate.

### Step 2: Decide whether narrow syntax support is valid

Goal: choose one bounded semantic syntax/metadata route or stop.

Actions:

- Compare source/LIR/BIR ownership options.
- Define structured identity, validation, malformed-input, and proof needs.
- Write `docs/rv64_explicit_register_inline_asm/02_narrow_support_decision.md`.

Completion check:

- One implementable narrow route is selected without broad redesign, or the
  document gives evidence to keep idea 724 parked.

### Step 3: Integrate and review the syntax result

Goal: complete the exact research output and reject-signal review.

Actions:

- Write `docs/rv64_explicit_register_inline_asm/index.md`.
- Verify exact file count, links, consistency, acceptance criteria, and reject signals.

Completion check:

- Exactly three required files exist and clearly recommend a narrow follow-up
  or stop decision without implementation changes.
