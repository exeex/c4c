Status: Active
Source Idea Path: ideas/open/177_template_record_owner_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Bounded Owner Path

# Current Packet

## Just Finished

Step 1 inspection selected the template struct instance record-owner path as the bounded owner path:
`src/frontend/hir/impl/templates/struct_instantiation.cpp::Lowerer::make_template_struct_instance_owner_key`,
called by `register_template_struct_instance_owner` and `instantiate_template_struct_body`.

Concrete files/functions to inspect first in the implementation packet:
`src/frontend/hir/hir_ir.hpp::HirRecordOwnerTemplateIdentity`,
`HirRecordOwnerKeyHash`, `hir_record_owner_key_has_complete_metadata`,
`make_hir_template_record_owner_key`, and
`src/frontend/hir/compile_time_engine.hpp::TemplateStructInstanceKey`.

Exact rendered-string dependency to replace or guard:
`identity.specialization_key = instance_key.spec_key.canonical` currently makes
`SpecializationKey::canonical` part of `HirRecordOwnerKey` equality/hash and
template-owner completeness for instantiated records. The selected packet should
move semantic owner identity to structured `SpecializationKey` owner/argument
facts, keeping the rendered canonical string as display/compatibility payload.

## Suggested Next

Implement the selected Step 2 slice for the template struct instance owner key:
extend `HirRecordOwnerTemplateIdentity` so semantic equality/hash/completeness
can use structured specialization identity copied from
`TemplateStructInstanceKey::spec_key`, then update
`make_template_struct_instance_owner_key` to reject or leave owner-key metadata
incomplete when the selected metadata-rich path lacks structured specialization
facts instead of trusting equal rendered `canonical` text.

Focused first proof command:
`cmake --build build --target cpp_hir_template_parameter_binding_key_test cpp_hir_template_realize_struct_metadata_test frontend_hir_tests && ctest --test-dir build -R '^(cpp_hir_template_parameter_binding_key_structured_metadata|cpp_hir_template_realize_struct_structured_metadata|frontend_hir_tests)$' --output-on-failure`

## Watchouts

Structured facts already available nearby: `TemplateStructInstanceKey` carries
`primary_def` and `spec_key`; `SpecializationKey` already compares/hashes by
`owner` plus ordered `arguments`; `try_make_structured_specialization_key`
builds owner-aware argument identities from complete structured type/NTTP
binding maps; `prepare_template_struct_instantiation` in
`src/frontend/hir/impl/templates/materialization.cpp` already prefers that
structured key before filling `prepared.instance_key`.

Nearby same-feature cases to avoid testcase-shaped fixing:
`tests/frontend/cpp_hir_template_parameter_binding_key_test.cpp` covers complete
and mismatched structured specialization binding maps;
`tests/frontend/cpp_hir_template_canonical_primary_origin_metadata_test.cpp`
covers stale rendered origin rejection;
`tests/frontend/frontend_hir_tests.cpp` has template owner-key static member and
member-owner lookup cases that must keep selecting structured owners over stale
rendered tags.

Display/dump compatibility: do not remove or reparse
`SpecializationKey::canonical`; `src/frontend/hir/impl/inspect/printer.cpp`
prints it for template instantiations/functions, and `HirRecordOwnerKey::debug_label`
currently includes the rendered specialization text. Keep rendered strings on
explicit display/compatibility fields only, not as semantic owner identity.

## Proof

Inspection-only packet. No build or test proof required, and no root-level log
was produced.
