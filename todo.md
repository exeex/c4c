Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Structured HIR Template Parameter Keys

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: added the structured HIR template parameter
binding key vocabulary without rewiring lookup, deduction, pending work, or
specialization behavior.

Concrete changes:
- Added `HirTemplateParameterBindingKind` and
  `HirTemplateParameterBindingKey` in `src/frontend/hir/hir_ir.hpp`.
- The key carries parameter kind, owner namespace context, owner global and
  qualifier metadata, owner template `TextId`, parameter index, and parameter
  spelling `TextId`.
- Added equality, inequality, ordering, hashing, and
  `hir_template_parameter_binding_key_has_complete_metadata`.
- Added `HirTemplateTypeBindings` and `HirTemplateNttpBindings` aliases keyed
  by the structured HIR key.
- Documented `TypeBindings`, `NttpBindings`, and `NttpTextBindings` as legacy
  compatibility mirrors. `NttpTextBindings` remains explicitly non-owner-aware.
- Added `cpp_hir_template_parameter_binding_key_structured_metadata`, a focused
  unit test proving same-spelled parameters remain distinct by owner/index,
  raw TextId-only keys are incomplete, invalid qualifier metadata is rejected,
  ordering keeps distinct keys separate, and hash lookup works through
  `HirTemplateNttpBindings`.
- Added the standard `internal;hir;cpp` CTest labels to the new structured
  binding key test so it matches neighboring HIR metadata tests.
- No behavior was rewired; existing string/TextId binding maps remain the maps
  consumed by current HIR call binding, deduction, specialization, consteval,
  and lowerer paths.

## Suggested Next

Implement the first bounded packet for Step 3: populate structured binding maps
inside call-binding construction and deduction without switching downstream
lookup authority yet. Start in `src/frontend/hir/impl/templates/deduction.cpp`
and the matching declarations in `src/frontend/hir/impl/lowerer.hpp`; dual-write
structured `HirTemplateTypeBindings` / `HirTemplateNttpBindings` when owner,
kind, index, and parameter `TextId` metadata are complete, while preserving
legacy `TypeBindings`, `NttpBindings`, and `NttpTextBindings` mirrors.

## Watchouts

- Do not treat rendered names, mangled names, or raw `TextId` alone as complete
  semantic identity when owner-aware metadata is available.
- Do not weaken tests or convert capability work into expectation-only changes.
- Keep routine implementation progress in this file rather than rewriting the
  source idea.
- `mangle_template_name`, `format_pending_template_type_key_for_display`,
  `encode_pending_type_ref`, and `SpecializationKey::canonical` are
  display/compatibility outputs; do not make them replacement semantic keys.
- Step 3 should treat the new aliases as additive dual-write state first; do
  not switch lookup, pending-template dedup, specialization equality, or lowerer
  context authority until parity is visible and covered by tests.
- `hir_template_parameter_binding_key_has_complete_metadata` only checks the
  HIR key fields that exist now. Callers still need to decide whether their
  source metadata is authoritative enough to create the key.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_|cpp_positive_sema_.*template|cpp_positive_sema_.*qualified_.*template)'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The selected subset
ran 322 tests with 100% passing after the build, including the new
`cpp_hir_template_parameter_binding_key_structured_metadata` test with
`internal;hir;cpp` labels.
