# Plan: Phase F3 Prepared Module Top-Level Printer Candidate

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Turn one remaining bounded candidate from idea 260 into an executable packet
without claiming broad `PreparedBirModule` field retirement.

## Goal

Implement and prove the `module` top-level printer candidate only: prepared
printer BIR body emission may use complete BIR module text only when section
order, BIR text, and blank-line behavior remain byte-stable for empty,
function-only, global/string-constant, phase, and note-header cases.

## Core Rule

Do not delete, privatize, wrap, or broadly retire any `PreparedBirModule`
field. This plan is one printer-reader candidate, not a structural exit for
the aggregate.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- Prepared printer implementation and declarations.
- Existing prepared-printer tests covering empty modules, functions, globals,
  string constants, phases, and note headers.
- Current BIR module printer or text-rendering helpers that could supply a
  complete module body without changing prepared-printer section policy.

## Current Scope

- Selected candidate: `module` top-level printer packet.
- Primary authority to introduce: complete BIR module text may be used only
  under an explicit agreement boundary that preserves prepared-printer layout.
- Retained compatibility surface: prepared-printer section order, blank lines,
  headers, notes, and public prepared aggregate observability remain
  authoritative.

## Non-Goals

- Do not implement another candidate from idea 260 in this runbook.
- Do not change route-debug strings, target output, helper/oracle status names,
  or backend lowering behavior to claim progress.
- Do not move target policy, backend formatting policy, or traversal/layout
  decisions unrelated to top-level prepared-printer BIR body emission into BIR.
- Do not weaken unsupported behavior, diagnostics, or prepared-printer
  byte-stability contracts.
- Do not claim broad structural exit, aggregate retirement, privatization, or
  wrapper progress.

## Working Model

The prepared printer remains the owner of top-level prepared output shape. The
BIR module may supply text for the selected body emission only when the module
text is complete and its integration is byte-stable against the current
prepared-printer surface. Any missing, partial, reordered, or layout-sensitive
row must keep the existing prepared-printer behavior.

## Execution Rules

- Keep edits local to the selected prepared-printer reader path and focused
  tests.
- Add or reuse a helper only when it makes the complete-module-text agreement
  boundary explicit.
- Prove nearby same-feature layout rows, not only one happy path.
- Preserve byte-stable prepared-printer output for empty, function-only,
  global/string-constant, phase, and note-header cases.
- Treat expectation rewrites, helper renames, classification-only movement, or
  broad printer rewrites as non-progress.

## Steps

### Step 1: Inventory Top-Level Printer Contract

Goal: identify the exact current prepared-printer behavior and consumers for
the selected candidate.

Actions:

- Inspect prepared-printer definitions, declarations, direct callers, and
  tests that own top-level BIR body emission.
- Record the existing section order, BIR body text, blank-line, empty-module,
  function-only, global/string-constant, phase, and note-header behavior in
  `todo.md`.
- Identify the narrow test bucket that already exercises prepared-printer
  layout and the nearest fixture surface for byte-stable proof rows.

Completion check:

- `todo.md` names the selected printer row, current behavior to preserve,
  candidate implementation files, and a narrow proof command for the next
  packet.

### Step 2: Design the Complete-Module-Text Boundary

Goal: define when complete BIR module text can be used without changing the
prepared-printer contract.

Actions:

- Choose whether the boundary belongs in an existing prepared-printer helper, a
  small local adapter, or an existing BIR printer/text helper.
- Specify the accepted rows: complete module text, stable section order, stable
  BIR body text, and stable surrounding blank-line/header behavior.
- Specify rejected or compatibility rows: empty or partial module structure,
  missing functions, global/string-constant ordering drift, phase/note header
  drift, and any case where using BIR text would alter existing prepared output.
- Record the design in `todo.md`; do not edit the source idea unless the
  selected candidate itself is ambiguous.

Completion check:

- `todo.md` contains an implementation packet with explicit owned files,
  retained compatibility behavior, and focused byte-stability proof
  requirements.

### Step 3: Implement Narrow Printer Bridge

Goal: allow the selected prepared-printer path to use complete BIR module text
only under the Step 2 agreement boundary.

Actions:

- Implement the narrow agreement helper or adapter from Step 2.
- Wire it only into the selected top-level prepared-printer BIR body emission
  path.
- Keep prepared-printer section policy, blank-line policy, public aggregate
  compatibility, and existing fallback behavior intact.
- Avoid touching unrelated `module`, `names`, `control_flow`, or
  `store_source_publications` candidates.

Completion check:

- Build proof passes.
- Focused prepared-printer tests pass.
- The diff does not contain output baseline rewrites, unsupported downgrades,
  or unrelated candidate movement.

### Step 4: Add Byte-Stable Proof Rows

Goal: prove the selected candidate preserves prepared-printer output across
nearby layout-sensitive cases.

Actions:

- Add or preserve positive rows showing complete BIR module text integration
  remains byte-stable.
- Add rows for empty modules, function-only modules, global/string-constant
  modules, phase output, note headers, and surrounding blank-line behavior as
  applicable to the helper under test.
- Preserve route-debug, target-output, and unrelated helper expectations unless
  the selected printer path already owns that surface and the change is
  explicitly proved.

Completion check:

- Focused proof covers positive and layout-sensitive byte-stability rows.
- `todo.md` records any unsupported fixture surfaces that should remain out of
  scope rather than being forced into this runbook.

### Step 5: Broader Validation and Closure Readiness

Goal: decide whether the selected candidate is complete and ready for
plan-owner closure review.

Actions:

- Run the focused proof again after any final cleanup.
- Run a broader backend/prepared subset if the printer helper is shared by more
  than one diagnostic or backend surface.
- Record proof commands, pass/fail result, and residual out-of-scope rows in
  `todo.md`.
- If the selected candidate is complete under idea 260 acceptance criteria,
  request plan-owner closure review; if more candidates remain, close or
  replace this runbook rather than expanding it in place.

Completion check:

- `todo.md` shows the selected candidate complete or blocked with exact
  evidence.
- No other idea 260 candidate has been absorbed into this active plan.
