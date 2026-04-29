# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Convert Parser Record Consumers To Typed Identity

## Just Finished

Completed Step 3 qualified-member owner packet. Converted the
`parse_dependent_typename_specifier()` nested-owner resolver in
`types/declarator.cpp` so a typedef-derived record `TypeSpec` uses
`resolve_record_type_spec()` / `TypeSpec::record_def` before falling back to
`struct_tag_def_map` by rendered tag.

Focused parser coverage now proves a stale rendered tag-map entry cannot
override a populated `record_def` when resolving `typename Alias::type` through
a record owner alias.

## Suggested Next

Next bounded packet: convert the duplicated incomplete-object checks in
`declarations.cpp` to prefer `TypeSpec::record_def` for direct record
TypeSpecs, but only if a focused test can honestly reach the stale rendered tag
case without adding parser hooks.

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

This packet intentionally changed only the dependent-typename qualified-owner
resolver. Other parser call sites that look up by rendered tag may still be
legitimate compatibility paths, especially where no `TypeSpec` is available.

## Proof

Delegated proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

Proof log: `test_after.log`
