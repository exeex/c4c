Status: Active
Source Idea Path: ideas/open/177_template_record_owner_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Thread Structured Owner Identity

# Current Packet

## Just Finished

Step 2 threaded structured specialization identity into the selected template
struct instance owner-key path. `HirRecordOwnerTemplateIdentity` now carries the
structured `SpecializationKey` for semantic equality/hash/completeness while
keeping the rendered canonical string as display/compatibility data, and
`make_template_struct_instance_owner_key` copies `TemplateStructInstanceKey::spec_key`
instead of treating `spec_key.canonical` as semantic identity. Focused tests now
cover same-rendered/different-structured template record owner keys and
display-only metadata rejection.

## Suggested Next

Continue with the next supervisor-selected packet that consumes the structured
template record owner keys in downstream record/member lookup paths, preserving
rendered tag compatibility only as an explicit fallback.

## Watchouts

Manual test fixtures that create template record owner keys must provide
complete structured specialization metadata. Display-only `specialization_key`
is intentionally incomplete for template-instantiation owner indexing.

## Proof

Ran:
`cmake --build build --target cpp_hir_template_parameter_binding_key_test cpp_hir_template_realize_struct_metadata_test frontend_hir_tests && ctest --test-dir build -R '^(cpp_hir_template_parameter_binding_key_structured_metadata|cpp_hir_template_realize_struct_structured_metadata|frontend_hir_tests)$' --output-on-failure > test_after.log`

Result: passed. Proof log: `test_after.log`.
