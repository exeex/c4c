# Sema Canonical Symbol Template Key Authority Runbook

Status: Active
Source Idea: ideas/open/160_sema_canonical_symbol_template_key_authority.md

## Purpose

Make canonical sema symbol identity stop using rendered spelling as ordinary
semantic authority when parser or sema metadata can provide structured keys.

## Goal

Use structured canonical identity for covered symbol, nominal type, and template
parameter paths while retaining strings for diagnostics, debug output, ABI text,
and no-metadata compatibility bridges.

## Core Rule

Do not replace string-first identity with another underspecified key. Any
structured key used for semantic equality, hashing, or substitution must carry
the owner, scope, nominal-domain, or template-parameter context needed for that
domain.

## Read First

- `ideas/open/160_sema_canonical_symbol_template_key_authority.md`
- `src/frontend/sema/canonical_symbol.hpp`
- `src/frontend/sema/canonical_symbol.cpp`
- Nearby parser/sema metadata carriers for `TextId`, `QualifiedNameKey`,
  template owner identity, and nominal record/type identity.

## Current Targets

- `CanonicalScopeSegment`, `CanonicalTemplateParam`, `CanonicalType`,
  `CanonicalSymbol`, and `CanonicalIdentity`
- `canonical_leaf_display_spelling`, `canonicalize_base_type`, and
  `collect_top_level_symbol`
- `substitute_template_args_impl` and `substitute_template_args`
- `types_equal`
- `mangle_type_impl`, `mangle_name`, and `mangle_symbol`
- `CanonicalIdentityHash`, `identity_of`, and `CanonicalSymbolTable` lookup

## Non-Goals

- Do not migrate full HIR `TypeBindings` or `NttpBindings` storage.
- Do not migrate backend `LinkNameId` or final symbol emission tables.
- Do not rewrite the Itanium ABI mangler except to keep rendering separate from
  identity.
- Do not remove diagnostic strings, source spelling strings, or debug
  formatting.
- Do not rework parser name construction outside narrow metadata carriers that
  canonical symbol identity needs.
- Do not weaken tests, downgrade supported paths, or add named-case shortcuts.

## Working Model

- Parser owns syntax carriers: interned spelling, qualifier segments,
  `QualifiedNameKey`, template parameter spelling plus owner identity, and
  source spelling for display.
- Sema owns canonical identity: symbol names carry domain keys beside display
  spelling; nominal type leaves carry nominal identity beside display spelling;
  template substitution binds by parameter identity where metadata is complete.
- HIR and backend may continue consuming rendered strings for emission, ABI
  text, and debug output after sema identity has already been domain-keyed.

## Execution Rules

- Preserve rendered strings as display or compatibility fields unless the
  source idea explicitly allows removal.
- Prefer structured metadata where complete; keep no-metadata fallbacks explicit
  and documented with owner, limitation, and removal condition.
- Add tests that prove same-spelled declarations in different structured
  domains do not collide.
- For code-changing steps, run a fresh build or compile proof before claiming
  the step complete. Use narrow tests first, then escalate when the touched
  surface spans multiple identity paths.
- Treat expectation rewrites, debug-text-only changes, or named-case matching
  as non-progress unless paired with real semantic identity repair.

## Step 1: Inventory Canonical String Authority

Goal: Classify every string field and string lookup in
`canonical_symbol.hpp/.cpp`.

Primary Target: `src/frontend/sema/canonical_symbol.hpp` and
`src/frontend/sema/canonical_symbol.cpp`.

Actions:
- Inspect every `std::string` field, string-keyed map, equality comparison, and
  hash input in the canonical symbol implementation.
- Classify each use as semantic authority, rendering output,
  diagnostic/debug text, ABI output, or compatibility bridge.
- Record retained rendered-string fallbacks in `todo.md`, including owner,
  limitation, and removal condition.

Completion Check:
- The executor can point to the semantic-authority sites that need structured
  keys before implementation begins.
- No implementation behavior is changed by this step except optional comments
  or notes that clarify retained fallbacks.

## Step 2: Add Structured Identity Carriers

Goal: Add domain-aware identity fields where canonical sema currently depends
on rendered spelling.

Primary Target: canonical symbol data structures in
`src/frontend/sema/canonical_symbol.hpp`.

Actions:
- Add or thread structured identity for scope/name segments where parser/sema
  metadata is available, such as `TextId` and `QualifiedNameKey`.
- Add explicit owner-aware template parameter keys for canonical template
  parameters.
- Add nominal identity fields for struct/union/enum/typedef leaves where the
  surrounding sema metadata can supply them.
- Keep display spelling fields available for diagnostics, debug formatting,
  ABI rendering, and no-metadata compatibility.

Completion Check:
- Existing callers compile with the new carriers.
- The new carriers are populated on covered metadata paths and are optional or
  explicitly absent on no-metadata bridges.

## Step 3: Replace Template Substitution Binding Authority

Goal: Stop normal template substitution from binding parameters by raw
`std::string` names when owner-aware metadata is complete.

Primary Target: `substitute_template_args_impl` and
`substitute_template_args`.

Actions:
- Replace the normal string-keyed template argument table with a structured
  template-parameter key.
- Update substitution lookup for template-parameter `CanonicalType` leaves to
  prefer that structured key.
- Keep any string fallback narrow, documented, and limited to incomplete
  metadata paths.

Completion Check:
- Same-spelled template parameters with different owners do not collide in
  substitution.
- No fallback path silently becomes the normal covered-metadata path.

## Step 4: Repair Nominal Type Equality

Goal: Make nominal type equality prefer structured nominal identity before
rendered `user_spelling`.

Primary Target: `types_equal`.

Actions:
- Compare struct/union/enum/typedef leaves by structured nominal identity when
  both sides have it.
- Compare template-parameter leaves by owner-aware template parameter identity
  when both sides have it.
- Retain rendered spelling comparison only as an explicit compatibility bridge
  for missing metadata.

Completion Check:
- Same-spelled nominal types from different namespaces, records, or owners no
  longer compare equal on covered paths.
- Existing no-metadata compatibility behavior remains intentionally scoped.

## Step 5: Repair Canonical Symbol Identity And Hashing

Goal: Make canonical symbol table identity and hashing use structured symbol
identity where available.

Primary Target: `CanonicalIdentity`, `CanonicalIdentityHash`, `identity_of`,
and `CanonicalSymbolTable`.

Actions:
- Extend canonical identity with structured symbol identity beside rendered
  display name.
- Update equality and hashing to prefer structured identity on covered paths.
- Keep rendered name as display and no-metadata compatibility, not the primary
  key when structured identity exists.

Completion Check:
- Same-spelled symbols in distinct structured domains do not collide in table
  lookup or hashing.
- `CanonicalIdentityHash` does not hash only rendered names for covered
  metadata paths.

## Step 6: Keep ABI And Debug Paths Rendering-Only

Goal: Preserve string-producing ABI and debug output without making those
strings semantic lookup authority.

Primary Target: `mangle_type_impl`, `mangle_name`, `mangle_symbol`, and display
helpers.

Actions:
- Feed mangling and display from canonical identity/display fields.
- Confirm mangled ABI text is not reused as a semantic key.
- Keep compatibility formatting stable unless semantic repair requires a
  deliberate rendered-name change.

Completion Check:
- ABI/debug paths still produce strings.
- No mangled or debug string becomes the replacement identity key.

## Step 7: Add Focused Identity-Separation Tests

Goal: Prove stale or same-spelled rendered names cannot override structured
canonical identity.

Primary Target: nearest existing frontend/sema tests for canonical symbol,
nominal type equality, and template substitution behavior.

Actions:
- Add tests for same-spelled names in different namespaces, records, or
  template owners.
- Add tests for template substitution where owner-aware parameter identity must
  distinguish same-spelled parameters.
- Avoid expectation-only changes unless they correspond to repaired semantic
  behavior.

Completion Check:
- Narrow tests fail before the relevant semantic repair or would have failed
  under the old string-keyed path.
- Tests pass with the structured identity implementation.

## Step 8: Validation Checkpoint

Goal: Prove the completed route did not regress nearby canonical sema behavior.

Actions:
- Run the delegated narrow proof for the touched tests.
- Run a fresh build or compile proof.
- Escalate to broader `ctest --test-dir build -j --output-on-failure` or the
  supervisor-selected regression guard when multiple identity paths have been
  touched.

Completion Check:
- Proof logs show the selected build and test commands passed.
- Remaining rendered-string fallbacks are listed as explicit compatibility
  bridges with removal conditions.
