Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate Structured Binding Maps

# Current Packet

## Just Finished

Completed the first bounded Step 3 packet of `plan.md`: explicit call-binding
construction now has an additive structured-map dual-write boundary for type and
NTTP parameters without switching any downstream reader away from legacy
`TypeBindings`, `NttpBindings`, or `NttpTextBindings`.

Concrete changes:
- Added `make_hir_template_parameter_binding_key` in
  `src/frontend/hir/hir_ir.hpp` to construct owner-aware keys only when owner
  template `TextId`, parameter index, parameter `TextId`, kind, and qualifier
  metadata are complete.
- Extended `build_call_bindings` and `build_call_nttp_bindings` with optional
  structured binding outputs, preserving existing call compatibility through
  default arguments.
- Dual-wrote non-pack explicit/default type and NTTP bindings into
  `HirTemplateTypeBindings` / `HirTemplateNttpBindings` where complete metadata
  is available. Pack bindings remain legacy-only for now because the current
  structured key has no pack-element index.
- Wired the local explicit call-binding flows in
  `src/frontend/hir/impl/templates/deduction.cpp` to allocate structured maps,
  while leaving lookup, deduction merge, pending work, specialization, consteval,
  and lowerer readers on existing legacy maps.
- Expanded `cpp_hir_template_parameter_binding_key_structured_metadata` to cover
  constructing distinct structured keys from same-spelled template parameters on
  distinct owners and indices, then using those keys in
  `HirTemplateTypeBindings`.

## Suggested Next

Implement the next bounded Step 3 packet: expose or carry the structured maps
far enough to observe parity at a real call-binding consumer, still without
making structured keys lookup authority. Keep the next slice additive and avoid
touching pending-template dedup or specialization equality until parity is
visible.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- This packet intentionally did not dual-write template parameter packs because
  `HirTemplateParameterBindingKey` does not yet encode a pack element index.
- The structured maps created in this packet are additive local state; no reader
  depends on them yet.
- `make_hir_template_parameter_binding_key` rejects incomplete owner/parameter
  metadata, but callers still own deciding whether the source owner is the
  correct semantic authority for a binding domain.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including
`cpp_hir_template_parameter_binding_key_structured_metadata`.
