Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Use Structured Keys for Pending and Specialization Identity

# Current Packet

## Just Finished

Completed the first bounded Step 4 packet of `plan.md`: pending/specialization
argument identity can now carry owner-aware structured template parameter
identity while legacy rendered parameter names remain the fallback display and
compatibility path. No reader authority, lookup authority, specialization dedup
call site, materialization path, or lowerer path was switched.

Concrete changes:
- Added optional `HirTemplateParameterBindingKey` metadata to
  `SpecializationArgumentIdentity`.
- Updated argument identity equality, ordering, and hashing so complete
  structured keys distinguish same-spelled parameters from different owners,
  different parameter TextIds, and different pack element indices.
- Fixed the structured identity consistency rule so complete structured
  identities with the same `parameter_key` ignore differing display names for
  equality, ordering, and hashing.
- Kept fallback identities compatible: when structured metadata is absent or
  incomplete, argument identity continues to use `parameter_name`.
- Added structured overloads for
  `make_pending_template_type_binding_identities` and
  `make_pending_template_nttp_binding_identities` that consume
  `HirTemplateTypeBindings` / `HirTemplateNttpBindings` and emit structured
  `SpecializationArgumentIdentity` entries.
- Extended `cpp_hir_template_parameter_binding_key_structured_metadata` with
  focused tests for structured argument identity, hash/equality behavior,
  legacy fallback behavior, and pending binding identity helper overloads.

## Suggested Next

Continue Step 4 with a narrow wiring packet that feeds structured binding
identity into one covered pending-template key construction path for parity or
observation only. Keep legacy-map call sites authoritative until the supervisor
chooses the specific dedup switch point.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- Existing `make_pending_template_type_key` call sites still pass legacy
  `TypeBindings` / `NttpBindings`; this packet deliberately added structured
  helper overloads without switching those call sites.
- Structured `SpecializationArgumentIdentity` entries currently keep
  `parameter_name` as display/fallback data only; equality and hashing use the
  structured key when complete metadata is present, and ordering follows the
  same identity rule.
- `make_hir_template_parameter_binding_key` rejects incomplete owner/parameter
  metadata, but callers still own deciding whether the source owner is the
  correct semantic authority for a binding domain.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including
`cpp_hir_template_parameter_binding_key_structured_metadata`.
