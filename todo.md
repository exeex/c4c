Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Record Definition Lookup Structured-Primary

# Current Packet

## Just Finished

Step 2 made the first record-definition lookup consumers structured-primary.
`resolves_to_record_ctor_type()` now asks `resolve_record_type_spec()` before
falling back to rendered `defined_struct_tags`/`struct_tag_def_map` membership,
and the fallback is documented as compatibility for tag-only paths.
`lookup_struct_member_typedef_recursive_for_type()` no longer rejects tagless
`TypeSpec` owners when `record_def` is present, preserves rendered map lookup as
an explicit fallback after structured identity fails, and recurses through base
records that carry direct `record_def` data without requiring rendered tags.

Nearby parser tests now cover tagless `record_def` member typedef owners,
tagless nested dependent typename owners, and constructor-type probes resolving
through typedef `record_def` before stale rendered map entries.

## Suggested Next

Next coherent packet: continue Step 2 by auditing the remaining parser-local
qualified-owner/type-head probes that still call `has_defined_struct_tag()` with
only rendered names, and convert any call site that can obtain a `TypeSpec`,
`QualifiedNameKey`, or direct `Node*` to a structured-primary check while
leaving TextId-less compatibility fallbacks visible.

## Watchouts

Rendered maps remain required compatibility bridges. `has_defined_struct_tag()`
is still string-only and should not be treated as structured authority; the
next packet should either keep such uses explicitly rendered-name-only or route
through a richer carrier before calling it.

## Proof

Passed:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Proof log: `test_after.log`.
