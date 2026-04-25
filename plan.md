# LIR Type Text Authority Demotion Runbook

Status: Active
Source Idea: ideas/open/114_lir_type_text_authority_demotion.md

## Purpose

Demote selected LIR rendered type text from semantic authority to printer-only,
bridge-required, or proof-only status without changing emitted LLVM/backend
output.

## Goal

Make structured type refs or `StructNameId` primary for the cleaned LIR paths,
while preserving legacy text wherever emission or compatibility still requires
it.

## Core Rule

Only demote surfaces that are already classified as `safe-to-demote` or
`legacy-proof-only`; do not turn a blocked surface into a narrow testcase fix.

## Read First

- `ideas/open/114_lir_type_text_authority_demotion.md`
- Existing LIR type-ref, verifier, HIR-to-LIR, and backend code around the
  candidate surfaces named below
- Existing tests covering globals, externs, signatures, calls, and struct type
  lowering

## Current Scope

Candidate surfaces from the source idea:

- `LirGlobal::llvm_type`
- `LirExternDecl::return_type_str`
- selected `LirFunction::signature_text` authority
- selected call return/argument formatted type text
- selected raw `LirTypeRef` text-only comparisons
- verifier checks that can become structured-first with text fallback

## Non-Goals

- Do not remove all LIR type strings.
- Do not change printer-only text needed for final LLVM/backend emission.
- Do not demote fields classified as `backend-blocked`,
  `layout-authority-blocked`, or `type-ref-authority-blocked`.
- Do not migrate or clean `src/backend/mir/` legacy type users as part of this
  route.
- Do not downgrade expectations or mark supported-path tests unsupported.

## Working Model

- Legacy rendered type text may remain as emitted text, debug text, bridge text,
  or compatibility proof.
- Semantic lookup should prefer structured identity where the structured fact
  already exists and has parity.
- Text fallback is acceptable for incomplete coverage, but the fallback should
  be visible in code and proof notes.
- Exact emitted output is part of the contract.

## Execution Rules

- Start every code-changing step with a narrow compile/build proof target chosen
  by the supervisor.
- Keep changes small enough to prove one authority demotion family at a time.
- When a path still needs rendered text for emission, preserve the field and
  rename/reframe only if that improves authority clarity without churn.
- Record remaining blocked text-authority surfaces in `todo.md`; promote them
  to a new idea only if they become a distinct initiative.
- If MIR legacy code blocks compilation, exclude the relevant MIR `.cpp` files
  from the current compile target rather than changing MIR semantics.

## Steps

### Step 1: Establish Type-Text Authority Inventory

Goal: identify which candidate surfaces are currently semantic authority and
which are printer-only or proof-only.

Primary targets:

- LIR data structures that store rendered type text
- verifier code comparing rendered type text
- HIR-to-LIR lowering paths that populate LIR type text
- backend emission paths that consume LIR type text

Actions:

- Inspect each candidate field and classify its current consumers as semantic,
  emission-only, bridge-required, proof-only, or blocked.
- Prefer structured parsers, type-ref APIs, and existing helper functions over
  ad hoc string matching when classifying consumers.
- Identify the smallest first demotion family with structured identity already
  available.
- Update `todo.md` with the selected first packet and any blocked surfaces.

Completion check:

- `todo.md` names the first demotion family, its proof command, and the
  candidate surfaces intentionally left untouched.

### Step 2: Demote One Safe Structured-First Consumer Family

Goal: convert one selected safe consumer family from rendered text authority to
structured identity authority.

Primary targets:

- The first family selected in Step 1, such as verifier checks, call type checks,
  or a global/extern/signature consumer with structured parity.

Actions:

- Change semantic comparison or lookup to use `LirTypeRef`, `StructNameId`, or
  existing structured identity first.
- Keep legacy rendered text as printer-only, bridge-required, or diagnostic
  fallback where needed.
- Preserve exact emitted LLVM/backend text.
- Add or update focused tests only when they prove structured authority rather
  than weakening expectations.

Completion check:

- The selected family no longer treats rendered type text as primary semantic
  authority.
- Narrow compile/build proof and targeted tests pass.

### Step 3: Repeat Demotion for Adjacent Safe Families

Goal: extend the same structured-first rule to adjacent safe candidate surfaces
without broad rewrites.

Primary targets:

- Additional candidate surfaces classified in Step 1 as `safe-to-demote` or
  `legacy-proof-only`.

Actions:

- Apply the Step 2 pattern one family at a time.
- Stop at the first surface that needs new structured coverage outside this
  source idea.
- Keep fallback behavior explicit for unconverted or incomplete paths.

Completion check:

- Each converted family has focused proof.
- Remaining blocked surfaces are recorded in `todo.md` with the blocking reason.

### Step 4: Verify Emission Parity and Authority Boundaries

Goal: prove the demotion preserved output while reducing text authority.

Primary targets:

- Focused LIR/HIR-to-LIR/backend tests touched by the converted families
- Broader validation chosen by the supervisor for the demotion blast radius

Actions:

- Run the supervisor-selected focused proof command.
- Run a broader validation checkpoint once multiple families have changed or a
  shared verifier/backend path has changed.
- Inspect diffs for expectation downgrades or testcase-shaped shortcuts.
- Update `todo.md` with final proof, leftovers, and any follow-up idea needed
  for still-blocked authority.

Completion check:

- Focused and broader validation pass without expectation downgrades.
- Converted surfaces use structured identity first.
- Legacy text remains only as printer-only, bridge-required, fallback, or
  proof-only for the converted scope.
