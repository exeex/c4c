Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the next bounded Step 4 packet of `plan.md`: structured binding
identity is now fed into a read-only pending-template key construction
observation at the template-call-result deduction boundary. The existing
legacy `seed_and_resolve_pending_template_type_if_needed` call remains the only
pending-template state mutation, so recorded dedup, lookup authority, reader
authority, specialization materialization, and lowerer forwarding authority are
unchanged.

Concrete changes:
- Added a structured `make_pending_template_type_key` overload that consumes
  `HirTemplateTypeBindings` / `HirTemplateNttpBindings` through the structured
  binding identity helpers.
- Added a read-only pending-template structured identity observation result
  that reports static-context parity, expected legacy-vs-structured key
  difference, structured binding counts, and incomplete structured identities.
- Wired the `ctx-deduction-template-call-result` path to construct legacy and
  structured pending keys side by side and observe them before the existing
  legacy pending-template seed/resolve call.
- Added focused tests proving the structured pending key helper emits complete
  structured identities and remains distinct from the legacy/fallback key while
  preserving the same pending static context.

## Suggested Next

Continue Step 4 by choosing the first actual pending-template dedup switch
point, or add one more observation point if the supervisor wants parity data
from a non-call-result pending-template path before switching authority.

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
  `TypeBindings` / `NttpBindings` key; the structured key is constructed only
  for observation at `ctx-deduction-template-call-result`.
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
