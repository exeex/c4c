# Parser Active-Context Structured Identity Runbook

Status: Active
Source Idea: ideas/open/86_parser_alias_template_structured_identity.md
Activated from: remaining active-context string/TextId mirror cleanup after the exhausted template-struct identity runbook

## Purpose

Continue idea 86 by reducing parser active-context reliance on duplicated
mutable spelling state where a stable `TextId` or structured key already
exists.

## Goal

Make `last_using_alias_name`, `last_resolved_typedef`, and
`current_struct_tag` use structured identity first, with string spellings kept
only as fallback, diagnostics, injected spelling, or compatibility bridge data.

## Core Rule

Do not widen this into a repo-wide string-to-`TextId` migration. Each step must
preserve parser behavior and move one active-context family toward structured
identity as the authoritative route.

## Read First

- `ideas/open/86_parser_alias_template_structured_identity.md`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_support.cpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- `src/frontend/parser/parser_expressions.cpp`

## Current Targets

- `ParserActiveContextState::last_using_alias_name`
- `ParserActiveContextState::last_using_alias_name_text_id`
- `ParserActiveContextState::last_using_alias_key`
- `ParserActiveContextState::last_resolved_typedef`
- `ParserActiveContextState::last_resolved_typedef_text_id`
- `ParserActiveContextState::current_struct_tag`
- `ParserActiveContextState::current_struct_tag_text_id`
- matching clear/set/text accessors in `parser.hpp`
- tentative save/restore and local saved-state paths that currently copy both
  string and `TextId`

## Non-Goals

- Do not reopen the completed lexical-scope or qualified-name lookup work from
  ideas 83 and 84.
- Do not rewrite template-struct primary, specialization, or instantiation
  lookup that the prior runbook already completed.
- Do not delete fallback strings when rollback, diagnostics, emitted spelling,
  or injected-token construction still needs them.
- Do not change parser expectations or downgrade tests as the main proof.
- Do not touch backend, HIR, or repo-wide semantic identity infrastructure.

## Working Model

- `TextId` is the preferred identity for unqualified active-context names.
- `QualifiedNameKey` is the preferred identity when alias registration already
  knows the owner path.
- `std::string` active-context fields are compatibility spellings and should not
  be the primary lookup authority when a valid structured identity is present.
- Accessors should centralize fallback spelling so call sites do not repeatedly
  reimplement string recovery.

## Execution Rules

- Start each code-changing step with a narrow inventory of its callers and
  saved-state paths.
- Prefer accessor and state-shape cleanup over scattered call-site rewrites.
- Keep bridge behavior explicit when a string fallback remains necessary.
- For code-changing steps, run:
  `cmake --build build -j --target c4c_frontend c4cll`
  and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
- Escalate beyond the parser subset only if the step changes shared
  qualified-name infrastructure or parser name-table behavior.

## Steps

### Step 1: Inventory Active-Context Mirror Callers

Goal: classify the remaining string/`TextId` mirror usage before changing state
shape.

Primary targets:

- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser.hpp`
- parser call sites that read or save `last_using_alias_name`,
  `last_resolved_typedef`, or `current_struct_tag`

Actions:

- List direct field readers and writers for the three active-context families.
- Classify each use as structured lookup, fallback spelling, snapshot/rollback,
  diagnostic/rendering, or injected/emitted spelling.
- Identify the first safe code packet for one family without changing unrelated
  alias-template or template-struct behavior.

Completion check:

- `todo.md` records the caller inventory, the classification, and the selected
  first code packet.
- No implementation files are changed in this step.

### Step 2: Tighten `last_resolved_typedef` Around `TextId`

Goal: make tentative typedef state use `last_resolved_typedef_text_id` as the
primary identity and keep spelling fallback localized.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_support.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_declarations.cpp`

Actions:

- Inspect save/restore paths that currently reconstruct both string and
  `TextId` state.
- Route lookups and saved-state comparisons through the valid `TextId` where
  available.
- Keep string spelling only for fallback text recovery or legacy call sites that
  cannot yet accept `TextId`.
- Avoid changing typedef visibility table semantics.

Completion check:

- `last_resolved_typedef` no longer acts as the primary lookup authority where
  `last_resolved_typedef_text_id` is valid.
- Parser build and `frontend_parser_tests` pass.

### Step 3: Tighten `current_struct_tag` Access

Goal: make current-struct active context prefer structured tag identity while
preserving fallback spelling for member registration and injected paths.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_expressions.cpp`

Actions:

- Inspect call sites that test `current_struct_tag.empty()` instead of using
  the accessor or `TextId`.
- Replace primary semantic checks with `current_struct_tag_text_id` or a
  centralized helper where that is behavior-preserving.
- Leave explicit fallback spelling in place for registration, diagnostics, and
  injected/emitted strings.
- Preserve all qualified-owner save/restore behavior.

Completion check:

- Semantic current-struct tests no longer depend primarily on the mutable string
  mirror when a valid `TextId` exists.
- Parser build and `frontend_parser_tests` pass.

### Step 4: Tighten `last_using_alias_name` Around Structured Alias Identity

Goal: make using-alias active context prefer `QualifiedNameKey`/`TextId` and
keep alias-name spelling as a fallback bridge.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_template.cpp`

Actions:

- Inspect alias registration and alias-template metadata handoff sites that
  still consume `last_using_alias_name_text()`.
- Prefer `last_using_alias_key` when populated and
  `last_using_alias_name_text_id` for unqualified alias identity.
- Keep rendered alias spelling only where bridge code still needs a diagnostic
  or compatibility spelling.
- Do not rework alias-template storage beyond this active-context handoff
  boundary.

Completion check:

- Alias registration/follow-through no longer uses rendered alias spelling as
  the first semantic key when structured active-context identity is present.
- Parser build and `frontend_parser_tests` pass.

### Step 5: Final Audit And Broader Parser Proof

Goal: confirm the active-context mirror cleanup is bounded, behavior-preserving,
and ready for lifecycle review.

Actions:

- Re-run the active-context field search from Step 1.
- Confirm remaining string mirrors are documented by use: fallback,
  diagnostics, injected/emitted spelling, or legacy bridge.
- Confirm no completed template-struct identity work was reopened.
- Run the parser build and frontend parser subset.
- Ask the supervisor whether this runbook needs broader validation before close
  or deactivation.

Completion check:

- `todo.md` records the final audit and proof.
- Any remaining string mirror dependency has an explicit reason.
- The active runbook is ready for supervisor lifecycle decision.
