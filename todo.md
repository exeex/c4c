Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the next bounded Step 4 packet of `plan.md`: the
`ctx-deduction-arg` and `ctx-deduction-constructed-arg` pending-template
seed/resolve paths now use structured `PendingTemplateTypeKey` identity for
pending dedup/progress state when the enclosing `FunctionCtx` structured
mirrors are complete and count-compatible with the legacy binding maps.
Incomplete structured mirrors keep the existing legacy seed/resolve path.

Concrete changes:
- Added a structured-aware `seed_and_resolve_pending_template_type_if_needed`
  overload that observes legacy-vs-structured pending identity, records with
  the structured key only when `pending_template_structured_identity_can_key_state`
  accepts it, and otherwise delegates to the legacy helper.
- Routed the two deduction argument pending-template contexts through the new
  overload with `ctx->structured_tpl_bindings` and
  `ctx->structured_nttp_bindings`.
- Added focused coverage proving complete `FunctionCtx` mirrors key pending
  state structurally, while incomplete structured mirrors preserve legacy state
  identity.

## Suggested Next

Continue Step 4 by moving to the next pending-template or specialization
identity mutation point that still records dedup/progress state from legacy
rendered binding maps after the deduction argument contexts.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- Existing pending-template state mutation still records the legacy
  `TypeBindings` / `NttpBindings` key outside the
  `ctx-deduction-template-call-result` complete-metadata path.
- The call-result switch intentionally requires structured type/NTTP binding
  counts to match the legacy maps, so enclosing context bindings without
  structured mirrors remain legacy-keyed instead of partially structured.
- Struct-method, non-method function, template-global initializer, and
  template-argument materialization expression `FunctionCtx` creation paths now
  have structured mirror handoff when their owner metadata is complete.
- Structured `SpecializationArgumentIdentity` entries currently keep
  `parameter_name` as display/fallback data only; equality and hashing use the
  structured key when complete metadata is present, and ordering follows the
  same identity rule.
- A structured-vs-legacy key difference is expected while legacy maps remain
  authoritative; the useful signal here is that structured identities are
  constructible, complete, and share the same pending static context.
- `make_hir_template_parameter_binding_key` rejects incomplete owner/parameter
  metadata, but callers still own deciding whether the source owner is the
  correct semantic authority for a binding domain.
- `populate_structured_template_binding_mirrors` intentionally mirrors only
  bindings that the supplied template owner can identify by complete
  owner/parameter metadata; legacy maps remain populated for fallback.
- The new structured-aware seed/resolve overload keeps realization inputs as
  the legacy `TypeBindings` / `NttpBindings`; structured keys affect only
  pending dedup/progress identity when complete and count-compatible.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including the updated
`cpp_hir_template_parameter_binding_key_structured_metadata`.
