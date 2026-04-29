# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Convert Parser Record Consumers To Typed Identity

## Just Finished

Completed Step 3 nested dependent-typename owner traversal conversion.
`types/declarator.cpp::parse_dependent_typename_specifier` now resolves
`nested_decl->type` through `resolve_record_type_spec()` /
`TypeSpec::record_def` before falling back to rendered `struct_tag_def_map`
lookup. Added focused parser coverage for `typename Root::Nested::type` where
the nested field's real `record_def` has the member typedef and the field's
rendered `type.tag` points at a stale tag-map record.

## Suggested Next

Next bounded packet: convert
`types/base.cpp::lookup_struct_member_typedef_recursive` so typedef-resolved
struct-like owners prefer `TypeSpec::record_def` before rendered tag lookup.
Add focused parser coverage where the resolved typedef's `record_def` carries
the target member typedef while the rendered tag map points at a stale record.

Suggested focused proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain. The nested dependent-typename traversal still keeps rendered tag and
qualified field-name fallback for tag-only or untyped records.

Template instantiation paths remain a larger propagation family and should not
be mixed into the `lookup_struct_member_typedef_recursive` packet.

## Proof

Delegated proof completed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Result: build completed; `frontend_parser_tests` and `frontend_hir_tests`
passed.

Proof log: `test_after.log`
