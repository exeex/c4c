# Sema Validate String Authority Audit Runbook

Status: Active
Source Idea: ideas/open/158_sema_validate_string_authority_audit.md

## Purpose

Audit and begin retiring string-keyed semantic authority inside
`src/frontend/sema/validate.cpp` now that parser-side structured carriers are
available.

## Goal

Make covered Sema validate lookup paths prefer structured identity
(`TextId`, `SemaStructuredNameKey`, and domain-specific keys) over rendered
`std::string` maps when complete metadata is present, and document the
remaining compatibility paths.

## Core Rule

Rendered strings may remain as diagnostics, display mirrors, compatibility
mirrors, or no-metadata fallbacks, but complete structured metadata must not be
overridden by rendered-name lookup.

## Read First

- `ideas/open/158_sema_validate_string_authority_audit.md`
- `src/frontend/sema/validate.cpp`
- Existing tests covering structured names, record domains, templates,
  consteval, and local scopes before adding new focused cases.

## Current Scope

- Inventory string-keyed state in `src/frontend/sema/validate.cpp`.
- Convert covered lookup paths to structured precedence.
- Fail closed on complete structured metadata misses for covered paths.
- Keep retained rendered maps explicitly classified with owner, limitation,
  and removal condition.
- Add focused tests proving rendered spelling cannot override structured
  identity in at least one covered path.

## Non-Goals

- Do not rewrite all overload resolution machinery in one packet.
- Do not perform a full `src/frontend/sema/consteval.cpp` domain-table
  conversion.
- Do not migrate HIR `TypeBindings` or `NttpBindings` storage.
- Do not remove diagnostics, source spelling, or local debug names.
- Do not weaken tests or mark supported paths unsupported.

## Working Model

- Parser owns partial syntax carriers: source spelling `TextId`, qualifier and
  owner `TextId` sequences, record/template/value carriers, and display text.
- Sema validate owns eager semantic interpretation for globals, functions,
  enum constants, records, overloads, template parameters, consteval
  visibility, and local scopes.
- HIR may consume validated structured carriers later, but this runbook should
  not expand into HIR storage migration.

## Execution Rules

- Keep changes local to `src/frontend/sema/validate.cpp` and narrowly related
  tests unless a helper declaration is already required by existing structure.
- Prefer semantic lookup generalization over testcase-shaped matching.
- Treat `TextId` as a spelling carrier unless paired with the required
  semantic domain context.
- For each code-changing step, prove at least: build or compile target, then
  the focused test subset touched by the step.
- Escalate to a broader CTest subset before accepting a milestone that changes
  more than one lookup family.
- Update `todo.md` with packet progress and proof after each executor slice.

## Step 1: Inventory Validate String Tables

Goal: classify all string-keyed state in `validate.cpp` before changing
authority.

Primary target:
- `src/frontend/sema/validate.cpp`

Actions:
- Locate the tables named in the source idea, including globals, enum
  constants, functions, overload signatures, complete records, structured
  record tag bridges, template type parameters, local scopes, and consteval
  function maps.
- Add or consolidate an in-file inventory near the owning state or helper
  region that classifies each rendered-name table as semantic authority,
  compatibility mirror, diagnostic/display, local syntax name, or unresolved
  bridge.
- For each retained rendered-name semantic bridge, record its owner,
  limitation, and removal condition.

Completion check:
- A reviewer can identify which string maps are still semantic authority and
  which are retained only as mirrors or fallbacks.

## Step 2: Global, Function, Enum, and Consteval Lookup Precedence

Goal: ensure value/function lookup prefers structured identity when references
carry complete metadata.

Primary targets:
- `globals_`, `structured_globals_`, `structured_global_keys_by_name_`
- `enum_consts_`
- `funcs_`, `structured_funcs_`, `structured_function_names_`
- `consteval_funcs_`, `consteval_funcs_by_text_`, `consteval_funcs_by_key_`

Actions:
- Inspect ordinary global, function, enum constant, and consteval lookup
  helpers and conflict checks.
- Route complete structured references through structured or `TextId` keyed
  maps before rendered-name maps.
- Make covered complete structured misses fail closed instead of reopening
  rendered string fallback.
- Keep no-metadata fallback behavior only where the parser carrier is
  genuinely incomplete, and document that fallback in the inventory.

Completion check:
- Focused tests or existing coverage show stale rendered spelling cannot win
  over structured value/function identity for at least one covered path.

## Step 3: Record Completion And Record Lookup Authority

Goal: make record-domain keys authoritative for covered record completion and
definition lookup.

Primary targets:
- `complete_structs_`, `complete_unions_`
- `complete_structs_by_key_`, `complete_unions_by_key_`
- `struct_defs_by_key_`
- `structured_record_keys_by_tag_`
- `struct_defs_by_unqualified_name_`
- `struct_field_text_ids_by_name_by_key_`

Actions:
- Audit record completion, tag lookup, and field lookup paths.
- Prefer `*_by_key_` and record-domain keyed state when complete record
  metadata is present.
- Keep rendered tag maps only as compatibility mirrors or no-metadata
  fallbacks, with removal conditions captured in the inventory.
- Add or adapt focused tests for same-spelling records across distinct domains
  if existing coverage does not already prove the rule.

Completion check:
- Covered record paths do not let stale rendered tags override structured
  record-domain identity.

## Step 4: Template Parameter And Local Scope Visibility

Goal: make template parameter and local-scope visibility respect structured
metadata before rendered names.

Primary targets:
- `template_type_params_`
- `template_type_param_text_ids_`
- `template_type_param_text_id_by_name_`
- `scopes_`
- `structured_scope_names_`
- `structured_scopes_`

Actions:
- Inspect template type parameter and local binding lookup paths.
- Prefer `TextId` plus domain/owner context where available.
- Keep rendered local names only as syntax/debug names or incomplete-carrier
  fallbacks.
- Add focused tests where same spelling across template or local domains must
  not collapse into one rendered-name authority.

Completion check:
- Covered template or local-scope lookup paths use structured metadata first
  and document any remaining rendered-name bridge.

## Step 5: Consteval Handoff Boundary Audit

Goal: audit validate-to-consteval/HIR binding handoff without expanding into a
full consteval rewrite.

Primary targets:
- validate-side consteval call binding preparation
- string NTTP maps built alongside `TextId` or structured maps

Actions:
- Identify where validate still constructs rendered string maps for consteval
  environments.
- Preserve current behavior where a full consteval migration is out of scope,
  but document whether each rendered map is compatibility, diagnostic, or an
  unresolved bridge.
- If a validate-side structured map already exists, prefer it for covered
  handoff decisions without changing consteval storage contracts.

Completion check:
- The runbook has not grown into `consteval.cpp` conversion, and any retained
  validate-side string handoff has an explicit limitation and removal
  condition.

## Step 6: Milestone Validation And Remaining Bridge Ledger

Goal: prove the accepted slices and leave a clear removal ledger for work not
completed by this runbook.

Actions:
- Run the focused test subsets touched by the implementation steps.
- Run a broader Sema/frontend CTest subset if more than one lookup family was
  changed.
- Confirm no supported-path expectation was weakened or marked unsupported.
- Ensure the inventory still matches the final code.

Completion check:
- `validate.cpp` documents the remaining string-keyed compatibility paths, and
  tests prove at least one stale rendered-name case cannot override structured
  identity.
