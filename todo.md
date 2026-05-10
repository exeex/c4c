Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate Structured Binding Maps

# Current Packet

## Just Finished

Completed the next bounded Step 3 packet of `plan.md`: structured type and NTTP
call-binding maps now reach the template-call result inference consumer for
parity observation only, while legacy `TypeBindings`, `NttpBindings`, and
`NttpTextBindings` remain lookup, substitution, mangling, and call-lowering
authority.

Concrete changes:
- Added `HirTemplateCallBindingParity` and
  `observe_hir_template_call_binding_parity` in
  `src/frontend/hir/hir_ir.hpp` to compare covered non-pack legacy call
  bindings against structured owner/index keys without performing lookup
  through the structured maps.
- Wired `try_infer_template_call_result_for_deduction` to pass its
  `build_call_bindings` / `build_call_nttp_bindings` structured outputs into a
  Lowerer parity observer before deduced bindings are merged for result-type
  inference.
- Added Lowerer parity counters for the observed call-result inference boundary;
  they are diagnostics-only state and do not affect behavior.
- Extended `cpp_hir_template_parameter_binding_key_structured_metadata` with a
  focused parity helper test covering matching type/NTTP maps and an NTTP
  mismatch count.

## Suggested Next

Implement the next bounded Step 3 packet: dual-write structured entries for
deduced type/NTTP bindings during the explicit/deduced merge path, still keeping
legacy maps authoritative and avoiding pending-template dedup or specialization
identity changes until structured merge parity is covered.

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
- The structured maps are now observed at the template-call result inference
  boundary only; no reader depends on them for lookup.
- The parity observation currently covers structured entries produced before
  deduced call-argument bindings are merged. Deduced merge dual-write remains
  the next Step 3 gap.
- `make_hir_template_parameter_binding_key` rejects incomplete owner/parameter
  metadata, but callers still own deciding whether the source owner is the
  correct semantic authority for a binding domain.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including
`cpp_hir_template_parameter_binding_key_structured_metadata`.
