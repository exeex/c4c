Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate Structured Binding Maps

# Current Packet

## Just Finished

Completed the next bounded Step 3 packet of `plan.md`: structured type and NTTP
entries are now dual-written for deduced non-pack bindings in the
explicit/deduced merge paths when complete owner/parameter metadata exists,
while legacy `TypeBindings`, `NttpBindings`, and `NttpTextBindings` remain
lookup, substitution, mangling, and call-lowering authority.

Concrete changes:
- Added owner-aware legacy-name mirror helpers for non-pack type and NTTP
  bindings; they refuse incomplete metadata and leave packs legacy-only.
- Extended `deduce_template_bindings_from_call_args` with optional structured
  type/NTTP outputs populated from deduced legacy maps.
- Wired `try_infer_template_call_result_for_deduction`,
  `merge_explicit_and_deduced_type_bindings`, and
  `merge_explicit_and_ctx_deduced_type_bindings` to add structured entries only
  for deduced names that legacy merge insertion accepts, so explicit/default
  legacy bindings remain authoritative.
- Moved call-result parity observation to the merged explicit+deduced maps and
  added parity observation at the explicit/deduced merge boundaries.
- Extended `cpp_hir_template_parameter_binding_key_structured_metadata` with a
  focused dual-write helper test covering complete metadata, type/NTTP entries,
  pack rejection, and incomplete owner metadata rejection.

## Suggested Next

Implement the next bounded Step 3 packet: decide the remaining structured map
boundary for defaulted/deduced pack bindings before any reader switch. Keep
legacy maps authoritative and avoid pending-template dedup or specialization
identity changes until the structured key can represent every binding domain
the reader would need.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- Template parameter packs remain legacy-only because
  `HirTemplateParameterBindingKey` still does not encode a pack element index.
- Structured maps are now observed after explicit+deduced merge at the covered
  call-result and merge boundaries; no reader depends on them for lookup.
- Defaulted type parameters are covered when they flow through the existing
  build/deduction helpers, but this slice did not switch any default reader or
  add pack-element identity.
- `make_hir_template_parameter_binding_key` rejects incomplete owner/parameter
  metadata, but callers still own deciding whether the source owner is the
  correct semantic authority for a binding domain.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including
`cpp_hir_template_parameter_binding_key_structured_metadata`.
