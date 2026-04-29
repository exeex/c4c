# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Convert Parser Record Consumers To Typed Identity

## Just Finished

Completed Step 3 `types/base.cpp::lookup_struct_member_typedef_recursive`
conversion for typedef-resolved struct-like owners. The recursive member typedef
lookup now accepts the owner `TypeSpec`, resolves `TypeSpec::record_def` through
`resolve_record_type_spec()` before rendered `struct_tag_def_map` fallback, and
keeps tag-only compatibility fallback in place. The `parse_base_type()` scoped
owner path now keeps record-backed typedef owners on this typed suffix route
instead of reducing them to a rendered qualified-name lookup first.

Added focused parser coverage for `Alias::type` where `Alias` has a real
`record_def` with the target member typedef while `Alias.tag` points at a stale
rendered tag-map record.

## Suggested Next

Next bounded packet: inventory the remaining Step 3 parser record consumers
after the base/declarator/declaration conversions and separate true
TypeSpec-backed candidates from rendered-name compatibility, final-spelling, and
template propagation paths.

Suggested focused proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain. The nested dependent-typename traversal still keeps rendered tag and
qualified field-name fallback for tag-only or untyped records.

Template instantiation paths remain a larger propagation family. This packet
preserved existing rendered lookup behavior there except where a `TypeSpec`
owner was already available to the recursive member typedef helper.

## Proof

Delegated proof completed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Result: build completed; `frontend_parser_tests` and `frontend_hir_tests`
passed.

Proof log: `test_after.log`
