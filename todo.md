Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate Structured Binding Maps

# Current Packet

## Just Finished

Completed the remaining Step 3 pack-boundary packet of `plan.md`: structured
type and NTTP binding keys can now represent pack elements separately from the
ordinary parameter key, and explicit type/NTTP pack call bindings dual-write
structured entries when complete owner/parameter metadata exists. Legacy
`TypeBindings`, `NttpBindings`, and `NttpTextBindings` remain lookup,
substitution, mangling, and call-lowering authority.

Concrete changes:
- Added `pack_element_index` to `HirTemplateParameterBindingKey`, with `-1`
  preserving non-pack identity and non-negative values representing pack
  elements.
- Included the pack element index in structured key equality, ordering, and
  hashing.
- Extended owner-aware legacy-name mirror helpers to parse `Name#N` pack
  binding names and populate structured type/NTTP pack entries only when owner
  metadata is complete.
- Wired `build_call_bindings` and `build_call_nttp_bindings` pack branches to
  dual-write structured explicit pack entries while leaving legacy maps as the
  authoritative result.
- Extended `cpp_hir_template_parameter_binding_key_structured_metadata` to cover
  distinct pack elements, non-pack-vs-pack identity, type pack dual-write, NTTP
  pack dual-write, and invalid unsuffixed pack names.

## Suggested Next

Step 3 now has structured dual-write coverage for explicit, defaulted
non-pack, deduced non-pack, and pack-element binding names that flow through the
owner-aware legacy mirror helpers. Suggested next packet: start Step 4 by
choosing one read-only parity observation point for structured-vs-legacy
binding lookup, without switching lookup authority or touching specialization
dedup.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- Structured maps are now observed after explicit+deduced merge at the covered
  call-result and merge boundaries; no reader depends on them for lookup.
- Pack bindings have structured identity only when the legacy binding name uses
  the existing `Name#N` suffix form; unsuffixed pack parameter names still refuse
  structured dual-write.
- `make_hir_template_parameter_binding_key` rejects incomplete owner/parameter
  metadata, but callers still own deciding whether the source owner is the
  correct semantic authority for a binding domain.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including
`cpp_hir_template_parameter_binding_key_structured_metadata`.
