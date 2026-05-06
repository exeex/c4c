Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Parser Record Lookup Families

# Current Packet

## Just Finished

Completed the Step 4 repair for parser context/TextId record lookup.
`record_definition_in_context_by_text_id` now scans unique record nodes from
`struct_tag_def_map`, preserves duplicate rendered keys for the same structured
record, and rejects same-context/same-TextId different-record ambiguity instead
of choosing by rendered map iteration order.

## Suggested Next

Continue Step 4 by migrating the next parser record lookup family that still
uses compatibility tag storage for structured carriers, keeping ambiguity checks
explicit when a lookup can see multiple rendered aliases.

## Watchouts

`record_definition_in_context_by_text_id` now returns `nullptr` for both misses
and ambiguity; callers that need diagnostics must keep ambiguity local instead
of reintroducing rendered-key ordering.

## Proof

Passed delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_eastl_vector_parse_recipe)$'`.
Proof log: `test_after.log`.
