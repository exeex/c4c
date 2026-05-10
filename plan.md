# HIR Template Binding Domain Key Authority Runbook

Status: Active
Source Idea: ideas/open/161_hir_template_binding_domain_key_authority.md

## Purpose

Move HIR template argument and template binding identity away from rendered
parameter-name maps where owner-aware parameter metadata is available.

## Goal

Use structured HIR template parameter keys for covered type/NTTP binding
creation, lookup, forwarding, pending-template dedup, and specialization
identity while keeping rendered names as display and compatibility outputs.

## Core Rule

Rendered parameter names, rendered specialization names, and mangled names are
outputs or compatibility mirrors. They must not be the semantic authority for
covered HIR template binding identity when owner-aware metadata exists.

## Read First

- `ideas/open/161_hir_template_binding_domain_key_authority.md`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/impl/templates/deduction.cpp`
- `src/frontend/hir/impl/lowerer.hpp`

## Current Targets

- `TypeBindings`, `NttpBindings`, `NttpTextBindings`, and
  `SpecializationArgumentIdentity`
- `TemplateSeedWorkItem`, `TemplateInstance`,
  `SelectedTemplateStructPattern`, `SelectedFunctionTemplatePattern`,
  `PendingTemplateTypeKey`, and `PendingTemplateTypeWorkItem`
- `make_pending_template_type_binding_identities`
- `make_pending_template_nttp_binding_identities`
- `make_pending_template_type_key`
- `mangle_template_name`
- `InstantiationRegistry` seed/instance matching
- Deduction helpers in `src/frontend/hir/impl/templates/deduction.cpp`
- HIR lowerer template binding context fields in
  `src/frontend/hir/impl/lowerer.hpp`

## Non-Goals

- Do not migrate backend `LinkNameId` or final symbol emission tables.
- Do not rewrite HIR compile-time registries that already have adequate
  structured declaration-key mirrors except where binding identity still forces
  string fallback.
- Do not remove rendered mangled names, dump text, diagnostics, or app-facing
  compatibility metadata.
- Do not rework parser template parsing except for narrow metadata handoff
  fixes required by HIR binding keys.
- Do not redesign the consteval interpreter.
- Do not weaken tests, change only display expectations, or add named-case
  shortcuts.

## Working Model

- Parser owns syntax carriers such as template parameter spelling `TextId`,
  owner identity when available, explicit template arguments, and deferred
  syntax payloads.
- Sema owns validated metadata such as parameter kind, owner, index, and
  structured name/key metadata.
- HIR owns late semantic materialization: deduction, explicit/deduced binding
  storage, specialization keys, pending template type keys, seed/instance
  dedup, and final rendered names.
- HIR may retain string maps only as documented compatibility mirrors with
  clear owners, limitations, and removal conditions.

## Execution Rules

- Prefer semantic lowering and owner-aware key propagation over testcase-shaped
  matching.
- Keep each code step behavior-focused and testable.
- Add or update tests that prove same-spelled parameters from different owners
  or nested contexts do not collide.
- Fail closed on complete structured misses instead of falling back to rendered
  string maps for covered metadata.
- Preserve rendered/mangled output behavior unless a semantic correction
  requires a deliberate test update.
- Document retained string fallback paths at the declaration or nearest owning
  helper.
- For code-changing steps, prove with a fresh build or compile check plus the
  narrow HIR/template tests touched by the step.

## Step 1: Inventory Binding Identity Authority

Goal: establish the exact HIR locations where rendered string names still act
as semantic binding authority.

Primary targets:
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/impl/templates/deduction.cpp`
- `src/frontend/hir/impl/lowerer.hpp`

Actions:
- Inspect all uses of `TypeBindings`, `NttpBindings`, `NttpTextBindings`, and
  `SpecializationArgumentIdentity`.
- Classify each use as semantic authority, compatibility bridge,
  display/mangling output, or diagnostic-only.
- Identify call paths that already carry `TextId`, owner declaration identity,
  namespace context, parameter index, or structured declaration metadata.
- Record the first conversion boundary and the first lookup/hash/dedup boundary
  for type bindings, NTTP bindings, pending template types, and specialization
  keys.
- Update `todo.md` with the inventory result and the narrow proof command to
  use for the next implementation packet.

Completion check:
- The next executor can name the first semantic authority site to convert and
  can distinguish it from display/mangling-only sites.

## Step 2: Introduce Structured HIR Template Parameter Keys

Goal: add an owner-aware key type that can represent type and NTTP template
parameters without relying on rendered strings.

Actions:
- Define a HIR template parameter binding key that can carry parameter kind,
  owner declaration identity or owner `TextId`, namespace/domain context,
  parameter index, and parameter spelling `TextId`.
- Add equality, ordering, and hashing support suitable for binding maps and
  specialization identity.
- Add structured type and NTTP binding map aliases beside or underneath the
  existing string-map aliases.
- Document the retained string maps as compatibility mirrors with removal
  conditions.
- Keep incomplete metadata explicit so callers cannot confuse raw `TextId` with
  complete cross-owner identity.

Completion check:
- The project builds, and the key type is available without changing unrelated
  HIR semantics.

## Step 3: Populate Structured Binding Maps

Goal: make HIR deduction and call-binding construction produce structured keys
  when metadata is complete.

Actions:
- Convert deduction helpers such as
  `template_type_param_binding_name_for_deduction`, `bind_deduced_type`,
  `bind_deduced_nttp`, `build_call_bindings`, and
  `build_call_nttp_bindings` to populate structured maps where possible.
- Preserve legacy `TypeBindings` and `NttpBindings` only as compatibility
  mirrors during the transition.
- Update merge/default helpers so structured bindings stay authoritative for
  covered metadata.
- Ensure complete structured misses fail closed instead of silently falling
  through to string maps.

Completion check:
- Narrow HIR/template deduction tests pass, and added tests show same-spelled
  parameters from different owners do not share binding entries.

## Step 4: Use Structured Keys for Pending and Specialization Identity

Goal: make pending-template and specialization dedup prefer owner-aware binding
identity.

Actions:
- Update `SpecializationArgumentIdentity` so `parameter_name` is not the sole
  equality, ordering, or hashing authority when complete metadata is present.
- Update `make_pending_template_type_binding_identities` and
  `make_pending_template_nttp_binding_identities` to emit structured parameter
  identities.
- Update `make_pending_template_type_key`, `PendingTemplateTypeKey`, and related
  work items so structured identities drive covered dedup.
- Keep `mangle_template_name`, `mangled_name`, and rendered specialization names
  as output or compatibility paths, not primary semantic identity.

Completion check:
- Pending template type dedup and compile-time seed/instance matching avoid
  collisions for same-spelled parameters from distinct owners.

## Step 5: Convert Forwarding and Lowerer Lookup Paths

Goal: align HIR lowerer template binding context with structured identity.

Actions:
- Convert lowerer context fields such as `tpl_bindings`, `tpl_bindings_by_text`,
  `nttp_bindings`, and `nttp_bindings_by_text` where they participate in
  semantic lookup or forwarding.
- Keep text-indexed paths only as documented compatibility or incomplete
  metadata bridges.
- Verify type and NTTP forwarding preserve owner-aware keys across nested
  templates and selected template patterns.
- Ensure complete structured lookup misses do not fall back to rendered names.

Completion check:
- Focused tests cover nested type-parameter forwarding and NTTP forwarding
  without rendered-name collisions.

## Step 6: Validate and Tighten Compatibility Boundaries

Goal: prove the migration covers the source idea without overfitting or
  degrading compatibility.

Actions:
- Add focused HIR tests for same-spelled template parameters from different
  owners, nested templates, pending work dedup, specialization dedup, and NTTP
  forwarding.
- Confirm retained string maps are documented with owner, limitation, and
  removal condition.
- Run the narrow HIR/template test subset after each packet and escalate to a
  broader build or CTest checkpoint once multiple identity paths have changed.
- Inspect diffs for expectation weakening, display-only changes claimed as
  progress, and named-case shortcuts.

Completion check:
- Build and relevant tests are green, reviewer reject signals from the source
  idea are not present, and rendered/mangled names remain compatibility outputs
  rather than semantic keys for covered HIR template work.
