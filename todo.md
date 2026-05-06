Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C started by splitting parser support record resolution for
constant-layout evaluation from the broader parser-local compatibility bridge.
`sizeof`, `alignof`, and `offsetof` now use direct complete `record_def`
authority for structured records and reject parser tag-map recovery for
TextId/context-only structured record carriers.

## Suggested Next

Continue Step 4C by migrating the next parser record lookup family that still
uses rendered tag maps as semantic authority, keeping parser-local
compatibility bridges out of structured layout decisions.

## Watchouts

`resolve_record_type_spec` still preserves a parser-local compatibility bridge
for non-layout probes and declaration checks; the stricter helper is private to
constant layout. Do not route `sizeof`/`alignof`/`offsetof` back through the
public compatibility bridge for structured carriers.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_support_residual_structured_metadata|frontend_hir_lookup_tests|cpp_eastl_vector_parse_recipe)$' > test_after.log`

Result: passed; `test_after.log` contains the green proof run.
