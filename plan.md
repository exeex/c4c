# AArch64 Direct LIR Aggregate Type Bridge Retirement Runbook

Status: Active
Source Idea: ideas/open/180_aarch64_direct_lir_aggregate_type_bridge_retirement.md

## Purpose

Retire or fence one AArch64 direct-LIR aggregate type text bridge now that
structured `LirTypeRef` equality and aggregate ABI facts exist.

## Goal

Make one bounded AArch64/direct-LIR aggregate route stop treating rendered type
text as normal semantic authority for metadata-rich inputs.

## Core Rule

Prefer structured `LirTypeRef` ids and aggregate layout/ABI facts over rendered
aggregate type text; keep text parsing only as an explicit no-id legacy
compatibility boundary.

## Read First

- ideas/open/180_aarch64_direct_lir_aggregate_type_bridge_retirement.md
- ideas/closed/174_aggregate_abi_classification_structured_facts.md
- ideas/closed/176_lir_type_ref_structured_equality.md

## Current Scope

- AArch64 direct-LIR aggregate type parser or fallback usage.
- One route where metadata-rich inputs can require or propagate structured
  aggregate type facts.
- Focused tests for structured success and stale/missing structured metadata.

## Non-Goals

- Do not retire every direct-LIR parser in this runbook.
- Do not change ABI printer syntax or final assembly output.
- Do not rewrite non-AArch64 backend ABI paths.
- Do not treat hand-authored no-id LIR fixtures as generated metadata-rich
  inputs.
- Do not add new rendered type parser branches.

## Working Model

- Metadata-rich generated inputs should carry enough structured identity for
  aggregate semantics.
- Rendered type text is acceptable for display, diagnostics, ABI spelling, or
  explicit legacy no-id direct-LIR fixtures.
- Missing or stale structured aggregate facts on the selected metadata-rich
  route should fail closed instead of silently reparsing text.

## Execution Rules

- Keep each step narrow and behavior-preserving except for the selected
  compatibility boundary.
- Inventory before changing behavior so the selected route is explicit.
- Tests must prove semantic authority moved to structured facts, not just that
  output text still matches.
- Use build proof for code-changing steps and targeted AArch64/direct-LIR or
  backend ABI coverage for acceptance.
- If the work reveals a broader bridge retirement or unrelated ABI initiative,
  record it as a separate idea instead of expanding this runbook.

## Steps

### Step 1: Inventory AArch64 Direct-LIR Aggregate Text Bridges

Goal: identify the concrete parser/fallback sites that can still treat rendered
aggregate type text as semantic authority on AArch64 direct-LIR routes.

Concrete actions:
- Search AArch64 backend and direct-LIR preparation paths for aggregate type
  text parsing, rendered type fallback, and legacy no-id handling.
- Classify each site as metadata-rich generated input, explicit no-id fixture
  compatibility, display/diagnostic use, or unrelated output spelling.
- Select one bounded route where structured aggregate metadata can be required
  or propagated without broad ABI rewrites.

Completion check:
- `todo.md` records the selected route, the sites inspected, and the proposed
  proof subset before behavior changes begin.

### Step 2: Require Structured Facts on the Selected Route

Goal: make the selected AArch64/direct-LIR aggregate route use structured
`LirTypeRef` ids and aggregate layout/ABI facts instead of rendered type text
for metadata-rich inputs.

Concrete actions:
- Thread or require the available structured aggregate metadata at the chosen
  boundary.
- Fence legacy rendered-text parsing behind an explicit no-id compatibility
  condition.
- Make stale or missing structured metadata fail closed for metadata-rich
  aggregate inputs.
- Avoid broad rewrites outside the selected route.

Completion check:
- The selected route no longer silently falls back to rendered aggregate type
  text for metadata-rich inputs.
- Legacy no-id direct-LIR compatibility remains explicit and isolated.
- The code builds.

### Step 3: Add Focused AArch64 Direct-LIR Coverage

Goal: prove structured aggregate facts own the selected route.

Concrete actions:
- Add a metadata-rich success case for the selected aggregate route.
- Add stale or missing structured metadata coverage that fails closed instead
  of reparsing rendered type text.
- Keep no-id legacy fixture coverage explicit if the route still supports it.

Completion check:
- Tests fail against the old text-authority behavior and pass with the
  structured-facts route.
- Tests do not rely only on printer text or final assembly spelling.

### Step 4: Validate and Summarize the Boundary

Goal: leave a clear acceptance record for the supervisor and reviewer.

Concrete actions:
- Run the supervisor-delegated proof command for the packet.
- Include targeted AArch64/direct-LIR or backend ABI coverage.
- Summarize retained rendered type text uses as display/output or explicit
  no-id compatibility.

Completion check:
- `todo.md` records build proof, targeted test proof, retained compatibility
  boundaries, and any follow-up idea candidates.
