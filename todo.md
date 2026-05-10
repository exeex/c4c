Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the next bounded Step 4 packet of `plan.md`: the
`ctx-deduction-template-call-result` pending-template mutation point now records
dedup/progress state with the structured `PendingTemplateTypeKey` when
structured binding identities are complete and static-compatible. Incomplete or
non-parity structured metadata keeps the existing legacy
`seed_and_resolve_pending_template_type_if_needed` path.

Concrete changes:
- Added a compile-time-state recorder that preserves legacy work-item maps and
  display keys while accepting an explicit pending-template identity key for
  dedup/resolved-progress authority.
- Added a reusable completeness gate for structured pending-template identity
  observations.
- Switched the deduction template-call-result boundary to use the structured
  key only when structured metadata is complete and covers the same legacy type
  and NTTP binding counts; otherwise it falls back to the legacy path.
- Added focused tests proving complete structured metadata becomes the state
  key, legacy display output remains present, resolved-progress checks use the
  structured key, and incomplete structured metadata is rejected for state-key
  authority.

## Suggested Next

Continue Step 4 by selecting the next pending-template or specialization
identity mutation point that already has complete structured binding metadata,
or expand structured metadata propagation for call-result cases that still fall
back because enclosing context bindings lack structured mirrors.

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

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including
`cpp_hir_template_parameter_binding_key_structured_metadata`.
