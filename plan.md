# Parser Intermediate Carrier Boundary Labeling Runbook

Status: Active
Source Idea: ideas/open/129_parser_intermediate_carrier_boundary_labeling.md

## Purpose

Make the parser-side boundary between parse-time-only state and AST handoff
producer state visible in the parser facade and parser helper type definitions.

## Goal

Document parser helper structs and public `Parser` members by role without
changing parser behavior or ownership.

## Core Rule

This is a documentation and classification pass. Do not change member
visibility, parser lookup behavior, AST ownership, or parser semantics.

## Read First

- `ideas/open/129_parser_intermediate_carrier_boundary_labeling.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_types.hpp`
- AST references only as needed to identify fields copied into `Node` or
  `TypeSpec`

## Scope

- Label the public parser facade and parser helper types by execution role.
- Distinguish parser-local state from AST handoff producers/builders.
- Classify parser-side string uses as local-only, generated-name-only, or
  suspicious cross-stage spelling authority.
- Capture suspicious cross-stage string authority as follow-up idea material
  instead of expanding this runbook into implementation cleanup.

## Non-Goals

- Do not move helper structs.
- Do not introduce new ownership or lifetime rules.
- Do not replace parser lookup behavior.
- Do not edit `Node` or `TypeSpec` except for references required by comments.
- Do not convert strings to `TextId` in this pass.

## Working Model

- Public visibility in `Parser` is not automatically a cross-IR contract.
- Parser helper structs may be parse-time carriers, AST projection sources, or
  durable cross-stage payloads.
- Parser-local `std::string` is acceptable when it never crosses the
  AST/Sema/HIR boundary.
- Rendered spelling that crosses the parser boundary is suspicious and should
  be queued for follow-up cleanup, not fixed here.

## Execution Rules

- Keep edits narrow to parser facade/type documentation and any required
  follow-up idea file.
- Prefer concise grouping comments over broad comment churn.
- Preserve behavior; a clean compile is the proof target for code-touching
  documentation changes.
- If a suspicious string authority path is found, record it in a separate
  `ideas/open/` follow-up only after confirming it is distinct from this
  labeling pass.

## Steps

### Step 1: Classify the Parser public surface

Goal: make `Parser` members readable by role from `parser.hpp`.

Primary target: `src/frontend/parser/parser.hpp`

Actions:

- Inspect public parse entry points, state references, lookup/binding helpers,
  AST handoff builders, and diagnostics/debug/testing hooks.
- Add or adjust grouping comments so a reader can tell which members are
  facade entry points, parser-local state references, lookup helpers, AST
  handoff producers/builders, or diagnostics/debug/testing hooks.
- Keep declaration order stable unless a local grouping move is clearly
  required for readability.

Completion check:

- `parser.hpp` distinguishes parser-local state from AST handoff producers
  without requiring implementation-file tracing.

### Step 2: Classify parser helper structs

Goal: document parser helper carrier roles in `parser_types.hpp`.

Primary target: `src/frontend/parser/parser_types.hpp`

Actions:

- Inspect qualified-name refs, template parse results, namespace contexts,
  symbol tables, alias-template metadata, and related helper structs.
- Label each relevant helper as parse-time carrier, AST projection source, or
  cross-stage durable payload.
- Keep labels local and factual; do not redesign the helper types.

Completion check:

- `parser_types.hpp` identifies which helper structs are local-only carriers,
  which feed AST projection, and which carry durable boundary data.

### Step 3: Audit parser-side string authority

Goal: classify parser-side spelling and string usage without changing behavior.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_types.hpp`
- parser implementation references only as needed to classify named fields

Actions:

- Identify parser-local `std::string` use that never crosses the AST/Sema/HIR
  boundary.
- Identify generated-name-only spelling that is not cross-stage semantic
  authority.
- Identify rendered spelling or string-backed fields that appear to cross the
  parser boundary as semantic authority.
- Do not convert or remove fields during this pass.

Completion check:

- Parser-side string use is classified as local-only, generated-name-only, or
  suspicious cross-stage authority.

### Step 4: Queue follow-up for suspicious cross-stage strings

Goal: preserve suspicious findings as durable follow-up work without expanding
this plan into cleanup.

Primary target: `ideas/open/`

Actions:

- If suspicious cross-stage string authority is found, create a narrow
  follow-up idea under `ideas/open/` with the concrete boundary path and cleanup
  intent.
- If no suspicious cross-stage authority is found, record that result in
  `todo.md` proof notes during execution.
- Do not modify unrelated open ideas.

Completion check:

- Every suspicious parser-side cross-stage string authority found by this pass
  is represented by a concrete follow-up idea, or the execution notes state
  that none were found.

### Step 5: Validate behavior preservation

Goal: prove the documentation/classification pass did not change parser
behavior.

Actions:

- Run the supervisor-delegated build or compile proof for the touched parser
  files.
- If comments only were changed, a normal build or compile proof is sufficient.
- Escalate only if execution unexpectedly changes code behavior.

Completion check:

- Fresh proof is captured in `todo.md` by the executor.
- Existing frontend/parser behavior remains unchanged.
