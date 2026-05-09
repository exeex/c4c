# Deferred Syntax Text Payload Contract

Status: Active
Source Idea: ideas/open/157_deferred_syntax_text_payload_contract.md

## Purpose

Define and enforce the boundary where parser string payloads are legitimate
deferred syntax text, diagnostics, debug/display spelling, or literal payloads,
not semantic identity keys.

Goal: keep source syntax text available where the language or delayed
evaluation requires it, while requiring structured binding, owner,
qualified-name, record, or template-domain carriers before semantic lookup.

## Core Rule

Deferred text may remain as syntax payload, diagnostics, debug text, final
display spelling, or explicit compatibility mirror. It must not become the
canonical key for template arguments, NTTP defaults, qualified names, record
owners, or type identity when structured metadata exists.

## Read First

- `ideas/open/157_deferred_syntax_text_payload_contract.md`
- Recent structured identity work:
  - `ideas/closed/149_template_instantiation_structured_argument_key.md`
  - `ideas/closed/150_nttp_type_binding_domain_key_contract.md`
- Parser syntax payload and template argument capture surfaces:
  - `src/frontend/parser/ast.hpp`
  - `src/frontend/parser/parser_types.hpp`
  - `src/frontend/parser/impl/core.cpp`
  - `src/frontend/parser/impl/declarations.cpp`
  - `src/frontend/parser/impl/types/base.cpp`
  - `src/frontend/parser/impl/types/declarator.cpp`
- Sema/HIR interpretation surfaces that consume deferred payloads:
  - `src/frontend/sema/consteval.cpp`
  - `src/frontend/sema/type_utils.cpp`
  - `src/backend/hir/`

## Current Targets

- `capture_template_arg_expr_text` and related template argument debug refs.
- Deferred NTTP/default expression text and token replay helpers.
- Reparse-token helpers and syntax text carried into HIR.
- Qualified-name, owner, record, template-argument, and type lookup paths that
  can still consume payload strings as keys.
- Tests proving stale debug or syntax text cannot override structured metadata.

## Non-Goals

- Do not remove deferred expression tokens or source syntax payloads.
- Do not remove diagnostics, debug strings, literal bytes, or final output
  spelling.
- Do not require all dependent expression evaluation to happen in parser or
  Sema.
- Do not replace source literal payloads with `TextId` when the payload is not
  a name.
- Do not reopen LinkNameId or LIR final-symbol spelling work.
- Do not downgrade supported tests or weaken contracts to claim progress.

## Working Model

Parser owns syntax payload preservation:

- token streams for deferred expressions
- source spelling for diagnostics and debug output
- literal text and bytes where exact payload matters
- display mirrors beside structured carriers

Sema owns semantic interpretation:

- type and value identity
- template parameter binding domains
- overload and qualified-name resolution
- non-dependent constant evaluation where enough metadata exists

HIR may evaluate deferred payloads later, but it must receive the structured
binding, owner, qualified-name, or domain carrier needed to interpret the text
safely.

## Execution Rules

- Classify each touched string as syntax payload, diagnostic/debug text,
  display/final spelling, compatibility mirror, or semantic key.
- Rename or comment ambiguous APIs so payload status is visible at the call
  site.
- Prefer structured carriers such as `TextId`, template parameter binding
  metadata, owner keys, `QualifiedNameKey`, record metadata, and domain keys
  before using text.
- If text remains reachable from semantic code, require the structured carrier
  first or document the path as a bounded compatibility bridge.
- Do not add named-fixture shortcuts or rendered-spelling matches to pass a
  testcase.
- For code-changing steps, run `cmake --build --preset default` plus a focused
  CTest subset covering the touched parser/frontend/HIR tests. Escalate to
  broader validation before closure.
- Keep source-idea edits out of routine execution unless durable source intent
  changes.

## Steps

### Step 1: Inventory Deferred Text Payload And Key Paths

Goal: locate and classify the string payloads named by the source idea.

Primary targets:

- `capture_template_arg_expr_text`
- template argument debug refs and display mirrors
- deferred NTTP/default expression text carriers
- reparse-token helpers
- HIR consumers of deferred expression text
- qualified-name, owner, record, template-argument, and type lookup paths that
  still accept text

Actions:

- Inspect the primary targets and their direct callers.
- Classify each string path as syntax payload, diagnostic/debug text,
  display/final spelling, compatibility mirror, or semantic key.
- Identify any semantic lookup path where text can be used without a
  structured carrier.
- Record the first repair packet, retained legitimate payloads, and focused
  proof command in `todo.md`.

Completion check:

- `todo.md` names the owned first repair path, known legitimate payload
  surfaces, suspicious semantic-key paths, and initial validation subset
  without changing implementation files.

### Step 2: Make Payload API Boundaries Explicit

Goal: make ambiguous text carriers read as payloads or display mirrors rather
than identity keys.

Actions:

- Rename local helpers or fields only when the new name clarifies payload
  semantics without broad churn.
- Add comments near retained deferred syntax text explaining owner, lifetime,
  and interpretation requirements.
- Keep literal bytes, diagnostics, debug strings, and final spelling intact.
- Avoid changing semantic behavior unless needed to prevent text from acting
  as authority.

Completion check:

- Touched APIs make payload status explicit, and call sites no longer imply
  that syntax text is a canonical key.

### Step 3: Require Structured Carriers Before Semantic Lookup

Goal: prevent deferred or debug text from overriding structured identity.

Actions:

- Inspect semantic consumers reached from Step 1.
- Route lookup through structured template binding, owner,
  `QualifiedNameKey`, record, type, or domain metadata before consulting text.
- Demote text-only routes to compatibility paths where no structured carrier
  exists.
- Ensure structured metadata misses are not followed by a stale text fallback
  that selects a different semantic entity.

Completion check:

- Covered lookup paths require structured carriers before payload text, and
  stale text cannot win after a structured miss.

### Step 4: Add Stale-Payload Regression Coverage

Goal: prove semantic authority is separated from syntax/debug/display text.

Actions:

- Add focused tests where stale or same-spelling payload text is present.
- Cover at least one template, owner, qualified-name, record, or type path
  touched by the repair.
- Keep tests capability-oriented; do not assert only formatting stability.
- Do not weaken existing supported-path contracts.

Completion check:

- Focused tests fail on text-authority behavior and pass only when structured
  metadata determines the result.

### Step 5: Audit Remaining Text Payloads And Compatibility Bridges

Goal: leave a clear contract for every remaining text payload in the touched
surfaces.

Actions:

- Re-scan Step 1 targets after code packets.
- Document retained payloads as syntax, diagnostic/debug, display/final
  spelling, literal payload, or compatibility mirror.
- Remove or demote any fallback that can still act as semantic authority.
- Record removal conditions for retained compatibility mirrors in `todo.md`.

Completion check:

- Remaining touched text paths have clear ownership and cannot serve as
  canonical semantic keys when structured metadata exists.

### Step 6: Validation And Closure Readiness

Goal: prove the route repaired the payload/identity boundary without
testcase-overfit.

Actions:

- Run `cmake --build --preset default`.
- Run focused CTest coverage selected by the supervisor for the touched
  parser/frontend/HIR tests.
- Escalate to broader validation if the route changes shared parser, Sema, or
  HIR interpretation behavior.
- Confirm no expectation downgrade, unsupported conversion, or named-case
  shortcut is part of the final route.
- Summarize retained text payload categories and compatibility bridges in
  `todo.md`.

Completion check:

- Build and selected tests pass, retained text payloads are justified, and the
  supervisor has enough proof to decide whether the source idea is complete.
