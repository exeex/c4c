# RV64 Explicit-Register Inline-Assembly BIR Syntax Runbook

Status: Active
Source Idea: ideas/open/726_rv64_explicit_register_inline_asm_bir_syntax.md
Activated after closing: ideas/closed/725_rv64_explicit_register_inline_asm_syntax_research.md

## Purpose

Implement the research-approved syntax-only seam before resuming any prepared
allocation work.

## Goal

Make canonical RV64 x-register operand constraints structured semantic BIR
authority with source-backed proof and precise fail-closed negatives.

## Core Rule

Change only the BIR grammar/metadata seam. Do not enter prepared allocation,
regalloc, publication, or target emission.

## Read First

- `ideas/open/726_rv64_explicit_register_inline_asm_bir_syntax.md`
- `docs/rv64_explicit_register_inline_asm/index.md`
- `docs/rv64_explicit_register_inline_asm/02_narrow_support_decision.md`

## Current Scope

- Canonical RV64 `{xN}`, `={xN}`, and `+{xN}`, N in 0..31.
- Structured BIR bank/index metadata and target-aware classification.
- Source positive and focused malformed/unsupported/clobber proof.

## Non-Goals

- No prepared/regalloc/home/publication/emission work.
- No aliases, other banks/targets, alternatives, or broad parser changes.
- No fixture-only positive or arbitrary named-value control.

## Execution Rules

- Preserve existing raw transport and role rendering.
- Parse complete canonical tokens, never substrings.
- Keep clobber agreement unchanged and semantically separate.
- Stop if target profile or source positive requires broad interface redesign.

## Ordered Steps

### Step 1: Add structured BIR syntax classification

Goal: classify the bounded canonical RV64 family at its first grammar owner.

Actions:

- Add the BIR-owned explicit GPR bank/index/spelling fact.
- Supply target profile to `make_inline_asm_metadata`.
- Parse input/output/read-write canonical x-register tokens.
- Publish precise wrong-target and malformed/unsupported facts.

Completion check:

- Direct LIR-to-BIR proof shows exact roles, indices, structured identities,
  and fail-closed malformed behavior without touching prepared allocation.

### Step 2: Prove the genuine source route and compatibility

Goal: establish semantic capability rather than fixture metadata.

Actions:

- Add source-backed input, scalar output, and read/write RV64 cases.
- Verify source-to-LIR spelling and semantic BIR structured facts.
- Re-run existing class/vector/tie/immediate/memory/address/clobber coverage.

Completion check:

- Source positives are structured and no existing token family regresses;
  clobbers remain separate.

### Step 3: Run focused acceptance and disposition

Goal: accept the bounded syntax slice before resuming idea 724.

Actions:

- Run the supervisor-selected matching build/test proof.
- Review against reject signals and confirm no prepared/regalloc/emission diff.
- Hand lifecycle disposition back to the plan owner.

Completion check:

- Focused proof has no new failures beyond the accepted inline-asm baseline,
  and the diff stays within syntax/BIR/tests/todo scope.
