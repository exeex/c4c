# HIR Template Binding Domain Key Authority

Status: Closed
Created: 2026-05-10
Closed: 2026-05-11

Parent Ideas:
- `ideas/closed/160_sema_canonical_symbol_template_key_authority.md`
- `ideas/closed/159_sema_consteval_domain_table_authority.md`
- `ideas/closed/157_deferred_syntax_text_payload_contract.md`

## Goal

Move HIR template argument and template binding identity away from rendered
parameter-name maps.

HIR may keep rendered template names, binding names, and mangled names for
diagnostics, debug dumps, metadata compatibility, and final display. But
template instantiation identity, pending template type identity, deduction
results, and NTTP/type binding lookup should use owner-aware parameter keys
when `TextId`, template owner, parameter index, or structured declaration
metadata is available.

## Why This Idea Exists

Ideas 159 and 160 cleaned sema consteval and sema canonical symbol authority.
The next boundary is HIR's compile-time/template engine. HIR already has many
structured mirrors, but the ordinary binding containers still make rendered
parameter names the common path:

- `NttpBindings = unordered_map<string, long long>` in
  `src/frontend/hir/hir_ir.hpp`.
- `TypeBindings = unordered_map<string, TypeSpec>` in
  `src/frontend/hir/hir_ir.hpp`.
- `SpecializationArgumentIdentity::parameter_name` participates in equality,
  ordering, and hashing as a rendered string.
- `TemplateSeedWorkItem`, `TemplateInstance`, `SelectedTemplateStructPattern`,
  and `SelectedFunctionTemplatePattern` carry `TypeBindings` / `NttpBindings`
  as semantic binding state.
- `mangle_template_name` sorts rendered parameter names and builds a rendered
  specialization name from those maps.
- `make_pending_template_type_binding_identities` and
  `make_pending_template_nttp_binding_identities` convert binding maps into
  structured-ish identity records, but the parameter identity inside each
  record is still a string.
- `src/frontend/hir/impl/templates/deduction.cpp` binds deduced type and NTTP
  parameters by `std::string` key, with TextId paths present only as partial
  side channels.

This leaves a gap after parser/sema cleanup: HIR can still accidentally make
same-spelled template parameters collide across different owners, nested
templates, specializations, or forwarded NTTP contexts.

## Responsibility Split

Parser owns syntax carriers:

- template parameter spelling interned as `TextId`
- parameter owner identity where parser can identify it
- explicit template argument syntax and deferred syntax payloads

Sema owns validated binding metadata:

- parameter kind, owner, index, and structured name/key metadata
- consteval and canonical-symbol handoff tables that can distinguish rendered
  display from semantic identity

HIR owns late semantic materialization:

- template call deduction
- explicit/deduced type and NTTP binding storage
- specialization keys and pending template type keys
- compile-time engine seed/instance dedup
- final rendered/mangled names as output, not as semantic authority

## In Scope

- Inventory HIR `TypeBindings`, `NttpBindings`, `NttpTextBindings`, and
  `SpecializationArgumentIdentity` usage and classify each use as semantic
  authority, compatibility bridge, display/mangling output, or diagnostic.
- Introduce an owner-aware HIR template parameter binding key that can carry:
  parameter kind, owner declaration identity or owner `TextId`, namespace
  context, parameter index, and parameter spelling `TextId`.
- Add structured type/NTTP binding maps beside or underneath the legacy
  `unordered_map<string, ...>` aliases, with legacy maps retained only as
  compatibility mirrors while callers are converted.
- Make specialization keys and pending-template-type keys prefer structured
  binding identities over `SpecializationArgumentIdentity::parameter_name`.
- Convert HIR deduction and call-binding construction to populate structured
  binding keys when metadata is complete.
- Make lookup/forwarding of type bindings and NTTP bindings fail closed on a
  complete structured miss instead of falling back to rendered-name maps.
- Keep `mangle_template_name`, `mangled_name`, and debug display strings as
  rendering outputs; they must not be used as seed/instance semantic identity
  when `primary_def + SpecializationKey` or structured binding keys exist.
- Add focused tests where same-spelled template parameters from different
  owners or nested templates do not collide in HIR deduction, pending work, or
  specialization dedup.
- Document retained string maps with owner, limitation, and removal condition.

## Out Of Scope

- Backend `LinkNameId` and final symbol emission table migration.
- Rewriting all HIR compile-time registries that already have structured
  declaration-key mirrors, except where binding identity still forces string
  fallback.
- Removing rendered mangled names, dump text, or app-facing compatibility
  metadata.
- Reworking parser template parsing except for narrow metadata handoff fixes
  required by HIR binding keys.
- Full consteval interpreter redesign; consteval was handled by idea 159.
- Weakening tests, changing only display expectations, or adding named-case
  shortcuts.

## Candidate Anchors

- `TypeBindings`, `NttpBindings`, `NttpTextBindings`,
  `SpecializationArgumentIdentity`, and `HirTemplateInstantiation` in
  `src/frontend/hir/hir_ir.hpp`.
- `TemplateSeedWorkItem`, `TemplateInstance`,
  `SelectedTemplateStructPattern`, `SelectedFunctionTemplatePattern`,
  `PendingTemplateTypeKey`, and `PendingTemplateTypeWorkItem` in
  `src/frontend/hir/compile_time_engine.hpp`.
- `make_pending_template_type_binding_identities`,
  `make_pending_template_nttp_binding_identities`,
  `make_pending_template_type_key`, and `mangle_template_name`.
- `InstantiationRegistry` seed/instance matching where rendered names and
  mangled names remain fallback paths.
- `src/frontend/hir/impl/templates/deduction.cpp` binding helpers:
  `template_type_param_binding_name_for_deduction`, `bind_deduced_type`,
  `bind_deduced_nttp`, `build_call_bindings`,
  `build_call_nttp_bindings`, and merge/default helpers.
- HIR lowerer context fields in `src/frontend/hir/impl/lowerer.hpp`,
  especially `tpl_bindings`, `tpl_bindings_by_text`,
  `nttp_bindings`, and `nttp_bindings_by_text`.

## Acceptance Criteria

- HIR template binding string maps are inventoried and classified.
- A structured HIR template parameter binding key exists and is used by covered
  type/NTTP binding creation, lookup, forwarding, and specialization identity.
- `SpecializationArgumentIdentity` no longer relies only on rendered
  `parameter_name` when complete owner-aware metadata is available.
- Pending template type dedup and compile-time seed/instance identity do not
  collide same-spelled template parameters from different owners.
- Rendered/mangled names remain output and compatibility paths, not primary
  semantic identity for covered HIR template work.
- Retained `TypeBindings` / `NttpBindings` string maps are documented
  compatibility mirrors with removal conditions.
- Focused HIR tests cover nested/same-spelled template parameter and NTTP
  forwarding cases.

## Closure Summary

Closed after Step 6 validation. The active runbook moved covered HIR template
binding identity to owner-aware structured keys for creation, lookup,
forwarding, pending/specialization identity, and compatibility-boundary
documentation. Retained rendered/string maps are compatibility mirrors for
display, incomplete metadata, and transition paths rather than primary
semantic authority for covered HIR template work.

Close proof used the committed Step 6 validation slice
`5ab4eb499 Validate template binding compatibility boundaries`. The available
full-suite `test_after.log` passed 3025/3025 tests with zero failures.

## Reviewer Reject Signals

- A slice claims HIR binding identity is structured while lookup or hashing
  still uses only rendered parameter names for covered metadata.
- `mangled_name` or a rendered specialization string becomes the replacement
  semantic key.
- A raw `TextId` is treated as sufficient cross-owner identity without owner,
  parameter index, or domain context where required.
- Structured misses continue to fall through to string maps when metadata is
  complete.
- Tests only update dump/mangle text and do not prove collision separation.
